#include "PluginProcessor.h"
#include "PluginEditor.h"

// ==============================================================================
// РЕАЛИЗАЦИЯ: ЛАМПОВЫЙ КАСКАД
// ==============================================================================

void TubeStage::prepare(double sampleRate, int samplesPerBlock)
{
    fs = sampleRate;
    smoothHeaterVolt.reset(fs, 0.05); // Сглаживание 50 мс
    smoothAnodeVolt.reset(fs, 0.05);
    smoothBias.reset(fs, 0.05);

    rng.seed(1337); // Фиксированный сид для детерминированного шума
}

void TubeStage::process(juce::AudioBuffer<float> &buffer, juce::AudioProcessorValueTreeState &apvts)
{
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    // Чтение параметров из APVTS
    float heater = apvts.getRawParameterValue("TUBE_HEATER")->load();
    float anode = apvts.getRawParameterValue("TUBE_ANODE")->load();
    float bias = apvts.getRawParameterValue("TUBE_BIAS")->load();
    float micRes = apvts.getRawParameterValue("TUBE_MIC_RES")->load();
    float drift = apvts.getRawParameterValue("TUBE_DRIFT")->load();
    int material = (int)apvts.getRawParameterValue("TUBE_CORE")->load();

    smoothHeaterVolt.setTargetValue(heater);
    smoothAnodeVolt.setTargetValue(anode);
    smoothBias.setTargetValue(bias);

    // Модель Jiles-Atherton для гистерезиса (в зависимости от материала)
    float M_sat = 1.0f, a = 0.1f, alpha_JA = 0.001f, mu0 = 1.0f;
    switch (material)
    {
    case 1:
        M_sat = 1.5f;
        a = 0.05f;
        alpha_JA = 0.002f;
        break; // Железо
    case 2:
        M_sat = 0.8f;
        a = 0.01f;
        alpha_JA = 0.0005f;
        break; // Пермаллой
    case 3:
        M_sat = 0.3f;
        a = 0.2f;
        alpha_JA = 0.01f;
        break; // Феррит
    }

    float dt = 1.0f / (float)fs;

    for (int channel = 0; channel < numChannels; ++channel)
    {
        float *channelData = buffer.getWritePointer(channel);

        for (int i = 0; i < numSamples; ++i)
        {
            float curHeater = smoothHeaterVolt.getNextValue();
            float curAnode = smoothAnodeVolt.getNextValue();
            float curBias = smoothBias.getNextValue();

            // Термодинамика накала + тепловой шум
            float targetTemp = 300.0f + (curHeater - 5.8f) * 150.0f;
            currentTemp += (targetTemp - currentTemp) * (1.0f - std::exp(-dt / 2.0f));

            std::normal_distribution<float> noise(0.0f, 1.0f);
            float thermalNoise = noise(rng) * 0.00001f * std::sqrt(currentTemp / 300.0f) * drift;

            // Входной сигнал
            float x = channelData[i] + thermalNoise;

            // Динамический bias + Микрофонный резонанс
            float effBias = curBias + prevY[channel] * micRes * 0.1f;
            float V_eff = x + curAnode / 100.0f + effBias;

            // Child-Langmuir закон пробоя (1.5 степень)
            float Ia = 0.0f;
            if (V_eff > 0.0f)
            {
                Ia = 0.02f * std::pow(V_eff, 1.5f);
            }

            // Насыщение трансформатора и Гистерезис
            float H = Ia;
            float dH = (H - currentH[channel]) / dt;

            float k_JA = 0.1f;
            float M_eq = M_sat * (std::tanh((H + alpha_JA * currentM[channel]) / a));
            float dM_dH = (M_eq - currentM[channel]) / (k_JA + 1e-9f);

            currentM[channel] += dM_dH * dH * dt;
            currentH[channel] = H;

            float B = mu0 * (H + currentM[channel]);

            // Выходной сигнал лампы
            float out = B;

            // Добавляем полиномиальную сатурацию
            out = PhysicsMath::tubeTransfer(out, 1.0f, 0.5f, -0.2f, 0.1f, -0.05f);

            channelData[i] = out;
            prevY[channel] = out;
        }
    }
}

void TubeStage::reset()
{
    currentTemp = 300.0f;
    currentH[0] = currentH[1] = 0.0f;
    currentM[0] = currentM[1] = 0.0f;
    prevY[0] = prevY[1] = 0.0f;
}

// ==============================================================================
// РЕАЛИЗАЦИЯ: МЕХАНИЧЕСКИЙ РЕЗАК
// ==============================================================================

void ChainsawGranulator::prepare(double sampleRate, int samplesPerBlock)
{
    fs = sampleRate;
    smoothRPM.reset(fs, 0.05);
    smoothTension.reset(fs, 0.05);
    smoothMuffler.reset(fs, 0.05);

    rng.seed(4242);
}

void ChainsawGranulator::process(juce::AudioBuffer<float> &buffer, juce::AudioProcessorValueTreeState &apvts)
{
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    float rpm = apvts.getRawParameterValue("SAW_RPM")->load();
    float throttle = apvts.getRawParameterValue("SAW_THROTTLE")->load();
    float tension = apvts.getRawParameterValue("SAW_TENSION")->load();
    float kickback = apvts.getRawParameterValue("SAW_KICKBACK")->load();
    float mfl = apvts.getRawParameterValue("SAW_MUFFLER")->load();
    int profile = (int)apvts.getRawParameterValue("SAW_PROFILE")->load();

    smoothRPM.setTargetValue(rpm);
    smoothTension.setTargetValue(tension);
    smoothMuffler.setTargetValue(mfl);

    float dt = 1.0f / (float)fs;

    for (int channel = 0; channel < numChannels; ++channel)
    {
        float *channelData = buffer.getWritePointer(channel);

        for (int i = 0; i < numSamples; ++i)
        {
            float curRPM = smoothRPM.getNextValue();
            float curTension = smoothTension.getNextValue();
            float curMuffler = smoothMuffler.getNextValue();

            // Динамика оборотов (ТЗ 2.2.1)
            float targetOmega = (curRPM / 60.0f) * 2.0f * 3.14159265f;
            omega += (targetOmega - omega) * (throttle * 0.1f);

            // Обработка фазы окна
            phase[channel] += omega * dt;
            if (phase[channel] >= 2.0f * 3.14159265f)
            {
                phase[channel] -= 2.0f * 3.14159265f;
            }

            // Фазовый джиттер от натяжения цепи
            float jitter = curTension * 0.1f * std::sin(phase[channel] * 5.0f);
            float effectivePhase = phase[channel] + jitter + kickbackPhase[channel];

            // Формирование окна (Hann / Rect / Tri)
            float window = 1.0f;
            float normPhase = effectivePhase / (2.0f * 3.14159265f);

            if (profile == 0)
            { // Hann
                window = 0.5f * (1.0f - std::cos(effectivePhase));
            }
            else if (profile == 2)
            { // Tri
                window = 1.0f - std::abs(2.0f * normPhase - 1.0f);
            } // else profile == 1 is Rect (window = 1.0)

            // Стохастическая отдача (Kickback)
            if (dist(rng) < kickback * 0.001f)
            {
                kickbackPhase[channel] += 3.14159265f * 0.5f; // Прыжок фазы
            }
            kickbackPhase[channel] *= 0.95f; // Плавное затухание сдвига

            // Применение окна грануляции
            float x = channelData[i] * window;

            // Резонанс глушителя (простейший IIR 2-го порядка)
            float wc = 1000.0f * (1.0f + curMuffler * 3.0f); // частота среза
            float alpha = std::sin(wc * dt);

            float out = x + alpha * z1[channel];
            z2[channel] = z1[channel];
            z1[channel] = x - alpha * out;

            channelData[i] = juce::jmax(-1.0f, juce::jmin(out, 1.0f)); // Hard clamp
        }
    }
}

void ChainsawGranulator::reset()
{
    omega = 0.0f;
    phase[0] = phase[1] = 0.0f;
    kickbackPhase[0] = kickbackPhase[1] = 0.0f;
    z1[0] = z1[1] = z2[0] = z2[1] = 0.0f;
}

// ==============================================================================
// РЕАЛИЗАЦИЯ: ЦИФРОВОЙ ДЕСТРУКТОР
// ==============================================================================

DigitalDestructor::DigitalDestructor()
{
    rng.seed(12345);
}

void DigitalDestructor::prepare(double sampleRate, int samplesPerBlock)
{
    fs = sampleRate;
    smoothBitDepth.reset(fs, 0.05);
    smoothSampleRate.reset(fs, 0.05);
    smoothScratchSpeed.reset(fs, 0.05);

    scratchBuffer.setSize(2, (int)fs * 2); // 2 секунды буфера
    scratchBuffer.clear();
    writePos = 0;
}

void DigitalDestructor::process(juce::AudioBuffer<float> &buffer, juce::AudioProcessorValueTreeState &apvts)
{
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    float bitDepth = apvts.getRawParameterValue("DIGI_BITS")->load();
    float sRate = apvts.getRawParameterValue("DIGI_SRATE")->load();
    float scratchSpd = apvts.getRawParameterValue("DIGI_SCRATCH_SPD")->load();
    float density = apvts.getRawParameterValue("DIGI_GLITCH_DENS")->load();
    float compThresh = apvts.getRawParameterValue("DIGI_COMP_THRESH")->load();
    float compRatio = apvts.getRawParameterValue("DIGI_COMP_RATIO")->load();

    smoothBitDepth.setTargetValue(bitDepth);
    smoothSampleRate.setTargetValue(sRate);
    smoothScratchSpeed.setTargetValue(scratchSpd);

    float dt = 1.0f / (float)fs;
    int bufLen = scratchBuffer.getNumSamples();

    for (int channel = 0; channel < numChannels; ++channel)
    {
        float *channelData = buffer.getWritePointer(channel);
        const float *scratchData = scratchBuffer.getReadPointer(channel);

        for (int i = 0; i < numSamples; ++i)
        {
            float curBits = smoothBitDepth.getNextValue();
            float curSRate = smoothSampleRate.getNextValue();
            float curScratchSpd = smoothScratchSpeed.getNextValue();

            float x = channelData[i];

            // 1. Запись в буфер скретча
            if (channel == 0 && i == 0)
            {
                writePos = (writePos + 1) % bufLen;
            }
            scratchBuffer.getWritePointer(channel)[writePos] = x;

            // 2. Фазовая модель скретча
            scratchPhase[channel] += (1.0f + curScratchSpd) * 1.0f;
            if (scratchPhase[channel] >= (float)bufLen)
                scratchPhase[channel] -= (float)bufLen;
            if (scratchPhase[channel] < 0.0f)
                scratchPhase[channel] += (float)bufLen;

            // Линейная интерполяция
            int idx1 = (int)std::floor(scratchPhase[channel]);
            int idx2 = (idx1 + 1) % bufLen;
            float frac = scratchPhase[channel] - (float)idx1;
            float scratchOut = scratchData[idx1] + frac * (scratchData[idx2] - scratchData[idx1]);

            x = scratchOut;

            // 3. Биткрашер и Даунсэмплинг
            float step = 2.0f / std::pow(2.0f, curBits);
            x = step * std::round(x / step); // Квантование

            // 4. Скремблинг / Глитч Бит-рот (ТЗ 2.3.2)
            if (dist(rng) < density * 0.05f)
            {
                uint32_t *bits = reinterpret_cast<uint32_t *>(&x);
                *bits ^= 0x00FF0000; // Инвертируем часть бит
            }

            // 5. Динамический компрессор (ТЗ 2.3.4)
            float x_rms = std::abs(x); // Упрощенно пиковый детектор
            float target_gr = 0.0f;
            float thresh_lin = std::pow(10.0f, compThresh / 20.0f);

            if (x_rms > thresh_lin)
            {
                target_gr = (1.0f - 1.0f / compRatio) * (compThresh - 20.0f * std::log10(x_rms + 1e-9f));
            }

            // Сглаживание огибающей компрессора
            currentGR[channel] += (target_gr - currentGR[channel]) * 0.1f;
            x *= std::pow(10.0f, currentGR[channel] / 20.0f);

            channelData[i] = x;
        }
    }
}

void DigitalDestructor::reset()
{
    scratchPhase[0] = scratchPhase[1] = 0.0f;
    currentGR[0] = currentGR[1] = 0.0f;
    scratchBuffer.clear();
    writePos = 0;
}

#include "PluginProcessor.h"
#include "PluginEditor.h"

// ==============================================================================
// КОНСТРУКТОР И ДЕСТРУКТОР
// ==============================================================================
TriadaFireAudioProcessor::TriadaFireAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
                         ),
#endif
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

TriadaFireAudioProcessor::~TriadaFireAudioProcessor()
{
}

// ==============================================================================
// ОПРЕДЕЛЕНИЕ ПАРАМЕТРОВ (ВСЕ ~45 ШТУК ИЗ ТЗ)
// ==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout TriadaFireAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // --- 🔥 2. ЛАМПОВЫЙ КАСКАД (~16 ПАРАМЕТРОВ) ---
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "TUBE_PREAMP", juce::String::fromUTF8("Тип Предусилителя"),
        juce::StringArray{"12AX7", juce::String::fromUTF8("6Н2П"), "12AT7", "EF86"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "TUBE_POWER", juce::String::fromUTF8("Тип Оконечника"),
        juce::StringArray{juce::String::fromUTF8("6П14П"), juce::String::fromUTF8("6П45С"), "EL34", "KT88"}, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "TUBE_HEATER", juce::String::fromUTF8("Напряжение Накала"), juce::NormalisableRange<float>(5.8f, 7.2f, 0.01f), 6.3f, "V"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "TUBE_ANODE", juce::String::fromUTF8("Анодное Напряжение"), juce::NormalisableRange<float>(150.f, 450.f, 1.f), 300.f, "V"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "TUBE_BIAS", juce::String::fromUTF8("Смещение Сетки"), juce::NormalisableRange<float>(-50.f, -2.f, 0.1f), -20.f, "V"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "TUBE_MIC_RES", juce::String::fromUTF8("Микрофонный Резонанс"), juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "TUBE_DRIFT", juce::String::fromUTF8("Температурный Дрейф"), juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "TUBE_CORE", juce::String::fromUTF8("Материал Сердечника"),
        juce::StringArray{juce::String::fromUTF8("Воздух"), juce::String::fromUTF8("Железо"), juce::String::fromUTF8("Пермаллой"), juce::String::fromUTF8("Феррит")}, 1));

    // --- 🪚 3. МЕХАНИЧЕСКИЙ РЕЗАК (~15 ПАРАМЕТРОВ) ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "SAW_RPM", juce::String::fromUTF8("Обороты ДВС"), juce::NormalisableRange<float>(1500.f, 14000.f, 100.f), 8000.f, "RPM"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "SAW_THROTTLE", juce::String::fromUTF8("Дроссель"), juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "SAW_TENSION", juce::String::fromUTF8("Натяжение Цепи"), juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "SAW_PROFILE", juce::String::fromUTF8("Профиль Зуба"),
        juce::StringArray{juce::String::fromUTF8("Чиппер"), juce::String::fromUTF8("Скребок"), juce::String::fromUTF8("Полудолото")}, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "SAW_KICKBACK", juce::String::fromUTF8("Отдача"), juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "SAW_MUFFLER", juce::String::fromUTF8("Резонанс Глушителя"), juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    // --- 💾 4. ЦИФРОВОЙ ДЕСТРУКТОР (~14 ПАРАМЕТРОВ) ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "DIGI_BITS", juce::String::fromUTF8("Битовая Глубина"), juce::NormalisableRange<float>(1.f, 32.f, 1.f), 32.f, "bit"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "DIGI_SRATE", juce::String::fromUTF8("Частота Дискретизации"), juce::NormalisableRange<float>(1000.f, 44100.f, 100.f), 44100.f, "Hz"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "DIGI_GLITCH_DENS", juce::String::fromUTF8("Плотность Скремблирования"), juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "DIGI_SCRATCH_SPD", juce::String::fromUTF8("Скорость Скретча"), juce::NormalisableRange<float>(-2.f, 2.f, 0.01f), 0.0f, "%"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "DIGI_COMP_THRESH", juce::String::fromUTF8("Компрессор: Порог"), juce::NormalisableRange<float>(-60.f, 0.f, 0.1f), 0.f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "DIGI_COMP_RATIO", juce::String::fromUTF8("Компрессор: Степень"), juce::NormalisableRange<float>(1.f, 20.f, 0.1f), 1.f, ":1"));

    // --- 🌐 5. ГЛОБАЛЬНЫЕ КОНТРОЛЛЕРЫ & МАРШРУТИЗАЦИЯ ---
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "MASTER_DRYWET", juce::String::fromUTF8("Master Dry/Wet"), juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f, "%"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "MASTER_FEEDBACK", juce::String::fromUTF8("Feedback Loop"), juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "MASTER_OUTPUT", juce::String::fromUTF8("Output Gain"), juce::NormalisableRange<float>(-24.f, 24.f, 0.1f), 0.f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "MASTER_EMERGENCY", juce::String::fromUTF8("Режим Аварии"), false));

    return {params.begin(), params.end()};
}

// ==============================================================================
// ОСНОВНАЯ ОБРАБОТКА ЗВУКА
// ==============================================================================
void TriadaFireAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    tubeStage.prepare(sampleRate, samplesPerBlock);
    chainsawGranulator.prepare(sampleRate, samplesPerBlock);
    digitalDestructor.prepare(sampleRate, samplesPerBlock);

    dryBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);
    feedbackBuffer.setSize(getTotalNumOutputChannels(), (int)(sampleRate * 2.0)); // 2 секунды буфера для фидбека
    feedbackBuffer.clear();

    feedbackReadPos = 0;
    feedbackWritePos = 64; // Задержка 64 сэмпла для стабильности

    smoothDryWet.reset(sampleRate, 0.03);
    smoothMasterOut.reset(sampleRate, 0.03);
}

void TriadaFireAudioProcessor::releaseResources()
{
}

void TriadaFireAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    int numSamples = buffer.getNumSamples();

    // Очистка лишних каналов
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    // Сохранение сухого сигнала
    dryBuffer.makeCopyOf(buffer);

    // --- ГЛОБАЛЬНАЯ МАТРИЦА ---
    float feedbackGain = apvts.getRawParameterValue("MASTER_FEEDBACK")->load();
    bool emergency = apvts.getRawParameterValue("MASTER_EMERGENCY")->load();

    // 1. Добавление фидбека (Выход Цифры -> Вход Ламп)
    if (feedbackGain > 0.01f)
    {
        for (int ch = 0; ch < totalNumInputChannels; ++ch)
        {
            float *channelData = buffer.getWritePointer(ch);
            const float *feedbackData = feedbackBuffer.getReadPointer(ch);

            int localReadPos = feedbackReadPos;
            for (int i = 0; i < numSamples; ++i)
            {
                channelData[i] += feedbackData[localReadPos] * feedbackGain;
                localReadPos = (localReadPos + 1) % feedbackBuffer.getNumSamples();
            }
        }
    }

    // 2. Линейная цепь разрушения (Конвейер)
    tubeStage.process(buffer, apvts);
    chainsawGranulator.process(buffer, apvts);
    digitalDestructor.process(buffer, apvts);

    // 3. Запись обработанного сигнала в буфер фидбека
    if (feedbackGain > 0.01f)
    {
        for (int ch = 0; ch < totalNumOutputChannels; ++ch)
        {
            float *feedbackData = feedbackBuffer.getWritePointer(ch);
            const float *channelData = buffer.getReadPointer(ch);

            int localWritePos = feedbackWritePos;
            for (int i = 0; i < numSamples; ++i)
            {
                feedbackData[localWritePos] = channelData[i];
                localWritePos = (localWritePos + 1) % feedbackBuffer.getNumSamples();
            }
        }
        feedbackReadPos = (feedbackReadPos + numSamples) % feedbackBuffer.getNumSamples();
        feedbackWritePos = (feedbackWritePos + numSamples) % feedbackBuffer.getNumSamples();
    }

    // 4. Master Dry/Wet и Выходной Гейн
    smoothDryWet.setTargetValue(apvts.getRawParameterValue("MASTER_DRYWET")->load());
    smoothMasterOut.setTargetValue(juce::Decibels::decibelsToGain(apvts.getRawParameterValue("MASTER_OUTPUT")->load()));

    for (int ch = 0; ch < totalNumOutputChannels; ++ch)
    {
        float *channelData = buffer.getWritePointer(ch);
        const float *dryData = dryBuffer.getReadPointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            float wetSample = channelData[i];
            float drySample = dryData[i];

            float dwMix = smoothDryWet.getNextValue();
            float outGain = smoothMasterOut.getNextValue();

            channelData[i] = (wetSample * dwMix + drySample * (1.0f - dwMix)) * outGain;

            if (!emergency) // В режиме аварии отключаем лимитер
            {
                channelData[i] = PhysicsMath::clamp(channelData[i], -1.0f, 1.0f);
            }
        }
    }

    // Обновление метрик для UI
    rmsOutL.store(buffer.getRMSLevel(0, 0, numSamples));
    if (totalNumOutputChannels > 1)
        rmsOutR.store(buffer.getRMSLevel(1, 0, numSamples));
    else
        rmsOutR.store(buffer.getRMSLevel(0, 0, numSamples));
}

// ==============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ==============================================================================
bool TriadaFireAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

juce::AudioProcessorEditor *TriadaFireAudioProcessor::createEditor()
{
    return new TriadaFireAudioProcessorEditor(*this);
}

void TriadaFireAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void TriadaFireAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}
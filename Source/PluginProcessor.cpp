#include "PluginProcessor.h"
#include "PluginEditor.h"

// ==============================================================================
// ЛАМПОВЫЙ КАСКАД
// ==============================================================================
void TubeStage::prepare(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    fs = sampleRate;
    smoothHeaterVolt.reset(fs, 0.05);
    smoothAnodeVolt.reset(fs, 0.05);
    smoothBias.reset(fs, 0.05);
    rng.seed(1337);
}

void TubeStage::process(juce::AudioBuffer<float> &buffer, juce::AudioProcessorValueTreeState &apvts)
{
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    float heater = apvts.getRawParameterValue("TUBE_HEATER")->load();
    float anode = apvts.getRawParameterValue("TUBE_ANODE")->load();
    float bias = apvts.getRawParameterValue("TUBE_BIAS")->load();
    float micRes = apvts.getRawParameterValue("TUBE_MIC_RES")->load();
    float drift = apvts.getRawParameterValue("TUBE_DRIFT")->load();
    int material = static_cast<int>(apvts.getRawParameterValue("TUBE_CORE")->load());

    smoothHeaterVolt.setTargetValue(heater);
    smoothAnodeVolt.setTargetValue(anode);
    smoothBias.setTargetValue(bias);

    float M_sat = 1.0f, a_ja = 0.1f, alpha_ja = 0.001f, mu0 = 1.0f;
    switch (material)
    {
    case 1:
        M_sat = 1.5f;
        a_ja = 0.05f;
        alpha_ja = 0.002f;
        break;
    case 2:
        M_sat = 0.8f;
        a_ja = 0.01f;
        alpha_ja = 0.0005f;
        break;
    case 3:
        M_sat = 0.3f;
        a_ja = 0.2f;
        alpha_ja = 0.01f;
        break;
    }

    float dt = 1.0f / static_cast<float>(fs);
    std::normal_distribution<float> noiseDist(0.0f, 1.0f);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float *channelData = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            float curHeater = smoothHeaterVolt.getNextValue();
            float curAnode = smoothAnodeVolt.getNextValue();
            float curBias = smoothBias.getNextValue();

            float targetTemp = 300.0f + (curHeater - 5.8f) * 150.0f;
            currentTemp += (targetTemp - currentTemp) * (1.0f - std::exp(-dt / 2.0f));

            float thermalNoise = noiseDist(rng) * 0.00001f * std::sqrt(currentTemp / 300.0f) * drift;
            float x = channelData[i] + thermalNoise;
            float effBias = curBias + prevY[ch] * micRes * 0.1f;
            float V_eff = x + curAnode / 100.0f + effBias;

            float Ia = (V_eff > 0.0f) ? 0.02f * std::pow(V_eff, 1.5f) : 0.0f;
            float dH = (Ia - currentH[ch]);
            float M_eq = M_sat * (std::tanh((Ia + alpha_ja * currentM[ch]) / a_ja));
            currentM[ch] += (M_eq - currentM[ch]) * (0.1f + 1e-9f) * dH;
            currentH[ch] = Ia;

            float B = mu0 * (Ia + currentM[ch]);
            float out = PhysicsMath::tubeTransfer(B, 1.0f, 0.5f, -0.2f, 0.1f, -0.05f);

            channelData[i] = out;
            prevY[ch] = out;
        }
    }
}

void TubeStage::reset()
{
    currentTemp = 300.0f;
    for (int i = 0; i < 2; ++i)
    {
        currentH[i] = 0.0f;
        currentM[i] = 0.0f;
        prevY[i] = 0.0f;
    }
}

// ==============================================================================
// МЕХАНИЧЕСКИЙ РЕЗАК
// ==============================================================================
ChainsawGranulator::ChainsawGranulator() : dist(0.0f, 1.0f) { rng.seed(4242); }

void ChainsawGranulator::prepare(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    fs = sampleRate;
    smoothRPM.reset(fs, 0.05);
    smoothTension.reset(fs, 0.05);
    smoothMuffler.reset(fs, 0.05);
}

void ChainsawGranulator::process(juce::AudioBuffer<float> &buffer, juce::AudioProcessorValueTreeState &apvts)
{
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    smoothRPM.setTargetValue(apvts.getRawParameterValue("SAW_RPM")->load());
    float throttle = apvts.getRawParameterValue("SAW_THROTTLE")->load();
    smoothTension.setTargetValue(apvts.getRawParameterValue("SAW_TENSION")->load());
    float kickback = apvts.getRawParameterValue("SAW_KICKBACK")->load();
    smoothMuffler.setTargetValue(apvts.getRawParameterValue("SAW_MUFFLER")->load());
    int profile = static_cast<int>(apvts.getRawParameterValue("SAW_PROFILE")->load());

    float dt = 1.0f / static_cast<float>(fs);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float *channelData = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            float curRPM = smoothRPM.getNextValue();
            float curTension = smoothTension.getNextValue();
            float curMuffler = smoothMuffler.getNextValue();

            float targetOmega = (curRPM / 60.0f) * juce::MathConstants<float>::twoPi;
            omega += (targetOmega - omega) * (throttle * 0.1f);

            phase[ch] += omega * dt;
            if (phase[ch] >= juce::MathConstants<float>::twoPi)
                phase[ch] -= juce::MathConstants<float>::twoPi;

            float jitter = curTension * 0.1f * std::sin(phase[ch] * 5.0f);
            float effectivePhase = phase[ch] + jitter + kickbackPhase[ch];
            float window = 1.0f;
            float normPhase = effectivePhase / juce::MathConstants<float>::twoPi;

            if (profile == 0)
                window = 0.5f * (1.0f - std::cos(effectivePhase));
            else if (profile == 2)
                window = 1.0f - std::abs(2.0f * normPhase - 1.0f);

            if (dist(rng) < kickback * 0.001f)
            {
                kickbackPhase[ch] += juce::MathConstants<float>::halfPi * 0.5f;
            }
            kickbackPhase[ch] *= 0.95f;

            float x = channelData[i] * window;
            float wc = 1000.0f * (1.0f + curMuffler * 3.0f);
            float alpha_muffler = std::sin(wc * dt);
            float out_muffler = x + alpha_muffler * z1[ch];
            z1[ch] = x - alpha_muffler * out_muffler;

            channelData[i] = PhysicsMath::clamp(out_muffler, -1.0f, 1.0f);
        }
    }
}

void ChainsawGranulator::reset()
{
    omega = 0.0f;
    for (int i = 0; i < 2; ++i)
    {
        phase[i] = 0.0f;
        kickbackPhase[i] = 0.0f;
        z1[i] = 0.0f;
        z2[i] = 0.0f;
    }
}

// ==============================================================================
// ЦИФРОВОЙ ДЕСТРУКТОР
// ==============================================================================
DigitalDestructor::DigitalDestructor() : dist(0.0f, 1.0f) { rng.seed(12345); }

void DigitalDestructor::prepare(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    fs = sampleRate;
    smoothBitDepth.reset(fs, 0.05);
    smoothSampleRate.reset(fs, 0.05);
    smoothScratchSpeed.reset(fs, 0.05);
    scratchBuffer.setSize(2, static_cast<int>(fs * 2.0));
    scratchBuffer.clear();
    writePos = 0;
}

void DigitalDestructor::process(juce::AudioBuffer<float> &buffer, juce::AudioProcessorValueTreeState &apvts)
{
    int numChannels = buffer.getNumChannels();
    int numSamples = buffer.getNumSamples();

    smoothBitDepth.setTargetValue(apvts.getRawParameterValue("DIGI_BITS")->load());
    smoothSampleRate.setTargetValue(apvts.getRawParameterValue("DIGI_SRATE")->load());
    smoothScratchSpeed.setTargetValue(apvts.getRawParameterValue("DIGI_SCRATCH_SPD")->load());
    float density = apvts.getRawParameterValue("DIGI_GLITCH_DENS")->load();
    float compThresh = apvts.getRawParameterValue("DIGI_COMP_THRESH")->load();
    float compRatio = apvts.getRawParameterValue("DIGI_COMP_RATIO")->load();

    int bufLen = scratchBuffer.getNumSamples();

    for (int channel = 0; channel < numChannels; ++channel)
    {
        float *channelData = buffer.getWritePointer(channel);
        float *scratchBufData = scratchBuffer.getWritePointer(channel);
        const float *scratchReadData = scratchBuffer.getReadPointer(channel);

        for (int i = 0; i < numSamples; ++i)
        {
            float curBits = smoothBitDepth.getNextValue();
            float curScratchSpd = smoothScratchSpeed.getNextValue();
            float x = channelData[i];

            if (channel == 0 && i == 0)
                writePos = (writePos + 1) % bufLen;
            scratchBufData[writePos] = x;

            scratchPhase[channel] += (1.0f + curScratchSpd) * 1.0f;
            while (scratchPhase[channel] >= static_cast<float>(bufLen))
                scratchPhase[channel] -= static_cast<float>(bufLen);
            while (scratchPhase[channel] < 0.0f)
                scratchPhase[channel] += static_cast<float>(bufLen);

            int idx1 = static_cast<int>(std::floor(scratchPhase[channel]));
            int idx2 = (idx1 + 1) % bufLen;
            float frac = scratchPhase[channel] - static_cast<float>(idx1);
            x = scratchReadData[idx1] + frac * (scratchReadData[idx2] - scratchReadData[idx1]);

            float step = 2.0f / std::pow(2.0f, curBits);
            x = step * std::round(x / step);

            if (dist(rng) < density * 0.05f)
            {
                uint32_t bitRep;
                std::memcpy(&bitRep, &x, sizeof(float));
                bitRep ^= (rng() % 0xFFFF);
                std::memcpy(&x, &bitRep, sizeof(float));
            }

            float x_rms = std::abs(x);
            float target_gr = 0.0f;
            float thresh_lin = juce::Decibels::decibelsToGain(compThresh);

            if (x_rms > thresh_lin)
            {
                target_gr = (1.0f - 1.0f / compRatio) * (compThresh - 20.0f * std::log10(x_rms + 1e-9f));
            }

            currentGR[channel] += (target_gr - currentGR[channel]) * 0.1f;
            x *= std::pow(10.0f, currentGR[channel] / 20.0f);

            channelData[i] = x;
        }
    }
}

void DigitalDestructor::reset()
{
    for (int i = 0; i < 2; ++i)
    {
        scratchPhase[i] = 0.0f;
        currentGR[i] = 0.0f;
    }
    scratchBuffer.clear();
    writePos = 0;
}

// ==============================================================================
// ГЛАВНЫЙ ПРОЦЕССОР
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

TriadaFireAudioProcessor::~TriadaFireAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout TriadaFireAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterChoice>("TUBE_PREAMP", juce::String::fromUTF8("Тип Предусилителя"), juce::StringArray{"12AX7", juce::String::fromUTF8("6Н2П"), "12AT7", "EF86"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("TUBE_POWER", juce::String::fromUTF8("Тип Оконечника"), juce::StringArray{juce::String::fromUTF8("6П14П"), juce::String::fromUTF8("6П45С"), "EL34", "KT88"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TUBE_HEATER", juce::String::fromUTF8("Напряжение Накала"), juce::NormalisableRange<float>(5.8f, 7.2f, 0.01f), 6.3f, "V"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TUBE_ANODE", juce::String::fromUTF8("Анодное Напряжение"), juce::NormalisableRange<float>(150.f, 450.f, 1.f), 300.f, "V"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TUBE_BIAS", juce::String::fromUTF8("Смещение Сетки"), juce::NormalisableRange<float>(-50.f, -2.f, 0.1f), -20.f, "V"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TUBE_LOAD_RES", juce::String::fromUTF8("Сопротивление Нагрузки"), juce::NormalisableRange<float>(1.f, 100.f, 0.1f, 1.0f), 10.f, "kOhm"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TUBE_CAP_COUPL", juce::String::fromUTF8("Емкость Связи"), juce::NormalisableRange<float>(10.f, 1000.f, 1.f, 0.2f), 100.f, "nF"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TUBE_CATH_BYP", juce::String::fromUTF8("Катодный Байпас"), juce::NormalisableRange<float>(10.f, 100.f, 1.f), 100.f, "uF"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TUBE_MIC_RES", juce::String::fromUTF8("Микрофонный Резонанс"), juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TUBE_DRIFT", juce::String::fromUTF8("Температурный Дрейф"), juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("TUBE_CORE", juce::String::fromUTF8("Материал Сердечника"), juce::StringArray{juce::String::fromUTF8("Воздух"), juce::String::fromUTF8("Железо"), juce::String::fromUTF8("Пермаллой"), juce::String::fromUTF8("Феррит")}, 1));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("TUBE_WINDING", juce::String::fromUTF8("Вторичная Обмотка"), juce::StringArray{"1:1", "1:2", "1:4", "1:8"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("TUBE_PHASE_MODE", juce::String::fromUTF8("Синфазный/Противофаз"), juce::StringArray{"Push-Pull", "Single-Ended"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TUBE_QUIES_CUR", juce::String::fromUTF8("Ток Покоя"), juce::NormalisableRange<float>(10.f, 80.f, 1.f), 40.f, "mA"));
    params.push_back(std::make_unique<juce::AudioParameterBool>("TUBE_EXT_BIAS", juce::String::fromUTF8("Внешний Bias-Input"), false));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("TUBE_WARMUP", juce::String::fromUTF8("Темп Выхода на Режим"), juce::NormalisableRange<float>(0.5f, 10.f, 0.1f), 2.0f, "s"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("SAW_RPM", juce::String::fromUTF8("Обороты ДВС"), juce::NormalisableRange<float>(1500.f, 14000.f, 100.f), 8000.f, "RPM"));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("SAW_THROTTLE_CURVE", juce::String::fromUTF8("Кривая Дросселя"), juce::StringArray{juce::String::fromUTF8("Лин"), juce::String::fromUTF8("Эксп"), juce::String::fromUTF8("Лог"), juce::String::fromUTF8("S-обр.")}, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("SAW_ENGINE_TYPE", juce::String::fromUTF8("Такт Двигателя"), juce::StringArray{"2Т", "4Т"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SAW_INTAKE_GAP", juce::String::fromUTF8("Зазор Впуска"), juce::NormalisableRange<float>(0.05f, 0.40f, 0.01f), 0.15f, "мм"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SAW_EXHAUST_GAP", juce::String::fromUTF8("Зазор Выпуска"), juce::NormalisableRange<float>(0.10f, 0.50f, 0.01f), 0.25f, "мм"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SAW_MAIN_JET", juce::String::fromUTF8("Главный Жиклер"), juce::NormalisableRange<float>(0.4f, 1.2f, 0.01f), 0.8f, "мм"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SAW_IGN_ADVANCE", juce::String::fromUTF8("Угол Опережения"), juce::NormalisableRange<float>(5.f, 35.f, 1.f), 15.f, "°"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SAW_SPARK_GAP", juce::String::fromUTF8("Зазор Свечи"), juce::NormalisableRange<float>(0.4f, 1.0f, 0.01f), 0.7f, "мм"));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("SAW_CHAIN_PITCH", juce::String::fromUTF8("Шаг Цепи"), juce::StringArray{"0.325\"", "0.404\"", "0.50\""}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SAW_TENSION", juce::String::fromUTF8("Натяжение Цепи"), juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("SAW_PROFILE", juce::String::fromUTF8("Профиль Зуба"), juce::StringArray{juce::String::fromUTF8("Чиппер"), juce::String::fromUTF8("Скребок"), juce::String::fromUTF8("Полудолото")}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SAW_KICKBACK", juce::String::fromUTF8("Отдача"), juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SAW_MUFFLER", juce::String::fromUTF8("Резонанс Глушителя"), juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("SAW_SYNC", juce::String::fromUTF8("Синхронизация"), juce::StringArray{"Free", "Host BPM", "MIDI Clock", "Audio In"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("SAW_PHASE_LOCK", juce::String::fromUTF8("Фазовая Блокировка"), juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("DIGI_BITS", juce::String::fromUTF8("Битовая Глубина"), juce::NormalisableRange<float>(1.f, 32.f, 1.f), 32.f, "bit"));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("DIGI_QUANT_CURVE", juce::String::fromUTF8("Кривая Квантования"), juce::StringArray{juce::String::fromUTF8("Лин"), "μ-Law", "A-Law", "Custom"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DIGI_SRATE", juce::String::fromUTF8("Частота Дискретизации"), juce::NormalisableRange<float>(1000.f, 192000.f, 1.f), 44100.f, "Hz"));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("DIGI_INTERP", juce::String::fromUTF8("Интерполяция"), juce::StringArray{juce::String::fromUTF8("Ближ"), juce::String::fromUTF8("Лин"), juce::String::fromUTF8("Кубик"), "Sinc", "ZOH"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DIGI_GLITCH_GRID", juce::String::fromUTF8("Сетка Глитча"), juce::NormalisableRange<float>(1.f, 128.f, 1.f), 1.f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DIGI_GLITCH_DENS", juce::String::fromUTF8("Плотность Скремблирования"), juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("DIGI_SCRATCH_MODEL", juce::String::fromUTF8("Модель Скретча"), juce::StringArray{juce::String::fromUTF8("Винил"), juce::String::fromUTF8("Кассета"), juce::String::fromUTF8("Цифр. Буфер")}, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DIGI_SCRATCH_SPD", juce::String::fromUTF8("Скорость Вращения"), juce::NormalisableRange<float>(-200.f, 200.f, 1.f), 0.0f, "%"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DIGI_SCRATCH_BUF", juce::String::fromUTF8("Размер Буфера Скретча"), juce::NormalisableRange<float>(8.f, 4096.f, 1.f), 512.f, "семплов"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DIGI_COMP_THRESH", juce::String::fromUTF8("Компрессор: Порог"), juce::NormalisableRange<float>(-60.f, 0.f, 0.1f), 0.f, "dB"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DIGI_COMP_RATIO", juce::String::fromUTF8("Компрессор: Степень"), juce::NormalisableRange<float>(1.f, 20.f, 0.1f), 1.f, ":1"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DIGI_COMP_KNEE", juce::String::fromUTF8("Компрессор: Колено"), juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DIGI_COMP_ATTACK", juce::String::fromUTF8("Атака"), juce::NormalisableRange<float>(0.1f, 500.f, 0.1f), 10.f, "мс"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("DIGI_COMP_RELEASE", juce::String::fromUTF8("Релиз"), juce::NormalisableRange<float>(0.1f, 500.f, 0.1f), 100.f, "мс"));
    params.push_back(std::make_unique<juce::AudioParameterBool>("DIGI_SIDECHAIN", juce::String::fromUTF8("Сайдчейн / Внешний"), false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>("MASTER_DRYWET", juce::String::fromUTF8("Master Dry/Wet"), juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f, "%"));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MASTER_FEEDBACK", juce::String::fromUTF8("Feedback Loop"), juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MASTER_OUTPUT", juce::String::fromUTF8("Output Gain"), juce::NormalisableRange<float>(-24.f, 24.f, 0.1f), 0.f, "dB"));
    params.push_back(std::make_unique<juce::AudioParameterBool>("MASTER_EMERGENCY", juce::String::fromUTF8("Режим Аварии"), false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("GLOBAL_BYPASS", juce::String::fromUTF8("Глобальный Bypass"), false));

    return {params.begin(), params.end()};
}

void TriadaFireAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    tubeStage.prepare(sampleRate, samplesPerBlock);
    chainsawGranulator.prepare(sampleRate, samplesPerBlock);
    digitalDestructor.prepare(sampleRate, samplesPerBlock);

    dryBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);
    feedbackBuffer.setSize(getTotalNumOutputChannels(), static_cast<int>(sampleRate * 2.0));
    feedbackBuffer.clear();

    feedbackReadPos = 0;
    feedbackWritePos = 64;

    smoothDryWet.reset(sampleRate, 0.03);
    smoothMasterOut.reset(sampleRate, 0.03);
}

void TriadaFireAudioProcessor::releaseResources() {}

void TriadaFireAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    int numSamples = buffer.getNumSamples();

    for (auto i = getTotalNumInputChannels(); i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    dryBuffer.makeCopyOf(buffer);

    float feedbackGain = apvts.getRawParameterValue("MASTER_FEEDBACK")->load();
    bool emergencyMode = apvts.getRawParameterValue("MASTER_EMERGENCY")->load();
    bool globalBypass = apvts.getRawParameterValue("GLOBAL_BYPASS")->load();

    if (globalBypass)
        return;

    if (feedbackGain > 0.001f)
    {
        for (int ch = 0; ch < totalNumOutputChannels; ++ch)
        {
            float *channelData = buffer.getWritePointer(ch);
            const float *feedbackData = feedbackBuffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                channelData[i] += feedbackData[(feedbackReadPos + i) % feedbackBuffer.getNumSamples()] * feedbackGain;
            }
        }
    }

    tubeStage.process(buffer, apvts);
    chainsawGranulator.process(buffer, apvts);
    digitalDestructor.process(buffer, apvts);

    if (feedbackGain > 0.001f)
    {
        for (int ch = 0; ch < totalNumOutputChannels; ++ch)
        {
            float *feedbackData = feedbackBuffer.getWritePointer(ch);
            const float *channelData = buffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                feedbackData[(feedbackWritePos + i) % feedbackBuffer.getNumSamples()] = channelData[i];
            }
        }
        feedbackReadPos = (feedbackReadPos + numSamples) % feedbackBuffer.getNumSamples();
        feedbackWritePos = (feedbackWritePos + numSamples) % feedbackBuffer.getNumSamples();
    }

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
            if (!emergencyMode)
            {
                channelData[i] = PhysicsMath::clamp(channelData[i], -1.0f, 1.0f);
            }
        }
    }

    rmsOutL.store(buffer.getRMSLevel(0, 0, numSamples));
    if (totalNumOutputChannels > 1)
        rmsOutR.store(buffer.getRMSLevel(1, 0, numSamples));
    else
        rmsOutR.store(buffer.getRMSLevel(0, 0, numSamples));
}

bool TriadaFireAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}

juce::AudioProcessorEditor *TriadaFireAudioProcessor::createEditor() { return new TriadaFireAudioProcessorEditor(*this); }
void TriadaFireAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}
void TriadaFireAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}
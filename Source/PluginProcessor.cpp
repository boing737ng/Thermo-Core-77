#include "PluginProcessor.h"
#include "PluginEditor.h"

ThermoCore77AudioProcessor::ThermoCore77AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
                         ),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
    // Инициализация модулей
    modules.push_back(std::make_unique<TubeModule>());           // ID 0
    modules.push_back(std::make_unique<ChainsawModule>());       // ID 1
    modules.push_back(std::make_unique<ReactorModule>());        // ID 2
    modules.push_back(std::make_unique<SpaceModule>());          // ID 3
    modules.push_back(std::make_unique<DigitalArchiveModule>()); // ID 4

    // Порядок по умолчанию
    for (int i = 0; i < 5; ++i)
        routingOrder.push_back(i);
}

// Громадный макет параметров с использованием кириллицы
juce::AudioProcessorValueTreeState::ParameterLayout ThermoCore77AudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // --- ЛАМПА ---
    auto tubeGroup = std::make_unique<juce::AudioProcessorParameterGroup>("tube", juce::String::fromUTF8("1. ЛАМПА"), "|");
    tubeGroup->addChild(std::make_unique<juce::AudioParameterInt>("tubeType", juce::String::fromUTF8("Тип Лампы"), 0, 3, 0));
    tubeGroup->addChild(std::make_unique<juce::AudioParameterFloat>("heaterV", juce::String::fromUTF8("Накал (В)"), 5.8f, 7.2f, 6.3f));
    tubeGroup->addChild(std::make_unique<juce::AudioParameterFloat>("anodeV", juce::String::fromUTF8("Анод (В)"), 150.0f, 450.0f, 250.0f));
    tubeGroup->addChild(std::make_unique<juce::AudioParameterFloat>("gridBias", juce::String::fromUTF8("Смещение (В)"), -50.0f, -2.0f, -12.0f));
    tubeGroup->addChild(std::make_unique<juce::AudioParameterFloat>("loadRes", juce::String::fromUTF8("Нагрузка (кОм)"), 1.0f, 100.0f, 47.0f));
    tubeGroup->addChild(std::make_unique<juce::AudioParameterFloat>("thermalDrift", juce::String::fromUTF8("Дрейф"), 0.0f, 1.0f, 0.1f));
    tubeGroup->addChild(std::make_unique<juce::AudioParameterFloat>("microphonic", juce::String::fromUTF8("Микрофон"), 0.0f, 1.0f, 0.0f));
    layout.add(std::move(tubeGroup));

    // --- МЕХАНИКА ---
    auto mechGroup = std::make_unique<juce::AudioProcessorParameterGroup>("mech", juce::String::fromUTF8("2. МЕХАНИКА"), "|");
    mechGroup->addChild(std::make_unique<juce::AudioParameterFloat>("rpm", juce::String::fromUTF8("Обороты ДВС"), 1500.0f, 14000.0f, 3000.0f));
    mechGroup->addChild(std::make_unique<juce::AudioParameterBool>("is4T", juce::String::fromUTF8("Цикл 4Т"), false));
    mechGroup->addChild(std::make_unique<juce::AudioParameterFloat>("intakeGap", juce::String::fromUTF8("Впуск"), 0.05f, 0.4f, 0.15f));
    mechGroup->addChild(std::make_unique<juce::AudioParameterFloat>("exhaustGap", juce::String::fromUTF8("Выпуск"), 0.1f, 0.5f, 0.25f));
    mechGroup->addChild(std::make_unique<juce::AudioParameterFloat>("fuelJet", juce::String::fromUTF8("Жиклер"), 0.4f, 1.2f, 0.8f));
    mechGroup->addChild(std::make_unique<juce::AudioParameterFloat>("sparkGap", juce::String::fromUTF8("Зазор Свечи"), 0.4f, 1.0f, 0.6f));
    mechGroup->addChild(std::make_unique<juce::AudioParameterFloat>("tension", juce::String::fromUTF8("Натяжение"), 0.0f, 1.0f, 0.8f));
    layout.add(std::move(mechGroup));

    // --- РЕАКТОР ---
    auto reactorGroup = std::make_unique<juce::AudioProcessorParameterGroup>("reactor", juce::String::fromUTF8("3. РЕАКТОР"), "|");
    reactorGroup->addChild(std::make_unique<juce::AudioParameterFloat>("rodPos", juce::String::fromUTF8("Стержни %"), 0.0f, 1.0f, 0.5f));
    reactorGroup->addChild(std::make_unique<juce::AudioParameterFloat>("rodInSpeed", juce::String::fromUTF8("Ск. Погруж."), 0.1f, 100.0f, 10.0f));
    reactorGroup->addChild(std::make_unique<juce::AudioParameterFloat>("rodOutSpeed", juce::String::fromUTF8("Ск. Извлеч."), 0.1f, 80.0f, 10.0f));
    reactorGroup->addChild(std::make_unique<juce::AudioParameterFloat>("rodInertia", juce::String::fromUTF8("Инерция"), 0.0f, 1.0f, 0.3f));
    reactorGroup->addChild(std::make_unique<juce::AudioParameterFloat>("coolant", juce::String::fromUTF8("Охлаждение"), 0.0f, 1.0f, 0.5f));
    reactorGroup->addChild(std::make_unique<juce::AudioParameterBool>("safety", juce::String::fromUTF8("Контейнмент"), true));
    layout.add(std::move(reactorGroup));

    // --- КОСМОС ---
    auto spaceGroup = std::make_unique<juce::AudioProcessorParameterGroup>("space", juce::String::fromUTF8("4. КОСМОС"), "|");
    spaceGroup->addChild(std::make_unique<juce::AudioParameterFloat>("lensMass", juce::String::fromUTF8("Масса Линзы"), 0.0f, 10.0f, 1.0f));
    spaceGroup->addChild(std::make_unique<juce::AudioParameterFloat>("horizon", juce::String::fromUTF8("Горизонт"), 0.0f, 1.0f, 0.0f));
    spaceGroup->addChild(std::make_unique<juce::AudioParameterFloat>("eccentricity", juce::String::fromUTF8("Орбита"), 0.0f, 0.9f, 0.2f));
    spaceGroup->addChild(std::make_unique<juce::AudioParameterFloat>("vacuum", juce::String::fromUTF8("Вакуум"), 0.0f, 1.0f, 0.1f));
    layout.add(std::move(spaceGroup));

    // --- ЦИФРА ---
    auto digiGroup = std::make_unique<juce::AudioProcessorParameterGroup>("digital", juce::String::fromUTF8("5. ЦИФРА"), "|");
    digiGroup->addChild(std::make_unique<juce::AudioParameterInt>("blockSize", juce::String::fromUTF8("Блок"), 8, 32, 16));
    digiGroup->addChild(std::make_unique<juce::AudioParameterFloat>("quality", juce::String::fromUTF8("Качество"), 1.0f, 100.0f, 85.0f));
    digiGroup->addChild(std::make_unique<juce::AudioParameterFloat>("glitch", juce::String::fromUTF8("Глитч"), 0.0f, 1.0f, 0.05f));
    digiGroup->addChild(std::make_unique<juce::AudioParameterFloat>("turbine", juce::String::fromUTF8("Турбина"), 0.0f, 24000.0f, 12000.0f));
    layout.add(std::move(digiGroup));

    // Глобальные
    layout.add(std::make_unique<juce::AudioParameterFloat>("masterMix", juce::String::fromUTF8("СМЕСЬ"), 0.0f, 1.0f, 1.0f));

    return layout;
}

void ThermoCore77AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = uint32(samplesPerBlock);
    spec.numChannels = uint32(getTotalNumOutputChannels());

    for (auto &module : modules)
        module->prepare(spec);

    // Сглаживание глобальных параметров
    masterMix.reset(sampleRate, 0.05);
}

void ThermoCore77AudioProcessor::releaseResources() {}

void ThermoCore77AudioProcessor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Очистка лишних выходов
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // 1. ОБНОВЛЕНИЕ ПАРАМЕТРОВ (Control Rate)
    updateModuleParameters();

    // 2. МАРШРУТИЗАЦИЯ И ОБРАБОТКА
    // Проходим по списку маршрутизации и вызываем process для каждого модуля
    for (int moduleId : routingOrder)
    {
        if (moduleId >= 0 && moduleId < (int)modules.size())
        {
            modules[moduleId]->process(buffer);
        }
    }

    // 3. ФИНАЛЬНЫЙ МИКС И ЗАЩИТА
    float mixVal = *apvts.getRawParameterValue("masterMix");
    masterMix.setTargetValue(mixVal);

    // Мягкий лимитер на выходе (безопасность)
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float *data = buffer.getWritePointer(ch);
        for (int s = 0; s < buffer.getNumSamples(); ++s)
        {
            // Проверка на NaN/Inf
            if (std::isnan(data[s]) || std::isinf(data[s]))
                data[s] = 0.0f;

            // Софт-клиппинг
            data[s] = std::tanh(data[s]);
        }
    }
}

void ThermoCore77AudioProcessor::updateModuleParameters()
{
    // Лампа
    auto *tube = static_cast<TubeModule *>(modules[0].get());
    TubeModule::Params tp;
    tp.type = (TubeModule::TubeType)(int)*apvts.getRawParameterValue("tubeType");
    tp.heaterVoltage = *apvts.getRawParameterValue("heaterV");
    tp.anodeVoltage = *apvts.getRawParameterValue("anodeV");
    tp.gridBias = *apvts.getRawParameterValue("gridBias");
    tp.loadResistor = *apvts.getRawParameterValue("loadRes");
    tp.thermalDrift = *apvts.getRawParameterValue("thermalDrift");
    tp.microphonicEff = *apvts.getRawParameterValue("microphonic");
    tube->updateParams(tp);

    // Механика
    auto *mech = static_cast<ChainsawModule *>(modules[1].get());
    ChainsawModule::Params mp;
    mp.rpm = *apvts.getRawParameterValue("rpm");
    mp.isFourStroke = *apvts.getRawParameterValue("is4T");
    mp.intakeGap = *apvts.getRawParameterValue("intakeGap");
    mp.exhaustGap = *apvts.getRawParameterValue("exhaustGap");
    mp.fuelJetSize = *apvts.getRawParameterValue("fuelJet");
    mp.sparkGap = *apvts.getRawParameterValue("sparkGap");
    mp.tension = *apvts.getRawParameterValue("tension");
    mech->updateParams(mp);

    // Реактор
    auto *reactor = static_cast<ReactorModule *>(modules[2].get());
    ReactorModule::Params rp;
    rp.rodPosition = *apvts.getRawParameterValue("rodPos");
    rp.rodSpeedIn = *apvts.getRawParameterValue("rodInSpeed");
    rp.rodSpeedOut = *apvts.getRawParameterValue("rodOutSpeed");
    rp.rodInertia = *apvts.getRawParameterValue("rodInertia");
    rp.coolantType = *apvts.getRawParameterValue("coolant");
    rp.safetyEnabled = *apvts.getRawParameterValue("safety");
    reactor->updateParams(rp);

    // Космос
    auto *space = static_cast<SpaceModule *>(modules[3].get());
    SpaceModule::Params sp;
    sp.lensMass = *apvts.getRawParameterValue("lensMass");
    sp.eventHorizon = *apvts.getRawParameterValue("horizon");
    sp.eccentricity = *apvts.getRawParameterValue("eccentricity");
    sp.vacuumPressure = *apvts.getRawParameterValue("vacuum");
    sp.keplerResonance = 1.0f; // Можно добавить в APVTS
    space->updateParams(sp);

    // Цифра
    auto *digi = static_cast<DigitalArchiveModule *>(modules[4].get());
    DigitalArchiveModule::Params dp;
    dp.blockSize = *apvts.getRawParameterValue("blockSize");
    dp.quality = *apvts.getRawParameterValue("quality");
    dp.glitchDensity = *apvts.getRawParameterValue("glitch");
    dp.turbineSpeed = *apvts.getRawParameterValue("turbine");
    digi->updateParams(dp);
}

// Сохранение и загрузка состояния
void ThermoCore77AudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ThermoCore77AudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// Фабрика процессора
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new ThermoCore77AudioProcessor();
}
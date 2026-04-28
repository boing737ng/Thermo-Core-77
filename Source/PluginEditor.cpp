#include "PluginProcessor.h"
#include "PluginEditor.h"

// ==============================================================================
// КОНСТРУКТОР
// ==============================================================================
TriadaFireAudioProcessorEditor::TriadaFireAudioProcessorEditor(TriadaFireAudioProcessor &p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setLookAndFeel(&lookAndFeel);

    // --- Инициализация и настройка компонентов ---

    // Модуль 1: Лампа
    for (auto *slider : {&tubeHeaterSlider, &tubeAnodeSlider, &tubeBiasSlider, &tubeMicResSlider, &tubeDriftSlider})
    {
        slider->setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slider->setTextBoxStyle(juce::Slider::TextBoxBelow, true, 80, 20);
        addAndMakeVisible(slider);
    }
    for (auto *combo : {&tubePreampChoice, &tubePowerChoice, &tubeCoreChoice})
    {
        combo->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(combo);
    }

    tubeHeaterLabel.setText(juce::String::fromUTF8("НАКАЛ"), juce::dontSendNotification);
    tubeAnodeLabel.setText(juce::String::fromUTF8("АНОД"), juce::dontSendNotification);
    tubeBiasLabel.setText(juce::String::fromUTF8("СМЕЩЕНИЕ"), juce::dontSendNotification);
    tubeMicResLabel.setText(juce::String::fromUTF8("РЕЗОНАНС"), juce::dontSendNotification);
    tubeDriftLabel.setText(juce::String::fromUTF8("ДРЕЙФ"), juce::dontSendNotification);
    for (auto *label : {&tubeHeaterLabel, &tubeAnodeLabel, &tubeBiasLabel, &tubeMicResLabel, &tubeDriftLabel})
    {
        label->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    }

    // Модуль 2: Пила
    for (auto *slider : {&sawRpmSlider, &sawThrottleSlider, &sawTensionSlider, &sawKickbackSlider, &sawMufflerSlider})
    {
        slider->setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slider->setTextBoxStyle(juce::Slider::TextBoxBelow, true, 80, 20);
        addAndMakeVisible(slider);
    }
    sawProfileChoice.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(&sawProfileChoice);

    sawRpmLabel.setText(juce::String::fromUTF8("ОБОРОТЫ"), juce::dontSendNotification);
    sawThrottleLabel.setText(juce::String::fromUTF8("ДРОССЕЛЬ"), juce::dontSendNotification);
    sawTensionLabel.setText(juce::String::fromUTF8("НАТЯЖЕНИЕ"), juce::dontSendNotification);
    sawKickbackLabel.setText(juce::String::fromUTF8("ОТДАЧА"), juce::dontSendNotification);
    sawMufflerLabel.setText(juce::String::fromUTF8("ГЛУШИТЕЛЬ"), juce::dontSendNotification);
    for (auto *label : {&sawRpmLabel, &sawThrottleLabel, &sawTensionLabel, &sawKickbackLabel, &sawMufflerLabel})
    {
        label->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    }

    // Модуль 3: Цифра
    for (auto *slider : {&digiBitsSlider, &digiSrateSlider, &digiGlitchSlider, &digiScratchSlider, &digiCompThreshSlider, &digiCompRatioSlider})
    {
        slider->setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slider->setTextBoxStyle(juce::Slider::TextBoxBelow, true, 80, 20);
        addAndMakeVisible(slider);
    }
    digiBitsLabel.setText(juce::String::fromUTF8("БИТЫ"), juce::dontSendNotification);
    digiSrateLabel.setText(juce::String::fromUTF8("ДИСКРЕТ."), juce::dontSendNotification);
    digiGlitchLabel.setText(juce::String::fromUTF8("ГЛИТЧ"), juce::dontSendNotification);
    digiScratchLabel.setText(juce::String::fromUTF8("СКРЕТЧ"), juce::dontSendNotification);
    digiCompThreshLabel.setText(juce::String::fromUTF8("ПОРОГ"), juce::dontSendNotification);
    digiCompRatioLabel.setText(juce::String::fromUTF8("СТЕПЕНЬ"), juce::dontSendNotification);
    for (auto *label : {&digiBitsLabel, &digiSrateLabel, &digiGlitchLabel, &digiScratchLabel, &digiCompThreshLabel, &digiCompRatioLabel})
    {
        label->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    }

    // Модуль 4: Мастер
    for (auto *slider : {&masterDryWetSlider, &masterFeedbackSlider, &masterOutputSlider})
    {
        slider->setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slider->setTextBoxStyle(juce::Slider::TextBoxBelow, true, 80, 20);
        addAndMakeVisible(slider);
    }
    masterEmergencyButton.setButtonText(juce::String::fromUTF8("! АВАРИЯ !"));
    addAndMakeVisible(&masterEmergencyButton);

    masterDryWetLabel.setText("DRY/WET", juce::dontSendNotification);
    masterFeedbackLabel.setText("FEEDBACK", juce::dontSendNotification);
    masterOutputLabel.setText("OUTPUT", juce::dontSendNotification);
    for (auto *label : {&masterDryWetLabel, &masterFeedbackLabel, &masterOutputLabel})
    {
        label->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    }

    // --- Привязка компонентов к параметрам (Attachments) ---
    tubeHeaterAttach = std::make_unique<SliderAttachment>(processorRef.apvts, "TUBE_HEATER", tubeHeaterSlider);
    tubeAnodeAttach = std::make_unique<SliderAttachment>(processorRef.apvts, "TUBE_ANODE", tubeAnodeSlider);
    tubeBiasAttach = std::make_unique<SliderAttachment>(processorRef.apvts, "TUBE_BIAS", tubeBiasSlider);
    tubeMicResAttach = std::make_unique<SliderAttachment>(processorRef.apvts, "TUBE_MIC_RES", tubeMicResSlider);
    tubeDriftAttach = std::make_unique<SliderAttachment>(processorRef.apvts, "TUBE_DRIFT", tubeDriftSlider);
    tubePreampAttach = std::make_unique<ComboBoxAttachment>(processorRef.apvts, "TUBE_PREAMP", tubePreampChoice);
    tubePowerAttach = std::make_unique<ComboBoxAttachment>(processorRef.apvts, "TUBE_POWER", tubePowerChoice);
    tubeCoreAttach = std::make_unique<ComboBoxAttachment>(processorRef.apvts, "TUBE_CORE", tubeCoreChoice);

    sawRpmAttach = std::make_unique<SliderAttachment>(processorRef.apvts, "SAW_RPM", sawRpmSlider);
    sawThrottleAttach = std::make_unique<SliderAttachment>(processorRef.apvts, "SAW_THROTTLE", sawThrottleSlider);
    sawTensionAttach = std::make_unique<SliderAttachment>(processorRef.apvts, "SAW_TENSION", sawTensionSlider);
    sawKickbackAttach = std::make_unique<SliderAttachment>(processorRef.apvts, "SAW_KICKBACK", sawKickbackSlider);
    sawMufflerAttach = std::make_unique<SliderAttachment>(processorRef.apvts, "SAW_MUFFLER", sawMufflerSlider);
    sawProfileAttach = std::make_unique<ComboBoxAttachment>(processorRef.apvts, "SAW_PROFILE", sawProfileChoice);

    digiBitsAttach = std::make_unique<SliderAttachment>(processorRef.apvts, "DIGI_BITS", digiBitsSlider);
    digiSrateAttach = std::make_unique<SliderAttachment>(processorRef.apvts, "DIGI_SRATE", digiSrateSlider);
    digiGlitchAttach = std::make_unique<SliderAttachment>(processorRef.apvts, "DIGI_GLITCH_DENS", digiGlitchSlider);
    digiScratchAttach = std::make_unique<SliderAttachment>(processorRef.apvts, "DIGI_SCRATCH_SPD", digiScratchSlider);
    digiCompThreshAttach = std::make_unique<SliderAttachment>(processorRef.apvts, "DIGI_COMP_THRESH", digiCompThreshSlider);
    digiCompRatioAttach = std::make_unique<SliderAttachment>(processorRef.apvts, "DIGI_COMP_RATIO", digiCompRatioSlider);

    masterDryWetAttach = std::make_unique<SliderAttachment>(processorRef.apvts, "MASTER_DRYWET", masterDryWetSlider);
    masterFeedbackAttach = std::make_unique<SliderAttachment>(processorRef.apvts, "MASTER_FEEDBACK", masterFeedbackSlider);
    masterOutputAttach = std::make_unique<SliderAttachment>(processorRef.apvts, "MASTER_OUTPUT", masterOutputSlider);
    masterEmergencyAttach = std::make_unique<ButtonAttachment>(processorRef.apvts, "MASTER_EMERGENCY", masterEmergencyButton);

    setSize(1000, 600); // Размер окна плагина
    startTimerHz(30);
}

TriadaFireAudioProcessorEditor::~TriadaFireAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

// ==============================================================================
// ОТРИСОВКА (PAINT)
// ==============================================================================
void TriadaFireAudioProcessorEditor::paint(juce::Graphics &g)
{
    // Фон
    g.fillAll(juce::Colour::fromString("#0B1A0F"));

    auto bounds = getLocalBounds().reduced(10);
    g.setColour(juce::Colour::fromString("#CCCCCC").withAlpha(0.3f));
    g.drawRect(bounds, 1.f);

    // Разделители и заголовки модулей
    auto sectionWidth = getWidth() / 4;
    g.setFont(juce::Font(24.0f, juce::Font::bold));

    juce::String titles[] = {
        juce::String::fromUTF8("🔥 ЛАМПА"),
        juce::String::fromUTF8("🪚 ПИЛА"),
        juce::String::fromUTF8("💾 ЦИФРА"),
        juce::String::fromUTF8("🌐 MASTER")};

    for (int i = 0; i < 4; ++i)
    {
        auto sectionBounds = juce::Rectangle<int>(i * sectionWidth, 0, sectionWidth, getHeight());
        g.setColour(juce::Colour::fromString("#CCCCCC"));
        g.drawText(titles[i], sectionBounds.reduced(10).withHeight(40), juce::Justification::centredTop);

        if (i > 0)
        {
            g.setColour(juce::Colour::fromString("#CCCCCC").withAlpha(0.3f));
            g.drawVerticalLine(i * sectionWidth, 5.f, getHeight() - 5.f);
        }
    }
}

// ==============================================================================
// РАСПОЛОЖЕНИЕ ЭЛЕМЕНТОВ (RESIZED)
// ==============================================================================
void TriadaFireAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(50); // Отступ для заголовка

    auto sectionWidth = getWidth() / 4;
    auto itemHeight = 100;
    auto labelHeight = 20;
    auto comboBoxHeight = 25;

    // --- Секция 1: ЛАМПА ---
    auto lampSection = bounds.removeFromLeft(sectionWidth).reduced(10);

    auto topRow = lampSection.removeFromTop(comboBoxHeight * 3 + 15);
    tubePreampChoice.setBounds(topRow.removeFromTop(comboBoxHeight).reduced(5, 0));
    tubePowerChoice.setBounds(topRow.removeFromTop(comboBoxHeight).reduced(5, 0));
    tubeCoreChoice.setBounds(topRow.removeFromTop(comboBoxHeight).reduced(5, 0));

    auto knobArea = lampSection;
    auto knobWidth = knobArea.getWidth() / 2;

    tubeHeaterLabel.setBounds(knobArea.withWidth(knobWidth).withHeight(labelHeight));
    tubeHeaterSlider.setBounds(knobArea.withWidth(knobWidth).withHeight(itemHeight).withY(knobArea.getY() + labelHeight));

    tubeAnodeLabel.setBounds(knobArea.withLeft(knobArea.getX() + knobWidth).withWidth(knobWidth).withHeight(labelHeight));
    tubeAnodeSlider.setBounds(knobArea.withLeft(knobArea.getX() + knobWidth).withWidth(knobWidth).withHeight(itemHeight).withY(knobArea.getY() + labelHeight));

    knobArea.removeFromTop(itemHeight + labelHeight + 10);

    tubeBiasLabel.setBounds(knobArea.withWidth(knobWidth).withHeight(labelHeight));
    tubeBiasSlider.setBounds(knobArea.withWidth(knobWidth).withHeight(itemHeight).withY(knobArea.getY() + labelHeight));

    tubeMicResLabel.setBounds(knobArea.withLeft(knobArea.getX() + knobWidth).withWidth(knobWidth).withHeight(labelHeight));
    tubeMicResSlider.setBounds(knobArea.withLeft(knobArea.getX() + knobWidth).withWidth(knobWidth).withHeight(itemHeight).withY(knobArea.getY() + labelHeight));

    knobArea.removeFromTop(itemHeight + labelHeight + 10);

    tubeDriftLabel.setBounds(knobArea.withWidth(knobWidth).withHeight(labelHeight));
    tubeDriftSlider.setBounds(knobArea.withWidth(knobWidth).withHeight(itemHeight).withY(knobArea.getY() + labelHeight));

    // --- Секция 2: ПИЛА ---
    auto sawSection = bounds.removeFromLeft(sectionWidth).reduced(10);
    sawProfileChoice.setBounds(sawSection.removeFromTop(comboBoxHeight).reduced(5, 0));

    knobArea = sawSection;
    knobWidth = knobArea.getWidth() / 2;

    sawRpmLabel.setBounds(knobArea.withWidth(knobWidth).withHeight(labelHeight));
    sawRpmSlider.setBounds(knobArea.withWidth(knobWidth).withHeight(itemHeight).withY(knobArea.getY() + labelHeight));

    sawThrottleLabel.setBounds(knobArea.withLeft(knobArea.getX() + knobWidth).withWidth(knobWidth).withHeight(labelHeight));
    sawThrottleSlider.setBounds(knobArea.withLeft(knobArea.getX() + knobWidth).withWidth(knobWidth).withHeight(itemHeight).withY(knobArea.getY() + labelHeight));

    knobArea.removeFromTop(itemHeight + labelHeight + 10);

    sawTensionLabel.setBounds(knobArea.withWidth(knobWidth).withHeight(labelHeight));
    sawTensionSlider.setBounds(knobArea.withWidth(knobWidth).withHeight(itemHeight).withY(knobArea.getY() + labelHeight));

    sawKickbackLabel.setBounds(knobArea.withLeft(knobArea.getX() + knobWidth).withWidth(knobWidth).withHeight(labelHeight));
    sawKickbackSlider.setBounds(knobArea.withLeft(knobArea.getX() + knobWidth).withWidth(knobWidth).withHeight(itemHeight).withY(knobArea.getY() + labelHeight));

    knobArea.removeFromTop(itemHeight + labelHeight + 10);

    sawMufflerLabel.setBounds(knobArea.withWidth(knobWidth).withHeight(labelHeight));
    sawMufflerSlider.setBounds(knobArea.withWidth(knobWidth).withHeight(itemHeight).withY(knobArea.getY() + labelHeight));

    // --- Секция 3: ЦИФРА ---
    auto digiSection = bounds.removeFromLeft(sectionWidth).reduced(10);
    knobArea = digiSection;

    digiBitsLabel.setBounds(knobArea.withWidth(knobWidth).withHeight(labelHeight));
    digiBitsSlider.setBounds(knobArea.withWidth(knobWidth).withHeight(itemHeight).withY(knobArea.getY() + labelHeight));

    digiSrateLabel.setBounds(knobArea.withLeft(knobArea.getX() + knobWidth).withWidth(knobWidth).withHeight(labelHeight));
    digiSrateSlider.setBounds(knobArea.withLeft(knobArea.getX() + knobWidth).withWidth(knobWidth).withHeight(itemHeight).withY(knobArea.getY() + labelHeight));

    knobArea.removeFromTop(itemHeight + labelHeight + 10);

    digiGlitchLabel.setBounds(knobArea.withWidth(knobWidth).withHeight(labelHeight));
    digiGlitchSlider.setBounds(knobArea.withWidth(knobWidth).withHeight(itemHeight).withY(knobArea.getY() + labelHeight));

    digiScratchLabel.setBounds(knobArea.withLeft(knobArea.getX() + knobWidth).withWidth(knobWidth).withHeight(labelHeight));
    digiScratchSlider.setBounds(knobArea.withLeft(knobArea.getX() + knobWidth).withWidth(knobWidth).withHeight(itemHeight).withY(knobArea.getY() + labelHeight));

    knobArea.removeFromTop(itemHeight + labelHeight + 10);

    digiCompThreshLabel.setBounds(knobArea.withWidth(knobWidth).withHeight(labelHeight));
    digiCompThreshSlider.setBounds(knobArea.withWidth(knobWidth).withHeight(itemHeight).withY(knobArea.getY() + labelHeight));

    digiCompRatioLabel.setBounds(knobArea.withLeft(knobArea.getX() + knobWidth).withWidth(knobWidth).withHeight(labelHeight));
    digiCompRatioSlider.setBounds(knobArea.withLeft(knobArea.getX() + knobWidth).withWidth(knobWidth).withHeight(itemHeight).withY(knobArea.getY() + labelHeight));

    // --- Секция 4: MASTER ---
    auto masterSection = bounds.removeFromLeft(sectionWidth).reduced(10);
    knobArea = masterSection;

    masterDryWetLabel.setBounds(knobArea.withWidth(knobWidth).withHeight(labelHeight));
    masterDryWetSlider.setBounds(knobArea.withWidth(knobWidth).withHeight(itemHeight).withY(knobArea.getY() + labelHeight));

    masterFeedbackLabel.setBounds(knobArea.withLeft(knobArea.getX() + knobWidth).withWidth(knobWidth).withHeight(labelHeight));
    masterFeedbackSlider.setBounds(knobArea.withLeft(knobArea.getX() + knobWidth).withWidth(knobWidth).withHeight(itemHeight).withY(knobArea.getY() + labelHeight));

    knobArea.removeFromTop(itemHeight + labelHeight + 10);

    masterOutputLabel.setBounds(knobArea.withWidth(knobWidth).withHeight(labelHeight));
    masterOutputSlider.setBounds(knobArea.withWidth(knobWidth).withHeight(itemHeight).withY(knobArea.getY() + labelHeight));

    masterEmergencyButton.setBounds(masterSection.removeFromBottom(50).reduced(10));
}

// ==============================================================================
// ОБНОВЛЕНИЕ UI (TIMER)
// ==============================================================================
void TriadaFireAudioProcessorEditor::timerCallback()
{
    // Здесь будет код для обновления визуализаторов в реальном времени (осциллограф, метры и т.д.)
}
#include "PluginProcessor.h"
#include "PluginEditor.h"

// ==============================================================================
TriadaFireAudioProcessorEditor::TriadaFireAudioProcessorEditor(TriadaFireAudioProcessor &p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setLookAndFeel(&lookAndFeel);

    auto addSlider = [this](juce::Slider &s, juce::String paramID, juce::String labelText)
    {
        s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 50, 15);
        addAndMakeVisible(s);
        sliderAttachments.push_back(std::make_unique<SliderAttachment>(processorRef.apvts, paramID, s));

        juce::Label *label = new juce::Label(paramID + "_label", labelText);
        label->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
        allLabels.push_back(label);
    };

    auto addComboBox = [this](juce::ComboBox &cb, juce::String paramID, juce::StringArray items)
    {
        cb.addItemList(items, 1);
        cb.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(cb);
        comboBoxAttachments.push_back(std::make_unique<ComboBoxAttachment>(processorRef.apvts, paramID, cb));
    };

    auto addToggleButton = [this](juce::ToggleButton &tb, juce::String paramID, juce::String buttonText)
    {
        tb.setButtonText(buttonText);
        addAndMakeVisible(tb);
        buttonAttachments.push_back(std::make_unique<ButtonAttachment>(processorRef.apvts, paramID, tb));
    };

    // --- TUBE ---
    addSlider(tubeHeaterSlider, "TUBE_HEATER", juce::String::fromUTF8("НАКАЛ"));
    addSlider(tubeAnodeSlider, "TUBE_ANODE", juce::String::fromUTF8("АНОД"));
    addSlider(tubeBiasSlider, "TUBE_BIAS", juce::String::fromUTF8("СМЕЩЕНИЕ"));
    addSlider(tubeLoadResSlider, "TUBE_LOAD_RES", juce::String::fromUTF8("НАГРУЗКА"));
    addSlider(tubeCapCouplSlider, "TUBE_CAP_COUPL", juce::String::fromUTF8("ЕМКОСТЬ"));
    addSlider(tubeCathBypSlider, "TUBE_CATH_BYP", juce::String::fromUTF8("БАЙПАС"));
    addSlider(tubeMicResSlider, "TUBE_MIC_RES", juce::String::fromUTF8("РЕЗОНАНС"));
    addSlider(tubeDriftSlider, "TUBE_DRIFT", juce::String::fromUTF8("ДРЕЙФ"));
    addSlider(tubeQuiesCurSlider, "TUBE_QUIES_CUR", juce::String::fromUTF8("ТОК ПОКОЯ"));
    addSlider(tubeWarmupSlider, "TUBE_WARMUP", juce::String::fromUTF8("ПРОГРЕВ"));
    addComboBox(tubePreampChoice, "TUBE_PREAMP", {"12AX7", juce::String::fromUTF8("6Н2П"), "12AT7", "EF86"});
    addComboBox(tubePowerChoice, "TUBE_POWER", {juce::String::fromUTF8("6П14П"), juce::String::fromUTF8("6П45С"), "EL34", "KT88"});
    addComboBox(tubeCoreChoice, "TUBE_CORE", {juce::String::fromUTF8("Воздух"), juce::String::fromUTF8("Железо"), juce::String::fromUTF8("Пермаллой"), juce::String::fromUTF8("Феррит")});
    addComboBox(tubeWindingChoice, "TUBE_WINDING", {"1:1", "1:2", "1:4", "1:8"});
    addComboBox(tubePhaseModeChoice, "TUBE_PHASE_MODE", {"Push-Pull", "Single-Ended"});
    addToggleButton(tubeExtBiasButton, "TUBE_EXT_BIAS", juce::String::fromUTF8("ВНЕШ. BIAS"));

    // --- SAW ---
    addSlider(sawRpmSlider, "SAW_RPM", juce::String::fromUTF8("RPM ДВС"));
    addSlider(sawIntakeGapSlider, "SAW_INTAKE_GAP", juce::String::fromUTF8("ВЗ ЗАЗОР"));
    addSlider(sawExhaustGapSlider, "SAW_EXHAUST_GAP", juce::String::fromUTF8("ВЫХ ЗАЗОР"));
    addSlider(sawMainJetSlider, "SAW_MAIN_JET", juce::String::fromUTF8("ЖИКЛЕР"));
    addSlider(sawIgnAdvanceSlider, "SAW_IGN_ADVANCE", juce::String::fromUTF8("ОПЕРЕЖ."));
    addSlider(sawSparkGapSlider, "SAW_SPARK_GAP", juce::String::fromUTF8("СВЕЧА"));
    addSlider(sawTensionSlider, "SAW_TENSION", juce::String::fromUTF8("НАТЯЖЕНИЕ"));
    addSlider(sawKickbackSlider, "SAW_KICKBACK", juce::String::fromUTF8("ОТДАЧА"));
    addSlider(sawMufflerSlider, "SAW_MUFFLER", juce::String::fromUTF8("ГЛУШИТЕЛЬ"));
    addSlider(sawPhaseLockSlider, "SAW_PHASE_LOCK", juce::String::fromUTF8("ФАЗА LOCK"));
    addComboBox(sawThrottleCurveChoice, "SAW_THROTTLE_CURVE", {juce::String::fromUTF8("Лин"), juce::String::fromUTF8("Эксп"), juce::String::fromUTF8("Лог"), juce::String::fromUTF8("S-обр.")});
    addComboBox(sawEngineTypeChoice, "SAW_ENGINE_TYPE", {"2Т", "4Т"});
    addComboBox(sawChainPitchChoice, "SAW_CHAIN_PITCH", {"0.325\"", "0.404\"", "0.50\""});
    addComboBox(sawProfileChoice, "SAW_PROFILE", {juce::String::fromUTF8("Чиппер"), juce::String::fromUTF8("Скребок"), juce::String::fromUTF8("Полудолото")});
    addComboBox(sawSyncChoice, "SAW_SYNC", {"Free", "Host BPM", "MIDI", "Audio"});

    // --- DIGI ---
    addSlider(digiBitsSlider, "DIGI_BITS", juce::String::fromUTF8("БИТНОСТЬ"));
    addSlider(digiSrateSlider, "DIGI_SRATE", juce::String::fromUTF8("SRATE"));
    addSlider(digiGlitchGridSlider, "DIGI_GLITCH_GRID", juce::String::fromUTF8("ГЛИТЧ СЕТКА"));
    addSlider(digiGlitchDensSlider, "DIGI_GLITCH_DENS", juce::String::fromUTF8("СКРЕМБЛИНГ"));
    addSlider(digiScratchSpdSlider, "DIGI_SCRATCH_SPD", juce::String::fromUTF8("ВРАЩЕНИЕ"));
    addSlider(digiScratchBufSlider, "DIGI_SCRATCH_BUF", juce::String::fromUTF8("БУФЕР СКРЕТЧ"));
    addSlider(digiCompThreshSlider, "DIGI_COMP_THRESH", juce::String::fromUTF8("ПОРОГ"));
    addSlider(digiCompRatioSlider, "DIGI_COMP_RATIO", juce::String::fromUTF8("РАТИО"));
    addSlider(digiCompKneeSlider, "DIGI_COMP_KNEE", juce::String::fromUTF8("КОЛЕНО"));
    addSlider(digiCompAttackSlider, "DIGI_COMP_ATTACK", juce::String::fromUTF8("АТАКА"));
    addSlider(digiCompReleaseSlider, "DIGI_COMP_RELEASE", juce::String::fromUTF8("РЕЛИЗ"));
    addComboBox(digiQuantCurveChoice, "DIGI_QUANT_CURVE", {juce::String::fromUTF8("Лин"), "μ-Law", "A-Law", "Custom"});
    addComboBox(digiInterpChoice, "DIGI_INTERP", {juce::String::fromUTF8("Ближ"), juce::String::fromUTF8("Лин"), juce::String::fromUTF8("Кубик"), "Sinc", "ZOH"});
    addComboBox(digiScratchModelChoice, "DIGI_SCRATCH_MODEL", {juce::String::fromUTF8("Винил"), juce::String::fromUTF8("Кассет"), juce::String::fromUTF8("Цифра")});
    addToggleButton(digiSidechainButton, "DIGI_SIDECHAIN", juce::String::fromUTF8("САЙДЧЕЙН"));

    // --- MASTER ---
    addSlider(masterDryWetSlider, "MASTER_DRYWET", juce::String::fromUTF8("MIX"));
    addSlider(masterFeedbackSlider, "MASTER_FEEDBACK", juce::String::fromUTF8("FEEDBACK"));
    addSlider(masterOutputSlider, "MASTER_OUTPUT", juce::String::fromUTF8("OUTPUT"));
    addToggleButton(masterEmergencyButton, "MASTER_EMERGENCY", juce::String::fromUTF8("⚠ АВАРИЯ"));
    addToggleButton(globalBypassButton, "GLOBAL_BYPASS", juce::String::fromUTF8("BYPASS"));

    setSize(1350, 750);
    startTimerHz(30);
}

TriadaFireAudioProcessorEditor::~TriadaFireAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
    for (auto *label : allLabels)
        delete label;
    allLabels.clear();
}

void TriadaFireAudioProcessorEditor::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colour::fromString("#0B1A0F"));
    auto bounds = getLocalBounds().reduced(5);
    g.setColour(juce::Colour::fromString("#CCCCCC").withAlpha(0.3f));
    g.drawRect(bounds, 1.0f);

    int sectionWidth = getWidth() / 4;

    // Исправление предупреждения C4996: Использование juce::FontOptions
    g.setFont(juce::FontOptions(24.0f).withName("Consolas").withStyle("Bold"));

    juce::String titles[] = {
        juce::String::fromUTF8("🔥 ЛАМПА"), juce::String::fromUTF8("🪚 ПИЛА"),
        juce::String::fromUTF8("💾 ЦИФРА"), juce::String::fromUTF8("🌐 МАСТЕР")};

    for (int i = 0; i < 4; ++i)
    {
        g.setColour(juce::Colour::fromString("#CCCCCC"));
        g.drawText(titles[i], i * sectionWidth, 10, sectionWidth, 30, juce::Justification::centred);
        if (i > 0)
        {
            g.setColour(juce::Colour::fromString("#CCCCCC").withAlpha(0.2f));
            g.drawVerticalLine(i * sectionWidth, 5.0f, static_cast<float>(getHeight() - 5));
        }
    }
}

void TriadaFireAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(45);

    int sectionWidth = area.getWidth() / 4;
    int knobSize = 85;
    int labelHeight = 15;
    int comboHeight = 22;
    int spacingY = 8;

    // Хелпер-лямбда для авто-размещения в сетку 2x2
    auto placeKnob = [&](juce::Slider &s, int index, juce::Label *l, int xOff, int yOff, int colW)
    {
        int row = index / 2;
        int col = index % 2;
        int kx = xOff + col * colW;
        int ky = yOff + row * (knobSize + labelHeight + spacingY);
        l->setBounds(kx, ky, colW, labelHeight);
        s.setBounds(kx, ky + labelHeight, colW, knobSize);
    };

    // --- TUBE ---
    juce::Rectangle<int> sT = area.removeFromLeft(sectionWidth).reduced(5);
    int currY = sT.getY();
    std::vector<juce::ComboBox *> tbCombos = {&tubePreampChoice, &tubePowerChoice, &tubeCoreChoice, &tubeWindingChoice, &tubePhaseModeChoice};
    for (auto *cb : tbCombos)
    {
        cb->setBounds(sT.getX(), currY, sT.getWidth(), comboHeight);
        currY += comboHeight + spacingY;
    }

    std::vector<juce::Slider *> tbSld = {&tubeHeaterSlider, &tubeAnodeSlider, &tubeBiasSlider, &tubeLoadResSlider, &tubeCapCouplSlider, &tubeCathBypSlider, &tubeMicResSlider, &tubeDriftSlider, &tubeQuiesCurSlider, &tubeWarmupSlider};
    for (size_t i = 0; i < tbSld.size(); ++i)
        placeKnob(*tbSld[i], static_cast<int>(i), allLabels[i], sT.getX(), currY + spacingY, sT.getWidth() / 2);
    tubeExtBiasButton.setBounds(sT.getX(), sT.getBottom() - 30, sT.getWidth(), 30);

    // --- SAW ---
    juce::Rectangle<int> sS = area.removeFromLeft(sectionWidth).reduced(5);
    currY = sS.getY();
    std::vector<juce::ComboBox *> swCombos = {&sawThrottleCurveChoice, &sawEngineTypeChoice, &sawChainPitchChoice, &sawProfileChoice, &sawSyncChoice};
    for (auto *cb : swCombos)
    {
        cb->setBounds(sS.getX(), currY, sS.getWidth(), comboHeight);
        currY += comboHeight + spacingY;
    }

    std::vector<juce::Slider *> swSld = {&sawRpmSlider, &sawIntakeGapSlider, &sawExhaustGapSlider, &sawMainJetSlider, &sawIgnAdvanceSlider, &sawSparkGapSlider, &sawTensionSlider, &sawKickbackSlider, &sawMufflerSlider, &sawPhaseLockSlider};
    for (size_t i = 0; i < swSld.size(); ++i)
        placeKnob(*swSld[i], static_cast<int>(i), allLabels[10 + i], sS.getX(), currY + spacingY, sS.getWidth() / 2);

    // --- DIGI ---
    juce::Rectangle<int> sD = area.removeFromLeft(sectionWidth).reduced(5);
    currY = sD.getY();
    std::vector<juce::ComboBox *> dgCombos = {&digiQuantCurveChoice, &digiInterpChoice, &digiScratchModelChoice};
    for (auto *cb : dgCombos)
    {
        cb->setBounds(sD.getX(), currY, sD.getWidth(), comboHeight);
        currY += comboHeight + spacingY;
    }

    std::vector<juce::Slider *> dgSld = {&digiBitsSlider, &digiSrateSlider, &digiGlitchGridSlider, &digiGlitchDensSlider, &digiScratchSpdSlider, &digiScratchBufSlider, &digiCompThreshSlider, &digiCompRatioSlider, &digiCompKneeSlider, &digiCompAttackSlider, &digiCompReleaseSlider};
    for (size_t i = 0; i < dgSld.size(); ++i)
        placeKnob(*dgSld[i], static_cast<int>(i), allLabels[20 + i], sD.getX(), currY + spacingY, sD.getWidth() / 2);
    digiSidechainButton.setBounds(sD.getX() + sD.getWidth() / 2, sD.getY() + currY + spacingY + 5 * (knobSize + labelHeight + spacingY), sD.getWidth() / 2, 30);

    // --- MASTER ---
    juce::Rectangle<int> sM = area.removeFromLeft(sectionWidth).reduced(5);
    currY = sM.getY();
    std::vector<juce::Slider *> mstSld = {&masterDryWetSlider, &masterFeedbackSlider, &masterOutputSlider};
    for (size_t i = 0; i < mstSld.size(); ++i)
        placeKnob(*mstSld[i], static_cast<int>(i), allLabels[31 + i], sM.getX(), currY + spacingY, sM.getWidth() / 2);

    masterEmergencyButton.setBounds(sM.getX(), sM.getBottom() - 80, sM.getWidth(), 30);
    globalBypassButton.setBounds(sM.getX(), sM.getBottom() - 40, sM.getWidth(), 30);
}

void TriadaFireAudioProcessorEditor::timerCallback() { masterEmergencyButton.repaint(); }
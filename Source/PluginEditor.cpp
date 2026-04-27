#include "PluginProcessor.h"
#include "PluginEditor.h"

ThermoCore77AudioProcessorEditor::ThermoCore77AudioProcessorEditor(ThermoCore77AudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&thermoLookAndFeel);

    // ==============================================================================
    // 🟠 ПАНЕЛЬ 1: ЛАМПОВЫЙ КАСКАД (ID Панели: 0)
    // ==============================================================================
    addCombo("tubeType", juce::String::fromUTF8("ТИП ЛАМПЫ"),
             {"6Н2П", "6П14П", "6П45С", "ГУ-50"}, 0);
    addSlider("heaterV", juce::String::fromUTF8("НАКАЛ (В)"), 0);
    addSlider("anodeV", juce::String::fromUTF8("АНОД (В)"), 0);
    addSlider("gridBias", juce::String::fromUTF8("СМЕЩЕНИЕ"), 0);
    addSlider("loadRes", juce::String::fromUTF8("НАГРУЗКА"), 0);
    addSlider("thermalDrift", juce::String::fromUTF8("ДРЕЙФ"), 0);
    addSlider("microphonic", juce::String::fromUTF8("МИКРОФОН"), 0);

    // ==============================================================================
    // 🔵 ПАНЕЛЬ 2: МЕХАНИЧЕСКИЙ РЕЗАК (ID Панели: 1)
    // ==============================================================================
    addSlider("rpm", juce::String::fromUTF8("ОБОРОТЫ"), 1);
    addToggle("is4T", juce::String::fromUTF8("РЕЖИМ 4Т"), 1);
    addSlider("intakeGap", juce::String::fromUTF8("ВУПСК. КЛАПАН"), 1);
    addSlider("exhaustGap", juce::String::fromUTF8("ВЫПУСК. КЛАПАН"), 1);
    addSlider("fuelJet", juce::String::fromUTF8("ЖИКЛЕР"), 1);
    addSlider("sparkGap", juce::String::fromUTF8("ЗАЗОР СВЕЧИ"), 1);
    addSlider("tension", juce::String::fromUTF8("НАТЯЖЕНИЕ"), 1);

    // ==============================================================================
    // 🟢 ПАНЕЛЬ 3: РЕАКТОРНЫЙ КОНТУР (ID Панели: 2)
    // ==============================================================================
    addSlider("rodPos", juce::String::fromUTF8("СТЕРЖНИ %"), 2);
    addSlider("rodInSpeed", juce::String::fromUTF8("СК. ПОГРУЖ."), 2);
    addSlider("rodOutSpeed", juce::String::fromUTF8("СК. ИЗВЛЕЧ."), 2);
    addSlider("rodInertia", juce::String::fromUTF8("ИНЕРЦИЯ"), 2);
    addSlider("coolant", juce::String::fromUTF8("ОХЛАЖДЕНИЕ"), 2);
    addToggle("safety", juce::String::fromUTF8("КОНТЕЙНМЕНТ"), 2);

    // ==============================================================================
    // 🟣 ПАНЕЛЬ 4: КОСМИЧЕСКАЯ СРЕДА (ID Панели: 3)
    // ==============================================================================
    addSlider("lensMass", juce::String::fromUTF8("МАССА ЛИНЗЫ"), 3);
    addSlider("horizon", juce::String::fromUTF8("ГОРИЗОНТ"), 3);
    addSlider("eccentricity", juce::String::fromUTF8("ОРБИТА"), 3);
    addSlider("vacuum", juce::String::fromUTF8("ВАКУУМ"), 3);

    // ==============================================================================
    // 🔴 ПАНЕЛЬ 5: ЦИФРОВОЙ АРХИВ (ID Панели: 4)
    // ==============================================================================
    addCombo("blockSize", juce::String::fromUTF8("РАЗМЕР БЛОКА"),
             {"8 сэмплов", "16 сэмплов", "32 сэмпла"}, 4);
    addSlider("quality", juce::String::fromUTF8("КАЧЕСТВО"), 4);
    addSlider("glitch", juce::String::fromUTF8("ГЛИТЧ"), 4);
    addSlider("turbine", juce::String::fromUTF8("ТУРБИНА"), 4);

    // Глобальные элементы (в самом низу)
    addSlider("masterMix", juce::String::fromUTF8("СМЕСЬ"), 0);
    // (Поместим его на первую панель в самый низ для простоты сетки)

    // Добавляем стрелочные приборы
    addAndMakeVisible(anodeMeter);
    addAndMakeVisible(neutronMeter);

    // Общие размеры (сделаем его еще шире для 5 огромных панелей)
    setSize(1200, 750);
    startTimerHz(30);
}

// --- ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ ГЕНЕРАЦИИ ---

void ThermoCore77AudioProcessorEditor::addSlider(juce::String paramID, juce::String labelText, int panelIndex)
{
    auto s = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
    addAndMakeVisible(*s);
    sliderAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, paramID, *s));

    auto l = std::make_unique<juce::Label>("", labelText);
    l->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*l);

    uiLayout.push_back({0, (int)sliders.size(), panelIndex});
    sliders.push_back(std::move(s));
    labels.push_back(std::move(l));
    elementsInPanel[panelIndex]++;
}

void ThermoCore77AudioProcessorEditor::addToggle(juce::String paramID, juce::String labelText, int panelIndex)
{
    auto t = std::make_unique<juce::ToggleButton>(labelText);
    addAndMakeVisible(*t);
    buttonAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, paramID, *t));

    uiLayout.push_back({1, (int)toggles.size(), panelIndex});
    toggles.push_back(std::move(t));
    elementsInPanel[panelIndex]++;
}

void ThermoCore77AudioProcessorEditor::addCombo(juce::String paramID, juce::String labelText, const juce::StringArray &choices, int panelIndex)
{
    auto c = std::make_unique<juce::ComboBox>();
    c->addItemList(choices, 1);
    addAndMakeVisible(*c);
    comboAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, paramID, *c));

    auto l = std::make_unique<juce::Label>("", labelText);
    l->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*l);

    uiLayout.push_back({2, (int)comboBoxes.size(), panelIndex});
    comboBoxes.push_back(std::move(c));
    labels.push_back(std::move(l));
    elementsInPanel[panelIndex]++;
}

void ThermoCore77AudioProcessorEditor::timerCallback()
{
    auto state = audioProcessor.getReactorState();
    neutronMeter.setValue(state.neutronFlux * 0.05f);
    anodeMeter.setValue(0.3f + state.neutronFlux * 0.1f);
}

void ThermoCore77AudioProcessorEditor::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colour(0xff0b1a0f)); // Наш фирменный бакелитовый темно-зеленый цвет ТЗ

    // Рисуем 5 вертикальных отсеков
    g.setColour(juce::Colours::orange.withAlpha(0.2f));
    int panelWidth = getWidth() / 5;
    for (int i = 1; i < 5; ++i)
        g.drawVerticalLine(i * panelWidth, 0.0f, (float)getHeight());

    // Заголовки панелей
    g.setColour(juce::Colours::orange);
    g.setFont(juce::Font(18.0f, juce::Font::bold));
    g.drawText(juce::String::fromUTF8("1. ЛАМПА"), 0, 10, panelWidth, 30, juce::Justification::centred);
    g.drawText(juce::String::fromUTF8("2. РЕЗАК"), panelWidth, 10, panelWidth, 30, juce::Justification::centred);
    g.drawText(juce::String::fromUTF8("3. ЯДРО"), panelWidth * 2, 10, panelWidth, 30, juce::Justification::centred);
    g.drawText(juce::String::fromUTF8("4. КОСМОС"), panelWidth * 3, 10, panelWidth, 30, juce::Justification::centred);
    g.drawText(juce::String::fromUTF8("5. ЦИФРА"), panelWidth * 4, 10, panelWidth, 30, juce::Justification::centred);
}

void ThermoCore77AudioProcessorEditor::resized()
{
    int panelWidth = getWidth() / 5;
    int knobSize = 65; // Уменьшим размер ручек, чтобы всё влезло

    // Плотная автоматическая расстановка по сетке
    int currentY[5] = {60, 60, 60, 60, 60};
    int sliderIdx = 0, toggleIdx = 0, comboIdx = 0;
    int labelIdx = 0;

    for (const auto &element : uiLayout)
    {
        int p = element.panel;
        int x = p * panelWidth + (panelWidth - knobSize) / 2;
        int y = currentY[p];

        if (element.type == 0) // Slider
        {
            sliders[element.indexInVector]->setBounds(x, y, knobSize, knobSize);
            labels[labelIdx]->setBounds(p * panelWidth + 10, y + knobSize - 5, panelWidth - 20, 20);
            labelIdx++;
            currentY[p] += knobSize + 25;
        }
        else if (element.type == 1) // Toggle
        {
            toggles[element.indexInVector]->setBounds(p * panelWidth + 20, y, panelWidth - 40, 30);
            currentY[p] += 40;
        }
        else if (element.type == 2) // Combo
        {
            comboBoxes[element.indexInVector]->setBounds(p * panelWidth + 20, y, panelWidth - 40, 25);
            labels[labelIdx]->setBounds(p * panelWidth + 10, y + 25, panelWidth - 20, 20);
            labelIdx++;
            currentY[p] += 55;
        }
    }

    // Размещение стрелочных приборов в самом низу (где осталось место)
    anodeMeter.setBounds(20, 580, 160, 120);
    neutronMeter.setBounds(panelWidth * 2 + 20, 580, 160, 120);
}

ThermoCore77AudioProcessorEditor::~ThermoCore77AudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}
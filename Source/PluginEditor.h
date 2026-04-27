#pragma once
#include "PluginProcessor.h"
#include "ThermoLookAndFeel.h"
#include "NeedleMeter.h"

class ThermoCore77AudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
  ThermoCore77AudioProcessorEditor(ThermoCore77AudioProcessor &);
  ~ThermoCore77AudioProcessorEditor() override;

  void paint(juce::Graphics &) override;
  void resized() override;
  void timerCallback() override;

private:
  ThermoCore77AudioProcessor &audioProcessor;
  ThermoLookAndFeel thermoLookAndFeel;

  // Векторы для хранения всех элементов интерфейса
  std::vector<std::unique_ptr<juce::Slider>> sliders;
  std::vector<std::unique_ptr<juce::ToggleButton>> toggles;
  std::vector<std::unique_ptr<juce::ComboBox>> comboBoxes;
  std::vector<std::unique_ptr<juce::Label>> labels;

  // Векторы для хранения связей с параметрами (APVTS)
  std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAtts;
  std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAtts;
  std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtts;

  // Стрелочные индикаторы
  NeedleMeter anodeMeter{juce::String::fromUTF8("ТОК АНОДА")};
  NeedleMeter neutronMeter{juce::String::fromUTF8("ПОТОК Φ")};

  // Методы для генерации плотного интерфейса
  void addSlider(juce::String paramID, juce::String labelText, int panelIndex);
  void addToggle(juce::String paramID, juce::String labelText, int panelIndex);
  void addCombo(juce::String paramID, juce::String labelText, const juce::StringArray &choices, int panelIndex);

  // Вспомогательный счетчик элементов на панелях для авто-расстановки
  int elementsInPanel[5] = {0, 0, 0, 0, 0};

  // Структура для хранения информации о расположении
  struct UIElement
  {
    int type; // 0: Slider, 1: Toggle, 2: Combo
    int indexInVector;
    int panel;
  };
  std::vector<UIElement> uiLayout;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThermoCore77AudioProcessorEditor)
};
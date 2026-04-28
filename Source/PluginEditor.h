#pragma once

#include "PluginProcessor.h"

// ==============================================================================
// КАСТОМНЫЙ LOOK AND FEEL (Стиль НИИ 1970-80х)
// ==============================================================================
class TriadaLookAndFeel : public juce::LookAndFeel_V4
{
public:
  TriadaLookAndFeel()
  {
    // Цветовая палитра из ТЗ
    setColour(juce::Slider::thumbColourId, juce::Colour::fromString("#FF8C00"));               // Актив
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromString("#0B1A0F"));    // Фон
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour::fromString("#CCCCCC")); // Обводка
    setColour(juce::Label::textColourId, juce::Colour::fromString("#CCCCCC"));
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour::fromString("#0B1A0F"));
  }

  void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height, float sliderPos,
                        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider &slider) override
  {
    auto bounds = juce::Rectangle<float>(x, y, width, height).reduced(10);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = juce::jmin(4.0f, radius * 0.5f);
    auto arcRadius = radius - lineW * 0.5f;

    // Фон крутилки (тяжелый металл)
    g.setColour(juce::Colour::fromString("#0B1A0F"));
    g.fillEllipse(bounds);

    // Обводка
    g.setColour(juce::Colour::fromString("#CCCCCC"));
    g.drawEllipse(bounds, 1.5f);

    // Индикатор-указатель
    juce::Path p;
    p.addPieSegment(bounds.getCentreX() - arcRadius, bounds.getCentreY() - arcRadius, arcRadius * 2.0f, arcRadius * 2.0f, rotaryStartAngle, toAngle, 0.6);
    g.setColour(juce::Colour::fromString("#FF8C00"));
    g.fillPath(p);

    // Центральная точка
    g.setColour(juce::Colour::fromString("#0B1A0F"));
    g.fillEllipse(bounds.getCentreX() - lineW, bounds.getCentreY() - lineW, lineW * 2.0f, lineW * 2.0f);
  }
};

// ==============================================================================
// ГЛАВНЫЙ РЕДАКТОР ИНТЕРФЕЙСА
// ==============================================================================
class TriadaFireAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
  explicit TriadaFireAudioProcessorEditor(TriadaFireAudioProcessor &);
  ~TriadaFireAudioProcessorEditor() override;

  // --- juce::Component Overrides ---
  void paint(juce::Graphics &) override;
  void resized() override;
  void timerCallback() override;

private:
  TriadaFireAudioProcessor &processorRef;
  TriadaLookAndFeel lookAndFeel;

  // --- Объявление всех UI-компонентов ---

  // Модуль 1: Лампа
  juce::Slider tubeHeaterSlider, tubeAnodeSlider, tubeBiasSlider, tubeMicResSlider, tubeDriftSlider;
  juce::ComboBox tubePreampChoice, tubePowerChoice, tubeCoreChoice;
  juce::Label tubeHeaterLabel, tubeAnodeLabel, tubeBiasLabel, tubeMicResLabel, tubeDriftLabel;

  // Модуль 2: Пила
  juce::Slider sawRpmSlider, sawThrottleSlider, sawTensionSlider, sawKickbackSlider, sawMufflerSlider;
  juce::ComboBox sawProfileChoice;
  juce::Label sawRpmLabel, sawThrottleLabel, sawTensionLabel, sawKickbackLabel, sawMufflerLabel;

  // Модуль 3: Цифра
  juce::Slider digiBitsSlider, digiSrateSlider, digiGlitchSlider, digiScratchSlider;
  juce::Slider digiCompThreshSlider, digiCompRatioSlider;
  juce::Label digiBitsLabel, digiSrateLabel, digiGlitchLabel, digiScratchLabel, digiCompThreshLabel, digiCompRatioLabel;

  // Модуль 4: Мастер
  juce::Slider masterDryWetSlider, masterFeedbackSlider, masterOutputSlider;
  juce::ToggleButton masterEmergencyButton;
  juce::Label masterDryWetLabel, masterFeedbackLabel, masterOutputLabel, masterEmergencyLabel;

  // --- Аттачменты для связи UI с параметрами ---
  using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
  using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
  using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

  std::unique_ptr<SliderAttachment> tubeHeaterAttach, tubeAnodeAttach, tubeBiasAttach, tubeMicResAttach, tubeDriftAttach;
  std::unique_ptr<ComboBoxAttachment> tubePreampAttach, tubePowerAttach, tubeCoreAttach;

  std::unique_ptr<SliderAttachment> sawRpmAttach, sawThrottleAttach, sawTensionAttach, sawKickbackAttach, sawMufflerAttach;
  std::unique_ptr<ComboBoxAttachment> sawProfileAttach;

  std::unique_ptr<SliderAttachment> digiBitsAttach, digiSrateAttach, digiGlitchAttach, digiScratchAttach;
  std::unique_ptr<SliderAttachment> digiCompThreshAttach, digiCompRatioAttach;

  std::unique_ptr<SliderAttachment> masterDryWetAttach, masterFeedbackAttach, masterOutputAttach;
  std::unique_ptr<ButtonAttachment> masterEmergencyAttach;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TriadaFireAudioProcessorEditor)
};
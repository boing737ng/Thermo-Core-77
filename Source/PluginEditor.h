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
    setColour(juce::Slider::thumbColourId, juce::Colour::fromString("#FF8C00"));
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromString("#0B1A0F"));
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour::fromString("#CCCCCC"));
    setColour(juce::Label::textColourId, juce::Colour::fromString("#CCCCCC"));
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour::fromString("#0B1A0F"));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour::fromString("#0B1A0F").brighter(0.1f));
    setColour(juce::ComboBox::outlineColourId, juce::Colour::fromString("#CCCCCC"));
    setColour(juce::ComboBox::textColourId, juce::Colour::fromString("#00CC66"));
    setColour(juce::ComboBox::arrowColourId, juce::Colour::fromString("#00CC66"));
    setColour(juce::ToggleButton::textColourId, juce::Colour::fromString("#FF1A1A"));
    setColour(juce::ToggleButton::tickColourId, juce::Colour::fromString("#FF1A1A"));
  }

  void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height, float sliderPos,
                        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider &slider) override
  {
    juce::ignoreUnused(slider);

    // Явное приведение типов (устранение warning C4244/C4305)
    auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width), static_cast<float>(height)).reduced(10.0f);
    float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    float toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    g.setColour(juce::Colour::fromString("#0B1A0F").brighter(0.1f));
    g.fillEllipse(bounds);

    g.setColour(juce::Colour::fromString("#CCCCCC"));
    g.drawEllipse(bounds, 1.5f);

    juce::Path p;
    p.addPieSegment(bounds.getCentreX() - radius, bounds.getCentreY() - radius, radius * 2.0f, radius * 2.0f, rotaryStartAngle, toAngle, 0.6f);
    g.setColour(juce::Colour::fromString("#FF8C00"));
    g.fillPath(p);

    g.setColour(juce::Colour::fromString("#0B1A0F"));
    g.fillEllipse(bounds.getCentreX() - radius * 0.3f, bounds.getCentreY() - radius * 0.3f, radius * 0.6f, radius * 0.6f);
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

  void paint(juce::Graphics &) override;
  void resized() override;
  void timerCallback() override;

private:
  TriadaFireAudioProcessor &processorRef;
  TriadaLookAndFeel lookAndFeel;

  std::vector<juce::Label *> allLabels;

  // --- Модуль 1: Лампа ---
  juce::Slider tubeHeaterSlider, tubeAnodeSlider, tubeBiasSlider, tubeMicResSlider, tubeDriftSlider;
  juce::Slider tubeLoadResSlider, tubeCapCouplSlider, tubeCathBypSlider, tubeQuiesCurSlider, tubeWarmupSlider;
  juce::ComboBox tubePreampChoice, tubePowerChoice, tubeCoreChoice, tubeWindingChoice, tubePhaseModeChoice;
  juce::ToggleButton tubeExtBiasButton;

  // --- Модуль 2: Пила ---
  juce::Slider sawRpmSlider, sawIntakeGapSlider, sawExhaustGapSlider, sawMainJetSlider, sawIgnAdvanceSlider, sawSparkGapSlider;
  juce::Slider sawTensionSlider, sawKickbackSlider, sawMufflerSlider, sawPhaseLockSlider;
  juce::ComboBox sawThrottleCurveChoice, sawEngineTypeChoice, sawChainPitchChoice, sawProfileChoice, sawSyncChoice;

  // --- Модуль 3: Цифра ---
  juce::Slider digiBitsSlider, digiSrateSlider, digiGlitchGridSlider, digiGlitchDensSlider, digiScratchSpdSlider, digiScratchBufSlider;
  juce::Slider digiCompThreshSlider, digiCompRatioSlider, digiCompKneeSlider, digiCompAttackSlider, digiCompReleaseSlider;
  juce::ComboBox digiQuantCurveChoice, digiInterpChoice, digiScratchModelChoice;
  juce::ToggleButton digiSidechainButton;

  // --- Модуль 4: Мастер ---
  juce::Slider masterDryWetSlider, masterFeedbackSlider, masterOutputSlider;
  juce::ToggleButton masterEmergencyButton, globalBypassButton;

  // --- Аттачменты ---
  using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
  using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
  using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

  std::vector<std::unique_ptr<SliderAttachment>> sliderAttachments;
  std::vector<std::unique_ptr<ComboBoxAttachment>> comboBoxAttachments;
  std::vector<std::unique_ptr<ButtonAttachment>> buttonAttachments;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TriadaFireAudioProcessorEditor)
};
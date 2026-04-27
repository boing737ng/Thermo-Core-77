#pragma once

#include <JuceHeader.h>
#include "BaseModule.h"
#include "TubeModule.h"
#include "ChainsawModule.h"
#include "ReactorModule.h"
#include "SpaceModule.h"
#include "DigitalArchiveModule.h"

// ==============================================================================
/**
 * ГЛАВНЫЙ ПРОЦЕССОР ОБЪЕКТА «ТЕРМО-ЯДРО-77»
 */
class ThermoCore77AudioProcessor : public juce::AudioProcessor
{
public:
  // ==============================================================================
  ThermoCore77AudioProcessor();
  ~ThermoCore77AudioProcessor() override;

  // ==============================================================================
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
#endif

  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

  // ==============================================================================
  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override;

  // ==============================================================================
  const juce::String getName() const override { return "ОБЪЕКТ ТЕРМО-ЯДРО-77"; }

  bool acceptsMidi() const override { return true; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }

  // ==============================================================================
  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int index) override {}
  const juce::String getProgramName(int index) override { return {}; }
  void changeProgramName(int index, const juce::String &newName) override {}

  // ==============================================================================
  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  // ==============================================================================
  // ПУБЛИЧНЫЙ ДОСТУП К ДАННЫМ ДЛЯ GUI

  // Возвращает текущее состояние реактора (для измерителей Гейгера и графиков)
  ReactorModule::KineticsState getReactorState() const
  {
    if (auto *reactor = static_cast<ReactorModule *>(modules[2].get()))
      return reactor->getCurrentState();
    return {};
  }

  // Доступ к APVTS
  juce::AudioProcessorValueTreeState apvts;

  // Метод изменения маршрута (вызывается из Editor при перетаскивании кабелей)
  void updateRouting(const std::vector<int> &newOrder)
  {
    const juce::ScopedLock sl(routingLock);
    routingOrder = newOrder;
  }

private:
  // Потокобезопасное обновление параметров
  void updateModuleParameters();

  // Статический макет параметров
  juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  // КОНТЕЙНЕР МОДУЛЕЙ
  // 0: Tube, 1: Chainsaw, 2: Reactor, 3: Space, 4: Digital
  std::vector<std::unique_ptr<BaseModule>> modules;

  // МАРШРУТИЗАЦИЯ
  std::vector<int> routingOrder;
  juce::CriticalSection routingLock;

  // ГЛОБАЛЬНЫЕ DSP ЭЛЕМЕНТЫ
  juce::LinearSmoothedValue<float> masterMix;

  // ==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThermoCore77AudioProcessor)
};
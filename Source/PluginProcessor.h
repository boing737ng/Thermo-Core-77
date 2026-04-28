#pragma once
#include <JuceHeader.h>
#include <random>

// ==============================================================================
// Вспомогательная математика и физика для DSP
// ==============================================================================
namespace PhysicsMath
{
  inline float clamp(float v, float minVal, float maxVal) { return std::max(minVal, std::min(v, maxVal)); }
  inline float tubeTransfer(float x, float a1, float a2, float a3, float a4, float a5)
  {
    float x2 = x * x;
    float x3 = x2 * x;
    float x4 = x3 * x;
    float x5 = x4 * x;
    return a1 * x + a2 * x2 + a3 * x3 + a4 * x4 + a5 * x5;
  }
}

// ==============================================================================
// DSP Модуль 1: ЛАМПОВЫЙ КАСКАД
// ==============================================================================
class TubeStage
{
public:
  TubeStage() = default;
  void prepare(double sampleRate, int samplesPerBlock);
  void process(juce::AudioBuffer<float> &buffer, juce::AudioProcessorValueTreeState &apvts);
  void reset();

private:
  double fs = 44100.0;
  float currentTemp = 300.0f;
  float currentH[2] = {0.0f, 0.0f};
  float currentM[2] = {0.0f, 0.0f};
  float prevY[2] = {0.0f, 0.0f};
  juce::SmoothedValue<float> smoothHeaterVolt, smoothAnodeVolt, smoothBias;
  std::mt19937 rng;
};

// ==============================================================================
// DSP Модуль 2: МЕХАНИЧЕСКИЙ РЕЗАК
// ==============================================================================
class ChainsawGranulator
{
public:
  ChainsawGranulator();
  void prepare(double sampleRate, int samplesPerBlock);
  void process(juce::AudioBuffer<float> &buffer, juce::AudioProcessorValueTreeState &apvts);
  void reset();

private:
  double fs = 44100.0;
  float omega = 0.0f;
  float phase[2] = {0.0f, 0.0f};
  float kickbackPhase[2] = {0.0f, 0.0f};
  float z1[2] = {0.0f, 0.0f};
  float z2[2] = {0.0f, 0.0f};
  juce::SmoothedValue<float> smoothRPM, smoothTension, smoothMuffler;
  std::mt19937 rng;
  std::uniform_real_distribution<float> dist;
};

// ==============================================================================
// DSP Модуль 3: ЦИФРОВОЙ ДЕСТРУКТОР
// ==============================================================================
class DigitalDestructor
{
public:
  DigitalDestructor();
  void prepare(double sampleRate, int samplesPerBlock);
  void process(juce::AudioBuffer<float> &buffer, juce::AudioProcessorValueTreeState &apvts);
  void reset();

private:
  double fs = 44100.0;
  float scratchPhase[2] = {0.0f, 0.0f};
  float currentGR[2] = {0.0f, 0.0f};
  juce::AudioBuffer<float> scratchBuffer;
  int writePos = 0;
  juce::SmoothedValue<float> smoothBitDepth, smoothSampleRate, smoothScratchSpeed;
  std::mt19937 rng;
  std::uniform_real_distribution<float> dist;
};

// ==============================================================================
// ГЛАВНЫЙ ПРОЦЕССОР
// ==============================================================================
class TriadaFireAudioProcessor : public juce::AudioProcessor
{
public:
  TriadaFireAudioProcessor();
  ~TriadaFireAudioProcessor() override;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override { return true; }
  const juce::String getName() const override { return juce::String::fromUTF8("АГРЕГАТ «ТРИАДА-ОГОНЬ»"); }
  bool acceptsMidi() const override { return true; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }
  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int index) override {}
  const juce::String getProgramName(int index) override { return juce::String::fromUTF8("Заводской"); }
  void changeProgramName(int index, const juce::String &newName) override {}
  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;
  static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

  juce::AudioProcessorValueTreeState apvts;
  std::atomic<float> rmsOutL{0.0f};
  std::atomic<float> rmsOutR{0.0f};

private:
  TubeStage tubeStage;
  ChainsawGranulator chainsawGranulator;
  DigitalDestructor digitalDestructor;
  juce::AudioBuffer<float> feedbackBuffer;
  int feedbackReadPos = 0;
  int feedbackWritePos = 64;
  juce::SmoothedValue<float> smoothDryWet;
  juce::SmoothedValue<float> smoothMasterOut;
  juce::AudioBuffer<float> dryBuffer;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TriadaFireAudioProcessor)
};
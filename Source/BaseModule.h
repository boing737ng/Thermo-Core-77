#pragma once
#include <JuceHeader.h>

// Базовый класс для всех модулей «ТЕРМО-ЯДРО-77»
class BaseModule
{
public:
    virtual ~BaseModule() = default;

    virtual void prepare(const juce::dsp::ProcessSpec &spec) = 0;
    virtual void reset() = 0;

    // Главный метод обработки
    void process(juce::AudioBuffer<float> &buffer)
    {
        if (isBypassed)
            return;

        // Копируем вход для Dry/Wet
        juce::AudioBuffer<float> dryBuffer;
        dryBuffer.makeCopyOf(buffer);

        processInternal(buffer);

        // Смешивание Dry/Wet
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            buffer.addFromWithRamp(ch, 0, dryBuffer.getReadPointer(ch), buffer.getNumSamples(), 1.0f - mix, 1.0f - mix);
            buffer.applyGain(ch, 0, buffer.getNumSamples(), mix);
        }
    }

    virtual void processInternal(juce::AudioBuffer<float> &buffer) = 0;

    void setBypass(bool shouldBypass) { isBypassed = shouldBypass; }
    void setMix(float newMix) { mix = juce::jlimit(0.0f, 1.0f, newMix); }

protected:
    bool isBypassed = false;
    float mix = 1.0f;
    double sampleRate = 44100.0;
};
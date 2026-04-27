#pragma once
#include "BaseModule.h"

class ChainsawModule : public BaseModule
{
public:
    struct Params
    {
        float rpm;         // 1500 - 14000
        bool isFourStroke; // 2Т или 4Т
        float intakeGap;   // зазор клапана
        float exhaustGap;
        float fuelJetSize;   // жиклер
        float ignitionAngle; // угол зажигания
        float sparkGap;      // зазор свечи
        int chainStep;       // 0: 0.325, 1: 0.404, 2: 0.50
        float tension;       // натяжение
    };

    void prepare(const juce::dsp::ProcessSpec &spec) override
    {
        sampleRate = spec.sampleRate;
        phase = 0.0f;
    }

    void reset() override { phase = 0.0f; }

    void updateParams(const Params &p) { params = p; }

    void processInternal(juce::AudioBuffer<float> &buffer) override
    {
        auto numSamples = buffer.getNumSamples();
        auto numChannels = buffer.getNumChannels();

        // Расчет частоты вращения (Гц)
        float freqHz = params.rpm / 60.0f;
        float phaseIncrement = freqHz / (float)sampleRate;

        for (int s = 0; s < numSamples; ++s)
        {
            phase += phaseIncrement;
            if (phase >= 1.0f)
                phase -= 1.0f;

            // Моделирование цикла ДВС (окно прозрачности)
            float cycleWindow = std::sin(juce::MathConstants<float>::pi * phase);
            if (params.isFourStroke)
                cycleWindow = std::pow(cycleWindow, 2.0f);

            // Детонационные щелчки свечи
            float sparkTrigger = (juce::Random::getSystemRandom().nextFloat() < (params.sparkGap * 0.01f)) ? 1.5f : 1.0f;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float *channelData = buffer.getWritePointer(ch);

                // Эффект "зубьев" - быстрая амплитудная модуляция (грануляция)
                float teethCount = (params.chainStep == 0) ? 32.0f : (params.chainStep == 1 ? 24.0f : 18.0f);
                float teethMod = std::abs(std::sin(phase * teethCount * juce::MathConstants<float>::pi));

                // Натяжение влияет на фазовый джиттер
                float jitter = (juce::Random::getSystemRandom().nextFloat() - 0.5f) * (1.0f - params.tension) * 0.1f;

                // Результирующий "разрез"
                channelData[s] *= teethMod * cycleWindow * sparkTrigger;

                // Примешивание шума карбюратора (зависит от жиклера)
                float intakeNoise = (juce::Random::getSystemRandom().nextFloat() - 0.5f) * params.intakeGap * 0.2f;
                channelData[s] += intakeNoise * params.fuelJetSize;
            }
        }
    }

private:
    Params params;
    float phase = 0.0f;
};
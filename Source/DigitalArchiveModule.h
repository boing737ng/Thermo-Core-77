#pragma once
#include "BaseModule.h"

class DigitalArchiveModule : public BaseModule
{
public:
    struct Params
    {
        int blockSize;       // 8, 16, 32
        float quality;       // 1 - 100 (Квантование)
        float chromaPhase;   // 0 - 360
        int oversampling;    // 1x - 16x
        float glitchDensity; // Плотность сетки
        int windowType;      // Hann, Rect...
        float turbineSpeed;  // Гул турбины (RPM)
    };

    void prepare(const juce::dsp::ProcessSpec &spec) override
    {
        sampleRate = spec.sampleRate;
        turbinePhase = 0.0f;
        blockBuffer.resize(spec.numChannels, std::vector<float>(128, 0.0f));
        writePointer = 0;
    }

    void reset() override
    {
        turbinePhase = 0.0f;
        writePointer = 0;
    }

    void updateParams(const Params &p) { params = p; }

    void processInternal(juce::AudioBuffer<float> &buffer) override
    {
        auto numSamples = buffer.getNumSamples();
        auto numChannels = buffer.getNumChannels();

        // Гул турбины (несущая частота цифрового охлаждения)
        float turbineFreq = params.turbineSpeed / 60.0f;
        float turbineInc = turbineFreq / (float)sampleRate;

        for (int s = 0; s < numSamples; ++s)
        {
            turbinePhase += turbineInc;
            if (turbinePhase >= 1.0f)
                turbinePhase -= 1.0f;
            float noise = (std::sin(turbinePhase * juce::MathConstants<float>::twoPi)) * 0.01f;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float *channelData = buffer.getWritePointer(ch);

                // Накопление блока для "сжатия"
                blockBuffer[ch][writePointer] = channelData[s];

                // Когда блок заполнен, применяем "квантование JPEG"
                if (writePointer >= params.blockSize - 1)
                {
                    processBlock(ch, params.blockSize);
                }

                // Глитч-сетка (ритмичные пропуски)
                if (juce::Random::getSystemRandom().nextFloat() < params.glitchDensity * 0.01f)
                {
                    channelData[s] = 0.0f;
                }
                else
                {
                    channelData[s] = blockBuffer[ch][writePointer] + noise;
                }

                // Хрома-фаза (сдвиг стерео-каналов относительно друг друга)
                if (ch == 1)
                { // Правый канал
                    float phaseShift = std::sin(params.chromaPhase * juce::MathConstants<float>::pi / 180.0f);
                    channelData[s] *= (1.0f + phaseShift * 0.1f);
                }
            }

            writePointer++;
            if (writePointer >= params.blockSize)
                writePointer = 0;
        }
    }

private:
    Params params;
    std::vector<std::vector<float>> blockBuffer;
    int writePointer = 0;
    float turbinePhase = 0.0f;

    // Имитация квантования (упрощенное ДКП-подобие)
    void processBlock(int channel, int size)
    {
        float qStep = (101.0f - params.quality) / 100.0f;
        if (qStep <= 0.0f)
            return;

        for (int i = 0; i < size; ++i)
        {
            // Квантуем амплитуду в блоке, создавая характерные ступеньки и звон
            float &sample = blockBuffer[channel][i];
            sample = std::round(sample / qStep) * qStep;
        }
    }
};
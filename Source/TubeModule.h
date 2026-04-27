#pragma once
#include "BaseModule.h"

class TubeModule : public BaseModule
{
public:
    enum class TubeType
    {
        n6N2P,
        n6P14P,
        n6P45S,
        GU50
    };

    struct Params
    {
        TubeType type;
        float heaterVoltage;  // 5.8 - 7.2V
        float anodeVoltage;   // 150 - 450V
        float gridBias;       // -2.0 - -50.0V
        float loadResistor;   // 1k - 100k
        float thermalDrift;   // 0.0 - 1.0
        float microphonicEff; // 0.0 - 1.0
        int transformerType;  // 0: Воздух, 1: Железо, 2: Пермаллой
    };

    void prepare(const juce::dsp::ProcessSpec &spec) override
    {
        sampleRate = spec.sampleRate;
        prevAnodeCurrent.resize(spec.numChannels, 0.0f);
        thermalState.resize(spec.numChannels, 0.0f);
    }

    void reset() override
    {
        std::fill(prevAnodeCurrent.begin(), prevAnodeCurrent.end(), 0.0f);
    }

    void updateParams(const Params &p) { params = p; }

    void processInternal(juce::AudioBuffer<float> &buffer) override
    {
        auto numSamples = buffer.getNumSamples();
        auto numChannels = buffer.getNumChannels();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float *channelData = buffer.getWritePointer(ch);

            for (int s = 0; s < numSamples; ++s)
            {
                float input = channelData[s];

                // 1. Термодинамический дрейф и шум накала
                float noise = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f) * std::pow(params.heaterVoltage / 6.3f, 2.0f) * 0.001f;

                // 2. Моделирование ВАХ: I_a = k * (V_g + V_a/mu)^(3/2)
                float mu = getMuForType(params.type);
                float k = 0.002f; // Коэффициент крутизны

                float V_g = input * 10.0f + params.gridBias; // Сигнал на сетке
                float effectiveVoltage = V_g + (params.anodeVoltage / mu);

                float Ia = 0.0f;
                if (effectiveVoltage > 0.0f)
                {
                    Ia = k * std::pow(effectiveVoltage, 1.5f);
                }

                // 3. Сопротивление нагрузки и выходное напряжение
                float output = Ia * params.loadResistor * 0.001f;

                // 4. Микрофонный эффект (фидбек от корпуса)
                if (params.microphonicEff > 0.0f)
                {
                    output += std::sin(s * 0.05f) * params.microphonicEff * 0.02f;
                }

                // 5. Гистерезис трансформатора (упрощенно)
                output = applyTransformer(output, ch);

                channelData[s] = juce::jlimit(-2.0f, 2.0f, output * 0.1f);
            }
        }
    }

private:
    Params params;
    std::vector<float> prevAnodeCurrent;
    std::vector<float> thermalState;

    float getMuForType(TubeType t)
    {
        switch (t)
        {
        case TubeType::n6N2P:
            return 100.0f;
        case TubeType::n6P14P:
            return 20.0f;
        case TubeType::GU50:
            return 5.0f;
        default:
            return 40.0f;
        }
    }

    float applyTransformer(float in, int ch)
    {
        // Простая модель насыщения сердечника
        float saturation = (params.transformerType == 1) ? 0.8f : 0.95f;
        return std::tanh(in * saturation);
    }
};
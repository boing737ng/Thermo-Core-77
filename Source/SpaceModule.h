#pragma once
#include "BaseModule.h"

class SpaceModule : public BaseModule
{
public:
    struct Params
    {
        float lensMass;        // 0.0 - 10.0 M☉ (Искривление фазы)
        float eventHorizon;    // 0.0 - 1.0 (Замедление времени)
        float eccentricity;    // 0.0 - 0.9 (Орбитальный ритм)
        float keplerResonance; // 1.0, 1.5, 1.25... (Соотношение задержек)
        float pulsarDrift;     // 0.0 - 1.0 (Джиттер такта)
        float dopplerShift;    // -0.3 - 0.3 (Смещение частоты)
        float vacuumPressure;  // 0.0 - 1.0 (Рассеивание/Прозрачность)
        int packetSize;        // 1 - 1024 (Телеметрия)
    };

    void prepare(const juce::dsp::ProcessSpec &spec) override
    {
        sampleRate = spec.sampleRate;

        // Многополосная линия задержки для гравитационного эха
        juce::dsp::DelayLine<float> delay(192000);
        orbitDelay.prepare(spec);
        orbitDelay.setDelay(0.5f * (float)sampleRate);

        // Фазовый фильтр для "Линзирования"
        allPassFilter.prepare(spec);
    }

    void reset() override
    {
        orbitDelay.reset();
        allPassFilter.reset();
    }

    void updateParams(const Params &p) { params = p; }

    void processInternal(juce::AudioBuffer<float> &buffer) override
    {
        auto numSamples = buffer.getNumSamples();
        auto numChannels = buffer.getNumChannels();

        // Расчет замедления времени (Time Dilation)
        // t' = t * sqrt(1 - rs/r)
        float timeDilation = 1.0f - (params.eventHorizon * 0.5f);

        for (int s = 0; s < numSamples; ++s)
        {
            // Динамический расчет орбитальной позиции
            orbitPhase += (1.0f / (float)sampleRate) * (1.0f - params.eccentricity);
            if (orbitPhase >= 1.0f)
                orbitPhase -= 1.0f;

            // Расчет Доплера: частота меняется от радиальной скорости "планеты"
            float dopplerMod = std::sin(orbitPhase * juce::MathConstants<float>::twoPi) * params.dopplerShift;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float *channelData = buffer.getWritePointer(ch);
                float x = channelData[s];

                // 1. Гравитационное линзирование (фазовый сдвиг)
                // Масса линзы меняет коэффициенты фильтра, искривляя АЧХ
                float apFreq = juce::jlimit(10.0f, 20000.0f, 1000.0f + params.lensMass * 500.0f);
                allPassFilter.setCoefficients(juce::dsp::IIR::Coefficients<float>::makeAllPass(sampleRate, apFreq));
                x = allPassFilter.processSample(x);

                // 2. Орбитальная задержка с резонансом Кеплера
                float currentDelay = (0.2f + std::cos(orbitPhase * juce::MathConstants<float>::twoPi) * params.eccentricity) * (float)sampleRate;
                orbitDelay.setDelay(currentDelay * params.keplerResonance);

                float echo = orbitDelay.popSample(ch);
                orbitDelay.pushSample(ch, x + echo * 0.3f);

                // Смешивание с учетом эффекта Вакуума (чем выше вакуум, тем меньше ВЧ в эхе)
                x = x * (1.0f - params.vacuumPressure * 0.5f) + echo * (params.vacuumPressure);

                // 3. Телеметрия (пропуски пакетов)
                if (params.packetSize > 1)
                {
                    if ((s / params.packetSize) % 10 == 0 && (juce::Random::getSystemRandom().nextFloat() < 0.1f))
                    {
                        x *= 0.0f; // Потеря пакета
                    }
                }

                // Применяем Доплер и замедление времени через простую интерполяцию (упрощенно)
                channelData[s] = x * timeDilation * (1.0f + dopplerMod);
            }
        }
    }

private:
    Params params;
    juce::dsp::DelayLine<float> orbitDelay{192000};
    juce::dsp::IIR::Filter<float> allPassFilter;
    float orbitPhase = 0.0f;
};
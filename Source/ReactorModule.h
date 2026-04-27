#pragma once
#include "BaseModule.h"
#include <vector>
#include <cmath>

class ReactorModule : public BaseModule
{
public:
    enum class FuelType
    {
        U235,
        Pu239,
        Th232,
        MOX
    };

    struct Params
    {
        FuelType fuel;
        float rodPosition; // 0 - 1.0 (z)
        float rodSpeedIn;  // % / s
        float rodSpeedOut;
        float rodInertia;     // 0 - 1.0
        int controlStep;      // 0: Analog, 1: 1%, 2: 5%, 3: 10%
        float friction;       // 0 - 1.0
        float coolantType;    // влияет на теплоотвод
        float pressure;       // 1 - 200 atm
        float modalResonance; // амплитуда резонансов ядра
        bool safetyEnabled;   // Контейнмент
    };

    // Состояние кинетики (память реактора)
    struct KineticsState
    {
        double neutronFlux = 1.0;   // Φ
        double precursors[6] = {0}; // C_i (6 групп запаздывающих нейтронов)
        double reactivity = 0.0;    // ρ
        double temperature = 300.0; // T_core (Kelvin)
        double iodine = 0.0;        // I-135
        double xenon = 0.0;         // Xe-135 (отравитель)
        double fuelBurnup = 0.0;    // Выгорание
        float actualRodPos = 0.0;   // Реальное положение стержней с учетом инерции
    };

    void prepare(const juce::dsp::ProcessSpec &spec) override
    {
        sampleRate = spec.sampleRate;
        state = KineticsState();

        // Резонансные фильтры активной зоны (модальные резонансы)
        juce::dsp::IIR::Coefficients<float>::Ptr coeffs =
            juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 1000.0f);

        coreFilters.clear();
        for (int i = 0; i < spec.numChannels; ++i)
        {
            auto filter = std::make_unique<juce::dsp::IIR::Filter<float>>();
            filter->prepare(spec);
            coreFilters.push_back(std::move(filter));
        }

        feedbackDelayLine.prepare(spec);
        feedbackDelayLine.setDelay(0.01f * (float)sampleRate); // 10ms базовый зазор
    }

    void reset() override
    {
        state = KineticsState();
        for (auto &f : coreFilters)
            f->reset();
        feedbackDelayLine.reset();
    }

    void updateParams(const Params &p) { params = p; }

    // Получение состояния для визуализатора
    KineticsState getCurrentState() const { return state; }

    void processInternal(juce::AudioBuffer<float> &buffer) override
    {
        auto numSamples = buffer.getNumSamples();
        auto numChannels = buffer.getNumChannels();

        // Константы топлива (упрощенно для U-235)
        const double Beta = 0.0065;   // Доля запаздывающих нейтронов
        const double Lambda = 0.0001; // Время жизни поколения
        const double decayConstants[6] = {0.0124, 0.0305, 0.111, 0.301, 1.14, 3.01};
        const double betaGroups[6] = {0.00021, 0.0014, 0.0012, 0.0025, 0.00074, 0.00027};

        for (int s = 0; s < numSamples; ++s)
        {
            // 1. КИНЕТИЧЕСКИЙ ШАГ (интегрирование ПКЭ)
            updateKinetics(Beta, Lambda, decayConstants, betaGroups);

            // 2. DSP СВЯЗЬ
            // Флуктуации потока Φ создают "нейтронный" бит-креш и шум
            float neutronNoise = (float)(state.neutronFlux * 0.00001 * (juce::Random::getSystemRandom().nextFloat() - 0.5));
            float saturationThreshold = 1.0f / (float)(state.neutronFlux + 0.1);

            // Тепловой дрейф фильтра
            float cutOff = juce::jlimit(20.0f, 20000.0f, (float)(1000.0 + state.temperature * 2.0));

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float *channelData = buffer.getWritePointer(ch);
                float x = channelData[s];

                // Модальный резонанс ядра (акустика активной зоны)
                coreFilters[ch]->setCoefficients(juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, cutOff, 0.707f + params.modalResonance));
                x = coreFilters[ch]->processSample(x);

                // Рекурсивный фидбек, зависящий от реактивности
                float feedbackGain = juce::jlimit(0.0f, 0.99f, (float)(state.neutronFlux * 0.1));
                float delayed = feedbackDelayLine.popSample(ch);
                feedbackDelayLine.pushSample(ch, x + delayed * feedbackGain);
                x += delayed * feedbackGain;

                // "Радиационный" бит-флип (глитч)
                if (juce::Random::getSystemRandom().nextFloat() < (state.neutronFlux * 0.000001))
                {
                    x = std::copysign(1.0f, x); // Жесткий пробой
                }

                // Лимитер контейнмента
                if (params.safetyEnabled)
                {
                    x = std::tanh(x);
                }

                channelData[s] = x + neutronNoise;
            }
        }
    }

private:
    Params params;
    KineticsState state;
    std::vector<std::unique_ptr<juce::dsp::IIR::Filter<float>>> coreFilters;
    juce::dsp::DelayLine<float> feedbackDelayLine{192000};

    void updateKinetics(double Beta, double Lambda, const double *decay, const double *betas)
    {
        // Управление инерцией стержней
        float targetPos = params.rodPosition;
        // Квантование шага управления
        if (params.controlStep > 0)
        {
            float steps = (params.controlStep == 1) ? 100.0f : (params.controlStep == 2 ? 20.0f : 10.0f);
            targetPos = std::round(targetPos * steps) / steps;
        }

        float speed = (targetPos > state.actualRodPos) ? params.rodSpeedIn : params.rodSpeedOut;
        float inertiaFactor = 1.0f - params.rodInertia * 0.99f;
        state.actualRodPos += (targetPos - state.actualRodPos) * speed * 0.0001f * inertiaFactor;

        // Расчет реактивности ρ
        // Стержни + Температурный коэффициент + Ксеноновое отравление
        double rho_rods = (0.5 - state.actualRodPos) * 0.02;
        double rho_temp = (300.0 - state.temperature) * 0.00005;
        double rho_xe = -state.xenon * 0.001;
        state.reactivity = rho_rods + rho_temp + rho_xe;

        // Уравнение потока dΦ/dt
        double sumC = 0;
        for (int i = 0; i < 6; ++i)
            sumC += decay[i] * state.precursors[i];

        double dPhi = ((state.reactivity - Beta) / Lambda) * state.neutronFlux + sumC;
        state.neutronFlux += dPhi * (1.0 / sampleRate);
        if (state.neutronFlux < 1e-5)
            state.neutronFlux = 1e-5;

        // Уравнения предшественников dCi/dt
        for (int i = 0; i < 6; ++i)
        {
            double dC = (betas[i] / Lambda) * state.neutronFlux - decay[i] * state.precursors[i];
            state.precursors[i] += dC * (1.0 / sampleRate);
        }

        // Тепловой баланс (нагрев от потока, охлаждение от теплоносителя)
        state.temperature += (state.neutronFlux * 0.01 - (state.temperature - 300.0) * params.coolantType * 0.001);

        // Динамика Йода и Ксенона (сильно ускорено для звукового эффекта)
        double dI = state.neutronFlux * 0.001 - 0.0001 * state.iodine;
        double dXe = 0.0001 * state.iodine - (0.0002 + state.neutronFlux * 0.0005) * state.xenon;
        state.iodine += dI * (1.0 / sampleRate);
        state.xenon += dXe * (1.0 / sampleRate);

        state.fuelBurnup += state.neutronFlux * 1e-9;
    }
};
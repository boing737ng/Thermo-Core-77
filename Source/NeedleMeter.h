#pragma once
#include <JuceHeader.h>

class NeedleMeter : public juce::Component, public juce::Timer
{
public:
    NeedleMeter(juce::String name) : labelName(name) { startTimerHz(30); }

    void setValue(float newValue) { targetValue = juce::jlimit(0.0f, 1.0f, newValue); }

    void timerCallback() override
    {
        // Физика стрелки: инерция и затухание
        float spring = 0.2f;
        float damping = 0.7f;
        float force = (targetValue - currentValue) * spring;
        velocity = (velocity + force) * damping;
        currentValue += velocity;
        repaint();
    }

    void paint(juce::Graphics &g) override
    {
        auto r = getLocalBounds().toFloat();

        // Шкала
        g.setColour(juce::Colour(0xffe0e0e0)); // Грязный белый
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(juce::Colours::black);
        g.drawRoundedRectangle(r, 4.0f, 2.0f);

        auto center = r.getCentre().translated(0, r.getHeight() * 0.4f);
        float startAngle = -juce::MathConstants<float>::pi * 0.4f;
        float endAngle = juce::MathConstants<float>::pi * 0.4f;
        float angle = startAngle + currentValue * (endAngle - startAngle);

        // Рисуем деления
        for (int i = 0; i <= 10; ++i)
        {
            float a = startAngle + (i / 10.0f) * (endAngle - startAngle);
            g.drawLine(center.x + std::sin(a) * r.getWidth() * 0.6f,
                       center.y - std::cos(a) * r.getWidth() * 0.6f,
                       center.x + std::sin(a) * r.getWidth() * 0.7f,
                       center.y - std::cos(a) * r.getWidth() * 0.7f, 1.0f);
        }

        // Стрелка
        g.setColour(juce::Colours::red.withAlpha(0.8f));
        g.drawLine(center.x, center.y,
                   center.x + std::sin(angle) * r.getWidth() * 0.8f,
                   center.y - std::cos(angle) * r.getWidth() * 0.8f, 2.0f);

        g.setColour(juce::Colours::black);
        g.setFont(14.0f);
        g.drawText(juce::String::fromUTF8(labelName.toRawUTF8()), getLocalBounds(), juce::Justification::bottomCentred);
    }

private:
    juce::String labelName;
    float targetValue = 0, currentValue = 0, velocity = 0;
};
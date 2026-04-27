#pragma once
#include <JuceHeader.h>

class ThermoLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ThermoLookAndFeel()
    {
        // Палитра ОБЪЕКТА
        setColour(juce::Slider::thumbColourId, juce::Colour::fromFloatRGBA(1.0f, 0.55f, 0.0f, 1.0f)); // Оранжевый
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xff0b1a0f));                  // Темный бакелит
        setColour(juce::Label::textColourId, juce::Colours::whitesmoke);
    }

    void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                          float sliderPos, const float rotaryStartAngle,
                          const float rotaryEndAngle, juce::Slider &slider) override
    {
        auto radius = (float)juce::jmin(width / 2, height / 2) - 4.0f;
        auto centreX = (float)x + (float)width * 0.5f;
        auto centreY = (float)y + (float)height * 0.5f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Рисуем корпус ручки (сталь/бакелит)
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

        g.setColour(juce::Colours::grey.withAlpha(0.5f));
        g.drawEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f, 2.0f);

        // Указатель (насечка с оранжевой краской)
        juce::Path p;
        auto pointerLength = radius * 0.8f;
        auto pointerThickness = 3.0f;
        p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));

        g.setColour(juce::Colour(0xffff8c00));
        g.fillPath(p);

        // Текст значения в стиле ГОСТ снизу
        g.setColour(juce::Colours::orange.withAlpha(0.7f));
        g.setFont(12.0f);
        g.drawText(slider.getDisplayLabel(), x, y + height - 10, width, 10, juce::Justification::centred);
    }

    void drawButtonBackground(juce::Graphics &g, juce::Button &button, const juce::Colour &backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        g.setColour(shouldDrawButtonAsDown ? juce::Colours::darkgrey : juce::Colour(0xff2a2a2a));
        g.fillRoundedRectangle(bounds, 2.0f);
        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.drawRoundedRectangle(bounds, 2.0f, 1.5f);
    }
};
/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class ThermoCore77AudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    ThermoCore77AudioProcessorEditor (ThermoCore77AudioProcessor&);
    ~ThermoCore77AudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    ThermoCore77AudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ThermoCore77AudioProcessorEditor)
};

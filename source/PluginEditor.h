#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class SignalbashAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                         private juce::Timer,
                                         private juce::Button::Listener
{
public:
    SignalbashAudioProcessorEditor (SignalbashAudioProcessor&);
    ~SignalbashAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:

    SignalbashAudioProcessor& audioProcessor;

    void timerCallback() override;
    void buttonClicked (juce::Button* button) override;

    void mouseDown (const juce::MouseEvent &event) override;
    void mouseMove (const juce::MouseEvent &event) override;

    void updateUIForCurrentView();

    bool viewSessionKeyEnter = true;
    bool viewDefault = false;
    bool viewSettings = false;
    bool settingsDebugMode = false;

    juce::TextButton flushButton;

    juce::TextEditor sessionKeyEditor;
    juce::Label sessionKeyLabel;
    juce::TextButton submitSessionKeyButton;
    juce::TextButton copySessionKeyButton;
    juce::TextButton changeSessionKeyButton;
    juce::TextButton editSessionKeyCancelButton;
    juce::TextButton retrySessionKeyValidateButton;

    juce::ToggleButton animationActiveToggle;

    juce::Image settingsCogImage;
    juce::Rectangle<float> settingsCogBounds;
    bool settingsCogHovered = false;

    juce::Image splashLogoImage;

    juce::Image rotatingImage;
    float rotationAngle = 0.0f;
    juce::Rectangle<float> spinnerBounds;

    juce::String getObfuscatedSessionKey ();

    juce::Colour bgColor;
    juce::Colour buttonFillColor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SignalbashAudioProcessorEditor)
};

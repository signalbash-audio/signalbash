#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class SignalbashRotatingLogoComponent  : public juce::Component,
                                         private juce::Timer
{
public:
    explicit SignalbashRotatingLogoComponent (SignalbashAudioProcessor&);
    ~SignalbashRotatingLogoComponent() override;

    void setLogoImage (juce::Image);
    void setBackgroundColour (juce::Colour);
    void setActive (bool);
    void resetRotation();

    int getRequiredSideLength() const;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    SignalbashAudioProcessor& audioProcessor;
    juce::Image logoImage;
    juce::Colour backgroundColour { 0xFF1D1F21 };
    float rotationAngle = 0.0f;
    bool active = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SignalbashRotatingLogoComponent)
};

class SignalbashSubmissionProgressBarComponent  : public juce::Component,
                                                  private juce::Timer
{
public:
    explicit SignalbashSubmissionProgressBarComponent (SignalbashAudioProcessor&);
    ~SignalbashSubmissionProgressBarComponent() override;

    void setBackgroundColour (juce::Colour);
    void setActive (bool);

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    double getSmoothProgress() const;

    SignalbashAudioProcessor& audioProcessor;
    juce::Colour backgroundColour { 0xFF1D1F21 };
    bool active = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SignalbashSubmissionProgressBarComponent)
};

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
    bool shouldShowRetryButton() const;
    juce::Rectangle<int> getSettingsDebugTextBounds() const;

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

    SignalbashRotatingLogoComponent rotatingLogo;
    SignalbashSubmissionProgressBarComponent progressBar;
    juce::Rectangle<float> spinnerBounds;

    juce::String getObfuscatedSessionKey ();

    juce::Colour bgColor;
    juce::Colour buttonFillColor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SignalbashAudioProcessorEditor)
};

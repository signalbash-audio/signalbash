/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include <string>

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SignalbashAudioProcessorEditor::SignalbashAudioProcessorEditor (SignalbashAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{

    setSize (400, 300);
    startTimerHz(60);

    splashLogoImage = juce::ImageCache::getFromMemory(BinaryData::signalbash_logo_text_990x624_png, BinaryData::signalbash_logo_text_990x624_pngSize);

    settingsCogImage = juce::ImageCache::getFromMemory(BinaryData::setting_cog_20px_png, BinaryData::setting_cog_20px_pngSize);
    settingsCogBounds = juce::Rectangle<float>(370, 5, 20, 20);

    rotatingImage = juce::ImageCache::getFromMemory(BinaryData::signalbash_logo_100x_png, BinaryData::signalbash_logo_100x_pngSize);
    spinnerBounds = juce::Rectangle<float>(
                                           30 + (370 - 100) / 2.0f,
                                           (300 - 100) / 2.0f,
                                           100, 100
                                           );

    bgColor = juce::Colour(0xFF1D1F21);
    buttonFillColor =juce::Colour(0xFF222426);

    addAndMakeVisible(sessionKeyLabel);
    sessionKeyLabel.setText("Enter Session Key:", juce::dontSendNotification);

    addAndMakeVisible(sessionKeyEditor);
    sessionKeyEditor.setPasswordCharacter((juce::juce_wchar) 0x2022);
    sessionKeyEditor.setJustification(juce::Justification::centred);
    sessionKeyEditor.setTextToShowWhenEmpty("Session Key", juce::Colours::grey);
    sessionKeyEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(buttonFillColor));

    addAndMakeVisible(submitSessionKeyButton);
    submitSessionKeyButton.setButtonText("Submit");
    submitSessionKeyButton.addListener(this);
    submitSessionKeyButton.setColour(juce::TextButton::buttonColourId, juce::Colour(buttonFillColor));

    addAndMakeVisible(animationActiveToggle);
    animationActiveToggle.setToggleState(audioProcessor.enableAnimation, juce::dontSendNotification);
    animationActiveToggle.setButtonText("Enable Animation");
    animationActiveToggle.addListener(this);

    addAndMakeVisible(copySessionKeyButton);
    copySessionKeyButton.setButtonText("Copy Session Key");
    copySessionKeyButton.addListener(this);
    copySessionKeyButton.setColour(juce::TextButton::buttonColourId, juce::Colour(buttonFillColor));

    addAndMakeVisible(changeSessionKeyButton);
    changeSessionKeyButton.setButtonText("Change Session Key");
    changeSessionKeyButton.addListener(this);
    changeSessionKeyButton.setColour(juce::TextButton::buttonColourId, juce::Colour(buttonFillColor));

    addAndMakeVisible(editSessionKeyCancelButton);
    editSessionKeyCancelButton.setButtonText("Cancel");
    editSessionKeyCancelButton.addListener(this);
    editSessionKeyCancelButton.setColour(juce::TextButton::buttonColourId, juce::Colour(buttonFillColor));

    addAndMakeVisible(retrySessionKeyValidateButton);
    retrySessionKeyValidateButton.setButtonText("Session Key Not Yet Validated - Retry");
    retrySessionKeyValidateButton.addListener(this);
    retrySessionKeyValidateButton.setColour(juce::TextButton::buttonColourId, juce::Colour(buttonFillColor));

    addAndMakeVisible(flushButton);
    flushButton.setButtonText("FLUSH");
    flushButton.addListener(this);
    flushButton.setColour(juce::TextButton::buttonColourId, juce::Colour(buttonFillColor));

    if (!audioProcessor.sessionKey.isEmpty()) {
        viewSessionKeyEnter = false;
        viewDefault = true;
    }
}

SignalbashAudioProcessorEditor::~SignalbashAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void SignalbashAudioProcessorEditor::paint (juce::Graphics& g)
{

    g.fillAll (bgColor);

    g.setColour (juce::Colours::black);
    g.fillRect(0, 0,getWidth(), 30);

    juce::String statusBarMessage = "Connection Active";
    if (audioProcessor.sessionKey.isEmpty()) {
        g.setColour(juce::Colours::orange);
        statusBarMessage = "Session Key Missing";
    }
    if (!audioProcessor.sessionKey.isEmpty() && audioProcessor.connectionHealthy) {

        g.setColour(juce::Colour(0xFF00E676));
        statusBarMessage = "Connection Healthy";
    }
    if (!audioProcessor.sessionKey.isEmpty() && !audioProcessor.connectionHealthy) {
        g.setColour(juce::Colours::red);
        statusBarMessage = "Offline (No Internet or Server Maintenance In Progress)";
    }
    if (!audioProcessor.sessionKey.isEmpty() && audioProcessor.currentSessionKeyInvalid) {
        g.setColour(juce::Colours::red);
        statusBarMessage = "Invalid Session Key";
    }

    g.drawEllipse(12, 7, 16, 16, 1);
    g.fillEllipse(15, 10, 10, 10);

    g.setColour (juce::Colours::white);
    g.drawFittedText(statusBarMessage,
                     40, 5,
                     300, 20,
                     juce::Justification::centredLeft, 1
                     );

    if (!viewSessionKeyEnter) {
        if (settingsCogHovered) {
            g.setColour(juce::Colours::grey);
            g.fillEllipse(settingsCogBounds);
        }

        juce::AffineTransform settingsCogTranslate;
        settingsCogTranslate = juce::AffineTransform::translation(370, 5);
        g.drawImageTransformed(settingsCogImage, settingsCogTranslate, false);
    }

    if (viewSessionKeyEnter) {
        g.setColour (juce::Colours::white);
        g.setFont (18.0f);

        juce::String promptMessage;
        if (audioProcessor.sessionKey.isEmpty()) {
            promptMessage = "Please Enter Your Session Key.";
            editSessionKeyCancelButton.setVisible(false);

            g.drawFittedText("Get your session key by logging into your account at signalbash.com",
                             100, 150 + 40 + 20,
                             200, 40,
                             juce::Justification::centred, 3
                             );

        } else {
            promptMessage = "Enter New Session Key";
            editSessionKeyCancelButton.setVisible(true);
        }

        g.drawImageWithin(splashLogoImage, 112, 40, 175, 90, juce::RectanglePlacement::xMid);

        flushButton.setVisible(false);
        animationActiveToggle.setVisible(false);
        sessionKeyEditor.setVisible(true);
        sessionKeyLabel.setText(promptMessage, juce::dontSendNotification);
        sessionKeyLabel.setVisible(true);
        submitSessionKeyButton.setVisible(true);
        copySessionKeyButton.setVisible(false);
        changeSessionKeyButton.setVisible(false);
        retrySessionKeyValidateButton.setVisible(false);

    }

    if (viewSettings) {

        flushButton.setVisible(true);
        sessionKeyEditor.setVisible(false);
        sessionKeyLabel.setVisible(false);
        submitSessionKeyButton.setVisible(false);
        animationActiveToggle.setVisible(true);
        copySessionKeyButton.setVisible(true);
        changeSessionKeyButton.setVisible(true);
        editSessionKeyCancelButton.setVisible(false);
        retrySessionKeyValidateButton.setVisible(false);

        g.setColour (juce::Colours::white);

        auto bounds = getLocalBounds();
        bounds.removeFromTop(40); bounds.removeFromLeft(40); bounds.removeFromRight(40);

        g.setFont (juce::FontOptions (17.0f));

        g.drawFittedText(settingsDebugMode ? "Settings (debug mode) - v" + audioProcessor._PLUGIN_VERSION  : "Settings",
                         bounds.removeFromTop(30),
                         juce::Justification::centredLeft, 1
                         );
        bounds.removeFromTop(5);
        g.setFont (juce::FontOptions (15.0f));
        g.drawFittedText("Host: " + audioProcessor.hostName,
                         bounds.removeFromTop(20),
                         juce::Justification::centredLeft, 1);
        if (settingsDebugMode) {
            g.drawFittedText("Current Time (UTC): " + audioProcessor.submissionWindowTimer.getCurrentUTCDateAsString(),
                             bounds.removeFromTop(20),
                             juce::Justification::centredLeft, 1);
            g.drawFittedText ("Pending Activity: " + juce::String(audioProcessor.activity.load()),
                              bounds.removeFromTop(20),
                              juce::Justification::centredLeft, 1);
        }
        g.drawFittedText ("Session Key: " + getObfuscatedSessionKey().toUpperCase(),
                          bounds.removeFromTop(20),
                          juce::Justification::centredLeft, 1);
        auto sessKeyBounds = bounds.removeFromTop(20);
        copySessionKeyButton.setBounds(sessKeyBounds.removeFromLeft(sessKeyBounds.getWidth() / 2));
        changeSessionKeyButton.setBounds(sessKeyBounds);

        g.drawFittedText ("Animation:",
                          bounds.removeFromTop(40),
                          juce::Justification::centredLeft, 1);
        animationActiveToggle.setBounds(bounds.removeFromTop(20));

        g.fillRect(0, 30,
                   getWidth() * audioProcessor.submissionWindowTimer.getProgress() / 100,
                   2);
        if (settingsDebugMode) {
            flushButton.setVisible(true);
        } else {
            flushButton.setVisible(false);
        }
    }

    if (viewDefault) {

        flushButton.setVisible(false);
        sessionKeyEditor.setVisible(false);
        sessionKeyLabel.setVisible(false);
        submitSessionKeyButton.setVisible(false);
        animationActiveToggle.setVisible(false);
        copySessionKeyButton.setVisible(false);
        changeSessionKeyButton.setVisible(false);
        editSessionKeyCancelButton.setVisible(false);
        retrySessionKeyValidateButton.setVisible(false);

        float centerX = getWidth() / 2.0f;
        float centerY = 30 + (getHeight() - 30) / 2.0f;

        float halfWidth = rotatingImage.getWidth() / 2.0f;
        float halfHeight = rotatingImage.getHeight() / 2.0f;

        juce::AffineTransform transform;

        transform = juce::AffineTransform::translation(-halfWidth, -halfHeight);

        if (audioProcessor.enableAnimation) {
            transform = transform.rotated(juce::degreesToRadians(rotationAngle));
        }

        transform = transform.translated(centerX, centerY);

        g.drawImageTransformed(rotatingImage, transform, false);

        if (
            (!audioProcessor.connectionHealthy && !audioProcessor.sessionKeyValidated) ||
            (!audioProcessor.currentSessionKeyInvalid && !audioProcessor.sessionKeyValidated && audioProcessor.sessionKey.isEmpty())
            ) {
            retrySessionKeyValidateButton.setVisible(true);
        }
        if (!audioProcessor.currentSessionKeyInvalid && !audioProcessor.sessionKeyValidated) {
            retrySessionKeyValidateButton.setVisible(true);
        }
    }

}

void SignalbashAudioProcessorEditor::resized()
{

    auto area = getLocalBounds();
    flushButton.setBounds(area.removeFromBottom(40).reduced(10));

    area.removeFromTop(30 + 110); area.removeFromLeft(20); area.removeFromRight(20);
    sessionKeyLabel.setBounds(area.removeFromTop(20));
    sessionKeyEditor.setBounds(area.removeFromTop(20));
    area.removeFromTop(5);
    submitSessionKeyButton.setBounds(area.removeFromTop(20));

    editSessionKeyCancelButton.setBounds(area.removeFromBottom(20));

    animationActiveToggle.setBounds(area.removeFromTop(20));

    retrySessionKeyValidateButton.setBounds(getLocalBounds().removeFromBottom(40).reduced(10));

    DBG("resized() called");

}

void SignalbashAudioProcessorEditor::timerCallback()
{

    if (audioProcessor.enableAnimation && audioProcessor.signalHot) {
        rotationAngle += 2.0f;
        if (rotationAngle >= 360.0f) {
            rotationAngle -= 360.0f;
        }
    }
    repaint();
}

void SignalbashAudioProcessorEditor::buttonClicked (juce::Button* button)
{
    if (button == &flushButton)
    {
        audioProcessor.flushAccumulator();
    }

    if (button == &submitSessionKeyButton)
    {
        juce::String newSessionKey = sessionKeyEditor.getText().trim().removeCharacters("-");
        if (newSessionKey.isEmpty() || newSessionKey.length() < 12) {
            DBG("Invalid Session Key");
            return;
        }
        DBG("Session Key Submitted: " << newSessionKey);
        audioProcessor.setSessionKey(newSessionKey);
        if (!newSessionKey.isEmpty()) {
            viewSessionKeyEnter = false;
            viewDefault = true;
            viewSettings = false;
        }
        else {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Error", "Session Key cannot be empty");
        }
    }

    if (button == &animationActiveToggle) {
        DBG("animationActiveToggle pressed");

        if (button->getToggleState()) {
            DBG("Toggle Button Now Active");
            audioProcessor.toggleAnimationEnabled(true);
        } else {
            DBG("Toggle Button Not Active");
            audioProcessor.toggleAnimationEnabled(false);
            rotationAngle = 0.0f;
        }
    }

    if (button == &copySessionKeyButton) {
        juce::SystemClipboard::copyTextToClipboard(audioProcessor.sessionKey.toUpperCase());
        DBG("Copied To Clipboard");
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Info", "Copied Session Key");
    }

    if (button == &changeSessionKeyButton) {

        if (audioProcessor.isCurrentSessionKeyValidated()) {
            sessionKeyEditor.clear();
        }

        viewSessionKeyEnter = true;
        viewDefault = false;
        viewSettings = false;
    }

    if (button == &editSessionKeyCancelButton) {
        viewSessionKeyEnter = false;
        viewDefault = false;
        viewSettings = true;
    }

    if (button == &retrySessionKeyValidateButton) {
        audioProcessor.validateSessionKey();
    }
}

void SignalbashAudioProcessorEditor::mouseDown (const juce::MouseEvent &event) {

    DBG ("Clicked at: " << event.getPosition().toString());

    if (spinnerBounds.contains(event.position)) {
        rotationAngle = 0.0;
    }

    if (settingsCogBounds.contains(event.position)) {

        if (event.mods.isShiftDown()) {
            DBG("SHIFT KEY IS DOWN DURING EVENT");
        }

        if (viewSessionKeyEnter) {
            viewSessionKeyEnter = false;
            viewDefault = true;
            settingsDebugMode = false;
            DBG("Switching to viewDefault");
        } else if (viewDefault) {
            if (event.mods.isShiftDown()) {
                DBG("Shift Key Detected: Activating Debug Mode Settings");
                settingsDebugMode = true;
            }
            viewDefault = false;
            viewSettings = true;
            DBG("Switching to viewSettings");
        } else if (viewSettings) {
            viewDefault = true;
            viewSettings = false;
            settingsDebugMode = false;
            DBG("Switching to viewDefault");
        } else {

        }
    }
}

void SignalbashAudioProcessorEditor::mouseMove (const juce::MouseEvent &event) {

    if (settingsCogBounds.contains(event.position)) {
        settingsCogHovered = true;
    } else {
        settingsCogHovered = false;
    }

}

juce::String SignalbashAudioProcessorEditor::getObfuscatedSessionKey () {

    auto sessionKey = audioProcessor.sessionKey;

    if (sessionKey.length() <= 8)
        return sessionKey;

    auto firstFour = sessionKey.substring(0, 4);
    auto lastFour = sessionKey.substring(sessionKey.length() - 4);

    auto middleLength = sessionKey.length() - 8;
    juce::String middleBullets = juce::String::repeatedString("•", middleLength);

    return firstFour + middleBullets + lastFour;
}

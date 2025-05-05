#pragma once

#include <cstdint>
#include <unordered_map>
#include <atomic>
#include <memory>
#include <JuceHeader.h>
#include "CurrentElapsedTimeProgress.h"

//==============================================================================
/**
*/
class SignalbashAudioProcessor  : public juce::AudioProcessor, public juce::Timer
{
public:
    //==============================================================================
    SignalbashAudioProcessor();
    ~SignalbashAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    std::atomic<int> activity;
    void flushAccumulator ();

    const double minDbThreshold = -60.0;

    const int activityDetectionWindow = 10;
    const int submissionAccumulatorWindow = 2 * 60;

    CurrentElapsedTimeProgress activityWindowTimer;
    CurrentElapsedTimeProgress submissionWindowTimer;

    int64_t currentActivityBlock;
    int64_t currentSubmissionBlock;

    std::atomic<bool> signalHot{false};

    int currentRecordedActivityMilliseconds;

    std::unordered_map<int, int> activityBlocks;

    void timerCallback() override;

    std::string _PLUGIN_VERSION = "1.1.0";

    bool hostNameInitialized = false;
    std::string hostName = "Unknown";
    std::string hostNameDisplay = "Unknown";
    void parseHost();

    #if JUCE_DEBUG
    std::string apiBase = "http://127.0.0.1:7575";
    #else
    std::string apiBase = "https://api.signalbash.com";
    #endif
    void commitActivity (bool immediateSubmit);
    int lastSuccessfullySubmittedBlock = -1;
    int64_t lastProcessBlockCallTimestamp = -1;

    juce::String uaheader = "JUCE_PLUGIN";

    std::string generateDedupID();
    std::string deduplicationID;

    std::atomic<bool> connectionHealthy{true};
    void checkConnectionHealth();

    juce::CriticalSection mutex;
    juce::ThreadPool threadPool;

    juce::String sessionKey;
    std::atomic<bool> enableAnimation{true};
    std::unique_ptr<juce::PropertiesFile> propertiesFile;

    std::atomic<bool> sessionKeyValidated{false};
    std::atomic<bool> currentSessionKeyInvalid{false};
    void validateSessionKey();
    void saveValidSessionKeyState();
    bool isCurrentSessionKeyValidated ();

    juce::String getSessionKey() const { return sessionKey; }
    void setSessionKey(const juce::String& newSessionKey);
    void loadSessionKeyFromFile();
    void saveSessionKeyToFile();

    void toggleAnimationEnabled (bool state);

    juce::AudioParameterBool *bypassParam = nullptr;
    juce::AudioProcessorParameter *getBypassParameter() const override { return bypassParam; }

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SignalbashAudioProcessor)
    JUCE_DECLARE_WEAK_REFERENCEABLE(SignalbashAudioProcessor)

    #if JUCE_ARM
    bool isARM = true;
    #else
    bool isARM = false;
    #endif

};

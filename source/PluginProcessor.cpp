#include <string>
#include <cmath>
#include <ctime>
#include <random>

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "RestRequest.h"

class BackgroundJob : public juce::ThreadPoolJob
{
public:
    BackgroundJob(std::function<void()> taskToRun)
        : ThreadPoolJob("BackgroundJob"), task(std::move(taskToRun))
    {
    }

    JobStatus runJob() override
    {
        task();
        return jobHasFinished;
    }

private:
    std::function<void()> task;
};

//==============================================================================
SignalbashAudioProcessor::SignalbashAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
: AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
, activityWindowTimer(10), submissionWindowTimer(120), threadPool(2)
{

    activity = 0;

    currentActivityBlock = activityWindowTimer.getCurrentBlockTimestamp();
    currentSubmissionBlock = submissionWindowTimer.getCurrentBlockTimestamp();
    currentRecordedActivityMilliseconds = 0;

    deduplicationID = generateDedupID();

    startTimerHz(2);

    bypassParam = new juce::AudioParameterBool({"bypass", 1}, "Bypass", 0);
    addParameter(bypassParam);

    juce::PropertiesFile::Options options;
    options.applicationName     = "signalbash_config";
    options.filenameSuffix      = "settings";
    options.folderName          = "Signalbash";
    options.osxLibrarySubFolder = "Application Support";

    auto propertiesFileFile = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                              .getChildFile(options.folderName)
                              .getChildFile(options.applicationName + "." + options.filenameSuffix);

    propertiesFile = std::make_unique<juce::PropertiesFile>(propertiesFileFile, options);

    auto filePath = propertiesFile->getFile().getFullPathName();
    DBG("Properties File Path: " << filePath);

    loadSessionKeyFromFile();

    parseHost();
    checkConnectionHealth();
}

SignalbashAudioProcessor::~SignalbashAudioProcessor()
{
    threadPool.removeAllJobs(true, 100);
    stopTimer();

    if (propertiesFile != nullptr)
    {
        propertiesFile->saveIfNeeded();
        propertiesFile = nullptr;
    }
}

//==============================================================================
const juce::String SignalbashAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SignalbashAudioProcessor::acceptsMidi() const
{
    return false;
}

bool SignalbashAudioProcessor::producesMidi() const
{
    return false;
}

bool SignalbashAudioProcessor::isMidiEffect() const
{
    return false;
}

double SignalbashAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SignalbashAudioProcessor::getNumPrograms()
{
    return 1;

}

int SignalbashAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SignalbashAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SignalbashAudioProcessor::getProgramName (int index)
{
    return {};
}

void SignalbashAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void SignalbashAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{

}

void SignalbashAudioProcessor::releaseResources()
{

}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SignalbashAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else

    if (layouts.getMainInputChannelSet().isDisabled())
        return false;

    #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    #endif

    return true;
  #endif
}
#endif

void SignalbashAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    {
        const juce::ScopedLock lock(mutex);

        if (lastSuccessfullySubmittedBlock != -1) {

            std::vector<int> keysToRemove;
            for (const auto& [key, value] : activityBlocks) {
                if (key <= lastSuccessfullySubmittedBlock) {
                    keysToRemove.push_back(key);
                }
            }
            for (auto key : keysToRemove) {
                activityBlocks.erase(key);
                DBG("Removed Activity Key: " << key);
            }

            lastSuccessfullySubmittedBlock = -1;
        }
    }

    if (currentActivityBlock != activityWindowTimer.getCurrentBlockTimestamp() && currentRecordedActivityMilliseconds > 0) {

        const juce::ScopedLock lock(mutex);
        activityBlocks.emplace(currentActivityBlock, currentRecordedActivityMilliseconds);
        currentActivityBlock = activityWindowTimer.getCurrentBlockTimestamp();
        currentRecordedActivityMilliseconds = 0;
    }

    if (currentSubmissionBlock != submissionWindowTimer.getCurrentBlockTimestamp()) {

        commitActivity(false);

        currentSubmissionBlock = submissionWindowTimer.getCurrentBlockTimestamp();
    }

    if (bypassParam != nullptr && bypassParam->get()) {
        signalHot.store(false);
        lastProcessBlockCallTimestamp = juce::Time::currentTimeMillis();
        return;
    }

    auto numSamples = buffer.getNumSamples();
    bool hasNonZeroData = false;

    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        auto rmsMagnitude = buffer.getRMSLevel(channel, 0, numSamples);

        double db = -196.0;
        if (rmsMagnitude > 0.0f) {
            db = 20 * std::log10(rmsMagnitude);
        }
        if (db >= minDbThreshold) {
            hasNonZeroData = true;
        }
        if (hasNonZeroData) {
            break;
        }
    }

    if (hasNonZeroData) {
        signalHot.store(true);
        ++activity;
        auto chunkDurationMilliseconds = numSamples / getSampleRate() * 1000;
        currentRecordedActivityMilliseconds += static_cast<int>(chunkDurationMilliseconds);
    } else {
        signalHot.store(false);
    }
    lastProcessBlockCallTimestamp = juce::Time::currentTimeMillis();
}

void SignalbashAudioProcessor::flushAccumulator () {

    for (const auto& [key, value] : activityBlocks) {
        DBG("Key: " << key << ", Value: " << value);
    }

    commitActivity(true);
}

void SignalbashAudioProcessor::parseHost () {

    if (!hostNameInitialized) {
        juce::PluginHostType type;
        hostName = type.getHostDescription();

        if (type.isAbletonLive()) {
            auto hostPath = type.getHostPath();
            auto hostFilename = juce::File(hostPath).getFileName();
            if (hostPath.containsIgnoreCase("Live 12") || hostFilename.containsIgnoreCase("Live 12")) {
                hostName = "Live 12";
            }
            if (hostPath.containsIgnoreCase("Live 13") || hostFilename.containsIgnoreCase("Live 13")) {
                hostName = "Live 13";
            }
        }

        #if JUCE_WINDOWS

        auto hostVersion = juce::File::getSpecialLocation(juce::File::hostApplicationPath).getVersion();
        juce::String arch = isARM ? "arm64" : "x64";

        uaheader = juce::String(hostName) + "/" + juce::String(hostVersion) + " - " + juce::SystemStats::getOperatingSystemName() + "/" + arch;
        #endif

        if (hostName == "ProTools") {
            hostNameDisplay = "Pro Tools";
        } else if (hostName == "Live 12") {
            hostNameDisplay = "Ableton Live 12";
        } else if (hostName == "Live 13") {
            hostNameDisplay = "Ableton Live 13";
        } else if (hostName == "FruityLoops") {
            hostNameDisplay = "FL Studio";
        } else if (hostName == "Apple Logic") {
            hostNameDisplay = "Logic Pro";
        } else {
            hostNameDisplay = hostName;
        }

        hostNameInitialized = true;
    }
}

void SignalbashAudioProcessor::timerCallback () {
    activityWindowTimer.update();
    submissionWindowTimer.update();

    if (activity > 0 && submissionWindowTimer.getCurrentBlockTimestamp() - currentSubmissionBlock > 1) {

        DBG("Timer curr block: " << activityWindowTimer.getCurrentBlockTimestamp() << " | Curr registered submission block: " << currentSubmissionBlock);

        if (juce::Time::currentTimeMillis() - lastProcessBlockCallTimestamp > 10000) {

            DBG("Curr ms: " << juce::Time::currentTimeMillis() << " | lastProcessBlockCallTimestamp: " << lastProcessBlockCallTimestamp);

            DBG("Fallback Activity Submission Triggered");

            commitActivity(false);

            currentSubmissionBlock = submissionWindowTimer.getCurrentBlockTimestamp();
        } else {
            DBG("Fallback Activity Submission Not activated");
        }
    }

    if (signalHot.load() && activityWindowTimer.getCurrentBlockTimestamp() - currentActivityBlock > 1) {
        signalHot.store(false);
    }

}

//==============================================================================
bool SignalbashAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* SignalbashAudioProcessor::createEditor()
{
    return new SignalbashAudioProcessorEditor (*this);
}

//==============================================================================
void SignalbashAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{

}

void SignalbashAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{

}

//==============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SignalbashAudioProcessor();
}

void SignalbashAudioProcessor::checkConnectionHealth ()
{
    auto targetEndpoint = apiBase + "/ping";

    auto weakThis = juce::WeakReference<SignalbashAudioProcessor>(this);
    std::function<void()> requestTask = [weakThis, targetEndpoint]()
    {
        if (weakThis == nullptr) return;

        int maxAttempts = 5;
        int currAttempt = 1;
        while (currAttempt <= maxAttempts) {
            if (weakThis == nullptr) break;

            RestRequest request;
            request.header("Content-Type", "application/json");
            RestRequest::Response response = request.get(targetEndpoint).execute();

            if (response.status == 200) {
                if (auto* proc = weakThis.get()) {
                    DBG("/ping => Connection Healthy");
                    proc->connectionHealthy.store(true);
                }
                break;
            }
            else if (response.result.getErrorMessage() == "No internet connection") {
                if (auto* proc = weakThis.get()) {
                    proc->connectionHealthy.store(false);
                    DBG("No internet detected.");
                }
            }
            else if (response.result.getErrorMessage() == "Server is offline or unreachable") {
                if (auto* proc = weakThis.get()) {
                    proc->connectionHealthy.store(false);
                    DBG("Server is temporarily offline or unreachable");
                }
            }

            if (weakThis == nullptr) break;
            juce::Thread::sleep(currAttempt * currAttempt * 1000);
            currAttempt += 1;
        }
    };
    threadPool.addJob(new BackgroundJob(requestTask), true);
}

void SignalbashAudioProcessor::commitActivity (bool immediateSubmit)
{
    if (sessionKey.isEmpty()) {
        DBG("Session key is not set, cannot submit activity");
        return;
    }

    if (activityBlocks.empty() || activity == 0) {
        DBG("No Pending Activity to commit.");
        return;
    }

    std::unordered_map<int, int> activityBlocksCopy;
    int mostRecentBlock = -1;
    {
        const juce::ScopedLock lock(mutex);
        activityBlocksCopy = activityBlocks;
    }

    juce::StringPairArray parameters;

    parameters.set("host", hostName);
    parameters.set("session_key", sessionKey);
    parameters.set("version", _PLUGIN_VERSION);
    parameters.set("deduplication_id", deduplicationID);
    parameters.set("ua", uaheader);

    DBG(parameters.getDescription());

    auto* activityDictObj = new juce::DynamicObject();
    for (const auto& [key, value] : activityBlocksCopy) {

        if (key > mostRecentBlock) {
            mostRecentBlock = key;
        }
        activityDictObj->setProperty(juce::String(key), juce::var(value));
    }
    juce::var activityVals = juce::var(activityDictObj);

    auto endpoint = apiBase + "/submit";

    auto weakThis = juce::WeakReference<SignalbashAudioProcessor>(this);

    std::function<void()> requestTask = [weakThis, parameters, activityVals, mostRecentBlock, endpoint, immediateSubmit]()
    {
        if (weakThis == nullptr) return;

        if (!immediateSubmit) {

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> distr(0, 10000);

            int randomSleepTime = distr(gen);
            juce::Thread::sleep(randomSleepTime);
            if (weakThis == nullptr) return;
        }

        int maxAttempts = 5;
        int currAttempt = 1;

        while (currAttempt <= maxAttempts) {
            if (weakThis == nullptr) break;

            if (auto* proc = weakThis.get()) {
                if (proc->activity == 0) {
                    return;
                }
            }

            RestRequest request;
            request.header("Content-Type", "application/json");
            #if JUCE_WINDOWS
            request.header("User-Agent", parameters["ua"]);
            #endif
            RestRequest::Response response = request.post(endpoint)
                .field("host", parameters["host"])
                .field("plugin_version", parameters["version"])
                .field("session_key", parameters["session_key"])
                .field("dd_id", parameters["deduplication_id"])
                .field("activity", activityVals)
                .execute();

            if (response.status == 200) {
                if (auto* proc = weakThis.get()) {
                    DBG("Activity Data Submitted! Resetting");
                    const juce::ScopedLock lock(proc->mutex);
                    proc->lastSuccessfullySubmittedBlock = mostRecentBlock;
                    proc->activity.exchange(0);
                    proc->connectionHealthy.store(true);
                }
                return;
            }
            else if (response.status == 429) {
                if (auto* proc = weakThis.get()) {
                    DBG("429 - Rate Limited. Will retry next pass.");
                    proc->connectionHealthy.store(true);
                }
                if (weakThis == nullptr) break;
                juce::Thread::sleep(currAttempt * currAttempt * 1000);
                currAttempt += 1;
            }
            else if (response.status == 0) {
                if (auto* proc = weakThis.get()) {
                    DBG("Internet connection down or Server Offline");
                    proc->connectionHealthy.store(false);
                }
                if (weakThis == nullptr) break;
                juce::Thread::sleep(currAttempt * currAttempt * 1000);
                currAttempt += 1;
            }
            else {
                DBG(response.bodyAsString);
                DBG(response.result.getErrorMessage());
                DBG("Status Code: " << response.status);
                DBG("Generic Request Error. Sleeping, then retrying");
                if (weakThis == nullptr) break;
                juce::Thread::sleep(currAttempt * currAttempt * 1000);
                currAttempt += 1;
            }
        }

        if (auto* proc = weakThis.get()) {
            proc->checkConnectionHealth();
        }
        DBG("Request Attempt Exhaustion.");
    };

    threadPool.addJob(new BackgroundJob(requestTask), true);
}

void SignalbashAudioProcessor::validateSessionKey ()
{
    if (sessionKey.isEmpty()) {
        DBG("No Session Key Set. Cannot validate.");
        return;
    }

    if (sessionKeyValidated.load()) {
        DBG("Session Key Validated. Nothing to do.");
        return;
    }

    juce::StringPairArray parameters;
    parameters.set("session_key", sessionKey);
    parameters.set("version", _PLUGIN_VERSION);
    parameters.set("ua", uaheader);

    auto targetEndpoint = apiBase + "/validate-session-key";

    auto weakThis = juce::WeakReference<SignalbashAudioProcessor>(this);
    std::function<void()> requestTask = [weakThis, parameters, targetEndpoint]()
    {
        if (weakThis == nullptr) return;

        RestRequest request;
        request.header("Content-Type", "application/json");
        #if JUCE_WINDOWS
        request.header("User-Agent", parameters["ua"]);
        #endif
        RestRequest::Response response = request.post(targetEndpoint)
            .field("plugin_version", parameters["version"])
            .field("session_key", parameters["session_key"])
            .execute();

        if (response.status == 200)
        {
            if (auto* proc = weakThis.get()) {
                DBG("Session Key Validated!");
                proc->currentSessionKeyInvalid.store(false);
                proc->sessionKeyValidated.store(true);
                proc->saveValidSessionKeyState();
                proc->connectionHealthy.store(true);
            }
        }
        else if (response.status == 404) {
            if (auto* proc = weakThis.get()) {
                DBG("Unknown Session Key");
                proc->currentSessionKeyInvalid.store(true);
                proc->sessionKeyValidated.store(false);
                proc->connectionHealthy.store(true);
            }
        }
        else if (response.status == 429)
        {
            if (auto* proc = weakThis.get()) {
                DBG("429 - Rate Limited. Will retry next pass.");
                proc->connectionHealthy.store(true);
            }
        }
        else if (response.status == 0) {
            if (auto* proc = weakThis.get()) {
                DBG("No Internet or Server Offline");
                proc->connectionHealthy.store(false);
                proc->checkConnectionHealth();
            }
        }
    };
    threadPool.addJob(new BackgroundJob(requestTask), true);
}

void SignalbashAudioProcessor::loadSessionKeyFromFile()
{
    if (propertiesFile != nullptr) {
        sessionKey = propertiesFile->getValue("sessionKey", "");
        DBG("Loaded Session Key From File: " << sessionKey);

        enableAnimation.store(propertiesFile->getBoolValue("animationEnabled", true));
        if (enableAnimation.load()) {
            DBG("Loaded Pref => Animation Enabled: ON");
        } else {
            DBG("Loaded Pref => Animation Enabled: OFF");
        }

        if (!sessionKey.isEmpty()) {
            auto sessionKeyValidatedKey = sessionKey.toUpperCase() + "_validity";
            sessionKeyValidated.store(propertiesFile->getBoolValue(sessionKeyValidatedKey));
        }
    }
}

void SignalbashAudioProcessor::saveSessionKeyToFile()
{
    if (propertiesFile != nullptr) {
        propertiesFile->setValue("sessionKey", sessionKey);
        propertiesFile->saveIfNeeded();
    }
}

void SignalbashAudioProcessor::saveValidSessionKeyState()
{
    if (propertiesFile != nullptr) {
        auto sessionKeyValidatedKey = sessionKey.toUpperCase() + "_validity";
        propertiesFile->setValue(sessionKeyValidatedKey, true);
        propertiesFile->saveIfNeeded();
    }
}

void SignalbashAudioProcessor::setSessionKey(const juce::String& newSessionKey)
{
    sessionKey = newSessionKey;
    saveSessionKeyToFile();

    auto sessionKeyValidatedKey = sessionKey.toUpperCase() + "_validity";
    if (propertiesFile != nullptr && !propertiesFile->getBoolValue(sessionKeyValidatedKey)) {
        sessionKeyValidated.store(false);
        validateSessionKey();
    } else {
        currentSessionKeyInvalid.store(false);
        sessionKeyValidated.store(true);
    }
}

bool SignalbashAudioProcessor::isCurrentSessionKeyValidated ()
{
    if (sessionKey.isEmpty()) {
        return false;
    }

    if (currentSessionKeyInvalid.load()) {
        return true;
    }

    return sessionKeyValidated.load();
}

void SignalbashAudioProcessor::toggleAnimationEnabled (bool state)
{
    enableAnimation.store(state);
    if (propertiesFile != nullptr) {
        propertiesFile->setValue("animationEnabled", state);
        propertiesFile->saveIfNeeded();
    }
}

std::string SignalbashAudioProcessor::generateDedupID()
{
    static unsigned int seed = static_cast<unsigned int>(std::time(nullptr));
    std::string result;
    result.reserve(8);

    for (int i = 0; i < 8; ++i) {
        seed = (1103515245 * seed + 12345);
        char digit = '0' + (seed % 10);
        result += digit;
    }
    return result;
}

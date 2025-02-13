#pragma once

#include <cstdint>
#include <JuceHeader.h>

class CurrentElapsedTimeProgress
{
public:
    CurrentElapsedTimeProgress(int totalDurSeconds) : totalDurationSeconds(totalDurSeconds)
    {
        _totalDurationSeconds64 = static_cast<int64_t>(totalDurationSeconds);
        localTimezoneOffsetSeconds = juce::Time::getCurrentTime().getUTCOffsetSeconds();

        totalDurationSecondsInvert = 1.0 / static_cast<double>(totalDurationSeconds);

        update();
    }

    void update() {

        nowTimeStamp = juce::Time::currentTimeMillis() * oneThouInvert;

        std::lldiv_t divmod = std::lldiv(nowTimeStamp, _totalDurationSeconds64);

        progress = 100.0 * ( static_cast<double>(divmod.rem) * totalDurationSecondsInvert);

        if (divmod.quot != blockNumber) {
            blockNumber = divmod.quot;
            blockTimestamp = divmod.quot * totalDurationSeconds;
        }
    }

    juce::String getCurrentUTCDateAsString () {
        auto now = juce::Time::getCurrentTime();
        auto offset = juce::RelativeTime::seconds(localTimezoneOffsetSeconds);
        auto currTime = now - offset;
        return currTime.formatted("%Y-%m-%d %H:%M:%S");
    }

    juce::String getCurrentLocalDateTimeAsString () {
        return juce::Time::getCurrentTime().formatted("%Y-%m-%d %H:%M:%S");
    }

    std::string getTimestamp () {
        return std::to_string(juce::Time::currentTimeMillis() / 1000);
    }

    void recalcLocalOffset () {
        localTimezoneOffsetSeconds = juce::Time::getCurrentTime().getUTCOffsetSeconds();
    }

    double getProgress() const { return progress; }

    int64_t getCurrentBlock () const { return blockNumber; }
    int64_t getCurrentBlockTimestamp () const { return blockTimestamp; }

private:
    int totalDurationSeconds;
    int64_t _totalDurationSeconds64;
    int localTimezoneOffsetSeconds;
    double progress = 0.0;
    int64_t blockTimestamp = 0;
    int64_t blockNumber = 0;

    double oneThouInvert = 0.001;
    double totalDurationSecondsInvert;

    int64_t nowTimeStamp = juce::Time::getCurrentTime().toMilliseconds() / 1000;
};

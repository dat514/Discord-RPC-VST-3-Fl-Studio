#pragma once

#include <JuceHeader.h>

class DawUiStateDetector
{
public:
    DawUiStateDetector() = default;

    struct Snapshot
    {
        bool hostActive = false;
        juce::String surface;
        juce::String projectName;
        juce::String focusedPlugin;
        juce::String mixerInsert;
        juce::String rawWindowTitle;
    };

    Snapshot scan();

private:
    Snapshot lastClickedSnapshot;
    int64_t lastClickMs = 0;
    bool hasLastClickedSnapshot = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DawUiStateDetector)
};

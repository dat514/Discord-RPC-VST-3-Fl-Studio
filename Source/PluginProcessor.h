#pragma once

#include <JuceHeader.h>
#include "DawUiStateDetector.h"
#include "DiscordIpcClient.h"

class DiscordRichPresenceAudioProcessor final : public juce::AudioProcessor,
                                                private juce::Timer
{
public:
    DiscordRichPresenceAudioProcessor();
    ~DiscordRichPresenceAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock(juce::AudioBuffer<double>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState();

    void setPresenceProperty(const juce::Identifier& name, const juce::String& value);
    juce::String getPresenceProperty(const juce::Identifier& name) const;
    juce::String getDiscordStatusText() const;
    juce::String getPreviewText() const;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    struct TransportSnapshot
    {
        std::atomic<bool> hasSeconds { false };
        std::atomic<double> seconds { 0.0 };

        std::atomic<bool> hasPpq { false };
        std::atomic<double> ppq { 0.0 };

        std::atomic<bool> hasBpm { false };
        std::atomic<double> bpm { 0.0 };

        std::atomic<bool> hasTimeSignature { false };
        std::atomic<int> numerator { 4 };
        std::atomic<int> denominator { 4 };

        std::atomic<bool> isPlaying { false };
        std::atomic<bool> isRecording { false };
        std::atomic<bool> isLooping { false };
        std::atomic<double> sampleRate { 0.0 };
    };

    template <typename FloatType>
    void processAudioBlock(juce::AudioBuffer<FloatType>& buffer);

    void updateTransportSnapshot();
    void timerCallback() override;

    bool parameterEnabled(const char* parameterId) const;
    void loadGlobalSettings();
    void saveGlobalSettingsIfChanged();
    juce::var buildDiscordActivity() const;
    juce::String buildDetailsLine() const;
    juce::String buildStateLine() const;
    juce::String buildPreviewLine() const;
    juce::String getHostDescription() const;

    static juce::String formatSeconds(double seconds);
    static juce::String truncateForDiscord(juce::String text);
    static juce::File getGlobalSettingsFile();
    static void ensureDefaultProperties(juce::ValueTree& state);

    juce::AudioProcessorValueTreeState parameters;
    TransportSnapshot snapshot;
    DawUiStateDetector uiDetector;
    DawUiStateDetector::Snapshot uiSnapshot;
    DiscordIpcClient discord;

    juce::String lastClientId;
    juce::String lastActivityJson;
    juce::String lastSavedSettingsXml;
    double lastSendMs = 0.0;
    bool activityWasEnabled = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DiscordRichPresenceAudioProcessor)
};

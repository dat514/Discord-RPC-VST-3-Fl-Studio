#pragma once

#include <JuceHeader.h>

#if JUCE_WINDOWS
#include <windows.h>
#endif

class DiscordIpcClient
{
public:
    DiscordIpcClient() = default;
    ~DiscordIpcClient();

    void setClientId(const juce::String& newClientId);
    bool connect();
    void disconnect();

    bool isConnected() const;
    juce::String getStatusText() const;

    bool setActivity(const juce::var& activity);
    bool clearActivity();

private:
    bool sendHandshake();
    bool sendCommand(const juce::String& commandName, const juce::var& args);
    bool writeFrame(int opCode, const juce::String& json);
    void drainIncomingFrames();
    void closeHandle();
    void setStatus(const juce::String& text);

    juce::String clientId = "1424318204095893524";
    juce::String statusText = "Idle";

#if JUCE_WINDOWS
    HANDLE pipeHandle = INVALID_HANDLE_VALUE;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DiscordIpcClient)
};

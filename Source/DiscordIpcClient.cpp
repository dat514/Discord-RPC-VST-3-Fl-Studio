#include "DiscordIpcClient.h"

namespace
{
    constexpr int opHandshake = 0;
    constexpr int opFrame = 1;

    juce::String makeNonce()
    {
        return juce::Uuid().toString();
    }

    juce::String sanitizeClientId(juce::String value)
    {
        value = value.trim();

        juce::String cleaned;
        for (auto c : value)
            if (juce::CharacterFunctions::isDigit(c))
                cleaned += c;

        return cleaned.isNotEmpty() ? cleaned : "1424318204095893524";
    }

    int getCurrentPid()
    {
       #if JUCE_WINDOWS
        return static_cast<int>(GetCurrentProcessId());
       #else
        return 0;
       #endif
    }
}

DiscordIpcClient::~DiscordIpcClient()
{
    disconnect();
}

void DiscordIpcClient::setClientId(const juce::String& newClientId)
{
    const auto sanitized = sanitizeClientId(newClientId);

    if (clientId == sanitized)
        return;

    disconnect();
    clientId = sanitized;
}

bool DiscordIpcClient::connect()
{
#if JUCE_WINDOWS
    if (isConnected())
        return true;

    for (int index = 0; index < 10; ++index)
    {
        const auto pipeName = L"\\\\.\\pipe\\discord-ipc-" + std::to_wstring(index);

        pipeHandle = CreateFileW(pipeName.c_str(),
                                 GENERIC_READ | GENERIC_WRITE,
                                 0,
                                 nullptr,
                                 OPEN_EXISTING,
                                 0,
                                 nullptr);

        if (pipeHandle != INVALID_HANDLE_VALUE)
        {
            DWORD mode = PIPE_READMODE_BYTE;
            SetNamedPipeHandleState(pipeHandle, &mode, nullptr, nullptr);

            if (sendHandshake())
            {
                setStatus("Connected to Discord");
                drainIncomingFrames();
                return true;
            }

            closeHandle();
        }
        else if (GetLastError() == ERROR_PIPE_BUSY)
        {
            WaitNamedPipeW(pipeName.c_str(), 150);
        }
    }

    setStatus("Discord IPC not found");
    return false;
#else
    setStatus("Discord IPC is Windows-only in this build");
    return false;
#endif
}

void DiscordIpcClient::disconnect()
{
    if (isConnected())
        clearActivity();

    closeHandle();
    setStatus("Disconnected");
}

bool DiscordIpcClient::isConnected() const
{
#if JUCE_WINDOWS
    return pipeHandle != INVALID_HANDLE_VALUE;
#else
    return false;
#endif
}

juce::String DiscordIpcClient::getStatusText() const
{
    return statusText;
}

bool DiscordIpcClient::setActivity(const juce::var& activity)
{
    if (!connect())
        return false;

    auto args = std::make_unique<juce::DynamicObject>();
    args->setProperty("pid", getCurrentPid());
    args->setProperty("activity", activity);

    return sendCommand("SET_ACTIVITY", juce::var(args.release()));
}

bool DiscordIpcClient::clearActivity()
{
    if (!isConnected())
        return true;

    auto args = std::make_unique<juce::DynamicObject>();
    args->setProperty("pid", getCurrentPid());
    args->setProperty("activity", juce::var());

    return sendCommand("SET_ACTIVITY", juce::var(args.release()));
}

bool DiscordIpcClient::sendHandshake()
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty("v", 1);
    object->setProperty("client_id", clientId);

    return writeFrame(opHandshake, juce::JSON::toString(juce::var(object.release()), true));
}

bool DiscordIpcClient::sendCommand(const juce::String& commandName, const juce::var& args)
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty("cmd", commandName);
    object->setProperty("args", args);
    object->setProperty("nonce", makeNonce());

    const auto ok = writeFrame(opFrame, juce::JSON::toString(juce::var(object.release()), true));
    drainIncomingFrames();

    if (ok)
        setStatus("Activity sent");

    return ok;
}

bool DiscordIpcClient::writeFrame(int opCode, const juce::String& json)
{
#if JUCE_WINDOWS
    if (!isConnected())
        return false;

    const auto utf8 = json.toRawUTF8();
    const auto length = static_cast<uint32_t>(std::strlen(utf8));
    const uint32_t op = static_cast<uint32_t>(opCode);

    DWORD written = 0;

    if (!WriteFile(pipeHandle, &op, sizeof(op), &written, nullptr) || written != sizeof(op))
    {
        closeHandle();
        setStatus("Discord write failed");
        return false;
    }

    if (!WriteFile(pipeHandle, &length, sizeof(length), &written, nullptr) || written != sizeof(length))
    {
        closeHandle();
        setStatus("Discord write failed");
        return false;
    }

    if (length > 0
        && (!WriteFile(pipeHandle, utf8, length, &written, nullptr) || written != length))
    {
        closeHandle();
        setStatus("Discord write failed");
        return false;
    }

    return true;
#else
    juce::ignoreUnused(opCode, json);
    return false;
#endif
}

void DiscordIpcClient::drainIncomingFrames()
{
#if JUCE_WINDOWS
    if (!isConnected())
        return;

    for (;;)
    {
        DWORD available = 0;
        if (!PeekNamedPipe(pipeHandle, nullptr, 0, nullptr, &available, nullptr) || available < 8)
            break;

        uint32_t op = 0;
        uint32_t length = 0;
        DWORD read = 0;

        if (!ReadFile(pipeHandle, &op, sizeof(op), &read, nullptr) || read != sizeof(op))
            break;

        if (!ReadFile(pipeHandle, &length, sizeof(length), &read, nullptr) || read != sizeof(length))
            break;

        if (length == 0)
            continue;

        juce::HeapBlock<char> payload(static_cast<size_t>(length) + 1);
        payload[length] = 0;

        if (!ReadFile(pipeHandle, payload.getData(), length, &read, nullptr) || read != length)
            break;

        juce::ignoreUnused(op);
    }
#endif
}

void DiscordIpcClient::closeHandle()
{
#if JUCE_WINDOWS
    if (pipeHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(pipeHandle);
        pipeHandle = INVALID_HANDLE_VALUE;
    }
#endif
}

void DiscordIpcClient::setStatus(const juce::String& text)
{
    statusText = text;
}

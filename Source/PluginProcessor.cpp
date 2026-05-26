#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace IDs
{
    constexpr const char* enabled = "enabled";
    constexpr const char* showProjectName = "showProjectName";
    constexpr const char* showHostName = "showHostName";
    constexpr const char* showTransportState = "showTransportState";
    constexpr const char* showClockTime = "showClockTime";
    constexpr const char* showBarsBeats = "showBarsBeats";
    constexpr const char* showBpm = "showBpm";
    constexpr const char* showTimeSignature = "showTimeSignature";
    constexpr const char* showSampleRate = "showSampleRate";
    constexpr const char* showDawView = "showDawView";
    constexpr const char* showFocusedPlugin = "showFocusedPlugin";
    constexpr const char* showMixerInsert = "showMixerInsert";
    constexpr const char* useElapsedTimestamp = "useElapsedTimestamp";

    const juce::Identifier clientId { "clientId" };
    const juce::Identifier projectName { "projectName" };
    const juce::Identifier detailsPrefix { "detailsPrefix" };
    const juce::Identifier statePrefix { "statePrefix" };
    const juce::Identifier largeImageKey { "largeImageKey" };
    const juce::Identifier largeImageText { "largeImageText" };
    const juce::Identifier smallImageKey { "smallImageKey" };
    const juce::Identifier smallImageText { "smallImageText" };
}

juce::AudioProcessorValueTreeState::ParameterLayout DiscordRichPresenceAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    auto addBool = [&params] (const char* id, const juce::String& name, bool defaultValue)
    {
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { id, 1 }, name, defaultValue));
    };

    addBool(IDs::enabled, "Enable Discord Presence", true);
    addBool(IDs::showProjectName, "Show Project Name", true);
    addBool(IDs::showHostName, "Show Host Name", true);
    addBool(IDs::showTransportState, "Show Transport State", true);
    addBool(IDs::showClockTime, "Show Clock Time", true);
    addBool(IDs::showBarsBeats, "Show Bars and Beats", true);
    addBool(IDs::showBpm, "Show BPM", true);
    addBool(IDs::showTimeSignature, "Show Time Signature", true);
    addBool(IDs::showSampleRate, "Show Sample Rate", false);
    addBool(IDs::showDawView, "Show DAW View", true);
    addBool(IDs::showFocusedPlugin, "Show Focused Plugin", true);
    addBool(IDs::showMixerInsert, "Show Mixer Insert", true);
    addBool(IDs::useElapsedTimestamp, "Use Discord Elapsed Timer", true);

    return { params.begin(), params.end() };
}

DiscordRichPresenceAudioProcessor::DiscordRichPresenceAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    ensureDefaultProperties(parameters.state);
    loadGlobalSettings();
    lastClientId = getPresenceProperty(IDs::clientId);
    discord.setClientId(lastClientId);
    startTimerHz(2);
}

DiscordRichPresenceAudioProcessor::~DiscordRichPresenceAudioProcessor()
{
    stopTimer();
}

void DiscordRichPresenceAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    snapshot.sampleRate.store(sampleRate);
}

void DiscordRichPresenceAudioProcessor::releaseResources()
{
}

bool DiscordRichPresenceAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainIn = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    return !mainOut.isDisabled() && mainIn == mainOut;
}

void DiscordRichPresenceAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused(midi);
    processAudioBlock(buffer);
}

void DiscordRichPresenceAudioProcessor::processBlock(juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused(midi);
    processAudioBlock(buffer);
}

template <typename FloatType>
void DiscordRichPresenceAudioProcessor::processAudioBlock(juce::AudioBuffer<FloatType>& buffer)
{
    updateTransportSnapshot();

    for (auto channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());
}

void DiscordRichPresenceAudioProcessor::updateTransportSnapshot()
{
    if (auto* playHead = getPlayHead())
    {
        const auto position = playHead->getPosition();

        if (position.hasValue())
        {
            const auto& info = *position;

            snapshot.isPlaying.store(info.getIsPlaying());
            snapshot.isRecording.store(info.getIsRecording());
            snapshot.isLooping.store(info.getIsLooping());

            const auto seconds = info.getTimeInSeconds();
            snapshot.hasSeconds.store(seconds.hasValue());
            if (seconds.hasValue())
                snapshot.seconds.store(*seconds);

            const auto ppq = info.getPpqPosition();
            snapshot.hasPpq.store(ppq.hasValue());
            if (ppq.hasValue())
                snapshot.ppq.store(*ppq);

            const auto bpm = info.getBpm();
            snapshot.hasBpm.store(bpm.hasValue());
            if (bpm.hasValue())
                snapshot.bpm.store(*bpm);

            const auto timeSignature = info.getTimeSignature();
            snapshot.hasTimeSignature.store(timeSignature.hasValue());
            if (timeSignature.hasValue())
            {
                snapshot.numerator.store(timeSignature->numerator);
                snapshot.denominator.store(timeSignature->denominator);
            }
        }
    }
}

juce::AudioProcessorEditor* DiscordRichPresenceAudioProcessor::createEditor()
{
    return new DiscordRichPresenceAudioProcessorEditor(*this);
}

bool DiscordRichPresenceAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String DiscordRichPresenceAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DiscordRichPresenceAudioProcessor::acceptsMidi() const
{
    return false;
}

bool DiscordRichPresenceAudioProcessor::producesMidi() const
{
    return false;
}

bool DiscordRichPresenceAudioProcessor::isMidiEffect() const
{
    return false;
}

double DiscordRichPresenceAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DiscordRichPresenceAudioProcessor::getNumPrograms()
{
    return 1;
}

int DiscordRichPresenceAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DiscordRichPresenceAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String DiscordRichPresenceAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void DiscordRichPresenceAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void DiscordRichPresenceAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    ensureDefaultProperties(state);

    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void DiscordRichPresenceAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        if (xml->hasTagName(parameters.state.getType()))
        {
            auto state = juce::ValueTree::fromXml(*xml);
            ensureDefaultProperties(state);
            parameters.replaceState(state);
            loadGlobalSettings();
            lastClientId = getPresenceProperty(IDs::clientId);
            discord.setClientId(lastClientId);
        }
    }
}

juce::AudioProcessorValueTreeState& DiscordRichPresenceAudioProcessor::getValueTreeState()
{
    return parameters;
}

void DiscordRichPresenceAudioProcessor::setPresenceProperty(const juce::Identifier& name, const juce::String& value)
{
    parameters.state.setProperty(name, value, nullptr);
    saveGlobalSettingsIfChanged();
}

juce::String DiscordRichPresenceAudioProcessor::getPresenceProperty(const juce::Identifier& name) const
{
    return parameters.state.getProperty(name).toString();
}

juce::String DiscordRichPresenceAudioProcessor::getDiscordStatusText() const
{
    return discord.getStatusText();
}

juce::String DiscordRichPresenceAudioProcessor::getPreviewText() const
{
    return buildPreviewLine();
}

void DiscordRichPresenceAudioProcessor::timerCallback()
{
    saveGlobalSettingsIfChanged();

    const auto enabled = parameterEnabled(IDs::enabled);

    if (!enabled)
    {
        if (activityWasEnabled)
        {
            discord.clearActivity();
            discord.disconnect();
            activityWasEnabled = false;
            lastActivityJson.clear();
        }

        return;
    }

    const auto clientId = getPresenceProperty(IDs::clientId);
    if (clientId != lastClientId)
    {
        discord.clearActivity();
        discord.setClientId(clientId);
        lastClientId = clientId;
        lastActivityJson.clear();
    }

    const auto latestUiSnapshot = uiDetector.scan();
    if (latestUiSnapshot.hostActive
        || latestUiSnapshot.projectName.isNotEmpty()
        || latestUiSnapshot.surface.isNotEmpty()
        || latestUiSnapshot.focusedPlugin.isNotEmpty()
        || latestUiSnapshot.mixerInsert.isNotEmpty())
    {
        uiSnapshot = latestUiSnapshot;
    }

    const auto activity = buildDiscordActivity();
    const auto activityJson = juce::JSON::toString(activity, true);
    const auto nowMs = juce::Time::getMillisecondCounterHiRes();
    const auto isPlaying = snapshot.isPlaying.load();
    const auto resendIntervalMs = isPlaying ? 5000 : 15000;

    if (activityJson != lastActivityJson || nowMs - lastSendMs >= resendIntervalMs)
    {
        if (discord.setActivity(activity))
        {
            lastActivityJson = activityJson;
            lastSendMs = nowMs;
            activityWasEnabled = true;
        }
    }
}

bool DiscordRichPresenceAudioProcessor::parameterEnabled(const char* parameterId) const
{
    if (auto* parameter = parameters.getRawParameterValue(parameterId))
        return parameter->load() >= 0.5f;

    return false;
}

void DiscordRichPresenceAudioProcessor::loadGlobalSettings()
{
    const auto file = getGlobalSettingsFile();

    if (! file.existsAsFile())
    {
        saveGlobalSettingsIfChanged();
        return;
    }

    if (auto xml = juce::XmlDocument::parse(file))
    {
        if (xml->hasTagName(parameters.state.getType()))
        {
            auto state = juce::ValueTree::fromXml(*xml);
            ensureDefaultProperties(state);
            parameters.replaceState(state);
            lastSavedSettingsXml = xml->toString();
        }
    }
}

void DiscordRichPresenceAudioProcessor::saveGlobalSettingsIfChanged()
{
    auto state = parameters.copyState();
    ensureDefaultProperties(state);

    auto xml = state.createXml();
    if (xml == nullptr)
        return;

    const auto text = xml->toString();
    if (text == lastSavedSettingsXml)
        return;

    const auto file = getGlobalSettingsFile();
    file.getParentDirectory().createDirectory();
    file.replaceWithText(text);
    lastSavedSettingsXml = text;
}

juce::var DiscordRichPresenceAudioProcessor::buildDiscordActivity() const
{
    auto activity = std::make_unique<juce::DynamicObject>();
    activity->setProperty("type", 0);
    activity->setProperty("details", truncateForDiscord(buildDetailsLine()));
    activity->setProperty("state", truncateForDiscord(buildStateLine()));
    activity->setProperty("instance", true);

    if (parameterEnabled(IDs::useElapsedTimestamp)
        && snapshot.isPlaying.load()
        && snapshot.hasSeconds.load())
    {
        auto timestamps = std::make_unique<juce::DynamicObject>();
        const auto nowMs = static_cast<juce::int64>(juce::Time::getCurrentTime().toMilliseconds());
        const auto projectPositionMs = static_cast<juce::int64>(snapshot.seconds.load() * 1000.0);
        timestamps->setProperty("start", nowMs - projectPositionMs);
        activity->setProperty("timestamps", juce::var(timestamps.release()));
    }

    const auto largeKey = getPresenceProperty(IDs::largeImageKey).trim();
    const auto smallKey = getPresenceProperty(IDs::smallImageKey).trim();

    if (largeKey.isNotEmpty() || smallKey.isNotEmpty())
    {
        auto assets = std::make_unique<juce::DynamicObject>();

        if (largeKey.isNotEmpty())
        {
            assets->setProperty("large_image", largeKey);
            assets->setProperty("large_text", truncateForDiscord(getPresenceProperty(IDs::largeImageText)));
        }

        if (smallKey.isNotEmpty())
        {
            assets->setProperty("small_image", smallKey);
            assets->setProperty("small_text", truncateForDiscord(getPresenceProperty(IDs::smallImageText)));
        }

        activity->setProperty("assets", juce::var(assets.release()));
    }

    return juce::var(activity.release());
}

juce::String DiscordRichPresenceAudioProcessor::buildDetailsLine() const
{
    juce::StringArray parts;
    const auto prefix = getPresenceProperty(IDs::detailsPrefix).trim();

    if (prefix.isNotEmpty())
        parts.add(prefix);

    if (parameterEnabled(IDs::showProjectName))
    {
        auto projectName = getPresenceProperty(IDs::projectName).trim();

        if (projectName.isEmpty()
            || projectName.equalsIgnoreCase("Untitled Project")
            || projectName.equalsIgnoreCase("Untitled"))
        {
            projectName = uiSnapshot.projectName.trim();
        }

        if (projectName.isNotEmpty())
            parts.add(projectName);
    }

    if (parameterEnabled(IDs::showHostName))
        parts.add(getHostDescription());

    return parts.isEmpty() ? "Making music" : parts.joinIntoString(" | ");
}

juce::String DiscordRichPresenceAudioProcessor::buildStateLine() const
{
    juce::StringArray parts;
    const auto prefix = getPresenceProperty(IDs::statePrefix).trim();

    if (prefix.isNotEmpty())
        parts.add(prefix);

    if (parameterEnabled(IDs::showTransportState))
    {
        if (snapshot.isRecording.load())
            parts.add("Recording");
        else if (snapshot.isPlaying.load())
            parts.add(snapshot.isLooping.load() ? "Playing loop" : "Playing");
        else
            parts.add("Stopped");
    }

    if (parameterEnabled(IDs::showDawView) && uiSnapshot.surface.isNotEmpty())
        parts.add("View: " + uiSnapshot.surface);

    if (parameterEnabled(IDs::showMixerInsert) && uiSnapshot.mixerInsert.isNotEmpty())
        parts.add(uiSnapshot.mixerInsert);

    if (parameterEnabled(IDs::showFocusedPlugin) && uiSnapshot.focusedPlugin.isNotEmpty())
        parts.add("VST: " + uiSnapshot.focusedPlugin);

    if (parameterEnabled(IDs::showClockTime) && snapshot.hasSeconds.load())
        parts.add(formatSeconds(snapshot.seconds.load()));

    if (parameterEnabled(IDs::showBarsBeats)
        && snapshot.hasPpq.load()
        && snapshot.hasTimeSignature.load())
    {
        const auto numerator = juce::jmax(1, snapshot.numerator.load());
        const auto ppq = juce::jmax(0.0, snapshot.ppq.load());
        const auto bar = static_cast<int>(std::floor(ppq / numerator)) + 1;
        const auto beat = static_cast<int>(std::floor(std::fmod(ppq, static_cast<double>(numerator)))) + 1;
        parts.add("Bar " + juce::String(bar) + " Beat " + juce::String(beat));
    }

    if (parameterEnabled(IDs::showBpm) && snapshot.hasBpm.load())
        parts.add(juce::String(snapshot.bpm.load(), 1) + " BPM");

    if (parameterEnabled(IDs::showTimeSignature) && snapshot.hasTimeSignature.load())
        parts.add(juce::String(snapshot.numerator.load()) + "/" + juce::String(snapshot.denominator.load()));

    if (parameterEnabled(IDs::showSampleRate) && snapshot.sampleRate.load() > 0.0)
        parts.add(juce::String(snapshot.sampleRate.load() / 1000.0, 1) + " kHz");

    return parts.isEmpty() ? "Waiting for host transport" : parts.joinIntoString(" | ");
}

juce::String DiscordRichPresenceAudioProcessor::buildPreviewLine() const
{
    return buildDetailsLine() + "\n" + buildStateLine();
}

juce::String DiscordRichPresenceAudioProcessor::getHostDescription() const
{
    juce::PluginHostType host;
    const auto description = juce::String(host.getHostDescription()).trim();
    return description.isNotEmpty() ? description : "Unknown DAW";
}

juce::String DiscordRichPresenceAudioProcessor::formatSeconds(double seconds)
{
    seconds = juce::jmax(0.0, seconds);

    const auto total = static_cast<int64_t>(std::floor(seconds + 0.5));
    const auto hours = total / 3600;
    const auto minutes = (total / 60) % 60;
    const auto secs = total % 60;

    if (hours > 0)
        return juce::String(hours) + ":" + juce::String(minutes).paddedLeft('0', 2) + ":" + juce::String(secs).paddedLeft('0', 2);

    return juce::String(minutes) + ":" + juce::String(secs).paddedLeft('0', 2);
}

juce::String DiscordRichPresenceAudioProcessor::truncateForDiscord(juce::String text)
{
    text = text.trim();

    if (text.length() > 120)
        return text.substring(0, 117).trim() + "...";

    return text;
}

juce::File DiscordRichPresenceAudioProcessor::getGlobalSettingsFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Discord Rich Presence VST3")
        .getChildFile("settings.xml");
}

void DiscordRichPresenceAudioProcessor::ensureDefaultProperties(juce::ValueTree& state)
{
    auto setDefault = [&state] (const juce::Identifier& id, const juce::String& value)
    {
        if (!state.hasProperty(id))
            state.setProperty(id, value, nullptr);
    };

    setDefault(IDs::clientId, "1424318204095893524");
    setDefault(IDs::projectName, "Untitled Project");
    setDefault(IDs::detailsPrefix, "In the studio");
    setDefault(IDs::statePrefix, "");
    setDefault(IDs::largeImageKey, "");
    setDefault(IDs::largeImageText, "Discord Rich Presence VST3");
    setDefault(IDs::smallImageKey, "");
    setDefault(IDs::smallImageText, "");
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DiscordRichPresenceAudioProcessor();
}

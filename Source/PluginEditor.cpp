#include "PluginEditor.h"

namespace
{
    constexpr int margin = 18;
    constexpr int rowHeight = 28;
    constexpr int labelWidth = 126;

    namespace ParamIDs
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
    }

    namespace Props
    {
        const juce::Identifier clientId { "clientId" };
        const juce::Identifier projectName { "projectName" };
        const juce::Identifier detailsPrefix { "detailsPrefix" };
        const juce::Identifier statePrefix { "statePrefix" };
        const juce::Identifier largeImageKey { "largeImageKey" };
        const juce::Identifier smallImageKey { "smallImageKey" };
    }

    void styleLabel(juce::Label& label, float fontSize = 14.0f)
    {
        label.setFont(juce::Font(fontSize));
        label.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.86f));
    }

    void styleTextEditor(juce::TextEditor& editor)
    {
        editor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff191b22));
        editor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
        editor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff343845));
        editor.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(0xff5865f2));
        editor.setColour(juce::TextEditor::highlightColourId, juce::Colour(0xff5865f2).withAlpha(0.45f));
        editor.setJustification(juce::Justification::centredLeft);
    }

    void styleToggle(juce::ToggleButton& button)
    {
        button.setColour(juce::ToggleButton::textColourId, juce::Colours::white.withAlpha(0.9f));
        button.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xff3ba55d));
        button.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(0xff6b7280));
    }
}

DiscordRichPresenceAudioProcessorEditor::DiscordRichPresenceAudioProcessorEditor(DiscordRichPresenceAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(680, 620);

    titleLabel.setText("Discord Rich Presence", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(22.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    statusLabel.setFont(juce::Font(13.0f));
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaeb4c2));
    addAndMakeVisible(statusLabel);

    previewLabel.setFont(juce::Font(14.0f));
    previewLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.88f));
    previewLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff191b22));
    previewLabel.setColour(juce::Label::outlineColourId, juce::Colour(0xff343845));
    previewLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(previewLabel);

    addTextField(clientIdLabel, clientIdEditor, "App ID", Props::clientId);
    addTextField(projectNameLabel, projectNameEditor, "Project", Props::projectName);
    addTextField(detailsPrefixLabel, detailsPrefixEditor, "Details prefix", Props::detailsPrefix);
    addTextField(statePrefixLabel, statePrefixEditor, "State prefix", Props::statePrefix);
    addTextField(largeImageKeyLabel, largeImageKeyEditor, "Large image", Props::largeImageKey);
    addTextField(smallImageKeyLabel, smallImageKeyEditor, "Small image", Props::smallImageKey);

    addToggle(enabledToggle, enabledAttachment, "Enable presence", ParamIDs::enabled);
    addToggle(showProjectToggle, showProjectAttachment, "Project name", ParamIDs::showProjectName);
    addToggle(showHostToggle, showHostAttachment, "DAW name", ParamIDs::showHostName);
    addToggle(showTransportToggle, showTransportAttachment, "Transport state", ParamIDs::showTransportState);
    addToggle(showClockToggle, showClockAttachment, "Clock time", ParamIDs::showClockTime);
    addToggle(showBarsToggle, showBarsAttachment, "Bar / beat", ParamIDs::showBarsBeats);
    addToggle(showBpmToggle, showBpmAttachment, "BPM", ParamIDs::showBpm);
    addToggle(showTimeSignatureToggle, showTimeSignatureAttachment, "Time signature", ParamIDs::showTimeSignature);
    addToggle(showSampleRateToggle, showSampleRateAttachment, "Sample rate", ParamIDs::showSampleRate);
    addToggle(showDawViewToggle, showDawViewAttachment, "DAW view", ParamIDs::showDawView);
    addToggle(showMixerInsertToggle, showMixerInsertAttachment, "Mixer insert", ParamIDs::showMixerInsert);
    addToggle(showFocusedPluginToggle, showFocusedPluginAttachment, "Focused VST", ParamIDs::showFocusedPlugin);
    addToggle(useTimestampToggle, useTimestampAttachment, "Discord elapsed timer", ParamIDs::useElapsedTimestamp);

    refreshTextFieldsFromState();
    startTimerHz(4);
}

DiscordRichPresenceAudioProcessorEditor::~DiscordRichPresenceAudioProcessorEditor()
{
    stopTimer();
}

void DiscordRichPresenceAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff111318));

    auto bounds = getLocalBounds().reduced(margin);
    auto header = bounds.removeFromTop(58);
    juce::ignoreUnused(header);

    g.setColour(juce::Colour(0xff232733));
    g.drawRoundedRectangle(getLocalBounds().reduced(margin).toFloat(), 8.0f, 1.0f);

    g.setColour(juce::Colour(0xff5865f2));
    g.fillRoundedRectangle(juce::Rectangle<float>(static_cast<float>(margin),
                                                  static_cast<float>(margin + 44),
                                                  static_cast<float>(getWidth() - margin * 2),
                                                  3.0f),
                           1.5f);
}

void DiscordRichPresenceAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(margin);

    auto header = bounds.removeFromTop(54);
    titleLabel.setBounds(header.removeFromTop(28));
    statusLabel.setBounds(header);

    bounds.removeFromTop(8);
    previewLabel.setBounds(bounds.removeFromTop(54));
    bounds.removeFromTop(12);

    auto fields = bounds.removeFromLeft(340);
    bounds.removeFromLeft(18);
    auto toggles = bounds;

    auto layoutField = [&fields] (juce::Label& label, juce::TextEditor& editor)
    {
        auto row = fields.removeFromTop(rowHeight);
        label.setBounds(row.removeFromLeft(labelWidth));
        editor.setBounds(row.reduced(0, 2));
        fields.removeFromTop(7);
    };

    layoutField(clientIdLabel, clientIdEditor);
    layoutField(projectNameLabel, projectNameEditor);
    layoutField(detailsPrefixLabel, detailsPrefixEditor);
    layoutField(statePrefixLabel, statePrefixEditor);
    layoutField(largeImageKeyLabel, largeImageKeyEditor);
    layoutField(smallImageKeyLabel, smallImageKeyEditor);

    auto layoutToggle = [&toggles] (juce::ToggleButton& toggle)
    {
        toggle.setBounds(toggles.removeFromTop(rowHeight));
        toggles.removeFromTop(5);
    };

    layoutToggle(enabledToggle);
    toggles.removeFromTop(5);
    layoutToggle(showProjectToggle);
    layoutToggle(showHostToggle);
    layoutToggle(showTransportToggle);
    layoutToggle(showClockToggle);
    layoutToggle(showBarsToggle);
    layoutToggle(showBpmToggle);
    layoutToggle(showTimeSignatureToggle);
    layoutToggle(showSampleRateToggle);
    layoutToggle(showDawViewToggle);
    layoutToggle(showMixerInsertToggle);
    layoutToggle(showFocusedPluginToggle);
    layoutToggle(useTimestampToggle);
}

void DiscordRichPresenceAudioProcessorEditor::timerCallback()
{
    statusLabel.setText("Discord: " + audioProcessor.getDiscordStatusText(), juce::dontSendNotification);
    previewLabel.setText(audioProcessor.getPreviewText(), juce::dontSendNotification);
}

void DiscordRichPresenceAudioProcessorEditor::addToggle(juce::ToggleButton& button,
                                                        std::unique_ptr<ButtonAttachment>& attachment,
                                                        const juce::String& text,
                                                        const juce::String& parameterId)
{
    button.setButtonText(text);
    styleToggle(button);
    addAndMakeVisible(button);

    attachment = std::make_unique<ButtonAttachment>(audioProcessor.getValueTreeState(), parameterId, button);
}

void DiscordRichPresenceAudioProcessorEditor::addTextField(juce::Label& label,
                                                           juce::TextEditor& editor,
                                                           const juce::String& labelText,
                                                           const juce::Identifier& propertyId)
{
    label.setText(labelText, juce::dontSendNotification);
    styleLabel(label);
    addAndMakeVisible(label);

    editor.setSelectAllWhenFocused(true);
    styleTextEditor(editor);
    editor.onTextChange = [this, &editor, propertyId]
    {
        audioProcessor.setPresenceProperty(propertyId, editor.getText());
    };
    addAndMakeVisible(editor);
}

void DiscordRichPresenceAudioProcessorEditor::refreshTextFieldsFromState()
{
    auto setText = [this] (juce::TextEditor& editor, const juce::Identifier& propertyId)
    {
        editor.setText(audioProcessor.getPresenceProperty(propertyId), juce::dontSendNotification);
    };

    setText(clientIdEditor, Props::clientId);
    setText(projectNameEditor, Props::projectName);
    setText(detailsPrefixEditor, Props::detailsPrefix);
    setText(statePrefixEditor, Props::statePrefix);
    setText(largeImageKeyEditor, Props::largeImageKey);
    setText(smallImageKeyEditor, Props::smallImageKey);
}

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class DiscordRichPresenceAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                      private juce::Timer
{
public:
    explicit DiscordRichPresenceAudioProcessorEditor(DiscordRichPresenceAudioProcessor&);
    ~DiscordRichPresenceAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void timerCallback() override;
    void addToggle(juce::ToggleButton& button,
                   std::unique_ptr<ButtonAttachment>& attachment,
                   const juce::String& text,
                   const juce::String& parameterId);
    void addTextField(juce::Label& label,
                      juce::TextEditor& editor,
                      const juce::String& labelText,
                      const juce::Identifier& propertyId);
    void refreshTextFieldsFromState();

    DiscordRichPresenceAudioProcessor& audioProcessor;

    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::Label previewLabel;

    juce::Label clientIdLabel;
    juce::TextEditor clientIdEditor;
    juce::Label projectNameLabel;
    juce::TextEditor projectNameEditor;
    juce::Label detailsPrefixLabel;
    juce::TextEditor detailsPrefixEditor;
    juce::Label statePrefixLabel;
    juce::TextEditor statePrefixEditor;
    juce::Label largeImageKeyLabel;
    juce::TextEditor largeImageKeyEditor;
    juce::Label smallImageKeyLabel;
    juce::TextEditor smallImageKeyEditor;

    juce::ToggleButton enabledToggle;
    juce::ToggleButton showProjectToggle;
    juce::ToggleButton showHostToggle;
    juce::ToggleButton showTransportToggle;
    juce::ToggleButton showClockToggle;
    juce::ToggleButton showBarsToggle;
    juce::ToggleButton showBpmToggle;
    juce::ToggleButton showTimeSignatureToggle;
    juce::ToggleButton showSampleRateToggle;
    juce::ToggleButton showDawViewToggle;
    juce::ToggleButton showFocusedPluginToggle;
    juce::ToggleButton showMixerInsertToggle;
    juce::ToggleButton useTimestampToggle;

    std::unique_ptr<ButtonAttachment> enabledAttachment;
    std::unique_ptr<ButtonAttachment> showProjectAttachment;
    std::unique_ptr<ButtonAttachment> showHostAttachment;
    std::unique_ptr<ButtonAttachment> showTransportAttachment;
    std::unique_ptr<ButtonAttachment> showClockAttachment;
    std::unique_ptr<ButtonAttachment> showBarsAttachment;
    std::unique_ptr<ButtonAttachment> showBpmAttachment;
    std::unique_ptr<ButtonAttachment> showTimeSignatureAttachment;
    std::unique_ptr<ButtonAttachment> showSampleRateAttachment;
    std::unique_ptr<ButtonAttachment> showDawViewAttachment;
    std::unique_ptr<ButtonAttachment> showFocusedPluginAttachment;
    std::unique_ptr<ButtonAttachment> showMixerInsertAttachment;
    std::unique_ptr<ButtonAttachment> useTimestampAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DiscordRichPresenceAudioProcessorEditor)
};

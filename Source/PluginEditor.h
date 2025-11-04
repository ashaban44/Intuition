/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "EnvelopeDisplay.h"
#include "WaveformDisplay.h"
#include "OscillatorDisplay.h"
#include "LFOTabs.h"
#include "WaveBankComponent.h"
#include "FilterDisplay.h"
#include "MainTabs.h"
#include "ItnMidiKeyboard.h"
#include "AIController.h"


//==============================================================================
/**
*/
class IntuitionAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                       private AIController::Listener
{
public:
    IntuitionAudioProcessorEditor (IntuitionAudioProcessor&);
    ~IntuitionAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void aiControllerStatusChanged(const juce::String& statusMessage, bool isError) override;
    void aiControllerAppliedChanges(const juce::Array<AIParameterChange>& changes) override;

private:
    IntuitionAudioProcessor& audioProcessor;
    juce::TooltipWindow tooltipWindow;
    ItnMidiKeyboard midiKeyboard;

    juce::Slider masterVolKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterVolAttachment;

    MainTabs mainTabs;

    std::unique_ptr<AIController> aiController;
    juce::GroupComponent aiGroup;
    std::unique_ptr<juce::PropertiesFile> aiProperties;
    juce::Label aiApiKeyLabel;
    juce::TextEditor aiApiKeyEditor;
    juce::Label aiEndpointLabel;
    juce::TextEditor aiEndpointEditor;
    juce::Label aiModelLabel;
    juce::TextEditor aiModelEditor;
    juce::TextButton aiConnectButton;
    juce::TextEditor aiPromptEditor;
    juce::TextButton aiSubmitButton;
    juce::Label aiStatusLabel;

    void configureAIFromStoredSettings();
    void applyAIConfigurationFromFields();
    void storeAISettings(const juce::String& apiKey,
                         const juce::String& endpoint,
                         const juce::String& model);
    static juce::PropertiesFile::Options createAIPropertiesOptions();
    void updateAISubmitButtonState();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (IntuitionAudioProcessorEditor)
};

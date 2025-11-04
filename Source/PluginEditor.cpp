/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
IntuitionAudioProcessorEditor::IntuitionAudioProcessorEditor(IntuitionAudioProcessor& p
) : AudioProcessorEditor(&p),
    audioProcessor(p),
    tooltipWindow(this),
    midiKeyboard(p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
    mainTabs(p.context) {

    // =================================================
    addAndMakeVisible(mainTabs);
    addAndMakeVisible(midiKeyboard);
    midiKeyboard.setAvailableRange(48, 108);
    midiKeyboard.setScrollButtonsVisible(false);
    midiKeyboard.clearKeyMappings();

    masterVolKnob.setSliderStyle(juce::Slider::Rotary);
    masterVolKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(masterVolKnob);

    masterVolAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters,
        "MASTER",
        masterVolKnob
    );

    aiController = std::make_unique<AIController>(audioProcessor.parameters);
    aiController->addListener(*this);

    aiGroup.setText("AI Performance Assistant");
    aiGroup.setTextLabelPosition(juce::Justification::centredLeft);
    addAndMakeVisible(aiGroup);

    aiProperties = std::make_unique<juce::PropertiesFile>(createAIPropertiesOptions());

    aiApiKeyLabel.setText("API Key", juce::dontSendNotification);
    aiApiKeyLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    aiApiKeyLabel.setJustificationType(juce::Justification::centredRight);
    aiApiKeyLabel.attachToComponent(&aiApiKeyEditor, true);
    aiApiKeyEditor.setTextToShowWhenEmpty("Paste provider key", juce::Colours::grey);
    aiApiKeyEditor.setSelectAllWhenFocused(true);
    aiApiKeyEditor.setPasswordCharacter(0x2022);
    aiApiKeyEditor.onReturnKey = [this]() { applyAIConfigurationFromFields(); };
    addAndMakeVisible(aiApiKeyEditor);

    aiEndpointLabel.setText("Endpoint", juce::dontSendNotification);
    aiEndpointLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    aiEndpointLabel.setJustificationType(juce::Justification::centredRight);
    aiEndpointLabel.attachToComponent(&aiEndpointEditor, true);
    aiEndpointEditor.setTextToShowWhenEmpty(getDefaultAIEndpoint(), juce::Colours::grey);
    aiEndpointEditor.setSelectAllWhenFocused(true);
    aiEndpointEditor.onReturnKey = [this]() { applyAIConfigurationFromFields(); };
    addAndMakeVisible(aiEndpointEditor);

    aiModelLabel.setText("Model", juce::dontSendNotification);
    aiModelLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    aiModelLabel.setJustificationType(juce::Justification::centredRight);
    aiModelLabel.attachToComponent(&aiModelEditor, true);
    aiModelEditor.setTextToShowWhenEmpty(getDefaultAIModel(), juce::Colours::grey);
    aiModelEditor.setSelectAllWhenFocused(true);
    aiModelEditor.onReturnKey = [this]() { applyAIConfigurationFromFields(); };
    addAndMakeVisible(aiModelEditor);

    aiConnectButton.setButtonText("Connect");
    aiConnectButton.onClick = [this]() { applyAIConfigurationFromFields(); };
    addAndMakeVisible(aiConnectButton);

    aiPromptEditor.setMultiLine(true);
    aiPromptEditor.setReturnKeyStartsNewLine(false);
    aiPromptEditor.setPopupMenuEnabled(true);
    aiPromptEditor.setTextToShowWhenEmpty("Describe the sound you want...", juce::Colours::grey);
    aiPromptEditor.onReturnKey = [this]() {
        if (aiController != nullptr)
        {
            auto text = aiPromptEditor.getText();
            if (text.trim().isEmpty())
                return;

            aiSubmitButton.setEnabled(false);
            aiController->sendPrompt(text);
        }
    };
    aiPromptEditor.onTextChange = [this]() { updateAISubmitButtonState(); };
    addAndMakeVisible(aiPromptEditor);

    aiSubmitButton.setButtonText("Send Prompt");
    aiSubmitButton.onClick = [this]() {
        if (aiController != nullptr)
        {
            auto text = aiPromptEditor.getText();
            if (text.trim().isEmpty())
                return;

            aiSubmitButton.setEnabled(false);
            aiController->sendPrompt(text);
        }
    };
    addAndMakeVisible(aiSubmitButton);

    aiStatusLabel.setJustificationType(juce::Justification::centredLeft);
    aiStatusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    aiStatusLabel.setText("AI assistant ready", juce::dontSendNotification);
    addAndMakeVisible(aiStatusLabel);

    configureAIFromStoredSettings();
    updateAISubmitButtonState();

    setSize(1200, 800);
}

IntuitionAudioProcessorEditor::~IntuitionAudioProcessorEditor()
{
    if (aiController != nullptr)
        aiController->removeListener(*this);
}

//==============================================================================
void IntuitionAudioProcessorEditor::paint (juce::Graphics& g) {
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void IntuitionAudioProcessorEditor::resized() {
    auto bounds = getLocalBounds();

    auto headerArea = bounds.removeFromTop(110);
    masterVolKnob.setBounds(headerArea.removeFromRight(120).reduced(10));

    auto footerHeight = 220;
    auto footerArea = bounds.removeFromBottom(footerHeight);

    mainTabs.setBounds(bounds);

    auto aiArea = footerArea.removeFromTop(footerArea.getHeight() - 90).reduced(10);
    aiGroup.setBounds(aiArea);

    auto groupBounds = aiGroup.getBounds().reduced(15, 30);
    auto configArea = groupBounds.removeFromTop(96);
    auto labelWidth = 120;

    auto keyRow = configArea.removeFromTop(28);
    keyRow.removeFromLeft(labelWidth);
    auto connectArea = keyRow.removeFromRight(120);
    aiConnectButton.setBounds(connectArea.reduced(5, 2));
    aiApiKeyEditor.setBounds(keyRow.reduced(5, 2));

    configArea.removeFromTop(8);

    auto endpointRow = configArea.removeFromTop(28);
    endpointRow.removeFromLeft(labelWidth);
    aiEndpointEditor.setBounds(endpointRow.reduced(5, 2));

    configArea.removeFromTop(8);

    auto modelRow = configArea.removeFromTop(28);
    modelRow.removeFromLeft(labelWidth);
    aiModelEditor.setBounds(modelRow.reduced(5, 2));

    auto promptArea = groupBounds;
    auto controlsRow = promptArea.removeFromBottom(36);
    aiPromptEditor.setBounds(promptArea);
    aiSubmitButton.setBounds(controlsRow.removeFromLeft(140).reduced(5, 2));
    aiStatusLabel.setBounds(controlsRow.reduced(5, 2));

    auto keyboardArea = footerArea.reduced(120, 10);
    midiKeyboard.setKeyWidth(keyboardArea.getWidth() / 36.0f);
    midiKeyboard.setBlackNoteWidthProportion(0.75);
    midiKeyboard.setBounds(keyboardArea.removeFromBottom(80));
}

void IntuitionAudioProcessorEditor::aiControllerStatusChanged(const juce::String& statusMessage, bool isError)
{
    auto colour = isError ? juce::Colours::salmon : juce::Colours::lightgreen;
    aiStatusLabel.setColour(juce::Label::textColourId, colour);
    aiStatusLabel.setText(statusMessage, juce::dontSendNotification);

    updateAISubmitButtonState();
}

void IntuitionAudioProcessorEditor::aiControllerAppliedChanges(const juce::Array<AIParameterChange>& changes)
{
    if (! changes.isEmpty())
        aiPromptEditor.clear();

    updateAISubmitButtonState();
}

void IntuitionAudioProcessorEditor::configureAIFromStoredSettings()
{
    if (aiProperties == nullptr)
        return;

    auto storedKey = aiProperties->getValue("aiApiKey");
    auto storedEndpoint = aiProperties->getValue("aiEndpoint");
    auto storedModel = aiProperties->getValue("aiModel");

    if (storedEndpoint.isEmpty())
        storedEndpoint = getDefaultAIEndpoint();

    if (storedModel.isEmpty())
        storedModel = getDefaultAIModel();

    aiApiKeyEditor.setText(storedKey, false);
    aiEndpointEditor.setText(storedEndpoint, false);
    aiModelEditor.setText(storedModel, false);

    if (aiController != nullptr)
        aiController->configureOpenAIClient(storedKey, storedEndpoint, storedModel);
}

void IntuitionAudioProcessorEditor::applyAIConfigurationFromFields()
{
    auto apiKey = aiApiKeyEditor.getText().trim();
    auto endpoint = aiEndpointEditor.getText().trim();
    auto model = aiModelEditor.getText().trim();

    storeAISettings(apiKey, endpoint, model);

    if (aiController != nullptr)
        aiController->configureOpenAIClient(apiKey, endpoint, model);

    updateAISubmitButtonState();
}

void IntuitionAudioProcessorEditor::storeAISettings(const juce::String& apiKey,
                                                   const juce::String& endpoint,
                                                   const juce::String& model)
{
    if (aiProperties == nullptr)
        return;

    if (apiKey.isNotEmpty())
        aiProperties->setValue("aiApiKey", apiKey);
    else
        aiProperties->removeValue("aiApiKey");

    if (endpoint.isNotEmpty() && endpoint != getDefaultAIEndpoint())
        aiProperties->setValue("aiEndpoint", endpoint);
    else
        aiProperties->removeValue("aiEndpoint");

    if (model.isNotEmpty() && model != getDefaultAIModel())
        aiProperties->setValue("aiModel", model);
    else
        aiProperties->removeValue("aiModel");

    aiProperties->saveIfNeeded();
}

juce::PropertiesFile::Options IntuitionAudioProcessorEditor::createAIPropertiesOptions()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "Intuition";
    options.filenameSuffix = "settings";
    options.osxLibrarySubFolder = "Application Support";
    options.folderName = "Intuition";
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    options.millisecondsBeforeSaving = 1000;
    return options;
}

void IntuitionAudioProcessorEditor::updateAISubmitButtonState()
{
    if (aiController == nullptr)
        return;

    auto hasPrompt = aiPromptEditor.getText().trim().isNotEmpty();
    auto busy = aiController->isBusy();
    aiSubmitButton.setEnabled(hasPrompt && ! busy);
}

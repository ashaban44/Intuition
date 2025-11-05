#include "AIControlPanel.h"

AIControlPanel::AIControlPanel(ItnContext& ctx)
    : context(ctx)
{
    // Create AI controller and key manager
    aiController = std::make_unique<ChatGPTParameterController>();
    apiKeyManager = std::make_unique<APIKeyManager>();

    // Load saved settings
    if (apiKeyManager->hasAPIKey())
    {
        aiController->setAPIKey(apiKeyManager->getAPIKey());
    }

    juce::String savedEndpoint = apiKeyManager->getAPIEndpoint();
    juce::String savedModel = apiKeyManager->getModel();

    aiController->setAPIEndpoint(savedEndpoint);
    aiController->setModel(savedModel);

    // Settings Group
    settingsGroup.setText("API Configuration");
    settingsGroup.setTextLabelPosition(juce::Justification::centredLeft);
    addAndMakeVisible(settingsGroup);

    // API Key
    apiKeyLabel.setText("API Key:", juce::dontSendNotification);
    apiKeyLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(apiKeyLabel);

    apiKeyEditor.setMultiLine(false);
    apiKeyEditor.setReturnKeyStartsNewLine(false);
    apiKeyEditor.setPasswordCharacter('*');
    apiKeyEditor.setText(apiKeyManager->hasAPIKey() ? "••••••••••••••••" : "");
    apiKeyEditor.setTooltip("Enter your OpenAI API key (starts with sk-)");
    addAndMakeVisible(apiKeyEditor);

    saveKeyButton.setButtonText("Save");
    saveKeyButton.onClick = [this] { onSaveKeyClicked(); };
    addAndMakeVisible(saveKeyButton);

    clearKeyButton.setButtonText("Clear");
    clearKeyButton.onClick = [this] { onClearKeyClicked(); };
    addAndMakeVisible(clearKeyButton);

    // Endpoint Selection
    endpointLabel.setText("Service:", juce::dontSendNotification);
    endpointLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(endpointLabel);

    endpointCombo.addItem("OpenAI (ChatGPT)", 1);
    endpointCombo.addItem("Grok (xAI)", 2);
    endpointCombo.addItem("Custom", 3);
    endpointCombo.onChange = [this] { onEndpointChanged(); };
    endpointCombo.setTooltip("Select AI service provider");

    // Set current selection based on saved endpoint
    if (savedEndpoint.contains("openai"))
        endpointCombo.setSelectedId(1);
    else if (savedEndpoint.contains("x.ai"))
        endpointCombo.setSelectedId(2);
    else
        endpointCombo.setSelectedId(3);

    addAndMakeVisible(endpointCombo);

    // Model Selection
    modelLabel.setText("Model:", juce::dontSendNotification);
    modelLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(modelLabel);

    modelCombo.addItem("GPT-4o Mini (Fast, Cheap)", 1);
    modelCombo.addItem("GPT-4o (Balanced)", 2);
    modelCombo.addItem("GPT-4 Turbo", 3);
    modelCombo.addItem("Grok Beta", 4);
    modelCombo.onChange = [this] { onModelChanged(); };
    modelCombo.setTooltip("Select AI model");

    // Set current model
    if (savedModel.contains("gpt-4o-mini"))
        modelCombo.setSelectedId(1);
    else if (savedModel.contains("gpt-4o"))
        modelCombo.setSelectedId(2);
    else if (savedModel.contains("gpt-4"))
        modelCombo.setSelectedId(3);
    else
        modelCombo.setSelectedId(4);

    addAndMakeVisible(modelCombo);

    // Prompt Group
    promptGroup.setText("AI Parameter Control");
    promptGroup.setTextLabelPosition(juce::Justification::centredLeft);
    addAndMakeVisible(promptGroup);

    // Prompt Input
    promptLabel.setText("Describe the sound you want:", juce::dontSendNotification);
    addAndMakeVisible(promptLabel);

    promptEditor.setMultiLine(true);
    promptEditor.setReturnKeyStartsNewLine(true);
    promptEditor.setScrollbarsShown(true);
    promptEditor.setTooltip("Example: 'make it brighter', 'add more bass', 'increase attack by 0.2'");
    addAndMakeVisible(promptEditor);

    // Action Buttons
    applyButton.setButtonText("Apply AI Changes");
    applyButton.onClick = [this] { onApplyClicked(); };
    applyButton.setEnabled(aiController->isConfigured());
    addAndMakeVisible(applyButton);

    cancelButton.setButtonText("Cancel");
    cancelButton.onClick = [this] { onCancelClicked(); };
    cancelButton.setEnabled(false);
    addAndMakeVisible(cancelButton);

    // Status Display
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    addAndMakeVisible(statusLabel);

    rateLimitLabel.setJustificationType(juce::Justification::centred);
    rateLimitLabel.setFont(juce::Font(11.0f));
    addAndMakeVisible(rateLimitLabel);

    // Start status update timer
    startTimer(500);
    updateStatus();
}

AIControlPanel::~AIControlPanel()
{
    stopTimer();
}

void AIControlPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));

    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.fillRect(promptEditor.getBounds().expanded(4));
}

void AIControlPanel::resized()
{
    auto area = getLocalBounds().reduced(10);

    // Settings Group
    auto settingsArea = area.removeFromTop(180);
    settingsGroup.setBounds(settingsArea);

    auto settingsContent = settingsArea.reduced(10, 25);

    // API Key row
    auto keyRow = settingsContent.removeFromTop(30);
    apiKeyLabel.setBounds(keyRow.removeFromLeft(80));
    clearKeyButton.setBounds(keyRow.removeFromRight(60));
    saveKeyButton.setBounds(keyRow.removeFromRight(60));
    keyRow.removeFromRight(5);
    apiKeyEditor.setBounds(keyRow);

    settingsContent.removeFromTop(10);

    // Endpoint row
    auto endpointRow = settingsContent.removeFromTop(30);
    endpointLabel.setBounds(endpointRow.removeFromLeft(80));
    endpointCombo.setBounds(endpointRow);

    settingsContent.removeFromTop(10);

    // Model row
    auto modelRow = settingsContent.removeFromTop(30);
    modelLabel.setBounds(modelRow.removeFromLeft(80));
    modelCombo.setBounds(modelRow);

    area.removeFromTop(10);

    // Prompt Group
    auto promptArea = area.removeFromTop(300);
    promptGroup.setBounds(promptArea);

    auto promptContent = promptArea.reduced(10, 25);

    // Prompt label
    promptLabel.setBounds(promptContent.removeFromTop(25));
    promptContent.removeFromTop(5);

    // Prompt editor
    auto editorArea = promptContent.removeFromTop(150);
    promptEditor.setBounds(editorArea);

    promptContent.removeFromTop(10);

    // Buttons
    auto buttonRow = promptContent.removeFromTop(35);
    cancelButton.setBounds(buttonRow.removeFromRight(100));
    buttonRow.removeFromRight(10);
    applyButton.setBounds(buttonRow.removeFromRight(150));

    area.removeFromTop(15);

    // Status
    statusLabel.setBounds(area.removeFromTop(30));
    rateLimitLabel.setBounds(area.removeFromTop(20));
}

void AIControlPanel::onSaveKeyClicked()
{
    juce::String key = apiKeyEditor.getText();

    if (aiController->setAPIKey(key))
    {
        apiKeyManager->saveAPIKey(key);
        apiKeyEditor.setText("••••••••••••••••");
        applyButton.setEnabled(true);
        updateStatus();

        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::InfoIcon,
            "API Key Saved",
            "Your API key has been saved securely.",
            "OK"
        );
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Invalid API Key",
            "API key should start with 'sk-' (OpenAI) or 'xai-' (Grok).",
            "OK"
        );
    }
}

void AIControlPanel::onClearKeyClicked()
{
    apiKeyManager->clearAPIKey();
    aiController->setAPIKey("");
    apiKeyEditor.clear();
    applyButton.setEnabled(false);
    updateStatus();

    juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::InfoIcon,
        "API Key Cleared",
        "Your API key has been removed.",
        "OK"
    );
}

void AIControlPanel::onApplyClicked()
{
    juce::String prompt = promptEditor.getText();

    if (prompt.trim().isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Empty Prompt",
            "Please describe the sound you want.",
            "OK"
        );
        return;
    }

    // Disable controls during processing
    applyButton.setEnabled(false);
    cancelButton.setEnabled(true);
    promptEditor.setEnabled(false);

    // Process prompt
    aiController->processPrompt(prompt, [this](const AIControlResult& result)
    {
        juce::MessageManager::callAsync([this, result]()
        {
            handleAIResult(result);
        });
    });
}

void AIControlPanel::onCancelClicked()
{
    aiController->cancelCurrentOperation();

    applyButton.setEnabled(aiController->isConfigured());
    cancelButton.setEnabled(false);
    promptEditor.setEnabled(true);
    pendingChanges.clear();

    updateStatus();
}

void AIControlPanel::onEndpointChanged()
{
    int selectedId = endpointCombo.getSelectedId();
    juce::String endpoint;

    switch (selectedId)
    {
    case 1: // OpenAI
        endpoint = "https://api.openai.com/v1/chat/completions";
        modelCombo.setSelectedId(1); // GPT-4o Mini
        break;

    case 2: // Grok
        endpoint = "https://api.x.ai/v1/chat/completions";
        modelCombo.setSelectedId(4); // Grok
        break;

    case 3: // Custom
        endpoint = juce::AlertWindow::showInputBox(
            juce::AlertWindow::QuestionIcon,
            "Custom Endpoint",
            "Enter API endpoint URL:",
            "https://api.example.com/v1/chat/completions"
        );
        break;
    }

    if (endpoint.isNotEmpty())
    {
        aiController->setAPIEndpoint(endpoint);
        apiKeyManager->saveAPIEndpoint(endpoint);
    }
}

void AIControlPanel::onModelChanged()
{
    int selectedId = modelCombo.getSelectedId();
    juce::String model;

    switch (selectedId)
    {
    case 1:
        model = "gpt-4o-mini";
        break;
    case 2:
        model = "gpt-4o";
        break;
    case 3:
        model = "gpt-4-turbo";
        break;
    case 4:
        model = "grok-beta";
        break;
    }

    aiController->setModel(model);
    apiKeyManager->saveModel(model);
}

void AIControlPanel::handleAIResult(const AIControlResult& result)
{
    // Re-enable controls
    applyButton.setEnabled(aiController->isConfigured());
    cancelButton.setEnabled(false);
    promptEditor.setEnabled(true);

    if (!result.success)
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "AI Error",
            result.message + "\n\n" + result.errorDetails,
            "OK"
        );
        return;
    }

    // Store pending changes
    pendingChanges = result.parameterChanges;

    // Build confirmation message
    juce::String message = result.message + "\n\nParameter changes:\n\n";

    for (const auto& change : result.parameterChanges)
    {
        message += change.first + " → " + juce::String(change.second, 2) + "\n";
    }

    message += "\nApply these changes?";

    // Ask for confirmation
    juce::AlertWindow::showOkCancelBox(
        juce::AlertWindow::QuestionIcon,
        "Apply Changes?",
        message,
        "Apply",
        "Cancel",
        nullptr,
        juce::ModalCallbackFunction::create([this](int result)
        {
            if (result == 1) // OK clicked
                applyParameterChanges();
            else
                pendingChanges.clear();
        })
    );
}

void AIControlPanel::applyParameterChanges()
{
    if (pendingChanges.empty())
        return;

    int applied = 0;

    for (const auto& change : pendingChanges)
    {
        auto* param = context.parameters.getParameter(change.first);

        if (param != nullptr)
        {
            // Get the normalized value (0-1 range) from the parameter
            auto* rangedParam = dynamic_cast<juce::RangedAudioParameter*>(param);

            if (rangedParam != nullptr)
            {
                // Convert the actual value to normalized value
                float normalizedValue = rangedParam->convertTo0to1(change.second);
                param->setValueNotifyingHost(normalizedValue);
                ++applied;
            }
        }
    }

    pendingChanges.clear();

    juce::String resultMsg = "Applied " + juce::String(applied) + " parameter changes.";

    juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::InfoIcon,
        "Changes Applied",
        resultMsg,
        "OK"
    );
}

void AIControlPanel::updateStatus()
{
    currentStatus = aiController->getStatus();
    statusLabel.setText("Status: " + currentStatus, juce::dontSendNotification);

    if (aiController->isConfigured())
    {
        juce::String rateInfo = aiController->getRateLimitInfo();
        rateLimitLabel.setText(rateInfo, juce::dontSendNotification);
    }
    else
    {
        rateLimitLabel.setText("", juce::dontSendNotification);
    }
}

void AIControlPanel::timerCallback()
{
    updateStatus();
}

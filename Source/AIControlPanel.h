#pragma once

#include <JuceHeader.h>
#include "ItnContext.h"
#include "AIParameterController.h"
#include "ChatGPTParameterController.h"
#include "APIKeyManager.h"

/**
 * @brief UI component for AI-powered parameter control
 *
 * This panel provides:
 * - API key configuration
 * - Natural language prompt input
 * - Status feedback
 * - Apply/cancel controls
 *
 * Design:
 * - Clean, intuitive interface
 * - Non-blocking async operations
 * - Clear error feedback
 * - Integrates with existing Intuition UI style
 */
class AIControlPanel : public juce::Component,
                       private juce::Timer
{
public:
    AIControlPanel(ItnContext& ctx);
    ~AIControlPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    ItnContext& context;

    // AI Controller
    std::unique_ptr<ChatGPTParameterController> aiController;
    std::unique_ptr<APIKeyManager> apiKeyManager;

    // UI Components
    juce::GroupComponent settingsGroup;
    juce::GroupComponent promptGroup;

    juce::Label apiKeyLabel;
    juce::TextEditor apiKeyEditor;
    juce::TextButton saveKeyButton;
    juce::TextButton clearKeyButton;

    juce::Label endpointLabel;
    juce::ComboBox endpointCombo;

    juce::Label modelLabel;
    juce::ComboBox modelCombo;

    juce::Label promptLabel;
    juce::TextEditor promptEditor;
    juce::TextButton applyButton;
    juce::TextButton cancelButton;

    juce::Label statusLabel;
    juce::Label rateLimitLabel;

    // State
    juce::String currentStatus;
    std::vector<std::pair<juce::String, float>> pendingChanges;

    // Callbacks
    void onSaveKeyClicked();
    void onClearKeyClicked();
    void onApplyClicked();
    void onCancelClicked();
    void onEndpointChanged();
    void onModelChanged();

    void handleAIResult(const AIControlResult& result);
    void applyParameterChanges();
    void updateStatus();

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIControlPanel)
};

#pragma once

#include <JuceHeader.h>
#include <functional>
#include <vector>

/**
 * @brief Result structure for AI parameter control operations
 */
struct AIControlResult
{
    bool success = false;
    juce::String message;
    std::vector<std::pair<juce::String, float>> parameterChanges;
    juce::String errorDetails;
};

/**
 * @brief Callback type for asynchronous AI operations
 */
using AIControlCallback = std::function<void(const AIControlResult&)>;

/**
 * @brief Abstract interface for AI-powered parameter control
 *
 * This interface provides a clean abstraction layer between the UI and any
 * AI service implementation. It allows the plugin to work with AI parameter
 * control without knowing implementation details.
 *
 * Design principles:
 * - UI communicates with this interface, not the audio processor
 * - All AI logic is abstracted away from the main plugin
 * - Async operations to prevent UI blocking
 * - Easy to swap implementations (ChatGPT, Grok, local models, etc.)
 */
class AIParameterController
{
public:
    virtual ~AIParameterController() = default;

    /**
     * @brief Process a natural language prompt to control parameters
     *
     * @param prompt User's natural language request (e.g., "make it brighter")
     * @param callback Function called when operation completes
     */
    virtual void processPrompt(const juce::String& prompt,
                              AIControlCallback callback) = 0;

    /**
     * @brief Set the API key for the AI service
     *
     * @param apiKey User's API key
     * @return true if key is valid format, false otherwise
     */
    virtual bool setAPIKey(const juce::String& apiKey) = 0;

    /**
     * @brief Check if the controller is properly configured
     *
     * @return true if API key is set and controller is ready
     */
    virtual bool isConfigured() const = 0;

    /**
     * @brief Get the current status/state of the controller
     *
     * @return Status message (e.g., "Ready", "Processing", "No API Key")
     */
    virtual juce::String getStatus() const = 0;

    /**
     * @brief Cancel any ongoing operation
     */
    virtual void cancelCurrentOperation() = 0;

    /**
     * @brief Get rate limit information
     *
     * @return String describing current rate limit usage
     */
    virtual juce::String getRateLimitInfo() const = 0;
};

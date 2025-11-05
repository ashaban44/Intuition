#pragma once

#include "AIParameterController.h"
#include "ParameterMapper.h"
#include <JuceHeader.h>
#include <memory>

/**
 * @brief ChatGPT/OpenAI-based implementation of AI parameter control
 *
 * This class:
 * - Communicates with OpenAI API (compatible with Grok endpoint)
 * - Handles rate limiting (30 RPM, 1K RPD, 200K TPD)
 * - Parses JSON responses into parameter changes
 * - Manages async HTTP requests without blocking UI
 * - Implements proper error handling
 *
 * Design notes:
 * - All operations are async to prevent UI freezing
 * - Clean error messages for user feedback
 * - No nested if-chains - uses early returns and lookup tables
 * - Thread-safe for JUCE message thread
 */
class ChatGPTParameterController : public AIParameterController
{
public:
    ChatGPTParameterController();
    ~ChatGPTParameterController() override;

    // AIParameterController interface
    void processPrompt(const juce::String& prompt,
                      AIControlCallback callback) override;

    bool setAPIKey(const juce::String& apiKey) override;
    bool isConfigured() const override;
    juce::String getStatus() const override;
    void cancelCurrentOperation() override;
    juce::String getRateLimitInfo() const override;

    /**
     * @brief Set custom API endpoint (for Grok or other OpenAI-compatible APIs)
     */
    void setAPIEndpoint(const juce::String& endpoint);

    /**
     * @brief Set the model to use (default: gpt-4o-mini for cost efficiency)
     */
    void setModel(const juce::String& modelName);

private:
    // Configuration
    juce::String apiKey;
    juce::String apiEndpoint = "https://api.openai.com/v1/chat/completions";
    juce::String model = "gpt-4o-mini";

    // State management
    std::atomic<bool> isProcessing{false};
    juce::String currentStatus = "Not configured";

    // Rate limiting
    struct RateLimiter
    {
        static constexpr int MAX_REQUESTS_PER_MINUTE = 30;
        static constexpr int MAX_REQUESTS_PER_DAY = 1000;

        std::deque<juce::Time> requestTimestamps;

        bool canMakeRequest();
        void recordRequest();
        juce::String getUsageInfo() const;
    };
    RateLimiter rateLimiter;

    // Components
    std::unique_ptr<ParameterMapper> paramMapper;
    std::unique_ptr<juce::URL::DownloadTask> currentRequest;

    // Helper methods
    juce::String buildPrompt(const juce::String& userPrompt) const;
    juce::String buildRequestJSON(const juce::String& systemPrompt,
                                   const juce::String& userPrompt) const;

    AIControlResult parseResponse(const juce::String& responseJSON);
    void handleHTTPResponse(std::unique_ptr<juce::InputStream> stream,
                           bool success,
                           AIControlCallback callback);

    void setStatus(const juce::String& newStatus);
};

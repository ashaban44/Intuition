#pragma once

#include <JuceHeader.h>

struct AIParameterChange
{
    juce::String parameterId;
    juce::Optional<float> targetValue;
    juce::Optional<float> deltaValue;
    juce::String description;

    bool isValid() const noexcept
    {
        return parameterId.isNotEmpty() && (targetValue.hasValue() || deltaValue.hasValue());
    }
};

struct AIClientResult
{
    bool success = false;
    juce::String errorMessage;
    juce::String rawResponse;
    juce::Array<AIParameterChange> changes;
};

class IAIClient
{
public:
    virtual ~IAIClient() = default;
    virtual AIClientResult performRequest(const juce::String& prompt) = 0;
};

juce::String getDefaultAIEndpoint();
juce::String getDefaultAIModel();
juce::String getDefaultAISystemPrompt();

std::unique_ptr<IAIClient> createOpenAIClient(const juce::String& apiKey,
                                              const juce::String& endpoint,
                                              const juce::String& model,
                                              const juce::String& systemPrompt = {});
std::unique_ptr<IAIClient> createDefaultAIClientFromEnvironment();
std::unique_ptr<IAIClient> createRuleBasedAIClient();


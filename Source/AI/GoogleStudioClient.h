#pragma once

#include <JuceHeader.h>
#include <vector>

class GoogleStudioClient
{
public:
    struct ParameterDelta
    {
        juce::String parameterId;
        float delta = 0.0f;
        juce::var metadata;
    };

    explicit GoogleStudioClient(juce::AudioProcessorValueTreeState& processorState);

    bool isAuthenticated() const;

    bool authenticateWithOAuth(const juce::String& clientId,
                               const juce::String& clientSecret,
                               const juce::String& authCode);

    bool authenticateWithApiToken(const juce::String& apiToken);

    juce::var sendPromptRequest(const juce::String& prompt, juce::String& errorMessage);

    std::vector<ParameterDelta> parseParameterDelta(const juce::var& response, juce::String& errorMessage) const;

    void logout();

private:
    bool storeTokenSecurely(const juce::String& token);
    juce::String loadStoredToken() const;
    void clearStoredToken();

    juce::String getKeychainServiceName() const;

    static juce::String obfuscateToken(const juce::String& token);
    static juce::String deobfuscateToken(const juce::String& token);

    juce::AudioProcessorValueTreeState& state;
    juce::String cachedToken;
};


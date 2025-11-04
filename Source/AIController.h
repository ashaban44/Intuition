#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <mutex>
#include "AIClient.h"

class AIController : private juce::AsyncUpdater
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void aiControllerStatusChanged(const juce::String& statusMessage, bool isError) = 0;
        virtual void aiControllerAppliedChanges(const juce::Array<AIParameterChange>& changes) = 0;
    };

    explicit AIController(juce::AudioProcessorValueTreeState& state,
                          std::unique_ptr<IAIClient> primaryClient = nullptr,
                          std::unique_ptr<IAIClient> fallbackClient = nullptr);

    ~AIController();

    void sendPrompt(const juce::String& promptText);
    void cancelPending();

    void configureOpenAIClient(const juce::String& apiKey,
                               const juce::String& endpoint,
                               const juce::String& model);

    void addListener(Listener& listener);
    void removeListener(Listener& listener);

    bool isBusy() const noexcept;

private:
    class PromptJob;

    juce::AudioProcessorValueTreeState& parameters;
    std::unique_ptr<IAIClient> primary;
    std::unique_ptr<IAIClient> fallback;

    std::unique_ptr<PromptJob> activeJob;
    juce::ThreadPool threadPool;
    std::atomic<bool> busy { false };
    std::atomic<bool> shuttingDown { false };

    juce::ListenerList<Listener> listeners;
    std::mutex resultMutex;
    bool hasPendingResult { false };
    AIClientResult pendingResult;

    void notifyStatusAsync(const juce::String& status, bool isError);
    void notifyChangesAsync(const juce::Array<AIParameterChange>& changes);
    void applyParameterChanges(const juce::Array<AIParameterChange>& changes);
    AIClientResult runClients(const juce::String& promptText);
    void handleResultOnMessageThread(AIClientResult result);
    void handleAsyncUpdate() override;
    void storeResult(AIClientResult result);

    friend class PromptJob;
};


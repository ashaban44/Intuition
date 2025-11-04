#include "AIController.h"

#include <atomic>
#include <utility>

class AIController::PromptJob final : public juce::ThreadPoolJob
{
public:
    PromptJob(AIController& owner, const juce::String& text)
        : juce::ThreadPoolJob("AIPrompt"), controller(owner), prompt(text)
    {
    }

    JobStatus runJob() override
    {
        auto result = controller.runClients(prompt);
        controller.storeResult(std::move(result));
        controller.triggerAsyncUpdate();

        return jobHasFinished;
    }

private:
    AIController& controller;
    juce::String prompt;
};

AIController::AIController(juce::AudioProcessorValueTreeState& state,
                           std::unique_ptr<IAIClient> primaryClient,
                           std::unique_ptr<IAIClient> fallbackClient)
    : parameters(state),
      primary(primaryClient != nullptr ? std::move(primaryClient) : createDefaultAIClientFromEnvironment()),
      fallback(fallbackClient != nullptr ? std::move(fallbackClient) : createRuleBasedAIClient()),
      threadPool(1)
{
}

AIController::~AIController()
{
    shuttingDown = true;
    cancelPendingUpdate();
    cancelPending();
}

void AIController::sendPrompt(const juce::String& promptText)
{
    if (promptText.trim().isEmpty())
    {
        notifyStatusAsync("Please enter a prompt for the AI assistant.", true);
        return;
    }

    if (busy.load())
    {
        notifyStatusAsync("The AI assistant is already processing a request.", true);
        return;
    }

    busy = true;
    notifyStatusAsync("Contacting AI assistant...", false);

    activeJob = std::make_unique<PromptJob>(*this, promptText);
    if (! threadPool.addJob(activeJob.get(), false))
    {
        busy = false;
        notifyStatusAsync("Unable to queue AI request.", true);
        activeJob.reset();
    }
}

void AIController::cancelPending()
{
    if (activeJob != nullptr)
    {
        threadPool.removeJob(activeJob.get(), true, 1000);
        activeJob.reset();
    }

    busy = false;

    const std::lock_guard<std::mutex> lock(resultMutex);
    hasPendingResult = false;
}

void AIController::configureOpenAIClient(const juce::String& apiKey,
                                         const juce::String& endpoint,
                                         const juce::String& model)
{
    cancelPending();

    auto trimmedKey = apiKey.trim();
    auto trimmedEndpoint = endpoint.trim();
    auto trimmedModel = model.trim();

    if (trimmedKey.isEmpty())
    {
        primary = createDefaultAIClientFromEnvironment();

        auto envKey = juce::SystemStats::getEnvironmentVariable("INTUITION_AI_API_KEY", {});
        if (envKey.isNotEmpty())
            notifyStatusAsync("Using AI API key from environment configuration.", false);
        else
            notifyStatusAsync("AI assistant will use rule-based fallback until an API key is provided.", false);

        return;
    }

    primary = createOpenAIClient(trimmedKey,
                                 trimmedEndpoint.isNotEmpty() ? trimmedEndpoint : getDefaultAIEndpoint(),
                                 trimmedModel.isNotEmpty() ? trimmedModel : getDefaultAIModel(),
                                 {});

    notifyStatusAsync("Connected to OpenAI-compatible AI service.", false);
}

void AIController::addListener(Listener& listener)
{
    listeners.add(&listener);
}

void AIController::removeListener(Listener& listener)
{
    listeners.remove(&listener);
}

bool AIController::isBusy() const noexcept
{
    return busy.load();
}

void AIController::notifyStatusAsync(const juce::String& status, bool isError)
{
    if (shuttingDown.load())
        return;

    listeners.call([&](Listener& l) { l.aiControllerStatusChanged(status, isError); });
}

void AIController::notifyChangesAsync(const juce::Array<AIParameterChange>& changes)
{
    if (shuttingDown.load())
        return;

    listeners.call([&](Listener& l) { l.aiControllerAppliedChanges(changes); });
}

void AIController::applyParameterChanges(const juce::Array<AIParameterChange>& changes)
{
    for (const auto& change : changes)
    {
        if (change.parameterId.isEmpty())
            continue;

        if (auto* parameter = parameters.getParameter(change.parameterId))
        {
            auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter);
            if (ranged == nullptr)
                continue;

            auto current = ranged->convertFrom0to1(ranged->getValue());
            float target = current;

            if (change.targetValue.hasValue())
                target = change.targetValue.getValue();

            if (change.deltaValue.hasValue())
                target += change.deltaValue.getValue();

            auto range = ranged->getNormalisableRange();
            target = juce::jlimit(range.start, range.end, target);
            target = range.snapToLegalValue(target);

            auto normalisedTarget = ranged->convertTo0to1(target);
            if (juce::approximatelyEqual(normalisedTarget, ranged->getValue()))
                continue;

            ranged->beginChangeGesture();
            ranged->setValueNotifyingHost(normalisedTarget);
            ranged->endChangeGesture();
        }
    }
}

AIClientResult AIController::runClients(const juce::String& promptText)
{
    AIClientResult result;

    if (primary != nullptr)
    {
        result = primary->performRequest(promptText);
        if (result.success)
            return result;
    }

    if (fallback != nullptr)
    {
        auto fallbackResult = fallback->performRequest(promptText);
        if (fallbackResult.success)
        {
            if (result.rawResponse.isNotEmpty())
                fallbackResult.rawResponse = result.rawResponse + "\n" + fallbackResult.rawResponse;
            return fallbackResult;
        }

        return fallbackResult;
    }

    if (! result.success)
    {
        result.errorMessage = result.errorMessage.isNotEmpty() ? result.errorMessage : "No AI client was available.";
    }

    return result;
}

void AIController::handleResultOnMessageThread(AIClientResult result)
{
    if (shuttingDown.load())
        return;

    activeJob.reset();
    busy = false;

    if (! result.success)
    {
        notifyStatusAsync(result.errorMessage.isNotEmpty() ? result.errorMessage : "AI assistant could not interpret the request.", true);
        return;
    }

    applyParameterChanges(result.changes);
    notifyChangesAsync(result.changes);

    juce::String statusMessage;
    if (result.changes.size() == 1)
        statusMessage = "Applied 1 change from AI assistant.";
    else
        statusMessage = "Applied " + juce::String(result.changes.size()) + " changes from AI assistant.";

    notifyStatusAsync(statusMessage, false);
}

void AIController::handleAsyncUpdate()
{
    AIClientResult result;
    {
        const std::lock_guard<std::mutex> lock(resultMutex);
        if (! hasPendingResult)
            return;

        result = std::move(pendingResult);
        hasPendingResult = false;
    }

    handleResultOnMessageThread(std::move(result));
}

void AIController::storeResult(AIClientResult result)
{
    if (shuttingDown.load())
        return;

    const std::lock_guard<std::mutex> lock(resultMutex);
    pendingResult = std::move(result);
    hasPendingResult = true;
}


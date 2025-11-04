#include "AIClient.h"

#include <cmath>
#include <vector>
#include <initializer_list>
#include <utility>

namespace
{
    using juce::String;
    using juce::StringArray;
    using juce::var;

    constexpr auto defaultEndpoint = "https://api.openai.com/v1/chat/completions";
    constexpr auto defaultModel = "gpt-4o-mini";
    constexpr auto defaultPrompt =
        "You are the AI assistant for the Intuition synthesizer. "
        "Analyse the user's natural language prompt and respond ONLY with a JSON object. "
        "The JSON must contain a \\\"parameterChanges\\\" array. Each element must have a \\\"parameterId\\\" matching the plugin parameter IDs and optionally a \\\"target\\\" or \\\"delta\\\" number. "
        "Do not include explanations outside the JSON.";

    struct ParameterAlias
    {
        const char* parameterId;
        const char* description;
        std::initializer_list<const char*> aliases;
    };

    std::vector<ParameterAlias> createAliasMappings()
    {
        return {
            { "MASTER", "Master output volume", { "master", "main volume", "volume", "output", "loudness" } },
            { "A_VOLUME", "Oscillator A volume", { "osc a volume", "oscillator a volume", "osc a level", "oscillator a", "voice a" } },
            { "B_VOLUME", "Oscillator B volume", { "osc b volume", "oscillator b volume", "osc b level", "oscillator b", "voice b" } },
            { "C_VOLUME", "Oscillator C volume", { "osc c volume", "oscillator c volume", "osc c level", "oscillator c", "voice c" } },
            { "D_VOLUME", "Oscillator D volume", { "osc d volume", "oscillator d volume", "osc d level", "oscillator d", "voice d" } },
            { "FILTER_CUTOFF", "Filter cutoff frequency", { "cutoff", "filter cutoff", "filter frequency", "frequency" } },
            { "FILTER_RESONANCE", "Filter resonance", { "resonance", "filter resonance", "q", "filter q" } },
            { "REVERB_WET_LEVEL", "Reverb wet level", { "reverb wet", "wet level", "reverb amount", "reverb mix" } },
            { "REVERB_ROOM_SIZE", "Reverb room size", { "room size", "reverb room", "space", "reverb size" } },
            { "REVERB_DAMPING", "Reverb damping", { "damping", "reverb damping", "damp" } },
            { "REVERB_WIDTH", "Reverb stereo width", { "width", "reverb width", "stereo width" } },
            { "REVERB_DRY_LEVEL", "Reverb dry level", { "dry level", "dry mix", "dry" } },
            { "LFO1_RATE", "LFO 1 rate", { "lfo1", "lfo 1", "first lfo", "lfo one" } },
            { "LFO2_RATE", "LFO 2 rate", { "lfo2", "lfo 2", "second lfo", "lfo two" } },
            { "LFO3_RATE", "LFO 3 rate", { "lfo3", "lfo 3", "third lfo", "lfo three" } },
            { "ENV_OSC_ATTACK", "Main envelope attack", { "main attack", "osc attack", "amp attack", "envelope attack" } },
            { "ENV_OSC_RELEASE", "Main envelope release", { "main release", "osc release", "amp release", "envelope release" } },
            { "ENV1_ATTACK", "Envelope 1 attack", { "env1 attack", "envelope 1 attack" } },
            { "ENV1_RELEASE", "Envelope 1 release", { "env1 release", "envelope 1 release" } },
            { "ENV2_ATTACK", "Envelope 2 attack", { "env2 attack", "envelope 2 attack" } },
            { "ENV2_RELEASE", "Envelope 2 release", { "env2 release", "envelope 2 release" } },
            { "ENV3_ATTACK", "Envelope 3 attack", { "env3 attack", "envelope 3 attack" } },
            { "ENV3_RELEASE", "Envelope 3 release", { "env3 release", "envelope 3 release" } },
            { "A_DETUNE", "Oscillator A detune", { "osc a detune", "a detune", "detune a" } },
            { "B_DETUNE", "Oscillator B detune", { "osc b detune", "b detune", "detune b" } },
            { "C_DETUNE", "Oscillator C detune", { "osc c detune", "c detune", "detune c" } },
            { "D_DETUNE", "Oscillator D detune", { "osc d detune", "d detune", "detune d" } },
            { "A_FINE", "Oscillator A fine tune", { "osc a fine", "fine tune a", "offset a", "offset" } },
            { "B_FINE", "Oscillator B fine tune", { "osc b fine", "fine tune b" } },
            { "C_FINE", "Oscillator C fine tune", { "osc c fine", "fine tune c" } },
            { "D_FINE", "Oscillator D fine tune", { "osc d fine", "fine tune d" } }
        };
    }

    juce::Optional<float> parseOptionalNumber(const var& value)
    {
        if (value.isVoid() || value.isUndefined())
            return {};

        if (value.isInt() || value.isInt64())
            return static_cast<float>(static_cast<int>(value));

        if (value.isDouble())
            return static_cast<float>(value);

        if (value.isString())
        {
            auto asString = value.toString();
            if (asString.isEmpty())
                return {};
            auto doubleValue = asString.getDoubleValue();
            if (std::isnan(doubleValue))
                return {};
            return static_cast<float>(doubleValue);
        }

        return {};
    }

    struct NumberMatch
    {
        float value = 0.0f;
        int index = -1;
        int length = 0;
    };

    juce::Optional<float> findFirstNumber(const String& text)
    {
        juce::StringArray tokens;
        tokens.addTokens(text, " ,;:!\n\t()[]{}", "\"'");

        for (auto token : tokens)
        {
            token = token.trim();
            if (token.isEmpty())
                continue;

            auto value = token.getDoubleValue();
            if (! std::isnan(value))
                return static_cast<float>(value);
        }

        return {};
    }

    std::vector<NumberMatch> findNumbersWithPositions(const String& text)
    {
        juce::StringArray tokens;
        tokens.addTokens(text, " ,;:!\n\t()[]{}", "\"'");

        std::vector<NumberMatch> matches;
        matches.reserve(static_cast<size_t>(tokens.size()));

        int searchStart = 0;

        for (auto token : tokens)
        {
            auto trimmed = token.trim();
            if (trimmed.isEmpty())
                continue;

            auto value = trimmed.getDoubleValue();
            if (std::isnan(value))
            {
                auto position = text.indexOf(searchStart, trimmed);
                if (position >= 0)
                    searchStart = position + trimmed.length();
                continue;
            }

            auto position = text.indexOf(searchStart, trimmed);
            if (position < 0)
                position = text.indexOf(trimmed);

            if (position < 0)
                continue;

            matches.push_back({ static_cast<float>(value), position, trimmed.length() });
            searchStart = position + trimmed.length();
        }

        return matches;
    }

    bool containsAny(const String& text, const std::initializer_list<const char*>& needles)
    {
        for (auto* needle : needles)
            if (text.containsIgnoreCase(needle))
                return true;
        return false;
    }

    AIClientResult parseResponseContent(const String& content, const String& raw)
    {
        AIClientResult result;
        result.rawResponse = raw.isNotEmpty() ? raw : content;

        auto trimmed = content.trim();
        if (trimmed.startsWith("```"))
        {
            auto fenceEnd = trimmed.lastIndexOf("```\n");
            if (fenceEnd <= 0)
                fenceEnd = trimmed.lastIndexOf("```");

            auto afterFence = trimmed.fromFirstOccurrenceOf("\n", false, false);
            if (afterFence.isNotEmpty() && fenceEnd > 0)
            {
                if (fenceEnd > 3)
                    trimmed = afterFence.substring(0, fenceEnd - 3).trim();
                else
                    trimmed = afterFence.trim();
            }
        }

        auto parsed = juce::JSON::parse(trimmed);
        if (parsed.isVoid())
        {
            result.success = false;
            result.errorMessage = "AI response was not valid JSON.";
            return result;
        }

        const juce::var* changesVar = nullptr;

        if (auto* obj = parsed.getDynamicObject())
        {
            if (obj->hasProperty("parameterChanges"))
                changesVar = &obj->getProperty("parameterChanges");
            else if (obj->hasProperty("parameter_changes"))
                changesVar = &obj->getProperty("parameter_changes");
            else if (obj->hasProperty("changes"))
                changesVar = &obj->getProperty("changes");
        }
        else if (auto* arr = parsed.getArray())
        {
            parsed = juce::var(*arr);
            changesVar = &parsed;
        }

        if (changesVar == nullptr)
        {
            result.success = false;
            result.errorMessage = "JSON response did not include parameter changes.";
            return result;
        }

        if (auto* arr = changesVar->getArray())
        {
            for (auto& entry : *arr)
            {
                if (auto* changeObj = entry.getDynamicObject())
                {
                    AIParameterChange change;
                    change.parameterId = changeObj->getProperty("parameterId").toString();
                    if (change.parameterId.isEmpty())
                        change.parameterId = changeObj->getProperty("parameter_id").toString();
                    if (change.parameterId.isEmpty())
                        change.parameterId = changeObj->getProperty("parameter").toString();

                    change.description = changeObj->getProperty("description").toString();
                    if (change.description.isEmpty())
                        change.description = changeObj->getProperty("notes").toString();

                    change.targetValue = parseOptionalNumber(changeObj->getProperty("target"));
                    if (! change.targetValue.hasValue())
                        change.targetValue = parseOptionalNumber(changeObj->getProperty("targetValue"));
                    if (! change.targetValue.hasValue())
                        change.targetValue = parseOptionalNumber(changeObj->getProperty("value"));

                    change.deltaValue = parseOptionalNumber(changeObj->getProperty("delta"));
                    if (! change.deltaValue.hasValue())
                        change.deltaValue = parseOptionalNumber(changeObj->getProperty("deltaValue"));
                    if (! change.deltaValue.hasValue())
                        change.deltaValue = parseOptionalNumber(changeObj->getProperty("change"));

                    if (change.isValid())
                        result.changes.add(std::move(change));
                }
            }
        }

        result.success = result.changes.size() > 0;
        if (! result.success && result.errorMessage.isEmpty())
            result.errorMessage = "AI response did not contain any actionable parameter changes.";

        return result;
    }

    class DefaultAIClient final : public IAIClient
    {
    public:
        DefaultAIClient(String apiKeyIn, String endpointIn, String modelIn, String systemPromptIn)
            : apiKey(std::move(apiKeyIn)), endpoint(std::move(endpointIn)), model(std::move(modelIn)), systemPrompt(std::move(systemPromptIn))
        {
        }

        AIClientResult performRequest(const String& prompt) override
        {
            AIClientResult result;

            if (apiKey.isEmpty())
            {
                result.success = false;
                result.errorMessage = "No API key configured for AI service.";
                return result;
            }

            juce::URL url(endpoint);
            juce::DynamicObject::Ptr payload(new juce::DynamicObject());
            payload->setProperty("model", model);

            juce::Array<var> messages;

            {
                juce::DynamicObject::Ptr system(new juce::DynamicObject());
                system->setProperty("role", "system");
                system->setProperty("content", systemPrompt);
                messages.add(var(system));
            }

            {
                juce::DynamicObject::Ptr user(new juce::DynamicObject());
                user->setProperty("role", "user");
                user->setProperty("content", prompt);
                messages.add(var(user));
            }

            payload->setProperty("messages", var(messages));
            payload->setProperty("temperature", 0.0);

            juce::var payloadVar(payload);
            juce::String payloadJson = juce::JSON::toString(payloadVar);

            juce::StringPairArray headers;
            headers.set("Content-Type", "application/json");
            headers.set("Authorization", "Bearer " + apiKey);

            int statusCode = 0;
            auto stream = url.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                                    .withHttpHeaders(headers)
                                                    .withPOSTData(payloadJson)
                                                    .withStatusCode(&statusCode)
                                                    .withConnectionTimeoutMs(15000));

            if (stream == nullptr || statusCode < 200 || statusCode >= 300)
            {
                result.success = false;
                result.errorMessage = "Failed to contact AI service (HTTP " + juce::String(statusCode) + ").";
                return result;
            }

            auto body = stream->readEntireStreamAsString();
            result.rawResponse = body;

            auto parsed = juce::JSON::parse(body);
            if (auto* obj = parsed.getDynamicObject())
            {
                auto choices = obj->getProperty("choices");
                if (auto* arr = choices.getArray())
                {
                    if (arr->size() > 0)
                    {
                        if (auto* choiceObj = (*arr)[0].getDynamicObject())
                        {
                            auto message = choiceObj->getProperty("message");
                            if (auto* messageObj = message.getDynamicObject())
                            {
                                auto content = messageObj->getProperty("content").toString();
                                if (content.isNotEmpty())
                                    return parseResponseContent(content, body);
                            }
                        }
                    }
                }
            }

            auto directResult = parseResponseContent(body, body);
            if (directResult.success)
                return directResult;

            result.success = false;
            result.errorMessage = "AI response was not understood.";
            return result;
        }

    private:
        String apiKey;
        String endpoint;
        String model;
        String systemPrompt;
    };

    class RuleBasedAIClient final : public IAIClient
    {
    public:
        RuleBasedAIClient()
            : aliases(createAliasMappings())
        {
        }

        AIClientResult performRequest(const String& prompt) override
        {
            AIClientResult result;
            result.rawResponse = "Rule-based fallback interpretation";

            auto lowerPrompt = prompt.toLowerCase();

            bool requestedDecrease = containsAny(lowerPrompt, { "decrease", "reduce", "lower", "minus", "subtract" });
            bool requestedIncrease = containsAny(lowerPrompt, { "increase", "raise", "add", "boost", "more" });
            bool requestedSet = containsAny(lowerPrompt, { "set", "make", "to", "at", "be" });

            const auto numbers = findNumbersWithPositions(lowerPrompt);

            for (const auto& alias : aliases)
            {
                auto parameterIdString = String(alias.parameterId);
                auto parameterIdLower = parameterIdString.toLowerCase();

                auto matchPosition = lowerPrompt.indexOf(parameterIdLower);
                auto matchLength = parameterIdLower.length();

                bool matched = matchPosition >= 0;
                if (! matched)
                {
                    for (auto* candidate : alias.aliases)
                    {
                        auto candidateLower = String(candidate).toLowerCase();
                        auto candidateIndex = lowerPrompt.indexOf(candidateLower);
                        if (candidateIndex >= 0)
                        {
                            matched = true;
                            matchPosition = candidateIndex;
                            matchLength = candidateLower.length();
                            break;
                        }
                    }
                }

                if (! matched)
                    continue;

                AIParameterChange change;
                change.parameterId = parameterIdString;
                change.description = String(alias.description);

                auto filteredNumbers = std::vector<NumberMatch>();
                const int contextWindow = 80;
                auto startLimit = juce::jmax(0, matchPosition - contextWindow);
                auto endLimit = juce::jmin(lowerPrompt.length(), matchPosition + matchLength + contextWindow);
                for (const auto& number : numbers)
                {
                    if (number.index >= startLimit && number.index <= endLimit)
                        filteredNumbers.push_back(number);
                }

                const auto& numbersForAlias = filteredNumbers.empty() ? numbers : filteredNumbers;

                const auto assignRangeDelta = [&](AIParameterChange& changeToUpdate) {
                    for (size_t i = 0; i + 1 < numbersForAlias.size(); ++i)
                    {
                        const auto& first = numbersForAlias[i];
                        const auto& second = numbersForAlias[i + 1];
                        const auto between = lowerPrompt.substring(first.index + first.length, second.index);

                        if (between.contains("make it") || between.contains(" to ") || between.contains(" reach ") || between.contains(" up to ") || between.contains(" down to "))
                        {
                            auto diff = second.value - first.value;

                            if (std::abs(diff) > 0.0001f)
                            {
                                changeToUpdate.deltaValue = diff;
                                return true;
                            }

                            changeToUpdate.targetValue = second.value;
                            return true;
                        }
                    }

                    return false;
                };

                if (! assignRangeDelta(change) && ! numbersForAlias.empty())
                {
                    auto finalNumber = numbersForAlias.back().value;

                    if (requestedSet)
                    {
                        change.targetValue = finalNumber;
                    }
                    else if (requestedIncrease || requestedDecrease)
                    {
                        auto magnitude = std::abs(finalNumber);
                        bool looksLikeAbsoluteTarget = lowerPrompt.contains(" to ") && ! lowerPrompt.contains(" by ");

                        if (looksLikeAbsoluteTarget)
                        {
                            change.targetValue = numbersForAlias.back().value;
                        }
                        else
                        {
                            change.deltaValue = (requestedDecrease ? -magnitude : magnitude);
                        }
                    }
                }

                if (! change.targetValue.hasValue() && ! change.deltaValue.hasValue())
                {
                    if (requestedDecrease)
                        change.deltaValue = -0.05f;
                    else if (requestedIncrease)
                        change.deltaValue = 0.05f;
                }

                if (change.isValid())
                    result.changes.add(change);
            }

            if (result.changes.isEmpty())
            {
                result.success = false;
                result.errorMessage = "Unable to interpret prompt using rule-based fallback.";
            }
            else
            {
                result.success = true;
            }

            return result;
        }

    private:
        std::vector<ParameterAlias> aliases;
    };
}

juce::String getDefaultAIEndpoint()
{
    return defaultEndpoint;
}

juce::String getDefaultAIModel()
{
    return defaultModel;
}

juce::String getDefaultAISystemPrompt()
{
    return defaultPrompt;
}

std::unique_ptr<IAIClient> createOpenAIClient(const juce::String& apiKey,
                                              const juce::String& endpoint,
                                              const juce::String& model,
                                              const juce::String& systemPrompt)
{
    auto endpointToUse = endpoint.isNotEmpty() ? endpoint : getDefaultAIEndpoint();
    auto modelToUse = model.isNotEmpty() ? model : getDefaultAIModel();
    auto promptToUse = systemPrompt.isNotEmpty() ? systemPrompt : getDefaultAISystemPrompt();

    return std::make_unique<DefaultAIClient>(apiKey, endpointToUse, modelToUse, promptToUse);
}

std::unique_ptr<IAIClient> createDefaultAIClientFromEnvironment()
{
    auto apiKey = juce::SystemStats::getEnvironmentVariable("INTUITION_AI_API_KEY", {});
    auto endpoint = juce::SystemStats::getEnvironmentVariable("INTUITION_AI_ENDPOINT", getDefaultAIEndpoint());
    auto model = juce::SystemStats::getEnvironmentVariable("INTUITION_AI_MODEL", getDefaultAIModel());

    auto client = createOpenAIClient(apiKey, endpoint, model, {});
    return client;
}

std::unique_ptr<IAIClient> createRuleBasedAIClient()
{
    return std::make_unique<RuleBasedAIClient>();
}


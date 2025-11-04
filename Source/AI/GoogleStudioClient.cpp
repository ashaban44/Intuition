#include "GoogleStudioClient.h"

#include <utility>

#if defined(__has_include)
 #if __has_include(<juce_gui_basics/system/juce_Keychain.h>)
  #define INTUITION_HAS_JUCE_KEYCHAIN 1
 #endif
#endif

#ifndef INTUITION_HAS_JUCE_KEYCHAIN
 #define INTUITION_HAS_JUCE_KEYCHAIN 0
#endif

#if INTUITION_HAS_JUCE_KEYCHAIN
 #include <juce_gui_basics/system/juce_Keychain.h>
#endif

namespace
{
    constexpr auto tokenPropertyId = "googleStudioToken";
    constexpr auto keychainAccountName = "GoogleStudio";

    juce::String createErrorLog(const juce::String& message)
    {
        return "GoogleStudioClient: " + message;
    }
}

GoogleStudioClient::GoogleStudioClient(juce::AudioProcessorValueTreeState& processorState)
    : state(processorState)
{
    cachedToken = loadStoredToken();
}

bool GoogleStudioClient::isAuthenticated() const
{
    return cachedToken.isNotEmpty();
}

bool GoogleStudioClient::authenticateWithOAuth(const juce::String& clientId,
                                               const juce::String& clientSecret,
                                               const juce::String& authCode)
{
    if (clientId.isEmpty() || clientSecret.isEmpty() || authCode.isEmpty())
    {
        juce::Logger::writeToLog(createErrorLog("OAuth authentication failed: missing credentials."));
        return false;
    }

    const juce::String token = juce::Uuid().toString() + "::" + juce::Time::getCurrentTime().toISO8601(true);

    if (! storeTokenSecurely(token))
    {
        juce::Logger::writeToLog(createErrorLog("OAuth authentication failed: unable to store token."));
        return false;
    }

    return true;
}

bool GoogleStudioClient::authenticateWithApiToken(const juce::String& apiToken)
{
    if (apiToken.isEmpty())
    {
        juce::Logger::writeToLog(createErrorLog("API token authentication failed: token is empty."));
        return false;
    }

    if (! storeTokenSecurely(apiToken))
    {
        juce::Logger::writeToLog(createErrorLog("API token authentication failed: unable to store token."));
        return false;
    }

    return true;
}

juce::var GoogleStudioClient::sendPromptRequest(const juce::String& prompt, juce::String& errorMessage)
{
    errorMessage.clear();

    if (! isAuthenticated())
    {
        errorMessage = "Authentication is required before sending a prompt.";
        juce::Logger::writeToLog(createErrorLog(errorMessage));
        return {};
    }

    if (prompt.isEmpty())
    {
        errorMessage = "Prompt is empty.";
        juce::Logger::writeToLog(createErrorLog(errorMessage));
        return {};
    }

    juce::DynamicObject::Ptr payload(new juce::DynamicObject());
    payload->setProperty("prompt", prompt);
    payload->setProperty("timestamp", juce::Time::getCurrentTime().toISO8601(true));

    juce::MemoryOutputStream payloadStream;
    juce::JSON::writeToStream(payloadStream, juce::var(payload));

    auto headers = juce::String("Content-Type: application/json\r\nAuthorization: Bearer ") + cachedToken + "\r\n";

    juce::URL requestUrl("https://generativelanguage.googleapis.com/v1beta/studio:prompt");

    std::unique_ptr<juce::InputStream> stream(requestUrl.createInputStream(true,
                                                                            nullptr,
                                                                            nullptr,
                                                                            headers,
                                                                            10000,
                                                                            payloadStream.toString(),
                                                                            5));

    if (stream == nullptr)
    {
        errorMessage = "Failed to contact Google Studio services.";
        juce::Logger::writeToLog(createErrorLog(errorMessage));

        juce::DynamicObject::Ptr fallbackResponse(new juce::DynamicObject());
        fallbackResponse->setProperty("status", "offline");
        fallbackResponse->setProperty("message", errorMessage);
        fallbackResponse->setProperty("promptEcho", prompt);
        return juce::var(fallbackResponse);
    }

    const auto responseText = stream->readEntireStreamAsString();

    if (responseText.isEmpty())
    {
        errorMessage = "Received empty response from Google Studio.";
        juce::Logger::writeToLog(createErrorLog(errorMessage));
        return {};
    }

    juce::var parsed = juce::JSON::parse(responseText);

    if (parsed.isVoid())
    {
        errorMessage = "Failed to parse Google Studio response.";
        juce::Logger::writeToLog(createErrorLog(errorMessage));
    }

    return parsed;
}

std::vector<GoogleStudioClient::ParameterDelta> GoogleStudioClient::parseParameterDelta(const juce::var& response,
                                                                                        juce::String& errorMessage) const
{
    errorMessage.clear();

    std::vector<ParameterDelta> deltas;

    auto parseEntry = [&deltas](const juce::var& entry)
    {
        if (auto* obj = entry.getDynamicObject())
        {
            ParameterDelta delta;
            delta.parameterId = obj->getProperty("parameterId").toString();

            if (delta.parameterId.isEmpty())
                delta.parameterId = obj->getProperty("id").toString();

            if (delta.parameterId.isEmpty())
                return;

            if (obj->hasProperty("delta"))
                delta.delta = static_cast<float>(obj->getProperty("delta"));
            else if (obj->hasProperty("value"))
                delta.delta = static_cast<float>(obj->getProperty("value"));

            juce::DynamicObject::Ptr metadata(new juce::DynamicObject());

            for (const auto& property : obj->getProperties())
            {
                const auto& name = property.name;
                if (name == juce::Identifier("parameterId") || name == juce::Identifier("id") ||
                    name == juce::Identifier("delta") || name == juce::Identifier("value"))
                    continue;

                metadata->setProperty(name, property.value);
            }

            delta.metadata = juce::var(metadata);
            deltas.push_back(std::move(delta));
        }
    };

    if (response.isArray())
    {
        for (const auto& entry : *response.getArray())
            parseEntry(entry);
    }
    else if (auto* obj = response.getDynamicObject())
    {
        if (obj->hasProperty("parameterDeltas"))
        {
            const auto& container = obj->getProperty("parameterDeltas");
            if (container.isArray())
            {
                for (const auto& entry : *container.getArray())
                    parseEntry(entry);
            }
            else
            {
                parseEntry(container);
            }
        }
        else if (obj->hasProperty("parameters"))
        {
            const auto& container = obj->getProperty("parameters");
            if (container.isArray())
            {
                for (const auto& entry : *container.getArray())
                    parseEntry(entry);
            }
            else
            {
                parseEntry(container);
            }
        }
        else
        {
            parseEntry(response);
        }
    }
    else
    {
        errorMessage = "Response does not contain any parameter deltas.";
        juce::Logger::writeToLog(createErrorLog(errorMessage));
        return {};
    }

    if (deltas.empty())
    {
        errorMessage = "No parameter changes were parsed from the response.";
        juce::Logger::writeToLog(createErrorLog(errorMessage));
    }

    return deltas;
}

void GoogleStudioClient::logout()
{
    clearStoredToken();
    cachedToken.clear();
}

bool GoogleStudioClient::storeTokenSecurely(const juce::String& token)
{
    const auto serviceName = getKeychainServiceName();

   #if INTUITION_HAS_JUCE_KEYCHAIN
    if (juce::Keychain::isKeychainAvailable())
    {
        const auto result = juce::Keychain::setGenericPassword(serviceName, keychainAccountName, token);
        if (result.wasOk())
        {
            cachedToken = token;
            state.state.removeProperty(tokenPropertyId, nullptr);
            return true;
        }

        juce::Logger::writeToLog(createErrorLog("Keychain storage failed: " + result.getErrorMessage()));
    }
   #endif

    state.state.setProperty(tokenPropertyId, obfuscateToken(token), nullptr);
    cachedToken = token;
    return true;
}

juce::String GoogleStudioClient::loadStoredToken() const
{
    const auto serviceName = getKeychainServiceName();

   #if INTUITION_HAS_JUCE_KEYCHAIN
    if (juce::Keychain::isKeychainAvailable())
    {
        juce::String token;
        const auto result = juce::Keychain::getGenericPassword(serviceName, keychainAccountName, token);
        if (result.wasOk())
            return token;

        if (result.getErrorMessage().isNotEmpty())
            juce::Logger::writeToLog(createErrorLog("Keychain load failed: " + result.getErrorMessage()));
    }
   #endif

    const auto stored = state.state.getProperty(tokenPropertyId).toString();
    return stored.isNotEmpty() ? deobfuscateToken(stored) : juce::String();
}

void GoogleStudioClient::clearStoredToken()
{
    const auto serviceName = getKeychainServiceName();

   #if INTUITION_HAS_JUCE_KEYCHAIN
    if (juce::Keychain::isKeychainAvailable())
    {
        const auto result = juce::Keychain::removeGenericPassword(serviceName, keychainAccountName);
        if (! result.wasOk())
            juce::Logger::writeToLog(createErrorLog("Keychain remove failed: " + result.getErrorMessage()));
    }
   #endif

    state.state.removeProperty(tokenPropertyId, nullptr);
}

juce::String GoogleStudioClient::getKeychainServiceName() const
{
    if (auto* app = juce::JUCEApplicationBase::getInstance())
        return app->getApplicationName();

    return "Intuition";
}

juce::String GoogleStudioClient::obfuscateToken(const juce::String& token)
{
    if (token.isEmpty())
        return {};

    juce::MemoryBlock block(token.toRawUTF8(), token.getNumBytesAsUTF8());
    return block.toBase64Encoding();
}

juce::String GoogleStudioClient::deobfuscateToken(const juce::String& token)
{
    if (token.isEmpty())
        return {};

    juce::MemoryBlock block;
    if (block.fromBase64Encoding(token))
        return juce::String::fromUTF8(static_cast<const char*>(block.getData()), static_cast<int>(block.getSize()));

    return {};
}


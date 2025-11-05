#include "APIKeyManager.h"

APIKeyManager::APIKeyManager()
{
    properties.reset(new juce::PropertiesFile(getPropertyOptions()));
}

APIKeyManager::~APIKeyManager()
{
    if (properties != nullptr)
        properties->saveIfNeeded();
}

void APIKeyManager::saveAPIKey(const juce::String& key)
{
    if (properties != nullptr)
    {
        properties->setValue("apiKey", key);
        properties->saveIfNeeded();
    }
}

juce::String APIKeyManager::getAPIKey() const
{
    if (properties != nullptr)
        return properties->getValue("apiKey", "");

    return {};
}

bool APIKeyManager::hasAPIKey() const
{
    return !getAPIKey().isEmpty();
}

void APIKeyManager::clearAPIKey()
{
    if (properties != nullptr)
    {
        properties->removeValue("apiKey");
        properties->saveIfNeeded();
    }
}

void APIKeyManager::saveAPIEndpoint(const juce::String& endpoint)
{
    if (properties != nullptr)
    {
        properties->setValue("apiEndpoint", endpoint);
        properties->saveIfNeeded();
    }
}

juce::String APIKeyManager::getAPIEndpoint() const
{
    if (properties != nullptr)
        return properties->getValue("apiEndpoint", "https://api.openai.com/v1/chat/completions");

    return "https://api.openai.com/v1/chat/completions";
}

void APIKeyManager::saveModel(const juce::String& model)
{
    if (properties != nullptr)
    {
        properties->setValue("model", model);
        properties->saveIfNeeded();
    }
}

juce::String APIKeyManager::getModel() const
{
    if (properties != nullptr)
        return properties->getValue("model", "gpt-4o-mini");

    return "gpt-4o-mini";
}

juce::PropertiesFile::Options APIKeyManager::getPropertyOptions()
{
    juce::PropertiesFile::Options options;

    options.applicationName = "Intuition";
    options.filenameSuffix = ".settings";
    options.osxLibrarySubFolder = "Application Support";
    options.folderName = juce::File::getSpecialLocation(
        juce::File::userApplicationDataDirectory
    ).getChildFile("Intuition").getFullPathName();

    options.storageFormat = juce::PropertiesFile::storeAsXML;
    options.millisecondsBeforeSaving = 1000;

    return options;
}

#pragma once

#include <JuceHeader.h>

/**
 * @brief Manages secure storage and retrieval of API keys
 *
 * Uses JUCE's PropertiesFile for persistent, encrypted storage
 * Keys are stored in the user's application data directory
 */
class APIKeyManager
{
public:
    APIKeyManager();
    ~APIKeyManager();

    /**
     * @brief Save API key to secure storage
     */
    void saveAPIKey(const juce::String& key);

    /**
     * @brief Retrieve saved API key
     *
     * @return API key, or empty string if not saved
     */
    juce::String getAPIKey() const;

    /**
     * @brief Check if an API key is saved
     */
    bool hasAPIKey() const;

    /**
     * @brief Clear saved API key
     */
    void clearAPIKey();

    /**
     * @brief Save API endpoint preference
     */
    void saveAPIEndpoint(const juce::String& endpoint);

    /**
     * @brief Get saved API endpoint
     */
    juce::String getAPIEndpoint() const;

    /**
     * @brief Save model preference
     */
    void saveModel(const juce::String& model);

    /**
     * @brief Get saved model
     */
    juce::String getModel() const;

private:
    std::unique_ptr<juce::PropertiesFile> properties;

    juce::PropertiesFile::Options getPropertyOptions();
};

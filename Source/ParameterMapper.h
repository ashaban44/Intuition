#pragma once

#include <JuceHeader.h>
#include <map>
#include <vector>

/**
 * @brief Information about a VST parameter for AI understanding
 */
struct ParameterInfo
{
    juce::String paramID;           // JUCE parameter ID
    juce::String displayName;       // Human-readable name
    juce::String description;       // What this parameter does
    float minValue;                 // Minimum value
    float maxValue;                 // Maximum value
    float defaultValue;             // Default value
    juce::String unit;              // Unit (Hz, dB, %, etc.)
    juce::StringArray aliases;      // Alternative names for AI understanding

    ParameterInfo() = default;
    ParameterInfo(const juce::String& id, const juce::String& name,
                 const juce::String& desc, float min, float max,
                 float defaultVal, const juce::String& unitStr = "",
                 const juce::StringArray& alt = {})
        : paramID(id), displayName(name), description(desc),
          minValue(min), maxValue(max), defaultValue(defaultVal),
          unit(unitStr), aliases(alt) {}
};

/**
 * @brief Maps VST parameters to information AI can understand
 *
 * This class:
 * - Maintains a registry of all controllable parameters
 * - Generates JSON descriptions for the AI
 * - Validates parameter changes
 * - Provides constraints and metadata
 */
class ParameterMapper
{
public:
    ParameterMapper();

    /**
     * @brief Register all VST parameters
     */
    void initialize();

    /**
     * @brief Get JSON representation of all parameters for AI prompt
     *
     * This creates a structured description the AI can understand
     *
     * @return JSON string describing available parameters
     */
    juce::String getParametersAsJSON() const;

    /**
     * @brief Get a simplified parameter list for AI context
     *
     * @return Human-readable parameter description
     */
    juce::String getParameterDescription() const;

    /**
     * @brief Validate a parameter change
     *
     * @param paramID Parameter ID
     * @param value Proposed value
     * @return Clamped valid value
     */
    float validateParameterValue(const juce::String& paramID, float value) const;

    /**
     * @brief Get parameter info by ID
     *
     * @param paramID Parameter ID
     * @return Pointer to parameter info, or nullptr if not found
     */
    const ParameterInfo* getParameterInfo(const juce::String& paramID) const;

    /**
     * @brief Find parameter by natural language alias
     *
     * @param alias Name or alias (e.g., "brightness", "filter cutoff")
     * @return Parameter ID, or empty string if not found
     */
    juce::String findParameterByAlias(const juce::String& alias) const;

    /**
     * @brief Get all registered parameters
     */
    const std::map<juce::String, ParameterInfo>& getAllParameters() const
    {
        return parameters;
    }

private:
    std::map<juce::String, ParameterInfo> parameters;

    void registerOscillatorParameters();
    void registerFilterParameters();
    void registerEnvelopeParameters();
    void registerLFOParameters();
    void registerEffectParameters();
    void registerMasterParameters();
};

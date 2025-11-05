#include "ParameterMapper.h"

ParameterMapper::ParameterMapper()
{
    initialize();
}

void ParameterMapper::initialize()
{
    parameters.clear();

    registerMasterParameters();
    registerOscillatorParameters();
    registerFilterParameters();
    registerEnvelopeParameters();
    registerLFOParameters();
    registerEffectParameters();
}

void ParameterMapper::registerMasterParameters()
{
    parameters.emplace("masterVol", ParameterInfo(
        "masterVol", "Master Volume", "Overall output volume",
        0.0f, 1.0f, 0.7f, "",
        {"volume", "level", "gain", "loudness"}
    ));
}

void ParameterMapper::registerOscillatorParameters()
{
    const char* oscNames[] = {"A", "B", "C", "D"};

    for (int i = 0; i < 4; ++i)
    {
        juce::String osc = oscNames[i];
        juce::String prefix = "osc" + osc;

        // Toggle
        parameters.emplace(prefix + "Toggle", ParameterInfo(
            prefix + "Toggle", "Oscillator " + osc + " On/Off",
            "Enable or disable oscillator " + osc,
            0.0f, 1.0f, 1.0f, "",
            {"osc" + osc, "oscillator" + osc}
        ));

        // Volume
        parameters.emplace(prefix + "Vol", ParameterInfo(
            prefix + "Vol", "Oscillator " + osc + " Volume",
            "Volume level for oscillator " + osc,
            0.0f, 1.0f, 0.5f, "",
            {"osc" + osc + " volume", "osc" + osc + " level"}
        ));

        // Unison
        parameters.emplace(prefix + "Unison", ParameterInfo(
            prefix + "Unison", "Oscillator " + osc + " Unison Voices",
            "Number of unison voices (1-8)",
            1.0f, 8.0f, 1.0f, "voices",
            {"osc" + osc + " unison", "osc" + osc + " voices"}
        ));

        // Detune
        parameters.emplace(prefix + "Detune", ParameterInfo(
            prefix + "Detune", "Oscillator " + osc + " Detune",
            "Unison detune amount in cents",
            0.0f, 100.0f, 10.0f, "cents",
            {"osc" + osc + " detune", "osc" + osc + " spread"}
        ));

        // Morph
        parameters.emplace(prefix + "Morph", ParameterInfo(
            prefix + "Morph", "Oscillator " + osc + " Wavetable Morph",
            "Position in wavetable (0 = table 1, 1 = table 8)",
            0.0f, 1.0f, 0.0f, "",
            {"osc" + osc + " morph", "osc" + osc + " wavetable", "osc" + osc + " timbre"}
        ));

        // Octave
        parameters.emplace(prefix + "Octave", ParameterInfo(
            prefix + "Octave", "Oscillator " + osc + " Octave",
            "Pitch in octaves (-4 to +4)",
            -4.0f, 4.0f, 0.0f, "octaves",
            {"osc" + osc + " octave", "osc" + osc + " pitch"}
        ));

        // Coarse
        parameters.emplace(prefix + "Coarse", ParameterInfo(
            prefix + "Coarse", "Oscillator " + osc + " Coarse Tune",
            "Pitch in semitones (-12 to +12)",
            -12.0f, 12.0f, 0.0f, "semitones",
            {"osc" + osc + " coarse", "osc" + osc + " semitones"}
        ));

        // Fine
        parameters.emplace(prefix + "Fine", ParameterInfo(
            prefix + "Fine", "Oscillator " + osc + " Fine Tune",
            "Fine pitch adjustment in cents",
            -100.0f, 100.0f, 0.0f, "cents",
            {"osc" + osc + " fine", "osc" + osc + " cents"}
        ));
    }
}

void ParameterMapper::registerFilterParameters()
{
    // Filter Send for each oscillator
    const char* oscNames[] = {"A", "B", "C", "D"};
    for (int i = 0; i < 4; ++i)
    {
        juce::String osc = oscNames[i];
        parameters.emplace("filterSend" + osc, ParameterInfo(
            "filterSend" + osc, "Filter Send " + osc,
            "Send oscillator " + osc + " to filter",
            0.0f, 1.0f, 1.0f, "",
            {"filter osc" + osc, osc + " filter"}
        ));
    }

    // Filter Cutoff
    parameters.emplace("filterCutoff", ParameterInfo(
        "filterCutoff", "Filter Cutoff",
        "Filter cutoff frequency",
        20.0f, 20000.0f, 1000.0f, "Hz",
        {"cutoff", "frequency", "brightness", "tone"}
    ));

    // Filter Resonance
    parameters.emplace("filterRes", ParameterInfo(
        "filterRes", "Filter Resonance",
        "Filter resonance/Q amount",
        0.01f, 1.0f, 0.1f, "",
        {"resonance", "Q", "emphasis"}
    ));

    // Filter Type
    parameters.emplace("filterType", ParameterInfo(
        "filterType", "Filter Type",
        "Filter type (0=Low, 1=High, 2=Band)",
        0.0f, 2.0f, 0.0f, "",
        {"filter type", "filter mode"}
    ));
}

void ParameterMapper::registerEnvelopeParameters()
{
    const char* envNames[] = {"ENV_OSC", "ENV1", "ENV2", "ENV3"};
    const char* displayNames[] = {"Amplitude Envelope", "Envelope 1", "Envelope 2", "Envelope 3"};

    for (int i = 0; i < 4; ++i)
    {
        juce::String env = envNames[i];
        juce::String display = displayNames[i];

        // Attack
        parameters.emplace(env + "_attack", ParameterInfo(
            env + "_attack", display + " Attack",
            "Attack time",
            0.0f, 1.0f, 0.1f, "",
            {env + " attack", display + " attack"}
        ));

        // Decay
        parameters.emplace(env + "_decay", ParameterInfo(
            env + "_decay", display + " Decay",
            "Decay time",
            0.0f, 1.0f, 0.3f, "",
            {env + " decay", display + " decay"}
        ));

        // Sustain
        parameters.emplace(env + "_sustain", ParameterInfo(
            env + "_sustain", display + " Sustain",
            "Sustain level",
            0.0f, 1.0f, 0.7f, "",
            {env + " sustain", display + " sustain"}
        ));

        // Release
        parameters.emplace(env + "_release", ParameterInfo(
            env + "_release", display + " Release",
            "Release time",
            0.0f, 1.0f, 0.4f, "",
            {env + " release", display + " release"}
        ));
    }
}

void ParameterMapper::registerLFOParameters()
{
    for (int i = 1; i <= 3; ++i)
    {
        juce::String lfo = "LFO" + juce::String(i);

        // Mode (Free/Synced)
        parameters.emplace(lfo + "_mode", ParameterInfo(
            lfo + "_mode", lfo + " Mode",
            "LFO timing mode (0=Free, 1=Synced)",
            0.0f, 1.0f, 0.0f, "",
            {lfo + " mode", lfo + " sync"}
        ));

        // Sync Division
        parameters.emplace(lfo + "_syncDiv", ParameterInfo(
            lfo + "_syncDiv", lfo + " Sync Division",
            "Tempo sync division",
            0.0f, 7.0f, 0.0f, "",
            {lfo + " division", lfo + " tempo"}
        ));

        // Rate
        parameters.emplace(lfo + "_rate", ParameterInfo(
            lfo + "_rate", lfo + " Rate",
            "LFO frequency in Hz (free mode)",
            0.01f, 30.0f, 1.0f, "Hz",
            {lfo + " rate", lfo + " speed", lfo + " frequency"}
        ));
    }
}

void ParameterMapper::registerEffectParameters()
{
    // Reverb
    parameters.emplace("reverbToggle", ParameterInfo(
        "reverbToggle", "Reverb On/Off",
        "Enable or disable reverb",
        0.0f, 1.0f, 0.0f, "",
        {"reverb", "reverb on"}
    ));

    parameters.emplace("reverbDamp", ParameterInfo(
        "reverbDamp", "Reverb Damping",
        "High frequency damping",
        0.0f, 1.0f, 0.5f, "",
        {"reverb damping", "reverb darkness"}
    ));

    parameters.emplace("reverbSize", ParameterInfo(
        "reverbSize", "Reverb Room Size",
        "Virtual room size",
        0.0f, 1.0f, 0.5f, "",
        {"reverb size", "room size", "reverb space"}
    ));

    parameters.emplace("reverbWidth", ParameterInfo(
        "reverbWidth", "Reverb Width",
        "Stereo width of reverb",
        0.0f, 1.0f, 1.0f, "",
        {"reverb width", "reverb stereo"}
    ));

    parameters.emplace("reverbDry", ParameterInfo(
        "reverbDry", "Reverb Dry Level",
        "Dry (unprocessed) signal level",
        0.0f, 1.0f, 0.8f, "",
        {"reverb dry", "dry level"}
    ));

    parameters.emplace("reverbWet", ParameterInfo(
        "reverbWet", "Reverb Wet Level",
        "Wet (processed) signal level",
        0.0f, 1.0f, 0.3f, "",
        {"reverb wet", "wet level", "reverb amount"}
    ));
}

juce::String ParameterMapper::getParametersAsJSON() const
{
    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    juce::Array<juce::var> paramArray;

    for (const auto& pair : parameters)
    {
        const auto& info = pair.second;
        juce::DynamicObject::Ptr paramObj = new juce::DynamicObject();

        paramObj->setProperty("id", info.paramID);
        paramObj->setProperty("name", info.displayName);
        paramObj->setProperty("description", info.description);
        paramObj->setProperty("min", info.minValue);
        paramObj->setProperty("max", info.maxValue);
        paramObj->setProperty("default", info.defaultValue);
        paramObj->setProperty("unit", info.unit);

        juce::Array<juce::var> aliasArray;
        for (const auto& alias : info.aliases)
            aliasArray.add(alias);
        paramObj->setProperty("aliases", aliasArray);

        paramArray.add(var(paramObj.get()));
    }

    root->setProperty("parameters", paramArray);

    return juce::JSON::toString(var(root.get()), true);
}

juce::String ParameterMapper::getParameterDescription() const
{
    juce::String desc = "Available synthesizer parameters:\n\n";

    desc += "MASTER:\n- Master Volume (0-1)\n\n";

    desc += "OSCILLATORS (A, B, C, D):\n";
    desc += "- Toggle (on/off), Volume (0-1), Unison (1-8 voices)\n";
    desc += "- Detune (0-100 cents), Morph (0-1 wavetable position)\n";
    desc += "- Octave (-4 to +4), Coarse (-12 to +12 semitones), Fine (-100 to +100 cents)\n\n";

    desc += "FILTER:\n";
    desc += "- Cutoff (20-20000 Hz), Resonance (0.01-1.0)\n";
    desc += "- Type (0=Low, 1=High, 2=Band), Send per oscillator\n\n";

    desc += "ENVELOPES (Amplitude, ENV1, ENV2, ENV3):\n";
    desc += "- Attack, Decay, Sustain, Release (0-1)\n\n";

    desc += "LFOs (1, 2, 3):\n";
    desc += "- Mode (0=Free, 1=Synced), Rate (0.01-30 Hz), Sync Division\n\n";

    desc += "REVERB:\n";
    desc += "- Toggle, Damping, Room Size, Width, Dry/Wet levels (0-1)\n";

    return desc;
}

float ParameterMapper::validateParameterValue(const juce::String& paramID, float value) const
{
    auto it = parameters.find(paramID);
    if (it == parameters.end())
        return value;

    const auto& info = it->second;
    return juce::jlimit(info.minValue, info.maxValue, value);
}

const ParameterInfo* ParameterMapper::getParameterInfo(const juce::String& paramID) const
{
    auto it = parameters.find(paramID);
    return (it != parameters.end()) ? &(it->second) : nullptr;
}

juce::String ParameterMapper::findParameterByAlias(const juce::String& alias) const
{
    juce::String lowerAlias = alias.toLowerCase().trim();

    // First try exact match on parameter ID
    if (parameters.find(alias) != parameters.end())
        return alias;

    // Then search aliases
    for (const auto& pair : parameters)
    {
        const auto& info = pair.second;

        // Check display name
        if (info.displayName.toLowerCase() == lowerAlias)
            return info.paramID;

        // Check aliases
        for (const auto& alt : info.aliases)
        {
            if (alt.toLowerCase() == lowerAlias)
                return info.paramID;
        }
    }

    return {};
}

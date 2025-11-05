# Intuition VST - AI Parameter Control

## Overview

This feature adds AI-powered parameter control to the Intuition wavetable synthesizer. Users can describe the sound they want in natural language, and the AI will intelligently adjust VST parameters to achieve that sound.

## Features

- **Natural Language Control**: Type descriptions like "make it brighter", "add more bass", or "increase attack by 0.2"
- **Multiple AI Providers**: Support for OpenAI (ChatGPT) and Grok (xAI)
- **Secure API Key Storage**: Keys are saved securely in your application data folder
- **Rate Limiting**: Built-in protection (30 requests/minute, 1000 requests/day)
- **Clean Architecture**: UI handles AI, not the audio processor - maintains clean separation
- **Non-blocking**: All AI operations are asynchronous - won't freeze your UI

## Setup

### 1. Get an API Key

**Option A: OpenAI (ChatGPT)**
- Go to https://platform.openai.com/api-keys
- Create a new API key
- Copy the key (starts with `sk-`)

**Option B: Grok (xAI)**
- Go to https://x.ai
- Create an API key
- Copy the key (starts with `xai-`)

### 2. Configure the Plugin

1. Open Intuition in your DAW
2. Navigate to the **AI** tab
3. Paste your API key in the "API Key" field
4. Click **Save**
5. Select your service (OpenAI or Grok)
6. Choose a model:
   - **GPT-4o Mini**: Fastest and most cost-effective (recommended)
   - **GPT-4o**: Balanced performance
   - **GPT-4 Turbo**: Most capable
   - **Grok Beta**: Alternative provider

## Usage

### Basic Examples

1. **Brightness**: "make it brighter"
   - AI increases filter cutoff
   - Selects brighter wavetable positions

2. **Bass**: "add more bass"
   - AI adjusts filter to emphasize low frequencies
   - May lower oscillator octaves

3. **Attack**: "increase attack by 0.2"
   - AI adds 0.2 to current envelope attack value

4. **Complex Changes**: "warm pad sound with slow attack and reverb"
   - AI adjusts multiple parameters intelligently

### How It Works

1. Type your description in the prompt box
2. Click **Apply AI Changes**
3. Wait for the AI to process (usually 1-3 seconds)
4. Review the proposed parameter changes
5. Click **Apply** to accept or **Cancel** to reject

## Architecture

### Design Principles

This implementation follows clean software architecture:

```
┌─────────────────────────────────────┐
│     PluginEditor (UI Layer)         │
│                                     │
│    AIControlPanel (new tab)         │
└─────────────────────────────────────┘
                 ↓
┌─────────────────────────────────────┐
│   AIParameterController Interface   │
│   (Abstraction - easily swappable)  │
└─────────────────────────────────────┘
                 ↓
┌─────────────────────────────────────┐
│  ChatGPTParameterController         │
│  - HTTP communication               │
│  - Rate limiting                    │
│  - Error handling                   │
└─────────────────────────────────────┘
                 ↓
┌─────────────────────────────────────┐
│      ParameterMapper                │
│  - Maps 100+ VST parameters         │
│  - Validates changes                │
└─────────────────────────────────────┘
```

### Key Components

#### AIParameterController (Interface)
- Abstract interface for AI operations
- Allows swapping AI providers without changing UI code
- Clean dependency inversion

#### ChatGPTParameterController
- Concrete implementation for OpenAI/Grok
- Handles HTTP requests, rate limiting, error handling
- No nested if-chains - clean, readable code
- Thread-safe for JUCE message thread

#### ParameterMapper
- Maintains registry of all VST parameters
- Generates JSON descriptions for AI
- Validates and clamps parameter values
- Maps natural language aliases to parameter IDs

#### APIKeyManager
- Secure persistent storage using JUCE PropertiesFile
- Cross-platform (Windows/Mac)
- Saves to user's application data directory

#### AIControlPanel (UI)
- JUCE component integrated into main tabs
- API key management
- Prompt input
- Status display
- Confirmation dialogs

## Supported Parameters

The AI understands all Intuition parameters:

### Master
- Master Volume

### Oscillators (A, B, C, D)
- Toggle, Volume, Unison (1-8 voices)
- Detune (0-100 cents)
- Morph (wavetable position 0-1)
- Octave (-4 to +4)
- Coarse tuning (-12 to +12 semitones)
- Fine tuning (-100 to +100 cents)

### Filter
- Cutoff (20-20000 Hz)
- Resonance (0.01-1.0)
- Type (Low/High/Band pass)
- Send per oscillator

### Envelopes (Amplitude, ENV1, ENV2, ENV3)
- Attack, Decay, Sustain, Release (0-1)

### LFOs (1, 2, 3)
- Mode (Free/Synced)
- Rate (0.01-30 Hz)
- Sync Division

### Reverb
- Toggle, Damping, Room Size, Width
- Dry/Wet levels

## Rate Limits

### Default Limits (OpenAI/Grok)
- **30 requests per minute**
- **1000 requests per day**
- **200K tokens per day**

The plugin automatically tracks and enforces these limits.

## Troubleshooting

### "Invalid API Key"
- Ensure key starts with `sk-` (OpenAI) or `xai-` (Grok)
- Check for extra spaces or characters
- Generate a new key if needed

### "Rate Limit Exceeded"
- Wait 1 minute before trying again
- Check rate limit display in UI
- Consider upgrading your API plan

### "Network Error"
- Check internet connection
- Verify firewall isn't blocking HTTPS
- Try again in a few seconds

### "Empty Response"
- Service may be temporarily down
- Try a different model
- Check API service status

## Cost Estimation

### OpenAI Pricing (as of 2025)
- **GPT-4o Mini**: ~$0.0001 per request (cheapest)
- **GPT-4o**: ~$0.002 per request
- **GPT-4 Turbo**: ~$0.01 per request

Typical session (20 adjustments): **$0.002 - $0.20**

### Tips to Minimize Cost
1. Use GPT-4o Mini (recommended)
2. Be specific in prompts to avoid re-tries
3. Batch multiple changes in one prompt
4. Set up billing alerts on your API account

## For Developers

### Adding New AI Providers

The clean interface design makes it easy to add new providers:

```cpp
class MyAIProvider : public AIParameterController
{
public:
    void processPrompt(const juce::String& prompt,
                      AIControlCallback callback) override
    {
        // Your implementation
    }

    // Implement other interface methods...
};
```

### Parameter Registration

Add new parameters in `ParameterMapper.cpp`:

```cpp
void ParameterMapper::registerMyParameters()
{
    parameters.emplace("myParam", ParameterInfo(
        "myParam",                  // ID
        "My Parameter",             // Display name
        "What it does",            // Description
        0.0f,                      // Min
        1.0f,                      // Max
        0.5f,                      // Default
        "unit",                    // Unit
        {"alias1", "alias2"}       // Aliases
    ));
}
```

### Testing

The architecture is designed for testability:
- Mock `AIParameterController` for UI tests
- Mock HTTP responses for API tests
- Validate parameter mapping independently

## Security

- API keys stored using JUCE's PropertiesFile
- Keys never logged or displayed in plain text
- HTTPS for all API communication
- No telemetry or data collection

## Future Enhancements

Potential improvements:
- Local AI model support (no internet required)
- Preset suggestions based on descriptions
- Voice input support
- AI-generated presets
- Learning user preferences
- Undo/redo for AI changes

## License

This feature is part of the Intuition VST project.
Copyright (c) 2025 Brazill & Shaban

## Support

For issues or questions:
- GitHub Issues: [Your repo URL]
- Email: [Your email]

---

Built with clean architecture principles.
UI controls AI, not the audio processor.
No nested if-chains.
Production-ready code.

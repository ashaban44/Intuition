# Intuition
Intuition is a VST/AU/standalone digital synthesizer designed to combine high-performance digital signal processing with an intuitive, modular workflow for sound design and music production. 

## Overview
The project aims to provide musicians and producers with a flexible environment for creating and manipulating sound in real time, without sacrificing usability or creative flow. Unlike many existing synthesizers that trade performance for complexity or simplicity for control, Intuition seeks to bridge that gap. It offers a robust synthesis engine built in C++ using JUCE, capable of handling multiple oscillators, modulation sources, and signal routing options efficiently. The system architecture emphasizes modularity, allowing features such as wavetable synthesis, unison detuning, and morphing to be expanded and customized over time. Ultimately, Intuition’s goal is to empower users to explore advanced sound design techniques through a responsive, user-friendly interface that feels creative rather than technical.

## Features
### 4 Powerful Oscillator Modules
### Wavetable Morphing
### Advanced Modulation Routing

## AI Performance Assistant
Intuition now ships with an optional AI-powered performance assistant that can understand natural language prompts and translate them into parameter adjustments inside the plugin UI. The assistant lives in the editor window and communicates with the existing controls, so the audio processor itself remains focused on sound generation.

### Recommended AI Provider
For the smoothest experience we recommend using the [OpenAI Chat Completions](https://platform.openai.com/docs/api-reference/chat/create) API with the `gpt-4o-mini` model. It offers a great balance between low latency and high quality parameter suggestions, and the project ships with prompts tuned for that model family.

### Configuring an AI Provider
The assistant can connect to any OpenAI-compatible chat completion endpoint. To supply your own credentials:

1. Launch the Intuition plugin or standalone app and open the **AI Performance Assistant** panel.
2. Paste your API key into the **API Key** field (the value is stored locally in your user settings folder in plain text). Optionally customise the **Endpoint** and **Model** fields if you are targeting a non-default provider.
   * macOS: `~/Library/Application Support/Intuition/Intuition.settings`
   * Windows: `%APPDATA%\Intuition\Intuition.settings`
3. Click **Connect** to save the credentials and activate cloud-backed prompting.

You can also configure the integration via environment variables before launching the plugin or standalone app:

| Variable | Purpose |
| --- | --- |
| `INTUITION_AI_API_KEY` | API key used to authenticate with the AI provider. |
| `INTUITION_AI_ENDPOINT` (optional) | Override the default endpoint (`https://api.openai.com/v1/chat/completions`). |
| `INTUITION_AI_MODEL` (optional) | Model identifier to request (defaults to `gpt-4o-mini`). |

When no API key is supplied the editor falls back to an offline, rule-based interpreter so the workflow continues to function during development.

### Using the Assistant
1. Open the **AI Performance Assistant** panel at the bottom of the editor.
2. Describe the change you want using natural language (e.g. “increase oscillator A volume by 10%” or “set the filter cutoff to 1500 Hz”).
3. Press **Send Prompt** or hit <kbd>Enter</kbd>.
4. The assistant analyses the request, applies parameter updates through the GUI, and reports the outcome in the status panel.

All adjustments are routed through the existing JUCE parameter system, keeping automation and host notifications intact on macOS and Windows builds.

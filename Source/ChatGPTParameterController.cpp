#include "ChatGPTParameterController.h"

ChatGPTParameterController::ChatGPTParameterController()
{
    paramMapper = std::make_unique<ParameterMapper>();
    setStatus("Ready - Please set API key");
}

ChatGPTParameterController::~ChatGPTParameterController()
{
    cancelCurrentOperation();
}

void ChatGPTParameterController::processPrompt(const juce::String& prompt,
                                              AIControlCallback callback)
{
    // Early validation
    if (!isConfigured())
    {
        AIControlResult result;
        result.success = false;
        result.message = "API key not set";
        result.errorDetails = "Please configure your OpenAI API key first";
        callback(result);
        return;
    }

    if (prompt.trim().isEmpty())
    {
        AIControlResult result;
        result.success = false;
        result.message = "Empty prompt";
        result.errorDetails = "Please enter a description of the sound you want";
        callback(result);
        return;
    }

    if (isProcessing.load())
    {
        AIControlResult result;
        result.success = false;
        result.message = "Already processing";
        result.errorDetails = "Please wait for the current request to complete";
        callback(result);
        return;
    }

    // Check rate limits
    if (!rateLimiter.canMakeRequest())
    {
        AIControlResult result;
        result.success = false;
        result.message = "Rate limit exceeded";
        result.errorDetails = "Please wait before making another request. " +
                             rateLimiter.getUsageInfo();
        callback(result);
        return;
    }

    // Build request
    isProcessing = true;
    setStatus("Processing prompt...");

    juce::String systemPrompt = buildPrompt(prompt);
    juce::String requestBody = buildRequestJSON(systemPrompt, prompt);

    // Prepare HTTP request
    juce::URL url(apiEndpoint);

    juce::String extraHeaders = "Content-Type: application/json\r\n";
    extraHeaders += "Authorization: Bearer " + apiKey + "\r\n";

    // Make async request
    currentRequest = url.downloadToStream(
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
            .withExtraHeaders(extraHeaders)
            .withHttpRequestCmd("POST")
            .withConnectionTimeoutMs(30000)
            .withDataToSend(requestBody.toStdString())
            .withProgressCallback([](int64, int64) { return true; })
            .withStatusCode(nullptr),
        [this, callback](std::unique_ptr<juce::InputStream> stream, bool success)
        {
            handleHTTPResponse(std::move(stream), success, callback);
        }
    );

    rateLimiter.recordRequest();
}

bool ChatGPTParameterController::setAPIKey(const juce::String& newKey)
{
    juce::String trimmedKey = newKey.trim();

    // Basic validation - OpenAI keys start with "sk-"
    if (trimmedKey.isEmpty())
    {
        apiKey.clear();
        setStatus("Ready - Please set API key");
        return false;
    }

    // Accept keys that look like OpenAI format (for Grok compatibility too)
    if (!trimmedKey.startsWith("sk-") && !trimmedKey.startsWith("xai-"))
    {
        setStatus("Invalid API key format");
        return false;
    }

    apiKey = trimmedKey;
    setStatus("Ready");
    return true;
}

bool ChatGPTParameterController::isConfigured() const
{
    return !apiKey.isEmpty();
}

juce::String ChatGPTParameterController::getStatus() const
{
    return currentStatus;
}

void ChatGPTParameterController::cancelCurrentOperation()
{
    if (currentRequest != nullptr)
    {
        currentRequest.reset();
        isProcessing = false;
        setStatus("Operation cancelled");
    }
}

juce::String ChatGPTParameterController::getRateLimitInfo() const
{
    return rateLimiter.getUsageInfo();
}

void ChatGPTParameterController::setAPIEndpoint(const juce::String& endpoint)
{
    apiEndpoint = endpoint;
}

void ChatGPTParameterController::setModel(const juce::String& modelName)
{
    model = modelName;
}

juce::String ChatGPTParameterController::buildPrompt(const juce::String& userPrompt) const
{
    juce::String systemPrompt = R"(You are an AI assistant helping control a wavetable synthesizer VST plugin.

The user will describe the sound they want, and you must respond with ONLY a JSON object containing parameter changes.

)";

    systemPrompt += paramMapper->getParameterDescription();

    systemPrompt += R"(

IMPORTANT RULES:
1. Respond ONLY with valid JSON - no explanations, no markdown, no code blocks
2. Use this exact format:
{
  "parameters": [
    {"id": "parameterID", "value": numberValue, "reason": "why this change"}
  ],
  "summary": "Brief description of changes"
}

3. When user says "increase by X" or "add X", calculate the new value relative to typical defaults
4. For example, if offset is 0.5 and user says "make it 1", add 0.5 to get the target
5. All values must be within parameter ranges (see above)
6. Only include parameters that need to change
7. Be musical and intelligent about parameter relationships

Example user request: "make it brighter"
Example response:
{
  "parameters": [
    {"id": "filterCutoff", "value": 5000, "reason": "Increase brightness"},
    {"id": "oscAMorph", "value": 0.6, "reason": "Use brighter wavetable"}
  ],
  "summary": "Increased filter cutoff and selected brighter wavetable"
}
)";

    return systemPrompt;
}

juce::String ChatGPTParameterController::buildRequestJSON(
    const juce::String& systemPrompt,
    const juce::String& userPrompt) const
{
    juce::DynamicObject::Ptr root = new juce::DynamicObject();

    root->setProperty("model", model);
    root->setProperty("temperature", 0.7);
    root->setProperty("max_tokens", 500);

    juce::Array<juce::var> messages;

    // System message
    juce::DynamicObject::Ptr sysMsg = new juce::DynamicObject();
    sysMsg->setProperty("role", "system");
    sysMsg->setProperty("content", systemPrompt);
    messages.add(var(sysMsg.get()));

    // User message
    juce::DynamicObject::Ptr userMsg = new juce::DynamicObject();
    userMsg->setProperty("role", "user");
    userMsg->setProperty("content", userPrompt);
    messages.add(var(userMsg.get()));

    root->setProperty("messages", messages);

    return juce::JSON::toString(var(root.get()));
}

void ChatGPTParameterController::handleHTTPResponse(
    std::unique_ptr<juce::InputStream> stream,
    bool success,
    AIControlCallback callback)
{
    isProcessing = false;

    if (!success || stream == nullptr)
    {
        setStatus("Request failed");
        AIControlResult result;
        result.success = false;
        result.message = "Network error";
        result.errorDetails = "Failed to connect to API. Check your internet connection.";
        callback(result);
        return;
    }

    juce::String response = stream->readEntireStreamAsString();

    if (response.isEmpty())
    {
        setStatus("Empty response");
        AIControlResult result;
        result.success = false;
        result.message = "Empty response";
        result.errorDetails = "API returned no data";
        callback(result);
        return;
    }

    // Parse and process response
    AIControlResult result = parseResponse(response);

    if (result.success)
        setStatus("Ready");
    else
        setStatus("Error: " + result.message);

    callback(result);
}

AIControlResult ChatGPTParameterController::parseResponse(const juce::String& responseJSON)
{
    AIControlResult result;

    var jsonData = juce::JSON::parse(responseJSON);

    if (!jsonData.isObject())
    {
        result.success = false;
        result.message = "Invalid JSON response";
        result.errorDetails = responseJSON.substring(0, 200);
        return result;
    }

    juce::DynamicObject* root = jsonData.getDynamicObject();

    // Check for API errors
    if (root->hasProperty("error"))
    {
        result.success = false;
        auto errorObj = root->getProperty("error");

        if (errorObj.isObject())
        {
            auto* errData = errorObj.getDynamicObject();
            result.message = errData->getProperty("message").toString();
            result.errorDetails = errData->getProperty("type").toString();
        }
        else
        {
            result.message = "API error";
            result.errorDetails = errorObj.toString();
        }

        return result;
    }

    // Extract AI response content
    if (!root->hasProperty("choices"))
    {
        result.success = false;
        result.message = "No choices in response";
        result.errorDetails = responseJSON.substring(0, 200);
        return result;
    }

    juce::Array<var>* choices = root->getProperty("choices").getArray();

    if (choices == nullptr || choices->isEmpty())
    {
        result.success = false;
        result.message = "Empty choices array";
        return result;
    }

    auto* choice = (*choices)[0].getDynamicObject();
    auto* message = choice->getProperty("message").getDynamicObject();
    juce::String content = message->getProperty("content").toString().trim();

    // Parse the parameter changes from AI response
    var aiResponse = juce::JSON::parse(content);

    if (!aiResponse.isObject())
    {
        result.success = false;
        result.message = "AI response not valid JSON";
        result.errorDetails = content.substring(0, 200);
        return result;
    }

    auto* aiData = aiResponse.getDynamicObject();

    // Extract summary
    result.message = aiData->getProperty("summary").toString();

    // Extract parameters
    if (!aiData->hasProperty("parameters"))
    {
        result.success = false;
        result.errorDetails = "No parameters in AI response";
        return result;
    }

    juce::Array<var>* params = aiData->getProperty("parameters").getArray();

    if (params == nullptr)
    {
        result.success = false;
        result.errorDetails = "Parameters not an array";
        return result;
    }

    // Process each parameter change
    for (const auto& paramVar : *params)
    {
        if (!paramVar.isObject())
            continue;

        auto* paramObj = paramVar.getDynamicObject();

        juce::String paramID = paramObj->getProperty("id").toString();
        float value = static_cast<float>(paramObj->getProperty("value"));

        // Validate and clamp value
        float validValue = paramMapper->validateParameterValue(paramID, value);

        result.parameterChanges.push_back({paramID, validValue});
    }

    result.success = !result.parameterChanges.empty();

    if (!result.success)
        result.errorDetails = "No valid parameter changes found";

    return result;
}

void ChatGPTParameterController::setStatus(const juce::String& newStatus)
{
    currentStatus = newStatus;
}

// RateLimiter implementation
bool ChatGPTParameterController::RateLimiter::canMakeRequest()
{
    auto now = juce::Time::getCurrentTime();

    // Remove timestamps older than 1 minute
    while (!requestTimestamps.empty())
    {
        auto age = now - requestTimestamps.front();
        if (age.inSeconds() < 60)
            break;

        requestTimestamps.pop_front();
    }

    // Check minute limit
    if (requestTimestamps.size() >= MAX_REQUESTS_PER_MINUTE)
        return false;

    // Remove timestamps older than 1 day
    auto dayAgo = now - juce::RelativeTime::days(1);
    while (!requestTimestamps.empty() && requestTimestamps.front() < dayAgo)
        requestTimestamps.pop_front();

    // Check daily limit
    return requestTimestamps.size() < MAX_REQUESTS_PER_DAY;
}

void ChatGPTParameterController::RateLimiter::recordRequest()
{
    requestTimestamps.push_back(juce::Time::getCurrentTime());
}

juce::String ChatGPTParameterController::RateLimiter::getUsageInfo() const
{
    auto now = juce::Time::getCurrentTime();

    // Count requests in last minute
    int lastMinute = 0;
    for (const auto& timestamp : requestTimestamps)
    {
        if ((now - timestamp).inSeconds() < 60)
            ++lastMinute;
    }

    // Count requests in last day
    int lastDay = static_cast<int>(requestTimestamps.size());

    return juce::String::formatted(
        "Usage: %d/%d per minute, %d/%d per day",
        lastMinute, MAX_REQUESTS_PER_MINUTE,
        lastDay, MAX_REQUESTS_PER_DAY
    );
}

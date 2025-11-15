#include "ApiModule.h"

ApiModule::ApiModule() 
  : initialized(false), lastRequestTime(0), consecutiveFailures(0),
    totalRequests(0), successfulRequests(0), failedRequests(0), totalResponseTime(0) {
  
  // Default retry configuration
  retryConfig.maxRetries = API_RETRY_ATTEMPTS;
  retryConfig.retryDelay = 1000;  // 1 second
  retryConfig.exponentialBackoff = true;
}

bool ApiModule::initialize(const String& url, const String& key, const String& devId) {
  if (key.length() == 0 || devId.length() == 0 || url.length() == 0) {
    LOG_ERROR("API initialization failed: Invalid configuration");
    return false;
  }
  
  baseUrl = url;
  apiKey = key;
  deviceId = devId;
  initialized = true;
  
  LOG_INFO("API Module initialized");
  LOG_INFO("Base URL: " + baseUrl);
  LOG_INFO("Device ID: " + deviceId);
  
  // Reset statistics
  resetStatistics();
  
  return true;
}

void ApiModule::setRetryConfig(int maxRetries, unsigned long retryDelay, bool exponentialBackoff) {
  retryConfig.maxRetries = maxRetries;
  retryConfig.retryDelay = retryDelay;
  retryConfig.exponentialBackoff = exponentialBackoff;
  
  LOG_INFO("Retry config updated: max=" + String(maxRetries) + 
           ", delay=" + String(retryDelay) + "ms");
}

String ApiModule::buildUrl(const String& endpoint) {
  String url = baseUrl;
  if (!url.endsWith("/")) url += "/";
  if (endpoint.startsWith("/")) {
    url += endpoint.substring(1);
  } else {
    url += endpoint;
  }
  return url;
}

bool ApiModule::validateResponse(const String& response) {
  if (response.length() == 0) {
    return false;
  }
  
  // Try to parse as JSON
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, response);
  
  if (error) {
    LOG_WARNING("Response validation failed: Invalid JSON");
    return false;
  }
  
  // Check for success field
  if (!doc.containsKey("success")) {
    LOG_WARNING("Response validation failed: Missing 'success' field");
    return false;
  }
  
  return true;
}

ApiResponse ApiModule::sendRequest(const String& method, const String& endpoint, 
                                   const String& payload, bool useRetry) {
  if (useRetry) {
    return sendRequestWithRetry(method, endpoint, payload);
  }
  
  ApiResponse response;
  response.result = API_NETWORK_ERROR;
  response.httpCode = 0;
  response.data = "";
  response.error = "";
  
  if (!initialized) {
    LOG_ERROR("API not initialized");
    response.error = "API not initialized";
    return response;
  }
  
  // Check memory before making request
  if (!CHECK_MEMORY()) {
    LOG_WARNING("Low memory - request may fail");
  }
  
  String url = buildUrl(endpoint);
  unsigned long startTime = millis();
  
  LOG_DEBUG("API Request: " + method + " " + endpoint);
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-api-key", apiKey);
  http.addHeader("User-Agent", String(DEVICE_NAME) + "/" + String(DEVICE_VERSION));
  http.setTimeout(API_TIMEOUT_MS);
  
  int httpCode;
  if (method == "POST") {
    if (payload.length() > 0) {
      LOG_DEBUG("Payload size: " + String(payload.length()) + " bytes");
    }
    httpCode = http.POST(payload);
  } else if (method == "PUT") {
    httpCode = http.PUT(payload);
  } else if (method == "GET") {
    httpCode = http.GET();
  } else if (method == "DELETE") {
    httpCode = http.sendRequest("DELETE");
  } else {
    LOG_ERROR("Unsupported HTTP method: " + method);
    response.error = "Unsupported method";
    http.end();
    return response;
  }
  
  lastRequestTime = millis();
  unsigned long requestDuration = lastRequestTime - startTime;
  totalRequests++;
  totalResponseTime += requestDuration;
  
  if (httpCode > 0) {
    response.data = http.getString();
    response.httpCode = httpCode;
    http.end();
    
    LOG_DEBUG("Response: " + String(httpCode) + " (" + String(requestDuration) + "ms)");
    
    if (httpCode >= 200 && httpCode < 300) {
      // Validate response structure
      if (validateResponse(response.data)) {
        response.result = API_SUCCESS;
        consecutiveFailures = 0;
        successfulRequests++;
      } else {
        response.result = API_JSON_ERROR;
        response.error = "Invalid response format";
        consecutiveFailures++;
        failedRequests++;
      }
    } else {
      response.result = API_HTTP_ERROR;
      response.error = "HTTP " + String(httpCode);
      consecutiveFailures++;
      failedRequests++;
      LOG_WARNING("HTTP Error: " + String(httpCode));
    }
  } else {
    response.error = http.errorToString(httpCode).c_str();
    response.httpCode = httpCode;
    consecutiveFailures++;
    failedRequests++;
    http.end();
    LOG_ERROR("Connection error: " + response.error);
  }
  
  return response;
}

ApiResponse ApiModule::sendRequestWithRetry(const String& method, const String& endpoint, 
                                           const String& payload) {
  ApiResponse response;
  int attempt = 0;
  unsigned long retryDelay = retryConfig.retryDelay;
  
  while (attempt <= retryConfig.maxRetries) {
    if (attempt > 0) {
      LOG_INFO("Retry attempt " + String(attempt) + "/" + String(retryConfig.maxRetries));
      delay(retryDelay);
      
      // Exponential backoff
      if (retryConfig.exponentialBackoff) {
        retryDelay *= 2;
      }
    }
    
    response = sendRequest(method, endpoint, payload, false);
    
    if (response.result == API_SUCCESS) {
      if (attempt > 0) {
        LOG_INFO("Request succeeded after " + String(attempt) + " retries");
      }
      return response;
    }
    
    attempt++;
  }
  
  LOG_ERROR("Request failed after " + String(retryConfig.maxRetries) + " retries");
  return response;
}

ApiResponse ApiModule::sendScan(const String& tagId, const String& location) {
  if (!IS_VALID_TAG_ID(tagId)) {
    ApiResponse response;
    response.result = API_JSON_ERROR;
    response.error = "Invalid tag ID";
    response.httpCode = 0;
    response.data = "";
    return response;
  }
  
  StaticJsonDocument<512> doc;
  doc["tagId"] = tagId;
  doc["deviceId"] = deviceId;
  doc["timestamp"] = millis();
  
  if (location.length() > 0) {
    doc["location"] = location;
  } else {
    doc["location"] = deviceConfig.location;
  }
  
  // Add device context
  doc["uptime"] = millis() / 1000;
  doc["freeHeap"] = ESP.getFreeHeap();
  
  String payload;
  serializeJson(doc, payload);
  
  LOG_INFO("Sending RFID scan: " + tagId);
  
  return sendRequest("POST", "/api/rfid/scan", payload);
}

ApiResponse ApiModule::sendHeartbeat(bool includeStats) {
  String endpoint = "/api/devices/" + deviceId + "/heartbeat";
  
  StaticJsonDocument<512> doc;
  doc["status"] = "online";
  doc["uptime"] = millis() / 1000;
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["location"] = deviceConfig.location;
  doc["firmwareVersion"] = FIRMWARE_VERSION;
  // Sync key device flags so server UI reflects current state without waiting for poll
  doc["registrationMode"] = registrationMode;
  doc["scanMode"] = deviceConfig.scanMode;
  if (expectedRegistrationTagId.length() > 0) {
    doc["pendingRegistrationTagId"] = expectedRegistrationTagId;
  }
  
  if (includeStats) {
    JsonObject stats = doc.createNestedObject("stats");
    stats["totalScans"] = systemStatus.scanCount;
    stats["errorCount"] = systemStatus.errorCount;
    stats["apiSuccessRate"] = getSuccessRate();
    stats["avgResponseTime"] = (totalRequests > 0) ? (totalResponseTime / totalRequests) : 0;
  }
  
  String payload;
  serializeJson(doc, payload);
  
  LOG_DEBUG("Sending heartbeat");
  
  return sendRequest("POST", endpoint, payload);
}

ApiResponse ApiModule::checkConnection() {
  LOG_DEBUG("Checking API connection");
  return sendRequest("GET", "/api/health", "", false);  // No retry for health check
}

ApiResponse ApiModule::getRegistrationStatus() {
  String endpoint = "/api/devices/" + deviceId + "/registration-status";
  LOG_DEBUG("Checking registration status");
  return sendRequest("GET", endpoint, "");
}

ApiResponse ApiModule::sendQueueOverride(int queueNumber, const String& reason) {
  if (!IS_VALID_QUEUE_NUMBER(queueNumber)) {
    ApiResponse response;
    response.result = API_JSON_ERROR;
    response.error = "Invalid queue number";
    response.httpCode = 0;
    response.data = "";
    return response;
  }
  
  String endpoint = "/api/devices/" + deviceId + "/queue-override";
  
  StaticJsonDocument<256> doc;
  doc["queueNumber"] = queueNumber;
  doc["reason"] = reason;
  doc["timestamp"] = millis();
  
  String payload;
  serializeJson(doc, payload);
  
  LOG_INFO("Sending queue override: " + String(queueNumber));
  
  return sendRequest("POST", endpoint, payload);
}

ApiResponse ApiModule::reportStatus(const String& status, const String& reason) {
  String endpoint = "/api/devices/" + deviceId + "/status";
  
  StaticJsonDocument<512> doc;
  doc["status"] = status;
  doc["reason"] = reason;
  doc["timestamp"] = millis();
  doc["uptime"] = millis() / 1000;
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["location"] = deviceConfig.location;
  doc["firmwareVersion"] = FIRMWARE_VERSION;
  doc["wifiConnected"] = systemStatus.wifiConnected;
  doc["rfidInitialized"] = systemStatus.rfidInitialized;
  doc["offlineMode"] = systemStatus.offlineMode;
  
  String payload;
  serializeJson(doc, payload);
  
  LOG_INFO("Reporting status: " + status);
  
  return sendRequest("POST", endpoint, payload);
}

ApiResponse ApiModule::registerDevice(const String& macAddress, const String& name, const String& location) {
  StaticJsonDocument<256> doc;
  doc["macAddress"] = macAddress;
  doc["name"] = name;
  doc["location"] = location;
  
  String payload;
  serializeJson(doc, payload);
  
  LOG_INFO("Registering device: " + macAddress);
  
  return sendRequest("POST", "/api/devices", payload);
}

ApiResponse ApiModule::updateDeviceConfig(const String& config) {
  String endpoint = "/api/devices/" + deviceId + "/config";
  
  LOG_INFO("Updating device configuration");
  
  return sendRequest("PUT", endpoint, config);
}

ApiResponse ApiModule::getDeviceConfig() {
  String endpoint = "/api/devices/" + deviceId + "/config";
  
  LOG_DEBUG("Fetching device configuration");
  
  return sendRequest("GET", endpoint, "");
}

ApiResponse ApiModule::reportError(const String& errorType, const String& errorMessage) {
  String endpoint = "/api/devices/" + deviceId + "/error";
  
  StaticJsonDocument<512> doc;
  doc["errorType"] = errorType;
  doc["errorMessage"] = errorMessage;
  doc["timestamp"] = millis();
  doc["uptime"] = millis() / 1000;
  doc["freeHeap"] = ESP.getFreeHeap();
  
  String payload;
  serializeJson(doc, payload);
  
  LOG_WARNING("Reporting error: " + errorType);
  
  return sendRequest("POST", endpoint, payload, false);  // Don't retry error reports
}

ApiResponse ApiModule::syncTime() {
  LOG_DEBUG("Syncing time from server");
  return sendRequest("GET", "/api/time", "");
}

ApiResponse ApiModule::sendBatchScans(const String scans[], int count) {
  if (count == 0 || count > MAX_SCAN_QUEUE_SIZE) {
    ApiResponse response;
    response.result = API_JSON_ERROR;
    response.error = "Invalid batch size";
    response.httpCode = 0;
    response.data = "";
    return response;
  }
  
  DynamicJsonDocument doc(2048);  // Larger buffer for batch
  JsonArray scanArray = doc.createNestedArray("scans");
  
  for (int i = 0; i < count; i++) {
    JsonObject scan = scanArray.createNestedObject();
    
    // Parse scan string (format: "tagId|timestamp")
    int separatorPos = scans[i].indexOf('|');
    if (separatorPos > 0) {
      scan["tagId"] = scans[i].substring(0, separatorPos);
      scan["timestamp"] = scans[i].substring(separatorPos + 1).toInt();
    } else {
      scan["tagId"] = scans[i];
      scan["timestamp"] = millis();
    }
    scan["deviceId"] = deviceId;
  }
  
  doc["deviceId"] = deviceId;
  doc["count"] = count;
  
  String payload;
  serializeJson(doc, payload);
  
  LOG_INFO("Sending batch scans: " + String(count) + " items");
  
  return sendRequest("POST", "/api/rfid/batch-scan", payload);
}

void ApiModule::getStatistics(unsigned long& total, unsigned long& success, 
                              unsigned long& failed, unsigned long& avgResponseTime) {
  total = totalRequests;
  success = successfulRequests;
  failed = failedRequests;
  avgResponseTime = (totalRequests > 0) ? (totalResponseTime / totalRequests) : 0;
}

void ApiModule::resetStatistics() {
  totalRequests = 0;
  successfulRequests = 0;
  failedRequests = 0;
  totalResponseTime = 0;
  consecutiveFailures = 0;
  
  LOG_INFO("API statistics reset");
}

float ApiModule::getSuccessRate() const {
  if (totalRequests == 0) {
    return 0.0f;
  }
  return (float)successfulRequests / (float)totalRequests * 100.0f;
}

bool ApiModule::testEndpoint(const String& endpoint) {
  LOG_INFO("Testing endpoint: " + endpoint);
  ApiResponse response = sendRequest("GET", endpoint, "", false);
  return response.result == API_SUCCESS;
}

String ApiModule::getLastError() const {
  // Error information is now returned in ApiResponse.error field
  // This method kept for backwards compatibility
  return "";
}

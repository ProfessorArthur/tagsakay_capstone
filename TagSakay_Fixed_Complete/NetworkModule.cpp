#include "NetworkModule.h"
#include "DisplayModule.h"
#include "UARTModule.h"

// NetworkModule Class Implementation
NetworkModule::NetworkModule() 
  : initialized(false), connected(false), lastConnectionAttempt(0),
    connectionTimeout(WIFI_RECONNECT_INTERVAL), reconnectAttempts(0),
    macAddress(""), ipAddress(""), consecutiveFailures(0) {}

bool NetworkModule::initialize(const char* ssid, const char* password) {
  Serial.println("[NETWORK] Initializing WiFi...");
  Serial.print("[NETWORK] SSID: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  macAddress = WiFi.macAddress();
  macAddress.replace(":", "");
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < MAX_WIFI_RECONNECT_ATTEMPTS) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    connected = true;
    initialized = true;
    ipAddress = WiFi.localIP().toString();
    
    Serial.println();
    Serial.println("[NETWORK] WiFi connected!");
    Serial.print("[NETWORK] IP: ");
    Serial.println(ipAddress);
    Serial.print("[NETWORK] MAC: ");
    Serial.println(macAddress);
    
    reconnectAttempts = 0;
    return true;
  } else {
    connected = false;
    Serial.println();
    Serial.println("[NETWORK] WiFi connection failed!");
    return false;
  }
}

bool NetworkModule::reconnect() {
  if (millis() - lastConnectionAttempt < connectionTimeout) {
    return false;
  }
  
  lastConnectionAttempt = millis();
  reconnectAttempts++;
  
  Serial.print("[NETWORK] Reconnecting... Attempt ");
  Serial.print(reconnectAttempts);
  Serial.print("/");
  Serial.println(MAX_WIFI_RECONNECT_ATTEMPTS);
  
  if (reconnectAttempts >= MAX_WIFI_RECONNECT_ATTEMPTS) {
    Serial.println("[NETWORK] Max reconnect attempts reached. Restarting WiFi...");
    WiFi.disconnect();
    delay(1000);
    WiFi.begin();
    reconnectAttempts = 0;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    connected = true;
    ipAddress = WiFi.localIP().toString();
    consecutiveFailures = 0;
    
    Serial.println("[NETWORK] Reconnected successfully!");
    return true;
  }
  
  connected = false;
  return false;
}

void NetworkModule::updateConnectionStatus() {
  bool currentStatus = (WiFi.status() == WL_CONNECTED);
  
  if (currentStatus != connected) {
    connected = currentStatus;
    
    if (connected) {
      ipAddress = WiFi.localIP().toString();
      Serial.println("[NETWORK] Connection restored");
    } else {
      Serial.println("[NETWORK] Connection lost");
    }
  }
}

// Legacy compatibility functions
bool connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  Serial.print("SSID: ");
  Serial.println(wifiConfig.ssid);

  WiFi.begin(wifiConfig.ssid, wifiConfig.password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < wifiConfig.maxRetries) {
    delay(wifiConfig.retryDelay);
    Serial.print(".");
    attempts++;

    String statusMsg = "WiFi: " + String(attempts) + "/" + String(wifiConfig.maxRetries);
    updateStatusSection(statusMsg, TFT_YELLOW);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC Address: ");
    Serial.println(WiFi.macAddress());

    updateStatusSection("WiFi CONNECTED", TFT_GREEN);
    return true;
  } else {
    Serial.println("\nWiFi connection failed!");
    updateStatusSection("WiFi FAILED", TFT_RED);
    return false;
  }
}

String getDeviceMacAddress() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");  // Remove colons to match backend format
  return mac;
}

bool initializeTime() {
  configTime(ntpConfig.gmtOffset_sec, ntpConfig.daylightOffset_sec, ntpConfig.ntpServer);

  Serial.println("Waiting for NTP time sync...");

  int attempts = 0;
  while (attempts < 10) {
    time_t now = time(nullptr);
    if (now > 1000000000) {  // Valid timestamp
      struct tm timeinfo;
      localtime_r(&now, &timeinfo);

      Serial.println("Time synchronized!");
      Serial.print("Current time: ");
      Serial.println(asctime(&timeinfo));

      return true;
    }
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  Serial.println("\nTime sync failed!");
  return false;
}

String getCurrentTimestamp() {
  time_t now = time(nullptr);
  if (now < 1000000000) {
    return "";
  }

  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  char timestamp[25];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);

  return String(timestamp);
}

ApiResponse makeApiRequest(const String& endpoint, const String& payload, const String& method) {
  ApiResponse response;
  response.result = API_NETWORK_ERROR;
  response.httpCode = 0;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    response.error = "WiFi not connected";
    return response;
  }

  HTTPClient http;
  String url = String(serverConfig.baseUrl) + endpoint;

  Serial.println("=== API Request ===");
  Serial.print("URL: ");
  Serial.println(url);
  Serial.print("Method: ");
  Serial.println(method);

  http.begin(url);
  http.setTimeout(serverConfig.timeout);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-api-key", serverConfig.apiKey);

  int httpCode;
  if (method == "POST") {
    Serial.print("Payload: ");
    Serial.println(payload);
    httpCode = http.POST(payload);
  } else if (method == "PUT") {
    Serial.print("Payload: ");
    Serial.println(payload);
    httpCode = http.PUT(payload);
  } else {
    httpCode = http.GET();
  }

  response.httpCode = httpCode;

  if (httpCode > 0) {
    String responseBody = http.getString();
    Serial.print("HTTP Response Code: ");
    Serial.println(httpCode);
    Serial.print("Response: ");
    Serial.println(responseBody);

    response.data = responseBody;

    if (httpCode >= 200 && httpCode < 300) {
      response.result = API_SUCCESS;
    } else {
      response.result = API_HTTP_ERROR;
      response.error = "HTTP " + String(httpCode);
    }
  } else {
    Serial.print("HTTP Request failed: ");
    Serial.println(http.errorToString(httpCode));
    response.error = http.errorToString(httpCode);
    response.result = API_NETWORK_ERROR;
  }

  http.end();
  Serial.println("===================");

  return response;
}

void handleRfidScan(String tagId) {
  Serial.println("Processing RFID scan for tag: " + tagId);

  updateStatusSection("SCANNING...", TFT_CYAN);
  updateScanSection(tagId, "Processing...", "", TFT_YELLOW);

  String endpoint = "/api/rfid/scan";

  StaticJsonDocument<512> doc;
  doc["tagId"] = tagId;
  doc["deviceId"] = deviceId;
  doc["timestamp"] = getCurrentTimestamp();
  doc["location"] = serverConfig.deviceLocation;

  String payload;
  serializeJson(doc, payload);

  ApiResponse response = makeApiRequest(endpoint, payload, "POST");

  if (response.result == API_SUCCESS) {
    handleScanResponse(response.data);
  } else {
    Serial.println("Scan request failed!");
    updateStatusSection("SCAN FAILED", TFT_RED);
    updateScanSection(tagId, "Network Error", response.error, TFT_RED);
    updateFooter("Failed to process scan");
    sendToLEDMatrix("ERROR", "NETWORK", "");
    blinkError(2);
  }
}

void handleScanResponse(const String& responseData) {
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, responseData);

  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    updateStatusSection("JSON ERROR", TFT_RED);
    blinkError(1);
    return;
  }

  bool success = doc["success"] | false;
  String message = doc["message"] | "Unknown response";

  Serial.print("API Success: ");
  Serial.println(success ? "true" : "false");
  Serial.print("Message: ");
  Serial.println(message);

  if (success) {
    JsonObject data = doc["data"];
    String tagId = data["tagId"] | "";
    String status = data["status"] | "";
    int queueNumber = data["queueNumber"] | 0;

    String firstName = data["driver"]["firstName"] | "";
    String lastName = data["driver"]["lastName"] | "";
    String userInfo = firstName + " " + lastName;

    Serial.println("--- Scan Details ---");
    Serial.print("Tag ID: ");
    Serial.println(tagId);
    Serial.print("Status: ");
    Serial.println(status);
    Serial.print("Queue Number: ");
    Serial.println(queueNumber);
    Serial.print("Driver: ");
    Serial.println(userInfo);
    Serial.println("--------------------");

    if (status == "registered") {
      indicateSuccess();
      updateScanSection(tagId, "REGISTERED", userInfo, TFT_GREEN);
      updateFooter("Scan successful - Queue #" + String(queueNumber));

      String queueStr = String(queueNumber);
      String driverShort = firstName.substring(0, min((int)firstName.length(), 8));
      sendToLEDMatrix("QUEUE", queueStr, driverShort);

    } else if (status == "unregistered") {
      indicateUnregisteredTag();
      updateScanSection(tagId, "UNREGISTERED", "Card not registered", TFT_ORANGE);
      updateFooter("Unregistered card detected");
      sendToLEDMatrix("UNREG", tagId.substring(0, 8), "");

    } else {
      updateStatusSection("UNKNOWN STATUS", TFT_ORANGE);
      updateScanSection(tagId, status, "", TFT_YELLOW);
      updateFooter("Unknown scan status");
      sendToLEDMatrix("STATUS", status.substring(0, 8), "");
    }

  } else {
    Serial.println("Scan failed on server");
    indicateError();
    updateScanSection(lastScannedTag, "FAILED", message, TFT_RED);
    updateFooter("Server reported error");
    sendToLEDMatrix("ERROR", "SERVER", "");
  }
}

void sendHeartbeat() {
  Serial.println("Sending heartbeat...");

  String endpoint = "/api/devices/" + deviceId + "/heartbeat";

  StaticJsonDocument<256> doc;
  doc["status"] = "online";
  doc["timestamp"] = getCurrentTimestamp();
  doc["uptime"] = millis() / 1000;
  doc["location"] = serverConfig.deviceLocation;

  String payload;
  serializeJson(doc, payload);

  showHeartbeat(true);

  ApiResponse response = makeApiRequest(endpoint, payload, "POST");

  if (response.result == API_SUCCESS) {
    Serial.println("Heartbeat sent successfully");
  } else {
    Serial.println("Heartbeat failed");
  }

  delay(100);
  showHeartbeat(false);
}

void reportDeviceStatus(String reason) {
  Serial.print("Reporting device status. Reason: ");
  Serial.println(reason);

  String endpoint = "/api/devices/" + deviceId + "/status";

  StaticJsonDocument<512> doc;
  doc["status"] = "active";
  doc["reason"] = reason;
  doc["timestamp"] = getCurrentTimestamp();
  doc["location"] = serverConfig.deviceLocation;
  doc["registrationMode"] = registrationMode;
  doc["uptime"] = millis() / 1000;
  doc["freeHeap"] = ESP.getFreeHeap();

  String payload;
  serializeJson(doc, payload);

  ApiResponse response = makeApiRequest(endpoint, payload, "POST");

  if (response.result == API_SUCCESS) {
    Serial.println("Device status reported successfully");
  } else {
    Serial.println("Failed to report device status");
  }
}

void checkRegistrationModeFromServer() {
  String endpoint = "/api/devices/" + deviceId + "/registration-status";

  ApiResponse response = makeApiRequest(endpoint, "", "GET");

  if (response.result == API_SUCCESS) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, response.data);

    if (!error) {
      bool serverRegistrationMode = doc["data"]["registrationMode"] | false;
      String expectedTag = doc["data"]["expectedTagId"] | "";

      if (serverRegistrationMode != registrationMode) {
        registrationMode = serverRegistrationMode;
        expectedRegistrationTagId = expectedTag;

        Serial.print("Registration mode updated from server: ");
        Serial.println(registrationMode ? "ENABLED" : "DISABLED");

        if (registrationMode) {
          Serial.print("Expected tag ID: ");
          Serial.println(expectedRegistrationTagId);
          registrationModeStartTime = millis();
          indicateRegistrationMode();
          updateScanSection("", "Waiting for tag", expectedRegistrationTagId, TFT_MAGENTA);
          sendToLEDMatrix("REG", "WAITING", expectedTag.substring(0, 8));
        } else {
          Serial.println("Registration mode disabled by server");
          indicateReady();
          updateScanSection("", "", "", TFT_WHITE);
        }
      }
    }
  }
}
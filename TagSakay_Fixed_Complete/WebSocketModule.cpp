#include "WebSocketModule.h"

// Static instance pointer for callback
WebSocketModule* WebSocketModule::instance = nullptr;

WebSocketModule::WebSocketModule() {
  ws = new WebSocketsClient();
  connected = false;
  lastHeartbeat = 0;
  lastReconnectAttempt = 0;
  onScanResponseCallback = nullptr;
  onConfigUpdateCallback = nullptr;
  onConnectionStatusCallback = nullptr;
  instance = this;
}

WebSocketModule::~WebSocketModule() {
  if (ws) {
    delete ws;
  }
}

void WebSocketModule::begin(String deviceId) {
  this->deviceId = deviceId;
  
  // Build WebSocket path with deviceId
  String path = String(WS_PATH) + "?deviceId=" + deviceId;
  
  // Connect to WebSocket server
  ws->begin(WS_HOST, WS_PORT, path);
  ws->onEvent(staticWebSocketEvent);
  
  // Set reconnect interval
  ws->setReconnectInterval(WS_RECONNECT_INTERVAL);
  
  Serial.println("[WS] Initializing WebSocket...");
  Serial.printf("[WS] Connecting to: %s:%d%s\n", WS_HOST, WS_PORT, path.c_str());
}

void WebSocketModule::loop() {
  ws->loop();
  
  // Send heartbeat every 30 seconds if connected
  if (connected && (millis() - lastHeartbeat > WS_PING_INTERVAL)) {
    sendHeartbeat();
  }
  
  // Attempt reconnection if disconnected
  if (!connected && (millis() - lastReconnectAttempt > WS_RECONNECT_INTERVAL)) {
    lastReconnectAttempt = millis();
    Serial.println("[WS] Attempting to reconnect...");
  }
}

bool WebSocketModule::isConnected() {
  return connected;
}

void WebSocketModule::sendScan(String tagId, String location) {
  if (!connected) {
    Serial.println("[WS] Not connected - cannot send scan");
    return;
  }
  
  JsonDocument doc;
  doc["action"] = "scan";
  doc["tagId"] = tagId;
  doc["location"] = location;
  doc["timestamp"] = millis();
  
  String message;
  serializeJson(doc, message);
  
  ws->sendTXT(message);
  Serial.println("[WS] Scan sent: " + tagId);
}

void WebSocketModule::sendHeartbeat() {
  if (!connected) return;
  
  JsonDocument doc;
  doc["action"] = "heartbeat";
  doc["timestamp"] = millis();
  
  String message;
  serializeJson(doc, message);
  
  ws->sendTXT(message);
  lastHeartbeat = millis();
  Serial.println("[WS] Heartbeat sent");
}

void WebSocketModule::sendConfig(bool registrationMode, bool scanMode) {
  if (!connected) return;
  
  JsonDocument doc;
  doc["action"] = "config";
  doc["registrationMode"] = registrationMode;
  doc["scanMode"] = scanMode;
  
  String message;
  serializeJson(doc, message);
  
  ws->sendTXT(message);
  Serial.println("[WS] Config update sent");
}

void WebSocketModule::setOnScanResponse(void (*callback)(JsonDocument&)) {
  onScanResponseCallback = callback;
}

void WebSocketModule::setOnConfigUpdate(void (*callback)(JsonDocument&)) {
  onConfigUpdateCallback = callback;
}

void WebSocketModule::setOnConnectionStatus(void (*callback)(bool)) {
  onConnectionStatusCallback = callback;
}

void WebSocketModule::staticWebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  if (instance) {
    instance->webSocketEvent(type, payload, length);
  }
}

void WebSocketModule::webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("[WS] Disconnected");
      connected = false;
      if (onConnectionStatusCallback) {
        onConnectionStatusCallback(false);
      }
      break;
      
    case WStype_CONNECTED:
      Serial.printf("[WS] Connected to: %s\n", payload);
      connected = true;
      lastHeartbeat = millis();
      if (onConnectionStatusCallback) {
        onConnectionStatusCallback(true);
      }
      break;
      
    case WStype_TEXT:
      Serial.printf("[WS] Message received: %s\n", payload);
      handleMessage(payload, length);
      break;
      
    case WStype_ERROR:
      Serial.printf("[WS] Error: %s\n", payload);
      break;
      
    case WStype_PING:
      Serial.println("[WS] Ping received");
      break;
      
    case WStype_PONG:
      Serial.println("[WS] Pong received");
      break;
  }
}

void WebSocketModule::handleMessage(uint8_t* payload, size_t length) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload, length);
  
  if (error) {
    Serial.print("[WS] JSON parse error: ");
    Serial.println(error.c_str());
    return;
  }
  
  // Check if it's a scan response
  if (doc.containsKey("scan") && onScanResponseCallback) {
    onScanResponseCallback(doc);
  }
  
  // Check if it's a config update
  if (doc.containsKey("config") && onConfigUpdateCallback) {
    onConfigUpdateCallback(doc);
  }
  
  // Check for heartbeat acknowledgment
  if (doc["action"] == "heartbeat_ack") {
    int scanCount = doc["scanCount"] | 0;
    Serial.printf("[WS] Heartbeat acknowledged (scans: %d)\n", scanCount);
  }
  
  // Check for error messages
  if (doc["success"] == false) {
    const char* error = doc["error"] | "Unknown error";
    Serial.printf("[WS] Error from server: %s\n", error);
  }
}

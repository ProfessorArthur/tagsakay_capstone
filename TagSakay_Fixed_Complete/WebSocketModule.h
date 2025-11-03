#ifndef WEBSOCKETMODULE_H
#define WEBSOCKETMODULE_H

#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "Config.h"

class WebSocketModule {
private:
  WebSocketsClient* ws;
  String deviceId;
  bool connected;
  unsigned long lastHeartbeat;
  unsigned long lastReconnectAttempt;
  
  // Callback for received messages
  void (*onScanResponseCallback)(JsonDocument&);
  void (*onConfigUpdateCallback)(JsonDocument&);
  void (*onConnectionStatusCallback)(bool);

public:
  WebSocketModule();
  ~WebSocketModule();
  
  void begin(String deviceId);
  void loop();
  bool isConnected();
  void sendScan(String tagId, String location = "");
  void sendHeartbeat();
  void sendConfig(bool registrationMode, bool scanMode);
  void setOnScanResponse(void (*callback)(JsonDocument&));
  void setOnConfigUpdate(void (*callback)(JsonDocument&));
  void setOnConnectionStatus(void (*callback)(bool));
  
private:
  void connect();
  void reconnect();
  void handleMessage(uint8_t* payload, size_t length);
  void webSocketEvent(WStype_t type, uint8_t* payload, size_t length);
  
  // Static callback handler
  static WebSocketModule* instance;
  static void staticWebSocketEvent(WStype_t type, uint8_t* payload, size_t length);
};

#endif

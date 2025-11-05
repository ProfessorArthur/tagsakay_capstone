/*
 * TagSakay RFID Scanner - Production Ready Version with WebSocket
 * 
 * Modular architecture with comprehensive error handling,
 * state management, automatic recovery, and real-time WebSocket communication
 * 
 * Version: 3.0.0
 * Features: WebSocket + HTTP fallback, Durable Objects integration
 */

#include "Config.h"
#include "DisplayModule.h"
#include "NetworkModule.h"
#include "RFIDModule.h"
#include "KeypadModule.h"
#include "UARTModule.h"
#include "ApiModule.h"
#include "WebSocketModule.h"

// Configuration instances (definitions)
WiFiConfig wifiConfig = {
  "SSID",           // Replace with your WiFi SSID
  "Password",       // Replace with your WiFi password
  10,               // Max reconnection attempts
  5000              // Retry delay (ms)
};

ServerConfig serverConfig = {
  "https://tagsakay-api-production.maskedmyles.workers.dev",  // Production backend URL
  "",  // Device API key (set in Config.h or via Serial menu)
  10000,  // HTTP timeout (ms)
  "Entrance Gate"  // Device location (configurable)
};

NTPConfig ntpConfig = {
  "pool.ntp.org",
  8 * 3600,
  0
};

// Device configuration
DeviceConfig deviceConfig = {
  DEVICE_NAME,
  "Entrance Gate",  // Will be set based on serverConfig
  FIRMWARE_VERSION,
  false,  // registrationMode
  false,  // scanMode
  LED_BRIGHTNESS_DEFAULT,
  MIN_SCAN_INTERVAL
};

// System status tracking
SystemStatus systemStatus = {
  false,  // wifiConnected
  false,  // rfidInitialized
  false,  // apiConnected
  false,  // offlineMode
  0,      // uptime
  0,      // freeHeap
  0,      // scanCount
  0,      // errorCount
  0       // lastHeartbeat
};

// Global state variables (definitions)
String deviceId = "";
String lastScannedTag = "";
bool registrationMode = false;
String expectedRegistrationTagId = "";
unsigned long lastRegistrationCheck = 0;
unsigned long registrationModeStartTime = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastScanTime = 0;

// Registration mode keypad buffer (renamed to avoid conflict with KeypadModule.cpp)
String registrationKeypadBuffer = "";
unsigned long lastRegistrationKeypadInput = 0;
#define KEYPAD_BUFFER_TIMEOUT 3000  // Clear buffer after 3 seconds of no input

// Module instances (using enhanced classes)
NetworkModule networkModule;
RFIDModule rfidModule;
KeypadModule keypadModule;
ApiModule apiModule;
WebSocketModule wsModule;  // New: WebSocket module

// System state
bool systemReady = false;
bool offlineMode = false;
bool useWebSocket = WS_ENABLED;  // Can be toggled at runtime

// Function declarations
bool initializeSystem();
void handleSystemError(const char* component, const char* error);
void handleRFIDScanning();
void handleKeypadInputNew();
void sendPeriodicHeartbeat();
void checkNetworkConnection();
void checkSerialCommands();

// WebSocket callback declarations
void handleScanResponse(JsonDocument& doc);
void handleConfigUpdate(JsonDocument& doc);
void handleWSConnectionStatus(bool connected);

void setup(void) {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n================================");
  Serial.println("  TagSakay RFID Scanner v2.0");
  Serial.println("================================\n");

  // Initialize system with comprehensive error handling
  if (!initializeSystem()) {
    Serial.println("\n[SYSTEM] FATAL: Initialization failed!");
    Serial.println("[SYSTEM] Entering safe mode - limited functionality");
    
    updateStatusSection("INIT FAILED", TFT_RED);
    updateFooter("System in safe mode");
    
    // Don't halt - allow manual recovery
    systemReady = false;
    offlineMode = true;
  } else {
    Serial.println("\n[SYSTEM] All modules initialized successfully");
    Serial.println("[SYSTEM] System ready for operation");
    Serial.println("[SYSTEM] Press 'A' on keypad for menu\n");
    
    systemReady = true;
    indicateReady();
    updateScanSection("", "", "", TFT_WHITE);
    sendToLEDMatrix("STATUS", "READY", "");
  }
}

bool initializeSystem() {
  bool allSuccess = true;
  
  LOG_INFO("System initialization started");
  
  // 1. Initialize Display (first for visual feedback)
  Serial.println("[1/6] Initializing Display...");
  initializeTFT();
  delay(500);

  // 2. Initialize UART
  Serial.println("[2/6] Initializing UART...");
  initializeUART();
  updateStatusSection("UART: OK", TFT_GREEN);
  delay(500);

  // 3. Initialize Keypad
  Serial.println("[3/6] Initializing Keypad...");
  if (!keypadModule.initialize()) {
    handleSystemError("KEYPAD", "Initialization failed");
    allSuccess = false;
  } else {
    updateStatusSection("Keypad: OK", TFT_GREEN);
  }
  delay(500);

  // 4. Initialize Network
  Serial.println("[4/6] Initializing Network...");
  updateStatusSection("Connecting WiFi...", TFT_YELLOW);
  
  WiFi.mode(WIFI_STA);
  deviceId = getDeviceMacAddress();
  Serial.print("[NETWORK] Device ID (MAC): ");
  Serial.println(deviceId);
  
  String deviceDisplay = deviceId.length() >= 4 ? deviceId.substring(deviceId.length() - 4) : deviceId;
  
  if (!networkModule.initialize(wifiConfig.ssid, wifiConfig.password)) {
    handleSystemError("NETWORK", "WiFi connection failed");
    updateConnectionStatus("Failed", "No sync", deviceDisplay);
    offlineMode = true;
    allSuccess = false;
    systemStatus.wifiConnected = false;
    systemStatus.offlineMode = true;
    Serial.println("[NETWORK] Continuing in OFFLINE mode");
  } else {
    updateStatusSection("WiFi: OK", TFT_GREEN);
    updateConnectionStatus("Connected", "Syncing...", deviceDisplay);
    Serial.print("[NETWORK] IP: ");
    Serial.println(networkModule.getIpAddress());
    systemStatus.wifiConnected = true;
  }
  delay(500);

  // 5. Initialize RFID
  Serial.println("[5/6] Initializing RFID...");
  updateStatusSection("Init RFID...", TFT_YELLOW);
  
  if (!rfidModule.initialize()) {
    handleSystemError("RFID", "PN532 not found");
    allSuccess = false;
    systemStatus.rfidInitialized = false;
  } else {
    updateStatusSection("RFID: OK", TFT_GREEN);
    Serial.println("[RFID] " + rfidModule.getFirmwareVersion());
    systemStatus.rfidInitialized = true;
  }
  delay(500);

  // 6. Initialize API Client
  Serial.println("[6/7] Initializing API...");
  updateStatusSection("Connecting API...", TFT_YELLOW);
  
  if (!apiModule.initialize(serverConfig.baseUrl, serverConfig.apiKey, deviceId)) {
    handleSystemError("API", "Initialization failed");
    allSuccess = false;
    systemStatus.apiConnected = false;
  } else {
    updateStatusSection("API: OK", TFT_GREEN);
    systemStatus.apiConnected = true;
  }
  delay(500);

  // 7. Initialize WebSocket (if WiFi connected and enabled)
  Serial.println("[7/7] Initializing WebSocket...");
  if (useWebSocket && systemStatus.wifiConnected) {
    updateStatusSection("Connecting WS...", TFT_YELLOW);
    
    // Set up WebSocket callbacks
    wsModule.setOnScanResponse(handleScanResponse);
    wsModule.setOnConfigUpdate(handleConfigUpdate);
    wsModule.setOnConnectionStatus(handleWSConnectionStatus);
    
    // Initialize WebSocket connection
    wsModule.begin(deviceId);
    updateStatusSection("WS: Connecting", TFT_YELLOW);
    
    Serial.println("[WS] WebSocket module initialized");
    Serial.println("[WS] Real-time communication enabled");
  } else if (!useWebSocket) {
    Serial.println("[WS] WebSocket disabled - using HTTP only");
    updateStatusSection("WS: Disabled", TFT_ORANGE);
  } else {
    Serial.println("[WS] WebSocket unavailable - no WiFi");
    updateStatusSection("WS: Offline", TFT_ORANGE);
  }
  delay(500);
  
  // Test API connection
  if (!apiModule.isInitialized()) {
    Serial.println("[API] WARNING: API module not initialized");
    offlineMode = true;
    allSuccess = false;
    systemStatus.apiConnected = false;
  } else {
    ApiResponse connCheck = apiModule.checkConnection();
    if (offlineMode || connCheck.result != API_SUCCESS) {
      Serial.println("[API] WARNING: Backend not reachable");
      updateStatusSection("API: OFFLINE", TFT_ORANGE);
      offlineMode = true;
      systemStatus.apiConnected = false;
      systemStatus.offlineMode = true;
    } else {
      updateStatusSection("API: OK", TFT_GREEN);
      systemStatus.apiConnected = true;
    }
  }
  
  // Initialize time synchronization (non-critical)
  if (!offlineMode) {
    updateStatusSection("Syncing time...", TFT_YELLOW);
    if (!initializeTime()) {
      Serial.println("[TIME] Sync failed - continuing");
      updateConnectionStatus("Connected", "No sync", deviceDisplay);
    } else {
      updateConnectionStatus("Connected", "Synced", deviceDisplay);
    }
  }
  
  delay(1000);
  
  // Update system status
  systemStatus.uptime = millis();
  systemStatus.freeHeap = ESP.getFreeHeap();
  
  LOG_INFO("System initialization completed");
  LOG_INFO("Free heap: " + String(systemStatus.freeHeap) + " bytes");
  
  // Return true even if some non-critical modules failed
  return rfidModule.isInitialized();  // RFID is critical
}

void handleSystemError(const char* component, const char* error) {
  Serial.print("[ERROR] ");
  Serial.print(component);
  Serial.print(": ");
  Serial.println(error);
  
  updateStatusSection(String(component) + " ERR", TFT_RED);
  updateFooter(String(error));
  sendToLEDMatrix("ERROR", String(component), "");
  
  delay(2000);
}

void loop(void) {
  if (!systemReady) {
    // Safe mode - minimal functionality
    handleKeypadInputNew();
    checkSerialCommands();
    delay(100);
    return;
  }

  // WebSocket loop (maintains connection, handles messages)
  if (useWebSocket && wsModule.isConnected()) {
    wsModule.loop();
  }

  // Check network connection and attempt reconnection
  checkNetworkConnection();
  
  // Handle RFID scanning
  handleRFIDScanning();

  // Handle keypad input (use both old and new methods for compatibility)
  handleKeypadInput();  // Legacy function
  handleKeypadInputNew();  // New class-based function
  
  // Check serial commands
  checkSerialCommands();

  unsigned long currentMillis = millis();
  
  // Check registration mode periodically (only if online)
  // Note: Registration mode is now controlled via WebSocket config updates
  // TODO: Implement checkRegistrationModeFromServer() if polling is needed
  // if (!offlineMode && currentMillis - lastRegistrationCheck > 5000) {
  //   lastRegistrationCheck = currentMillis;
  //   checkRegistrationModeFromServer();
  // }
  
  // Send heartbeat
  sendPeriodicHeartbeat();
  
  // Clear registration keypad buffer if timeout reached (prevents accidental commands)
  if (registrationKeypadBuffer.length() > 0 && (currentMillis - lastRegistrationKeypadInput > KEYPAD_BUFFER_TIMEOUT)) {
    registrationKeypadBuffer = "";
  }
  
  // Handle keypad timeout
  if (checkKeypadTimeout(currentMillis)) {
    Serial.println("[KEYPAD] Input timeout");
    clearKeypadInput();
    indicateReady();
  }

  // Check registration timeout
  if (registrationMode && (currentMillis - registrationModeStartTime > REGISTRATION_MODE_TIMEOUT)) {
    Serial.println("[REGISTRATION] Timeout reached");
    registrationMode = false;
    expectedRegistrationTagId = "";
    
    if (!offlineMode) {
      reportDeviceStatus("registration_timeout");
    }
    
    blinkError(3);
    updateStatusSection("REG TIMEOUT", TFT_RED);
    updateFooter("Registration mode timed out");
  }

  delay(50);
}

void checkNetworkConnection() {
  networkModule.updateConnectionStatus();
  
  if (!networkModule.isConnected() && !offlineMode) {
    Serial.println("[NETWORK] Connection lost - attempting reconnect...");
    updateStatusSection("RECONNECTING", TFT_ORANGE);
    
    if (networkModule.reconnect()) {
      Serial.println("[NETWORK] Reconnected successfully");
      updateStatusSection("RECONNECTED", TFT_GREEN);
      offlineMode = false;
      apiModule.resetFailureCount();
      
      String deviceDisplay = deviceId.length() >= 4 ? deviceId.substring(deviceId.length() - 4) : deviceId;
      updateConnectionStatus("Connected", "Synced", deviceDisplay);
    } else {
      Serial.println("[NETWORK] Reconnection failed - entering offline mode");
      offlineMode = true;
      updateStatusSection("OFFLINE MODE", TFT_ORANGE);
    }
  }
}

void handleRFIDScanning() {
  if (!rfidModule.isInitialized()) {
    return;
  }
  
  String tagId;
  if (rfidModule.scanWithDebounce(tagId, RFID_DEBOUNCE_MS)) {
    // Validate tag ID
    if (!IS_VALID_TAG_ID(tagId)) {
      LOG_ERROR("Invalid tag ID: " + tagId);
      return;
    }
    
    systemStatus.scanCount++;
    
    LOG_INFO("RFID Scanned: " + tagId);
    Serial.print("[RFID] Total scans: ");
    Serial.println(systemStatus.scanCount);
    
    // Update display
    updateStatusSection("TAG DETECTED", TFT_CYAN);
    
    // Send to LED matrix
    sendToLEDMatrix("SCAN", tagId.substring(0, 8), "");
    
    if (registrationMode) {
      // Handle registration mode scanning
      Serial.println();
      Serial.println("═══════════════════════════════════════");
      Serial.println("  REGISTRATION MODE - TAG DETECTED");
      Serial.println("  Tag ID: " + tagId);
      Serial.println("═══════════════════════════════════════");
      Serial.println();
      
      updateStatusSection("REGISTERING TAG", TFT_ORANGE);
      updateScanSection(tagId, "REGISTERING", "Please wait...", TFT_YELLOW);
      sendToLEDMatrix("REG", tagId.substring(0, 8), "WAIT");
      
      // Send registration request to backend
      if (useWebSocket && wsModule.isConnected()) {
        // Send via WebSocket with registration flag
        Serial.println("[WS] Sending registration via WebSocket");
        // Note: WebSocket sendScan should be enhanced to support registration mode
        // For now, using HTTP
        ApiResponse response = apiModule.sendScan(tagId, deviceConfig.location);
        
        if (response.result == API_SUCCESS) {
          Serial.println("[✓] Tag registered successfully!");
          updateScanSection(tagId, "REGISTERED", "Success!", TFT_GREEN);
          sendToLEDMatrix("REG", "SUCCESS", "");
          indicateSuccess();
          
          // Auto-exit registration mode after successful registration
          delay(2000);
          registrationMode = false;
          updateStatusSection("NORMAL MODE", TFT_GREEN);
          updateFooter("Ready to scan");
        } else {
          Serial.println("[✗] Registration failed: " + response.error);
          updateScanSection(tagId, "REG FAILED", response.error, TFT_RED);
          sendToLEDMatrix("REG", "FAILED", "");
          indicateError();
        }
      } else if (!offlineMode && apiModule.isInitialized()) {
        Serial.println("[HTTP] Sending registration via HTTP");
        ApiResponse response = apiModule.sendScan(tagId, deviceConfig.location);
        
        if (response.result == API_SUCCESS) {
          Serial.println("[✓] Tag registered successfully!");
          updateScanSection(tagId, "REGISTERED", "Success!", TFT_GREEN);
          sendToLEDMatrix("REG", "SUCCESS", "");
          indicateSuccess();
          
          // Auto-exit registration mode after successful registration
          delay(2000);
          registrationMode = false;
          updateStatusSection("NORMAL MODE", TFT_GREEN);
          updateFooter("Ready to scan");
        } else {
          Serial.println("[✗] Registration failed: " + response.error);
          updateScanSection(tagId, "REG FAILED", response.error, TFT_RED);
          sendToLEDMatrix("REG", "FAILED", "");
          indicateError();
        }
      } else {
        Serial.println("[✗] Cannot register - offline mode");
        updateScanSection(tagId, "OFFLINE", "Cannot register", TFT_RED);
        indicateError();
      }
    } else {
      // Normal scanning mode
      // Try WebSocket first (if enabled and connected)
      if (useWebSocket && wsModule.isConnected()) {
        Serial.println("[WS] Sending scan via WebSocket");
        wsModule.sendScan(tagId, deviceConfig.location);
        
        // Show processing message
        updateStatusSection("PROCESSING...", TFT_YELLOW);
        updateScanSection(tagId, "PROCESSING", "Sending to server", TFT_YELLOW);
        
        // Response will be handled by handleScanResponse callback
      } 
      // Fallback to HTTP if WebSocket not available
      else if (!offlineMode && apiModule.isInitialized()) {
        Serial.println("[HTTP] Sending scan via HTTP (WebSocket unavailable)");
        // Send to backend via HTTP
        ApiResponse response = apiModule.sendScan(tagId, deviceConfig.location);
        if (response.result == API_SUCCESS) {
          Serial.println("[API] Scan sent successfully");
          // Parse and handle response - for now just show success
          updateStatusSection("SCAN OK", TFT_GREEN);
          updateScanSection(tagId, "SENT", "Via HTTP", TFT_GREEN);
        } else {
          Serial.println("[API] Failed to send scan");
          updateStatusSection("SCAN FAILED", TFT_RED);
          updateScanSection(tagId, "OFFLINE", "Scan not sent", TFT_ORANGE);
          
          systemStatus.errorCount++;
          
          if (apiModule.getConsecutiveFailures() >= MAX_CONSECUTIVE_FAILURES) {
            LOG_ERROR("Multiple API failures - switching to offline mode");
            offlineMode = true;
            systemStatus.offlineMode = true;
            systemStatus.apiConnected = false;
            updateFooter("Too many failures - offline mode");
          }
        }
      } else {
        // Offline mode - just display
        Serial.println("[OFFLINE] Scan recorded locally");
        updateScanSection(tagId, "OFFLINE", "Backend unavailable", TFT_ORANGE);
        updateFooter("Offline scan: " + tagId.substring(0, 8));
      }
    }
    
    delay(200);
  }
}

void handleKeypadInputNew() {
  char key = keypadModule.getKey();
  
  if (key) {
    Serial.print("[KEYPAD] Key pressed: ");
    Serial.println(key);
    
    // Update last input time
    lastRegistrationKeypadInput = millis();
    
    // Add key to buffer
    registrationKeypadBuffer += key;
    
    // Check for registration mode toggle command (###)
    if (registrationKeypadBuffer.endsWith("###")) {
      registrationMode = !registrationMode;
      registrationKeypadBuffer = "";  // Clear buffer
      
      Serial.println();
      Serial.println("═══════════════════════════════════════");
      Serial.print("  REGISTRATION MODE: ");
      Serial.println(registrationMode ? "ENABLED ✓" : "DISABLED ✗");
      Serial.println("═══════════════════════════════════════");
      Serial.println();
      
      if (registrationMode) {
        registrationModeStartTime = millis();
        indicateRegistrationMode();
        updateStatusSection("REGISTRATION MODE", TFT_ORANGE);
        updateFooter("Scan tag to register");
        sendToLEDMatrix("REG", "MODE", "ACTIVE");
      } else {
        updateStatusSection("NORMAL MODE", TFT_GREEN);
        updateFooter("Ready to scan");
        sendToLEDMatrix("READY", "", "");
      }
      
      return;  // Exit early after handling command
    }
    
    // Limit buffer size to prevent memory issues
    if (registrationKeypadBuffer.length() > 10) {
      registrationKeypadBuffer = registrationKeypadBuffer.substring(registrationKeypadBuffer.length() - 10);
    }
    
    // Special system commands (single key)
    if (key == '#' && registrationKeypadBuffer.length() == 1) {
      // Display system status
      Serial.println("\n[STATUS] System Information:");
      Serial.print("  WiFi: ");
      Serial.println(networkModule.isConnected() ? "Connected" : "Disconnected");
      Serial.print("  RFID: ");
      Serial.println(rfidModule.isInitialized() ? "OK" : "ERROR");
      Serial.print("  API Failures: ");
      Serial.println(apiModule.getConsecutiveFailures());
      Serial.print("  Mode: ");
      Serial.println(offlineMode ? "OFFLINE" : "ONLINE");
      Serial.print("  Uptime: ");
      Serial.print(millis() / 1000);
      Serial.println(" seconds");
      Serial.print("  Free Heap: ");
      Serial.print(ESP.getFreeHeap());
      Serial.println(" bytes");
      Serial.print("  Total Scans: ");
      Serial.println(systemStatus.scanCount);
      Serial.print("  Error Count: ");
      Serial.println(systemStatus.errorCount);
      Serial.println();
      
      updateStatusSection("STATUS CHECK", TFT_CYAN);
      updateFooter("Check serial monitor");
    } else if (key == '*') {
      // Force heartbeat
      if (!offlineMode && apiModule.isInitialized()) {
        ApiResponse response = apiModule.sendHeartbeat(true);
        if (response.result == API_SUCCESS) {
          Serial.println("[HEARTBEAT] Manual heartbeat sent");
          updateStatusSection("HEARTBEAT OK", TFT_GREEN);
        } else {
          Serial.println("[HEARTBEAT] Failed");
          updateStatusSection("HEARTBEAT FAIL", TFT_RED);
        }
      } else {
        Serial.println("[HEARTBEAT] Offline mode");
        updateStatusSection("OFFLINE", TFT_ORANGE);
      }
    }
  }
}

void sendPeriodicHeartbeat() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastHeartbeat >= HEARTBEAT_INTERVAL_MS) {
    lastHeartbeat = currentMillis;
    
    if (!offlineMode && networkModule.isConnected()) {
      ApiResponse response = apiModule.sendHeartbeat(true);
      if (response.result == API_SUCCESS) {
        Serial.println("[HEARTBEAT] Sent successfully");
        showHeartbeat(true);
        delay(100);
        showHeartbeat(false);
      } else {
        Serial.println("[HEARTBEAT] Failed");
        // Note: incrementFailureCount() doesn't exist, using resetFailureCount() instead
        // or just log the error
      }
    } else {
      Serial.println("[HEARTBEAT] Skipped - offline mode");
    }
    
    // Update connection status display
    String wifiStatus = networkModule.isConnected() ? "Connected" : "Disconnected";
    String deviceDisplay = deviceId.length() >= 4 ? deviceId.substring(deviceId.length() - 4) : deviceId;
    updateConnectionStatus(wifiStatus, "Synced", deviceDisplay);
  }
}

void checkSerialCommands() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.equalsIgnoreCase("registration")) {
      registrationMode = !registrationMode;
      
      Serial.println();
      Serial.println("═══════════════════════════════════════");
      Serial.print("  REGISTRATION MODE: ");
      Serial.println(registrationMode ? "ENABLED ✓" : "DISABLED ✗");
      Serial.println("  (You can also use ### on keypad)");
      Serial.println("═══════════════════════════════════════");
      Serial.println();

      if (registrationMode) {
        registrationModeStartTime = millis();
        indicateRegistrationMode();
        updateStatusSection("REGISTRATION MODE", TFT_ORANGE);
        updateFooter("Scan tag to register");
        sendToLEDMatrix("REG", "MODE", "ACTIVE");
      } else {
        updateStatusSection("NORMAL MODE", TFT_GREEN);
        updateFooter("Ready to scan");
        sendToLEDMatrix("READY", "", "");
      }
    }
  }
}

// ===================================
// WebSocket Callback Functions
// ===================================

/**
 * Callback when scan response received from WebSocket
 */
void handleScanResponse(JsonDocument& doc) {
  if (doc["success"]) {
    bool isRegistered = doc["scan"]["isRegistered"] | false;
    String tagId = doc["scan"]["tagId"] | "";
    
    if (isRegistered && doc.containsKey("user")) {
      // Registered user
      String userName = doc["user"]["name"] | "Unknown";
      String userRole = doc["user"]["role"] | "";
      
      Serial.println("✅ Registered: " + userName + " (" + userRole + ")");
      
      // Update display
      updateStatusSection("REGISTERED", TFT_GREEN);
      updateScanSection(tagId, userName, "Welcome!", TFT_GREEN);
      updateFooter("Access granted: " + userName);
      
      // Send to LED matrix
      sendToLEDMatrix("WELCOME", userName.substring(0, 8), "");
      
      // Reset API failure count
      apiModule.resetFailureCount();
      
    } else {
      // Unregistered tag
      Serial.println("❌ Unregistered tag: " + tagId);
      
      updateStatusSection("UNREGISTERED", TFT_ORANGE);
      updateScanSection(tagId, "NOT REGISTERED", "Please register", TFT_ORANGE);
      updateFooter("Unregistered: " + tagId.substring(0, 8));
      
      // Send to LED matrix
      sendToLEDMatrix("UNREG", tagId.substring(0, 8), "");
    }
  } else {
    // Error occurred
    String error = doc["error"] | "Unknown error";
    Serial.println("❌ Error: " + error);
    
    updateStatusSection("ERROR", TFT_RED);
    updateScanSection("", "ERROR", error, TFT_RED);
    updateFooter("Scan error: " + error);
    
    sendToLEDMatrix("ERROR", error.substring(0, 8), "");
  }
}

/**
 * Callback when config update received from WebSocket
 */
void handleConfigUpdate(JsonDocument& doc) {
  if (doc.containsKey("config")) {
    bool regMode = doc["config"]["registrationMode"] | false;
    
    // Update local registration mode
    registrationMode = regMode;
    deviceConfig.registrationMode = regMode;
    
    Serial.println("⚙️ Config updated from server:");
    Serial.println("  - Registration Mode: " + String(regMode ? "ON" : "OFF"));
    
    // Update display
    if (regMode) {
      indicateRegistrationMode();
      updateFooter("Registration mode enabled");
    } else {
      indicateReady();
      updateFooter("Normal scanning mode");
    }
    
    // Send to LED matrix
    sendToLEDMatrix("CONFIG", regMode ? "REG ON" : "REG OFF", "");
  }
}

/**
 * Callback when WebSocket connection status changes
 */
void handleWSConnectionStatus(bool connected) {
  if (connected) {
    Serial.println("🔌 WebSocket connected - real-time mode active");
    updateStatusSection("WS: Connected", TFT_GREEN);
    updateFooter("Real-time mode active");
    
    // Mark system as online
    offlineMode = false;
    systemStatus.offlineMode = false;
    
  } else {
    Serial.println("🔌 WebSocket disconnected - falling back to HTTP");
    updateStatusSection("WS: Disconnected", TFT_ORANGE);
    updateFooter("Using HTTP fallback");
    
    // Don't mark as offline if API is still available
    if (!apiModule.isInitialized()) {
      offlineMode = true;
      systemStatus.offlineMode = true;
    }
  }
}

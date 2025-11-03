#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =======================
// Phase 1 Optimizations
// =======================
// Disable assertion strings to save flash (~7 KB)
#define NDEBUG  // Disable assert() completely

// =======================
// ESP32 Pin Assignments
// =======================

// PN532 HSPI pins (TFT uses VSPI)
#define PN532_SCK 14
#define PN532_MISO 12
#define PN532_MOSI 13
#define PN532_SS 27

// UART Communication to LED Matrix ESP32
#define UART_TX 17
#define UART_RX 16
#define UART_BAUD 115200

// 4x4 Keypad Matrix Pins
#define KEYPAD_ROWS 4
#define KEYPAD_COLS 4

// =======================
// TFT Color Definitions
// =======================

#define TFT_ORANGE 0xFD20
#define TFT_DARKGREEN 0x03E0
#define TFT_NAVY 0x000F
#define TFT_DARKGREY 0x7BEF
#define TFT_LIGHTGREY 0xC618
#define TFT_PURPLE 0x780F

// =======================
// Display Layout Constants
// =======================

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

#define HEADER_HEIGHT 35
#define STATUS_SECTION_Y 40
#define STATUS_SECTION_HEIGHT 70
#define SCAN_SECTION_Y 115
#define SCAN_SECTION_HEIGHT 80
#define FOOTER_Y 200
#define FOOTER_HEIGHT 40

#define LEFT_MARGIN 5
#define RIGHT_MARGIN 315
#define CENTER_X 160

// =======================
// Timing Constants
// =======================

#define HEARTBEAT_INTERVAL 30000  // 30 seconds
#define REGISTRATION_MODE_TIMEOUT 120000  // 2 minutes
#define KEY_INPUT_TIMEOUT 5000  // 5 seconds
#define TEST_MODE_TIMEOUT 10000  // 10 seconds
#define RFID_DEBOUNCE_MS 1500  // 1.5 seconds
#define KEYPAD_DEBOUNCE_MS 300  // 300ms
#define HEARTBEAT_INTERVAL_MS 60000  // 1 minute
#define MENU_REMINDER_INTERVAL 30000  // 30 seconds

// =======================
// API Configuration
// =======================

// PRODUCTION Configuration (uses custom domain api.tagsakay.com)
#ifndef WS_HOST
  #define WS_HOST "api.tagsakay.com"  // Production API domain
#endif

#ifndef WS_PORT
  #define WS_PORT 443  // HTTPS/WSS standard port
#endif

#ifndef WS_PATH
  #define WS_PATH "/ws/device"  // WebSocket endpoint
#endif

#define WS_RECONNECT_INTERVAL 5000   // Reconnect every 5 seconds if disconnected
#define WS_PING_INTERVAL 30000       // Send heartbeat every 30 seconds
#define WS_ENABLED true              // Enable WebSocket (set false to use HTTP only)
#define USE_SECURE_WS true           // Production uses HTTPS/WSS (secure WebSocket)

// HTTP endpoint (fallback when WebSocket unavailable)
#ifndef API_BASE_URL
  #define API_BASE_URL "https://api.tagsakay.com"  // Production API URL
#endif

#ifndef API_DEFAULT_KEY
  #define API_DEFAULT_KEY ""  // Set your default API key here (or configure via Serial)
#endif

#define API_TIMEOUT_MS 5000
#define API_RETRY_ATTEMPTS 3
#define MAX_CONSECUTIVE_FAILURES 5

// DEVELOPMENT Configuration (uncomment for local testing)
// Comment out production config above and uncomment these lines:
// #define WS_HOST "192.168.1.100"  // Replace with your local dev machine IP
// #define WS_PORT 8787              // Local Cloudflare Workers dev port
// #define USE_SECURE_WS false       // Local uses HTTP/WS (non-secure)
// #define API_BASE_URL "http://192.168.1.100:8787"  // Local dev URL

// =======================
// Network Configuration
// =======================

#define WIFI_RECONNECT_INTERVAL 30000  // 30 seconds
#define MAX_WIFI_RECONNECT_ATTEMPTS 10

// =======================
// RFID Configuration
// =======================

#define RFID_RETRY_ATTEMPTS 3
#define RFID_SCAN_TIMEOUT 50  // milliseconds

// =======================
// Error Codes
// =======================

#define ERROR_RFID_INIT_FAILED -1
#define ERROR_NETWORK_FAILED -2
#define ERROR_API_FAILED -3
#define ERROR_DISPLAY_FAILED -4

// =======================
// Test Mode Configuration
// =======================

// Alternative pin configuration for testing
#define ALT_KEYPAD_ROW_PINS {5, 19, 21, 22}
#define ALT_KEYPAD_COL_PINS {25, 26, 32, 33}

// =======================
// Device Configuration
// =======================

#define DEVICE_NAME "TagSakay Scanner"
#define DEVICE_VERSION "2.0"
#define FIRMWARE_VERSION "2.0.0"

// =======================
// Memory & Performance
// =======================

#define LOW_MEMORY_THRESHOLD 10000  // bytes
#define MAX_SCAN_QUEUE_SIZE 100  // offline scan buffer
#define STATUS_REPORT_INTERVAL 300000  // 5 minutes
#define WATCHDOG_TIMEOUT 30000  // 30 seconds

// =======================
// LED Matrix Configuration
// =======================

#define LED_BRIGHTNESS_DEFAULT 100
#define LED_DISPLAY_DURATION 3000  // 3 seconds
#define LED_SCROLL_SPEED 50  // milliseconds per frame

// =======================
// TFT Display Configuration
// =======================

#define DISPLAY_TIMEOUT 300000  // 5 minutes (0 = never)
#define DISPLAY_BRIGHTNESS 255  // 0-255
#define SCREEN_SAVER_ENABLE false

// =======================
// Scan Configuration
// =======================

#define MIN_SCAN_INTERVAL 1000  // minimum 1 second between scans
#define MAX_TAG_ID_LENGTH 16
#define DUPLICATE_SCAN_WINDOW 3000  // 3 seconds

// =======================
// Queue System Configuration
// =======================

#define MAX_QUEUE_NUMBER 999
#define QUEUE_NUMBER_TIMEOUT 86400000  // 24 hours

// =======================
// Logging Configuration
// =======================

#define SERIAL_BAUD_RATE 115200
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO 1
#define LOG_LEVEL_WARNING 2
#define LOG_LEVEL_ERROR 3
// Phase 1 Optimization: Reduced from LOG_LEVEL_INFO to LOG_LEVEL_ERROR (saves ~25 KB)
#define CURRENT_LOG_LEVEL LOG_LEVEL_ERROR

// =======================
// Feature Flags
// =======================

#define FEATURE_OFFLINE_MODE true
#define FEATURE_AUTO_RECONNECT true
#define FEATURE_LOCAL_STORAGE false  // SPIFFS/LittleFS not implemented yet
#define FEATURE_OTA_UPDATE false  // OTA not implemented yet
#define FEATURE_KEYPAD_MENU true
#define FEATURE_TEST_MODE true
#define FEATURE_LED_MATRIX true

// =======================
// Configuration Structures
// =======================

struct WiFiConfig {
  const char* ssid;
  const char* password;
  int maxRetries;
  unsigned long retryDelay;
};

struct ServerConfig {
  const char* baseUrl;
  const char* apiKey;
  int timeout;
  const char* deviceLocation;
};

struct NTPConfig {
  const char* ntpServer;
  long gmtOffset_sec;
  int daylightOffset_sec;
};

struct DeviceConfig {
  String name;
  String location;
  String version;
  bool registrationMode;
  bool scanMode;
  int ledBrightness;
  unsigned long scanInterval;
};

struct SystemStatus {
  bool wifiConnected;
  bool rfidInitialized;
  bool apiConnected;
  bool offlineMode;
  unsigned long uptime;
  uint32_t freeHeap;
  int scanCount;
  int errorCount;
  unsigned long lastHeartbeat;
};

// =======================
// Global Configuration Instances
// =======================

extern WiFiConfig wifiConfig;
extern ServerConfig serverConfig;
extern NTPConfig ntpConfig;
extern DeviceConfig deviceConfig;
extern SystemStatus systemStatus;

// =======================
// Global State Variables
// =======================

extern String deviceId;
extern String lastScannedTag;
extern bool registrationMode;
extern String expectedRegistrationTagId;
extern unsigned long lastRegistrationCheck;
extern unsigned long registrationModeStartTime;
extern unsigned long lastHeartbeat;
extern unsigned long lastScanTime;

// =======================
// Utility Macros
// =======================

// Logging macros
#define LOG_DEBUG(msg) if(CURRENT_LOG_LEVEL <= LOG_LEVEL_DEBUG) { Serial.print("[DEBUG] "); Serial.println(msg); }
#define LOG_INFO(msg) if(CURRENT_LOG_LEVEL <= LOG_LEVEL_INFO) { Serial.print("[INFO] "); Serial.println(msg); }
#define LOG_WARNING(msg) if(CURRENT_LOG_LEVEL <= LOG_LEVEL_WARNING) { Serial.print("[WARNING] "); Serial.println(msg); }
#define LOG_ERROR(msg) if(CURRENT_LOG_LEVEL <= LOG_LEVEL_ERROR) { Serial.print("[ERROR] "); Serial.println(msg); }

// Memory check macro
#define CHECK_MEMORY() (ESP.getFreeHeap() > LOW_MEMORY_THRESHOLD)

// Timing helpers
#define MILLIS_OVERFLOW_SAFE(current, previous) ((current >= previous) ? (current - previous) : (ULONG_MAX - previous + current))

// Feature check macros
#define IS_FEATURE_ENABLED(feature) (feature == true)

// Validation macros
#define IS_VALID_QUEUE_NUMBER(num) ((num > 0) && (num <= MAX_QUEUE_NUMBER))
#define IS_VALID_TAG_ID(id) ((id.length() > 0) && (id.length() <= MAX_TAG_ID_LENGTH))

#endif // CONFIG_H
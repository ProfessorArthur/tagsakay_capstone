#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Keypad.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_PN532.h>
#include <ArduinoJson.h>
#include "Config.h"

TFT_eSPI tft = TFT_eSPI();

// RFID Reader (PN532 on HSPI)
SPIClass hspi(HSPI);
Adafruit_PN532 nfc(PN532_SS, &hspi);

// WiFi credentials (edit in Config.h or here)
const char* WIFI_SSID = "MyQueen";
const char* WIFI_PASSWORD = "ThaiQueen07";

// API Configuration
const char* API_BASE_URL = "https://api.tagsakay.com";
const char* API_KEY = "tsk_EWFS5d43StQY1TgFmusJ2aNPyWnjPcER";  // Set your device API key here

// System state
bool wifiConnected = false;
bool rfidInitialized = false;
bool apiConnected = false;
bool registrationMode = false;
bool deviceActive = false;
bool scanModeEnabled = false;
String serverLastSeen = "";
String deviceMac = "";
String expectedRegistrationTagId = "";
unsigned long registrationModeStartTime = 0;
int rfidScanCount = 0;
String lastRfidTag = "";
unsigned long lastScanTime = 0;
unsigned long lastCommandPoll = 0;
unsigned long lastKeyTime = 0;
char lastProcessedKey = 0;
const unsigned long KEY_DEBOUNCE_MS = 500;

int apiRequestCount = 0;
int apiHealthChecks = 0;
int apiHeartbeats = 0;
int commandPollCount = 0;
String lastApiResponse = "";
int lastApiHttpCode = 0;
int lastHealthHttpCode = 0;
int lastHeartbeatHttpCode = 0;
unsigned long lastHeartbeatTime = 0;
int lastCommandHttpCode = 0;


byte keypadRowPins[KEYPAD_ROWS] = {25, 26, 32, 33};
byte keypadColPins[KEYPAD_COLS] = {5, 19, 21, 22};
char keypadKeys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

Keypad keypad = Keypad(makeKeymap(keypadKeys), keypadRowPins, keypadColPins, KEYPAD_ROWS, KEYPAD_COLS);

enum TestStage : uint8_t {
  STAGE_BOOT = 0,
  STAGE_RED,
  STAGE_GREEN,
  STAGE_BLUE,
  STAGE_WHITE,
  STAGE_BLACK,
  STAGE_COLOR_BARS,
  STAGE_GRADIENT,
  STAGE_GRID,
  STAGE_TEXT,
  STAGE_PIXELS,
  STAGE_WIFI_TEST,
  STAGE_RFID_TEST,
  STAGE_API_TEST,
  STAGE_REGISTRATION_TEST,
  STAGE_FULL_SYSTEM,
  STAGE_COUNT
};

struct StageLabel {
  TestStage stage;
  const char *label;
};

const StageLabel stageLabels[] = {
  {STAGE_BOOT, "Boot/reference"},
  {STAGE_RED, "Solid red"},
  {STAGE_GREEN, "Solid green"},
  {STAGE_BLUE, "Solid blue"},
  {STAGE_WHITE, "Solid white"},
  {STAGE_BLACK, "Solid black"},
  {STAGE_COLOR_BARS, "Color bars"},
  {STAGE_GRADIENT, "Greyscale gradient"},
  {STAGE_GRID, "Geometry grid"},
  {STAGE_TEXT, "Font sharpness"},
  {STAGE_PIXELS, "Pixel focus"},
  {STAGE_WIFI_TEST, "WiFi test"},
  {STAGE_RFID_TEST, "RFID test"},
  {STAGE_API_TEST, "API test"},
  {STAGE_REGISTRATION_TEST, "Registration & poll"},
  {STAGE_FULL_SYSTEM, "Full system"}
};

constexpr uint32_t AUTO_ADVANCE_MS = 3500;

const int MENU_PANEL_WIDTH = 160;
const int MENU_PANEL_MARGIN = 8;
const int MENU_PANEL_X = SCREEN_WIDTH - MENU_PANEL_WIDTH - MENU_PANEL_MARGIN;
const int MENU_PANEL_Y = 36;
const int MENU_PANEL_HEIGHT = FOOTER_Y - MENU_PANEL_Y - 8;

bool menuVisible = false;
bool menuActive = false;
char lastKeyPressed = 0;
String footerMessage = "";

TestStage currentStage = STAGE_BOOT;
unsigned long lastStageChange = 0;
bool autoAdvance = true;

void printIntro();
void drawStage(TestStage stage);
void drawBootStage();
void drawSolid(uint16_t color, const char *label);
void drawColorBars();
void drawGradient();
void drawGrid();
void drawTextDemo();
void drawPixelTargets();
void runPanelDiagnostics();
void showStageLabel(TestStage stage);
void nextStage();
void previousStage();
void handleKeypad();
void handleMenuSelection(char key);
void renderMenuPanel();
void setFooterMessage(const String& msg);
void setFooterMessage(const __FlashStringHelper* msg);
void redrawFooter();
bool initializeWiFi();
bool initializeRFID();
void drawWiFiTest();
void drawRFIDTest();
void drawAPITest();
void drawRegistrationTest();
void drawFullSystemTest();
void handleRFIDScanning();
bool testAPIHealth();
bool testAPIHeartbeat();
bool sendRFIDScanToAPI(const String& tagId);
void initializePolling();
void pollCommands();

void setup() {
  Serial.begin(115200);
  delay(500);
  printIntro();

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);

  // Initialize WiFi and get MAC address
  WiFi.mode(WIFI_STA);
  delay(1000);  // Allow WiFi to initialize before accessing MAC
  deviceMac = WiFi.macAddress();
  deviceMac.replace(":", "");
  Serial.print("[NETWORK] Device MAC: ");
  Serial.println(deviceMac);

  // Initialize RFID with HSPI
  Serial.println("[RFID] Initializing PN532...");
  hspi.begin(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
  nfc.begin();
  
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (versiondata) {
    Serial.print("[RFID] Found PN53x chip, version: ");
    Serial.println(versiondata, HEX);
    Serial.print("[RFID] Firmware v");
    Serial.print((versiondata >> 16) & 0xFF, DEC);
    Serial.print(".");
    Serial.println((versiondata >> 8) & 0xFF, DEC);
    
    // Configure SAM
    if (nfc.SAMConfig()) {
      rfidInitialized = true;
      Serial.println("[RFID] SAMConfig successful");
    } else {
      Serial.println("[RFID] SAMConfig failed!");
      rfidInitialized = false;
    }
  } else {
    Serial.println("[RFID] PN532 not detected!");
    rfidInitialized = false;
  }

  runPanelDiagnostics();
  
  setFooterMessage(F("Auto advance enabled (press A for menu)"));
  drawStage(currentStage);
  lastStageChange = millis();
}

void loop() {
  handleKeypad();
  
  // RFID scanning in RFID test, registration test, and full system modes
  if ((currentStage == STAGE_RFID_TEST || currentStage == STAGE_REGISTRATION_TEST || currentStage == STAGE_FULL_SYSTEM) && rfidInitialized) {
    handleRFIDScanning();
  }
  
  // Registration mode timeout
  if (registrationMode && (millis() - registrationModeStartTime > REGISTRATION_MODE_TIMEOUT)) {
    Serial.println("[REGISTRATION] Timeout - disabling registration mode");
    registrationMode = false;
    expectedRegistrationTagId = "";
    if (currentStage == STAGE_REGISTRATION_TEST) {
      drawStage(currentStage);
    }
  }

  const unsigned long now = millis();
  if (autoAdvance && now - lastStageChange >= AUTO_ADVANCE_MS) {
    nextStage();
    lastStageChange = now;
  }

  if (Serial.available() > 0) {
    const char command = Serial.read();
    switch (command) {
      case 'n':
      case 'N':
        autoAdvance = false;
        setFooterMessage(F("Auto advance paused"));
        nextStage();
        lastStageChange = millis();
        break;
      case 'p':
      case 'P':
        autoAdvance = false;
        setFooterMessage(F("Auto advance paused"));
        previousStage();
        lastStageChange = millis();
        break;
      case 'a':
      case 'A':
        autoAdvance = !autoAdvance;
        Serial.print("[DIAG] Auto-advance ");
        Serial.println(autoAdvance ? "enabled" : "paused");
        setFooterMessage(autoAdvance ? F("Auto advance enabled") : F("Auto advance paused"));
        break;
      case 'r':
      case 'R':
        currentStage = STAGE_BOOT;
        drawStage(currentStage);
        lastStageChange = millis();
        Serial.println("[DIAG] Restarted stage cycle");
        break;
      default:
        break;
    }
  }
}

void printIntro() {
  Serial.println();
  Serial.println("========================================");
  Serial.println("  TagSakay TFT Diagnostic Utility");
  Serial.println("========================================");
  Serial.println("Commands:");
  Serial.println("  n - next stage");
  Serial.println("  p - previous stage");
  Serial.println("  a - toggle auto advance");
  Serial.println("  r - restart from stage 0");
  Serial.println();
}

void runPanelDiagnostics() {
  struct Probe { uint8_t command; const char *label; };
  const Probe probes[] = {
    {0x04, "Display status"},
    {0x09, "Display power"},
    {0x0A, "Pixel format"},
    {0x0C, "Display ID 1"},
    {0x0D, "Display ID 2"},
    {0x0E, "Display ID 3"}
  };

  Serial.println("[DIAG] Controller readback start");
  for (const auto &probe : probes) {
    uint8_t value = tft.readcommand8(probe.command);
    Serial.print("  ");
    Serial.print(probe.label);
    Serial.print(" (0x");
    if (probe.command < 0x10) Serial.print('0');
    Serial.print(probe.command, HEX);
    Serial.print(") = 0x");
    if (value < 0x10) Serial.print('0');
    Serial.println(value, HEX);
  }
  Serial.println("[DIAG] Controller readback complete\n");
}

void drawStage(TestStage stage) {
  switch (stage) {
    case STAGE_BOOT:
      drawBootStage();
      break;
    case STAGE_RED:
      drawSolid(TFT_RED, "RED");
      break;
    case STAGE_GREEN:
      drawSolid(TFT_GREEN, "GREEN");
      break;
    case STAGE_BLUE:
      drawSolid(TFT_BLUE, "BLUE");
      break;
    case STAGE_WHITE:
      drawSolid(TFT_WHITE, "WHITE");
      break;
    case STAGE_BLACK:
      drawSolid(TFT_BLACK, "BLACK");
      break;
    case STAGE_COLOR_BARS:
      drawColorBars();
      break;
    case STAGE_GRADIENT:
      drawGradient();
      break;
    case STAGE_GRID:
      drawGrid();
      break;
    case STAGE_TEXT:
      drawTextDemo();
      break;
    case STAGE_PIXELS:
      drawPixelTargets();
      break;
    case STAGE_WIFI_TEST:
      drawWiFiTest();
      break;
    case STAGE_RFID_TEST:
      drawRFIDTest();
      break;
    case STAGE_API_TEST:
      drawAPITest();
      break;
    case STAGE_REGISTRATION_TEST:
      drawRegistrationTest();
      break;
    case STAGE_FULL_SYSTEM:
      drawFullSystemTest();
      break;
    default:
      break;
  }
  showStageLabel(stage);
}

void drawBootStage() {
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, TFT_NAVY);
  tft.setTextColor(TFT_YELLOW, TFT_NAVY);
  tft.setTextSize(2);
  tft.setCursor(LEFT_MARGIN, 12);
  tft.println("TagSakay Display Test");

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(LEFT_MARGIN, 70);
  tft.println("1. Confirm orientation");
  tft.setCursor(LEFT_MARGIN, 100);
  tft.println("2. Verify color fill");
  tft.setCursor(LEFT_MARGIN, 130);
  tft.println("3. Check borders");

  tft.drawRect(LEFT_MARGIN, STATUS_SECTION_Y, SCREEN_WIDTH - (LEFT_MARGIN * 2), STATUS_SECTION_HEIGHT, TFT_DARKGREY);
  tft.drawRect(LEFT_MARGIN, SCAN_SECTION_Y, SCREEN_WIDTH - (LEFT_MARGIN * 2), SCAN_SECTION_HEIGHT, TFT_DARKGREY);
  tft.drawLine(CENTER_X, HEADER_HEIGHT, CENTER_X, SCREEN_HEIGHT, TFT_DARKGREY);
  tft.drawLine(0, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT / 2, TFT_DARKGREY);

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(LEFT_MARGIN, FOOTER_Y + 6);
  tft.println("Use Serial console to step through tests.");
}

void drawSolid(uint16_t color, const char *label) {
  tft.fillScreen(color);
  tft.setTextColor((color == TFT_BLACK) ? TFT_WHITE : TFT_BLACK, color);
  tft.setTextSize(3);
  tft.setCursor(CENTER_X - 60, SCREEN_HEIGHT / 2 - 10);
  tft.println(label);
}

void drawColorBars() {
  const uint16_t palette[] = {
    TFT_RED, TFT_ORANGE, TFT_YELLOW, TFT_GREEN,
    TFT_CYAN, TFT_BLUE, TFT_PURPLE, TFT_WHITE
  };

  const int barWidth = SCREEN_WIDTH / static_cast<int>(sizeof(palette) / sizeof(palette[0]));
  for (size_t i = 0; i < sizeof(palette) / sizeof(palette[0]); ++i) {
    tft.fillRect(static_cast<int>(i) * barWidth, 0, barWidth, SCREEN_HEIGHT, palette[i]);
  }
}

void drawGradient() {
  for (int x = 0; x < SCREEN_WIDTH; ++x) {
    uint8_t level = map(x, 0, SCREEN_WIDTH - 1, 0, 255);
    uint16_t color = tft.color565(level, level, level);
    tft.drawFastVLine(x, 0, SCREEN_HEIGHT, color);
  }
}

void drawGrid() {
  tft.fillScreen(TFT_BLACK);
  tft.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, TFT_WHITE);

  for (int x = 0; x <= SCREEN_WIDTH; x += 40) {
    tft.drawFastVLine(x, 0, SCREEN_HEIGHT, TFT_DARKGREY);
  }
  for (int y = 0; y <= SCREEN_HEIGHT; y += 40) {
    tft.drawFastHLine(0, y, SCREEN_WIDTH, TFT_DARKGREY);
  }

  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  for (int x = 0; x <= SCREEN_WIDTH; x += 80) {
    for (int y = 0; y <= SCREEN_HEIGHT; y += 80) {
      tft.setCursor(x + 2, y + 2);
      tft.print(x);
      tft.print(",");
      tft.print(y);
    }
  }
}

void drawTextDemo() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  for (int size = 1; size <= 6; ++size) {
    tft.setTextSize(size);
    tft.setCursor(LEFT_MARGIN, 30 * size);
    tft.print("Size ");
    tft.print(size);
    tft.print(": ABC123");
  }

  tft.setTextSize(1);
  tft.setCursor(LEFT_MARGIN, SCREEN_HEIGHT - 40);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.println("Check for crisp edges and readable small fonts.");
}

void drawPixelTargets() {
  tft.fillScreen(TFT_BLACK);
  const int markerRadius = 4;

  const int positions[][2] = {
    {16, 16}, {SCREEN_WIDTH - 16, 16},
    {16, SCREEN_HEIGHT - 16}, {SCREEN_WIDTH - 16, SCREEN_HEIGHT - 16},
    {CENTER_X, SCREEN_HEIGHT / 2}
  };

  for (const auto &pos : positions) {
    tft.drawCircle(pos[0], pos[1], markerRadius, TFT_GREEN);
    tft.drawFastHLine(pos[0] - 10, pos[1], 20, TFT_RED);
    tft.drawFastVLine(pos[0], pos[1] - 10, 20, TFT_RED);
  }

  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, SCREEN_HEIGHT - 60);
  tft.println("Inspect for stuck pixels");
  tft.setCursor(LEFT_MARGIN, SCREEN_HEIGHT - 30);
  tft.println("Tap reset to rerun");
}

void showStageLabel(TestStage stage) {
  tft.fillRect(0, 0, SCREEN_WIDTH, 26, TFT_BLACK);
  tft.drawFastHLine(0, 26, SCREEN_WIDTH, TFT_DARKGREY);

  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 6);

  for (const auto &entry : stageLabels) {
    if (entry.stage == stage) {
      tft.print("Stage ");
      tft.print(static_cast<int>(stage));
      tft.print("/");
      tft.print(STAGE_COUNT - 1);
      tft.print(": ");
      tft.println(entry.label);
      break;
    }
  }

  if (menuVisible) {
    renderMenuPanel();
  }

  redrawFooter();
}

void nextStage() {
  currentStage = static_cast<TestStage>((static_cast<uint8_t>(currentStage) + 1) % STAGE_COUNT);
  drawStage(currentStage);

  Serial.print("[DIAG] Stage -> ");
  Serial.println(static_cast<int>(currentStage));
}

void previousStage() {
  uint8_t index = static_cast<uint8_t>(currentStage);
  currentStage = static_cast<TestStage>((index == 0) ? (STAGE_COUNT - 1) : (index - 1));
  drawStage(currentStage);

  Serial.print("[DIAG] Stage -> ");
  Serial.println(static_cast<int>(currentStage));
}

void handleKeypad() {
  char key = keypad.getKey();
  if (!key) {
    return;
  }

  // Debounce: ignore if same key pressed within debounce window
  unsigned long now = millis();
  if (key == lastProcessedKey && (now - lastKeyTime) < KEY_DEBOUNCE_MS) {
    return;  // Ignore repeated key press
  }

  // Update debounce tracking
  lastProcessedKey = key;
  lastKeyTime = now;
  lastKeyPressed = key;
  
  Serial.print("[KEYPAD] Key = ");
  Serial.println(key);

  setFooterMessage(String("Key: ") + key);

  if (!menuVisible) {
    if (key == 'A') {
      menuVisible = true;
      menuActive = true;
      autoAdvance = false;
      setFooterMessage(F("Menu active (use 1-4, # to close)"));
      renderMenuPanel();
    } else if (key == 'B') {
      autoAdvance = !autoAdvance;
      setFooterMessage(autoAdvance ? F("Auto advance enabled") : F("Auto advance paused"));
    } else if (key == 'C') {
      nextStage();
      lastStageChange = millis();
      autoAdvance = false;
      setFooterMessage(F("Manual next stage"));
    } else if (key == 'D') {
      previousStage();
      lastStageChange = millis();
      autoAdvance = false;
      setFooterMessage(F("Manual prev stage"));
    }
  } else {
    handleMenuSelection(key);
  }
}

void handleMenuSelection(char key) {
  switch (key) {
    case '1':
      Serial.println("[MENU] Show test reference");
      currentStage = STAGE_BOOT;
      drawStage(currentStage);
      autoAdvance = false;
      setFooterMessage(F("Reference screen"));
      break;
    case '2':
      Serial.println("[MENU] Toggle gradients");
      currentStage = STAGE_GRADIENT;
      drawStage(currentStage);
      autoAdvance = false;
      setFooterMessage(F("Gradient focus"));
      break;
    case '3':
      Serial.println("[MENU] Run color bars");
      currentStage = STAGE_COLOR_BARS;
      drawStage(currentStage);
      autoAdvance = false;
      setFooterMessage(F("Color bars"));
      break;
    case '4':
      Serial.println("[MENU] Pixel focus");
      currentStage = STAGE_PIXELS;
      drawStage(currentStage);
      autoAdvance = false;
      setFooterMessage(F("Pixel target"));
      break;
    case '5':
      Serial.println("[MENU] WiFi Test");
      currentStage = STAGE_WIFI_TEST;
      drawStage(currentStage);
      autoAdvance = false;
      setFooterMessage(F("WiFi connectivity test"));
      break;
    case '6':
      Serial.println("[MENU] RFID Test");
      currentStage = STAGE_RFID_TEST;
      rfidScanCount = 0;
      lastRfidTag = "";
      drawStage(currentStage);
      autoAdvance = false;
      setFooterMessage(F("RFID scanning test"));
      break;
    case '7':
      Serial.println("[MENU] API Test");
      currentStage = STAGE_API_TEST;
      apiRequestCount = 0;
      apiHealthChecks = 0;
      apiHeartbeats = 0;
      lastApiResponse = "";
      lastHealthHttpCode = 0;
      lastHeartbeatHttpCode = 0;
      drawStage(currentStage);
      autoAdvance = false;
      setFooterMessage(F("API connectivity test"));
      break;
    case '8':
      Serial.println("[MENU] Registration Test");
      currentStage = STAGE_REGISTRATION_TEST;
      drawStage(currentStage);
      autoAdvance = false;
      setFooterMessage(F("Registration & polling test"));
      break;
    case '0':
      Serial.println("[MENU] Full System Test");
      currentStage = STAGE_FULL_SYSTEM;
      drawStage(currentStage);
      autoAdvance = false;
      setFooterMessage(F("All modules active"));
      break;
    case '#':
      menuVisible = false;
      menuActive = false;
      setFooterMessage(F("Menu hidden (press A to reopen)"));
      drawStage(currentStage);
      break;
    case '*':
    // WiFi connect/disconnect toggle or API test action
    if (currentStage == STAGE_WIFI_TEST) {
        if (wifiConnected) {
            WiFi.disconnect();
            wifiConnected = false;
            Serial.println("[WIFI] Disconnected");
        } else {
            initializeWiFi();
        }
        drawStage(currentStage);
    } else if (currentStage == STAGE_API_TEST) {
        // Test API connection (health then heartbeat)
        if (wifiConnected) {
            setFooterMessage(F("Testing API..."));
            testAPIHealth();
            testAPIHeartbeat();
            drawStage(currentStage);
        } else {
            setFooterMessage(F("WiFi required for API test"));
        }
    } else if (currentStage == STAGE_REGISTRATION_TEST) {
    if (wifiConnected) {
      initializePolling();
      setFooterMessage(F("Polled commands and sent heartbeat"));
    } else {
      setFooterMessage(F("WiFi required for polling"));
    }
        drawStage(currentStage);
    } else if (currentStage == STAGE_FULL_SYSTEM) {
        // Send last RFID tag to API
        if (wifiConnected && lastRfidTag.length() > 0) {
            setFooterMessage(F("Sending to API..."));
            sendRFIDScanToAPI(lastRfidTag);
            drawStage(currentStage);
        }
    }
    break;
    default:
      setFooterMessage(F("Invalid menu key"));
      break;
  }
}

void renderMenuPanel() {
  tft.fillRect(MENU_PANEL_X - 2, MENU_PANEL_Y - 2, MENU_PANEL_WIDTH + 4, MENU_PANEL_HEIGHT + 4, TFT_DARKGREY);
  tft.fillRect(MENU_PANEL_X, MENU_PANEL_Y, MENU_PANEL_WIDTH, MENU_PANEL_HEIGHT, TFT_BLACK);

  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(MENU_PANEL_X + 10, MENU_PANEL_Y + 6);
  tft.println("Diagnostics");

  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(MENU_PANEL_X + 10, MENU_PANEL_Y + 32);
  tft.println("1: Reference");
  tft.setCursor(MENU_PANEL_X + 10, MENU_PANEL_Y + 44);
  tft.println("2: Gradient");
  tft.setCursor(MENU_PANEL_X + 10, MENU_PANEL_Y + 56);
  tft.println("3: Color bars");
  tft.setCursor(MENU_PANEL_X + 10, MENU_PANEL_Y + 68);
  tft.println("4: Pixel grid");
  tft.setCursor(MENU_PANEL_X + 10, MENU_PANEL_Y + 80);
  tft.println("5: WiFi test");
  tft.setCursor(MENU_PANEL_X + 10, MENU_PANEL_Y + 92);
  tft.println("6: RFID test");
  tft.setCursor(MENU_PANEL_X + 10, MENU_PANEL_Y + 104);
  tft.println("7: API test");
  tft.setCursor(MENU_PANEL_X + 10, MENU_PANEL_Y + 116);
  tft.println("8: Registration/poll");
  tft.setCursor(MENU_PANEL_X + 10, MENU_PANEL_Y + 128);
  tft.println("0: Full system");
  tft.setCursor(MENU_PANEL_X + 10, MENU_PANEL_Y + 148);
  tft.println("B: Auto toggle");
  tft.setCursor(MENU_PANEL_X + 10, MENU_PANEL_Y + 160);
  tft.println("C/D: Next/Prev");
  tft.setCursor(MENU_PANEL_X + 10, MENU_PANEL_Y + 172);
  tft.println("#: Close menu");
}

void setFooterMessage(const String& msg) {
  footerMessage = msg;
  redrawFooter();
}

void setFooterMessage(const __FlashStringHelper* msg) {
  footerMessage = String(msg);
  redrawFooter();
}

void redrawFooter() {
  tft.fillRect(0, FOOTER_Y + 2, SCREEN_WIDTH, FOOTER_HEIGHT - 2, TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, FOOTER_Y + 6);
  tft.println(footerMessage.length() ? footerMessage : String(F("Press A for menu, B to toggle auto")));
  tft.setCursor(LEFT_MARGIN, FOOTER_Y + 18);
  tft.print("Auto advance: ");
  tft.println(autoAdvance ? "ON" : "OFF");
  if (lastKeyPressed) {
    tft.setCursor(SCREEN_WIDTH - 120, FOOTER_Y + 6);
    tft.print("Last key: ");
    tft.println(lastKeyPressed);
  }
}

bool initializeWiFi() {
  Serial.println("[WIFI] Connecting...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\n[WIFI] Connected!");
    Serial.print("[WIFI] IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    wifiConnected = false;
    Serial.println("\n[WIFI] Connection failed!");
    return false;
  }
}

void drawWiFiTest() {
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 30);
  tft.println("WiFi Test");
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 60);
  tft.print("Device MAC: ");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.println(deviceMac);
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 80);
  tft.print("SSID: ");
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.println(WIFI_SSID);
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 100);
  tft.print("Status: ");
  
  if (wifiConnected) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("CONNECTED");
    
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(LEFT_MARGIN, 120);
    tft.print("IP Address: ");
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println(WiFi.localIP());
    
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(LEFT_MARGIN, 140);
    tft.print("Signal: ");
    int rssi = WiFi.RSSI();
    tft.setTextColor(rssi > -70 ? TFT_GREEN : (rssi > -85 ? TFT_ORANGE : TFT_RED), TFT_BLACK);
    tft.print(rssi);
    tft.println(" dBm");
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("DISCONNECTED");
    
    tft.setTextSize(1);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setCursor(LEFT_MARGIN, 130);
    tft.println("Press * to connect");
  }
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 180);
  tft.println("*: Toggle connection");
  tft.setCursor(LEFT_MARGIN, 195);
  tft.println("#: Back to menu");
}

void drawRFIDTest() {
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 30);
  tft.println("RFID Test");
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 60);
  tft.print("Reader Status: ");
  
  if (rfidInitialized) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("READY");
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("NOT DETECTED");
  }
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 90);
  tft.println("Last Tag Scanned:");
  
  tft.setTextSize(2);
  if (lastRfidTag.length() > 0) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(LEFT_MARGIN, 110);
    tft.println(lastRfidTag);
  } else {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setCursor(LEFT_MARGIN, 110);
    tft.println("None");
  }
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 150);
  tft.print("Scan Count: ");
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.println(rfidScanCount);
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 180);
  tft.println(rfidInitialized ? "Scan an RFID card..." : "PN532 not detected!");
  
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 195);
  tft.println("#: Back to menu");
}

void drawFullSystemTest() {
  tft.fillScreen(TFT_BLACK);
  
  // Header
  tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, TFT_NAVY);
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW, TFT_NAVY);
  tft.setCursor(LEFT_MARGIN, 12);
  tft.println("Full System Test");
  
  // System status section
  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 60);
  tft.println("System Status:");
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 80);
  tft.print("WiFi: ");
  tft.setTextColor(wifiConnected ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.println(wifiConnected ? "OK" : "Offline");
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 95);
  tft.print("RFID: ");
  tft.setTextColor(rfidInitialized ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.println(rfidInitialized ? "OK" : "Failed");
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 110);
  tft.print("API: ");
  tft.setTextColor(apiConnected ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.println(apiConnected ? "OK" : "Offline");
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 125);
  tft.print("Device: ");
  tft.setTextColor(deviceActive ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.println(deviceActive ? "Active" : "Inactive");

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 140);
  tft.print("Scan Mode: ");
  tft.setTextColor(scanModeEnabled ? TFT_ORANGE : TFT_CYAN, TFT_BLACK);
  tft.println(scanModeEnabled ? "Enabled" : "Disabled");

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 155);
  tft.print("Polling: ");
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.println("Manual trigger");
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 170);
  tft.print("Display: ");
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println("OK");
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 185);
  tft.print("Keypad: ");
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.println("OK");
  
  // RFID scan section
  tft.drawRect(LEFT_MARGIN, 170, SCREEN_WIDTH - (LEFT_MARGIN * 2), 70, TFT_DARKGREY);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN + 5, 175);
  tft.println("RFID SCAN");
  
  if (lastRfidTag.length() > 0) {
    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(LEFT_MARGIN + 5, 195);
    tft.println(lastRfidTag);
    
    tft.setTextSize(1);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setCursor(LEFT_MARGIN + 5, 220);
    tft.print("Scans: ");
    tft.println(rfidScanCount);
  } else {
    tft.setTextSize(1);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setCursor(LEFT_MARGIN + 5, 205);
    tft.println("Waiting for RFID card...");
  }
  
  // Footer
  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 255);
  if (registrationMode) {
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.print("REG MODE: ");
    tft.print(expectedRegistrationTagId.length() > 0 ? expectedRegistrationTagId : "Awaiting...");
  } else {
    tft.println("#: Menu | *: Toggle WiFi");
  }
}

void handleRFIDScanning() {
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;
  
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100)) {
    // Build tag ID string
    String tagId = "";
    for (uint8_t i = 0; i < uidLength; i++) {
      if (uid[i] < 0x10) tagId += "0";
      tagId += String(uid[i], HEX);
    }
    tagId.toUpperCase();
    
    // Only update if it's a new tag (debouncing)
    if (tagId != lastRfidTag || millis() - lastScanTime > 2000) {
      lastRfidTag = tagId;
      rfidScanCount++;
      lastScanTime = millis();
      
      Serial.print("[RFID] Tag detected: ");
      Serial.println(tagId);
      Serial.print("[RFID] Scan count: ");
      Serial.println(rfidScanCount);
      
      // Handle registration mode
      if (registrationMode && currentStage == STAGE_REGISTRATION_TEST) {
        if (expectedRegistrationTagId.length() > 0) {
          if (tagId.equalsIgnoreCase(expectedRegistrationTagId)) {
            Serial.println("[REGISTRATION] ✓ Tag match! Registration confirmed.");
            registrationMode = false;
            expectedRegistrationTagId = "";
            setFooterMessage(F("Registration successful!"));
          } else {
            Serial.println("[REGISTRATION] ✗ Tag mismatch!");
            setFooterMessage(F("Wrong tag - try again"));
          }
        } else {
          Serial.println("[REGISTRATION] No expected tag set");
          setFooterMessage(F("No tag expected"));
        }
        drawStage(currentStage);
      }
      // In full system mode, automatically send to API via HTTP
  else if (currentStage == STAGE_FULL_SYSTEM && wifiConnected) {
        Serial.println("[FULL SYSTEM] Sending scan via HTTP...");
        
        HTTPClient http;
        String url = "http://" + String(API_HOST) + ":" + String(API_PORT) + "/api/rfid/scan";
        
        http.begin(url);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("X-API-Key", apiKey);
        
        JsonDocument doc;
        doc["tagId"] = tagId;
        doc["deviceId"] = deviceId;
        doc["location"] = "Diagnostics Station";
        doc["timestamp"] = millis();
        
        String message;
        serializeJson(doc, message);
        
        int httpCode = http.POST(message);
        if (httpCode == 200 || httpCode == 201) {
          Serial.println("[HTTP] Scan sent successfully: " + tagId);
        } else {
          Serial.printf("[HTTP] Scan failed: %d\n", httpCode);
        }
        http.end();
        
        drawStage(currentStage);
      }
      // Normal test modes - just update display
      else {
        drawStage(currentStage);
      }
    }
  }
}

bool testAPIHealth() {
  if (!wifiConnected) {
    Serial.println("[API] WiFi not connected");
    lastApiResponse = "WiFi not connected";
    lastHealthHttpCode = 0;
    apiConnected = false;
    return false;
  }
  
  HTTPClient http;
  String url = String(API_BASE_URL) + "/health";
  
  Serial.print("[API] Testing health endpoint: ");
  Serial.println(url);
  
  http.begin(url);
  
  int httpCode = http.GET();
  apiRequestCount++;
  apiHealthChecks++;
  lastHealthHttpCode = httpCode;
  
  if (httpCode > 0) {
    String response = http.getString();
    Serial.print("[API] Health response code: ");
    Serial.println(httpCode);
    Serial.print("[API] Health response: ");
    Serial.println(response);
    
    lastApiResponse = response;
    lastApiHttpCode = httpCode;
    apiConnected = (httpCode >= 200 && httpCode < 300);
  } else {
    lastApiResponse = http.errorToString(httpCode);
    Serial.print("[API] Health error: ");
    Serial.println(lastApiResponse);
    lastApiHttpCode = httpCode;
    apiConnected = false;
  }
  
  http.end();
  return apiConnected;
}

bool testAPIHeartbeat() {
  if (!wifiConnected) {
    Serial.println("[API] WiFi not connected");
    lastHeartbeatHttpCode = 0;
    return false;
  }
  
  HTTPClient http;
  String url = String(API_BASE_URL) + "/api/devices/" + deviceMac + "/heartbeat";
  
  Serial.print("[API] Testing heartbeat endpoint: ");
  Serial.println(url);
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  if (strlen(API_KEY) > 0) {
    http.addHeader("x-api-key", API_KEY);
  }
  
  StaticJsonDocument<200> doc;
  doc["timestamp"] = millis();
  doc["status"] = "diagnostic_test";
  
  String payload;
  serializeJson(doc, payload);
  
  int httpCode = http.POST(payload);
  apiRequestCount++;
  apiHeartbeats++;
  lastHeartbeatHttpCode = httpCode;
  apiConnected = (httpCode >= 200 && httpCode < 300);
  bool refreshNeeded = false;
  
  if (httpCode > 0) {
    String response = http.getString();
    Serial.print("[API] Heartbeat response code: ");
    Serial.println(httpCode);
    Serial.print("[API] Heartbeat response: ");
    Serial.println(response);
    
    // Don't overwrite successful health check response
    if (lastHealthHttpCode < 200 || lastHealthHttpCode >= 300) {
      lastApiResponse = response;
      lastApiHttpCode = httpCode;
    }

    JsonDocument doc;
    DeserializationError jsonError = deserializeJson(doc, response);
    if (!jsonError && doc["success"]) {
      JsonObject data = doc["data"];
      if (!data.isNull() && !data["device"].isNull()) {
        JsonObject device = data["device"];

        if (!device["registrationMode"].isNull()) {
          bool reg = device["registrationMode"].as<bool>();
          if (reg != registrationMode) {
            Serial.printf("[REG] Heartbeat sync: registration %s\n", reg ? "ENABLED" : "DISABLED");
            registrationMode = reg;
            if (!registrationMode) {
              expectedRegistrationTagId = "";
              registrationModeStartTime = 0;
            } else {
              registrationModeStartTime = millis();
            }
            refreshNeeded = true;
          }
        }

        if (!device["scanMode"].isNull()) {
          bool scan = device["scanMode"].as<bool>();
          if (scan != scanModeEnabled) {
            Serial.printf("[SCAN MODE] Heartbeat sync: %s\n", scan ? "ENABLED" : "DISABLED");
            scanModeEnabled = scan;
            refreshNeeded = true;
          }
        }

        if (!device["isActive"].isNull()) {
          bool active = device["isActive"].as<bool>();
          if (active != deviceActive) {
            Serial.printf("[DEVICE] Heartbeat sync: device %s\n", active ? "ACTIVE" : "INACTIVE");
            deviceActive = active;
            refreshNeeded = true;
          }
        }

        if (!device["pendingRegistrationTagId"].isNull()) {
          String pending = device["pendingRegistrationTagId"].as<String>();
          if (pending.length() > 0 && pending != expectedRegistrationTagId) {
            expectedRegistrationTagId = pending;
            Serial.printf("[REG] Heartbeat pending tag: %s\n", pending.c_str());
            refreshNeeded = true;
          }
        }

        if (!device["lastSeen"].isNull()) {
          String seen = device["lastSeen"].as<String>();
          if (seen != serverLastSeen) {
            serverLastSeen = seen;
            refreshNeeded = true;
          }
        }
      }
    }
  } else {
    String errorMsg = http.errorToString(httpCode);
    Serial.print("[API] Heartbeat error: ");
    Serial.println(errorMsg);
    apiConnected = false;
  }
  
  http.end();
  lastHeartbeatTime = millis();
  if (refreshNeeded && (currentStage == STAGE_REGISTRATION_TEST || currentStage == STAGE_FULL_SYSTEM || currentStage == STAGE_API_TEST)) {
    drawStage(currentStage);
  }
  return (httpCode >= 200 && httpCode < 300);
}

bool sendRFIDScanToAPI(const String& tagId) {
  if (!wifiConnected) {
    Serial.println("[API] WiFi not connected");
    return false;
  }
  
  HTTPClient http;
  String url = String(API_BASE_URL) + "/api/rfid/scan";
  
  Serial.print("[API] Sending RFID scan to: ");
  Serial.println(url);
  Serial.print("[API] Tag ID: ");
  Serial.println(tagId);
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  if (strlen(API_KEY) > 0) {
    http.addHeader("x-api-key", API_KEY);
  }
  
  StaticJsonDocument<300> doc;
  doc["tagId"] = tagId;
  doc["deviceId"] = deviceMac;
  doc["location"] = "Diagnostics Station";
  doc["timestamp"] = millis();
  
  String payload;
  serializeJson(doc, payload);
  
  int httpCode = http.POST(payload);
  apiRequestCount++;
  lastApiHttpCode = httpCode;
  
  if (httpCode > 0) {
    lastApiResponse = http.getString();
    Serial.print("[API] Scan response code: ");
    Serial.println(httpCode);
    Serial.print("[API] Scan response: ");
    Serial.println(lastApiResponse);
    
    http.end();
    return (httpCode >= 200 && httpCode < 300);
  } else {
    lastApiResponse = http.errorToString(httpCode);
    Serial.print("[API] Scan error: ");
    Serial.println(lastApiResponse);
    
    http.end();
    return false;
  }
}

void drawAPITest() {
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 30);
  tft.println("API Test");
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 60);
  tft.print("Base URL: ");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  // Truncate long URLs
  String baseUrl = String(API_BASE_URL);
  if (baseUrl.length() > 38) {
    baseUrl = baseUrl.substring(0, 38) + "...";
  }
  tft.println(baseUrl);
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 75);
  tft.print("Device MAC: ");
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.println(deviceMac);
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 95);
  tft.print("WiFi: ");
  tft.setTextColor(wifiConnected ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.println(wifiConnected ? "Connected" : "Disconnected");
  
  // Health endpoint test results
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 115);
  tft.println("GET /health:");
  
  if (apiHealthChecks > 0) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(LEFT_MARGIN + 10, 130);
    tft.print("Status: ");
    tft.setTextColor(lastHealthHttpCode >= 200 && lastHealthHttpCode < 300 ? TFT_GREEN : TFT_RED, TFT_BLACK);
    tft.print(lastHealthHttpCode);
    tft.setTextColor(lastHealthHttpCode >= 200 && lastHealthHttpCode < 300 ? TFT_GREEN : TFT_RED, TFT_BLACK);
    tft.println(lastHealthHttpCode >= 200 && lastHealthHttpCode < 300 ? " OK" : " FAIL");
  } else {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setCursor(LEFT_MARGIN + 10, 130);
    tft.println("Not tested");
  }
  
  // Heartbeat endpoint test results
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 150);
  tft.println("POST /devices/.../heartbeat:");
  
  if (apiHeartbeats > 0) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(LEFT_MARGIN + 10, 165);
    tft.print("Status: ");
    bool heartbeatOk = (lastHeartbeatHttpCode >= 200 && lastHeartbeatHttpCode < 300);
    tft.setTextColor(heartbeatOk ? TFT_GREEN : (lastHeartbeatHttpCode == 404 ? TFT_ORANGE : TFT_RED), TFT_BLACK);
    tft.print(lastHeartbeatHttpCode);
    if (heartbeatOk) {
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.println(" OK");
    } else if (lastHeartbeatHttpCode == 404) {
      tft.setTextColor(TFT_ORANGE, TFT_BLACK);
      tft.println(" (Not registered)");
    } else {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println(" FAIL");
    }
  } else {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setCursor(LEFT_MARGIN + 10, 165);
    tft.println("Not tested");
  }
  
  // Summary
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 190);
  tft.print("Total Requests: ");
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.println(apiRequestCount);
  
  // Show truncated response
  if (lastApiResponse.length() > 0) {
    tft.setTextSize(1);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setCursor(LEFT_MARGIN, 210);
    tft.println("Last Response:");
    tft.setCursor(LEFT_MARGIN, 225);
    String truncated = lastApiResponse.substring(0, min(50, (int)lastApiResponse.length()));
    tft.println(truncated);
  }
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 255);
  tft.println("*: Test both endpoints");
  tft.setCursor(LEFT_MARGIN, 270);
  tft.println("#: Back to menu");
}

void initializePolling() {
  if (!wifiConnected) {
    Serial.println("[POLL] WiFi not connected");
    return;
  }
  Serial.println("[POLL] HTTP command poll triggered");
  pollCommands();
  testAPIHeartbeat();
}

void pollCommands() {
  if (!wifiConnected || deviceMac.isEmpty()) return;
  
  HTTPClient http;
  String url = String(API_BASE_URL) + "/api/devices/" + deviceMac + "/commands";
  
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  if (strlen(API_KEY) > 0) {
    http.addHeader("X-API-Key", API_KEY);
  }
  
  commandPollCount++;
  int httpCode = http.GET();
  lastCommandHttpCode = httpCode;
  
  Serial.printf("[POLL] Command poll #%d - HTTP %d\n", commandPollCount, httpCode);
  
  if (httpCode == 200) {
    String payload = http.getString();
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error && doc["success"]) {
      JsonObject data = doc["data"];
      JsonArray commands = data["commands"];
      bool refreshNeeded = false;

      // Sync device state flags with server DB so diagnostics matches admin view
      if (!data["deviceStatus"].isNull()) {
        JsonObject status = data["deviceStatus"];

        bool serverRegistration = status["registrationMode"].isNull()
                                     ? registrationMode
                                     : status["registrationMode"].as<bool>();
        bool serverScanMode = status["scanMode"].isNull()
                                   ? scanModeEnabled
                                   : status["scanMode"].as<bool>();
        bool serverActive = status["isActive"].isNull()
                                  ? deviceActive
                                  : status["isActive"].as<bool>();

        if (serverRegistration != registrationMode) {
          Serial.printf("[REG] Server registration flag = %s (was %s)\n",
                        serverRegistration ? "true" : "false",
                        registrationMode ? "true" : "false");
          registrationMode = serverRegistration;
          if (!registrationMode) {
            expectedRegistrationTagId = "";
            registrationModeStartTime = 0;
          }
          if (registrationMode) {
            registrationModeStartTime = millis();
          }
          refreshNeeded = true;
        }

        if (serverScanMode != scanModeEnabled) {
          Serial.printf("[SCAN MODE] Server scan mode = %s\n",
                        serverScanMode ? "ENABLED" : "DISABLED");
          scanModeEnabled = serverScanMode;
          refreshNeeded = true;
        }

        if (serverActive != deviceActive) {
          Serial.printf("[DEVICE] Server reports device %s\n",
                        serverActive ? "ACTIVE" : "INACTIVE");
          deviceActive = serverActive;
          refreshNeeded = true;
        }
      }

      for (JsonObject cmd : commands) {
        String action = cmd["action"].as<String>();
        
        if (action == "enable_registration") {
          String tagId = cmd["tagId"].as<String>();
          if (tagId.length() > 0) {
            registrationMode = true;
            expectedRegistrationTagId = tagId;
            registrationModeStartTime = millis();
            Serial.printf("[REG] Registration enabled for tag: %s\n", tagId.c_str());
            setFooterMessage(String("Reg mode: ") + expectedRegistrationTagId);
            refreshNeeded = true;
          }
        } else if (action == "disable_registration") {
          registrationMode = false;
          expectedRegistrationTagId = "";
          Serial.println("[REG] Registration disabled by server");
          setFooterMessage(F("Reg mode OFF"));
          refreshNeeded = true;
        } else if (action == "scan_mode") {
          bool enabled = cmd["enabled"].isNull() ? scanModeEnabled : cmd["enabled"].as<bool>();
          scanModeEnabled = enabled;
          Serial.printf("[SCAN MODE] Command sets scan mode %s\n", enabled ? "ENABLED" : "DISABLED");
          refreshNeeded = true;
        }
      }

      if (refreshNeeded && (currentStage == STAGE_REGISTRATION_TEST || currentStage == STAGE_FULL_SYSTEM)) {
        drawStage(currentStage);
      }
      
    }
  } else {
    Serial.printf("[POLL] Failed to get commands - HTTP %d\n", httpCode);
  }
  
  http.end();
  lastCommandPoll = millis();
}

void drawRegistrationTest() {
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 30);
  tft.println("Registration Test");
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 60);
  tft.print("WiFi: ");
  tft.setTextColor(wifiConnected ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.println(wifiConnected ? "Connected" : "Disconnected");
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 75);
  tft.print("Poll Count: ");
  if (commandPollCount > 0) {
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.println(commandPollCount);
  } else {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.println("No polls yet");
  }
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 90);
  tft.print("Last Poll: ");
  if (commandPollCount > 0) {
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    unsigned long secondsAgo = (millis() - lastCommandPoll) / 1000;
    tft.print(secondsAgo);
    tft.println("s ago");
  } else {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.println("N/A");
  }
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 105);
  tft.print("Poll HTTP: ");
  if (commandPollCount > 0) {
    if (lastCommandHttpCode == 200) {
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
    } else if (lastCommandHttpCode >= 400) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
    } else if (lastCommandHttpCode == 0) {
      tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    } else {
      tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    }
    tft.println(lastCommandHttpCode);
  } else {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.println("N/A");
  }
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 120);
  tft.print("Heartbeat Count: ");
  if (apiHeartbeats > 0) {
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.println(apiHeartbeats);
  } else {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.println("None");
  }
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 135);
  tft.print("Heartbeat HTTP: ");
  if (apiHeartbeats > 0) {
    if (lastHeartbeatHttpCode >= 200 && lastHeartbeatHttpCode < 300) {
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
    } else if (lastHeartbeatHttpCode == 0) {
      tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    } else if (lastHeartbeatHttpCode >= 400) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
    } else {
      tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    }
    tft.println(lastHeartbeatHttpCode);
  } else {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.println("N/A");
  }
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 150);
  tft.print("Last Heartbeat: ");
  if (apiHeartbeats > 0 && lastHeartbeatTime > 0) {
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    unsigned long secondsAgo = (millis() - lastHeartbeatTime) / 1000;
    tft.print(secondsAgo);
    tft.println("s ago");
  } else {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.println("N/A");
  }
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 165);
  tft.print("Server Last Seen: ");
  if (serverLastSeen.length() > 0) {
    String truncated = serverLastSeen.substring(0, min(19, static_cast<int>(serverLastSeen.length())));
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.println(truncated);
  } else {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.println("Unknown");
  }

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 180);
  tft.print("Device Active: ");
  tft.setTextColor(deviceActive ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.println(deviceActive ? "Yes" : "No");

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 195);
  tft.print("Scan Mode: ");
  tft.setTextColor(scanModeEnabled ? TFT_ORANGE : TFT_CYAN, TFT_BLACK);
  tft.println(scanModeEnabled ? "Enabled" : "Disabled");

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 210);
  tft.print("Registration Mode: ");
  tft.setTextColor(registrationMode ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.println(registrationMode ? "ACTIVE" : "INACTIVE");

  int lastTagLabelY = registrationMode ? 255 : 225;

  if (registrationMode) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(LEFT_MARGIN, 225);
    tft.print("Expected Tag: ");
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    if (expectedRegistrationTagId.length() > 0) {
      tft.println(expectedRegistrationTagId);
    } else {
      tft.setTextColor(TFT_ORANGE, TFT_BLACK);
      tft.println("Awaiting server...");
    }

    unsigned long elapsed = (millis() - registrationModeStartTime) / 1000;
    unsigned long remaining = (REGISTRATION_MODE_TIMEOUT / 1000) - elapsed;

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(LEFT_MARGIN, 240);
    tft.print("Timeout in: ");
    tft.setTextColor(remaining < 30 ? TFT_ORANGE : TFT_CYAN, TFT_BLACK);
    tft.print(remaining);
    tft.println("s");
  }

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, lastTagLabelY);
  tft.println("Last Tag Scanned:");

  tft.setTextSize(2);
  int tagValueY = lastTagLabelY + 20;
  if (lastRfidTag.length() > 0) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(LEFT_MARGIN, tagValueY);
    tft.println(lastRfidTag);
  } else {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setCursor(LEFT_MARGIN, tagValueY);
    tft.println("None");
  }
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 285);
  if (registrationMode) {
    tft.println("Scan the card to register...");
  } else {
    tft.println("Enable registration mode in admin, then poll");
  }
  
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 305);
  tft.println("*: Poll commands + heartbeat");
  tft.setCursor(LEFT_MARGIN, 320);
  tft.println("#: Back to menu");
}

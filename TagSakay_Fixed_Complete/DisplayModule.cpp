#include "DisplayModule.h"
#include "NetworkModule.h"

TFT_eSPI tft = TFT_eSPI();

// Keypad menu layout helpers
extern bool keypadMenuActive;
extern bool keypadMenuVisible;

namespace {
  const int MENU_PANEL_WIDTH = 150;
  const int MENU_PANEL_X = SCREEN_WIDTH - MENU_PANEL_WIDTH;
  const int MENU_PANEL_PADDING = 10;
  const int MENU_PANEL_TOP = STATUS_SECTION_Y + 12;
  const int MENU_PANEL_HEIGHT = FOOTER_Y - MENU_PANEL_TOP - 12;

  int getContentWidth() {
    if (keypadMenuVisible) {
      int adjustedWidth = MENU_PANEL_X - LEFT_MARGIN - 6;
      if (adjustedWidth > 0) {
        return adjustedWidth;
      }
    }
    return SCREEN_WIDTH - (LEFT_MARGIN * 2);
  }

  void clearMenuPanelArea() {
    int panelX = MENU_PANEL_X - 6;
    if (panelX < 0) {
      panelX = 0;
    }
    int panelWidth = SCREEN_WIDTH - panelX;
    if (panelWidth <= 0) {
      return;
    }

    tft.fillRect(panelX, STATUS_SECTION_Y + 5, panelWidth,
                 FOOTER_Y - STATUS_SECTION_Y - 8, TFT_BLACK);
  }

  void drawMenuPanelFrame() {
    int frameX = MENU_PANEL_X - 4;
    if (frameX < 0) {
      frameX = 0;
    }
    int frameHeight = FOOTER_Y - STATUS_SECTION_Y - 8;
    tft.drawFastVLine(frameX, STATUS_SECTION_Y + 5, frameHeight, TFT_DARKGREY);

    int rectX = MENU_PANEL_X - 2;
    if (rectX < 0) {
      rectX = 0;
    }
    int rectWidth = MENU_PANEL_WIDTH + 2;
    if (rectX + rectWidth > SCREEN_WIDTH) {
      rectWidth = SCREEN_WIDTH - rectX;
    }
    if (rectWidth > 0) {
      tft.drawRect(rectX, MENU_PANEL_TOP - 6, rectWidth, MENU_PANEL_HEIGHT + 12, TFT_LIGHTGREY);
    }
  }
}

void initializeTFT() {
  tft.init();
  tft.setRotation(1);  // Landscape orientation for 480x320 ILI9488
  clearScreen();
  drawHeader();
  drawSectionBorders();
  
  updateStatusSection("Initializing...", TFT_YELLOW);
  updateConnectionStatus("Disconnected", "No sync", "Starting");
}

void clearScreen() {
  tft.fillScreen(TFT_BLACK);
}

void drawHeader() {
  tft.fillRect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, TFT_NAVY);
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW, TFT_NAVY);
  tft.setCursor(LEFT_MARGIN, 12);
  tft.println("TagSakay RFID Scanner");
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_NAVY);
  tft.setCursor(SCREEN_WIDTH - 130, 24);
  tft.println("v2.0");
}

void drawSectionBorders() {
  tft.drawLine(0, HEADER_HEIGHT, SCREEN_WIDTH, HEADER_HEIGHT, TFT_WHITE);
  
  tft.drawRect(0, STATUS_SECTION_Y, SCREEN_WIDTH, STATUS_SECTION_HEIGHT, TFT_DARKGREY);
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, STATUS_SECTION_Y + 2);
  tft.println("STATUS");
  
  tft.drawRect(0, SCAN_SECTION_Y, SCREEN_WIDTH, SCAN_SECTION_HEIGHT, TFT_DARKGREY);
  tft.setCursor(LEFT_MARGIN, SCAN_SECTION_Y + 2);
  tft.println("RFID SCAN");
  
  tft.drawLine(0, FOOTER_Y, SCREEN_WIDTH, FOOTER_Y, TFT_DARKGREY);
}

void updateStatusSection(const String& msg, uint16_t color) {
  tft.fillRect(LEFT_MARGIN, STATUS_SECTION_Y + 15, getContentWidth(), 20, TFT_BLACK);
  
  tft.setTextSize(2);
  tft.setTextColor(color, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, STATUS_SECTION_Y + 15);
  tft.println(msg.substring(0, 24));
}

void updateConnectionStatus(const String& wifi, const String& time, const String& device) {
  tft.fillRect(LEFT_MARGIN, STATUS_SECTION_Y + 40, getContentWidth(), 25, TFT_BLACK);
  
  tft.setTextSize(1);
  
  tft.setTextColor((wifi == "Connected") ? TFT_GREEN : TFT_RED, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, STATUS_SECTION_Y + 40);
  tft.print("WiFi: ");
  tft.println(wifi.substring(0, 16));
  
  tft.setTextColor((time == "Synced") ? TFT_GREEN : TFT_ORANGE, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, STATUS_SECTION_Y + 52);
  tft.print("Time: ");
  tft.println(time.substring(0, 16));
  
  // Display full MAC address (12 chars)
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, STATUS_SECTION_Y + 64);
  tft.print("MAC: ");
  tft.println(device.substring(0, 12));
  
  // Registration mode indicator on the right side
  if (registrationMode) {
    tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
    int regX = keypadMenuVisible ? MENU_PANEL_X - 95 : SCREEN_WIDTH - 95;
    if (regX < LEFT_MARGIN) {
      regX = LEFT_MARGIN;
    }
    tft.setCursor(regX, STATUS_SECTION_Y + 64);
    tft.println("REG MODE");
  } else {
    int clearX = keypadMenuVisible ? MENU_PANEL_X - 95 : SCREEN_WIDTH - 95;
    if (clearX < LEFT_MARGIN) {
      clearX = LEFT_MARGIN;
    }
    tft.fillRect(clearX, STATUS_SECTION_Y + 64, 80, 10, TFT_BLACK);
  }
}

void updateScanSection(const String& tagId, const String& status, const String& userInfo, uint16_t color) {
  // Always clear the scan section area first
  tft.fillRect(LEFT_MARGIN, SCAN_SECTION_Y + 15, getContentWidth(), SCAN_SECTION_HEIGHT - 20, TFT_BLACK);
  
  // Only draw content if tagId is provided
  if (tagId.length() > 0) {
    tft.setTextSize(1);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.setCursor(LEFT_MARGIN, SCAN_SECTION_Y + 15);
    tft.print("Tag: ");
    tft.println(tagId.substring(0, 20));
    
    tft.setTextSize(2);
    tft.setTextColor(color, TFT_BLACK);
    tft.setCursor(LEFT_MARGIN, SCAN_SECTION_Y + 30);
    tft.println(status.substring(0, 20));
    
    if (userInfo.length() > 0) {
      tft.setTextSize(1);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setCursor(LEFT_MARGIN, SCAN_SECTION_Y + 55);
      tft.println(userInfo.substring(0, 36));
    }
    
    String timestamp = getCurrentTimestamp();
    if (timestamp.length() > 0) {
      tft.setTextSize(1);
      tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
      tft.setCursor(LEFT_MARGIN, SCAN_SECTION_Y + 70);
      tft.println(timestamp.substring(11, 19));
    }
  } else {
    tft.setTextSize(1);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setCursor(LEFT_MARGIN, SCAN_SECTION_Y + 35);
    tft.println("Waiting for RFID card...");
  }
}

void updateFooter(const String& msg) {
  tft.fillRect(0, FOOTER_Y + 2, SCREEN_WIDTH, FOOTER_HEIGHT - 2, TFT_BLACK);
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, FOOTER_Y + 5);
  tft.println(msg.substring(0, 45));
  
  String timestamp = getCurrentTimestamp();
  if (timestamp.length() > 0) {
    String timeStr = timestamp.substring(11, 19);
    int timestampX = keypadMenuVisible ? MENU_PANEL_X - 120 : SCREEN_WIDTH - 120;
    if (timestampX < LEFT_MARGIN) {
      timestampX = LEFT_MARGIN;
    }
    tft.setCursor(timestampX, FOOTER_Y + 5);
    tft.println(timeStr);
  }
  
  unsigned long uptime = millis() / 1000;
  String uptimeStr = "Up: " + String(uptime / 3600) + "h " + String((uptime % 3600) / 60) + "m";
  tft.setCursor(LEFT_MARGIN, FOOTER_Y + 18);
  tft.println(uptimeStr);
}

void showHeartbeat(bool active) {
  uint16_t color = active ? TFT_GREEN : TFT_DARKGREY;
  int indicatorX = keypadMenuVisible ? MENU_PANEL_X - 30 : SCREEN_WIDTH - 30;
  if (indicatorX < LEFT_MARGIN + 20) {
    indicatorX = LEFT_MARGIN + 20;
  }
  tft.fillCircle(indicatorX, FOOTER_Y + 12, 4, color);
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(indicatorX - 25, FOOTER_Y + 18);
  tft.println("HB");
}

void showStatus(const String& msg, uint16_t color, int x, int y, int textSize) {
  tft.setTextColor(color, TFT_BLACK);
  tft.setTextSize(textSize);
  int clearWidth = SCREEN_WIDTH - x - LEFT_MARGIN;
  if (clearWidth < 0) {
    clearWidth = SCREEN_WIDTH;
  }
  tft.fillRect(x, y, clearWidth, 20, TFT_BLACK);
  tft.setCursor(x, y);
  tft.println(msg);
}

void showRFIDScan(const String& tagId, const String& status, uint16_t color) {
  String userInfo = "";
  updateScanSection(tagId, status, userInfo, color);
}

void indicateSuccess() {
  updateStatusSection("SCAN SUCCESS", TFT_GREEN);
  updateFooter("Last scan: Successful");
  Serial.println("✓ Scan successful");
}

void indicateError() {
  updateStatusSection("SCAN ERROR", TFT_RED);
  updateFooter("Last scan: Error occurred");
  Serial.println("✗ Scan error");
}

void indicateUnregisteredTag() {
  updateStatusSection("UNREGISTERED TAG", TFT_ORANGE);
  updateFooter("Last scan: Unregistered card");
  Serial.println("⚠ Unregistered tag detected");
}

void indicateRegistrationMode() {
  updateStatusSection("REGISTRATION MODE", TFT_MAGENTA);
  updateFooter("Registration mode active");
  Serial.println("🔧 Registration mode active");
}

void indicateReady() {
  bool wasMenuVisible = keypadMenuVisible;
  bool wasMenuActive = keypadMenuActive;
  clearScreen();
  drawHeader();
  drawSectionBorders();
  updateStatusSection("SYSTEM READY", TFT_GREEN);
  updateScanSection("", "", "", TFT_WHITE);  // Clear scan section properly
  updateFooter("System ready - waiting for cards");
  keypadMenuVisible = wasMenuVisible;
  keypadMenuActive = wasMenuActive;
  if (wasMenuVisible) {
    showKeypadMenu(false);
  }
  Serial.println("✓ System ready");
}

void indicateRegistrationTagDetected() {
  updateStatusSection("REGISTRATION OK", TFT_GREEN);
  updateFooter("Registration tag detected");
  Serial.println("✓ Registration tag detected");
}

void blinkError(int times) {
  for (int i = 0; i < times; i++) {
    updateStatusSection("ERROR " + String(i + 1) + "/" + String(times), TFT_RED);
    delay(500);
    updateStatusSection("", TFT_BLACK);
    delay(200);
  }
  updateStatusSection("SYSTEM READY", TFT_GREEN);
  updateFooter("Error sequence completed");
  Serial.println("✗ Error occurred (" + String(times) + " times)");
}

void displayKeypadPrompt(const String& prompt, const String& buffer) {
  tft.fillRect(LEFT_MARGIN, SCAN_SECTION_Y + 15, getContentWidth(), SCAN_SECTION_HEIGHT - 20, TFT_BLACK);
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, SCAN_SECTION_Y + 20);
  tft.println(prompt);
  
  tft.setTextSize(3);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, SCAN_SECTION_Y + 45);
  tft.println(buffer.length() > 0 ? buffer : "_");
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, SCAN_SECTION_Y + 75);
  tft.println("#:Confirm  *:Cancel");
  
  updateFooter("Enter number and press #");
}

void showKeypadMenu(bool refreshFooter) {
  keypadMenuVisible = true;
  clearMenuPanelArea();
  drawMenuPanelFrame();

  int innerX = MENU_PANEL_X + 2;
  int innerWidth = MENU_PANEL_WIDTH - 12;
  if (innerWidth < 0) {
    innerWidth = MENU_PANEL_WIDTH - 4;
  }
  tft.fillRect(innerX, MENU_PANEL_TOP - 2, innerWidth, MENU_PANEL_HEIGHT + 4, TFT_BLACK);

  tft.setTextSize(1);
  tft.setTextColor(keypadMenuActive ? TFT_CYAN : TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(MENU_PANEL_X + MENU_PANEL_PADDING, MENU_PANEL_TOP + 2);
  tft.println("Keypad Menu");

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(MENU_PANEL_X + MENU_PANEL_PADDING, MENU_PANEL_TOP + 16);
  if (keypadMenuActive) {
    tft.println("Select 1-4 or #");
  } else {
    tft.println("Press A to select");
  }

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  int cursorY = MENU_PANEL_TOP + 32;
  tft.setCursor(MENU_PANEL_X + MENU_PANEL_PADDING, cursorY);
  tft.println("1: Send heartbeat");

  cursorY += 14;
  tft.setCursor(MENU_PANEL_X + MENU_PANEL_PADDING, cursorY);
  tft.println("2: Enable reg mode");

  cursorY += 14;
  tft.setCursor(MENU_PANEL_X + MENU_PANEL_PADDING, cursorY);
  tft.println("3: Disable reg mode");

  cursorY += 14;
  tft.setCursor(MENU_PANEL_X + MENU_PANEL_PADDING, cursorY);
  tft.println("4: Sync device");

  cursorY += 18;
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.setCursor(MENU_PANEL_X + MENU_PANEL_PADDING, cursorY);
  tft.println("#: Close menu");

  if (refreshFooter) {
    if (keypadMenuActive) {
      updateFooter("Select menu option");
    } else {
      updateFooter("Press A then choose an option");
    }
  }

  Serial.println("=== KEYPAD MENU ===");
  if (!keypadMenuActive) {
    Serial.println("Press 'A' to activate menu selections");
  }
  Serial.println("1: Send heartbeat");
  Serial.println("2: Enable registration mode");
  Serial.println("3: Disable registration mode");
  Serial.println("4: Sync device profile");
  Serial.println("#: Exit menu");
}

void hideKeypadMenu() {
  keypadMenuVisible = false;
  keypadMenuActive = false;
  clearMenuPanelArea();
  drawSectionBorders();
}

// Test mode display functions
void showMenu(const char* title, const char* items) {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN);
  tft.println(title);
  tft.setTextSize(1);
  tft.println("");
  tft.setTextColor(TFT_WHITE);
  tft.println(items);
}

void showTestResult(const char* testName, bool passed, const char* details) {
  tft.fillRect(0, 100, SCREEN_WIDTH, 80, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 100);
  
  tft.setTextSize(2);
  tft.setTextColor(passed ? TFT_GREEN : TFT_RED);
  tft.println(passed ? "PASS" : "FAIL");
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.println("");
  tft.println(testName);
  
  if (details) {
    tft.setTextColor(TFT_LIGHTGREY);
    tft.println(details);
  }
}

void showKeypadInput(char key, int count) {
  tft.fillRect(0, 100, SCREEN_WIDTH, 80, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 100);
  
  tft.setTextColor(TFT_MAGENTA);
  tft.println("KEYPAD:");
  tft.setTextSize(4);
  tft.setTextColor(TFT_YELLOW);
  tft.println(key);
  tft.setTextSize(1);
  
  if (count > 0) {
    tft.setTextColor(TFT_WHITE);
    tft.print("Count: ");
    tft.println(count);
  }
}

void showRFIDScan(const String& tagId, int count) {
  tft.fillRect(0, 170, SCREEN_WIDTH, 70, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 170);
  
  tft.setTextColor(TFT_CYAN);
  tft.println("RFID:");
  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN);
  tft.println(tagId.substring(0, 16));
  tft.setTextSize(1);
  
  if (count > 0) {
    tft.setTextColor(TFT_WHITE);
    tft.print("Count: ");
    tft.println(count);
  }
}

void drawHeartbeat() {
  // Draw heartbeat indicator
  int hbX = SCREEN_WIDTH - 16;
  tft.fillCircle(hbX, 8, 4, TFT_GREEN);
  delay(100);
  tft.fillCircle(hbX, 8, 4, TFT_BLACK);
}

void showColumnTest(int col, const char* expectedKeys) {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(TFT_YELLOW);
  tft.print("COLUMN ");
  tft.print(col);
  tft.println(" TEST");
  tft.setTextColor(TFT_WHITE);
  tft.println("");
  tft.print("Expected: ");
  tft.println(expectedKeys);
  tft.println("");
  tft.println("Press any key...");
  tft.println("(10s timeout)");
}

void showPinStates(const byte* rowPins, const byte* colPins, int rowCount, int colCount) {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(TFT_YELLOW);
  tft.println("PIN INSPECTOR");
  tft.setTextColor(TFT_WHITE);
  tft.println("");
  
  tft.println("ROW PINS:");
  for (int i = 0; i < rowCount; i++) {
    int state = digitalRead(rowPins[i]);
    tft.print("R");
    tft.print(i);
    tft.print("(");
    tft.print(rowPins[i]);
    tft.print("): ");
    tft.setTextColor(state ? TFT_GREEN : TFT_RED);
    tft.println(state ? "HIGH" : "LOW");
    tft.setTextColor(TFT_WHITE);
  }
  
  tft.println("");
  tft.println("COL PINS:");
  for (int j = 0; j < colCount; j++) {
    int state = digitalRead(colPins[j]);
    tft.print("C");
    tft.print(j);
    tft.print("(");
    tft.print(colPins[j]);
    tft.print("): ");
    tft.setTextColor(state ? TFT_GREEN : TFT_RED);
    tft.println(state ? "HIGH" : "LOW");
    tft.setTextColor(TFT_WHITE);
  }
  
  tft.println("");
  tft.setTextColor(TFT_CYAN);
  tft.println("Press any key...");
}

void showTitle(const char* title) {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN);
  tft.println(title);
  tft.setTextSize(1);
  tft.println("");
}

void showMessage(const String& title, const String& message) {
  tft.fillRect(0, 80, SCREEN_WIDTH, 80, TFT_BLACK);
  tft.setCursor(LEFT_MARGIN, 80);
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN);
  tft.println(title);
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN);
  tft.println(message.substring(0, 20));
  tft.setTextSize(1);
}
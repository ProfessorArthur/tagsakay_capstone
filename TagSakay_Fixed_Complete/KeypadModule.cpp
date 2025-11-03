#include "KeypadModule.h"
#include "DisplayModule.h"
#include "NetworkModule.h"
#include "UARTModule.h"

// Keypad pin configuration
byte rowPins[KEYPAD_ROWS] = {32, 33, 25, 26};  // Rows 1-4
byte colPins[KEYPAD_COLS] = {4, 2, 15, 5};     // Cols 1-4

// Keypad layout
char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);

// Keypad state variables
String keypadBuffer = "";
bool keypadActive = false;
bool keypadMenuActive = false;
unsigned long keypadLastInput = 0;

void initializeKeypad() {
  Serial.println("Keypad initialized on pins 32,33,25,26 (rows) and 4,2,15,5 (cols)");
}

void handleKeypadInput() {
  char key = keypad.getKey();

  if (key) {
    Serial.print("Key pressed: ");
    Serial.println(key);

    keypadLastInput = millis();

    if (keypadMenuActive) {
      handleKeypadMenuSelection(key);
    } else if (key == 'A') {
      // 'A' key opens menu
      keypadMenuActive = true;
      showKeypadMenu();
    } else {
      processKeypadKey(key);
    }
  }
}

void processKeypadKey(char key) {
  if (key == '#') {
    // Confirm input
    if (keypadBuffer.length() > 0) {
      Serial.print("Processing keypad input: ");
      Serial.println(keypadBuffer);

      processQueueOverride(keypadBuffer);
      clearKeypadInput();
    }
  } else if (key == '*') {
    // Cancel input
    Serial.println("Keypad input cancelled");
    clearKeypadInput();
    indicateReady();
  } else if (isDigit(key)) {
    // Add digit to buffer
    keypadBuffer += key;
    keypadActive = true;

    Serial.print("Current buffer: ");
    Serial.println(keypadBuffer);

    displayKeypadPrompt("Enter Queue #:", keypadBuffer);
  }
}

void handleKeypadMenuSelection(char key) {
  switch (key) {
    case '1':
      Serial.println("Menu: Queue Override selected");
      keypadMenuActive = false;
      keypadActive = true;
      keypadBuffer = "";
      displayKeypadPrompt("Enter Queue #:", keypadBuffer);
      break;

    case '2':
      Serial.println("Menu: Clear Display selected");
      keypadMenuActive = false;
      sendToLEDMatrix("CLEAR", "", "");
      indicateReady();
      updateFooter("Display cleared via keypad");
      break;

    case '3':
      Serial.println("Menu: Test Display selected");
      keypadMenuActive = false;
      sendToLEDMatrix("TEST", "", "");
      updateStatusSection("DISPLAY TEST", TFT_CYAN);
      updateFooter("Testing LED matrix display");
      delay(2000);
      indicateReady();
      break;

    case '4':
      Serial.println("Menu: Device Status selected");
      keypadMenuActive = false;
      reportDeviceStatus("manual_status_check");
      updateStatusSection("STATUS SENT", TFT_GREEN);
      updateFooter("Device status reported");
      delay(1000);
      indicateReady();
      break;

    case '#':
      Serial.println("Menu: Exit");
      keypadMenuActive = false;
      indicateReady();
      break;

    default:
      Serial.println("Invalid menu selection");
      break;
  }
}

void processQueueOverride(const String& queueNumber) {
  Serial.print("Processing queue override for number: ");
  Serial.println(queueNumber);

  HTTPClient http;
  String url = String(serverConfig.baseUrl) + "/api/devices/" + deviceId + "/queue-override";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-api-key", serverConfig.apiKey);

  StaticJsonDocument<200> doc;
  doc["queueNumber"] = queueNumber.toInt();
  doc["reason"] = "Manual keypad override";

  String payload;
  serializeJson(doc, payload);

  Serial.println("Sending queue override request:");
  Serial.println(payload);

  int httpCode = http.POST(payload);

  if (httpCode > 0) {
    String response = http.getString();
    Serial.print("HTTP Response: ");
    Serial.println(httpCode);
    Serial.println(response);

    if (httpCode == 200) {
      updateStatusSection("OVERRIDE OK", TFT_GREEN);
      updateFooter("Queue override successful");
      sendToLEDMatrix("OVERRIDE", queueNumber, "");
    } else {
      updateStatusSection("OVERRIDE FAIL", TFT_RED);
      updateFooter("Queue override failed");
    }
  } else {
    Serial.print("HTTP Error: ");
    Serial.println(http.errorToString(httpCode));
    updateStatusSection("NET ERROR", TFT_RED);
    updateFooter("Network error during override");
  }

  http.end();
}

void clearKeypadInput() {
  keypadBuffer = "";
  keypadActive = false;
  keypadMenuActive = false;
}

bool checkKeypadTimeout(unsigned long currentMillis) {
  if (keypadActive && (currentMillis - keypadLastInput > KEY_INPUT_TIMEOUT)) {
    return true;
  }
  return false;
}

// KeypadModule Class Implementation
KeypadModule::KeypadModule() 
  : keypad(nullptr), keypadBuffer(""), keypadActive(false), 
    keypadMenuActive(false), keypadLastInput(0), lastKey(0), lastKeyTime(0) {
  
  // Initialize key layout
  char defaultKeys[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
  };
  
  memcpy(keys, defaultKeys, sizeof(keys));
}

bool KeypadModule::initialize() {
  Serial.println("[KEYPAD] Initializing...");
  
  rowPins = new byte[KEYPAD_ROWS]{32, 33, 25, 26};
  colPins = new byte[KEYPAD_COLS]{4, 2, 15, 5};
  
  setupPins();
  
  keypad = new Keypad(makeKeymap(keys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);
  keypad->setDebounceTime(50);
  
  Serial.println("[KEYPAD] Initialized successfully");
  return true;
}

void KeypadModule::setupPins() {
  for (int i = 0; i < KEYPAD_ROWS; i++) {
    pinMode(rowPins[i], INPUT_PULLUP);
  }
  for (int j = 0; j < KEYPAD_COLS; j++) {
    pinMode(colPins[j], OUTPUT);
    digitalWrite(colPins[j], HIGH);
  }
  delay(50);
}

char KeypadModule::getKey() {
  if (!keypad) {
    return 0;
  }
  
  char key = keypad->getKey();
  
  if (key) {
    unsigned long currentTime = millis();
    if (key != lastKey || (currentTime - lastKeyTime) > KEYPAD_DEBOUNCE_MS) {
      lastKey = key;
      lastKeyTime = currentTime;
      keypadLastInput = currentTime;
      return key;
    }
  }
  
  return 0;
}

void KeypadModule::reinitialize() {
  setupPins();
  if (keypad) {
    keypad->setDebounceTime(50);
  }
}

char KeypadModule::scanManual() {
  for (int col = 0; col < KEYPAD_COLS; col++) {
    for (int c = 0; c < KEYPAD_COLS; c++) {
      digitalWrite(colPins[c], (c == col) ? LOW : HIGH);
    }
    
    delayMicroseconds(100);
    
    for (int row = 0; row < KEYPAD_ROWS; row++) {
      if (digitalRead(rowPins[row]) == LOW) {
        // Reset all columns
        for (int c = 0; c < KEYPAD_COLS; c++) {
          digitalWrite(colPins[c], HIGH);
        }
        return keys[row][col];
      }
    }
  }
  
  // Reset all columns
  for (int c = 0; c < KEYPAD_COLS; c++) {
    digitalWrite(colPins[c], HIGH);
  }
  
  return 0;
}

bool KeypadModule::testColumn(int col, char* detectedKey) {
  if (col < 0 || col >= KEYPAD_COLS) {
    return false;
  }
  
  // Set all columns HIGH
  for (int c = 0; c < KEYPAD_COLS; c++) {
    digitalWrite(colPins[c], HIGH);
  }
  
  // Set target column LOW
  digitalWrite(colPins[col], LOW);
  
  unsigned long startTime = millis();
  while (millis() - startTime < TEST_MODE_TIMEOUT) {
    for (int row = 0; row < KEYPAD_ROWS; row++) {
      if (digitalRead(rowPins[row]) == LOW) {
        *detectedKey = keys[row][col];
        
        // Wait for key release
        while (digitalRead(rowPins[row]) == LOW) {
          delay(10);
        }
        
        // Reset columns
        for (int c = 0; c < KEYPAD_COLS; c++) {
          digitalWrite(colPins[c], HIGH);
        }
        
        return true;
      }
    }
    delay(50);
  }
  
  // Reset columns
  for (int c = 0; c < KEYPAD_COLS; c++) {
    digitalWrite(colPins[c], HIGH);
  }
  
  return false;
}

void KeypadModule::getPinStates(int* rowStates, int* colStates) {
  for (int i = 0; i < KEYPAD_ROWS; i++) {
    rowStates[i] = digitalRead(rowPins[i]);
  }
  
  for (int j = 0; j < KEYPAD_COLS; j++) {
    colStates[j] = digitalRead(colPins[j]);
  }
}

bool KeypadModule::testSwappedPins(const byte* altRowPins, const byte* altColPins) {
  Serial.println("[KEYPAD] Testing swapped pin configuration...");
  
  // Configure alternative pins
  for (int i = 0; i < KEYPAD_ROWS; i++) {
    pinMode(altRowPins[i], INPUT_PULLUP);
  }
  for (int j = 0; j < KEYPAD_COLS; j++) {
    pinMode(altColPins[j], OUTPUT);
    digitalWrite(altColPins[j], HIGH);
  }
  
  bool success = false;
  unsigned long startTime = millis();
  
  while (millis() - startTime < TEST_MODE_TIMEOUT && !success) {
    for (int col = 0; col < KEYPAD_COLS; col++) {
      for (int c = 0; c < KEYPAD_COLS; c++) {
        digitalWrite(altColPins[c], (c == col) ? LOW : HIGH);
      }
      delayMicroseconds(100);
      
      for (int row = 0; row < KEYPAD_ROWS; row++) {
        if (digitalRead(altRowPins[row]) == LOW) {
          char detectedKey = keys[row][col];
          Serial.print("[KEYPAD] Swapped config detected: ");
          Serial.println(detectedKey);
          success = true;
          break;
        }
      }
      if (success) break;
    }
    delay(10);
  }
  
  // Restore original configuration
  setupPins();
  
  return success;
}
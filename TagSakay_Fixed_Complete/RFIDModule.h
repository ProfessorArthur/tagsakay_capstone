#ifndef RFID_MODULE_H
#define RFID_MODULE_H

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include "Config.h"

class RFIDModule {
private:
  Adafruit_PN532* nfc;
  SPIClass* hspi;
  bool initialized;
  String lastScannedTag;
  unsigned long lastScanTime;
  int consecutiveFailures;
  
public:
  RFIDModule();
  
  bool initialize();
  bool isInitialized() const { return initialized; }
  
  // Scanning operations
  String readTag();
  bool scanWithDebounce(String& tagId, unsigned long debounceMs = RFID_DEBOUNCE_MS);
  
  // State management
  String getLastScannedTag() const { return lastScannedTag; }
  unsigned long getLastScanTime() const { return lastScanTime; }
  void clearLastScan() { lastScannedTag = ""; lastScanTime = 0; }
  int getConsecutiveFailures() const { return consecutiveFailures; }
  void resetFailureCount() { consecutiveFailures = 0; }
  
  // Diagnostics
  bool testConnection();
  String getFirmwareVersion();
};

// Legacy compatibility functions
extern Adafruit_PN532 nfc;
void initializeRFID();
String readRfidTag();
void handleRFIDLoop();

#endif // RFID_MODULE_H
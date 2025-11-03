#include "UARTHandler.h"
#include "DisplayModes.h"
#include "DisplayCore.h"

HardwareSerial RFIDSerial(2);
String messageBuffer = "";

void initializeUART() {
  Serial.println("Initializing UART communication...");
  RFIDSerial.begin(UART_BAUD, SERIAL_8N1, UART_RX, UART_TX);
  Serial.println("UART initialized - listening for commands");
}

void processUARTCommand() {
  while (RFIDSerial.available()) {
    char c = RFIDSerial.read();
    
    if (c == '\n') {
      if (messageBuffer.length() > 0) {
        parseCommand(messageBuffer);
        messageBuffer = "";
      }
    } else {
      messageBuffer += c;
      if (messageBuffer.length() > 256) {
        Serial.println("Buffer overflow - clearing");
        messageBuffer = "";
      }
    }
  }
}

void parseCommand(String command) {
  Serial.print("Received: ");
  Serial.println(command);
  
  int firstDelim = command.indexOf('|');
  int secondDelim = command.indexOf('|', firstDelim + 1);
  
  if (firstDelim == -1) {
    Serial.println("Invalid command format");
    return;
  }
  
  String cmd = command.substring(0, firstDelim);
  String data1 = "";
  String data2 = "";
  
  if (secondDelim != -1) {
    data1 = command.substring(firstDelim + 1, secondDelim);
    data2 = command.substring(secondDelim + 1);
  } else {
    data1 = command.substring(firstDelim + 1);
  }
  
  cmd.trim();
  data1.trim();
  data2.trim();
  
  handleCommand(cmd, data1, data2);
  sendACK(cmd);
}

void handleCommand(String cmd, String data1, String data2) {
  Serial.println("CMD: " + cmd + " | D1: " + data1 + " | D2: " + data2);
  
  if (cmd == "INIT") {
    deviceId = data1;
    location = data2;
    Serial.println("Initialized - Device: " + deviceId + " | Location: " + location);
    displayWelcomeScreen();
    delay(1500);
    displayIdleScreen();
    
  } else if (cmd == "STATUS") {
    uint16_t color = COLOR_INFO;
    if (data1 == "READY") color = COLOR_READY;
    else if (data1 == "ERROR") color = COLOR_ERROR;
    else if (data1 == "UNREGISTERED") color = COLOR_WARNING;
    displayStatus(data1, color);
    
  } else if (cmd == "QUEUE") {
    int queueNum = data1.toInt();
    displayQueueNumber(queueNum, data2);
    
  } else if (cmd == "CASCADE") {
    int queueNums[40];
    int count = 0;
    int startPos = 0;
    
    while (startPos < data1.length() && count < 40) {
      int commaPos = data1.indexOf(',', startPos);
      String numStr;
      
      if (commaPos == -1) {
        numStr = data1.substring(startPos);
        startPos = data1.length();
      } else {
        numStr = data1.substring(startPos, commaPos);
        startPos = commaPos + 1;
      }
      
      numStr.trim();
      if (numStr.length() > 0) {
        queueNums[count++] = numStr.toInt();
      }
    }
    
    if (count > 0) {
      displayCascade(queueNums, count);
    }
    
  } else if (cmd == "OVERRIDE") {
    int queueNum = data1.toInt();
    displayQueueNumber(queueNum, "OVERRIDE: " + data2);
    
  } else if (cmd == "CLEAR") {
    clearDisplay();
    displayIdleScreen();
    
  } else if (cmd == "SCAN") {
    displayScanResult(data1, data2);
    
  } else if (cmd == "MESSAGE") {
    displayMessage(data1, COLOR_INFO);
    
  } else if (cmd == "ERROR") {
    displayError(data1, data2);
    
  } else if (cmd == "TEST") {
    displayTestPattern();
    
  } else if (cmd == "BEEP") {
    dma_display->fillRect(0, 0, 4, 4, COLOR_YELLOW);
    dma_display->flipDMABuffer();
    delay(50);
    dma_display->fillRect(0, 0, 4, 4, COLOR_BLACK);
    dma_display->flipDMABuffer();
    
  } else if (cmd == "REFRESH") {
    updateDisplay();
    
  } else if (cmd == "BRIGHTNESS") {
    int level = data1.toInt();
    if (level >= 0 && level <= 255) {
      setBrightness(level);
      Serial.println("Brightness set to: " + String(level));
    }
    
  } else {
    Serial.println("Unknown command: " + cmd);
  }
}

void sendACK(String command) {
  RFIDSerial.println("ACK|" + command + "|OK");
}
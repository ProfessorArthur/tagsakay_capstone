#include "DisplayModes.h"
#include "DisplayCore.h"
#include "PixelFont.h"
#include "Animations.h"

void displayIdleScreen() {
  currentDisplay.mode = MODE_IDLE;
  currentDisplay.startTime = millis();
  currentDisplay.duration = 0;
  currentDisplay.scrolling = false;
  
  dma_display->fillScreen(COLOR_BLACK);
  drawBorder(COLOR_BLUE);
  
  dma_display->setTextSize(1);
  dma_display->setTextColor(COLOR_CYAN);
  drawCenteredText("TagSakay", 10, COLOR_CYAN, 1);
  drawCenteredText("RFID System", 25, COLOR_WHITE, 1);
  
  if (location.length() > 0) {
    drawCenteredText(location, 40, COLOR_GREEN, 1);
  }
  
  drawCenteredText("Ready", 55, COLOR_READY, 1);
  
  dma_display->flipDMABuffer();
  Serial.println("Display: Idle screen");
}

void displayQueueNumber(int queueNumber, String name) {
  currentDisplay.mode = MODE_QUEUE;
  currentDisplay.queueNumber = queueNumber;
  currentDisplay.primaryText = name;
  currentDisplay.startTime = millis();
  currentDisplay.duration = QUEUE_DISPLAY_DURATION;
  currentDisplay.scrolling = (name.length() > 10);
  currentDisplay.scrollPosition = PANEL_RES_X;
  
  dma_display->fillScreen(COLOR_BLACK);
  animateTransition();
  drawBorder(COLOR_GREEN);
  
  drawCenteredText("NOW SERVING", 5, COLOR_YELLOW, 1);
  drawLargePixelNumber(10, 20, queueNumber, COLOR_GREEN);
  
  if (currentDisplay.scrolling) {
    drawScrollingText(name, 50, COLOR_WHITE);
  } else {
    drawCenteredText(name, 50, COLOR_WHITE, 1);
  }
  
  dma_display->flipDMABuffer();
  Serial.println("Display: Queue #" + String(queueNumber) + " - " + name);
}

void displayCascade(int* queueNumbers, int numQueues) {
  currentDisplay.mode = MODE_CASCADE;
  currentDisplay.queueList = queueNumbers;
  currentDisplay.queueListSize = numQueues;
  currentDisplay.startTime = millis();
  currentDisplay.duration = 0;
  
  dma_display->fillScreen(COLOR_BLACK);
  
  int queueIndex = 0;
  int yPos = START_Y;
  int rowCount = 0;
  
  while (queueIndex < numQueues && rowCount < MAX_ROWS) {
    int xPos = MARGIN;
    
    for (int col = 0; col < NUMBERS_PER_ROW && queueIndex < numQueues; col++) {
      int queueNum = queueNumbers[queueIndex];
      uint16_t color = getQueueColor(queueNum);
      
      drawPixelNumber(xPos, yPos, queueNum, color);
      xPos += (DIGIT_WIDTH * 2);
      
      if (col < NUMBERS_PER_ROW - 1 && queueIndex < numQueues - 1) {
        xPos += 1;
        drawPixelPipe(xPos, yPos, COLOR_WHITE);
        xPos += PIPE_WIDTH;
      }
      
      queueIndex++;
    }
    
    yPos += ROW_HEIGHT;
    rowCount++;
  }
  
  dma_display->flipDMABuffer();
  delay(1000 / 60);
  
  Serial.print("Display: Cascade - ");
  Serial.print(numQueues);
  Serial.println(" queue numbers");
}

void displayStatus(String status, uint16_t color) {
  currentDisplay.mode = MODE_STATUS;
  currentDisplay.primaryText = status;
  currentDisplay.color = color;
  currentDisplay.startTime = millis();
  currentDisplay.duration = MESSAGE_DURATION;
  currentDisplay.scrolling = false;
  
  dma_display->fillScreen(COLOR_BLACK);
  drawBorder(color);
  drawCenteredText(status, 28, color, 1);
  
  dma_display->flipDMABuffer();
  Serial.println("Display: Status - " + status);
}

void displayMessage(String message, uint16_t color) {
  currentDisplay.mode = MODE_MESSAGE;
  currentDisplay.primaryText = message;
  currentDisplay.color = color;
  currentDisplay.startTime = millis();
  currentDisplay.duration = MESSAGE_DURATION;
  currentDisplay.scrolling = (message.length() > 10);
  currentDisplay.scrollPosition = PANEL_RES_X;
  
  dma_display->fillScreen(COLOR_BLACK);
  drawBorder(color);
  
  if (currentDisplay.scrolling) {
    drawScrollingText(message, 28, color);
  } else {
    drawCenteredText(message, 28, color, 1);
  }
  
  dma_display->flipDMABuffer();
  Serial.println("Display: Message - " + message);
}

void displayScanResult(String name, String eventType) {
  currentDisplay.mode = MODE_SCAN;
  currentDisplay.primaryText = name;
  currentDisplay.secondaryText = eventType;
  currentDisplay.startTime = millis();
  currentDisplay.duration = MESSAGE_DURATION;
  currentDisplay.scrolling = (name.length() > 10);
  currentDisplay.scrollPosition = PANEL_RES_X;
  
  dma_display->fillScreen(COLOR_BLACK);
  animateSuccess();
  drawBorder(COLOR_SUCCESS);
  
  uint16_t eventColor = COLOR_GREEN;
  if (eventType.indexOf("SUCCESS") >= 0) eventColor = COLOR_GREEN;
  else if (eventType.indexOf("OUT") >= 0) eventColor = COLOR_ORANGE;
  else if (eventType.indexOf("IN") >= 0) eventColor = COLOR_CYAN;
  
  drawCenteredText(eventType, 8, eventColor, 1);
  
  dma_display->fillRect(28, 20, 8, 3, COLOR_GREEN);
  dma_display->fillRect(32, 23, 3, 8, COLOR_GREEN);
  
  if (currentDisplay.scrolling) {
    drawScrollingText(name, 40, COLOR_WHITE);
  } else {
    drawCenteredText(name, 40, COLOR_WHITE, 1);
  }
  
  dma_display->flipDMABuffer();
  Serial.println("Display: Scan - " + name + " | " + eventType);
}

void displayError(String errorType, String message) {
  currentDisplay.mode = MODE_ERROR;
  currentDisplay.primaryText = errorType;
  currentDisplay.secondaryText = message;
  currentDisplay.startTime = millis();
  currentDisplay.duration = MESSAGE_DURATION;
  currentDisplay.scrolling = false;
  
  dma_display->fillScreen(COLOR_BLACK);
  drawBorder(COLOR_ERROR);
  
  dma_display->drawLine(26, 10, 38, 22, COLOR_ERROR);
  dma_display->drawLine(38, 10, 26, 22, COLOR_ERROR);
  dma_display->drawLine(27, 10, 39, 22, COLOR_ERROR);
  dma_display->drawLine(39, 10, 27, 22, COLOR_ERROR);
  
  drawCenteredText(errorType, 30, COLOR_ERROR, 1);
  
  if (message.length() > 0) {
    drawCenteredText(message.substring(0, 10), 45, COLOR_YELLOW, 1);
  }
  
  dma_display->flipDMABuffer();
  Serial.println("Display: Error - " + errorType + " | " + message);
}

void displayTestPattern() {
  currentDisplay.mode = MODE_TEST;
  currentDisplay.startTime = millis();
  currentDisplay.duration = 5000;
  
  dma_display->fillScreen(COLOR_BLACK);
  
  drawPixelNumber(5, 5, 12, COLOR_WHITE);
  drawPixelNumber(20, 5, 34, COLOR_AMBER);
  drawPixelNumber(35, 5, 56, COLOR_GREEN);
  
  drawLargePixelNumber(5, 20, 78, COLOR_CYAN);
  drawLargePixelNumber(5, 40, 90, COLOR_MAGENTA);
  
  drawCenteredText("TEST PATTERN", 58, COLOR_WHITE, 1);
  
  dma_display->flipDMABuffer();
  Serial.println("Display: Test pattern");
}

void displayWelcomeScreen() {
  dma_display->fillScreen(COLOR_BLACK);
  drawBorder(COLOR_CYAN);
  
  dma_display->setTextSize(2);
  dma_display->setTextColor(COLOR_CYAN);
  drawCenteredText("TagSakay", 15, COLOR_CYAN, 2);
  
  dma_display->setTextSize(1);
  drawCenteredText("RFID System", 35, COLOR_WHITE, 1);
  drawCenteredText("Initializing...", 50, COLOR_GREEN, 1);
  
  for (int i = 0; i <= 100; i += 10) {
    drawProgressBar(i, 58, COLOR_GREEN);
    dma_display->flipDMABuffer();
    delay(100);
  }
  
  Serial.println("Display: Welcome screen");
}
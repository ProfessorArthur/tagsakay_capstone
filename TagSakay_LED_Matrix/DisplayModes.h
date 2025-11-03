#ifndef DISPLAY_MODES_H
#define DISPLAY_MODES_H

#include <Arduino.h>
#include "Config.h"

// Display mode functions
void displayIdleScreen();
void displayQueueNumber(int queueNumber, String name);
void displayCascade(int* queueNumbers, int numQueues);
void displayStatus(String status, uint16_t color);
void displayMessage(String message, uint16_t color);
void displayScanResult(String name, String eventType);
void displayError(String errorType, String message);
void displayTestPattern();
void displayWelcomeScreen();

#endif // DISPLAY_MODES_H
#ifndef DISPLAY_CORE_H
#define DISPLAY_CORE_H

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <ESP32-VirtualMatrixPanel-I2S-DMA.h>
#include "Config.h"

// Matrix hardware objects
extern MatrixPanel_I2S_DMA *dma_display;
extern VirtualMatrixPanel *virtualDisp;

// Core display functions
void initializeMatrix();
void clearDisplay();
void setBrightness(uint8_t level);
void updateDisplay();

// Border and drawing utilities
void drawBorder(uint16_t color);
void drawProgressBar(int progress, int y, uint16_t color);
void drawCenteredText(String text, int y, uint16_t color, uint8_t textSize = 1);
void drawScrollingText(String text, int y, uint16_t color);

// Color utility
uint16_t getQueueColor(int queueNum);

#endif // DISPLAY_CORE_H
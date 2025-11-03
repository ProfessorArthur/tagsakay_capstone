#include "PixelFont.h"
#include "DisplayCore.h"

const byte DIGIT_PATTERNS[10][5] = {
  {0b111, 0b101, 0b101, 0b101, 0b111},  // 0
  {0b010, 0b110, 0b010, 0b010, 0b111},  // 1
  {0b111, 0b001, 0b111, 0b100, 0b111},  // 2
  {0b111, 0b001, 0b111, 0b001, 0b111},  // 3
  {0b101, 0b101, 0b111, 0b001, 0b001},  // 4
  {0b111, 0b100, 0b111, 0b001, 0b111},  // 5
  {0b111, 0b100, 0b111, 0b101, 0b111},  // 6
  {0b111, 0b001, 0b010, 0b010, 0b010},  // 7
  {0b111, 0b101, 0b111, 0b101, 0b111},  // 8
  {0b111, 0b101, 0b111, 0b001, 0b111}   // 9
};

void drawPixelDigit(int x, int y, int digit, uint16_t color) {
  if (digit < 0 || digit > 9) return;
  
  for (int row = 0; row < 5; row++) {
    byte pattern = DIGIT_PATTERNS[digit][row];
    for (int col = 0; col < 3; col++) {
      if (pattern & (1 << (2 - col))) {
        dma_display->drawPixel(x + col, y + row, color);
      } else {
        dma_display->drawPixel(x + col, y + row, COLOR_BLACK);
      }
    }
  }
}

void drawPixelNumber(int x, int y, int number, uint16_t color) {
  if (number < 10) {
    // Clear tens place
    for (int row = 0; row < 5; row++) {
      for (int col = 0; col < 3; col++) {
        dma_display->drawPixel(x + col, y + row, COLOR_BLACK);
      }
    }
    // Draw ones place right-aligned
    drawPixelDigit(x + DIGIT_WIDTH, y, number, color);
  } else {
    int tens = number / 10;
    int ones = number % 10;
    drawPixelDigit(x, y, tens, color);
    drawPixelDigit(x + DIGIT_WIDTH, y, ones, color);
  }
}

void drawLargePixelNumber(int x, int y, int number, uint16_t color) {
  int scale = 3;
  
  if (number < 10) {
    for (int row = 0; row < 5; row++) {
      byte pattern = DIGIT_PATTERNS[number][row];
      for (int col = 0; col < 3; col++) {
        if (pattern & (1 << (2 - col))) {
          dma_display->fillRect(
            x + col * scale + 6 * scale,
            y + row * scale,
            scale, scale, color
          );
        }
      }
    }
  } else {
    int tens = number / 10;
    int ones = number % 10;
    
    for (int row = 0; row < 5; row++) {
      byte pattern = DIGIT_PATTERNS[tens][row];
      for (int col = 0; col < 3; col++) {
        if (pattern & (1 << (2 - col))) {
          dma_display->fillRect(x + col * scale, y + row * scale, scale, scale, color);
        }
      }
    }
    
    for (int row = 0; row < 5; row++) {
      byte pattern = DIGIT_PATTERNS[ones][row];
      for (int col = 0; col < 3; col++) {
        if (pattern & (1 << (2 - col))) {
          dma_display->fillRect(x + col * scale + 4 * scale, y + row * scale, scale, scale, color);
        }
      }
    }
  }
}

void drawPixelPipe(int x, int y, uint16_t color) {
  for (int i = 0; i < 5; i++) {
    dma_display->drawPixel(x, y + i, color);
  }
}
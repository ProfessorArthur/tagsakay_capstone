#ifndef UART_HANDLER_H
#define UART_HANDLER_H

#include <Arduino.h>
#include "Config.h"

extern HardwareSerial RFIDSerial;
extern String messageBuffer;

void initializeUART();
void processUARTCommand();
void parseCommand(String command);
void handleCommand(String cmd, String data1, String data2);
void sendACK(String command);

#endif // UART_HANDLER_H
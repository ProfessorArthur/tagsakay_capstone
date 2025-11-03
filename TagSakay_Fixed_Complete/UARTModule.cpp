#include "UARTModule.h"

HardwareSerial UARTSerial(2);  // Use UART2 (GPIO16 RX, GPIO17 TX)

void initializeUART() {
  UARTSerial.begin(UART_BAUD, SERIAL_8N1, UART_RX, UART_TX);
  Serial.println("UART initialized for LED Matrix communication");
  Serial.print("UART TX: GPIO");
  Serial.print(UART_TX);
  Serial.print(", RX: GPIO");
  Serial.println(UART_RX);
  Serial.print("Baud rate: ");
  Serial.println(UART_BAUD);
}

void sendToLEDMatrix(const String& command, const String& param1, const String& param2) {
  String message = command + "|" + param1 + "|" + param2 + "\n";

  Serial.print("Sending to LED Matrix: ");
  Serial.println(message);

  UARTSerial.print(message);
  UARTSerial.flush();  // Wait for transmission to complete

  delay(50);  // Small delay to ensure message is sent
}
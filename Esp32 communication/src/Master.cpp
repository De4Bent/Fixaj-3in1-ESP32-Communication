#include "LoRa_E22.h"
#include <HardwareSerial.h>

#define M0 32
#define M1 33
#define RX 27
#define TX 35
#define BUTTON_PIN 14  // Emergency stop button pin

HardwareSerial fixajSerial(1);
LoRa_E22 e22(TX, RX, &fixajSerial, UART_BPS_RATE_9600);

void setup() {
  pinMode(M0, OUTPUT);
  pinMode(M1, OUTPUT);
  digitalWrite(M0, LOW);
  digitalWrite(M1, LOW);

  pinMode(BUTTON_PIN, INPUT_PULLUP); // button should be connected to ground when pressed 

  Serial.begin(9600);
  delay(500);
  e22.begin();
  delay(500);

  Serial.println("Master's ready.");
}

void loop() {
  static bool lastButtonState = HIGH;
  bool buttonState = digitalRead(BUTTON_PIN);

  if (buttonState == LOW && lastButtonState == HIGH) {
    const char msg[] = "STOP";
    ResponseStatus rs = e22.sendFixedMessage(0, 1, 18, (void*)msg, sizeof(msg));
    Serial.print("Sent STOP message, status: ");
    Serial.println(rs.getResponseDescription());
    delay(500);
  }

  lastButtonState = buttonState;
}

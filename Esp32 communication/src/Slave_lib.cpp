#include "LoRa_E22.h"
#include <Arduino.h>

#define M0_PIN        32
#define M1_PIN        33
#define LORA_RX_PIN   27
#define LORA_TX_PIN   35
#define LED1_PIN      12

HardwareSerial LoRaSerial(1);
LoRa_E22 e22(LORA_TX_PIN, LORA_RX_PIN, &LoRaSerial, UART_BPS_RATE_9600);

bool leds_on = false;

void setup() {
  pinMode(M0_PIN, OUTPUT);
  pinMode(M1_PIN, OUTPUT);
  digitalWrite(M0_PIN, LOW);
  digitalWrite(M1_PIN, LOW);

  pinMode(LED1_PIN, OUTPUT);
  digitalWrite(LED1_PIN, HIGH); // If you are using NC type of relay switch it to LOW

  Serial.begin(115200);
  LoRaSerial.begin(9600, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);
  e22.begin();
}

void loop() {
  if (e22.available() > 1) {
    ResponseStructContainer rsc = e22.receiveMessage(32);
    if (rsc.status.code == SUCCESS) {
      char* msg = (char*)rsc.data;
      Serial.print("Slave: received something: ");
      Serial.println(msg);

      if (strcmp(msg, "STOP") == 0) {
        leds_on = !leds_on;
        digitalWrite(LED1_PIN, leds_on ? LOW : HIGH);
        Serial.println(leds_on ? "Slave: Relay OFF" : "Slave: LEDs ON");

        const char ack[] = "ACK:STOP";
        e22.sendFixedMessage(0, 0, 18, (void*)ack, strlen(ack));
      }
    }
    rsc.close();
  }
}

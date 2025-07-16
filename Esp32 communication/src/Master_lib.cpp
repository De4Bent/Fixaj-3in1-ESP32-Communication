#include "LoRa_E22.h"
#include <Arduino.h>

#define M0_PIN        32
#define M1_PIN        33
#define LORA_RX_PIN   27
#define LORA_TX_PIN   35
#define BUTTON_PIN    14

HardwareSerial LoRaSerial(1);
LoRa_E22 e22(LORA_TX_PIN, LORA_RX_PIN, &LoRaSerial, UART_BPS_RATE_9600);

bool waiting_ack = false;
unsigned long last_send_time = 0;
unsigned long cooldown_start = 0;
bool in_cooldown = false;

long ack_timeout = 2000;
int max_retries = 3;
int retry_count = 0;
char* command = "STOP";

void setup() {
  pinMode(M0_PIN, OUTPUT);
  pinMode(M1_PIN, OUTPUT);
  digitalWrite(M0_PIN, LOW);
  digitalWrite(M1_PIN, LOW);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  LoRaSerial.begin(9600, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);
  e22.begin();
}

void send_command() {
  ResponseStatus status = e22.sendFixedMessage(0, 1, 18, (void*)command, strlen(command));
  last_send_time = millis();
  waiting_ack = true;
  retry_count++;

  Serial.print("Master: sent command: ");
  Serial.println(command);

  if (status.code != SUCCESS) {
    Serial.println("Master: failed to send :( :  .");
  }
}

void loop() {
  if (in_cooldown && millis() - cooldown_start >= 5000) {
    in_cooldown = false;
  }

  if (!in_cooldown && !waiting_ack && digitalRead(BUTTON_PIN) == LOW) {
    delay(20);
    if (digitalRead(BUTTON_PIN) == LOW) {
      retry_count = 0;
      send_command();
      in_cooldown = true;
      cooldown_start = millis();
    }
  }

  if (waiting_ack && e22.available() > 1) {
    ResponseStructContainer rsc = e22.receiveMessage(32);
    if (rsc.status.code == SUCCESS) {
      char* msg = (char*)rsc.data;
      Serial.print("Master: received: ");
      Serial.println(msg);
      if (strncmp(msg, "ACK:", 4) == 0) {
        waiting_ack = false;
        retry_count = 0;
        Serial.println("Master: ACK received.");
      }
    }
    rsc.close();
  }

  if (waiting_ack && (millis() - last_send_time > ack_timeout)) {
    if (retry_count < max_retries) {
      Serial.println("Master: ACK timeout. retrying...."); // great design choice with(...) to show that ACK is trying to get a massage. Such a smart design choice, isn't? 
      send_command();
    } else {
      Serial.println("Master: max retries reached. Rest in peace .");
      waiting_ack = false;
    }
  }
}

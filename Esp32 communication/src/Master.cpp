#include "LoRa_E22.h"
#define M0_PIN 32
#define M1_PIN 33
#define LORA_RX_PIN 16 // UART1 RX
#define LORA_TX_PIN 17 // UART1 TX
#define BUTTON_PIN 14 // Emergency Stop button

HardwareSerial LoRaSerial(1);  // UART1

LoRa_E22 e22(LORA_TX_PIN, LORA_RX_PIN, &LoRaSerial, UART_BPS_RATE_9600);

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
bool lastButtonState = HIGH;

bool waitingAck = false;
unsigned long lastSendTime = 0;
long ackTimeout = 2000; //millis
int count = 0;//retry count
int maxRetries = 3; // amount of tries for recieveng the signal

char* command = "STOP"; //Transfer Data

void sendCommand() {
  ResponseStatus status = e22.sendFixedMessage(0, 1, 18, (void*)command, strlen(command));

  lastSendTime = millis();
  waitingAck = true;
  count++;

  Serial.print("Мастер: отправлено сообщение:  ");
  Serial.print(command);
  Serial.print(" (попытка ");
  Serial.print(count);
  Serial.println(")");

  if (status.code != SUCCESS) {
    Serial.println("Мастер: Ошибка при отправке!");
  }
}



void setup() {
  pinMode(M0_PIN, OUTPUT);
  pinMode(M1_PIN, OUTPUT);
  digitalWrite(M0_PIN, LOW);
  digitalWrite(M1_PIN, LOW);  // normal mode 

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(115200);
  delay(500);

  LoRaSerial.begin(9600, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);

  bool xD = e22.begin();
/*  if (!xD) {
    Serial.println("Мастер: Не удалось инициализировать E22!");
  } else {
    Serial.println("Maстер: Модуль LoRa инициализировано успешно");
  } */
}

void loop() {
  bool reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW && lastButtonState == HIGH && !waitingAck) {
      count = 0;
      sendCommand();
    }
  }
  lastButtonState = reading;

  if (waitingAck && e22.available() > 1) {
    ResponseStructContainer rsc = e22.receiveMessage(32);
    if (rsc.status.code == SUCCESS) {
      char* resp = (char*)rsc.data;
      Serial.print(" Получено сообщение: ");
      Serial.println(resp);

      if (strncmp(resp, "ACK:", 4) == 0) {
        waitingAck = false;
        Serial.println("Мастер: ACK получено.");
        count = 0;
      }
    }
    rsc.close();
  }

  if (waitingAck && (millis() - lastSendTime > ackTimeout)) {
    if (count < maxRetries) {
      Serial.println("Мастер: время для получаения АСК истекло, повторная отправка....");
    } else {
      Serial.println("Мастер: достигнуто максимальное колличество отправок. АСК не получено");
      waitingAck = false;
      count = 0;
    }
  }
}

#include <Arduino.h>

#define M0_PIN 32
#define M1_PIN 33
#define LORA_RX_PIN 27
#define LORA_TX_PIN 35
#define BUTTON_PIN 14

HardwareSerial LoRaSerial(1); // UART1

char* command = "STOP\n";
char* expectedAck = "ACK:STOP";

unsigned long last_debounce = 0;
unsigned long debounce_delay = 50;
bool last_butt_state = HIGH;

bool wait_ack = false;
unsigned long last_send_time = 0;
int retry_count = 0;
int max_tries = 3; // amount of tries ACK will try to get a response from reciever
unsigned long ack_timeout = 2000; // ACK wait time(millis)

// Lockout after button press
bool input_lock = false;
unsigned long lockout_start = 0;
const unsigned long lockout_dur = 5000; // the time of button input delay after first press(millis)

void setup() {
  pinMode(M0_PIN, OUTPUT);
  pinMode(M1_PIN, OUTPUT);
  digitalWrite(M0_PIN, LOW);
  digitalWrite(M1_PIN, LOW); // normal mode 

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.begin(115200); 
  delay(500);

  LoRaSerial.begin(9600, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);
  Serial.println("Master: HAZIR AMK!!!!!");
}

void sendCommand() {
  LoRaSerial.print(command);
  last_send_time = millis();
  wait_ack = true;
  retry_count++;

  Serial.printf("Master: sent '%s' (try %d)\n", command, retry_count);

  // start 5 second lockout
  input_lock = true;
  lockout_start = millis();
}

void loop() {
  if (input_lock && (millis() - lockout_start >= lockout_dur)) {
    input_lock = false;
    Serial.println("Master: Button input reenabled.");
  }

  bool reading = digitalRead(BUTTON_PIN);

  if (reading != last_butt_state) {
    last_debounce = millis();
  }

  if ((millis() - last_debounce) > debounce_delay) {
    if (reading == LOW && last_butt_state == HIGH && !wait_ack && !input_lock) {
      retry_count = 0;
      sendCommand();
    }
  }
  last_butt_state = reading;

  if (wait_ack && LoRaSerial.available()) {
    String resp = LoRaSerial.readStringUntil('\n');
    resp.trim();

    Serial.printf("Master: received: '%s'\n", resp.c_str());

    if (resp == expectedAck) {
      Serial.println("Master: ACK received.");
      wait_ack = false;
      retry_count = 0;
    }
  }

  if (wait_ack && (millis() - last_send_time > ack_timeout)) {
    if (retry_count < max_tries) {
      Serial.println("Master: ACK timeout, retrying...");
      sendCommand();
    } else {
      Serial.println("Master: max retries reached. I give up folks");
      wait_ack = false;
      retry_count = 0;
    }
  }
}

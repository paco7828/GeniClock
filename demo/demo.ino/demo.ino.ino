#include <Wire.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <RTClib.h>  // Adafruit RTC library

// Shift register control pins (connected to ESP32 GPIOs)
const byte SER = 10;
const byte SRCLK = 8;
const byte RCLK = 9;

// Button and buzzer pins
const byte buttonPins[4] = {0, 1, 2, 3};
const byte buzzerPin = 4;

// GPS and RTC
TinyGPSPlus gps;
HardwareSerial GPSserial(1);  // UART1 for GPS
RTC_PCF8563 rtc;

unsigned long lastDisplayUpdate = 0;
int displayState = 0;  // 0: GPS time, 1: lat, 2: lon, 3: RTC time

void setup() {
  // Shift register pins
  pinMode(SER, OUTPUT);
  pinMode(SRCLK, OUTPUT);
  pinMode(RCLK, OUTPUT);

  // Buttons and buzzer
  for (int i = 0; i < 4; i++) pinMode(buttonPins[i], INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  // Serial + GPS + I2C
  Serial.begin(115200);
  GPSserial.begin(9600, SERIAL_8N1, 20, -1);  // GPS RX = GPIO20
  Wire.begin(6, 7);  // RTC SDA, SCL

  if (!rtc.begin()) {
    Serial.println("RTC not found!");
    displayText("RTC FAIL");
  }

  resetDisplay();
}

void loop() {
  while (GPSserial.available()) {
    gps.encode(GPSserial.read());
  }

  // Check button presses to trigger actions or state changes
  handleButtons();

  // Update display every 2 seconds
  if (millis() - lastDisplayUpdate > 2000) {
    lastDisplayUpdate = millis();

    if (gps.location.isValid() && gps.time.isValid()) {
      switch (displayState) {
        case 0:
          {
            char buf[9];
            int hour = (gps.time.hour() + 2) % 24;  // UTC+2
            snprintf(buf, sizeof(buf), "%02d:%02d:%02d", hour, gps.time.minute(), gps.time.second());
            displayText(buf);
            break;
          }
        case 1:
          {
            char buf[9];
            dtostrf(gps.location.lat(), 8, 2, buf);
            displayText(buf);
            break;
          }
        case 2:
          {
            char buf[9];
            dtostrf(gps.location.lng(), 8, 2, buf);
            displayText(buf);
            break;
          }
        case 3:
          {
            DateTime now = rtc.now();
            char buf[9];
            snprintf(buf, sizeof(buf), "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
            displayText(buf);
            break;
          }
      }
      displayState = (displayState + 1) % 4;
    } else {
      displayText("NO GPS  ");
    }
  }
}

void handleButtons() {
  static bool prevState[4] = {true, true, true, true};  // pull-ups

  for (int i = 0; i < 4; i++) {
    bool curr = digitalRead(buttonPins[i]);
    if (!curr && prevState[i]) {
      // Button i pressed
      Serial.printf("Button %d pressed\n", i + 1);
      digitalWrite(buzzerPin, HIGH);
      delay(100);
      digitalWrite(buzzerPin, LOW);

      // Optional: perform specific actions per button
      // Example: button 1 resets displayState
      if (i == 0) displayState = 0;
    }
    prevState[i] = curr;
  }
}

// ---------------------- Display Control ----------------------

void resetDisplay() {
  byte reg2 = 0b00000000;  // RST=0
  byte reg1 = 0b00000001;  // CE=1 (disabled)

  shiftOutRegisters(reg2, reg1);
  delayMicroseconds(10);

  reg2 = 0b00000001;  // RST=1
  shiftOutRegisters(reg2, reg1);
  delayMicroseconds(150);
}

void displayText(char* text) {
  char buffer[9];
  for (int i = 0; i < 8; i++) {
    buffer[i] = (text[i] != '\0') ? text[i] : ' ';
  }
  buffer[8] = '\0';
  sendToDisplay(buffer);
}

void sendToDisplay(char* data) {
  for (int addr = 0; addr < 8; addr++) {
    char ch = data[addr];
    if (ch == '\0') ch = ' ';

    byte reg2 = 0b00000001 | ((addr & 0x07) << 1);
    byte reg1 = (ch & 0x7F) << 1;
    reg1 &= 0xFE;  // CE=0 (active)

    shiftOutRegisters(reg2, reg1);
    delay(2);

    reg1 |= 0x01;  // CE=1 (inactive)
    shiftOutRegisters(reg2, reg1);
    delay(2);
  }
}

void shiftOutRegisters(byte reg2, byte reg1) {
  digitalWrite(RCLK, LOW);
  shiftOut(SER, SRCLK, MSBFIRST, reg2);
  shiftOut(SER, SRCLK, MSBFIRST, reg1);
  digitalWrite(RCLK, HIGH);
}

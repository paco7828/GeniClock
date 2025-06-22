#include "constants.h"
#include "HDSPDisplay.h"
#include "gps.h"
#include "ntp.h"
#include <Wire.h>
#include <RTClib.h>

// Display
HDSPDisplay HDSP(SER, SRCLK, RCLK);

// GPS
GPS gps;

// NTP
NTP ntp(SSID, PASSW);

// RTC
RTC_DS3231 rtc;

void setup() {
  // Display
  HDSP.begin();

  // Buttons
  pinMode(CONFIRM_BTN, INPUT_PULLUP);
  pinMode(ADD_BTN, INPUT_PULLUP);
  pinMode(SUBTRACT_BTN, INPUT_PULLUP);
  pinMode(CANCEL_BTN, INPUT_PULLUP);

  // Buzzer
  pinMode(BUZZER, OUTPUT);
  noTone(BUZZER);

  // GPS
  gps.begin(GPS_RX);

  // NTP
  ntp.begin();

  // RTC
  Wire.begin(RTC_SDA, RTC_SCL);
  if (!rtc.begin()) {
    HDSP.displayText("RTC FAIL");
    delay(1500);
  }

  // Turn off display
  HDSP.resetDisplay();

  // Default display text
  HDSP.displayText("  GENI  ");
}

void displayTime(String hour, String minute, String second) {
  String text = hour + ':' + minute + ':' + second;
  HDSP.displayText(text);
  // add 0 formatting
}

void displayDate(String year, String month, String day) {
  // add 0 formatting
}

void loop() {
  // GPS time
  gps.update();
  if (gps.hasFix()) {
    HDSP.displayText("GPS");
    delay(1000);
    HDSP.displayText("");
  } else {
    Serial.println("No fix");
  }

  // NTP time
  Serial.println("NTP Date:");
  Serial.print(ntp.getWeekdayName());
  Serial.print(", ");
  Serial.print(ntp.getMonthName());
  Serial.print(" ");
  Serial.print(ntp.getDayOfMonth());
  Serial.print(", ");
  Serial.println(ntp.getYear());
  Serial.println("NTP Time:");
  Serial.printf("%02d:%02d:%02d\n", ntp.getHour(), ntp.getMinute(), ntp.getSecond());

  // RTC time
  DateTime now = rtc.now();
  Serial.print("RTC Date: ");
  Serial.print(now.year());
  Serial.print("-");
  Serial.print(now.month());
  Serial.print("-");
  Serial.println(now.day());

  Serial.print("RTC Time: ");
  Serial.print(now.hour());
  Serial.print(":");
  Serial.print(now.minute());
  Serial.print(":");
  Serial.println(now.second());
  delay(2000);
}
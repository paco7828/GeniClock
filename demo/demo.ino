#include "constants.h"
#include "HDSPDisplay.h"
#include "gps.h"
#include "ntp.h"
#include <Wire.h>
#include <RTClib.h>

// Last button states
byte lastConfirmState = HIGH;
byte lastAddState = HIGH;
byte lastSubtractState = HIGH;
byte lastCancelState = HIGH;

// Button states
byte confirmState = HIGH;
byte addState = HIGH;
byte subtractState = HIGH;
byte cancelState = HIGH;

// Button debounce helpers
unsigned long lastDebounceTimeConfirm = 0;
unsigned long lastDebounceTimeAdd = 0;
unsigned long lastDebounceTimeSubtract = 0;
unsigned long lastDebounceTimeCancel = 0;

struct timeData {
  int year;
  byte month;
  byte day;
  byte dayIndex;
  byte hour;
  byte minute;
  byte second;
};

timeData currentTime;

// Time source status flags
bool gpsAvailable = false;
bool ntpAvailable = false;
bool rtcAvailable = false;
bool wifiConnected = false;

// Timing for connection attempts
unsigned long lastNtpAttempt = 0;
unsigned long lastRtcRead = 0;
unsigned long lastDisplayUpdate = 0;

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

  // RTC
  Wire.begin(RTC_SDA, RTC_SCL);
  if (rtc.begin()) {
    rtcAvailable = true;
  } else {
    HDSP.displayText("RTC FAIL");
    delay(1500);
  }

  // Default display text
  HDSP.displayText("- GENI -");
  delay(1000);
}

void loop() {
  updateTimeSource();
  updateDisplay();
}

void updateTimeSource() {
  bool timeUpdated = false;

  // Always check GPS first (highest priority)
  gps.update();

  if (gps.hasFix()) {
    if (!gpsAvailable) {
      gpsAvailable = true;
    }

    // Use GPS data
    currentTime.year = gps.getYear();
    currentTime.month = gps.getMonth();
    currentTime.day = gps.getDay();
    currentTime.dayIndex = gps.getDayIndex();
    currentTime.hour = gps.getHour();
    currentTime.minute = gps.getMinute();
    currentTime.second = gps.getSecond();
    timeUpdated = true;

    // Adjust RTC time to GPS time
    if (rtcAvailable) {
      rtc.adjust(DateTime(currentTime.year, currentTime.month, currentTime.day, currentTime.hour, currentTime.minute, currentTime.second));
    }
  } else {
    gpsAvailable = false;
  }

  // If GPS not available, try NTP (medium priority)
  if (!gpsAvailable) {
    // Initialize NTP
    if (!wifiConnected && (millis() - lastNtpAttempt > ntpRetryInterval)) {
      lastNtpAttempt = millis();

      if (!wifiConnected) {
        // Try to connect to WiFi
        WiFi.begin(SSID, PASSW);
        unsigned long wifiStart = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - wifiStart) < 3000) {
          delay(100);
        }

        if (WiFi.status() == WL_CONNECTED) {
          wifiConnected = true;
          ntp.begin();
        }
      }
    }

    if (wifiConnected && ntp.update()) {
      if (!ntpAvailable) {
        ntpAvailable = true;
      }

      // Use NTP data
      currentTime.year = ntp.getYear();
      currentTime.month = ntp.getMonth();
      currentTime.day = ntp.getDay();
      currentTime.dayIndex = ntp.getDayIndex();
      currentTime.hour = ntp.getHour();
      currentTime.minute = ntp.getMinute();
      currentTime.second = ntp.getSecond();
      timeUpdated = true;

      // Adjust RTC time to NTP time (only if GPS is not available)
      if (rtcAvailable && !gpsAvailable) {
        rtc.adjust(DateTime(currentTime.year, currentTime.month, currentTime.day, currentTime.hour, currentTime.minute, currentTime.second));
      }
    } else {
      ntpAvailable = false;
    }
  }

  // If neither GPS nor NTP available, use RTC (lowest priority)
  if (!gpsAvailable && !ntpAvailable && rtcAvailable) {
    if (millis() - lastRtcRead > rtcReadInterval) {
      lastRtcRead = millis();

      DateTime now = rtc.now();

      if (now.isValid()) {
        // Use RTC data
        currentTime.year = now.year();
        currentTime.month = now.month();
        currentTime.day = now.day();
        currentTime.dayIndex = now.dayOfTheWeek();
        currentTime.hour = now.hour();
        currentTime.minute = now.minute();
        currentTime.second = now.second();
        timeUpdated = true;
      }
    }
  }
}

void updateDisplay() {
  if (millis() - lastDisplayUpdate > displayUpdateInterval) {
    lastDisplayUpdate = millis();

    // If no time source is available, display error
    if (!gpsAvailable && !ntpAvailable && !rtcAvailable) {
      HDSP.displayText(" NO TIME ");
    } else {
      // Display the current time
      HDSP.displayTime(currentTime.hour, currentTime.minute, currentTime.second);
    }
  }
}

// Function for button debouncing
void debounceBtn(byte btnPin, byte* btnState, byte* lastBtnState, unsigned long* lastDebounceTime, void (*callback)()) {
  int reading = digitalRead(btnPin);

  // If the switch changed, due to noise or pressing:
  if (reading != *lastBtnState) {
    *lastDebounceTime = millis();
  }

  // If the reading has been stable for longer than the debounce delay
  if ((millis() - *lastDebounceTime) > debounceDelay) {
    // If the button state has actually changed:
    if (reading != *btnState) {
      *btnState = reading;

      // Only call callback on button press (HIGH to LOW transition)
      if (*btnState == LOW) {
        callback();
      }
    }
  }

  // Save the reading for next time
  *lastBtnState = reading;
}
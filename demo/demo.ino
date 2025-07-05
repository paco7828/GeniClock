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

// Time & Date values storage
timeData currentTime;

byte currentMode = 0;  // Start with HH:MM:SS mode
/*
0 -> time
1 -> year+month
2 -> day+short day name
3 -> timer
*/

// Settings index inside modes
byte currentSetting = 0;

// Mode selection state
bool changingModes = false;  // Start in normal display mode, not mode selection

// Time source status flags
bool gpsAvailable = false;
bool ntpAvailable = false;
bool rtcAvailable = false;
bool wifiConnected = false;
bool wifiConnecting = false;

// Status message variables
bool showingStatusMessage = false;
unsigned long statusMessageStart = 0;
const unsigned long statusMessageDuration = 3000;  // 3 seconds

// Timing for connection attempts
unsigned long lastNtpAttempt = 0;
unsigned long lastRtcRead = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long wifiConnectionStart = 0;

// Display update interval
unsigned long displayUpdateInterval = 500;  // milliseconds

// Display
HDSPDisplay HDSP(SER, SRCLK, RCLK);

// GPS
GPS gps;

// NTP
NTP ntp(SSID, PASSW);

// RTC
RTC_DS3231 rtc;

void setup() {
  Serial.begin(115200);  // Add serial for debugging

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
    Serial.println("RTC initialized successfully");
    showStatusMessage(" RTC OK ");
  } else {
    Serial.println("RTC initialization failed");
    HDSP.displayText("RTC FAIL");
    delay(1500);
  }

  // Default display text
  HDSP.displayText("- GENI -");
  delay(1000);

  // Initialize display update timer
  lastDisplayUpdate = millis();

  // Start showing time immediately (mode 0)
  Serial.println("Starting in HH:MM:SS mode");
}

void loop() {
  // Handle button presses
  handleButtons();

  // Update time source
  updateTimeSource();

  // Update display based on current state
  if (changingModes) {
    // In mode selection - display is handled by button presses
  } else {
    // In normal mode - update display based on current mode
    updateTDDisplay();
  }
}

void showStatusMessage(char *message) {
  showingStatusMessage = true;
  statusMessageStart = millis();
  HDSP.forceDisplayText(message);
  Serial.print("Status message: ");
  Serial.println(message);
}

void handleButtons() {
  if (changingModes) {
    // In mode selection state

    // ADD button cycles forwards through modes
    debounceBtn(ADD_BTN, &addState, &lastAddState, &lastDebounceTimeAdd, []() {
      currentMode++;
      if (currentMode > MAX_MODE) currentMode = MIN_MODE;

      // Show the title of the new mode
      HDSP.displayText(MODE_TITLES[currentMode]);
      Serial.print("Mode changed to: ");
      Serial.println(MODE_TITLES[currentMode]);
    });

    // SUBTRACT button cycles backwards through modes
    debounceBtn(SUBTRACT_BTN, &subtractState, &lastSubtractState, &lastDebounceTimeSubtract, []() {
      currentMode--;
      if (currentMode < MIN_MODE) currentMode = MAX_MODE;

      // Show the title of the new mode
      HDSP.displayText(MODE_TITLES[currentMode]);
      Serial.print("Mode changed to: ");
      Serial.println(MODE_TITLES[currentMode]);
    });

    // CONFIRM button selects the current mode
    debounceBtn(CONFIRM_BTN, &confirmState, &lastConfirmState, &lastDebounceTimeConfirm, []() {
      changingModes = false;

      // Set appropriate update interval for selected mode
      switch (currentMode) {
        case 0: displayUpdateInterval = 500; break;    // time updates every 500ms
        case 1: displayUpdateInterval = 10000; break;  // year+month updates every 10s
        case 2: displayUpdateInterval = 10000; break;  // day+name updates every 10s
        case 3:
          // Timer mode - handle separately
          HDSP.displayText(" TIMER  ");
          break;
      }

      Serial.print("Mode selected: ");
      Serial.println(MODE_TITLES[currentMode]);

      // Reset display update timer
      lastDisplayUpdate = millis();
    });

    // CANCEL button exits mode selection without changing mode
    debounceBtn(CANCEL_BTN, &cancelState, &lastCancelState, &lastDebounceTimeCancel, []() {
      changingModes = false;
      Serial.println("Mode selection cancelled");

      // Reset display update timer
      lastDisplayUpdate = millis();
    });

  } else {
    // In normal display mode

    // ADD button enters mode selection and cycles forward
    debounceBtn(ADD_BTN, &addState, &lastAddState, &lastDebounceTimeAdd, []() {
      changingModes = true;

      // Cycle to next mode
      currentMode++;
      if (currentMode > MAX_MODE) currentMode = MIN_MODE;

      // Show the title of the new mode
      HDSP.displayText(MODE_TITLES[currentMode]);
      Serial.print("Entering mode selection, mode: ");
      Serial.println(MODE_TITLES[currentMode]);
    });

    // SUBTRACT button enters mode selection and cycles backward
    debounceBtn(SUBTRACT_BTN, &subtractState, &lastSubtractState, &lastDebounceTimeSubtract, []() {
      changingModes = true;

      // Cycle to previous mode
      currentMode--;
      if (currentMode < MIN_MODE) currentMode = MAX_MODE;

      // Show the title of the new mode
      HDSP.displayText(MODE_TITLES[currentMode]);
      Serial.print("Entering mode selection, mode: ");
      Serial.println(MODE_TITLES[currentMode]);
    });

    // CONFIRM button could be used for mode-specific functions
    if (currentMode == 3) {  // Timer mode
      debounceBtn(CONFIRM_BTN, &confirmState, &lastConfirmState, &lastDebounceTimeConfirm, []() {
        // Handle timer start/stop
        Serial.println("Timer function");
      });
    }

    // CANCEL button could be used for mode-specific functions
    // For now, leaving it unused in normal modes
  }
}

void updateTimeSource() {
  bool timeUpdated = false;

  // Always check GPS first (highest priority)
  gps.update();

  if (gps.hasFix()) {
    if (!gpsAvailable) {
      gpsAvailable = true;
      Serial.println("GPS fix acquired");
      showStatusMessage(" GPS OK ");
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
    if (gpsAvailable) {
      gpsAvailable = false;
      Serial.println("GPS fix lost");
    }
  }

  // If GPS not available, try NTP (medium priority)
  if (!gpsAvailable) {
    // Try to connect to WiFi if not connected and not currently connecting
    if (!wifiConnected && !wifiConnecting && (millis() - lastNtpAttempt > ntpRetryInterval)) {
      lastNtpAttempt = millis();
      wifiConnecting = true;
      wifiConnectionStart = millis();
      Serial.println("Starting WiFi connection...");
      WiFi.begin(SSID, PASSW);
    }

    // Check WiFi connection status if we're currently connecting
    if (wifiConnecting) {
      if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        wifiConnecting = false;
        Serial.println("WiFi connected, initializing NTP...");
        ntp.begin();
      } else if (millis() - wifiConnectionStart > 8000) {  // 8 second timeout
        wifiConnecting = false;
        Serial.println("WiFi connection timeout");
        WiFi.disconnect();
      }
    }

    if (wifiConnected && ntp.update()) {
      if (!ntpAvailable) {
        ntpAvailable = true;
        Serial.println("NTP time acquired");
        showStatusMessage(" NTP OK ");
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
      if (ntpAvailable) {
        ntpAvailable = false;
        Serial.println("NTP time lost");
      }
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

void updateTDDisplay() {
  // Check if we're showing a status message
  if (showingStatusMessage) {
    if (millis() - statusMessageStart >= statusMessageDuration) {
      showingStatusMessage = false;
      lastDisplayUpdate = millis();  // Reset display timer to update immediately
    }
    return;  // Don't update display while showing status message
  }

  // Regular display updates (only when not in mode selection)
  if (millis() - lastDisplayUpdate >= displayUpdateInterval) {
    lastDisplayUpdate = millis();

    // If no time source is available, display error
    if (!gpsAvailable && !ntpAvailable && !rtcAvailable) {
      HDSP.displayText(" NO T&D ");
    } else {
      switch (currentMode) {
        // time (HH:MM:SS)
        case 0:
          HDSP.displayTime(currentTime.hour, currentTime.minute, currentTime.second);
          break;
        // year + month (2025. 06)
        case 1:
          HDSP.displayYearMonth(currentTime.year, currentTime.month);
          break;
        // day + short day name (31 Wed)
        case 2:
          HDSP.displayDayAndName(currentTime.day, currentTime.dayIndex);
          break;
        // timer mode
        case 3:
          HDSP.displayText(" TIMER  ");
          break;
      }
    }
  }
}

// Function for button debouncing
void debounceBtn(byte btnPin, byte *btnState, byte *lastBtnState, unsigned long *lastDebounceTime, void (*callback)()) {
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
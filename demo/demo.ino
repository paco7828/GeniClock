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
3 -> temperature
4 -> timer
*/

// Settings index inside modes
byte currentSetting = 0;

// Mode title display state
bool showingModeTitle = false;
unsigned long modeTitleStartTime = 0;

// Time source status flags
bool gpsAvailable = false;
bool ntpAvailable = false;
bool rtcAvailable = false;
bool wifiConnected = false;
bool wifiConnecting = false;

// Status message variables
bool showingStatusMessage = false;
unsigned long statusMessageStart = 0;

// Timing for connection attempts
unsigned long lastNtpAttempt = 0;
unsigned long lastRtcRead = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long wifiConnectionStart = 0;

// Temperature reading variables
unsigned long lastTemperatureRead = 0;
float currentTemperature = 0.0;

// Display update interval
unsigned long displayUpdateInterval = 500;  // milliseconds

// Hour notification variables
byte lastHour = 255;  // Initialize to invalid value to avoid notification on startup
bool hourNotificationEnabled = true;
bool playingHourNotification = false;
unsigned long notificationStartTime = 0;
byte notificationStep = 0;

// Startup sound variables
bool playingStartupSound = false;
unsigned long startupSoundStartTime = 0;
byte startupSoundStep = 0;

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
  } else {
    Serial.println("RTC initialization failed");
    HDSP.displayText("RTC FAIL");
    delay(1500);
  }

  // Start startup sound and display sequence
  playingStartupSound = true;
  startupSoundStartTime = millis();
  startupSoundStep = 0;

  // Show startup message
  HDSP.displayText("- GENI -");

  // Play first startup tone
  tone(BUZZER, startupMelody[0], startupNoteDurations[0]);

  Serial.println("Startup sound started");

  // Initialize display update timer
  lastDisplayUpdate = millis();
}

void loop() {
  // Handle startup sound first
  if (playingStartupSound) {
    handleStartupSound();
    return;  // Don't do anything else during startup sound
  }

  // Handle button presses
  handleButtons();

  // Update time source
  updateTimeSource();

  // Update temperature reading
  updateTemperature();

  // Handle hour notification
  handleHourNotification();

  // Check if mode title should auto-confirm
  if (showingModeTitle && (millis() - modeTitleStartTime >= TITLE_SHOW_TIME)) {
    showingModeTitle = false;

    // Set appropriate update interval for selected mode
    switch (currentMode) {
      case 0: displayUpdateInterval = 500; break;   // time updates every 500ms
      case 1: displayUpdateInterval = 1000; break;  // year+month updates every 1s
      case 2: displayUpdateInterval = 1000; break;  // day+name updates every 1s
      case 3: displayUpdateInterval = 2000; break;  // temperature updates every 2s
      case 4:
        // Timer mode - handle separately
        HDSP.displayText(" TIMER  ");
        break;
    }

    Serial.print("Mode auto-confirmed: ");
    Serial.println(MODE_TITLES[currentMode]);

    // Force immediate display update by setting lastDisplayUpdate to 0
    lastDisplayUpdate = 0;
  }

  // Update display based on current state
  if (!showingModeTitle && !playingHourNotification) {
    // In normal mode - update display based on current mode
    updateTDDisplay();
  }
}

void updateTemperature() {
  // Read temperature from RTC every temperatureReadInterval
  if (rtcAvailable && (millis() - lastTemperatureRead >= temperatureReadInterval)) {
    lastTemperatureRead = millis();
    currentTemperature = rtc.getTemperature();

    Serial.print("Temperature read: ");
    Serial.print(currentTemperature);
    Serial.println(" °C");
  }
}

void handleStartupSound() {
  // Check if it's time to play the next note
  if (millis() - startupSoundStartTime >= startupNoteDurations[startupSoundStep]) {
    startupSoundStep++;

    if (startupSoundStep < startupMelodyLength) {
      // Play next note in startup melody
      startupSoundStartTime = millis();
      tone(BUZZER, startupMelody[startupSoundStep], startupNoteDurations[startupSoundStep]);

      Serial.print("Playing startup note ");
      Serial.print(startupSoundStep + 1);
      Serial.print("/");
      Serial.println(startupMelodyLength);
    } else {
      // Startup sound finished
      playingStartupSound = false;
      noTone(BUZZER);

      Serial.println("Startup sound finished");

      // Show RTC status after startup sound
      if (rtcAvailable) {
        showStatusMessage(" RTC OK ");
      }

      // Wait a bit before starting normal operation
      delay(500);

      // Start showing time immediately (mode 0)
      Serial.println("Starting in HH:MM:SS mode");

      // Force immediate display update
      lastDisplayUpdate = 0;
    }
  }
}

void handleHourNotification() {
  // Check if we should start a new hour notification
  if (hourNotificationEnabled && !playingHourNotification && lastHour != 255 && currentTime.hour != lastHour && currentTime.minute == 0 && currentTime.second == 0) {

    // Start hour notification
    playingHourNotification = true;
    notificationStartTime = millis();
    notificationStep = 0;

    Serial.print("Hour notification started for ");
    Serial.print(currentTime.hour);
    Serial.println(":00");

    // Play first tone
    tone(BUZZER, notificationMelody[0], notificationStepDuration);

    // Show hour notification on display
    char hourMsg[9];
    sprintf(hourMsg, " %02d:00  ", currentTime.hour);
    HDSP.forceDisplayText(hourMsg);
  }

  // Handle notification progression
  if (playingHourNotification) {
    if (millis() - notificationStartTime >= (notificationStep + 1) * notificationStepDuration) {
      notificationStep++;

      if (notificationStep < notificationMelodyLength) {
        // Play next tone
        tone(BUZZER, notificationMelody[notificationStep], notificationStepDuration);
      } else {
        // Notification finished
        playingHourNotification = false;
        noTone(BUZZER);

        // Force display update by resetting timer
        lastDisplayUpdate = 0;

        Serial.println("Hour notification finished");
      }
    }
  }

  // Update lastHour for next comparison
  lastHour = currentTime.hour;
}

void showStatusMessage(char *message) {
  showingStatusMessage = true;
  statusMessageStart = millis();
  HDSP.forceDisplayText(message);
  Serial.print("Status message: ");
  Serial.println(message);
}

void startModeSwitch(byte newMode) {
  // Ensure the new mode is within valid range
  if (newMode > MAX_MODE) newMode = MIN_MODE;

  currentMode = newMode;
  showingModeTitle = true;
  modeTitleStartTime = millis();

  // Show the title of the new mode
  HDSP.displayText(MODE_TITLES[currentMode]);
  Serial.print("Switching to mode: ");
  Serial.println(MODE_TITLES[currentMode]);
}

void handleButtons() {
  // In normal display mode - simplified button handling

  // ADD button cycles forward through modes
  debounceBtn(ADD_BTN, &addState, &lastAddState, &lastDebounceTimeAdd, []() {
    byte newMode = currentMode + 1;
    if (newMode > MAX_MODE) newMode = MIN_MODE;
    startModeSwitch(newMode);
  });

  // SUBTRACT button cycles backward through modes
  debounceBtn(SUBTRACT_BTN, &subtractState, &lastSubtractState, &lastDebounceTimeSubtract, []() {
    byte newMode;
    if (currentMode == MIN_MODE) {
      newMode = MAX_MODE;
    } else {
      newMode = currentMode - 1;
    }
    startModeSwitch(newMode);
  });

  // CONFIRM button could be used for mode-specific functions
  if (currentMode == 4 && !showingModeTitle) {  // Timer mode (now mode 4)
    debounceBtn(CONFIRM_BTN, &confirmState, &lastConfirmState, &lastDebounceTimeConfirm, []() {
      // Handle timer start/stop
      Serial.println("Timer function");
    });
  }

  // CANCEL button toggles hour notification on/off
  debounceBtn(CANCEL_BTN, &cancelState, &lastCancelState, &lastDebounceTimeCancel, []() {
    hourNotificationEnabled = !hourNotificationEnabled;

    if (hourNotificationEnabled) {
      showStatusMessage("BEEP ON ");
      Serial.println("Hour notification enabled");
    } else {
      showStatusMessage("BEEP OFF");
      Serial.println("Hour notification disabled");

      // Stop any currently playing notification
      if (playingHourNotification) {
        playingHourNotification = false;
        noTone(BUZZER);
      }
    }
  });
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

  // Regular display updates (only when not showing mode title)
  if (millis() - lastDisplayUpdate >= displayUpdateInterval) {
    lastDisplayUpdate = millis();

    // If no time source is available for time modes, display error
    if ((currentMode == 0 || currentMode == 1 || currentMode == 2) && !gpsAvailable && !ntpAvailable && !rtcAvailable) {
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
        // temperature (-24.5 C-)
        case 3:
          if (rtcAvailable) {
            HDSP.displayTemperature(currentTemperature);
          } else {
            HDSP.displayText("NO TEMP ");
          }
          break;
        // timer mode
        case 4:
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
#include "constants.h"
#include "HDSPDisplay.h"
#include "gps.h"
#include "ntp.h"
#include "wifimanager.h"
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
5 -> GPS latitude
6 -> GPS longitude
7 -> GPS speed
*/

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

// WiFi setup mode variables
bool inWiFiSetupMode = false;
bool wifiCredentialsAvailable = false;

// Display
HDSPDisplay HDSP(SER, SRCLK, RCLK);

// GPS
GPS gps;

// NTP (will be initialized later with stored credentials)
NTP ntp;

// WiFi Manager
WiFiManager wifiManager;

// RTC
RTC_DS3231 rtc;

void setup() {
  Serial.begin(115200);

  // Display
  HDSP.begin();

  // Always show GENI first
  HDSP.displayText("- GENI -");

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
    // RTC FAIL will be shown after startup sequence
  }

  // Initialize WiFi Manager
  wifiManager.begin();

  // Check if WiFi credentials are stored
  if (wifiManager.hasStoredCredentials()) {
    wifiCredentialsAvailable = true;
    Serial.println("WiFi credentials found in storage");

    // Initialize NTP with stored credentials
    String ssid, password;
    wifiManager.getStoredCredentials(ssid, password);
    ntp.setCredentials(ssid.c_str(), password.c_str());
  } else {
    Serial.println("No WiFi credentials found - entering setup mode");
    inWiFiSetupMode = true;

    // Start AP for WiFi configuration
    wifiManager.startAP();

    // Show setup message after GENI
    delay(2000);
    HDSP.displayText("SET WIFI");
    delay(3000);

    // Show AP info
    HDSP.displayText("AP:CLOCK");
    delay(2000);
    HDSP.displayText("192.168");
    delay(2000);
    HDSP.displayText("   .4.1");
    delay(2000);
  }

  // Start startup sound and display sequence only if not in WiFi setup mode
  if (!inWiFiSetupMode) {
    playingStartupSound = true;
    startupSoundStartTime = millis();
    startupSoundStep = 0;

    // GENI is already shown, play first startup tone
    tone(BUZZER, STARTUP_MELODY[0], STARTUP_NOTE_DURATIONS[0]);

    Serial.println("Startup sound started");
  }

  // Initialize display update timer
  lastDisplayUpdate = millis();
}

void loop() {
  // Handle WiFi setup mode
  if (inWiFiSetupMode) {
    wifiManager.handleClient();

    // Check if credentials were saved
    if (wifiManager.credentialsUpdated()) {
      Serial.println("New WiFi credentials received");

      // Get the new credentials
      String ssid, password;
      wifiManager.getStoredCredentials(ssid, password);

      // Initialize NTP with new credentials
      ntp.setCredentials(ssid.c_str(), password.c_str());

      // Exit setup mode
      inWiFiSetupMode = false;
      wifiCredentialsAvailable = true;
      wifiManager.stopAP();

      showStatusMessage("WIFI SET");

      // Start startup sequence
      playingStartupSound = true;
      startupSoundStartTime = millis();
      startupSoundStep = 0;

      // Show startup message
      HDSP.displayText("- GENI -");

      // Play first startup tone
      tone(BUZZER, STARTUP_MELODY[0], STARTUP_NOTE_DURATIONS[0]);

      Serial.println("Startup sound started");
    }
    return;
  }

  // Handle startup sound first
  if (playingStartupSound) {
    handleStartupSound();
    return;  // Don't do anything else during startup sound
  }

  // Handle button presses
  handleButtons();

  // Update time source (only if WiFi credentials are available)
  if (wifiCredentialsAvailable) {
    updateTimeSource();
  }

  // Update temperature reading (only when in temperature mode)
  if (currentMode == 3) {
    updateTemperature();
  }

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
      case 5: displayUpdateInterval = 2000; break;  // GPS lat updates every 2s
      case 6: displayUpdateInterval = 2000; break;  // GPS lon updates every 2s
      case 7: displayUpdateInterval = 1000; break;  // GPS speed updates every 1s
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
  // Read temperature from RTC
  if (rtcAvailable && (millis() - lastTemperatureRead >= TEMPERATURE_READ_INTERVAL)) {
    lastTemperatureRead = millis();
    currentTemperature = rtc.getTemperature();

    Serial.print("Temperature read: ");
    Serial.print(currentTemperature);
    Serial.println(" °C");
  }
}

void handleStartupSound() {
  // Check if it's time to play the next note
  if (millis() - startupSoundStartTime >= STARTUP_NOTE_DURATIONS[startupSoundStep]) {
    startupSoundStep++;

    if (startupSoundStep < STARTUP_MELODY_LENGTH) {
      // Play next note in startup melody
      startupSoundStartTime = millis();
      tone(BUZZER, STARTUP_MELODY[startupSoundStep], STARTUP_NOTE_DURATIONS[startupSoundStep]);

      Serial.print("Playing startup note ");
      Serial.print(startupSoundStep + 1);
      Serial.print("/");
      Serial.println(STARTUP_MELODY_LENGTH);
    } else {
      // Startup sound finished
      playingStartupSound = false;
      noTone(BUZZER);

      Serial.println("Startup sound finished");

      // Show status messages after startup sound
      if (rtcAvailable) {
        showStatusMessage(" RTC OK ");
      } else {
        showStatusMessage("RTC FAIL");  // Show RTC FAIL after startup
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
    tone(BUZZER, NOTIF_MELODY[0], NOTIF_STEP_DURATION);

    // Show hour notification on display
    char hourMsg[9];
    sprintf(hourMsg, " %02d:00  ", currentTime.hour);
    HDSP.forceDisplayText(hourMsg);
  }

  // Handle notification progression
  if (playingHourNotification) {
    if (millis() - notificationStartTime >= (notificationStep + 1) * NOTIF_STEP_DURATION) {
      notificationStep++;

      if (notificationStep < NOTIF_MELODY_LENGTH) {
        // Play next tone
        tone(BUZZER, NOTIF_MELODY[notificationStep], NOTIF_STEP_DURATION);
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

// Simplified status message function
void showStatusMessage(const char *message) {
  showingStatusMessage = true;
  statusMessageStart = millis();
  HDSP.forceDisplayText((char *)message);
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

// Button beep frequencies for unique press effects
const int BUTTON_BEEP_FREQS[] = { 800, 1000, 1200, 600 };  // CONFIRM, CANCEL, ADD, SUBTRACT
const int BUTTON_BEEP_DURATION = 50;

void playButtonBeep(byte buttonIndex) {
  tone(BUZZER, BUTTON_BEEP_FREQS[buttonIndex], BUTTON_BEEP_DURATION);
}

void handleButtons() {
  // ADD button cycles forward through modes
  debounceBtn(ADD_BTN, &addState, &lastAddState, &lastDebounceTimeAdd, []() {
    playButtonBeep(2);
    byte newMode = currentMode + 1;
    if (newMode > MAX_MODE) newMode = MIN_MODE;
    startModeSwitch(newMode);
  });

  // SUBTRACT button cycles backward through modes
  debounceBtn(SUBTRACT_BTN, &subtractState, &lastSubtractState, &lastDebounceTimeSubtract, []() {
    playButtonBeep(3);
    byte newMode;
    if (currentMode == MIN_MODE) {
      newMode = MAX_MODE;
    } else {
      newMode = currentMode - 1;
    }
    startModeSwitch(newMode);
  });

  // CONFIRM button could be used for mode-specific functions or WiFi setup
  debounceBtn(CONFIRM_BTN, &confirmState, &lastConfirmState, &lastDebounceTimeConfirm, []() {
    playButtonBeep(0);
    if (currentMode == 4 && !showingModeTitle) {
      // Handle timer start/stop
      Serial.println("Timer function");
    } else {
      // Long press CONFIRM button to enter WiFi setup mode
      static unsigned long confirmPressStart = 0;
      static bool confirmPressed = false;

      if (!confirmPressed) {
        confirmPressed = true;
        confirmPressStart = millis();
      }

      // Check for long press (3 seconds)
      if (millis() - confirmPressStart >= 3000) {
        confirmPressed = false;

        // Enter WiFi setup mode
        Serial.println("Entering WiFi setup mode");
        inWiFiSetupMode = true;
        wifiManager.startAP();

        showStatusMessage("SET WIFI");
        delay(1000);
        HDSP.displayText("AP:CLOCK");
        delay(2000);
        HDSP.displayText("192.168");
        delay(1000);
        HDSP.displayText("   .4.1");
      }
    }
  });

  // CANCEL button toggles hour notification on/off
  debounceBtn(CANCEL_BTN, &cancelState, &lastCancelState, &lastDebounceTimeCancel, []() {
    playButtonBeep(1);
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
    gps.getHungarianTime(currentTime.year, currentTime.month, currentTime.day,
                         currentTime.dayIndex, currentTime.hour, currentTime.minute, currentTime.second);
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

  // If GPS not available, try NTP (medium priority) - only if we have WiFi credentials
  if (!gpsAvailable && wifiCredentialsAvailable) {
    // Try to connect to WiFi if not connected and not currently connecting
    if (!wifiConnected && !wifiConnecting && (millis() - lastNtpAttempt > NTP_RETRY_INTERVAL)) {
      lastNtpAttempt = millis();
      wifiConnecting = true;
      wifiConnectionStart = millis();
      Serial.println("Starting WiFi connection...");

      String ssid, password;
      wifiManager.getStoredCredentials(ssid, password);
      WiFi.begin(ssid.c_str(), password.c_str());
    }

    // Check WiFi connection status if we're currently connecting
    if (wifiConnecting) {
      if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        wifiConnecting = false;
        Serial.println("WiFi connected, initializing NTP...");
        showStatusMessage("WIFI SET");  // Show WiFi connection success message
        ntp.begin();
      } else if (millis() - wifiConnectionStart > WIFI_CONNECTION_TIMEOUT) {
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
        showStatusMessage("NTP LOST");
      }
    }
  }

  // If neither GPS nor NTP available, use RTC (lowest priority)
  if (!gpsAvailable && !ntpAvailable && rtcAvailable) {
    if (millis() - lastRtcRead > RTC_READ_INTERVAL) {
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
    if (millis() - statusMessageStart >= STATUS_MSG_DURATION) {
      showingStatusMessage = false;
      lastDisplayUpdate = millis();  // Reset display timer to update immediately
    }
    return;  // Don't update display while showing status message
  }

  // Regular display updates (only when not showing mode title)
  if (millis() - lastDisplayUpdate >= displayUpdateInterval) {
    lastDisplayUpdate = millis();

    // If in WiFi setup mode and no credentials available, show setup message
    if (!wifiCredentialsAvailable) {
      HDSP.displayText("SET WIFI");
      return;
    }

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
        // GPS latitude
        case 5:
          if (gpsAvailable) {
            HDSP.displayGPSLatitude(gps.getLatitude());
          } else {
            HDSP.displayText(" NO GPS ");
          }
          break;
        // GPS longitude
        case 6:
          if (gpsAvailable) {
            HDSP.displayGPSLongitude(gps.getLongitude());
          } else {
            HDSP.displayText(" NO GPS ");
          }
          break;
        // GPS speed
        case 7:
          if (gpsAvailable) {
            HDSP.displayGPSSpeed(gps.getSpeedKmph());
          } else {
            HDSP.displayText(" NO GPS ");
          }
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
  if ((millis() - *lastDebounceTime) > DEBOUNCE_DELAY) {
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
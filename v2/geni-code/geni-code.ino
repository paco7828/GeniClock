#include "constants.h"
#include "HDSPDisplay.h"
#include "gps.h"
#include "ntp.h"
#include "wifimanager.h"
#include <Wire.h>
#include <RTClib.h>
#include <Preferences.h>
#include "timer.h"
#include "alarm.h"
#include "Better-JoyStick.h"

// Joystick
BetterJoystick joystick;

// Joystick állapot (debounce + edge detect)
byte lastJsDirection = 0;
bool lastJsButton = false;
unsigned long lastJsDebounceTime = 0;
unsigned long lastJsBtnDebounceTime = 0;
bool jsButtonState = false;
bool lastJsButtonState = false;
// Triple press (confirm = JS gomb)
unsigned long firstConfirmPress = 0;
byte confirmPressCount = 0;

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
5 -> alarm
6 -> GPS latitude
7 -> GPS longitude
8 -> GPS speed
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
bool showingAPInfo = false;
unsigned long lastAPInfoSwitch = 0;
byte apInfoMode = 0;  // 0=AP name, 1="PASSWORD", 2=actual password

// Display
HDSPDisplay HDSP;  // default 0x20

// GPS
GPS gps;

// NTP (will be initialized later with stored credentials)
NTP ntp;

// WiFi Manager
WiFiManager wifiManager;

// RTC
RTC_DS3231 rtc;

// Preferences for persistent storage
Preferences preferences;

// Timer
Timer timer(&HDSP, BUZZER);

// Alarm - RENAMED from 'alarm' to 'alarmClock' to avoid conflict with system alarm() function
Alarm alarmClock(&HDSP, BUZZER, &preferences);

void setup() {
  // Initialize preferences
  preferences.begin("geniClock", false);

  // I2C init
  Wire.begin(I2C_SDA, I2C_SCL);

  // Display
  runDisplayTypeSetup();
  HDSP.displayText("- GENI -");

  // Initialize time structure to prevent 00:00:00 display
  currentTime.year = 2025;
  currentTime.month = 1;
  currentTime.day = 1;
  currentTime.dayIndex = 3;  // Wednesday
  currentTime.hour = 0;
  currentTime.minute = 0;
  currentTime.second = 0;

  // Joystick
  joystick.begin(JS_SW, JS_X, JS_Y);

  // Buzzer
  pinMode(BUZZER, OUTPUT);
  tone(BUZZER, 3000, 200);

  // GPS
  gps.begin(GPS_RX);

  if (rtc.begin()) {
    rtcAvailable = true;

    // Immediately read RTC time to avoid 00:00:00 display
    DateTime now = rtc.now();
    if (now.isValid()) {
      currentTime.year = now.year();
      currentTime.month = now.month();
      currentTime.day = now.day();
      currentTime.dayIndex = now.dayOfTheWeek();
      currentTime.hour = now.hour();
      currentTime.minute = now.minute();
      currentTime.second = now.second();

      // Initialize lastRtcRead so updateTimeSource doesn't wait
      lastRtcRead = millis();
    }
  }
  // RTC FAIL will be shown after startup sequence

  // Initialize WiFi Manager
  wifiManager.begin();

  // Check if WiFi credentials are stored
  if (wifiManager.hasStoredCredentials()) {
    wifiCredentialsAvailable = true;

    // Initialize NTP with stored credentials
    String ssid, password;
    wifiManager.getStoredCredentials(ssid, password);
    ntp.setCredentials(ssid.c_str(), password.c_str());
  } else {
    inWiFiSetupMode = true;
    showingAPInfo = true;
    lastAPInfoSwitch = millis();
    apInfoMode = 0;

    // Start AP with captive portal
    wifiManager.startAP();

    // Show setup message after GENI
    delay(2000);
    // AP info will be handled in the main loop
  }

  // Start startup sound and display sequence only if not in WiFi setup mode
  if (!inWiFiSetupMode) {
    playingStartupSound = true;
    startupSoundStartTime = millis();
    startupSoundStep = 0;

    // GENI is already shown, play first startup tone
    tone(BUZZER, STARTUP_MELODY[0], STARTUP_NOTE_DURATIONS[0]);
  }

  // Initialize display update timer
  lastDisplayUpdate = millis();
}

void loop() {
  // Handle WiFi setup mode with captive portal
  if (inWiFiSetupMode) {
    wifiManager.handleClient();

    // Handle AP info display switching
    handleAPInfoDisplay();

    // Check if credentials were saved via captive portal
    if (wifiManager.credentialsUpdated()) {
      // Get the new credentials
      String ssid, password;
      wifiManager.getStoredCredentials(ssid, password);

      // Initialize NTP with new credentials
      ntp.setCredentials(ssid.c_str(), password.c_str());

      // Exit setup mode
      inWiFiSetupMode = false;
      wifiCredentialsAvailable = true;
      showingAPInfo = false;
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
    }
    return;
  }

  // Reset confirm press counter if timeout exceeded
  if (confirmPressCount > 0 && (millis() - firstConfirmPress > TRIPLE_PRESS_WINDOW)) {
    confirmPressCount = 0;
    firstConfirmPress = 0;
  }

  // Handle startup sound first
  if (playingStartupSound) {
    handleStartupSound();
    return;  // Don't do anything else during startup sound
  }

  handleJoystick();

  // Check if timer wants to exit to main mode
  if (currentMode == 4 && timer.shouldExitTimerMode()) {
    timer.clearExitFlag();
    currentMode = 0;  // Go back to HH:MM:SS mode
    startModeSwitch(0);
    return;
  }

  // Check if alarm wants to exit to main mode
  if (currentMode == 5 && alarmClock.shouldExitAlarmMode()) {
    alarmClock.clearExitFlag();
    currentMode = 0;  // Go back to HH:MM:SS mode
    startModeSwitch(0);
    return;
  }

  // Update time source (only if WiFi credentials are available)
  if (wifiCredentialsAvailable) {
    updateTimeSource();
  }

  // Update temperature reading (only when in temperature mode)
  if (currentMode == 3) {
    updateTemperature();
  }

  // Check for alarm trigger (always check when alarm is enabled)
  if (alarmClock.isAlarmEnabled()) {
    alarmClock.checkAlarmTrigger(currentTime.hour, currentTime.minute, currentTime.second);
  }

  // Handle hour notification
  handleHourNotification();

  // Check if mode title should auto-confirm
  if (showingModeTitle && (millis() - modeTitleStartTime >= TITLE_SHOW_TIME)) {
    showingModeTitle = false;

    // For timer mode, trigger the setting title display
    if (currentMode == 4) {
      timer.forceShowSettingTitle();
    }

    // For alarm mode, trigger the setting title display
    if (currentMode == 5) {
      alarmClock.forceShowSettingTitle();
    }

    // Set appropriate update interval for selected mode
    switch (currentMode) {
      case 0: displayUpdateInterval = 500; break;   // time updates every 500ms
      case 1: displayUpdateInterval = 1000; break;  // year+month updates every 1s
      case 2: displayUpdateInterval = 1000; break;  // day+name updates every 1s
      case 3: displayUpdateInterval = 2000; break;  // temperature updates every 2s
      case 4:
        // Timer mode - handle separately, no need to set interval
        break;
      case 5:
        // Alarm mode - handle separately, no need to set interval
        break;
      case 6: displayUpdateInterval = 2000; break;  // GPS lat updates every 2s
      case 7: displayUpdateInterval = 2000; break;  // GPS lon updates every 2s
      case 8: displayUpdateInterval = 1000; break;  // GPS speed updates every 1s
    }

    // Force immediate display update by setting lastDisplayUpdate to 0
    lastDisplayUpdate = 0;
  }

  // Update display based on current state
  if (!showingModeTitle && !playingHourNotification) {
    // In normal mode - update display based on current mode
    updateTDDisplay();
  }
}

void handleAPInfoDisplay() {
  if (!showingAPInfo) return;

  // Check if it's time to switch display
  if (millis() - lastAPInfoSwitch >= AP_INFO_SWITCH_INTERVAL) {
    lastAPInfoSwitch = millis();

    // Check if someone has connected to the AP
    if (wifiManager.hasConnectedClients()) {
      // Show a message indicating captive portal is active
      switch (apInfoMode) {
        case 0:
          HDSP.displayText("CAPTIVE ");
          apInfoMode = 1;
          break;
        case 1:
          HDSP.displayText("PORTAL  ");
          apInfoMode = 2;
          break;
        case 2:
          HDSP.displayText("ACTIVE  ");
          apInfoMode = 0;
          break;
      }
    } else {
      // Show AP credentials in 3-step cycle
      switch (apInfoMode) {
        case 0:
          {
            String apText = "AP:" + String(AP_SSID);
            char apBuffer[9];
            apText.toCharArray(apBuffer, 9);
            HDSP.displayText(apBuffer);
            apInfoMode = 1;
          }
          break;
        case 1:
          HDSP.displayText("PASSWORD");
          apInfoMode = 2;
          break;
        case 2:
          {
            String pwText = String(AP_PASSWORD);
            char pwBuffer[9];
            pwText.toCharArray(pwBuffer, 9);
            HDSP.displayText(pwBuffer);
            apInfoMode = 0;
          }
          break;
      }
    }
  }
}

void updateTemperature() {
  // Read temperature from RTC
  if (rtcAvailable && (millis() - lastTemperatureRead >= TEMPERATURE_READ_INTERVAL)) {
    lastTemperatureRead = millis();
    currentTemperature = rtc.getTemperature();
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
    } else {
      // Startup sound finished
      playingStartupSound = false;
      noTone(BUZZER);

      // Show status messages after startup sound
      if (rtcAvailable) {
        showStatusMessage(" RTC OK ");
      } else {
        showStatusMessage("RTC FAIL");  // Show RTC FAIL after startup
      }

      // Wait a bit before starting normal operation
      delay(500);

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
}

void startModeSwitch(byte newMode) {
  // Ensure the new mode is within valid range
  if (newMode > MAX_MODE) newMode = MIN_MODE;

  currentMode = newMode;
  showingModeTitle = true;
  modeTitleStartTime = millis();

  // Reset timer when entering timer mode
  if (newMode == 4) {
    timer.reset();
  }

  // Reset alarm when entering alarm mode
  if (newMode == 5) {
    alarmClock.reset();
  }

  // Show the title of the new mode
  HDSP.displayText(MODE_TITLES[currentMode]);
}

void playButtonBeep(byte buttonIndex) {
  tone(BUZZER, BUTTON_BEEP_FREQS[buttonIndex], BUTTON_BEEP_DURATION);
}

void handleJoystick() {
  unsigned long now = millis();

  // --- Irány (mode váltás / add / subtract) ---
  byte dir = joystick.getDirection();
  if (dir != lastJsDirection) {
    lastJsDebounceTime = now;
    lastJsDirection = dir;
  }
  if (dir != 0 && (now - lastJsDebounceTime) > DEBOUNCE_DELAY) {
    // Csak az első stabil irányra reagálunk, addig nem ismételünk
    // (ha hold-repeat kell, azt külön lehet hozzáadni)
    lastJsDebounceTime = now + 300;  // 300ms auto-repeat gát

    if (dir == 2) {  // RIGHT → ADD / mode forward
      playButtonBeep(2);
      if (currentMode == 4 && !showingModeTitle) {
        timer.handleAddButton();
        return;
      }
      if (currentMode == 5 && !showingModeTitle) {
        alarmClock.handleAddButton();
        return;
      }
      byte newMode = (currentMode + 1 > MAX_MODE) ? MIN_MODE : currentMode + 1;
      startModeSwitch(newMode);
    } else if (dir == 1) {  // LEFT → SUBTRACT / mode backward
      playButtonBeep(3);
      if (currentMode == 4 && !showingModeTitle) {
        timer.handleSubtractButton();
        return;
      }
      if (currentMode == 5 && !showingModeTitle) {
        alarmClock.handleSubtractButton();
        return;
      }
      byte newMode = (currentMode == MIN_MODE) ? MAX_MODE : currentMode - 1;
      startModeSwitch(newMode);
    } else if (dir == 4) {  // UP → CONFIRM (timer/alarm confirm)
      if (currentMode == 4 && !showingModeTitle) {
        timer.handleConfirmButton();
        return;
      }
      if (currentMode == 5 && !showingModeTitle) {
        alarmClock.handleConfirmButton();
        return;
      }
      // Triple press → WiFi setup (UP 3x)
      if (now - firstConfirmPress <= TRIPLE_PRESS_WINDOW) {
        confirmPressCount++;
        if (confirmPressCount >= 3) {
          inWiFiSetupMode = true;
          showingAPInfo = true;
          lastAPInfoSwitch = now;
          apInfoMode = 0;
          wifiManager.startAP();
          showStatusMessage("AP MODE ");
          confirmPressCount = 0;
          firstConfirmPress = 0;
        }
      } else {
        confirmPressCount = 1;
        firstConfirmPress = now;
      }
    } else if (dir == 3) {  // DOWN → CANCEL
      playButtonBeep(1);
      if (currentMode == 4 && !showingModeTitle) {
        timer.handleCancelButton();
        if (timer.shouldExitTimerMode()) {
          timer.clearExitFlag();
          currentMode = 0;
          startModeSwitch(0);
        }
        return;
      }
      if (currentMode == 5 && !showingModeTitle) {
        alarmClock.handleCancelButton();
        if (alarmClock.shouldExitAlarmMode()) {
          alarmClock.clearExitFlag();
          currentMode = 0;
          startModeSwitch(0);
        }
        return;
      }
      hourNotificationEnabled = !hourNotificationEnabled;
      showStatusMessage(hourNotificationEnabled ? "BEEP ON " : "BEEP OFF");
      if (!hourNotificationEnabled && playingHourNotification) {
        playingHourNotification = false;
        noTone(BUZZER);
      }
    }
  }

  // --- Gomb (JS SW) debounce ---
  bool btnRaw = joystick.getButtonPress();
  if (btnRaw != lastJsButtonState) lastJsBtnDebounceTime = now;
  if ((now - lastJsBtnDebounceTime) > DEBOUNCE_DELAY) {
    if (btnRaw != jsButtonState) {
      jsButtonState = btnRaw;
      if (jsButtonState) {  // press event (LOW = pressed, getButtonPress() returns true)
        playButtonBeep(0);
        // Gomb = CONFIRM funkció (timer start/stop, alarm confirm)
        if (currentMode == 4 && !showingModeTitle) {
          timer.handleConfirmButton();
        } else if (currentMode == 5 && !showingModeTitle) {
          alarmClock.handleConfirmButton();
        }
      }
    }
  }
  lastJsButtonState = btnRaw;
}

void updateTimeSource() {
  bool timeUpdated = false;

  // Always check GPS first (highest priority)
  gps.update();

  if (gps.hasFix()) {
    if (!gpsAvailable) {
      gpsAvailable = true;
      showStatusMessage(" GPS OK ");
    }

    // Temporary variables
    int year, month, day, dayIndex, hour, minute, second;

    // Retrieve GPS time
    gps.getHungarianTime(year, month, day, dayIndex, hour, minute, second);

    // Insert into structure
    currentTime.year = year;
    currentTime.month = month;
    currentTime.day = day;
    currentTime.dayIndex = dayIndex;
    currentTime.hour = hour;
    currentTime.minute = minute;
    currentTime.second = second;

    timeUpdated = true;

    // RTC synchronization
    if (rtcAvailable) {
      rtc.adjust(DateTime(year, month, day, hour, minute, second));
    }
  } else {
    if (gpsAvailable) {
      gpsAvailable = false;
    }
  }

  // If GPS not available, try NTP (medium priority)
  if (!gpsAvailable && wifiCredentialsAvailable) {
    if (!wifiConnected && !wifiConnecting && (millis() - lastNtpAttempt > NTP_RETRY_INTERVAL)) {
      lastNtpAttempt = millis();
      wifiConnecting = true;
      wifiConnectionStart = millis();

      String ssid, password;
      wifiManager.getStoredCredentials(ssid, password);
      WiFi.begin(ssid.c_str(), password.c_str());
    }

    if (wifiConnecting) {
      if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        wifiConnecting = false;
        showStatusMessage("WIFI SET");
        ntp.begin();
      } else if (millis() - wifiConnectionStart > WIFI_CONNECTION_TIMEOUT) {
        wifiConnecting = false;
        WiFi.disconnect();
      }
    }

    if (wifiConnected && ntp.update()) {
      if (!ntpAvailable) {
        ntpAvailable = true;
        showStatusMessage(" NTP OK ");
      }

      // Set NTP time
      currentTime.year = ntp.getYear();
      currentTime.month = ntp.getMonth();
      currentTime.day = ntp.getDay();
      currentTime.dayIndex = ntp.getDayIndex();
      currentTime.hour = ntp.getHour();
      currentTime.minute = ntp.getMinute();
      currentTime.second = ntp.getSecond();

      timeUpdated = true;

      // RTC synchronization
      if (rtcAvailable && !gpsAvailable) {
        rtc.adjust(DateTime(currentTime.year, currentTime.month, currentTime.day,
                            currentTime.hour, currentTime.minute, currentTime.second));
      }
    } else {
      if (ntpAvailable) {
        ntpAvailable = false;
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
        // Set RTC time
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

    // If in WiFi setup mode and no credentials available, show AP info
    if (!wifiCredentialsAvailable && showingAPInfo) {
      // AP info is handled by handleAPInfoDisplay()
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
          timer.update();  // This handles all timer logic and display
          break;
        // alarm mode
        case 5:
          alarmClock.update();  // This handles all alarm logic and display
          break;
        // GPS latitude
        case 6:
          if (gpsAvailable) {
            HDSP.displayGPSLatitude(gps.getLatitude());
          } else {
            HDSP.displayText(" NO GPS ");
          }
          break;
        // GPS longitude
        case 7:
          if (gpsAvailable) {
            HDSP.displayGPSLongitude(gps.getLongitude());
          } else {
            HDSP.displayText(" NO GPS ");
          }
          break;
        // GPS speed
        case 8:
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

void runDisplayTypeSetup() {
  pinMode(BUZZER, OUTPUT);
  pinMode(JS_SW, INPUT_PULLUP);

  uint8_t selectedType = preferences.getUChar("clockType", DEFAULT_CLOCK_TYPE);
  HDSP.setClockType(selectedType);
  HDSP.begin();
  HDSP.forceDisplayText(DISPLAY_TEST_TEXT);

  bool wasPressed = false;
  unsigned long pressStart = 0;
  byte pressCount = 0;
  unsigned long lastReleaseTime = 0;
  bool longPressHandled = false;

  while (true) {
    bool isPressed = (digitalRead(JS_SW) == LOW);

    // lenyomás (falling edge)
    if (isPressed && !wasPressed) {
      delay(DEBOUNCE_DELAY);
      if (digitalRead(JS_SW) != LOW) continue;  // zajszűrés
      wasPressed = true;
      pressStart = millis();
      longPressHandled = false;
    }

    // nyomva tartás - 2mp után confirm
    if (isPressed && wasPressed && !longPressHandled) {
      if (millis() - pressStart >= DISPLAY_SETUP_CONFIRM_HOLD) {
        longPressHandled = true;
        preferences.putUChar("clockType", selectedType);
        tone(BUZZER, 1500, 300);
        delay(300);
        return;  // confirmed - az óra normálisan indul tovább
      }
    }

    // elengedés (rising edge)
    if (!isPressed && wasPressed) {
      delay(DEBOUNCE_DELAY);
      if (digitalRead(JS_SW) == LOW) continue;  // zajszűrés
      wasPressed = false;

      if (!longPressHandled) {
        if (millis() - lastReleaseTime > DISPLAY_SETUP_PRESS_WINDOW) {
          pressCount = 0;  // túl nagy szünet, újrakezdjük a számlálást
        }
        pressCount++;
        lastReleaseTime = millis();
        tone(BUZZER, 800, 30);

        if (pressCount >= DISPLAY_SETUP_SWITCH_PRESSES) {
          selectedType = selectedType ? 0 : 1;
          HDSP.setClockType(selectedType);
          HDSP.begin();
          HDSP.forceDisplayText(DISPLAY_TEST_TEXT);
          pressCount = 0;
        }
      }
    }

    delay(5);
  }
}
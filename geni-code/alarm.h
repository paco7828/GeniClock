#ifndef ALARM_H
#define ALARM_H

#include "HDSPDisplay.h"
#include "constants.h"
#include <Preferences.h>

class Alarm {
private:
  // Alarm state
  bool settingAlarm;
  bool alarmEnabled;
  bool alarmTriggered;
  byte currentSetting;  // 0=hours, 1=minutes, 2=enable/disable

  // Alarm values
  int alarmHours;
  int alarmMinutes;

  // Alarm sound
  bool alarmPlaying;
  unsigned long alarmStartTime;
  byte alarmStep;
  byte currentAlarmCycle;
  unsigned long alarmCycleGap;
  bool snoozed;
  unsigned long snoozeStartTime;
  unsigned long noteStartTime;  // Track individual note timing

  // Setting display state
  bool showingSettingTitle;
  unsigned long settingTitleStartTime;

  // Cancel button tracking
  unsigned long lastCancelPress;
  byte cancelPressCount;
  bool cancelSingleProcessed;

  // Reference to external components
  HDSPDisplay* display;
  byte buzzerPin;
  Preferences* preferences;

  // Exit flag
  bool exitAlarmMode;

public:
  Alarm(HDSPDisplay* hdspDisplay, byte buzzer, Preferences* prefs)
    : display(hdspDisplay), buzzerPin(buzzer), preferences(prefs) {
    reset();
    loadAlarmSettings();
  }

  void reset() {
    settingAlarm = true;
    alarmTriggered = false;
    currentSetting = 0;
    alarmPlaying = false;
    alarmStartTime = 0;
    alarmStep = 0;
    currentAlarmCycle = 0;
    alarmCycleGap = 0;
    snoozed = false;
    snoozeStartTime = 0;
    noteStartTime = 0;
    showingSettingTitle = true;
    settingTitleStartTime = millis();
    cancelPressCount = 0;
    lastCancelPress = 0;
    cancelSingleProcessed = false;
    exitAlarmMode = false;
  }

  // Load alarm settings from preferences
  void loadAlarmSettings() {
    alarmHours = preferences->getInt("alarmHours", 0);
    alarmMinutes = preferences->getInt("alarmMins", 0);
    alarmEnabled = preferences->getBool("alarmOn", false);
  }

  // Save alarm settings to preferences
  void saveAlarmSettings() {
    preferences->putInt("alarmHours", alarmHours);
    preferences->putInt("alarmMins", alarmMinutes);
    preferences->putBool("alarmOn", alarmEnabled);
  }

  // Check if alarm should trigger
  void checkAlarmTrigger(int currentHour, int currentMinute, int currentSecond) {
    if (!alarmEnabled || alarmTriggered || alarmPlaying) return;

    // Check if it's alarm time (trigger at 0 seconds)
    if (currentHour == alarmHours && currentMinute == alarmMinutes && currentSecond == 0) {
      if (!snoozed) {
        triggerAlarm();
      } else {
        // Check if snooze period is over
        if (millis() - snoozeStartTime >= ALARM_SNOOZE_DURATION) {
          snoozed = false;
          triggerAlarm();
        }
      }
    }

    // Reset alarm triggered flag at the start of a new day (00:00:00)
    if (currentHour == 0 && currentMinute == 0 && currentSecond == 0) {
      alarmTriggered = false;
      snoozed = false;
    }
  }

  // Handle alarm updates (call this in main loop when in alarm mode)
  void update() {
    // Handle cancel button timeout and single press processing
    if (cancelPressCount > 0 && (millis() - lastCancelPress > ALARM_CANCEL_DOUBLE_PRESS_WINDOW)) {
      if (cancelPressCount == 1 && !cancelSingleProcessed) {
        // Process single press - reset current setting to 0
        handleCancelSinglePress();
        cancelSingleProcessed = true;
      }
      cancelPressCount = 0;
    }

    // Handle setting title timeout
    if (showingSettingTitle && (millis() - settingTitleStartTime >= ALARM_SETTING_TITLE_DURATION)) {
      showingSettingTitle = false;
    }

    if (alarmPlaying) {
      handleAlarmSound();
      updateAlarmDisplay();
      return;
    }

    updateDisplay();
  }

  // Force show setting title (call this when entering alarm mode)
  void forceShowSettingTitle() {
    showingSettingTitle = true;
    settingTitleStartTime = millis();
  }

  // Button handlers
  void handleConfirmButton() {
    if (alarmPlaying) {
      // Stop alarm completely
      stopAlarm();
      alarmTriggered = true;  // Prevent retriggering today
      return;
    }

    if (settingAlarm) {
      handleValueConfirm();
    }
  }

  void handleAddButton() {
    if (settingAlarm && !alarmPlaying && !showingSettingTitle) {
      if (currentSetting == 2) {
        // Enable/disable setting - ADD turns alarm ON
        alarmEnabled = true;
        saveAlarmSettings();
      } else {
        handleAdd();
      }
    }
  }

  void handleSubtractButton() {
    if (settingAlarm && !alarmPlaying && !showingSettingTitle) {
      if (currentSetting == 2) {
        // Enable/disable setting - SUBTRACT turns alarm OFF
        alarmEnabled = false;
        saveAlarmSettings();
      } else {
        handleSubtract();
      }
    }
  }

  void handleCancelButton() {
    if (alarmPlaying) {
      // Snooze the alarm
      snoozeAlarm();
      return;
    }

    unsigned long currentTime = millis();

    // Check for double press
    if (currentTime - lastCancelPress <= ALARM_CANCEL_DOUBLE_PRESS_WINDOW && cancelPressCount == 1) {
      // Double press detected - exit alarm mode
      cancelPressCount = 0;
      exitAlarmMode = true;
      return;
    }

    // Record the press
    cancelPressCount = 1;
    lastCancelPress = currentTime;
    cancelSingleProcessed = false;
  }

  // Get current state for display purposes
  bool isInSettingMode() const {
    return settingAlarm;
  }
  bool isAlarmActive() const {
    return alarmPlaying;
  }
  bool shouldExitAlarmMode() const {
    return exitAlarmMode;
  }
  bool isAlarmEnabled() const {
    return alarmEnabled;
  }

  // Reset exit flag (call from main code after handling exit)
  void clearExitFlag() {
    exitAlarmMode = false;
  }

private:
  // Handle single cancel press - reset current setting to 0
  void handleCancelSinglePress() {
    if (!settingAlarm || showingSettingTitle) return;

    switch (currentSetting) {
      case 0:  // Hours
        alarmHours = 0;
        break;
      case 1:  // Minutes
        alarmMinutes = 0;
        break;
      case 2:  // Enable/disable - toggle
        alarmEnabled = !alarmEnabled;
        saveAlarmSettings();
        break;
    }
  }

  // Format alarm values with leading zeros
  String formatAlarmValue(int value) const {
    if (value < 10) {
      return "0" + String(value);
    }
    return String(value);
  }

  // Get formatted alarm time string
  String getAlarmTimeString() const {
    return " " + formatAlarmValue(alarmHours) + ":" + formatAlarmValue(alarmMinutes) + "  ";
  }

  // Get current setting title
  String getCurrentSettingTitle() const {
    switch (currentSetting) {
      case 0: return "SET HRS ";
      case 1: return "SET MINS";
      case 2: return "SET ALRM";
      default: return "";
    }
  }

  // Handle value confirmation (hours -> minutes -> enable/disable -> done)
  void handleValueConfirm() {
    if (showingSettingTitle) return;

    currentSetting++;
    if (currentSetting >= 3) {
      // All values set, save and exit setting mode
      saveAlarmSettings();
      settingAlarm = false;
      exitAlarmMode = true;
    } else {
      // Move to next setting, show title
      showingSettingTitle = true;
      settingTitleStartTime = millis();
    }
  }

  // Handle adding to current setting
  void handleAdd() {
    switch (currentSetting) {
      case 0:  // Hours
        alarmHours++;
        if (alarmHours > 23) {
          alarmHours = 0;
        }
        break;
      case 1:  // Minutes
        alarmMinutes++;
        if (alarmMinutes > 59) {
          alarmMinutes = 0;
        }
        break;
    }
  }

  // Handle subtracting from current setting
  void handleSubtract() {
    switch (currentSetting) {
      case 0:  // Hours
        alarmHours--;
        if (alarmHours < 0) {
          alarmHours = 23;
        }
        break;
      case 1:  // Minutes
        alarmMinutes--;
        if (alarmMinutes < 0) {
          alarmMinutes = 59;
        }
        break;
    }
  }

  // FIXED: Trigger the alarm - DON'T specify duration in tone()
  void triggerAlarm() {
    alarmTriggered = true;
    alarmPlaying = true;
    alarmStartTime = millis();
    noteStartTime = millis();
    alarmStep = 0;
    currentAlarmCycle = 0;
    alarmCycleGap = 0;

    // Stop any existing tone and play first tone WITHOUT duration parameter
    noTone(buzzerPin);
    delay(10);                         // Small delay to ensure clean start
    tone(buzzerPin, ALARM_MELODY[0]);  // NO duration parameter!
  }

  // FIXED: Handle alarm sound progression with proper timing
  void handleAlarmSound() {
    unsigned long currentMillis = millis();

    // Handle gap between cycles
    if (alarmCycleGap > 0) {
      if (currentMillis - alarmCycleGap >= ALARM_CYCLE_GAP_DURATION) {
        // Gap finished, start next cycle
        alarmCycleGap = 0;
        alarmStep = 0;
        noteStartTime = currentMillis;

        // Start new cycle
        noTone(buzzerPin);
        delay(5);
        tone(buzzerPin, ALARM_MELODY[0]);  // NO duration parameter!
      }
      return;
    }

    // Check if current note duration has elapsed
    if (currentMillis - noteStartTime >= ALARM_STEP_DURATION) {
      alarmStep++;

      if (alarmStep < ALARM_MELODY_LENGTH) {
        // Play next tone in melody
        noteStartTime = currentMillis;
        noTone(buzzerPin);                         // Stop current tone
        delay(5);                                  // Brief pause between notes
        tone(buzzerPin, ALARM_MELODY[alarmStep]);  // NO duration parameter!
      } else {
        // Melody finished
        noTone(buzzerPin);
        currentAlarmCycle++;

        if (currentAlarmCycle < ALARM_CYCLES) {
          // Start gap before next cycle
          alarmCycleGap = currentMillis;
        } else {
          // All cycles finished, restart from beginning
          currentAlarmCycle = 0;
          alarmStep = 0;
          noteStartTime = currentMillis;
          delay(5);
          tone(buzzerPin, ALARM_MELODY[0]);  // NO duration parameter!
        }
      }
    }
  }

  // Stop the alarm completely
  void stopAlarm() {
    alarmPlaying = false;
    snoozed = false;
    noTone(buzzerPin);
  }

  // Snooze the alarm
  void snoozeAlarm() {
    alarmPlaying = false;
    snoozed = true;
    snoozeStartTime = millis();
    noTone(buzzerPin);
  }

  // Update display based on current state
  void updateDisplay() {
    if (settingAlarm) {
      if (showingSettingTitle) {
        // Show setting title (SET HRS, SET MINS, SET ALRM)
        String titleStr = getCurrentSettingTitle();
        char buffer[9];
        titleStr.toCharArray(buffer, 9);
        display->displayText(buffer);
      } else {
        if (currentSetting == 2) {
          // Show alarm status (ALRM ON / ALRM OFF)
          String statusStr = alarmEnabled ? "ALRM ON " : "ALRM OFF";
          char buffer[9];
          statusStr.toCharArray(buffer, 9);
          display->displayText(buffer);
        } else {
          // Show current alarm time
          String alarmStr = getAlarmTimeString();
          char buffer[9];
          alarmStr.toCharArray(buffer, 9);
          display->displayText(buffer);
        }
      }
    } else {
      // Show alarm time when not in setting mode
      String alarmStr = getAlarmTimeString();
      char buffer[9];
      alarmStr.toCharArray(buffer, 9);
      display->displayText(buffer);
    }
  }

  // Update alarm display when ringing
  void updateAlarmDisplay() {
    // Flash "WAKE UP!" message
    if ((millis() / 400) % 2 == 0) {
      display->displayText("WAKE UP!");
    } else {
      display->displayText("        ");
    }
  }
};

#endif  // ALARM_H
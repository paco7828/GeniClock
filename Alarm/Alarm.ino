#include <Wire.h>
#include "RTClib.h"

RTC_DS3231 rtc;

// Button pins
const byte CONFIRM_BTN = A1;
const byte ADD_BTN = A2;
const byte SUBTRACT_BTN = A3;
// Piezo pin
const byte PIEZO = A0;

// Last button states
int lastConfirmState = HIGH;
int lastAddState = HIGH;
int lastSubtractState = HIGH;

// Button states
int confirmState = HIGH;
int addState = HIGH;
int subtractState = HIGH;

// Helper variables
unsigned long lastDebounceTimeConfirm = 0;
unsigned long lastDebounceTimeAdd = 0;
unsigned long lastDebounceTimeSubtract = 0;
const unsigned long debounceDelay = 50;

byte currentSetting = 0;
/*
0 -> day
1 -> hour
2 -> minute
*/

// Alarm variables
bool settingAlarm = true;
int alarmDay = 1, alarmHour = 0, alarmMinute = 0;
bool alarmTurnedOn = true;
bool alarmTriggered = false;
bool alarmAlreadyTriggered = false;  // Prevents re-triggering in same minute

// Alarm sound variables
unsigned long lastAlarmTime = 0;
const unsigned long alarmInterval = 500;  // Beep every 500ms
bool alarmBeepState = false;
const unsigned long alarmDuration = 30000;  // Alarm sounds for 30 seconds
unsigned long alarmStartTime = 0;

// Month names array for display
const char* monthNames[] = {
  "January", "February", "March", "April", "May", "June",
  "July", "August", "September", "October", "November", "December"
};

void setup() {
  pinMode(PIEZO, OUTPUT);
  pinMode(CONFIRM_BTN, INPUT_PULLUP);
  pinMode(ADD_BTN, INPUT_PULLUP);
  pinMode(SUBTRACT_BTN, INPUT_PULLUP);

  Serial.begin(9600);

  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }

  // Check if RTC lost power and needs time reset
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting the time!");
    // Set to current date/time - adjust as needed
    rtc.adjust(DateTime(2025, 6, 14, 13, 0, 0));  // YYYY, MM, DD, HH, MM, SS
  }

  // Initialize alarm to current time + 1 minute
  DateTime now = rtc.now();
  alarmDay = now.day();
  alarmHour = now.hour();
  alarmMinute = now.minute() + 1;
  if (alarmMinute > 59) {
    alarmMinute = 0;
    alarmHour++;
    if (alarmHour > 23) {
      alarmHour = 0;
      alarmDay++;
      if (alarmDay > getDaysInMonth(now.month(), now.year())) {
        alarmDay = 1;
      }
    }
  }

  Serial.println("======== ALARM ========");
  displayCurrentTime();
  Serial.print("Select day -> ");
  Serial.println(formatTimeValue(alarmDay));
}

void loop() {
  DateTime now = rtc.now();

  if (settingAlarm) {
    debounceBtn(CONFIRM_BTN, &confirmState, &lastConfirmState, &lastDebounceTimeConfirm, []() {
      switch (currentSetting) {
        case 0:
          Serial.print("Day set to ");
          Serial.println(alarmDay);
          Serial.print("Set hour -> ");
          Serial.println(formatTimeValue(alarmHour));
          break;
        case 1:
          Serial.print("Hour set to ");
          Serial.println(alarmHour);
          Serial.print("Set minute -> ");
          Serial.println(formatTimeValue(alarmMinute));
          break;
        case 2:
          Serial.print("Minutes set to ");
          Serial.println(alarmMinute);
          DateTime now = rtc.now();
          Serial.print("Alarm set for: ");
          Serial.print(monthNames[now.month() - 1]);
          Serial.print(" ");
          Serial.print(formatTimeValue(alarmDay));
          Serial.print(" ");
          Serial.print(formatTimeValue(alarmHour));
          Serial.print(":");
          Serial.println(formatTimeValue(alarmMinute));
          settingAlarm = false;
          alarmTriggered = false;         // Reset alarm trigger when setting new alarm
          alarmAlreadyTriggered = false;  // Reset the already triggered flag for new alarm
          break;
      }
      currentSetting++;
    });

    debounceBtn(ADD_BTN, &addState, &lastAddState, &lastDebounceTimeAdd, []() {
      DateTime now = rtc.now();
      switch (currentSetting) {
        case 0:
          alarmDay++;
          if (alarmDay > getDaysInMonth(now.month(), now.year())) alarmDay = 1;
          Serial.println(formatTimeValue(alarmDay));
          break;
        case 1:
          alarmHour++;
          if (alarmHour > 23) alarmHour = 0;
          Serial.println(formatTimeValue(alarmHour));
          break;
        case 2:
          alarmMinute++;
          if (alarmMinute > 59) alarmMinute = 0;
          Serial.println(formatTimeValue(alarmMinute));
          break;
      }
    });

    debounceBtn(SUBTRACT_BTN, &subtractState, &lastSubtractState, &lastDebounceTimeSubtract, []() {
      DateTime now = rtc.now();
      switch (currentSetting) {
        case 0:
          // Don't allow setting alarm day to past dates
          if (alarmDay > now.day()) {
            alarmDay--;
            if (alarmDay < 1) alarmDay = getDaysInMonth(now.month(), now.year());
            Serial.println(formatTimeValue(alarmDay));
          }
          break;
        case 1:
          // Don't allow setting alarm hour to past times on current day
          if (alarmDay > now.day() || (alarmDay == now.day() && alarmHour > now.hour())) {
            alarmHour--;
            if (alarmHour < 0) alarmHour = 23;
            Serial.println(formatTimeValue(alarmHour));
          }
          break;
        case 2:
          // Don't allow setting alarm minute to past times on current day and hour
          if (alarmDay > now.day() || (alarmDay == now.day() && alarmHour > now.hour()) || (alarmDay == now.day() && alarmHour == now.hour() && alarmMinute > now.minute())) {
            alarmMinute--;
            if (alarmMinute < 0) alarmMinute = 59;
            Serial.println(formatTimeValue(alarmMinute));
          }
          break;
      }
    });
  } else {
    // Handle alarm on/off and snooze when not setting alarm
    debounceBtn(ADD_BTN, &addState, &lastAddState, &lastDebounceTimeAdd, []() {
      if (!alarmTurnedOn) {
        Serial.println("Alarm turned on");
        alarmTurnedOn = true;
        alarmTriggered = false;  // Reset trigger state
        // Don't reset alarmAlreadyTriggered - prevents immediate re-trigger
      } else if (alarmTriggered) {
        // Stop alarm completely
        alarmTriggered = false;
        alarmTurnedOn = false;  // Turn off alarm completely
        Serial.println("Alarm turned off");
        noTone(PIEZO);  // Stop alarm sound
      }
    });

    debounceBtn(SUBTRACT_BTN, &subtractState, &lastSubtractState, &lastDebounceTimeSubtract, []() {
      if (alarmTurnedOn && !alarmTriggered) {
        Serial.println("Alarm turned off");
        alarmTurnedOn = false;
      } else if (alarmTriggered) {
        // Stop alarm and turn it off completely
        alarmTriggered = false;
        alarmTurnedOn = false;  // Turn off alarm completely
        Serial.println("Alarm turned off");
        noTone(PIEZO);  // Stop alarm sound
      }
    });

    debounceBtn(CONFIRM_BTN, &confirmState, &lastConfirmState, &lastDebounceTimeConfirm, []() {
      if (alarmTriggered) {
        // Snooze alarm - add 5 minutes to alarm time
        DateTime now = rtc.now();
        alarmMinute += 5;
        if (alarmMinute > 59) {
          alarmMinute -= 60;
          alarmHour++;
          if (alarmHour > 23) {
            alarmHour = 0;
            alarmDay++;
            if (alarmDay > getDaysInMonth(now.month(), now.year())) {
              alarmDay = 1;
            }
          }
        }
        alarmTriggered = false;
        alarmAlreadyTriggered = false;  // Reset for new snooze time
        Serial.print("Alarm snoozed to ");
        Serial.print(formatTimeValue(alarmHour));
        Serial.print(":");
        Serial.println(formatTimeValue(alarmMinute));
        noTone(PIEZO);  // Stop alarm sound
      } else {
        // Go back to setting mode
        settingAlarm = true;
        currentSetting = 0;
        Serial.println("======== ALARM ========");
        displayCurrentTime();
        Serial.print("Select day -> ");
        Serial.println(formatTimeValue(alarmDay));
      }
    });
  }

  // Check if alarm should trigger (check every second)
  static unsigned long lastAlarmCheck = 0;
  if (millis() - lastAlarmCheck >= 1000) {
    DateTime now = rtc.now();

    // Reset alarmAlreadyTriggered when time moves past the alarm time
    if (alarmAlreadyTriggered) {
      if (now.day() != alarmDay || now.hour() != alarmHour || now.minute() != alarmMinute) {
        alarmAlreadyTriggered = false;
      }
    }

    if (alarmTurnedOn && !alarmTriggered && !settingAlarm && !alarmAlreadyTriggered) {
      if (now.day() == alarmDay && now.hour() == alarmHour && now.minute() == alarmMinute) {
        alarmTriggered = true;
        alarmAlreadyTriggered = true;  // Prevent re-triggering in same minute
        alarmStartTime = millis();
        Serial.println("*** ALARM TRIGGERED! ***");
        displayCurrentTime();
        Serial.println("Press CONFIRM, ADD (snooze), or SUBTRACT to stop");
      }
    }
    lastAlarmCheck = millis();
  }

  // Handle alarm sound
  if (alarmTriggered) {
    // Stop alarm after specified duration
    if (millis() - alarmStartTime >= alarmDuration) {
      alarmTriggered = false;
      noTone(PIEZO);
      Serial.println("Alarm auto-stopped after timeout");
    } else {
      // Generate alarm beep pattern
      if (millis() - lastAlarmTime >= alarmInterval) {
        alarmBeepState = !alarmBeepState;
        if (alarmBeepState) {
          tone(PIEZO, 1000);  // 1kHz tone
        } else {
          noTone(PIEZO);
        }
        lastAlarmTime = millis();
      }
    }
  } else {
    noTone(PIEZO);  // Ensure piezo is off when alarm not triggered
  }

  // Display current time every 10 seconds (optional - remove if too verbose)
  static unsigned long lastTimeDisplay = 0;
  if (millis() - lastTimeDisplay >= 10000) {
    if (!settingAlarm && !alarmTriggered) {
      displayCurrentTime();
    }
    lastTimeDisplay = millis();
  }
}

// Display current time from RTC
void displayCurrentTime() {
  DateTime now = rtc.now();
  Serial.print("Current time: ");
  Serial.print(monthNames[now.month() - 1]);
  Serial.print(" ");
  Serial.print(formatTimeValue(now.day()));
  Serial.print(" ");
  Serial.print(formatTimeValue(now.hour()));
  Serial.print(":");
  Serial.print(formatTimeValue(now.minute()));
  Serial.print(":");
  Serial.println(formatTimeValue(now.second()));
}

// Format time values to two digits
String formatTimeValue(int value) {
  return (value < 10 ? "0" : "") + String(value);
}

// Get days in month (handles leap years)
byte getDaysInMonth(byte month, int year) {
  byte daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

  if (month == 2 && isLeapYear(year)) {
    return 29;
  }
  return daysInMonth[month - 1];
}

// Check if year is leap year
bool isLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// Function for button debouncing
void debounceBtn(byte btnPin, int* btnState, int* lastBtnState, unsigned long* lastDebounceTime, void (*callback)()) {
  int reading = digitalRead(btnPin);
  if (reading != *lastBtnState) {
    *lastDebounceTime = millis();
  }

  if ((millis() - *lastDebounceTime) > debounceDelay) {
    if (reading != *btnState) {
      *btnState = reading;
      if (*btnState == LOW) {
        callback();
      }
    }
  }
  *lastBtnState = reading;
}
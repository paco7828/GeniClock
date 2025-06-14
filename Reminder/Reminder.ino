#include <Wire.h>
#include <RTClib.h>

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

// Reminder settings
byte reminderMinutes = 1;  // Default 1 minute reminder
String reminderName = "";  // Reminder name
bool reminderActive = false;
DateTime lastReminderTime;

// Setting modes
byte currentSetting = 0;
/*
0 -> Set minutes for reminder (1-60)
1 -> Set name for reminder (3-8 chars)
2 -> Ready mode (reminder set, can turn on/off)
*/

// Character selection for name setting
const char availableChars[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const byte totalChars = sizeof(availableChars) - 1;
const byte FINISH_INDEX = 255;  // Special index for FINISH option
byte currentCharIndex = 1;      // Start with 'A'
byte namePosition = 0;

void setup() {
  Serial.begin(9600);
  pinMode(PIEZO, OUTPUT);
  pinMode(CONFIRM_BTN, INPUT_PULLUP);
  pinMode(ADD_BTN, INPUT_PULLUP);
  pinMode(SUBTRACT_BTN, INPUT_PULLUP);

  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }

  // If RTC lost power, set the time to compile time
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, setting time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  Serial.println("======== DS3231 Reminder System ========");
  displayCurrentSetting();
}

void loop() {
  // Handle button inputs
  debounceBtn(CONFIRM_BTN, &confirmState, &lastConfirmState, &lastDebounceTimeConfirm, handleConfirmBtn);
  debounceBtn(ADD_BTN, &addState, &lastAddState, &lastDebounceTimeAdd, handleAddBtn);
  debounceBtn(SUBTRACT_BTN, &subtractState, &lastSubtractState, &lastDebounceTimeSubtract, handleSubtractBtn);

  // Check for reminder trigger if reminder is active
  if (reminderActive && currentSetting == 2) {
    checkReminder();
  }

  delay(100);  // Small delay to prevent excessive checking
}

void handleConfirmBtn() {
  if (currentSetting == 0) {
    // Confirm minute setting, move to name setting
    currentSetting = 1;
    reminderName = "";
    namePosition = 0;
    currentCharIndex = 1;  // Start with 'A'
    Serial.println("Minutes set!");
    displayCurrentSetting();

  } else if (currentSetting == 1) {
    // Confirm character selection for name
    if (currentCharIndex == FINISH_INDEX) {
      // User selected FINISH option
      finishNameSetting();
    } else if (namePosition < 8) {
      // Add selected character to name
      reminderName += availableChars[currentCharIndex];
      namePosition++;

      if (namePosition < 8) {
        currentCharIndex = 1;  // Reset to 'A' for next character
        displayCurrentSetting();
      } else {
        // Max 8 characters reached, automatically finish
        finishNameSetting();
      }
    }

  } else if (currentSetting == 2) {
    // In ready mode, CONFIRM finishes name if minimum length reached
    if (currentSetting == 1 && reminderName.length() >= 3) {
      finishNameSetting();
    }
  }
}

void handleAddBtn() {
  if (currentSetting == 0) {
    // Setting minutes
    if (reminderMinutes < 60) {
      reminderMinutes++;
      displayCurrentSetting();
    }

  } else if (currentSetting == 1) {
    // Setting name - cycle through characters
    if (reminderName.length() >= 3) {
      // After 3+ characters, include FINISH option
      if (currentCharIndex == totalChars - 1) {
        currentCharIndex = FINISH_INDEX;  // Go to FINISH option
      } else if (currentCharIndex == FINISH_INDEX) {
        currentCharIndex = 0;  // Go back to space/start of characters
      } else {
        currentCharIndex++;
      }
    } else {
      // Less than 3 characters, normal cycling
      currentCharIndex = (currentCharIndex + 1) % totalChars;
    }
    displayCurrentSetting();

  } else if (currentSetting == 2) {
    // Ready mode - turn reminder ON (only if it's OFF)
    if (!reminderActive) {
      reminderActive = true;
      lastReminderTime = rtc.now();
      Serial.println("Reminder turned ON!");
      displayReminderStatus();
      playBeep(1);  // Confirmation beep
    }
  }
}

void handleSubtractBtn() {
  if (currentSetting == 0) {
    // Setting minutes
    if (reminderMinutes > 1) {
      reminderMinutes--;
      displayCurrentSetting();
    }

  } else if (currentSetting == 1) {
    // Setting name - cycle through characters (backwards)
    if (reminderName.length() >= 3) {
      // After 3+ characters, include FINISH option
      if (currentCharIndex == 0) {
        currentCharIndex = FINISH_INDEX;  // Go to FINISH option
      } else if (currentCharIndex == FINISH_INDEX) {
        currentCharIndex = totalChars - 1;  // Go to last character
      } else {
        currentCharIndex--;
      }
    } else {
      // Less than 3 characters, normal cycling
      currentCharIndex = (currentCharIndex == 0) ? totalChars - 1 : currentCharIndex - 1;
    }
    displayCurrentSetting();

  } else if (currentSetting == 2) {
    // Ready mode - turn reminder OFF (only if it's ON)
    if (reminderActive) {
      reminderActive = false;
      Serial.println("Reminder turned OFF!");
      displayReminderStatus();
      playBeep(2);  // Confirmation beeps
    }
  }
}

void finishNameSetting() {
  if (reminderName.length() < 3) {
    Serial.println("Name too short! Minimum 3 characters required.");
    return;
  }

  // Trim name and move to ready mode
  reminderName.trim();
  currentSetting = 2;
  Serial.println("Reminder setup complete!");
  Serial.println("Name: '" + reminderName + "'");
  Serial.print("Interval: ");
  Serial.print(reminderMinutes);
  Serial.println(" minute(s)");
  Serial.println("Use ADD to turn reminder ON, SUBTRACT to turn reminder OFF");
  displayReminderStatus();
}

void displayCurrentSetting() {
  if (currentSetting == 0) {
    Serial.print("Set reminder interval: ");
    Serial.print(reminderMinutes);
    Serial.println(" minute(s) - Press CONFIRM to continue");

  } else if (currentSetting == 1) {
    Serial.print("Setting name - Position ");
    Serial.print(namePosition + 1);
    Serial.print("/8: '");
    Serial.print(reminderName);

    if (currentCharIndex == FINISH_INDEX) {
      Serial.print("[FINISH]");
    } else {
      Serial.print(availableChars[currentCharIndex]);
    }

    Serial.println("' - ADD/SUBTRACT to change, CONFIRM to select");

    if (reminderName.length() >= 3) {
      Serial.println("(Use ADD/SUBTRACT to find [FINISH] option to complete name)");
    }

  } else if (currentSetting == 2) {
    displayReminderStatus();
  }
}

void displayReminderStatus() {
  Serial.print("Reminder '");
  Serial.print(reminderName);
  Serial.print("' every ");
  Serial.print(reminderMinutes);
  Serial.print(" minute(s) - Status: ");
  Serial.println(reminderActive ? "ON" : "OFF");
}

void checkReminder() {
  DateTime now = rtc.now();

  // Calculate time difference in seconds
  long timeDiff = now.unixtime() - lastReminderTime.unixtime();

  // Check if the set number of minutes have passed
  if (timeDiff >= (reminderMinutes * 60)) {
    triggerReminder();
    lastReminderTime = now;  // Reset the timer
  }
}

void triggerReminder() {
  DateTime now = rtc.now();
  Serial.print("REMINDER: '");
  Serial.print(reminderName);
  Serial.print("' - Time: ");
  Serial.print(formatTimeValue(now.hour()));
  Serial.print(":");
  Serial.print(formatTimeValue(now.minute()));
  Serial.print(":");
  Serial.print(formatTimeValue(now.second()));
  Serial.print(" (Next in ");
  Serial.print(reminderMinutes);
  Serial.println(" minute(s))");

  // Play reminder beeps
  playBeep(3);
}

void playBeep(int beepCount) {
  for (int i = 0; i < beepCount; i++) {
    // Generate beep sound
    for (int j = 0; j < 100; j++) {
      digitalWrite(PIEZO, HIGH);
      delayMicroseconds(500);  // Adjust for desired frequency
      digitalWrite(PIEZO, LOW);
      delayMicroseconds(500);
    }
    delay(200);  // Pause between beeps
  }
}

// Format time values to two digits
String formatTimeValue(int value) {
  return (value < 10 ? "0" : "") + String(value);
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
// Button pins
const byte CONFIRM_BTN = A0;
const byte ADD_BTN = A1;
const byte SUBTRACT_BTN = A2;

// Piezo pin
const byte PIEZO = A3;

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
bool settingTimer = true;
bool startedTimer = false;

// Timer variables
String timerValue = "00:00:00";
int currentHours, currentMinutes, currentSeconds;

// Countdown variables
unsigned long previousMillis = 0;
const unsigned long interval = 1000;

void setup() {
  // Set inputs & outputs
  pinMode(CONFIRM_BTN, INPUT_PULLUP);
  pinMode(ADD_BTN, INPUT_PULLUP);
  pinMode(SUBTRACT_BTN, INPUT_PULLUP);
  pinMode(PIEZO, OUTPUT);

  // Start serial communcation with default message
  Serial.begin(9600);
  Serial.println("================================");
  Serial.println("Timer");
  Serial.println("================================");
  Serial.print("Set hours -> ");
  Serial.println(formatTimerValues(currentHours));
}

void loop() {
  // If the timer is in setting mode
  if (settingTimer) {
    // CONFIRM_BTN -> confirms values (hours, minutes, seconds)
    debounceBtn(CONFIRM_BTN, &confirmState, &lastConfirmState, &lastDebounceTimeConfirm, []() {
      handleValueConfirm();
    });

    // ADD_BTN -> adds values (hours, minutes, seconds)
    debounceBtn(ADD_BTN, &addState, &lastAddState, &lastDebounceTimeAdd, []() {
      handleAdd();
    });

    // SUBTRACT_BTN -> subtracts values (hours, minutes, seconds)
    debounceBtn(SUBTRACT_BTN, &subtractState, &lastSubtractState, &lastDebounceTimeSubtract, []() {
      handleSubtract();
    });
  }
  // If timer is not started
  else if (!startedTimer) {
    // CONFIRM_BTN -> starts the timer
    debounceBtn(CONFIRM_BTN, &confirmState, &lastConfirmState, &lastDebounceTimeConfirm, []() {
      handleTimerStart();
    });
  }
  // If timer is started
  else if (startedTimer) {
    // Countdown begins
    handleCountdown();

    // CONFIRM_BTN -> stops the timer
    debounceBtn(CONFIRM_BTN, &confirmState, &lastConfirmState, &lastDebounceTimeConfirm, []() {
      handleTimerStop();
    });
  }
}

// Piezo function when timer goes off
void timerAlarm() {
  Serial.println("!!!TIME IS UP!!!");
  for (int i = 0; i < 3; i++) {
    for (int i = 0; i < 3; i++) {
      tone(PIEZO, 1000, 100);
      delay(200);
    }
    delay(1000);
  }
}

// Timer countdown logic
void handleCountdown() {
  unsigned long currentMillis = millis();

  // Every 1 second
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // If seconds are greater than 0 -> subtract 1 from seconds
    if (currentSeconds > 0) {
      currentSeconds--;
    }
    // If minutes are greater than 0 -> subtract 1 from minutes & set seconds to 59
    else if (currentMinutes > 0) {
      currentMinutes--;
      currentSeconds = 59;
    }
    // If hours are greater than 0 -> subtract 1 from hours & set seconds + minutes to 59
    else if (currentHours > 0) {
      currentHours--;
      currentMinutes = 59;
      currentSeconds = 59;
    }
    // When countdown is over -> piezo goes off & reset everything to default
    else {
      timerAlarm();

      startedTimer = false;
      settingTimer = true;
      currentSetting = 0;
      currentHours = 0;
      currentMinutes = 0;
      currentSeconds = 0;

      Serial.println("Set new timer: ");
      Serial.print("Set hours -> ");
      Serial.println(formatTimerValues(currentHours));
      return;
    }

    // Update and display current timer value
    timerValue = formatTimerValues(currentHours) + ":" + formatTimerValues(currentMinutes) + ":" + formatTimerValues(currentSeconds);
    Serial.print("Time remaining: ");
    Serial.println(timerValue);
  }
}

// Function to handle value confirmations (hours, minutes, seconds)
void handleValueConfirm() {
  tone(PIEZO, 1000, 50);
  switch (currentSetting) {
    // Hours
    case 0:
      Serial.println("Confirmed hours");
      Serial.print("Set minutes -> ");
      Serial.println(formatTimerValues(currentMinutes));
      break;
    // Minutes
    case 1:
      Serial.println("Confirmed minutes");
      Serial.print("Set seconds -> ");
      Serial.println(formatTimerValues(currentSeconds));
      break;
    // Seconds
    case 2:
      Serial.println("Confirmed timer settings");
      Serial.print("Timer set: ");
      timerValue = formatTimerValues(currentHours) + ":" + formatTimerValues(currentMinutes) + ":" + formatTimerValues(currentSeconds);
      Serial.println(timerValue);
      settingTimer = false;
      break;
  }
  currentSetting++;
  if (currentSetting == 3) currentSetting = 0;
}

// Function to handle adding to values (hours, minutes, seconds)
void handleAdd() {
  tone(PIEZO, 1000, 50);
  switch (currentSetting) {
    // Hours
    case 0:
      currentHours++;
      if (currentHours > 24) {
        currentHours = 0;
      }
      Serial.print("Current hours: ");
      Serial.println(formatTimerValues(currentHours));
      break;
    // Minutes
    case 1:
      currentMinutes++;
      if (currentMinutes > 60) {
        currentMinutes = 0;
      }
      Serial.print("Current minutes: ");
      Serial.println(formatTimerValues(currentMinutes));
      break;
    // Seconds
    case 2:
      currentSeconds++;
      if (currentSeconds > 60) {
        currentSeconds = 0;
      }
      Serial.print("Current seconds: ");
      Serial.println(formatTimerValues(currentSeconds));
      break;
  }
}

// Function to handle subtraction from values (hours, minutes, seconds)
void handleSubtract() {
  tone(PIEZO, 1000, 50);
  switch (currentSetting) {
    // Hours
    case 0:
      currentHours--;
      if (currentHours < 0) {
        currentHours = 24;
      }
      Serial.print("Current hours: ");
      Serial.println(formatTimerValues(currentHours));
      break;
    // Minutes
    case 1:
      currentMinutes--;
      if (currentMinutes < 0) {
        currentMinutes = 60;
      }
      Serial.print("Current minutes: ");
      Serial.println(formatTimerValues(currentMinutes));
      break;
    // Seconds
    case 2:
      currentSeconds--;
      if (currentSeconds < 0) {
        currentSeconds = 60;
      }
      Serial.print("Current seconds: ");
      Serial.println(formatTimerValues(currentSeconds));
      break;
  }
}

// Function to handle start of timer
void handleTimerStart() {
  tone(PIEZO, 1000, 50);
  Serial.println("Timer started");
  startedTimer = true;
}

// Function to handle pause of timer
void handleTimerStop() {
  tone(PIEZO, 1000, 50);
  Serial.println("Stopped timer");
  startedTimer = false;
}

// Function to format values correctly (hours, minutes seconds)
String formatTimerValues(int value) {
  String valueStr = String(value);

  if (value < 10) {
    valueStr = "0" + valueStr;
  }

  return valueStr;
}

// Function for button debouncing
void debounceBtn(byte btnPin, int* btnState, int* lastBtnState, unsigned long* lastDebounceTime, void (*callback)()) {
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
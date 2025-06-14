// =================== Pomodoro Timer ===================
// Button pins
const byte CONFIRM_BTN = A1;
const byte ADD_BTN = A2;
const byte SUBTRACT_BTN = A3;

// Piezo pin
const byte PIEZO = A0;

// Button states
int lastConfirmState = HIGH;
int lastAddState = HIGH;
int lastSubtractState = HIGH;
int confirmState = HIGH;
int addState = HIGH;
int subtractState = HIGH;

// Debounce helpers
unsigned long lastDebounceTimeConfirm = 0;
unsigned long lastDebounceTimeAdd = 0;
unsigned long lastDebounceTimeSubtract = 0;
const unsigned long debounceDelay = 50;

// Timer settings
int workMinutes = 0;
int breakMinutes = 0;
byte currentSetting = 0;
bool settingPomodoro = true;
bool pomodoroStarted = false;

// Phase tracking
bool isWorkPhase = true;
unsigned long phaseStartTime = 0;
unsigned long phaseDuration = 0;
int completedWorkSessions = 0;

// Setup
void setup() {
  pinMode(CONFIRM_BTN, INPUT_PULLUP);
  pinMode(ADD_BTN, INPUT_PULLUP);
  pinMode(SUBTRACT_BTN, INPUT_PULLUP);
  pinMode(PIEZO, OUTPUT);

  Serial.begin(9600);
  Serial.println("=========== POMODORO TIMER ===========");
  Serial.print("Set work minutes -> ");
  Serial.println(formatTimeValue(workMinutes));
}

// Main loop
void loop() {
  if (settingPomodoro) {
    debounceBtn(CONFIRM_BTN, &confirmState, &lastConfirmState, &lastDebounceTimeConfirm, []() {
      if (currentSetting == 0) {
        Serial.print("Work minutes set -> ");
        Serial.println(formatTimeValue(workMinutes));
        currentSetting = 1;
        Serial.print("Set break minutes -> ");
        Serial.println(formatTimeValue(breakMinutes));
      } else if (currentSetting == 1) {
        Serial.print("Break minutes set -> ");
        Serial.println(formatTimeValue(breakMinutes));
        Serial.print("Pomodoro: ");
        Serial.print(workMinutes);
        Serial.print("W ");
        Serial.print(breakMinutes);
        Serial.println("B");
        settingPomodoro = false;
      }
    });

    debounceBtn(ADD_BTN, &addState, &lastAddState, &lastDebounceTimeAdd, []() {
      if (currentSetting == 0) {
        workMinutes += 5;
        Serial.print("Work: ");
        Serial.println(formatTimeValue(workMinutes));
      } else if (currentSetting == 1) {
        breakMinutes += 5;
        Serial.print("Break: ");
        Serial.println(formatTimeValue(breakMinutes));
      }
    });

    debounceBtn(SUBTRACT_BTN, &subtractState, &lastSubtractState, &lastDebounceTimeSubtract, []() {
      if (currentSetting == 0 && workMinutes >= 5) {
        workMinutes -= 5;
        Serial.print("Work: ");
        Serial.println(formatTimeValue(workMinutes));
      } else if (currentSetting == 1 && breakMinutes >= 5) {
        breakMinutes -= 5;
        Serial.print("Break: ");
        Serial.println(formatTimeValue(breakMinutes));
      }
    });

  } else {
    debounceBtn(CONFIRM_BTN, &confirmState, &lastConfirmState, &lastDebounceTimeConfirm, []() {
      if (pomodoroStarted) {
        Serial.println("Pomodoro timer stopped");
        pomodoroStarted = false;
      } else {
        Serial.println("Pomodoro timer started");
        pomodoroStarted = true;
        isWorkPhase = true;
        completedWorkSessions = 0;
        phaseStartTime = millis();
        phaseDuration = workMinutes * 60000UL;
        Serial.print("Work started: ");
        Serial.print(workMinutes);
        Serial.println(" minutes");
      }
    });

    if (pomodoroStarted) {
      handlePomodoroCd();
    }
  }
}

// Handle the countdown logic
void handlePomodoroCd() {
  unsigned long currentTime = millis();

  if (currentTime - phaseStartTime >= phaseDuration) {
    tone(PIEZO, 1000, 500);  // Beep
    isWorkPhase = !isWorkPhase;

    if (isWorkPhase) {
      // Start work
      Serial.print("Work started: ");
      Serial.print(workMinutes);
      Serial.println(" minutes");

      phaseDuration = workMinutes * 60000UL;
      phaseStartTime = currentTime;
    } else {
      // Work just completed
      completedWorkSessions++;

      int breakLength = breakMinutes;
      if (completedWorkSessions % 4 == 0) {
        breakLength *= 4;
        Serial.print("Long break started: ");
      } else {
        Serial.print("Break started: ");
      }

      Serial.print(breakLength);
      Serial.println(" minutes");

      phaseDuration = breakLength * 60000UL;
      phaseStartTime = currentTime;
    }
  }

  // Display time left every second
  static unsigned long lastUpdate = 0;
  if (currentTime - lastUpdate >= 1000) {
    lastUpdate = currentTime;
    unsigned long timeLeft = (phaseDuration - (currentTime - phaseStartTime)) / 1000;
    int mins = timeLeft / 60;
    int secs = timeLeft % 60;
    Serial.print("Time left: ");
    Serial.print(formatTimeValue(mins));
    Serial.print(":");
    Serial.println(formatTimeValue(secs));
  }
}

// Format time values
String formatTimeValue(int value) {
  if (value < 10) return "0" + String(value);
  return String(value);
}

// Debounce button utility
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

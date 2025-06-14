// Button pins
const byte ACTION_BTN = A1;
const byte CAPTURE_BTN = A2;
const byte RESET_BTN = A3;
// Piezo pin
const byte PIEZO = A0;

// Last button states
int lastActionState = HIGH;
int lastCaptureState = HIGH;
int lastResetState = HIGH;

// Button states
int actionState = HIGH;
int captureState = HIGH;
int resetState = HIGH;

// Helper variables
unsigned long lastDebounceTimeAction = 0;
unsigned long lastDebounceTimeCapture = 0;
unsigned long lastDebounceTimeReset = 0;
const unsigned long debounceDelay = 50;
bool stopperStarted = false;

// Timer variables
unsigned long startTime = 0;
unsigned long elapsedTime = 0;

// Captured times
const byte MAX_CAPTURES = 10;
String capturedTimes[MAX_CAPTURES];
byte capturedIndex = 0;
byte viewIndex = 0;

unsigned long lastDisplayUpdate = 0;

void setup() {
  pinMode(PIEZO, OUTPUT);
  pinMode(ACTION_BTN, INPUT_PULLUP);
  pinMode(CAPTURE_BTN, INPUT_PULLUP);
  pinMode(RESET_BTN, INPUT_PULLUP);

  Serial.begin(9600);
  Serial.println("========Stopper========");
  Serial.println("Press ACTION to start/stop, CAPTURE to save time, RESET to clear.");
}

void loop() {
  debounceBtn(ACTION_BTN, &actionState, &lastActionState, &lastDebounceTimeAction, []() {
    if (!stopperStarted) {
      startTime = millis() - elapsedTime;  // Continue from where left off
      Serial.println("Stopper started");
    } else {
      Serial.println("Stopper stopped");
    }
    stopperStarted = !stopperStarted;
    tone(PIEZO, 1000, 100);
  });

  debounceBtn(CAPTURE_BTN, &captureState, &lastCaptureState, &lastDebounceTimeCapture, []() {
    if (stopperStarted) {
      String timeStr = getFormattedTime();
      if (capturedIndex < MAX_CAPTURES) {
        capturedTimes[capturedIndex++] = timeStr;
        Serial.print("Captured: ");
        Serial.println(timeStr);
      } else {
        Serial.println("Capture limit reached.");
      }
    } else {
      if (capturedIndex == 0) {
        Serial.println("No times captured yet.");
      } else {
        Serial.print("Captured [");
        Serial.print(viewIndex + 1);
        Serial.print("]: ");
        Serial.println(capturedTimes[viewIndex]);
        viewIndex = (viewIndex + 1) % capturedIndex;
      }
    }
    tone(PIEZO, 1200, 100);
  });

  debounceBtn(RESET_BTN, &resetState, &lastResetState, &lastDebounceTimeReset, []() {
    stopperStarted = false;
    elapsedTime = 0;
    startTime = 0;
    capturedIndex = 0;
    viewIndex = 0;
    Serial.println("Stopper reset");
    tone(PIEZO, 800, 100);
  });

  if (stopperStarted) {
    elapsedTime = millis() - startTime;

    Serial.print("Time: ");
    Serial.println(getFormattedTime());
  }
}

String getFormattedTime() {
  unsigned long totalMillis = elapsedTime;
  unsigned long totalSeconds = totalMillis / 1000;
  unsigned long minutes = (totalSeconds / 60) % 60;
  unsigned long hours = totalSeconds / 3600;
  unsigned long seconds = totalSeconds % 60;
  unsigned long milliseconds = totalMillis % 1000;

  return formatTimeValue(hours) + ":" + formatTimeValue(minutes) + ":" + formatTimeValue(seconds) + "." + formatMilliseconds(milliseconds);
}

// Format time values to two digits
String formatTimeValue(int value) {
  return (value < 10 ? "0" : "") + String(value);
}

// Format milliseconds to three digits
String formatMilliseconds(int value) {
  if (value < 10) return "00" + String(value);
  else if (value < 100) return "0" + String(value);
  else return String(value);
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

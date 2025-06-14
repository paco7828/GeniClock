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
0 -> Set minute for reminder
1 -> Set second for reminder
2 -> Set name for reminder (max 8 chars)
Same button functionalities as alarm (toggle on/off etc)
*/

void setup() {
  Serial.begin(9600);
  pinMode(PIEZO, OUTPUT);
  pinMode(CONFIRM_BTN, INPUT_PULLUP);
  pinMode(ADD_BTN, INPUT_PULLUP);
  pinMode(SUBTRACT_BTN, INPUT_PULLUP);
  Serial.println("======== Reminder ========");
}

void loop() {
}

// Format time values to two digits
String formatTimeValue(int value) {
  return (value < 10 ? "0" : "") + String(value);
}

byte getMaxDaysByMonth(String monthName) {
  monthName.toLowerCase();
  if (monthName == "january") return 31;
  if (monthName == "february") {
    return !(currentYear % 4) ? 29 : 28;
  }
  if (monthName == "march") return 31;
  if (monthName == "april") return 30;
  if (monthName == "may") return 31;
  if (monthName == "june") return 30;
  if (monthName == "july") return 31;
  if (monthName == "august") return 31;
  if (monthName == "september") return 30;
  if (monthName == "october") return 31;
  if (monthName == "november") return 30;
  if (monthName == "december") return 31;
  return 30;
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
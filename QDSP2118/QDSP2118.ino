// Define pin numbers for control and data lines
const byte RST = 10;  // Reset pin
const byte CE = 2;    // Chip Enable pin
const byte D0 = 3;    // Data pins (7 bits used)
const byte D1 = 4;
const byte D2 = 5;
const byte D3 = 6;
const byte D4 = 7;
const byte D5 = 8;
const byte D6 = 9;

// Group data pins into an array
const byte DATA_PINS[] = { D0, D1, D2, D3, D4, D5, D6 };

// Define address pins
const byte ADDR0 = 11;
const byte ADDR1 = 12;
const byte ADDR2 = 13;
const byte ADDR_PINS[] = { ADDR0, ADDR1, ADDR2 };

// Setup function - runs once at the beginning
void setup() {
  pinMode(RST, OUTPUT);
  pinMode(CE, OUTPUT);
  pinMode(51, OUTPUT);  // Optional or unused

  for (int i = 0; i <= 6; i++) {
    pinMode(DATA_PINS[i], OUTPUT);
  }
  for (int j = 0; j < 3; j++) {
    pinMode(ADDR_PINS[j], OUTPUT);
  }

  Serial.begin(9600);
  digitalWrite(CE, HIGH);
  delay(100);
  resetDisplay();
}

// Main loop - repeats endlessly
void loop() {
  char text[] = "Szevasz tavasz! Nyitva 0/24";
  displayText("Welcome!");
}

// Function to reset the display
void resetDisplay() {
  digitalWrite(RST, LOW);
  delayMicroseconds(1);
  digitalWrite(RST, HIGH);
  delayMicroseconds(150);
}

// Function to write a full 8-character string to display
void sendToDisplay(char* data) {
  for (int addr = 0; addr < 8; addr++) {
    // Set address pins (3-bit binary)
    digitalWrite(ADDR0, bitRead(addr, 0));
    digitalWrite(ADDR1, bitRead(addr, 1));
    digitalWrite(ADDR2, bitRead(addr, 2));

    // Get character to display at this address
    char ch = data[addr];
    if (ch == '\0') ch = ' ';  // Replace null with space

    // Send each bit of the character to data pins (only 7 bits used)
    for (int i = 0; i <= 6; i++) {
      digitalWrite(DATA_PINS[i], bitRead(ch, i));
    }

    delay(1);
    digitalWrite(CE, LOW);
    delay(1);
    digitalWrite(CE, HIGH);
  }
}

void scrollText(char* words) {
  char buffer[9];
  int i = 0;

  while (words[i] - 1 != 0) {
    bool blank = false;

    for (int j = 0; j < 8; j++) {
      if (!blank && words[i + j] == 0) {
        blank = true;
      }
      buffer[j] = blank ? ' ' : words[i + j];
    }

    buffer[8] = 0;
    sendToDisplay(buffer);

    if (i == 0) delay(500);
    delay(200);
    i++;
  }
}

void displayText(char* text) {
  char buffer[9];

  for (int i = 0; i < 8; i++) {
    buffer[i] = (text[i] != '\0') ? text[i] : ' ';
  }
  buffer[8] = '\0';  // Null-terminate the string

  sendToDisplay(buffer);
}

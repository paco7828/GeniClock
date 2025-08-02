#include "register.h"

class HDSPDisplay {
private:
  // Register class instance
  Register helperRegister;

  // Store last displayed content to avoid unnecessary updates
  char lastDisplayedText[9];
  bool displayInitialized;

  // Helper function used by displayText function
  void sendToDisplay(char* data) {
    for (int addr = 0; addr < 8; addr++) {
      char ch = data[addr];
      if (ch == '\0') ch = ' ';

      byte reg2 = 0b00000001 | ((addr & 0x07) << 1);
      byte reg1 = (ch & 0x7F) << 1;
      reg1 &= 0xFE;  // CE=0 (active)

      this->helperRegister.shiftOutRegisters(reg2, reg1);
      delay(2);

      reg1 |= 0x01;  // CE=1 (inactive)
      this->helperRegister.shiftOutRegisters(reg2, reg1);
      delay(2);
    }
  }

public:
  HDSPDisplay(byte ser, byte srclk, byte rclk)
    : helperRegister(ser, srclk, rclk), displayInitialized(false) {
    // Initialize last displayed text to empty
    for (int i = 0; i < 9; i++) {
      lastDisplayedText[i] = '\0';
    }
  }

  void begin() {
    this->helperRegister.begin();
    displayInitialized = false;
  }

  void resetDisplay() {
    byte reg2 = 0b00000000;  // RST=0
    byte reg1 = 0b00000001;  // CE=1 (disabled)

    this->helperRegister.shiftOutRegisters(reg2, reg1);
    delayMicroseconds(10);

    reg2 = 0b00000001;  // RST=1
    this->helperRegister.shiftOutRegisters(reg2, reg1);
    delayMicroseconds(150);

    // Clear the last displayed text since we reset
    for (int i = 0; i < 9; i++) {
      lastDisplayedText[i] = '\0';
    }
    displayInitialized = true;
  }

  void displayText(char* text) {
    char buffer[9];

    // Prepare the buffer with proper spacing
    for (int i = 0; i < 8; i++) {
      buffer[i] = (text[i] != '\0') ? text[i] : ' ';
    }
    buffer[8] = '\0';

    // Check if content has changed or display needs initialization
    bool contentChanged = false;
    if (!displayInitialized) {
      contentChanged = true;
    } else {
      for (int i = 0; i < 8; i++) {
        if (buffer[i] != lastDisplayedText[i]) {
          contentChanged = true;
          break;
        }
      }
    }

    // Only update if content changed or display needs reset
    if (contentChanged) {
      // Only reset display if not initialized
      if (!displayInitialized) {
        resetDisplay();
      }

      this->sendToDisplay(buffer);

      // Store the current content
      for (int i = 0; i < 9; i++) {
        lastDisplayedText[i] = buffer[i];
      }
    }
  }

  void displayTime(byte hour, byte minute, byte second) {
    char buffer[9];
    sprintf(buffer, "%02d:%02d:%02d", hour, minute, second);
    this->displayText(buffer);
  }

  void displayYearMonth(int year, byte month) {
    char buffer[9];
    sprintf(buffer, "%d. %02d", year, month);
    this->displayText(buffer);
  }

  void displayDayAndName(byte day, byte dayIndex) {
    char buffer[9];
    sprintf(buffer, " %02d %s ", day, DAYS_OF_WEEK[dayIndex]);
    this->displayText(buffer);
  }

  void displayTemperature(float temperature) {
    char buffer[9];

    // Handle different temperature ranges for proper formatting
    if (temperature >= 100.0 || temperature <= -100.0) {
      // For very high/low temperatures, show without decimal
      sprintf(buffer, " %03.0f C ", abs((int)temperature));
    } else if (temperature >= 10.0 || temperature <= -10.0) {
      // For double digit temperatures: "-24.5 C-"
      sprintf(buffer, " %04.1f C ", abs(temperature));
    } else if (temperature >= 0.0) {
      // For single digit positive temperatures: "-5.25 C-"
      sprintf(buffer, " %04.2f C ", temperature);
    } else {
      // For single digit negative temperatures: "--5.2 C-"
      sprintf(buffer, " -%03.1f C ", abs(temperature));
    }

    this->displayText(buffer);
  }

  void displayGPSLatitude(double latitude) {
    char buffer[9];

    // Format latitude to show 8 digits with proper precision
    // Example: 47.49735 (8 characters including decimal point)
    if (latitude >= 0) {
      sprintf(buffer, "%8.5f", latitude);
    } else {
      sprintf(buffer, "%8.4f", latitude);  // Negative sign takes one character
    }

    this->displayText(buffer);
  }

  void displayGPSLongitude(double longitude) {
    char buffer[9];

    // Format longitude to show 8 digits with proper precision
    // Example: 19.04075 (8 characters including decimal point)
    if (longitude >= 0) {
      sprintf(buffer, "%8.5f", longitude);
    } else {
      sprintf(buffer, "%8.4f", longitude);  // Negative sign takes one character
    }

    this->displayText(buffer);
  }

  void displayGPSSpeed(double speed) {
    char buffer[9];

    // Format speed as "XXkm/h " with proper formatting
    if (speed >= 100.0) {
      sprintf(buffer, "%03.0f km/h ", speed);  // "123 km/h "
    } else if (speed >= 10.0) {
      sprintf(buffer, " %02.0f km/h ", speed);  // " 50 km/h "
    } else {
      sprintf(buffer, " %01.0f km/h ", speed);  // " 5 km/h "
    }

    this->displayText(buffer);
  }

  // Force a display reset and update (useful for status messages)
  void forceDisplayText(char* text) {
    resetDisplay();
    displayText(text);
  }
};
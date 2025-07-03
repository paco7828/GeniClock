#include "register.h"

class HDSPDisplay {
private:
  // Register class instance
  Register helperRegister;

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
    : helperRegister(ser, srclk, rclk) {}

  void begin() {
    this->helperRegister.begin();
  }

  void resetDisplay() {
    byte reg2 = 0b00000000;  // RST=0
    byte reg1 = 0b00000001;  // CE=1 (disabled)

    this->helperRegister.shiftOutRegisters(reg2, reg1);
    delayMicroseconds(10);

    reg2 = 0b00000001;  // RST=1
    this->helperRegister.shiftOutRegisters(reg2, reg1);
    delayMicroseconds(150);
  }

  void displayText(char* text) {
    resetDisplay();
    char buffer[9];
    for (int i = 0; i < 8; i++) {
      buffer[i] = (text[i] != '\0') ? text[i] : ' ';
    }
    buffer[8] = '\0';
    this->sendToDisplay(buffer);
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
    sprintf(buffer, " %02d %s ", day, daysOfWeek[dayIndex]);
    this->displayText(buffer);
  }
};
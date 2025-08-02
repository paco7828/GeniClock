#ifndef REGISTER_H
#define REGISTER_H

class Register {
private:
  byte SER;
  byte SRCLK;
  byte RCLK;

public:
  Register(byte ser, byte srclk, byte rclk) {
    this->SER = ser;
    this->SRCLK = srclk;
    this->RCLK = rclk;
  }

  void begin() {
    pinMode(this->SER, OUTPUT);
    pinMode(this->SRCLK, OUTPUT);
    pinMode(this->RCLK, OUTPUT);
  }

  void shiftOutRegisters(byte reg2, byte reg1) {
    digitalWrite(this->RCLK, LOW);
    shiftOut(this->SER, this->SRCLK, MSBFIRST, reg2);
    shiftOut(this->SER, this->SRCLK, MSBFIRST, reg1);
    digitalWrite(this->RCLK, HIGH);
  }
};

#endif  // REGISTER_H
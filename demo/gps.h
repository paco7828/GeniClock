#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

class GPS {
private:
  HardwareSerial gpsSerial;
  TinyGPSPlus gps;

  // Zeller’s Congruence to calculate day of the week
  int calculateDayOfWeek(int y, int m, int d) {
    if (m < 3) {
      m += 12;
      y -= 1;
    }
    int k = y % 100;
    int j = y / 100;
    int f = d + 13 * (m + 1) / 5 + k + k / 4 + j / 4 + 5 * j;
    return ((f + 5) % 7);  // Sunday = 0
  }

public:
  GPS()
    : gpsSerial(1) {}

  void begin(byte gpsRx) {
    gpsSerial.begin(9600, SERIAL_8N1, gpsRx, -1);
  }

  void update() {
    while (gpsSerial.available()) {
      gps.encode(gpsSerial.read());
    }
  }

  bool hasFix() {
    return gps.location.isValid();
  }

  double getLatitude() {
    return gps.location.lat();  // 49.1234
  }

  double getLongitude() {
    return gps.location.lng();  // 18.1234
  }

  double getSpeedKmph() {
    return gps.speed.kmph();  // 72.15
  }

  int getYear() {
    return gps.date.isValid() ? gps.date.year() : 0;
  }

  byte getMonth() {
    return gps.date.isValid() ? gps.date.month() : 0;
  }

  byte getDay() {
    return gps.date.isValid() ? gps.date.day() : 0;
  }

  byte getHour() {
    return gps.time.isValid() ? gps.time.hour() : 0;
  }

  byte getMinute() {
    return gps.time.isValid() ? gps.time.minute() : 0;
  }

  byte getSecond() {
    return gps.time.isValid() ? gps.time.second() : 0;
  }

  byte getDayIndex() {
    return this->calculateDayOfWeek(this->getYear(), this->getMonth(), this->getDay());
  }
};

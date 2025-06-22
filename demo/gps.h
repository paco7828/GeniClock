#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

class GPS {
private:
  HardwareSerial gpsSerial;
  TinyGPSPlus gps;

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

  String getDate(char partOfDate) {
    if (gps.date.isValid()) {
      partOfDate.toLowerCase();
      if (partOfDate == "y") {
        return gps.date.year();  // 2025
      } else if (partOfDate == "m") {
        return gps.date.month();  // 5
      } else if (partOfDate == "d") {
        return gps.date.day();  // 30
      } else {
        return "????";
      }
    }

    String getTime(char partOfTime) {
      if (gps.time.isValid()) {
        partOfTime.toLowerCase();
        if (partOfTime == "h") {
          return gps.time.hour(); // 13
        } else if (partOfTime == "m") {
          return gps.time.minute(); // 52
        } else if (partOfTime == "s") {
          return gps.time.second(); // 15
        } else {
          return "??";
        }
      }
    };

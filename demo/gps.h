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
    return gps.location.lat();
  }

  double getLongitude() {
    return gps.location.lng();
  }

  double getSpeedKmph() {
    return gps.speed.kmph();
  }

  String getDate() {
    if (gps.date.isValid()) {
      char buffer[11];
      sprintf(buffer, "%02d/%02d/%04d",
              gps.date.day(), gps.date.month(), gps.date.year());
      return String(buffer);
    }
    return "No Date";
  }

  String getTime() {
    if (gps.time.isValid()) {
      char buffer[9];
      sprintf(buffer, "%02d:%02d:%02d",
              gps.time.hour(), gps.time.minute(), gps.time.second());
      return String(buffer);
    }
    return "No Time";
  }
};

#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

class GPS {
private:
  HardwareSerial gpsSerial;
  TinyGPSPlus gps;

  // Zeller's Congruence to calculate day of the week
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

  // Check if it's daylight saving time in Hungary (CEST)
  // DST starts last Sunday in March at 2:00 AM
  // DST ends last Sunday in October at 3:00 AM
  bool isDaylightSavingTime(int year, int month, int day, int hour) {
    // Before March or after October - definitely standard time
    if (month < 3 || month > 10) {
      return false;
    }

    // April to September - definitely daylight saving time
    if (month > 3 && month < 10) {
      return true;
    }

    // March - need to check if we're past the last Sunday
    if (month == 3) {
      // Find the last Sunday of March
      int lastSunday = 31;
      while (calculateDayOfWeek(year, 3, lastSunday) != 0) {
        lastSunday--;
      }

      if (day < lastSunday) {
        return false;  // Before DST starts
      } else if (day > lastSunday) {
        return true;  // After DST starts
      } else {
        // On the last Sunday - DST starts at 2:00 AM UTC (3:00 AM CET)
        return hour >= 1;  // 1:00 UTC = 2:00 CET when DST starts
      }
    }

    // October - need to check if we're before the last Sunday
    if (month == 10) {
      // Find the last Sunday of October
      int lastSunday = 31;
      while (calculateDayOfWeek(year, 10, lastSunday) != 0) {
        lastSunday--;
      }

      if (day < lastSunday) {
        return true;  // Before DST ends
      } else if (day > lastSunday) {
        return false;  // After DST ends
      } else {
        // On the last Sunday - DST ends at 3:00 AM CEST (1:00 AM UTC)
        return hour < 1;  // Before 1:00 UTC (2:00 AM CET when DST ends)
      }
    }

    return false;
  }

  // Get Hungarian timezone offset (+1 CET, +2 CEST)
  int getHungarianTimezoneOffset(int year, int month, int day, int hour) {
    return isDaylightSavingTime(year, month, day, hour) ? 2 : 1;
  }

  // Convert UTC time to Hungarian local time
  void convertToHungarianTime(int &year, int &month, int &day, int &hour, int &minute, int &second) {
    // First, get the timezone offset based on UTC time
    int offset = getHungarianTimezoneOffset(year, month, day, hour);

    // Add the offset to the hour
    hour += offset;

    // Handle day overflow
    if (hour >= 24) {
      hour -= 24;
      day++;

      // Days in each month
      int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

      // Check for leap year
      bool isLeapYear = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
      if (isLeapYear) daysInMonth[1] = 29;

      // Handle month overflow
      if (day > daysInMonth[month - 1]) {
        day = 1;
        month++;

        // Handle year overflow
        if (month > 12) {
          month = 1;
          year++;
        }
      }
    }

    // Handle day underflow (shouldn't happen with positive offset, but just in case)
    else if (hour < 0) {
      hour += 24;
      day--;

      // Handle day underflow
      if (day < 1) {
        month--;

        // Handle month underflow
        if (month < 1) {
          month = 12;
          year--;
        }

        // Days in each month
        int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

        // Check for leap year
        bool isLeapYear = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        if (isLeapYear) daysInMonth[1] = 29;

        day = daysInMonth[month - 1];
      }
    }
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
    return gps.location.isValid() && gps.date.isValid() && gps.time.isValid();
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

  int getYear() {
    if (!hasFix()) return 0;

    int year = gps.date.year();
    int month = gps.date.month();
    int day = gps.date.day();
    int hour = gps.time.hour();
    int minute = gps.time.minute();
    int second = gps.time.second();

    convertToHungarianTime(year, month, day, hour, minute, second);
    return year;
  }

  byte getMonth() {
    if (!hasFix()) return 0;

    int year = gps.date.year();
    int month = gps.date.month();
    int day = gps.date.day();
    int hour = gps.time.hour();
    int minute = gps.time.minute();
    int second = gps.time.second();

    convertToHungarianTime(year, month, day, hour, minute, second);
    return month;
  }

  byte getDay() {
    if (!hasFix()) return 0;

    int year = gps.date.year();
    int month = gps.date.month();
    int day = gps.date.day();
    int hour = gps.time.hour();
    int minute = gps.time.minute();
    int second = gps.time.second();

    convertToHungarianTime(year, month, day, hour, minute, second);
    return day;
  }

  byte getHour() {
    if (!hasFix()) return 0;

    int year = gps.date.year();
    int month = gps.date.month();
    int day = gps.date.day();
    int hour = gps.time.hour();
    int minute = gps.time.minute();
    int second = gps.time.second();

    convertToHungarianTime(year, month, day, hour, minute, second);
    return hour;
  }

  byte getMinute() {
    if (!hasFix()) return 0;

    int year = gps.date.year();
    int month = gps.date.month();
    int day = gps.date.day();
    int hour = gps.time.hour();
    int minute = gps.time.minute();
    int second = gps.time.second();

    convertToHungarianTime(year, month, day, hour, minute, second);
    return minute;
  }

  byte getSecond() {
    if (!hasFix()) return 0;

    int year = gps.date.year();
    int month = gps.date.month();
    int day = gps.date.day();
    int hour = gps.time.hour();
    int minute = gps.time.minute();
    int second = gps.time.second();

    convertToHungarianTime(year, month, day, hour, minute, second);
    return second;
  }

  byte getDayIndex() {
    if (!hasFix()) return 0;

    int year = gps.date.year();
    int month = gps.date.month();
    int day = gps.date.day();
    int hour = gps.time.hour();
    int minute = gps.time.minute();
    int second = gps.time.second();

    convertToHungarianTime(year, month, day, hour, minute, second);
    return calculateDayOfWeek(year, month, day);
  }

  // Debug function to check if DST is active
  bool isDSTActive() {
    if (!hasFix()) return false;

    int year = gps.date.year();
    int month = gps.date.month();
    int day = gps.date.day();
    int hour = gps.time.hour();

    return isDaylightSavingTime(year, month, day, hour);
  }

  // Get current timezone offset being used
  int getCurrentTimezoneOffset() {
    if (!hasFix()) return 1;

    int year = gps.date.year();
    int month = gps.date.month();
    int day = gps.date.day();
    int hour = gps.time.hour();

    return getHungarianTimezoneOffset(year, month, day, hour);
  }
};
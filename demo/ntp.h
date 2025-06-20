#include <WiFi.h>
#include "time.h"
#include "sntp.h"

class NTP {
private:
  char* SSID;
  char* PASSW;
  struct tm timeInfo;

public:
  NTP(char* ssid, char* passw) {
    this->SSID = ssid;
    this->PASSW = passw;
  }

  void begin() {
    WiFi.begin(this->SSID, this->PASSW);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nWiFi connected!");

    // Set timezone to Hungary (CET/CEST)
    configTzTime("CET-1CEST,M3.5.0/2,M10.5.0/3", "pool.ntp.org");
  }

  bool update() {
    return getLocalTime(&timeInfo);
  }

  String getWeekdayName() {
    if (!update()) return "N/A";
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%A", &timeInfo);
    return String(buffer);
  }

  String getMonthName() {
    if (!update()) return "N/A";
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%B", &timeInfo);
    return String(buffer);
  }

  byte getDayOfMonth() {
    return update() ? timeInfo.tm_mday : 0;
  }

  int getYear() {
    return update() ? timeInfo.tm_year + 1900 : 0;
  }

  byte getHour() {
    return update() ? timeInfo.tm_hour : 0;
  }

  byte getMinute() {
    return update() ? timeInfo.tm_min : 0;
  }

  byte getSecond() {
    return update() ? timeInfo.tm_sec : 0;
  }

  void setNewCredentials(char* ssid, char* passw) {
    this->SSID = ssid;
    this->PASSW = passw;

    WiFi.disconnect();
    WiFi.begin(this->SSID, this->PASSW);

    Serial.print("Reconnecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nWiFi reconnected!");

    // Reconfigure NTP
    configTzTime("CET-1CEST,M3.5.0/2,M10.5.0/3", "pool.ntp.org");
  }
};
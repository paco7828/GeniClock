#include <WiFi.h>
#include "time.h"
#include "sntp.h"

class NTP {
private:
  char* SSID;
  char* PASSW;
  struct tm timeInfo;
  bool hasValidTime;    // Track if we have valid time data
  bool ntpInitialized;  // Track if NTP has been initialized

public:
  NTP(char* ssid, char* passw) {
    this->SSID = ssid;
    this->PASSW = passw;
    this->hasValidTime = false;
    this->ntpInitialized = false;
  }

  void begin() {
    // Only initialize NTP if not already done and WiFi is connected
    if (!ntpInitialized && WiFi.status() == WL_CONNECTED) {
      Serial.println("Initializing NTP...");

      // Set timezone to Hungary (CET/CEST)
      configTzTime("CET-1CEST,M3.5.0/2,M10.5.0/3", "pool.ntp.org");

      ntpInitialized = true;

      // Try to get initial time
      this->update();
    }
  }

  bool update() {
    if (ntpInitialized && getLocalTime(&timeInfo)) {
      this->hasValidTime = true;
      return true;
    }
    return false;
  }

  bool hasTime() {
    return this->hasValidTime;
  }

  byte getMonth() {
    return hasValidTime ? timeInfo.tm_mon + 1 : 0;  // tm_mon is 0-based, so add 1
  }

  byte getDay() {
    return hasValidTime ? timeInfo.tm_mday : 0;
  }

  byte getDayIndex() {
    return hasValidTime ? timeInfo.tm_wday : 0;
  }

  int getYear() {
    return hasValidTime ? timeInfo.tm_year + 1900 : 0;
  }

  byte getHour() {
    return hasValidTime ? timeInfo.tm_hour : 0;
  }

  byte getMinute() {
    return hasValidTime ? timeInfo.tm_min : 0;
  }

  byte getSecond() {
    return hasValidTime ? timeInfo.tm_sec : 0;
  }

  void setNewCredentials(char* ssid, char* passw) {
    this->SSID = ssid;
    this->PASSW = passw;

    WiFi.disconnect();
    ntpInitialized = false;
    hasValidTime = false;

    WiFi.begin(this->SSID, this->PASSW);

    Serial.print("Reconnecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nWiFi reconnected!");

    // Reinitialize NTP
    this->begin();
  }
};
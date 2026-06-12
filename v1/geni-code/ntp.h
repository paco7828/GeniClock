#pragma once

#include <WiFi.h>
#include "time.h"
#include "sntp.h"

class NTP {
private:
  String SSID;
  String PASSW;
  struct tm timeInfo;
  bool hasValidTime;
  bool ntpInitialized;

public:
  NTP() {
    this->hasValidTime = false;
    this->ntpInitialized = false;
  }

  void setCredentials(const char* ssid, const char* passw) {
    this->SSID = String(ssid);
    this->PASSW = String(passw);
  }

  void begin() {
    // Only initialize NTP if not already done and WiFi is connected
    if (!ntpInitialized && WiFi.status() == WL_CONNECTED) {
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

  byte getMonth() {
    return hasValidTime ? timeInfo.tm_mon + 1 : 0;  // tm_mon is 0 based => so add 1
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
};
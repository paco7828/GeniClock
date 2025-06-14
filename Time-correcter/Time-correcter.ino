#include <WiFi.h>
#include <time.h>
#include <SoftwareSerial.h>
#include <RTClib.h>
#include <Wire.h>

// GPS Configuration
SoftwareSerial gpsSerial(4, 2);  // RX, TX pins for GPS module
String gpsData = "";
bool gpsFixed = false;

// NTP Configuration
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;      // Adjust for your timezone
const int daylightOffset_sec = 0;  // Adjust for daylight saving

// RTC Configuration
RTC_DS3231 rtc;

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Time sources priority and status
enum TimeSource {
  GPS_TIME = 0,
  NTP_TIME = 1,
  RTC_TIME = 2,
  NO_TIME = 3
};

struct TimeData {
  unsigned long epochTime;
  TimeSource source;
  bool valid;
  unsigned long lastUpdate;
  int accuracy;  // Accuracy in seconds (lower is better)
};

TimeData timeData[3];
TimeSource currentPrimarySource = NO_TIME;
unsigned long lastSyncAttempt = 0;
const unsigned long SYNC_INTERVAL = 60000;  // Sync every minute
const unsigned long GPS_TIMEOUT = 30000;    // GPS timeout 30 seconds
const unsigned long NTP_TIMEOUT = 10000;    // NTP timeout 10 seconds

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600);
  Wire.begin();

  // Initialize time data structures
  initializeTimeData();

  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
  } else {
    Serial.println("RTC initialized");
    if (rtc.lostPower()) {
      Serial.println("RTC lost power, setting initial time");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  }

  // Initialize WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");

  // Initial time synchronization
  performTimeSync();

  Serial.println("Time Correction System Initialized");
  printCurrentTime();
}

void loop() {
  // Continuous GPS monitoring
  readGPS();

  // Periodic synchronization
  if (millis() - lastSyncAttempt > SYNC_INTERVAL) {
    performTimeSync();
    lastSyncAttempt = millis();
  }

  // Print status every 10 seconds
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 10000) {
    printTimeStatus();
    lastPrint = millis();
  }

  delay(100);
}

void initializeTimeData() {
  for (int i = 0; i < 3; i++) {
    timeData[i].epochTime = 0;
    timeData[i].source = (TimeSource)i;
    timeData[i].valid = false;
    timeData[i].lastUpdate = 0;
    timeData[i].accuracy = 999;  // Default high inaccuracy
  }

  // Set accuracy priorities (lower = more accurate)
  timeData[GPS_TIME].accuracy = 1;   // GPS: ±1 second
  timeData[NTP_TIME].accuracy = 5;   // NTP: ±5 seconds
  timeData[RTC_TIME].accuracy = 60;  // RTC: ±60 seconds
}

void performTimeSync() {
  Serial.println("\n=== Starting Time Synchronization ===");

  // Try GPS first (highest priority)
  if (getGPSTime()) {
    Serial.println("GPS time acquired - Primary source");
    currentPrimarySource = GPS_TIME;
    correctOtherClocks(GPS_TIME);
  }
  // Fall back to NTP
  else if (getNTPTime()) {
    Serial.println("NTP time acquired - Secondary source");
    if (currentPrimarySource != GPS_TIME) {
      currentPrimarySource = NTP_TIME;
    }
    correctOtherClocks(NTP_TIME);
  }
  // Final fallback to RTC
  else if (getRTCTime()) {
    Serial.println("RTC time acquired - Tertiary source");
    if (currentPrimarySource == NO_TIME) {
      currentPrimarySource = RTC_TIME;
    }
  } else {
    Serial.println("No time source available!");
    currentPrimarySource = NO_TIME;
  }

  Serial.println("=== Synchronization Complete ===\n");
}

bool getGPSTime() {
  Serial.print("Attempting GPS time sync... ");
  unsigned long startTime = millis();

  while (millis() - startTime < GPS_TIMEOUT) {
    if (gpsSerial.available()) {
      String sentence = gpsSerial.readStringUntil('\n');
      if (parseGPSTime(sentence)) {
        Serial.println("Success!");
        return true;
      }
    }
    delay(10);
  }

  Serial.println("Timeout/Failed");
  return false;
}

bool parseGPSTime(String sentence) {
  // Parse GPRMC sentence for time and date
  if (sentence.startsWith("$GPRMC")) {
    int commas[12];
    int commaCount = 0;

    // Find comma positions
    for (int i = 0; i < sentence.length() && commaCount < 12; i++) {
      if (sentence.charAt(i) == ',') {
        commas[commaCount++] = i;
      }
    }

    if (commaCount >= 9) {
      String status = sentence.substring(commas[1] + 1, commas[2]);
      if (status == "A") {  // Valid fix
        String timeStr = sentence.substring(commas[0] + 1, commas[1]);
        String dateStr = sentence.substring(commas[8] + 1, commas[9]);

        if (timeStr.length() >= 6 && dateStr.length() >= 6) {
          // Extract time components
          int hour = timeStr.substring(0, 2).toInt();
          int minute = timeStr.substring(2, 4).toInt();
          int second = timeStr.substring(4, 6).toInt();

          // Extract date components
          int day = dateStr.substring(0, 2).toInt();
          int month = dateStr.substring(2, 4).toInt();
          int year = 2000 + dateStr.substring(4, 6).toInt();

          // Convert to epoch time
          struct tm timeinfo;
          timeinfo.tm_year = year - 1900;
          timeinfo.tm_mon = month - 1;
          timeinfo.tm_mday = day;
          timeinfo.tm_hour = hour;
          timeinfo.tm_min = minute;
          timeinfo.tm_sec = second;

          timeData[GPS_TIME].epochTime = mktime(&timeinfo);
          timeData[GPS_TIME].valid = true;
          timeData[GPS_TIME].lastUpdate = millis();
          gpsFixed = true;

          return true;
        }
      }
    }
  }
  return false;
}

bool getNTPTime() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("NTP failed - WiFi not connected");
    return false;
  }

  Serial.print("Attempting NTP time sync... ");

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  struct tm timeinfo;
  unsigned long startTime = millis();

  while (millis() - startTime < NTP_TIMEOUT) {
    if (getLocalTime(&timeinfo)) {
      timeData[NTP_TIME].epochTime = mktime(&timeinfo);
      timeData[NTP_TIME].valid = true;
      timeData[NTP_TIME].lastUpdate = millis();
      Serial.println("Success!");
      return true;
    }
    delay(100);
  }

  Serial.println("Timeout/Failed");
  return false;
}

bool getRTCTime() {
  Serial.print("Attempting RTC time sync... ");

  if (!rtc.begin()) {
    Serial.println("RTC not available");
    return false;
  }

  DateTime now = rtc.now();
  timeData[RTC_TIME].epochTime = now.unixtime();
  timeData[RTC_TIME].valid = true;
  timeData[RTC_TIME].lastUpdate = millis();

  Serial.println("Success!");
  return true;
}

void correctOtherClocks(TimeSource masterSource) {
  if (!timeData[masterSource].valid) return;

  unsigned long masterTime = timeData[masterSource].epochTime;
  Serial.println("Correcting other clocks based on " + getSourceName(masterSource));

  // Correct RTC if GPS or NTP is master
  if (masterSource != RTC_TIME && rtc.begin()) {
    DateTime masterDateTime(masterTime);
    DateTime rtcTime = rtc.now();

    long timeDiff = abs((long)(masterTime - rtcTime.unixtime()));

    if (timeDiff > 2) {  // Only correct if difference > 2 seconds
      rtc.adjust(masterDateTime);
      Serial.println("RTC corrected by " + String(timeDiff) + " seconds");

      // Update RTC time data
      timeData[RTC_TIME].epochTime = masterTime;
      timeData[RTC_TIME].valid = true;
      timeData[RTC_TIME].lastUpdate = millis();
    }
  }
}

void readGPS() {
  while (gpsSerial.available()) {
    char c = gpsSerial.read();
    gpsData += c;

    if (c == '\n') {
      parseGPSTime(gpsData);
      gpsData = "";
    }
  }
}

String getSourceName(TimeSource source) {
  switch (source) {
    case GPS_TIME: return "GPS";
    case NTP_TIME: return "NTP";
    case RTC_TIME: return "RTC";
    default: return "NONE";
  }
}

String formatTime(unsigned long epochTime) {
  time_t rawTime = epochTime;
  struct tm* timeInfo = localtime(&rawTime);

  char buffer[20];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeInfo);
  return String(buffer);
}

void printCurrentTime() {
  Serial.println("\n=== Current Time Status ===");
  Serial.println("Primary Source: " + getSourceName(currentPrimarySource));

  if (currentPrimarySource != NO_TIME && timeData[currentPrimarySource].valid) {
    Serial.println("Current Time: " + formatTime(timeData[currentPrimarySource].epochTime));
  } else {
    Serial.println("No valid time available");
  }
}

void printTimeStatus() {
  Serial.println("\n--- Time Sources Status ---");

  for (int i = 0; i < 3; i++) {
    TimeSource src = (TimeSource)i;
    Serial.print(getSourceName(src) + ": ");

    if (timeData[i].valid) {
      Serial.print("✓ " + formatTime(timeData[i].epochTime));
      Serial.print(" (Age: " + String((millis() - timeData[i].lastUpdate) / 1000) + "s)");
      Serial.println(" [Accuracy: ±" + String(timeData[i].accuracy) + "s]");
    } else {
      Serial.println("✗ Invalid/Unavailable");
    }
  }

  Serial.println("Active Source: " + getSourceName(currentPrimarySource));

  // Show time differences between sources
  if (timeData[GPS_TIME].valid && timeData[NTP_TIME].valid) {
    long diff = abs((long)(timeData[GPS_TIME].epochTime - timeData[NTP_TIME].epochTime));
    Serial.println("GPS-NTP difference: " + String(diff) + " seconds");
  }

  if (timeData[currentPrimarySource].valid && timeData[RTC_TIME].valid && currentPrimarySource != RTC_TIME) {
    long diff = abs((long)(timeData[currentPrimarySource].epochTime - timeData[RTC_TIME].epochTime));
    Serial.println("Primary-RTC difference: " + String(diff) + " seconds");
  }

  Serial.println("------------------------\n");
}
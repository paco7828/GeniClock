#pragma once

// Shift register control
const byte SER = 10;
const byte SRCLK = 8;
const byte RCLK = 9;

// Control buttons
const byte CONFIRM_BTN = 0;
const byte CANCEL_BTN = 1;
const byte ADD_BTN = 2;
const byte SUBTRACT_BTN = 3;

// Buzzer
const byte BUZZER = 4;
const int NOTIF_MELODY[] = { 523, 659, 783, 1047 };  // C5, E5, G5, C6
const int NOTIF_MELODY_LENGTH = 4;
const int STARTUP_MELODY[] = { 392, 440, 523, 587, 659, 784, 880 };  // G4, A4, C5, D5, E5, G5, A5
const int STARTUP_MELODY_LENGTH = 7;

// GPS
const byte GPS_RX = 20;

// RTC
const byte RTC_SDA = 6;
const byte RTC_SCL = 7;

// Button debounce delay
const unsigned long DEBOUNCE_DELAY = 50;

// Short day names based on day index
const char *DAYS_OF_WEEK[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

// Intervals
const unsigned long NTP_RETRY_INTERVAL = 5000;         // 5 seconds
const unsigned long RTC_READ_INTERVAL = 1000;          // 1 second
const unsigned long TEMPERATURE_READ_INTERVAL = 2000;  // 2 seconds
const unsigned long WIFI_CONNECTION_TIMEOUT = 8000;    // 8 seconds

// Mode titles
char *MODE_TITLES[] = {
  "HH:MM:SS",  // 0 - 14:00:00
  "YYYY. MM",  // 1 - 2025. 07
  " DD --- ",  // 2 - 10 Mon
  "- TEMP -",  // 3 - 25.6 C
  " TIMER  ",  // 4 - Timer mode
  " ALARM  ",  // 5 - Alarm mode
  "GPS LAT ",  // 6 - 20.12345
  "GPS LON ",  // 7 - 10.12345
  "SPEED KM"   // 8 - 0 km/h
};

// Modes
const byte MIN_MODE = 0;
const byte MAX_MODE = 8;

// Durations
const int TITLE_SHOW_TIME = 2000;                // 2 seconds
const unsigned long STATUS_MSG_DURATION = 3000;  // 3 seconds
const unsigned long NOTIF_STEP_DURATION = 200;   // milliseconds
const int STARTUP_NOTE_DURATIONS[] = {
  120,  // G4 - short
  120,  // A4 - short
  180,  // C5 - medium
  120,  // D5 - short
  120,  // E5 - short
  200,  // G5 - long
  300   // A5 - longest (finale)
};

// WiFi Setup mode constants (Access Point)
const char *AP_SSID = " Geni ";
const char *AP_PASSWORD = "GNCLK123";

// GPS timezone and time conversion constants
const unsigned long GPS_CACHE_VALIDITY_MS = 500;  // Cache valid for 500ms

// Timer state and timing constants
const unsigned long TIMER_COUNTDOWN_INTERVAL = 1000;         // 1 second
const unsigned long TIMER_SETTING_TITLE_DURATION = 2000;     // 2 seconds
const unsigned long TIMER_CANCEL_DOUBLE_PRESS_WINDOW = 500;  // 500ms for double press detection
const unsigned long TIMER_AUTO_RETURN_DELAY = 3000;          // 3 seconds after alarm stops

// Timer alarm constants
const int TIMER_ALARM_MELODY[] = { 1200, 1400, 1600, 1400, 1600, 1800 };
const int TIMER_ALARM_MELODY_LENGTH = 6;
const unsigned long TIMER_ALARM_STEP_DURATION = 100;       // 100ms - faster alarm
const byte TIMER_ALARM_CYCLES = 3;                         // Number of full melody cycles
const unsigned long TIMER_ALARM_CYCLE_GAP_DURATION = 300;  // 300ms gap between cycles

// Alarm constants
const unsigned long ALARM_SETTING_TITLE_DURATION = 2000;     // 2 seconds
const unsigned long ALARM_CANCEL_DOUBLE_PRESS_WINDOW = 500;  // 500ms for double press detection

// Alarm sound constants
const int ALARM_MELODY[] = { 800, 1000, 1200, 1000, 1200, 1400, 1200, 1000 };
const int ALARM_MELODY_LENGTH = 8;
const unsigned long ALARM_STEP_DURATION = 200;       // 200ms per note
const byte ALARM_CYCLES = 5;                         // Number of full melody cycles
const unsigned long ALARM_CYCLE_GAP_DURATION = 500;  // 500ms gap between cycles
const unsigned long ALARM_SNOOZE_DURATION = 300000;  // 5 minutes in milliseconds

// Button beep frequencies for unique press effects
const int BUTTON_BEEP_FREQS[] = { 800, 1000, 1200, 600 };  // CONFIRM, CANCEL, ADD, SUBTRACT
const int BUTTON_BEEP_DURATION = 50;

// Triple press detection for WiFi setup
const unsigned long TRIPLE_PRESS_WINDOW = 3000;  // 3 seconds

// Captive portal configuration
const byte WIFI_CHANNEL = 6;
const int DNS_INTERVAL = 30;
const byte WIFI_LOCAL_IP[4] = { 4, 3, 2, 1 };
const byte WIFI_GATEWAY_IP[4] = { 4, 3, 2, 1 };
const byte WIFI_SUBNET_MASK[4] = { 255, 255, 255, 0 };
const String WIFI_LOCAL_IP_URL = "http://4.3.2.1";

// AP info display switching
const unsigned long AP_INFO_SWITCH_INTERVAL = 2000;  // 2 seconds
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
const char* DAYS_OF_WEEK[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

// Intervals
const unsigned long NTP_RETRY_INTERVAL = 5000;         // 5 seconds
const unsigned long RTC_READ_INTERVAL = 1000;          // 1 second
const unsigned long TEMPERATURE_READ_INTERVAL = 2000;  // 2 seconds
const unsigned long WIFI_CONNECTION_TIMEOUT = 8000;

// Modes
const byte MIN_MODE = 0;
const byte MAX_MODE = 7;

// Mode titles
char* MODE_TITLES[] = {
  "HH:MM:SS", // 14:00:00
  "YYYY. MM", // 2025. 07
  " DD --- ", // 10 Mon
  "- TEMP -", // 25.6 C
  " TIMER  ",
  "GPS LAT ", //
  "GPS LON ", // 
  "SPEED KM"
};

// Durations
const int TITLE_SHOW_TIME = 2000;                    // 2 seconds
const unsigned long STATUS_MSG_DURATION = 3000;    // 3 seconds
const unsigned long NOTIF_STEP_DURATION = 200;  // milliseconds
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
const char* AP_SSID = "--Geni--";
const char* AP_PASSWORD = "clock123";
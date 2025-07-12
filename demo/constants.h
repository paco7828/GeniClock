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

// GPS
const byte GPS_RX = 20;

// RTC
const byte RTC_SDA = 6;
const byte RTC_SCL = 7;

// Wifi credentials
char* SSID = "Xiaomi_88A6";
char* PASSW = "Xiaomi_88A6";

// Button debounce delay
const unsigned long debounceDelay = 50;

// Short day names based on day index
const char* daysOfWeek[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

// Intervals
const unsigned long ntpRetryInterval = 5000;  // 5 seconds
const unsigned long rtcReadInterval = 1000;   // 1 second

// Modes
const byte MIN_MODE = 0;
const byte MAX_MODE = 3;

// Mode titles
char* MODE_TITLES[] = { "HH:MM:SS", "YYYY. MM", " DD --- ", " TIMER  " };

// For how much time the selected mode's title is shown
const int TITLE_SHOW_TIME = 2000;  // 2 seconds
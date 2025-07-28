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
const int notificationMelody[] = {523, 659, 783, 1047}; // C5, E5, G5, C6
const int notificationMelodyLength = 4;
const int startupMelody[] = {392, 440, 523, 587, 659, 784, 880}; // G4, A4, C5, D5, E5, G5, A5
const int startupMelodyLength = 7;

// GPS
const byte GPS_RX = 20;

// RTC
const byte RTC_SDA = 6;
const byte RTC_SCL = 7;

// Wifi credentials
char *SSID = "Xiaomi_88A6";
char *PASSW = "Xiaomi_88A6";

// Button debounce delay
const unsigned long debounceDelay = 50;

// Short day names based on day index
const char *daysOfWeek[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

// Intervals
const unsigned long ntpRetryInterval = 5000;        // 5 seconds
const unsigned long rtcReadInterval = 1000;         // 1 second
const unsigned long temperatureReadInterval = 2000; // 2 seconds

// Modes
const byte MIN_MODE = 0;
const byte MAX_MODE = 4;

// Mode titles
char *MODE_TITLES[] = {"HH:MM:SS", "YYYY. MM", " DD --- ", "- TEMP -", " TIMER  "};

// Durations
const int TITLE_SHOW_TIME = 2000;                   // 2 seconds
const unsigned long statusMessageDuration = 3000;   // 3 seconds
const unsigned long notificationStepDuration = 200; //  tone step
const unsigned long startupSoundStepDuration = 150; // startup tone step
const int startupNoteDurations[] = {
    120, // G4 - short
    120, // A4 - short
    180, // C5 - medium
    120, // D5 - short
    120, // E5 - short
    200, // G5 - long
    300  // A5 - longest (finale)
};
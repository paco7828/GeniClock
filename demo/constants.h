// Shift register control
const byte SER = 10;
const byte SRCLK = 8;
const byte RCLK = 9;

// Control buttons
const byte CONFIRM_BTN = 0;
const byte ADD_BTN = 1;
const byte SUBTRACT_BTN = 2;
const byte CANCEL_BTN = 3;

// Buzzer
const byte BUZZER = 4;

// GPS
const byte GPS_RX = 20;

// RTC
const byte RTC_SDA = 6;
const byte RTC_SCL = 7;

// Wifi credentials
char* SSID = "UPCD679A2A";
char* PASSW = "Koszegi1963";

// Button debounce delay
const unsigned long debounceDelay = 50;

// Short day names based on day index
const char* daysOfWeek[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

// Intervals
const unsigned long ntpRetryInterval = 5000;  // 5 seconds
const unsigned long rtcReadInterval = 1000;  // 1 second
const unsigned long displayUpdateInterval = 500;  // 500 milliseconds
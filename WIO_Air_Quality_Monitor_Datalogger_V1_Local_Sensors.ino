/**************************************************************************************************************************
Arduino sketch for the Seeed Studio WIO Terminal all-in-one device https://wiki.seeedstudio.com/Wio-Terminal-Getting-Started/

Full function program for the Air Quality Monitor & Datalogger V1 (local sensors) system.
Measures, graphs and records PM1, PM2.5, PM4, PM10, VOC, NOx, CO2, NO2, C2H5CH from multiple sensors.
To track indoor pollutants when 3D printing, cooking, woodworking, sleeping, etc.
See blog post link below for overview, features, implementation and results.

Version = 0.81    (8/2024)    [targeting all Must Features in 1.0]
Written by Raphtronic https://raphtronic.blogspot.com

Usage: . Download on WIO Terminal, load the appropriate libraries
       . Works with no sensor at all. Boot screen will show missing sensors. Just press the joystick to proceed to the application
       . Obviously, at least one Sensirion SEN55 sensor should be connected though
       . Optional: Seeed Multigas V2, External RTC, SCD40, larger 18650-cells battery pack
       .            The Values and Chart screens will simply display 0, or no chart, for absent sensors
       . Comment out '#define INTERNAL_RTC' below to use an external Adafruit RTC
       . See blog post series for overview, features, setup, results
       . Starting with: [DIY] Air Quality Monitor & Datalogger - Part 1 Introduction  https://raphtronic.blogspot.com/2024/08/diy-pm-voc-air-quality-monitor-with.html
       . In case of issues see post "Part 2 Implementation" for details, and Basic Sketches for troubleshooting

Libraries
  Mandatory:
    Adafruit BusIO by Adafruit 1.16.1 (dunno what it's for. Is I2C/SPI related)
    Adafruit Zero DMA Library by Adafruit 1.13. Used by Seeed for the TFT screen. Details here: https://wiki.seeedstudio.com/Wio-Terminal-LCD-Overview/
    Seeed Arduino FS by Hongtai.liu 2.1.1  https://github.com/Seeed-Studio/Seeed_Arduino_FS
    Sensirion Core by Sensirion 0.6.0  https://github.com/Sensirion/arduino-core
    Sensirion I2C SEN5X by Sensirion 0.3.0  https://github.com/Sensirion/arduino-i2c-sen5x
    Sparkfun BQ27441 LiPo Fuel Gauge Arduino Library by Sparkfun Electronics 1.1.0  https://github.com/sparkfun/SparkFun_BQ27441_Arduino_Library
    Not a library but MUST be included in the same folder as this sketch: lcd_backlight.hpp  https://github.com/Seeed-Studio/Seeed_Arduino_Sketchbook/tree/master/examples/WioTerminal_BackLight 
  Optional: (comment out the #include if don't need those)
    Grove - Multichannel Gas Sensor by Seeed Studio 1.0.0  https://wiki.seeedstudio.com/Grove-Multichannel-Gas-Sensor-V2/
    RTCLib by Adafruit  https://github.com/adafruit/RTClib
    Sensirion I2C SCD4x by Sensirion 0.4.0  https://github.com/Sensirion/arduino-i2c-scd4x

Compiling messages - getting the following:
    . parameter passing changed in GCC 7.1" warnings during compilation, pointing at Seeed libraries
    . Multiple libraries were found for "TFT_eSPI.h". Used: ...\Seeed_Arduino_LCD-master. Not used: ...\Seeed_Arduino_LCD
    . Multiple libraries were found for "Adafruit_ZeroDMA.h". Used: ...\Adafruit_ZeroDMA. Not used: ...\Adafruit_Zero_DMA_Library
    . Annoying but works so far...

Feel free to copy / leverage this work

Warning: at this point in my self-taught SW journey I am not yet familiar enough with copyright and open-source
         licenses / etiquette to properly flag my Arduino programs and credit contributors I leveraged bits from,
         without copying / pasting many headers from Sensirion, Adafruit, SEEED, Sparkfun, individuals.
         Welcoming feedback. Still learning.
**************************************************************************************************************************/

/* Version Planning
(0.9) LCD brightness setting bug: erase 1 more character space as when decreasing from 100 leaves a trail on the right
(0.9) add UNITS to value screens
(0.9) allow screenSampleInterval > logInterval when logging not ON
(0.9) hide sensors in Values and Chart screens when they're not present (add before Multigas & SCD40 display code: if (!sensorerror) { proceed })
(1.0) add Enable / Disable static choice for each sensor to allow for different HW configuration btwn users
(1.0) rationalize where if(sensorError) must be used. Remove from LOOP, add to each sensorMeasure sub ?
(1.0) rationalize use of fakesensordata: only if setupError on sensor. For errors after successful setup send back 0s, like in SCD40 code
(1.0) disable SD Card Eject while recording on, instead of NOW
(1.0) cleanup for readability in setup only call device1setup device2setup, etc, instead of doing it in line. Use True / False for call success and Error message or in the subs themselves ?
(1.0) clean up DEBUG & REMOVE lines
(1.0) conditional statement for all serial.print in code when !Serial is detected in setup at boot
(1.0) fix cursors not available when in banner mode --> actually make white settings become grey
(1.0) issue cursor when toggling btwn banner and settings / battery: see cursor lagging. Because an UPDATESCREEN was not triggered ???
(1.0) add RTC for correct time at power-on and sampling
(1.0) add Alert thresholds to settings screen
(1.0) add a setting for temp in Farenheit
(1.0) disable SCD40 Auto Self Calibration or it'll assume 400ppm once a week and start drifting
             details: https://www.reddit.com/r/AirQuality/comments/12y0nwm/warning_about_the_sensirion_scd4041_co2_sensors/
             datasheet chapter 3.7 https://www.mouser.com/datasheet/2/682/Sensirion_SCD4x_Datasheet-3392604.pdf
(1.1) make battery gauge more reliable by cross gating empty thresholds w voltage (ex: soc<10% && voltage<3.4v)
(1.1) bq27441's SOC estimate widely pessimistic, does not compensate even when shows depleted but still sees high voltage (~3.6v). Take several datalogging runs from FULL to EMPTY to track performance soc/V/I, then do TI's calibration per application note
(1.1) add optional Large Capacity battery
(1.1) no flickering screen on refresh. Confusing as happens on screenInterval (2s) which is NOT sampleInterval. No sample acquired
(1.1) add a setting to stop datalogging when voltage < 3.1v, 3.2v...
(1.1) add a setting to stop operation and go to sleep when voltage < 3V to avoid dying when accessing sensors ?
(1.1) add a setting that Initiates Sen55 Cleaning
(1.1) add a setting that controls the chart data's nb of samples displayed ?
(1.1) add setting for SCD40 altitude correction
(1.1) improved Multigas V2 sensor values code
(1.1) add to settings the Firmware Version & Node Name
(1.1) add setting to Force SCD40 Self Calibration.
(1.1) optimization: replace all drawstring(,,font) by a single proper setFreeFont + drawString(,) to reduce calls to setFont. Problem: (,,2) results in lean font, what's the setFreeFont equivalent ?
(1.1) optimization instead of padding time hh mm ss with call to padding functions, use inline code if hh<10 then txt +=0; txt += hh; like at end of drawBanner
(1.1) rationalize battery measurements. Exists in both batteryMeasure() and screenBatttery()
(1.2) add a setting for sensor names (most won't recompile to change them. Save to FLASH)
(1.2) implement an error display system. Like SD Open File fail during logging, etc. Maybe error icon on banner flashing and an associated display screen when clicked on
(1.2) add a 24h and a ALL Points and a View Current Record charts mode, selected w joystick up / down. 118KB sram free if 100 chart points --> 118.000 / 15 chart series / ~2Bytes = 3933 datasets. If points every 5' --> 327 hours  = 13.6 days
(1.2) all SD files show a 12/31/2017 11pm date. Logged a github issue against seeed_arduino_fs library and in their Wiki. No answer yet
(1.2) save session settings in FLASH
*/

#include <TFT_eSPI.h>                 // WIO Terminal's TFT screen
#include "lcd_backlight.hpp"          // LCD brightness control
// #include <Arduino.h>
#include <SensirionI2CSen5x.h>        // Sensirion SEN55 PM/VOC/NOX/T/R sensor
#include <SensirionI2CScd4x.h>
#include <Wire.h>
#include "seeed_line_chart.h"         // library to draw charts
#include <Multichannel_Gas_GMXXX.h>   // Seeed Studio Grove sensor
#include "SPI.h"
#include <Seeed_Arduino_FS.h>         // SD card file system
#include <SparkFunBQ27441.h>          // BQ27441 battery SOC chip in WIO battery pack

#define INTERNAL_RTC                  // Comment out if using the Adafruit PCF8523 external RTC
#ifdef INTERNAL_RTC
#include "RTC_SAMD51.h"               // WIO's internal RTC. Works but loses data on power off as the SAMD51 has no VBat power supply
RTC_SAMD51 rtc;                       // WIO Terminal's internal RTC. No button battery. Loses times if WIO is turned off (unless the battery backpack is connected to USB-C and ON)
#else
#include "RTClib.h"                   // for Adafruit PCF8523 RTC board
RTC_PCF8523 rtc;
#endif
//#include "DateTime.h"
DateTime rtcNow;
DateTime rtcSample;

const String WIO_NAME = "Master logger";
const String WIO_FIRMWARE = "Local Sensors 0.81"; // version
const bool FAKE_SENSOR_DATA = false;              // set to true during development if need to get and display sensor values without the sensors present
const bool DEBUG_MODE = false;                    // verbose mode on serial
bool serialPresentAtBoot;                         // record if serial was successfully initialized at boot. If not, will skip issuing serial commands

#define devSD  SD                                 // SD card interface

extern "C" char* sbrk(int incr);                  // to calculate free memory (heap)

#define NB_CHART_POINTS 100                       // maximum number of data points to display in charts
#define NB_SEN55 2                                // number of SEN55 sensors possibly present on this WIO. 1 on I2C0, 1 on I2C1. So up to 2 if not using an I2C expander

const unsigned int BATTERY_CAPACITY = 650;                      // Set Wio Terminal Battery's Capacity. 650 is from factory. Change if LiPo pouch replaced by 18650 cells
unsigned int batterySOC, batteryVoltage, batteryCapacityLeft;   // Measurement values from SEEED Battery Chassis module's BQ27441 BMS chip
int batteryCurrent = 0;

// Sensirion SEN55 sensor instances on this WIO
SensirionI2CSen5x sen55[NB_SEN55];          
String sen55Name0 = "Printer chamber";
String sen55Name1 = "Room";
String sen55Name2 = "Sensirion 55(2)";
int nbSen55ToDisplay = 0;                         // nb of sen55s to display. Based on the nb of sen55s found during setup
bool sen55SetupError[NB_SEN55];                   // record error during setup if a sen55 does not respond
float sen55Pm1[NB_SEN55], sen55Pm2p5[NB_SEN55], sen55Pm4[NB_SEN55], sen55Pm10[NB_SEN55];  // sen55 Measurements
float sen55RelHumi[NB_SEN55], sen55Temp[NB_SEN55], sen55Voc[NB_SEN55], sen55Nox[NB_SEN55];
doubles sen55ChartSeries[NB_SEN55][8];            // queue of double per line_chart library. Data points for the chart

// SEEED Grove multigas sensor
GAS_GMXXX<TwoWire> gas;                     
unsigned int mgVoc, mgCo, mgNo2, mgC2h5ch;        // measurement values
bool multigasSetupError = false;                  // record error during setup. If true, will not be displayed / logged
doubles multigasChartSeries[4];                   // queue of double per line_chart library. Data points for the chart
String multigasName = "Multigas V2";

// Adafruit Sensirion SCD40 board
SensirionI2CScd4x scd40;
bool scd40SetupError = false;
doubles scd40ChartSeries[3];                      // queue of double per line_chart library. Data points for the chart
uint16_t scd40CO2 = 0;
float scd40Temp = 0.0f;
float scd40RelHumi = 0.0f;
String scd40Name = "SCD40";

//TFT screen
TFT_eSPI tft;
static LCDBackLight tftBackLight;
int tftBrightness = 100;                          // 0 to 100%
#define SCREEN_WIDTH 320                          // WIO screen is 320 x 240
#define SCREEN_HEIGHT 240
#define TFT_DARKYELLOW 0xC600         //* 192, 192,   0 */ 1100 0110 0000 0000  RGB values are 5/6/5 bits: rrrr rggg gggb bbbb
#define TFT_GREY 0xA514               //* 150, 150, 150 */ 1010 0101 0001 0100   20 40 20
#define TFT_DARKERGREY 0x4208         //*               */ 0100 0010 0000 1000
#define TFT_VERYDARKGREY 0x38E7       //*               */ 0011 1000 1110 0111
#define TFT_LIGHTNAVY 0x0014
// #define LCD_BACKLIGHT (72Ul)       // Control Pin of LCD

unsigned long logSampleInterval = 11;             // sampling interval (seconds) between log records
unsigned long screenSampleInterval = 5;           // sampling interval (seconds) between samples for display (not logged). Must be <= logInterval
unsigned long bannerRefreshInterval = 2000;       // interval (milliseconds) between display refreshes (to show updated time, battery, countdown, etc)
unsigned long millisOldLog = 0;                   // last log time
unsigned long millisOldSample = 0;                // last sampling time
unsigned long millisOldScreen = 0;
unsigned long millisNow = 0;
unsigned long millisLogStart = 0;
unsigned long millisLogCountdownEnd = 0;
unsigned long nbLogRecords = 0;                   // nb of log data points recorded so far
unsigned long nbSamples = 0;                      // nb of samples so far
unsigned long millisSinceSensorsInit = 0;         // milliseconds since initialization of sensors. To ensure sensor Power Up Times are elapsed before 1st sample
bool sensorsReady = false;
unsigned long millisAtPowerOn = 0;

unsigned long millisJoystick = 0;
unsigned long millisButtons = 0;
unsigned long debounceTime = 100;                 // milliseconds
unsigned long millisOldJoystick = 0;
unsigned long millisOldButtons = 0;
unsigned long millisSD = 0;
unsigned long debounceSD = 1000;                  // milliseconds

String settingLogDuration = "manual stop";
int settingLogCountdownHH = 0;                    // time countdown left when logging samples in countdown mode
int settingLogCountdownMM = 2;
int settingLogCountdownSS = 11;
String settingShutdown = "N";
int settingPowerUpTime = 10;
int settingChangingValue;
String settingChangingText;
bool settingChanging = false;
int settingsXstart = 4;
int settingsYstart = 26;
int settingsYspacing = 16;
int settingsXcursor = 0;
int settingsYcursor = 0;
int sdXcursor = 0;
int sdYcursor = 0;
int sdcardChangingValue;
String sdcardChangingText;
bool sdcardChanging = false;
int sdcardXcursor = 0;
int sdcardYcursor = 0;
int bannerXcursor = 0;
const int BANNER_CURSOR_WIDTH = 20;
const int BANNER_HEIGHT = 20;
const int BANNER_X_START = 4;
const int BANNER_X1 = BANNER_X_START;
const int BANNER_X2 = 156;
const int BANNER_X3 = 176;
const int BANNER_X4 = 198;
const int BANNER_X5 = 272;
bool bannerActive = false;
const int VALUES_X_START = 96;
const int VALUES_Y_START = 42;
const int VALUES_Y_SPACING = 24;

String screenActive = "values1";
bool screenShotActive = false;

// Display content parameters
const auto FONT_SENSOR_TYPE_PRIMARY = &FreeSans12pt7b;
const auto FONT_SENSOR_TYPE_SECONDARY = &FreeSansOblique12pt7b;
const auto FONT_SENSOR_TYPE = &FreeSans12pt7b;
const auto FONT_SENSOR_TYPE_SMALLER = &FreeSans12pt7b;
// const auto FONTSENSORDATA = &FreeSansBold18pt7b;
const auto FONT_SENSOR_DATA_PRIMARY = &FreeSansBold12pt7b;
const auto FONT_SENSOR_DATA_SECONDARY = &FreeSansOblique12pt7b;
const auto FONT_SETTINGS = &FreeSans9pt7b;
int settingsFontWidth, settingsFontHeight;

#define TFT_DARKISHCYAN 0x0618     // 0000 0110 0001 1000 / *  0, 192 (1100 0000), 192 (1100 0000)
const color_t COLOR_BANNER_ICON_ENABLED = TFT_WHITE;
const color_t COLOR_BANNER_ICON_DISABLED = TFT_DARKERGREY;
const color_t COLOR_BANNER_CURSOR = TFT_BLUE;
const color_t COLOR_SD_ABSENT = COLOR_BANNER_ICON_DISABLED;
const color_t COLOR_CHART_BACKGND = TFT_WHITE;    // using color other than white shows issues: ghost traces, white labels, etc
const color_t COLOR_SENSOR_DATA = TFT_WHITE;
const color_t COLOR_SENSOR_TYPE = TFT_WHITE;
const color_t COLOR_SENSOR_TYPE1_BACKGND = TFT_BLUE;
const color_t COLOR_SENSOR_TYPE2_BACKGND = TFT_DARKCYAN;
const color_t COLOR_SENSOR_TYPE3_BACKGND = TFT_DARKISHCYAN;
const color_t COLOR_SETTING_TEXT = TFT_DARKGREY;
const color_t COLOR_SETTING_USER = TFT_WHITE;
const color_t COLOR_SETTING_CHANGING = TFT_ORANGE;
const color_t COLOR_SD_ABSENT_TEXT = TFT_DARKERGREY;
const color_t COLOR_BATTERY_ABSENT_TEXT = TFT_DARKERGREY;
const color_t COLOR_CURSOR = TFT_BLUE;

String joystickState = "";
String oldJoystickState = "";
String buttonState = "";
String oldButtonState = "";

bool sdSwitch = false;
bool sdPresent = false;
bool sdEjected = true;
String logFileName;                               // data logging file
File logFile;
bool loggingOn = false;

uint8_t  pixelsIN[2 * SCREEN_WIDTH];              // receives pixel 16b (565) color values from a screen line during a screen snapshot
uint8_t  pixels888[3 * SCREEN_WIDTH];             // converted pixel colors to 24b (888) values for BMP file
File bmpFile;
String bmpFileName = "screenshot";

# pragma pack (push)
# pragma pack (2)
// save previous packing, then set packing for 16-bits
typedef struct  BMPheaderType{
  uint16_t  bfType = 0x4d42;  //'bm';
  uint32_t  bfSize = SCREEN_WIDTH * SCREEN_HEIGHT * 3 + 54;  // 230454: 76800 pixels x 3 colors + 54 header
  uint16_t  bfReserved1 = 0;
  uint16_t  bfReserved2 = 0;
  uint32_t  bfOffBits =  54;  // 14 bytes to here
  uint32_t  biSize = 40;
  int32_t   biWidth = SCREEN_WIDTH;
  int32_t   biHeight = -SCREEN_HEIGHT;  // windows wants negative for top-down image
  int16_t   biPlanes = 1;
  uint16_t  biBitCount = 24 ;
  uint32_t  biCompression = 0;  // bitfields used needs color masks
  uint32_t  biSizeImage = SCREEN_WIDTH * SCREEN_HEIGHT * 3; // 320 * 240 * 3
  int32_t   biXPelsPerMeter = 0;
  int32_t   biYPelsPerMeter = 0;
  uint32_t  biClrUsed  = 0;
  uint32_t  biClrImportant = 0;  // 54 bytes to end
} BMPHDR_VGA;
// restore previous packing
# pragma pack (pop)

BMPheaderType bmpHeader;  // Screenshot: header structure describing the BMP image's attributes


String settingsText[] = {  // Information & setting lines displayed in the Settings screen
  // in each string all chars left of ':' are displayed and used as a selection string by the display and setting-change routines
  //     (screenMainSettings, screenSdCardSettings...) to link to the corresponding variable/setting name
  // '<' signals a user modifiable setting. Mandatory.
  //     But for displaying on screen, all chars after ':' are ignored
  "Date:",                                        // also displays Time Since Turn-On
  "Current nb of samples:",                       // also displays Current Nb Of Logged Samples
  "SRAM memory Total/Free:",
  "Screen sample interval (s):<99>",              // Screen Sample Interval MUST be the 1st user modifiable setting in the list
  "SD log sample interval (s):<999>",
  "SD log duration:<countdown|manual stop>",
  "SD log countdown (HHMMSS):<HH:MM:SS>",
  "Sensor shutdown after sample:<Y|N>",
  "Sensor power up wait time (s):<99>",
  // "Screen standby time (s):<9999>",
  "LCD brightness:<100>",
  // "/n:",
  // "/    >>> Scroll Right for next screen <<<:"
  "Hold Time IOxx once PMs < Alert (s):<9999>"
  // Hold Time IOxx once VOCs < Alert (s):<9999>"
  // Hold Time IOxx once Temp < MAX Alert (s):<9999>"
  // Hold Time IOxx once Temp > MIN Alert (s):<9999>"
  // Hold Time IOxx once Humidity < MAX Alert (s):<9999>"
  // Hold Time IOxx once Humidity > MIN Alert (s):<9999>"
  // "Initiate cleaning cycle Sen55[0]:<Go>"
  // "Radio:<on/off>"
  // "Radio sensors to use:", // Sen55A, Sen55B, Mult1
  // "Radio sensors found:"   // Sen55B, Mult2 <<====== color coded: green = all, orange some, red = none
  // Alert Threshold PM xxx yyy VOC xxx yyy
  // Alert Threshold NO2 xxx yyy CO2 xxx yyy 
};
const size_t NB_SETTING_LINES = sizeof(settingsText) / sizeof(settingsText[0]);

String sdText[] = {  // Information & setting lines displayed in the SD Card settings screen
  "SD card status:",
  "SD card eject:>NOW<",
  "SD card Used/Total:",
  "SD card size:",
  "Files list: -> move joystick to the RIGHT <-"
};
const size_t NB_SD_LINES = sizeof(sdText) / sizeof(sdText[0]);

const byte ICON_WIFI3[16][2] = {
  {7, 240},
  {31, 252},
  {56, 14},
  {96, 3},
  {67, 225},
  {15,248},
  {28, 28},
  {16, 4},
  {1, 192},
  {7, 240},
  {4, 16},
  {0, 0},
  {0, 0},
  {1, 192},
  {1, 192},
  {1, 192}
}; 

const byte ICON_SETTINGS4[16][2] = {
  {0, 0},
  {0, 0},
  {0, 32},
  {0, 112},
  {127, 118},
  {0, 112},
  {4, 32},
  {14, 0},
  {110, 255},
  {14, 0},
  {4, 16},
  {0, 56},
  {127, 186},
  {0, 56},
  {0, 16},
  {0, 0}
};

const byte ICON_RECORD_LARGE[16][8] = {
  {15,255,255,255,255,255,255,240},
  {48,0,0,0,0,0,0,12},
  {64,0,0,0,0,0,0,2},
  {67,224,0,0,0,0,0,2},
  {135,240,0,0,0,0,0,1},
  {143,248,0,0,0,0,0,1},
  {159,252,0,0,0,0,0,1},
  {159,252,0,0,0,0,0,1},
  {159,252,0,0,0,0,0,1},
  {159,252,0,0,0,0,0,1},
  {159,252,0,0,0,0,0,1},
  {143,248,0,0,0,0,0,1},
  {71,240,0,0,0,0,0,2},
  {67,224,0,0,0,0,0,2},
  {48,0,0,0,0,0,0,12},
  {15,255,255,255,255,255,255,240}
};

const byte ICON_BATTERY[16][2] = {
  {3,192},
  {3,192},
  {31,248},
  {16,8},
  {16,8},
  {16,8},
  {16,8},
  {16,8},
  {16,8},
  {16,8},
  {16,8},
  {16,8},
  {16,8},
  {16,8},
  {16,8},
  {31,248}
};

const byte ICON_BATTERY_PLUG[12][1] = {
  {0},
  {0},
  {72},
  {72},
  {252},
  {252},
  {252},
  {252},
  {120},
  {48},
  {48},
  {24}
};

const byte ICON_STOPWATCH[16][2] = {
  {3,224},
  {1,192},
  {0,128},
  {3,224},
  {12,24},
  {16,4},
  {16,116},
  {32,50},
  {32,82},
  {32,130},
  {33,2},
  {32,2},
  {16,4},
  {16,4},
  {12,24},
  {3,224}
};

const byte ICON_RADIO[16][4] = {
  {4,0,0,32},
  {8,0,0,16},
  {18,0,0,72},
  {36,0,0,36},
  {41,54,192,148},
  {42,127,254,84},
  {41,64,2,148},
  {36,64,2,36},
  {18,64,2,72},
  {8,64,2,16},
  {4,64,2,32},
  {0,64,2,0},
  {0,64,2,0},
  {0,64,14,0},
  {0,64,14,0},
  {0,127,254,0}
};

const byte ICON_SCREENSHOT[16][2] = {
  {0,0},
  {0,0},
  {3,224},
  {52,16},
  {120,15},
  {65,193},
  {66,33},
  {68,17},
  {68,17},
  {68,17},
  {66,33},
  {65,193},
  {64,1},
  {127,255},
  {0,0},
  {0,0}
};  

const byte ICON_SD_CARD[16][2] = {
  {31,254},
  {32,1},
  {36,169},
  {74,169},
  {138,169},
  {224,1},
  {32,1},
  {32,1},
  {76,113},
  {136,73},
  {134,73},
  {129,73},
  {142,113},
  {128,1},
  {128,1},
  {255,255}
};

void sen55Measure(int index) {  // Sensirion SEN55. Measures PM1,2.5,4,10 / VOC / NOx / T / RH
  if (DEBUG_MODE) {
    Serial.print("SUB sen55Measure(");
    Serial.print(index);
    Serial.println(")");
  }
  uint16_t error;
  char errorMessage[256];
  error = sen55[index].readMeasuredValues(sen55Pm1[index], sen55Pm2p5[index], sen55Pm4[index],
          sen55Pm10[index], sen55RelHumi[index], sen55Temp[index], sen55Voc[index], sen55Nox[index]);
  if (error) {
    Serial.print("Error readMeasuredValues sen55[");
    Serial.print(index);
    Serial.print("]: ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
    sen55Pm1[index] = 0;
    sen55Pm2p5[index] = 0;
    sen55Pm4[index] = 0;
    sen55Pm10[index] = 0;
    sen55RelHumi[index] = 0;
    sen55Temp[index] = 0;
    sen55Voc[index] = 0;
    sen55Nox[index] = 0;
  }
  if (FAKE_SENSOR_DATA) {
    sen55Pm1[index] = analogRead(A0);
    sen55Pm2p5[index] = analogRead(A0);
    sen55Pm4[index] = analogRead(A0);
    sen55Pm10[index] = analogRead(A0);
    sen55RelHumi[index] = analogRead(A0);
    sen55Temp[index] = analogRead(A0);
    sen55Voc[index] = analogRead(A0);
    sen55Nox[index] = analogRead(A0);
  }
  if (DEBUG_MODE && !error) {
    Serial.print("sen55Pm1p0[");
    Serial.print(index);
    Serial.print("]:");
    Serial.print(sen55Pm1[index]);
    Serial.print("\t");
    Serial.print("sen55Pm2p5:");
    Serial.print(sen55Pm2p5[index]);
    Serial.print("\t");
    Serial.print("sen55Pm4p0:");
    Serial.print(sen55Pm4[index]);
    Serial.print("\t");
    Serial.print("sen55Pm10p0:");
    Serial.print(sen55Pm10[index]);
    Serial.print("\t");
    Serial.print("AmbientHumidity:");
    if (isnan(sen55RelHumi[index])) {
      Serial.print("n/a");
    } else {
      Serial.print(sen55RelHumi[index]);
    }
    Serial.print("\t");
    Serial.print("AmbientTemperature:");
    if (isnan(sen55Temp[index])) {
      Serial.print("n/a");
    } else {
      Serial.print(sen55Temp[index]);
    }
    Serial.print("\t");
    Serial.print("VocIndex:");
    if (isnan(sen55Voc[index])) {
      Serial.print("n/a");
    } else {
      Serial.print(sen55Voc[index]);
    }
    Serial.print("\t");
    Serial.print("NoxIndex:");
    if (isnan(sen55Nox[index])) {
      Serial.println("n/a");
    } else {
      Serial.println(sen55Nox[index]);
    }
  }
}

void sen55ChartAppendData(int index) {
  sen55ChartSeries[index][0].push(sen55Pm1[index]);
  sen55ChartSeries[index][1].push(sen55Pm2p5[index]);
  sen55ChartSeries[index][2].push(sen55Pm4[index]);
  sen55ChartSeries[index][3].push(sen55Pm10[index]);
  sen55ChartSeries[index][4].push(sen55Voc[index]);
  sen55ChartSeries[index][5].push(sen55Nox[index]);
  sen55ChartSeries[index][6].push(sen55Temp[index]);
  sen55ChartSeries[index][7].push(sen55RelHumi[index]);
  if (sen55ChartSeries[index][0].size() > NB_CHART_POINTS) {  // cull the queue down to max nb of points to display
    for ( int i = 0; i < 8; i++) {
      sen55ChartSeries[index][i].pop();
    }
  }
}

void sen55Setup(int index) {
  if (DEBUG_MODE) {
    Serial.print("SUB sen55Setup(");
    Serial.print(index);
    Serial.println(")");
  }
  if (index == 0) {
    // Wire.begin();           // WIO's SCL1-SDA1 I2C bus. 3.3V signals is fine, but requires 5V power
    sen55[0].begin(Wire);   // I2C address of SEN55 is 0x69
  } else {
    // Wire1.begin();          // WIO's SCL0-SDA0 I2C bus. 3.3V signals is fine, but requires 5V power
    sen55[1].begin(Wire1);  // I2C address of SEN55 is 0x69
  }
  uint16_t error;
  char errorMessage[256];
  error = sen55[index].deviceReset();
  sen55SetupError[index] = false;
  if (error) {
    Serial.print("Error trying to execute deviceReset() for sen55[");
    Serial.print(index);
    Serial.print("]: ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
    sen55SetupError[index] = true;
  }

  // Set a temperature offset in degrees celsius (SEN54 & SEN55)
  // By default, the temperature and humidity outputs from the sensor
  // are compensated for the modules self-heating. If the module is
  // designed into a device, the temperature compensation might need
  // to be adapted to incorporate the change in thermal coupling and
  // self-heating of other device components.
  // See the app note at www.sensirion.com: “SEN5x – Temperature
  // Compensation Instruction”.
  float tempOffset = 0.0;
  error = sen55[index].setTemperatureOffsetSimple(tempOffset);
  if (error) {
    Serial.print("Error trying to execute setTemperatureOffsetSimple() for sen55[");
    Serial.print(index);
    Serial.print("]: ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
    sen55SetupError[index] = true;
  } else {
    Serial.print("Temperature Offset set to ");
    Serial.print(tempOffset);
    Serial.println(" deg. Celsius (SEN54/SEN55 only)");
  }
  // Start Measurement
  error = sen55[index].startMeasurement();
  if (error) {
    Serial.print("Error trying to execute startMeasurement() for sen55[");
    Serial.print(index);
    Serial.print("]: ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
    sen55SetupError[index] = true;
  }
  sen55Pm1[index] = 0;
  sen55Pm2p5[index] = 0;
  sen55Pm4[index] = 0;
  sen55Pm10[index] = 0;
  sen55RelHumi[index] = 0;
  sen55Temp[index] = 0;
  sen55Voc[index] = 0;
  sen55Nox[index] = 0;
}

void multigasMeasure() {
  if (DEBUG_MODE) {
    Serial.println("SUB multigasMeasure");
  }
  mgVoc = gas.getGM502B();      // GM502B sensor
  if (mgVoc > 999) mgVoc = 999;
  mgCo = gas.getGM702B();       // GM702B sensor
  if (mgCo > 999) mgCo = 999;
  mgNo2 = gas.getGM102B();      // GM102B sensor
  if (mgNo2 > 999) mgNo2 = 999;
  mgC2h5ch = gas.getGM302B();   // GM302B sensor
  if (mgC2h5ch > 999) mgC2h5ch = 999;
  if (FAKE_SENSOR_DATA) {
    mgVoc = analogRead(A0);
    mgCo = analogRead(A0);
    mgNo2 = analogRead(A0);
    mgC2h5ch = analogRead(A0);
  }
  if (DEBUG_MODE) {
    Serial.print("Seeed Multigas VOC:"); Serial.print(mgVoc); Serial.print(" CO:"); Serial.print(mgCo); Serial.print(" NO2:"); Serial.print(mgNo2); Serial.print(" C2H5CH:"); Serial.println(mgC2h5ch); 
  }
}

void multigasChartAppendData() {  // Seeed Studio Grove Multigas V2. Unreliably measures VOC / CO / NO2 / C2H5CH
  multigasChartSeries[0].push(mgVoc);  // add to queue of chart data points
  multigasChartSeries[1].push(mgCo);
  multigasChartSeries[2].push(mgNo2);
  multigasChartSeries[3].push(mgC2h5ch);
  if (multigasChartSeries[0].size() > NB_CHART_POINTS) { // cull the queue down to max nb of points to display
    for ( int i = 0; i < 4; i++) {
      multigasChartSeries[i].pop();
    }
  }
}

void batteryMeasure() {  // Measure battery data from BQ27441vBMS chip in the WIO Battery Chassis accessory
  batterySOC = lipo.soc();
  if (batterySOC <= 100) {
    batteryVoltage = lipo.voltage();
    batteryCurrent = lipo.current(AVG);
    batteryCapacityLeft = lipo.capacity(REMAIN);
  } else {                                              // Battery not present or not responding
    batteryVoltage = 0;
    batteryCurrent = 0;
    batteryCapacityLeft = 0;
  }
}

void scd40Setup() {
  if (DEBUG_MODE) {
    Serial.print("SUB scd40Setup(");
  }
  uint16_t error;
  char errorMessage[256];

  scd40.begin(Wire);

  // stop potentially previously started measurement
  error = scd40.stopPeriodicMeasurement();
  if (error) {
    Serial.print("Error from SCD40 stopPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
    scd40SetupError = true;
  } else {
    scd40SetupError = false;
  }
  error = scd40.startPeriodicMeasurement();
  if (error) {
    Serial.print("Error from SCD40 startPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
    scd40SetupError = true;
  } else {
    scd40SetupError = false;
  }
}

void scd40Measure() { // Sensirion SCD40. measure CO2/T/RH
  if (DEBUG_MODE) {
    Serial.print("SUB scd40Measure(");
  }

  // Read Measurement
  uint16_t error;
  char errorMessage[256];
  bool isDataReady = false;
  error = scd40.getDataReadyFlag(isDataReady);
  if (error) {
      Serial.print("Error from SCD40 getDataReadyFlag(): ");
      errorToString(error, errorMessage, 256);
      Serial.println(errorMessage);
      scd40CO2 = 0;
      scd40Temp = 0;
      scd40RelHumi = 0;
      return;
  }
  if (!isDataReady) {
      Serial.println("Warning: SCD40 data not ready");        // After a successful measurement request SCD40 has a 5 second refractory window before new values. Reusing previous values if not ready
      return;
  }
  error = scd40.readMeasurement(scd40CO2, scd40Temp, scd40RelHumi);
  if (error) {
      scd40CO2 = 0;
      scd40Temp = 0;
      scd40RelHumi = 0;
      Serial.print("Error from scd40 readMeasurement(): ");
      errorToString(error, errorMessage, 256);
      Serial.println(errorMessage);
  } else if (scd40CO2 == 0) {
      scd40CO2 = 0;
      scd40Temp = 0;
      scd40RelHumi = 0;
      Serial.println("Invalid SCD40 CO2 sample detected, skipping.");
  }
  if ((FAKE_SENSOR_DATA || error) && FAKE_SENSOR_DATA) {
    scd40CO2 = analogRead(A0);
    scd40Temp = analogRead(A0);
    scd40RelHumi = analogRead(A0);
  }
  if (DEBUG_MODE && !error) {
    Serial.print("SCD40 CO2,Temp,RelHumi: ");
    Serial.println(String(scd40CO2) + ", " + String(scd40Temp) + ", " + String(scd40RelHumi));
  }
}

void scd40ChartAppendData() {
  scd40ChartSeries[0].push(scd40CO2);  // add to queue of chart data points
  scd40ChartSeries[1].push(scd40Temp);
  scd40ChartSeries[2].push(scd40RelHumi);
  if (scd40ChartSeries[0].size() > NB_CHART_POINTS) { // cull the queue down to max nb of points to display
    for ( int i = 0; i < 3; i++) {
      scd40ChartSeries[i].pop();
    }
  }
}

void screenValues1() {  // SEN55 sensor values
  if (DEBUG_MODE) {
    Serial.println("SUB screenValues1");
  }
  screenActive = "values1";
  int index = 0;

  tft.fillScreen(TFT_BLACK);
  tft.fillRoundRect(0, VALUES_Y_START - 4, VALUES_X_START -2 , 4 * VALUES_Y_SPACING, 10 - 2, COLOR_SENSOR_TYPE1_BACKGND);
  tft.fillRoundRect(0, VALUES_Y_START - 4 +  4 * VALUES_Y_SPACING + 4, VALUES_X_START -2 , 2 * VALUES_Y_SPACING, 10 - 2, COLOR_SENSOR_TYPE1_BACKGND);
  tft.fillRoundRect(0, VALUES_Y_START - 4 +  6 * VALUES_Y_SPACING + 8, VALUES_X_START -2 , 2 * VALUES_Y_SPACING, 10 - 2, COLOR_SENSOR_TYPE1_BACKGND);
  
  // Sensirion SEN55 sensors
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(COLOR_SENSOR_TYPE);
  tft.setFreeFont(FONT_SENSOR_TYPE_PRIMARY);
  tft.drawString("   PM1", 0, VALUES_Y_START , 1);
  tft.drawString("      2.5", 0, VALUES_Y_START + 1 * VALUES_Y_SPACING , 1);
  tft.drawString("         4", 0, VALUES_Y_START + 2 * VALUES_Y_SPACING , 1);
  tft.drawString("       10", 0, VALUES_Y_START + 3 * VALUES_Y_SPACING , 1);
  tft.drawString("   VOC", 0, VALUES_Y_START + 4 * VALUES_Y_SPACING + 4, 1);
  tft.drawString("   NOx", 0, VALUES_Y_START + 5 * VALUES_Y_SPACING + 4, 1);
  tft.setFreeFont(FONT_SENSOR_TYPE_SECONDARY);
  tft.setTextColor(TFT_GREY);
  tft.drawString("   T(c)", 0, VALUES_Y_START  + 6 * VALUES_Y_SPACING + 8, 1);
  tft.drawString("   RH%", 0, VALUES_Y_START + 7 * VALUES_Y_SPACING + 8, 1);

  tft.setTextColor(COLOR_SENSOR_DATA);
  tft.setTextDatum(2); // set text justification to upper right corner
  // int column = 0;
  int xSpacing = (SCREEN_WIDTH - VALUES_X_START) / NB_SEN55 / 2;
  int x = VALUES_X_START + xSpacing + 30;
  for (int i=0; i < NB_SEN55; i++) {
    if (!sen55SetupError[i]) {
      tft.setFreeFont(FONT_SENSOR_DATA_PRIMARY);
      tft.setTextColor(COLOR_SENSOR_DATA);
      tft.drawFloat(sen55Pm1[i], 1, x, VALUES_Y_START , 1); // display 1 decimal on floats
      tft.drawFloat(sen55Pm2p5[i], 1, x, VALUES_Y_START + 1 * VALUES_Y_SPACING, 1);
      tft.drawFloat(sen55Pm4[i], 1, x, VALUES_Y_START + 2 * VALUES_Y_SPACING, 1);
      tft.drawFloat(sen55Pm10[i], 1, x, VALUES_Y_START + 3 * VALUES_Y_SPACING, 1);
      tft.drawFloat(sen55Voc[i], 1, x, VALUES_Y_START + 4 * VALUES_Y_SPACING + 4, 1);
      tft.drawFloat(sen55Nox[i], 1, x, VALUES_Y_START + 5 * VALUES_Y_SPACING + 4, 1);
      tft.setFreeFont(FONT_SENSOR_DATA_SECONDARY);
      tft.setTextColor(TFT_GREY);
      tft.drawFloat(sen55Temp[i], 1, x, VALUES_Y_START + 6 * VALUES_Y_SPACING + 8, 1);
      tft.drawFloat(sen55RelHumi[i], 1, x, VALUES_Y_START + 7 * VALUES_Y_SPACING + 8, 1);
      // column++;
    }
    x += xSpacing * 2;
  }

  // display sensor names
  tft.setTextColor(TFT_DARKCYAN);
  tft.setTextDatum(TC_DATUM);
  tft.setTextFont(NULL);  // NULL results in a tiny font that works well here

  x = VALUES_X_START + xSpacing;
  for (int i=0; i < NB_SEN55; i++) {
    if (i == 0) {
      tft.drawString(sen55Name0, x, VALUES_Y_START - 20, 2);
    } else {
      tft.drawString(sen55Name1, x, VALUES_Y_START - 20, 2);
    } 
    x += xSpacing * 2;
  }
  drawBanner();
}

void screenValues2() {  // Multigas & SCD40 sensor values
  if (DEBUG_MODE) {
    Serial.println("SUB screenValues2");
  }
  screenActive = "values2";
  int index = 0;


  // ADD IF NOT ERROR MULTIGAS THEN PROCEED
  // SEEED Multigas V2 sensor
  tft.fillScreen(TFT_BLACK);
  tft.setFreeFont(FONT_SENSOR_TYPE_PRIMARY);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(COLOR_SENSOR_TYPE);
  tft.fillRoundRect(0, VALUES_Y_START + 1, VALUES_X_START - 2, 4 * VALUES_Y_SPACING, 10, COLOR_SENSOR_TYPE2_BACKGND);
  tft.drawString(" VOC", 0, VALUES_Y_START + 5, 1);
  tft.drawString(" CO", 0, VALUES_Y_START + 1 * VALUES_Y_SPACING + 5, 1);
  tft.drawString(" NO2", 0, VALUES_Y_START + 2 * VALUES_Y_SPACING + 5, 1);
  tft.setFreeFont(FONT_SENSOR_TYPE_SMALLER);
  tft.drawString("C2H5CH", 0, VALUES_Y_START + 3 * VALUES_Y_SPACING + 5, 1);

  tft.setFreeFont(FONT_SENSOR_DATA_PRIMARY);          // display sensor data
  tft.setTextColor(COLOR_SENSOR_DATA);
  tft.setTextDatum(TR_DATUM);
  int x = VALUES_X_START + (SCREEN_WIDTH - VALUES_X_START) / 2 + 5;
  tft.drawNumber(mgVoc, x, VALUES_Y_START + 5, 1);
  tft.drawNumber(mgCo, x, VALUES_Y_START + 1 * VALUES_Y_SPACING + 5, 1);
  tft.drawNumber(mgNo2, x, VALUES_Y_START + 2 * VALUES_Y_SPACING + 5, 1);
  tft.drawNumber(mgC2h5ch, x, VALUES_Y_START + 3 * VALUES_Y_SPACING + 5, 1);
  
  tft.setTextColor(COLOR_SENSOR_TYPE2_BACKGND);   // display sensor name
  tft.setTextDatum(TC_DATUM);
  tft.setTextFont(NULL);  // NULL results in a tiny font that works well here
  tft.drawString(multigasName, x - 12, VALUES_Y_START - 15, 2);

  // ADD IF NOT ERROR MULTIGAS THEN PROCEED
  // Adafruit SCD40 CO2 sensor
  tft.setFreeFont(FONT_SENSOR_TYPE_PRIMARY);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(COLOR_SENSOR_TYPE);
  tft.fillRoundRect(0, VALUES_Y_START + 5 * VALUES_Y_SPACING - 1, VALUES_X_START - 2, VALUES_Y_SPACING, 10, COLOR_SENSOR_TYPE3_BACKGND);
  tft.fillRoundRect(0, VALUES_Y_START + 6 * VALUES_Y_SPACING + 3, VALUES_X_START - 2, 2 * VALUES_Y_SPACING, 10, COLOR_SENSOR_TYPE3_BACKGND);
  tft.drawString(" CO2", 0, VALUES_Y_START + 5 * VALUES_Y_SPACING + 3, 1);
  tft.setFreeFont(FONT_SENSOR_TYPE_SECONDARY);
  tft.setTextColor(TFT_DARKGREY);
  tft.drawString(" T(c)", 0, VALUES_Y_START + 6 * VALUES_Y_SPACING + 7, 1);
  tft.drawString(" RH%", 0, VALUES_Y_START + 7 * VALUES_Y_SPACING + 7, 1);

  tft.setFreeFont(FONT_SENSOR_DATA_PRIMARY);          // display sensor data
  tft.setTextColor(COLOR_SENSOR_DATA);
  tft.setTextDatum(TR_DATUM);
  tft.drawNumber(scd40CO2, x, VALUES_Y_START + 5 * VALUES_Y_SPACING + 3, 1);
  tft.setFreeFont(FONT_SENSOR_DATA_SECONDARY); 
  tft.setTextColor(TFT_GREY);
  tft.drawFloat(scd40Temp, 2, x, VALUES_Y_START + 6 * VALUES_Y_SPACING + 7, 1);
  tft.drawFloat(scd40RelHumi, 2, x, VALUES_Y_START + 7 * VALUES_Y_SPACING + 7, 1);

  tft.setTextColor(COLOR_SENSOR_TYPE3_BACKGND);  // display sensor name
  tft.setTextDatum(TC_DATUM);
  tft.setTextFont(NULL);  // NULL results in a tiny font that works well here
  tft.drawString("Sensirion SCD40", x - 12, VALUES_Y_START  + 4 * VALUES_Y_SPACING + 7, 2);

  drawBanner();
}

void screenCharts1() {  // SEN55 PMxy
  if (DEBUG_MODE) {
    Serial.println("SUB screenCharts1");
  }
  tft.fillScreen(COLOR_CHART_BACKGND);
  screenActive = "charts1";
  if (nbSamples > 0) {  // skip if no data to display
    // int x = SCREEN_WIDTH - SCREEN_WIDTH / nbSen55ToDisplay;
    int x = 0;
    // for (int i = NB_SEN55 - 1; i >= 0; i--) {     // starting from rightmost graph to hide Yaxis values position bug from line_chart library
    for (int i = 0; i < NB_SEN55; i++) {
      if (!sen55SetupError[i]) {
        auto graph = line_chart(x, BANNER_HEIGHT); //(x,y) where the line graph begins
        graph
          .height(SCREEN_HEIGHT - BANNER_HEIGHT)    // actual height of the line chart
          .width(SCREEN_WIDTH / nbSen55ToDisplay)  // actual width of the line chart
          // .based_on(50.0)                         // Starting value of y-axis (0 by default), must be a float
          .based_on(0.0)                         // Starting value of y-axis (0 by default), must be a float
          .show_circle(false)                     // drawing a cirle at each point, default is on.
          .value({sen55ChartSeries[i][0], sen55ChartSeries[i][1], sen55ChartSeries[i][2], sen55ChartSeries[i][3]}) // passing the data to line graph
          .max_size(NB_CHART_POINTS)
          .color(TFT_BLUE, TFT_RED, TFT_GREEN, TFT_DARKYELLOW) // Setting the color for the line
          .backgroud(TFT_WHITE);                   // Setting the color for the backgroud
        graph.draw(&tft);
        x += SCREEN_WIDTH / nbSen55ToDisplay;
      }
    }
  } else {
    tft.drawString("    Waiting for samples...     ", 0, 100, 2);
  }
  screenChartFooter("PM1", TFT_BLUE, "2.5", TFT_RED, "4", TFT_GREEN, "10", TFT_DARKYELLOW);
  drawBanner();
}

void screenCharts2() {  // SEN55 VOC, NOx
  if (DEBUG_MODE) {
    Serial.println("SUB screenCharts2");
  }
  tft.fillScreen(COLOR_CHART_BACKGND);
  screenActive = "charts2";
  // int x = SCREEN_WIDTH - SCREEN_WIDTH / nbSen55ToDisplay;
  // for (int i = NB_SEN55 - 1; i >= 0; i--) {     // starting from rightmost graph to hide Yaxis values position bug from line_chart library
  int x = 0;
  for (int i = 0; i < NB_SEN55; i++) { 
    if (!sen55SetupError[i]) {
      auto graph = line_chart(x, BANNER_HEIGHT); //(x,y) where the line graph begins
      graph
        .height(SCREEN_HEIGHT - BANNER_HEIGHT)    // actual height of the line chart
        .width(SCREEN_WIDTH / nbSen55ToDisplay)  // actual width of the line chart
        // .based_on(50.0)                         // Starting value of y-axis (0 by default), must be a float
        .based_on(0.0)                         // Starting value of y-axis (0 by default), must be a float
        .show_circle(false)                     // drawing a cirle at each point, default is on.
        .value({sen55ChartSeries[i][4], sen55ChartSeries[i][5]}) // passing the data to line graph
        .max_size(NB_CHART_POINTS)
        .color(TFT_BLUE, TFT_MAGENTA, TFT_GREEN, TFT_DARKYELLOW) // Setting the color for the line
        .backgroud(TFT_WHITE);                  // Setting the color for the backgroud
      graph.draw(&tft);
      x += SCREEN_WIDTH / nbSen55ToDisplay;
    }
  }
  screenChartFooter("", TFT_BLUE, "VOC", TFT_BLUE, "NOx", TFT_MAGENTA, "", TFT_BLUE);
  drawBanner();
}

void screenCharts3() {  // SEN55 Temp, RelHumi
  if (DEBUG_MODE) {
    Serial.println("SUB screenCharts3");
  }
  tft.fillScreen(COLOR_CHART_BACKGND);
  screenActive = "charts3";
  // int x = SCREEN_WIDTH - SCREEN_WIDTH / nbSen55ToDisplay;
  // for (int i = NB_SEN55 - 1; i >= 0; i--) {     // starting from rightmost graph to hide Yaxis values position bug from line_chart library
  int x = 0;
  for (int i = 0; i < NB_SEN55; i++) { 
    if (!sen55SetupError[i]) {
      auto graph = line_chart(x, BANNER_HEIGHT); //(x,y) where the line graph begins
      graph
        .height(SCREEN_HEIGHT - BANNER_HEIGHT)    // actual height of the line chart
        .width(SCREEN_WIDTH / nbSen55ToDisplay)  // actual width of the line chart
        // .based_on(50.0)                         // Starting value of y-axis (0 by default), must be a float
        .based_on(0.0)                         // Starting value of y-axis (0 by default), must be a float
        .show_circle(false)                     // drawing a cirle at each point, default is on.
        .value({sen55ChartSeries[i][6], sen55ChartSeries[i][7]}) // passing the data to line graph
        .max_size(NB_CHART_POINTS)
        .color(TFT_GREEN, TFT_DARKYELLOW) // Setting the color for the line
        .backgroud(TFT_WHITE);                  // Setting the color for the backgroud
      graph.draw(&tft);
      x += SCREEN_WIDTH / nbSen55ToDisplay;
    }
  }
  screenChartFooter("", TFT_BLUE, "T(c)", TFT_GREEN, "RH%", TFT_DARKYELLOW, "", TFT_RED);
  drawBanner();
}

void screenCharts4() {  // Multigas VOC, CO, NO2, C2H5CH
  if (DEBUG_MODE) {
    Serial.println("SUB screenCharts4");
  }
  tft.fillScreen(COLOR_CHART_BACKGND);
  screenActive = "charts4";
  if (!multigasSetupError) {
    auto graph = line_chart(0, BANNER_HEIGHT); //(x,y) where the line graph begins
    graph
      .height(SCREEN_HEIGHT - BANNER_HEIGHT)    // actual height of the line chart
      .width(SCREEN_WIDTH)  // actual width of the line chart
      // .based_on(50.0)                         // Starting value of y-axis (0 by default), must be a float
      .based_on(0.0)                         // Starting value of y-axis (0 by default), must be a float
      .show_circle(false)                     // drawing a cirle at each point, default is on.
      .value({multigasChartSeries[0], multigasChartSeries[1], multigasChartSeries[2], multigasChartSeries[3]})      // passing the data to line graph
      .max_size(NB_CHART_POINTS)
      .color(TFT_BLUE, TFT_RED, TFT_MAGENTA, TFT_DARKCYAN) // Setting the color for the line
      .backgroud(TFT_WHITE)                  // Setting the color for the backgroud
      .draw(&tft);
  }
  screenChartFooter("VOC", TFT_BLUE, "CO", TFT_RED, "NO2", TFT_MAGENTA, "C2H5CH", TFT_DARKCYAN);
  drawBanner();
}

void screenCharts5() {  // SCD40 CO2, Temp, RelHumi
  if (DEBUG_MODE) {
    Serial.println("SUB screenCharts5");
  }
  tft.fillScreen(COLOR_CHART_BACKGND);
  screenActive = "charts5";
  if (!scd40SetupError) {
    auto graph = line_chart(0, BANNER_HEIGHT);
    graph
      .height(SCREEN_HEIGHT - BANNER_HEIGHT)    // actual height of the line chart
      .width(SCREEN_WIDTH / 2)                  // actual width of the line chart
      .based_on(0.0)                            // Starting value of y-axis (0 by default), must be a float
      .show_circle(false)                       // drawing a cirle at each point, default is on.
      .value({scd40ChartSeries[0]})             // passing the data to line graph
      .max_size(NB_CHART_POINTS)
      .color(TFT_RED)                           // Setting the color for the line
      .backgroud(TFT_WHITE)                     // Setting the color for the backgroud
      .draw(&tft);
    int x = SCREEN_WIDTH - SCREEN_WIDTH / 2;
    graph = line_chart(x, BANNER_HEIGHT);       //(x,y) where the line graph begins
    graph
      .height(SCREEN_HEIGHT - BANNER_HEIGHT)    // actual height of the line chart
      .width(SCREEN_WIDTH / 2)                  // actual width of the line chart
      .based_on(0.0)                            // Starting value of y-axis (0 by default), must be a float
      .show_circle(false)                       // drawing a cirle at each point, default is on.
      .value({scd40ChartSeries[1], scd40ChartSeries[2]})  // passing the data to line graph
      .max_size(NB_CHART_POINTS)
      .color(TFT_GREEN, TFT_DARKYELLOW)         // Setting the color for the line
      .backgroud(TFT_WHITE)                     // Setting the color for the backgroud
      .draw(&tft);
  }
  screenChartFooter("", TFT_RED, "CO2", TFT_RED, "T(c)", TFT_GREEN, "RH%", TFT_DARKYELLOW);
  drawBanner();
}

void screenChartFooter(String label0, color_t col0, String label1, color_t col1, String label2, color_t col2, String label3, color_t col3) {
  int x = 64;
  int delta = 46;
  int w = 42;
  int x0 = w / 2;
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(TC_DATUM);
  tft.setTextFont(NULL);
  if (label0 != "") {
    tft.fillRoundRect(x, SCREEN_HEIGHT - 12, w, 12, 4, col0);
    tft.drawString(label0, x + x0, SCREEN_HEIGHT - 14, 2);
  }
  x += delta;
  if (label1 != "") {
    tft.fillRoundRect(x, SCREEN_HEIGHT - 12, w, 12, 4, col1);
    tft.drawString(label1, x + x0, SCREEN_HEIGHT - 14, 2);
  }
  x += delta;
  if (label2 != "") {
    tft.fillRoundRect(x, SCREEN_HEIGHT - 12, w, 12, 4, col2);
    tft.drawString(label2, x + x0, SCREEN_HEIGHT - 14, 2);
  }
  x += delta;
  if (label3 != "") {
    tft.fillRoundRect(x, SCREEN_HEIGHT - 12, w, 12, 4, col3);
    tft.drawString(label3, x + x0, SCREEN_HEIGHT - 14, 2);
  }

  // display the sensor name on each chart
  tft.setTextColor(TFT_BLACK);
  tft.setTextDatum(BL_DATUM);
  int senIndex = 0;
  String txt = "";
  if (screenActive == "charts1" || screenActive == "charts2" || screenActive == "charts3") {
    for (int i=0; i<NB_SEN55; i++) {
      int xtxt = 0;
      if (!sen55SetupError[i]) {
        if (i == 0) {
          txt = sen55Name0;
        } else if (i == 1) {
          txt = sen55Name1;
        } else {
          txt = sen55Name2;
        }
        if (senIndex == 0) {
          xtxt = 35;
        } else if (senIndex == 1) {
          if (nbSen55ToDisplay == 2) {
            xtxt = 195;
          } else {
            xtxt = 100;    // TODO 3 graphs not tested yet
          }
        } else {
          xtxt = 190;
        }
        tft.drawString(txt, xtxt, SCREEN_HEIGHT - 16, 1);
        senIndex += 1;
      }
    }
  } else if (screenActive == "charts4") {
    tft.drawString(multigasName, 35, SCREEN_HEIGHT - 16, 1);    
  } else if (screenActive == "charts5") {
    tft.drawString(scd40Name, 35, SCREEN_HEIGHT - 16, 1);    
    tft.drawString(scd40Name, 195, SCREEN_HEIGHT - 16, 1);    
  }
  String xt;
  if (nbSamples < NB_CHART_POINTS) {
    xt = "Xaxis=" + String(nbSamples * screenSampleInterval) + "s";
  } else {
    xt = "Xaxis=" + String(NB_CHART_POINTS * screenSampleInterval) + "s";
  }
  tft.setTextDatum(BR_DATUM);
  tft.drawString(xt, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, 1);
}

void screenMainSettings() { // display the list of settings and their values
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(COLOR_SETTING_TEXT);
  String txt, txt2, suffix;
  int posChar;
  int y0 = settingsYstart;
  auto color2 = COLOR_SETTING_TEXT;
  for (int i = 0; i < NB_SETTING_LINES; i++) {
    txt = settingsText[i] + "  ";
    txt2 = "";
    // suffix = "";
    rtcNow = rtc.now();
    if (txt.startsWith("Date:")) {
      txt = "";
      txt2 = String(rtcNow.year()) + "-" + String(rtcNow.month()) + "-" + String(rtcNow.day()) + " " + String(padWithLeadingZeros(rtcNow.hour(), 2)) + ":" + String(padWithLeadingZeros(rtcNow.minute(), 2)) + ":" + String(padWithLeadingZeros(rtcNow.second(), 2));
      txt2 += "  Since turn-on: " + timeSinceTurnOn();
    } else if (txt.startsWith("Current nb of sample")) {
      txt2 = String(nbSamples) + "  Logged: ";
      if (loggingOn) {
        txt2 += String(nbLogRecords);
      }
    } else if (txt.startsWith("SRAM memory")) {
      char top;
      int mem = &top - reinterpret_cast<char*>(sbrk(0)); // memory space between top of heap and bottom of stack
      mem = int(mem / 1024);
      txt2 = "192/" + String(mem) + "KB";
    } else if (txt.startsWith("Screen sample")) {
      // suffix = " (>=5 for SCD40)";                              // SCD40 has a refractory window of <5s after a measurement. Faster sampling gets "data not ready" and previous C/T/RH values
      txt2 = padWithLeadingZeros(screenSampleInterval, 2);
      color2 = COLOR_SETTING_USER;
    } else if (txt.startsWith("SD log sample interval")) {
      // suffix = " (>=5 for SCD40)";                              // SCD40 has a refractory window of <5s after a measurement. Faster sampling gets "data not ready" and previous C/T/RH values
      txt2 = padWithLeadingZeros(logSampleInterval, 3);
      color2 = COLOR_SETTING_USER;
    } else if (txt.startsWith("SD log duration")) {
      txt2 = settingLogDuration;
      color2 = COLOR_SETTING_USER;
    } else if (txt.startsWith("SD log countdown")) {
      txt2 += padWithLeadingZeros(settingLogCountdownHH, 2) + " ";
      txt2 += padWithLeadingZeros(settingLogCountdownMM, 2) + " ";
      txt2 += padWithLeadingZeros(settingLogCountdownSS, 2);
      color2 = COLOR_SETTING_USER;
    } else if (txt.startsWith("Sensor shutdown")) {
      txt2 = settingShutdown;
      color2 = COLOR_SETTING_USER;
    } else if (txt.startsWith("Sensor power up")) {
      txt2 = padWithLeadingZeros(settingPowerUpTime, 3);
      color2 = COLOR_SETTING_USER;
    } else if (txt.startsWith("LCD brightness")) {
      txt2 = String(tftBrightness);
      color2 = COLOR_SETTING_USER;
    }
    if (txt != "") {
      posChar = txt.indexOf(":");
      txt = txt.substring(0, posChar + 1);
    }
    if (settingsYcursor == 0) { // initialize user cursor
      if (txt.startsWith("Screen sample")) {    // 1st setting that is user modifyable
        settingsYcursor = y0;
        settingsXcursor = settingsXstart + tft.textWidth(txt, 2);
        settingChangingValue = screenSampleInterval;
      }
    }
    if (txt != "") {
      txt += " ";
      tft.drawString(txt, settingsXstart, y0, 2);
    }
    int x = tft.textWidth(txt, 2);
    // if (suffix != "") {
    //   tft.drawString(suffix, settingsXstart + x + 20, y0, 2);
    // }
    tft.setTextColor(color2);
    tft.drawString(txt2, settingsXstart + x, y0, 2);
    if (!loggingOn && !bannerActive) { // can't change settings while logging
      if (settingsYcursor == y0) { // Display cursor
        drawCursor(settingsXcursor, settingsYcursor);
        Serial.print(settingsXcursor); Serial.print(":");Serial.print(settingsYcursor); Serial.print("=");Serial.println(y0);
      }
    }
    color2 = COLOR_SETTING_TEXT;
    tft.setTextColor(color2);
    y0 += settingsYspacing;
  }
  drawBanner();
}

void screenSnapshot() {  // Take a snapshot of the current screen
  if (sdCardMount()) {
    if (createBMPfile()) {
      writeBMPheader();
      writePixelsToSDcard();
    }
  }
  flashBanner("stop");
}

bool createBMPfile() {  // Create a BMP file for the screenshot
  int i = 0;
  String name = "";
  do {
    name = bmpFileName + String(i) + ".bmp";
    if (!devSD.exists(name)) {
      bmpFile = devSD.open(name, FILE_WRITE);
      if (devSD.exists(name)) {
        Serial.println("Created file " + name);
        return true;
        break;
      } else {
        // found an unused name but creating file failed
        Serial.println("Failed to create BMP file " + name + " for screenshot");
        return false;
        break;
      }
    } else {
      i++;            // increment name suffix
    }
  } while (i < 1000); // after 1000 attempts it might be time to move on :-)
}

void writeBMPheader() {
  if (DEBUG_MODE) {
    Serial.println("Writing BMP header");
  }
  bmpFile.write((uint8_t*)&bmpHeader, sizeof(bmpHeader));
  if (DEBUG_MODE) {
    Serial.println("BMP header written");
  }
}

void writePixelsToSDcard() {  // Read each pixel line and write to SD card
  // Color accurate, even for mixed r,g,b colors.
  // Slow custom method that reads 16b (565) RGB pixel values and converts to 888
  // Replaces the native tft.readRGB call that provides very inaccurate mixed colors (only works for pure R, G or B)

  if (DEBUG_MODE) {
    Serial.println("Writing pixels, custom 565-888 method");
  }
  for (int i = 0; i < SCREEN_HEIGHT; i++) {
    if (i == BANNER_HEIGHT) {
      flashBanner("start");   // can now flash the banner as its pixels have been captured
    }
    tft.readRect(0, i, SCREEN_WIDTH, 1, (uint16_t*)&pixelsIN);
    convert565to888_color_accurate();
    bmpFile.write((uint8_t*)&pixels888, sizeof(pixels888));
  }
  Serial.println("Screenshot saved to SD card");
  bmpFile.close();
  Serial.println("BMP file closed");
}

void convert565to888_color_accurate() {  // convert each pixel's 16b (565) value to 24b (888). Color accurate.
  uint8_t r5, g6, b5;   // 565 RGB values
  uint8_t r8, g8, b8;   // 888 RGB converted values
  uint16_t *val16b;
  uint16_t val565; // value with swapped endian-ness (ie low / high bytes swapped)
  float f;

  for (int i = 0; i < SCREEN_WIDTH; i++) {
    // get 16b-pixel value
    val16b = (uint16_t*)&pixelsIN[i * 2];

    // swap high / low bytes (little/big endian swap)
    val565 = ((*val16b & 0xFF00) >> 8);
    val565 += (*val16b & 0x00FF) * 256;

    // extract individual colors: 565 = rrrr rggg gggb bbbb
    r5 = ((val565 & 0xF800) >> 8);
    g6 = ((val565 & 0x07E0) >> 3);
    b5 = ((val565 & 0x001F) << 3);

    // scale colors from 248(5b<<3) and 252(6b<<2) to 255
    f = b5 * 255;
    f /= 248;
    b8 = (uint8_t)f;
    f = g6 * 255;
    f /= 252;
    g8 = (uint8_t)f;
    f = r5 * 255;
    f /= 248;
    r8 = (uint8_t)f;

    // save rgb888 values into 24b-pixel line
    pixels888[i*3] = b8;
    pixels888[i*3+1] = g8;
    pixels888[i*3+2] = r8;
  }
}

void screenBattery() {  // Display power measurements and battery information
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(COLOR_SETTING_TEXT);

  // Read battery stats from the BQ27441-G1A
  // Test results: by default the WIO battery pack gets charged to 4.2v and cuts off around 2.98v (2nd run 2.976)
  String socStr, voltsStr, currentStr, fullCapacityStr, capacityStr, powerStr, statusStr, temperatureStr, tempStr;
  unsigned int socVal = lipo.soc();                                 // Read state-of-charge (%)
  if (socVal <= 100) {                                             // if can't find the bq27441 most values read 65535
    unsigned int voltsVal = lipo.voltage();                         // Read battery voltage (mV)
    int currentVal = lipo.current(AVG);                             // Read average current (mA)
    unsigned int fullCapacityVal = lipo.capacity(FULL);             // Read full capacity (mAh)
    unsigned int capacityVal = lipo.capacity(REMAIN);               // Read remaining capacity (mAh)
    int powerVal = lipo.power();                                    // Read average power draw (mW)
    unsigned int statusVal = lipo.status();
    unsigned int temperatureVal = lipo.temperature(INTERNAL_TEMP);  // Sparkfun library claims value in degree celsius, but seems to report in °C x 100 (ie shifted by 2 decimal digits). bq27441 datasheet reports register unit is 0.1°K
    // unsigned int health = lipo.soh();                            // Read state-of-health (%). Removing, always displays 0
    // unsigned int temp_battery = lipo.temperature(BATTERY);       // the WIO battery pack does NOT implement a battery thermistor
    socStr = String(socVal) + " % ";
    voltsStr = String(voltsVal) + " mV ";
    currentStr = String(currentVal) + " mA ";
    fullCapacityStr = String(fullCapacityVal) + " mAh ";
    capacityStr = String (capacityVal) + " mAh ";
    powerStr = String(powerVal) + " mW ";
    statusStr = String (statusVal);

    // Separating integer value from decimal in temperature reading
    int tint = int(temperatureVal / 100);
    int tdec = temperatureVal - tint * 100;
    tempStr = String(tint) + ".";
    if (tdec > 9) {
      tempStr += String(tdec) + " oC ";
    } else {
      tempStr += "0" + String(tdec) + " oC ";
    }
  } else {
    tft.setTextColor(COLOR_BATTERY_ABSENT_TEXT);
    socStr = "N/A";
    voltsStr = "N/A";
    currentStr = "N/A";
    fullCapacityStr = "N/A";
    capacityStr = "N/A";
    powerStr = "N/A";
    statusStr = "N/A";
    tempStr = "N/A";
  }

  int xdata = 146;
  int y0 = settingsYstart;
  tft.drawString("State of Charge", settingsXstart, y0, 2);
  tft.drawString(socStr, xdata, y0, 2);
  y0 += settingsYspacing;
  tft.drawString("Voltage", settingsXstart, y0, 2);
  tft.drawString(voltsStr, xdata, y0, 2);
  y0 += settingsYspacing;
  tft.drawString("Current", settingsXstart, y0, 2);
  tft.drawString(currentStr, xdata, y0, 2);
  y0 += settingsYspacing;
  tft.drawString("Power", settingsXstart, y0, 2);
  tft.drawString(powerStr, xdata, y0, 2);
  y0 += settingsYspacing;
  tft.drawString("Capacity Left", settingsXstart, y0, 2);
  tft.drawString(capacityStr, xdata, y0, 2);
  y0 += settingsYspacing;
  tft.drawString("Capacity when Full", settingsXstart, y0, 2);
  tft.drawString(fullCapacityStr + " (spec= " + String(BATTERY_CAPACITY) + ")", xdata, y0, 2);
  y0 += settingsYspacing;
  tft.drawString("BQ27441 BMS status", settingsXstart, y0, 2);
  tft.drawString(statusStr, xdata, y0, 2);
  y0 += settingsYspacing;
  tft.drawString("BQ27's Temperature", settingsXstart, y0, 2);
  tft.drawString(tempStr, xdata, y0, 2);

  drawBanner();
}

void screenBanner() {  // Displays the Banner at the top of the screen
  if (settingChanging) { // cancel setting editing
    settingChanging = false;
    displaySingleSetting();
  }
  if (!bannerActive) {
    bannerXcursor = BANNER_X1;
  }
  displayBannerCursor(true);
  if (joystickState == "press") {
    if (bannerXcursor == BANNER_X1) {
      if (loggingOn) {
        loggingStop();
      } else {
        if (!sdEjected) {
          loggingStart();
        }
      }
    } else if (bannerXcursor == BANNER_X2) {
      if (!loggingOn) {
        screenActive = "settings";
        displayBannerCursor(false);
        screenMainSettings();
      } else {  // while logging can't change settings
        screenActive = "settings";
        screenMainSettings();
      }
    } else if (bannerXcursor == BANNER_X3) {
      if (sdPresent) {
        // if (!loggingOn) {
            screenActive = "sdcard1";
            displayBannerCursor(false);
            screenSdCardSettings();
        // } else {  // while logging can't change settings
        //     screenActive = "sdcard1";
        //     screenSdCardSettings();
        // }
      }
    } else if (bannerXcursor == BANNER_X4) {
      screenSnapshot();
    } else if (bannerXcursor == BANNER_X5) {
      screenActive = "battery";
      screenBattery();
    }
  } else if (joystickState == "right") {
    if (bannerXcursor == BANNER_X1) {
      bannerXcursor = BANNER_X2;
    } else if (bannerXcursor == BANNER_X2) {
      bannerXcursor = BANNER_X3;
    } else if (bannerXcursor == BANNER_X3) {
      bannerXcursor = BANNER_X4;
    } else if (bannerXcursor == BANNER_X4) {
      bannerXcursor = BANNER_X5;
    }
    displayBannerCursor(true);
  } else if (joystickState == "left") {
    if (bannerXcursor == BANNER_X2) {
      bannerXcursor = BANNER_X1;
    } else if (bannerXcursor == BANNER_X3) {
      bannerXcursor = BANNER_X2;
    } else if (bannerXcursor == BANNER_X4) {
      bannerXcursor = BANNER_X3;
    } else if (bannerXcursor == BANNER_X5) {
      bannerXcursor = BANNER_X4;
    }
    displayBannerCursor(true);
  }
}

void drawBanner() {
  tft.fillRect(0, 0, SCREEN_WIDTH, 20, TFT_BLACK);
  tft.setFreeFont(NULL);
  tft.setTextDatum(TL_DATUM);
  auto color = TFT_DARKGREY;
  tft.setTextColor(color);
  int xspacing = 1;
  int ystart = 2;
  int x = BANNER_X_START;


  // display the RECORD icon
  if (sdEjected) {
    color = COLOR_SD_ABSENT;
  } else {
    if (loggingOn) {
      color = TFT_RED;
    } else {
      color = TFT_WHITE;
    }
  }
  tft.drawBitmap(x, ystart, *ICON_RECORD_LARGE, 64, 16, color);


  // display the record ELAPSED / TIMER time
  x+= 42;
  String txt = "hh:mm:ss";
  tft.setTextColor(color);
  if (settingLogDuration == "countdown") {   // display Samples Record (ie 'log') countdown time
    if (loggingOn) {
      unsigned long time = millisLogCountdownEnd - millis();
      int h, m, s;
      convertMillisHMS(time, h, m, s);
      txt = padWithLeadingZeros(h, 2) + ":" + padWithLeadingZeros(m, 2) + ":" + padWithLeadingZeros(s, 2);
    } else {
      txt = padWithLeadingZeros(settingLogCountdownHH, 2) + ":" + padWithLeadingZeros(settingLogCountdownMM, 2) + ":" + padWithLeadingZeros(settingLogCountdownSS, 2);
    }
  } else if (loggingOn) {                     // display time since Sample Record (ie 'log') started
    unsigned long time = millis() - millisLogStart;
    int h, m, s;
    convertMillisHMS(time, h, m, s);
    txt = padWithLeadingZeros(h, 2) + ":" + padWithLeadingZeros(m, 2) + ":" + padWithLeadingZeros(s, 2);
  } else {
    txt = "RECORD";
  }
  if (txt == "RECORD") {
    tft.drawString(txt, 23, ystart + 5, 1);
  }else {
    tft.drawString(txt, 19, ystart + 5, 1);
  }
  tft.setTextColor(TFT_DARKGREY);
  x += 43;


  // display the SAMPLING INTERVALS
  if (screenSampleInterval < logSampleInterval) { // if log internal is shorter it'll sample faster, no more need for sample interval. Visual clue it is disabled
    tft.setTextDatum(TR_DATUM);
    tft.drawString(String(screenSampleInterval) + "\"", x, ystart + 5, 1);
    tft.setTextDatum(TL_DATUM);
  }
  x -= 2;
  tft.drawBitmap(x, ystart - 1, *ICON_STOPWATCH, 16, 16, TFT_DARKGREY);
  x += 17;
  tft.drawString(String(logSampleInterval) + "\"", x, ystart + 5, 1);

  tft.drawBitmap(BANNER_X2, ystart, *ICON_SETTINGS4, 16, 16, COLOR_BANNER_ICON_ENABLED);


  // display the SD CARD icon
  color_t c = COLOR_BANNER_ICON_ENABLED;
  if (sdEjected) {
    c = COLOR_SD_ABSENT;
  }
  tft.drawBitmap(BANNER_X3, ystart, *ICON_SD_CARD, 16, 16, c);


  // display the SCREENSHOT icon
  tft.drawBitmap(BANNER_X4, ystart, *ICON_SCREENSHOT, 16, 16, c);


  // display CURRENT TIME as HH:MM
  String timeHHMM = "HH:MM"; // Reserving memory
  timeHHMM = "";
  rtcNow = rtc.now();
  int hh = rtcNow.hour();
  if (hh < 10) {timeHHMM += "0";}
  timeHHMM += String(hh) + ":";
  int mm = rtcNow.minute();
  if (mm < 10) {timeHHMM += "0";}
  timeHHMM += String(mm);
  x = 290;
  // tft.setTextDatum(TL_DATUM);
  tft.drawString(timeHHMM, x, ystart + 5, 1);


  // display BATTERY icon with state of charge level
  // tft.setTextDatum(TL_DATUM);
  x -= 19 + xspacing;
  // tft.fillRect(x + 4, ystart + 10, 8, 6, TFT_ORANGE);
  unsigned int soc = lipo.soc();  // Read state-of-charge (%)
  if (soc > 100) {   // SOC should be 0-100. When battery pack is not responding soc = 64535. Attempt an initialization in case it was just inserted
    lipo.begin();
    soc = lipo.soc();
  }
  if (soc <= 100) {   // might be too soon for the battery pack to respond w valid data if an initialization was just performed. But will be ready at next banner refresh
    int power = lipo.power();  // Read average power draw (mW)
    tft.drawBitmap(x, ystart, *ICON_BATTERY, 16, 16, TFT_WHITE);
    int rectW = 8;
    int rectH = 12;
    int rectX = x + 4;
    int rectY = ystart + 3;
    auto colorGauge = TFT_GREEN;
    int gaugeH = 12 * soc;
    int rem = gaugeH % 100;
    gaugeH /= 100;
    if (rem > 0) {
      gaugeH += 1; // rounding up
    }
    if (gaugeH > 12) { // should theoritically never happen though
      gaugeH = 12;
    }
    tft.fillRect(rectX, rectY, rectW, rectH, TFT_BLACK);    // erasing previous gauge level
    // if (soc <= 100) {
    if (soc <= 17) {               // icon has a gauge rectangle 12 pixels high. Aiming for 2 pixels high to show RED level has been crossed: red_level_threshold = 100 / 12 * 2 = 16.7
      colorGauge = TFT_RED;
      tft.drawBitmap(x, ystart, *ICON_BATTERY, 16, 16, TFT_RED);
    } else if (soc <= 42) {       // Aiming for least 5 pixels high to show Yellow level has been crossed: yellow_level_threshold = 100 / 12 * 5 = 41.7
      colorGauge = TFT_YELLOW;
    }
    tft.fillRect(rectX, rectY + rectH - gaugeH, rectW, gaugeH, colorGauge);   // display SOC gauge
    // } else {  // should not happen, max soc value should be 100
    //   tft.drawBitmap(x, ystart, *iconBattery, 16, 16, TFT_DARKERGREY);
    //   tft.fillRect(rectX, rectY, rectW, rectH, TFT_RED);
    // }
    if (power >= 0) {    // USB-C power is ON, so display a plug charging icon
      // tft.drawBitmap(x + 5, ystart + 4, *iconBatteryPlug, 8, 12, TFT_BLUE);
      tft.fillRect(x + 9, ystart + 4, 8, 12, TFT_BLACK);
      tft.drawBitmap(x + 10, ystart + 4, *ICON_BATTERY_PLUG, 8, 12, TFT_WHITE);
    }
  } else {    // Did not find a battery pack. Greying out the battery icon
    tft.drawBitmap(x, ystart, *ICON_BATTERY, 16, 16, TFT_DARKERGREY);
  }


  // Remaining icons
  // x -= 18 + xspacing;
  // tft.drawBitmap(x, ystart, *ICON_WIFI3, 16, 16, TFT_GREEN);
  // x -= 33 + xspacing;
  // tft.drawBitmap(x, ystart, *ICON_RADIO, 32, 16, TFT_DARKGREY);
  // tft.setTextColor(TFT_ORANGE);
  // tft.drawString("2", x + 13, ystart + 7, 1);

  if (bannerActive) {
    displayBannerCursor(true);
  }
}

void displayBannerCursor(bool enable) {
  auto c = TFT_BLACK;
  int height = 20;
  int y = 18;
  int wRec = 66;  // width of Record icon's rectangle cursor
  tft.drawRect(BANNER_X_START - 2, 0, wRec + 2, height, c);
  tft.drawRect(BANNER_X_START - 1, 1, wRec, height - 2, c);
  tft.drawRect(BANNER_X2 - 2, 0, BANNER_CURSOR_WIDTH, height, c);
  tft.drawRect(BANNER_X2 - 1, 1, BANNER_CURSOR_WIDTH - 2, height - 2, c);
  tft.drawRect(BANNER_X3 - 2, 0, BANNER_CURSOR_WIDTH, height, c);
  tft.drawRect(BANNER_X3 - 1, 1, BANNER_CURSOR_WIDTH - 2, height - 2, c);
  tft.drawRect(BANNER_X4 - 2, 0, BANNER_CURSOR_WIDTH, height, c);
  tft.drawRect(BANNER_X4 - 1, 1, BANNER_CURSOR_WIDTH - 2, height - 2, c);
  tft.drawRect(BANNER_X5 - 3, 0, BANNER_CURSOR_WIDTH, height, c);
  tft.drawRect(BANNER_X5 - 2, 1, BANNER_CURSOR_WIDTH - 2, height - 2, c);
  bannerActive = false;
  if (enable) {
    bannerActive = true;
    c = COLOR_BANNER_CURSOR;
    if (bannerXcursor == BANNER_X1) {
      tft.drawRect(BANNER_X_START - 2, 0, wRec + 2, height, c);
      tft.drawRect(BANNER_X_START - 1, 1, wRec, height - 2, c);
    } else if (bannerXcursor == BANNER_X5) {
      tft.drawRect(bannerXcursor - 3, 0, BANNER_CURSOR_WIDTH, height, c);
      tft.drawRect(bannerXcursor - 2, 1, BANNER_CURSOR_WIDTH - 2, height - 2, c);
    } else {
      tft.drawRect(bannerXcursor - 2, 0, BANNER_CURSOR_WIDTH, height, c);
      tft.drawRect(bannerXcursor - 1, 1, BANNER_CURSOR_WIDTH - 2, height - 2, c);
    }
    settingsYcursor = 0; // resetting cursor from Settings screen
  }
}

void flashBanner(String action) {  // Blinks the banner to show system busy with a long action (like taking a screenshot)
  if (action == "start") {
    tft.fillRect(0, 0, SCREEN_WIDTH, BANNER_HEIGHT, TFT_BLACK);
  } else if (action == "stop") {
    drawBanner();
  } else {
    tft.invertDisplay(1);
    // tft.fillRect(0, 0, SCREEN_WIDTH, BANNERHEIGHT, TFT_BLACK);
    delay(action.toInt());  // DEBUG ONLY. Delays are blocking instructions, interferes with sampling timing. Do not use in final code
    // drawBanner();
    tft.invertDisplay(0);
  }
}

String timeSinceTurnOn() {
      String t = "HHH:MM:SS"; // Reserving memory space. Note: if more than 24 hours since PwrOn then HH could be more than 2 digits
      int h, m, s;
      convertMillisHMS(millis() - millisAtPowerOn, h, m, s);
      t = "";
      if (h < 10) {
        t += "0";
      }
      t += String(h) +":";
      if (m < 10) {
        t += "0";
      }
      t += String(m) + ":";
      if (s < 10) {
        t += "0";
      }
      t += String(s);
      // t = padWithLeadingZeros(h, 2) + ":" + padWithLeadingZeros(m, 2) + ":" + padWithLeadingZeros(s, 2);
      return t;
}

String padWithLeadingZeros(int val, int nbDigits) {
  String pad = "";
  String valTxt = String(val);
  int n = nbDigits - valTxt.length();
  for (int i = 0; i < n; i++) {
    pad += "0";
  }
  pad += valTxt;
  return pad;
}

String getSettingXFieldIndex(String name) {
  int x1 = tft.textWidth(name, 2) + settingsXstart; // 1st field's cursor position
  if (name.startsWith("SD log countdown")) { // 3 fields: HH MM SS
    int x2 = tft.textWidth(name + " 00", 2) + settingsXstart; // cursor position for MM
    if (settingsXcursor == x1) {
      return("h"); // HH field
    } else if (settingsXcursor == x2) {
      return("m"); // MM field
    } else {
      return("s"); // SS field
    }
  }
}

void displaySingleSetting() {
  auto c = COLOR_SETTING_USER;
  if (settingChanging) {
    c = COLOR_SETTING_CHANGING;
  }
  tft.setTextColor(c);
  String txt;
  String name = settingsText[getSettingYIndexFromCursor()];
  name = name.substring(0, name.indexOf(":") + 1);
  if (name.startsWith("Screen sample")) {
    if (!settingChanging) {
      txt = ">" + padWithLeadingZeros(screenSampleInterval, 2);
    } else {
      txt = ">" + padWithLeadingZeros(settingChangingValue, 2);
    }
    tft.fillRect(settingsXcursor, settingsYcursor, 4 * settingsFontWidth, settingsFontHeight, TFT_BLACK); // erasing previous text
    tft.drawString(txt, settingsXcursor, settingsYcursor, 2);
    drawCursor(settingsXcursor, settingsYcursor);
  } else if (name.startsWith("SD log sample interval")) {
    if (!settingChanging) {
      txt = ">" + padWithLeadingZeros(logSampleInterval, 3);
    } else {
      txt = ">" + padWithLeadingZeros(settingChangingValue, 3);
    }
    tft.fillRect(settingsXcursor, settingsYcursor, 5 * settingsFontWidth, settingsFontHeight, TFT_BLACK); // erasing previous text
    tft.drawString(txt, settingsXcursor, settingsYcursor, 2);
    drawCursor(settingsXcursor, settingsYcursor);
  } else if (name.startsWith("SD log duration")) {
    if (!settingChanging) {
      txt = ">" + settingLogDuration;
    } else {
      txt = ">" + settingChangingText;
    }
    tft.fillRect(settingsXcursor, settingsYcursor, 13 * settingsFontWidth, settingsFontHeight, TFT_BLACK); // erasing previous text
    tft.drawString(txt, settingsXcursor, settingsYcursor, 2);
    drawCursor(settingsXcursor, settingsYcursor);
  } else if (name.startsWith("SD log countdown")) {
    tft.fillRect(settingsXcursor, settingsYcursor, tft.textWidth(">00", 2), settingsFontHeight, TFT_BLACK); // erasing previous text
    String field = getSettingXFieldIndex(name);
    if (field == "h") {
      if (!settingChanging) {
        txt = ">" + padWithLeadingZeros(settingLogCountdownHH, 2);
      } else {
        txt = ">" + padWithLeadingZeros(settingChangingValue, 2);
      }
    } else if (field == "m") {
      if (!settingChanging) {
        txt = ">" + padWithLeadingZeros(settingLogCountdownMM, 2);
      } else {
        txt = ">" + padWithLeadingZeros(settingChangingValue, 2);
      }
    } else {
      if (!settingChanging) {
        txt = ">" + padWithLeadingZeros(settingLogCountdownSS, 2);
      } else {
        txt = ">" + padWithLeadingZeros(settingChangingValue, 2);
      }
    }
    tft.drawString(txt, settingsXcursor, settingsYcursor, 2);
    drawCursor(settingsXcursor, settingsYcursor);
  } else if (name.startsWith("Sensor shutdown")) {
    if (!settingChanging) {
      txt = ">" + settingShutdown;
    } else {
      txt = ">" + settingChangingText;
    }
    tft.fillRect(settingsXcursor, settingsYcursor, 3 * settingsFontWidth, settingsFontHeight, TFT_BLACK); // erasing previous text
    tft.drawString(txt, settingsXcursor, settingsYcursor, 2);
    drawCursor(settingsXcursor, settingsYcursor);
  } else if (name.startsWith("Sensor power")) {
    if (!settingChanging) {
      txt = ">" + padWithLeadingZeros(settingPowerUpTime, 3);
    } else {
      txt = ">" + padWithLeadingZeros(settingChangingValue, 3);
    }
    tft.fillRect(settingsXcursor, settingsYcursor, 5 * settingsFontWidth, settingsFontHeight, TFT_BLACK); // erasing previous text
    tft.drawString(txt, settingsXcursor, settingsYcursor, 2);
    drawCursor(settingsXcursor, settingsYcursor);
  } else if (name.startsWith("LCD brightness")) {
    if (!settingChanging) {
      txt = ">" + String(tftBrightness);
    } else {
      txt = ">" + String(settingChangingValue);
    }
    tft.fillRect(settingsXcursor, settingsYcursor, 4 * settingsFontWidth, settingsFontHeight, TFT_BLACK); // erasing previous text
    tft.drawString(txt, settingsXcursor, settingsYcursor, 2);
    drawCursor(settingsXcursor, settingsYcursor);
  }
}

void drawCursor(int x, int y) { // drawing a filled triangle to represent the cursor
  int x0 = x + 0;
  int x1 = x + 5;
  int y0 = y + 2;
  int y1 = y + 12;
  int y2 = y + 7;
  tft.fillTriangle(x0, y0, x0, y1, x1, y2, COLOR_CURSOR);
}

void settingPressEvent() {  // Manages user's joystick press event when in the Settings screen
  if (DEBUG_MODE) {
    Serial.println("Sub settingPressEvent");
  }
  if (!loggingOn) { // can't change settings while logging
    String name = settingsText[getSettingYIndexFromCursor()];
    name = name.substring(0, name.indexOf(":") + 1);
    if (!settingChanging) { // Setting was not being modified. Set up the editing variable and switch to being changed status
      if (name.startsWith("Screen sample")) {
        settingChangingValue = screenSampleInterval;
      } else if (name.startsWith("SD log sample interval")) {
        settingChangingValue = logSampleInterval;
      } else if (name.startsWith("SD log duration")) {
        settingChangingText = settingLogDuration;
      } else if (name.startsWith("SD log countdown")) {
        String field = getSettingXFieldIndex(name);
        if (field == "h") {
          settingChangingValue = settingLogCountdownHH;
        } else if (field == "m") {
          settingChangingValue = settingLogCountdownMM;
        } else {
          settingChangingValue = settingLogCountdownSS;
        }
      } else if (name.startsWith("Sensor shutdown")) {
        settingChangingText = settingShutdown;
      } else if (name.startsWith("Sensor power")) {
        settingChangingValue = settingPowerUpTime;
      } else if (name.startsWith("LCD brightness")) {
        settingChangingValue = tftBrightness;
      }
      settingChanging = true;
    } else { // setting was being modified. Record the new value and switch to not changing status
      if (name.startsWith("Screen sample")) {
        screenSampleInterval = settingChangingValue;
      } else if (name.startsWith("SD log sample interval")) {
        logSampleInterval = settingChangingValue;
      } else if (name.startsWith("SD log duration")) {
        settingLogDuration = settingChangingText;
      } else if (name.startsWith("SD log countdown")) {
        String field = getSettingXFieldIndex(name);
        if (field == "h") {
          settingLogCountdownHH = settingChangingValue;
        } else if (field == "m") {
          settingLogCountdownMM = settingChangingValue;
        } else {
          settingLogCountdownSS = settingChangingValue;
        }
      } else if (name.startsWith("Sensor shutdown")) {
        settingShutdown = settingChangingText;
      } else if (name.startsWith("Sensor power")) {
        settingPowerUpTime = settingChangingValue;
      } else if (name.startsWith("LCD brightness")) {
        tftBrightness = settingChangingValue;
        tftBackLight.setBrightness(tftBrightness);
      }
      settingChanging = false;
    }
    displaySingleSetting(); // update display with the new state of the setting
  }
}

int getSettingYIndexFromCursor() { // index to the setting array member the screen cursor is pointing at
  return (settingsYcursor - settingsYstart) / settingsYspacing;
}

int getNextUserSetting(int line, String direction) {
  int nextLine = -1;
  if (direction == "down") {
    for (int i = line + 1; i < NB_SETTING_LINES; i++) { // looking for a setting that is user modifiable
      if (settingsText[i].indexOf("<") != -1) {
        nextLine = i;
        break;
      }
    }
  } else {
    for (int i = line - 1; i > 0; i--) { // looking for a setting that is user modifiable
      if (settingsText[i].indexOf("<") != -1) {
        nextLine = i;
        break;
      }
    }
  }
  if (nextLine == -1) {
    nextLine = line;
  }
  return nextLine;
}

void settingJoystickEvent() {  // Manages user's joystick event when in the Settings screen
  if (DEBUG_MODE) {
    Serial.println("Sub settingJoystickEvent");
  }
  if (!loggingOn) { // can't change settings while logging
    tft.setFreeFont(FONT_SETTINGS); // todo might be redundant
    int w;
    int line = getSettingYIndexFromCursor(); 
    int nextLine;
    String name = settingsText[line];
    name = name.substring(0, name.indexOf(":") + 1);
    String txt;
    if (!settingChanging) { // Setting was not being modified, so navigate among settings
      tft.setTextColor(COLOR_SETTING_USER);
      if (joystickState == "down") {
        nextLine = getNextUserSetting(line, "down");
        if (nextLine != line) {
          tft.fillRect(settingsXcursor, settingsYcursor, settingsFontWidth, settingsFontHeight - 1, TFT_BLACK); // erase current '>' cursor
          settingsYcursor += settingsYspacing * (nextLine - line);
          txt = settingsText[nextLine];
          settingsXcursor = settingsXstart + tft.textWidth(txt.substring(0, txt.indexOf(":") + 1), 2);
        }
      } else if (joystickState == "up") {
        nextLine = getNextUserSetting(line, "up");
        if (nextLine != line) {
          tft.fillRect(settingsXcursor, settingsYcursor, settingsFontWidth, settingsFontHeight - 1, TFT_BLACK); // erase current '>' cursor
          settingsYcursor -= settingsYspacing * (line - nextLine);
          txt = settingsText[nextLine];
          settingsXcursor = settingsXstart + tft.textWidth(txt.substring(0, txt.indexOf(":") + 1), 2);
        }
      } else if (joystickState == "right") { // navigate through the HH, MM, SS fields
        if (name.startsWith("SD log countdown")) {
          int x3 = tft.textWidth(name + " 00 00", 2) + settingsXstart; // cursor position for SS
          if (settingsXcursor != x3) { // shift cursor to MM or SS fields
            tft.fillRect(settingsXcursor, settingsYcursor, settingsFontWidth, settingsFontHeight - 1, TFT_BLACK); // erase current '>' cursor
            settingsXcursor += tft.textWidth(" 00", 2);
          }
        }
      } else if (joystickState == "left") {
        if (name.startsWith("SD log countdown")) {
          int x1 = tft.textWidth(name, 2) + settingsXstart; // cursor position for HH
          if (settingsXcursor != x1) { // shift cursor to MM or SS fields
            tft.fillRect(settingsXcursor, settingsYcursor, settingsFontWidth, settingsFontHeight - 1, TFT_BLACK); // erase current '>' cursor
            settingsXcursor -= tft.textWidth(" 00", 2);
          }
        }
      }
    } else {
      if (joystickState == "down" | joystickState == "up") {
        if (name.startsWith("Screen sample")) {
          int direction = -1; // decrement value if joystick is down
          if (joystickState == "up") { direction = 1; }
          settingChangingValue += direction;
          if (settingChangingValue < 1) {
            settingChangingValue = 99;
          } else if (settingChangingValue > 99) {
            settingChangingValue = 1;
          }
        } else if (name.startsWith("SD log sample interval")) {
          int direction = -1; // decrement value if joystick is down
          if (joystickState == "up") { direction = 1; }
          settingChangingValue += direction;
          if (settingChangingValue < 1) {
            settingChangingValue = 999;
          } else if (settingChangingValue > 999) {
            settingChangingValue = 1;
          }
        } else if (name.startsWith("SD log duration")) {
          if (settingChangingText == "countdown") {
            settingChangingText = "manual stop";
          } else {
            settingChangingText = "countdown";
          }
        } else if (name.startsWith("SD log countdown")) {
          int direction = -1; // decrement value if joystick is down
          if (joystickState == "up") { direction = 1; }
          settingChangingValue += direction;
          String field = getSettingXFieldIndex(name);
          if (field == "h") {
            if (settingChangingValue < 0) {
              settingChangingValue = 99;
            } else if (settingChangingValue > 99) {
              settingChangingValue = 0;
            }
          } else {
            if (settingChangingValue < 0) {
              settingChangingValue = 59;
            } else if (settingChangingValue > 59) {
              settingChangingValue = 0;
            }
          }
        } else if (name.startsWith("Sensor shutdown")) {
          if (settingChangingText == "Y") {
            settingChangingText = "N";
          } else {
            settingChangingText = "Y";
          }
        } else if (name.startsWith("Sensor power")) {
          int direction = -1; // decrement value if joystick is down
          if (joystickState == "up") { direction = 1; }
          settingChangingValue += direction;
          if (settingChangingValue < 1) {
            settingChangingValue = 999;
          } else if (settingChangingValue > 999) {
            settingChangingValue = 1;
          }
        } else if (name.startsWith("LCD brightness")) {
          int direction = -1; // decrement value if joystick is down
          if (joystickState == "up") { direction = 1; }
          settingChangingValue += direction;
          if (settingChangingValue < 1) {
            settingChangingValue = 1;
          } else if (settingChangingValue > 100) {
            settingChangingValue = 100;
          }
        }
      } else if (joystickState == "right" | joystickState == "left") {
        if (name.startsWith("Screen sample")) {
          int direction = -10; // decrement value if joystick is down
          if (joystickState == "right") { direction = 10; }
          settingChangingValue += direction;
          if (settingChangingValue < 1) {
            settingChangingValue = 99;
          } else if (settingChangingValue > 99) {
            settingChangingValue = 1;
          }
        } else if (name.startsWith("SD log sample interval")) {
          int direction = -30; // decrement value if joystick is down
          if (joystickState == "right") { direction = 30; }
          settingChangingValue += direction;
          if (settingChangingValue < 1) {
            settingChangingValue = 999;
          } else if (settingChangingValue > 999) {
            settingChangingValue = 1;
          }
        // } else if (name.startsWith("SD log duration")) {
        } else if (name.startsWith("SD log countdown")) {
          int direction = -10; // decrement value if joystick is down
          if (joystickState == "right") { direction = 10; }
          settingChangingValue += direction;
          String field = getSettingXFieldIndex(name);
          if (field == "h") {
            if (settingChangingValue < 0) {
              settingChangingValue = 99;
            } else if (settingChangingValue > 99) {
              settingChangingValue = 0;
            }
          } else {
            if (settingChangingValue < 0) {
              settingChangingValue = 59;
            } else if (settingChangingValue > 59) {
              settingChangingValue = 0;
            }
          }
        // } else if (name.startsWith("Sensor shutdown")) {
        } else if (name.startsWith("Sensor power")) {
          int direction = -30; // decrement value if joystick is down
          if (joystickState == "right") { direction = 30; }
          settingChangingValue += direction;
          if (settingChangingValue < 1) {
            settingChangingValue = 999;
          } else if (settingChangingValue > 999) {
            settingChangingValue = 1;
          }
        } else if (name.startsWith("LCD brightness")) {
          int direction = -10; // decrement value if joystick is down
          if (joystickState == "right") { direction = 10; }
          settingChangingValue += direction;
          if (settingChangingValue < 1) {
            settingChangingValue = 1;
          } else if (settingChangingValue > 100) {
            settingChangingValue = 100;
          }
        }
      }
      // displaySetting();
    }
    displaySingleSetting();
  }
}

void screenSdCardSettings() { // display SD card information and settings
  tft.fillScreen(TFT_BLACK);
  String txt = "";
  String txt2 = "";
  String sdStatus, sdValue;
  int posChar;
  int y = settingsYstart;
  int xy;
  auto color2 = COLOR_SETTING_TEXT;
  for (int i = 0; i < NB_SD_LINES; i++) {
    txt = sdText[i] + "  ";
    txt2 = "";
    color2 = COLOR_SETTING_TEXT;
    if (txt.startsWith("SD card status")) {
      if (!sdPresent) {
        txt2 = "No Card";
        color2 = COLOR_SD_ABSENT_TEXT;
        i = NB_SD_LINES; // complete current For loop but exit after that
      } else if (sdPresent & sdEjected) {
        txt2 = "Present but Ejected";
        color2 = COLOR_SD_ABSENT_TEXT;
        i = NB_SD_LINES; // complete current For loop but exit after that
      } else {
        txt2 = "Ready";
      }
    } else if (sdPresent & !sdEjected) {
      if (txt.startsWith("SD card eject")) {
        txt2 = "NOW";
        color2 = COLOR_SETTING_USER;
      } else if (txt.startsWith("SD card Used")) {
        getSdCardData("usedbytes", sdValue, sdStatus);
        if (sdStatus == "ok") {
          txt2 = sdValue + " / ";
          getSdCardData("totalbytes", sdValue, sdStatus);
          if (sdStatus == "ok"){
            txt2 += sdValue;
          } else {
            txt2 += sdStatus + "!!!"; // sd card error
            color2 = TFT_RED;
          }
        }
      } else if (txt.startsWith("SD card size")) {
        getSdCardData("cardsize", sdValue, sdStatus);
        if (sdStatus == "ok") {
          txt2 = sdValue;
        } else {
          txt2 = sdStatus + "!!!"; // sd card error
          color2 = TFT_RED;
        }
      } else if (txt.startsWith("Files list")) {
        posChar = txt.indexOf(":");
        txt2 = txt.substring(posChar + 1, txt.length());
      }
    }
    posChar = txt.indexOf(":");
    txt = txt.substring(0, posChar + 1);
    if (sdYcursor == 0) { // initialize user cursor
      if (txt.startsWith("SD card eject")) {    // 1st setting that is user modifyable
        sdYcursor = y;
        sdXcursor = settingsXstart + tft.textWidth(txt, 2) + 1;
      }
    }
    if (!sdEjected) {
      tft.setTextColor(COLOR_SETTING_TEXT);
    } else {
      tft.setTextColor(COLOR_SD_ABSENT_TEXT);
    }
    tft.drawString(txt, settingsXstart, y, 2);
    int x = tft.textWidth(txt, 2);
    if (!sdEjected) {
      tft.setTextColor(color2);
    } else {
      tft.setTextColor(COLOR_SD_ABSENT_TEXT);
    }
    tft.drawString(" " + txt2, settingsXstart + x, y, 2);
    if (!loggingOn && !bannerActive && !sdEjected) { // settings not active while logging
      if (sdYcursor == y) {
        drawCursor(settingsXstart + x, y);
      }
    }
    y += settingsYspacing;
  }
  drawBanner();
}

void sdcardJoystickEvent() {
}

void sdCardPressEvent() {  // Manages user's joystick press event when in the SD Card screen
  if (DEBUG_MODE) {
    Serial.println("Sub sdCardPressEvent");
  }
  if (!sdEjected && !loggingOn) { // settings not active while logging
    // if (loggingOn) {
    //   loggingStop();
    // }
    devSD.end();
    sdEjected = true;
    screenSdCardSettings();
  }
}

void getSdCardData(String name, String &value, String &status) {
  if (sdPresent) {
    devSD.end();
    if (!devSD.begin(SDCARD_SS_PIN,SDCARD_SPI,4000000UL)) {
      status = "Card Mount Failed";
    } else {
      uint8_t cardType = devSD.cardType();
      if (cardType == CARD_NONE) {
        status = "No SD card attached";
      } else {
        status = "ok";
        if (name == "cardsize") {
          uint32_t cardSize = devSD.cardSize(); // / (1024 * 1024);
          value = String(cardSize);
        } else if (name == "totalbytes") {
          uint32_t totalBytes = devSD.totalBytes();
          value = String(totalBytes);
        } else if (name == "usedbytes") {
          uint32_t usedBytes = devSD.usedBytes();
          value = String(usedBytes);
        } else {
          status = "Unrecognized request";
        }
      }
    }
  } else {
    status = "No SD card";
  }
}

bool sdCardMount() {
  if (sdEjected) {
    Serial.println("SD Card absent");
    return false;
  } else {
    Serial.println("SD Card present");
    devSD.end();
    if (!devSD.begin(SDCARD_SS_PIN,SDCARD_SPI,4000000UL)) {
      Serial.println("Card mount Failed");
      return false;
    } else {
      uint8_t cardType = devSD.cardType();
      if (cardType == CARD_NONE) {
        Serial.println("No SD card attached, or type not recognized");
        return false;
      } else {
        Serial.println("Card mount succesful");
        return true;
      }
    }
  }
}

void sdCardCreateLogFile() {  // Create the datalogging file
  String name;
  bool success;
  if (sdCardMount()) { // SD card mount was succesful
    do { // create a unique file name /datalog_YYMMDDHHMMSS.csv
      rtcNow = rtc.now();
      name = trimPadToXchars(rtcNow.year(), 2);
      name += trimPadToXchars(rtcNow.month(), 2);
      name += trimPadToXchars(rtcNow.day(), 2);
      name += trimPadToXchars(rtcNow.hour(), 2);
      name += trimPadToXchars(rtcNow.minute(), 2);
      name += trimPadToXchars(rtcNow.second(), 2);
      name = "/datalog_" + name + ".csv";
      success = !devSD.exists(name);
      if (!success && !sdEjected) {
        delay(500);
      }
    } while (!success && !sdEjected);

    if (!sdEjected) {
      logFileName = name;
      logFile = devSD.open(logFileName, FILE_APPEND);
      if (devSD.exists(logFileName)) {
        Serial.print("Created log file ");
        Serial.println(logFileName);
      } else {
        Serial.print("Failed to create log file ");
        Serial.println(logFileName);
        logFileName = "";
      }
      logFile.close();
    } else {
      Serial.println("SD card not present. Failed to create log file");
      logFileName = ""; // SD card no longer present. Abort
    }
  } else { // SD card mount was not succesful
    logFileName = "";
  }
}

String trimPadToXchars(int val, int nbDigits) {
  String pad = "";
  String valTxt = String(val);
  int n = nbDigits - valTxt.length();
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      pad += "0";
    }
    pad += valTxt;
    return pad;
  } else if (n <0) {
    return valTxt.substring(valTxt.length() - nbDigits);
  } else {
    return valTxt;
  }
}

void loggingStart() {  // User requested datalogging to start. Create the information header and column titles in the CSV datalog file
  if (!sdEjected) {
    sdCardCreateLogFile();
    if ((logFileName != "") && !sdEjected) { // file creation was succesful
      loggingOn = true;
      millisLogStart = millis();
      if (settingLogDuration == "countdown") {
        millisLogCountdownEnd = millisLogStart + (settingLogCountdownHH * 3600 + settingLogCountdownMM * 60 + settingLogCountdownSS) * 1000;
      }
      nbLogRecords = 0;
      if (!sdEjected) {
        logFile = devSD.open(logFileName, FILE_APPEND);
        if (logFile) {
          rtcSample = rtc.now();
          String txt = rtcSample.timestamp(DateTime::TIMESTAMP_DATE);
          logFile.print("Created,"); logFile.print(txt);
          txt = rtcSample.timestamp(DateTime::TIMESTAMP_TIME);
          logFile.println("," + txt);
          logFile.print("WIO name,"); logFile.println(WIO_NAME);
          logFile.print("WIO firmware,"); logFile.println(WIO_FIRMWARE);
          logFile.print("Log interval (s),"); logFile.println(logSampleInterval);
          logFile.print("Time since turn-on,"); logFile.println(timeSinceTurnOn());
          logFile.print("Sensor name,,Battery,,,,");
          txt = "";
          for (int i=0; i<NB_SEN55; i++) {
            if (!sen55SetupError[i]) {
              if (i == 0) {
                txt += sen55Name0 + ",,,,,,,,";
              } else {
                txt += sen55Name1 + ",,,,,,,,";
              }
            }
          }
          if (!multigasSetupError) {
            txt += multigasName + ",,,,";
          }
          if (!scd40SetupError) {
            txt += scd40Name + ",,";
          }
          logFile.println(txt);
          logFile.print("Time,Sample time (s),SOC,Capacity,Voltage,Current,"); 
          for (int i=0; i<NB_SEN55; i++) {
            if (!sen55SetupError[i]) {
              logFile.print("PM1,"); logFile.print("PM2.5,"); logFile.print("PM4,"); logFile.print("PM10,");
              logFile.print("VOC,"); logFile.print("NO,"); logFile.print("Temp,"); logFile.print("RelHumi,");
            }
          }
          if (!multigasSetupError) {
            logFile.print("VOC,"); logFile.print("CO,"); logFile.print("NO2,"); logFile.print("C2H5CH,");
          }
          if (!scd40SetupError) {
            logFile.print("CO2,"); logFile.print("Temp,"); logFile.println("RelHumi");
          }
          logFile.close(); // closing to ensure file integrity even if run is interrupted / crashes
        } else {
          loggingOn = false;
          Serial.println("Error data log. File open failed");
        }
      } else {
        loggingOn = false;
        Serial.println("Error data log. SD card ejected");
      }
      if (DEBUG_MODE) {
        Serial.println("Logging started");
      }
    } else {
      loggingOn = false;
    }
  }
  drawBanner();
}

void loggingStop() {  // End of datalogging session. Update screen accordingly
  loggingOn = false;
  logFileName = "";
  logFile.close();
  if (DEBUG_MODE) {
    Serial.println("Logging stopped");
  }
  if (screenActive == "settings") {
    screenMainSettings();
  } else if (screenActive == "sdcard1") {
    screenSdCardSettings();
  } else {
    drawBanner();
  }
}

void logSensorData() {  // Save a new set of measurements
  String txt = rtcSample.timestamp(DateTime::TIMESTAMP_TIME) + ",";
  unsigned long time = (millis() - millisLogStart) / 1000;
  txt += String(time) + "," + String(batterySOC) + "," + String(batteryCapacityLeft) + "," + String(batteryVoltage) + "," + String(batteryCurrent) + ",";
  for (int i=0; i<NB_SEN55; i++) {
    if (!sen55SetupError[i]) {
      txt += String(sen55Pm1[i]) + "," + String(sen55Pm2p5[i]) + "," + String(sen55Pm4[i]) + "," + String(sen55Pm10[i]) + "," + String(sen55Voc[i]) + "," + String(sen55Nox[i]) + "," + String(sen55Temp[i]) + "," + String(sen55RelHumi[i]) + ",";
    }
  }
  if (!multigasSetupError) {
    txt += String(mgVoc) + "," + String(mgCo) + "," + String(mgNo2) + "," + String(mgC2h5ch) + ",";
  }
  if (!scd40SetupError) {
    txt += String(scd40CO2) + "," + String(scd40Temp) + "," + String(scd40RelHumi);
  }
  if (DEBUG_MODE) {
    Serial.println(txt);
  }
  if (!sdEjected) {
    logFile = devSD.open(logFileName, FILE_APPEND);
    if (logFile) {
      logFile.println(txt);
      logFile.close(); // closing btwn samples to ensure file integrity even if run is interrupted or crashes
    } else {
      loggingOn = false;
      Serial.println("Error data log. File open failed");
    }
  } else {
    loggingOn = false;
    Serial.println("Error data log. SD card ejected");
  }
}

void convertMillisHMS(unsigned long millisec, int &h, int &m, int &s) {
  millisec /= 1000;
  h = millisec / 3600;
  millisec -= h * 3600;
  m = millisec / 60;
  millisec -= m * 60;
  s = millisec;
}

void keyEvent() {
  if (DEBUG_MODE) {
    Serial.print("SUB nextScreen --- Key event:");
    Serial.println(joystickState);
  }
  if (joystickState == "up") {
    if (!bannerActive) {
      if (screenActive == "settings") {
        settingJoystickEvent();
      } else if (screenActive == "sdcard1" || screenActive == "sdcard2") {
        sdcardJoystickEvent();
      }
    }
  } else if (joystickState == "down") {
    if (!bannerActive) {
      if (screenActive == "settings") {
        settingJoystickEvent();
      } else if (screenActive == "sdcard1" || screenActive == "sdcard2") {
        sdcardJoystickEvent();
      }
    }
  } else if (joystickState == "left") {
    if (!bannerActive) {
      if (screenActive == "values1") {
        screenValues2();
      } else if (screenActive == "values2") {
        screenValues1();
      } else if (screenActive == "charts1") {
        screenCharts5();
      } else if (screenActive == "charts2") {
        screenCharts1();
      } else if (screenActive == "charts3") {
        screenCharts2();
      } else if (screenActive == "charts4") {
        screenCharts3();
      } else if (screenActive == "charts5") {
        screenCharts4();
      }else if (screenActive == "settings") {
        settingJoystickEvent();
      }else if (screenActive == "sdcard1" || screenActive == "sdcard2") {
        sdcardJoystickEvent();
      }
    } else {
      screenBanner();
    }
  } else if (joystickState == "right") {
    if (!bannerActive) {
      if (screenActive == "values1") {
        screenValues2();
      } else if (screenActive == "values2") {
        screenValues1();
      } else if (screenActive == "charts1") {
        screenCharts2();
      } else if (screenActive == "charts2") {
        screenCharts3();
      } else if (screenActive == "charts3") {
        screenCharts4();
      } else if (screenActive == "charts4") {
        screenCharts5();
      } else if (screenActive == "charts5") {
        screenCharts1();
      } else if (screenActive == "settings") {
        settingJoystickEvent();
      } else if (screenActive == "sdcard1" || screenActive == "sdcard2") {
        sdcardJoystickEvent();
      }
    } else {
      screenBanner();
    }
  } else if (joystickState == "press") {
    if (!bannerActive) {
      if (screenActive == "settings") {
        settingPressEvent();
        if (!settingChanging) {
          drawBanner();
        }
      } else if (screenActive == "sdcard1" || screenActive == "sdcard2") {
        sdCardPressEvent();
        if (!sdcardChanging) {
          drawBanner();
        }
      }
    } else {
      screenBanner();
    }
  }
}

void updateScreen() {
  if (screenActive == "values1") {
    screenValues1();
  } else if (screenActive == "values2") {
    screenValues2();
  } else if (screenActive == "charts1") {
    screenCharts1();
  } else if (screenActive == "charts2") {
    screenCharts2();
  } else if (screenActive == "charts3") {
    screenCharts3();
  } else if (screenActive == "charts4") {
    screenCharts4();
  } else if (screenActive == "charts5") {
    screenCharts5();
  } else if (screenActive == "settings") {
    if (!settingChanging) {
      screenMainSettings();
    } else {
      // displaySetting();
      drawBanner();
    }
    // drawBanner();
  } else if (screenActive == "sdcard1") {
    screenSdCardSettings();
    // drawBanner();
  } else if (screenActive == "battery") {
    screenBattery();
  }
}

void setupDisplay() {
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_DARKERGREY);
  tftBackLight.initialize();
  tftBackLight.setBrightness(tftBrightness);
  tft.setFreeFont(FONT_SETTINGS);
  tft.setTextColor(TFT_GREY);
  String txt = " ";
  settingsFontWidth = tft.textWidth(txt, 2);
  settingsFontHeight = 18;
}

bool setupBattery () {
  bool battInit = lipo.begin();
  if (battInit) {
    unsigned int fullCapacity = lipo.capacity(FULL);
    if (fullCapacity != BATTERY_CAPACITY) {
      lipo.setCapacity(BATTERY_CAPACITY);
    }
  }
  return battInit;
}

void setup() {
  bool warning = false;
  millisAtPowerOn = millis();
  Serial.begin(115200);
  int loops = 0;
  while (!Serial) { // Short wait loop. Sometimes serial over USB takes a while to initialize
    delay(1000);
    loops +=1;
    if (loops > 5) break;
  }
  serialPresentAtBoot = Serial;

  Wire.begin();                         // WIO's SCL1-SDA1 I2C bus
  Wire1.begin();                        // WIO's SCL0-SDA0 I2C bus

  Serial.println("Starting Setup");

  Serial.println("Init Screen");
  setupDisplay();

  int y = 2;
  int dy = 18;
  int delayWarning = 1000;
  String txt;

  if (serialPresentAtBoot) {
    tft.drawString("Serial port present", 10, y, 2);    
  } else {
    tft.drawString("Serial port NOT present", 10, y, 2);    
  }

  // initializing the Battery
  txt = "Init Battery Capacity";
  y += dy;
  tft.drawString(txt, 10, y, 2);
  Serial.println(txt);
  if (!setupBattery()) {
    txt = "  FAILED initialization. Battery not responding";
    tft.setTextColor(TFT_ORANGE);
    y += dy;
    tft.drawString(txt, 10, y, 2);
    tft.setTextColor(TFT_GREY);
    Serial.println(txt);
    warning = true;
    delay(delayWarning);
  }

  // Initializing the RTC. Depends of which RTC is being used: internal or external
  txt = "Init RTC (";
  #ifdef INTERNAL_RTC
    txt += "internal)";
  #else
    txt += "external Adafruit PCF8523)";
  #endif
  Serial.println(txt);
  y += dy;
  tft.drawString(txt, 10, y, 2);
  rtc.begin();
  if (!rtc.begin()) {
    txt = "ERROR: couldn't find the RTC";
    tft.setTextColor(TFT_RED);
    y += dy;
    tft.drawString(txt, 10, y, 2);
    Serial.println(txt);
    //Serial.flush();
    while (1) delay(1000);        // stop the execution
  }
  #ifdef INTERNAL_RTC
  #else
    // if (! rtc.initialized() || rtc.lostPower()) {
    //   Serial.println("RTC is NOT initialized. Setting the time");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));   // sets the RTC to the date & time this sketch was compiled
      //Example setting a January 21, 2014 at 3am date & time: rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    // }
    // When the RTC was stopped and stays connected to the battery, it has
    // to be restarted by clearing the STOP bit. Let's do this to ensure the RTC is running.
    rtc.start();
  #endif

  pinMode(A0, INPUT); // Analog input pin. For code dev, to generate random values
  
  // WIO's 5 way switch (joystick)
  pinMode(WIO_5S_UP, INPUT_PULLUP);
  pinMode(WIO_5S_DOWN, INPUT_PULLUP);
  pinMode(WIO_5S_LEFT, INPUT_PULLUP);
  pinMode(WIO_5S_RIGHT, INPUT_PULLUP);
  pinMode(WIO_5S_PRESS, INPUT_PULLUP);
  // WIO's 3 top buttons
  pinMode(WIO_KEY_A, INPUT_PULLUP);
  pinMode(WIO_KEY_B, INPUT_PULLUP);
  pinMode(WIO_KEY_C, INPUT_PULLUP);

  // SD card detect pin
  pinMode(SDCARD_DET_PIN, INPUT_PULLUP);

  // initializing the Sensirion Sen55 sensors
  for (int i=0; i<NB_SEN55; i++) {
    txt = "Init Sensirion Sen55 sensor [" + String(i) + "]";
    y += dy;
    tft.drawString(txt, 10, y, 2);
    Serial.println(txt);
    sen55Setup(i);
    if (!sen55SetupError[i]) {
      nbSen55ToDisplay++; 
    } else {
      txt = "  FAILED initialization. Disabling sensor";
      tft.setTextColor(TFT_ORANGE);
      y += dy;
      tft.drawString(txt, 10, y, 2);
      tft.setTextColor(TFT_GREY);
      Serial.println(txt);
      warning = true;
      delay(delayWarning);
    }
  }
  if (nbSen55ToDisplay == 0 & FAKE_SENSOR_DATA == true) {
    nbSen55ToDisplay = 2;           // DEBUG, display fake sensor data if no sensor detected and fakesensor data is requested
    sen55SetupError[0] = false;     // DEBUG, using fake values
    sen55SetupError[1] = false;     // DEBUG, using fake values
  }

  // initializing Seeed Multigas sensor
  txt = "Init Seeed Multigas sensor";
  y += dy;
  tft.drawString(txt, 10, y, 2);
  Serial.println(txt);
  gas.begin(Wire, 0x08);            // seeed multigas sensor address
  multigasSetupError = false;       // DEBUG, how can presence / working be determined ? BEGIN is void for this sensor. Try to remove sensor and see if telltale signs in reasdings for absence (note: needs preheating time for some values)

  scd40Setup();
  txt = "Init Sensirion SCD40 sensor";
  tft.setTextColor(TFT_GREY);
  y += dy;
  tft.drawString(txt, 10, y, 2);
  if (scd40SetupError) {
    txt = "  FAILED initialization. Disabling sensor";
    tft.setTextColor(TFT_ORANGE);
    y += dy;
    tft.drawString(txt, 10, y, 2);
    warning = true;
    delay(delayWarning);
  }
  Serial.println(txt);
  if (FAKE_SENSOR_DATA) {
    scd40SetupError = false;      // DEBUG, using fake values
  }

  millisSinceSensorsInit = millis();

  millisOldJoystick = millis();

  if (!warning) {
    txt = "Setup DONE. Happy air monitoring to all !";
    tft.setTextColor(TFT_GREEN);
    y += dy;
    tft.drawString(txt, 10, y, 2);
    Serial.println(txt);
    delay(1000);
  } else {
    txt = "  <to proceed press joystick>";
    tft.setTextColor(TFT_ORANGE);
    tft.setTextDatum(TC_DATUM);
    y += 1.5 * dy;
    tft.drawString(txt, 160, y, 2);
    while (digitalRead(WIO_5S_PRESS) != LOW) {
      delay(100);
    }
  }
}

void loop() {
  millisNow = millis();
  // if (millisNow - millisOldSample < 0) {  // TODO millis() rolls over every ~50 days
  //   millisOldLog = 0;                     // Just resetting old values without carrying over the time past so far
  //   millisOldSample = 0;                  // Will create jitter on that sample, but c'est la vie. Not been tested though
  // }

  if (!digitalRead(SDCARD_DET_PIN)) { // SD card is present
    if (!sdPresent) { // Debouncing if not done already
      if (!sdSwitch) {
        millisSD = millisNow; // start debouncing, only on insertion
        sdSwitch = true;
      } else if (millisNow > millisSD + debounceSD) {
        sdPresent = true;
        sdEjected = false;
        if (screenActive == "sdcard1" || screenActive == "sdcard2") {
          screenSdCardSettings();
        }
        drawBanner();
      }
    }
  } else { // no debouncing on removal
    sdSwitch = false;
    if (sdPresent) {
      sdPresent = false;
      sdEjected = true;
      loggingOn = false;
      logFileName = "";
      if (screenActive == "sdcard1" || screenActive == "sdcard2") {
        screenSdCardSettings();
      }
      drawBanner();
    }
  }

  if (((millisNow - millisOldLog >= logSampleInterval * 1000) && loggingOn) || (millisNow - millisOldSample >= screenSampleInterval *1000)) {
    flashBanner("start");               // visual clue that sampling is in progress and system can't respond to user actions
    if (loggingOn) {
      rtcSample = rtc.now();            // Timestamp for the samples in case of logging
    }
    for (int i=0; i<NB_SEN55; i++) {
      if (!sen55SetupError[i]) sen55Measure(i);
    }
    if (!multigasSetupError) multigasMeasure();     // get multigas values
    if (!scd40SetupError) scd40Measure();           // get scd40 values
    batteryMeasure();
    if ((millisNow - millisOldLog >= logSampleInterval * 1000) && loggingOn) { // logging data to SD card
      if (millisNow - millisSinceSensorsInit >= settingPowerUpTime) {
        sensorsReady = true;
      }
      if (loggingOn && sensorsReady) {  // waiting past sensor Power Up Time before logging data
        logSensorData();
      }
      nbLogRecords++;
      millisOldLog = millisNow;
    }
    nbSamples++;
    // Now that sampling's time critical work is done, push sensor data to chart series queues
    for (int i=0; i<NB_SEN55; i++) {
      if (!sen55SetupError[i]) sen55ChartAppendData(i);
    }
    if (!multigasSetupError) multigasChartAppendData();
    if (!scd40SetupError) scd40ChartAppendData();
    updateScreen();                     // end of visual clue and update the main screen w new data if need be
    millisOldSample = millisNow;        // resetting sample interval even if not expired as in that case a log sample was just acquired and displayed
    millisOldScreen = millis();         // resetting screen refresh interval as update just occured
  } else if (millisNow - millisOldScreen > bannerRefreshInterval) {   // refresh banner if need be. Not time critical, unlike sensor sampling
    if (screenActive == "settings" && !settingChanging) {             // no refresh while a setting is being edited
      screenMainSettings();             // Refresh the Settings screen
    } else {        // if (loggingOn) {
      drawBanner();                     // Only refresh the banner (for time, battery, log countdown, etc)
    }
    millisOldScreen = millisNow;
  }

  if (loggingOn) {
    if ((settingLogDuration == "countdown") && (millisNow > millisLogCountdownEnd)) { // end of log countdown. Stop logging data. Not time critical
      loggingStop();
    }
  }

  // 5 way switch (joystick) event detection
  if (digitalRead(WIO_5S_LEFT) == LOW) {
    if (joystickState == "") {
      joystickState = "left";  }
  } else if (digitalRead(WIO_5S_RIGHT) == LOW) {
    if (joystickState == "") {
        joystickState = "right"; }
  } else if (digitalRead(WIO_5S_PRESS) == LOW) {
    if (joystickState == "") {
      joystickState = "press";  }
  } else if (digitalRead(WIO_5S_UP) == LOW) {
    if (joystickState == "") {
      joystickState = "up"; }
  } else if (digitalRead(WIO_5S_DOWN) == LOW) {
    if (joystickState == "") {
      joystickState = "down"; }
  } else {
    joystickState = "";
  }
  if (joystickState != oldJoystickState) {
    millisJoystick = millis(); // TODO logic seems wrong. should reset oldtime whenever different. see sd card logic
    if (millisJoystick > millisOldJoystick + debounceTime) {
      millisOldJoystick = millisJoystick;
      oldJoystickState = joystickState;
      if (joystickState != "") {
        keyEvent();
      }
    } else {
      joystickState = oldJoystickState;
    }
  }

  if (joystickState == "") {      // Giving priority to joystick event detection as Button pushes are less frequent and lower user-repeat rate
    if (digitalRead(WIO_KEY_A) == LOW) { 
      if (buttonState == "") {
        buttonState = "a";
        if (!bannerActive) {
          screenBanner();
        } else {
          displayBannerCursor(false);
        }
      }
    } else if (digitalRead(WIO_KEY_B) == LOW) {
      if (buttonState == "") {
        buttonState = "b";
        screenCharts1();
      }
      displayBannerCursor(false);
    } else if (digitalRead(WIO_KEY_C) == LOW) {
      if (buttonState == "") {
        buttonState = "c";
        screenValues1();
      }
      displayBannerCursor(false);
    } else {
      buttonState = "";
    }
  }
  if (buttonState != oldButtonState) {
    millisButtons = millis();
    if (millisButtons > millisOldButtons + debounceTime) {
      millisOldButtons = millisButtons;
      oldButtonState = buttonState;
    }
  }
}

// tft.color565(255,255,255) = white
// TFT_WHITE 0xFFFF       /* 255, 255, 255 */ 5 bits RED + 6 bits GREEN + 5 bits BLUE
// TFT_LIGHTGREY 0xC618   /* 192, 192, 192 */ 1100 0110 0001 1000 24 48 24
// TFT_DARKGREY 0x7BEF    /* 128, 128, 128 */ 0111 1011 1110 1111 15 31 15
// TFT_BLACK 0x0000       /*   0,   0,   0 */
// TFT_NAVY 0x000F        /*   0,   0, 128 */
// TFT_BLUE 0x001F        /*   0,   0, 255 */ 0000 0000 0001 1111
// TFT_DARKCYAN 0x03EF    /*   0, 128, 128 */
// TFT_CYAN 0x07FF        /*   0, 255, 255 */
// TFT_GREEN 0x07E0       /*   0, 255,   0 */ 0000 0111 1110 0000
// TFT_DARKGREEN 0x03E0   /*   0, 128,   0 */
// TFT_OLIVE 0x7BE0       /* 128, 128,   0 */
// TFT_GREENYELLOW 0xB7E0 /* 180, 255,   0 */
// TFT_YELLOW 0xFFE0      /* 255, 255,   0 */ 1111 1111 1110 0000
// TFT_ORANGE 0xFDA0      /* 255, 180,   0 */ 1111 1100 1010 0000
// TFT_PINK 0xFC9F
// TFT_MAGENTA 0xF81F     /* 255,   0, 255 */ 1111 1000 0001 1111
// TFT_RED 0xF800         /* 255,   0,   0 */ 1111 1000 0000 0000
// TFT_PURPLE 0x780F      /* 128,   0, 128 */
// TFT_MAROON 0x7800      /* 128,   0,   0 */
// TFT_TRANSPARENT 0x0120 / ???????????????????????????

// FreeMono12pt7b.h
// FreeMono18pt7b.h
// FreeMono24pt7b.h
// FreeMono9pt7b.h
// FreeMonoBold12pt7b.h
// FreeMonoBold18pt7b.h
// FreeMonoBold24pt7b.h
// FreeMonoBold9pt7b.h
// FreeMonoBoldOblique12pt7b.h
// FreeMonoBoldOblique18pt7b.h
// FreeMonoBoldOblique24pt7b.h
// FreeMonoBoldOblique9pt7b.h
// FreeMonoOblique12pt7b.h
// FreeMonoOblique18pt7b.h
// FreeMonoOblique24pt7b.h
// FreeMonoOblique9pt7b.h
// FreeSans12pt7b.h
// FreeSans18pt7b.h
// FreeSans24pt7b.h
// FreeSans9pt7b.h
// FreeSansBold12pt7b.h
// FreeSansBold18pt7b.h
// FreeSansBold24pt7b.h
// FreeSansBold9pt7b.h
// FreeSansBoldOblique12pt7b.h
// FreeSansBoldOblique18pt7b.h
// FreeSansBoldOblique24pt7b.h
// FreeSansBoldOblique9pt7b.h
// FreeSansOblique12pt7b.h
// FreeSansOblique18pt7b.h
// FreeSansOblique24pt7b.h
// FreeSansOblique9pt7b.h
// FreeSerif12pt7b.h
// FreeSerif18pt7b.h
// FreeSerif24pt7b.h
// FreeSerif9pt7b.h
// FreeSerifBold12pt7b.h
// FreeSerifBold18pt7b.h
// FreeSerifBold24pt7b.h
// FreeSerifBold9pt7b.h
// FreeSerifBoldItalic12pt7b.h
// FreeSerifBoldItalic18pt7b.h
// FreeSerifBoldItalic24pt7b.h
// FreeSerifBoldItalic9pt7b.h
// FreeSerifItalic12pt7b.h
// FreeSerifItalic18pt7b.h
// FreeSerifItalic24pt7b.h
// FreeSerifItalic9pt7b.h
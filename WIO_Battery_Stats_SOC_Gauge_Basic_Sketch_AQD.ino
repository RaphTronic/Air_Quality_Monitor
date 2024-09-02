/*************************************************************
Basic Arduino sketch for the Seeed Studio WIO Terminal all-in-one device https://wiki.seeedstudio.com/Wio-Terminal-Getting-Started/

Reports battery measurements by the BQ27441 BMS chip: voltage, current, SOC, etc, and displays a graphical SOC gauge.

Version = 1.0    (8/2024)
Written by Raphtronic https://raphtronic.blogspot.com

Usage: . Plug the WIO Terminal into a WIO Battery Pack Chassis
       . Observe the current value: positive (charging), negative (powering the WIO), zero (charged)
       .     while moving the USB-C plug btwn the WIO and the battery pack, and depending on the state of the pack's power button
       . Note how unreliable the SOC & CapacityLeft values are when <3.8V. Often has >30% of actual capacity left but displays SOC=0
       .     Suspecting a number of factors, including BQ27441 & lipo pouch calibration not performed by SEEED (see process in TI datasheet)
       . See blog post for context and setup: [DIY] Air Quality Monitor & Datalogger - Part 2 Implementation
       .      https://raphtronic.blogspot.com/2024/08/diy-air-quality-monitor-datalogger-part.html

Libraries
       . Adafruit Zero DMA Library by Adafruit 1.13. Used by Seeed for the TFT screen. Details here: https://wiki.seeedstudio.com/Wio-Terminal-LCD-Overview/
       . Sparkfun BQ27441 LiPo Fuel Gauge Arduino Library by Sparkfun Electronics 1.1.0  https://github.com/sparkfun/SparkFun_BQ27441_Arduino_Library

Based on "Sparkfun BQ27441 LiPo Fuel Gauge > BQ27441_basic.ino" example sketch (copyright header below)

Modifications:
   . Adapted for the WIO Terminal Arduino system from Seeed Studio
   . Added graphical gauge with low-SOC color warnings
   . Added the Internal Temperature reading

Feel free to copy / leverage this work
*************************************************************/

/******************************************************************************
BQ27441_Basic
BQ27441 Library Basic Example
Jim Lindblom @ SparkFun Electronics
May 9, 2016
https://github.com/sparkfun/SparkFun_BQ27441_Arduino_Library

Demonstrates how to set up the BQ27441 and read state-of-charge (soc),
battery voltage, average current, remaining capacity, average power, and
state-of-health (soh).

After uploading, open up the serial monitor to 115200 baud to view your 
battery's stats.

Hardware Resources:
- Arduino Development Board
- SparkFun Battery Babysitter

Development environment specifics:
Arduino 1.6.7
SparkFun Battery Babysitter v1.0
Arduino Uno (any 'duino should do)
******************************************************************************/
#include"TFT_eSPI.h"
#include <SparkFunBQ27441.h>

TFT_eSPI tft;

const unsigned int BATTERY_CAPACITY = 650; // Set Wio Terminal Battery's Capacity. 650 is from factory. Change if LiPo pouch replaced by 18650 cells

bool setupBQ27441(void) {
  bool bqInit = lipo.begin();
  if (bqInit) {
    unsigned int fullCapacity = lipo.capacity(FULL);
    if (fullCapacity != BATTERY_CAPACITY) {
      lipo.setCapacity(BATTERY_CAPACITY);
    }
  }
  return bqInit;
}

void printBatteryStats() {  // Read battery stats from the BQ27441-G1A
  // Test results: by default the WIO battery pack gets charged to 4.2v and cuts off around 2.98v
  String socStr, voltsStr, currentStr, fullCapacityStr, capacityStr, powerStr, statusStr, temperatureStr, tempStr;
  unsigned int socVal = lipo.soc();                                 // Read state-of-charge (%)
  if (socVal <= 100) {                                              // if can't find the bq27441 most values read 65535
    unsigned int voltsVal = lipo.voltage();                         // Read battery voltage (mV)
    int currentVal = lipo.current(AVG);                             // Read average current (mA)
    unsigned int fullCapacityVal = lipo.capacity(FULL);             // Read full capacity (mAh)
    unsigned int capacityVal = lipo.capacity(REMAIN);               // Read remaining capacity (mAh)
    int powerVal = lipo.power();                                    // Read average power draw (mW)
    unsigned int statusVal = lipo.status();                         // BQ27441 status register. See TI's datasheet
    // unsigned int health = lipo.soh();                            // Read state-of-health (%). Removing, always displays 0
    unsigned int temperatureVal = lipo.temperature(INTERNAL_TEMP);  // Sparkfun library claims value in degree celsius, but seems to report in °C x 100 (ie shifted by 2 decimal digits). bq27441 datasheet reports register unit is 0.1°K
    // unsigned int temp_battery = lipo.temperature(BATTERY);       // the WIO battery pack does NOT implement a battery thermistor

    // Convert values to String for display
    socStr = String(socVal) + " % ";
    voltsStr = String(voltsVal) + " mV ";
    currentStr = String(currentVal) + " mA ";
    fullCapacityStr = String(fullCapacityVal) + " mAh ";
    capacityStr = String (capacityVal) + " mAh ";
    powerStr = String(powerVal) + " mW ";
    statusStr = String (statusVal);

    // Convert Temp to oC as reading is in oC x 100
    int tint = int(temperatureVal / 100);       // Separating integer value from decimal
    int tdec = temperatureVal - tint * 100;
    tempStr = String(tint) + ".";
    if (tdec > 9) {
      tempStr += String(tdec) + " oC ";
    } else {
      tempStr += "0" + String(tdec) + " oC ";   // Pad with 0 to maintain 2 decimals
    }

  } else {                    // No response from a BQ27441  
    lipo.begin();             // Attempt an initialization in case the battery just got inserted
    socStr = "N/A";           // No values for this screen refresh cycle. If battery just got discovered, will be read at next refresh
    voltsStr = "N/A";
    currentStr = "N/A";
    fullCapacityStr = "N/A";
    capacityStr = "N/A";
    powerStr = "N/A";
    statusStr = "N/A";
    tempStr = "N/A";
  }

  // Display values
  int x = 146;
  int y = 0;
  int dy = 14;
  tft.drawString("State of Charge", 0, y, 2);
  tft.drawString(socStr, x, y, 2);
  y += dy;
  tft.drawString("Voltage", 0, y, 2);
  tft.drawString(voltsStr, x, y, 2);
  y += dy;
  tft.drawString("Current", 0, y, 2);
  tft.drawString(currentStr, x, y, 2);
  y += dy;
  tft.drawString("Power", 0, y, 2);
  tft.drawString(powerStr, x, y, 2);
  y += dy;
  tft.drawString("Capacity Left", 0, y, 2);
  tft.drawString(capacityStr, x, y, 2);
  y += dy;
  tft.drawString("Capacity when Full", 0, y, 2);
  tft.drawString(fullCapacityStr + " (spec= " + String(BATTERY_CAPACITY) + ")", x, y, 2);
  y += dy;
  tft.drawString("BQ27441 BMS status", 0, y, 2);
  tft.drawString(statusStr, x, y, 2);
  y += dy;
  tft.drawString("BQ27's Temperature", 0, y, 2);
  tft.drawString(tempStr, x, y, 2);
}

void displaySOCgauge() {    // displays a graphical SOC gauge. Green: 30-100, Yellow:10-20%, Red 0-10%
  unsigned int socVal = lipo.soc();   // Read state-of-charge (%)
  if (socVal > 100) {                 // Battery not responding. SOC should be 0-100. When battery pack is not present soc = 64535
    lipo.begin();                     // Attempt an initialization in case battery was just inserted
    socVal = lipo.soc();
  }
  if (socVal <= 100) {                // Might be too soon for the battery pack to respond w valid data if an initialization was just performed. But will be ready at next screen refresh
    int current = lipo.current();     // Read average power draw (mW)
    int width = 50;
    int height = 100;
    int x = 40;
    int y = 130;
    auto colorOutline = TFT_BLUE;    // battery charging
    auto colorGauge = TFT_GREEN;      // SOC >= 30%

    // Draw gauge outline
    if (current < 0) {
      colorOutline = TFT_RED;         // battery discharging
    } else if (current = 0) {
      colorOutline = TFT_GREEN;       // battery charged, no load
    }
    tft.drawRect(x-1, y-1, width+2, height+2, colorOutline);

    // draw gauge
    if (socVal < 10) {
      colorGauge = TFT_RED;           // SOC < 10%
    } else if (socVal < 30) {
      colorGauge = TFT_YELLOW;        // SOC < 30%
    }
    tft.fillRect(x, y + height - socVal, width, socVal, colorGauge);
  }
}

void setup()
{
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_WHITE);
  tft.setFreeFont(&FreeSans12pt7b); 
  tft.setTextColor(TFT_BLACK);

  setupBQ27441();
}

void loop()  
{
  tft.fillScreen(TFT_BLACK);

  printBatteryStats();

  displaySOCgauge();

  delay(2000);
}
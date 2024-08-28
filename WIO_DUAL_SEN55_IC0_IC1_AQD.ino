// *************************************************************
// Based on "Sensirion I2C SEN5X > exampleUsage.ino" (copyright header below)
// Version = 1.0    (8/2024)
//
// Modifications by RaphTronic:
//    . For WIO Terminal Arduino system from SEEED Studio
//    . Dual SEN55 support
//    . Removed all graphics
//    . Sensor data displayed on TFT screen
//
// Feel free to use / copy my changes
//
// The SEN55s must be on different I2C buses to avoid address conflicts: one on I2C1 (Wire), one on I2C0 (Wire1)
// Either one can be present or absent. If absent, an error message is displayed in red for that sensor
// Both require 5V power (not 3.3V)
// See blog post for schematics: [DIY] Air Quality Monitor & Datalogger - Part 2 Implementation
//     https://raphtronic.blogspot.com/2024/08/diy-air-quality-monitor-datalogger-part.html
// *************************************************************

/*
 * I2C-Generator: 0.3.0
 * Yaml Version: 2.1.3
 * Template Version: 0.7.0-112-g190ecaa
 */
/*
 * Copyright (c) 2021, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <TFT_eSPI.h>
#include <SensirionI2CSen5x.h>
#include <Wire.h>

TFT_eSPI tft;
TFT_eSprite spr = TFT_eSprite(&tft);  //sprite

SensirionI2CSen5x sen5x_0;
SensirionI2CSen5x sen5x_1;

void displayValues(String txt, int y, int dy, float pm1, float pm2p5, float pm4, float pm10, float humid, float temp, float voc, float nox) {
    tft.drawString(txt, 0, y, 2);
    y += dy;
    txt = "      PM1=" + String(pm1, 1) + " PM2.5=" + String(pm2p5, 1) + " PM4=" + String(pm4, 1) + " PM10=" + String(pm10, 1);
    tft.drawString(txt, 0, y, 2);
    y += dy;
    txt = "      VOC=" + String(voc, 0) + " NOx=" + String(nox, 0) + " T=" + String(temp, 2) + " RH=" + String(humid, 2);
    tft.drawString(txt, 0, y, 2);
}

void displayError(String txt, uint16_t error, int y, int dy) {
  char errorMessage[256];
  errorToString(error, errorMessage, 256);
  tft.setTextColor(TFT_RED);
  tft. drawString(txt, 0, y, 2);
  y += dy;
  Serial.print(txt + " ");
  txt = String(errorMessage);
  tft. drawString(txt, 0, y, 2);
  tft.setTextColor(TFT_WHITE);
  Serial.println(txt);
}

void setup() {

  pinMode(LED_BUILTIN, OUTPUT);

  // initialize serial port. Port not required, so not waiting for it
  Serial.begin(115200);
  delay(1000);

  // initialize WIO's TFT screen
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setFreeFont(&FreeSans12pt7b);
  tft.setTextColor(TFT_WHITE);
  int y = 0;                                    // screen Y position for next string to display
  int dy = 14;                                  // cursor increment to next display line
  tft.drawString("Dual SEN55 basic sketch", 0, y , 2);
  y += 2 * dy;

  // initialize I2C1 bus
  Wire.begin();
  // initialize I2C0 bus
  Wire1.begin();

  // initialize SEN55 sensors
  sen5x_0.begin(Wire1); // I2C0 bus
  sen5x_1.begin(Wire);  // I2C1 bus

  // Reset sensors
  uint16_t error;
  char errorMessage[256];
  error = sen5x_0.deviceReset();
  String txt;
  if (error) {
    txt = "Error deviceReset() SEN55 (I2C0):";
    displayError(txt, error, y, dy);
    y += 3 * dy;
    delay(1000);
  }

  error = sen5x_1.deviceReset();
  if (error) {
    txt = "Error deviceReset() SEN55 (I2C1):";
    displayError(txt, error, y, dy);
    y += 3 * dy;
    delay(1000);
  }

  // Start Measurement
  error = sen5x_0.startMeasurement();
  if (error) {
    txt = "Error startMeasurement() SEN55 (I2C0):";
    displayError(txt, error, y, dy);
    y += 3 * dy;
    delay(1000);
  }

  error = sen5x_1.startMeasurement();
  if (error) {
    txt = "Error startMeasurement() SEN55 (I2C1):";
    displayError(txt, error, y, dy);
    y += 3 * dy;
    delay(1000);
  }

  y += dy;
  tft. drawString("Setup done", 0, y, 2);
  Serial.println("Setup done");
  delay(2000);
}

void loop() {
  uint16_t error;
  char errorMessage[256];

  tft.fillScreen(TFT_BLACK);                    // clear the display
  int y = 0;                                    // screen Y position for next string to display
  int dy = 14;                                  // cursor increment to next display line

  float pm1, pm2p5, pm4, pm10, humid, temp, voc, nox;
  String txt;

  // Read sensor values (I2C0)
  error = sen5x_0.readMeasuredValues(pm1, pm2p5, pm4, pm10, humid, temp, voc, nox);
  if (!error) {
    txt = "SEN55 (I2C0):";
    displayValues(txt, y, dy, pm1, pm2p5, pm4, pm10, humid, temp, voc, nox);
    y += 4 * dy;
  } else {
    txt = "Error readMeasuredValues() SEN55 (I2C0):";
    displayError(txt, error, y, dy);
    y += 3 * dy;
  }

  // Read sensor values (I2C1)
  error = sen5x_1.readMeasuredValues(pm1, pm2p5, pm4, pm10, humid, temp, voc, nox);
  if (!error) {
    txt = "SEN55 (I2C1):";
    displayValues(txt, y, dy, pm1, pm2p5, pm4, pm10, humid, temp, voc, nox);
    y += 4 * dy;
  } else {
    txt = "Error readMeasuredValues() SEN55 (I2C1):";
    displayError(txt, error, y, dy);
    y += 3 * dy;
  }

  delay(3000);
}

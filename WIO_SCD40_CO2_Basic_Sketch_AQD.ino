/*************************************************************
Basic Arduino sketch for the Seeed Studio WIO Terminal all-in-one device https://wiki.seeedstudio.com/Wio-Terminal-Getting-Started/

Displays measurements from Sensirion SCD40 CO2/Temp/RH sensor from Adafruit's board #5187

Version = 1.0    (8/2024)
Written by Raphtronic https://raphtronic.blogspot.com

Usage: . Download the Adafruit SCD4x library, as described here: https://learn.adafruit.com/adafruit-scd-40-and-scd-41/arduino
       . Plug the Adafruit board into the WIO Terminal's Grove port with the I2C1 bus (front left port, left of USB-C port)
       . Note that at power-on it can take several minutes for the sensor to stabilize near 400ppm (modern era's CO2 level)
       . See blog post for schematics: [DIY] Air Quality Monitor & Datalogger - Part 2 Implementation
            https://raphtronic.blogspot.com/2024/08/diy-air-quality-monitor-datalogger-part.html

Based on "Sensirion I2C SCD4X > exampleUsage.ino" example sketch (copyright header below)

Modifications:
   . Adapted for the WIO Terminal Arduino system from Seeed Studio
   . Sensor data displayed on TFT screen

Feel free to copy / leverage this work
*************************************************************/

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
#include <SensirionI2CScd4x.h>
#include <Wire.h>

TFT_eSPI tft;

SensirionI2CScd4x scd4x;

void setup() {

  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_WHITE);
  tft.setFreeFont(&FreeSans12pt7b); 
  tft.setTextColor(TFT_BLACK);

  pinMode(WIO_5S_PRESS, INPUT_PULLUP);      // Set up the pin connected to the WIO's joystick Press switch

  Wire.begin();

  bool warning = false;
  uint16_t error;
  char errorMessage[256];
  String txt;

  scd4x.begin(Wire);

  // stop potentially previously started measurement
  error = scd4x.stopPeriodicMeasurement();
  if (error) {
    errorToString(error, errorMessage, 256);
    tft.drawString("Error from stopPeriodicMeasurement():", 0, 10, 2);
    tft.drawString(String(errorMessage), 0, 30, 2);
    warning = true;
  }

  // Start Measurement
  error = scd4x.startPeriodicMeasurement();
  if (error) {
    errorToString(error, errorMessage, 256);
    tft.drawString("Error from startPeriodicMeasurement():", 0, 50, 2);
    tft.drawString(String(errorMessage), 0, 70, 2);
    warning = true;
  }

  if (warning) {
    tft.drawString("  <to proceed press joystick>", 0, 110, 2);
    while (digitalRead(WIO_5S_PRESS) == HIGH) {}
  }
}

void loop() {
  tft.fillScreen(TFT_WHITE);

  uint16_t error;
  char errorMessage[256];
  String txt;

  uint16_t co2 = 0;
  float temperature = 0.0f;
  float humidity = 0.0f;

  // The sensor has a ~5s refraction period after a sample read (same value will be read for 5s)
  // Checking getDataReadyFlag() ensures a new sample is ready
  bool isDataReady = false;
  error = scd4x.getDataReadyFlag(isDataReady);
  if (error) {
    errorToString(error, errorMessage, 256);
    tft.drawString("Error from getDataReadyFlag():", 0, 30, 2);
    tft.drawString(String(errorMessage), 0, 50, 2);
    return;
  }
  if (!isDataReady) { 
    tft.drawString("Sample Not Ready", 0, 0, 2);
    delay(100);
    return;           
  }

  // Read Measurement
  error = scd4x.readMeasurement(co2, temperature, humidity);
  if (error) {
    errorToString(error, errorMessage, 256);
    tft.drawString("Error from readMeasurement():", 0, 30, 2);
    tft.drawString(String(errorMessage), 0, 50, 2);
  } else if (co2 == 0) {
    tft.drawString("Invalid sample detected, skipping.", 0, 30, 2);
  } else {
    tft.drawString("CO2:  " + String(co2), 0, 30, 2);
    tft.drawString("Temp: " + String(temperature), 0, 50, 2);
    tft.drawString("Humidity: " + String(humidity), 0, 70, 2);
  }

  delay(2000);    
}

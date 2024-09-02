/*************************************************************
Basic Arduino sketch for the Seeed Studio WIO Terminal all-in-one device https://wiki.seeedstudio.com/Wio-Terminal-Getting-Started/

Shows micro-SD Card presence & status after insertion, removal, ejection events

Version = 1.0    (8/2024)
Written by Raphtronic https://raphtronic.blogspot.com

Usage: . Was developed as the WIO example sketches assume card is always in and hang on insert / removal
       . The code below handles these events, plus ejection request, gracefully
       . Boot w/o an SD card --> displays "SD absent"
       . Insert a formatted micro-SD card (in my case: 16GB, Windows formatting, FAT32) --> displays SD card status: size, used, etc
       . Remove & reinsert card --> cycles between the previous 2 steps
       . Pressing the joystick when card is present initiates a request for ejection --> displays "ejected" until card is removed
       . See blog post for context and setup: [DIY] Air Quality Monitor & Datalogger - Part 2 Implementation
              https://raphtronic.blogspot.com/2024/08/diy-air-quality-monitor-datalogger-part.html

Feel free to copy / leverage this work
*************************************************************/



#include <TFT_eSPI.h>                       // WIO Terminal's TFT screen
#include <Seeed_Arduino_FS.h>               // File system for SD card

TFT_eSPI tft;                               // WIO's TFT screen

#define devSD  SD                           // WIO's micro-SD Card interface

unsigned long sdMillis = 0;
unsigned long debounceSD = 1000;            // milliseconds

bool sdSwitch = false;                      // used for debouncing the SD Card presence switch
bool sdPresent = false;                     // SD Card is present (debounced)
bool sdEjected = false;                     // User initiated an ejection. SD Card not accessible anymore, but possibly still present.
                                            // To access the card again, the user must remove and insert it again

void setup() {

  tft.begin();                              // Init screen
  tft.setRotation(3);
  tft.setTextColor(TFT_BLACK);
  tft.setFreeFont(&FreeSans12pt7b);

  pinMode(WIO_5S_PRESS, INPUT_PULLUP);      // Set up the pin connected to the WIO's joystick Press switch

  pinMode(SDCARD_DET_PIN, INPUT_PULLUP);    // Set up the pin connected to the SD Card's presence switch
}

void loop() {

  tft.fillScreen(TFT_WHITE);                // Clear screen
  int y = 0;
  int dy = 14;
  String txt, status;
  unsigned long nowMillis;

  // Debounce the micro-SD Card's presence switch
  nowMillis = millis();
  if (!digitalRead(SDCARD_DET_PIN)) {       // SD card is present. Debouncing
    if (!sdSwitch) {
      sdMillis = nowMillis;                 // start debouncing, only on insertion
      sdSwitch = true;
    } else if (nowMillis > sdMillis + debounceSD) {
      sdPresent = true;
    }
  } else {
    sdSwitch = false;
    sdPresent = false;                      // no debouncing on removal
    sdEjected = false;
  }

  // Display the micro-SD Card's status
  if (sdPresent) {
    tft.drawString("SD card present", 0, y, 2);
    y += dy;
    if (!sdEjected) {
      devSD.end();
      if (!devSD.begin(SDCARD_SS_PIN,SDCARD_SPI,4000000UL)) {
        tft.drawString("Card Mount Failed", 0, y, 2);
      } else {
        uint8_t cardType = devSD.cardType();
        if (cardType == CARD_NONE) {
            tft.drawString("No SD card attached", 0, y, 2);
        } else {
          tft.drawString("SD card status: Ready", 0, y, 2);
          y += dy;
          uint32_t usedBytes = devSD.usedBytes();
          txt = String(usedBytes);
          txt = "Used Bytes: " + txt;
          tft.drawString(txt, 0, y, 2);
          y += dy;
          uint32_t totalBytes = devSD.totalBytes();
          txt = String(totalBytes);
          txt = "Total Bytes: " + txt;
          tft.drawString(txt, 0, y, 2);
          y += dy;
          uint32_t cardSize = devSD.cardSize();
          txt = String(cardSize);
          txt = "Card Size: " + txt;
          tft.drawString(txt, 0, y, 2);
          y += 2 * dy;
          tft.drawString("  <to eject SD card press joystick>", 0, y, 2);
        }
      }
    } else {
      tft.drawString("SD card status: Present but Ejected", 0, y, 2);
    }
  } else {
    tft.drawString("SD card absent", 0, y, 2);
  }

  // Wait before next screen refresh
  int count = 0;
  while (count < 20) {   // Wait 2s
    delay(100);
    count += 1;
    if (!sdEjected && (digitalRead(WIO_5S_PRESS) == LOW)) {   // Ejection requested
      if (sdPresent) {
        devSD.end();                                          // End SD Card services
        sdEjected = true;                                     // Register 'ejected' status
        break;                                                // Get out of delay loop
      }
    }
  }
}
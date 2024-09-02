/*************************************************************
Basic Arduino sketch for the SEEED Studio WIO Terminal all-in-one device https://wiki.seeedstudio.com/Wio-Terminal-Getting-Started/

Takes a color-accurate Screenshot, and saves to SD card as a BMP file viewable in Windows

Version = 1.0    (8/2024)
Written by Raphtronic https://raphtronic.blogspot.com

Usage: . Insert a formatted micro-SD card (in my case: 16GB, Windows formatting, FAT32)
       . Upload and boot sketch
       . Display will show lines of text in 12 different colors
       . If no SD card inserted, Serial port reports "Card mount failed"
       . If SD card is present, Serial port lists each line being converted and "Pixels written, BMP file closed"
       . open "screenshot.bmp" file in Windows
       . Note: proper timestamping of the BMP file is not implemented (no RTC, no dateTimeCallback() equivalent, etc)
       . See blog post for context and setup: [DIY] Air Quality Monitor & Datalogger - Part 2 Implementation
              https://raphtronic.blogspot.com/2024/08/diy-air-quality-monitor-datalogger-part.html


Feel free to copy / leverage this work
*************************************************************/


#include <TFT_eSPI.h>                       // WIO Terminal's TFT screen
#include <Seeed_Arduino_FS.h>               // SD card

#define devSD  SD // SD card

// #define DEBUG                            // Uncomment if need to see each pixel converted on Serial port

TFT_eSPI tft;                               // WIO's screen is 320 x 240 pixels

File bmpFile;                               // File object used to save the BMP file

uint8_t  pixels565[2 * 320];                // Buffer holding the pixels read from the screen. 565 RGB format --> 2 bytes per pixel
uint8_t  pixels888[3 * 320];                // Buffer holding the converted pixels for the BMP file. 888 RGB format --> 3 bytes per pixel

// BMP header generation inspired from mborgerson's reply
// at https://forum.pjrc.com/index.php?threads/create-an-image-as-an-array-of-pixels-and-save-as-bmp-file.74006/#post-334757
# pragma pack (push)
# pragma pack (2)
// save previous packing, then set packing for 16-bits
typedef struct  BMPheaderType{
  uint16_t  bfType = 0x4d42;                //'bm';
  uint32_t  bfSize = 320 * 240 * 3 + 54;    // 76800 pixels x 3 color bytes + 54 header
  uint16_t  bfReserved1 = 0;
  uint16_t  bfReserved2 = 0;
  uint32_t  bfOffBits =  54;                // 14 bytes to here
  uint32_t  biSize = 40;
  int32_t   biWidth = 320;
  int32_t   biHeight = -240;                // windows wants negative for top-down image
  int16_t   biPlanes = 1;
  uint16_t  biBitCount = 24 ;
  uint32_t  biCompression = 0;              // bitfields used needs color masks
  uint32_t  biSizeImage = 320 * 240 * 3;    // 320 * 240 * 3 bytes (24b)
  int32_t   biXPelsPerMeter = 0;
  int32_t   biYPelsPerMeter = 0;
  uint32_t  biClrUsed  = 0;
  uint32_t  biClrImportant = 0;             // 54 bytes to end
} BMPHDR_VGA;
// restore previous packing
# pragma pack (pop)

BMPheaderType Header;                       // header structure describing the BMP image's attributes


bool sdCardMount() {
  devSD.end();    // Ensures card is ready for initialization. Begin() fails in corner cases (card removed then reinserted, etc)
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

bool createFile() {
  bmpFile = devSD.open("screenshot.bmp", FILE_WRITE);   // Note: will override an existing file if it has the same name
  if (devSD.exists("screenshot.bmp")) {
    Serial.print("Created file ");
    Serial.println("screenshot.bmp");
    return true;
  } else {
    Serial.print("Failed to create file ");
    return false;
  }
}

void writeBMPheader() {
  Serial.println("Writing BMP header");
  bmpFile.write((uint8_t*)&Header, sizeof(Header));
  Serial.println("BMP header written");
}

void writePixelsToSDcard_rectRGB_method() { // for reference only. This native library method is NOT color accurate
  Serial.println("Writing pixels, rectRGB method");
  for (int i = 0; i < 240; i++) {
    // readRectRGB directly provides 24bit (888) pixel color values. Fast native library method but colors not accurate: 
    // although pure R, G or B colors work well, mixed colors are completey off, like orange becomes dark cyan, etc
    tft.readRectRGB(0, i, 320, 1, (uint8_t*)&pixels565);      
    bmpFile.write((uint8_t*)&pixels565, sizeof(pixels565));
  }
  Serial.println("Pixels written");
  bmpFile.close();
  Serial.println("BMP file closed");
}

void writePixelsToSDcard_custom_565to888_conversion() {   // color accurate method used in this sketch
  Serial.println("Writing pixels, custom 565-888 method");
  // Read each pixel line and write to SD card. Slower custom method but color accurate.
  // Read 16b (565) RGB values and custom convert to 888 to preserve mixed colors
  for (int i = 0; i < 240; i++) {
  // for (int i = 0; i < 80; i++) {
    Serial.println("Line " + String(i));
    tft.readRect(0, i, 320, 1, (uint16_t*)&pixels565);
    convert565to888();
    bmpFile.write((uint8_t*)&pixels888, sizeof(pixels888));
  }
  Serial.println("Pixels written");
  bmpFile.close();
  Serial.println("BMP file closed");
}

void convert565to888() {  // convert each pixel's 16b (565) value to 24b (888) in color-accurate fashion
  uint8_t r5, g6, b5;     // 565 RGB values
  uint8_t r8, g8, b8;     // 888 RGB converted values
  uint16_t *val16b;
  uint16_t val565;        // value with swapped endian-ness (ie low / high bytes swapped)
  float f;

  for (int i = 0; i < 320; i++) {
    // get 16b-pixel value
    val16b = (uint16_t*)&pixels565[i * 2];

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

#ifdef DEBUG
Serial.print(String(val565));
Serial.print(" ");
Serial.print(String(val565, HEX));
Serial.print(" ");
Serial.print(String(val565, BIN));
Serial.print(" " + String(r5) + "." + String(g6) + "." + String(b5));
Serial.println(" " + String(r8) + "." + String(g8) + "." + String(b8));
#endif

  }
}

void displayColoredText(int x) {  // display text in 12 colors
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Text color: white", x, 0, 2);
  tft.setTextColor(TFT_RED);
  tft.drawString("Text color: red", x, 20, 2);
  tft.setTextColor(TFT_GREEN);
  tft.drawString("Text color: green", x, 40, 2);
  tft.setTextColor(TFT_BLUE);
  tft.drawString("Text color: blue", x, 60, 2);
  tft.setTextColor(TFT_BLACK);
  tft.drawString("Text color: black", x, 80, 2);
  tft.setTextColor(TFT_CYAN);
  tft.drawString("Text color: cyan", x, 100, 2);
  tft.setTextColor(TFT_MAGENTA);
  tft.drawString("Text color: magenta", x, 120, 2);
  tft.setTextColor(TFT_MAROON);
  tft.drawString("Text color: maroon", x, 140, 2);
  tft.setTextColor(TFT_OLIVE);
  tft.drawString("Text color: olive", x, 160, 2);
  tft.setTextColor(TFT_ORANGE);
  tft.drawString("Text color: orange", x, 180, 2);
  tft.setTextColor(TFT_PURPLE);
  tft.drawString("Text color: purple", x, 200, 2);
  tft.setTextColor(TFT_YELLOW);
  tft.drawString("Text color: yellow", x, 220, 2);
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n\nBMP File Test");

  // Initialize screen
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setFreeFont(&FreeSans9pt7b);

  // fill left side of screen with black, right side w white
  // and display colored text in each half
  displayColoredText(1);
  tft.fillRect(320 / 2, 0, 320 / 2, 240, TFT_WHITE);
  displayColoredText(320 / 2);

  // Initialize SD card, capture screen pixels, save  to SD card
  if (sdCardMount()) {
    if (createFile()) {
      writeBMPheader();
      // writePixelsToSDcard_rectRGB_method(); // fast native 565 to 888 library method. Not color accurate.
      writePixelsToSDcard_custom_565to888_conversion(); // color accurate method
    }
  }

}

void loop() {

}

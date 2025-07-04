#include <Adafruit_GFX.h>     // Core graphics library
#include <Adafruit_ST7789.h>  // Hardware-specific library for ST7789
#include <SPI.h>
#include <Fonts/FreeSerif12pt7b.h>

// These pins will also work for the 1.8" TFT shield.
#define TFT_CS 5
#define TFT_RST 4  // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC 2
#define TFT_MOSI 23  // Data out
#define TFT_SCLK 18  // Clock out

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

String dataArray[6] = { "", "", "", "", "", "" };
volatile bool dataReady = false;  // Flag for new data
volatile int count = 0;
String receivedString = "";
float percentDone;
float lastPercentDone;
String lastsongName;
int lastVolume;
const int potPin = 34;

void setup() {
  Serial.begin(9600);
  delay(100);  //delay
  tft.init(240, 320);
  delay(100);  //delay
  Serial.println(F("Initialized"));
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  delay(100);  //delay
  Serial.println("ready");
  tft.setFont(&FreeSerif12pt7b);
}

void loop() {
  float ADCvalue = analogRead(potPin);
  float ADCvoltage = 3.3*ADCvalue/4095;
  float delayValue = 0.1 * pow(10000, (ADCvalue/4095));
  Serial.println(delayValue);
  Serial.print("Voltage: ");
  Serial.println(ADCvoltage);
  delay(delayValue);
  tft.fillScreen(ST77XX_BLACK);
  testfastlines(ST77XX_WHITE, ST77XX_BLUE);
  //tft.fillScreen(ST77XX_WHITE);
}

void testfastlines(uint16_t color1, uint16_t color2) {
  tft.fillScreen(ST77XX_BLACK);
  for (int16_t y=0; y < tft.height(); y+=5) {
    tft.drawFastHLine(0, y, tft.width(), color1);
  }
  for (int16_t x=0; x < tft.width(); x+=5) {
    tft.drawFastVLine(x, 0, tft.height(), color2);
  }
}

/*void drawVoltageMarkers() {
    int markerCount = 4;
    for (int i = 0; i <= markerCount; i++) {
        float voltage = i * (3.3 / markerCount);
        int y = map(i, 0, markerCount, SCREEN_HEIGHT - 1, 0);  
        for (int i = 0; i <= 8; i++) {
            display.drawLine(16 * i, y, 16 * i + 3, y, WHITE);
        }
        display.setCursor(2, y - 6);
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.print(voltage, 1);
        display.print("V");
    }
} */

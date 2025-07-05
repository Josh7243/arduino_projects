#include <Adafruit_GFX.h>    // Core graphics library
 #include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
 #include <SPI.h>
 #include <Fonts/FreeSerif12pt7b.h> // Using your preferred font

 // These pins will also work for the 1.8" TFT shield.
 #define TFT_CS 5
 #define TFT_RST 4   // Or set to -1 and connect to Arduino RESET pin
 #define TFT_DC 2
 #define TFT_MOSI 23 // Data out
 #define TFT_SCLK 18 // Clock out

 Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

 // We will define NUM_POINTS based on the effective width after rotation
 int NUM_POINTS;
 int *values;     // Current waveform data
 int *lastValues; // Previous waveform data for erasing

 // Define your potentiometer pins
 const int potPin = 34;   // For the main waveform analog input
 const int potPin2 = 33;  // For controlling the delay/sample rate

 // Offset for the waveform to leave space for voltage markers on the left
 const int WAVEFORM_X_OFFSET = 40;

 // Margins for voltage markers to prevent text/ticks from being cut off
 const int Y_MARGIN_TOP = 10;
 const int Y_MARGIN_BOTTOM = 10;

 void setup() {
   Serial.begin(9600);
   delay(100);
   tft.init(240, 320); // Initialize ST7789 with your display's native resolution
   delay(100);
   Serial.println(F("Initialized"));
   tft.setRotation(1); // Set to landscape mode (effective: 320 wide, 240 tall)
                       // Now tft.width() will be 320, tft.height() will be 240.
   tft.fillScreen(ST77XX_BLACK);
   delay(100);
   Serial.println("ready");
   tft.setFont(&FreeSerif12pt7b);
   tft.setTextWrap(false);

   // After rotation, tft.width() is 320, tft.height() is 240.
   NUM_POINTS = tft.width() - WAVEFORM_X_OFFSET; // Use the actual effective width

   // Allocate memory for both current and previous waveform data
   values = (int *) malloc(NUM_POINTS * sizeof(int));
   lastValues = (int *) malloc(NUM_POINTS * sizeof(int));

   if (values == NULL || lastValues == NULL) {
     Serial.println("Failed to allocate memory for waveform arrays!");
     while (true); // Halt if memory allocation fails
   }

   // Initialize arrays with a starting value (e.g., 0V at the bottom)
   for (int i = 0; i < NUM_POINTS; i++) {
     values[i] = tft.height() - 1 - Y_MARGIN_BOTTOM; // Initialize to 0V (bottom adjusted for margin)
     lastValues[i] = tft.height() - 1 - Y_MARGIN_BOTTOM; // Same for last values
   }

   // Draw static elements (voltage markers) only once
   drawVoltageMarkers();
 }

 void drawVoltageMarkers() {
   int markerCount = 4;
   // Effective screen height is tft.height() (e.g., 240 in landscape)
   // Adjusted plotting height for markers, accounting for top and bottom margins
   int effectivePlotHeight = tft.height() - Y_MARGIN_TOP - Y_MARGIN_BOTTOM;

   for (int i = 0; i <= markerCount; i++) {
     float voltage = i * (3.3 / markerCount);
     // Map the voltage to a Y-coordinate within the effective plotting height
     // 0V maps to bottom of plot area, 3.3V maps to top of plot area
     int y = map(i, 0, markerCount, tft.height() - 1 - Y_MARGIN_BOTTOM, Y_MARGIN_TOP);

     // Draw horizontal tick marks for the voltage markers on the left
     tft.drawLine(0, y, WAVEFORM_X_OFFSET - 5, y, ST77XX_WHITE);

     // Display the voltage text
     tft.setCursor(2, y - 6); // Adjust cursor position for text
     tft.setTextSize(1);
     tft.setTextColor(ST77XX_WHITE);
     tft.print(voltage, 1);
     tft.print("V");
   }
 }

 void loop() {
   // Read analog values continuously
   int analogValue = analogRead(potPin);
   int analogValue2 = analogRead(potPin2);

   // Calculate sampling delay based on potPin2
   float sampleDelay = pow(2, (analogValue2 / 4095.0) * log2(1000)) - 1;
   if (sampleDelay < 1) sampleDelay = 1; // Ensure minimum 1ms delay

   // Scale the analog value to fit the screen's effective plotting height
   // 0 to 4095 (12-bit ADC) maps to (bottom - margin) to (top + margin)
   int scaledValue = map(analogValue, 0, 4095,
                         tft.height() - 1 - Y_MARGIN_BOTTOM, // Bottom pixel of plotting area
                         Y_MARGIN_TOP);                     // Top pixel of plotting area

   // --- Store current 'values' into 'lastValues' BEFORE shifting 'values' ---
   memmove(lastValues, values, NUM_POINTS * sizeof(int));

   // --- Shift current data points and add new one ---
   memmove(values, values + 1, (NUM_POINTS - 1) * sizeof(int));
   values[NUM_POINTS - 1] = scaledValue;

   // --- Drawing: Erase old waveform, then draw new waveform ---

   // Erase the previous waveform by drawing it in black
   // Draw slightly thicker to ensure complete erasure of old pixels
   for (int i = 1; i < NUM_POINTS; i++) {
     tft.drawLine(i - 1 + WAVEFORM_X_OFFSET, lastValues[i - 1], i + WAVEFORM_X_OFFSET, lastValues[i], ST77XX_BLACK);
     // Draw adjacent pixels for "thickness" to ensure erase
     // Only add thickness if the line isn't perfectly horizontal
     if (lastValues[i - 1] != lastValues[i]) {
       tft.drawLine(i - 1 + WAVEFORM_X_OFFSET, lastValues[i - 1] + 1, i + WAVEFORM_X_OFFSET, lastValues[i] + 1, ST77XX_BLACK);
       tft.drawLine(i - 1 + WAVEFORM_X_OFFSET, lastValues[i - 1] - 1, i + WAVEFORM_X_OFFSET, lastValues[i] - 1, ST77XX_BLACK);
     }
   }

   // Draw the new waveform in white
   for (int i = 1; i < NUM_POINTS; i++) {
     tft.drawLine(i - 1 + WAVEFORM_X_OFFSET, values[i - 1], i + WAVEFORM_X_OFFSET, values[i], ST77XX_WHITE);
   }

   // Serial output for debugging
   Serial.print("Analog 1: ");
   Serial.print(analogValue);
   Serial.print(", Analog 2: ");
   Serial.print(analogValue2);
   Serial.print(", Sample Delay: ");
   Serial.println(sampleDelay);

   delay(sampleDelay); // This delay directly controls the animation speed
 }

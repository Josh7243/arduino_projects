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

 // We will define NUM_POINTS after tft.width() is known in setup()
 int NUM_POINTS; // Now just a variable, not a preprocessor define
 int *values;    // A pointer, we'll allocate memory for the array dynamically

 // Define your potentiometer pins
 const int potPin = 34;   // For the main waveform
 const int potPin2 = 33;  // For controlling the delay, similar to your original code

 // Offset for the waveform to leave space for voltage markers
 // This will be on the left side of the 240 pixel wide screen
 const int WAVEFORM_X_OFFSET = 40;

 // Variables for controlling the display update rate
 unsigned long previousMillis = 0;
 const long interval = 1000; // Update interval in milliseconds (1000ms = 1 second)

 void setup() {
   Serial.begin(9600);
   delay(100); //delay
   tft.init(240, 320); // Initialize ST7789 with your display's resolution (240 wide, 320 tall)
   delay(100); //delay
   Serial.println(F("Initialized"));
   tft.setRotation(0); // Explicitly set to portrait (width 240, height 320)
   tft.fillScreen(ST77XX_BLACK);
   delay(100); //delay
   Serial.println("ready");
   tft.setFont(&FreeSerif12pt7b); // Set your preferred font
   tft.setTextWrap(false); // Disable text wrapping

   // --- DEBUGGING DIMENSIONS ---
   Serial.print("TFT Width: ");
   Serial.println(tft.width());
   Serial.print("TFT Height: ");
   Serial.println(tft.height());
   // --- END DEBUGGING ---

   // Now that tft.width() is known (after tft.init() and tft.setRotation()),
   // we can set NUM_POINTS and allocate memory for 'values'.
   // NUM_POINTS will be based on the actual width after rotation (which is 240 for rotation 0)
   NUM_POINTS = tft.width() - WAVEFORM_X_OFFSET; // Adjust for the offset on the left
   values = (int *) malloc(NUM_POINTS * sizeof(int)); // Allocate memory for the array

   // It's good practice to check if allocation was successful
   if (values == NULL) {
     Serial.println("Failed to allocate memory for 'values' array!");
     while (true); // Halt execution if memory allocation fails
   }
   // Initialize values to 0 to avoid garbage data on first plot
   for (int i = 0; i < NUM_POINTS; i++) {
     values[i] = tft.height() - 1; // Start with line at 0V (bottom of the 320-pixel height)
   }

   // Draw voltage markers only once in setup()
   drawVoltageMarkers();
 }

 void drawVoltageMarkers() {
   int markerCount = 4;
   // The screen is 240 pixels wide and 320 pixels tall (portrait).
   // Voltage markers will be drawn vertically on the left side.
   // Max voltage is 3.3V (assuming ESP32 ADC reference).
   for (int i = 0; i <= markerCount; i++) {
     float voltage = i * (3.3 / markerCount);
     // Map the voltage to a Y-coordinate on the screen's 320 height.
     // tft.height() - 1 is the bottom of the screen (0V), 0 is the top (3.3V)
     int y = map(i, 0, markerCount, tft.height() - 1, 0);

     // Draw horizontal tick marks for the voltage markers
     // These will be on the left edge up to the offset
     tft.drawLine(0, y, WAVEFORM_X_OFFSET - 5, y, ST77XX_WHITE);

     // Display the voltage text
     // Position adjusted for the portrait screen and left offset
     tft.setCursor(2, y - 6);
     tft.setTextSize(1); // Keep text size small to fit
     tft.setTextColor(ST77XX_WHITE);
     tft.print(voltage, 1); // Print voltage with 1 decimal place
     tft.print("V");
   }
 }

 void loop() {
   // Read analog values frequently, regardless of display update rate
   int analogValue = analogRead(potPin);
   int analogValue2 = analogRead(potPin2);

   // This sampleDelay controls how fast you acquire new data.
   float sampleDelay = pow(2, (analogValue2 / 4095.0) * log2(1000)) - 1;
   if (sampleDelay < 1) sampleDelay = 1;

   // Scale the analog value to fit the screen height (which is 320 in portrait)
   // 0 to 4095 (12-bit ADC) maps to tft.height()-1 (bottom) to 0 (top)
   int scaledValue = map(analogValue, 0, 4095, tft.height() - 1, 0);

   // --- Data Acquisition and Shifting (happens continuously) ---
   // Shift all existing values to the left
   memmove(values, values + 1, (NUM_POINTS - 1) * sizeof(int));
   // Add the new scaled value to the end of the array
   values[NUM_POINTS - 1] = scaledValue;

   // --- Display Update Control ---
   unsigned long currentMillis = millis();

   if (currentMillis - previousMillis >= interval) {
     // Save the last time we updated the display
     previousMillis = currentMillis;

     // --- Drawing Optimization: Clear only the plotting area ---
     // Clear the rectangular area where the waveform is drawn
     // X-start: WAVEFORM_X_OFFSET (e.g., 40 pixels from left edge)
     // Y-start: 0 (top of screen)
     // Width: NUM_POINTS (e.g., 240 - 40 = 200 pixels wide)
     // Height: tft.height() (e.g., 320 pixels tall)
     tft.fillRect(WAVEFORM_X_OFFSET, 0, NUM_POINTS, tft.height(), ST77XX_BLACK);

     // Draw the new waveform
     for (int i = 1; i < NUM_POINTS; i++) {
       // Draw new line segment with white
       // X-coordinates are offset by WAVEFORM_X_OFFSET
       // Y-coordinates are values[i] (which map to the 320-pixel height)
       tft.drawLine(i - 1 + WAVEFORM_X_OFFSET, values[i - 1], i + WAVEFORM_X_OFFSET, values[i], ST77XX_WHITE);
     }

     // Serial output for debugging (only print when display updates)
     Serial.print("Analog 1: ");
     Serial.print(analogValue);
     Serial.print(", Analog 2: ");
     Serial.print(analogValue2);
     Serial.print(", Sample Delay: ");
     Serial.println(sampleDelay);
   }

   // Use the sampleDelay here to control how fast you acquire new data.
   // This will fill the 'values' array at the rate determined by potPin2.
   // The display will only update every 'interval' milliseconds, showing the latest data.
   delay(sampleDelay);
 }

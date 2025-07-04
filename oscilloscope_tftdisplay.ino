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

 void setup() {
   Serial.begin(9600);
   delay(100); //delay
   tft.init(240, 320); // Initialize ST7789 with your display's resolution
   delay(100); //delay
   Serial.println(F("Initialized"));
   tft.setRotation(1); // Set rotation for landscape mode (320x240)
   tft.fillScreen(ST77XX_BLACK);
   delay(100); //delay
   Serial.println("ready");
   tft.setFont(&FreeSerif12pt7b); // Set your preferred font
   tft.setTextWrap(false); // Disable text wrapping

   // --- FIX START ---
   // Now that tft.width() is known (after tft.init() and tft.setRotation()),
   // we can set NUM_POINTS and allocate memory for 'values'.
   NUM_POINTS = tft.width();
   values = (int *) malloc(NUM_POINTS * sizeof(int)); // Allocate memory for the array

   // It's good practice to check if allocation was successful
   if (values == NULL) {
     Serial.println("Failed to allocate memory for 'values' array!");
     while (true); // Halt execution if memory allocation fails
   }
   // Initialize values to 0 to avoid garbage data on first plot
   for (int i = 0; i < NUM_POINTS; i++) {
     values[i] = 0;
   }
   // --- FIX END ---
 }

 void drawVoltageMarkers() {
   int markerCount = 4;
   // The screen is 320 pixels wide and 240 pixels tall after rotation(1).
   // Voltage markers will be drawn vertically on the left side.
   // Max voltage is 3.3V (assuming ESP32 ADC reference).
   for (int i = 0; i <= markerCount; i++) {
     float voltage = i * (3.3 / markerCount);
     // Map the voltage to a Y-coordinate on the screen.
     // SCREEN_HEIGHT - 1 is the bottom of the screen (0V), 0 is the top (3.3V)
     int y = map(i, 0, markerCount, tft.height() - 1, 0);

     // Draw horizontal tick marks for the voltage markers
     // You can adjust the length of these ticks
     for (int j = 0; j <= 8; j++) {
       tft.drawLine(0, y, 10, y, ST77XX_WHITE); // Draw tick marks on the left side
     }

     // Display the voltage text
     tft.setCursor(12, y - 6); // Adjust cursor position for text
     tft.setTextSize(1);
     tft.setTextColor(ST77XX_WHITE);
     tft.print(voltage, 1); // Print voltage with 1 decimal place
     tft.print("V");
   }
 }

 void loop() {
   int analogValue = analogRead(potPin);    // Read the main analog value
   int analogValue2 = analogRead(potPin2);  // Read analog value for delay control

   // Calculate delay value based on potPin2, similar to your original code
   // The power function here creates a logarithmic response for the delay
   float delayValue = pow(2, (analogValue2 / 4095.0) * log2(1000)) - 1;
   if (delayValue < 1) delayValue = 1; // Ensure a minimum delay of 1ms

   // Scale the analog value to fit the screen height
   // 0 to 4095 (12-bit ADC) maps to tft.height()-1 (bottom) to 0 (top)
   int scaledValue = map(analogValue, 0, 4095, tft.height() - 1, 0);

   // Shift all existing values to the left
   memmove(values, values + 1, (NUM_POINTS - 1) * sizeof(int));
   // Add the new scaled value to the end of the array
   values[NUM_POINTS - 1] = scaledValue;

   tft.fillScreen(ST77XX_BLACK); // Clear the screen

   drawVoltageMarkers(); // Draw voltage markers on the screen

   // Draw the waveform
   // Start from 1 to draw lines between points
   for (int i = 1; i < NUM_POINTS; i++) {
     // Draw a line from the previous point to the current point
     // The X-coordinate for plotting starts after the voltage markers (e.g., 40 pixels in)
     tft.drawLine(i - 1 + 40, values[i - 1], i + 40, values[i], ST77XX_WHITE);
   }

   // Serial output for debugging
   Serial.print("Analog 1: ");
   Serial.print(analogValue);
   Serial.print(", Analog 2: ");
   Serial.print(analogValue2);
   Serial.print(", Delay: ");
   Serial.println(delayValue);

   delay(delayValue); // Delay based on the potentiometer
 }

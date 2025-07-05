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
 int *values;    // Current waveform data

 // Define your potentiometer pins
 const int potPin = 34;   // For the main waveform analog input
 const int potPin2 = 33;  // For controlling msPerPoint (graph speed)

 // Offset for the waveform to leave space for voltage markers on the left
 const int WAVEFORM_X_OFFSET = 40;

 // Margins for voltage markers to prevent text/ticks from being cut off
 const int Y_MARGIN_TOP = 10;
 const int int Y_MARGIN_BOTTOM = 10;

 // --- Variables for controlling data acquisition rate per point ---
 unsigned long previousSampleMillis = 0; // When the last point was added to the graph
 float msPerPoint = 100.0;             // Milliseconds each point on the graph represents (0.1 to 1000 seconds)

 // Raw ADC sampling variables (happens continuously in loop)
 unsigned long rawSampleCount = 0;
 long rawSampleSum = 0;

 // Potentiometer 2 range for mapping
 const float POT2_MIN_MS = 100.0;      // 0.1 seconds = 100 ms
 const float POT2_MAX_MS = 1000000.0;  // 1000 seconds = 1,000,000 ms

 void setup() {
   Serial.begin(9600);
   delay(100);
   tft.init(240, 320); // Initialize ST7789 with your display's native resolution
   delay(100);
   Serial.println(F("Initialized"));
   tft.setRotation(1); // Set to landscape mode (effective: 320 wide, 240 tall)
                       // After this, tft.width() should be 320, tft.height() should be 240.
   tft.fillScreen(ST77XX_BLACK);
   delay(100);
   Serial.println("ready");
   tft.setFont(&FreeSerif12pt7b);
   tft.setTextWrap(false);

   // --- DEBUGGING DIMENSIONS ---
   Serial.print("TFT Width (after rotation): ");
   Serial.println(tft.width());
   Serial.print("TFT Height (after rotation): ");
   Serial.println(tft.height());
   // --- END DEBUGGING ---

   // NUM_POINTS will be based on the effective width after rotation (320 pixels)
   NUM_POINTS = tft.width() - WAVEFORM_X_OFFSET; // Use the actual effective width

   // Allocate memory for the current waveform data
   values = (int *) malloc(NUM_POINTS * sizeof(int));

   if (values == NULL) {
     Serial.println("Failed to allocate memory for waveform array!");
     while (true); // Halt if memory allocation fails
   }

   // Initialize array with a starting value (e.g., 0V at the bottom)
   for (int i = 0; i < NUM_POINTS; i++) {
     values[i] = tft.height() - 1 - Y_MARGIN_BOTTOM; // Initialize to 0V (bottom adjusted for margin)
   }

   // Draw static elements (voltage markers) only once
   drawVoltageMarkers();
 }

 void drawVoltageMarkers() {
   int markerCount = 4;
   // Effective screen height is tft.height() (e.g., 240 in landscape)
   // Adjusted plotting height for markers, accounting for top and bottom margins
   // Plotting area top: Y_MARGIN_TOP, Plotting area bottom: tft.height() - 1 - Y_MARGIN_BOTTOM
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
   unsigned long currentMillis = millis();

   // --- Control msPerPoint based on potPin2 ---
   // Map potPin2 (0-4095) logarithmically to msPerPoint (0.1s to 1000s)
   // Using log scale for a better feel over such a wide range
   float pot2Normalized = analogRead(potPin2) / 4095.0; // 0.0 to 1.0
   msPerPoint = exp(pot2Normalized * log(POT2_MAX_MS / POT2_MIN_MS)) * POT2_MIN_MS;
   msPerPoint = constrain(msPerPoint, POT2_MIN_MS, POT2_MAX_MS); // Ensure it stays within bounds

   // --- Raw ADC Reading and Accumulation (as fast as possible) ---
   int rawAnalogValue = analogRead(potPin);
   rawSampleSum += rawAnalogValue;
   rawSampleCount++;

   // --- Check if it's time to add a new point to the graph ---
   if (currentMillis - previousSampleMillis >= msPerPoint) {
     // Calculate the average raw sample value for this point
     int averagedAnalogValue = 0;
     if (rawSampleCount > 0) {
       averagedAnalogValue = rawSampleSum / rawSampleCount;
     }

     // Reset accumulation for the next point
     rawSampleSum = 0;
     rawSampleCount = 0;
     previousSampleMillis = currentMillis; // Set new reference point for next average

     // Scale the averaged value to fit the screen's effective plotting height
     int scaledValue = map(averagedAnalogValue, 0, 4095,
                           tft.height() - 1 - Y_MARGIN_BOTTOM, // Bottom pixel of plotting area
                           Y_MARGIN_TOP);                     // Top pixel of plotting area

     // --- Update Graph Data ---
     // Shift existing values to the left
     memmove(values, values + 1, (NUM_POINTS - 1) * sizeof(int));
     // Add the new scaled value to the end of the array
     values[NUM_POINTS - 1] = scaledValue;

     // --- Display Update (every time a new point is added) ---
     // Clear the specific plotting area to prevent ghosting
     tft.fillRect(WAVEFORM_X_OFFSET, Y_MARGIN_TOP, // X, Y start of the plotting area
                  NUM_POINTS,                     // Width of plotting area
                  tft.height() - Y_MARGIN_TOP - Y_MARGIN_BOTTOM, // Height of plotting area
                  ST77XX_BLACK);

     // Draw the new waveform in white
     for (int i = 1; i < NUM_POINTS; i++) {
       tft.drawLine(i - 1 + WAVEFORM_X_OFFSET, values[i - 1], i + WAVEFORM_X_OFFSET, values[i], ST77XX_WHITE);
     }

     // Serial output for debugging
     Serial.print("Analog: ");
     Serial.print(averagedAnalogValue);
     Serial.print(", msPerPoint: ");
     Serial.print(msPerPoint);
     Serial.print(" (");
     Serial.print(msPerPoint / 1000.0, 3); // Print in seconds
     Serial.println("s)");
   }

   // No delay() here. loop() runs as fast as the code inside it allows.
   // Raw ADC readings are continuous, graph points are averaged over msPerPoint.
 }

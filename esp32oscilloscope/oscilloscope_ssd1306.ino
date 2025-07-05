#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128  
#define SCREEN_HEIGHT 64  
#define OLED_RESET -1     
#define SCREEN_ADDRESS 0x3C  

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUM_POINTS 128  
int values[NUM_POINTS];  

void setup() {
    Serial.begin(115200);
    Wire.setClock(400000);  // Speed up I2C to 400kHz

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("SSD1306 allocation failed");
        for (;;);
    }
    display.clearDisplay();
}

void drawVoltageMarkers() {
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
}

void loop() {
    int analogValue = analogRead(33);  
    int analogValue2 = analogRead(34);
    float logValue = pow(2, (analogValue2 / 4095.0) * log2(1000)) - 1;

    int scaledValue = map(analogValue, 0, 4095, SCREEN_HEIGHT - 1, 0);

    memmove(values, values + 1, (NUM_POINTS - 1) * sizeof(int));
    values[NUM_POINTS - 1] = scaledValue;  

    display.clearDisplay();
    drawVoltageMarkers();

    for (int i = 1; i < NUM_POINTS; i++) {
        display.drawPixel(i - 1, values[i - 1], WHITE);  
        display.drawPixel(i, values[i], WHITE); 
    }

    display.display();  // Update only changed parts
    delay(logValue);

}

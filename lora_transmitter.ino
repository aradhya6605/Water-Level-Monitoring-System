/*************************************************
E32-433T30D TRANSMITTER (BUTTON + LED + OLED)
1. Sends "CNT:X" only when Button (GPIO 13) is pressed
2. LED (GPIO 26) blinks on transmission
3. Receives "ACK:X" from Receiver and calculates Link Quality
*************************************************/

#include <Arduino.h>
#include <LoRa_E32.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* ===== OLED CONFIG ===== */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDR     0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/* ===== UART & PIN CONFIG ===== */
#define E32_RX 16
#define E32_TX 17
#define LED_PIN 26    // LED with 220-330 ohm resistor
#define BUTTON_PIN 13 // Button with 10k ohm pull-down resistor

LoRa_E32 e32(&Serial2);

/* ===== GLOBAL VARIABLES ===== */
uint32_t packetCount = 0;
uint32_t ackCount = 0;
bool lastButtonState = LOW; 
bool needsUpdate = true;
String lastStatus = "READY";

void setup() {
    Serial.begin(115200);
    
    // Pin Modes
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT); // Using external 10k resistor to GND

    // Initialize E32 (Ensure M0 and M1 are connected to GND)
    Serial2.begin(9600, SERIAL_8N1, E32_RX, E32_TX);
    e32.begin();

    // Initialize OLED
    Wire.begin(21, 22);
    if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("OLED failed");
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 20);
    display.println("TX SYSTEM ONLINE");
    display.println("Press Button to Send");
    display.display();
}

void loop() {
    // 1. READ BUTTON (External Pull-Down: HIGH when pressed)
    bool currentButtonState = digitalRead(BUTTON_PIN);

    if (currentButtonState == HIGH && lastButtonState == LOW) {
        delay(50); // Debounce delay
        
        packetCount++;
        String message = "CNT:" + String(packetCount);
        
        // Visual Feedback (LED ON)
        digitalWrite(LED_PIN, HIGH);
        
        // Send LoRa Message
        Serial.print("TX: "); Serial.println(message);
        e32.sendMessage(message);
        
        lastStatus = "SENDING...";
        needsUpdate = true;
        
        // Keep LED on for a moment then turn off
        delay(150); 
        digitalWrite(LED_PIN, LOW);
    }
    lastButtonState = currentButtonState;

    // 2. CHECK FOR ACK (Incoming from Receiver)
    if (e32.available() > 0) {
        ResponseContainer rc = e32.receiveMessage();
        if (rc.data.startsWith("ACK:")) {
            ackCount++;
            lastStatus = "ACK RECEIVED!";
            Serial.print("RX: "); Serial.println(rc.data);
            needsUpdate = true;
        }
    }

    // 3. UPDATE OLED DISPLAY
    if (needsUpdate) {
        float percentage = 0;
        if (packetCount > 0) {
            percentage = ((float)ackCount / (float)packetCount) * 100.0;
        }

        display.clearDisplay();
        
        // Top Bar
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.print("TX:"); display.print(packetCount);
        display.print("  ACK:"); display.print(ackCount);
        display.drawFastHLine(0, 12, 128, SSD1306_WHITE);

        // Percentage
        display.setCursor(30, 22);
        display.setTextSize(3);
        display.print((int)percentage); display.print("%");

        // Bottom Status
        display.setTextSize(1);
        display.setCursor(0, 53);
        display.print("ST:"); display.print(lastStatus);

        display.display();
        needsUpdate = false;
    }
    
    delay(1); // System stability
}

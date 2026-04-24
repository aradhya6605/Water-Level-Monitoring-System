/*************************************************
E32-433T30D RECEIVER (OLED + AUTO-ACK + BLINK)
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
#define LED_PIN 26 // The LED will blink on this pin

LoRa_E32 e32(&Serial2);

/* ===== GLOBAL VARIABLES ===== */
int totalReceived = 0;
String lastVal = "0";
bool needsUpdate = true;

void setup() {
    Serial.begin(115200);
    
    // Initialize LED Pin
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW); // Start with LED off

    // Initialize E32 Serial (Default 9600)
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
    display.setCursor(0, 10);
    display.println("RECEIVER ONLINE");
    display.println("Waiting for data...");
    display.display();
    
    Serial.println("Receiver Ready. Waiting for CNT packets...");
}

void loop() {
    // 1. CHECK FOR INCOMING DATA
    if (e32.available() > 0) {
        ResponseContainer rc = e32.receiveMessage();
        
        if (rc.status.code == SUCCESS) {
            String incoming = rc.data;
            
            // Check if it's a Count packet
            if (incoming.startsWith("CNT:")) {
                // --- VISUAL FEEDBACK (BLINK) ---
                digitalWrite(LED_PIN, HIGH); // Turn LED ON
                
                totalReceived++;
                lastVal = incoming.substring(4);
                
                // 2. SEND ACK BACK
                String ackMsg = "ACK:" + lastVal;
                e32.sendMessage(ackMsg);
                
                Serial.print("Received: "); Serial.print(incoming);
                Serial.print(" | Sent: "); Serial.println(ackMsg);
                
                delay(100);                  // Keep LED on for 100ms so it's visible
                digitalWrite(LED_PIN, LOW);  // Turn LED OFF
                
                needsUpdate = true;
            }
        }
    }

    // 3. UPDATE OLED DISPLAY
    if (needsUpdate) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.print("LORA RX: "); display.print(totalReceived);
        display.drawFastHLine(0, 12, 128, SSD1306_WHITE);

        display.setCursor(10, 25);
        display.setTextSize(2);
        display.print("VAL: ");
        display.print(lastVal);

        display.setTextSize(1);
        display.setCursor(0, 55);
        display.print("Status: DATA RECEIVED");
        display.display();
        needsUpdate = false;
    }
}

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino.h>
#include <ArduinoJson.h>


#define TFT_CS   12  // D12, GPIO12
#define TFT_RST  25  // D25, GPIO25
#define TFT_DC   2   // D2,  GPIO2

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

const char* ssid = "Keenetic-4604";
const char* password = "23137099";

unsigned long previousMillis = 0;
const long interval = 500;
int dotCount = 0;

int cursorY = 10; 

void setup() {
    Serial.begin(115200);
    delay(1000);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    tft.initR(INITR_BLACKTAB);
    tft.setRotation(1);
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, cursorY);

    while(WiFi.status() != WL_CONNECTED){
        unsigned long currentMillis = millis();
        
        if (currentMillis - previousMillis >= interval) {
            previousMillis = currentMillis;

            tft.fillScreen(ST77XX_BLACK);
            tft.setCursor(10, 10);

            tft.print("Connecting");
            for (int i = 0; i < dotCount; i++) {
                tft.print(".");
            }

            dotCount++;
            if (dotCount > 3) {
                dotCount = 0;
            }
        }
    }

    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(10, 10);
    tft.println("Connected:");
    tft.println(WiFi.localIP());

    HTTPClient http;

    String serverName = "https://kekkai-api.redume.su/api/getRate/";

    String serverPath = serverName + "?from_currency=USD&conv_currency=RUB&date=2025-05-26";

    http.begin(serverPath.c_str());

    int httpResCode = http.GET();

    if (httpResCode > 0) {
        tft.fillScreen(ST77XX_BLACK);
        tft.setCursor(10, 10);
        tft.setTextSize(1.3);

        String payload = http.getString();

        StaticJsonDocument<256> doc;

        DeserializationError error = deserializeJson(doc, payload);
        if (error) {
            tft.println("JSON error");
            return;
        }

        String from = doc["from_currency"].as<const char*>();
        String to = doc["conv_currency"].as<const char*>();
        float rate = doc["rate"].as<float>();
        tft.print("1 " + String(from) + " = " + String(rate) + " " + String(to));
    }
}

void loop() {}

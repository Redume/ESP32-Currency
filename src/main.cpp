#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define TFT_CS   12
#define TFT_DC    2
#define TFT_RST  25
Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);

const char* ssid            = "NAME_WIFI";
const char* password        = "WIFI_PSWD";
const char* kekkai_instace  = "https://kekkai-api.redume.su";

unsigned long previousMillis = 0;
const long interval          = 500;
int dotCount                 = 0;

void drawGraph(const float *val, int len,
               uint16_t x0, uint16_t y0,
               uint16_t w,  uint16_t h,
               uint16_t colorLine, uint16_t colorAxis)
{
    float vMin = val[0], vMax = val[0];
    for (int i = 1; i < len; ++i) {
        vMin = min(vMin, val[i]);
        vMax = max(vMax, val[i]);
    }
    float vRange = (vMax - vMin == 0) ? 1.0f : (vMax - vMin);

    tft.fillRect(x0, y0, w, h, ST77XX_BLACK);
    tft.drawRect(x0, y0, w, h, colorAxis);

    const uint8_t GRID_Y = 4;
    for (uint8_t g = 1; g < GRID_Y; ++g) {
        uint16_t y = y0 + g * h / GRID_Y;
        tft.drawFastHLine(x0 + 1, y, w - 2, 0x7BEF);
    }

    for (int i = 0; i < len - 1; ++i) {
        int x1 = x0 + 1 + (i     * (w - 2)) / (len - 1);
        int y1 = y0 + h - 2 - (int)((val[i]     - vMin) * (h - 3) / vRange);
        int x2 = x0 + 1 + ((i+1) * (w - 2)) / (len - 1);
        int y2 = y0 + h - 2 - (int)((val[i + 1] - vMin) * (h - 3) / vRange);
        tft.drawLine(x1, y1, x2, y2, colorLine);
    }

    tft.setTextColor(colorAxis);
    tft.setTextSize(1);
    tft.setCursor(x0 + 120, y0 - 8);
    tft.printf("%.1f", vMax);
    tft.setCursor(x0 + 2, y0 + h + 2);
    tft.printf("%.1f", vMin);
}

void setup(){
    Serial.begin(115200);
    delay(1000);

    tft.initR(INITR_BLACKTAB);
    tft.setRotation(1);
    tft.setSPISpeed(40000000);
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);
    tft.setCursor(0, 10);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
            previousMillis = currentMillis;
            tft.fillScreen(ST77XX_BLACK);
            tft.setCursor(0, 10);
            tft.print("Connecting");
            for (int i = 0; i < dotCount; i++) tft.print(".");
            dotCount = (dotCount + 1) % 4;
        }
    }
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 10);
    tft.print("Connected: ");
    tft.print(WiFi.localIP());
    delay(1000);

    HTTPClient       http;
    WiFiClientSecure client;
    client.setInsecure();

    String dateRateUrl = String(kekkai_instace)
                       + "/api/getRate/?from_currency=USD"
                       + "&conv_currency=RUB"
                       + "&date=2025-05-26";
    http.begin(client, dateRateUrl.c_str());
    int httpResCode = http.GET();
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 10);

    if (httpResCode > 0) {
        String payload = http.getString();
        StaticJsonDocument<256> doc;
        auto error = deserializeJson(doc, payload);
        if (error) {
            tft.print("JSON error1");
            Serial.println(error.c_str());
        } else {
            String from = doc["from_currency"].as<const char*>();
            String to   = doc["conv_currency"].as<const char*>();
            float  rate = doc["rate"].as<float>();
            tft.printf("1 %s = %.4f %s", from.c_str(), rate, to.c_str());
        }
    }
    http.end();
    delay(1000);

    String periodRateUrl = String(kekkai_instace)
                         + "/api/getRate/?from_currency=USD"
                         + "&conv_currency=RUB"
                         + "&start_date=2025-05-01"
                         + "&end_date=2025-05-28";
    http.begin(client, periodRateUrl.c_str());
    httpResCode = http.GET();
    if (httpResCode < 200) {
        tft.fillScreen(ST77XX_BLACK);
        tft.setCursor(0, 10);
        tft.printf("API Error: %d", httpResCode);
        http.end();
        return;
    }

    String payload = http.getString();
    DynamicJsonDocument doc(32 * 1024);
    auto error = deserializeJson(doc, payload);
    if (error) {
        Serial.printf("JSON error2: %s\n", error.c_str());
        tft.fillScreen(ST77XX_BLACK);
        tft.setCursor(0, 10);
        tft.print("JSON error2");
        http.end();
        return;
    }

    JsonArray arr = doc.as<JsonArray>();
    size_t ratesLen = arr.size();
    if (ratesLen < 2) {
        tft.fillScreen(ST77XX_BLACK);
        tft.setCursor(0, 10);
        tft.print("No data");
        http.end();
        return;
    }

    float *rates = new float[ratesLen];
    for (size_t i = 0; i < ratesLen; ++i) {
        rates[i] = arr[i]["rate"].as<float>();
    }

    drawGraph(rates, ratesLen,
              8, 32, 144, 80,
              ST77XX_CYAN, ST77XX_WHITE);

    http.end();
    delete[] rates;
}

void loop() {}

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

#define TFT_CS 12
#define TFT_DC 2
#define TFT_RST 25
Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);

const char* ssid            = "NAME_WIFI";
const char* password        = "WIFI_PSWD";
const char* kekkai = "https://kekkai-api.redume.su";

unsigned long previousMillis = 0;
const long interval = 500;
int dotCount = 0;

void drawGraph(const float *val, int len,
               uint16_t x0, uint16_t y0,
               uint16_t w,  uint16_t h,
               uint16_t colorLine, uint16_t colorAxis) {
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
        int x1 = x0 + 1 + (i * (w - 2)) / (len - 1);
        int y1 = y0 + h - 2 - (int)((val[i] - vMin) * (h - 3) / vRange);
        int x2 = x0 + 1 + ((i + 1) * (w - 2)) / (len - 1);
        int y2 = y0 + h - 2 - (int)((val[i + 1] - vMin) * (h - 3) / vRange);
        tft.drawLine(x1, y1, x2, y2, colorLine);
    }

    tft.setTextColor(colorAxis);
    tft.setTextSize(1);
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f", vMax);
    int16_t x1, y1;
    uint16_t bw, bh;
    tft.getTextBounds(buf, 0, 0, &x1, &y1, &bw, &bh);
    tft.setCursor(x0 + w - bw - 1, y0 - bh - 1);
    tft.print(buf);

    snprintf(buf, sizeof(buf), "%.1f", vMin);
    tft.setCursor(x0 + 1, y0 + h + 1);
    tft.print(buf);
}

void setup() {
    Serial.begin(115200);
    tft.initR(INITR_BLACKTAB);
    tft.setRotation(1);
    tft.setSPISpeed(40000000);
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
            previousMillis = currentMillis;
            tft.fillScreen(ST77XX_BLACK);
            tft.setCursor(0, 10);
            tft.print("Connecting");
            for (int i = 0; i < dotCount; ++i) tft.print('.');
            dotCount = (dotCount + 1) % 4;
        }
    }

    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 10);
    tft.print("Connected: ");
    tft.print(WiFi.localIP());

    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    struct tm ti;
    while (!getLocalTime(&ti));
    char endDate[11], startDate[11];
    strftime(endDate, sizeof(endDate), "%Y-%m-%d", &ti);
    ti.tm_mon--;
    mktime(&ti);
    strftime(startDate, sizeof(startDate), "%Y-%m-%d", &ti);

    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 10);

    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    String url = String(kekkai) + "/api/getRate/?from_currency=USD&conv_currency=RUB&date=" + endDate;
    http.begin(client, url.c_str());
    int code = http.GET();
    if (code == HTTP_CODE_OK) {
        DynamicJsonDocument d(1024);
        String p = http.getString();
        deserializeJson(d, p);
        float rate = d["rate"].as<float>();

        tft.printf("1 USD = %.4f RUB", rate);
    }
    http.end();

    url = String(kekkai)
        + "/api/getRate/?from_currency=USD&conv_currency=RUB&start_date=" + startDate
        + "&end_date=" + endDate;
    http.begin(client, url.c_str());
    code = http.GET();
    if (code == HTTP_CODE_OK) {
        DynamicJsonDocument d(32768);
        String p = http.getString();
        deserializeJson(d, p);
        JsonArray a = d.as<JsonArray>();
        int len = a.size();
        float *rates = new float[len];
        for (int i = 0; i < len; ++i) {
            rates[i] = a[i]["rate"].as<float>();
        }
        drawGraph(rates, len, 8, 32, 144, 80, ST77XX_CYAN, ST77XX_WHITE);
        delete[] rates;
    }
    http.end();
}

void loop() {}

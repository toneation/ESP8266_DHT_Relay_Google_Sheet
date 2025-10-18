#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

// CONFIGURATION
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* scriptURL = "https://script.google.com/macros/s/YOUR_SCRIPT_ID/exec";

// HARDWARE SETUP
#define DHT_PIN 2
#define DHT_TYPE DHT11
#define RELAY_PIN 5
#define OLED_SDA 14
#define OLED_SCL 12
#define OLED_ADDR 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define TEMP_THRESHOLD 25.0

// TIMING
const unsigned long readInterval = 10000;
const unsigned long sendInterval = 60000;
unsigned long lastReadTime = 0;
unsigned long lastSendTime = 0;

// OBJECTS
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
float currentTemp = 0.0, currentHum = 0.0;
String currentStatus = "OFF";

// OLED DISPLAY
void printToOLED(String statusMsg, float temp, float hum) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor((SCREEN_WIDTH - 6 * 11) / 2, 0);
  display.print("Fan Status");
  display.setTextSize(2);
  display.setCursor((SCREEN_WIDTH - statusMsg.length() * 12) / 2, 16);
  display.println(statusMsg);
  display.setTextSize(1);
  display.setCursor(0, 44);
  display.print("Temp: "); display.print(temp, 1); display.println(" C");
  display.setCursor(0, 54);
  display.print("Humidity: "); display.print(hum, 1); display.println(" %");
  display.display();
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  dht.begin();
  printToOLED("Set up", 0.0, 0.0);
  delay(3000);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  lastReadTime = millis();
  lastSendTime = millis();
}

void loop() {
  unsigned long now = millis();

  if (now - lastReadTime >= readInterval) {
    lastReadTime = now;
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    if (!isnan(temp) && !isnan(hum)) {
      currentTemp = temp;
      currentHum = hum;
      currentStatus = (temp < TEMP_THRESHOLD) ? "OFF" : "ON";
      digitalWrite(RELAY_PIN, (currentStatus == "ON") ? LOW : HIGH);
      printToOLED(currentStatus, currentTemp, currentHum);
    }
  }

  if (now - lastSendTime >= sendInterval) {
    lastSendTime = now;
    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      WiFiClientSecure client;
      client.setInsecure();
      String url = String(scriptURL) + "?temp=" + String(currentTemp, 1) + "&hum=" + String(currentHum, 1) + "&status=" + currentStatus;
      http.begin(client, url);
      int httpCode = http.GET();
      if (httpCode > 0) Serial.println("Data sent");
      else Serial.printf("HTTP Error: %s\n", http.errorToString(httpCode).c_str());
      http.end();
    }
  }
}

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SoftwareSerial.h>
#include "TFMPlus.h"
#include "TFMiniSensor.h"


const char* ssid = "ESP8266_AP";
const char* password = "12345678";
const char* serverIP = "192.168.4.1"; // IP address of the Access Point
const uint16_t serverPort = 80;
IPAddress apIP(192, 168, 4, 1);
WiFiClient wifiClient;

TFMPlus tfmini;
SoftwareSerial SerialTFMini(12, 13);

void setup() {
  delay(3000); // Wait for 3 seconds
  Serial.begin(115200);
  setupTFMini(SerialTFMini, tfmini);
  initWiFi();
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    // Add the following line to print the current Wi-Fi status code
    Serial.printf(" (WiFi status: %d)\n", WiFi.status());
  }

  Serial.println("\nConnected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}


void sendDistanceToAPBoard(int distance) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "http://" + apIP.toString() + "/speed";
    http.begin(wifiClient, url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    unsigned long timestamp = millis();
    String postData = "DIST1=" + String(distance) + "&TIME1=" + String(timestamp);
    int httpCode = http.POST(postData);
    
    Serial.print("Sending distance to AP board: "); // Add this line
    Serial.println(distance);    
    if (httpCode > 0) {
      Serial.printf("[HTTP] POST... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("Response:");
        Serial.println(payload);
      }
    } else {
      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  }
};

unsigned long printInterval = 5000; // Print every 5000 ms (5 seconds)
unsigned long lastPrintTime = 0;

void loop() {
  int16_t distance = 0;
  int16_t strength = 0;
  int16_t temperature = 0;

  tfmini.getData(distance, strength, temperature);
  unsigned long currentTime = millis();
  if (currentTime - lastPrintTime >= printInterval) {
    Serial.print("STA_Distance: ");
    Serial.println(distance);
    lastPrintTime = currentTime;

  static int prevDistance = 0;
  static unsigned long prevTime = millis();
  unsigned long currentTime = millis();

  if (currentTime - prevTime >= 1000) { // 1000 ms = 1 second
    if (abs(prevDistance - distance) > 50) { // Replace 50 with the minimum change in distance you expect
      sendDistanceToAPBoard(distance);
    }
    prevDistance = distance;
    prevTime = currentTime;
  }
}
}
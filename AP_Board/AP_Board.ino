#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>
#include "TFMPlus.h"
#include <FS.h> // Include the SPIFFS library
#include "TFMiniSensor.h"

// Replace with your network credentials
const char *ssid = "ESP8266_AP";
const char *password = "12345678";

ESP8266WebServer server(80);

TFMPlus tfmini;
SoftwareSerial SerialTFMini(12, 13);
File speedFile;

void initSPIFFS()
{
  SPIFFS.begin();
  if (SPIFFS.exists("/speedData.csv"))
  {
    SPIFFS.remove("/speedData.csv");
  }
  speedFile = SPIFFS.open("/speedData.csv", "w");
  speedFile.println("timestamp,speed,sta_distance,ap_distance");
  speedFile.close();
}

void setup()
{
  delay(3000); // Wait for 3 seconds

  Serial.begin(115200);
  Serial.print("Initializing ...");
  setupTFMini(SerialTFMini, tfmini);
  initWiFi();
  initSPIFFS();
  WiFi.softAP(ssid, password);
  server.on("/download", HTTP_GET, handleFileDownload);
  server.on("/speed", HTTP_POST, handleSpeedData);
  server.on("/", HTTP_GET, handleRoot);
  server.begin();
  Serial.println("Server started");
}

unsigned long printInterval = 5000; // Print every 5000 ms (5 seconds)
unsigned long lastPrintTime = 0;

void loop()
{
  int16_t distance = 0;
  int16_t strength = 0;
  int16_t temperature = 0;

  tfmini.getData(distance, strength, temperature);

  unsigned long currentTime = millis();
  if (currentTime - lastPrintTime >= printInterval) {
    Serial.print("AP_Distance: ");
    Serial.println(distance);
    lastPrintTime = currentTime;
  }

  server.handleClient();
  delay(5);
}


void initWiFi()
{
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  Serial.println("Setting up Access Point...");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
}

void handleSpeedData() {
  static int16_t prevDistance = 0;

  if (server.hasArg("DIST1") && server.hasArg("TIME1")) {
    int staDistance = server.arg("DIST1").toInt();
    unsigned long time1 = server.arg("TIME1").toInt();

    int16_t distance = 0;
    int16_t strength = 0;
    int16_t temperature = 0;
    tfmini.getData(distance, strength, temperature);

    if (abs(distance - prevDistance) > 50) {
      Serial.print("Distance change detected: ");
      Serial.println(distance - prevDistance);
      unsigned long time2 = millis();
      float speed = calculateSpeed(time1, time2, 10);
      storeSpeedData(speed, staDistance, distance);
      prevDistance = distance;
      server.send(200, "text/plain", "Speed data stored");
      Serial.println("Event registered: Speed data stored");
    } else {
      Serial.println("Distance change is less than 50cm, data not stored");
    }
  } else {
    server.send(400, "text/plain", "Missing DIST1 or TIME1 parameter");
    Serial.println("Server response sent: Missing DIST1 or TIME1 parameter");
  }
}

float calculateSpeed(unsigned long time1, unsigned long time2, float distanceBetweenSensors)
{
  float timeElapsed = (time2 - time1) / 1000.0f;      // Convert to seconds
  float speed = distanceBetweenSensors / timeElapsed; // Speed in m/s
  speed = speed * 2.23694;                            // Convert to mph
  return speed;
}

void storeSpeedData(float speed, int staDistance, int apDistance) {
  Serial.println("Speed data route accessed.");
  unsigned long timestamp = millis();
  speedFile = SPIFFS.open("/speedData.csv", "a");
  if (!speedFile) {
    Serial.println("Error opening file for writing");
    return;
  }
  speedFile.print(timestamp);
  speedFile.print(",");
  speedFile.print(speed, 4);
  speedFile.print(",");
  speedFile.print(staDistance);
  speedFile.print(",");
  speedFile.println(apDistance);
  speedFile.close();
  Serial.print("Speed data stored: ");
  Serial.print(timestamp);
  Serial.print("ms, ");
  Serial.print(speed);
  Serial.println("m/s");
}

void handleFileDownload()
{
  Serial.println("Download route accessed.");
  String path = "/speedData.csv";
  if (SPIFFS.exists(path))
  {
    File dataFile = SPIFFS.open(path, "r");
    server.sendHeader("Content-Disposition", "attachment; filename=speedData.csv");
    server.streamFile(dataFile, "text/csv");
    dataFile.close();
  }
  else
  {
    server.send(404, "text/plain", "File not found");
  }
}

void handleRoot()
{
  Serial.println("Root route accessed.");
  server.send(200, "text/plain", "Hello from ESP8266 AP!");
}

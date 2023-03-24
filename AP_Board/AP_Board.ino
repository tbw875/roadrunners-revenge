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
  Serial.begin(115200);
  Serial.println("Initializing ...");
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

void loop()
{
  int distance = 0;
  int strength = 0;
  getTFminiData(SerialTFMini, &distance, &strength);

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.print(" Strength: ");
  Serial.println(strength);
  server.handleClient();

  delay(50);
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

void handleSpeedData()
{
  if (server.hasArg("DIST1") && server.hasArg("TIME1"))
  {
    int dist1 = server.arg("DIST1").toInt();
    unsigned long time1 = server.arg("TIME1").toInt();
    int distance = 0;
    int strength = 0;
    getTFminiData(SerialTFMini, &distance, &strength); // Call the function without an if statement
    unsigned long time2 = millis();
    float speed = calculateSpeed(time1, time2, 10);
    storeSpeedData(speed, dist1, distance);
    server.send(200, "text/plain", "Speed data stored");
  }
  else
  {
    server.send(400, "text/plain", "Missing DIST1 or TIME1 parameter");
  }
}

float calculateSpeed(unsigned long time1, unsigned long time2, float distanceBetweenSensors)
{
  float timeElapsed = (time2 - time1) / 1000.0f;      // Convert to seconds
  float speed = distanceBetweenSensors / timeElapsed; // Speed in m/s
  speed = speed * 2.23694;                            // Convert to mph
  return speed;
}

void storeSpeedData(float speed, int staDistance, int apDistance)
{
  Serial.println("Speed data route accessed.");
  unsigned long timestamp = millis();
  speedFile = SPIFFS.open("/speedData.csv", "a");
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

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Ticker.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <Adafruit_NeoPixel.h>
#include "DHT.h"
#include "config.h"

Ticker ticker;
WiFiClient espClient;
#define DHTPIN 12
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define MAINLEDPIN 14 // D5
#define MAINLEDS 1
Adafruit_NeoPixel mainleds(MAINLEDS, MAINLEDPIN, NEO_GRB + NEO_KHZ800);

void tick() {
  //toggle state
  int state = digitalRead(2);
  digitalWrite(2, !state);
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode...");
  Serial.println(WiFi.softAPIP());
  ticker.detach();
  ticker.attach(0.1, tick);
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setup() {
  pinMode(2, OUTPUT);
  digitalWrite(2, 1);

  /* Wifi setup */
  ticker.attach(0.7, tick);
  Serial.begin(115200);

  // Initialize WiFiManager and then destroy it after it's used
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setConnectTimeout(20);

  // Enable to hard reset saved settings
  //wifiManager.resetSettings();

  // Set the SSID name
  wifiManager.autoConnect("NodeMCU");
  Serial.println("Connected to Wifi.");

  ticker.detach();

  mainleds.clear();
  mainleds.begin();
  mainleds.setBrightness(60);
  mainleds.show();

  // Initialize any endpoints here
  digitalWrite(2, HIGH); // HIGH = off FWIW...
  dht.begin();
}

void loop() {
  int h = dht.readHumidity(); // Humidity
  int t = dht.readTemperature(); // Cel
  int f = dht.readTemperature(true); // F

  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.println(f);

  mainleds.clear();

  if(f < 32) {
    mainleds.setPixelColor(0, mainleds.Color(9, 229, 255));
  } else if(f < 42) {
    mainleds.setPixelColor(0, mainleds.Color(9, 255, 194));
  } else if(f < 60) {
    mainleds.setPixelColor(0, mainleds.Color(9, 255, 84));
  } else if(f < 70) {
    mainleds.setPixelColor(0, mainleds.Color(9, 229, 255));
  } else if(f < 80) {
    mainleds.setPixelColor(0, mainleds.Color(229, 255, 9));
  } else if(f < 90) {
    mainleds.setPixelColor(0, mainleds.Color(255, 235, 9));
  } else if(f < 100) {
    mainleds.setPixelColor(0, mainleds.Color(255, 183, 9));
  } else if(f >= 100) {
    mainleds.setPixelColor(0, mainleds.Color(255, 0, 0));
  };

  mainleds.setBrightness(80);
  mainleds.show();

  HTTPClient http;
  String PushBoxHTTP = String("http://api.pushingbox.com/pushingbox?devid=") + PUSHINGBOXDEVID + String("&room=garage&temperature=") + f + String("&humidity=") + h;

  if (http.begin(PushBoxHTTP)) {
    int httpCode = http.GET();

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("HTTP Get succeeded Payload: " + payload);
    }

    http.end();
  } else {
    Serial.printf("[HTTP] Unable to connect\n");
  }

  Serial.println("");
  delay(600000);
}
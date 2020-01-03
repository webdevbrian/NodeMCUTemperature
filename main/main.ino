#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Ticker.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <Adafruit_NeoPixel.h>
#include <TaskScheduler.h>
#include "DHT.h"
#include "config.h"

// Ticker, temperature sensor and start wificlient
Ticker ticker;
WiFiClient espClient;
#define DHTPIN 12
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// SMTP Settings
char server[] = "mail.smtp2go.com";

// LED
#define MAINLEDPIN 14 // D5
#define MAINLEDS 1
Adafruit_NeoPixel mainleds(MAINLEDS, MAINLEDPIN, NEO_GRB + NEO_KHZ800);

// Register tasks
Scheduler runner;
void checkTempCallback();
Task checkTemp(1200000, TASK_FOREVER, &checkTempCallback); // Run every 20 minutes

// Global temp variables
int h; // Humidity
int t; // Temperature in C
int f; // Temperature in F

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
  runner.init();
  pinMode(2, OUTPUT);
  digitalWrite(2, 1);

  // Wifi Setup
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

  // Initialize LED
  mainleds.clear();
  mainleds.begin();
  mainleds.setBrightness(60);
  mainleds.show();

  // Turn off onboard LED and turn on the temperature sensor
  digitalWrite(2, HIGH); // HIGH = off FWIW...
  dht.begin();

  // Start the runner task(s) and enable checkTemp()
  runner.startNow();
  runner.addTask(checkTemp);
  checkTemp.enable();
}

void checkTempCallback() {
  h = dht.readHumidity(); // Humidity
  t = dht.readTemperature(); // Cel
  f = dht.readTemperature(true); // F

  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(f);
  Serial.println("F");
  mainleds.clear();

  if(f < 32) {
    mainleds.setPixelColor(0, mainleds.Color(9, 229, 255));
  } else if(f < 42) {
    byte ret = sendEmail();
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
}

void loop() {
  runner.execute();
}

byte sendEmail() {
  if (espClient.connect(server, 2525) == 1) 
  {
    Serial.println(F("connected"));
  } 
  else 
  {
    Serial.println(F("connection failed"));
    return 0;
  }
  if (!emailResp()) 
    return 0;
  //
  Serial.println(F("Sending EHLO"));
  espClient.println("EHLO www.example.com");
  if (!emailResp()) 
    return 0;
  //
  /*Serial.println(F("Sending TTLS"));
  espClient.println("STARTTLS");
  if (!emailResp()) 
  return 0;*/
  //  
  Serial.println(F("Sending auth login"));
  espClient.println("AUTH LOGIN");
  if (!emailResp()) 
    return 0;
  //  
  Serial.println(F("Sending User"));
  // Change this to your base64, ASCII encoded username
  /*
  For example, the email address test@gmail.com would be encoded as dGVzdEBnbWFpbC5jb20=
  */
  espClient.println(SMTPUSER); //base64, ASCII encoded Username
  if (!emailResp()) 
    return 0;
  //
  Serial.println(F("Sending Password"));
  // change to your base64, ASCII encoded password
  /*
  For example, if your password is "testpassword" (excluding the quotes),
  it would be encoded as dGVzdHBhc3N3b3Jk
  */
  espClient.println(SMTPPASSWORD);//base64, ASCII encoded Password
  if (!emailResp()) 
    return 0;
  //
  Serial.println(F("Sending From"));
  // Sender email address
  espClient.println(F("MAIL From: phrozen755@gmail.com"));
  if (!emailResp()) 
    return 0;
  // Recipient address
  Serial.println(F("Sending To"));
  espClient.println(F("RCPT To: webdevbrian@gmail.com"));
  if (!emailResp()) 
    return 0;
  //
  Serial.println(F("Sending DATA"));
  espClient.println(F("DATA"));
  if (!emailResp()) 
    return 0;
  Serial.println(F("Sending email"));
  // Recipient address
  espClient.println(F("To:  webdevbrian@gmail.com"));
  // From address
  espClient.println(F("From: phrozen755@gmail.com"));
  espClient.println(F("Subject: Garage is below 42F!\r\n"));
  espClient.print(F("The current temperature in the garage is "));
  espClient.print((f));
  espClient.print(F("F at "));
  espClient.print((h));
  espClient.println(F("% humidity"));
  // This last . is needed
  espClient.println(F("."));
  if (!emailResp()) 
    return 0;
  //
  Serial.println(F("Sending QUIT"));
  espClient.println(F("QUIT"));
  if (!emailResp()) 
    return 0;
  //
  espClient.stop();
  Serial.println(F("disconnected"));
  return 1;
}

byte emailResp()
{
  byte responseCode;
  byte readByte;
  int loopCount = 0;

  while (!espClient.available()) 
  {
    delay(1);
    loopCount++;
    // Wait for 20 seconds and if nothing is received, stop.
    if (loopCount > 20000) 
    {
      espClient.stop();
      Serial.println(F("\r\nTimeout"));
      return 0;
    }
  }

  responseCode = espClient.peek();
  while (espClient.available())
  {
    readByte = espClient.read();
    Serial.write(readByte);
  }

  if (responseCode >= '4')
  {
    //  efail();
    return 0;
  }
  return 1;
}
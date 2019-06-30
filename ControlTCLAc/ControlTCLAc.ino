/*
*
* An IR LED circuit *MUST* be connected to the ESP8266 on a pin
* as specified by kIrLed below.
*
* TL;DR: The IR LED needs to be driven by a transistor for a good result.
*
* Suggested circuit:
*     https://github.com/markszabo/IRremoteESP8266/wiki#ir-sending
*     
*     
*/

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Tcl.h>
#include <WiFi.h>
#include <IotWebConf.h>
#include <PubSubClient.h>

const char thingName[] = "ESP-AIRCO";
const char wifiInitialApPassword[] = "password123";

#define STRING_LEN 128
#define CONFIG_PIN 16
#define STATUS_PIN 18

DNSServer dnsServer;
WebServer server(80);
IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword);


const uint16_t kIrLed = 4;         // ESP32 GPIO pin the LED
IRTcl112Ac ac(kIrLed);             // Set the GPIO used for sending IR messages

// -- Configuration specific key. The value should be modified if config structure was changed.
#define CONFIG_VERSION "0.01"

void setTemperature() {
  //Serial.println("Set temperature to " + temp);
  //ac.setTemp(temp);
}


void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting ESP Airconditioner controller...");

  // Init config
  iotWebConf.setStatusPin(STATUS_PIN);
  iotWebConf.setConfigPin(CONFIG_PIN);
  iotWebConf.init();

  // Set up required URL handlers
  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });

  Serial.println("Ready.");
  
  
}

void loop() {
  iotWebConf.doLoop();
  
}

void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>ESP-AIRCO</title></head><body><h1>ESP Airco Controller</h1><br />";
  s += "<ul>";
  s += "<li><a href='config'>Configure settings</a></li>";
  s += "</ul></body></html>\n";

  server.send(200, "text/html", s);
}

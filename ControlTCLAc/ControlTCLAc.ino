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


/*************************************************************************     
* Includes
**************************************************************************/
// Libraries
#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Tcl.h>
#include <WiFi.h>
#include <IotWebConf.h>
#include <MQTT.h>
#include <DHT.h>

// Pages
#include "html.h"

/*************************************************************************     
* Constants
**************************************************************************/
const char thingName[] = "ESP-AIRCO";
const char wifiInitialApPassword[] = "password123";  // Obviously should be changed for security purposes. 
const uint16_t kIrLed = 4;         // ESP32 GPIO pin for the IR LED

/*************************************************************************     
* Definitions
**************************************************************************/
#define STRING_LEN 128
#define CONFIG_PIN 16
#define STATUS_PIN 18
#define DHT_TYPE DHT22  // Humidity and temp sensor type
#define DHT_PIN 17 // Humidity and temp sensor pin
#define DHT_INTERVAL 2000

// -- Configuration specific key. The value should be modified if config structure was changed.
#define CONFIG_VERSION "ESPAIRCO"

DNSServer dnsServer;
WebServer server(80);
IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword);
IRTcl112Ac ac(kIrLed);             // Set the GPIO used for sending IR messages
DHT dht(DHT_PIN, DHT_TYPE);

/*************************************************************************     
* Variables
**************************************************************************/
float dhtTmpC = 0;
float dhtTmpF = 0;
float dhtHumd = 0;
int acTemp = 19;
String acFan = "auto";
int acSwingVert = 0;
int acSwingHorz = 0;
int acPower = 0;
String acMode = "auto";


long previousMillis = 0;

void wifiConnected();
void configSaved();

void printState() {
  // Display the settings.
  Serial.println("Panasonic A/C remote is in the following state:");
  Serial.printf("  %s\n", ac.toString().c_str());
  // Display the encoded IR sequence.
  unsigned char* ir_code = ac.getRaw();
  Serial.print("IR Code: 0x");
  for (uint8_t i = 0; i < kTcl112AcStateLength; i++)
    Serial.printf("%02X", ir_code[i]);
  Serial.println();
}


void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting ESP Airconditioner controller...");
  Serial.println("-----------------------------------------");
  Serial.println();
  ac.begin();
  dht.begin();
  

  // Init config
  iotWebConf.setStatusPin(STATUS_PIN);
  iotWebConf.setConfigPin(CONFIG_PIN);
  iotWebConf.setConfigSavedCallback(&configSaved);
  iotWebConf.setWifiConnectionCallback(&wifiConnected);
  iotWebConf.init();

  // Set up required URL handlers
  server.on("/", handleRoot);
  server.on("/api", handleAPI);
  server.on("/docs", handleDocs);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });
  Serial.println();
  Serial.println("-----------------------------------------");
  Serial.println("Startup complete");
  Serial.println("-----------------------------------------");
  
  
}

void loop() {
  iotWebConf.doLoop();

  // DHT sensor read loop
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis > DHT_INTERVAL) {
    previousMillis = currentMillis;
    dhtTmpC = dht.readTemperature();
    dhtTmpF = dht.readTemperature(true);
    dhtHumd = dht.readHumidity();
    
    
  } 
}

void setAC()
{
  Serial.println("Transmitting data to A/C unit.");
  Serial.println("Settings: Mode:" + acMode +  " Temp:" + String(acTemp) + " Fan:" + acFan + " Swing: V:" + String(acSwingVert) + "  H:" + String(acSwingHorz));
  
  // A/C Temperature
  ac.setTemp(acTemp);

  // A/C Mode
  if (acMode == "auto") {
   ac.setMode(kTcl112AcAuto);
  } else if (acMode == "cool") {
   ac.setMode(kTcl112AcCool);
  } else if (acMode == "heat") {
   ac.setMode(kTcl112AcHeat);
  } else if (acMode == "dry") {
   ac.setMode(kTcl112AcDry);
  } else if (acMode == "fan") {
   ac.setMode(kTcl112AcFan);
  }

   // A/C Fan speed
  if (acFan == "auto") {
   ac.setFan(kTcl112AcFanAuto);
  } else if (acFan == "low") {
   ac.setFan(kTcl112AcFanLow);
  } else if (acFan == "medium") {
   ac.setFan(kTcl112AcFanMed);
  } else if (acFan == "high") {
   ac.setFan(kTcl112AcFanHigh);
  }

  // A/C Vertical swing
  if (acSwingVert == 0) {
   ac.setSwingVertical(kTcl112AcSwingVOn);
  } else if (acSwingVert == 1) {
   ac.setSwingVertical(kTcl112AcSwingVOff);
  }

   // A/C Horizontal swing
  if (acSwingHorz == 0) {
   ac.setSwingHorizontal(false);
  } else if (acSwingVert == 1) {
   ac.setSwingHorizontal(true);
  }

  if (acPower == 1) {
    ac.on();
  } else if (acPower == 0) {
    ac.off();
  }

  ac.send();
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
  s += "<title>ESP-AIRCO</title></head><style>" + cssStyle + "</style><body>";
  s += "<script>" + jsFunctions + "</script>";
  s += "<div class='header'><h1>ESP-Airco</h1></div>";
  s += "<div class='content'>";
  s += "<p>Room temperature: <span id='curTemps'>" + String(dhtTmpC) + " °C / " + String(dhtTmpF) + " °F</span></p>";
  s += "<p>Room humidity: <span id='curHum'>" + String(dhtHumd) + "%</span></p>";
  s += "<a href='config' class='button'>Configure settings</a>";
  
  s += "</div>";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
}

void handleAPI()
{
  String srvMethod = server.method() == HTTP_GET ? "GET" : "POST";
  if (srvMethod == "GET") {
    String apiData = "{";
    apiData += "\"ac_power\": " + String(acPower) + ",";
    apiData += "\"ac_temp\": " + String(acTemp) + ",";
    apiData += "\"ac_swing_vert\": " + String(acSwingVert) + ",";
    apiData += "\"ac_swing_horz\": " + String(acSwingHorz) + ",";
    apiData += "\"ac_fan\": \"" + acFan + "\",";
    apiData += "\"current_temp_c\": " + String(dhtTmpC) + ",";
    apiData += "\"current_temp_f\": " + String(dhtTmpF) + ",";
    apiData += "\"current_humidity\": " + String(dhtHumd);
    apiData += "}";
    server.sendHeader("Content-Type", "application/json; charset=utf-8");
    server.send(200, "text/html", apiData);
  }
  else if (srvMethod == "POST") {
    // Unfortunately the iotWebConf lib has no documentation to parse the body of a POST. 
    // Opened an issue on the github repo, if there is an option, will modify.
    // For now, using URL args as a workaround.
    Serial.println("API: Received POST.");
    
    int reqTemp = server.arg("temp").toInt();
    String reqFan = server.arg("fan");
    String reqMode = server.arg("mode");
    int reqPower = server.arg("power").toInt();
    int reqSwingVert = server.arg("swingvert").toInt();
    int reqSwingHorz = server.arg("swinghorz").toInt();

    if ((reqTemp >= 16) && (reqTemp <= 31)) {
       acTemp = reqTemp;
    }
    
    if (reqFan == "auto" || reqFan == "low" || reqFan == "medium" || reqFan == "high" ) {
      acFan = reqFan;
    }

    if (reqMode == "auto" || reqMode == "cool" || reqMode == "heat" || reqMode == "dry" || reqMode == "fan" ) {
      acMode = reqMode;
    }

    if (reqPower == 1 || reqPower == 0 ) {
       acPower = reqPower;
    }
    
    if (reqSwingVert == 1 || reqSwingVert == 0 ) {
       acSwingVert = reqSwingVert;
    }

    if (reqSwingHorz == 1 || reqSwingHorz == 0 ) {
       acSwingHorz = reqSwingHorz;
    }

    // Reply to POST with the current state in JSON format
    String apiData = "{";
    apiData += "\"ac_power\": " + String(acPower) + ",";
    apiData += "\"ac_temp\": " + String(acTemp) + ",";
    apiData += "\"ac_swing_vert\": " + String(acSwingVert) + ",";
    apiData += "\"ac_swing_horz\": " + String(acSwingHorz) + ",";
    apiData += "\"ac_fan\": \"" + acFan + "\",";
    apiData += "\"current_temp_c\": " + String(dhtTmpC) + ",";
    apiData += "\"current_temp_f\": " + String(dhtTmpF) + ",";
    apiData += "\"current_humidity\": " + String(dhtHumd);
    apiData += "}";
    server.sendHeader("Content-Type", "application/json; charset=utf-8");
    server.send(200, "text/html", apiData);

    setAC();
    
  }
}

void handleDocs()
{

  //int reqTemp = server.arg("temp").toInt();
  //  String reqFan = server.arg("fan");
  //  String reqMode = server.arg("mode");
  //  int reqPower = server.arg("power").toInt();
  //  int reqSwingVert = server.arg("swingvert").toInt();
  //  int reqSwingHorz = server.arg("swinghorz").toInt();
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>ESP-AIRCO</title></head><style>" + cssStyle + "</style><body>";
  s += "<div class='header'><h1>ESP-Airco</h1><p>Quick API Documentation</p></div>";
  s += "<div class='content'>";
  s += "<h2>Fetching data</h2>";
  s += "<p>Statistics and data of the current state of the remote are available using the /api call. <br />Data is provided in the JSON format.</p>";
  s += "<h2>POST data</h2>";
  s += "<p>Unfortunately, due to limitations of the ESP webconfig library used, it is not possible to post JSON back to the device.<br />It is however possible to use URL arguments instead. For available arguments, see the table below.</p>";
  s += "<p><strong>Example: </strong>Perform a POST to <i>http://hostname-of-device/api?power=1&amp;temp=20&amp;mode=cool&amp;swingvert=1&amp;swinghorz=1</i> to turn on the A/C, set temp to 20C, cooling mode, and swing horizontally and vertically.</p>";
  s += "<table width=100%><tbody>";
  s += "<tr><td>power</td><td><strong>Type:</strong>Integer<br /><strong>Value:</strong>1 or 0<br /><strong>Result:</strong>Turn A/C on or off.</td></tr>";
  s += "<tr><td>temp</td><td><strong>Type:</strong>Integer<br /><strong>Value:</strong>16 to 31<br /><strong>Result:</strong>Set A/C temperature value.</td></tr>";
  s += "<tr><td>fan</td><td><strong>Type:</strong>String<br /><strong>Value:</strong>auto - low - medium - high<br /><strong>Result:</strong>Set A/C fan speed.</td></tr>";
  s += "<tr><td>mode</td><td><strong>Type:</strong>String<br /><strong>Value:</strong>auto - cool - heat - dry - fan<br /><strong>Result:</strong>Set A/C mode.</td></tr>";
  s += "<tr><td>swingvert</td><td><strong>Type:</strong>Integer<br /><strong>Value:</strong>1 or 0<br /><strong>Result:</strong>Set A/C vertical swing.</td></tr>";
  s += "<tr><td>swinghorz</td><td><strong>Type:</strong>Integer<br /><strong>Value:</strong>1 or 0<br /><strong>Result:</strong>Set A/C horizontal swing.</td></tr>";
  s += "</tbody></table>";
  s += "</div>";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
}

void wifiConnected()
{
  Serial.println("WiFi was connected.");
}

void configSaved()
{
  Serial.println("Configuration was updated.");
}

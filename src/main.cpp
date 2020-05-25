#include <Arduino.h>
// ESP
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
// RTC/Time
#include <Wire.h>
#include <TimeLib.h>
#include <DS3232RTC.h>
// JSON
#include <AsyncJson.h>
#include <ArduinoJson.h>


const char *ssid = "Arrosage";
const char *password = "azerty7532";

float celsius;

AsyncWebServer server(80);

DS3232RTC myRTC(false);

void StartValve(int id) {
  return;
} 

void StopValve(int id) {
  return;
} 

void DeleteCycle(size_t idtodelete) {
  File schedules = SPIFFS.open("/schedules.json", "r");

  if(schedules && schedules.size()) {

    DynamicJsonDocument schedulesjson(1300);
    DeserializationError err = deserializeJson(schedulesjson, schedules);
    Serial.println(err.c_str());
    if (err) {
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(err.c_str());
    }
    else {
      schedulesjson.remove(idtodelete);
      for(int i=0; i<schedulesjson.size(); i++) {
        schedulesjson[i]["id_prog"] = i;
        /* DEBUG
          String id = schedulesjson[i]["id_prog"];
          String name = schedulesjson[i]["name"];
          Serial.println(name + " : " + id);
        */
      }
      schedules.close();
      schedules = SPIFFS.open("/schedules.json", "w");
      serializeJson(schedulesjson, schedules);
      schedules.close();   
    } 
    schedules.close();
  }
  else {
    Serial.println("Impossible de lire le fichier.");
  }
}

void setup() {

  // Serial
  Serial.begin(9600);
  Serial.println();

  // RTC            
  myRTC.begin();      
  // setTime(13, 17, 0, 19, 5, 2020);   
  // myRTC.set(now());                     //set the RTC from the system time
  
  setSyncProvider(myRTC.get);
  
  if (timeStatus() != timeSet) {
    Serial.println("Fail");
  }
  else {
    Serial.println("Success");
  }

  // WiFi Hostpot
  Serial.print("Setting soft-AP ... ");
  boolean result = WiFi.softAP(ssid, password);
  if(result == true)
  {
    Serial.println("Ready");
  }
  else
  {
    Serial.println("Failed!");
  }

  // SPIFFS
  SPIFFS.begin();
    /* A garder au cas où pour tester :)
      File schedules = SPIFFS.open("/schedules.json", "r");
      String text;
      if(schedules && schedules.size()) {
        Serial.println("File Content:");

        while (schedules.available()){
            text += char(schedules.read());
        }
        schedules.close();
        Serial.println("=====================================");
        Serial.println(text);
        Serial.println("=====================================");
      }
      else {
        Serial.println("Impossible de lire le fichier.");
      }

    */

  // Web Server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.on("/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/bootstrap.min.css", "text/css");
  });

  server.on("/bootstrap.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/bootstrap.min.js", "text/javascript");
  });

  server.on("/jquery.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/jquery.min.js", "text/javascript");
  });

  server.on("/popper.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/popper.min.js", "text/javascript");
  });

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/script.js", "text/javascript");
  });

  server.on("/valves.json", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/valves.json", "application/json");
  });

  server.on("/schedules.json", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/schedules.json", "application/json");
  });

  server.on("/DeleteCycle", HTTP_POST, [](AsyncWebServerRequest *request) {
    String msg;
    if(request->hasParam("id", true)) {
      msg = request->getParam("id", true)->value();
      int idtodelete = msg.toInt();
      Serial.println("ID à supprimer : " + String(idtodelete));
      DeleteCycle(idtodelete);
    }
    request->send(204);
  });

  server.on("/datetime.json", HTTP_GET, [](AsyncWebServerRequest *request) {
    celsius = myRTC.temperature() / 4.0;
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    const int dtcapacity = JSON_OBJECT_SIZE(9);
    StaticJsonDocument<dtcapacity> root;
    root["dayofweek"] = weekday();;
    root["day"] = day();
    root["month"] = month();
    root["year"] = year();
    root["hour"] = hour();
    root["minute"] = minute();
    root["sec"] = second();
    root["temperature"] = celsius;
    serializeJson(root, *response);
    request->send(response);
  });

  server.begin();
  Serial.println("Serveur HTTP Async Actif !");
}

void loop() {

}



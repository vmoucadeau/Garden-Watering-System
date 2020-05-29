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

const int ActiveRelayPin = 15;
boolean ActiveRelay = true;

const char *ssid = "Arrosage";
const char *password = "azerty7532";

float celsius;

AsyncWebServer server(80);

DS3232RTC myRTC(false);

DynamicJsonDocument schedulesjson(2048);
JsonArray schedulesarray = schedulesjson.to<JsonArray>();


DynamicJsonDocument valvesjson(1024);
JsonArray valvesarray = valvesjson.to<JsonArray>();


void StartValve(int id) {
  return;
} 

void StopValve(int id) {
  return;
} 

void DeleteCycle(size_t idtodelete) {
  File schedules = SPIFFS.open("/schedules.json", "r");

  if(schedules && schedules.size()) {
    
    DeserializationError err = deserializeJson(schedulesjson, schedules);
    Serial.println(err.c_str());
    if (err) {
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(err.c_str());
    }
    else {
      schedulesjson.remove(idtodelete);
      int i = 0;
      for(JsonObject loop : schedulesarray) {
        loop["id_prog"] = i;
        i++;
        /* DEBUG
          String id = schedulesjson[i]["id_prog"];
          String name = schedulesjson[i]["name"];
          Serial.println(name + " : " + id);
        */
      }
      schedules.close();
      schedules = SPIFFS.open("/schedules.json", "w");
      serializeJson(schedulesjson, schedules);
      schedulesjson.clear();
      schedules.close();   
    } 
    schedules.close();
  }
  else {
    Serial.println("Impossible de lire le fichier.");
  }
}

void AddCycle(String name, int id_ev, int starth, int startm, int endh, int endm, JsonObject daysjson, int temp) {
  File schedules = SPIFFS.open("/schedules.json", "r");

  if(schedules && schedules.size()) {

    DeserializationError err = deserializeJson(schedulesjson, schedules);
    Serial.println(err.c_str());
    if (err) {
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(err.c_str());
    }
    else {
    
      int id_prog = schedulesjson.size();
      JsonObject schedule = schedulesarray.createNestedObject();
      schedule["name"] = name;
      schedule["id_ev"] = id_ev;
      schedule["id_prog"] = id_prog;
      schedule["Hourstart"] = starth;
      schedule["Minstart"] = startm;
      schedule["Hourstop"] = endh;
      schedule["Minstop"] = endm;
      schedule["daysActive"] = daysjson;
      schedule["temporary"] = temp ? true : false;

      /* DEBUG
      Serial.println("Valeur de schedulesjson : ");
      serializeJsonPretty(schedulesjson, Serial);
      Serial.println(" ");
      for(JsonObject loop : schedulesarray) {
        String id = loop["id_prog"];
        String name = loop["name"];
        Serial.println(name + " : " + id);
      }
      */
      schedules.close();
      schedules = SPIFFS.open("/schedules.json", "w");
      serializeJson(schedulesjson, schedules);
      schedulesjson.clear();
      schedules.close();   
    } 
    schedules.close();
  }
  else {
    Serial.println("Impossible de lire le fichier.");
  }
}



void AddValve(String name, String type, int startpin, int Hpin1, int Hpin2, String starturl, String stopurl) {
  File valves = SPIFFS.open("/valves.json", "r");

  if(valves && valves.size()) {
    
    DeserializationError err = deserializeJson(valvesjson, valves);
    Serial.println(err.c_str());
    
    if (err) {
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(err.c_str());
    }
    
    else {
      int id_ev = valvesjson.size();
      JsonObject valve = valvesarray.createNestedObject();
      
      valve["name"] = name;
      valve["id_ev"] = id_ev;
      
      if(type == "local") {
        valve["type"] = 0;
        valve["startpin"] = startpin;
      }
      else if(type == "locallatching") {
        valve["type"] = 2;
        valve["Hpin1"] = Hpin1;
        valve["Hpin2"] = Hpin2;
      }
      else if(type == "distante") {
        valve["type"] = 1;
        valve["starturl"] = starturl;
        valve["stopurl"] = stopurl;
      }
      else {
        return;
      }
      valve["state"] = false;
      
      /* DEBUG
      Serial.println("Valeur de valvesjson : ");
      serializeJsonPretty(valvesjson, Serial);
      Serial.println(" ");
      for(JsonObject loop : valvesarray) {
        String id = loop["id_ev"];
        String name = loop["name"];
        Serial.println(name + " : " + id);
      }
      */
         
      valves.close();
      valves = SPIFFS.open("/valves.json", "w");
      serializeJson(valvesjson, valves);
      valvesjson.clear();
      valves.close();
      
    } 
    valves.close();
    
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

  // ActiveRelay
  if(ActiveRelay) {
    Serial.println("Relais activé.");
    pinMode(ActiveRelayPin, OUTPUT);
    digitalWrite(ActiveRelayPin, LOW);
  }

  // Pins init
  pinMode(16, OUTPUT);
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  digitalWrite(16, LOW);
  digitalWrite(0, LOW);
  digitalWrite(2, HIGH);
  digitalWrite(14, LOW);
  digitalWrite(12, LOW);
  digitalWrite(13, LOW);

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
  server.on("/AddCycle", HTTP_POST, [](AsyncWebServerRequest *request) {
    
    if(request->hasParam("name", true) && request->hasParam("id_ev", true) && request->hasParam("starth", true) && request->hasParam("startm", true) && request->hasParam("endh", true) && request->hasParam("endm", true) && request->hasParam("monday", true) && request->hasParam("tuesday", true) && request->hasParam("wednesday", true) && request->hasParam("thursday", true) && request->hasParam("friday", true) && request->hasParam("saturday", true) && request->hasParam("sunday", true) && request->hasParam("temporary", true) ) {
      String name = request->getParam("name", true)->value();
      int id_ev = request->getParam("id_ev", true)->value().toInt();
      int starth = request->getParam("starth", true)->value().toInt();
      int startm = request->getParam("startm", true)->value().toInt();
      int endh = request->getParam("endh", true)->value().toInt();
      int endm = request->getParam("endm", true)->value().toInt();

      int monday = request->getParam("monday", true)->value().toInt();
      int tuesday = request->getParam("tuesday", true)->value().toInt();
      int wednesday = request->getParam("wednesday", true)->value().toInt();
      int thursday = request->getParam("thursday", true)->value().toInt();
      int friday = request->getParam("friday", true)->value().toInt();
      int saturday = request->getParam("saturday", true)->value().toInt();
      int sunday = request->getParam("sunday", true)->value().toInt();

      int temporary = request->getParam("temporary", true)->value().toInt();

      StaticJsonDocument<600> doc;
      JsonObject days = doc.to<JsonObject>();
      
      days["monday"] = monday ? true : false;
      days["tuesday"] = tuesday ? true : false;
      days["wednesday"] = wednesday ? true : false;
      days["thursday"] = thursday ? true : false;
      days["friday"] = friday ? true : false;
      days["saturday"] = saturday ? true : false;
      days["sunday"] = sunday ? true : false;


      /*
      Serial.println("Valeur reçues : ");
      Serial.println("Nom cycle : " + name);
      Serial.println("Vanne : " + String(id_ev));
      Serial.println("Temporaire : " + String(temporary));
      Serial.println("Heure début : " + String(starth) + "h" + String(startm));
      Serial.println("Heure fin : " + String(endh) + "h" + String(endm));
      Serial.println("Jours : ");
      serializeJsonPretty(days, Serial);
      */

      AddCycle(name, id_ev, starth, startm, endh, endm, days, temporary);

    }
    
    request->send(204);
  });


  server.on("/AddValve", HTTP_POST, [](AsyncWebServerRequest *request) {
    
    if(request->hasParam("name", true) && request->hasParam("type", true)) {
      String name = request->getParam("name", true)->value();
      int type = request->getParam("type", true)->value().toInt();
      if(type == 0) {
        if(request->hasParam("startpin", true)) {
          int startpin = request->getParam("startpin", true)->value().toInt();
          Serial.println("Ajout d'une valve locale : ");
          Serial.println("StartPin : " + String(startpin));
          AddValve(name, "local", startpin, 0, 0, "", "");
        }
      }
      if(type == 1) {
        if(request->hasParam("starturl", true) && request->hasParam("stopurl", true)) {
          String starturl = request->getParam("starturl", true)->value();
          String stopurl = request->getParam("stopurl", true)->value();
          Serial.println("Ajout d'une valve distante : ");
          Serial.println("StartURL : " + String(starturl));
          Serial.println("EndURL : " + String(stopurl));
          AddValve(name, "distante", 0, 0, 0, starturl, stopurl);
        }
      }
      if(type == 2) {
        if(request->hasParam("Hpin1", true) && request->hasParam("Hpin2", true)) {
          int Hpin1 = request->getParam("Hpin1", true)->value().toInt();
          int Hpin2 = request->getParam("Hpin2", true)->value().toInt();
          Serial.println("Ajout d'une valve avec pont en H : ");
          Serial.println("Hpin1 : " + String(Hpin1));
          Serial.println("Hpin2 : " + String(Hpin2));
          AddValve(name, "locallatching", 0, Hpin1, Hpin2, "", "");
        }
      }

      

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



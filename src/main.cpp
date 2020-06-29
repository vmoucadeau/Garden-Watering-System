#include <Arduino.h>
// ESP
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
// RTC/Time
// #include <Wire.h> // Useless?
#include <TimeLib.h>
#include <DS3232RTC.h>
#include <NTPClientLib.h>
#include <millisDelay.h>
// JSON
#include <ArduinoJson.h>
#include <AsyncJson.h>

boolean DEBUG = false;
boolean invertHbridgelogic = false;

// Temporary
int Hpin1tostop;
int Hpin2tostop;

millisDelay CloseValveDelay;

float celsius;

AsyncWebServer server(80);

// RTC
DS3232RTC myRTC(false);

// Check Cycles
const int checkcycleinterval = 5000;
millisDelay CheckCyclesDelay;

// JSON Documents
DynamicJsonDocument schedulesjson(2048);
JsonArray schedulesarray = schedulesjson.to<JsonArray>();

DynamicJsonDocument valvesjson(1024);
JsonArray valvesarray = valvesjson.to<JsonArray>();

StaticJsonDocument<300> configjson;
JsonArray configarray = configjson.to<JsonArray>();


void DebugSerial(String msg) {
  if(DEBUG) {
    Serial.println("[DEBUG]: " + msg);
  }
}

boolean CheckDay(JsonObject daystotest) {
  // Works, but it's not clean... I know.
  if(weekday() == 1) {
    if(daystotest["sunday"]) {
      return true;
    }
  }
  if(weekday() == 2) {
    if(daystotest["monday"]) {
      return true;
    }
  }
  if(weekday() == 3) {
    if(daystotest["tuesday"]) {
      return true;
    }
  }
  if(weekday() == 4) {
    if(daystotest["wednesday"]) {
      return true;
    }
  }
  if(weekday() == 5) {
    if(daystotest["thursday"]) {
      return true;
    }
  }
  if(weekday() == 6) {
    if(daystotest["friday"]) {
      return true;
    }
  }
  if(weekday() == 7) {
    if(daystotest["saturday"]) {
      return true;
    }
  }
  return false;
}

boolean CheckHours(int starth, int startm, int endh, int endm) {
  int minstart = starth * 60 + startm;
  int minend = endh * 60 + endm;
  int mincurrent = hour() * 60 + minute();
  if(mincurrent >= minstart && mincurrent < minend) {
    return true;
  }
  else {
    return false;
  }
}

boolean StartValve(int id_ev) {
  boolean success = false;
  String test = valvesarray[0]["name"];
  if(test != "null") {
      for(JsonObject loop : valvesarray) {
        String id = loop["id_ev"];
        if(id.toInt() == id_ev) {
          String started = loop["state"];
          if(started == "false") {
            String name = loop["name"];
            String type = loop["type"];
            if(type.toInt() == 0) {
              String startpin = loop["startpin"];
              digitalWrite(startpin.toInt(), HIGH);
              success = true;
            }
            else if(type.toInt() == 1) {
              String url = loop["starturl"];
              HTTPClient http;
              http.begin(url);
              int httpCode = http.GET();
              if(httpCode == 200 || httpCode == 204) {
                success = true;
              }
              http.end();
            }
            else if(type.toInt() == 2) {
              
              String Hpin1 = loop["Hpin1"];
              String Hpin2 = loop["Hpin2"];
              if(invertHbridgelogic) {
                digitalWrite(Hpin1.toInt(), LOW);
                digitalWrite(Hpin2.toInt(), HIGH);
              }
              else {
                digitalWrite(Hpin1.toInt(), HIGH);
                digitalWrite(Hpin2.toInt(), LOW);
              }
              Hpin1tostop = Hpin1.toInt();
              Hpin2tostop = Hpin2.toInt();
              delay(500);
              CloseValveDelay.start(1500);
              success = true;
            }
            if(success) {
              loop["state"] = true;
            }
          }
          else {
            success = true;
          }  
        }
      }
  }
  else {
    DebugSerial("valvesjson Error.");
  }
  return success;
} 

boolean StopValve(int id_ev, boolean forcestop = false, boolean startup = false) {
  boolean success = false;
  String test = valvesarray[0]["name"];
  if(test != "null") {
      for(JsonObject loop : valvesarray) {
        String id = loop["id_ev"];
        if(id.toInt() == id_ev) {
          String started = loop["state"];
          if(started == "true" || forcestop) {
            String name = loop["name"];
            String type = loop["type"];
            if(type.toInt() == 0) {
              String startpin = loop["startpin"];
              digitalWrite(startpin.toInt(), LOW);
              success = true;
            }
            else if(type.toInt() == 1) {
              String url = loop["stopurl"];
              HTTPClient http;
              http.begin(url);
              int httpCode = http.GET();
              if(httpCode == 200 || httpCode == 204) {
                success = true;
              }
              http.end();
            }
            else if(type.toInt() == 2) {
              String Hpin1 = loop["Hpin1"];
              String Hpin2 = loop["Hpin2"];
              
              if(invertHbridgelogic) {
                digitalWrite(Hpin1.toInt(), HIGH);
                digitalWrite(Hpin2.toInt(), LOW);
              }
              else {
                digitalWrite(Hpin1.toInt(), LOW);
                digitalWrite(Hpin2.toInt(), HIGH);
              }
              if(startup) {
                delay(1500);
                digitalWrite(Hpin1.toInt(), LOW);
                digitalWrite(Hpin2.toInt(), LOW);
              }
              else {
                Hpin1tostop = Hpin1.toInt();
                Hpin2tostop = Hpin2.toInt();
                delay(500);
                CloseValveDelay.start(1500);
              }
              success = true;
            }
            if(success) {
              loop["state"] = false;
            }
          }
          else {
            success = true;
          }
        }
      }
  }
  else {
    DebugSerial("Error - valvesjson");
  }
  return success;
} 

void DeleteCycle(int idtodelete) {
  String test = schedulesjson[0]["name"];
  if(test != "null") {
    int id_ev = schedulesjson[idtodelete]["id_ev"];
    String state = valvesjson[id_ev]["state"];
    if(state == "true") {
      DebugSerial("Stopping valve before deleting cycle."); // This is not working and I don't know why.
      
      if(!StopValve(idtodelete, true)) {
        DebugSerial("Can't stop valve.");
        return;
      }
      
      
    }
    
    schedulesjson.remove(idtodelete);
    int i = 0;
    for(JsonObject loop : schedulesarray) {
      loop["id_prog"] = i;
      i++;
    }
    File schedules = SPIFFS.open("/schedules.json", "w");
    serializeJson(schedulesjson, schedules);
    schedules.close();   
    DebugSerial("Cycle deleted.");
    
  }
  else {
    DebugSerial("Impossible de lire le fichier.");
  }
}

void AddCycle(String name, int id_ev, int starth, int startm, int endh, int endm, JsonObject daysjson, boolean temp) { 
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
  schedule["temporary"] = temp;
  schedule["state"] = false;

  File schedules = SPIFFS.open("/schedules.json", "w");
  serializeJson(schedulesjson, schedules);
  schedules.close(); 
  return;  
}

void DeleteValve(int idtodelete) {
  String test = valvesjson[0]["name"];
  if(test != "null") {
    String state = valvesjson[idtodelete]["state"];
    if(state == "true") {
      DebugSerial("Stopping valve before deleting valve.");
      
      if(!StopValve(idtodelete, true)) {
        DebugSerial("Can't stop valve.");
        return;
      }
      
    }
    for(JsonObject valvecycle : schedulesarray) {
      if(valvecycle["id_ev"] == idtodelete) {
        int idcycle = valvecycle["id_prog"];
        String cyclename = valvecycle["name"];
        DebugSerial("Deleting valve cycle : " + cyclename);
        DeleteCycle(idcycle);
      }
    }
    valvesjson.remove(idtodelete);
    int i = 0;
    for(JsonObject loop : valvesarray) {
      loop["id_ev"] = i;
      i++;
    }
    File valves = SPIFFS.open("/valves.json", "w");
    serializeJson(valvesjson, valves); 
    valves.close(); 
  }
  else {
    DebugSerial("Erreur : DeleteValve");
  } 
  return;
}

void AddValve(String name, String type, int startpin, int Hpin1, int Hpin2, String starturl, String stopurl) {
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
      
  File valves = SPIFFS.open("/valves.json", "w");
  serializeJson(valvesjson, valves);
  valves.close();
  return;
}

void SetWiFi(WiFiMode mode, const char *ssid, const char *password) {
  WiFi.disconnect(); // Useless?
  WiFi.mode(mode);
  if(mode == WIFI_AP) {
    if(WiFi.softAP(ssid, password)) {
      DebugSerial("WiFi Access Point started");
    }
  }
  if(mode == WIFI_STA) {
    WiFi.begin(ssid, password);
    DebugSerial("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    Serial.println(" ");
    DebugSerial("Connected to WiFi");
    WiFi.setAutoReconnect(true);
  }
}


void setup() {

  // Serial
  Serial.begin(9600);
  Serial.println();

  // SPIFFS
  SPIFFS.begin();

  File schedules = SPIFFS.open("/schedules.json", "r");
  DeserializationError err = deserializeJson(schedulesjson, schedules);
  if(err) {
    DebugSerial("Erreur deserialization");
  }
  schedules.close();

  File valves = SPIFFS.open("/valves.json", "r");
  DeserializationError err2 = deserializeJson(valvesjson, valves);
  if(err2) {
    DebugSerial("Erreur deserialization");
  }
  valves.close();

  File config = SPIFFS.open("/config.json", "r");
  DeserializationError err3 = deserializeJson(configjson, config);
  if(err3) {
    DebugSerial("Erreur deserialization");
  }
  config.close();

  // WiFi
  String wifimode = configarray[0]["wifi"]["mode"];
  const char *ssid = configarray[0]["wifi"]["ssid"];
  const char *password = configarray[0]["wifi"]["password"];

  if(wifimode == "STA") {
    SetWiFi(WIFI_STA, ssid, password);
  }
  else if(wifimode == "AP") {
    SetWiFi(WIFI_AP, ssid, password);
  }


  // Time
  myRTC.begin();
  String timeprovider = configarray[0]["time"]["syncprovider"];
  if(timeprovider == "rtc"){
    
    setSyncProvider(myRTC.get);
    DebugSerial("Time Sync Provider : RTC");
  }
  else if(timeprovider == "ntp") {
    int timezone = configarray[0]["time"]["timezone"];
    boolean summertime = configarray[0]["time"]["summertime"];
    String ntpserver = configarray[0]["time"]["ntpserver"];
    NTP.begin(ntpserver, timezone, summertime);
    NTP.setInterval(300);
    DebugSerial("Time Sync Provider : NTP");
  }


  // Pins init, for now, you need to replace with your pins if you don't have an ESP12E
  // With an ESP12E, you can control 4 classic valves with relays localy and 2 latching valves (with L293D or two relays per valves)
  pinMode(16, OUTPUT);
  pinMode(0, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  digitalWrite(2, HIGH); // To switch off the builtin LED

  delay(500);

  for (JsonObject loop : valvesarray) {
    String id_ev = loop["id_ev"];
    String name = loop["name"];
    DebugSerial("Stopping valve : " + name);
    StopValve(id_ev.toInt(), true, true);
  }

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
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    serializeJson(valvesjson, *response);
    request->send(response);
  });

  server.on("/schedules.json", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    serializeJson(schedulesjson, *response);
    request->send(response);
  });

  server.on("/config.json", HTTP_GET, [](AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    serializeJson(configjson, *response);
    request->send(response);
  });

  server.on("/SetRTCTime", HTTP_POST, [](AsyncWebServerRequest *request) {
    if(request->hasParam("timestamp", true)) {
      int timestamp = request->getParam("timestamp", true)->value().toInt();
      time_t timetoset = timestamp;
      setTime(timetoset);   
      delay(100);
      myRTC.set(now()); 
      configarray[0]["time"]["syncprovider"] = "rtc";  
      File config = SPIFFS.open("/config.json", "w");
      serializeJson(configjson, config);
      config.close(); 
      setSyncProvider(myRTC.get);
    }
    request->send(204);
  });

  server.on("/SetNTPTime", HTTP_POST, [](AsyncWebServerRequest *request) {
    if(request->hasParam("timezone", true) && request->hasParam("ntpserver", true) && request->hasParam("summertime", true)) {
      int timezone = request->getParam("timezone", true)->value().toInt();
      int smt = request->getParam("summertime", true)->value().toInt();
      String ntpserver = request->getParam("ntpserver", true)->value();
      boolean summertime = false;
      if(smt == 1) {summertime = true;}

      NTP.begin(ntpserver, timezone, summertime);
      NTP.setInterval(300);
      configarray[0]["time"]["syncprovider"] = "ntp"; 
      configarray[0]["time"]["ntpserver"] = ntpserver;
      configarray[0]["time"]["timezone"] = timezone; 
      configarray[0]["time"]["summertime"] = summertime;
      File config = SPIFFS.open("/config.json", "w");
      serializeJson(configjson, config);
      config.close(); 
      DebugSerial("NTP Time set : ");
      DebugSerial("NTP Server : " + ntpserver);
      DebugSerial("Timezone : " + String(timezone));
    }
    request->send(204);
  });

  server.on("/SetWiFiMode", HTTP_POST, [](AsyncWebServerRequest *request) {
    if(request->hasParam("mode", true) && request->hasParam("ssid", true) && request->hasParam("password", true)) {
      String mode = request->getParam("mode", true)->value();
      String ssid = request->getParam("ssid", true)->value();
      String password = request->getParam("password", true)->value();

      configarray[0]["wifi"]["ssid"] = ssid;
      configarray[0]["wifi"]["password"] = password; 

      DebugSerial("WiFi SSID : " + ssid);
      DebugSerial("WiFi Password : " + password);

      if(mode == "station") {
        configarray[0]["wifi"]["mode"] = "STA";
      }
      if(mode == "ap") {
        configarray[0]["wifi"]["mode"] = "AP";
      }
      
      File config = SPIFFS.open("/config.json", "w");
      serializeJson(configjson, config);
      config.close(); 
    }
    request->send(204);
  });
  
  server.on("/DeleteCycle", HTTP_POST, [](AsyncWebServerRequest *request) {
    if(request->hasParam("id", true)) {
      int idtodelete = request->getParam("id", true)->value().toInt();
      int id_ev = schedulesarray[idtodelete]["id_ev"];
      DebugSerial("Cycle à supprimer : " + String(idtodelete));
      StopValve(id_ev);
      delay(500);
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

      // Not very clean:
      int monday = request->getParam("monday", true)->value().toInt();
      int tuesday = request->getParam("tuesday", true)->value().toInt();
      int wednesday = request->getParam("wednesday", true)->value().toInt();
      int thursday = request->getParam("thursday", true)->value().toInt();
      int friday = request->getParam("friday", true)->value().toInt();
      int saturday = request->getParam("saturday", true)->value().toInt();
      int sunday = request->getParam("sunday", true)->value().toInt();

      String temporary = request->getParam("temporary", true)->value();
      boolean temp = (temporary == "true") ? true : false;

      StaticJsonDocument<600> doc;
      JsonObject days = doc.to<JsonObject>();
      
      days["monday"] = monday ? true : false;
      days["tuesday"] = tuesday ? true : false;
      days["wednesday"] = wednesday ? true : false;
      days["thursday"] = thursday ? true : false;
      days["friday"] = friday ? true : false;
      days["saturday"] = saturday ? true : false;
      days["sunday"] = sunday ? true : false;


      AddCycle(name, id_ev, starth, startm, endh, endm, days, temp);

    }
    
    request->send(204);
  });

  server.on("/DeleteValve", HTTP_POST, [](AsyncWebServerRequest *request) {
    if(request->hasParam("id", true)) {
      int idtodelete = request->getParam("id", true)->value().toInt();
      DebugSerial("Vanne à supprimer : " + String(idtodelete));
      StopValve(idtodelete);
      DeleteValve(idtodelete);
    }
    request->send(204);
  });

  server.on("/StopValve", HTTP_POST, [](AsyncWebServerRequest *request) {
    if(request->hasParam("id", true)) {
      int idtoclose = request->getParam("id", true)->value().toInt();
      DebugSerial("Vanne à fermer : " + String(idtoclose));
      StopValve(idtoclose, true);
    }
    request->send(204);
  });

  server.on("/StartValve", HTTP_POST, [](AsyncWebServerRequest *request) {
    if(request->hasParam("id", true)) {
      int idtoopen = request->getParam("id", true)->value().toInt();
      DebugSerial("Vanne à ouvrir : " + String(idtoopen));
      StartValve(idtoopen);
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
          DebugSerial("Ajout d'une valve locale : ");
          DebugSerial("StartPin : " + String(startpin));
          AddValve(name, "local", startpin, 0, 0, "", "");
        }
      }
      if(type == 1) {
        if(request->hasParam("starturl", true) && request->hasParam("stopurl", true)) {
          String starturl = request->getParam("starturl", true)->value();
          String stopurl = request->getParam("stopurl", true)->value();
          DebugSerial("Ajout d'une valve distante : ");
          DebugSerial("StartURL : " + String(starturl));
          DebugSerial("EndURL : " + String(stopurl));
          AddValve(name, "distante", 0, 0, 0, starturl, stopurl);
        }
      }
      if(type == 2) {
        if(request->hasParam("Hpin1", true) && request->hasParam("Hpin2", true)) {
          int Hpin1 = request->getParam("Hpin1", true)->value().toInt();
          int Hpin2 = request->getParam("Hpin2", true)->value().toInt();
          DebugSerial("Ajout d'une valve avec pont en H : ");
          DebugSerial("Hpin1 : " + String(Hpin1));
          DebugSerial("Hpin2 : " + String(Hpin2));
          AddValve(name, "locallatching", 0, Hpin1, Hpin2, "", "");
        }
      }

      

    }
    
    request->send(204);
  });

  server.on("/datetime.json", HTTP_GET, [](AsyncWebServerRequest *request) {
    String timeprovider = configarray[0]["time"]["syncprovider"];
    celsius = myRTC.temperature() / 4.0;
    if(timeprovider != "rtc") {
      celsius = 0;
    }
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
  DebugSerial("Serveur HTTP Async Actif !");
  CheckCyclesDelay.start(checkcycleinterval);
}

void CheckCycles() {
  String test = schedulesarray[0]["name"];
  if(test != "null") {

    for (JsonObject loop : schedulesarray) {
      delay(100);
      JsonObject days = loop["daysActive"];
      int id_ev = loop["id_ev"];
      int id_prog = loop["id_prog"];
      int starth = loop["Hourstart"];
      int startm = loop["Minstart"];
      int endh = loop["Hourstop"];
      int endm = loop["Minstop"];
      boolean state = loop["state"];
      String temp = loop["temporary"];
      if(CheckDay(days)) {
        if(CheckHours(starth, startm, endh, endm)) {
          // loop["state"] = true;
          if(state) {
            DebugSerial("Valve already started, cycle : " + String(id_prog));
          }
          else {
            DebugSerial("Starting valve, cycle : " + String(id_prog));
            if(StartValve(id_ev)) {
              loop["state"] = true;
            }
          }
        }
        else {   
          if(state) {
            DebugSerial("Stopping cycle (hour not valid) : " + String(id_prog));
            StopValve(id_ev);
            loop["state"] = false;
            
          }
          else {
            DebugSerial("Valve already stopped (hour not valid), cycle : " + String(id_prog));
            if(temp == "true") {
              DeleteCycle(id_prog);
            }
          }
        }
      }
      else {
        if(state) {
          DebugSerial("Stopping cycle (day not valid) : " + String(id_prog));
          StopValve(id_ev);
          loop["state"] = false;
          
        }
        else {
          DebugSerial("Valve already stopped (day not valid), cycle : " + String(id_prog));
          if(temp == "true") {
            DeleteCycle(id_prog);
          }
        }
      }
      
    }
  }
  else {
    DebugSerial("Aucun cycle n'est dans le fichier ou problème de lecture.");
  }
  

}

void loop() {
  String wifimode = configarray[0]["wifi"]["mode"];

  if(wifimode == "STA") {
    if(WiFi.status() != WL_CONNECTED) {
      DebugSerial("Disconnected from WiFi, reconnecting...");
      WiFi.reconnect();
      while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
    }
  }

  if(CheckCyclesDelay.justFinished()) {
    CheckCycles();
    CheckCyclesDelay.start(checkcycleinterval);
  }
  if(CloseValveDelay.justFinished()) {
    digitalWrite(Hpin1tostop, LOW);
    digitalWrite(Hpin2tostop, LOW);
  }
  
}

#include <Arduino.h>
// ESP
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESPAsyncWebServer.h>
#include <WiFiUdp.h>
#include <LittleFS.h>
// RTC/Time
#include <NTPClient.h>
#include <DS3232RTC.h>
#include <millisDelay.h>
// JSON
#include <ArduinoJson.h>
#include <AsyncJson.h>


boolean DEBUG = true;

// UDP for NTP
WiFiUDP ntpUDP;
// NTP Client
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

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

time_t NTPgetTime()
{
  timeClient.update();
  return timeClient.getEpochTime(); //get Epoch time from NTP
}
getExternalTime NTPTime = &NTPgetTime;


void DebugSerial(String msg) {
  if(DEBUG) {
    Serial.println("[DEBUG]: " + msg);
  }
}

void StartValve(int id, boolean force = false) {
  JsonObject valve = valvesarray[id];
  String name = valve["name"];
  bool state = valve["state"];
  int type = valve["type"];
  int pin1 = valve["pin1"];
  int pin2 = valve["pin2"];
  if(!state || force) {
    if (type == 0) {
      digitalWrite(pin1, HIGH);
    }
    else if (type == 1) {
      // To be continued
    }
    else if (type == 2) {
      long last_time = millis();
      pinMode(pin1, OUTPUT);
      pinMode(pin2, OUTPUT);
      while(millis() - last_time < 1500) {
        digitalWrite(pin1, HIGH);
        digitalWrite(pin2, LOW);
      }
      digitalWrite(pin1, LOW);
      digitalWrite(pin2, LOW);
    }
    DebugSerial(name + String(" started"));
    valvesarray[id]["state"] = true;
  }
}

void StopValve(int id, boolean force = false) {
  JsonObject valve = valvesarray[id];
  String name = valve["name"];
  int type = valve["type"];
  int pin1 = valve["pin1"];
  int pin2 = valve["pin2"];
  bool state = valve["state"];
  if(state || force) {
    if (type == 0) {
      digitalWrite(pin1, LOW);
    }
    else if (type == 1) {
      // To be continued
    }
    else if (type == 2) {
      long last_time = millis();
      pinMode(pin1, OUTPUT);
      pinMode(pin2, OUTPUT);
      while(millis() - last_time < 1500) {
        digitalWrite(pin1, LOW);
        digitalWrite(pin2, HIGH);
      }
      digitalWrite(pin1, LOW);
      digitalWrite(pin2, LOW);
    }
    DebugSerial(name + String(" stopped"));
    valvesarray[id]["state"] = false;
  }
}


void InitValves() {
  for(int i = 0; i < valvesarray.size(); i++) {
    JsonObject valve = valvesarray[i];
    String name = valve["name"];
    int type = valve["type"];
    int pin1 = valve["pin1"];
    int pin2 = valve["pin2"];
    String starturl = valve["starturl"];
    String stopurl = valve["stopurl"];
    pinMode(pin1, OUTPUT);
    pinMode(pin2, OUTPUT);
    StopValve(i, true);
    DebugSerial(name + String(" initialized"));
  }
}


boolean CheckDay(JsonObject daystotest) {
  String days[7] = {"sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday"};
  if (daystotest[days[weekday()-1]]) {
    return true;
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



void DeleteCycle(int idtodelete) {
  String test = schedulesjson[0]["name"];
  if(test != "null") {
    int id_ev = schedulesjson[idtodelete]["id_ev"];
    StopValve(id_ev);
    schedulesjson.remove(idtodelete);
    File schedules = LittleFS.open("/schedules.json", "w");
    serializeJson(schedulesjson, schedules);
    schedules.close();   
    DebugSerial("Cycle deleted.");
  }
  else {
    DebugSerial("Impossible de lire le fichier.");
  }
}

void AddCycle(String name, int id_ev, int starth, int startm, int endh, int endm, JsonObject daysjson, boolean temp) { 
  JsonObject schedule = schedulesarray.createNestedObject();
  schedule["name"] = name;
  schedule["id_ev"] = id_ev;
  schedule["Hourstart"] = starth;
  schedule["Minstart"] = startm;
  schedule["Hourstop"] = endh;
  schedule["Minstop"] = endm;
  schedule["daysActive"] = daysjson;
  schedule["temporary"] = temp;
  schedule["state"] = false;

  File schedules = LittleFS.open("/schedules.json", "w");
  serializeJson(schedulesjson, schedules);
  schedules.close(); 
  return;  
}

void DeleteValve(int idtodelete) {
  String test = valvesjson[0]["name"];
  if(test != "null") {
    StopValve(idtodelete);
    for(JsonObject valvecycle : schedulesarray) {
      if(valvecycle["id_ev"] == idtodelete) {
        int idcycle = valvecycle["id_prog"];
        String cyclename = valvecycle["name"];
        DebugSerial("Deleting valve cycle : " + cyclename);
        DeleteCycle(idcycle);
      }
    }
    valvesjson.remove(idtodelete);
    File valves = LittleFS.open("/valves.json", "w");
    serializeJson(valvesjson, valves); 
    valves.close(); 
  }
  else {
    DebugSerial("Erreur : DeleteValve");
  } 
  
  return;
}

void AddValve(String name = "Valve", int type = 0, int pin1 = 0, int pin2 = 0, String starturl = "", String stopurl = "") {
  JsonObject valve = valvesarray.createNestedObject();
  valve["name"] = name;
  valve["type"] = type;
  valve["pin1"] = pin1;
  valve["pin2"] = pin2;
  valve["starturl"] = starturl;
  valve["stopurl"] = stopurl;
  valve["state"] = false;
  valve["type"] = type;
  File valves = LittleFS.open("/valves.json", "w");
  serializeJson(valvesjson, valves);
  valves.close();
  return;
}

void SetWiFi(WiFiMode mode, const char *ssid, const char *password) {
  WiFi.disconnect();
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

  // LittleFS
  LittleFS.begin();

  File schedules = LittleFS.open("/schedules.json", "r");
  DeserializationError err = deserializeJson(schedulesjson, schedules);
  if(err) {
    DebugSerial("Erreur deserialization");
  }
  schedules.close();

  File valves = LittleFS.open("/valves.json", "r");
  DeserializationError err2 = deserializeJson(valvesjson, valves);
  if(err2) {
    DebugSerial("Erreur deserialization");
  }
  valves.close();

  File config = LittleFS.open("/config.json", "r");
  DeserializationError err3 = deserializeJson(configjson, config);
  if(err3) {
    DebugSerial("Erreur deserialization");
  }
  config.close();

  // Valves
  InitValves();

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
  
  String timeprovider = configarray[0]["time"]["syncprovider"];
  if(timeprovider == "rtc"){
    myRTC.begin();
    setSyncProvider(myRTC.get);
    DebugSerial("Time Sync Provider : RTC");
  }
  else if(timeprovider == "ntp") {
    int timezone = configarray[0]["time"]["timezone"];
    boolean summertime = configarray[0]["time"]["summertime"];
    String ntpserver = configarray[0]["time"]["ntpserver"];
    int timeoffset = summertime ? 3600*timezone+3600 : 3600*timezone;
    timeClient.setTimeOffset(timeoffset);
    timeClient.setPoolServerName(ntpserver.c_str());
    timeClient.begin();
    setSyncProvider(NTPTime);
    DebugSerial("Time Sync Provider : NTP");
  }



  // Web Server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.on("/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/bootstrap.min.css", "text/css");
  });

  server.on("/bootstrap.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/bootstrap.min.js", "text/javascript");
  });

  server.on("/jquery.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/jquery.min.js", "text/javascript");
  });

  server.on("/popper.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/popper.min.js", "text/javascript");
  });

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/script.js", "text/javascript");
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
      File config = LittleFS.open("/config.json", "w");
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
      boolean summertime = smt;
      timeClient.setPoolServerName(ntpserver.c_str());
      int timeoffset = summertime ? 3600*timezone+3600 : 3600*timezone;
      timeClient.setTimeOffset(timeoffset);
      timeClient.begin();
      setSyncProvider(NTPTime);
      configarray[0]["time"]["syncprovider"] = "ntp"; 
      configarray[0]["time"]["ntpserver"] = ntpserver;
      configarray[0]["time"]["timezone"] = timezone; 
      configarray[0]["time"]["summertime"] = summertime;
      File config = LittleFS.open("/config.json", "w");
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
      
      File config = LittleFS.open("/config.json", "w");
      serializeJson(configjson, config);
      config.close(); 
    }
    request->send(204);
  });
  
  server.on("/DeleteCycle", HTTP_POST, [](AsyncWebServerRequest *request) {
    if(request->hasParam("id", true)) {
      int idtodelete = request->getParam("id", true)->value().toInt();
      DebugSerial("Cycle à supprimer : " + String(idtodelete));
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
      DeleteValve(idtodelete);
    }
    request->send(204);
  });

  server.on("/StopValve", HTTP_POST, [](AsyncWebServerRequest *request) {
    if(request->hasParam("id", true)) {
      int idtoclose = request->getParam("id", true)->value().toInt();
      DebugSerial("Vanne à fermer : " + String(idtoclose));
      StopValve(idtoclose);
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
    String params[] = {"name", "type", "pin1", "pin2", "starturl", "stopurl"};

    for(String param : params) {
      if(!request->hasParam(param, true)) {
        request->send(400);
        return;
      }
    }
    
      
    String name = request->getParam("name", true)->value();
    int type = request->getParam("type", true)->value().toInt();
    int pin1 = request->getParam("pin1", true)->value().toInt();
    int pin2 = request->getParam("pin2", true)->value().toInt();
    //String starturl = request->getParam("starturl", true)->value();
    //String stopurl = request->getParam("stopurl", true)->value();
    AddValve(name, type, pin1, pin2, "", "");

    
    
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
    root["dayofweek"] = weekday();
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
  if (schedulesarray[0]["name"] == "null") {
    DebugSerial("Aucun cycle n'est dans le fichier ou problème de lecture.");
    return;
  }

  for (int id_prog = 0; id_prog < schedulesarray.size(); id_prog++) {
    JsonObject prog = schedulesarray[id_prog];
    JsonObject days = prog["daysActive"];
    String name = prog["name"];
    int id_ev = prog["id_ev"];
    int starth = prog["Hourstart"];
    int startm = prog["Minstart"];
    int endh = prog["Hourstop"];
    int endm = prog["Minstop"];
    boolean state = prog["state"];
    bool temp = prog["temporary"];
    if(CheckDay(days)) {
      if(CheckHours(starth, startm, endh, endm)) {
        if(state) {
          DebugSerial("Valve already started, cycle : " + name);
        }
        else {
          DebugSerial("Starting valve, cycle : " + name);
          StartValve(id_ev);
          prog["state"] = true;
        }
      }
      else {   
        if(state) {
          DebugSerial("Stopping cycle (hour not valid) : " + name);
          StopValve(id_ev);
          prog["state"] = false;
        }
        else {
          DebugSerial("Valve already stopped (hour not valid), cycle : " + name);
          if(temp) {
            DeleteCycle(id_prog);
          }
        }
      }
    }
    else {
      if(state) {
        DebugSerial("Stopping cycle (day not valid) : " + name);
        StopValve(id_ev);
        prog["state"] = false;
      }
      else {
        DebugSerial("Valve already stopped (day not valid), cycle : " + name);
        if(temp) {
          DeleteCycle(id_prog);
        }
      }
    }
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

}

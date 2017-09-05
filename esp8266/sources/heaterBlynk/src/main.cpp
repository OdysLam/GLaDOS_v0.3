// Depends for wifiManager
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
// Needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h> 
// Blynk, Timer and OTA depend
#include <TickerScheduler.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

char defBlynkToken[] = "8bd01200dfd24bc28633f7349ec47559";
char blynk_token[] = "8bd01200dfd24bc28633f7349ec47559";
//flag for saving data
bool shouldSaveConfig = false;
bool timerRunning = false;
int countdownTimer;
int countdownRemainInitial;
int countdownRemain;
TickerScheduler tasks(1);
//callback notifying us of the need to save config

void updateGauge();
void setTimer(int i);

void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}
void timerCallback() {
  countdownRemain--;
  Serial.println(countdownRemain);
  //remove 1 every second
  updateGauge();
  if (!countdownRemain){
    setTimer(0);
    Blynk.virtualWrite(V0,LOW);
  }
}
void setTimer(int timerOn){
  if (timerOn){
    if(tasks.enable(0)){
      Serial.println("timer started");
      timerRunning = true;
      countdownRemainInitial = countdownRemain;
      digitalWrite(2, HIGH);
      Blynk.notify("The timer started");
    }
  }
  else{
  if(tasks.disable(0)){
    Serial.println("timer stopped");
    countdownRemain = countdownRemainInitial;
    timerRunning = false;
    digitalWrite(2, LOW);
    Blynk.notify("The timer just ended, ready for a hot bath?");
    }
  }
}
void updateGauge(){
  Blynk.virtualWrite(V1,countdownRemain/60);
}


void setup() {
    Serial.begin(9600);

  // Read memory for blynk_token
    Serial.println("\n mounting FS...");
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          strcpy(blynk_token, json["blynk_token"]);
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
    else{
      Serial.println("Didn't find /config.json");
    }
  } else {
    Serial.println("failed to mount FS");
  }// after 
  Serial.println("\n setting up wifi...");
  WiFiManagerParameter custom_blynk_token("blynk", "blynk token", blynk_token, 32);
  WiFiManager wifiManager;
  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&custom_blynk_token);
  wifiManager.setTimeout(300);
  wifiManager.autoConnect("Odys Heater Module v.01");
  Serial.println("wifi was setup");
  delay(1000);
  Serial.println("delay just passed");
  //read updated parameter
  strcpy(blynk_token, custom_blynk_token.getValue());
  // Save updated parameter
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["blynk_token"] = blynk_token;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }
  Serial.println("\n local ip");
  Serial.println(WiFi.localIP());

  // Wifi is set and I have the blynk_token ready to use
  pinMode(2,OUTPUT);   
  
  // initialize scheduler for timer and disabled immediately
  //adding task to ticketscheduler. syntax (task_id,interval_ms, callback, void, true to fire cb right away
  if( tasks.add(0, 1000, [&](void*) { timerCallback(); }, nullptr, false) ){
    Serial.println("task initialized with interval 1000ms");
  }
  else{
    Serial.println("Failed to initialize task");
  }
  if(tasks.disable(0)){
    Serial.println("task was disabled");
  }
  Serial.println("I just passed setupWiFi() and timer object");
    // Now setup blynk
  Serial.println("blynk token:");
  Serial.println(blynk_token);
  Blynk.config(blynk_token);
  // Make sure that it connects to the Blynk server
  while (Blynk.connect() == false) {  
    Serial.println("connecting to Blynk server...");
  }
  Serial.println("connected to Blynk server!"); 

  // Set up ArduinoOTA
  ArduinoOTA.setHostname("myesp8266");
  ArduinoOTA.setPassword((const char *)"defiler007");
  ArduinoOTA.onStart([]() {
    Serial.println("Start OTA");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd OTA ");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.begin();
}

void loop(){
  Blynk.run();
  tasks.update();
  ArduinoOTA.handle();
  }
// when start/stop button is pressed
// BUTTON
BLYNK_WRITE(V0){
  if (param.asInt()){
    Serial.println("now timer is on");
    if (countdownRemain ){
      setTimer(1);
    }
    else{
      Blynk.virtualWrite(V0, LOW); // keep button to 0, don't start timer
      Blynk.notify("You can't start Timer for 0 min");
    }
  }
  else{
    setTimer(0);
  } 

}
// SLIDER
BLYNK_WRITE(V3) {
  if (timerRunning) { // only update if timer not running
    Blynk.virtualWrite(V3, param.asInt() ); // if running, refuse to let use change slider
  } else {
    countdownRemain = param.asInt() * 60;
    updateGauge();
  }
}

BLYNK_CONNECTED() {
Serial.println("Connected to blynk server");
}



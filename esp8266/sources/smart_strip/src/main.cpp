#include <ESP8266WiFi.h>      
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 
#include <MQTT.h>
#include <PubSubClient.h>
#include <PubSubClient_JSON.h>
#include <ArduinoJson.h>

#define PC_GPIO    5
#define RELAY_1    14
#define RELAY_2    4
#define RELAY_3    12
#define RELAY_4    13

// Global Objects
IPAddress server(192,168,1,25);
WiFiClient wclient;
PubSubClient myclient(wclient, server); 
void turnOnPc();

// Create Global Jsonbuffer
// more at https://github.com/bblanchon/ArduinoJson

const size_t bufferSize = JSON_OBJECT_SIZE(1) + 30;
DynamicJsonBuffer jsonBuffer(bufferSize);
int relays[] = {RELAY_1,RELAY_2,RELAY_3,RELAY_4};

void mqttCallback(const MQTT::Publish& pub) {
    Serial.println("message received:"); 
    Serial.println("topic = " + pub.topic());
    if (pub.topic() == "home/systems/smart_strip"){
        uint8_t * messageInt = pub.payload();
        char * message = (char*)messageInt;
        Serial.println(message);
        // message = {"relayNmbr":"action"}, relayNmbr = 1-4, action = 1/0
        JsonObject& messageJson = jsonBuffer.parseObject(message); 
        // I do not know the key, I will have to iterate once as I know that there will be only 1 pair
        JsonObject::iterator iter = messageJson.begin();
        unsigned int relay = atoi(iter->key);
        unsigned int action = atoi(iter->value);
        Serial.println("relay = ");     
        Serial.println(relay);
        Serial.println("action = ");
        Serial.println(action);
        if(action){
            Serial.println("action is OPEN");
        }
        else{
            Serial.println("action is CLOSE");
        }
        // relay_1 = relays[0], relays use inverted logic
        // relay == 5 is computer
        if(relay == 5){
             digitalWrite(PC_GPIO, HIGH);
             delay(500);
             digitalWrite(PC_GPIO, LOW);
        }
        else{
            digitalWrite(relays[relay-1],1 - action); 
        }
    }
}
void setupWiFi() {
    WiFiManager wifiManager;
    wifiManager.autoConnect("EspDoor");
    delay(3000);
}
void setup() {
    Serial.begin(9600);
    setupWiFi();
    pinMode(PC_GPIO, OUTPUT);
    digitalWrite(PC_GPIO, HIGH);
    pinMode(RELAY_1, OUTPUT);
    digitalWrite(RELAY_1, HIGH);
    pinMode(RELAY_2, OUTPUT);
    digitalWrite(RELAY_2, HIGH);
    pinMode(RELAY_3, OUTPUT);
    digitalWrite(RELAY_3, HIGH);
    pinMode(RELAY_4, OUTPUT);
    digitalWrite(RELAY_4, HIGH);
    // setup up 4 digital pins 

    // sketch is pretty simple, test OTA.
    Serial.println("wifi has been set up");

} 
void loop() {
    if (!myclient.connected()){
      Serial.println("Connecting to Homeassistant Broker..");
      if(myclient.connect(MQTT::Connect("EspHeaterv01").set_auth("homeassistant","defiler007").set_will("home/systems/smart_strip/status","disconnected")))
      {
        Serial.println("Connected to MQTT homeassistant server");
        myclient.set_callback(mqttCallback);
        myclient.subscribe("home/systems/smart_strip");
        } 
       else {
          Serial.println("Could not connect to MQTT server");   
       }
    }
    if (myclient.connected()){
      myclient.loop();
    }
}  

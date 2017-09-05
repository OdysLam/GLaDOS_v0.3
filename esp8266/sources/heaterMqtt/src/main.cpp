#include <ESP8266WiFi.h>      
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 

#include <MQTT.h>
#include <PubSubClient.h>
#include <PubSubClient_JSON.h>

IPAddress server(192,168,1,25);
WiFiClient wclient;
PubSubClient myclient(wclient, server); 

void mqttCallback(const MQTT::Publish& pub) {
    Serial.println("message received:");
    String message = pub.payload_string();
    Serial.println(message);
    if(message.toInt() == 1){
      digitalWrite(2,HIGH);
    }
    else{
      digitalWrite(2,LOW);
    }
        
 }
void setupWiFi() {
    WiFiManager wifiManager;
    wifiManager.autoConnect("EspHeater");
    delay(3000);

void setup() {
    Serial.begin(9600);
    setupWiFi();
    // if I reach here, I have connected to the internet, I set up my MQTT
    Serial.println("I just passed setupWiFi()");  
    pinMode(2,OUTPUT);   
    digitalWrite(2,LOW);
}


void loop() {
    if (!myclient.connected()){
      Serial.println("Connecting to Homeassistant Broker..");
      if(myclient.connect(MQTT::Connect("EspHeaterv01")
      .set_auth("homeassistant","defiler007")
      .set_will("home/appliances/heater/status","disconnected"))){
        Serial.println("connected to MQTT homeassistant server");
        myclient.set_callback(mqttCallback);
        myclient.subscribe("home/appliances/heater");
        } 
       else {
          Serial.println("Could not connect to MQTT server");   
       }
    }
    if (myclient.connected()){
      Serial.println("my client loop");
      myclient.loop();  
    }
}     


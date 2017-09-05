#include <ESP8266WiFi.h>      
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 
#include <MQTT.h>
#include <PubSubClient.h>
#include <PubSubClient_JSON.h>
#include <Servo.h> 

IPAddress server(192,168,1,25);
WiFiClient wclient;
PubSubClient myclient(wclient, server); 
Servo myservo;  
int door;
int doorRoutine = 0 ;
int doorStatusFlag = 0;
int openDoorFlag = 0;
int counter1 = 0;
int counter2 = 0;
int counter = 0;
void handleServo(int);
void openDoor();
void doorStatus();
int doorStatusPin = 13;
int openDoorPin = 14;
void mqttCallback(const MQTT::Publish& pub) {
    Serial.println("message received:");
    String message = pub.payload_string();
    String topic = pub.topic();
    Serial.println("topic = " + topic);
    Serial.println(message);
    if (pub.topic() == "home/systems/door"){
        if (message.toInt() == 1){
            handleServo(1);
        }
        else if (message.toInt() == 0){
            handleServo(0);
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
    // if I reach here, I have connected to the internet, I set up my MQTT
    Serial.println("I just passed setupWiFi()");
    Serial.println("Attaching interrupts & Servo...." );
    pinMode(openDoorPin,INPUT_PULLUP);
    pinMode(doorStatusPin,INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(openDoorPin), openDoor, CHANGE);
    attachInterrupt(digitalPinToInterrupt(doorStatusPin), doorStatus, CHANGE);
    myservo.attach(12); // attach servo to pin 12    
    Serial.println("Interrupts & Servo attached!");
    doorStatus();
} 
void loop() {
    if (!myclient.connected()){
      Serial.println("Connecting to Homeassistant Broker..");
      if(myclient.connect(MQTT::Connect("EspHeaterv01").set_auth("homeassistant","defiler007").set_will("home/systems/door/status","disconnected")))
      {
        Serial.println("connected to MQTT homeassistant server");
        myclient.set_callback(mqttCallback);
        myclient.subscribe("home/systems/door");
        } 
       else {
          Serial.println("Could not connect to MQTT server");   
       }
    }
    if (myclient.connected()){
      myclient.loop();
    }
    if (doorStatusFlag){
        delay(100); // seems right time, need to test with actual reed switch
        door = digitalRead(13);
        String door_string = String(door);
        myclient.publish("home/systems/door/status",door_string);
        Serial.println("door status is:");
        Serial.println(door);
        doorStatusFlag = 0;
    }
    if (openDoorFlag){
        delay(300); // need to test with proper button for bouncing
        Serial.println("just enterd opendoor() from interrupt");
        counter2 +=1;
        if(counter2 == 2){
            Serial.println("enterd if, running handleServo(1)");
            counter2 = 0;
            openDoorFlag = 0;
            handleServo(1);

        }
    }
}  
// ~~~~~~ Interrupt Functions ~~~~~~~~ 
void doorStatus(){
    if (doorStatusFlag==0){
        doorStatusFlag = 1;
        // 0 = close, 1 = open
        if (doorRoutine ){
            Serial.println("enterd doorRoutine, door is open");
            counter1 += 1; // 0--> 1 
            if(counter1 == 2){
                Serial.println("entered doorRoutine and counter1 == 1, door is closed");
                counter1 = 0;
                handleServo(0);
            }
        }
    }
}
//open door from button
void openDoor(){
    openDoorFlag = 1;
}
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void handleServo(int open){
    int pos;
    if(open == 1){ // 1 = open door, 0 = close
        doorRoutine = 1;
        for(pos = 0; pos <= 180; pos += 1)  {
            myservo.write(pos);
            }
    }              // tell servo to go to position in variable 'pos'                      
    else{
        doorRoutine = 0;
        delay(500);
        for(pos = 180; pos>=0; pos-=1) {                                
        myservo.write(pos);              // tell servo to go to position in variable 'pos'   
        }
    }   
}
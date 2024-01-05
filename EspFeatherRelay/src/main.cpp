#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <MQTT.h>
#include <TimeLib.h>
#include "secret.h"

#define LED_GND 12
#define LED_RED 13
#define LED_GRN 27
#define PIN_DOOR 12
#define PIN_RELEASE 4

WiFiClient networkClient;
String ssid =     "bitraf-tools";
String password = "Geethi3O";
MQTTClient * mqttClient;
int reconnectCounter = 0;

bool isCheckedOut = false;
unsigned long nextBlink;

enum class ConnectionStatus
{
    NOT_CONNECTED,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    MQTT_CONNECTING,
    CONNECTED,
    CONNECTION_LOST,
    UPDATING
};
ConnectionStatus currentStatus;

void maintainLed(); // predeclare

void setup() {
  Serial.begin(115200);
  pinMode(LED_GND, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GRN, OUTPUT);
  pinMode(PIN_DOOR, INPUT);
  pinMode(PIN_RELEASE, OUTPUT);
  digitalWrite(PIN_RELEASE, LOW);
  maintainLed();
  delay(5000); // Wait for Serial Monitor & display
  mqttClient = new MQTTClient(1024+512);
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  uint i = 0;
  uint tLen = topic.length();
  int secondSlash = 0;
  int thirdSlash = 0;
  int slashCount = 0;

  // find the slashes in the topic, so we can parse it
  for(i=0;i<tLen;i++)
  {
    char c = topic[i];
    if( c == '/')
    {
      if( slashCount == 0){
        // foo
      } else if( slashCount == 1){
        secondSlash = i;
      } else if( slashCount == 2){
        thirdSlash = i;
      }
      slashCount++;
    }
  }
  
  String commandRequested = topic.substring(thirdSlash+1, topic.length());
  if( commandRequested == "unlock")
  {
    Serial.println("unlock requested");
    isCheckedOut = true;
    digitalWrite(PIN_RELEASE, HIGH);
    delay(1000);
    digitalWrite(PIN_RELEASE, LOW);
  }
  else if( commandRequested == "lock")
  {
    Serial.println("lock requested");
    isCheckedOut = false;
    digitalWrite(PIN_RELEASE, HIGH);
    delay(1000);
    digitalWrite(PIN_RELEASE, LOW);
  }
  else
  {
    Serial.print("Unknown command: ");
    Serial.println(commandRequested);
  }
}

void setConnectionStatus(ConnectionStatus status){
  currentStatus = status;
}

void maintainLed(){
  unsigned long now = millis();
  if(now > nextBlink){
    nextBlink = now + 500;

    if( currentStatus != ConnectionStatus::CONNECTED )
    {
      if( !digitalRead( LED_RED ))
      {
        digitalWrite(LED_GND, LOW);
        digitalWrite(LED_RED, HIGH);
        digitalWrite(LED_GRN, HIGH);
      }
      else
      {
        digitalWrite(LED_GND, LOW);
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_GRN, LOW);
      }
      
    }
    else if( currentStatus == ConnectionStatus::CONNECTED )
    {
      if( isCheckedOut )
      {
        digitalWrite(LED_GND, LOW);
        digitalWrite(LED_RED, HIGH);
        digitalWrite(LED_GRN, LOW);
      }
      else
      {
        digitalWrite(LED_GND, LOW);
        digitalWrite(LED_RED, LOW);
        digitalWrite(LED_GRN, HIGH);
      }
    }
  }
  
}

void reconnect()
{
  Serial.println("reconnect");
  setConnectionStatus(ConnectionStatus::WIFI_CONNECTING);
  int timeOutTime = 20*1000; // Allow reconnections for X seconds
  int timeOut = millis() + timeOutTime;

  mqttClient->begin(MQTT_SERVER, 1883, networkClient);
  mqttClient->onMessage(messageReceived);

  if( WiFi.status() != WL_CONNECTED ){
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid.c_str(), password.c_str());

      while ( millis() < timeOut && WiFi.status() != WL_CONNECTED){
          maintainLed();
      }
  }
  
  // Connect to MQTT
  bool ret = false;
  if(WiFi.status() != WL_CONNECTED)
  {
      Serial.println("Stop reconnecting for a while!");
      WiFi.disconnect();
      setConnectionStatus(ConnectionStatus::NOT_CONNECTED);
      reconnectCounter++;
      delay(5000);
  }
  else if( WiFi.status() == WL_CONNECTED )
  {
    log_i("Connected to %s", WiFi.SSID().c_str());
    reconnectCounter = 0; // If we can connect, reset the reconnect counter
    setConnectionStatus(ConnectionStatus::MQTT_CONNECTING);
    
    ret = mqttClient->connect(MQTT_DEVICE_NAME, MQTT_USERNAME, MQTT_PASSWORD);
    if( ret )
    {
      String topic = String("bitraf/tool/embroidery/unlock");
      Serial.print("Subscribing to ");
      Serial.println(topic.c_str());
      bool isSubscribed = mqttClient->subscribe( topic.c_str() );
      if( isSubscribed )
      {
        topic = String("bitraf/tool/embroidery/lock");
        Serial.print("Subscribing to ");
        Serial.println(topic.c_str());
        isSubscribed = mqttClient->subscribe( topic.c_str() );
        if( isSubscribed )
        {
          setConnectionStatus(ConnectionStatus::CONNECTED);
        }
      }
      else
      {
        //setConnectionStatus(ConnectionStatus::NOT_CONNECTED);
      }
    }
    else
    {
        setConnectionStatus(ConnectionStatus::NOT_CONNECTED);
    }
  }
}

void loop() {
  maintainLed();

  // Stay connected even if we drop out in the middle of the loop
  if (WiFi.status() != WL_CONNECTED) {
      Serial.println("**** Wifi no longer connected?");
      reconnect();
  }

  if (!mqttClient->connected()) {
      Serial.println("**** MQTT no longer connected?");
      reconnect();
  }
  else
  {
      mqttClient->loop();
  }
}

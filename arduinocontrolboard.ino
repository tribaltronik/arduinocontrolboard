/*
 ControlBoard MQTT Arduino 0.1
 
  - connects to an Controlboard MQTT server
  - publishes "hello world" to the topic "outTopic"
  - subscribes to the topic "inTopic"
*/

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

#include "Timer.h"
Timer t;
const unsigned long PERIOD = 10000;   //ten seconds

#include <Wire.h>
#include <Adafruit_BMP085.h>
// Only used for sprintf
#include <stdio.h>

// Connect VCC of the BMP085 sensor to 3.3V (NOT 5.0V!)
// Connect GND to Ground
// Connect SCL to i2c clock - on '168/'328 Arduino Uno/Duemilanove/etc thats Analog 5
// Connect SDA to i2c data - on '168/'328 Arduino Uno/Duemilanove/etc thats Analog 4
// EOC is not used, it signifies an end of conversion
// XCLR is a reset pin, also not used here

Adafruit_BMP085 bmp;
  
// Update these with values suitable for your network.
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
IPAddress ip(192, 168, 1, 24);
IPAddress server(172, 16, 0, 2);
IPAddress dnsIP(8, 8, 8, 8);
IPAddress gateway(192, 168, 1, 254);
IPAddress subnet(255, 255, 255, 0);

String mainDeviceId = String("E1LMO-Y1b");
String  temp;
String  pressure;

// movement variables
const byte interruptPin = 2;
volatile byte state = LOW;

void callback(char* topic, byte* payload, unsigned int length) {
  //Serial.print("Message arrived:");
  //Serial.println(topic);
  
  String s = String((char*)payload);

  //topic == "E1LMO-Y1b/status" && 
  if(s.substring(0,5) == "alive" )
  {
      Serial.println("Send status");
      SendStatus();
  }
  Serial.println();
}

EthernetClient ethClient;
PubSubClient client("dev.controlboard.net", 1883, callback, ethClient);


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("arduinoClient")) {
      Serial.println("connected");
    
      //subscribe status  
      char* topicStatus;
      String statusStr =mainDeviceId +"/status";
      statusStr.toCharArray(topicStatus,statusStr.length());
      client.subscribe(topicStatus);
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(57600);
  if (!bmp.begin()) {
  Serial.println("Could not find a valid BMP085 sensor, check wiring!");
  while (1) {}
  }
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), MoveDetected, CHANGE);
  pinMode(6, OUTPUT);
  digitalWrite(6, HIGH);
  Ethernet.begin(mac, ip,dnsIP, gateway, subnet);
  // Allow the hardware to sort itself out
  t.every(PERIOD,ReadTempHum);
  delay(1500);
}

void loop()
{

  
 
  if (!client.connected()) {
    reconnect();
  }
  t.update();
  client.loop();
}

void ReadTempHum()
{
    Serial.println("Arduino ID:"+ mainDeviceId);
 
   Serial.print("Requesting data...");
     Serial.print("Temperature = ");
     temp = bmp.readTemperature();
    Serial.print(temp);
    Serial.println(" *C");
    
    Serial.print("Pressure = ");
    pressure = bmp.readPressure();
    Serial.print(pressure);
    Serial.println(" Pa");

      SendTempPressure();

}

void SendTempPressure()
{
    // Once connected, publish an announcement...
      String tempString = "{\"value\": \"" + String(temp) + "\"}";
      char tempChar[tempString.length()+1];
      tempString.toCharArray(tempChar, tempString.length()+1);
      Serial.println(tempChar);
      client.publish("E1LMO-Y1b/temp",tempChar,true);
      String pressureString = "{\"value\": \"" + String(pressure) + "\"}";
      char pressureChar[pressureString.length()+1];
      pressureString.toCharArray(pressureChar, pressureString.length()+1);
      Serial.println(pressureChar);
      boolean pubresult = client.publish("E1LMO-Y1b/pressure",pressureChar,true);
        if (pubresult)
        Serial.println("successfully sent");
      else
        Serial.println("unsuccessfully sent");
}

void SendStatus()
{
    String statusString = "{\"version\": \" 0.1\",\"ip\": \"192.168.1.24 \"}";
    char statusChar[statusString.length()+1];
    statusString.toCharArray(statusChar, statusString.length()+1);
    client.publish("E1LMO-Y1b/status",statusChar);
}

void MoveDetected() {
      state = !state;
      String moveString = "{\"value\": \"" + String(state) + "\"}";
      char moveChar[moveString.length()+1];
      moveString.toCharArray(moveChar, moveString.length()+1);
      Serial.println(moveChar);
      boolean pubresult = client.publish("E1LMO-Y1b/move",moveChar);
      if (pubresult)
        Serial.println("Move successfully sent");
      else
        Serial.println("Move unsuccessfully sent");
}

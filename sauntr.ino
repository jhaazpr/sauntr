#include <Phant.h>
#include <Time.h>
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                         SPI_CLOCK_DIVIDER); // you can change this clock speed

// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define IDLE_TIMEOUT_MS  3000
#define WEBSITE "data.sparkfun.com"
#define WEBPAGE "/input/4JdODwdWr9hqg8K6xoRq.txt"
#define PRIVATE "b5v8WyvXKNI27r1npAV2"

#define WLAN_SSID       "FangPhone"
#define WLAN_PASS       "3141592653"
const int analogInPin0 = A0;  // Analog input pin that the FSR is attached to
const int analogInPin1 = A1;  // Analog input pin that the FSR is attached to
const int analogInPin2 = A2;  // Analog input pin that the FSR is attached to
const int analogInPin3 = A3;  // Analog input pin that the FSR is attached to
const int thresh = 30;

int sensorValue0 = 0;
int sensorValue1 = 0;
int sensorValue2 = 0;
int sensorValue3 = 0;
boolean newMeasure = true;
int count = 0;
float avg = 0;
int lastWeight = 0;
uint32_t ip;

time_t lastTime;
Phant phant("data.sparkfun.com", "4JdODwdWr9hqg8K6xoRq", "b5v8WyvXKNI27r1npAV2");

void setup() {
  Serial.begin(9600);

  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
  
  
  Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
   
  Serial.println(F("Connected!"));
  
  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  

  ip = 0;
  // Try looking up the website's IP address
  while (ip == 0) {
    if (! cc3000.getHostByName(WEBSITE, &ip)) {
      Serial.println(F("Couldn't resolve!"));
    }
    delay(500);
  }

  cc3000.printIPdotsRev(ip);
  
//  Adafruit_CC3000_Client www = cc3000.connectTCP(ip, 80);
//  if (www.connected()) {
//    www.fastrprint(F("GET "));
//    www.fastrprint("/index.html");
//    www.fastrprint(F(" HTTP/1.1\r\n"));
//    www.fastrprint(F("Host: ")); www.fastrprint("www.andrewbfang.com"); www.fastrprint(F("\r\n"));
//    www.fastrprint(F("\r\n"));
//    www.println();
//  } else {
//    Serial.println(F("Connection failed"));    
//    return;
//  }
//
//  Serial.println(F("-------------------------------------"));
//  
//  /* Read data until either the connection is closed, or the idle timeout is reached. */ 
//  unsigned long lastRead = millis();
//  while (www.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
//    while (www.available()) {
//      char c = www.read();
//      Serial.print(c);
//      lastRead = millis();
//    }
//  }
//  www.close();
//  Serial.println(F("-------------------------------------"));
  
  /* You need to make sure to clean up after yourself or the CC3000 can freak out */
  /* the next time your try to connect ... */
//  Serial.println(F("\n\nDisconnecting"));
//  cc3000.disconnect();
}

void loop() {
  sensorValue0 = analogRead(analogInPin0);
  sensorValue1 = analogRead(analogInPin1);
  sensorValue2 = analogRead(analogInPin2);
  sensorValue3 = analogRead(analogInPin3);

  //  // print the results to the serial monitor:
//  Serial.print("sensor0 = " );
//  Serial.println(sensorValue0);
  //  Serial.print(" | sensor1 = " );
  //  Serial.print(sensorValue1);
  //  Serial.print(" | sensor2 = " );
  //  Serial.print(sensorValue2);
  //  Serial.print(" | sensor3 = " );
  //  Serial.println(sensorValue3);

  int newWeight = (sensorValue0);// + sensorValue1 + sensorValue2 + sensorValue3);
  if (newMeasure) {
    lastTime = now();
    newMeasure = false;
  } else {
    if ((now() - lastTime) < 10) {
      if (newWeight > thresh && (lastWeight-newWeight) < 0) {
        Serial.println("count");
        count = count + 1;
      }
    } else {
      lastTime = now();
      newMeasure = true;
      avg = 0.5 * (avg + count);
      Serial.println(avg);
      makePost(avg);
      count = 0;
    }
    lastWeight = newWeight;
  }

  // wait 2 milliseconds before the next loop
  delay(200);
}

void makePost(int value) {

//  Serial.println(phant.post());
  Adafruit_CC3000_Client client = cc3000.connectTCP(ip, 80);
  Serial.println(phant.post());
  if (client.connected()) {
    Serial.println("Posting...");
    phant.add("steps", avg);
    client.println(phant.post());
//    client.println(phant.get());
    /* Read data until either the connection is closed, or the idle timeout is reached. */ 
    unsigned long lastRead = millis();
    while (client.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
      while (client.available()) {
        char c = client.read();
        Serial.print(c);
        lastRead = millis();
      }
    }
//    client.fastrprint(F("POST "));
//    client.fastrprint(WEBPAGE);
//    client.fastrprint(F(" HTTP/1.1\r\n"));
//    Serial.print("POST ");
//    Serial.print(WEBPAGE);
//    Serial.print(" HTTP/1.1\r\n");
//    client.fastrprint(F("Host: ")); client.fastrprint(WEBSITE); client.fastrprint(F("\r\n"));
//    Serial.print("Host: "); Serial.print(WEBSITE); Serial.print("\r\n");
//    
//    client.fastrprint(F("Phant-Private-Key: ")); client.fastrprint(PRIVATE);client.fastrprint(F("\r\n"));
//    Serial.print("Phant-Private-Key: "); Serial.print(PRIVATE); Serial.print("\r\n");
//        
//    client.fastrprint(F("Connection: close")); client.fastrprint(F("\r\n"));
//    Serial.print("Connection: close"); Serial.print("\r\n");
//    
//    client.fastrprint(F("Content-Type: application/x-www-form-urlencoded")); client.fastrprint(F("\r\n"));
//    Serial.print("Content-Type: application/x-www-form-urlencoded"); Serial.print("\r\n");
//    
//    client.fastrprint(F("Content-Length: 12")); client.fastrprint(F("\r\n"));
//    Serial.print("Content-Length: 12");
//    client.fastrprint(F("\r\n"));
//    client.println();
//    Serial.println("Posted...");
  } else {
    Serial.println(F("Connection failed"));    
    return;
  }
  
//  /* For get requests Read data until either the connection is closed, or the idle timeout is reached. */ 
//  unsigned long lastRead = millis();
//  while (client.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
//    while (client.available()) {
//      char c = www.read();
//      Serial.print(c);
//      lastRead = millis();
//    }
//  }
//  client.close();
  Serial.println(F("-------------------------------------"));
  
  client.close();
  Serial.println("done");
}



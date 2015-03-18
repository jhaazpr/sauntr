/*
 * Reader.ino
 */
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include "utility/debug.h"

#include <Time.h>
#include <ctype.h>

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIV2); // you can change this clock speed

#define WLAN_SSID       "CalVisitor"           // cannot be longer than 32 characters!
#define WLAN_PASS       ""
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_UNSEC
#define IDLE_TIMEOUT_MS  3000      // Amount of time to wait (in milliseconds) with no data 
                                   // received before closing the connection.  If you know the server
                                   // you're accessing is quick to respond, you can reduce this value.

// What page to grab!
#define IDLE_TIMEOUT_MS  3000
#define WEBSITE "data.sparkfun.com"
/* ?gt[timestamp]=now%20-5min limits results to last 10 minutes */
#define WEBPAGE "/output/4JdODwdWr9hqg8K6xoRq.json?gt[timestamp]=now%20-10min"
#define PRIVATE "b5v8WyvXKNI27r1npAV2"

#define BUF_SIZE 128 //for reading
#define WRITE_BUF_SIZE 16//for writing a float string
#define SAMP_INTERVAL 200 // milliseconds between running calculations and sending data

/* Pin initializations */
const int buttonInputPin = 2;
const int actuatorPin = 4;
const int analogInPin0 = A0;
const int connectNotifyPin = 7;

/* Non-network-related initializations */
float steps;
boolean newMeasure = true;
int count = 0;
float avg = 0;
double walkerSteps = 0;
int lastWeight = 0;
time_t lastTime;
const int thresh = 80;
const int pressThresh = 100;
char readBuf[BUF_SIZE];
char writeBuf[WRITE_BUF_SIZE];

/**************************************************************************/
/*!
    @brief  Sets up the HW and the CC3000 module (called automatically
            on startup)
*/
/**************************************************************************/

uint32_t ip;

void setup(void)
{
  pinMode(buttonInputPin, INPUT);
  pinMode(actuatorPin, OUTPUT);
  pinMode(analogInPin0, INPUT);
  pinMode(connectNotifyPin, OUTPUT);
  
  digitalWrite(connectNotifyPin, HIGH);
  establishConnection();
  digitalWrite(connectNotifyPin, LOW);
}

void loop(void)
{
  int sensorValue0 = analogRead(analogInPin0);
  int newWeight = (sensorValue0);
  Serial.println(newWeight);
  if (newMeasure) {
    lastTime = now();
    newMeasure = false;
  } else {
    if ((now() - lastTime) < 10) {
      if (newWeight > thresh && (lastWeight) < pressThresh) {
        Serial.println("count");
        count = count + 1;
      }
    } else {
      lastTime = now();
      newMeasure = true;
      avg = 1.0 * (count);
      Serial.print("Average: ");
      Serial.println(avg);
      walkerSteps = getServerSteps();
      Serial.print("Walker: ");
      Serial.println(walkerSteps);
      compareAndActuate((double) avg, walkerSteps);
      count = 0;
    }
    lastWeight = newWeight;
  }
  
  delay(SAMP_INTERVAL);
}


/**************************************************************************/
/*!
    @brief  Tries to read the IP address and other connection details
*/
/**************************************************************************/

void establishConnection(void)
{
  Serial.begin(9600);
  Serial.println(F("Hello, CC3000!\n")); 

  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  
  /* Initialise the module */
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
  
  // Optional SSID scan
  // listSSIDResults();
  
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

  /* Display the IP address DNS, Gateway, etc. */  
  while (! displayConnectionDetails()) {
    delay(1000);
  }

  ip = 0;
  // Try looking up the website's IP address
  Serial.print(WEBSITE); Serial.print(F(" -> "));
  while (ip == 0) {
    if (! cc3000.getHostByName(WEBSITE, &ip)) {
      Serial.println(F("Couldn't resolve!"));
    }
    delay(500);
  }

  cc3000.printIPdotsRev(ip);
}

void handleFailedConnection(void)
{
  cc3000.disconnect();
  establishConnection();
}

bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}

/* Returns the most recent amount of steps from the writer */
double getServerSteps(void)
{
  readJSONToBuffer();
//  Serial.println("In read buffer: ");
//  Serial.println(readBuf);
  getFirstValue(readBuf, writeBuf);
  double steps = atof(writeBuf);
//  Serial.print("Final value: ");
//  Serial.println(steps);
  return steps;
}

/* Locs the first digit into readBuf */
void readJSONToBuffer()
{
  /* Set up socket and send input GET request */
  Serial.println("Connecting to website...");
  digitalWrite(connectNotifyPin, HIGH); // notify user of pending connection
  Adafruit_CC3000_Client www = cc3000.connectTCP(ip, 80);
  if (www.connected()) {
    www.fastrprint(F("GET "));
    www.fastrprint(WEBPAGE);
    
    www.fastrprint(F(" HTTP/1.1\r\n"));
    www.fastrprint(F("Host: ")); www.fastrprint(WEBSITE); www.fastrprint(F("\r\n"));
    www.fastrprint(F("\r\n"));
    www.println();
  } else {
    Serial.println(F("Connection failed"));
    actuate(connectNotifyPin);
//    handleFailedConnection();   
    return;
  }
  
  Serial.println(F("-------------------------------------"));
  
  /* Read data until either the connection is closed, or the idle timeout is reached. */ 
  unsigned long lastRead = millis();
  int i = 0;
  int seenOpenBrace = 0;
  int seenCloseBrace = 0;
  while (www.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (www.available()) {
      char c = www.read();
      if (!seenOpenBrace && c == '{') {
        seenOpenBrace = 1;
      }
      if (seenOpenBrace && !seenCloseBrace) {
        readBuf[i] = c;
        i++;
      }
      if (seenOpenBrace && c == '}') {
        seenCloseBrace = 1;
      }
//      Serial.print(c);
      lastRead = millis();
    }
  }
  www.close();
  digitalWrite(connectNotifyPin, LOW);
  Serial.println(F("-------------------------------------"));
}

void getFirstValue(char *readBuf, char *writeBuf) {
  int i = 0;
  int j = 0;
  do {
    i++;
  } while (*(readBuf + i) != ':');
  i += 2; // pointer on :, skip : and "
  while (*(readBuf + i) != '\"') {
    *(writeBuf + j) = *(readBuf + i);
    i++;
    j++;
  }
  j++;
  *(writeBuf + j) = '\0';
}

void compareAndActuate (double shoeSteps, double walkerSteps)
{
  if (shoeSteps > walkerSteps)
  {
    actuate(actuatorPin);
  }
}

void actuate(int pin)
{
  for (int i = 0; i < 5; i++)
  {
    digitalWrite(pin, HIGH);
    delay(250);
    digitalWrite(pin, LOW);
    delay(250);
  }
}


#include <Time.h>

const int analogInPin0 = A0;  // Analog input pin that the FSR is attached to
const int analogInPin1 = A1;  // Analog input pin that the FSR is attached to
const int analogInPin2 = A2;  // Analog input pin that the FSR is attached to
const int analogInPin3 = A3;  // Analog input pin that the FSR is attached to
const int thresh = 25;

int sensorValue0 = 0;
int sensorValue1 = 0;
int sensorValue2 = 0;
int sensorValue3 = 0;
boolean newMeasure = true;
int count = 0;
float avg = 0;
int lastWeight = 0;

time_t lastTime;

void setup() {
  Serial.begin(9600);
}

void loop() {
  sensorValue0 = analogRead(analogInPin0);
  sensorValue1 = analogRead(analogInPin1);
  sensorValue2 = analogRead(analogInPin2);
  sensorValue3 = analogRead(analogInPin3);       

//  // print the results to the serial monitor:
//  Serial.print("sensor0 = " );                       
//  Serial.print(sensorValue0);
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
      if (newWeight > thresh && lastWeight == 0) {
        Serial.println("count");
        count = count + 1;
      }
    } else {
      lastTime = now();
      newMeasure = true;
      avg = 0.5 * (avg + count);
      Serial.println(avg);
      count = 0;
    }
    lastWeight = newWeight;
  }

  // wait 2 milliseconds before the next loop
  delay(200);
}



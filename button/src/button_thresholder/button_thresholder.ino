/* Button Thresholder

Reports button thresholds
Circuit details will live here
*/

/* INITIALIZATIONS */

/* Pin setup */
const int buttonPin = 2;
const int ledPin = 13;

/* Global variables */
int buttonState = 0;

void setup() {
	pinMode(ledPin, OUTPUT);
	pinMode(buttonPin, INPUT);
}

void loop() {
	buttonState = digitalRead(buttonPin);

	if (buttonState == HIGH) {
		digitalWrite(ledPin, HIGH);
	} else {
		digitalWrite(ledPin, LOW);
	}
}

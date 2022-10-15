//#include <PinChangeInterruptSettings.h>
//#include <PinChangeInterruptPins.h>
//#include <PinChangeInterruptBoards.h>
//#include <PinChangeInterrupt.h>
//#include "pinout.h"


#define outputA 2
#define outputB 3

volatile int counter = 0;
volatile int n = LOW;
volatile int encoderPinALast = LOW;

void setup() {
    pinMode(outputA, INPUT_PULLUP);
    pinMode(outputB, INPUT_PULLUP);
    Serial.begin(9600);
    attachInterrupt(digitalPinToInterrupt(outputA), updateEncoder, CHANGE);
   
}

void loop() {
   
}

void updateEncoder() {
	n = digitalRead(outputA);
	if ((encoderPinALast == LOW) && (n == HIGH)) {
		if (digitalRead(outputB) == LOW) {
			if (counter > 0) {
				counter--;
			}
		}
		else {
			if (counter < 400) {
				counter++;
			}
		}		
	}
	encoderPinALast = n;
	Serial.println(counter);
}

#include <PinChangeInterruptSettings.h>
#include <PinChangeInterruptPins.h>
#include <PinChangeInterruptBoards.h>
#include <PinChangeInterrupt.h>
#include <LiquidCrystal_I2C.h>
#include "pinout.h"

LiquidCrystal_I2C lcd(LCDaddr, 16, 2);
#define encoderConnected
#define DEBUG
//#define fanControl
//#define modeButtons
//#define mux
//#define StartStopButtons

bool n;
bool encoderPinALast;
double EncoderLowLim = 00.00;
double EncoderHighLim = 20.00;
double EncoderPos = 00.00;
double ConstantCurrent = 00.00;
bool   writeState = LOW;



void setup() {
	lcd.init();                      // initialize the lcd 
	lcd.clear();
	lcd.createChar(0, R);
	lcd.createChar(1, V);
	lcd.createChar(4, V2);
	lcd.createChar(2, I);
	lcd.createChar(3, C);
	lcd.setCursor(0, 0);	
	lcd.backlight();

#ifdef DEBUG
	Serial.begin(9600);
	Serial.println("ImHere");
#endif // DEBUG

///
//Hw Buttons
///

#ifdef encoderConnected
	pinMode(EncA, INPUT_PULLUP);
	pinMode(EncB, INPUT_PULLUP);
	pinMode(EncBTN, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(EncA), EncoderISR, FALLING);
	//attachInterrupt(digitalPinToInterrupt(EncB), EncoderISR, FALLING);
	//attachInterrupt(digitalPinToInterrupt(EncB), encoderButtonISR, FALLING);
	attachPinChangeInterrupt(digitalPinToPCINT(EncBTN), encoderButtonISR, FALLING);

#endif // encoderConnected

#ifdef fanControl
	pinMode(Fan_PWM, OUTPUT);
#endif // fanControl

#ifdef modeButtons
	pinMode(CVbtn, INPUT_PULLUP);
	pinMode(CCbtn, INPUT_PULLUP);
	pinMode(CPbtn, INPUT_PULLUP);
	pinMode(CRbtn, INPUT_PULLUP);
#endif //modeButtons

#ifdef StartStopButtons
	pinMode(StartBtn, INPUT_PULLUP);
	pinMode(StopBtn, INPUT_PULLUP);
#endif //StartStopButtons

#ifdef mux
	pinMode(Mux_A0, OUTPUT);
	pinMode(Mux_A1, OUTPUT);
	pinMode(Mux_A2, OUTPUT);
#endif // mux

}

void loop() {
	/*lcd.setCursor(14,1);
	lcd.print("CV");
	delay(5000);
	lcd.setCursor(15, 1);
	lcd.write(1);
	delay(5000);
	lcd.setCursor(15, 1);
	lcd.write(4);
	delay(5000);*/
	delay(500);
	
}

void drawLCD() {
	lcd.print("00.000V|000.000W");//15
	lcd.setCursor(0, 1);
	lcd.print("00.000A|00.000");//18! (0W)
	//lcd.write(3);
	//lcd.write(0);
}

void EncoderISR() {
	n = digitalRead(EncA);
	if ((encoderPinALast == LOW) && (n == HIGH)) {
		if (digitalRead(EncB) == LOW) {
			if (EncoderPos > EncoderLowLim) {
				EncoderPos--;
			}
		}
		else {
			if (EncoderPos < EncoderHighLim) {
				EncoderPos++;
			}
		}		
	}
	encoderPinALast = n;

	if (writeState==true) {
		ConstantCurrent = EncoderPos;
		
	}

	Serial.print(EncoderPos);
	Serial.print("|");
	Serial.println(ConstantCurrent);
}
void encoderButtonISR() {
		writeState = !writeState;
		if (writeState == true)
		{
			//Set LCD to inverted char
		}

		if (writeState == false)
		{
			//set LCD to regular char
		}
	Serial.println(writeState);
}
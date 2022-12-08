#include <PinChangeInterruptSettings.h>
#include <PinChangeInterruptPins.h>
#include <PinChangeInterruptBoards.h>
#include <PinChangeInterrupt.h>
#include <LiquidCrystal_I2C.h>
#include "pinout.h"

LiquidCrystal_I2C lcd(LCDaddr, 20, 4);
#define encoderConnected
#define DEBUG
#define modeButtons
#define StartStopButtons

int n;
int encoderPinALast;
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
	drawLCD();
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
	attachInterrupt(digitalPinToInterrupt(EncA), EncoderISR, CHANGE);
	attachInterrupt(digitalPinToInterrupt(EncB), EncoderISR, CHANGE);
	attachPinChangeInterrupt(digitalPinToPCINT(EncBTN), encoderButtonISR, FALLING);

#endif // encoderConnected


#ifdef modeButtons
	pinMode(CVbtn, INPUT_PULLUP);
	pinMode(CCbtn, INPUT_PULLUP);
	pinMode(CPbtn, INPUT_PULLUP);
	pinMode(CRbtn, INPUT_PULLUP);

	attachPinChangeInterrupt(digitalPinToPCINT(CVbtn), CVbuttonISR, FALLING);
	attachPinChangeInterrupt(digitalPinToPCINT(CCbtn), CCbuttonISR, FALLING);
	attachPinChangeInterrupt(digitalPinToPCINT(CPbtn), CPbuttonISR, FALLING);
	attachPinChangeInterrupt(digitalPinToPCINT(CRbtn), CRbuttonISR, FALLING);
	attachPinChangeInterrupt(digitalPinToPCINT(BATbtn), batteryISR, FALLING);
#endif //modeButtons

#ifdef StartStopButtons
	pinMode(StartBtn, INPUT_PULLUP);
	pinMode(StopBtn, INPUT_PULLUP);
	pinMode(settingsSW, INPUT_PULLUP);
	attachPinChangeInterrupt(digitalPinToPCINT(StartBtn), startButtonISR, FALLING);
	attachPinChangeInterrupt(digitalPinToPCINT(StopBtn), stopButtonISR, FALLING);
	attachPinChangeInterrupt(digitalPinToPCINT(settingsSW), settingsISR, FALLING);
#endif //StartStopButtons



}

void loop() {
	delay(500);
	lcd.setCursor(0, 0);
	lcd.print(EncoderPos,3);
	
	if (writeState) {
		clearLCD();
		writeState = 0;
	}
}

void drawLCD() {
	//lcd.print("00.000V|000.000W");//15
	//lcd.setCursor(0, 1);
	//lcd.print("00.000A|00.000");//18! (0W)
	//lcd.write(3);
	//lcd.write(0);
	lcd.setCursor(6, 0);
	lcd.print("V|");
	lcd.setCursor(6, 1);
	lcd.print("A|");
	lcd.setCursor(6, 2);
	lcd.print("W|");
	lcd.setCursor(6, 3);
	lcd.print("R|");
	lcd.setCursor(14, 0);
	lcd.write(0);
	lcd.setCursor(14, 1);
	lcd.write(1);
	lcd.setCursor(14, 2);
	lcd.write(2);
	lcd.setCursor(14, 3);
	lcd.write(3);
}

void clearLCD() {
	lcd.setCursor(0, 0);
	lcd.print("      ");
	lcd.setCursor(0, 1);
	lcd.print("      ");
	lcd.setCursor(0, 2);
	lcd.print("      ");
	lcd.setCursor(0, 3);
	lcd.print("      ");
	lcd.setCursor(8, 0);
	lcd.print("      ");
	lcd.setCursor(8, 1);
	lcd.print("      ");
	lcd.setCursor(8, 2);
	lcd.print("      ");
	lcd.setCursor(8, 3);
	lcd.print("      ");
	lcd.write(3);
}

void EncoderISR() {
	/*n = digitalRead(EncA);	
	if ((encoderPinALast == LOW) && (n == HIGH)) {
		if (digitalRead(EncB) == LOW) {
			if (EncoderPos > EncoderLowLim) {
				EncoderPos++;
				Serial.println("CW");
			}
		}
		else {
			if (EncoderPos < EncoderHighLim) {
				EncoderPos--;
				Serial.println("CCW");
			}
		}		
	}*/
	
	n = digitalRead(EncA);
	if ((encoderPinALast == LOW) && (n == HIGH)) {
		if (digitalRead(EncB) == LOW) {
			if (EncoderPos > 0) {
				EncoderPos = EncoderPos - 10;
			}
		}
		else {
			if (EncoderPos < 400) {
				EncoderPos = EncoderPos + 10;
			}
		}
		
	}
	encoderPinALast = n;
	Serial.println(EncoderPos);
}
void encoderButtonISR() {
		writeState = !writeState;
		if (writeState == true)
		{
			ConstantCurrent = EncoderPos;
			//Set LCD to inverted char
		}

		if (writeState == false)
		{
			//set LCD to regular char
		}
	Serial.println(writeState);
	
}

void CVbuttonISR() {
	Serial.println("CV");
}

void CCbuttonISR() {
	Serial.println("CC");
}

void CPbuttonISR() {
	Serial.println("CP");
}

void CRbuttonISR() {
	Serial.println("CR");
}

void startButtonISR() {
	Serial.println("Start");
}
void stopButtonISR() {
	Serial.println("Stop");
}

void settingsISR() {
	Serial.println("SET");
}

void batteryISR() {
	Serial.println("BAT");
}
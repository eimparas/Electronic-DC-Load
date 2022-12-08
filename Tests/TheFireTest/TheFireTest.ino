#include <PinChangeInterruptSettings.h>
#include <PinChangeInterruptPins.h>
#include <PinChangeInterruptBoards.h>
#include <PinChangeInterrupt.h>
#include <LiquidCrystal_I2C.h>
#include <ADS1X15.h>
#include <MCP4725.h>

#include "pinout.h"

MCP4725 MCP1(DAC1addr);
MCP4725 MCP2(DAC2addr);
ADS1115 ADS(ADCaddr);
LiquidCrystal_I2C lcd(LCDaddr, 20, 4);

#define DEBUG
#define encoderConnected
#define modeButtons
#define StartStopButtons

int n;
int encoderPinALast;
double EncoderLowLim = 00.00;
double EncoderHighLim = 20.00;
double EncoderPos = 00.00;
double ConstantCurrent = 00.00;
bool   writeState = false;
bool encoderButtonState =false;
bool CVbuttonState = false;


float f = 0;

float _A0 = 0.0;
float _A1 = 0.0;
float _A2 = 0.0;
float _A3 = 0.0;

float VoltInternal = 0.0;
float VoltExternal = 0.0;
float Current = 0.0;
float ACS1_A = 0.0;
float ACS2_A = 0.0;
float ACS1_offset = 2.22;
float ACS2_offset = 2.22;
float acs1AscR = 0.0;
float acs2AscR = 0.0;

void setup() {
#ifdef DEBUG
    Serial.begin(115200);
    Serial.println("ImHere");
#endif // DEBUG

//============================================================
//DAC & ADC 
//============================================================
    if (MCP1.begin() == false)
    {
        Serial.println("Could not init DAC1");
    }  
    if (MCP2.begin() == false)
    {
        Serial.println("Could not init DAC2");
    }
    /// <summary>
    /// DAC INIT 
    /// </summary>

    ADS.begin();
    if (!ADS.isConnected()) {
        Serial.println("ADS1115 not presant on bus , at expected Addr 0x48");
    }
    Serial.println("ADS connected on bus,at expected Addr 0x48");
    f = ADS.toVoltage();
    Serial.print("the conversion factor is : ");
    Serial.print(f, 5);
    /// <summary>
    /// ADC init
    /// </summary>
    
//============================================================
//LCD 
//============================================================

    lcd.init();
    lcd.clear();
    lcd.createChar(0, R);
    lcd.createChar(1, V);
    lcd.createChar(4, V2);
    lcd.createChar(2, I);
    lcd.createChar(3, C);
    lcd.setCursor(0, 0);
    lcd.backlight();
	drawLCD();

    ///<summary>
    /// LCD Init
    /// </summary>

//============================================================
//Keyboard
//============================================================

#ifdef encoderConnected
    pinMode(EncA, INPUT_PULLUP);
    pinMode(EncB, INPUT_PULLUP);
    pinMode(EncBTN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(EncA), EncoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(EncB), EncoderISR, CHANGE);
    attachPinChangeInterrupt(digitalPinToPCINT(EncBTN), encoderButtonISR, FALLING);

#endif // encoderConnected
    ///<summary>
    /// Rottery Encoder
   /// </summary>
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



    Serial.println("\n end of setup()");
}

void loop() {
	if (encoderButtonState) {
		encoderButtonState = false;
		MCP1.setValue(EncoderPos);
	}

	if (CVbuttonState) {
		CVbuttonState = false;
		MCP2.setValue(EncoderPos);
	}
	_A0 = ADS.readADC(0);
	_A1 = ADS.readADC(1);
	

	VoltExternal = _A0 * f;
	VoltInternal = _A1 * f;

	
	readCurrent();

	Serial.print(acs1AscR);
	Serial.print(",");
	Serial.print(acs2AscR);
	Serial.print(",");
	Serial.print(ACS1_A);
	Serial.print(",");
	Serial.print(ACS2_A);
	Serial.print(",");
	Serial.println(Current);
	updateLCD();
}
//============================================================
//LCD Support Functions
//============================================================

void drawLCD() {
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

void updateLCD() {
	lcd.setCursor(0, 0);
	lcd.print(VoltInternal,2);
	lcd.setCursor(0, 1);
	lcd.print(Current,3);
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

//============================================================
// Interupt Service Routines 
//============================================================

void EncoderISR() {
	n = digitalRead(EncA);
	if ((encoderPinALast == LOW) && (n == HIGH)) {
		if (digitalRead(EncB) == LOW) {
			if (EncoderPos > 0) {
				EncoderPos = EncoderPos - 10;
			}
		}
		else {
			if (EncoderPos < 4096) {
				EncoderPos = EncoderPos + 10;
			}
		}

	}
	encoderPinALast = n;
	Serial.println(EncoderPos);
}
void encoderButtonISR() {
	encoderButtonState =true;

	if (writeState == true)
	{
		ConstantCurrent = EncoderPos;
		
	}

	if (writeState == false)
	{
		
	}
	Serial.println(writeState);

}

void CVbuttonISR() {
	Serial.println("CV");
	CVbuttonState = true;
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

//============================================================
// Support Functions
//============================================================

void readCurrent() {
	_A2 = ADS.readADC(2);
	_A3 = ADS.readADC(3);
	acs1AscR = _A2 * f;
	acs2AscR = _A3 * f;
	//Current = (AcsOffset – (measured analog reading)) / Sensitivity
	ACS1_A = (ACS1_offset - acs1AscR) / 66.0;
	ACS2_A = (ACS2_offset - acs2AscR) / 66.0;
	Current = ACS1_A + ACS2_A;
}
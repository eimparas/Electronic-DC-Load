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


float ADCVoltageFactor = 0;

float Current = 0.0;
float ACS1_A = 0.0;
float ACS2_A = 0.0;
float ACS1_offset = 2.47;
float ACS2_offset = 2.47;
float acs1Raw = 0.0;
float acs2Raw = 0.0;


float VoltInternal = 0.0;
float VoltInternalRaw = 0.0;
float VoltExternal = 0.0;
float VoltExternalRaw = 0.0;



float PowerExternal = 0.0;
float RLoad = 0.0;

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
    // DAC INIT 
    

    ADS.begin();
    if (!ADS.isConnected()) {
        Serial.println("ADS1115 not presant on bus");
    }
    Serial.println("ADS connected on bus,at expected Addr 0x48");
	ADCVoltageFactor = ADS.toVoltage();
	Serial.print("the ADC Voltage factor is : ");
    Serial.print(ADCVoltageFactor, 5);
    // ADC init
    
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

    // LCD Init
	

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
    // Rottery Encoder

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
	

	
	readCurrent();
	readVoltage();
	CalcPower();
	CalcResistance();
	Serial.print(readTemp(Temp0));
	Serial.print(",");
	Serial.print(readTemp(Temp1));
	Serial.print(",");
	Serial.print(readTemp(Temp2));
	Serial.print(",");
	Serial.print(acs1Raw);
	Serial.print(",");
	Serial.print(acs2Raw);
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
	lcd.print(VoltExternal,2);
	lcd.setCursor(0, 1);
	lcd.print(Current,3);
	lcd.setCursor(0, 2);
	lcd.print(PowerExternal);
	lcd.setCursor(0, 3);
	lcd.print(RLoad);
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
void readVoltage() {
	float ADCA0 = 0.0;
	float ADCA1 = 0.0;
	ADCA0 = ADS.readADC(0);
	ADCA1 = ADS.readADC(1);
	VoltExternalRaw = ADCA0 * ADCVoltageFactor;
	VoltInternalRaw = ADCA1 * ADCVoltageFactor;
	//Vin=(VoltExternalRaw*(R11+R12+R2)/R2;
	VoltExternal = (VoltExternalRaw * (R11 + R12 + R2) / R2);
	VoltInternal = (VoltInternalRaw * (R11 + R12 + R2) / R2);
}

void readCurrent() {
	float ADCA2 = 0.0;
	float ADCA3 = 0.0;
	ADCA2 = ADS.readADC(2);
	ADCA3 = ADS.readADC(3);
	acs1Raw = ADCA2 * ADCVoltageFactor;
	acs2Raw = ADCA3 * ADCVoltageFactor;
	//Current = (AcsOffset – (measured analog reading)) / Sensitivity
	ACS1_A = (ACS1_offset - acs1Raw)/0.066;
	ACS2_A = (ACS2_offset - acs2Raw)/0.066;
	Current = ACS1_A + ACS2_A;
}


void CalcPower() {
	PowerExternal = VoltExternal * Current;
}

void CalcResistance() {
	RLoad = VoltExternal / Current;
}

float readTemp(int sensor) {
	float val = analogRead(sensor);
	float mv = (val / 1024.0) * 5000;
	return mv / 10;
}
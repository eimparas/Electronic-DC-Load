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
short int EncoderPos = 00;
double ConstantCurrent = 00.00;
double multiplier = 1;

bool writeState = false;
bool clearScreen = false;
bool settingsButtonState = false;
bool encoderButtonState =false;
bool CVbuttonState = false;
bool isRuning = false;
int mode = 0;//CC CV CP CR
float CVoltageSet = 0.0;

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
    //============
    /// DAC INIT 
    //============

    ADS.begin();
    if (!ADS.isConnected()) {
        Serial.println("ADS1115 not presant on bus , at expected Addr 0x48");
    }
    Serial.println("ADS connected on bus,at expected Addr 0x48");
	ADCVoltageFactor = ADS.toVoltage();
	Serial.print("the ADC Voltage factor is : ");
    Serial.print(ADCVoltageFactor, 5);
	
    //================
    /// ADC init
    //=================    
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
	delay(1000);
	calibrateACS();//Calibrate the ACS_Offset after the LCD is fully initialized and in full backlit

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
	
	readCurrent();
	readVoltage();
	CalcPower();
	CalcResistance();
	Serial.print(acs1Raw);
	Serial.print(",");
	Serial.print(acs2Raw);
	Serial.print(",");
	Serial.print(ACS1_A);
	Serial.print(",");
	Serial.print(ACS2_A);
	Serial.print(",");
	Serial.println(Current);
	if (!settingsButtonState) {
		if (clearScreen) {
			lcd.clear();
			drawLCD();
			clearScreen = false;
		}	
		
		updateLCD();
		//clearLCD();
	}
	else {
		if (clearScreen) {
			lcd.clear();
			clearScreen = false;
		}
		settingsLCD();
	}
	CVoltageSet += (EncoderPos * (multiplier / 100));
	EncoderPos = 0;
	if (CVoltageSet < 0) {
		CVoltageSet = 0;
	}
//delay(10);
//End of Loop();
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
	lcd.print("V");	
	lcd.setCursor(14, 1);
	lcd.print("A");
	lcd.setCursor(14, 2);
	lcd.print("W");
	lcd.setCursor(14, 3);
	lcd.print("R");
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
}

void updateLCD() {
	lcd.setCursor(0, 0);
	clearAndPrintFloat( abs( VoltExternal),2);
	lcd.setCursor(0, 1);
	clearAndPrintFloat(abs( Current),3);
	lcd.setCursor(0, 2);
	clearAndPrintFloat(abs(PowerExternal),3);
	lcd.setCursor(0, 3);
	clearAndPrintFloat(abs(RLoad),0);
	lcd.setCursor(8, 0);
	clearAndPrintFloat(CVoltageSet,2);
	lcd.setCursor(8, 1);
	lcd.print("      ");
	lcd.setCursor(8, 2);
	lcd.print("      ");
	lcd.setCursor(8, 3);
	lcd.print("      ");	
}
void settingsLCD() {
	float shuntTemp = (readTemp(Temp0) + readTemp(Temp1)) / 2;//average of shunt temperature
	lcd.setCursor(0, 0);
	lcd.print("Shunt Temp: ");
	lcd.print(shuntTemp, 2);

	lcd.setCursor(0, 1);
	lcd.print("Mosfet Temp:");
	lcd.print(readTemp(Temp2), 2);

	lcd.setCursor(0, 2);
	lcd.print("IPv4:004.020.006.009");
	lcd.setCursor(0, 3);
	lcd.print("External Sense:OFF");
;
}

void clearAndPrintFloat(float num, int digits) {
	lcd.print(num, digits);
	digits++;
	while (num >= 10) {
		digits++;
		num /= 10;
	}
	for (byte i = 0; i < 5 - digits; i++) {
		lcd.print(" ");
	}
}

//============================================================
// Interupt Service Routines 
//============================================================

void EncoderISR() {
	n = digitalRead(EncA);
	if ((encoderPinALast == LOW) && (n == HIGH)) {
		if (digitalRead(EncB) == LOW) {
			if (EncoderPos > -1000) {
				EncoderPos--;
				//CVoltageSet -= multiplier / 100;
			}
		}
		else {
			if (EncoderPos < 4096) {
				EncoderPos++;
				Serial.print("r");
				//CVoltageSet += multiplier / 100;
			}
		}

	}
	encoderPinALast = n;
	Serial.println(EncoderPos);
}
void encoderButtonISR() {
	if (multiplier <= 10) {
		multiplier *= 10;
	}
	else {
		multiplier = 1.0;
	}
	//Serial.println(multiplier);
}

void CVbuttonISR() {
	Serial.println("CV");
	CVbuttonState = true;
	mode = 1;
	Serial.println(mode);
}

void CCbuttonISR() {
	Serial.println("CC");
	mode = 0;
	Serial.println(mode);
}

void CPbuttonISR() {
	Serial.println("CP");
	mode = 3;
	Serial.println(mode);
}

void CRbuttonISR() {
	Serial.println("CR");
	mode = 4;
	Serial.println(mode);
}

void startButtonISR() {
	Serial.println("Start");
	encoderButtonState = true;

	if (writeState == true)
	{
		ConstantCurrent = EncoderPos;

	}

	if (writeState == false)
	{

	}
	Serial.println(writeState);
	isRuning = true;
}
void stopButtonISR() {
	Serial.println("Stop");
	isRuning = false;
}

void settingsISR() {
	Serial.println("SET");
	if (settingsButtonState) {
		settingsButtonState = false;
	}
	else {
		settingsButtonState = true;
	}
	clearScreen = true;
}

void batteryISR() {
	Serial.println("BAT");
	mode = 5;
	Serial.println(mode);
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
	if (abs(Current ) == 00.00) {
		RLoad = 00.00;
	}
	else {
		RLoad = abs( VoltExternal )/ abs( Current);
	}
}

float readTemp(int sensor) {
	float val = analogRead(sensor);
	float mv = (val / 1024.0) * 5000;
	return mv / 10;
}

void calibrateACS() {
	float ADCA2 = 0.0;
	float ADCA3 = 0.0;
	ADCA2 = ADS.readADC(2);
	ADCA3 = ADS.readADC(3);
	acs1Raw = ADCA2 * ADCVoltageFactor;
	acs2Raw = ADCA3 * ADCVoltageFactor;
	Serial.print("ACS1 offset: ");
	Serial.println(acs1Raw);
	Serial.print("ACS1 offset: ");
	Serial.println(acs1Raw);
	ACS1_offset = acs1Raw;
	ACS2_offset = acs2Raw;
}
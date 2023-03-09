#include <LiquidCrystal_I2C.h>
#include <ADS1X15.h>
#include <MCP4725.h>
#include <Arduino.h>
#include <Vrekrer_scpi_parser.h>
#include <SPI.h>
#include <EthernetENC.h>
#include "pinout.h"

byte mac[6] = { 0x74, 0x69, 0x69, 0x2D, 0x30, 0x31 };

#define DEBUG
#define encoderConnected
#define modeButtons
#define StartStopButtons

#define Vmax 40
#define Imax 20

uint16_t t1 = 0, t2 = 0;
int n;
int encoderPinALast;
short int EncoderPos = 00;
int multiplier = 1;
short int DAC1Point = 0;
short int DAC2Point = 0;

byte antiFlicker = 0;

bool writeState = false;
bool clearScreen = false;
bool SclearScreen = false;
bool settingsButtonState = false;
bool extSense = false;
bool isRuning = false;
bool batteryMode = false;
bool EthSetupStatus = false;
bool MCP = false;

//float CVoltageSet = 0.0;

float ADCVoltageFactor = 0;

float Current = 0.0;
float ACS1_A = 0.0;
float ACS2_A = 0.0;
float ACS1_offset = 2.47;
float ACS2_offset = 2.47;
float acs1Raw = 0.0;
float acs2Raw = 0.0;

unsigned long startTime;

struct setPoint
{
	float Voltage;
	float Current;
	float Resistance;
	float Power;
} setPoints;

struct batterySetPoint {
	float Vstop;
	float Current;
	float CapLimit;
} battPoints;

struct loadReadings {
	float Voltage;
	float Current;
	float Power;
	float Resistance;
	float WattHrs;
	float AmpHrs;
} Readings;

enum operationMode {
	CC,
	CV,
	CP,
	CR
};


float VoltInternal = 0.0;
float VoltInternalRaw = 0.0;
float VoltExternal = 0.0;
float VoltExternalRaw = 0.0;



float PowerExternal = 0.0;
float RLoad = 0.0;


float CVoltage = 0.0;
float CCurrent = 0.0;
float CRessistance = 0.0;
float CPower = 0.0;


MCP4725 MCP1(DAC1addr);
MCP4725 MCP2(DAC2addr);
ADS1115 ADS(ADCaddr);
LiquidCrystal_I2C lcd(LCDaddr, 20, 4);

SCPI_Parser my_instrument;
EthernetServer server(5555);
EthernetClient client;

operationMode Mode;


void setup() {
#ifdef DEBUG
    Serial.begin(115200);
    Serial.println("ImHere");
#endif // DEBUG

//============================================================
//DAC & ADC 
//============================================================
	//============
	/// DAC INIT 
	//============
	if (MCP1.begin() == false)
    {
        Serial.println("Could not init DAC1");
    }  
    if (MCP2.begin() == false)
    {
        Serial.println("Could not init DAC2");
    }
    
	//================
	/// ADC init
	//=================
    ADS.begin();
    if (!ADS.isConnected()) {
        Serial.println("ADS1115 not presant on bus , at expected Addr 0x48");
    }
    Serial.println("ADS connected on bus,at expected Addr 0x48");
	ADCVoltageFactor = ADS.toVoltage();
	Serial.print("the ADC Voltage factor is : ");
    Serial.print(ADCVoltageFactor, 5);
	
	    
//============================================================
//LCD 
//============================================================

    lcd.init();
    lcd.clear();
	lcd.createChar(0, A);
	lcd.createChar(1, V2);
    lcd.createChar(3, R);
	lcd.createChar(2, W);

    lcd.setCursor(0, 0);
    lcd.backlight();
	
	splashScreen();
	delay(100);
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
	attachInterrupt(digitalPinToInterrupt(EncBTN), encoderButtonISR, FALLING);

#endif // encoderConnected
    
#ifdef modeButtons
    pinMode(CVbtn, INPUT_PULLUP);
    pinMode(CCbtn, INPUT_PULLUP);
    pinMode(CPbtn, INPUT_PULLUP);
    pinMode(CRbtn, INPUT_PULLUP);
	pinMode(BATsw, INPUT_PULLUP);

	attachInterrupt(digitalPinToInterrupt(CVbtn), CVbuttonISR, FALLING);
	attachInterrupt(digitalPinToInterrupt(CCbtn), CCbuttonISR, FALLING);
	attachInterrupt(digitalPinToInterrupt(CPbtn), CPbuttonISR, FALLING);
	attachInterrupt(digitalPinToInterrupt(CRbtn), CRbuttonISR, FALLING);
	attachInterrupt(digitalPinToInterrupt(BATsw), batteryISR, FALLING);
#endif //modeButtons

#ifdef StartStopButtons
    pinMode(StartBtn, INPUT_PULLUP);
    pinMode(StopBtn, INPUT_PULLUP);
    pinMode(settingsSW, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(StartBtn), startButtonISR, FALLING);
	attachInterrupt(digitalPinToInterrupt(StopBtn), stopButtonISR, FALLING);
	attachInterrupt(digitalPinToInterrupt(settingsSW), settingsISR, FALLING);
#endif //StartStopButtons
	
//=================================
//SCPI
//=================================
	
	my_instrument.RegisterCommand(F("*IDN?"), &Identify);
	my_instrument.RegisterCommand(F("SYSTem:ERRor?"), &GetLastEror);
	my_instrument.SetErrorHandler(&myErrorHandler);	
	//Not setting an error handler will just ignore the errors.

	
	Ethernet.init(ETH_CS); // initialize the Ethernet library
	if (Ethernet.linkStatus() == LinkON) { // check if cable is connected
		Serial.println("Ethernet cable connected");
		EthSetupStatus = true;
		if (Ethernet.begin(mac) == 0) { // start DHCP
			Serial.println("Failed to get IP address using DHCP");
			// you can also assign a static IP address if DHCP fails, see the Ethernet library examples
			lcd.setCursor(0, 3);
			lcd.print("LAN connected DHCP failed");
			delay(2000);
			lcd.clear();
			drawLCD();
		}//FailedDHCP loop (consider a hardcoded fallback Static?
		else
		{
			Serial.print("IP address: ");
			Serial.println(Ethernet.localIP());
			server.begin(); // start the server
			Serial.print("Server is at ");
			Serial.println(Ethernet.localIP()); // print the server IP address to the serial monitor
			lcd.setCursor(0, 3);
			lcd.print("Ehternet connected");//Update the Splasscreen
			delay(200);
			lcd.clear();
			drawLCD();//Clean and continue to the normal flow of the program
		}//Got address from DHCP
	}
	else {
		Serial.println("Ethernet cable disconnected");
		EthSetupStatus = false;
		lcd.setCursor(0, 3);
		lcd.print("Ehternet Not Connected");//Update the Splasscreen
		delay(4000);
		lcd.clear();
		drawLCD();//Clean and contineew to the normal flow of the program
	}//Ethernet Not connected.


	calibrateACS();
    Serial.println("\n end of setup()");
}

void loop() {
	if (batteryMode) t1 = millis();
	/*readCurrent();
	readVoltage();
	CalcPower();
	CalcResistance();*/
	Readings.Voltage = readVoltage();
	Readings.Current = readCurrent();
	if (batteryMode) {
		Readings.Power = Readings.Voltage * Readings.Current;
		Readings.AmpHrs += Readings.Current * (t2) / 3600000;
		Readings.WattHrs += Readings.Power * (t2) / 3600000;
	}
	else {
		Readings.Power = Readings.Voltage * Readings.Current;
		Readings.Resistance = Readings.Voltage / Readings.Current;
	}
	Serial.print(t1);
	Serial.print(",");
	Serial.print(t2);
	Serial.print(",");
	//Serial.print(SclearScreen);
	Serial.print(",");
	Serial.print(Readings.AmpHrs);
	Serial.print(",");
	Serial.println(Readings.WattHrs);

	//===============================
	//Diferent screens
	//===============================


	if (!(settingsButtonState || batteryMode)) {
		if (clearScreen) {
			lcd.clear();
			drawLCD();
			clearScreen = false;
		}		
		updateLCD();
	}
	else if (batteryMode) {
		if (clearScreen) {
			lcd.clear();
			batteryModeLCD();
			clearScreen = false;
		}
		updateBatteryMode();
	}
	else {
		if (clearScreen) {
			lcd.clear();
			clearScreen = false;
		}
		settingsLCD();
	}
	if (isRuning) {
		runLoad();
		Serial.print("DAC1:");
		Serial.print(DAC1Point);
		Serial.print(" DAC2:");
		Serial.println(DAC2Point);
	}
	else
	{
		MCP1.writeDAC(0);
		MCP2.writeDAC(0);
		DAC1Point = 0;
		DAC2Point = 0;
	}
	copyEncoder();

	if (antiFlicker == 5) antiFlicker = 0;
	antiFlicker++;
//==========================================================
//SCPI
//==========================================================

	client = server.available();
	if (client.connected()) {
		my_instrument.ProcessInput(client, "\r\n");//Ethercard.h was using \n termination 
	};
	if (batteryMode) t2 = millis() - t1;
//delay(10);
//End of Loop();

	
}

void runLoad() {
	if (!batteryMode) {
		switch (Mode) {
		case CC:
			if (setPoints.Current > Readings.Current) {
				incrementDAC(1);
			}
			else {
				decrementDAC(1);
			}
			break;
		case CV:
			if (setPoints.Voltage < Readings.Voltage) {
				incrementDAC(1);
			}
			else {
				decrementDAC(1);
			}
			break;
		case CP:
			if (setPoints.Power > Readings.Power) {
				incrementDAC(1);
			}
			else {
				decrementDAC(1);
			}
			break;
		case CR:
			if (setPoints.Resistance < Readings.Resistance) {
				incrementDAC(1);
			}
			else {
				decrementDAC(1);
			}
			break;
		}
	}
	else
	{
		if (battPoints.Current > Readings.Current) {
			incrementDAC(1);
		}
		else {
			decrementDAC(1);
		}
		if (battPoints.Vstop > Readings.Voltage) isRuning = false;
	}
	MCP1.writeDAC(DAC1Point);
	MCP2.writeDAC(DAC2Point);
}

void incrementDAC(int val) {
	if (MCP) {
		DAC1Point += val;
		if (DAC1Point > 4095) DAC1Point = 4095;
		MCP = false;
	}
	else {
		DAC2Point += val;
		if (DAC2Point > 4095) DAC2Point = 4095;
		MCP = true;
	}
}

void decrementDAC(int val) {
	if (!MCP) {
		DAC1Point -= val;
		if (DAC1Point < 0) DAC1Point = 0;
		MCP = true;
	}
	else {
		DAC2Point -= val;
		if (DAC2Point < 0) DAC2Point = 0;
		MCP = false;
	}
}

void copyEncoder() {
	if (!batteryMode) {
		switch (Mode) {
		case CC:
			setPoints.Current += (EncoderPos * (multiplier / 100.0));
			if (setPoints.Current < 0) {
				setPoints.Current = 0;
			}
			if (setPoints.Current > Imax) {
				setPoints.Current = Imax;
			}
			break;
		case CV:
			setPoints.Voltage += (EncoderPos * (multiplier / 100.0));
			if (setPoints.Voltage < 0) {
				setPoints.Voltage = 0;
			}
			if (setPoints.Voltage > Vmax) {
				setPoints.Voltage = Vmax;
			}
			break;
		case CP:
			setPoints.Power += (EncoderPos * (multiplier / 100.0));
			if (setPoints.Power < 0) {
				setPoints.Power = 0;
			}
			break;
		case CR:
			setPoints.Resistance += (EncoderPos * (multiplier));
			if (setPoints.Resistance < 0) {
				setPoints.Resistance = 0;
			}
			break;
		}
	}
	else {
		switch (Mode) {
		case CC:
			battPoints.Current += (EncoderPos * (multiplier / 100.0));
			if (battPoints.Current < 0) {
				battPoints.Current = 0;
			}
			if (battPoints.Current > Imax) {
				battPoints.Current = Imax;
			}
			break;
		case CV:
			battPoints.Vstop += (EncoderPos * (multiplier / 100.0));
			if (battPoints.Vstop < 0) {
				battPoints.Vstop = 0;
			}
			if (battPoints.Vstop > Vmax) {
				battPoints.Vstop = Vmax;
			}
			break;
		case CP:
			battPoints.CapLimit += (EncoderPos * (multiplier / 100.0));
			if (battPoints.CapLimit < 0) {
				battPoints.CapLimit = 0;
			}
			break;

		}
	}
	EncoderPos = 0;
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
	if (Mode != CC) {
		lcd.setCursor(14, 1);
		lcd.print("A");
	}
	if (Mode != CV) {
		lcd.setCursor(14, 0);
		lcd.print("V");
	}
	if (Mode != CP) {
		lcd.setCursor(14, 2);
		lcd.print("W");
	}
	if (Mode != CR) {
		lcd.setCursor(14, 3);
		lcd.print("R");
	}
	
}

void batteryModeLCD() {
	lcd.setCursor(6, 0);
	lcd.print("V|");
	lcd.setCursor(6, 1);
	lcd.print("A|");
	lcd.setCursor(5, 2);
	lcd.print("Ah|");
	lcd.setCursor(5, 3);
	lcd.print("Wh|");
	lcd.setCursor(14, 0);
	lcd.print("V_Stop");//Cuttof point
	lcd.setCursor(14, 1);
	lcd.print("A");
	lcd.setCursor(14, 2);
	lcd.print("C_lim");//Cuttof POint
	//lcd.setCursor(14, 3);
	//lcd.print("");
	if (isRuning) {
		lcd.setCursor(16, 3);
		lcd.print("RUN ");
	}
	else {
		lcd.setCursor(16, 3);
		lcd.print("STOP");
	}
}

void updateBatteryMode() {
	if (antiFlicker == 1) {
		lcd.setCursor(0, 0);
		clearAndPrintFloat(Readings.Voltage, 2);
		lcd.setCursor(0, 1);
		clearAndPrintFloat(Readings.Current, 3);
		lcd.setCursor(0, 2);
		clearAndPrint4Float(Readings.AmpHrs, 2);
		lcd.setCursor(0, 3);
		clearAndPrint4Float(Readings.WattHrs, 2);
	}
	lcd.setCursor(8, 0);

	if (Mode == CV) {
		clearAndPrintFloat(battPoints.Vstop, 2);
	}
	else {
		lcd.print(" ---- ");
	}
	lcd.setCursor(8, 1);
	if (Mode==CC) {
		clearAndPrintFloat(battPoints.Current, 2);
	}
	else {
		lcd.print(" ---- ");
	}
	lcd.setCursor(8, 2);
	if (Mode==CP) {
		clearAndPrintFloat(battPoints.CapLimit, 2);
	}
	else {
		lcd.print(" ---- ");
	}
	//lcd.setCursor(8, 3);
	//if (mode == 3) {
	//	clearAndPrintFloat(battPoints.Resistance, 0);
	//}
	//else {
	//	//lcd.print(" ---- ");
	//}
	if (isRuning) {
		lcd.setCursor(16, 3);
		lcd.print("RUN ");
	}
	else {
		lcd.setCursor(16, 3);
		lcd.print("STOP");
	}
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
	if (antiFlicker == 1) {
		lcd.setCursor(0, 0);
		clearAndPrintFloat(Readings.Voltage, 2);
		lcd.setCursor(0, 1);
		clearAndPrintFloat(Readings.Current, 3);
		lcd.setCursor(0, 2);
		clearAndPrintFloat(Readings.Power, 3);
		lcd.setCursor(0, 3);
		clearAndPrintFloat(Readings.Resistance, 0);
	}
	lcd.setCursor(8, 0);
	if (Mode == CV) {
		clearAndPrintFloat(setPoints.Voltage, 2);
	}
	else {
		lcd.print(" ---- ");
	}
	lcd.setCursor(8, 1);
	if (Mode == CC) {
		clearAndPrintFloat(setPoints.Current, 2);
	}
	else {
		lcd.print(" ---- ");
	}
	lcd.setCursor(8, 2);
	if (Mode == CP) {
		clearAndPrintFloat(setPoints.Power, 2);
	}
	else {
		lcd.print(" ---- ");
	}
	lcd.setCursor(8, 3);
	if (Mode == CR) {
		clearAndPrintFloat(setPoints.Resistance, 0);
	}
	else {
		lcd.print(" ---- ");
	}
	if (isRuning) {
		lcd.setCursor(16, 3);
		lcd.print("RUN ");
	}
	else {
		lcd.setCursor(16, 3);
		lcd.print("STOP");
	}
	
	switch (Mode)
	{
		case CC:
			drawLCD();
			lcd.setCursor(14, 1);
			lcd.write(0);
			//Serial.print("Mode: 0\n");
			break;
		case CV:
			drawLCD();
			lcd.setCursor(14, 0);
			lcd.write(1);
			//Serial.print("Mode: 1\n");
			break;
		case CP:
			drawLCD();
			lcd.setCursor(14, 2);
			lcd.write(2);
			//Serial.print("Mode: 2\n");
			break;
		case CR:
			drawLCD();
			lcd.setCursor(14, 3);
			lcd.write(3);
			//Serial.print("Mode: 3\n");
			break;
	default:
		//Serial.print("Default\n");
		break;
	}
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
	lcd.print("IPv4:");
	lcd.print(Ethernet.localIP());
	lcd.setCursor(0, 3);
	lcd.print("External Sense:O");//To reduce flicker from the loop(); refresh , we have the "o" Permanent 
	if (extSense) {
		lcd.print("N ");//, Write N for " ON" 
	}
	else {
		lcd.print("FF");//or , Write FF for "OFF" Overriding the N 
	}//thus updaing only whats needed
}



void clearAndPrintFloat(float num, int digits) {
	lcd.print(num, digits);// Print the number to the LCD with the specified number of digits.
	digits++;// Increment the number of digits to account for the decimal point.
	while (num >= 10) {// Count the number of digits in the integer part of the number.
		digits++;
		num /= 10;
	}
	for (byte i = 0; i < 5 - digits; i++) {// Print spaces to fill the remaining characters on the LCD display.
		lcd.print(" ");
	}
}

void clearAndPrint4Float(float num, int digits) {
	lcd.print(num, digits);// Print the number to the LCD with the specified number of digits.
	digits++;// Increment the number of digits to account for the decimal point.
	while (num >= 10) {// Count the number of digits in the integer part of the number.
		digits++;
		num /= 10;
	}
	for (byte i = 0; i < 4 - digits; i++) {
		lcd.print(" ");
	}// Print spaces to fill the remaining characters on the LCD display. With the Diff being we fill 4 parts ,
	//to account for the extra letters (Ah , Wh)
}

void splashScreen() {

	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Electronic DC load");
	lcd.setCursor(1, 1);
	lcd.print("0-40V 0-20A ");
	lcd.setCursor(8, 2);
	lcd.print("THMMY");
}

//============================================================
// Interupt Service Routines 
//============================================================

void EncoderISR() {
	if (settingsButtonState) {
		return;

	}	
	static uint8_t state = 0;
	bool CLKstate = digitalRead(EncB);
	bool DTstate = digitalRead(EncA);
	switch (state) {
	case 0:                         // Idle state, encoder not turning
		if (!CLKstate) {             // Turn clockwise and CLK goes low first
			state = 1;
		}
		else if (!DTstate) {      // Turn anticlockwise and DT goes low first
			state = 4;
		}
		break;
		// Clockwise rotation
	case 1:
		if (!DTstate) {             // Continue clockwise and DT will go low after CLK
			state = 2;
		}
		break;
	case 2:
		if (CLKstate) {             // Turn further and CLK will go high first
			state = 3;
		}
		break;
	case 3:
		if (CLKstate && DTstate) {  // Both CLK and DT now high as the encoder completes one step clockwise
			state = 0;
			++EncoderPos;
		}
		break;
		// Anticlockwise rotation
	case 4:                         // As for clockwise but with CLK and DT reversed
		if (!CLKstate) {
			state = 5;
		}
		break;
	case 5:
		if (DTstate) {
			state = 6;
		}
		break;
	case 6:
		if (CLKstate && DTstate) {
			state = 0;
			--EncoderPos;
		}
		break;
	}
}
void encoderButtonISR() {
	if (!settingsButtonState) {
		if (multiplier < 10) {
			multiplier *= 100;
		}
		else {
			multiplier = 1;
		}
	}//If the instrument is at the main screen , the Encoder`s button 
	// will opperate for seting the seting pressision. 
	else {
		if (extSense) {
			extSense = false;
		}
		else {
			extSense = true;
		}
	}//when on Setings menu , it will be used for enabling or disabling the 
	// external sense . 
}

void CCbuttonISR() {
	Serial.println("CC");
	//mode = 0;
	Mode = CC;
	//Serial.println(mode);
}

void CVbuttonISR() {
	Serial.println("CV");
	//mode = 1;
	Mode = CV;
	//Serial.println(mode);
}

void CRbuttonISR() {
	if (batteryMode) return; // Disable on battery mode
	Serial.println("CR");
	//mode = 3;
	Mode = CR;
	//Serial.println(mode);
}

void CPbuttonISR() {
	Serial.println("CP");
	//mode = 2;
	Mode = CP;
	//Serial.println(mode);
}

void batteryISR() {
	Serial.println("BAT");
	Mode = CC;
	clearScreen = true;
	//Serial.println(mode);
	settingsButtonState = false;
	if (batteryMode) {
		batteryMode = false;
	}
	else {
		batteryMode = true;
	}
	
}

void startButtonISR() {
	Serial.println("Start");
	Serial.println(writeState);
	isRuning = true;
}
void stopButtonISR() {
	Serial.println("Stop");
	isRuning = false;
}

void settingsISR() {
	Serial.println("SET");
	batteryMode = false;//Disable the battery mode,
	if (settingsButtonState) {
		settingsButtonState = false;
	}//If setings are ON, close/disable them. 
	else {
		settingsButtonState = true;
	}//If Setings are OFF , Open/Enable them 
	clearScreen = true;//Set a flag to clear the screen , to prepare for the new layout
}



//============================================================
// Support Functions
//============================================================

//void readVoltage() {
//	float ADCA0 = 0.0;
//	float ADCA1 = 0.0;
//	ADCA0 = ADS.readADC(0);
//	ADCA1 = ADS.readADC(1);
//	VoltExternalRaw = ADCA0 * ADCVoltageFactor;
//	VoltInternalRaw = ADCA1 * ADCVoltageFactor;
//	//Vin=(VoltExternalRaw*(R11+R12+R2)/R2;
//	VoltExternal = (VoltExternalRaw * (R11 + R12 + R2) / R2);
//	VoltInternal = (VoltInternalRaw * (R11 + R12 + R2) / R2);
//}

float readVoltage() {
	float ADC = 0.0;	
	ADC = (!extSense) ? ADS.readADC(0) : ADS.readADC(1);//Whyyy TIFOMI , WHYYYY
	float VoltRaw = ADC * ADCVoltageFactor;
	if (VoltRaw < 0) VoltRaw = 0;

	
	
	if (!extSense) {
		return VoltRaw * (R7 + R8 + R9) /R9;
	}
    else {
		return VoltRaw * (R10 + R11 + R12) / R12 ;
	}
}

float readCurrent() {
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
	if (Current < 0.0001) Current = 0.0001;
	return Current; //Crude , i will come back to this , its 2:20AM give me A brake
}


void CalcPower() {
	PowerExternal = readVoltage() * Current;
}

void CalcResistance() {
	if (abs(readCurrent()) == 00.00) {
		RLoad = 00.00;
	}
	else {
		RLoad = abs(readVoltage())/ abs(readCurrent());
	}
}

//float CalcWattHours(float V_in, float A_drawn) {
//	//float kwh;
//	//float ah;
//	//uint32_t ts1 = millis();
//
//	//uint32_t ts2 = millis();//This needs to be Corrected
//
//
//	//// print the time interval in seconds
//	//uint32_t ts3 = (ts2 - ts1) / 1000;
//	//totalET = totalET + ts3;
//	//Serial.print(“Seconds: “);
//	//Serial.println(totalET);
//
//	//kwh = PowerExternal * ts3 / 3600;
//	//ah = kwh / VoltExternal;
//	
//
// // Check if V_in is above V_Stop
//	if (V_in > V_Stop) {
//		// If this is the first time calling the function, set startTime to current millis()
//		if (startTime == 0) {
//			startTime = millis();
//		}
//		// Calculate the elapsed time in hours
//		float elapsedTime = (millis() - startTime) / 3600000.0;
//		// Calculate battery capacity in mAh
//		float batteryCapacity = A_drawn * elapsedTime;
//		// Check if battery capacity is below C_Stop
//		if (batteryCapacity < C_Stop) {
//			// Return battery capacity
//			return batteryCapacity;
//		}
//		else {
//			// If battery capacity is above C_Stop, reset startTime to zero and return zero
//			startTime = 0;
//			// Set flag to true when the Capacity stop point is reached.
//			cflag = true;
//			return 0;
//		}
//	}
//	else {
//		// If V_in is below V_Stop, reset startTime to zero and return zero
//		startTime = 0;
//		// Set flag and vFlag to true When Voltage stop point is reached.
//		flag = true;
//		vFlag = true;
//		return 0;
//	}
//}

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



//=======================================
// SCPI Functions 
//=======================================


void myErrorHandler(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	//This function is called every time an error occurs

	/* The error type is stored in my_instrument.last_error
	   Possible errors are:
		 SCPI_Parser::ErrorCode::NoError
		 SCPI_Parser::ErrorCode::UnknownCommand
		 SCPI_Parser::ErrorCode::Timeout
		 SCPI_Parser::ErrorCode::BufferOverflow
	*/

	/* For BufferOverflow errors, the rest of the message, still in the interface
	buffer or not yet received, will be processed later and probably
	trigger another kind of error.
	Here we flush the incomming message*/
	if (my_instrument.last_error == SCPI_Parser::ErrorCode::BufferOverflow) {
		delay(2);
		while (interface.available()) {
			delay(2);
			interface.read();
		}
	}

	/*
	For UnknownCommand errors, you can get the received unknown command and
	parameters from the commands and parameters variables.
	*/
}

void Identify(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	interface.println(F("Vrekrer,SCPI Error Handling Example,#00,v0.4.2"));
}

void GetLastEror(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	switch (my_instrument.last_error) {
	case my_instrument.ErrorCode::BufferOverflow:
		interface.println(F("Buffer overflow error"));
		break;
	case my_instrument.ErrorCode::Timeout:
		interface.println(F("Communication timeout error"));
		break;
	case my_instrument.ErrorCode::UnknownCommand:
		interface.println(F("Unknown command received"));
		break;
	case my_instrument.ErrorCode::NoError:
		interface.println(F("No Error"));
		break;
	}
	my_instrument.last_error = my_instrument.ErrorCode::NoError;
}

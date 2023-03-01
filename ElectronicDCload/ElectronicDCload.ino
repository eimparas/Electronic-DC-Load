#include <LiquidCrystal_I2C.h>
#include <ADS1X15.h>
#include <MCP4725.h>
#include "Arduino.h"
#include "Vrekrer_scpi_parser.h"
#include <SPI.h>
#include <EthernetENC.h>
#include "pinout.h"


MCP4725 MCP1(DAC1addr);
MCP4725 MCP2(DAC2addr);
ADS1115 ADS(ADCaddr);
LiquidCrystal_I2C lcd(LCDaddr, 20, 4);

SCPI_Parser my_instrument;
EthernetServer server(5555);
EthernetClient client;
byte mac[6] = { 0x74, 0x69, 0x69, 0x2D, 0x30, 0x31 };

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
bool SclearScreen = false;
bool settingsButtonState = false;
bool encoderButtonState =false;
bool CVbuttonState = false;
bool extSense = false;
bool isRuning = false;
bool batteryMode = false;
bool EthSetupStatus = false;
int mode = 0;//CC default , CC:0, CV:1,CR:2,CP:3

float CVoltageSet = 0.0;

float ADCVoltageFactor = 0;

float Current = 0.0;
float ACS1_A = 0.0;
float ACS2_A = 0.0;
float ACS1_offset = 2.47;
float ACS2_offset = 2.47;
float acs1Raw = 0.0;
float acs2Raw = 0.0;

struct setPoint
{
	float Voltage;
	float Current;
	float Resistance;
	float Power;
} setPoints;

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

	
	Ethernet.init(5); // initialize the Ethernet library
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
			drawLCD();//Clean and contineew to the normal flow of the program
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
	
	/*readCurrent();
	readVoltage();
	CalcPower();
	CalcResistance();*/
	Serial.print(setPoints.Voltage);
	Serial.print(",");
	Serial.print(setPoints.Current);
	Serial.print(",");
	//Serial.print(SclearScreen);
	Serial.print(",");
	Serial.print(setPoints.Power);
	Serial.print(",");
	Serial.println(setPoints.Resistance);

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
			clearScreen = false;
		}
		batteryModeLCD();
	}
	else {
		if (clearScreen) {
			lcd.clear();
			clearScreen = false;
		}
		settingsLCD();
	}
	
	switch (mode) {
		case 0:
			setPoints.Current += (EncoderPos * (multiplier / 100));
			if (setPoints.Current < 0) {
				setPoints.Current = 0;
			}
			break;
		case 1:
			setPoints.Voltage += (EncoderPos * (multiplier / 100));
			if (setPoints.Voltage < 0) {
				setPoints.Voltage = 0;
			}
			break;
		case 2:
			setPoints.Power += (EncoderPos * (multiplier / 100));
			if (setPoints.Power < 0) {
				setPoints.Power = 0;
			}
			break;
		case 3:
			setPoints.Resistance += (EncoderPos * (multiplier));
			if (setPoints.Resistance < 0) {
				setPoints.Resistance = 0;
			}
			break;
	}
	EncoderPos = 0;


//==========================================================
//SCPI
//==========================================================

	client = server.available();
	if (client.connected()) {
		my_instrument.ProcessInput(client, "\r\n");//Ethercard.h was using \n termination 
	};

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
	if (mode != 0) {
		lcd.setCursor(14, 1);
		lcd.print("A");
	}
	if (mode != 1) {
		lcd.setCursor(14, 0);
		lcd.print("V");
	}
	if (mode != 2) {
		lcd.setCursor(14, 2);
		lcd.print("W");
	}
	if (mode != 3) {
		lcd.setCursor(14, 3);
		lcd.print("R");
	}
	/*lcd.setCursor(14, 0);
	lcd.print("V");
	lcd.setCursor(14, 1);
	lcd.print("A");
	lcd.setCursor(14, 2);
	lcd.print("W");
	lcd.setCursor(14, 3);
	lcd.print("R");*/
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
	clearAndPrintFloat(abs( VoltExternal),2);
	lcd.setCursor(0, 1);
	clearAndPrintFloat(abs( Current),3);
	lcd.setCursor(0, 2);
	clearAndPrintFloat(abs(PowerExternal),3);
	lcd.setCursor(0, 3);
	clearAndPrintFloat(abs(RLoad),0);
	lcd.setCursor(8, 0);
	if (mode == 1) {
		clearAndPrintFloat(setPoints.Voltage, 2);
	}
	else {
		lcd.print(" ---- ");
	}
	lcd.setCursor(8, 1);
	if (mode == 0) {
		clearAndPrintFloat(setPoints.Current, 2);
	}
	else {
		lcd.print(" ---- ");
	}
	lcd.setCursor(8, 2);
	if (mode == 2) {
		clearAndPrintFloat(setPoints.Power, 2);
	}
	else {
		lcd.print(" ---- ");
	}
	lcd.setCursor(8, 3);
	if (mode == 3) {
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
	
	switch (mode)
	{
		case 0:
			drawLCD();
			lcd.setCursor(14, 1);
			lcd.write(0);
			//Serial.print("Mode: 0\n");
			break;
		case 1:
			drawLCD();
			lcd.setCursor(14, 0);
			lcd.write(1);
			//Serial.print("Mode: 1\n");
			break;
		case 2:
			drawLCD();
			lcd.setCursor(14, 2);
			lcd.write(2);
			//Serial.print("Mode: 2\n");
			break;
		case 3:
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
	lcd.print("External Sense:O");
	if (extSense) {
		lcd.print("N ");
	}
	else {
		lcd.print("FF");
	}
}

void batteryModeLCD() {
	lcd.setCursor(0, 0);
	lcd.print("targetVoltage:");
	lcd.setCursor(0, 1);
	lcd.print("Current :");
	lcd.setCursor(0, 2);
	lcd.print("Capacity: ");
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
	bool printFlag = false;
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
			printFlag = true;
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
			printFlag = true;
		}
		break;
	}
}
void encoderButtonISR() {
	if (!settingsButtonState) {
		if (multiplier <= 10) {
			multiplier *= 10;
		}
		else {
			multiplier = 1.0;
		}
	}
	else {
		if (extSense) {
			extSense = false;
		}
		else {
			extSense = true;
		}
	}
}

void CCbuttonISR() {
	Serial.println("CC");
	mode = 0;
	Serial.println(mode);
}

void CVbuttonISR() {
	Serial.println("CV");
	CVbuttonState = true;
	mode = 1; 
	Serial.println(mode);
}

void CRbuttonISR() {
	Serial.println("CR");
	mode = 3;
	Serial.println(mode);
}

void CPbuttonISR() {
	Serial.println("CP");
	mode = 2;
	Serial.println(mode);
}

void batteryISR() {
	Serial.println("BAT");
	mode = 4;
	clearScreen = true;
	Serial.println(mode);
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
	batteryMode = false;
	if (settingsButtonState) {
		settingsButtonState = false;
	}
	else {
		settingsButtonState = true;
	}
	clearScreen = true;
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
	if (abs( Current ) == 00.00) {
		RLoad = 00.00;
	}
	else {
		RLoad = abs( VoltExternal )/ abs(Current);
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

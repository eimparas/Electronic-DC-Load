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
//bool EthSetupStatus = false;
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
	SCPIsetup();

	Ethernet.init(ETH_CS); // initialize the Ethernet library
	if (Ethernet.linkStatus() == LinkON) { // check if cable is connected
		Serial.println("Ethernet cable connected");
		//EthSetupStatus = true;
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
		//EthSetupStatus = false;
		lcd.setCursor(0, 3);
		lcd.print("Ehternet Not Connected");//Update the Splasscreen
		delay(4000);
		lcd.clear();
		drawLCD();//Clean and continue to the normal flow of the program
	}//Ethernet Not connected.


	calibrateACS();
    Serial.println("\n end of setup()");

}

void loop() {
	if (batteryMode) t1 = millis();
	Readings.Voltage = readVoltage();
	Readings.Current = readCurrent();
	Readings.Power = Readings.Voltage * Readings.Current;

	
	if (batteryMode) {
		Readings.AmpHrs += Readings.Current * (t2) / 3600000;
		Readings.WattHrs += Readings.Power * (t2) / 3600000;
	}
	else {
		Readings.Resistance = Readings.Voltage / Readings.Current;
	}

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
	#ifdef DEBUG
		Serial.print("DAC1:");
		Serial.print(DAC1Point);
		Serial.print(" DAC2:");
		Serial.println(DAC2Point);
	#endif // DEBUG
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
//End of Loop();	
}

void runLoad() {
	if (!batteryMode) {	// If not in battery mode, adjust the DACs based on the specified mode and setpoints.
		switch (Mode) {
		case CC:// In constant current (CC) mode, adjust the DACs based on the difference between the set current and the measured current.
			if (setPoints.Current > Readings.Current) {
				incrementDAC(setPoints.Current - Readings.Current);
			}
			else {
				decrementDAC(Readings.Current-setPoints.Current);
			}
			break;
		case CV:// In constant voltage (CV) mode, adjust the DACs based on the difference between the set voltage and the measured voltage.

			if (setPoints.Voltage < Readings.Voltage) {
				incrementDAC(Readings.Voltage - setPoints.Voltage);
			}
			else {
				decrementDAC(setPoints.Voltage - Readings.Voltage);
			}
			break;
		case CP:// In constant power (CP) mode, adjust the DACs based on the difference between the set power and the measured power.
			if (setPoints.Power > Readings.Power) {
				incrementDAC(setPoints.Power - Readings.Power);
			}
			else {
				decrementDAC(Readings.Power - setPoints.Power);
			}
			break;
		case CR:// In constant resistance (CR) mode, adjust the DACs based on the difference between the set resistance and the measured resistance.
			if (setPoints.Resistance < Readings.Resistance) {
				incrementDAC(Readings.Resistance - setPoints.Resistance);
			}
			else {
				decrementDAC(setPoints.Resistance - Readings.Resistance);
			}
			break;
		}
	}
	else// If in battery mode, adjust the DACs based on the battery setpoints and stop running if the voltage exceeds the battery's stop voltage.
	{
		if (battPoints.Current > Readings.Current) {
			incrementDAC(1);
		}
		else {
			decrementDAC(1);
		}
		if (battPoints.Vstop > Readings.Voltage) isRuning = false;// Stop running if the voltage is above the battery stop voltage setpoint.
	}

}

void incrementDAC(float diff) {
	// Determine the value by which to increment the DACs based on the magnitude of the difference.
	int val;
	if (diff > 1) {
		val = 100;
	}
	else if (diff > 0.1) {
		val = 10;
	}
	else {
		val = 1;
	}

	// Increment the DACs by 'val' until 'val' is 0.
	while (val != 0) {

		// Switch between incrementing the output of the first DAC (DAC1) and the second DAC (DAC2).
		if (MCP) {
			DAC1Point += 1;
			if (DAC1Point > 4095) DAC1Point = 4095;
			MCP = false;
		}
		else {
			DAC2Point += 1;
			if (DAC2Point > 4095) DAC2Point = 4095;
			MCP = true;
		}
		val--;
	}

	// Write the new output values to the DACs.
	MCP1.writeDAC(DAC1Point);
	MCP2.writeDAC(DAC2Point);
	
}

void decrementDAC(float diff) {
	/*if (!MCP) {
		DAC1Point -= val;
		if (DAC1Point < 0) DAC1Point = 0;
		MCP = true;
	}
	else {
		DAC2Point -= val;
		if (DAC2Point < 0) DAC2Point = 0;
		MCP = false;
	}*/

	// Determine the value by which to decrement the DACs based on the magnitude of the difference.
	int val;
	if (diff > 1) {
		val = 100;
	}
	else if (diff > 0.1) {
		val = 10;
	}
	else {
		val = 1;
	}

	// Decrement the DACs by 'val' until 'val' is 0.
	while (val != 0) {

		// Switch between decrementing the output of the first DAC (DAC1) and the second DAC (DAC2).
		if (MCP) {
			DAC1Point -= 1;
			if (DAC1Point < 0) DAC1Point = 0;
			MCP = false;
		}
		else {
			DAC2Point -= 1;
			if (DAC2Point < 0) DAC2Point = 0;
			MCP = true;
		}
		val--;
	}

	// Write the new output values to the DACs.
	MCP1.writeDAC(DAC1Point);
	MCP2.writeDAC(DAC2Point);
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
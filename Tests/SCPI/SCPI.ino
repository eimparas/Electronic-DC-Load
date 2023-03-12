#include <Vrekrer_scpi_parser.h>
#include <SPI.h>
#include <EthernetENC.h>

//const static byte dns[] = { 0, 0, 0, 0 };
byte mac[6] = { 0x74, 0x69, 0x69, 0x2D, 0x30, 0x31 };


const byte csPin = 5; //ChipSelect Pin

SCPI_Parser my_instrument;
EthernetServer server(5555);
EthernetClient client;

boolean fromSerial = true;

void setup() {
    Serial.begin(9600);
    while (!Serial);
	
	
	
	
    my_instrument.RegisterCommand("*IDN?", &Identify);
	my_instrument.SetCommandTreeBase(F("SYSTem"));
	my_instrument.RegisterCommand(F("TEMPerature?"), &GetTemperature);	
    my_instrument.RegisterCommand(F("ERRor?"), &GetLastEror);
	//Reset the command tree base to the root level
	my_instrument.SetCommandTreeBase("");
    my_instrument.SetErrorHandler(&myErrorHandler);
    /*
	my_instrument.SetCommandTreeBase("MEASure");
    my_instrument.RegisterCommand(":VOLTage?", &SetIP);
    my_instrument.RegisterCommand(":CURRent?", &GetIP);
    my_instrument.RegisterCommand(":POWEr?", &SetGW);
    my_instrument.SetCommandTreeBase("SET:");
    my_instrument.RegisterCommand(":VOLTage?", &SetIP);
    my_instrument.RegisterCommand(":CURRent?", &GetIP);
    my_instrument.RegisterCommand(":POWEr?", &SetGW);
    ======
	my_instrument.SetCommandTreeBase(F("SOURce"));
		my_instrument.RegisterCommand(F(":VOLTage:VLIMt"), &SetVoltageLimit);
		my_instrument.RegisterCommand(F(":VOLTage:VLIMt?"), &GetVoltageLimit);
		my_instrument.RegisterCommand(F(":CURRent:ILIMt"), &SetCurrentLimit);
		my_instrument.RegisterCommand(F(":CURRent:ILIMt?"), &GetCurrentLimit);
		my_instrument.RegisterCommand(F(":RESistance:RLIMt"), &SetResistanceLimit);
		my_instrument.RegisterCommand(F(":RESistance:RLIMt?"), &GetResistanceLimit);
		my_instrument.RegisterCommand(F(":POWer:PLIMt"), &SetPowerLimit);
		my_instrument.RegisterCommand(F(":POWer:PLIMt?"), &GetPowerLimit);
		my_instrument.RegisterCommand(F(":INPut:STATe"), &SetInputState);
		my_instrument.RegisterCommand(F(":INPut:STATe?"), &GetInputState);
		my_instrument.RegisterCommand(F(":LIST:MODE"), &SetListMode);
		my_instrument.RegisterCommand(F(":LIST:MODE?"), &GetListMode);
	my_instrument.SetCommandTreeBase("");
	
	my_instrument.SetCommandTreeBase(F("MEASure"));
		my_instrument.RegisterCommand(F(":VOLTage:DC?"), &GetVoltageDC);
		my_instrument.RegisterCommand(F(":CURRent:DC?"), &GetCurrentDC);
		my_instrument.RegisterCommand(F(":POWer:DC?"), &GetPowerDC);
		my_instrument.RegisterCommand(F(":RESistance:DC?"), &GetResistanceDC);
						
		my_instrument.RegisterCommand(F(":CAPability?"), &GetCapability);
		my_instrument.RegisterCommand(F(":WATThours?"), &GetWattHours);
		my_instrument.RegisterCommand(F(":DISChargingTime?"), &GetDischargingTime);
	my_instrument.SetCommandTreeBase("");
	*/
	
	// Set up the parent command node for source-related commands
	my_instrument.SetCommandTreeBase(F("SOURce"));
		// Register commands for setting and requesting voltage, current, power, and resistance
		my_instrument.RegisterCommand(F("VOLTage:DC"), &SetCVpoint);		
		my_instrument.RegisterCommand(F("VOLTage:DC?"), &GetCVpoint);

		my_instrument.RegisterCommand(F("CURRent:DC"), &SetCCpoint);
		my_instrument.RegisterCommand(F("CURRent:DC?"), &GetCCpoint);

		my_instrument.RegisterCommand(F("POWer:DC"), &SetCPpoint);
		my_instrument.RegisterCommand(F("POWer:DC?"), &GetCPpoint);

		my_instrument.RegisterCommand(F("RESistance:DC"), &SetCRpoint);
		my_instrument.RegisterCommand(F("RESistance:DC?"), &GetCRpoint);

		 //Register command for starting/stopping the load
		my_instrument.RegisterCommand(F("INPut:STATe"), &SetLoadState);
		my_instrument.RegisterCommand(F("INPut:STATe?"), &GetLoadState);

		 //Register command for setting the working mode
		my_instrument.RegisterCommand(F("LIST:MODE"), &SetWorkingMode);
        my_instrument.RegisterCommand(F("LIST:MODE?"), &GetWorkingMode);

		 //Register commands for setting and requesting current and voltage limits
		my_instrument.RegisterCommand(F("VOLTage:VLIMt?"), &GetVoltageLimit);
		my_instrument.RegisterCommand(F("VOLTage:VLIMt"), &SetVoltageLimit);
		
		my_instrument.RegisterCommand(F("CURRent:ILIMt?"), &GetCurrentLimit);
		my_instrument.RegisterCommand(F("CURRent:ILIMt"), &SetCurrentLimit);
	//Reset the command tree base to the root level
	my_instrument.SetCommandTreeBase("");

     //Set up the parent command node for ADC measurement commands
    my_instrument.SetCommandTreeBase(F("MEASure"));
		//Register commands for requesting voltage, current, resistance, and power measurements
		my_instrument.RegisterCommand(F("VOLTage:DC?"), &GetVoltage);
		my_instrument.RegisterCommand(F("CURRent:DC?"), &GetCurrent);
		//my_instrument.RegisterCommand(F("RESistance:DC?"), &GetResistance);//Crashes the arduino when i enable this
		//my_instrument.RegisterCommand(F("POWer:DC?"), &GetPower);//arduino not replying to anything when i open this 

		//(Communication timeout error)

	//my_instrument.RegisterCommand(F(":CAPability?"), &GetAhours);
	//my_instrument.RegisterCommand(F(":WATThours?"), &GetWattHours);

	//	//Register command for requesting discharge time measurement
	//	my_instrument.RegisterCommand(F("DISChargingTime?"), &GetDischargeTime);
	
 //    //Reset the command tree base to the root level
    my_instrument.SetCommandTreeBase("");

	


Ethernet.init(csPin); // initialize the Ethernet library
	if (Ethernet.linkStatus() == LinkON) { // check if cable is connected
		Serial.println("Ethernet cable connected");		
		if (Ethernet.begin(mac) == 0) { // start DHCP
			Serial.println("Failed to get IP address using DHCP");
			// you can also assign a static IP address if DHCP fails, see the Ethernet library examples			
		}//FailedDHCP loop (consider a hardcoded fallback Static?
		else
		{
			Serial.print("IP address: ");
			Serial.println(Ethernet.localIP());
			server.begin(); // start the server
			Serial.print("Server is at ");
			Serial.println(Ethernet.localIP()); // print the server IP address to the serial monitor			
		}//Got address from DHCP
	}
	else {
		Serial.println("Ethernet cable disconnected");
	}//Ethernet Not connected.
    
}

void loop() {
	fromSerial = true;
	my_instrument.ProcessInput(Serial, "\n");

	client = server.available();
	if (client.connected()) {
		my_instrument.ProcessInput(client, "\r\n"); //Ethercard.h was using \n termination 
	};
}


/* SCPI FUNCTIONS */

void Identify(SCPI_C commands, SCPI_P parameters, Stream& interface) {
  interface.println(F("Vrekrer,SCPI Error Handling Example,#00,v0.4.2"));
}


void GetCVpoint(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	float voltage = 10.062;
	
	interface.println(voltage,2);
}

void SetCVpoint(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	//Here we print the recieved commands.
	for (size_t i = 0; i < commands.Size(); i++) {
		Serial.print(commands[i]);
		if (i < commands.Size() - 1) {
			Serial.print(":");
		}
		else {
			Serial.print(" ");
		}
	}
	Serial.println("Recived Params:");
	//Here we print the recieved parameters.
	for (size_t i = 0; i < parameters.Size(); i++) {
		Serial.print(parameters[i]);
		if (i < parameters.Size() - 1)
			Serial.print(", ");
	}
	Serial.println("Recived CMD:");
}

void GetCCpoint(SCPI_C commands, SCPI_P parameters, Stream& interface) {

}

void SetCCpoint(SCPI_C commands, SCPI_P parameters, Stream& interface) {

}

void GetCPpoint(SCPI_C commands, SCPI_P parameters, Stream& interface) {

}

void SetCPpoint(SCPI_C commands, SCPI_P parameters, Stream& interface) {

}

void GetCRpoint(SCPI_C commands, SCPI_P parameters, Stream& interface) {

}

void SetCRpoint(SCPI_C commands, SCPI_P parameters, Stream& interface) {

}

void SetLoadState(SCPI_C commands, SCPI_P parameters, Stream& interface) {

}

void GetLoadState(SCPI_C commands, SCPI_P parameters, Stream& interface) {

}

void SetWorkingMode(SCPI_C commands, SCPI_P parameters, Stream& interface) {

}

void GetWorkingMode(SCPI_C commands, SCPI_P parameters, Stream& interface) {

}

void SetVoltageLimit(SCPI_C commands, SCPI_P parameters, Stream& interface) {

}

void GetVoltageLimit(SCPI_C commands, SCPI_P parameters, Stream& interface) {

}

void SetCurrentLimit(SCPI_C commands, SCPI_P parameters, Stream& interface) {

}

void GetCurrentLimit(SCPI_C commands, SCPI_P parameters, Stream& interface) {

}

void GetVoltage(SCPI_C commands, SCPI_P parameters, Stream& interface) {

}
void GetCurrent(SCPI_C commands, SCPI_P parameters, Stream& interface) {

}
void GetPower(SCPI_C commands, SCPI_P parameters, Stream& interface) {

}
void GetResistance(SCPI_C commands, SCPI_P parameters, Stream& interface) {

}

void GetTemperature(SCPI_C commands, SCPI_P parameters, Stream& interface) {

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
}

/* HELPER FUNCTIONS */
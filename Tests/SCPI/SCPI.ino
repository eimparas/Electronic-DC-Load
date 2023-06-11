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
	//my_instrument.SetCommandTreeBase("");
    my_instrument.SetErrorHandler(&myErrorHandler);
    
	// Set up the parent command node for setPoint commands
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
	//Set up the parent command node for ADC measurement commands
    my_instrument.SetCommandTreeBase(F("MEASure"));
		//Register commands for requesting voltage, current, resistance, and power measurements
		my_instrument.RegisterCommand(F("VOLTage:DC?"), &GetVoltage);//Crashes the arduino when i enable this
		my_instrument.RegisterCommand(F("CURRent:DC?"), &GetCurrent);//arduino not replying to anything when i open this 
		my_instrument.RegisterCommand(F("RESistance:DC?"), &GetResistance);//Did we reach the limit of the Library in terms of command set? 
		my_instrument.RegisterCommand(F("POWer:DC?"), &GetPower);//im either getting (Communication timeout error) or a totaly crashed avr			
		my_instrument.RegisterCommand(F("CAPability?"), &GetAhours);
		my_instrument.RegisterCommand(F("WATThours?"), &GetWattHours);
		//Register command for requesting discharge time measurement
		my_instrument.RegisterCommand(F("DISChargingTime?"), &GetDischargeTime); 
		//Reset the command tree base to the root level
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
	if (fromSerial = true) {
		while (Serial.available())
		{
			my_instrument.ProcessInput(Serial, "\n");
		}
	}

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
	float f = 1.02;
	interface.println(f, 3);
}

void SetCCpoint(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	float cc;
	//// Here we print the recieved commands.
	//	for (size_t i = 0; i < commands.Size(); i++) {
	//		Serial.print(commands[i]);
	//		if (i < commands.Size() - 1) {
	//			Serial.print(":");
	//		}
	//		else {
	//			Serial.print(" ");
	//		}
	//	}
	Serial.println("Recived Params:");
	//Here we print the recieved parameters.
	for (size_t i = 0; i < parameters.Size(); i++) {
		cc=atof( parameters[i]);
		if (i < parameters.Size() - 1)
			return;
	}//why the fak im doing Float , strings and ptr in 2AM again ? 
	Serial.print(cc, 3);
}

void GetCPpoint(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	float f = 6.02;
	interface.println(f, 3);
}

void SetCPpoint(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	float cp;
	
	Serial.println("Recived Params:");
	//Here we print the recieved parameters.
	for (size_t i = 0; i < parameters.Size(); i++) {
		cp = atof(parameters[i]);
		if (i < parameters.Size() - 1)
			return;
	}
	Serial.print(cp, 3);
}

void GetCRpoint(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	float f = 5.02;
	interface.println(f, 3);
}

void SetCRpoint(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	float cp;

	Serial.println("Recived Params:");
	//Here we print the recieved parameters.
	for (size_t i = 0; i < parameters.Size(); i++) {
		cp = atof(parameters[i]);
		if (i < parameters.Size() - 1)
			return;
	}
	Serial.print(cp, 3);
}

void SetLoadState(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	String LoadState = String(parameters.First());
	

	Serial.print("Recived Params:");
	//Here we print the recieved parameters.
	//for (size_t i = 0; i < strlen(parameters[0]); i++) {
		if (LoadState == "1") {
			Serial.println("1");
			//LoadState = true;
		}
		if (LoadState == "0") {
			Serial.println("0");
			//LoadState = false;
		}
		//Serial.println(parameters[i]);
		//if (i < parameters.Size() - 1)
			//return;
	//}
	//Serial.println(LoadState);
}

void GetLoadState(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	float f = 7.02;
	interface.println(f, 3);
}

void SetWorkingMode(SCPI_C commands, SCPI_P parameters, Stream& interface) {

}

void GetWorkingMode(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	
	interface.println(1);
}

void SetVoltageLimit(SCPI_C commands, SCPI_P parameters, Stream& interface) {

}

void GetVoltageLimit(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	float f = 46.09;
	interface.println(f, 3);
}

void SetCurrentLimit(SCPI_C commands, SCPI_P parameters, Stream& interface) {

}

void GetCurrentLimit(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	float f = 45;
	interface.println(f, 3);
}

void GetVoltage(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	float f = 42.902;
	interface.println(f, 3);
}
void GetCurrent(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	float f = 8.02;
	interface.println(f, 3);
}
void GetPower(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	float f = 9.02;
	interface.println(f, 3);
}
void GetResistance(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	float f = 100.1;
	interface.println(f, 3);
}

void GetTemperature(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	float f = 32;
	interface.println(f, 1);
}

void GetAhours(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	float f =4900;
	interface.println(f, 3);
}
void GetWattHours(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	float f = 5100;
	interface.println(f, 3);
}

void GetDischargeTime(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	float f = 6969;
	interface.println(f, 1);
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

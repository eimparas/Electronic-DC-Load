void SCPIsetup() {

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
	//Reset the command tree base to the root level
	my_instrument.SetCommandTreeBase("");
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


void GetCVpoint(SCPI_C commands, SCPI_P parameters, Stream& interface) {	
	interface.println(setPoints.Voltage, 2);
}

void SetCVpoint(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	String param = String(parameters.First());
	float point = param.toFloat();
	Mode = CV;
	setPoints.Voltage = point;
}

void GetCCpoint(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	interface.println(setPoints.Current, 2);
}

void SetCCpoint(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	String param = String(parameters.First());
	float point = param.toFloat();
	Mode = CC;
	setPoints.Current = point;
}

void GetCPpoint(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	interface.println(setPoints.Power, 2);
}

void SetCPpoint(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	String param = String(parameters.First());
	float point = param.toFloat();
	Mode = CP;
	setPoints.Power = point;
}

void GetCRpoint(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	interface.println(setPoints.Resistance, 2);
}

void SetCRpoint(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	String param = String(parameters.First());
	float point = param.toFloat();
	Mode = CR;
	setPoints.Resistance = point;
}

void SetLoadState(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	String LoadState = String(parameters.First());
	if ((LoadState == "1") || (LoadState == "RUN")) {
		isRuning = true;
	}
	if ((LoadState == "0")||(LoadState == "STOP")) {
		isRuning = false;
	}
}

void GetLoadState(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	interface.println(isRuning);
}

void SetWorkingMode(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	String param = String(parameters.First());
	int point = param.toInt();
	if ((point >= 0) && (point < 4)) {
		Mode = point;
		if (batteryMode || settingsButtonState) {
			clearScreen = true;
			settingsButtonState = false;
			batteryMode = false;
		}
	}
	if (point == 4) {
		Mode = CC;
		clearScreen = true;
		settingsButtonState = false;
		batteryMode = true;
	}
}

void GetWorkingMode(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	if (batteryMode) {
		interface.println(4);
	}
	else interface.println(Mode);
}

void SetVoltageLimit(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	String param = String(parameters.First());
	float point = param.toFloat();
	battPoints.Vstop = point;
}

void GetVoltageLimit(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	
	interface.println(battPoints.Vstop,2);
}

void SetCurrentLimit(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	String param = String(parameters.First());
	float point = param.toFloat();
	battPoints.Current = point;
}

void GetCurrentLimit(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	interface.println(battPoints.Current, 2);
}

void GetVoltage(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	
	interface.println(Readings.Voltage, 2);
}
void GetCurrent(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	interface.println(Readings.Current, 3);
}
void GetPower(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	interface.println(Readings.Power, 3);
}
void GetResistance(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	interface.println(Readings.Resistance, 3);
}

void GetTemperature(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	String param = String(parameters.First());
	if (param == "MOS") {
		interface.println(readTemp(Temp2),1);
		
	}
	if (param == "RES") {
		interface.println((readTemp(Temp0) + readTemp(Temp1)) / 2, 1);
	}
}

void GetAhours(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	interface.println(Readings.AmpHrs, 3);
}
void GetWattHours(SCPI_C commands, SCPI_P parameters, Stream& interface) {
	interface.println(Readings.WattHrs, 3);
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

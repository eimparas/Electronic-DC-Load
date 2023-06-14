
//============================================================
// Support Functions
//============================================================


float readVoltage() {
	float ADC = 0.0;
	ADC = (!extSense) ? ADS.readADC(0) : ADS.readADC(1);
	float VoltRaw = ADC * ADCVoltageFactor;
	if (VoltRaw < 0) VoltRaw = 0;

	if (!extSense) {
		return VoltRaw * (R7 + R8 + R9) / R9;
	}
	else {
		return VoltRaw * (R10 + R11 + R12) / R12;
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
	ACS1_A = (ACS1_offset - acs1Raw) / 0.066;
	ACS2_A = (ACS2_offset - acs2Raw) / 0.066;
	Current = ACS1_A + ACS2_A;
	if (Current < 0.0001) Current = 0.0001;
	return Current; //Crude , i will come back to this , its 2:20AM give me A brake
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



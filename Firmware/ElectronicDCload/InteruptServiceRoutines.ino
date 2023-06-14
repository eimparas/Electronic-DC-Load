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
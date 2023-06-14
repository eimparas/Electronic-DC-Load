
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
	if (Mode == CC) {
		clearAndPrintFloat(battPoints.Current, 2);
	}
	else {
		lcd.print(" ---- ");
	}
	lcd.setCursor(8, 2);
	if (Mode == CP) {
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
		break;
	case CV:
		drawLCD();
		lcd.setCursor(14, 0);
		lcd.write(1);
		break;
	case CP:
		drawLCD();
		lcd.setCursor(14, 2);
		lcd.write(2);
		break;
	case CR:
		drawLCD();
		lcd.setCursor(14, 3);
		lcd.write(3);
		break;
	default:
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

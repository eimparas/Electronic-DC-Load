#include

/* This sketch describes how to connect a ACS715 Current Sense Carrier
(http://www.pololu.com/catalog/product/1186) to the Arduino,
and read current flowing through the sensor.

*/

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

/*

Vcc on carrier board to Arduino +5v
GND on carrier board to Arduino GND
OUT on carrier board to Arduino A0



Insert the power lugs into the loads positive lead circuit,
arrow on carrier board points to load, other lug connects to
power supply positive

Voltage Divider

9k Ohm from + to A4
3k Ohm from A4 to Gnd


*/



int Vin = 20;

int Vout = 5;

int ratio = Vin / Vout; // Calculated from Vin / Vout

int batMonPin = A4; // input pin for the voltage divider
int ADCVal = 0; // variable for the A/D value
float pinVoltage = 0; // variable to hold the calculated voltage
float batteryVoltage = 0;

int analogInPin = A0; // Analog input pin that the carrier board OUT is connected to
int sensorValue = 0; // value read from the carrier board
int outputValue = 0; // output in milliamps
unsigned long msec = 0;
float time = 0.0;
int sample = 0;
float totalCharge = 0.0;
float averageAmps = 0.0;
float ampSeconds = 0.0;
float ampHours = 0.0;
float wattHours = 0.0;
float amps = 0.0;

int R1 = 9000; // Resistance of R1 in ohms
int R2 = 3000; // Resistance of R2 in ohms

int ratio = 0; // Calculated from Vin / Vout

void setup() {
// initialize serial communications at 9600 bps:
Serial.begin(9600);
lcd.begin(20, 4);
}

void loop() {

int sampleADCVal = 0;
int avgADCVal = 0;
int sampleAmpVal = 0;
int avgSAV = 0;

for (int x = 0; x < 10; x++){ // run through loop 10x

// read the analog in value:
sensorValue = analogRead(analogInPin);
sampleAmpVal = sampleAmpVal + sensorValue; // add samples together

ADCVal = analogRead(batMonPin); // read the voltage on the divider
sampleADCVal = sampleADCVal + ADCVal; // add samples together

delay (10); // let ADC settle before next sample

}

avgSAV = sampleAmpVal / 10;

// convert to milli amps
outputValue = (((long)avgSAV * 5000 / 1024) - 500 ) * 1000 / 133;

/* sensor outputs about 100 at rest.
Analog read produces a value of 0-1023, equating to 0v to 5v.
"((long)sensorValue * 5000 / 1024)" is the voltage on the sensor's output in millivolts.
There's a 500mv offset to subtract.
The unit produces 133mv per amp of current, so
divide by 0.133 to convert mv to ma

*/


avgADCVal = sampleADCVal / 10; //divide by 10 (number of samples) to get a steady reading

pinVoltage = avgBVal * .00488; // Calculate the voltage on the A/D pin
/* A reading of 1 for the A/D = 0.0048mV
if we multiply the A/D reading by 0.00488 then
we get the voltage on the pin.



Also, depending on wiring and
where voltage is being read, under
heavy loads voltage displayed can be
well under voltage at supply. monitor
at load or supply and decide.
*/


batteryVoltage = pinVoltage * ratio; // Use the ratio calculated for the voltage divider
// to calculate the battery voltage


amps = (float) outputValue / 1000;
float watts = amps * batteryVoltage;

Serial.print("Volts = " );
Serial.print(batteryVoltage);
Serial.print("\t Current (amps) = ");
Serial.print(amps);
Serial.print("\t Power (Watts) = ");
Serial.print(watts);


sample = sample + 1;

msec = millis();



time = (float) msec / 1000.0;

totalCharge = totalCharge + amps;

averageAmps = totalCharge / sample;

ampSeconds = averageAmps*time;

ampHours = ampSeconds/3600;

wattHours = batteryVoltage * ampHours;





Serial.print("\t Time (hours) = ");
Serial.print(time/3600);

Serial.print("\t Amp Hours (ah) = ");
Serial.print(ampHours);
Serial.print("\t Watt Hours (wh) = ");
Serial.println(wattHours);


lcd.setCursor(0,0);
lcd.print(batteryVoltage);
lcd.print(" V ");
lcd.print(amps);
lcd.print(" A ");

lcd.setCursor(0,1);
lcd.print(watts);
lcd.print(" W ");
lcd.print(time/3600);
lcd.print(" H ");

lcd.setCursor(0,2);
lcd.print(ampHours);
lcd.print(" Ah ");
lcd.print(wattHours);
lcd.print(" Wh ");

lcd.setCursor(0,3);
lcd.print(ratio, 5);
lcd.print(" ");
lcd.print(avgBVal);

// wait 10 milliseconds before the next loop
// for the analog-to-digital converter to settle
// after the last reading:
delay(10);
}

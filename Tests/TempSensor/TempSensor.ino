
//LM35 HeatSink temperature Sensor
#define Temp0 A0
#define Temp1 A6
#define Temp2 A7

float val = 0.0;


void setup() {
	Serial.begin(9600);


}

void loop() {
	val = analogRead(Temp1);
	float mv = (val / 1024.0) * 5000;
	float cel = mv / 10;
	float farh = (cel * 9) / 5 + 32;
	Serial.print("TEMPRATURE = ");
	Serial.print(cel);
	Serial.print("*C");
	Serial.println();
	delay(1000);


}


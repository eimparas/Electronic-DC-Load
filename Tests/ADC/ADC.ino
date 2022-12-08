#include <ADS1X15.h>
//https://github.com/RobTillaart/ADS1X15



ADS1115 ADS(0x48);

float f = 0;

float _A0 = 0.0;
float _A1 = 0.0;
float _A2 = 0.0;
float _A3 = 0.0;



void setup() {
	Serial.begin(115200);
	ADS.begin();
	//Serial.println(__FILE__);
	//Serial.print("ADS1X15_LIB_VERSION: ");
	//Serial.println(ADS1X15_LIB_VERSION);
	if (!ADS.isConnected()) {
		Serial.println("ADS1115 not presant on bus , at expected Addr 0x48");
	}
	Serial.println("ADS connected on bus, expected Addr 0x48");

	f = ADS.toVoltage();
	Serial.print("the conversion factor is : ");
	Serial.print(f,3);
}

void loop() {
	for (int i = 0; i <= 4; i++) {
		 _A0 = ADS.readADC(0);
		 _A1 = ADS.readADC(1);
		 _A2 = ADS.readADC(2);
		 _A3 = ADS.readADC(3);
	}

	//Serial.println(_A0,0);
	//Serial.println(_A1,1);
	Serial.println(_A2*f,4);
	//Serial.println(_A3*myFloat,3);
	
}
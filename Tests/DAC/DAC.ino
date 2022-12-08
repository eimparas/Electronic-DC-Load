
#include <MCP4725.h>
//https://github.com/RobTillaart/MCP4725

MCP4725 MCP(0x62);

void setup() {
	
	Serial.begin(9600);
	if (MCP.begin()) {
		Serial.println("MCP4725 successfully initialized ");
	}
	else {
		Serial.println("There was a issue initializing the DAC");
	}
	//MCP.setPercentage(100.0);
	//Serial.println(MCP.readDAC());
}

void loop() {
	/*MCP.writeDAC(0, false);
	MCP.writeDAC(4096, false);*/
	MCP.setPercentage(100.0);
	//Serial.println(MCP.readDAC());
	MCP.setPercentage(0);
	//Serial.println(MCP.readDAC());
}
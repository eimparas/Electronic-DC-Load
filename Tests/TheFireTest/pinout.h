#ifndef PINOUT_H
#define PINOUT_H
#include <Arduino.h>
#endif

#define LCDaddr 0x27
#define ADCaddr 0x48
#define DAC1addr 0x62
#define DAC2addr 0x63



byte R[] = {
  B11111,
  B10011,
  B10101,
  B10011,
  B10101,
  B10101,
  B10101,
  B11111
};

byte V[] = {
  B11111,
  B11111,
  B10101,
  B10101,
  B10101,
  B11011,
  B11111,
  B11111
};

byte I[] = {
  B11111,
  B10001,
  B11011,
  B11011,
  B11011,
  B11011,
  B10001,
  B11111
};
byte C[] = {
  B11111,
  B11011,
  B10101,
  B10111,
  B10111,
  B10101,
  B11011,
  B11111
};
byte V2[] = {
  B11111,
  B10101,
  B10101,
  B10101,
  B10101,
  B10101,
  B11011,
  B11111
};//Noice


///
//Pins definitions , Use same names on schematics.
///

//encoder
#define EncA 2
#define EncB 3
#define EncBTN A3

//StartStopButtons
#define StopBtn 4
#define StartBtn 6

//SCPI ethernet SPI cs
#define ETH_CS 5



//modeButtons
#define CVbtn 10
#define CCbtn 9
#define CPbtn 8
#define CRbtn 7
#define BATbtn A2

//SettingsSW
#define settingsSW A1

//LM35 HeatSink temperature Sensor
#define Temp0 A0
#define Temp1 A6
#define Temp2 A7//Mosfet

///
// Component Constants , Use same Names on schematics
///


//Voltage Divider Restistor Values
#define R11 68000
#define R12 10000
#define R2 10000
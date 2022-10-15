#ifndef PINOUT_H
#define PINOUT_H
#include <Arduino.h>
#endif

#define LCDaddr 0x27

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
#define EncBTN 4
//FanControl
#define Fan_PWM 5
//modeButtons
#define CVbtn 6
#define CCbtn 7
#define CPbtn 8
#define CRbtn 9
//StartStopButtons
#define StartBtn 10
#define StopBtn 11
//Mux for shunts
#define Mux_A0 A0
#define Mux_A1 A1
#define Mux_A2 A2

//LM35 HeatSink temperature Sensor
#define Temp A6

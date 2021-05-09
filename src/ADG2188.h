#ifndef __ADG2188_H__
#define __ADG2188_H__

#include <Arduino.h>
#include <Wire.h>

#define ADG2188_I2CADDR_DEFAULT 0x70


class ADG2188 {

public:
  ADG2188();
  void begin(uint8_t i2c_addr = ADG2188_I2CADDR_DEFAULT, TwoWire *wire = &Wire);
  void set(bool state, uint8_t x, uint8_t y, bool ldsw = true);
  // void reset();
  void updateState();
  void printState();


private:
  TwoWire *_wire;
  uint8_t _i2caddr;
  uint8_t _state[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  uint8_t _xTable[8] = {0, 1, 2, 3, 4, 5, 8, 9};
  uint8_t _readback_addr[8] = {0x34, 0x3C, 0x74, 0x7C, 0x35, 0x3D, 0x75, 0x7D};
};


#endif

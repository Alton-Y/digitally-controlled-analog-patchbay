#include "ADG2188.h"

// https://www.analog.com/media/en/technical-documentation/data-sheets/ADG2188.pdf

ADG2188::ADG2188()
{
}

void ADG2188::begin(uint8_t i2c_addr, TwoWire *theWire)
{
  _i2caddr = i2c_addr;
  _wire = &Wire;
  _wire->begin();
  _wire->setClock(3400000);
}

void ADG2188::set(bool state, uint8_t x, uint8_t y, bool ldsw)
{
  if (x > 7 || y > 7)
  {
    Serial.print("Invalid Inputs: X ");
    Serial.print(x);
    Serial.print(" Y ");
    Serial.println(y);
    return;
  }

  uint8_t wData[2];

  state ? wData[0] = 0x01 : wData[0] = 0x00; // ON/OFF state
  wData[0] = wData[0] << 7 | _xTable[x] << 3 | y;

  ldsw ? wData[1] = 0x01 : wData[1] = 0x00; // LDSW

  _wire->beginTransmission(_i2caddr);
  _wire->write(wData[0]);
  _wire->write(wData[1]);
  _wire->endTransmission();
}

void ADG2188::updateState()
{
  for (uint8_t i = 0; i < 8; i++)
  {
    _wire->beginTransmission(_i2caddr);
    _wire->write(_readback_addr[i]);
    _wire->write(0x00);
    _wire->endTransmission();

    _wire->requestFrom(_i2caddr, (uint8_t)2);
    if (_wire->available())
    {
      _wire->read();
      _state[i] = _wire->read();
    }
  }
}

bool ADG2188::getState(uint8_t x, uint8_t y, bool update)
{
  if (x > 7 || y > 7)
  {
    return false;
  }

  if (update)
  {
    updateState();
  }

  return (_state[x] >> y) & 1;
}



void ADG2188::printState()
{
  updateState();

  Serial.println("---------------------------");
  Serial.println("   Y0 Y1 Y2 Y3 Y4 Y5 Y6 Y7");

  for (uint8_t i = 0; i < 8; i++)
  {
    uint8_t wRow = _state[i];

    Serial.print("X");
    Serial.print(i);
    Serial.print("  ");

    for (uint8_t j = 0; j < 8; j++)
    {
      Serial.print(wRow & 1);
      wRow = wRow >> 1;
      Serial.print("  ");
    }
    Serial.println();
  }

  Serial.println("---------------------------");
  Serial.println();
}

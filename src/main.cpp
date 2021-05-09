#include <Arduino.h>
#include <ADG2188.h>

ADG2188 adg2188;
bool state = true;

void setup()
{
  Serial.begin(115200);

  // SETUP - ADG2188
  adg2188.begin();
}

void loop()
{
  
  for (uint8_t x = 0; x < 8; x++)
  {
    for (uint8_t y = 0; y < 8; y++)
    {
      adg2188.set(state, x, y, true);
      adg2188.printState();
      delay(300);
    }
  }

  state = !state;
}

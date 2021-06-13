#include <Arduino.h>
#include <ADG2188.h>

ADG2188 adg2188;
bool state = true;

int x = 0;
int y = 0;

void setup()
{
  Serial.begin(115200);

  // SETUP - ADG2188
  adg2188.begin();
}

void loop()
{
  if (Serial.available() > 0) {
    int inByte = Serial.read();
    switch (inByte) {
      case 'x':
        x = Serial.parseInt();
        break;
      case 'y':
        y = Serial.parseInt();
        break;
      case 't':
        state = true;
        break;
      case 'f':
        state = false;
      case '\n':
        adg2188.set(state, x, y, true);
        Serial.print("X");
        Serial.print(x);
        Serial.print(" Y");
        Serial.print(y);
        Serial.print(" ");
        Serial.println(state);
        adg2188.printState();
        break;
    }
  }

  // for (uint8_t x = 0; x < 8; x++)
  // {
  //   for (uint8_t y = 0; y < 8; y++)
  //   {
  //     adg2188.set(state, x, y, true);
  //     adg2188.printState();
  //     delay(300);
  //   }
  // }



  // state = !state;
}

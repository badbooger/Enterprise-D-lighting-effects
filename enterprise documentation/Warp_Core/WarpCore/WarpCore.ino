#include <SoftPWM.h>
#include <SoftPWM_timer.h>

#define DELAY 100
//uint8_t leds[] = {7, 8, 9, 10, 11, 2, 3, 4, 5, 6};
uint8_t leds[] = {A4, A3, A2, A1, A0, 2, 3, 4, 5, 6};
//uint8_t leds[] = {A0, A1, A2, A3, A4, 6, 5, 4, 3, 2};

uint8_t nleds = sizeof(leds) / sizeof(uint8_t);

int GetDelay()
{
  return map(analogRead(A7), 0, 1023, 200, 20);
}

void setup()
{
  SoftPWMBegin();

  for (int i = 0; i < nleds; i++)
    SoftPWMSet(leds[i], 0);

  SoftPWMSetFadeTime(ALL, 100, 200);

  SoftPWMSetFadeTime(11, 100, 250);
  SoftPWMSetFadeTime(2, 150, 200);

  SoftPWMSet(A5, 50);
  SoftPWMSetFadeTime(A5, 100, (100 * (nleds + 1)));
}

void loop()
{
  int i;

  SoftPWMSet(leds[0], 255);
  delay(GetDelay());
  for (i = 0; i < nleds - 1; i++)
  {
    SoftPWMSet(leds[i+1], 255);
    SoftPWMSet(leds[i], 0);
    if (i < nleds - 1)
      delay(GetDelay());
  }
  SoftPWMSet(leds[nleds - 1], 0);
  SoftPWMSet(A5, 255);

  delay(GetDelay() * 2);
  SoftPWMSet(A5, 50);
  delay(GetDelay() * 2);
}

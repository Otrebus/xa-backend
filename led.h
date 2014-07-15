#ifndef LED_H_
#define LED_H_

#include "TinyTimber.h"

#define initLed() { initObject() }

typedef struct
{
    Object super;
} Led;

int blink(Led* self, int dummy);
void setupLed();

extern Led led;

#endif
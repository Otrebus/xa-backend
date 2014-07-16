#ifndef LED_H_
#define LED_H_

#include "TinyTimber.h"
#include "vm.h"
#include <stdbool.h>

#define initLed() { initObject() }

typedef struct
{
    Object super;
} Led;

int blink(Led* self, int dummy);
void setupLed();
int turnOn(Led* self, int dummy);
int turnOff(Led* self, int dummy);
int toggle(Led* self, int dummy);
bool isOn(Led* self, int dummy);

void vmToggleLed(VmThread* thread);
void vmSetLed(VmThread* thread);

extern Led led;

#endif
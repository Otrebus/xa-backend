#include "led.h"
#include <stdbool.h>
#include <avr/io.h>

Led led = initLed();

void setupLed()
{
    DDRB |= (1 << 7);
}    

int turnOn(Led* self, int dummy)
{
    PORTB |= (1 << 7);
    return 1;
}

int turnOff(Led* self, int dummy)
{
    PORTB &= ~(1 << 7);
    return 1;
}

int toggle(Led* self, int dummy)
{
    PORTB ^= (1 << 7);
    return 1;
}

bool isOn(Led* self, int dummy)
{
    return (PORTB | (1 << 7)) != 0;
}

int blink(Led* self, int dummy)
{
    toggle(self, dummy);
    AFTER(MSEC(250), self, blink, 0);
    return 0;
}    
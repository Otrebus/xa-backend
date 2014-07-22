#ifndef LED_H
#define LED_H

#include "led.h"
#include "vm.h"
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

void vmToggleLed(VmThread* thread)
{
    // No arguments to this function, so we don't need to care about the thread sent
    SYNC(&led, toggle, 0);
    thread->sp = thread->fp + 4;
}

void vmSetLed(VmThread* thread)
{
    if(getChar(thread->fp + 4) == 0)
        SYNC(&led, turnOn, 0);
    else
        SYNC(&led, turnOff, 0);
    thread->sp = thread->fp + 5;
}

#endif
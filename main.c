#include "uart.h"
#include "led.h"
#include "vm.h"
#include "TinyTimber.h"
#include <avr/wdt.h>

void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));

void wdt_init(void)
{
    MCUSR = 0;
    wdt_disable();

    return;
}

Led led;

int main(void)
{        
    setupLed();
    setupUart();
    vmInit();
    
    install((Object*) &uart, (Method) uartReceiveInterrupt, IRQ_USART0_RX);
    install((Object*) &uart, (Method) uartSentInterrupt, IRQ_USART0_TX);
    
	TINYTIMBER(NULL, NULL, 1);
}
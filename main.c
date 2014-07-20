#include "uart.h"
#include "TinyTimber.h"
#include "led.h"

Led led;

int main(void)
{        
    setupLed();
    setupUart();
    vmInit();
    
    install((Object*) &uart, (Method) uartReceiveInterrupt, IRQ_USART0_RX);
    install((Object*) &uart, (Method) uartSentInterrupt, IRQ_USART0_TX);
    
    //TINYTIMBER(&led, blink, 0);
	TINYTIMBER(NULL, NULL, 1);
    blink(&led, 0);
}
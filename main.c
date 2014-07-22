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
    
	TINYTIMBER(NULL, NULL, 1);
}
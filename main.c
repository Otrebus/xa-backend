#include "uart.h"
#include "TinyTimber.h"

int main(void)
{        
    if(sizeof(int) == sizeof(char*) && sizeof(void*) == sizeof(int))
    setupUart();
    install((Object*) &uart, (Method) uartReceiveInterrupt, IRQ_USART0_RX);
    install((Object*) &uart, (Method) uartSentInterrupt, IRQ_USART0_TX);
	TINYTIMBER(NULL, NULL, 1);
}
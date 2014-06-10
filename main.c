#include "uart.h"
#include "TinyTimber.h"

int main(void)
{        
    setupUart();
    install((Object*) &uart, (Method) uartReceiveInt, IRQ_USART0_RX);
    install((Object*) &uart, (Method) uartSentInt, IRQ_USART0_TX);
	TINYTIMBER(NULL, NULL, 1);
}
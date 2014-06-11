#include "uart.h"
#include "TinyTimber.h"

int main(void)
{        
    setupUart();
    install((Object*) &uart, (Method) uartReceiveInterrupt, IRQ_USART0_RX);
    install((Object*) &uart, (Method) uartSentInterrupt, IRQ_USART0_TX);
	TINYTIMBER(NULL, NULL, 1);
}
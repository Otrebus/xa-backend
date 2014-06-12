#include "uart.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>

Uart uart = initUart();

int handleCompleteAppFrame(Uart* self)
{
    self->tInfo.length = self->pBuf;
    self->tInfo.buf = (self->buffer);
    transmit(self, self->tInfo);
    return 0;
}

int transmit(Uart* self, transmitInfo tInfo)
{
    for(int i = 0; i < tInfo.length; i++)
    {
        transBuf[self->pEnd] = tInfo.buf[i];
        pEnd = (pEnd + 1) % UART_TB_SIZE;
    }        

    if(!self->transmitting)
    {
        self->transmitting = true;
        UDR0 = FRAME_DELIMITER;
    }        
    return 0;
}

int handleReceivedProgByte(Uart* self, unsigned char byte)
{
    switch(self->progRecvState)
    {
        case ProgRecvIdle:
            ASSERT(0);
            break;
        case ExpectingLength:
            if(self->subState == 0)
                self->programLength = 0;
            self->programLength |= ((int)byte << (8*self->subState++));
            if(self->subState > 1)
            {
                self->subState = 0;
                self->progRecvState = ExpectingData;
            }
            break;
        case ExpectingData:
            if(byte == FRAME_DELIMITER)
            {
                self->pBuf -= 4; // TODO: actually handle checksum
                self->recvState = RecvIdle;
                    
                self->transBuf[0] = ACK_HEADER;
                self->transBuf[1] = self->pBuf & 0xFF;
                self->transBuf[2] = (int)self->pBuf >> 8;
                self->tInfo.length = 3;
                self->tInfo.buf = self->transBuf;
                transmit(self, self->tInfo);
 
                if(self->pBuf == self->programLength + 4)
                {
 
                    handleCompleteAppFrame(self); // TODO: this is of course not what we do
                    self->progRecvState = ProgRecvIdle;
                }
            }
            else
                self->buffer[self->pBuf++] = byte;
            break;
    }
    return 0;
}

int handleReceivedAppByte(Uart* self, unsigned char arg)
{
    self->buffer[self->pBuf++] = (unsigned char) arg;
    return 0;
}

int handleReceivedByte(Uart* self, int arg)
{
    unsigned char byte = (char) arg;
    if(byte == 0x7D)
    {
        self->escape = true;
        return 0;
    }
    if(self->escape)
    {
        byte = byte ^ (1 << 5);
        self->escape = false;
    }
    
    switch(self->recvState)
    {
        case RecvIdle:
            if(byte == FRAME_DELIMITER) // Delimiter, starts new frame
            self->recvState = Receiving;
            break;
        case Receiving:
            if(byte == INITSEND_HEADER) // Sender wants to reprogram
            {
                self->recvState = ProgReceiving;
                self->progRecvState = ExpectingLength;
            }
            else if(byte == MORESEND_HEADER)
            {
                self->recvState = ProgReceiving;
                self->progRecvState = ExpectingData;
            }
            else
                self->recvState = AppReceiving;
            break;
        case AppReceiving:
            if(byte == FRAME_DELIMITER)
            {
                self->recvState = RecvIdle;
                handleCompleteAppFrame(self);
            }
            else
                handleReceivedAppByte(self, byte);
            break;
        case ProgReceiving:
            handleReceivedProgByte(self, byte);
        break;
    }
    return 0;
}

int uartReceiveInterrupt(Uart* self, int arg)
{
    handleReceivedByte(self, UDR0);
    return 0;
}

int handleSentByte(Uart* self)
{
    if(self->pStart == self->pEnd)
    {
        UDR0 = 0x7E;
        self->transmitting = false;
        return 0;
    }
    UDR0 = self->transBuffer[self->pStart];
    pStart = (pStart + 1) % UART_TB_SIZE;
    return 0;
}

int uartSentInterrupt(Uart* self, int arg)
{
    handleSentByte(self);
    return 0;
}
 
void setupUart()
{
    // Set baud speed
    UBRR0H = (BAUD_PRESCALE >> 8);
    UBRR0L = BAUD_PRESCALE;
    
    // Enable transmission, reception and interrupt at reception
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0) | (1 << TXCIE0);
    
    // 8 bit frame sizes. 1 stop bit and no parity is on by default
    UCSR0C = (1 << UCSZ00) | (1 << UCSZ01);
}
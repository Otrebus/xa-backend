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
    if(self->transmitting)
        return 0;
    self->transmitting = true;
    UDR0 = 0x7E;
    return 0;
}

int handleReceivedProgByte(Uart* self, unsigned char byte)
{
    switch(self->progRecvState)
    {
        case ProgRecvIdle:
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
            if(byte == 0x7E)
            {
                if(self->pBuf == self->programLength + 4)
                {
                    handleCompleteAppFrame(self); // TODO: this is of course not what we do
                    self->recvState = RecvIdle;
                    self->progRecvState = ProgRecvIdle;
                    self->pBuf -= 4; // TODO: actually handle checksum
                }
                else
                {
                    self->recvState = RecvIdle;
                    self->pBuf -= 4; // TODO: actually handle checksum
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
            if(byte == 0x7E) // Delimiter, starts new frame
            self->recvState = Receiving;
            break;
        case Receiving:
            if(byte == 0x00) // Sender wants to reprogram
            {
                self->recvState = ProgReceiving;
                self->progRecvState = ExpectingLength;
            }
            else if(byte == 0x01)
            {
                self->recvState = ProgReceiving;
                self->progRecvState = ExpectingData;
            }
            else if(byte > 0x01)
                self->recvState = AppReceiving;
            break;
        case AppReceiving:
            if(byte == 0x7E)
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

int uartSentInterrupt(Uart* self, int arg)
{
    if(!self->transmitting)
        return 0;
    if(self->curTransByte == self->tInfo.length)
    {
        UDR0 = 0x7E;
        self->transmitting = false;
        return 0;
    }
    UDR0 = self->tInfo.buf[self->curTransByte++];
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
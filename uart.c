#include "uart.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>

Uart uart = initUart();

int handleCompleteAppFrame(Uart* self)
{
    // TOOD: do stuff
    unsigned char stuff[32];
    stuff[0] = FRAME_DELIMITER;
    stuff[1] = 0xaa;
    
    TransmitInfo tInfo;
    tInfo.length = self->pBuf + 3;
    tInfo.buf = stuff;
    for(int i = 0; i < self->pBuf; i++)
        stuff[2+i] = self->buffer[i];
    stuff[self->pBuf + 2] = FRAME_DELIMITER;
    transmit(self, tInfo);
    return 0;
}

int transmit(Uart* self, TransmitInfo tInfo)
{
    // Check if the transmission buffer can hold what they want to send
    cli();
    if(self->pStart > self->pEnd && tInfo.length >= (self->pStart - self->pEnd))
        return 0;
    if(self->pEnd >= self->pStart && tInfo.length >= UART_TB_SIZE - (self->pEnd - self->pStart))
        return 0;
        
    for(int i = 0; i < tInfo.length; i++)
    {
        self->transBuf[self->pEnd] = tInfo.buf[i];
        self->pEnd = (self->pEnd + 1) % UART_TB_SIZE;
    }

    if(!self->transmitting && tInfo.length > 0)
    {
        self->transmitting = true;
        // Gotta wait until we can write the first byte, rest is interrupt driven though
        while ((UCSR0A & (1 << UDRE0)) == 0);
        UDR0 = self->transBuf[self->pStart];
        self->pStart = (self->pStart + 1) % UART_TB_SIZE;
    }        
    sei();
    return 1;
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
            {
                self->programLength = 0;
                self->pBuf = 0;
            }                
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
                
                unsigned char sendBuf[] = { FRAME_DELIMITER, ACK_HEADER, self->pBuf & 0xFF, (int) self->pBuf >> 8, FRAME_DELIMITER };
                TransmitInfo tInfo;
                tInfo.length = 5;
                tInfo.buf = sendBuf;
                transmit(self, tInfo);
 
                if(self->pBuf == self->programLength)
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
        self->transmitting = false;
        return 0;
    }
    UDR0 = self->transBuf[self->pStart];
    self->pStart = (self->pStart + 1) % UART_TB_SIZE;
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
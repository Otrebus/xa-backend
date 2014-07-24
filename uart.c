#include "uart.h"
#include "vm.h"
#include "led.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>

Uart uart = initUart();

void sendAck(Uart* self)
{
    unsigned char sendBuf[] = { FRAME_DELIMITER, ACK_HEADER, self->confirmedReceived & 0xFF, 
                                (int) self->confirmedReceived >> 8, 0, 0, 0, 0, FRAME_DELIMITER };
    unsigned long chkSum = 0;
    chkSum = sendBuf[1] + sendBuf[2] + sendBuf[3];
    *((unsigned long*) (&sendBuf[4])) = chkSum;
    transmit(self, 1, sendBuf);
    transmitChecked(self, sizeof(sendBuf) - 2, sendBuf + 1);
    transmit(self, 1, sendBuf + sizeof(sendBuf) - 1);
}

void addToChecksum(Uart* self, unsigned char byteToAdd)
{
    self->checksum += byteToAdd;
}

int handleCompleteAppFrame(Uart* self)
{
    // TODO: this is just test code, fix into some callback call or something
    unsigned char stuff[4] = {FRAME_DELIMITER, 0xaa, 0xDE, FRAME_DELIMITER };
    transmit(self, 4, stuff);
    /*unsigned char stuff[32];
    stuff[0] = FRAME_DELIMITER;
    stuff[1] = 0xaa;
    
    for(int i = 0; i < self->programLength; i++)
        stuff[2+i] = self->progBuffer[i];
    stuff[self->programLength + 2] = FRAME_DELIMITER;
    
    transmit(self, self->programLength + 3, stuff);*/
    return 0;
}

int transmit(Uart* self, unsigned int length, unsigned char* buffer)
{
    // Check if the transmission buffer can hold what they want to send
    if((self->pStart > self->pEnd && length >= (self->pStart - self->pEnd))
       || (self->pEnd >= self->pStart && length >= UART_TB_SIZE - (self->pEnd - self->pStart)))
        return 0;

    for(int i = 0; i < length; i++)
    {
        self->transBuf[self->pEnd] = buffer[i];
        self->pEnd = (self->pEnd + 1) % UART_TB_SIZE;
    }

    if(!self->transmitting && length > 0)
    {
        self->transmitting = true;
        // Gotta wait until we can write the first byte, rest is interrupt driven though
        while ((UCSR0A & (1 << UDRE0)) == 0);
        UDR0 = self->transBuf[self->pStart];
        self->pStart = (self->pStart + 1) % UART_TB_SIZE;
    }        

    return 1;
}

int transmitChecked(Uart* self, unsigned int length, unsigned char* buffer)
{
    // Check if the transmission buffer can hold what they want to send
    if((self->pStart > self->pEnd && length >= (self->pStart - self->pEnd))
    || (self->pEnd >= self->pStart && length >= UART_TB_SIZE - (self->pEnd - self->pStart)))
    return 0;

    for(int i = 0; i < length; i++)
    {
        if(buffer[i] == FRAME_DELIMITER || buffer[i] == ESCAPE_OCTET)
        {
            self->transBuf[self->pEnd] = ESCAPE_OCTET; 
            self->pEnd = (self->pEnd + 1) % UART_TB_SIZE;
            self->transBuf[self->pEnd] = buffer[i] ^ (1 << 5);
            self->pEnd = (self->pEnd + 1) % UART_TB_SIZE;
        }
        else
        {
            self->transBuf[self->pEnd] = buffer[i];
            self->pEnd = (self->pEnd + 1) % UART_TB_SIZE;
        }
    }

    if(!self->transmitting && length > 0)
    {
        self->transmitting = true;
        // Gotta wait until we can write the first byte, rest is interrupt driven though
        while ((UCSR0A & (1 << UDRE0)) == 0);
        UDR0 = self->transBuf[self->pStart];
        self->pStart = (self->pStart + 1) % UART_TB_SIZE;
    }

    return 1;
}

void timeout(Uart* self, int dummy)
{
//   self->progRecvState = ProgRecvIdle;
//   self->recvState = RecvIdle;
//   self->subState = 0;
}

void cancelTimeout(Uart* self)
{
    ABORT(self->timeout);
}

void resetTimeout(Uart* self, Time t)
{
    ABORT(self->timeout);
    self->timeout = AFTER(t, self, timeout, 0);
}

int handleReceivedProgByte(Uart* self, unsigned char byte)
{
    switch(self->progRecvState)
    {
        case ProgRecvIdle:
            break;
        case ExpectingLength:
            addToChecksum(self, byte);
            if(self->subState == 0)
            {
                self->confirmedReceived = 0;
                self->tentativeProgramLength = 0;
                self->pBuf = 0;
            }                
            self->tentativeProgramLength |= ((int)byte << (8*self->subState++));
            if(self->subState > 1)
            {
                self->subState = 0;
                self->progRecvState = ExpectingData;
            }
            break;
        case ExpectingSeq:
            addToChecksum(self, byte);
            if(self->subState == 0)
                self->seq = 0;
            self->seq |= ((int)byte) << (8*self->subState++);
            if(self->subState > 1)
            {
                if(self->seq < self->confirmedReceived)
                    self->confirmedReceived = self->seq;
                self->subState = 0;
                self->progRecvState = ExpectingData;
            }
            break;
        case ExpectingData:
            if(byte == FRAME_DELIMITER && !self->escape)
            {
                if(self->pBuf < 4)
                    return 0;
                self->pBuf -= 4;
                self->recvState = RecvIdle;

                unsigned long receivedChecksum = *((unsigned long*) &(self->frameBuffer[self->pBuf]));
                resetTimeout(self, MSEC(20));
                for(int i = 0; i < self->pBuf; i++)
                    addToChecksum(self, self->frameBuffer[i]);
                if(receivedChecksum != self->checksum || (self->seq < self->confirmedReceived && self->seq + self->pBuf > self->confirmedReceived))
                {
                    self->recvState = RecvIdle;
                    self->checksum = 0;
                    self->pBuf = 0;
                    self->progRecvState = ProgRecvIdle;
                    return 0;
                }
                self->programLength = self->tentativeProgramLength;
                if(self->seq + self->pBuf <= self->confirmedReceived)
                {
                    sendAck(self);
                    self->pBuf = 0;
                    self->progRecvState = ProgRecvIdle;
                    self->recvState = RecvIdle;
                    self->checksum = 0;
                    return 1;
                }                    
                        
                for(int i = 0; i < self->pBuf; i++)
                    self->progBuffer[i + self->confirmedReceived] = self->frameBuffer[i];
                self->confirmedReceived = self->pBuf + self->seq;
                loadProgramSegment(self->programLength, self->seq, self->pBuf, self->frameBuffer);
                sendAck(self);
                self->pBuf = 0;
                if(self->confirmedReceived == self->programLength)
                {
                    self->progRecvState = ProgRecvIdle;
                    self->recvState = RecvIdle;
                }
                self->checksum = 0;
            }
            else
            {
                self->frameBuffer[self->pBuf++] = byte;
            }                
            break;
    }
    return 0;
}

int handleReceivedAppByte(Uart* self, unsigned char arg)
{
    self->frameBuffer[self->pBuf] = arg;
    self->pBuf = (self->pBuf + 1) % UART_TB_SIZE;
    return 0;
}

int handleReceivedByte(Uart* self, int arg)
{
    resetTimeout(self, MSEC(5));
    unsigned char byte = (unsigned char) arg;
    if(byte == 0x7D)
    {
        self->escape = true;
        return 0;
    }
    if(self->escape)
        byte = byte ^ (1 << 5);
    
    switch(self->recvState)
    {
        case RecvIdle:
            if(byte == FRAME_DELIMITER && !self->escape) // Delimiter, starts new frame
            self->recvState = Receiving;
            break;
        case Receiving:
            if(byte == INITSEND_HEADER && !self->escape) // Sender wants to reprogram
            {
                addToChecksum(self, byte);
                self->recvState = ProgReceiving;
                self->progRecvState = ExpectingLength;
                self->subState = 0;
            }
            else if(byte == MORESEND_HEADER && !self->escape)
            {
                addToChecksum(self, byte);
                self->recvState = ProgReceiving;
                self->progRecvState = ExpectingSeq;
                self->subState = 0;
            }
            else
                self->recvState = AppReceiving;
            break;
        case AppReceiving:
            if(byte == FRAME_DELIMITER && !self->escape)
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
    if(self->escape)
        self->escape = false;
    
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
// int transmitChecked(Uart* self, unsigned int length, unsigned char* buffer)

void lockedTransmit(Uart* self, int arg)
{
    VmThread* thread = (VmThread*) arg;
    if(self->transmitting)
    {
        setChar(thread->fp + 6, 0);
        thread->sp = thread->fp + 6;
        return;
    }        
    unsigned char header[] = { FRAME_DELIMITER, 0x00 };
    unsigned char footer[] = { FRAME_DELIMITER };
    char length = popChar(thread);
    unsigned char* buf = (unsigned char*) popPtr(thread);
    
    transmit(self, sizeof(header), header);
    transmitChecked(self, length, buf);
    transmit(self, sizeof(footer), footer);
    
    for(int i = 0; i < length; i++)
    {
        while ((UCSR0A & (1 << UDRE0)) == 0);
        UDR0 = buf[i];
    }
    setChar(thread->fp + 6, 0);
    thread->sp = thread->fp + 6;
}    

void vmTransmit(VmThread* thread)
{
    SYNC(&uart, lockedTransmit, (int) thread);
}

void setCallback(Uart* self, int arg)
{
    VmThread* thread = (VmThread*) arg;
    self->callbackObj = (Object*) popPtr(thread);
    self->callbackMeth = popPtr(thread);
    
}    
    
void vmSetCallback(VmThread* thread)
{
    SYNC(&uart, setCallback, (int) thread);
}
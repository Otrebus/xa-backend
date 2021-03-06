#include "uart.h"
#include "vm.h"
#include "led.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <string.h>
#include <avr/wdt.h>

#define soft_reset() do { wdt_enable(WDTO_15MS); for(;;) { } } while(0)

Uart uart = initUart();

void sendAck(Uart* self, int confirmedReceived)
{
    unsigned char sendBuf[] = { FRAME_DELIMITER, ACK_HEADER, confirmedReceived & 0xFF,
    (int) confirmedReceived >> 8, 0, 0, 0, 0, FRAME_DELIMITER };
    unsigned long chkSum = 0;
    chkSum = sendBuf[1] + sendBuf[2] + sendBuf[3];
    *((unsigned long*) (&sendBuf[4])) = chkSum;
    transmit(self, 1, sendBuf);
    transmitChecked(self, sizeof(sendBuf) - 2, sendBuf + 1);
    transmit(self, 1, sendBuf + sizeof(sendBuf) - 1);
}

void addToChecksum(unsigned long *checksum, unsigned char byteToAdd)
{
    *checksum += byteToAdd;
}

int handleCompleteAppFrame(Uart* self)
{
    cli();
    VmArgBin* argBin = popVmArgBin();
    sei();
    unsigned char argStack[] = { self->pBuf - 1};
    argBin->argSize = sizeof(argStack);
    memcpy(argBin->argStack, argStack, argBin->argSize);
    argBin->methodAddr = self->callbackMeth;
    memcpy(self->callbackBuf, self->frameBuffer + 1, self->pBuf - 1);
    ASYNC(self->callbackObj, exec, argBin);
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
   /*self->progRecvState = ProgRecvIdle;
   self->recvState = RecvIdle;
   self->subState = 0;
   self->timeout = NULL;*/
}

void cancelTimeout(Uart* self)
{
/*    if(self->timeout)
    {
        ABORT(self->timeout);
        self->timeout = NULL;
    }        */
}

void resetTimeout(Uart* self, Time t)
{
    /*if(self->timeout)
    {
        ABORT(self->timeout);
        self->timeout = NULL;
    }        
    self->timeout = AFTER(t, self, timeout, 0);*/
}

int handleProgFrame(Uart* self)
{
    if(self->pBuf < 7)
        return 0;
    if(self->frameBuffer[0] == INITSEND_HEADER)
    {
        self->programLength = *((unsigned int*) (self->frameBuffer + 1));
        int dataLength = self->pBuf - 4;
        int progChunkLength = self->pBuf - 7;
        
        unsigned long checksum = 0;
        long providedChecksum = *((unsigned long*) (self->frameBuffer + dataLength));
        
        for(int i = 0; i < dataLength; i++)
            addToChecksum(&checksum, self->frameBuffer[i]);
        if(checksum != providedChecksum)
            return 0;
        loadProgramSegment(self->programLength, self->seq, progChunkLength, self->frameBuffer + 3);
        self->seq = progChunkLength;
        sendAck(self, self->seq);
    }        
    else
    {
        unsigned int receivedSeq = *((unsigned int*) (self->frameBuffer + 1));
        int dataLength = self->pBuf - 4;
        int progChunkLength = self->pBuf - 7;
        
        unsigned long checksum = 0;
        long providedChecksum = *((unsigned long*) (self->frameBuffer + dataLength));
        
        for(int i = 0; i < dataLength; i++)
            addToChecksum(&checksum, self->frameBuffer[i]);
        if(checksum != providedChecksum || receivedSeq > self->seq)
            return 0;
        loadProgramSegment(self->programLength, self->seq, progChunkLength, self->frameBuffer + 3);
        self->seq = receivedSeq + progChunkLength;
        sendAck(self, self->seq);
    }
    return 1;       
}    

int handleReceivedByte(Uart* self, int arg)
{
    resetTimeout(self, MSEC(5));
    unsigned char byte = (unsigned char) arg;
    if(byte == ESCAPE_OCTET)
    {
        self->escape = true;
        return 0;
    }
    if(self->escape)
        byte = byte ^ (1 << 5);
        
    if(byte == FRAME_DELIMITER && !self->escape)
    {
        if(!self->receiving)
        {
            self->receiving = true;
            return 0;
        }
        
        switch(self->frameBuffer[0])
        {
        case INITSEND_HEADER:
        case MORESEND_HEADER:
            handleProgFrame(self);
            break;
        case RESET_HEADER:
            soft_reset();
            break;
        default:
            handleCompleteAppFrame(self);
            break;
        }
        self->pBuf = 0;
        self->receiving = false;
    }
    else
        self->frameBuffer[self->pBuf++] = byte;
    
    if(self->escape)
        self->escape = false;
    return 0;
}

int uartReceiveInterrupt(Uart* self, int arg)
{
    ASYNC(self, handleReceivedByte, UDR0);
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
    srand(0);
    // Set baud speed
    UBRR0H = (BAUD_PRESCALE >> 8);
    UBRR0L = BAUD_PRESCALE;
    
    // Enable transmission, reception and interrupt at reception
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0) | (1 << TXCIE0);
    
    // 8 bit frame sizes. 1 stop bit and no parity is on by default
    UCSR0C = (1 << UCSZ00) | (1 << UCSZ01);
}

int lockedTransmit(Uart* self, int arg)
{
    VmThread* thread = (VmThread*) arg;

    unsigned char header[] = { FRAME_DELIMITER, 0x00 };
    unsigned char footer[] = { FRAME_DELIMITER };
    char length = getChar(thread->fp + 4);
    unsigned char* buf = (unsigned char*) getPtr(thread->fp + 5);
    
    transmit(self, sizeof(header), header);
    transmitChecked(self, length, buf);
    transmit(self, sizeof(footer), footer);
    
    thread->sp = thread->fp + 7;
    return 0;
}    

void vmTransmit(VmThread* thread)
{
    SYNC(&uart, lockedTransmit, (int) thread);
}

int setCallback(Uart* self, int arg)
{
    VmThread* thread = (VmThread*) arg;
    self->callbackMeth = getPtr(thread->fp + 8);
    self->callbackObj = (Object*) getPtr(thread->fp + 6);
    self->callbackBuf = getPtr(thread->fp + 4);
    thread->sp = thread->fp + 10;
    return 0;
}
    
void vmSetCallback(VmThread* thread)
{
    SYNC(&uart, setCallback, (int) thread);
}
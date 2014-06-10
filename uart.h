#ifndef UART_H_
#define UART_H_

#include <stdbool.h>
#include "TinyTimber.h"

#define F_CPU 16000000
#define USART_BAUDRATE 9600
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

typedef enum { Idle, Receiving, AppReceiving, ProgReceiving } UartRecvrecvState ;
typedef enum { ExpectingLengthLsb, ExpectingLengthMsb, ExpectingData } ProgRecvrecvState;

typedef struct {
    unsigned int length;
    const unsigned char* buf;
} transmitInfo;

typedef struct {
    Object super;                 // Inherited TinyTimber superobject
    unsigned char buffer[256];    // Reception buffer
    unsigned int pBuf;            // First free character in transmission buffer

    UartRecvrecvState recvState;  // State reflecting the stage of transmission reception
    bool escape;                  // Flag indicating whether previous byte was an escape character            dlfjdllfl
    bool transmitting;
    
    unsigned int programLength;
    unsigned int totalReceived;
    
    transmitInfo tInfo;
    unsigned int curTransByte;
} Uart;

#define initUart() { initObject(), {}, 0, Idle, false, false, 0, 0, { 0, 0 }, 0 }
    
extern Uart uart;

int handleCompleteAppFrame(Uart* self);
int transmit(Uart* self, transmitInfo tInfo);
int handleReceivedProgByte(Uart* self, unsigned char arg);
int handleReceivedAppByte(Uart* self, unsigned char arg);
int handleReceivedByte(Uart* self, int arg);
int uartReceiveInt(Uart* self, int arg);
int uartSentInt(Uart* self, int arg);
void setupUart();

#endif
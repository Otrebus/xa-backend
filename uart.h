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
    Object super;                 // Inherited TinyTimber grandfather object
    unsigned char buffer[256];    // Reception buffer
    unsigned int pBuf;            // First free character in transmission buffer

    UartRecvrecvState recvState;  // State reflecting the stage of transmission reception
    bool escape;                  // Was previous byte escape character?
    bool transmitting;            // Are we currently transmitting?
    
    unsigned int programLength;   // Length of program currently being received
    unsigned int totalReceived;   // Amount of program received and confirmed so far
    
    transmitInfo tInfo;           // Info about what we are currently transmitting (size and &buf)
    unsigned int curTransByte;    // How much we've transmitted of the above
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
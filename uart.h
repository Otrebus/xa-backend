#ifndef UART_H_
#define UART_H_

#include <stdbool.h>
#include "TinyTimber.h"

#define F_CPU 16000000
#define USART_BAUDRATE 9600
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

#define FRAME_DELIMITER 0x7E
#define ESCAPE_OCTET    0x7D
#define INITSEND_HEADER 0x00
#define MORESEND_HEADER 0x01
#define ACK_HEADER      0x02
#define ECHO_HEADER     0x03

typedef enum { RecvIdle, Receiving, AppReceiving, ProgReceiving } UartRecvState ;
typedef enum { ProgRecvIdle, ExpectingLength, ExpectingData } ProgRecvState;

typedef struct {
    unsigned int length;          
    const unsigned char* buf;     
} transmitInfo;

typedef struct {
    Object super;                 // Inherited TinyTimber grandfather object
    unsigned char buffer[256];    // Reception buffer
    unsigned int pBuf;            // First free character in transmission buffer

    UartRecvState recvState;      // State reflecting the stage of transmission reception
    ProgRecvState progRecvState;  // State reflecting the stage of the program reception process
    int subState;                 // Possible sub state to above
    bool escape;                  // Was previous byte escape character?
    bool transmitting;            // Are we currently transmitting?
    
    unsigned int programLength;   // Length of program currently being received
    unsigned int totalReceived;   // Amount of program received and confirmed so far
    
    unsigned char transBuf[256];  // TODO: only for testing, remove later
    transmitInfo tInfo;           // Info about what we are currently transmitting (size and &buf)
    unsigned int curTransByte;    // How much we've transmitted of the above
} Uart;

#define initUart() { initObject(), {}, 0,  RecvIdle, ProgRecvIdle, 0, false, false,  0, 0,  {}, { 0, 0 }, 0 }
    
extern Uart uart;

int handleCompleteAppFrame(Uart* self);
int transmit(Uart* self, transmitInfo tInfo);
int handleReceivedProgByte(Uart* self, unsigned char arg);
int handleReceivedAppByte(Uart* self, unsigned char arg);
int handleReceivedByte(Uart* self, int arg);
int uartReceiveInterrupt(Uart* self, int arg);
int uartSentInterrupt(Uart* self, int arg);
void setupUart();

#endif
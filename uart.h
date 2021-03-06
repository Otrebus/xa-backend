#ifndef UART_H_
#define UART_H_

#include <stdbool.h>
#include "TinyTimber.h"
#include "vm.h"

#define F_CPU 16000000
#define USART_BAUDRATE 9600
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

#define FRAME_DELIMITER 0x7E
#define ESCAPE_OCTET    0x7D
#define INITSEND_HEADER 0x0A
#define MORESEND_HEADER 0x0B
#define ACK_HEADER      0x0C
#define RESET_HEADER    0x0D

#define UART_RB_SIZE 256
#define UART_TB_SIZE 64 // Must be <= 256

typedef enum { RecvIdle, Receiving, AppReceiving, ProgReceiving, ResetReceiving } UartRecvState;
typedef enum { ProgRecvIdle, ExpectingLength, ExpectingData, ExpectingSeq } ProgRecvState;

typedef struct {
    unsigned int length;          
    const unsigned char* buf;     
} TransmitInfo;

typedef struct {
    Object super;                             // Inherited TinyTimber grandfather object
    unsigned char frameBuffer[UART_RB_SIZE];  // Frame reception buffer
    unsigned char pBuf;                       // First free character in frame reception buffer
    unsigned int seq;                         // Sequence number of the current frame
    
    bool escape;                              // Was previous byte escape character?
    bool transmitting;                        // Are we currently transmitting?
    bool receiving;

    unsigned int programLength;               // Length of program currently being received
    
    unsigned char transBuf[UART_TB_SIZE];     // Transmission (ring) buffer
    unsigned char pStart;                     // First untransmitted char
    unsigned char pEnd;                       // First empty space in buffer
    Msg timeout;
    
    Object* callbackObj;
    void* callbackMeth;
    void* callbackBuf;
} Uart;

#define initUart() { initObject(), {}, 0, 0, false, false, false, 0, {}, 0, 0, 0, 0, 0, 0 }
                         
extern Uart uart;

int transmit(Uart* self, unsigned int length, unsigned char* buffer);
int transmitChecked(Uart* self, unsigned int length, unsigned char* buffer);
int uartReceiveInterrupt(Uart* self, int arg);
int uartSentInterrupt(Uart* self, int arg);
void setupUart();

void vmTransmit(VmThread* thread);
void vmSetCallback(VmThread* thread);

#endif
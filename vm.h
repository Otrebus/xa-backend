#ifndef VM_H_
#define VM_H_

#define VM_MAX_ARGSIZE 64
#define VM_STACKSIZE 256

#define VM_NARGBINS 8
#define VM_NTHREADS 4

#include <avr/pgmspace.h>
// save some unsigned ints

PROGMEM const unsigned char instructionLength[] = 
    { 0,
      3, // OP_PUSHFP
      3, // OP_PUSHIMM 
      3, // OP_PUSHADDR     
      3, // OP_PUSHBYTEFP   
      3, // OP_PUSHWORDFP   
      3, // OP_PUSHDWORDFP  
      3, // OP_PUSHBYTEADDR 
      3, // OP_PUSHWORDADDR 
      3, // OP_PUSHDWORDADDR
      2, // OP_PUSHBYTEIMM  
      3, // OP_PUSHWORDIMM  
      5, // OP_PUSHDWORDIMM 
      1, // OP_PUSHBYTE     
      1, // OP_PUSHWORD     
      1, // OP_PUSHDWORD    
      3, // OP_POPIMM       
      3, // OP_POPBYTEFP    
      3, // OP_POPWORDFP    
      3, // OP_POPDWORDFP   
      3, // OP_POPBYTEADDR  
      3, // OP_POPWORDADDR  
      3, // OP_POPDWORDADDR 
      3, // OP_CALL         
      3, // OP_RET          
      1, // OP_SYNC         
      1  // OP_ASYNC
    };

#define OP_PUSHFP           0x01
#define OP_PUSHIMM          0x02
#define OP_PUSHADDR         0x03
#define OP_PUSHBYTEFP       0x04
#define OP_PUSHWORDFP       0x05
#define OP_PUSHDWORDFP      0x06
#define OP_PUSHBYTEADDR     0x07
#define OP_PUSHWORDADDR     0x08
#define OP_PUSHDWORDADDR    0x09
#define OP_PUSHBYTEIMM      0x0A
#define OP_PUSHWORDIMM      0x0B
#define OP_PUSHDWORDIMM     0x0C
#define OP_PUSHBYTE         0x0D
#define OP_PUSHWORD         0x0E
#define OP_PUSHDWORD        0x0F
#define OP_POPIMM           0x10
#define OP_POPBYTEFP        0x11
#define OP_POPWORDFP        0x12
#define OP_POPDWORDFP       0x13
#define OP_POPBYTEADDR      0x14
#define OP_POPWORDADDR      0x15
#define OP_POPDWORDADDR     0x16
#define OP_CALL             0x17
#define OP_RET              0x18
#define OP_SYNC             0x19
#define OP_ASYNC            0x1A

typedef struct VmThread
{
    void* fp;
    void* sp;
    void* pc;
    void* stack;
    void* bottom;
    struct VmThread* next;
}  VmThread;

typedef struct VmArgBin
{
    int argSize;
    char argStack[VM_MAX_ARGSIZE];
    struct VmArgBin* next;
    VmThread* thread;
    void* returnAddr;
    void* methodAddr;
} VmArgBin;    

#endif
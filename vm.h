#ifndef VM_H
#define VM_H

#define VM_MAX_ARGSIZE 64
#define VM_STACKSIZE 256

#define VM_NARGBINS 8
#define VM_NTHREADS 4

#define VM_MEMORY_SIZE 4096

#include <avr/pgmspace.h>
#include "TinyTimber.h"

extern const PROGMEM unsigned char instructionLength[];

#define OP_PUSHFP 0x01
#define OP_PUSHIMM 0x02
#define OP_PUSHADDR 0x03
#define OP_PUSHBYTEFP 0x04
#define OP_PUSHWORDFP 0x05
#define OP_PUSHDWORDFP 0x06
#define OP_PUSHBYTEADDR 0x07
#define OP_PUSHWORDADDR 0x08
#define OP_PUSHDWORDADDR 0x09
#define OP_PUSHBYTEIMM 0x0A
#define OP_PUSHWORDIMM 0x0B
#define OP_PUSHDWORDIMM 0x0C
#define OP_PUSHBYTE 0x0D
#define OP_PUSHWORD 0x0E
#define OP_PUSHDWORD 0x0F
#define OP_POPIMM 0x10
#define OP_POPBYTEFP 0x11
#define OP_POPWORDFP 0x12
#define OP_POPDWORDFP 0x13
#define OP_POPBYTEADDR 0x14
#define OP_POPWORDADDR 0x15
#define OP_POPDWORDADDR 0x16
#define OP_POPBYTE 0x17
#define OP_POPWORD 0x18
#define OP_POPDWORD 0x19
#define OP_CALL 0x1A
#define OP_RET 0x1B
#define OP_SYNC 0x1C
#define OP_ASYNC 0x1D
#define OP_CALLE 0x1E
#define OP_ADDBYTE 0x1F
#define OP_ADDWORD 0x20
#define OP_ADDDWORD 0x21
#define OP_SUBBYTE 0x22
#define OP_SUBWORD 0x23
#define OP_SUBDWORD 0x24
#define OP_MULBYTE 0x25
#define OP_MULWORD 0x26
#define OP_MULDWORD 0x27
#define OP_DIVBYTE 0x28
#define OP_DIVWORD 0x29
#define OP_DIVDWORD 0x2A
#define OP_MODBYTE 0x2B
#define OP_MODWORD 0x2C
#define OP_MODDWORD 0x2D
    
#define OP_ANDBYTE 0x2E
#define OP_ANDWORD 0x2F
#define OP_ANDDWORD 0x30
    
#define OP_ORBYTE 0x31
#define OP_ORWORD 0x32
#define OP_ORDWORD 0x33
    
#define OP_XORBYTE 0x34
#define OP_XORWORD 0x35
#define OP_XORDWORD 0x36
    
#define OP_SGZBYTE 0x37
#define OP_SGZWORD 0x38
#define OP_SGZDWORD 0x39

#define OP_SGEZBYTE 0x3A
#define OP_SGEZWORD 0x3B
#define OP_SGEZDWORD 0x3C
           
#define OP_SEZBYTE 0x3D
#define OP_SEZWORD 0x3E
#define OP_SEZDWORD 0x3F
           
#define OP_SNEZBYTE 0x40
#define OP_SNEZWORD 0x41
#define OP_SNEZDWORD 0x42
    
#define OP_JMP 0x43

#define OP_JEZ 0x44
#define OP_JNZ 0x45

typedef struct VmThread
{
    char* fp;
    char* sp;
    char* pc;
    char* stack;
    char* bottom;
    struct VmThread* next;
}  VmThread;

typedef struct VmArgBin
{
    int argSize;
    char argStack[VM_MAX_ARGSIZE];
    struct VmArgBin* next;
    VmThread* thread;
    char* returnAddr;
    char* methodAddr;
} VmArgBin;

void vmInit();
char getChar(void* pos);
int getInt(void* pos);
long getLong(void* pos);
void* getPtr(void* pos);
void setChar(void* pos, char c);
void setInt(void* pos, int i);
void setLong(void* pos, long l);
void setPtr(void* pos, void* ptr);
char popChar(VmThread* t);
int popInt(VmThread* t);
long popLong(VmThread* t);
void* popPtr(VmThread* t);
void pushChar(VmThread* t, char c);
void pushInt(VmThread* t, int i);
void pushLong(VmThread* t, long l);
void pushPtr(VmThread* t, void* p);
void pushArray(VmThread* t, const void* data, int size);
void popArray(void* data, VmThread* t, int size);
VmArgBin* popVmArgBin();

void loadProgramSegment(int totalLength, int seq, int segmentLength, void* buffer);
void exec(Object* obj, int arg);

#endif
#ifndef VM_H
#define VM_H

#define VM_MAX_ARGSIZE 64
#define VM_STACKSIZE 256

#define VM_NARGBINS 8
#define VM_NTHREADS 4

#define VM_MEMORY_SIZE 4096

#include <avr/pgmspace.h>
// save some unsigned ints

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
#define OP_CALL 0x17
#define OP_RET 0x18
#define OP_SYNC 0x19
#define OP_ASYNC 0x1A
#define OP_CALLE 0x1B
#define OP_ADDBYTE 0x1C
#define OP_ADDWORD 0x1D
#define OP_ADDDWORD 0x1E
#define OP_SUBBYTE 0x1F
#define OP_SUBWORD 0x20
#define OP_SUBDWORD 0x21
#define OP_MULBYTE 0x22
#define OP_MULWORD 0x23
#define OP_MULDWORD 0x24
#define OP_DIVBYTE 0x25
#define OP_DIVWORD 0x26
#define OP_DIVDWORD 0x27
#define OP_MODBYTE 0x28
#define OP_MODWORD 0x29
#define OP_MODDWORD 0x2a
#define OP_ANDBYTE 0x2b
#define OP_ANDWORD 0x2c
#define OP_ANDDWORD 0x2d
#define OP_ORBYTE 0x2e
#define OP_ORWORD 0x2f
#define OP_ORDWORD 0x30
#define OP_XORBYTE 0x31
#define OP_XORWORD 0x32
#define OP_XORDWORD 0x33
#define OP_JGZBYTE 0x34
#define OP_JGZWORD 0x35
#define OP_JGZDWORD 0x36
#define OP_JGEZBYTE 0x37
#define OP_JGEZWORD 0x38
#define OP_JGEZDWORD 0x39
#define OP_JEZBYTE 0x3a
#define OP_JEZWORD 0x3b
#define OP_JEZDWORD 0x3c
#define OP_JNEZBYTE 0x3d
#define OP_JNEZWORD 0x3e
#define OP_JNEZDWORD 0x3f

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

void loadProgramSegment(int totalLength, int seq, int segmentLength, void* buffer);

#endif
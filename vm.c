#include "vm.h"
#include "TinyTimber.h"

#include "led.h"

#include <stdbool.h>
#include <string.h>

bool currentlyLoading = false;
int programSection;
int entryPoint;
int externSection;

const unsigned PROGMEM char instructionLength[] =
{   0,
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
    1, // OP_ASYNC
    3  // OP_CALLE
};

char mem[VM_MEMORY_SIZE];

VmArgBin vmArgBins[VM_NARGBINS];
VmThread vmThreads[VM_NTHREADS];

VmArgBin* vmArgBinStack = vmArgBins;
VmThread* vmThreadStack = vmThreads;

void exec(Object* obj, int arg);

VmThread* popVmThread()
{
    VmThread* ret = vmThreadStack;
    vmThreadStack = vmThreadStack->next;
    ret->sp = ret->stack;
    return ret;
}

void pushVmThread(VmThread* v)
{
    v->next = vmThreadStack;
    vmThreadStack = v;
}

VmArgBin* popVmArgBin()
{
    VmArgBin* ret = vmArgBinStack;
    vmArgBinStack = vmArgBinStack->next;
    return ret;
}

void pushVmArgBin(VmArgBin* v)
{
    v->next = vmArgBinStack;
    vmArgBinStack = v;
}

void vmStop()
{
}

void vmStart()
{
}

void vmInit()
{
    for(int i = 0; i < VM_NARGBINS - 1; i++)
        vmArgBinStack[i].next = &(vmArgBinStack[i+1]);
    vmArgBinStack[VM_NARGBINS-1].next = 0;
    
    for(int i = 0; i < VM_NTHREADS - 1; i++)
        vmThreadStack[i].next = &(vmThreadStack[i+1]);
    vmThreadStack[VM_NTHREADS-1].next = 0;
}

inline char getChar(void* pos)
{
    return *((char*) pos);
}

inline int getInt(void* pos)
{
    return *((int*) pos);
}

inline long getLong(void* pos)
{
    return *((long*) pos);
}

inline void* getPtr(void* pos)
{
    return *((void**) pos);
}

inline void setChar(void* pos, char c)
{
    *((char*) pos) = c;
}

inline void setInt(void* pos, int i)
{
    *((int*) pos) = i;
}

inline void setLong(void* pos, long l)
{
    *((long*) pos) = l;
}

inline void setPtr(void* pos, void* ptr)
{
    *((void**) pos) = ptr;
}    

inline char popChar(VmThread* t)
{
    char c = getChar(t->sp);
    t->sp += 1;
    return c;
}

inline int popInt(VmThread* t)
{
    int i = getInt(t->sp);
    t->sp += 2;
    return i;
}

inline long popLong(VmThread* t)
{
    long l = getLong(t->sp);
    t->sp += 4;
    return l;
}

inline void* popPtr(VmThread* t)
{
    void* p = (void*) getInt(t->sp);
    t->sp += 2;
    return p;
}

inline void pushChar(VmThread* t, char c)
{
    t->sp -= 1;
    setChar(t->sp, c);
}

inline void pushInt(VmThread* t, int i)
{
    t->sp -= 2;
    setInt(t->sp, i);
}

inline void pushLong(VmThread* t, long l)
{
    t->sp -= 4;
    setLong(t->sp, l);
}

inline void pushPtr(VmThread* t, void* p)
{
    t->sp -= 2;
    setInt(t->sp, (int) p);
}

inline void pushArray(VmThread* t, const void* data, int size)
{
    t->sp -= size;
    memcpy(t->sp, data, size);
}

inline void popArray(void* data, VmThread* t, int size)
{
    memcpy(data, t->sp, size);
    t->sp += size;
}

void* getAddrFromName(const char* name)
{
    // This is hardcoded and lame, but will use this for now
    if(strcmp(name, "toggleLed") == 0)
        return (void*) vmToggleLed;
    else if(strcmp(name, "setLed") == 0)
        return (void*) vmSetLed;
    return 0;
}    

void linkProgram()
{
    char* pos = mem + programSection;
    while(pos < mem + externSection)
    {
        unsigned char opCode = getChar(pos);
        switch(opCode)
        {
        case OP_PUSHADDR:
        case OP_PUSHBYTEADDR:
        case OP_PUSHWORDADDR:
        case OP_PUSHDWORDADDR:
        case OP_POPBYTEADDR:
        case OP_POPWORDADDR:
        case OP_POPDWORDADDR: ;
            int addr = getInt(pos + 1);
            setPtr(pos + 1, mem + addr);
            break;
        case OP_CALL:
        case OP_CALLE: ;
            addr = getInt(pos + 1);
            if(addr < externSection)
                setPtr(pos + 1, mem + addr);
            else
            {
                setChar(pos, OP_CALLE);
                setPtr(pos + 1, getAddrFromName(mem + addr));
            }
            break;
        default:
            break;
        }
        pos += pgm_read_byte(instructionLength + opCode);
    }
}

void initStacks()
{
    int totStackSize = (VM_MEMORY_SIZE - externSection);
    int stackSize = totStackSize/VM_NTHREADS;
    
    vmThreads->bottom = mem + externSection;
    vmThreads->stack = mem + externSection + stackSize + totStackSize % VM_NTHREADS;

    VmThread* lastThread = vmThreads;
    for(VmThread* thread = vmThreads->next; thread; thread = thread->next)
    {
        thread->bottom = lastThread->stack;
        thread->stack = thread->bottom + stackSize;
        lastThread = thread;
    }        
}           

void loadProgramSegment(int totalLength, int seq, int segmentLength, void* buffer)
{
    // if currentlyLoading is 0:
    // TODO: halt currently executing vm (loop through activeStack, check for thread->msg->meth == exec
    // loop through active msgs, check if meth == exec, abort those who are
    
    // clear all I/O callback functions as well
    currentlyLoading = true;
    
    for(int i = 0; i < segmentLength; i++)
        mem[seq++] = ((char*) buffer)[i];
    
    if(seq == totalLength)
    {
        // Extract the header information and shift the code backwards
        programSection = getInt(mem);
        entryPoint = getInt(mem + 2);
        externSection = getInt(mem + 4);
        for(int i = 0; i < totalLength - 6; i++)
        mem[i] = mem[i + 6];
        
        linkProgram();
        initStacks();
    }
}

bool executeInstruction(VmThread* thread, VmArgBin* argBin)
{
    void* addr;
    switch(getChar(thread->pc))
    {
    case OP_PUSHFP: // push $fp+c
        pushPtr(thread, thread->fp + getInt(thread->pc + 1));
        thread->pc += 3;
        break;
        
    case OP_PUSHIMM: // push imm (reduce $sp with immediate)
        thread->sp -= getInt(thread->pc + 1);
        thread->pc += 3;
        break;
        
    case OP_PUSHADDR: // push label
        pushInt(thread, getInt(thread->pc + 1));
        thread->pc += 3;
        break;
        
    case OP_PUSHBYTEFP: // push byte [$fp+c]
        addr = thread->fp + getInt(thread->pc + 1);
        pushChar(thread, getChar(addr));
        thread->pc += 3;
        break;
        
    case OP_PUSHWORDFP: // push word [$fp+c]
        addr = thread->fp + getInt(thread->pc + 1);
        pushInt(thread, getInt(addr));
        thread->pc += 3;
        break;
        
    case OP_PUSHDWORDFP: // push dword [$fp+c]
        addr = thread->fp + getInt(thread->pc + 1);
        pushLong(thread, getLong(addr));
        thread->pc += 3;
        break;
        
    case OP_PUSHBYTEADDR: // push byte [label]
        addr = getPtr(thread->pc + 1);
        pushChar(thread, getChar(addr));
        thread->pc += 3;
        break;
        
    case OP_PUSHWORDADDR: // push word [label]
        addr = getPtr(thread->pc + 1);
        pushInt(thread, getInt(addr));
        thread->pc += 3;
        break;

    case OP_PUSHDWORDADDR: // push dword [label]
        addr = getPtr(thread->pc + 1);
        pushLong(thread, getLong(addr));
        thread->pc += 3;
        break;

    case OP_PUSHBYTEIMM: // push byte imm
        pushChar(thread, getChar(thread->pc + 1));
        thread->pc += 2;
        break;
        
    case OP_PUSHWORDIMM: // push word imm
        pushInt(thread, getInt(thread->pc + 1));
        thread->pc += 3;
        break;

    case OP_PUSHDWORDIMM: // push dword imm
        pushLong(thread, getLong(thread->pc + 1));
        thread->pc += 5;
        break;

    case OP_PUSHBYTE: // push byte (replace [$sp] with [[$sp]])
        addr = popPtr(thread); // This can be optimized
        pushChar(thread, getChar(addr));
        thread->pc += 1;
        break;
        
    case OP_PUSHWORD: // push word (replace [$sp] with [[$sp]])
        addr = popPtr(thread);
        pushInt(thread, getInt(addr));
        thread->pc += 1;
        break;
        
    case OP_PUSHDWORD: // push word (replace [$sp] with [[$sp]])
        addr = popPtr(thread);
        pushLong(thread, getLong(addr));
        thread->pc += 1;
        break;
        
    case OP_POPIMM:
        thread->sp += getInt(thread->pc + 1);
        thread->pc += 3;
        break;
        
    case OP_POPBYTEFP: // pop byte [$fp + c]
        addr = thread->fp + getInt(thread->pc + 1);
        setChar(addr, popChar(thread));
        thread->pc += 3;
        break;
        
    case OP_POPWORDFP: // pop word [$fp + c]
        addr = thread->fp + getInt(thread->pc + 1);
        setInt(addr, popInt(thread));
        thread->pc += 3;
        break;

    case OP_POPDWORDFP:  // pop dword [$fp + c]
        addr = thread->fp + getInt(thread->pc + 1);
        setLong(addr, popLong(thread));
        thread->pc += 3;
        break;
        
    case OP_POPBYTEADDR: // pop byte [label]
        addr = getPtr(thread->pc + 1);
        setChar(addr, popChar(thread));
        thread->pc += 3;
        break;
        
    case OP_POPWORDADDR: // pop word [label]
        addr = getPtr(thread->pc + 1);
        setInt(addr, popInt(thread));
        thread->pc += 3;
        break;
        
    case OP_POPDWORDADDR: // pop dword [label]
        addr = getPtr(thread->pc + 1);
        setLong(addr, popLong(thread));
        thread->pc += 3;
        break;
        
    case OP_CALL:
        pushInt(thread, (int) (thread->pc + 3));
        pushInt(thread, (int) (thread->fp));
        thread->fp = thread->sp;
        thread->pc = getPtr(thread->pc + 1);
        break;
        
    case OP_RET: ;
        int spDec = getInt(thread->pc + 1);
        // $sp = $fp + arg + 4
        thread->sp = thread->fp + spDec + 4;
        // $pc = [$fp + 2]
        void* retAddr = getPtr(thread->fp + 2);
        thread->pc = retAddr;
        // $fp = [$fp]
        thread->fp = getPtr(thread->fp);
        // If $fp is 0, we reached the bottom of either a sync or async call
        if(thread->fp == 0)
        {
            if(retAddr == 0) // This was an async call, recycle the thread obj
            popVmThread(thread);
            return false; // Stop executing instructions on this object
        }
        
    case OP_SYNC: ;
        void* methodAddress = popPtr(thread);
        void* obj = popPtr(thread);
        // We can reuse the current thread in sync calls
        argBin->thread = thread;
        argBin->methodAddr = methodAddress;
        // Here, we replace the method address and object with the return address and frame pointer,
        // presenting whatever method we call with a stack frame identical to a normal call
        pushPtr(thread, thread->pc + 1); // size of a sync instruction is 1
        // Setting $fp to 0 is our way of letting exec know it should return control once its stack
        // has reached this position
        pushPtr(thread, 0);
        // But we still need to save and restore $fp, so we save it here ...
        void* oldFp = thread->fp;
        thread->fp = thread->sp;
        SYNC(obj, exec, argBin);
        // ... and restore it after the call
        thread->fp = oldFp;
        break;
        
    case OP_ASYNC: ;
        char argSize = popChar(thread);
        long baseline = popLong(thread);
        long deadline = popLong(thread);
        obj = popPtr(thread);
        methodAddress = popPtr(thread);
        VmArgBin* argBin = popVmArgBin();
        argBin->argSize = argSize;
        popArray(argBin->argStack, thread, argSize);
        argBin->methodAddr = methodAddress;
        argBin->returnAddr = 0;
        SEND(USEC(baseline), USEC(deadline), obj, exec, argBin);
        thread->pc++;
        break;
        
    case OP_CALLE: // call external (I/O) function
        // First push a dummy return value and old $fp onto the stack just to
        // present a stack frame consistent with the rest of the different call types
        pushInt(thread, 0);
        pushInt(thread, 0);
        void* savedFp = thread->fp;
        thread->fp = thread->sp;
        void (*fun)(VmThread*) = getPtr(thread->pc + 1);
        fun(thread);
        thread->fp = savedFp;
        thread->pc += 3;        
    }
    return true;
}

void exec(Object* obj, int arg)
{
    VmArgBin* argBin = (VmArgBin*) arg;
    VmThread* thread;
    
    // If a thread was provided, this is a sync call
    if(argBin->thread)
    {
        thread = argBin->thread;
        thread->pc = argBin->methodAddr;
    }
    else
    {
        // Fetch a new thread object and populate it with the stack contents
        thread = popVmThread();
        pushArray(thread, argBin->argStack, argBin->argSize);
        pushInt(thread, 0); // fake return address
        pushInt(thread, 0); // fake old frame pointer
        thread->fp = thread->sp;
        thread->pc = argBin->methodAddr;
        // We extracted all the information that we needed from the argBin, so we can recycle it
        pushVmArgBin(argBin);
    }
    
    while(executeInstruction(thread, argBin));
}
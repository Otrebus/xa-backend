#include "vm.h"
#include "TinyTimber.h"

#include <stdbool.h>
#include <string.h>

char mem[4096];
bool currentlyLoading = false;
int entryPoint;
int externSection;

VmArgBin vmArgBins[VM_NARGBINS];
VmThread vmThreads[VM_NTHREADS];

VmArgBin* vmArgBinStack = vmArgBins;
VmThread* vmThreadStack = vmThreads;

void exec(Object* obj, int arg);

VmThread* popVmThread()
{
    VmThread* ret = vmThreadStack;
    vmThreadStack = vmThreadStack->next;
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
    t->sp -= 4;
    setInt(t->sp, (int) p);
}    

inline void pushArray(VmThread* t, const void* data, int size)
{
    memcpy(t->sp - size, data, size);
    t->sp -= size;
}

inline void popArray(void* data, VmThread* t, int size)
{
    memcpy(data, t->sp, size);
    t->sp += size;
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
    
    if(seq == segmentLength)
    {
        // Extract the header information and shift the code backwards
        entryPoint = *((int*)mem);
        externSection = *((int*)mem + 2);
        for(int i = 0; i < totalLength - 4; i++)
            mem[i] = mem[i + 4];
    }
    
    // TODO: link stuff
    // TODO: initialize thread stacks
}

bool executeInstruction(VmThread* thread, VmArgBin* argBin)
{
    switch(getChar(thread->pc))
    {
    case OP_PUSHFP: // Push $fp + offset
        pushInt(thread, getInt(thread->pc + 1));
        thread->pc += 3;
        break;
        
    case OP_PUSHIMM: // Push immediate (reduce $sp with immediate)
        thread->sp -= getInt(thread->pc + 1);
        thread->pc += 3;
        break;
        
    case OP_PUSHADDR: // Push memory address (label)
        pushInt(thread, getInt(thread->pc + 1));
        thread->pc += 3;
        break;
    
    case OP_PUSHBYTEFP: ; // Push [$fp + offset]
        int offset = getInt(thread->pc + 1);
        int addr = getPtr(thread->fp + offset);
        char val = getChar(addr);
        pushChar(thread, val);
        thread->pc += 3;
        break;
    
    case OP_PUSHWORDFP: ; // Push [$fp + offset]
        int offset = getInt(thread->pc + 1);
        int addr = getPtr(thread->fp + offset);
        int val = getInt(addr);
        pushInt(thread, val);
        thread->pc += 3;
        break;
        
    case OP_PUSHDWORDFP: ; // Push [$fp + offset]
        int offset = getInt(thread->pc + 1);
        int addr = getPtr(thread->fp + offset);
        int val = getLong(addr);
        pushLong(thread, val);
        thread->pc += 3;
        break;
        
    case OP_PUSHBYTEADDR:
        
        
    case OP_PUSHWORDADDR:
    case OP_PUSHDWORDADDR:
    case OP_PUSHBYTEIMM:
    case OP_PUSHWORDIMM:
    case OP_PUSHDWORDIMM:
    case OP_PUSHBYTE:
    case OP_PUSHWORD:
    case OP_PUSHDWORD:
    case OP_POPIMM:
    case OP_POPBYTEFP:
    case OP_POPWORDFP:
    case OP_POPDWORDFP:
    case OP_POPBYTEADDR:
    case OP_POPWORDADDR:
    case OP_POPDWORDADDR:
    case OP_CALL:
        pushInt(thread, (int) (thread->pc + 3));
        pushInt(thread, (int) (thread->fp));
        thread->pc = getPtr(thread->pc + 1);
        
    case OP_RET: ;
        int spDec = getInt(thread->pc + 1);
        // $sp = $fp - arg
        thread->sp = thread->fp - spDec;
        // $pc = [$fp - 2]
        thread->pc = getPtr(thread->fp - 2);
        // $fp = [$fp]
        thread->fp = getPtr(thread->fp);
        // If $fp is 0, we reached the bottom of the stack - thread is done
        if(thread->fp == 0)
           return false; // Stop executing instructions on this object
        
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
        SEND(baseline, deadline, obj, exec, argBin);
        thread->pc++;
        break;
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
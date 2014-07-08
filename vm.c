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
    {
        vmThreadStack[i].next = &(vmThreadStack[i+1]);
        vmThreadStack[i].sp = VM_STACKSIZE;
    }        
    vmThreadStack[VM_NTHREADS-1].next = 0;
}

void pushChar(VmThread* t, char c)
{
    t->sp -= 1;
    t->stack[t->sp] = c;
}

void pushInt(VmThread* t, int i)
{
    t->sp -= 2;
    *((int*)(t->stack + t->sp)) = i;
}

void pushLong(VmThread* t, long l)
{
    t->sp -= 4;
    *((int*)(t->stack + t->sp)) = l;
}

void copyStackSegment(VmThread* t, int size, char* data)
{
    memcpy(t->stack + (t->sp - size), data, size);
    t->sp -= size;
}    

void loadProgramSegment(int totalLength, int seq, int segmentLength, char* buffer)
{
    // if currentlyLoading is 0:
    // TODO: halt currently executing vm (loop through activeStack, check for thread->msg->meth == exec
    // loop through active msgs, check if meth == exec, abort those who are
    
    // clear all I/O callback functions as well
    
    currentlyLoading = true;
    
    for(int i = 0; i < segmentLength; i++) 
        mem[seq++] = buffer[i];
    
    if(seq == segmentLength)
    {
        // Extract the header information and shift the code backwards
        entryPoint = *((int*)mem);
        externSection = *((int*)mem + 2);
        for(int i = 0; i < totalLength - 4; i++)
            mem[i] = mem[i + 4];
    }
}

bool executeInstruction(VmThread* thread)
{
    switch(mem[thread->pc])
    {
        case OP_PUSHFP:
            thread->sp -= 2;
            thread->stack[thread->sp] = thread->fp + *((int*)(mem + thread->pc + 1));
        break;
    }
}

void exec(Object* obj, int arg)
{
    VmArgBin* argBin = (VmArgBin*) arg;
    VmThread* thread;
    
    // If a thread was provided, this is a sync call
    if(argBin->thread)
        thread = argBin->thread;
    else
    {
        // Fetch a new thread object and populate it with the stack contents
        thread = popVmThread();
        pushInt(thread, argBin->returnAddr);
        pushInt(thread, 0);
        copyStackSegment(thread, argBin->argSize, argBin->argStack);
        thread->pc = argBin->methodAddr;
    }
    
    while(executeInstruction(thread));
}
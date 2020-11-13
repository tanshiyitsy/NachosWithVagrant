// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------
void PageFaultHandler(){
    int badVAddr = machine->ReadRegister(39); // registers[BadVAddrReg]
    // ExceptionType exception = machine->FIFOSwap(badVAddr);
    ExceptionType exception = machine->LRUSwap(badVAddr);
    if(exception != NoException)
        machine->RaiseException(exception,badVAddr);

}
void UserProgClear(){
    // TranslationEntry *pageTable;
    // unsigned int pageTableSize;
    // int pageTableSize = machine->pageTableSize;
    // TranslationEntry *pageTable = machine->pageTable;
    printf("start to clear bitmap,and clear the pageTable\n");
    for(int i = 0;i < NumPhysPages;i++){
        // 清除当前进程的页表占用信息
        if(machine->pageTable[i].valid && (machine->pageTable[i].pid == currentThread->getPid())){
            printf("clear pid:%d pageTable:%d\n", currentThread->getPid(),i);
            if((machine->pageTable[i].dirty)){
                machine->ReWritePage(machine->pageTable[i].virtualPage,i);
            }
            machine->pageTable[i].virtualPage = -1;
            machine->pageTable[i].valid = FALSE;
            machine->pageTable[i].createTime = 0;
            machine->pageTable[i].visitTime = 0;
            machine->pageTable[i].dirty = FALSE;
        }        
        
    }
    
}
void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    // printf("which=%d type=%d\n", which,type);
    if ((which == SyscallException) && (type == SC_Halt)) {
    	DEBUG('a', "Shutdown, initiated by user program.\n");
        printf("Suspended the thread pid=%d\n", currentThread->getPid());
        UserProgClear();
        currentThread->Suspended();

    } 
    // pageFadult去页表里查找
    // 页表里默认有所有滴的数据代码
    else if(which == PageFaultException){
        PageFaultHandler();
    }
    else if((which == SyscallException) && (type == SC_Exit)){
        printf("this prog is going to exit\n");
        printf("Suspended the thread pid=%d\n", currentThread->getPid());
        UserProgClear();
        currentThread->Suspended();
    }
    else if((which ==  AddressErrorException) && (type == SC_Halt)){
        printf("AddressErrorException\n");
        UserProgClear();
        interrupt->Halt();
    }
    else if((which == IllegalInstrException)){
        // IllegalInstrException, // Unimplemented or reserved instr.
        printf("here is IllegalInstrException\n");
    }
    else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}

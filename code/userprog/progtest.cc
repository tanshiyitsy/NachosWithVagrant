// progtest.cc 
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.  
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"

//----------------------------------------------------------------------
// StartProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------

void
StartProcess(char *filename)
{
    OpenFile *executable = fileSystem->Open(filename);
    AddrSpace *space;
    if (executable == NULL) {
    	printf("Unable to open file %s\n", filename);
    	return;
    }
    printf("now in StartProcess.....\n");
    space = new AddrSpace(executable,filename);    
    currentThread->space = space;

    delete executable;			// close file

    space->InitRegisters();		// set the initial register values
    space->RestoreState();		// load page table register

    machine->Run();			// jump to the user progam
    ASSERT(FALSE);			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}

void simpleTest(int which){
    machine->Run();         // jump to the user progam
}
void createSingleUserProcess(char *filename){
    // 1. 创建用户空间
    printf("start to run filename=%s\n", filename);
    OpenFile *executable = fileSystem->Open(filename);
    if (executable == NULL) {
        printf("Unable to open file %s\n", filename);
        return;
    }
    AddrSpace *space = new AddrSpace(executable);   
    delete executable;          // close file
    
    //2. 创建用户进程 
    Thread *t1 = new Thread(filename);
    t1->space = space;
    
    space->InitRegisters();     // set the initial register values
    space->RestoreState();      // load page table register

    t1->Fork(simpleTest,0);
    
}
void MultiUserProcess(){
    char *filename1 = "../test/halt.noff";
    char *filename2 = "../test/halt.noff";
    createSingleUserProcess(filename1);
    createSingleUserProcess(filename2);

    for(int i = 0;i < 2;i++){
        printf("thread name:%s pid:%d uid:%d priority:%d\n", currentThread->getName(), 
            currentThread->getPid(), currentThread->getUid(), currentThread->base_priority);
            // 每运行一次当前线程，就让出CPU，让另一个线程继续执行
            currentThread->Yield();
    }
}
// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.

static Console *console;
static Semaphore *readAvail;
static Semaphore *writeDone;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

static void ReadAvail(int arg) { readAvail->V(); }
static void WriteDone(int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------

void 
ConsoleTest (char *in, char *out)
{
    char ch;

    console = new Console(in, out, ReadAvail, WriteDone, 0);
    readAvail = new Semaphore("read avail", 0);
    writeDone = new Semaphore("write done", 0);
    
    for (;;) {
	readAvail->P();		// wait for character to arrive
	ch = console->GetChar();
	console->PutChar(ch);	// echo it!
	writeDone->P() ;        // wait for write to finish
	if (ch == 'q') return;  // if q, quit
    }
}

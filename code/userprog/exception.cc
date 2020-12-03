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
#include "filesys.h"

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
    int pageTableSize = machine->pageTableSize;
    TranslationEntry *pageTable = machine->pageTable;
    printf("start to clear bitmap,and clear the pageTable\n");
    for(int i = 0;i < pageTableSize;i++){
        // 清除位图标志
        int pn = pageTable[i].physicalPage;
        if(pn >= 0){
            printf("clear the memory of bitmap is %d\n", pageTable[i].physicalPage);
            bitmap->Clear(pageTable[i].physicalPage);
            // 清除pageTable映射
            pageTable[i].virtualPage = i;
            pageTable[i].physicalPage = -1;
            pageTable[i].valid = FALSE;
            pageTable[i].use = FALSE;
            pageTable[i].dirty = FALSE;
            pageTable[i].readOnly = FALSE;
            pageTable[i].createTime = 0;
            pageTable[i].visitTime = 0;
        }
        // else pn == -1   这种情况说明该页面长时间没被访问，页框被其他页面占用了，该页面就不在内存了，所以无需清除
    }
    
}
void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    ExceptionType temp = NoException;
    // printf("which=%d type=%d\n", which,type);
	// #define SC_Exec		2
	// #define SC_Join		3
	// #define SC_Fork		9
	// #define SC_Yield	10
    if (which == SyscallException) {
    	if(type == SC_Halt){
    		DEBUG('a', "Shutdown, initiated by user program.\n");
	        UserProgClear();
	       	interrupt->Halt();
    	}
    	else if(type == SC_Create){
            printf("------------------now in SC_Create--------------\n");
    		int addr = machine->ReadRegister(4);
    		char fileName[20];
    		int value = 0,cnt = 0;
    		do{
    			if(!machine->ReadMem(addr+cnt,1,&value)){
                    value = 7;
                    continue;
                }
    			fileName[cnt++] = value;
    		}while(value != 0);
    		if(fileSystem->Create(fileName,128)){
                printf("successfully to create file:%s\n", fileName);
                // WriteRegister(int num, int value)
                machine->WriteRegister(2,0);
            }
            else{
                printf("fail to create file:%s\n", fileName);
                machine->WriteRegister(2,-1);
            }
            machine->PCAdvanced();
    	}
    	else if(type == SC_Open){
            printf("------------------now in SC_Open--------------\n");
            int addr = machine->ReadRegister(4);
            char fileName[20];
            int value = 0,cnt = 0;
            do{
                if(!machine->ReadMem(addr+cnt,1,&value)){
                    value = 7;
                    continue;
                }
                fileName[cnt++] = value;
            }while(value != 0);
            OpenFile* openfile = fileSystem->Open(fileName);
            machine->WriteRegister(2,int(openfile));
            machine->PCAdvanced();
    	}
    	else if(type == SC_Close){
            printf("------------------now in SC_Close--------------\n");
            // 关闭打开的文件指针
            OpenFile* openfile = (OpenFile*)machine->ReadRegister(4);
            delete openfile;
            machine->PCAdvanced();
    	}
    	else if(type == SC_Write){
            printf("------------------now in SC_Write--------------\n");
            // void Write(char *buffer, int size, OpenFileId id);
            // 把buffer里的内容写入file 
            int addr = machine->ReadRegister(4);
            int size = machine->ReadRegister(5);
            OpenFile* openfile = (OpenFile*)machine->ReadRegister(6);
            char buffer[size+1];
            int data;
            int i = 0;
            while(i < size){
                if(machine->ReadMem(addr + i,1,&data)){
                    buffer[i] = data;
                    i++;
                }
            }
            buffer[size] = '\0';
            printf("the buffer is:%s\n", buffer);
            if((int)openfile == ConsoleOutput){
                printf("%s\n", buffer);
            }
            else
                openfile->Write(buffer,size);
            machine->PCAdvanced();
    	}
    	else if(type == SC_Read){
            printf("------------------now in SC_Read--------------\n");
            // int Read(char *buffer, int size, OpenFileId id);
            // 把file里的内容读入buffer
            int addr = machine->ReadRegister(4);
            int size = machine->ReadRegister(5);
            OpenFile* openfile = (OpenFile*)machine->ReadRegister(6);
            char temp[size+1]; //暂时用于存放读取的数据
            int ans = openfile->Read(temp,size);
            // 写入buffer所在内存
            int i = 0;
            while(i < size){
                if(machine->WriteMem(addr + i,1,(int)temp[i])){
                    i++;
                }
            }
            printf("temp is:%s,this is read from file\n", temp);
            printf("ans is %d\n", ans);
            machine->WriteRegister(2,ans);
            machine->PCAdvanced();
    	}
    } 
    // pageFadult去页表里查找
    // 页表里默认有所有滴的数据代码
    else if(which == PageFaultException){
        PageFaultHandler();
    }
    else if((which == SyscallException) && (type == SC_Exit)){
        printf("this prog is going to exit\n");
        UserProgClear();
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

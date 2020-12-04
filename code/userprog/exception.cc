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
#include "thread.h"

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
void exec_func(int which){

}
void fork_func(int func_addr){
    machine->WriteRegister(PCReg,func_addr);
    machine->WriteRegister(NextPCReg,func_addr+4);
    machine->Run();
}
void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    ExceptionType temp = NoException;
    printf("which=%d type=%d\n", which,type);
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
        else if(type == SC_Exec){
            // SpaceId Exec(char *name);
            // 加载并执行name的nachos的可执行文件，返回地址空间标识符
            printf("------------------now in SC_Exec--------------\n");
            int addr = machine->ReadRegister(4);
            char threadName[20];
            int i = 0,value = 0;
            do{
                if(!machine->ReadMem(addr + i),1,&value){
                    value = 9;
                    continue;
                }
                threadName[i++] = value;
            }while(value != 0);
            
            OpenFile *executable = fileSystem->Open(fileName);
            if(executable == NULL){
                printf("Unable to open file %s\n", fileName);
                return;
            }
            AddrSpace* space = new AddrSpace(executable,fileName);
            delete executable;
            currentThread->space = space;
            space->InitRegisters();
            space->RestoreState();
            // 需要重新fork吗？
            // t-Fork(exec_func,0);
            machine->Run();
            machine->WriteRegister(2,currentThread->getPid());
            machine->PCAdvanced();
        }
        else if(type == SC_Fork){
            // Address 新增属性fileName
            // 需要逐页复制页表的内容
            // 建立线程，执行fork_func
            // void Fork(void (*func)());
            // 参数为函数指针，其实是用户程序地址空间中函数的虚拟地址
            // 用户级别的线程可以运行多个进程运行在一个用户空间内
            // 运行相同的 用户程序，不同的是，fork是从执行位置开始运行的，
            // 因此必须初始化PC为给定的函数指针
            printf("------------------now in SC_Fork--------------\n");
            int func_addr = machine->ReadRegister(4); // 函数指针的位置
            // 复制父进程的地址空间
            OpenFile *executable = fileSystem->Open(currentThread->space->fileName);
            if(executable == NULL){
                printf("Unable to open file %s\n", fileName);
                return;
            }
            // 这里的地址空间是打开同一个可执行文件，还是
            // 子进程的PC指针是和父进程一样？还是为0？
            AddrSpace *child_space = new AddrSpace(executable);
            // 复制space
            child_space->copySpace(currentThread->space);
            delete executable;
            Thread *child = new Thread("child!");
            child->space = child_space;
            child->Fork(fork_func,func_addr);
            // 判断是否需要抢占
            machine->WriteRegister(2,child->getPid());
            machine->PCAdvanced();
        }
        else if(type == SC_Yield){
            printf("------------------now in SC_Yield--------------\n");
            machine->PCAdvanced();
            currentThread->Yield();
        }
        else if(type == SC_Join){
            // 等待标识符为id的用户线程运行完毕，返回其退出状态
            // 1. 获取线程id
            // 2. 检查线程池，确定特定线程是否处于活跃状态，如果活跃，那么切换
            printf("------------------now in SC_Join--------------\n");
            int thread_id = machine->ReadRegister(4);
            while(pid_pool[thread_id] == 1){
                currentThread->Yield();
            }
            machine->PCAdvanced();
        }
        else if(type == SC_Exit){
            /* This user program is done (status = 0 means exited normally). */
            // void Exit(int status);  
            // 用户程序退出，status = 0 表示正常
            // 1. 获取退出状态，输出相关信息
            // 2. 释放页表空间
            // 3. 更新PC
            // 4. 结束当前线程
            printf("------------------now in SC_Exit--------------\n");
            int status = machine->ReadRegister(4);
            printf("thread:%s exit with status:%d\n", currentThread->getName(),status);
            // 这里释放的应该只是当前进程的页表
            UserProgClear();
            machine->PCAdvanced();
            currentThread->Finish();
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

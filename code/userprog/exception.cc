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
    int pid = currentThread->getPid();
    printf("pid:%d start to clear bitmap,and clear the pageTable\n",currentThread->getPid());
    for(int i = 0;i < pageTableSize;i++){
        // 清除位图标志
        int pn = pageTable[i].physicalPage;
        int temp_pid = pageTable[i].pid;
        // printf("i=%d pa=%d va=%d pid=%d\n", i,pageTable[i].physicalPage,pageTable[i].virtualPage,temp_pid);
        if(pn >= 0 && pid == temp_pid){
            // printf("clear the memory of bitmap is %d\n", pageTable[i].physicalPage);
            bitmap->Clear(pageTable[i].physicalPage);
            // 清除pageTable映射
            pageTable[i].virtualPage = i;
            pageTable[i].physicalPage = -1;
            pageTable[i].pid = -1;
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
    // Thread *parent = currentThread->parent;
    printf("now in child thread,func_addr = %d pid=%d\n", func_addr,currentThread->getPid());
    // 初始化寄存器,这里的currentThread指的是child
    currentThread->space->InitRegisters();
    currentThread->space->RestoreState();

    // printf("child PCReg = %d\n", machine->ReadRegister(PCReg));
    machine->WriteRegister(PCReg,func_addr);
    machine->WriteRegister(NextPCReg,func_addr+4);
    // printf("child PCReg = %d\n", machine->ReadRegister(PCReg));
    machine->Run();
    // 这句按理不会执行到
    ASSERT(FALSE);
}
void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    if(which == NoException)
        printf("which=%d type=%d NoException\n", which,type);
    else if(which == PageFaultException);
        // printf("which=%d type=%d PageFaultException\n", which,type);
    else if(which == ReadOnlyException)
        printf("which=%d type=%d PageFaultException\n", which,type);
    else if(which == BusErrorException)
        printf("which=%d type=%d BusErrorException\n", which,type);
    else if(which == AddressErrorException)
        printf("which=%d type=%d AddressErrorException \n", which,type);
    else if(which == OverflowException)
        printf("which=%d type=%d OverflowException\n", which,type);
    else if(which == IllegalInstrException)
        printf("which=%d type=%d IllegalInstrException\n", which,type);
    else if(which == NumExceptionTypes)
        printf("which=%d type=%d NumExceptionTypes\n", which,type);
    // printf("which=%d type=%d\n", which,type);


    if (which == SyscallException) {
    	if(type == SC_Halt){
    		DEBUG('a', "Shutdown, initiated by user program.\n");
            printf("now in halt\n");
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
            printf("currentThread is %d\n", currentThread->getPid());
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
            char fileName[20];
            int i = 0,value = 0;
            do{
                if(!machine->ReadMem(addr + i,1,&value)){
                    value = 9;
                    continue;
                }
                fileName[i++] = value;
            }while(value != 0);
            printf("fileName = %s\n", fileName);
            OpenFile *executable = fileSystem->Open(fileName);
            if(executable == NULL){
                printf("Unable to open file %s\n", fileName);
                interrupt->Halt();
                return;
            }
            else{
                printf("successfully open the file\n");
            }

            AddrSpace* space = new AddrSpace(executable,fileName);
            delete executable;
            // 释放当前进程的页表
            UserProgClear();
            printf("InitRegisters.....\n");
            space->InitRegisters();
            space->RestoreState();
            currentThread->space = space;
            printf("renew the space....\n");
            // 需要重新fork吗？不需要
            // t-Fork(exec_func,0);
            printf("rerun....\n");
            machine->Run();
            machine->WriteRegister(2,currentThread->getPid());
            machine->PCAdvanced();
        }
        else if(type == SC_Fork){
            // Address 新增属性fileName
            // 需要逐页复制页表的内容,PTE新增pid属性，深拷贝的时候注意为当前进程 的pid
            // 建立线程，执行fork_func
            // void Fork(void (*func)());
            // 参数为函数指针，其实是用户程序地址空间中函数的虚拟地址
            // 用户级别的线程可以运行多个进程运行在一个用户空间内
            // 运行相同的 用户程序，不同的是，fork是从执行位置开始运行的，
            // 因此必须初始化PC为给定的函数指针
            printf("------------------now in SC_Fork--------------\n");
            int func_addr = machine->ReadRegister(4); // 函数指针的位置
            
            Thread *child = new Thread("child!");
            OpenFile *executable = fileSystem->Open(currentThread->space->fileName);
            if(executable == NULL){
                printf("Unable to open file %s\n", currentThread->space->fileName);
                interrupt->Halt();
                return;
            }
            else{
                printf("successfully open the file\n");
            }
            // 这里的地址空间是打开同一个可执行文件
            // printf("new child_space...\n");
            AddrSpace *child_space = new AddrSpace(executable); delete executable;
            // 复制space,这里拷贝后就出错
            // child_space->copySpace(currentThread->space,child->getPid());
            // printf("Finish copy the pageTable\n");
            child->space = child_space;
            // machine的寄存器赋值为子进程的func_addr,现在运行的是哪个进程？
            // fork之后进入就绪队列，当执行该线程时才重新初始化寄存器
            // printf("now begin to fork\n");
            child->Fork(fork_func,func_addr);
            // printf("end to fork\n");
            machine->PCAdvanced();
            // 这一句必须在child分配内存之后
            // currentThread->child = child;
            // 判断是否需要抢占
            machine->WriteRegister(2,child->getPid());
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
            printf("thread_id = %d\n", thread_id);
            while(pid_pool[thread_id] == 1){
                printf("pid:%d Yield\n", currentThread->getPid());
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
    else if((which ==  AddressErrorException)){
        printf("AddressErrorException\n");
        ASSERT(FALSE);
        // UserProgClear();
        // interrupt->Halt();
    }
    else if((which == IllegalInstrException)){
        // IllegalInstrException, // Unimplemented or reserved instr.
        printf("here is IllegalInstrException\n");
        ASSERT(FALSE);
    }
    else {
		printf("Unexpected user mode exception %d %d\n", which, type);
		ASSERT(FALSE);
    }
}

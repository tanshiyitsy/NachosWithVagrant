// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value == 0) { 			// semaphore not available
        // printf("thread:%s not get semaphore,will block\n", currentThread->getName());
    	queue->Append((void *)currentThread);	// so go to sleep
    	currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
    {
        scheduler->ReadyToRun(thread);
        // printf("thread:%s wake up from semaphore\n", currentThread->getName());
    }
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!
Lock::Lock(char* debugName) {
    // 声明一个信号量
    mutex = new  Semaphore(debugName,1);
    name = debugName;
    status = FREE;
    pid = -1;
    queue = new List;
}
Lock::~Lock() {
    // delete status;
}
// 这里只用Acquire，不用开关中断的语句好像不可以
// sleep里面要求关中断
void Lock::Acquire() {
    // 1. 判断锁的状态
    // 互斥量能拿到的话
    // mutex->P();
    // 如果阻塞了，中断是什么时候打开的？
    // 这里在单处理器中，A线程拿到锁后；B上CPU，想要拿锁，阻塞在这里，中断关了之后，没法儿切换了，B下不来了，不就死锁了吗？
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    while(status == BUSY){
        // 1.1 没有获取到，阻塞在这里
        // printf("thread:%s not Acquire the lock:%s,will block\n", currentThread->getName(),name);
        queue->Append((void *)currentThread);
        currentThread->Sleep();
    }
    // 1.2 获取到了，修改状态
    // printf("thread:%s  Acquire the lock:%s\n", currentThread->getName(),name);
    status = BUSY;
    pid = currentThread->getPid();
    (void) interrupt->SetLevel(oldLevel);   // re-enable interrupts
    // mutex->V();
}
void Lock::Release() {
    // 释放锁，需要判断是拥有锁的线程释放的
    // mutex->P();
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    while((pid == currentThread->getPid()) && (status == BUSY)){
        // printf("thread:%s Release lock:%s\n", currentThread->getName(),name);
        status = FREE;
        pid = -1;
         // 唤醒一个线程
        Thread *thread;
        thread = (Thread *)queue->Remove();
        if (thread != NULL) {   // make thread ready, consuming the V immediately
            // printf("thread :%s is wakeup in lock:%s\n", thread->getName(),name);
            scheduler->ReadyToRun(thread);
        }
    }
    (void) interrupt->SetLevel(oldLevel);
    // mutex->V();
}



// 条件变量中的条件是在哪里声明的？是在用户程序代码中声明的
Condition::Condition(char* debugName) {
    name = debugName;
    queue = new List;
 }
Condition::~Condition() { 
    // delete name;
}
// 让一个线程等在条件变量上
// 为什么条件变量是在里面释放锁，释放条件变量的时候是在外面加锁？
void Condition::Wait(Lock* conditionLock) {
    // ASSERT(FALSE); 
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
    printf("thread:%s will be block conditionLock\n", currentThread->getName());
    // 1. 把线程加入阻塞队列
    queue->Append((void *)currentThread);
    // 2. 释放锁
    conditionLock->Release();
    currentThread->Sleep();

    // 3. 在这里上锁吗？
    // printf("thread:%s obtain conditionLock again\n", currentThread->getName());
    conditionLock->Acquire();
     (void) interrupt->SetLevel(oldLevel);
}
void Condition::Signal(Lock* conditionLock) { 
    // 从阻塞队列中唤醒一个线程
     IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
     // 这里是否需要上锁？
     // 先上上吧，调用signal的用户进程会上锁，进来如果又上锁的话，不会死锁吗？
     // 上锁的时候，锁的拥有者初始化为被唤醒的进程,必须被执行的进行获取锁
     Thread *thread;
     thread = (Thread *)queue->Remove();

     if(thread != NULL){
        printf("thread:%s is wakeup in conditionLock\n", thread->getName());
        scheduler->ReadyToRun(thread);
     } 
     (void) interrupt->SetLevel(oldLevel);
}
// 把阻塞队列中的线程全部唤醒
void Condition::Broadcast(Lock* conditionLock) {
    IntStatus oldLevel = interrupt->SetLevel(IntOff);   // disable interrupts
     Thread *thread;
     thread = (Thread *)queue->Remove();
     while(thread != NULL){
        printf("thread:%s is wakeup in conditionLock\n", thread->getName());
        scheduler->ReadyToRun(thread);
        thread = (Thread *)queue->Remove();
     } 
     (void) interrupt->SetLevel(oldLevel);
 }

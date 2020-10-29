// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"

// testnum is set in main.cc
int testnum = 2;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

// ------------------------* origin *-------------------
// void
// SimpleThread(int which)
// {
//     int num;
    
//     for (num = 0; num < 5; num++) {
// 	printf("*** thread %d looped %d times\n", which, num);
//         currentThread->Yield();
//     }
// }

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 4; num++) {
        printf("*** thread %d looped %d times\n", which, num); 
        printf("thread name:%s pid:%d uid:%d priority:%d\n", currentThread->getName(), 
            currentThread->getPid(), currentThread->getUid(), currentThread->base_priority);
        printf("\n");
        // 每运行一次当前线程，就让出CPU，让另一个线程继续执行
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

// ------------------------* origin *-------------------
// void
// ThreadTest1()
// {
//     DEBUG('t', "Entering ThreadTest1");

//     Thread *t = new Thread("forked thread");

//     t->Fork(SimpleThread, 1);
//     SimpleThread(0);
// }

// ------------------------* new *-------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");
    // int num = 3;
    // Thread *t[num];
    // for(int i = 0; i < num;i++){
    //     t[i] = new Thread("test thread", i+1);
    // }
    // for(int i = num-1;i >= 0 ;i--){
    //     t[i]->Fork(SimpleThread, t[i]->pid);
    // }
    SimpleThread(0);
}

// 参数为该线程运行的时间
void TestTimeSlice(int time){
    for(int i = 0;i < time;i++){
        IntStatus oldLevel = interrupt->SetLevel(IntOff);
        (void)interrupt->SetLevel(oldLevel);
        printf("current: name=%s pid=%d use_trick = %d loop time:%d\n", currentThread->getName(),currentThread->getPid(),currentThread->getTricks(),i+1);
    }
}
// ------------------------* time slice *-------------------
void ThreadTest2(){
    Thread *t1 = new Thread("t1");
    Thread *t2 = new Thread("t2");
    t1->Fork(TestTimeSlice,11);
    t2->Fork(TestTimeSlice,20);
    TestTimeSlice(30);
}
//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum) {
        case 1:
        	ThreadTest1();
        	break;
        case 2:
            ThreadTest2();
            break;
        default:
        	printf("No test specified.\n");
        	break;
    }
}


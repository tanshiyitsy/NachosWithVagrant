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
int testnum = 7;

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
// 总共的缓冲区大小为4
#include "synch.h"
#include "unistd.h"
int cnt = 0;
int n = 5;
int buffer[5];
void produce_item(){
    cnt = 5;
    for(int i = 0;i < n;i++){
        if(buffer[i] == -1){
            // -1 表示为空
            // 9 表示内容
            buffer[i] = 9;
            // cnt++;
            // printf("%s produce an intem in %d\n", currentThread->getName(),i);
            // return;
        }
    }
    printf("%s: fill all the buffer\n", currentThread->getName());
}
void consumer_item(){
    for(int i = 0;i < n;i++){
        if(buffer[i] != -1){
            buffer[i] = -1;
            cnt--;
            printf("%s consume an item in %d\n", currentThread->getName(),i);
            return;
        }
    }
}
// 条件变量和锁实现,生产者和消费者用两个条件变量
Lock *mutex = new Lock("mutex");
Condition *pro_con = new Condition("pro_con");
Condition *cons_con = new Condition("cons_con");
void Producer(int which){
    while(true){
        mutex->Acquire();
        // printf("thread:%s Acquire the mutex\n", currentThread->getName());
        if(cnt == n){
            // 缓冲区满了，用条件变量阻塞当前进程
            printf("buffer is full\n");
            pro_con->Wait(mutex);
        }
        // 满足条件
        produce_item();
        sleep(1);
        // 唤醒所有消费者
        cons_con->Broadcast(mutex);
        // if(cnt == 1){
        //     // 唤醒一个消费者
        //     printf("buffer have new item\n");
        //     cons_con->Signal(mutex);
        // }
        // printf("(thread:%s Release Lock)\n", currentThread->getName());
        mutex->Release();
    }
}
void Consumer(int which){
    while(true){
        mutex->Acquire();
        // printf("thread:%s Acquire the mutex\n", currentThread->getName());
        if(cnt == 0){
            // 缓冲区为空，阻塞
            printf("buffer is empty\n");
            cons_con->Wait(mutex);
        }
        consumer_item();
        sleep(1);
        if(cnt == n-1){
            // 唤醒生产者
            // printf("buffer have new empty\n");
            pro_con->Signal(mutex);
        }
        // printf("(thread:%s Release Lock)\n", currentThread->getName());
        mutex->Release();
    }
}


// 用信号量和锁实现
/*
Lock *mutex = new Lock("mutex");
Semaphore *full = new Semaphore("full",0);
Semaphore *empty = new Semaphore("empty",n);
void Consumer(int which){
    while(true){
        full->P();
        mutex->Acquire();
        consumer_item();
        sleep(1);
        mutex->Release();
        empty->V();
    }
}
void Producer(int which){
    while(true){
        empty->P();
        mutex->Acquire();
        produce_item();
        sleep(1);
        mutex->Release();
        full->V();
    }
}
void ThreadTestLab3(){
    // 初始化缓冲区
    for(int i = 0;i < n;i++)
        buffer[i] = -1;
    // 创建三个生产者
    Thread *producer1 = new Thread("producer1");
    producer1->Fork(Producer,1);
    Thread *producer2 = new Thread("producer2");
    producer2->Fork(Producer,1);
    Thread *producer3 = new Thread("producer3");
    producer3->Fork(Producer,1);

    // 创建三个消费者
    Thread *consumer1 = new Thread("consumer1");
    consumer1->Fork(Consumer,1);
    Thread *consumer2 = new Thread("consumer2");
    consumer2->Fork(Consumer,1);
    Thread *consumer3 = new Thread("consumer3");
    consumer3->Fork(Consumer,1);
    currentThread->Yield();
}
*/

// Lock *r_mutex = new Lock("r_mutex");
// Lock *w_mutex = new Lock("w_mutex");
Lock *RWmutex = new Lock("RW_mutex");
Lock *Rmutex = new Lock("R_mutex");
// 这里rc本身就是共享的，需要加锁
int rc = 0; // 读进程的计数
void Read(int which){
   // 判断是否是第一个读者
    while(true){
        Rmutex->Acquire();
        if(rc == 0){
            RWmutex->Acquire();
        }
        // else if(rc > 0){
        //     // 已经有读者在读，所以可以直接读
        // }
        rc++;
        printf("thread:%s is reading,now have %d readers\n", currentThread->getName(),rc);
        Rmutex->Release();
        sleep(1);
        // 结束
       Rmutex->Acquire();
       rc--;
       if(rc == 0){
            // 最后一个读者
            RWmutex->Release();
       }
       Rmutex->Release();
    }
}
void Write(int which){
    while(true){
        RWmutex->Acquire();
        printf("thread:%s is writing,now have %d readers\n", currentThread->getName(),rc);
        sleep(2);
        RWmutex->Release();
    }
}
void Lab3RWLockTest(){
    // 创建4个读，1个写
    Thread *reader1 = new Thread("reader1");
    reader1->Fork(Read,1);

    Thread *writer1 = new Thread("writer1");
    writer1->Fork(Write,1);

    Thread *reader2 = new Thread("reader2");
    reader2->Fork(Read,1);
    Thread *reader3 = new Thread("reader3");
    reader3->Fork(Read,1);
    // Thread *reader4 = new Thread("reader4");
    // reader4->Fork(Read,1);

}
void Lab3Chan1Test(){
    // 创建一个生产者
    Thread *producer1 = new Thread("producer1");
    producer1->Fork(Producer,1);

    // 创建三个消费者
    Thread *consumer1 = new Thread("consumer1");
    consumer1->Fork(Consumer,1);
    Thread *consumer2 = new Thread("consumer2");
    consumer2->Fork(Consumer,1);
    Thread *consumer3 = new Thread("consumer3");
    consumer3->Fork(Consumer,1);
    currentThread->Yield();
}
void funcSender(int receive_pid){
    // printf("thread:%s ")
    ASSERT(currentThread->Send("hello OS Lab7",receive_pid));
}
void funcReceive(int which){
    char msg[Msg_Len];
    ASSERT(currentThread->Receive(msg)>0);
    printf("thread:%s receive msg:%s\n",currentThread->getName(),msg);
}
void Lab7Test(){
    // printf("this lab7 test\n");
    Thread *receiver=new Thread("receiver");
    Thread *sender = new Thread("sender");
    sender->Fork(funcSender,receiver->getPid());
    receiver->Fork(funcReceive,sender->getPid());
    currentThread->Yield();
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
        case 3:
            // ThreadTestLab3();
            // Lab3RWLockTest();
        Lab3Chan1Test();
            break;
        case 7:
            Lab7Test();
            break;
        default:
        	printf("No test specified.\n");
        	break;
    }
}


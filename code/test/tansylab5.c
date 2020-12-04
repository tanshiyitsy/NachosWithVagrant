/* halt.c
 *	Simple program to test whether running a user program works.
 *	
 *	Just do a "syscall" that shuts down the OS.
 *
 * 	NOTE: for some reason, user programs with global data structures 
 *	sometimes haven't worked in the Nachos environment.  So be careful
 *	out there!  One option is to allocate data structures as 
 * 	automatics within a procedure, but if you do this, you have to
 *	be careful to allocate a big enough stack to hold the automatics!
 */

#include "syscall.h"

void func(){
	// 1. 创建文件
	char *name = "test_fork";
	if(Create(name) != -1){
		// 2. 打开文件
		int fd = Open(name);
		if(fd != -1){
			// 3. 写入数据
			int cnt = 35;
			Write("this content is for test fork func",cnt,fd);
			Close(fd);
		}
	}
}
int
main()
{
	// 1. 创建文件
	// char *name = "test_lab5";
	// if(Create(name) != -1){
	// 	// 2. 打开文件
	// 	int fd = Open(name);
	// 	if(fd != -1){
	// 		// 3. 写入数据
	// 		char buffer[30];
	// 		int cnt = 33;
	// 		Write("hello this is lab5 about syscall",cnt,fd);
	// 		Close(fd);
	// 		fd = Open(name);
	// 		cnt = Read(buffer,cnt,fd);
	// 		Close(fd);
	// 		fd = Open(name);
	// 		Write(buffer,cnt,ConsoleOutput);
	// 		Close(fd);
	// 	}
	// }
	// 1. fork
	int fid = Fork(func);
	int eid = Exec("halt");
	Yield();
	Join(id);
	Join(eid);

	Exit(0);
    // Halt();
    /* not reached */
}

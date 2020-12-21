// fstest.cc 
//	Simple test routines for the file system.  
//
//	We implement:
//	   Copy -- copy a file from UNIX to Nachos
//	   Print -- cat the contents of a Nachos file 
//	   Perftest -- a stress test for the Nachos file system
//		read and write a really large file in tiny chunks
//		(won't work on baseline system!)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "utility.h"
#include "filesys.h"
#include "system.h"
#include "thread.h"
#include "disk.h"
#include "stats.h"

#define TransferSize 	10 	// make it small, just to be difficult

//----------------------------------------------------------------------
// Copy
// 	Copy the contents of the UNIX file "from" to the Nachos file "to"
//----------------------------------------------------------------------

void
Copy(char *from, char *to)
{
    FILE *fp;
    OpenFile* openFile;
    int amountRead, fileLength;
    char *buffer;

// Open UNIX file
    if ((fp = fopen(from, "r")) == NULL) {	 
	printf("Copy: couldn't open input file %s\n", from);
	return;
    }

// Figure out length of UNIX file
    fseek(fp, 0, 2);		
    fileLength = ftell(fp);
    fseek(fp, 0, 0);

// Create a Nachos file of the same length
    DEBUG('f', "Copying file %s, size %d, to file %s\n", from, fileLength, to);
    if (!fileSystem->Create(to, fileLength)) {	 // Create Nachos file
	printf("Copy: couldn't create output file %s\n", to);
	fclose(fp);
	return;
    }
    
    openFile = fileSystem->Open(to);
    ASSERT(openFile != NULL);
    
// Copy the data in TransferSize chunks
    buffer = new char[TransferSize];
    while ((amountRead = fread(buffer, sizeof(char), TransferSize, fp)) > 0)
	openFile->Write(buffer, amountRead);	
    delete [] buffer;

// Close the UNIX and the Nachos files
    delete openFile;
    fclose(fp);
}

//----------------------------------------------------------------------
// Print
// 	Print the contents of the Nachos file "name".
//----------------------------------------------------------------------

void
Print(char *name)
{
    OpenFile *openFile;    
    int i, amountRead;
    char *buffer;

    if ((openFile = fileSystem->Open(name)) == NULL) {
	printf("Print: unable to open file %s\n", name);
	return;
    }
    
    buffer = new char[TransferSize];
    while ((amountRead = openFile->Read(buffer, TransferSize)) > 0)
	for (i = 0; i < amountRead; i++)
	    printf("%c", buffer[i]);
    delete [] buffer;

    delete openFile;		// close the Nachos file
    return;
}

//----------------------------------------------------------------------
// PerformanceTest
// 	Stress the Nachos file system by creating a large file, writing
//	it out a bit at a time, reading it back a bit at a time, and then
//	deleting the file.
//
//	Implemented as three separate routines:
//	  FileWrite -- write the file
//	  FileRead -- read the file
//	  PerformanceTest -- overall control, and print out performance #'s
//----------------------------------------------------------------------

#define FileName 	"TestFile"
#define Contents 	"1234567890"
#define ContentSize 	strlen(Contents)
#define FileSize 	((int)(ContentSize * 5000))
#define MAXN 100

static void 
FileWrite()
{
    printf("now in FileWrite...\n");
    OpenFile *openFile;    
    int i, numBytes;

    printf("Sequential write of %d byte file, in %d byte chunks\n", 
	FileSize, ContentSize);
    if (!fileSystem->Create(FileName, 0)) {
      printf("Perf test: can't create %s\n", FileName);
      return;
    }
    openFile = fileSystem->Open(FileName);
    if (openFile == NULL) {
	printf("Perf test: unable to open %s\n", FileName);
	return;
    }
    for (i = 0; i < FileSize; i += ContentSize) {
        numBytes = openFile->Write(Contents, ContentSize);
	if (numBytes < 10) {
	    printf("Perf test: unable to write %s\n", FileName);
	    delete openFile;
	    return;
	}
    }
    delete openFile;	// close file
}

static void 
FileRead()
{
    // printf("now in FileRead...\n");
    OpenFile *openFile;    
    char *buffer = new char[ContentSize];
    int i, numBytes;

    printf("Sequential read of %d byte file, in %d byte chunks\n", 
	FileSize, ContentSize);

    if ((openFile = fileSystem->Open(FileName)) == NULL) {
    	printf("Perf test: unable to open file %s\n", FileName);
    	delete [] buffer;
    	return;
    }
    for (i = 0; i < MAXN; i += ContentSize) {
        numBytes = openFile->Read(buffer, ContentSize);
    	if ((numBytes < 10) || strncmp(buffer, Contents, ContentSize)) {
    	    printf("Perf test: unable to read %s\n", FileName);
    	    delete openFile;
    	    delete [] buffer;
    	    return;
    	}
    }
    delete [] buffer;
    delete openFile;	// close file
}
// #define DIRECTORY 0
// #define NORFILE 1
void testExercise2(char *filename){
    if (!fileSystem->Create(filename, 0,1,"/")) {
      printf("Perf test: can't create %s\n", filename);
      return;
    }
    else{
        printf("successfully create file:%s\n",filename);
    }

}
void testExercise3(char *filename){
    if (!fileSystem->Create(filename, 5400,1,"/")) {
      printf("Perf test: can't create %s\n", filename);
      return;
    }
    else{
        printf("successfully create file:%s\n",filename);
    }

}
void testExercise4(){
    fileSystem->Create("dir1",10,0,"/");
    fileSystem->Create("dir2",10,0,"/dir1/");
    fileSystem->Create("dir3",10,0,"/");
    fileSystem->Create("file1",5400,1,"/dir1/dir2/");
    printf("-----------------------------------------list file-----------------------------\n");
    fileSystem->List();
    fileSystem->Remove("file1","/dir1/dir2");
    fileSystem->Remove("dir2","/dir1/");
    fileSystem->Remove("dir1","/");
    fileSystem->Remove("dir3","/");
}
void testExercise5(){
    char *name = "file1";
    char *path = "/";
    fileSystem->Create("file1",256,1,"/");
    OpenFile *openFile = fileSystem->Open(name,path);
    openFile->ExtendFile(256);
    openFile->ExtendFile(1250);
    openFile->ExtendFile(5400);
    printf("-----------------------------------------list file-----------------------------\n");
    fileSystem->List();
    fileSystem->Remove("file1","/");
}
void testExercise6(){
    char *name = "file1";
    char *path = "/";
    char *from = "hello world";
    fileSystem->Create("file1",256,1,"/");
    OpenFile *openFile = fileSystem->Open(name,path);
    for(int i = 0;i < 128;i++){
        // printf("i=%d\n", i);
        openFile->Write(from, 11);
    }
    printf("-----------------------------------------list file-----------------------------\n");
    fileSystem->Print();
    fileSystem->Remove("file1","/");
}
void read(int which){
    printf("thread:%s in read\n", currentThread->getName());
    FileRead();
    
}
void write(){
    printf("thread:%s in write\n", currentThread->getName());
    OpenFile *openFile;    
    int i, numBytes;
    
    openFile = fileSystem->Open(FileName);
    if (openFile == NULL) {
        printf("Perf test: unable to open %s\n", FileName);
        return;
    }
    for (i = 0; i < MAXN; i += ContentSize) {
        numBytes = openFile->Write(Contents, ContentSize);
        if (numBytes < 10) {
            printf("Perf test: unable to write %s\n", FileName);
            delete openFile;
            return;
        }
    }
    delete openFile;    // close file
}
void testExercise7(){
    if (!fileSystem->Create(FileName, 0)) {
      printf("Perf test: can't create %s\n", FileName);
      return;
    }
    else{
        printf("successfully create file\n");
    }
    write();
    Thread* t1 = new Thread("reader1");
    t1->Fork(read,1);
    read(1);
    printf("%s remove the file\n",currentThread->getName());
    while(fileSystem->Remove(FileName)==FALSE);
}
void remove(int which){
    // printf("thread:%s remove the file\n",currentThread->getName());
    fileSystem->Remove(FileName,"/");
    // while(fileSystem->Remove(FileName,"/")==FALSE);
}
void testExercise7_remove(){
    if (!fileSystem->Create(FileName, 0,1,"/")) {
      printf("Perf test: can't create %s\n", FileName);
      return;
    }
    else{
        printf("successfully create file\n");
    }
    // write();
    OpenFile *openFile = fileSystem->Open(FileName,"/");
    Thread* t1 = new Thread("remove");
    t1->Fork(remove,1);
    currentThread->Yield();
    
}
#define PIPE "PIPE"
#define MaxPipeSize SectorSize
void PipeWrite(int which){
    // 1. 打开管道
    OpenFile *pipFile = fileSystem->Open(PIPE);
    char *buffer = new char[MaxPipeSize];
    printf("thread:%s input to pipe...\n", currentThread->getName());
    // 2. 从控制台读入内容
    scanf("%s",buffer);
    int size = strlen(buffer);
    // 3. 把内容重定向到管道
    pipFile->Write(buffer,size);
    delete [] buffer;
    delete pipFile;
}
void PipeRead(int which){
    // 1. 打开管道
    OpenFile *pipFile = fileSystem->Open(PIPE);
    char *buffer = new char[MaxPipeSize];
    // 2. 从管道读取内容
    // printf("len=%d\n", pipFile->Length());
    pipFile->Read(buffer,pipFile->Length());
    buffer[pipFile->Length()] = '\0';
    // 3. 重定向到终端
    printf("thread:%s output from pip:%s\n", currentThread->getName(),buffer);
    delete [] buffer;
    delete pipFile;
}
void Pipe(){
    // 1. 创建管道文件
    if (!fileSystem->Create(PIPE, 0,1,"/")) {
      printf("Perf test: can't create %s\n", FileName);
      return;
    }
    else{
        printf("successfully create file\n");
    }
    Thread *pip_write = new Thread("pip_write");
    Thread *pip_read = new Thread("pip_read");
    pip_write->Fork(PipeWrite,0);
    pip_read->Fork(PipeRead,0);
}
void
PerformanceTest()
{
    printf("Starting file system performance test:\n");
    Pipe();
    // testExercise3("testfile_this_is_a_very_long_filename_test1");
    // testExercise4();
    // testExercise6();
    // testExercise7();
    // testExercise7_remove();
    // printf("-----------------------------------------list file-----------------------------\n");
    // fileSystem->List();
    // stats->Print();
    // FileWrite();
    // FileRead();
    // if (!fileSystem->Remove(FileName)) {
    //   printf("Perf test: unable to remove %s\n", FileName);
    //   return;
    // }
    // stats->Print();
}


// openfile.h 
//	Data structures for opening, closing, reading and writing to 
//	individual files.  The operations supported are similar to
//	the UNIX ones -- type 'man open' to the UNIX prompt.
//
//	There are two implementations.  One is a "STUB" that directly
//	turns the file operations into the underlying UNIX operations.
//	(cf. comment in filesys.h).
//
//	The other is the "real" implementation, that turns these
//	operations into read and write disk sector requests. 
//	In this baseline implementation of the file system, we don't 
//	worry about concurrent accesses to the file system
//	by different threads -- this is part of the assignment.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef OPENFILE_H
#define OPENFILE_H

#include "copyright.h"
#include "utility.h"

#ifdef FILESYS_STUB			// Temporarily implement calls to 
					// Nachos file system as calls to UNIX!
					// See definitions listed under #else
// 定义了一个打开文件控制结构，当用户打开了一个文件时，系统为其产生一个打开文件控制结构，
// 以后用户对该文件的访问都可以通过该结构。
// 针对filesystem的两套实现，这里也有两套实现。
class OpenFile {
  public:
    OpenFile(int f) { file = f; currentOffset = 0; }	// open the file
    ~OpenFile() { Close(file); }			// close the file

    int ReadAt(char *into, int numBytes, int position) { 
        //     int retVal = lseek(fd, offset, whence);
        // ASSERT(retVal >= 0);
        //  whence为0 表示把文件偏移量 设置为position
    		Lseek(file, position, 0); 
            // return read(fd, buffer, nBytes);
            // fd是文件描述符，nBytes是请求读取的字节数
            // 读取的数据保存在缓冲区中，同时文件的当前位置要往后移
		return ReadPartial(file, into, numBytes); 
		}	
    int WriteAt(char *from, int numBytes, int position) { 
    		Lseek(file, position, 0); 
		WriteFile(file, from, numBytes); 
		return numBytes;
		}	
    int Read(char *into, int numBytes) {
		int numRead = ReadAt(into, numBytes, currentOffset); 
		currentOffset += numRead;
		return numRead;
    		}
    int Write(char *from, int numBytes) {
		int numWritten = WriteAt(from, numBytes, currentOffset); 
		currentOffset += numWritten;
		return numWritten;
		}

    int Length() { Lseek(file, 0, 2); return Tell(file); }
    
  private:
    int file;
    int currentOffset;
};

#else // FILESYS
class FileHeader;

class OpenFile {
  public:
    // 打开一个文件，该文件的文件头在sector扇区
    OpenFile(int sector);		// Open a file whose header is located
					// at "sector" on the disk
    // 关闭文件
    ~OpenFile();			// Close the file

    // 移动文件位置指针，从文件头开始
    void Seek(int position); 		// Set the position from which to 
					// start reading/writing -- UNIX lseek

    // 从文件中读取numByte到into缓冲，同时移动文件位置指针（通过ReadAt实现）
    int Read(char *into, int numBytes); // Read/write bytes from the file,
					// starting at the implicit position.
					// Return the # actually read/written,
					// and increment position in file.
    // 将from缓冲内容写入numBytes字节到文件中，同时移动文件位置指针（通过writeAt实现）
    int Write(char *from, int numBytes);

    // 将position开始的numBytes读入into缓冲
    int ReadAt(char *into, int numBytes, int position);
    					// Read/write bytes from the file,
					// bypassing the implicit position.
    // 将from缓冲中numBytes写入从position开始的区域
    int WriteAt(char *from, int numBytes, int position);

    // 返回文件长度
    int Length(); 			// Return the number of bytes in the
					// file (this interface is simpler 
					// than the UNIX idiom -- lseek to 
					// end of file, tell, lseek back 
    
  private:
    // 该文件对应的文件头（建立关系）
    FileHeader *hdr;			// Header for this file 
    // 当前文件位置指针
    int seekPosition;			// Current position within the file
};

#endif // FILESYS

#endif // OPENFILE_H

// filehdr.h 
//	Data structures for managing a disk file header.  
//
//	A file header describes where on disk to find the data in a file,
//	along with other information about the file (for instance, its
//	length, owner, etc.)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef FILEHDR_H
#define FILEHDR_H

#include "disk.h"
#include "bitmap.h"

#define NumDirect 	((SectorSize - 2 * sizeof(int)) / sizeof(int))
#define MaxFileSize 	(NumDirect * SectorSize)

// The following class defines the Nachos "file header" (in UNIX terms,  
// the "i-node"), describing where on disk to find all of the data in the file.
// The file header is organized as a simple table of pointers to
// data blocks. 
//
// The file header data structure can be stored in memory or on disk.
// When it is on disk, it is stored in a single sector -- this means
// that we assume the size of this data structure to be the same
// as one disk sector.  Without indirect addressing, this
// limits the maximum file length to just under 4K bytes.
//
// There is no constructor; rather the file header can be initialized
// by allocating blocks for the file (if it is a new file), or by
// reading it from disk.
// 文件头模块分析
// 文件头就是iNode，包含除了文件名之外的所有属性。
// 包括文件长度，地址索引表，文件名在目录项中。
// 索引表是文件的 逻辑地址 和 物理地址 的对应关系，nachos的文件的索引表只有直接索引。
// nachos中，每个扇区大小为128个字节，每个iNode占用一个扇区，共有30个直接索引
// 因此nachos中最大的文件不能超过3840字节
class FileHeader {
  public:
    // 通过文件大小，初始化文件头
    bool Allocate(BitMap *bitMap, int fileSize);// Initialize a file header, 
						//  including allocating space 
						//  on disk for the file data
    // 将一个 文件 所占用的数据空间释放，没有释放 文件头 所占用的空间
    void Deallocate(BitMap *bitMap);  		// De-allocate this file's 
						//  data blocks
    // 从磁盘扇区中取出文件头
    void FetchFrom(int sectorNumber); 	// Initialize file header from disk
    // 将文件头写入磁盘扇区
    void WriteBack(int sectorNumber); 	// Write modifications to file header
					//  back to disk

    // 文件逻辑地址向物理地址的转换
    int ByteToSector(int offset);	// Convert a byte offset into the file
					// to the disk sector containing
					// the byte

    // 返回文件长度
    int FileLength();			// Return the length of the file 
					// in bytes

    // 打印文件头信息
    void Print();			// Print the contents of the file.

  private:
    // 文件长度——字节数
    int numBytes;			// Number of bytes in the file
    // 文件占用的扇区数
    int numSectors;			// Number of data sectors in the file
    // 文件索引表
    int dataSectors[NumDirect];		// Disk sector numbers for each data 
					// block in the file
};

#endif // FILEHDR_H

// directory.h 
//	Data structures to manage a UNIX-like directory of file names.
// 
//      A directory is a table of pairs: <file name, sector #>,
//	giving the name of each file in the directory, and 
//	where to find its file header (the data structure describing
//	where to find the file's data blocks) on disk.
//
//      We assume mutual exclusion is provided by the caller.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "openfile.h"

#define FileNameMaxLen 		9	// for simplicity, we assume 
					// file names are <= 9 characters long

// The following class defines a "directory entry", representing a file
// in the directory.  Each entry gives the name of the file, and where
// the file's header is to be found on disk.
//
// Internal data structures kept public so that Directory operations can
// access them directly.
// 目录在文件系统中是一个很重要的部分， 它实际上是一张表， 将字符形式的文件名与实际文
// 件的文件头相对应。这样用户就能方便地通过文件名来访问文件。
// Nachos 中的目录结构非常简单，它只有一级目录，也就是只有根目录；而且根目录的大小
// 是固定的， 整个文件系统中只能存放有限个文件。 

// 目录项结构
class DirectoryEntry {
  public:
    // 是否在使用标志
    bool inUse;				// Is this directory entry in use?
    // 对应文件的文件头位置
    int sector;				// Location on disk to find the 
					//   FileHeader for this file 
    // 对应文件的文件名
    char name[FileNameMaxLen + 1];	// Text name for file, with +1 for 
					// the trailing '\0'
};

// The following class defines a UNIX-like "directory".  Each entry in
// the directory describes a file, and where to find it on disk.
//
// The directory data structure can be stored in memory, or on disk.
// When it is on disk, it is stored as a regular Nachos file.
//
// The constructor initializes a directory structure in memory; the
// FetchFrom/WriteBack operations shuffle the directory information
// from/to disk. 

class Directory {
  public:
    // 初始化，size规定目录中可以放多少文件
    Directory(int size); 		// Initialize an empty directory
					// with space for "size" files
    ~Directory();			// De-allocate the directory

    // 从目录文件中读入目录结构
    void FetchFrom(OpenFile *file);  	// Init directory contents from disk
    // 将该目录结构写回目录文件
    void WriteBack(OpenFile *file);	// Write modifications to 
					// directory contents back to disk

    // 在目录中寻找文件名，返回文件头的物理位置
    int Find(char *name);		// Find the sector number of the 
					// FileHeader for file: "name"
    // 将一个文件加入到目录中
    bool Add(char *name, int newSector);  // Add a file name into the directory

    // 将一个文件从目录中删除
    bool Remove(char *name);		// Remove a file from the directory

    // 列出目录中所有的文件和内容
    void List();			// Print the names of all the files
					//  in the directory
    void Print();			// Verbose print of the contents
					//  of the directory -- all the file
					//  names and their contents.

  private:
    // 目录项数目
    int tableSize;			// Number of directory entries
    // 目录项表
    DirectoryEntry *table;		// Table of pairs: 
					// <file name, file header location> 
    // 根据文件名找出该文件在目录项表中的表项序号
    int FindIndex(char *name);		// Find the index into the directory 
					//  table corresponding to "name"
};

#endif // DIRECTORY_H

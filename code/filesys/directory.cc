// directory.cc 
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//	Fixing this is one of the parts to the assignment.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"
#include "malloc.h"

//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------
int sector;             // Location on disk to find the 
                    //   FileHeader for this file 
    // char name[FileNameMaxLen + 1];   // Text name for file, with +1 for 
                    // the trailing '\0'
Directory::Directory(int size)
{
    table = new DirectoryEntry[size];
    tableSize = size;
    for (int i = 0; i < tableSize; i++)
	{
        table[i].inUse = FALSE;
        table[i].sector = 0;
        table[i].type = 0;
        memset(table[i].name,0,sizeof(table[i].name));
        memset(table[i].path,0,sizeof(table[i].path));
    }
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory()
{ 
    delete [] table;
} 

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void
Directory::FetchFrom(OpenFile *file)
{
    (void) file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void
Directory::WriteBack(OpenFile *file)
{
    (void) file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::FindIndex(char *name)
{
    for (int i = 0; i < tableSize; i++){
        // printf("i=%d table[i].name=%s\n", i,table[i].name);
        if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen))
        return i;
    }
        
    return -1;		// name not in directory
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't 
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::Find(char *name)
{
    int i = FindIndex(name);

    if (i != -1)
	return table[i].sector;
    return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------

bool
Directory::Add(char *name, int newSector)
{ 
    if (FindIndex(name) != -1)
	return FALSE;

    for (int i = 0; i < tableSize; i++)
        if (!table[i].inUse) {
            table[i].inUse = TRUE;
            strncpy(table[i].name, name, FileNameMaxLen); 
            table[i].sector = newSector;
        return TRUE;
	}
    return FALSE;	// no space.  Fix when we have extensible files.
}
bool
Directory::Add(char *name, int newSector,int type,char *path)
{ 
    if (FindIndex(name) != -1)
        return FALSE;
    for (int i = 0; i < tableSize; i++)
        if (!table[i].inUse) {
            table[i].inUse = TRUE;
            // table[i].name = new char(strlen(name));
            strncpy(table[i].name, name,FileNameMaxLen);
            table[i].sector = newSector;
            table[i].type = type;
            // table[i].path = new char(strlen(path));
            strncpy(table[i].path, path, FilePathMaxLen);
            // printf("gagfagfd name is %s path is %s\n", table[i].name,table[i].path);
            // printf("add i=%d name %s sector=%d\n", i,name,newSector);
            return TRUE;
        }
    return FALSE;   // no space.  Fix when we have extensible files.
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory. 
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool
Directory::Remove(char *name)
{ 
    int i = FindIndex(name);
    if (i == -1)
    return FALSE;       // name not in directory
    table[i].inUse = FALSE;
    return TRUE;    
}
//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory. 
//----------------------------------------------------------------------
void
Directory::RecurList(int sector){
    // 1. 读取目录文件
    OpenFile *dir_file = new OpenFile(sector);
    Directory *directory = new Directory(NumDirEntries);
    directory->FetchFrom(dir_file);
    directory->List();
   //  FileHeader *hdr = new FileHeader;
   //  for (int i = 0; i < tableSize; i++){
   //  if (table[i].inUse){
   //      hdr->FetchFrom(table[i].sector);
   //      printf("%s\t%d\t%d\t%s\t%s\t%s\t%s\n", table[i].name,table[i].sector,table[i].type,table[i].path,
   //          hdr->get_ctime(),hdr->get_last_vtime(),hdr->get_last_mtime());
   //      if(table[i].type == 0){
   //          // 目录文件
   //          RecurList(table[i].sector);
   //      }
   //  }
   //  // printf("\n");
   // }
   // delete hdr;
}
void
Directory::List()
{
   FileHeader *hdr = new FileHeader;
   for (int i = 0; i < tableSize; i++){
        if (table[i].inUse){
            hdr->FetchFrom(table[i].sector);
            printf("%s\t%d\t%d\t%s\t%s\t\t%s\t\t%s\n", table[i].name,table[i].sector,table[i].type,table[i].path,
                hdr->get_ctime(),hdr->get_last_vtime(),hdr->get_last_mtime());
            if(table[i].type == 0){
                RecurList(table[i].sector);
            }
        }
   }
    delete hdr;
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void
Directory::Print()
{ 
    FileHeader *hdr = new FileHeader;

    printf("Directory contents:\n");
    printf("Name\tSector\ttype\tpath\tctime\tlast_vtime\tlast_mtime\t\n");
    for (int i = 0; i < tableSize; i++)
	if (table[i].inUse) {
	    // printf("Name: %s, Sector: %d type:%d path:%s\n", table[i].name, table[i].sector,table[i].type,table[i].path);
	    hdr->FetchFrom(table[i].sector);
        printf("%s\t%d\t%d\t%s\t%s\t%s\t%s\t\n", table[i].name,table[i].sector,table[i].type,table[i].path,
            hdr->get_ctime(),hdr->get_last_vtime(),hdr->get_last_mtime());
	    hdr->Print();
	}
    printf("\n");
    delete hdr;
}
int Directory::GetDirSector(char *path){
    // 根据绝对path获取父目录文件所在磁盘号
    // 1. 读取根目录文件
    int sector = 1;
    OpenFile *dir_file = new OpenFile(sector);
    Directory *dir = new Directory(NumDirEntries);
    dir->FetchFrom(dir_file);
    int str_pos=1,sub_str_pos=0;
    char sub_str[FilePathMaxLen];
    while(str_pos < strlen(path)){
        sub_str[sub_str_pos++] = path[str_pos++];
        if(path[str_pos] == '/'){
            sub_str[sub_str_pos] = '\0';
            sector = dir->Find(sub_str);
            // printf("sub_str is %s,Sector is %d\n", sub_str,sector);
            dir_file = new OpenFile(sector);
            dir = new Directory(NumDirEntries);
            dir->FetchFrom(dir_file);
            str_pos++;
            sub_str_pos = 0;
        }
    }
    return sector;
}
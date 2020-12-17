// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"
#include "time.h"
#include "string.h"

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
    // NumDirect is 11
    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);
    if (freeMap->NumClear() < numSectors){
        return FALSE;       // not enough space
    }
    printf("fileSize is %d numSectors is %d\n", fileSize,numSectors);
    if(numSectors < 6){
        for(int i=0;i < numSectors;i++){
            dataSectors[i] = freeMap->Find();
            printf("i=%d sector=%d\n", i,dataSectors[i]);
        }
        return TRUE;
    }
    else{
        // i记录当前已经分配的数据物理块，cnt记录当前占用的直接索引表项
        int i=0;
        for(i=0;i<6;i++){
            dataSectors[i] = freeMap->Find();
            printf("i=%d sector=%d\n", i,dataSectors[i]);
        }
        int cnt = 6;
        while(i < numSectors){
            dataSectors[cnt] = freeMap->Find();
            printf("indirect_index=%d indirect_sector=%d\n", cnt,dataSectors[cnt]);
            int indirect[32];
            for(int j=0;j<32 && i < numSectors;j++,i++){
                indirect[j] = freeMap->Find();
                printf("i=%d j=%d sector=%d\n", i,j,indirect[j]);
            }
            synchDisk->WriteSector(dataSectors[cnt],(char *)indirect);
            cnt++;
        }
        return TRUE;
    }
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
    printf("in Deallocate...\n");
    if(numSectors < 6){
        for(int i=0;i < numSectors;i++){
            printf("i=%d sector=%d\n", i,dataSectors[i]);
            ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
            freeMap->Clear((int) dataSectors[i]);
        }
    }
    else{
        int i = 0;
        for(i=0;i<6;i++){
            printf("i=%d sector=%d\n", i,dataSectors[i]);
            ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
            freeMap->Clear((int) dataSectors[i]);
        }
        int cnt = 6;
        while(i < numSectors){
            char *indirect = new char[SectorSize];
            synchDisk->ReadSector(dataSectors[cnt],indirect);
            printf("indirect_index=%d indirect_sector=%d\n", cnt,dataSectors[cnt]);
            for(int j=0;j<32 && i < numSectors;j++,i++){
                printf("i=%d j=%d sector=%d\n", i,j,indirect[j*4]);
                ASSERT(freeMap->Test((int) indirect[j * 4]));  // ought to be marked!
                freeMap->Clear((int) indirect[j * 4]);
            }
            cnt++;
        }

    }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
    if(offset < 6 * SectorSize){
        return(dataSectors[offset / SectorSize]);
    }
    else{
        // 所在物理块数
        int sector_nums = offset/SectorSize;
        // 所在一级索引表项
        int indirect_index = (sector_nums - 6)/32;
        char *indirect = new char[SectorSize];
        // 把其所在的一级索引表读出来
        synchDisk->ReadSector(dataSectors[indirect_index+6],indirect);
        // 找到其在一级索引表的位置
        int indirect_offset=0;
        if(indirect_index==0){
            indirect_index = sector_nums-6;
        }
        else{
            indirect_index = sector_nums-6-(indirect_index-1)*32;
        }
        printf("ByteTosector:offset=%d indirect_index = %d indirect_offset = %d\n", offset,indirect_index,indirect_offset);
        return (int)indirect[indirect_offset * 4];
    }
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    if(numSectors < 6){
        for (i = k = 0; i < numSectors; i++) {
            printf("i=%d sector=%d\n", i,dataSectors[i]);
            synchDisk->ReadSector(dataSectors[i], data);
            for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
                if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
                    printf("%c", data[j]);
                else
                    printf("\\%x", (unsigned char)data[j]);
        }
        }
    }
    else{
        // 先输出直接索引的内容
        for(i=k=0;i<6;i++){
            printf("i=%d sector=%d\n", i,dataSectors[i]);
            synchDisk->ReadSector(dataSectors[i],data);
            for(j=0;(j < SectorSize) && (k < numBytes);j++,k++){
                if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
                printf("%c", data[j]);
            else
                printf("\\%x", (unsigned char)data[j]);
            }
        }
        printf("use indirect index\n");
        // 找到二级索引表
        int cnt = 6;
        while(i < numSectors){
            printf("indirect_cnt=%d indirect_index=%d\n", cnt,dataSectors[cnt]);
            char *indirect = new char[SectorSize];
            synchDisk->ReadSector(dataSectors[cnt],indirect);
            for(int indirect_num=0;indirect_num<32 && i <numSectors;indirect_num++,i++){
                printf("i=%d indirect_num=%d sector=%d\n",i,indirect_num,indirect[indirect_num*4]);
                synchDisk->ReadSector((int)indirect[indirect_num*4],data);
                for(j=0;(j < SectorSize) && (k < numBytes);j++,k++){
                    if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
                        printf("%c", data[j]);
                    else
                        printf("\\%x", (unsigned char)data[j]);
                }
            }
            cnt++;
        }
    }
    printf("\n"); 
 //    for (i = 0; i < numSectors; i++)
	// printf("%d ", dataSectors[i]);
 //    printf("\nFile contents:\n");
 //    for (i = k = 0; i < numSectors; i++) {
	// synchDisk->ReadSector(dataSectors[i], data);
 //        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	//     if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
	// 	printf("%c", data[j]);
 //            else
	// 	printf("\\%x", (unsigned char)data[j]);
	// }
 //        printf("\n"); 
 //    }
    delete [] data;
}
void FileHeader::set_ctime(){
    time_t timep;
    time(&timep);
    strncpy(ctime,asctime(gmtime(&timep)),25);
    ctime[24] = '\0';
    // printf("ctime is %s\n", ctime);
}
void FileHeader::set_last_vtime(){
    time_t timep;
    time(&timep);
    strncpy(last_vtime,asctime(gmtime(&timep)),25);
    last_vtime[24] = '\0';
    // printf("last_vtime is %s\n", last_vtime);
}
void FileHeader::set_last_mtime(){
    time_t timep;
    time(&timep);
    strncpy(last_mtime,asctime(gmtime(&timep)),25);
    last_mtime[24]='\0';
    // printf("last_mtime is %s\n", last_mtime);
}

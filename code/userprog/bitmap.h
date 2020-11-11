// bitmap.h 
//	Data structures defining a bitmap -- an array of bits each of which
//	can be either on or off.
//
//	Represented as an array of unsigned integers, on which we do
//	modulo arithmetic to find the bit we are interested in.
//
//	The bitmap can be parameterized with with the number of bits being 
//	managed.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef BITMAP_H
#define BITMAP_H

#include "copyright.h"
#include "utility.h"
#include "openfile.h"

// Definitions helpful for representing a bitmap as an array of integers
#define BitsInByte 	8
#define BitsInWord 	32

// The following class defines a "bitmap" -- an array of bits,
// each of which can be independently set, cleared, and tested.
//
// Most useful for managing the allocation of the elements of an array --
// for instance, disk sectors, or main memory pages.
// Each bit represents whether the corresponding sector or page is
// in use or free.

class BitMap {
  public:
    BitMap(int nitems);		// Initialize a bitmap, with "nitems" bits
				// initially, all bits are cleared.
    ~BitMap();			// De-allocate bitmap
    
    // 标志which位被占用
    void Mark(int which);   	// Set the "nth" bit
    // 清楚which位
    void Clear(int which);  	// Clear the "nth" bit
    // 测试which位是否被占用，返回true表示被占用
    bool Test(int which);   	// Is the "nth" bit set?
    // 找到第一个未被占用的位，并标志为占用；返回-1表示未找到
    int Find();            	// Return the # of a clear bit, and as a side
				// effect, set the bit. 
				// If no bits are clear, return -1.
    // 返回多少位没有被占用
    int NumClear();		// Return the number of clear bits

    // 打印整个位图
    void Print();		// Print contents of bitmap
    
    // These aren't needed until FILESYS, when we will need to read and 
    // write the bitmap to a file
    void FetchFrom(OpenFile *file); 	// fetch contents from disk 
    void WriteBack(OpenFile *file); 	// write contents to disk

  private:
    int numBits;			// number of bits in the bitmap
    int numWords;			// number of words of bitmap storage
					// (rounded up if numBits is not a
					//  multiple of the number of bits in
					//  a word)
    unsigned int *map;			// bit storage
};

#endif // BITMAP_H

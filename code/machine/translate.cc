// translate.cc 
//	Routines to translate virtual addresses to physical addresses.
//	Software sets up a table of legal translations.  We look up
//	in the table on every memory reference to find the true physical
//	memory location.
//
// Two types of translation are supported here.
//
//	Linear page table -- the virtual page # is used as an index
//	into the table, to find the physical page #.
//
//	Translation lookaside buffer -- associative lookup in the table
//	to find an entry with the same virtual page #.  If found,
//	this entry is used for the translation.
//	If not, it traps to software with an exception. 
//
//	In practice, the TLB is much smaller than the amount of physical
//	memory (16 entries is common on a machine that has 1000's of
//	pages).  Thus, there must also be a backup translation scheme
//	(such as page tables), but the hardware doesn't need to know
//	anything at all about that.
//
//	Note that the contents of the TLB are specific to an address space.
//	If the address space changes, so does the contents of the TLB!
//
// DO NOT CHANGE -- part of the machine emulation
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "machine.h"
#include "addrspace.h"
#include "system.h"

// Routines for converting Words and Short Words to and from the
// simulated machine's format of little endian.  These end up
// being NOPs when the host machine is also little endian (DEC and Intel).

unsigned int
WordToHost(unsigned int word) {
#ifdef HOST_IS_BIG_ENDIAN
	 register unsigned long result;
	 result = (word >> 24) & 0x000000ff;
	 result |= (word >> 8) & 0x0000ff00;
	 result |= (word << 8) & 0x00ff0000;
	 result |= (word << 24) & 0xff000000;
	 return result;
#else 
	 return word;
#endif /* HOST_IS_BIG_ENDIAN */
}

unsigned short
ShortToHost(unsigned short shortword) {
#ifdef HOST_IS_BIG_ENDIAN
	 register unsigned short result;
	 result = (shortword << 8) & 0xff00;
	 result |= (shortword >> 8) & 0x00ff;
	 return result;
#else 
	 return shortword;
#endif /* HOST_IS_BIG_ENDIAN */
}

unsigned int
WordToMachine(unsigned int word) { return WordToHost(word); }

unsigned short
ShortToMachine(unsigned short shortword) { return ShortToHost(shortword); }


//----------------------------------------------------------------------
// Machine::ReadMem
//      Read "size" (1, 2, or 4) bytes of virtual memory at "addr" into 
//	the location pointed to by "value".
//
//   	Returns FALSE if the translation step from virtual to physical memory
//   	failed.
//
//	"addr" -- the virtual address to read from
//	"size" -- the number of bytes to read (1, 2, or 4)
//	"value" -- the place to write the result
//----------------------------------------------------------------------
// machine->ReadMem(registers[PCReg], 4, &raw)
bool
Machine::ReadMem(int addr, int size, int *value)
{
    int data;
    ExceptionType exception;
    int physicalAddress;
    
    DEBUG('a', "Reading VA 0x%x, size %d\n", addr, size);
    
    exception = Translate(addr, &physicalAddress, size, FALSE);
    // printf("VA=%d PA=%d\n", addr, physicalAddress);
    if (exception != NoException) {
	   	machine->RaiseException(exception, addr);
	   	return FALSE;
    }
    switch (size) {
      case 1:
		data = machine->mainMemory[physicalAddress];
		*value = data;
		break;
	
      case 2:
		data = *(unsigned short *) &machine->mainMemory[physicalAddress];
		*value = ShortToHost(data);
		break;
	
      case 4:
		data = *(unsigned int *) &machine->mainMemory[physicalAddress];
		*value = WordToHost(data);
		break;

      default: ASSERT(FALSE);
    }
    
    // printf("raw data = %d value=%d\n", data,*value);
    DEBUG('a', "\tvalue read = %8.8x\n", *value);
    return (TRUE);
}

//----------------------------------------------------------------------
// Machine::WriteMem
//      Write "size" (1, 2, or 4) bytes of the contents of "value" into
//	virtual memory at location "addr".
//
//   	Returns FALSE if the translation step from virtual to physical memory
//   	failed.
//
//	"addr" -- the virtual address to write to
//	"size" -- the number of bytes to be written (1, 2, or 4)
//	"value" -- the data to be written
//----------------------------------------------------------------------

bool
Machine::WriteMem(int addr, int size, int value)
{
    ExceptionType exception;
    int physicalAddress;
     
    DEBUG('a', "Writing VA 0x%x, size %d, value 0x%x\n", addr, size, value);

    exception = Translate(addr, &physicalAddress, size, TRUE);
    if (exception != NoException) {
	machine->RaiseException(exception, addr);
	return FALSE;
    }
    switch (size) {
      case 1:
	machine->mainMemory[physicalAddress] = (unsigned char) (value & 0xff);
	break;

      case 2:
	*(unsigned short *) &machine->mainMemory[physicalAddress]
		= ShortToMachine((unsigned short) (value & 0xffff));
	break;
      
      case 4:
	*(unsigned int *) &machine->mainMemory[physicalAddress]
		= WordToMachine((unsigned int) value);
	break;
	
      default: ASSERT(FALSE);
    }
    
    return TRUE;
}

//----------------------------------------------------------------------
// Machine::Translate
// 	Translate a virtual address into a physical address, using 
//	either a page table or a TLB.  Check for alignment and all sorts 
//	of other errors, and if everything is ok, set the use/dirty bits in 
//	the translation table entry, and store the translated physical 
//	address in "physAddr".  If there was an error, returns the type
//	of the exception.
//
//	"virtAddr" -- the virtual address to translate
//	"physAddr" -- the place to store the physical address
//	"size" -- the amount of memory being read or written
// 	"writing" -- if TRUE, check the "read-only" bit in the TLB
//----------------------------------------------------------------------

ExceptionType
Machine::Translate(int virtAddr, int* physAddr, int size, bool writing)
{
    int i;
    unsigned int vpn, offset;
    TranslationEntry *entry;
    unsigned int pageFrame;

    DEBUG('a', "\tTranslate 0x%x, %s: ", virtAddr, writing ? "write" : "read");

	// check for alignment errors
    if (((size == 4) && (virtAddr & 0x3)) || ((size == 2) && (virtAddr & 0x1))){
    	DEBUG('a', "alignment problem at %d, size %d!\n", virtAddr, size);
    	return AddressErrorException;
    }
    
    // we must have either a TLB or a page table, but not both!
    // ASSERT(tlb == NULL || pageTable == NULL);	
    ASSERT(tlb != NULL || pageTable != NULL);


	// calculate the virtual page number, and offset within the page,
	// from the virtual address
    vpn = (unsigned) virtAddr / PageSize;
    offset = (unsigned) virtAddr % PageSize;
    // printf("virtAddr = %d vpn = %d offset = %d\n", virtAddr,vpn,offset);
    for (entry = NULL, i = 0; i < TLBSize; i++){
    	if (tlb[i].valid && (tlb[i].virtualPage == vpn) && (tlb[i].pid == currentThread->getPid())) {
			entry = &tlb[i];			// FOUND!
			break;
	 	}
    }
	    
	if (entry == NULL) {				// not found
    	    DEBUG('a', "*** no valid TLB entry found for this virtual page!\n");
    	    // printf("*** no valid TLB entry found for this virtual page!\n");
    	    // printf("pid = %d vpn=%d\n", currentThread->getPid(),vpn);
    	    return PageFaultException;		// really, this is a TLB fault,
						// the page may be in memory,
						// but not in the TLB
	}
	// else 就是找到了
    if (entry->readOnly && writing) {	// trying to write to a read-only page
		DEBUG('a', "%d mapped read-only at %d in TLB!\n", virtAddr, i);
		return ReadOnlyException;
    }
    pageFrame = entry->physicalPage;
    // if the pageFrame is too big, there is something really wrong! 
    // An invalid translation was loaded into the page table or TLB. 
    if (pageFrame >= NumPhysPages) { 
	DEBUG('a', "*** frame %d > %d!\n", pageFrame, NumPhysPages);
	return BusErrorException;
    }
    entry->use = TRUE;		// set the use, dirty bits
    if (writing)
	   entry->dirty = TRUE;
	entry->visitTime = stats->totalTicks;
	// printf("find the page in tlb,renew the visitTime = %d\n",entry->visitTime);
    *physAddr = pageFrame * PageSize + offset;
    // printf("pageFrame = %d offset=%d vpn = %d\n", pageFrame,offset,vpn);
    ASSERT((*physAddr >= 0) && ((*physAddr + size) <= MemorySize));
    DEBUG('a', "phys addr = 0x%x\n", *physAddr);
    return NoException;
}

// 这个方法
// 1. 查找页表
// 2. 更新快表
// 3. FIFO需要记录来到的次序
ExceptionType Machine::FIFOSwap(int virtAddr){
	
	// printf("now in FIFOSwap\n");
	ASSERT(pageTable != NULL);
	int i;
    unsigned int vpn, offset;
    TranslationEntry *entry;

    vpn = (unsigned) virtAddr / PageSize;
    offset = (unsigned) virtAddr % PageSize;

    // 1. 找页表pageTable
    if (vpn >= pageTableSize) {
	    DEBUG('a', "virtual page # %d too large for page table size %d!\n", 
			virtAddr, pageTableSize);
	    return AddressErrorException;
	} else if (!pageTable[vpn].valid) {
	    DEBUG('a', "virtual page # %d too large for page table size %d!\n", 
			virtAddr, pageTableSize);
	    return PageFaultException;
	}
	
	
	entry = &pageTable[vpn];

	// 2. 更新tlb
	// 2.1 优先找无效的页进行覆盖
	int index = -1,min = tlb[0].createTime;
	for(i=0;i < TLBSize;i++){
		if(tlb[i].valid == FALSE){
			index = i;
			break;
		}
	}
	if(index == -1){
		// 2.2 当前tlb全部都有效，找createTIME最小的进行替换
		for(i=0;i < TLBSize;i++){
			// printf("i=%d min=%d tlb.createTime=%d\n", i,min,tlb[i].createTime);
			if(tlb[i].createTime <= min){
				min = tlb[i].createTime;
				index = i;
			}
		}
	}
	// else 找到了无效的页，可以直接进行替换
	
	tlb[index] = *entry;
	// 2.3 更新来到时间
	tlb[index].createTime = stats->totalTicks;
	// printf("index %d page will be coverd,new virtualPage=%d createTime=%d\n", index,tlb[index].virtualPage,tlb[index].createTime);
	

	return NoException;
}


ExceptionType Machine::LRUSwap(int virtAddr){
	// printf("now started PageFaultException handler\n");
	ASSERT(pageTable != NULL);
	int i;
    unsigned int vpn, offset;
    TranslationEntry *entry;

    vpn = (unsigned) virtAddr / PageSize;
    offset = (unsigned) virtAddr % PageSize;

    // 1. 找页表pageTable
    if (vpn >= pageTableSize) {
	    DEBUG('a', "virtual page # %d too large for page table size %d!\n", 
			virtAddr, pageTableSize);
	    return AddressErrorException;
	} else if (!pageTable[vpn].valid || (pageTable[vpn].pid != currentThread->getPid())) {
	    // 不在页表里，去内存里拿
		// 1. 分配物理页框
		// 这里还要考虑分配失败的情况，如果失败的话要置换页面
		// printf("not hint the pageTable\n");
		int pn = bitmap->Find();
		int index = vpn;
		if(pn == -1){
			// 分配失败
			int min = pageTable[0].visitTime;
			// 找拥有物理页框的页面进行覆盖
			for(int i = 0;i < pageTableSize;i++){
				if(pageTable[i].physicalPage != -1){
					index = i;
				}
			}
			// printf("index = %d pageTable[index].physicalPage=%d\n", index,pageTable[index].physicalPage);
			pageTable[index].valid = FALSE;
			pn = pageTable[index].physicalPage;
			pageTable[index].physicalPage = -1;
		}
	
		// printf("successfully to allocate the bitmap,the pn is %d,va is %d pid is %d\n",pn,vpn,currentThread->getPid());
		// 2. 读取一页的内容进来
		OpenFile *openfile = fileSystem->Open("virtual_memory");
		openfile->ReadAt(&(machine->mainMemory[pn * PageSize]),PageSize,vpn * PageSize);
		// 3. 找一个页表项进行刷新
		pageTable[vpn].physicalPage = pn;
		pageTable[vpn].pid = currentThread->getPid();
		pageTable[vpn].valid = TRUE;
		pageTable[vpn].visitTime = stats->totalTicks;
		// printf("vpn=%d pa=%d machine->pa=%d\n", vpn,pn,machine->pageTable[vpn].physicalPage);
	}
	else{
		// printf("hint the pageTable\n");
	}
	
	// 原来是直接使用，假设已经存在；现在要判断一下
	entry = &pageTable[vpn];

	// 2. 更新tlb
	// 2.1 优先找无效的页进行覆盖
	int index = -1,min = tlb[0].visitTime;
	for(i=0;i < TLBSize;i++){
		if(tlb[i].valid == FALSE){
			index = i;
			break;
		}
	}
	if(index == -1){
		// 2.2 当前tlb全部都有效，找visitTIME最小的进行替换
		for(i=0;i < TLBSize;i++){
			// printf("i=%d min=%d tlb.visitTime=%d\n", i,min,tlb[i].visitTime);
			if(tlb[i].visitTime <= min){
				min = tlb[i].visitTime;
				index = i;
			}
		}
	}
	// else 找到了无效的页，可以直接进行替换
	
	// 2.3 更新来到时间
	entry->visitTime = stats->totalTicks;
	tlb[index] = *entry;
	// printf("tlb has been renewed,pa=%d va=%d pid=%d\n",tlb[index].physicalPage,tlb[index].virtualPage,tlb[index].pid);
	return NoException;
}
void Machine::UserProgClear(){
	// TranslationEntry *pageTable;
    // unsigned int pageTableSize;
    int pageTableSize = machine->pageTableSize;
    TranslationEntry *pageTable = machine->pageTable;
    int pid = currentThread->getPid();
    printf("pid:%d start to clear bitmap,and clear the pageTable\n",currentThread->getPid());
    // bool Remove(char *name);
    fileSystem->Remove("virtual_memory");
    for(int i = 0;i < pageTableSize;i++){
        // 清除位图标志
        int pn = pageTable[i].physicalPage;
        int temp_pid = pageTable[i].pid;
        // printf("i=%d pa=%d va=%d pid=%d\n", i,pageTable[i].physicalPage,pageTable[i].virtualPage,temp_pid);
        if(pn >= 0 && pid == temp_pid){
            // printf("clear the memory of bitmap is %d\n", pageTable[i].physicalPage);
            bitmap->Clear(pageTable[i].physicalPage);
            // 清除pageTable映射
            pageTable[i].virtualPage = i;
            pageTable[i].physicalPage = -1;
            pageTable[i].pid = -1;
            pageTable[i].valid = FALSE;
            pageTable[i].use = FALSE;
            pageTable[i].dirty = FALSE;
            pageTable[i].readOnly = FALSE;
            pageTable[i].createTime = 0;
            pageTable[i].visitTime = 0;
        }
        // else pn == -1   这种情况说明该页面长时间没被访问，页框被其他页面占用了，该页面就不在内存了，所以无需清除
    }
    
    // 
    for (int i = 0; i < TLBSize; i++){
         tlb[i].valid=false;
    }
}
// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#include "table.h"
#include "synch.h"

extern "C" { int bzero(char *, int); };

Table::Table(int s) : map(s), table(0), lock(0), size(s) {
    table = new void *[size];
    lock = new Lock("TableLock");
}

Table::~Table() {
    if (table) {
	delete table;
	table = 0;
    }
    if (lock) {
	delete lock;
	lock = 0;
    }
}

void *Table::Get(int i) {
    // Return the element associated with the given if, or 0 if
    // there is none.

    return (i >=0 && i < size && map.Test(i)) ? table[i] : 0;
}

int Table::Put(void *f) {
    // Put the element in the table and return the slot it used.  Use a
    // lock so 2 files don't get the same space.
    int i;	// to find the next slot

    lock->Acquire();
    i = map.Find();
    lock->Release();
    if ( i != -1)
	table[i] = f;
    return i;
}

void *Table::Remove(int i) {
    // Remove the element associated with identifier i from the table,
    // and return it.

    void *f =0;

    if ( i >= 0 && i < size ) {
	lock->Acquire();
	if ( map.Test(i) ) {
	    map.Clear(i);
	    f = table[i];
	    table[i] = 0;
	}
	lock->Release();
    }
    return f;
}

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	"executable" is the file containing the object code to load into memory
//
//      It's possible to fail to fully construct the address space for
//      several reasons, including being unable to allocate memory,
//      and being unable to read key parts of the executable.
//      Incompletely consretucted address spaces have the member
//      constructed set to false.
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *exec) : fileTable(MaxOpenFiles) {
    executable = exec;
    NoffHeader noffH;
    unsigned int i, size;

    // Don't allocate the input or output to disk files
    fileTable.Put(0);
    fileTable.Put(0);

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size ;
    
    baseDataSize = divRoundUp(size, PageSize) +  divRoundUp(UserStackSize,PageSize);//(old numPages w/1 stack) something to use publicly to find my stack
    //TESTING JACK REMOVED * 50 in front of divroundup below
    numPages = divRoundUp(size, PageSize) +  divRoundUp(UserStackSize,PageSize);
                                                // we need to increase the size
						// to leave room for the stack
    size = numPages * PageSize;

    //ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory
    
    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
// first, set up the translation 
    pageTable = new PTEntry[numPages + 50*8];
	int executableBound = divRoundUp(noffH.code.size + noffH.initData.size, PageSize);
	DEBUG('a', "addrspace constructor:: executableBound(divRoundUp(code.size+initData.size,PageSize)):%i \n", executableBound);
    for (i = 0; i < numPages +50*8; i++) {
      /*
	  int ppn = freePageBitMap->Find();
      //printf("PPN: %d\n", ppn);
      if(ppn < 0){
        printf("BitMap Find returned <0. OUT OF MEMORY/n");
		interrupt->Halt();
      }
	*/
		pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
		pageTable[i].physicalPage = -1;
		pageTable[i].valid = TRUE;
		pageTable[i].use = FALSE;
		pageTable[i].dirty = FALSE;
		pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
						// a separate page, we could set its 
						// pages to be read-only
		pageTable[i].byteOffset = 40 + (i*PageSize);//location of VP in executable
		if (i < executableBound){
			pageTable[i].diskLocation = 0; //0=executable;
		} else {
			pageTable[i].diskLocation = 2; //2=neither swap nor executable
		}
		/*
		//Populate IPT: don't set ppn yet
		IPT[i].virtualPage = i;	
		//IPT[i].physicalPage = ppn;
		IPT[i].valid = FALSE;
		IPT[i].use = FALSE;
		IPT[i].dirty = FALSE;
		IPT[i].readOnly = FALSE;
		IPT[i].owner = this; //set owner to this addrspace. is this necessary not in constructor?
		*/
		//executable->ReadAt(&(machine->mainMemory[ppn*PageSize]) , PageSize , 40 + (i*PageSize));
    }
    
// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
/*    CROWLEY SAYS COMMENT OUT. load in page by page...
bzero(machine->mainMemory, size);

// then, copy in the code and data segments into memory
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
			noffH.code.virtualAddr, noffH.code.size);
        executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),
			noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
			noffH.initData.virtualAddr, noffH.initData.size);
        executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),
			noffH.initData.size, noffH.initData.inFileAddr);
    }
*/
}

//------
//made by jack to update pagetable with new thread stack
//------
int AddrSpace::CreateStack(int thread)
{
  int stackLoc;
  //set up 8 pages for this new thread's stack
  int ppn;
  /*
  numPages += 8;
  PTEntry* temp = new PTEntry[numPages];
  for (int i=0;i<numPages; i++){
	  if (i < numPages - 8){
		  temp[i].virtualPage = pageTable[i].virtualPage;
		  temp[i].physicalPage = pageTable[i].physicalPage;
		  temp[i].valid =  pageTable[i].valid;
		  temp[i].dirty =  pageTable[i].dirty;
		  temp[i].use =  pageTable[i].use;
		  temp[i].readOnly =  pageTable[i].readOnly;
		  temp[i].byteOffset = pageTable[i].byteOffset;
		  temp[i].diskLocation = pageTable[i].diskLocation;
	  } else {
		temp[i].virtualPage = i;	// for now, virtual page # = phys page #
		//pageTable[i].physicalPage = ppn;
		temp[i].valid = TRUE;
		temp[i].use = FALSE;
		temp[i].dirty = FALSE;
		temp[i].readOnly = FALSE;
		temp[i].byteOffset = 40 + (i*PageSize);//location of VP in executable
		temp[i].diskLocation = 2; //2=neither swap nor executable
	  }
  }
	  pageTable = temp;
*/
	  for(int i = 0; i<8; i++)
		{
			/*
		  ppn = freePageBitMap->Find();
		  if(ppn < 0)
		{
		  printf("CreateStack::Bitmapfind returned out of memory\n");
		  interrupt->Halt();
		}
		*/
		  pageTable[thread].virtualPage = thread;
		  //pageTable[thread].physicalPage = ppn;
		  pageTable[thread].valid = TRUE;
		  pageTable[thread].use = FALSE;
		  pageTable[thread].dirty = FALSE;
		  pageTable[thread].readOnly = FALSE;
		/*
			//populate IPT because of Find()
			IPT[ppn].virtualPage = thread;	
			IPT[ppn].physicalPage = ppn;
			IPT[ppn].valid = TRUE;
			IPT[ppn].use = FALSE;
			IPT[ppn].dirty = FALSE;
			IPT[ppn].readOnly = FALSE;
			IPT[ppn].owner = this;
		*/
		  thread++;
		}
  stackLoc = (PageSize * thread) -16;

  return stackLoc;
}

//-------
//made by Jack, deallocates stack from pageTable
//-------
void AddrSpace::DestroyStack(int thread)
{
  for(int i = 0; i <8; i++)
    {
		  thread--;

/*
		for (int j = 0; j<numPages+50*8;j++){	
			printf("PT[j].vpn:%i, PT[j].ppn:%i, PT[j].valid:%i\n", pageTable[j].virtualPage, pageTable[j].physicalPage, pageTable[j].valid);
			if (pageTable[j].virtualPage == thread ){
						printf("HIT::PT[thread].vpn:%i, PT[thread].ppn:%i, PT[thread].valid:%i\n", pageTable[j].virtualPage, pageTable[j].physicalPage, pageTable[j].valid);
			}
		}
*/
		int ppn = pageTable[thread].physicalPage;
/*
		DEBUG('p',"PT[thread].vpn:%i, PT[thread].ppn:%i, PT[thread].valid:%i\n", pageTable[thread].virtualPage, pageTable[thread].physicalPage, pageTable[thread].valid);
		DEBUG('p',"IPT[pt].vpn:%i, IPT[pt].ppn:%i, IPT[pt].valid:%i\n", IPT[ppn].virtualPage, IPT[ppn].physicalPage, IPT[ppn].valid);
*/
      if(pageTable[thread].valid)
		{
		  pageTable[thread].valid = FALSE;
		}
		if (IPT[ppn].valid)
		{
			DEBUG('p', "clearing used stack ppn:%i\n", ppn);
			IPT[thread].valid = FALSE;//is this supposed to be there?
			freePageBitMap->Clear(ppn);
		}
    }
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
//
// 	Dealloate an address space.  release pages, page tables, files
// 	and file tables
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg,  baseDataSize*PageSize - 16);//changed to not use numPages by JACK// moved to old numPages by Ian
    DEBUG('a', "Initializing stack register to %x\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{
    IntStatus old = interrupt->SetLevel(IntOff);
    for(int i = 0; i < TLBSize; i ++) {
        if(machine->tlb[i].valid){
			IPT[machine->tlb[i].physicalPage].dirty = machine->tlb[i].dirty;
		}
        machine->tlb[i].valid = false;
    }
    (void) interrupt->SetLevel(old);
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    //machine->pageTable = pageTable;
   // machine->pageTableSize = numPages;
}

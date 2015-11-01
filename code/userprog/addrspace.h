// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include "table.h"

#define UserStackSize		1024 	// increase this as necessary!

#define MaxOpenFiles 256
#define MaxChildSpaces 256

class AddrSpace {
  public:
    AddrSpace(OpenFile *executable);	// Create an address space,
					// initializing it with the program
					// stored in the file "executable"
    ~AddrSpace();			// De-allocate an address space

    void InitRegisters();		// Initialize user-level CPU registers,
					// before jumping to user code

    void SaveState();			// Save/restore address space-specific
    void RestoreState();		// info on a context switch
    Table fileTable;			// Table of openfiles
    int getID() {return processID;}
    void setID(int nID){processID = nID;}
    int getNumPages(){return numPages;}
    void setNumPages(unsigned int np){numPages = np;} 
    int getBaseDataSize(){return baseDataSize;}

    /*take in last vpn of new thread, spit out stack mem end*/
    int CreateStack(int threadNum);

    /*take in last vpn of new thread, clear out stack*/
    void DestroyStack(int threadNum);


    TranslationEntry *pageTable;	// Assume linear page table translation
					// for now!
    IPTEntry *IPT;
    unsigned int numPages;		// Number of pages in the virtual 
					// address space
    int processID;  //added by JACK to use as key in processTable

    int baseDataSize;
};


#endif // ADDRSPACE_H

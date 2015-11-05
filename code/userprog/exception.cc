// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "synch.h"
#include <stdio.h>
#include <iostream>
#include <sstream>

using namespace std;

const int TABLE_SIZE = 200;//shoud there be a max size?

struct KernelCondition{
  KernelCondition(Condition* c, AddrSpace* a);
  Condition* cv;
  AddrSpace* addrSpace;
  bool isToBeDeleted;
};
struct KernelLock{
	KernelLock(Lock* l, AddrSpace* a);
	Lock* lock;
	AddrSpace* addrSpace;
  	bool isToBeDeleted;
};
KernelLock::KernelLock(Lock* l, AddrSpace* a)
{
	lock = l;
	addrSpace = a;
	isToBeDeleted = false;
}
KernelCondition::KernelCondition(Condition* c, AddrSpace* a)
{
  cv = c;
  addrSpace = a;
  isToBeDeleted = false;
}

KernelCondition* ConditionTable [TABLE_SIZE];
KernelLock* LockTable [TABLE_SIZE];
int lockCounter = 0; // is this necessary to keep track of the lock?
int conditionCounter = 0; //index of the lowest free index of ConditionTable

int threadCounter = 1;//used to assign ID to threads

//process table
int processCounter = 1;//TODO fix this. it starts at zero in progtest, so it is 1 now
struct Process{
  AddrSpace* addrSpace;
  int numThreads;//number of threads in this process
  //more?
  int threadStackStart[50];
  Process(AddrSpace* a, int nt);
  ~Process();
  Process(int nt);
};
Process::Process(int nt)
{
  numThreads = nt;
}
Process::Process(AddrSpace* a, int nt)
{
  addrSpace = a;
  numThreads = nt;
//map thread id's to stack location?
}


Process* ProcessTable[TABLE_SIZE];
Process* mainProcess = new Process(1);
//ProcessTable[0] = new Process(1);//the main thread
int numProcesses = 1;
bool mainThreadFinished = FALSE;
Lock* ProcessLock = new Lock("ProcessLock");//the lock for ProcessTable

int copyin(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes from the current thread's virtual address vaddr.
    // Return the number of bytes so read, or -1 if an error occors.
    // Errors can generally mean a bad virtual address was passed in.
    bool result;
    int n=0;			// The number of bytes copied in
    int *paddr = new int;

    while ( n >= 0 && n < len) {
      result = machine->ReadMem( vaddr, 1, paddr );
      while(!result) // FALL 09 CHANGES
	  {
   			result = machine->ReadMem( vaddr, 1, paddr ); // FALL 09 CHANGES: TO HANDLE PAGE FAULT IN THE ReadMem SYS CALL
	  }	
      
      buf[n++] = *paddr;
     
      if ( !result ) {
	//translation failed
	return -1;
      }

      vaddr++;
    }

    delete paddr;
    return len;
}

int copyout(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes to the current thread's virtual address vaddr.
    // Return the number of bytes so written, or -1 if an error
    // occors.  Errors can generally mean a bad virtual address was
    // passed in.
    bool result;
    int n=0;			// The number of bytes copied in

    while ( n >= 0 && n < len) {
      // Note that we check every byte's address
      result = machine->WriteMem( vaddr, 1, (int)(buf[n++]) );

      if ( !result ) {
	//translation failed
	return -1;
      }

      vaddr++;
    }

    return n;
}

void sendMsgToServer(char* msg)
{
  PacketHeader outPktHdr;
    MailHeader outMailHdr;

    // construct packet, mail header for original message
    // To: destination machine, mailbox 0
    // From: our machine, reply to: mailbox 1
    outPktHdr.to = 0;//TODO hard testing		
    outMailHdr.to = 0;
    outMailHdr.from = 1;
    outMailHdr.length = strlen(msg) + 1;

    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, msg); 

}


//use nachos thread::fork to get here from Fork
void Kernel_Thread(int func)
{
  // printf("ABOUT TO ACQUIRE LOCK: %s\n", currentThread->getName());
 ProcessLock->Acquire(); 
 //printf("ACQUIRED LOCK: %s\n", currentThread->getName());
 //printf("KERNELTHREAD\n");
  //set up my registers
  currentThread->space->InitRegisters();//zero out
  machine->WriteRegister(PCReg, func);
  machine->WriteRegister(NextPCReg, func + 4);
  //TODO optimize the follwoing line. lots of arrows...
  int currentProcess = currentThread->space->getID();
  int thisThread = currentThread->getID();
  //printf("CREATESTACK");
  int stackLoc = currentThread->space->CreateStack(ProcessTable[currentProcess]->threadStackStart[thisThread]);

  printf("STACK LOCATION: %d Stack page: %d\n", stackLoc, divRoundUp(stackLoc, PageSize)); 
  /* 
 // int currentProcess = currentThread->space->getID();
 // int stackLoc = ProcessTable[currentProcess]->threadStackStart[currentThread->getID()];
  */

  machine->WriteRegister(StackReg, stackLoc);  
  currentThread->space->RestoreState();
 ProcessLock->Release(); 

 machine->Run();//now, use the registers i set above and LIVE

}


/* Fork a thread to run a procedure ("func") in the *same* address space 
 * as the current thread.
 */
void Fork_Syscall(int func)//or should it be void (*func)()
{
  //printf("Calling Fork Syscall: %d\n",func);// freePageBitMap->Find());
  //NOTE: Will need LOCK to lock down shared resources like stack and process table!

  Thread* nt = new Thread("forkedThread");
  int currentProcess = currentThread->space->getID();
 
  ProcessLock->Acquire();

  //next lines removed for main thread update
  if(!ProcessTable[currentProcess])
    {
      printf("FORKSYS: artificially making process %d", currentProcess);
      numProcesses++;
      //processCounter++;
      Process* proc = new Process(currentThread->space, 0);
      ProcessTable[currentProcess] = proc;
    }
 

 ProcessTable[currentProcess]->numThreads++;
  int threadID = ProcessTable[currentProcess]->numThreads;
  nt->setID(threadID);
  

  int nPages =  currentThread->space->getNumPages();
 ProcessTable[currentProcess]->threadStackStart[threadID] = nPages;
 currentThread->space->setNumPages(nPages+8);
 nt->space = currentThread->space;
 ProcessLock->Release();
  nt->Fork((VoidFunctionPtr)Kernel_Thread, func);//ready new thread to go to KT function

////
/*
 int threadID = ProcessTable[thisProcess]->numThreads;///fix v1
  threadCounter++;
  nt->setID(threadID);
  if(!ProcessTable[currentProcess])
    {
      numProcesses++;
      Process* proc = new Process(currentThread->space, 0);
      ProcessTable[currentProcess] = proc;
    }
  ProcessTable[currentProcess]->numThreads++;
  ProcessTable[currentProcess]->threadStackStart[nt->getID()] = currentThread->space->getBaseDataSize() + (UserStackSize * ProcessTable[currentProcess]->numThreads) - 16;

  nt->space = currentThread->space;
  ProcessLock->Release();
  nt->Fork((VoidFunctionPtr)Kernel_Thread, func);//ready new thread to go to KT function
*/
  return;  
}

/* Yield the CPU to another runnable thread, whether in this address space 
 * or not. 
 */
void Yield_Syscall()
{
	//TODO
	DEBUG('a', "Yield\n");
	currentThread->Yield();
}

/*
 * Lock Syscalls
*/
// Creates a new Lock object in kernel space. Returns the kernel lock table index to the user program.
int CreateLock_Syscall(unsigned int vaddr, int len)
{
	char *buf = new char[len+1];	// Kernel buffer to put the name in
	ProcessLock->Acquire();
    if (!buf) 
      {
	printf("%s", "CreateLock::Can't allocate kernel buffer in CreateLock\n");
	ProcessLock->Release();
	return -1;
      }

    if( copyin(vaddr,len,buf) == -1 ) {
	printf("%s","CreateLock::Bad pointer passed to CreateLock\n");
	delete buf;
ProcessLock->Release();
	return -1;
    }
    buf[len]='\0';
#ifdef NETWORK
    //build message

  printf("Network CL in progress\n");

    PacketHeader inPktHdr;
    MailHeader inMailHdr;
    char buffer[MaxMailSize];

    stringstream ss;
    ss<<"CL "<<buf<< " ";
    char* msg = (char*)ss.str().c_str();

    sendMsgToServer(msg);

    postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
    printf("Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);

    int lockIndex = atoi(buffer);//convert char* to int
    
    printf("CreateLock:: Lock Index given %d\n", lockIndex);
    return lockIndex;
#else
    ProcessLock->Acquire();    
    printf("\nCreateLocK::NAME:%s \n", buf);

	Lock* newLock = new Lock(buf);
	KernelLock* kLock = new KernelLock(newLock, currentThread->space);
	int thisLock = lockCounter;
	LockTable[thisLock] = kLock;
	lockCounter++;
	ProcessLock->Release();
	return thisLock;
#endif /*NETWORK*/
}

// Takes an integer number as an argument, which is the table index of the lock to "acquire".
int Acquire_Syscall(int lockIndex)
{
  if(lockIndex < 0 || lockIndex >= TABLE_SIZE)
    {
      printf("%s\n", "Acquire::ERROR: Lock Index out of bounds");
      return -1;
    }
#ifdef NETWORK

  printf("Network AL in progress\n");

    PacketHeader inPktHdr;
    MailHeader inMailHdr;
    char buffer[MaxMailSize];

    stringstream ss;
    ss<<"AL "<<lockIndex;//TODO HERE JACK
    char* msg = (char*)ss.str().c_str();

    sendMsgToServer(msg);

    postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
    printf("Acquire::Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);

    // int cvIndex = atoi(buffer);//convert char* to int
    
    // printf("Acquire\n");
    return 1;
#else
  KernelLock* kl = LockTable[lockIndex];
if(!(kl))
    {
      printf("%s\n", "Acquire::ERROR: Lock is null");
      return -1;
    } 
 if(!(kl->lock))
    {
      printf("%s\n", "Acquire::ERROR: Lock is null");
      return -1;
    }
  //Lock exists and input was proper so far, check if lock belongs to this addrSpace
  if(kl->addrSpace != currentThread->space)
    {
      printf("%s\n", "Acquire::ERROR: Lock does not belong to this address space");
      return -1;
    }
  //all good to go, do acquire
  kl->lock->Acquire();
#endif /*NETWORK*/
  return 1;
}

// Takes an integer number as an argument - the lock table index of the lock to release.
int Release_Syscall(int lockIndex)
{
  if(lockIndex < 0 || lockIndex >= TABLE_SIZE)
    {
      printf("%s\n", "Release::ERROR: Lock Index out of bounds");
      return -1;
    }
  KernelLock* kl = LockTable[lockIndex];
if(!(kl))
    {
      printf("%s\n", "Release::ERROR: Lock is null");
      return -1;
    }  
if(!(kl->lock))
    {
      printf("%s\n", "Release::ERROR: Lock is null");
      return -1;
    }
  //does this lock belong to current address space?
  if(kl->addrSpace != currentThread->space)
    {
      printf("%s\n", "Release::ERROR: Lock does not belong to this address space");
      return -1;
    }

  //error checking done, do action
  kl->lock->Release();
  //check isToBeDeleted
  if(kl->lock->isLockWaitQueueEmpty() && kl->isToBeDeleted)
    {
      DEBUG('a', "Deleting Lock in release\n");
      delete kl->lock;
      delete kl;
    }
  return 1;
}

// Deletes a lock from the lock table using an interger argument, IF the lock is not in use. If the lock is in use, it is eventually deleted when the lock is no longer in use.
int DestroyLock_Syscall(int lockIndex)
{
   /**/
#ifdef NETWORK
  printf("Network DL in progress\n");

    PacketHeader inPktHdr;
    MailHeader inMailHdr;
    char buffer[MaxMailSize];

    stringstream ss;
    ss<<"DL "<<lockIndex;//TODO HERE JACK
    char* msg = (char*)ss.str().c_str();

    sendMsgToServer(msg);

    postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
    printf("Acquire::Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);

    // int cvIndex = atoi(buffer);//convert char* to int
    
    // printf("Destroy\n");
    return 1;
#else
	if (lockIndex <0 || lockIndex >= TABLE_SIZE){
		printf("DestroyLock::Error: Lock Index out of bounds\n");
		return -1;
	}
	KernelLock* kl = LockTable[lockIndex];
	if(!(kl))
	{
		printf("%s\n", "Destroy::ERROR: Lock is null");
		return -1;
	}
	if(!(kl->lock))
	{
		printf("%s\n", "Destroy::ERROR: Lock is null");
		return -1;
	}
	//does this lock belong to current address space?
	if(kl->addrSpace != currentThread->space)
	{
		printf("%s\n", "Destroy::ERROR: Lock does not belong to this address space");
		return -1;
	}
	if (kl->lock->isLockWaitQueueEmpty()) {
	  DEBUG('a', "Destroying Lock %i\n", lockIndex);
	  delete kl->lock;
	}
	else {
	  DEBUG('a', "Marking lock %i for deletion\n", lockIndex);
	  kl->isToBeDeleted = true;
	}
	return 1;
#endif/*NETWORK*/
  /**/
}

/*
 * Condition Syscalls
*/

// Creates a new Condition object in kernel space.
int CreateCondition_Syscall(unsigned int vaddr, int len)//TODO should pass in value
{
  ProcessLock->Acquire();
    char *buf = new char[len+1];	// Kernel buffer to put the name in

    if (!buf) 
      {
	printf("%s", "CreateCV::Can't allocate kernel buffer in CreateCondition\n");
	ProcessLock->Release();
	return -1;
      }

    if( copyin(vaddr,len,buf) == -1 ) {
	printf("%s","CreateCV::Bad pointer passed to CreateCondition\n");
	delete buf;
	ProcessLock->Release();

	return -1;
    }

    buf[len]='\0';
    printf("\nCreateCV::NAME:%s\n", buf);
#ifdef NETWORK

  printf("Network CCV in progress\n");

    PacketHeader inPktHdr;
    MailHeader inMailHdr;
    char buffer[MaxMailSize];

    stringstream ss;
    ss<<"CCV "<<buf;
    char* msg = (char*)ss.str().c_str();

    sendMsgToServer(msg);

    postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
    printf("Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);

    int cvIndex = atoi(buffer);//convert char* to int
    
    printf("CreateCV:: CV Index given %d\n", cvIndex);
    return cvIndex;
#else
 Condition* cv = new Condition(buf);
  //build KernelCondtion
  KernelCondition* newKC = new KernelCondition(cv, currentThread->space);

  int thisCV = conditionCounter;
  conditionCounter++;

  if(cv)
    {
      ConditionTable[thisCV] = newKC;
    }
  else
    {
      printf("%s\n", "CreatCondition::ERROR: ERROR, on creating cv?");
	ProcessLock->Release();

      return -1;
    }
  //printf("creating CV: %i\n", conditionCounter-1);
	ProcessLock->Release();

  return thisCV;//mehh
#endif/*NETWORK*/
}
// 
int DestroyCondition_Syscall(int conditionIndex)
{
	 /**/
	if (conditionIndex <0 || conditionIndex >= TABLE_SIZE){
		printf("DestroyCondition::Error: Condition Index out of bounds\n");
		return -1;
	}
	KernelCondition* kc = ConditionTable[conditionIndex];
	if(!(kc))
	  {
	    printf("%s\n", "DestroyCondition::Error: Kernel Condiiton is null");
		return -1;
	  }
	if (!(kc->cv)) {
		printf("%s\n", "DestroyCondition::Error: Kernel Condiiton is null");
		return -1;
	}
	//does this condition belong to current address space?
	if(kc->addrSpace != currentThread->space)
	{
		printf("%s\n", "DestroyCondition::ERROR: Lock does not belong to this address space");
		return -1;
	}
	if (kc->cv->isWaitQueueEmpty()) {
		DEBUG('a', "Deleting Condition %i\n", conditionIndex);
		delete kc->cv;
	} 
	else {
		DEBUG('a', "Marking cv for deletion\n");
		kc->isToBeDeleted = true;
	}
	return 1;
	/**/
}
// 
int Wait_Syscall(int lockIndex, int conditionIndex)
{
    if(lockIndex < 0 || lockIndex >= TABLE_SIZE)
    {
      printf("%s\n", "Wait::ERROR: Lock Index out of bounds");
      return -1;
    }
  KernelLock* kl = LockTable[lockIndex];
if(!(kl))
    {
      printf("%s\n", "Wait::ERROR: Lock is null");
      return -1;
    }  
if(!(kl->lock))
    {
      printf("%s\n", "Wait::ERROR: Lock is null");
      return -1;
    }
  //now, repeat for condition
  if(conditionIndex < 0 || conditionIndex >= TABLE_SIZE)
    {
      printf("%s\n","Wait::ERROR: Condition Index is out of bounds");
      return -1;
    }
  KernelCondition* kc = ConditionTable[conditionIndex];
 if(!(kc))
    {
      printf("%s\n","Wait::ERROR: Condition is null");
      return -1;
    }  
if(!(kc->cv))
    {
      printf("%s\n","Wait::ERROR: Condition is null");
      return -1;
    }

  //Ok, now we know lock and condition are both valid
  //check if this lock AND condition belong to this thread
  if(kl->addrSpace != currentThread->space || kc->addrSpace != currentThread->space)
    {
      printf("%s\n", "Wait::ERROR: lock or cv does not belong to this thread");
      return -1;
    }
  //We're all good!
  kc->cv->Wait(kl->lock);
  return 1;
}

// 
int Signal_Syscall(int lockIndex, int conditionIndex)
{
  if(lockIndex < 0 || lockIndex >= TABLE_SIZE)
    {
      printf("%s%d\n", "Signal::ERROR: Lock Index out of bounds:", lockIndex);
      return -1;
    }
  KernelLock* kl = LockTable[lockIndex];
  if(!(kl))
    {
      printf("%s\n", "Signal::ERROR: Lock is null");
      return -1;
    }
 if(!(kl->lock))
    {
      printf("%s\n", "Signal::ERROR: Lock is null");
      return -1;
    }
  //now, repeat for condition
  if(conditionIndex < 0 || conditionIndex >= TABLE_SIZE)
    {
      printf("%s\n","Signal::ERROR: Condition Index is out of bounds");
      return -1;
    }
  KernelCondition* kc = ConditionTable[conditionIndex];
 if(!(kc))
    {
      printf("%s\n","Signal::ERROR: Condition is null");
      return -1;
    }  
if(!(kc->cv))
    {
      printf("%s\n","Signal::ERROR: Condition is null");
      return -1;
    }

  //Ok, now we know lock and condition are both valid
  //check if this lock AND condition belong to this thread
  if(kl->addrSpace != currentThread->space || kc->addrSpace != currentThread->space)
    {
      printf("%s\n", "Signal::ERROR: lock or cv does not belong to this thread");
      return -1;
    }
  //We're all good!
  kc->cv->Signal(kl->lock);
  if(kc->cv->isWaitQueueEmpty() && kc->isToBeDeleted)//not totally sure about this...
    {
      delete kc->cv;
      delete kc;
      DEBUG('a', "Deleting CV after signalling\n");
    }
  return 1;
}

int Broadcast_Syscall(int lockIndex, int conditionIndex)
{
  if(lockIndex < 0 || lockIndex >= TABLE_SIZE)
    {
      printf("%s\n", "Broadcast::ERROR: Lock Index out of bounds");
      return -1;
    }
  KernelLock* kl = LockTable[lockIndex];
  if(!(kl))
    {
      printf("%s\n", "Broadcast::ERROR: Lock is null");
      return -1;
    }
if(!(kl->lock))
    {
      printf("%s\n", "Broadcast::ERROR: Lock is null");
      return -1;
    }
  //now, repeat for condition
  if(conditionIndex < 0 || conditionIndex >= TABLE_SIZE)
    {
      printf("%s\n","Broadcast::ERROR: Condition Index is out of bounds");
      return -1;
    }
  KernelCondition* kc = ConditionTable[conditionIndex];
  if(!(kc))
    {
      printf("%s\n","Broadcast::ERROR: Condition is null");
      return -1;
    }
 if(!(kc->cv))
    {
      printf("%s\n","Broadcast::ERROR: Condition is null");
      return -1;
    }

  //Ok, now we know lock and condition are both valid
  //check if this lock AND condition belong to this thread
  if(kl->addrSpace != currentThread->space || kc->addrSpace != currentThread->space)
    {
      printf("%s\n", "Broadcaste::ERROR: lock or cv does not belong to this thread");
      return -1;
    }
  //We're all good!
  kc->cv->Broadcast(kl->lock);
  if(kc->cv->isWaitQueueEmpty() && kc->isToBeDeleted)//not totally sure about this...
    {
      delete kc->cv;
      delete kc;
      DEBUG('a', "Deleting CV after Broadcasting\n");
    }
  return 1;
}

/*Print cstring from vaddr with option for cstring arguments 1 and 2. all args will only be read to the length parameter*/
void Print_Syscall(unsigned int vaddr, int len, unsigned int arg1, unsigned int arg2) {
    
    char *buf;		// Kernel buffer for output
    
    if ( !(buf = new char[len]) ) {
	printf("%s","Error allocating kernel buffer for write!\n");
	return;
    } else {
        if ( copyin(vaddr,len,buf) == -1 ) {
	    printf("%s","Bad pointer passed to to write: data not written\n");
	    delete[] buf;
	    return;
	}
    }  
    char *buf1;		// Kernel buffer for output
    
    if ( !(buf1 = new char[len]) ) {
	printf("%s","Error allocating kernel buffer for write!\n");
	return;
    }else {
        if ( copyin(arg1,len,buf1) == -1 ) {
	    printf("%s","Bad pointer passed to to write: data not written\n");
	    delete[] buf1;
	    return;
	}
    }
    char *buf2;		// Kernel buffer for output
    
    if ( !(buf2 = new char[len]) ) {
	printf("%s","Error allocating kernel buffer for write!\n");
	return;
    } else {
        if ( copyin(arg2,len,buf2) == -1 ) {
	    printf("%s","Bad pointer passed to to write: data not written\n");
	    delete[] buf2;
	    return;
	}
    }

    printf(buf, buf1, buf2);

    delete[] buf;
    delete[] buf1;
    delete[] buf2;
}

/*Print cstring from vaddr with option for int  arguments 1 and 2*/
void PrintInt_Syscall(unsigned int vaddr, int len, int arg1, int arg2) {
    
    char *buf;		// Kernel buffer for output
    
    if ( !(buf = new char[len]) ) {
	printf("%s","Error allocating kernel buffer for write!\n");
	return;
    } else {
        if ( copyin(vaddr,len,buf) == -1 ) {
	    printf("%s","Bad pointer passed to to write: data not written\n");
	    delete[] buf;
	    return;
	}
    }  
    printf(buf, arg1, arg2);

    delete[] buf;
}

int Rand_Syscall()
{
	return rand();
}

void Exec_Thread(){
	currentThread->space->InitRegisters();             // set the initial register values
    currentThread->space->RestoreState();              // load page table register
    machine->Run();                     // jump to the user progam
}
/* Exec syscall runs executable and returns int/SpaceId of addrSpace */
int Exec_Syscall(unsigned int vaddr, int len) 
{
  //printf("IN EXEC\n");
  if(vaddr < 0 || len < 0)
    {
      printf("Exec::ERROR: out of bounds input");
      return -1;
    }
	//Open file (
	 char *buf = new char[len+1];	// Kernel buffer to put the name in
    OpenFile *executable;			// The new open file
    int id;				// The openfile id
    
    if (!buf) {
		printf("%s","Can't allocate kernel buffer in Open\n");
		return -1;
    }
   
    if( copyin(vaddr,len,buf) == -1 ) {
		printf("%s","Bad pointer passed to Open\n");
		delete[] buf;
		return -1;
    }
   
    buf[len]='\0';
    executable = fileSystem->Open(buf);
    if ( executable == NULL ) {
      //if ((id = currentThread->space->fileTable.Put(executable)) != -1 ) {
      //	  printf("%s\n", "Exec::Error: fielTable problem");
      //	  return -1;
      //	}
      // } else {
       printf("Exec::Unable to open file %s\n", buf);
       return -1;
    }
    delete[] buf;

	// above tweaked from Open_Syscall. Now have OpenFile* executable
	AddrSpace* space = new AddrSpace(executable);
	Thread* thread = new Thread("exec thread");
	ProcessLock->Acquire();
	thread->setID(0);//first thread in this process
	
	
	space->setID(processCounter);
	thread->space = space;
	int spaceId = processCounter;
	//Update process table
	Process* process = new Process(space, 1); //process->addrSpace = space;	process->numThreads = 1;
	
	ProcessTable[processCounter] = process;
	ProcessTable[processCounter]->threadStackStart[0] = space->getNumPages();
		     processCounter++;
	numProcesses++;
	ProcessLock->Release();
	
	thread->Fork((VoidFunctionPtr)Exec_Thread,0);
	
	return spaceId;//machine->WriteRegister(2, space->getID()); done at end of exec	
}

void Exit_Syscall(int status){
	/*
	• reclaim resources: memory, locks, conditions
	• Must have currentThread->Finish();
	• Functions that call fork must call Exit
	1. A thread calls Exit - not the last executing thread
		a. Reclaim pages of stack (they're contiguous and in sets of 8)
	2. Last execution thread in last process
		a. Interrupt->Halt()
	3. Last execution thread in not last process
		a. Reclaim all unreclaimed memory:
			i. Vpn	Ppn	Valid bit
			ii. If (valid bit) { memoryBitmap->Clear(ppn); }
			iii. Valid bit = false
		b. Locks/cvs (Match addrspace* w/ processtable)

	*/
  // currentThread->Finish();
  ProcessLock->Acquire();
 
  int thisThread = currentThread->getID();
  int thisProcess = currentThread->space->getID();
  if(currentThread->getID() == -1) //if main thread, just exit
    {
      //printf("MAIN THREAD IS DEAD WOOHOO******\n");
      mainThreadFinished = true;
      numProcesses--;
      
      if(numProcesses == 0)
	{
	  ProcessLock->Release();//not necessary, but meh
	  printf("Exit::Main thread is last thread in program. Halting\n");
	  interrupt->Halt();
	  return;
	}
      ProcessLock->Release();
      currentThread->Finish();
      return;
      //      if(ProcessTable[currentThread->space->getID()])//TODO
      //	ProcessTable[currentThread->space->getID()]->numThreads--;
      // currentThread->Finish();
      // ProcessLock->Release();
      //return;
    }

  ProcessTable[thisProcess]->numThreads--;  

  if(ProcessTable[thisProcess]->numThreads == 0)
    {
      printf("Exit::kill a process\n");
      numProcesses--;
      //if this is last process running
      if(numProcesses == 0)
	{

	  printf("Exit:: Last Process in program, exiting: %s\n", currentThread->getName());
	  interrupt->Halt();
	  return; //successful end whole program
	}
      printf("Exit:: last thread in not-last process closing, deallocating process %d\n", thisProcess);
      //take care of locks and cv's 
      for(int i = 0; i < lockCounter; i++)
	{
	  if(LockTable[i]->addrSpace == currentThread->space)
	    {
	      delete LockTable[i]->lock;
	      // delete LockTable[i];
	    }
	}
      for(int i = 0; i < conditionCounter; i++)
	{
	  if(ConditionTable[i]->addrSpace == currentThread->space)
	    {
	      delete ConditionTable[i]->cv;
	      //delete ConditionTable[i];
	    }
	}

    }//end clean up for last thread in process


  // reclaim stack
  int stackLoc =   ProcessTable[currentThread->space->getID()]->threadStackStart[currentThread->getID()];
  printf("about to DESTROY STACK of thread: %s starting at: %d\n", currentThread->getName(), stackLoc);
  currentThread->space->DestroyStack(stackLoc); 

  printf("EXIT COMPLETE\n");
  ProcessLock->Release();
	currentThread->Finish();
}



void Create_Syscall(unsigned int vaddr, int len) {
    // Create the file with the name in the user buffer pointed to by
    // vaddr.  The file name is at most MAXFILENAME chars long.  No
    // way to return errors, though...
    char *buf = new char[len+1];	// Kernel buffer to put the name in

    if (!buf)

    if( copyin(vaddr,len,buf) == -1 ) {
	printf("%s","Bad pointer passed to Create\n");
	delete buf;
	return;
    }

    buf[len]='\0';

    fileSystem->Create(buf,0);
    delete[] buf;
    return;
}

int Open_Syscall(unsigned int vaddr, int len) {
    // Open the file with the name in the user buffer pointed to by
    // vaddr.  The file name is at most MAXFILENAME chars long.  If
    // the file is opened successfully, it is put in the address
    // space's file table and an id returned that can find the file
    // later.  If there are any errors, -1 is returned.
    char *buf = new char[len+1];	// Kernel buffer to put the name in
    OpenFile *f;			// The new open file
    int id;				// The openfile id

    if (!buf) {
	printf("%s","Can't allocate kernel buffer in Open\n");
	return -1;
    }

    if( copyin(vaddr,len,buf) == -1 ) {
	printf("%s","Bad pointer passed to Open\n");
	delete[] buf;
	return -1;
    }

    buf[len]='\0';

    f = fileSystem->Open(buf);
    delete[] buf;

    if ( f ) {
	if ((id = currentThread->space->fileTable.Put(f)) == -1 )
	    delete f;
	return id;
    }
    else
	return -1;
}

void Write_Syscall(unsigned int vaddr, int len, int id) {
    // Write the buffer to the given disk file.  If ConsoleOutput is
    // the fileID, data goes to the synchronized console instead.  If
    // a Write arrives for the synchronized Console, and no such
    // console exists, create one. For disk files, the file is looked
    // up in the current address space's open file table and used as
    // the target of the write.
    
    char *buf;		// Kernel buffer for output
    OpenFile *f;	// Open file for output

    if ( id == ConsoleInput) return;
    
    if ( !(buf = new char[len]) ) {
	printf("%s","Error allocating kernel buffer for write!\n");
	return;
    } else {
        if ( copyin(vaddr,len,buf) == -1 ) {
	    printf("%s","Bad pointer passed to to write: data not written\n");
	    delete[] buf;
	    return;
	}
    }

    if ( id == ConsoleOutput) {
      for (int ii=0; ii<len; ii++) {
	printf("%c",buf[ii]);
      }

    } else {
	if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
	    f->Write(buf, len);
	} else {
	    printf("%s","Bad OpenFileId passed to Write\n");
	    len = -1;
	}
    }

    delete[] buf;
}

int Read_Syscall(unsigned int vaddr, int len, int id) {
    // Write the buffer to the given disk file.  If ConsoleOutput is
    // the fileID, data goes to the synchronized console instead.  If
    // a Write arrives for the synchronized Console, and no such
    // console exists, create one.    We reuse len as the number of bytes
    // read, which is an unnessecary savings of space.
    char *buf;		// Kernel buffer for input
    OpenFile *f;	// Open file for output

    if ( id == ConsoleOutput) return -1;
    
    if ( !(buf = new char[len]) ) {
	printf("%s","Error allocating kernel buffer in Read\n");
	return -1;
    }

    if ( id == ConsoleInput) {
      //Reading from the keyboard
      scanf("%s", buf);

      if ( copyout(vaddr, len, buf) == -1 ) {
	printf("%s","Bad pointer passed to Read: data not copied\n");
      }
    } else {
	if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
	    len = f->Read(buf, len);
	    if ( len > 0 ) {
	        //Read something from the file. Put into user's address space
  	        if ( copyout(vaddr, len, buf) == -1 ) {
		    printf("%s","Bad pointer passed to Read: data not copied\n");
		}
	    }
	} else {
	    printf("%s","Bad OpenFileId passed to Read\n");
	    len = -1;
	}
    }

    delete[] buf;
    return len;
}

void Close_Syscall(int fd) {
    // Close the file associated with id fd.  No error reporting.
    OpenFile *f = (OpenFile *) currentThread->space->fileTable.Remove(fd);

    if ( f ) {
      delete f;
    } else {
      printf("%s","Tried to close an unopen file\n");
    }
}

void ExceptionHandler(ExceptionType which) {
    int type = machine->ReadRegister(2); // Which syscall?
    int rv=0; 	// the return value from a syscall

    if ( which == SyscallException ) {
	switch (type) {
	    default:
		DEBUG('a', "Unknown syscall - shutting down.\n");
	    case SC_Halt:
		DEBUG('a', "Shutdown, initiated by user program.\n");
		interrupt->Halt();
		break;
	    case SC_Create:
		DEBUG('a', "Create syscall.\n");
		Create_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;
	    case SC_Open:
		DEBUG('a', "Open syscall.\n");
		rv = Open_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;
	    case SC_Write:
		DEBUG('a', "Write syscall.\n");
		Write_Syscall(machine->ReadRegister(4),
			      machine->ReadRegister(5),
			      machine->ReadRegister(6));
		break;
	    case SC_Read:
		DEBUG('a', "Read syscall.\n");
		rv = Read_Syscall(machine->ReadRegister(4),
			      machine->ReadRegister(5),
			      machine->ReadRegister(6));
		break;
	    case SC_Close:
		DEBUG('a', "Close syscall.\n");
		Close_Syscall(machine->ReadRegister(4));
		break;
		
	    case SC_CreateCondition:
	        DEBUG('a', "Create Condition syscall. \n");
	        rv = CreateCondition_Syscall(machine->ReadRegister(4),
					     machine->ReadRegister(5));
		break;
	    case SC_CreateLock:
		DEBUG('a', "Create lock syscall. \n");
		rv = CreateLock_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
		break;
	    case SC_DestroyLock:
		DEBUG('a', "Destroy lock syscall. \n");
		rv = DestroyLock_Syscall(machine->ReadRegister(4));
		break;
	    case SC_DestroyCondition:
		DEBUG('a', "Destroy condition syscall. \n");
		rv = DestroyCondition_Syscall(machine->ReadRegister(4));
		break;
	    case SC_Acquire:
	        DEBUG('a', "Acquire Lock syscall. \n");
	        rv = Acquire_Syscall(machine->ReadRegister(4));
		break;
	    case SC_Release:
	      DEBUG('a', "Release Lock Syscall. \n");
	      rv = Release_Syscall(machine->ReadRegister(4));
	      break;
	case SC_Wait:
	  DEBUG('a', "Wait Condition Syscall. \n");
	  rv = Wait_Syscall(machine->ReadRegister(4),
			    machine->ReadRegister(5));
	  break;
	case SC_Signal:
	  DEBUG('a', "Signal Condition Syscall. \n");
	  rv = Signal_Syscall(machine->ReadRegister(4),
			      machine->ReadRegister(5));
	  break;
	case SC_Broadcast:
	  DEBUG('a', "Broadcast Condition Syscall. \n");
	  rv = Broadcast_Syscall(machine->ReadRegister(4),
				 machine->ReadRegister(5));
	  break;
	    case SC_Print:
		DEBUG('a', "Print syscall.\n");
		Print_Syscall(machine->ReadRegister(4),
			      machine->ReadRegister(5),
			      machine->ReadRegister(6),
			      machine->ReadRegister(7));
		break;
	    case SC_PrintInt:
		DEBUG('a', "PrintInt syscall.\n");
		PrintInt_Syscall(machine->ReadRegister(4),
			      machine->ReadRegister(5),
			      machine->ReadRegister(6),
			      machine->ReadRegister(7));
		break;
		
	case SC_Exec:
		DEBUG('a', "Exec syscall.\n");
		rv = Exec_Syscall(machine->ReadRegister(4),
			      machine->ReadRegister(5));
		break;
	case SC_Fork:
	  DEBUG('a', "Fork syscall. \n");
	  Fork_Syscall(machine->ReadRegister(4));
	  break;
	case SC_Exit:
	  DEBUG('a', "Exit Syscall. \n");
	  Exit_Syscall(machine->ReadRegister(4));
	  break;
	 case SC_Rand:
	  DEBUG('a', "Rand Syscall. \n");
	  rv = Rand_Syscall();
	  break;
		
	 case SC_Yield:
	  DEBUG('a', "Yield Syscall. \n");
	  Yield_Syscall();
	  break;
	}

	// Put in the return value and increment the PC
	machine->WriteRegister(2,rv);
	machine->WriteRegister(PrevPCReg,machine->ReadRegister(PCReg));
	machine->WriteRegister(PCReg,machine->ReadRegister(NextPCReg));
	machine->WriteRegister(NextPCReg,machine->ReadRegister(PCReg)+4);
	return;
    } else {
      cout<<"Unexpected user mode exception - which:"<<which<<"  type:"<< type<<endl;
      interrupt->Halt();
    }
}

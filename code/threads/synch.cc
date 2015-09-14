// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{

    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value == 0) { 			// semaphore not available
	queue->Append((void *)currentThread);	// so go to sleep
	currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!
Lock::Lock(char* debugName) {}
Lock::~Lock() {}
void Lock::Acquire() {}
void Lock::Release() {}

Condition::Condition(char* debugName) { 

}

Condition::~Condition() { }

void Condition::Wait(Lock* conditionLock) 
{ 
//ASSERT(FALSE);//do we know why this is?
	//Is this current thread? I'm not sure
	Thread *thread;
	//Disable interupts 
	IntStatus oldLevel = interrupt->SetLevel(IntOff);

	if (conditionLock == NULL){
		printf("Condition::Wait: lock input was NULL"); 
		//Restore interrupts
		(void) interrupt->SetLevel(oldLevel);

		return;
	}
	if (_waitingLock == NULL){
		//no one waiting
		_waitingLock = conditionLock;
	}
	if (_waitingLock != conditionLock)
	{
		printf("Condition::Wait: lock mismatches waitinglock");
		//Restore interrupts
		(void) interrupt->SetLevel(oldLevel);

		return;
	}

	//ok to wait: conditionlock is the same as waitingLock, add to wait q, cede condition lock and sleep thread
	_waitingQueue.push(thread);
	conditionLock->Release();
	thread->Sleep();

	//do I restore interupts at the end? 
	(void) interrupt->SetLevel(oldLevel);
}

void Condition::Signal(Lock* conditionLock) {
  //created tentaively by Jack
  
  //1. Disable Interrupts
  //2. if conditionLock is NULL, print error, restore interrupts, and return
  if(!conditionLock)
    {
      printf("conditionLock passed to Wait is NULL");
      //(void) interrupt->SetLevel(oldLevel);
      return;
    }
  //3. if waitingLock is NULL, there is no one waiting, so waitingLock = conditionLock
  //4. if waitingLock is not conditionLock, print error message, restore interrupts, return
  //
  //waitQ.add(currentThread)
  //conditionLock->releaser();
  //currentThread->Sleep()
  //conditionLock->acquire();
  //restore interrupts

 }

void Condition::Broadcast(Lock* conditionLock) 
{
	if (conditionLock != _waitingLock)
	{
	  printf("Condition::Broadcast: input lock ptr mismatches _waitingLock");
	  return;
	}
	while (!_waitingQueue.empty()) 
	{
	  Signal(_waitingLock);
	  _waitingQueue.pop();
	}
}

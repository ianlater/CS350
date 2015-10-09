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
    //printf("about to re enable interrupts in Semaphor P");
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
Lock::Lock(char* debugName) {

	_myThread = NULL;
	name = debugName;
	_isBusy = false;
	_waitQueue = new List;
}
Lock::~Lock() {}

bool Lock::isLockWaitQueueEmpty()
{
  return _waitQueue->IsEmpty();
}

void Lock::Acquire() {

	//disable interrupts
	IntStatus oldLevel = interrupt->SetLevel(IntOff);
	

	if (_myThread!= NULL && isHeldByCurrentThread())
	{
		//current thread is lock owner, nothing to do
		(void) interrupt->SetLevel(oldLevel);
		return;
	}
	
	if (!_isBusy)
	{
		//make State busY	
	
		_isBusy = true;
		//let current thread take lock
		_myThread = currentThread; 
	}
	else //lock is busy
	{
		//put current thread on lock's wait queue
		_waitQueue->Append(currentThread);
		//put current thread to sleep
		currentThread->Sleep();	
	}
	//restore interrupts
	(void) interrupt->SetLevel(oldLevel);	
	
}
void Lock::Release() {
	//disable interrupts
	IntStatus oldLevel = interrupt->SetLevel(IntOff);

	if(_myThread == NULL)
	  {
	    printf("%s: Trying to release a lock w/ no owner\n", currentThread->getName());
	    (void) interrupt->SetLevel(oldLevel);
	    return;
	  }

	//if current thread is not thread owner
	if (currentThread != _myThread)
	{
		printf("Trying to release a lock that does not belong to me..\n");
		(void) interrupt->SetLevel(oldLevel);
		return;	
	}

	void * nextThread = _waitQueue->Remove();	
	//if lock wait queue is not empty, nextThread exists
	if (nextThread != NULL)	
	{
		//cast the void pointer to a thread pointer
		Thread* nextThreadPtr = (Thread*)nextThread;
		//make this thread the lock owner
		_myThread = nextThreadPtr;
		scheduler->ReadyToRun(_myThread);	
	}	
	else //wait queue empty
	{
		//make lock available
		_isBusy = false;
	}
	//restore interrupts
	(void) interrupt->SetLevel(oldLevel);	
}

bool Lock::isHeldByCurrentThread(){

	return currentThread == _myThread;

}
Condition::Condition(char* debugName) { 
  name = debugName;
}

Condition::~Condition() { }

bool Condition::isWaitQueueEmpty()
{
  return _waitingQueue.empty();
}


void Condition::Wait(Lock* conditionLock) 
{ 
//ASSERT(FALSE);//do we know why this is?
	//Is this current thread? I'm not sure
	//Thread *thread;
	//Disable interupts 
	IntStatus oldLevel = interrupt->SetLevel(IntOff);

	if (!conditionLock){
		printf("Condition::Wait: lock input was NULL"); 
		//Restore interrupts
		(void) interrupt->SetLevel(oldLevel);

		return;
	}
	if (!_waitingLock){
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
	_waitingQueue.push(currentThread);
	conditionLock->Release();
	currentThread->Sleep();
	conditionLock->Acquire();

	//do I restore interupts at the end? 
	(void) interrupt->SetLevel(oldLevel);
}

void Condition::Signal(Lock* conditionLock) {
  //created tentatively by Jack
  //disable interrupts
  IntStatus oldLevel = interrupt->SetLevel(IntOff);
  
  //2. if _waitingLock is null, restore interrupts and return
  if(!_waitingLock)//or if waitq empty
    {
      (void) interrupt->SetLevel(oldLevel);
      return;
    }
  if(_waitingLock != conditionLock)
    {
      printf("signal called with different lock, bad");
      (void) interrupt->SetLevel(oldLevel);
      return;
    }
  //Wakeup 1 waiting thread
  Thread* thread = _waitingQueue.front();
  _waitingQueue.pop();
  if(thread)
    {
      scheduler->ReadyToRun(thread); //thread goes in ready Q
    }
  if(_waitingQueue.empty())
    {
      _waitingLock = NULL;//this work?
    }
  //enable interrupts
  (void) interrupt->SetLevel(oldLevel);

 }

void Condition::Broadcast(Lock* conditionLock) 
{
	if (conditionLock != _waitingLock)
	{
	  printf("Condition::Broadcast: input lock ptr mismatches _waitingLock\n");
	  return;
	}
	while (!_waitingQueue.empty()) 
	{
	  Signal(_waitingLock);
	  // _waitingQueue.pop();
	}
}

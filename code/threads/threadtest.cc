// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#ifdef CHANGED
#include "synch.h"
#endif

#ifdef CHANGED

#define APPLICATION_CLERK_TYPE 0
#define PICTURE_CLERK_TYPE 1
#define PASSPORT_CLERK_TYPE 2
#define CASHIER_CLERK_TYPE 3

//global vars, mostly Monitors//

//
const int NUM_CLERKS = 5;
const int NUM_CLERK_TYPES = 4;
//
//Monitor setup:
//array of lock(ptrs) for each clerk+their lines
Lock* clerkLock[NUM_CLERKS];
//Lock* clerkLineLock[NUM_CLERKS]; //i think we onky need 1 lock for all lines
Lock* clerkLineLock = new Lock("ClerkLineLock");

//Condition Variables
Condition* clerkLineCV[NUM_CLERKS];
//Condition* clerkBribeLineCV[NUM_CLERKS];
Condition* clerkCV[NUM_CLERKS];//I think we need this? -Jack

//Monitor Variables
int clerkLineCount[NUM_CLERKS] = {99,99,99,99,99};//start big so we can compare later
//int clerkBribeLineCount[NUM_CLERKS];
int clerkState[NUM_CLERKS];//keep track of state of clerks with ints 0=free,1=busy,2-free //sidenote:does anyone know how to do enums? would be more expressive?
int totalEarnings[NUM_CLERK_TYPES] = {0};//keep track of money submitted by each type of clerk
int numCustomers = 0;


//---------------------------------------------------------------------
//Struct declarations for peoplee in US Passport Office
//Clerk - Passport, Picture, Cashier, Application
//Manager 
//Customer - Senator
//
//---------------------------------------------------------------------

////////////////////////
//Clerk
///////////////////////
class Clerk
{
public:
  Clerk(char* name, int id);//id is unique identifier of clerk and represents its index in clerkLineCV, clerkLock, clerkLineCount, clerkCV, and clerks arrays
  ~Clerk();
  char* GetName(){return _name;}
  int GetType(){return _type;}
  
  virtual void doJob() = 0;
  void run();
protected:
  int _type;//represents type of clerk 1 = ApplicationClerk, 2 = PictureClerk, 3 = PassPortClerk (used to to facilitate abstract use of clerk)
  char* _name;
  int _id;
private:

};
Clerk* clerks[NUM_CLERKS];//global array of clerk ptrs

Clerk::Clerk(char* name, int id) 	
{
	_id = id;
	_name = name + id;
	//Locks
	clerkLock[_id] = new Lock("ClerkLock" + _id);
	////clerkLineLock[_id] = new Lock("ClerkLineLock" + _id);
	//CVs & MVs
	clerkLineCV[_id] = new Condition("ClerkLineCV" + _id);
	clerkLineCount[_id] =0;//Assumption, start empty
	clerkCV[_id] = new Condition("ClerkCV" + _id);
	//clerkState[_id] = FREE;//Assumption, start free
	clerks[id] = this;
}

Clerk::~Clerk()
{

}

void Clerk::run()
{

  while(true)
  {
    //acquire clerkLineLock when i want to update line values 
    clerkLineLock->Acquire();
    //do bribe stuff TODO
    //else if(clerkLineCount[MINE] > 0) //i got someone in line
    if(clerkLineCount[_id] > 0)
      {
	printf(" is Busy\n");
	clerkLineCV[_id]->Signal(clerkLineLock);
	//clerkState[_id] = BUSY;//im helping a customer
      }
    else
      {
	printf( "\n%s  is available\n", _name);
	//clerkState[_id] = AVAILABLE;
      }
    //now do actual interaction
    clerkLock[_id]->Acquire();
    clerkLineLock->Release();
    ///wait for customer data
    clerkCV[_id]->Wait(clerkLock[_id]);
    //once we're here, the customer is waiting for me to do my job
    doJob();
    clerkCV[_id]->Signal(clerkLock[_id]);
    clerkCV[_id]->Wait(clerkLock[_id]);
    clerkLock[_id]->Release();//we're done here, back to top of while for next cust
    //
  }

}
///////////////////////////////
//Application Clerk
///////////////////////////////
class ApplicationClerk : public Clerk
{
public:
  ApplicationClerk(char* name, int id);
  ~ApplicationClerk(){};
  void doJob();
};

ApplicationClerk::ApplicationClerk(char* name, int id) : Clerk(name, id)
{
	_type = APPLICATION_CLERK_TYPE;
}


void ApplicationClerk::doJob()
{

  printf("Filing application\n");
  //required delay of 20 -100 cycles before going back
  for(int i = 0; i < 50; i++)
    currentThread->Yield();

  printf("Here's your application \n");
}
////////////////////////////
//Picture Clerk
///////////////////////////

class PictureClerk : public Clerk
{
public:
  PictureClerk(char* name, int id);
  ~PictureClerk() {};
  void doJob();
};

PictureClerk::PictureClerk(char* name, int id) : Clerk(name, id)
{
	_type = PICTURE_CLERK_TYPE;
}

void PictureClerk::doJob()
{
  printf("Taking a gorgeous picture\n");
  //required delay of 20 -100 cycles before going back
  for(int i = 0; i < 50; i++)
    currentThread->Yield();
}

//////////////////////////
//Passport Clerk
/////////////////////////

class PassPortClerk : public Clerk
{
public:
  PassPortClerk(char* name, int id);
  ~PassPortClerk() {};
  void doJob();
};

PassPortClerk::PassPortClerk(char* name, int id) : Clerk(name, id)
{
	_type = PASSPORT_CLERK_TYPE;
}

void PassPortClerk::doJob()
{

  printf("Checking materials \n");
  //required delay of 20 -100 cycles before going back
  for(int i = 0; i < 50; i++)
    currentThread->Yield();

  printf("Here's your passport\n");
}


///////////////////////
//CashierClerk
//////////////////////
class CashierClerk : public Clerk
{
public:
  CashierClerk(char* name, int id);
  ~CashierClerk(){};
  void doJob();
};

CashierClerk::CashierClerk(char* name, int id) : Clerk(name, id)
{
  _type = CASHIER_CLERK_TYPE;
}

void CashierClerk::doJob()
{
  printf("Checking passport receipt\n");
  //TODO validate they have passport
  printf("Thank you. One moment\n");
  //TODO cashier needs to record that this customer in particular has been issued a passport and the money recieved 
  for(int i = 0; i < 50; i++)
    currentThread->Yield();

  printf("Here's your completed passport\n");
}

////////////////////////
//Customer
///////////////////////

class Customer
{
public:
  Customer(char* name);
  ~Customer(){numCustomers--;};
  char* GetName(){return _name;}
  void run();
protected:
  void pickLine();
private:
  bool isNextClerkType(int type);
  void giveData(int clerkType);
  char* _name;
  int _money;
  int _myLine;
  int _ssn; //unique ssn for each customer
  int _credentials[NUM_CLERK_TYPES];
};

Customer::Customer(char* name) :_name(name)
{
	_money =  100 + 500*(rand() % 4);//init money increments of 100,600,1100,1600
	numCustomers++;
}
bool Customer::isNextClerkType(int type)
{
    //for adding customers who go out of order, we can add in a random number check that returns true randomly

    //check whether clerk is valid next type (check credentials or keep a status)
    if (!_credentials[type]) //credentials is what you have i.e. picture etc. (int[NUM_CLERKS]) w/ index corrosponding to clerk type, 0=have 1=don't have
    {
        if (type == PICTURE_CLERK_TYPE || type == APPLICATION_CLERK_TYPE)//application and picture need no prior credentials
            return true;
        
        if (type == PASSPORT_CLERK_TYPE) //passport clerk requires both application and pictue
        {
            if (_credentials[APPLICATION_CLERK_TYPE] && _credentials[PICTURE_CLERK_TYPE]) 
                return true;
            
            return false;
        }
        if (type == CASHIER_CLERK_TYPE && _credentials[PASSPORT_CLERK_TYPE]) //cashier requires only passport (ASSUMPTION?)
        {
            return true;
	}
        
        return false;  
    }
}

void Customer::giveData(int clerkType)
{
	switch (clerkType) {
	  case APPLICATION_CLERK_TYPE:
		break;

	  case PICTURE_CLERK_TYPE:
		//ask for a picture to be taken
		printf("i would like a picture\n");
		break;

	  case PASSPORT_CLERK_TYPE:
		break;

	  case CASHIER_CLERK_TYPE:
		break;
	}
}

void Customer::run()
{
  int visited = 1;
  while(true)
  {
	clerkLineLock->Acquire();//im going to consume linecount values, this is a CS
	pickLine();
	//now, _myLine is the index of the shortest line
	//TODO if clerk is busy, for testing i guarantee free
	//clerkState[_myLine] = BUSY;
	clerkLineLock->Release();//i no longer need to consume lineCount values, leave this CS

	clerkLock[_myLine]->Acquire();//we are now in a new CS, need to share data with my clerk
	giveData(clerks[_myLine]->GetType());
	clerkCV[_myLine]->Signal(clerkLock[_myLine]);
	//now we wait for clerk to take pic
	clerkCV[_myLine]->Wait(clerkLock[_myLine]);

	if (clerks[_myLine]->GetType() == PICTURE_CLERK_TYPE) {
	  //check if I like my photo RANDOM VAL
	  int picApproval = rand() % 10;//generate random num between 0 and 10
	  if(picApproval >1)
	    {
	      printf("\nI approve of this picture\n");
	      //store that i have pic
	      clerkCV[_myLine]->Signal(clerkLock[_myLine]);
	    }
	  else
	    {
	      printf("this picture is heinous! retake\n");
	    }
	}
	//chose exit condition here
	if(visited++ == 2)
	  break;
  }
  printf("WE OUTTA HERE\n");
}
int testLine = 69;
void Customer::pickLine()
{
  _myLine = -1;
  int lineSize = 1001;
  for(int i = 0; i < NUM_CLERKS; i++)
    {
     	  //check if the type of this line is something I need! TODO
	if(clerks[i] != NULL && isNextClerkType(clerks[i]->GetType())) {
	  if(clerkLineCount[i] < lineSize)// && clerkState[i] != ONBREAK)
	    {
	      _myLine = i;
	      lineSize = clerkLineCount[i];
	    }
	}
    }
  testLine = _myLine;
}
//////////////////////
//Senator
/////////////////////

class Senator : public Customer
{
public:
  Senator(char* name);
  ~Senator();
  void EnterFacility();/*should this and exit facility be functions or should the simulation itself keep track of customers and handle sending them away and bringing them back?*/
};

Senator::Senator(char* name) : Customer(name){}

////////////////////
//Manager
///////////////////
class Manager
{
public:
  Manager(char* name);
  ~Manager(){};
  void run();
private:
  void OutputEarnings();
  char* _name;
};

Manager::Manager(char* name) : _name(name)
{
}

void Manager::OutputEarnings()
{
	int total = 0;
	for (int i =0; i < NUM_CLERK_TYPES; i++) {
		total += totalEarnings[i];
	}
	printf("Earnings report: \n");
	printf("ApplicationClerks: %i \n",totalEarnings[APPLICATION_CLERK_TYPE]);
	printf("PictureClerks: %i \n",totalEarnings[PICTURE_CLERK_TYPE]);
	printf("PassportClerks: %i \n",totalEarnings[PASSPORT_CLERK_TYPE]);
	printf("Cashiers: %i \n",totalEarnings[CASHIER_CLERK_TYPE]);
	printf("TOTAL: %i \n",total);
}

void Manager::run()
{
	//manager doesn't modify anybodies critical section yet
	//wait for some amount of time before printing money status
	for(int i = 0; i < 90000; i++)
		currentThread->Yield();
	OutputEarnings();
}
/*
while (!simulation_over)
{

//check something (linecounts?) and set simulation_over if true? break or return on errors after printing
}
*/
//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest()
{
    DEBUG('t', "Entering SimpleTest");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}

// --------------------------------------------------
// Test Suite
// --------------------------------------------------

//--------------------------------------------------
//Part 2 tests
//--------------------------------------------------

//-------------------------------------------------
//Simple pictureClerk test, by Jack
//custoomer just goes, gets pic, done
//-------------------------------------------------
void p2_customer()
{
  Customer cust = Customer("testCustomer");
  cust.run();
}
int nextClerk = 0;
void p2_pictureClerk()
{
  PictureClerk picClerk = PictureClerk("testPictureClerk", nextClerk++);
  picClerk.run();

}

void p2_applicationClerk()
{
  ApplicationClerk appClerk = ApplicationClerk("testApplicationClerk", nextClerk++);
  appClerk.run();
}

void p2_manager()
{
	Manager manager = Manager("testManager");
	manager.run();
}


// --------------------------------------------------
// Test 1 - see TestSuite() for details
// --------------------------------------------------
Semaphore t1_s1("t1_s1",0);       // To make sure t1_t1 acquires the
                                  // lock before t1_t2
Semaphore t1_s2("t1_s2",0);       // To make sure t1_t2 Is waiting on the 
                                  // lock before t1_t3 releases it
Semaphore t1_s3("t1_s3",0);       // To make sure t1_t1 does not release the
                                  // lock before t1_t3 tries to acquire it
Semaphore t1_done("t1_done",0);   // So that TestSuite knows when Test 1 is
                                  // done
Lock t1_l1("t1_l1");		  // the lock tested in Test 1

// --------------------------------------------------
// t1_t1() -- test1 thread 1
//     This is the rightful lock owner
// --------------------------------------------------
void t1_t1() {
    t1_l1.Acquire();
    t1_s1.V();  // Allow t1_t2 to try to Acquire Lock
 
    printf ("%s: Acquired Lock %s, waiting for t3\n",currentThread->getName(),
	    t1_l1.getName());
    t1_s3.P();
    printf ("%s: working in CS\n",currentThread->getName());
    for (int i = 0; i < 1000000; i++) ;
    printf ("%s: Releasing Lock %s\n",currentThread->getName(),
	    t1_l1.getName());
    t1_l1.Release();
    t1_done.V();
}

// --------------------------------------------------
// t1_t2() -- test1 thread 2
//     This thread will wait on the held lock.
// --------------------------------------------------
void t1_t2() {

    t1_s1.P();	// Wait until t1 has the lock
    t1_s2.V();  // Let t3 try to acquire the lock

    printf("%s: trying to acquire lock %s\n",currentThread->getName(),
	    t1_l1.getName());
    t1_l1.Acquire();

    printf ("%s: Acquired Lock %s, working in CS\n",currentThread->getName(),
	    t1_l1.getName());
    for (int i = 0; i < 10; i++)
	;
    printf ("%s: Releasing Lock %s\n",currentThread->getName(),
	    t1_l1.getName());
    t1_l1.Release();
    t1_done.V();
}

// --------------------------------------------------
// t1_t3() -- test1 thread 3
//     This thread will try to release the lock illegally
// --------------------------------------------------
void t1_t3() {

    t1_s2.P();	// Wait until t2 is ready to try to acquire the lock

    t1_s3.V();	// Let t1 do it's stuff
    for ( int i = 0; i < 3; i++ ) {
	printf("%s: Trying to release Lock %s\n",currentThread->getName(),
	       t1_l1.getName());
	t1_l1.Release();
    }
}

// --------------------------------------------------
// Test 2 - see TestSuite() for details
// --------------------------------------------------
Lock t2_l1("t2_l1");		// For mutual exclusion
Condition t2_c1("t2_c1");	// The condition variable to test
Semaphore t2_s1("t2_s1",0);	// To ensure the Signal comes before the wait
Semaphore t2_done("t2_done",0);     // So that TestSuite knows when Test 2 is
                                  // done

// --------------------------------------------------
// t2_t1() -- test 2 thread 1
//     This thread will signal a variable with nothing waiting
// --------------------------------------------------
void t2_t1() {
    t2_l1.Acquire();
    printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
	   t2_l1.getName(), t2_c1.getName());
    t2_c1.Signal(&t2_l1);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
	   t2_l1.getName());
    t2_l1.Release();
    t2_s1.V();	// release t2_t2
    t2_done.V();
}

// --------------------------------------------------
// t2_t2() -- test 2 thread 2
//     This thread will wait on a pre-signalled variable
// --------------------------------------------------
void t2_t2() {
    t2_s1.P();	// Wait for t2_t1 to be done with the lock
    t2_l1.Acquire();
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
	   t2_l1.getName(), t2_c1.getName());
    t2_c1.Wait(&t2_l1);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
	   t2_l1.getName());
    t2_l1.Release();
}
// --------------------------------------------------
// Test 3 - see TestSuite() for details
// --------------------------------------------------
Lock t3_l1("t3_l1");		// For mutual exclusion
Condition t3_c1("t3_c1");	// The condition variable to test
Semaphore t3_s1("t3_s1",0);	// To ensure the Signal comes before the wait
Semaphore t3_done("t3_done",0); // So that TestSuite knows when Test 3 is
                                // done

// --------------------------------------------------
// t3_waiter()
//     These threads will wait on the t3_c1 condition variable.  Only
//     one t3_waiter will be released
// --------------------------------------------------
void t3_waiter() {
    t3_l1.Acquire();
    t3_s1.V();		// Let the signaller know we're ready to wait
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
	   t3_l1.getName(), t3_c1.getName());
    t3_c1.Wait(&t3_l1);
    printf("%s: freed from %s\n",currentThread->getName(), t3_c1.getName());
    t3_l1.Release();
    t3_done.V();
}


// --------------------------------------------------
// t3_signaller()
//     This threads will signal the t3_c1 condition variable.  Only
//     one t3_signaller will be released
// --------------------------------------------------
void t3_signaller() {

    // Don't signal until someone's waiting
    
    for ( int i = 0; i < 5 ; i++ ) 
	t3_s1.P();
    t3_l1.Acquire();
    printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
	   t3_l1.getName(), t3_c1.getName());
    t3_c1.Signal(&t3_l1);
    printf("%s: Releasing %s\n",currentThread->getName(), t3_l1.getName());
    t3_l1.Release();
    t3_done.V();
}
 
// --------------------------------------------------
// Test 4 - see TestSuite() for details
// --------------------------------------------------
Lock t4_l1("t4_l1");		// For mutual exclusion
Condition t4_c1("t4_c1");	// The condition variable to test
Semaphore t4_s1("t4_s1",0);	// To ensure the Signal comes before the wait
Semaphore t4_done("t4_done",0); // So that TestSuite knows when Test 4 is
                                // done

// --------------------------------------------------
// t4_waiter()
//     These threads will wait on the t4_c1 condition variable.  All
//     t4_waiters will be released
// --------------------------------------------------
void t4_waiter() {
    t4_l1.Acquire();
    t4_s1.V();		// Let the signaller know we're ready to wait
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
	   t4_l1.getName(), t4_c1.getName());
    t4_c1.Wait(&t4_l1);
    printf("%s: freed from %s\n",currentThread->getName(), t4_c1.getName());
    t4_l1.Release();
    t4_done.V();
}


// --------------------------------------------------
// t2_signaller()
//     This thread will broadcast to the t4_c1 condition variable.
//     All t4_waiters will be released
// --------------------------------------------------
void t4_signaller() {

    // Don't broadcast until someone's waiting
    
    for ( int i = 0; i < 5 ; i++ ) 
	t4_s1.P();
    t4_l1.Acquire();
    printf("%s: Lock %s acquired, broadcasting %s\n",currentThread->getName(),
	   t4_l1.getName(), t4_c1.getName());
    t4_c1.Broadcast(&t4_l1);
    printf("%s: Releasing %s\n",currentThread->getName(), t4_l1.getName());
    t4_l1.Release();
    t4_done.V();
}
// --------------------------------------------------
// Test 5 - see TestSuite() for details
// --------------------------------------------------
Lock t5_l1("t5_l1");		// For mutual exclusion
Lock t5_l2("t5_l2");		// Second lock for the bad behavior
Condition t5_c1("t5_c1");	// The condition variable to test
Semaphore t5_s1("t5_s1",0);	// To make sure t5_t2 acquires the lock after
                                // t5_t1

// --------------------------------------------------
// t5_t1() -- test 5 thread 1
//     This thread will wait on a condition under t5_l1
// --------------------------------------------------
void t5_t1() {
    t5_l1.Acquire();
    t5_s1.V();	// release t5_t2
    printf("%s: Lock %s acquired, waiting on %s\n",currentThread->getName(),
	   t5_l1.getName(), t5_c1.getName());
    t5_c1.Wait(&t5_l1);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
	   t5_l1.getName());
    t5_l1.Release();
}

// --------------------------------------------------
// t5_t1() -- test 5 thread 1
//     This thread will wait on a t5_c1 condition under t5_l2, which is
//     a Fatal error
// --------------------------------------------------
void t5_t2() {
    t5_s1.P();	// Wait for t5_t1 to get into the monitor
    t5_l1.Acquire();
    t5_l2.Acquire();
    printf("%s: Lock %s acquired, signalling %s\n",currentThread->getName(),
	   t5_l2.getName(), t5_c1.getName());
    t5_c1.Signal(&t5_l2);
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
	   t5_l2.getName());
    t5_l2.Release();
    printf("%s: Releasing Lock %s\n",currentThread->getName(),
	   t5_l1.getName());
    t5_l1.Release();
}

// --------------------------------------------------
// TestSuite()
//     This is the main thread of the test suite.  It runs the
//     following tests:
//
//       1.  Show that a thread trying to release a lock it does not
//       hold does not work
//
//       2.  Show that Signals are not stored -- a Signal with no
//       thread waiting is ignored
//
//       3.  Show that Signal only wakes 1 thread
//
//	 4.  Show that Broadcast wakes all waiting threads
//
//       5.  Show that Signalling a thread waiting under one lock
//       while holding another is a Fatal error
//
//     Fatal errors terminate the thread in question.
// --------------------------------------------------
void TestSuite() {
  
  printf("Test Suite has started! Start the trials of pain\n\n");

    Thread *t;
    char *name;
    int i;

    printf("starting MultiClerk test");
    t = new Thread("pClerkThread");
    t->Fork((VoidFunctionPtr) p2_pictureClerk,0);

    t = new Thread("aClerkThread");
    t->Fork((VoidFunctionPtr) p2_applicationClerk,0);
    
    t = new Thread("customerThread");
    t->Fork((VoidFunctionPtr) p2_customer,0);

	t = new Thread("managerThread");
	t->Fork((VoidFunctionPtr) p2_manager,0);

    return;//TODO remove after testing
    
    // Test 1

    printf("Starting Test 1\n");

    t = new Thread("t1_t1");
    t->Fork((VoidFunctionPtr)t1_t1,0);

    t = new Thread("t1_t2");
    t->Fork((VoidFunctionPtr)t1_t2,0);

    t = new Thread("t1_t3");
    t->Fork((VoidFunctionPtr)t1_t3,0);

    // Wait for Test 1 to complete
    printf("waiting for Test 1 to complete...\n");
    for (  i = 0; i < 2; i++ )
	t1_done.P();

    // Test 2

    printf("Starting Test 2.  Note that it is an error if thread t2_t2 ");
    printf("completes!\n");
    
    t = new Thread("t2_t1");
    printf("created new thread t2_t1");
    t->Fork((VoidFunctionPtr)t2_t1,0);
    printf("forked t2_t1");
    t = new Thread("t2_t2");
    t->Fork((VoidFunctionPtr)t2_t2,0);
    printf("forked t2_t2");
    // Wait for Test 2 to complete
    printf("waiting for test 2 to complete");
    t2_done.P();

    // Test 3

    printf("Starting Test 3\n");

    for (  i = 0 ; i < 5 ; i++ ) {
	name = new char [20];
	sprintf(name,"t3_waiter%d",i);
	t = new Thread(name);
	t->Fork((VoidFunctionPtr)t3_waiter,0);
    }
    t = new Thread("t3_signaller");
    t->Fork((VoidFunctionPtr)t3_signaller,0);

    // Wait for Test 3 to complete
    for (  i = 0; i < 2; i++ )
	t3_done.P();

    // Test 4

    printf("Starting Test 4\n");

    for (  i = 0 ; i < 5 ; i++ ) {
	name = new char [20];
	sprintf(name,"t4_waiter%d",i);
	t = new Thread(name);
	t->Fork((VoidFunctionPtr)t4_waiter,0);
    }
    t = new Thread("t4_signaller");
    t->Fork((VoidFunctionPtr)t4_signaller,0);

    // Wait for Test 4 to complete
    for (  i = 0; i < 6; i++ )
	t4_done.P();

    // Test 5

    printf("Starting Test 5.  Note that it is an error if thread t5_t1\n");
    printf("completes\n");

    t = new Thread("t5_t1");
    t->Fork((VoidFunctionPtr)t5_t1,0);

    t = new Thread("t5_t2");
    t->Fork((VoidFunctionPtr)t5_t2,0);

}
#endif

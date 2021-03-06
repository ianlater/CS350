// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by cbbballing Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#ifdef CHANGED
#include "synch.h"
#include <queue>
#endif

#ifdef CHANGED

#define APPLICATION_CLERK_TYPE 0
#define PICTURE_CLERK_TYPE 1
#define PASSPORT_CLERK_TYPE 2
#define CASHIER_CLERK_TYPE 3

//global vars, mostly Monitors//

const int NUM_CLERK_TYPES = 4;
int numClerks[NUM_CLERK_TYPES];
const int NUM_CLERKS = 25;//max num of clerks needed.. change later when dynamic stuff is figured out
//
//Monitor setup:
//array of lock(ptrs) for each clerk+their lines
Lock* clerkLock[NUM_CLERKS];
    
//Lock* clerkLineLock[NUM_CLERKS]; //i think we onky need 1 lock for all lines
Lock* clerkLineLock = new Lock("ClerkLineLock");
Lock* outsideLock = new Lock("OutsideLock");
Semaphore* senatorSemaphore = new Semaphore("senatorSemaphore", 1);

//Condition Variables
Condition* clerkLineCV[NUM_CLERKS];
Condition* clerkBribeLineCV[NUM_CLERKS];
Condition* clerkCV[NUM_CLERKS];//I think we need this? -Jack
Condition* clerkBreakCV[NUM_CLERKS]; //CV for break, for use with manager
Condition* senatorCV = new Condition("SenatorCV");

//Monitor Variables
int clerkLineCount[NUM_CLERKS] = {0};//start big so we can compare later
int clerkBribeLineCount[NUM_CLERKS];
int clerkState[NUM_CLERKS];//keep track of state of clerks with ints 0=free,1=busy,2-on breaK //sidenote:does anyone know how to do enums? would be more expressive?
int totalEarnings[NUM_CLERK_TYPES] = {0};//keep track of money submitted by each type of clerk
int numCustomers = 0;
bool senatorInBuilding = false;
int clerkCurrentCustomer[NUM_CLERKS];//relate clerk id to customer id
int clerkCurrentCustomerSSN[NUM_CLERKS];//relate clerk id to customer ssn


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
  int _type;//represents type of clerk 1 = ApplicationClerk, 2 = PictureClerk, 3 = PassportClerk (used to to facilitate abstract use of clerk)
  char* _name;
  int _id;
private:

};
Clerk* clerks[NUM_CLERKS];//global array of clerk ptrs

Clerk::Clerk(char* name, int id) 	
{
	_id = id;
	int newLen = strlen(name) + 16;
	_name = new char[newLen];
	sprintf(_name, "%s%i", name, id);
	//Locks
	//char* buffer1 = new char[50];
	//sprintf(buffer1, "ClerkLock%i", id);
	//clerkLock[_id] = new Lock(buffer1);
	////clerkLineLock[_id] = new Lock("ClerkLineLock" + _id);

	//CVs & MVs
	clerkBreakCV[_id] = new Condition("ClerkBreakCV" +_id);
	char* buffer2 = new char[50];
	sprintf(buffer2, "ClerkLineCv%i", id);
	clerkLineCV[_id] = new Condition(buffer2);
	clerkLineCount[_id] =0;//Assumption, start empty
	clerkBribeLineCount[_id] = 0;
	clerkBribeLineCV[_id] = new Condition(buffer2);

	char* buffer3 = new char[50];
	sprintf(buffer3, "ClerkCV%i", id);
	clerkCV[_id] = new Condition(buffer3);
	clerkState[_id] = 0;//Assumption, start free
	clerks[id] = this;
}

Clerk::~Clerk()
{

}

void Clerk::run()
{
printf("%s beginning to run\n", _name);
  while(true)
  {
/*
  if (senatorInBuilding) {
	clerkState[_id] = 0;
	clerkLock[_id]->Acquire();
//	clerkCV[_id]->Signal(clerkLock[_id]);
	clerkLock[_id]->Release();
//	clerkCV[_id]->Wait(clerkLock[_id]);//SLEEPING FOREVER HERE IS THIS RIGHT? was in b4
  }
  else {
*/
    //acquire clerkLineLock when i want to update line values 
    clerkLineLock->Acquire();
    //do bribe stuff TODO
    //else if(clerkLineCount[MINE] > 0) //i got someone in line
    if(clerkBribeLineCount[_id] > 0)
      {
	//printf("%s: is Busy taking a BRIBE\n", _name);
	clerkBribeLineCV[_id]->Signal(clerkLineLock);
	printf("%s has signalled a Customer to come to their counter\n", _name);
	clerkState[_id] = 1; //busy
	clerkLock[_id]->Acquire();
	clerkLineLock->Release();
	clerkCV[_id]->Wait(clerkLock[_id]);
	//	printf("%s: about to do job for BRIBE****\n", _name );
	printf("%s has received $500 from customer", _name);
	printf("%d (BRIBE)\n", clerkCurrentCustomer[_id]);
	printf("%s has received SSN ", _name);
	printf("%d from Customer", clerkCurrentCustomerSSN[_id]);
	printf("%d\n", clerkCurrentCustomer[_id]);

	doJob();
	//printf("%s: Did job for cust: ", _name);
	//printf("%d\n", clerkCurrentCustomer[_id]);
	clerkCV[_id]->Signal(clerkLock[_id]);
	clerkCV[_id]->Wait(clerkLock[_id]);
	//clerkLock[_id]->Release();
      }
    else  if(clerkLineCount[_id] > 0)
      {
	printf("%s: is Busy\n", _name);
	clerkLineCV[_id]->Signal(clerkLineLock);
	printf("%s has signalled a Customer to come to their counter\n", _name);
	clerkState[_id] = 1;//im helping a customer
	//acquire clerk lock and release line lock
	clerkLock[_id]->Acquire();
	clerkLineLock->Release();
	clerkCV[_id]->Wait(clerkLock[_id]); //WAS IN b4
    //once we're here, the customer is waiting for me to do my job
	printf("%s has received SSN ", _name);
	printf("%d from Customer", clerkCurrentCustomerSSN[_id]);
	printf("%d\n", clerkCurrentCustomer[_id]);

	doJob();
	//printf("%s: Did job for cust: ", _name);
	//printf("%d\n", clerkCurrentCustomer[_id]);
	clerkCV[_id]->Signal(clerkLock[_id]);
	clerkLock[_id]->Release(); //we're done here, back to top of while for next cust
      }
    else if (clerkLineCount[_id] == 0 && clerkBribeLineCount[_id] == 0)  //go on break
      {
	//acquire my lock
	clerkLock[_id]->Acquire();
	//set my status
	clerkState[_id] = 2;

	//release clerk line lock after signalling possible senator
	if (senatorInBuilding) {
		clerkBribeLineCV[_id]->Signal(clerkLineLock);
		clerkLineCV[_id]->Signal(clerkLineLock);
	}
	printf("%s is going on break\n", _name);
	clerkLineLock->Release();
	//wait on clerkBreakCV from manager
	clerkBreakCV[_id]->Wait(clerkLock[_id]);
	printf("%s is coming off break\n", _name);
	//clerkLock[_id]->Acquire();
      }
 // }
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

  ///printf("Filing application\n");
  //required delay of 20 -100 cycles before going back
  for(int i = 0; i < 50; i++)
    currentThread->Yield();
  printf("%s: Has recorded a completed application for Customer", _name);
  printf("%d\n", clerkCurrentCustomer[_id]);
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
  printf("%s has taken a picture of Customer", _name);
  printf("%d\n", clerkCurrentCustomer[_id]);
  //required delay of 20 -100 cycles before going back
  for(int i = 0; i < 50; i++)
    currentThread->Yield();
  //printf("%s: Here you go!\n", _name);
}

//////////////////////////
//Passport Clerk
/////////////////////////

class PassportClerk : public Clerk
{
public:
  PassportClerk(char* name, int id);
  ~PassportClerk() {};
  void doJob();
};

PassportClerk::PassportClerk(char* name, int id) : Clerk(name, id)
{
	_type = PASSPORT_CLERK_TYPE;
}

void PassportClerk::doJob()
{

  printf("Checking materials \n");
  //required delay of 20 -100 cycles before going back
  for(int i = 0; i < 50; i++)
    currentThread->Yield();

  printf("%s has recorded Customer", _name);
  printf("%d\n passport documentation\n", clerkCurrentCustomer[_id]);
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
  printf("%s has provided Customer", _name);
  printf("%d their completed passport\n", clerkCurrentCustomer[_id]);
  for(int i = 0; i < 50; i++)
    currentThread->Yield();

  printf("%s has recorded that Customer", _name);
  printf("%d has been given their completed passport\n", clerkCurrentCustomer[_id]);
}

////////////////////////
//Customer
///////////////////////

class Customer
{
public:
  Customer(char* name);
  Customer(char* name, int credentials[]);
  ~Customer(){numCustomers--;};
  char* GetName(){return _name;}
  void run();
protected:
  void pickLine();
private:
  bool isNextClerkType(int type);
  void giveData(int clerkType);
  void checkSenator();

  char* _name;
  int _money;
  int _myLine;//-1 represents not in a line
  int _id;
  int _ssn; //unique ssn for each customer
  int _credentials[NUM_CLERK_TYPES];
  bool _rememberLine;
  bool _isBribing;
};
std::queue<Customer*> senators;//parent type customer to allow for abstraction of customer::run for other senators

Customer::Customer(char* name) 
{
        _id =numCustomers;
	_ssn = numCustomers + 1000;
	_name = new char[strlen(name) + 16];
	sprintf(_name, "%s%i",name,_id);
	_money =  100 + 500*(rand() % 4);//init money increments of 100,600,1100,1600
	_myLine = -1;
	_rememberLine = false;
	numCustomers++;
}

Customer::Customer(char* name, int* credentials) 
{
	for(int i=0;i<NUM_CLERK_TYPES;i++) {
	  _credentials[i] = credentials[i];
	}
        _id =numCustomers;
	_ssn = numCustomers + 1000;
	_name = new char[strlen(name) + 16];
	sprintf(_name, "%s%i",name,_id);
	_money =  100 + 500*(rand() % 4);//init money increments of 100,600,1100,1600
	_myLine = -1;
	_rememberLine = false;
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
       } 
        return false;  
}

void Customer::giveData(int clerkType)
{
	switch (clerkType) {
	  case APPLICATION_CLERK_TYPE:
		printf("%s: may I have application please?\n", _name);
		if(_isBribing)
		  totalEarnings[APPLICATION_CLERK_TYPE] += 500;
		break;

	  case PICTURE_CLERK_TYPE:
		//ask for a picture to be taken
		printf("%s: i would like a picture\n", _name);
		if(_isBribing)
		  totalEarnings[PICTURE_CLERK_TYPE] += 500;
		break;

	  case PASSPORT_CLERK_TYPE:
		printf("%s: ready for my passport\n", _name);
		if(_isBribing)
		  totalEarnings[PASSPORT_CLERK_TYPE] += 500;
		break;

	  case CASHIER_CLERK_TYPE:
		printf("%s has given Cashier ", _name);
		printf("%s $100\n", clerks[_myLine]->GetName());
		_money-=100;
		totalEarnings[CASHIER_CLERK_TYPE] += 100;
		break;
	}
}

void Customer::checkSenator()
{
	//if there is a Senator in the building (and you're not that particular senator), wait until he's gone
	if (senatorInBuilding && this != senators.front()){
		printf("%s: waiting outside\n", _name);
		senatorSemaphore->P();
		senatorSemaphore->V();
		//go back inside
	}
}

void Customer::run()
{
  while(true)
  {

	clerkLineLock->Acquire();//im going to consume linecount values, this is a CS
	pickLine();
	//now, _myLine is the index of the shortest line
	//if the clerk is busy or on break, get into line
	if (clerkState[_myLine] != 0) {
	  if(_isBribing)
	    {
	      printf("%s has gotten in a bribe line for ", _name);
	      printf("%s\n", clerks[_myLine]->GetName());//feels wrong
	      clerkBribeLineCount[_myLine]++;
	    }
	  else
	    {
	      printf("%s has gotten in a regular line for ", _name);
	      printf("%s\n", clerks[_myLine]->GetName());//feels wrong
	      clerkLineCount[_myLine]++;
	    }
		printf("%s: waiting in line for %s\n", _name, clerks[_myLine]->GetName());
		if(_isBribing)
		  {
		    clerkBribeLineCV[_myLine]->Wait(clerkLineLock);
		    clerkLineLock->Acquire();   
		    clerkBribeLineCount[_myLine]--;
		    printf("bribe line%i count: %i",_myLine, clerkBribeLineCount[_myLine]);
		  }
		else
		  {
		    clerkLineCV[_myLine]->Wait(clerkLineLock);
		    clerkLineLock->Acquire();   
		    clerkLineCount[_myLine]--;
		    printf("regular line%i count: %i",_myLine, clerkLineCount[_myLine]);
		  }
		if (senatorInBuilding && this !=senators.front()) {
		  _rememberLine = true;//you're in line being kicked out by senatr. senator can't kick self out
  			//make sure to signal senator who may be in line 
			if(_isBribing) {
			  clerkBribeLineCV[_myLine]->Signal(clerkLineLock);
			}
			else {
			  clerkLineCV[_myLine]->Signal(clerkLineLock);
			}
			  clerkLineLock->Release();
		  }

		//senator may have sent everyone out of lineCV so this nesting is for getting back in line	
		checkSenator(); //after this point senator is gone- get back in line if you were kicked out
		if (_rememberLine) {
			clerkLineLock->Acquire();
			//you may be the first one in line now so check. in the case that you were senator you wouldn't remember line 
			if (clerkState[_myLine] != 0) {
			  if(_isBribing)
			    {
			      clerkBribeLineCount[_myLine]++;
			       clerkBribeLineCV[_myLine]->Wait(clerkLineLock);
			      clerkBribeLineCount[_myLine]--;
			    }
			  else
			    {
			      clerkLineCount[_myLine]++;
				printf("%s: waiting in line for %s\n", _name, clerks[_myLine]->GetName());
				clerkLineCV[_myLine]->Wait(clerkLineLock);
				clerkLineCount[_myLine]--;
			    }
			}
		}
		//at this point we assume won't have to go outside till finished with current clerk
	}

	clerkState[_myLine] = 1;
	clerkLineLock->Release();//i no longer need to consume lineCount values, leave this CS

	clerkLock[_myLine]->Acquire();//we are now in a new CS, need to share data with my clerk
	clerkCurrentCustomerSSN[_myLine] = _ssn;
	printf("%s has given SSN ", _name);
	printf("%d", _ssn);
	printf("to %s\n", clerks[_myLine]->GetName());
	clerkCurrentCustomer[_myLine] = _id;
	int type = clerks[_myLine]->GetType();
	giveData(type);
	_isBribing = false;
	clerkCV[_myLine]->Signal(clerkLock[_myLine]);
	//now we wait for clerk to do job
	clerkCV[_myLine]->Wait(clerkLock[_myLine]);
	
	//set credentials
	_credentials[type] = true;
	printf("%s: Thank you %s\n", _name, clerks[_myLine]->GetName());

	if (type == PICTURE_CLERK_TYPE) {
	  //check if I like my photo RANDOM VAL
	  int picApproval = rand() % 10;//generate random num between 0 and 10
	  if(picApproval >8)
	    {
	      printf("%s: does like their picture from ", _name);
	      printf("%s\n", clerks[_myLine]->GetName());
	      //store that i have pic
	    }
	  else
	    {
	      printf("%s: does not like their picture from ", _name);
	      printf("%s, please retake\n", clerks[_myLine]->GetName());
	      //_credentials[type] = false;//lets seeye
	    }
	}
	clerkCV[_myLine]->Signal(clerkLock[_myLine]);

	_myLine = -1;
	_rememberLine = false;
	
	//chose exit condition here
	if(_credentials[CASHIER_CLERK_TYPE])
	  break;
  }
  printf("%s: IS LEAVING THE PASSPORT OFFICE\n", _name);
}
int testLine = 69;
void Customer::pickLine()
{
  // _isBribing = false;
  if (!_rememberLine)//if you don't have to remember a line, pick a new one
  {
	  _myLine = -1;
	  int lineSize = 1001;
	  for(int i = 0; i < NUM_CLERKS; i++)
	    {
		  //check if the type of this line is something I need! TODO
		if(clerks[i] != NULL && isNextClerkType(clerks[i]->GetType())) {
		  if(clerkLineCount[i] < lineSize )//&& clerkState[i] != 2)
		    {		      _myLine = i;
		      lineSize = clerkLineCount[i];
		    }
		}
	    }
	  int desireToBribe = rand() % 10;
	  //if i want to bribe, let's lock at bribe lines
	  if(_money > 600 && desireToBribe > 8)
	    {
	      for(int i = 0; i < NUM_CLERKS; i++)
		{
		  if(clerks[i] != NULL && isNextClerkType(clerks[i]->GetType())) 
		    {
		      if(clerkBribeLineCount[i] <=  lineSize)//for TESTING. do less than only for real
			{
			  //printf("%s: I'm BRIBING\n", _name);
			  _money -= 500;
			  _myLine = i;
			  _isBribing = true;
			  lineSize = clerkBribeLineCount[i];
			}
		    }
		}
	    }
  }
}
//////////////////////
//Senator
/////////////////////

class Senator : public Customer
{
public:
  Senator(char* name);
  ~Senator(){};
  void run();
private:
  void EnterOffice();
  void ExitOffice();
};

Senator::Senator(char* name) : Customer(name)
{
  senators.push(this);
}

void Senator::EnterOffice()
{
  //walk in acquire all clerk locks to prevent next in line from getting to clerk
  senatorSemaphore->P(); //use to lock down entire run section
  senatorInBuilding = true;//signals all people waiting to exit
  
  clerkLineLock->Acquire(); //acquire to broadcast current customers. released in Customer::run()
  for (int i=0; i<NUM_CLERKS;i++) {
    if (clerkLineCV[i] != NULL) {
	clerkLineCV[i]->Broadcast(clerkLineLock); //clear out lines. they will stop because of semaphore after leaving line/before returning to it
    }

    if (clerkBribeLineCV[i] != NULL) {
	clerkBribeLineCV[i]->Broadcast(clerkLineLock); //clear out lines. they will stop because of semaphore after leaving line/before returning to it
    }
  }
 
  clerkLineLock->Release();
  for (int i=0; i<NUM_CLERKS;i++) {
	//wait for bribe line to empty
	while(clerkBribeLineCount[i] > 0) {
	  clerkLineLock->Acquire();
	  clerkBribeLineCV[i]->Signal(clerkLineLock);
	  clerkBribeLineCV[i]->Wait(clerkLineLock);
	}
	//wait for regular line to empty
	while (clerkLineCount[i] > 0) {
	  clerkLineLock->Acquire();
	  clerkLineCV[i]->Signal(clerkLineLock);
	  clerkLineCV[i]->Wait(clerkLineLock);
	}
  }
  //everyone is out of lines now wait for clerks to finish
  for (int i=0; i<NUM_CLERKS;i++) {
    if (clerkLock[i] != NULL) {
	while (clerkState[i] == 1) {
		clerkLock[i]->Acquire();
		clerkCV[i]->Signal(clerkLock[i]);
		clerkCV[i]->Wait(clerkLock[i]);
	}
    }
  }
}

void Senator::ExitOffice()
{
  senators.pop();//remove self from senator q
  senatorInBuilding = false;
  senatorSemaphore->V();
}

void Senator::run()
{
	//enter facility
	EnterOffice();
	//proceed as a normal customer
	Customer::run();
	//exit facility
	ExitOffice();
}

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
	printf("\n----Earnings report:---- \n");
	printf("ApplicationClerks: %i \n",totalEarnings[APPLICATION_CLERK_TYPE]);
	printf("PictureClerks: %i \n",totalEarnings[PICTURE_CLERK_TYPE]);
	printf("PassportClerks: %i \n",totalEarnings[PASSPORT_CLERK_TYPE]);
	printf("Cashiers: %i \n",totalEarnings[CASHIER_CLERK_TYPE]);
	printf("TOTAL: %i \n------------------------\n\n",total);
}

void Manager::run()
{
  while(true) {
	for (int i = 0; i < 1000; i++)
		currentThread->Yield();
	for (int i = 0; i < NUM_CLERKS; i++)
	{
		if (clerkState[i] == 2 && (clerkLineCount[i] >= 3 || clerkBribeLineCount[i] >= 1 || senatorInBuilding) )
		{
			//wake up clerk
			clerkLock[i]->Acquire();	
			printf("%s waking up ", _name);
			printf("%s\n", clerks[i]->GetName());
			clerkState[i] = 0;//set to available	
			clerkBreakCV[i]->Signal(clerkLock[i]);	
			clerkLock[i]->Release();	
		}
	}
	OutputEarnings();
	if (numCustomers == 0) {
		break;
	}
  }
}
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

void p2_customerAtCashier()
{
  int credentials[NUM_CLERK_TYPES] = {1,1,1,0};
  Customer custAtCashier = Customer("testCustomerAtCashier", credentials);
  custAtCashier.run();
}
void p2_customerWPassport()
{
  int credentials[NUM_CLERK_TYPES] = {0,0,1,0};
  Customer custWCreds = Customer("testCustomerWCreds", credentials);
  custWCreds.run();
}

void p2_senator()
{
  Senator senator = Senator("testSenator");
  senator.run();
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

void p2_passportClerk()
{
  PassportClerk passportClerk = PassportClerk("testPassportClerk", nextClerk++);
  passportClerk.run();
}

void p2_cashierClerk()
{
  CashierClerk cashierClerk = CashierClerk("testCashierClerk", nextClerk++);
  cashierClerk.run();
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
//---------------------------------------------------
// Repeatable test code
//---------------------------------------------------
/*1*/
void p1Test(){
	    // Test 1
	Thread *t;
	int i;
	char * name;	
    printf("Starting Test 1\n");
    t = new Thread("t1_t1");
    t->Fork((VoidFunctionPtr)t1_t1,0);

    t = new Thread("t1_t2");
    t->Fork((VoidFunctionPtr)t1_t2,0);

    t = new Thread("t1_t3");
    t->Fork((VoidFunctionPtr)t1_t3,0);

    // Wait for Test 1 to complete
    printf("waiting for Test 1 to complete...\n");
    for ( int i = 0; i < 2; i++ )
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
	name = new char[20];
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
	name = new char[20];
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
/*1*/
void shortLineTest()
{
	//instantiate two customer threads
	Thread *t = new Thread("clerk1");
	t->Fork((VoidFunctionPtr) p2_pictureClerk, 0);
	printf("Clerk 1 created \n");
	printf("Creating two customers:\n");	
	t = new Thread("c1");
	t->Fork((VoidFunctionPtr) p2_customer, 0);
	t = new Thread("c2");
	t->Fork((VoidFunctionPtr) p2_customer,0);
	
}
/*2*/
void managerClerkTest()
{
}
/*3*/
void cashierTest()
{
	Thread *t = new Thread("clerk");
        t = new Thread("clerk4");
        t->Fork((VoidFunctionPtr) p2_cashierClerk, 0);
  //need at least 3 to so cashier doesn't go on break
  for (int i=0; i < 3; i++){
	t = new Thread("c1");
	t->Fork((VoidFunctionPtr) p2_customerWPassport, 0);
  }

	t = new Thread("manager");
	t->Fork((VoidFunctionPtr) p2_manager, 0);
}
/*4*/
void clerkWaitTest()
{
	Thread *t = new Thread("clerk");
	t->Fork((VoidFunctionPtr) p2_pictureClerk, 0);
	t = new Thread("clerk2");
	t->Fork((VoidFunctionPtr) p2_applicationClerk, 0);
	t = new Thread("clerk3");
	t->Fork((VoidFunctionPtr) p2_passportClerk, 0);
	t = new Thread("clerk4");
	t->Fork((VoidFunctionPtr) p2_cashierClerk, 0);
}
/*5*/
void managerWakeTest()
{
	Thread *t = new Thread("clerk");
	t->Fork((VoidFunctionPtr) p2_pictureClerk, 0);
	t = new Thread("manager");
	t->Fork((VoidFunctionPtr) p2_manager, 0);
	//wait for clerks to go to sleep
	for(int i=0; i < 200; i++)
	  currentThread->Yield();
	//need at least 3 to so cashier doesn't go on break
	for (int i=0; i < 3; i++){
	  t = new Thread("c1");
	  t->Fork((VoidFunctionPtr) p2_customer, 0);
	}


}
/*6*/
int salesUpdate()
{
  int temp = 0;
  	for (int i =0; i < NUM_CLERK_TYPES; i++) {
		temp += totalEarnings[i];
	}
	return temp;
}
void salesRCTest()
{
  int updateCount = 15;
  int total = 0;

  Thread* t;
  t = new Thread("manager");
  t->Fork((VoidFunctionPtr)  p2_manager, 0);
  t = new Thread("cash");
  t->Fork((VoidFunctionPtr) p2_cashierClerk, 0);
  //t = new Thread("cash0");
  //t->Fork((VoidFunctionPtr) p2_cashierClerk, 0);
  t = new Thread("cust");
  for(int i = 0; i <updateCount; i++)
    {
      t->Fork((VoidFunctionPtr) p2_customerAtCashier, 0);
      t = new Thread("cust0");
    }

  while(updateCount != 0)
    {
      int newTotal = salesUpdate();
      if(newTotal!= total)
	{
	  total = newTotal;
	  updateCount--;
	  printf("****SALES UPDATED: ONLY %d CUSTOMERS LEFT TO UPDATE SALES*****\n", updateCount);
	}
      currentThread->Yield();
    }
  printf("TEST COMPLETE\n");



}

/*7*/
void senatorTest()
{
	//initiate all clerks
	Thread *t = new Thread("clerk");
	t->Fork((VoidFunctionPtr) p2_pictureClerk, 0);
	t = new Thread("clerk2");
	t->Fork((VoidFunctionPtr) p2_applicationClerk, 0);
	t = new Thread("clerk3");
	t->Fork((VoidFunctionPtr) p2_passportClerk, 0);
	t = new Thread("clerk4");
	t->Fork((VoidFunctionPtr) p2_cashierClerk, 0);
	
	//initiate some customers
	//need at least 3 to so cashier doesn't go on break
	for (int i=0; i < 3; i++){
	  t = new Thread("c1");
	  t->Fork((VoidFunctionPtr) p2_customer, 0);
	}

	//initiate senators to test senator effectiveness/consecutive senators
	t = new Thread("senator");
	t->Fork((VoidFunctionPtr) p2_senator, 0);
	t = new Thread("senator");
	t->Fork((VoidFunctionPtr) p2_senator, 0);
/*
	t = new Thread("manager");
	t->Fork((VoidFunctionPtr) p2_manager, 0);
*/
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
	printf("Repeatable tests:\n");
	printf("0) Run part 1 tests (locks and conditions).\n");
	printf("1) Prove that no 2 customers ever choose the same shortest line at the same time.\n");
	printf("2) Prove that managers only read from one Clerk's total money received, at a time\n");
	printf("3) Prove that Customers do not leave until they are given their passport by the Cashier. The Cashier does not start on another customer until they know that the last Customer has left their area.\n");
	printf("4) Prove that Clerks go on break when they have no one waiting in their line.\n");
	printf("5) Prove that Managers get Clerks off their break when lines get too long.\n");
	printf("6) Prove that total sales never sufers from a race condition.\n");
	printf("7) Prove that Customers behave properly when Senators arrive.\n");
	printf("Enter a number between 1 and 7 to choose a test to run, or type s to begin the big system test.\n"); 
	char entry;
	scanf("%c", &entry);
	printf("You chose %c \n", entry);	
	
	int clerkNumArray[4];
	//instantiate all clerk locks (max number)
	for( int i = 0; i < NUM_CLERKS; i++)
      {
	char* buffer1 = new char[50];
	sprintf(buffer1, "ClerkLock%i", i);
	clerkLock[i] = new Lock(buffer1);
	//clerks[i] =new  Clerk();
      }
	if(entry != 's')
	{
		int num = (int)entry - 48 ;
	//remove \n from entry
		char garbage;
		scanf("%c", &garbage);	
		if (num>7 || num < 0)
		{
			printf("Please enter a valid entry.\n");
		} 
		else
		{
			Thread *t;
			if (num == 0)
				p1Test();
			if (num == 1)
				shortLineTest();
			else if (num ==2) {
				printf("Because we implemented the money count not through individual member variables, but in a monitor variable that customers directly access, there will never be a race condition in this instance.");
			}
			else if (num == 3) {cashierTest();}
			else if (num == 4) {
				clerkWaitTest();
			}
			else if (num == 5) {managerWakeTest();}
			else if (num == 6) {salesRCTest();}
			else if (num == 7) {senatorTest();}	
			printf("Test completed. ");
		}
			printf("\n");
	}
	else
	{
		int numCustomersInput;
		printf("Enter number of Picture Clerks (between 1 and 5): ");
		scanf("%d", &clerkNumArray[PICTURE_CLERK_TYPE]);
		printf("\nEnter number of Application Clerks (between 1 and 5): ");
		scanf("%d", &clerkNumArray[APPLICATION_CLERK_TYPE]);
		printf("\nEnter number of Passport Clerks (between 1 and 5): ");
		scanf("%d", &clerkNumArray[PASSPORT_CLERK_TYPE]);
		printf("\nEnter number of Cashiers (between 1 and 5): ");
		scanf("%d", &clerkNumArray[CASHIER_CLERK_TYPE]);
		printf("\nEnter number of Customers (between 20 and 50): ");
		scanf("%d", &numCustomersInput);
		//test: print array to see if stored correctly
		for (int i = 0; i < NUM_CLERK_TYPES; i++)
		{
			//do something, add them to an array? or count?
		}
		Thread *t;
	    char* name;
	    int thread_id = 0;
	    int i;
	    printf("starting MultiClerk test");

	    for (int i = 0; i < clerkNumArray[PICTURE_CLERK_TYPE]; i++)
	    {
		char* buffer1 = new char[5];
		sprintf(buffer1, "pic%i", thread_id);
		t = new Thread(buffer1);
		t->Fork((VoidFunctionPtr) p2_pictureClerk, 0);
		thread_id++;
	    }
	    for (int i = 0; i < clerkNumArray[APPLICATION_CLERK_TYPE];i++ )
	    {
		char* buffer1 = new char[5];
		sprintf(buffer1, "app%i", thread_id);
		t = new Thread(buffer1);
		t->Fork((VoidFunctionPtr) p2_applicationClerk, 0);
		thread_id++;
	    }
	    for (int i = 0; i < clerkNumArray[PASSPORT_CLERK_TYPE];i++)
	    {
		char* buffer1 = new char[5];
		sprintf(buffer1, "pas%i", thread_id);
		t = new Thread(buffer1);
		t->Fork((VoidFunctionPtr) p2_passportClerk, 0);
		thread_id++;
	    }
	    for (int i = 0; i < clerkNumArray[CASHIER_CLERK_TYPE]; i++)
	    {
		char* buffer1 = new char[5];
		sprintf(buffer1, "cas%i", thread_id);
		t = new Thread(buffer1);
		t->Fork((VoidFunctionPtr) p2_cashierClerk,0);
		thread_id++;
	    }
	  for (int i = 0; i< numCustomersInput; i++) { 
		char* buffer1 = new char[5];
		sprintf(buffer1, "cus%i", thread_id);
		t = new Thread(buffer1);
		//t = new Thread(thread_id);
	    t->Fork((VoidFunctionPtr) p2_customer,0);
	    thread_id++;
	  }
	  //new senator thread
	  	char* buffer1 = new char[10];
		sprintf(buffer1, "senator%i", thread_id);
		thread_id++;
		t = new Thread(buffer1);
		t->Fork((VoidFunctionPtr) p2_senator, 0);

		buffer1 = new char[10];
		sprintf(buffer1, "manager%i", thread_id);
		t = new Thread(buffer1);
		t->Fork((VoidFunctionPtr) p2_manager,0);
	}//end else statement

    return;//TODO remove after testing
    


}

/*********************************************
 *Project 2 tests BELOW. triggered with -P2
 *
 *
 *********************************************
 */
void Part2()
{
  printf("/nSTARTING PROJECT 2 TEST SUITE/n");
}
#endif

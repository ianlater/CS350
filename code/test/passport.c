/* passport.c

*/
#include "copyright.h"
#include "system.h"
#include "syscall.h"
int a[3];
int b, c;

#ifdef CHANGED
#include "synch.h"
#endif

#ifdef CHANGED

#define APPLICATION_CLERK_TYPE 0
#define PICTURE_CLERK_TYPE 1
#define PASSPORT_CLERK_TYPE 2
#define CASHIER_CLERK_TYPE 3

/*global vars, mostly Monitors*/

const int NUM_CLERK_TYPES = 4;
int numClerks[NUM_CLERK_TYPES];
const int NUM_CLERKS = 25;/*max num of clerks needed.. change later when dynamic stuff is figured out
/*
Monitor setup:
array of lock(ptrs) for each clerk+their lines
*/
int clerkLock[NUM_CLERKS];
    
int* clerkLineLock = CreateLock("ClerkLineLock", 13);
int* outsideLock = CreateLock("OutsideLock",11);
/*
Semaphore* senatorSemaphore = new Semaphore("senatorSemaphore", 1);
*/
int* senatorLock = CreateLock("SenatorLock", 11);

/*Condition Variables*/
int clerkLineCV[NUM_CLERKS];
int clerkBribeLineCV[NUM_CLERKS];
int clerkCV[NUM_CLERKS];/*I think we need this? -Jack*/
int clerkBreakCV[NUM_CLERKS]; /*CV for break, for use with manager*/
int senatorCV = CreateCondition("SenatorCV", 9);

/*Monitor Variables*/
int clerkLineCount[NUM_CLERKS] = {0};/*start big so we can compare later*/
int clerkBribeLineCount[NUM_CLERKS];
int clerkState[NUM_CLERKS];/*keep track of state of clerks with ints 0=free,1=busy,2-on breaK /*sidenote:does anyone know how to do enums? would be more expressive?*/
int totalEarnings[NUM_CLERK_TYPES] = {0};/*keep track of money submitted by each type of clerk*/
int numCustomers = 0;
bool senatorInBuilding = false;
int clerkCurrentCustomer[NUM_CLERKS];/*relate clerk id to customer id*/
int clerkCurrentCustomerSSN[NUM_CLERKS];/*relate clerk id to customer ssn*/
int currentSenatorId;
int i;/*iterator for loops*/
struct Clerk
{
  int type;/*represents type of clerk 1 = ApplicationClerk, 2 = PictureClerk, 3 = PassportClerk (used to to facilitate abstract use of clerk)*/
  char* name;
  id;
};

Clerk* clerks[NUM_CLERKS];/*global array of clerk ptrs*/

void Clerk(struct Clerk clerk, char* name, id) 	
{
	clerk.id = id;
	int newLen = strlen(name) + 16;
	clerk.name = new char[newLen];
	sPrint(clerk.name, "%s%i", name, id);
	/*Locks
	char* buffer1 = new char[50];
	sPrint(buffer1, "ClerkLock%i", id);
	clerkLock[id] = CreateLock(buffer1);*/
	clerkLineLock[id] = CreateLock("ClerkLineLock" + id);

	/*CVs & MVs*/
	clerkBreakCV[id] = CreateCondition("ClerkBreakCV" +id);
	char* buffer2 = new char[50];
	sPrint(buffer2, "ClerkLineCv%i", id);
	clerkLineCV[id] = CreateCondition(buffer2);
	clerkLineCount[id] =0;/*Assumption, start empty*/
	clerkBribeLineCount[id] = 0;
	clerkBribeLineCV[id] = CreateCondition(buffer2);

	char* buffer3 = new char[50];
	sPrint(buffer3, "ClerkCV%i", id);
	clerkCV[id] = CreateCondition(buffer3);
	clerkState[id] = 0;/*Assumption, start free*/
	clerks[id] = this;
}

void DestroyClerk(struct Clerk clerk)
{

}

void doJob(int type){
	switch type{
		case APPLICATION_CLERK_TYPE:
			for(i = 0; i < 50; i++)
				Yield()();
			Print("%s: Has recorded a completed application for Customer", name);
			Print("%d\n", clerkCurrentCustomer[id]);
			break;
		case PICTURE_CLERK_TYPE:
			for(i = 0; i < 50; i++)
				Yield()();
			Print("%s has taken a picture of Customer", name);
			Print("%d\n", clerkCurrentCustomer[id]);
			break;
		case PASSPORT_CLERK_TYPE:
			Yield()();Print("Checking materials \n");
			  /*required delay of 20 -100 cycles before going back*/
			for(i = 0; i < 50; i++)
				Yield()();

			Print("%s has recorded Customer", name);
			Print("%d\n passport documentation\n", clerkCurrentCustomer[id]);
			break;
		case CASHIER_CLERK_TYPE:
			Print("Checking passport receipt\n");
			/*TODO validate they have passport*/
			Print("Thank you. One moment\n");
			/*TODO cashier needs to record that this customer in particular has been issued a passport and the money recieved */
			Print("%s has provided Customer", name);
			Print("%d their completed passport\n", clerkCurrentCustomer[id]);
			for(i = 0; i < 50; i++)
				Yield()();

			Print("%s has recorded that Customer", name);
			Print("%d has been given their completed passport\n", clerkCurrentCustomer[id]);
			break;
	}
}
void ClerkRun(struct Clerk clerk)
{
Print("%s beginning to run\n", 24, name, "");
  while(true)
  {
    /*acquire clerkLineLock when i want to update line values */
    Acquire(clerkLineLock);
    
    if(clerkBribeLineCount[clerk.id] > 0)
      {
	/*Print("%s: is Busy taking a BRIBE\n", name);*/
	Signal(clerkBribeLineCV[clerk.id], clerkLineLock);
	Print("%s has signalled a Customer to come to their counter\n",50, name, "");
	clerkState[clerk.id] = 1; /*busy*/
	clerkLock[clerk.id]->Acquire();
	Release(clerkLineLock);
	clerkCV[clerk.id]->Wait(clerkLock[clerk.id]);
	Print("%s has received $500 from customer", 50,_name, "");
	PrintInt("%d (BRIBE)\n", 10, clerkCurrentCustomer[clerk.id], 0);
	Print("%s has received SSN ", 24, name,"");
	PrintInt("%d from Customer",14, clerkCurrentCustomerSSN[clerk.id],0);
	PrintInt("%d\n",4, clerkCurrentCustomer[clerk.id],0);

	doJob(clerk.type);
	/*Print("%s: Dclerk.id job for cust: ", name);*/
	/*Print("%d\n", clerkCurrentCustomer[clerk.id]);*/
	Signal(clerkCV[clerk.id],clerkLock[clerk.id]);
	Wait(clerkCV[clerk.id], clerkLock[clerk.id]);
	/*clerkLock[clerk.id]->Release();*/
      }
    else  if(clerkLineCount[clerk.id] > 0)
      {
	Print("%s: is Busy\n", 20, name, "");
	Signal(clerkLineCV[clerk.id], clerkLineLock);
	Print("%s has signalled a Customer to come to their counter\n",50, name,"");
	clerkState[clerk.id] = 1;/*im helping a customer*/
	/*acquire clerk lock and release line lock*/
	Acquire(clerkLock[clerk.id]);
	Release(clerkLineLock);
	Wait(clerkCV[clerk.id], clerkLock[clerk.id]); /*WAS IN b4*/
    /*once we're here, the customer is waiting for me to do my job*/
	Print("%s has received SSN ", 16, name);
	PrintInt("%d from Customer", 12,clerkCurrentCustomerSSN[clerk.id],"");
	PrintInt("%d\n", 4, clerkCurrentCustomer[clerk.id],"");

	doJob(clerk.type);
	/*Print("%s: Dclerk.id job for cust: ", name);*/
	/*Print("%d\n", clerkCurrentCustomer[clerk.id]);*/
	Signal(clerkCV[clerk.id], clerkLock[clerk.id]);
	Release(clerkLock[clerk.id]); /*we're done here, back to top of while for next cust*/
      }
    else if (clerkLineCount[clerk.id] == 0 && clerkBribeLineCount[clerk.id] == 0)  /*go on break*/
      {
		/*acquire my lock*/
		Acquire(clerkLock[clerk.id]);
		/*set my status*/
		clerkState[clerk.id] = 2;

		/*release clerk line lock after signalling possible senator*/
		if (senatorInBuilding) {
			Signal(clerkBribeLineCV[clerk.id],clerkLineLock);
			Signal(clerkLineCV[clerk.id],clerkLineLock);
		}
		Print("%s is going on break\n", 20, name, "");
		Release(clerkLineLock);
		/*wait on clerkBreakCV from manager*/
		Wait(clerkBreakCV[clerk.id],clerkLock[clerk.id]);
		Print("%s is coming off break\n",24, name, "");
      }
  }
}

struct Customer
{
  bool isNextClerkType(int type);
  void giveData(int clerkType);
  void checkSenator();

  char* name;
  int money;
  int myLine;/*-1 represents not in a line*/
  id;
  int ssn; /*unique ssn for each customer*/
  int credentials[NUM_CLERK_TYPES];
  bool rememberLine;
  bool isBribing;
  bool isSenator
};

/* creates new customer w/ given name. should declare new customer ahead of time and place where needed. this just fills in info*/
void CreateCustomer(struct Customer customer, char* name) 
{	
    customer->id = numCustomers;
	customer->ssn = numCustomers + 1000;
	name = new char[strlen(name) + 16];
	sPrint(customer->name, "%s%i",name,id);
	customer->money =  100 + 500*(rand() % 4);/*init money increments of 100,600,1100,1600*/
	customer->myLine = -1;
	customer->rememberLine = false;
	customer->isSenator = false;
	numCustomers++;
}

CreateCustomer_WithCredentials(struct Customer customer, char* name, int* credentials) 
{
	for(i=0;i<NUM_CLERK_TYPES;i++) {
	  customer->credentials[i] = credentials[i];
	}
    customer->id =numCustomers;
	customer->ssn = numCustomers + 1000;
	name = new char[strlen(name) + 16];
	sPrint(customer->name, "%s%i",name,id);
	customer->money =  100 + 500*(rand() % 4);/*init money increments of 100,600,1100,1600*/
	customer->myLine = -1;
	customer->rememberLine = false;
	customer->isSenator = false;
	customer->numCustomers++;
}

bool isNextClerkType(struct Customer customer, int clerk_type)
{
    /*for adding customers who go out of order, we can add in a random number check that returns true randomly*/

    /*check whether clerk is valid next type (check credentials or keep a status)*/
    if (!customer->credentials[clerk_type]) /*credentials is what you have i.e. picture etc. (int[NUM_CLERKS]) w/ index corrosponding to clerk type, 0=have 1=don't have*/
    {
        if (clerk_type == PICTURE_CLERK_TYPE || clerk_type == APPLICATION_CLERK_TYPE)/*application and picture need no prior credentials*/
            return true;
        
        if (clerk_type == PASSPORT_CLERK_TYPE) /*passport clerk requires both application and pictue*/
        {
            if (customer->credentials[APPLICATION_CLERK_TYPE] && customer->credentials[PICTURE_CLERK_TYPE]) 
                return true;
            
            return false;
        }
        if (clerk_type == CASHIER_CLERK_TYPE && customer->credentials[PASSPORT_CLERK_TYPE]) /*cashier requires only passport (ASSUMPTION?)*/
        {
            return true;
	}
       } 
        return false;  
}

void giveData(struct Customer customer, )
{
	switch (clerks[customer->myLine]->type) {
	  case A:
		Print("%s: may I have application please?\n", customer->name);
		if(customer->isBribing)
		  customer->money-=500;
		  totalEarnings[APPLICATION_CLERK_TYPE] += 500;
		break;

	  case PICTURE_CLERK_TYPE:
		/*ask for a picture to be taken*/
		Print("%s: i would like a picture\n", customer->name);
		if(customer->isBribing)
		  customer->money-=500;
		  totalEarnings[PICTURE_CLERK_TYPE] += 500;
		break;

	  case PASSPORT_CLERK_TYPE:
		Print("%s: ready for my passport\n", customer->name);
		if(customer->isBribing)
		  customer->money-=500;
		  totalEarnings[PASSPORT_CLERK_TYPE] += 500;
		break;

	  case CASHIER_CLERK_TYPE:
		Print("%s has given Cashier ", customer->name);
		Print("%s $100\n", clerks[customer->myLine]->name);
		customer->money-=100;
		totalEarnings[CASHIER_CLERK_TYPE] += 100;
		break;
	}
}


void checkSenator()
{
	/*if there is a Senator in the building wait until he's gone*/
	if (senatorInBuilding){
		Print("%s: waiting outside\n", name);
		/* TODO:rework this to lock
		senatorSemaphore->P();
		senatorSemaphore->V();
		*/
		/*go back inside*/
	}
}

void Customer_Run(struct Customer* customer)
{
  while(true)
  {

	Acquire(clerkLineLock);/*im going to consume linecount values, this is a CS*/
	pickLine(customer);
	/*now, myLine is the index of the shortest line*/
	/*if the clerk is busy or on break, get into line*/
	if (clerkState[customer->myLine] != 0) {
	  if(customer->isBribing)
	    {
	      Print("%s has gotten in a bribe line for ",30, customer->name, "");
	      Print("%s\n", 4, clerks[customer->myLine]->name,"");/*feels wrong*/
	      clerkBribeLineCount[customer->myLine]++;
	    }
	  else
	    {
	      Print("%s has gotten in a regular line for ",30, customer->name,"");
	      Print("%s\n",4, clerks[customer->myLine]->name,"");/*feels wrong*/
	      clerkLineCount[customer->myLine]++;
	    }
		Print("%s: waiting in line for %s\n", 24, name, clerks[customer->myLine]->name);
		if(customer->isBribing)
		  {
		    Wait(clerkBribeLineCV[customer->myLine], clerkLineLock);
		    Acquire(clerkLineLock);   
		    clerkBribeLineCount[customer->myLine]--;
		    PrintInt("bribe line%i count: %i",22, customer->myLine, clerkBribeLineCount[customer->myLine]);
		  }
		else
		  {
		    Wait(clerkLineCV[customer->myLine], clerkLineLock);
		    Acquire(clerkLineLock);   
		    clerkLineCount[customer->myLine]--;
		    PrintInt("regular line%i count: %i", 25, customer->myLine, clerkLineCount[customer->myLine]);
		  }
		if (senatorInBuilding) {
		  customer->rememberLine = true;/*you're in line being kicked out by senatr. senator can't kick self out*/
  			/*make sure to signal senator who may be in line */
			if(customer->isBribing) {
			  clerkBribeLineCV[customer->myLine]->Signal(clerkLineLock);
			}
			else {
			  clerkLineCV[customer->myLine]->Signal(clerkLineLock);
			}
			  Release(clerkLineLock);
		  }

		/*senator may have sent everyone out of lineCV so this nesting is for getting back in line	*/
		checkSenator(); /*after this point senator is gone- get back in line if you were kicked out*/
		if (customer->rememberLine) {
			Acquire(clerkLineLock);
			/*you may be the first one in line now so check. in the case that you were senator you wouldn't remember line */
			if (clerkState[customer->myLine] != 0) {
			  if(customer->isBribing)
			    {
			      clerkBribeLineCount[customer->myLine]++;
			       clerkBribeLineCV[customer->myLine]->Wait(clerkLineLock);
			      clerkBribeLineCount[customer->myLine]--;
			    }
			  else
			    {
			      clerkLineCount[customer->myLine]++;
				Print("%s: waiting in line for %s\n", 24, customer->name, clerks[customer->myLine]->name);
				Wait(clerkLineCV[customer->myLine]clerkLineLock);
				clerkLineCount[customer->myLine]--;
			    }
			}
		}
		/*at this point we assume won't have to go outside till finished with current clerk*/
	}

	clerkState[customer->myLine] = 1;
	Release(clerkLineLock);/*i no longer need to consume lineCount values, leave this CS*/

	Acquire(clerkLock[customer->myLine]);/*we are now in a new CS, need to share data with my clerk*/
	clerkCurrentCustomerSSN[customer->myLine] = customer->ssn;
	Print("%s has given SSN ",16, customer->name,"");
	PrintInt("%d",2, customer->ssn,0);
	Print("to %s\n", 8, clerks[customer->myLine]->name, "");
	clerkCurrentCustomer[customer->myLine] = id;
	giveData(customer, clerks[customer->myLine]->type);
	customer->isBribing = false;
	Signal(clerkCV[customer->myLine],clerkLock[customer->myLine]);
	/*now we wait for clerk to do job*/
	Wait(clerkCV[customer->myLine], clerkLock[customer->myLine]);
	
	/*set credentials*/
	customer->credentials[clerks[customer->myLine]->type] = true;
	Print("%s: Thank you %s\n", 16, name, clerks[customer->myLine]->name);

	if (clerks[customer->myLine]->type == PICTURE_CLERK_TYPE) {
	  /*check if I like my photo RANDOM VAL*/
	  int picApproval = rand() % 10;/*generate random num between 0 and 10*/
	  if(picApproval >8)
	    {
	      Print("%s: does like their picture from ", 26, customer->name, "");
	      Print("%s\n", 4, clerks[customer->myLine]->name,"");
	      /*store that i have pic*/
	    }
	  else
	    {
	      Print("%s: does not like their picture from ",30, customer->name, "");
	      Print("%s, please retake\n", 16, clerks[customer->myLine]->name, "");
	      /*_credentials[type] = false;/*lets seeye*/
	    }
	}
	Signal(clerkCV[customer->myLine], clerkLock[customer->myLine]);

	customer->myLine = -1;
	customer->rememberLine = false;
	
	/*chose exit condition here*/
	if(_credentials[CASHIER_CLERK_TYPE])
	  break;
  }
  Print("%s: IS LEAVING THE PASSPORT OFFICE\n", 36, customer->name, "");
}
int testLine = 69;
void pickLine(struct Customer* customer)
{
  /* isBribing = false;*/
  if (!customer->rememberLine)/*if you don't have to remember a line, pick a new one*/
  {
	  customer->myLine = -1;
	  int lineSize = 1001;
	  for(i = 0; i < NUM_CLERKS; i++)
	    {
		  /*check if the type of this line is something I need! TODO*/
		if(clerks[i] != NULL && isNextClerkType(clerks[i]->type)) {
		  if(clerkLineCount[i] < lineSize )/*&& clerkState[i] != 2)*/
		    {		      
			  customer->myLine = i;
		      lineSize = clerkLineCount[i];
		    }
		}
	    }
	  int desireToBribe = rand() % 10;
	  /*if i want to bribe, let's lock at bribe lines*/
	  if(customer->money > 600 && desireToBribe > 8)
	    {
	      for(i = 0; i < NUM_CLERKS; i++)
			{
				if(clerks[i] != NULL && isNextClerkType(clerks[i]->type)) 
				{
				  if(clerkBribeLineCount[i] <=  lineSize)/*for TESTING. do less than only for real*/
					{
					  /*Print("%s: I'm BRIBING\n", name);*/
					  customer->money -= 500;
					  customer->myLine = i;
					  customer->isBribing = true;
					  lineSize = clerkBribeLineCount[i];
					}
				}
			}
	    }
  }
}


Customer* senators[NUM_SENATORS];

void Senator_EnterOffice(struct Customer* senator)
{
  /*walk in acquire all clerk locks to prevent next in line from getting to clerk*/
  senatorSemaphore->P(); /*use to lock down entire run section*/
  senatorInBuilding = true;/*signals all people waiting to exit*/
  
  Acquire(clerkLineLock); /*acquire to broadcast current customers. released in Customer::run()*/
  for (i=0; i<NUM_CLERKS;i++) {
    if (clerkLineCV[i] != NULL) {
		Broadcast(clerkLineCV[i],clerkLineLock); /*clear out lines. they will stop because of semaphore after leaving line/before returning to it*/
    }

    if (clerkBribeLineCV[i] != NULL) {
	clerkBribeLineCV[i]->Broadcast(clerkLineLock); /*clear out lines. they will stop because of semaphore after leaving line/before returning to it*/
    }
  }
 
  Release(clerkLineLock);
  for (i=0; i<NUM_CLERKS;i++) {
	/*wait for bribe line to empty*/
	while(clerkBribeLineCount[i] > 0) {
	  Acquire(clerkLineLock);
	  clerkBribeLineCV[i]->Signal(clerkLineLock);
	  clerkBribeLineCV[i]->Wait(clerkLineLock);
	}
	/*wait for regular line to empty*/
	while (clerkLineCount[i] > 0) {
	  Signal(clerkLineCV[i], clerkLineLock);
	  Wait(clerkLineCV[i], clerkLineLock);
	}
  }
  /*everyone is out of lines now wait for clerks to finish*/
  for (i=0; i<NUM_CLERKS;i++) {
    if (clerkLock[i] != NULL) {
		while (clerkState[i] == 1) {
			clerkLock[i]->Acquire();
			Signal(clerkCV[i], clerkLock[i]);
			Wait(clerkCV[i], clerkLock[i]);
		}
    }
  }
}

void Senator_ExitOffice(struct Customer* senator)
{
  senators.pop();/*remove self from senator q*/
  senatorInBuilding = false;
  senatorSemaphore->V();
}

void Senator_Run(struct Customer* senator)
{
	/*enter facility*/
	Senator_EnterOffice(senator);
	/*proceed as a normal customer*/
	Customer_Run(senator);
	/*exit facility*/
	Senator_ExitOffice(senator);
}

struct Manager
{
  char* name;
};

void OutputEarnings()
{
	int total = 0;
	for (i =0; i < NUM_CLERK_TYPES; i++) {
		total += totalEarnings[i];
	}
	Print("\n----Earnings report:---- \n");
	Print("ApplicationClerks: %i \n",totalEarnings[APPLICATION_CLERK_TYPE]);
	Print("PictureClerks: %i \n",totalEarnings[PICTURE_CLERK_TYPE]);
	Print("PassportClerks: %i \n",totalEarnings[PASSPORT_CLERK_TYPE]);
	Print("Cashiers: %i \n",totalEarnings[CASHIER_CLERK_TYPE]);
	Print("TOTAL: %i \n------------------------\n\n",total);
}

void Manager_Run(struct Manager* manager)
{
  while(true) {
	for (i = 0; i < 1000; i++)
		Yield();
	for (i = 0; i < NUM_CLERKS; i++)
	{
		if (clerkState[i] == 2 && (clerkLineCount[i] >= 3 || clerkBribeLineCount[i] >= 1 || senatorInBuilding) )
		{
			/*wake up clerk*/
			Acquire(clerkLock[i]);	
			Print("%s waking up %s\n", 16, manager->name, clerks[i]->name);
			clerkState[i] = 0;/*set to available	*/
			Signal(clerkBreakCV[i], clerkLock[i]);	
			Release(clerkLock[i]);	
		}
	}
	OutputEarnings();
	if (numCustomers == 0) {
		break;
	}
  }
}
/*----------------------------------------------------------------------
/* SimpleThread
/* 	Loop 5 times, yielding the CPU to another ready thread 
/*	each iteration.
/*
/*	"which" is simply a number identifying the thread, for debugging
/*	purposes.
/*----------------------------------------------------------------------
*/
void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	Print("*** thread %d looped %d times\n", which, num);
        Yield()();
    }
}

/*----------------------------------------------------------------------
/* ThreadTest
/* 	Set up a ping-pong between two threads, by forking a thread 
/*	to call SimpleThread, and then calling SimpleThread ourselves.
/*----------------------------------------------------------------------
*/
void
ThreadTest()
{
    DEBUG('t', "Entering SimpleTest");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}

/* --------------------------------------------------
/* Test Suite
/* --------------------------------------------------
*/
/*--------------------------------------------------
/*Part 2 tests
/*--------------------------------------------------
*/
/*-------------------------------------------------
/*Simple pictureClerk test, by Jack
/*custoomer just goes, gets pic, done
/*-------------------------------------------------
*/
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


/* --------------------------------------------------
/* Test 1 - see TestSuite() for details
/* --------------------------------------------------
*/
Semaphore t1_s1("t1_s1",0);       /* To make sure t1_t1 acquires the*/
                                  /* lock before t1_t2*/
Semaphore t1_s2("t1_s2",0);       /* To make sure t1_t2 Is waiting on the */
                                  /* lock before t1_t3 releases it*/
Semaphore t1_s3("t1_s3",0);       /* To make sure t1_t1 does not release the*/
                                  /* lock before t1_t3 tries to acquire it*/
Semaphore t1_done("t1_done",0);   /* So that TestSuite knows when Test 1 is*/
                                  /* done*/
Lock t1_l1("t1_l1");		  /* the lock tested in Test 1*/

/* --------------------------------------------------
/* t1_t1() -- test1 thread 1
/*     This is the rightful lock owner
/* --------------------------------------------------
*/
void t1_t1() {
    t1_l1.Acquire();
    t1_s1.V();  /* Allow t1_t2 to try to Acquire Lock*/
 
    Print ("%s: Acquired Lock %s, waiting for t3\n",currentThread.name,
	    t1_l1.getName());
    t1_s3.P();
    Print ("%s: working in CS\n",currentThread.name);
    for (i = 0; i < 1000000; i++) ;
    Print ("%s: Releasing Lock %s\n",currentThread.name,
	    t1_l1.getName());
    t1_l1.Release();
    t1_done.V();
}

/* --------------------------------------------------
/* t1_t2() -- test1 thread 2
/*     This thread will wait on the held lock.
/* --------------------------------------------------
*/
void t1_t2() {

    t1_s1.P();	/* Wait until t1 has the lock*/
    t1_s2.V();  /* Let t3 try to acquire the lock*/

    Print("%s: trying to acquire lock %s\n",currentThread.name,
	    t1_l1.getName());
    t1_l1.Acquire();

    Print ("%s: Acquired Lock %s, working in CS\n",currentThread.name,
	    t1_l1.getName());
    for (i = 0; i < 10; i++)
	;
    Print ("%s: Releasing Lock %s\n",currentThread.name,
	    t1_l1.getName());
    t1_l1.Release();
    t1_done.V();
}

/* --------------------------------------------------
/* t1_t3() -- test1 thread 3
/*     This thread will try to release the lock illegally
/* --------------------------------------------------
*/
void t1_t3() {

    t1_s2.P();	/* Wait until t2 is ready to try to acquire the lock*/

    t1_s3.V();	/* Let t1 do it's stuff*/
    for ( i = 0; i < 3; i++ ) {
	Print("%s: Trying to release Lock %s\n",currentThread.name,
	       t1_l1.getName());
	t1_l1.Release();
    }
}

/* --------------------------------------------------
/* Test 2 - see TestSuite() for details
/* --------------------------------------------------
*/
Lock t2_l1("t2_l1");		/* For mutual exclusion*/
Condition t2_c1("t2_c1");	/* The condition variable to test*/
Semaphore t2_s1("t2_s1",0);	/* To ensure the Signal comes before the wait*/
Semaphore t2_done("t2_done",0);     /* So that TestSuite knows when Test 2 is done*/

/* --------------------------------------------------
/* t2_t1() -- test 2 thread 1
/*     This thread will signal a variable with nothing waiting
/* --------------------------------------------------
*/
void t2_t1() {
    t2_l1.Acquire();
    Print("%s: Lock %s acquired, signalling %s\n",currentThread.name,
	   t2_l1.getName(), t2_c1.getName());
    t2_c1.Signal(&t2_l1);
    Print("%s: Releasing Lock %s\n",currentThread.name,
	   t2_l1.getName());
    t2_l1.Release();
    t2_s1.V();	/* release t2_t2*/
    t2_done.V();
}

/* --------------------------------------------------
/* t2_t2() -- test 2 thread 2
/*     This thread will wait on a pre-signalled variable
/* --------------------------------------------------
*/
void t2_t2() {
    t2_s1.P();	/* Wait for t2_t1 to be done with the lock*/
    t2_l1.Acquire();
    Print("%s: Lock %s acquired, waiting on %s\n",currentThread.name,
	   t2_l1.getName(), t2_c1.getName());
    t2_c1.Wait(&t2_l1);
    Print("%s: Releasing Lock %s\n",currentThread.name,
	   t2_l1.getName());
    t2_l1.Release();
}
/* --------------------------------------------------
/* Test 3 - see TestSuite() for details
/* --------------------------------------------------
*/
Lock t3_l1("t3_l1");		/* For mutual exclusion*/
Condition t3_c1("t3_c1");	/* The condition variable to test*/
Semaphore t3_s1("t3_s1",0);	/* To ensure the Signal comes before the wait
Semaphore t3_done("t3_done",0); /* So that TestSuite knows when Test 3 is
                                /* done

/* --------------------------------------------------
/* t3_waiter()
/*     These threads will wait on the t3_c1 condition variable.  Only
/*     one t3_waiter will be released
/* --------------------------------------------------
void t3_waiter() {
    t3_l1.Acquire();
    t3_s1.V();		/* Let the signaller know we're ready to wait
    Print("%s: Lock %s acquired, waiting on %s\n",currentThread.name,
	   t3_l1.getName(), t3_c1.getName());
    t3_c1.Wait(&t3_l1);
    Print("%s: freed from %s\n",currentThread.name, t3_c1.getName());
    t3_l1.Release();
    t3_done.V();
}


/* --------------------------------------------------
/* t3_signaller()
/*     This threads will signal the t3_c1 condition variable.  Only
/*     one t3_signaller will be released
/* --------------------------------------------------
void t3_signaller() {

    /* Don't signal until someone's waiting
    
    for ( i = 0; i < 5 ; i++ ) 
	t3_s1.P();
    t3_l1.Acquire();
    Print("%s: Lock %s acquired, signalling %s\n",currentThread.name,
	   t3_l1.getName(), t3_c1.getName());
    t3_c1.Signal(&t3_l1);
    Print("%s: Releasing %s\n",currentThread.name, t3_l1.getName());
    t3_l1.Release();
    t3_done.V();
}
 
/* --------------------------------------------------
/* Test 4 - see TestSuite() for details
/* --------------------------------------------------
Lock t4_l1("t4_l1");		/* For mutual exclusion
Condition t4_c1("t4_c1");	/* The condition variable to test
Semaphore t4_s1("t4_s1",0);	/* To ensure the Signal comes before the wait
Semaphore t4_done("t4_done",0); /* So that TestSuite knows when Test 4 is
                                /* done

/* --------------------------------------------------
/* t4_waiter()
/*     These threads will wait on the t4_c1 condition variable.  All
/*     t4_waiters will be released
/* --------------------------------------------------
void t4_waiter() {
    t4_l1.Acquire();
    t4_s1.V();		/* Let the signaller know we're ready to wait
    Print("%s: Lock %s acquired, waiting on %s\n",currentThread.name,
	   t4_l1.getName(), t4_c1.getName());
    t4_c1.Wait(&t4_l1);
    Print("%s: freed from %s\n",currentThread.name, t4_c1.getName());
    t4_l1.Release();
    t4_done.V();
}


/* --------------------------------------------------
/* t2_signaller()
/*     This thread will broadcast to the t4_c1 condition variable.
/*     All t4_waiters will be released
/* --------------------------------------------------
void t4_signaller() {

    /* Don't broadcast until someone's waiting
    
    for ( i = 0; i < 5 ; i++ ) 
	t4_s1.P();
    t4_l1.Acquire();
    Print("%s: Lock %s acquired, broadcasting %s\n",currentThread.name,
	   t4_l1.getName(), t4_c1.getName());
    t4_c1.Broadcast(&t4_l1);
    Print("%s: Releasing %s\n",currentThread.name, t4_l1.getName());
    t4_l1.Release();
    t4_done.V();
}
/* --------------------------------------------------
/* Test 5 - see TestSuite() for details
/* --------------------------------------------------
Lock t5_l1("t5_l1");		/* For mutual exclusion
Lock t5_l2("t5_l2");		/* Second lock for the bad behavior
Condition t5_c1("t5_c1");	/* The condition variable to test
Semaphore t5_s1("t5_s1",0);	/* To make sure t5_t2 acquires the lock after
                                /* t5_t1

/* --------------------------------------------------
/* t5_t1() -- test 5 thread 1
/*     This thread will wait on a condition under t5_l1
/* --------------------------------------------------
void t5_t1() {
    t5_l1.Acquire();
    t5_s1.V();	/* release t5_t2
    Print("%s: Lock %s acquired, waiting on %s\n",currentThread.name,
	   t5_l1.getName(), t5_c1.getName());
    t5_c1.Wait(&t5_l1);
    Print("%s: Releasing Lock %s\n",currentThread.name,
	   t5_l1.getName());
    t5_l1.Release();
}

/* --------------------------------------------------
/* t5_t1() -- test 5 thread 1
/*     This thread will wait on a t5_c1 condition under t5_l2, which is
/*     a Fatal error
/* --------------------------------------------------
void t5_t2() {
    t5_s1.P();	/* Wait for t5_t1 to get into the monitor
    t5_l1.Acquire();
    t5_l2.Acquire();
    Print("%s: Lock %s acquired, signalling %s\n",currentThread.name,
	   t5_l2.getName(), t5_c1.getName());
    t5_c1.Signal(&t5_l2);
    Print("%s: Releasing Lock %s\n",currentThread.name,
	   t5_l2.getName());
    t5_l2.Release();
    Print("%s: Releasing Lock %s\n",currentThread.name,
	   t5_l1.getName());
    t5_l1.Release();
}
/*---------------------------------------------------
/* Repeatable test code
/*---------------------------------------------------
/*1*/
void p1Test(){
	    /* Test 1
	Thread *t;
	i;
	char * name;	
    Print("Starting Test 1\n");
    t = new Thread("t1_t1");
    t->Fork((VoidFunctionPtr)t1_t1,0);

    t = new Thread("t1_t2");
    t->Fork((VoidFunctionPtr)t1_t2,0);

    t = new Thread("t1_t3");
    t->Fork((VoidFunctionPtr)t1_t3,0);

    /* Wait for Test 1 to complete
    Print("waiting for Test 1 to complete...\n");
    for ( i = 0; i < 2; i++ )
	t1_done.P();

    /* Test 2

    Print("Starting Test 2.  Note that it is an error if thread t2_t2 ");
    Print("completes!\n");
    
    t = new Thread("t2_t1");
    Print("created new thread t2_t1");
    t->Fork((VoidFunctionPtr)t2_t1,0);
    Print("forked t2_t1");
    t = new Thread("t2_t2");
    t->Fork((VoidFunctionPtr)t2_t2,0);
    Print("forked t2_t2");
    /* Wait for Test 2 to complete
    Print("waiting for test 2 to complete");
    t2_done.P();

    /* Test 3

    Print("Starting Test 3\n");

    for (  i = 0 ; i < 5 ; i++ ) {
	name = new char[20];
	sPrint(name,"t3_waiter%d",i);
	t = new Thread(name);
	t->Fork((VoidFunctionPtr)t3_waiter,0);
    }
    t = new Thread("t3_signaller");
    t->Fork((VoidFunctionPtr)t3_signaller,0);

    /* Wait for Test 3 to complete
    for (  i = 0; i < 2; i++ )
	t3_done.P();

    /* Test 4

    Print("Starting Test 4\n");

    for (  i = 0 ; i < 5 ; i++ ) {
	name = new char[20];
	sPrint(name,"t4_waiter%d",i);
	t = new Thread(name);
	t->Fork((VoidFunctionPtr)t4_waiter,0);
    }
    t = new Thread("t4_signaller");
    t->Fork((VoidFunctionPtr)t4_signaller,0);

    /* Wait for Test 4 to complete
    for (  i = 0; i < 6; i++ )
	t4_done.P();

    /* Test 5

    Print("Starting Test 5.  Note that it is an error if thread t5_t1\n");
    Print("completes\n");

    t = new Thread("t5_t1");
    t->Fork((VoidFunctionPtr)t5_t1,0);

    t = new Thread("t5_t2");
    t->Fork((VoidFunctionPtr)t5_t2,0);
}
/*1*/
void shortLineTest()
{
	/*instantiate two customer threads
	Thread *t = new Thread("clerk1");
	t->Fork((VoidFunctionPtr) p2_pictureClerk, 0);
	Print("Clerk 1 created \n");
	Print("Creating two customers:\n");	
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
  /*need at least 3 to so cashier doesn't go on break
  for (i=0; i < 3; i++){
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
	/*wait for clerks to go to sleep
	for(i=0; i < 200; i++)
	  Yield()();
	/*need at least 3 to so cashier doesn't go on break
	for (i=0; i < 3; i++){
	  t = new Thread("c1");
	  t->Fork((VoidFunctionPtr) p2_customer, 0);
	}


}
/*6*/
int salesUpdate()
{
  int temp = 0;
  	for (i =0; i < NUM_CLERK_TYPES; i++) {
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
  /*t = new Thread("cash0");
  /*t->Fork((VoidFunctionPtr) p2_cashierClerk, 0);
  t = new Thread("cust");
  for(i = 0; i <updateCount; i++)
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
	  Print("****SALES UPDATED: ONLY %d CUSTOMERS LEFT TO UPDATE SALES*****\n", updateCount);
	}
      Yield()();
    }
  Print("TEST COMPLETE\n");



}

/*7*/
void senatorTest()
{
	/*initiate all clerks
	Thread *t = new Thread("clerk");
	t->Fork((VoidFunctionPtr) p2_pictureClerk, 0);
	t = new Thread("clerk2");
	t->Fork((VoidFunctionPtr) p2_applicationClerk, 0);
	t = new Thread("clerk3");
	t->Fork((VoidFunctionPtr) p2_passportClerk, 0);
	t = new Thread("clerk4");
	t->Fork((VoidFunctionPtr) p2_cashierClerk, 0);
	
	/*initiate some customers
	/*need at least 3 to so cashier doesn't go on break
	for (i=0; i < 3; i++){
	  t = new Thread("c1");
	  t->Fork((VoidFunctionPtr) p2_customer, 0);
	}

	/*initiate senators to test senator effectiveness/consecutive senators
	t = new Thread("senator");
	t->Fork((VoidFunctionPtr) p2_senator, 0);
	t = new Thread("senator");
	t->Fork((VoidFunctionPtr) p2_senator, 0);
/*
	t = new Thread("manager");
	t->Fork((VoidFunctionPtr) p2_manager, 0);
*/
}
/* --------------------------------------------------
/* TestSuite()
/*     This is the main thread of the test suite.  It runs the
/*     following tests:
/*
/*       1.  Show that a thread trying to release a lock it does not
/*       hold does not work
/*
/*       2.  Show that Signals are not stored -- a Signal with no
/*       thread waiting is ignored
/*
/*       3.  Show that Signal only wakes 1 thread
/*
/*	 4.  Show that Broadcast wakes all waiting threads
/*
/*       5.  Show that Signalling a thread waiting under one lock
/*       while holding another is a Fatal error
/*
/*     Fatal errors terminate the thread in question.
/* --------------------------------------------------
void TestSuite() {
  
  Print("Test Suite has started! Start the trials of pain\n\n");
	Print("Repeatable tests:\n");
	Print("0) Run part 1 tests (locks and conditions).\n");
	Print("1) Prove that no 2 customers ever choose the same shortest line at the same time.\n");
	Print("2) Prove that managers only read from one Clerk's total money received, at a time\n");
	Print("3) Prove that Customers do not leave until they are given their passport by the Cashier. The Cashier does not start on another customer until they know that the last Customer has left their area.\n");
	Print("4) Prove that Clerks go on break when they have no one waiting in their line.\n");
	Print("5) Prove that Managers get Clerks off their break when lines get too long.\n");
	Print("6) Prove that total sales never sufers from a race condition.\n");
	Print("7) Prove that Customers behave properly when Senators arrive.\n");
	Print("Enter a number between 1 and 7 to choose a test to run, or type s to begin the big system test.\n"); 
	char entry;
	scanf("%c", &entry);
	Print("You chose %c \n", entry);	
	
	int clerkNumArray[4];
	/*instantiate all clerk locks (max number)
	for( i = 0; i < NUM_CLERKS; i++)
      {
	char* buffer1 = new char[50];
	sPrint(buffer1, "ClerkLock%i", i);
	clerkLock[i] = CreateLock(buffer1);
	/*clerks[i] =new  Clerk();
      }
	if(entry != 's')
	{
		int num = (int)entry - 48 ;
	/*remove \n from entry
		char garbage;
		scanf("%c", &garbage);	
		if (num>7 || num < 0)
		{
			Print("Please enter a valid entry.\n");
		} 
		else
		{
			Thread *t;
			if (num == 0)
				p1Test();
			if (num == 1)
				shortLineTest();
			else if (num ==2) {
				Print("Because we implemented the money count not through individual member variables, but in a monitor variable that customers directly access, there will never be a race condition in this instance.");
			}
			else if (num == 3) {cashierTest();}
			else if (num == 4) {
				clerkWaitTest();
			}
			else if (num == 5) {managerWakeTest();}
			else if (num == 6) {salesRCTest();}
			else if (num == 7) {senatorTest();}	
			Print("Test completed. ");
		}
			Print("\n");
	}
	else
	{
		int numCustomersInput;
		Print("Enter number of Picture Clerks (between 1 and 5): ");
		scanf("%d", &clerkNumArray[PICTURE_CLERK_TYPE]);
		Print("\nEnter number of Application Clerks (between 1 and 5): ");
		scanf("%d", &clerkNumArray[APPLICATION_CLERK_TYPE]);
		Print("\nEnter number of Passport Clerks (between 1 and 5): ");
		scanf("%d", &clerkNumArray[PASSPORT_CLERK_TYPE]);
		Print("\nEnter number of Cashiers (between 1 and 5): ");
		scanf("%d", &clerkNumArray[CASHIER_CLERK_TYPE]);
		Print("\nEnter number of Customers (between 20 and 50): ");
		scanf("%d", &numCustomersInput);
		/*test: print array to see if stored correctly
		for (i = 0; i < NUM_CLERK_TYPES; i++)
		{
			/*do something, add them to an array? or count?
		}
		Thread *t;
	    char* name;
	    int threadid = 0;
	    i;
	    Print("starting MultiClerk test");

	    for (i = 0; i < clerkNumArray[PICTURE_CLERK_TYPE]; i++)
	    {
		char* buffer1 = new char[5];
		sPrint(buffer1, "pic%i", threadid);
		t = new Thread(buffer1);
		t->Fork((VoidFunctionPtr) p2_pictureClerk, 0);
		threadid++;
	    }
	    for (i = 0; i < clerkNumArray[APPLICATION_CLERK_TYPE];i++ )
	    {
		char* buffer1 = new char[5];
		sPrint(buffer1, "app%i", threadid);
		t = new Thread(buffer1);
		t->Fork((VoidFunctionPtr) p2_applicationClerk, 0);
		threadid++;
	    }
	    for (i = 0; i < clerkNumArray[PASSPORT_CLERK_TYPE];i++)
	    {
		char* buffer1 = new char[5];
		sPrint(buffer1, "pas%i", threadid);
		t = new Thread(buffer1);
		t->Fork((VoidFunctionPtr) p2_passportClerk, 0);
		threadid++;
	    }
	    for (i = 0; i < clerkNumArray[CASHIER_CLERK_TYPE]; i++)
	    {
		char* buffer1 = new char[5];
		sPrint(buffer1, "cas%i", threadid);
		t = new Thread(buffer1);
		t->Fork((VoidFunctionPtr) p2_cashierClerk,0);
		threadid++;
	    }
	  for (i = 0; i< numCustomersInput; i++) { 
		char* buffer1 = new char[5];
		sPrint(buffer1, "cus%i", threadid);
		t = new Thread(buffer1);
		/*t = new Thread(threadid);
	    t->Fork((VoidFunctionPtr) p2_customer,0);
	    threadid++;
	  }
	  /*new senator thread
	  	char* buffer1 = new char[10];
		sPrint(buffer1, "senator%i", threadid);
		threadid++;
		t = new Thread(buffer1);
		t->Fork((VoidFunctionPtr) p2_senator, 0);

		buffer1 = new char[10];
		sPrint(buffer1, "manager%i", threadid);
		t = new Thread(buffer1);
		t->Fork((VoidFunctionPtr) p2_manager,0);
	}/*end else statement

    return;/*TODO remove after testing
    


}

/*********************************************
 *Project 2 tests BELOW. triggered with -P2
 *
 *
 *********************************************
 */
void Part2()
{
  Print("/nSTARTING PROJECT 2 TEST SUITE/n");
}
#endif

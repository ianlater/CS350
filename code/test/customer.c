/* customer.c
   includes senators
*/
#include "syscall.h"
#include "setup.h"
int id, money, ssn, myLine;
bool credentials[4]; 
bool rememberLine, isSenator;
/*
ARRAYS:
	c_id
	c_ssn
	c_name
*/
/* creates new customer w/ given name. should declare new customer ahead of time and place where needed. this just fills in info*/
int CreateCustomer() 
{	
	
	Acquire(createLock);
	id = GetMonitor(customersInBuilding,0);
	this.id = id;
	SetMonitor(c_id, id, id);
	SetMonitor(c_ssn, id, id + 1000);
	this.ssn = id + 1000;
	/*strcpy(customers[customersInBuilding].name, name);
	strcat(customers[customersInBuilding].name, customers[customersInBuilding].id);*/
	this.name = name;
	this.money =  100 + 500*(Rand() % 4);/*init money increments of 100,600,1100,1600*/
	SetMonitor(c_money, id, this.money);
	this.myLine = -1;
	this.rememberLine = false;
	this.isSenator = false;
	/*strcpy(customers[customersInBuilding].name, name);
	strcat(customers[customersInBuilding].name, customers[customersInBuilding].id);*/
	Release(createLock);
	SetMonitor(customersInBuilding, 0, id + 1);
	return id;
}
int CreateCustomer_WithCredentials(int* credentials) 
{
	Acquire(createLock);
	for(i=0;i<NUM_CLERK_TYPES;i++) {
	  this.credentials[i] = credentials[i];
	}
	id = GetMonitor(customersInBuilding,0);
	this.id = id;
SetMonitor(c_id, id, id);
	this.ssn = id + 1000;
	SetMonitor(c_ssn, id, this.ssn);
	/*strcpy(customers[customersInBuilding].name, name);
	strcat(customers[customersInBuilding].name, customers[customersInBuilding].id);*/
	this.money =  100 + 500*(Rand() % 4);/*init money increments of 100,600,1100,1600*/
	SetMonitor(c_money, id, this.money);
	this.myLine = -1;
	this.rememberLine = false;
	this.isSenator = false;
	/*strcpy(customers[customersInBuilding].name, name);
	strcat(customers[customersInBuilding].name, customers[customersInBuilding].id);*/
	Release(createLock);
	SetMonitor(customersInBuilding, 0, id + 1);
	return id;
}

bool isNextClerkType(int clerk_type)
{
    /*for adding customers who go out of order, we can add in a Random number check that returns true Randomly*/

    /*check whether clerk is valid next type (check credentials or keep a status)*/
    if (!this.credentials[clerk_type]) /*credentials is what you have i.e. picture etc. (int[NUM_CLERKS]) w/ index corrosponding to clerk type, 0=have 1=don't have*/
    {
        if (clerk_type == PICTURE_CLERK_TYPE || clerk_type == APPLICATION_CLERK_TYPE)/*application and picture need no prior credentials*/
            return true;
        
        if (clerk_type == PASSPORT_CLERK_TYPE) /*passport clerk requires both application and pictue*/
        {
            if (this.credentials[APPLICATION_CLERK_TYPE] && this.credentials[PICTURE_CLERK_TYPE]) 
                return true;
            
            return false;
        }
        if (clerk_type == CASHIER_CLERK_TYPE && this.credentials[PASSPORT_CLERK_TYPE]) /*cashier requires only passport (ASSUMPTION?)*/
        {
            return true;
	}
       } 
        return false;  
}

void giveData()
{
	switch (GetMonitor(clerkTypes, this.myLine)) {
	  case APPLICATION_CLERK_TYPE:
		PrintInt("Customer%i: may I have application please?\n", 44,  this.id, 0);
		if(this.isBribing) {
		  this.money-=500;
		  SetMonitor(totalEarnings, APPLICATION_CLERK_TYPE, GetMonitor(totalEarnings, APPLICATION_CLERK_TYPE)+500);
		}
		break;

	  case PICTURE_CLERK_TYPE:
		/*ask for a picture to be taken*/
		PrintInt("Customer%i: I would like a picture\n", 36, this.id, 0);
		if(this.isBribing){
		  this.money-=500;
		  SetMonitor(totalEarnings, PICTURE_CLERK_TYPE, GetMonitor(totalEarnings, PICTURE_CLERK_TYPE)+500);
		}
		break;

	  case PASSPORT_CLERK_TYPE:
		PrintInt("Customer%i: ready for my passport\n", 35, this.id, 0);
		if(this.isBribing){
		  this.money-=500;
		  SetMonitor(totalEarnings, PASSPORT_CLERK_TYPE, GetMonitor(totalEarnings, PASSPORT_CLERK_TYPE)+500);
		}
		break;

	  case CASHIER_CLERK_TYPE:
		PrintInt("Customer%i has given Cashier %i $100\n", 38, this.id, GetMonitor(clerkIds, this.myLine));
		if(this.isBribing){
		  this.money-=500;
		  SetMonitor(totalEarnings, CASHIER_CLERK_TYPE, GetMonitor(totalEarnings, CASHIER_CLERK_TYPE)+500);
		}
		this.money-=100;
		  SetMonitor(totalEarnings, CASHIER_CLERK_TYPE, GetMonitor(totalEarnings, CASHIER_CLERK_TYPE)+100);
		break;
	}
}


void checkSenator(/*struct Customer *customer*/)
{
	/*if there is a Senator in the building wait until he's gone*/
	if (GetMonitor(senatorInBuilding, 0)){
		/*PrintInt("Clerk%i: waiting outside\n",  name);*/
		/* TODO:rework this to lock
		senatorSemaphore->P();
		senatorSemaphore->V();
		*/
		/*go back inside*/
	}
}

int picApproval;
void Customer_Run()
{
  simulationStarted = true;
  SetMonitor(activeCustomers,  0, GetMonitor(activeCustomers, 0)+1);/* regulate w/clerklinelock. senator should check this number and GetMonitor(senatorInBuilding, 0) before  operation and wait in senatorLineCV if both true */
  Release(createLock);
  while(true)
  {
	if(GetMonitor(senatorInBuilding, 0)){
		Acquire(senatorLock);
		SetMonitor(activeCustomers,  0, GetMonitor(activeCustomers, 0)-1);
		if(GetMonitor(activeCustomers, 0) == 0){ /* if you're last to go outside signal first senator to come in */
			Signal(senatorLock, SenatorLineCV);
		}
		Wait(senatorLock, OutsideCV);
		SetMonitor(activeCustomers,  0, GetMonitor(activeCustomers, 0)+1); /* come back inside */
		Release(senatorLock);
	}
	Acquire(clerkLineLock);/*im going to consume linecount values, this is a CS*/
	pickLine();
	/*now, myLine is the index of the shortest line*/
	/*if the clerk is busy or on break, get into line*/
	if (GetMonitor(clerkState, myLine) != 0) {
		if(isBribing)
		  {
			PrintInt("Customer%i has gotten in a bribe line for Clerk%i\n",51, id, GetMonitor(ClerkIds, myLine));
			SetMonitor(clerkBribeLineCount, myLine, GetMonitor(clerkBribeLineCount, myLine)+1);
			
		    Wait(clerkLineLock, clerkBribeLineCV[myLine]);
			PrintInt("Customer%i leaving bribe line for Clerk%i\n",43, id, GetMonitor(ClerkIds, myLine));
		    SetMonitor(clerkBribeLineCount, myLine, GetMonitor(clerkBribeLineCount, myLine)-1)
		    PrintInt("bribe line%i count: %i\n",23, myLine, GetMonitor(clerkBribeLineCount, myLine));
		  }
		else
		  {
			PrintInt("Customer%i has gotten in a regular line for Clerk%i\n",53, id, GetMonitor(ClerkIds, myLine));
			SetMonitor(clerkLineCount, myLine, GetMonitor(clerkLineCount, myLine)+1)
			
			Wait(clerkLineLock, clerkLineCV[myLine]);
			PrintInt("Customer%i leaving regular line for Clerk%i\n",45, id, GetMonitor(ClerkIds, myLine));
			SetMonitor(clerkLineCount, myLine, GetMonitor(clerkLineCount, myLine)-1)
			PrintInt("regular line%i count: %i\n", 26, myLine, GetMonitor(clerkLineCount, myLine));
		  }
	}
	if (GetMonitor(senatorInBuilding, 0)) {
		rememberLine = true;/*you're in line being kicked out by senator. */
		Release(clerkLineLock); /* free up your locks */
		continue; /* go back up to beginning of while loop where you will leave building.pickLine should manage line remembering */
	}
	
	SetMonitor(clerkState, myLine, 1);
	Release(clerkLineLock);/*i no longer need to consume lineCount values, leave this CS*/
	
	myClerksLock = GetMonitor(clerkLock, myLine);
	myClerksCV = GetMonitor(clerkCV, myLine);
	Acquire(myClerksLock);/*we are now in a new CS, need to share data with my clerk*/
	SetMonitor(clerkCurrentCustomerSSN, myLine, ssn);
	PrintInt("Customer%i has given SSN %i", 27, id, ssn);
	PrintInt("to Clerk%i\n", 12, GetMonitor(ClerkIds, myLine), 0);
	SetMonitor(clerkCurrentCustomer, myLine, id);
	giveData();
	isBribing = false;
	Signal(myClerksLock, myClerksCV);
	/*now we wait for clerk to do job*/
	Wait(myClerksLock, myClerksCV);
	
	/*set credentials*/
	credentials[GetMonitor(ClerkTypes, myLine)] = true;
	PrintInt("Customer%i: Thank you Clerk%i\n", 31, id,  GetMonitor(ClerkIds, myLine));

	if (GetMonitor(ClerkTypes, myLine) == PICTURE_CLERK_TYPE) {
	  /*check if I like my photo RandOM VAL*/
	  picApproval = Rand() % 10;/*generate Random num between 0 and 10*/
	  if(picApproval >8)
	    {
	      PrintInt("Customer%i: does like their picture from Clerk%i", 48, id, GetMonitor(ClerkIds, myLine));
	      /*store that i have pic*/
	    }
	  else
	    {
	      PrintInt("Customer%i: does not like their picture from Clerk%i, please retake\n",69, id, GetMonitor(ClerkIds, myLine));
	      /*_credentials[type] = false;/*lets seeye*/
	    }
	}
	Signal(myClerksLock, myClerksCV);/*let clerk know you're leaving*/
	Release(myClerksLock);/*give up lock*/
	
	myLine = -1;
	rememberLine = false;
	
	/*chose exit condition here*/
	if(credentials[CASHIER_CLERK_TYPE])
	  break;
  }
  SetMonitor(activeCustomers,  0, GetMonitor(activeCustomers, 0)-1);
  PrintInt("Customer%i: IS LEAVING THE PASSPORT OFFICE\n", 44, id, 0);
  SetMonitor(customersInBuilding, GetMonitor(customersInBuilding)-1);/*assumption: customers are all created at start before any can leave office*/
}
int testLine = 69;
int lineSize = 1001;
int desireToBribe;
void pickLine()
{
  /* isBribing = false;*/
  if (!this.rememberLine)/*if you don't have to remember a line, pick a new one*/
  {
	  this.myLine = -1;
	  lineSize = 1001;
	  for(i = 0; i < NUM_CLERKS; i++)
	    {
		  /*check if the type of this line is something I need! TODO*/
			if(/*clerks[i] != NULL &&*/ isNextClerkType(customer, GetMonitor(clerkTypes, i))) {
			  if(GetMonitor(clerkLineCount, i) < lineSize )/*&& clerkState[i] != 2)*/
				{		      
				  this.myLine = i;
				  lineSize = GetMonitor(clerkLineCount, i);
				}
			}
	    }
	  desireToBribe = Rand() % 10;
	  /*if i want to bribe, let's lock at bribe lines*/
	  if(this.money > 600 && desireToBribe > 8)
	    {
	      for(i = 0; i < NUM_CLERKS; i++)
			{
				if(/*clerks[i] != NULL &&*/ isNextClerkType(customer, GetMonitor(clerkTypes, i))) 
				{
					int theBribeLineCount = GetMonitor(clerkBribeLineCount, i);
				  	if(theBribeLineCount <=  lineSize)/*for TESTING. do less than only for real*/
					{
					  /*PrintInt("Clerk%i: I'm BRIBING\n", name);*/
					  this.money -= 500;
					  this.myLine = i;
					  this.isBribing = true;
					  lineSize = theBribeLineCount;
					}
				}
			}
	    }
  }
}

int main()
{
	Customer_Run();
	Exit(0);
}

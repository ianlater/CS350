/* customer.c
   includes senators
*/
#include "syscall.h"
#include "setup.h"
int id, money, ssn, myLine;
bool credentials[4]; 
bool rememberLine, isSenator;//index of customer arrays to use
//instead of an array of customers struct, use one array for each customers field
/*
ARRAYS:
	c_id
	c_ssn
	c_name
*/
/* creates new customer w/ given name. should declare new customer ahead of time and place where needed. this just fills in info*/
int CreateCustomer(char* name) 
{	
	
	//need to acquire createLock before creating
	createLock->Acquire();
	id = customersInBuilding;
	this.id = id;
	SetMonitor(c_id, id, id);
	SetMonitor(c_ssn, id, id + 1000;);
	this.ssn = id + 1000;
	/*strcpy(customers[customersInBuilding].name, name);
	strcat(customers[customersInBuilding].name, customers[customersInBuilding].id);*/
	this.name = name;
//	SetMonitor(c_name, id, customers[id].name);
	this.money =  100 + 500*(Rand() % 4);/*init money increments of 100,600,1100,1600*/
	SetMonitor(c_money, id, this.money);
	this.myLine = -1;
	this.rememberLine = false;
	this.isSenator = false;
	/*strcpy(customers[customersInBuilding].name, name);
	strcat(customers[customersInBuilding].name, customers[customersInBuilding].id);*/
	createLock->Release();
	return customersInBuilding++; 
}

int CreateCustomer_WithCredentials(char* name, int* credentials) 
{
	createLock->Acquire();
	//TODO: how to deal with credential arrays.
	//possibly change them to a single variable for each customer
	
	for(i=0;i<NUM_CLERK_TYPES;i++) {
	  customers[customersInBuilding].credentials[i] = credentials[i];
	}
	id = customersInBuilding;
	this.id = id;
SetMonitor(c_id, id, id);
	this.ssn = id + 1000;
	SetMonitor(c_ssn, id, this.ssn);
	/*strcpy(customers[customersInBuilding].name, name);
	strcat(customers[customersInBuilding].name, customers[customersInBuilding].id);*/
	//customers[id].name = name;
	//SetMonitor(c_name, id, customers[id].name);
	this.money =  100 + 500*(Rand() % 4);/*init money increments of 100,600,1100,1600*/
	SetMonitor(c_money, id, this.money);
	this.myLine = -1;
	this.rememberLine = false;
	this.isSenator = false;
	/*strcpy(customers[customersInBuilding].name, name);
	strcat(customers[customersInBuilding].name, customers[customersInBuilding].id);*/
	createLock->Release();
	return customersInBuilding++;
}

bool isNextClerkType(struct Customer* customer, int clerk_type)
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

void giveData(struct Customer *customer)
{
	switch (GetMonitor(clerkTypes, customer->myLine)) {
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
		customer->money-=100;
		  SetMonitor(totalEarnings, CASHIER_CLERK_TYPE, GetMonitor(totalEarnings, CASHIER_CLERK_TYPE)+100);
		break;
	}
}


void checkSenator(/*struct Customer *customer*/)
{
	/*if there is a Senator in the building wait until he's gone*/
	if (senatorInBuilding){
		/*PrintInt("Clerk%i: waiting outside\n",  customer->name);*/
		/* TODO:rework this to lock
		senatorSemaphore->P();
		senatorSemaphore->V();
		*/
		/*go back inside*/
	}
}

int picApproval;
void Customer_Run(struct Customer* customer)
{
  simulationStarted = true;
  while(true)
  {
	  
	
	Acquire(clerkLineLock);/*im going to consume linecount values, this is a CS*/
	
	activeCustomers++;/* regulate w/clerklinelock. senator should check this number and senatorInBuilding before  operation and wait in senatorLineCV if both true */
	if(senatorInBuilding){
		Acquire(senatorLock);
		activeCustomers--;
		if(activeCustomers == 0){ /* if you're last to go outside signal first senator to come in */
			Signal(SenatorLineCV, senatorLock);
		}
		Wait(OutsideCV, senatorLock);
		activeCustomers++; /* come back inside */
		Release(senatorLock);
	}
	
	pickLine(customer);
	/*now, myLine is the index of the shortest line*/
	/*if the clerk is busy or on break, get into line*/
	if (GetMonitor(clerkState, customer->myLine) != 0){
		if(customer->isBribing)
		  {
		  	int myLineBribeCount = GetMonitor(clerkIds, customer->myLine);
			PrintInt("Customer%i has gotten in a bribe line for Clerk%i\n",51, customer->id, myLineBribeCount);
			SetMonitor(clerkBribeLineCount, customer->myLine, myLineBribeCount + 1 );
			
		    Wait(clerkLineLock, GetMonitor(clerkBribeLineCV, customer->myLine));
		    myLineBribeCount = GetMonitor(clerkBribeLineCount, customer->myLine);
			PrintInt("Customer%i leaving bribe line for Clerk%i\n",43, customer->id, GetMonitor(clerkIds, customer->myLine));
		    SetMonitor(clerkBribeLineCount, customer->myLine, myLineBribeCount-1);
		    myLineBribeCount--;
		    PrintInt("bribe line%i count: %i\n",23, customer->myLine, myLineBribeCount);
		  }
		else
		  {
		  	int theClerkId = GetMonitor(clerkIds, customer->myLine);
			PrintInt("Customer%i has gotten in a regular line for Clerk%i\n",53, customer->id, theClerkId);
			clerkLineCount[customer->myLine]++;
			
			Wait(clerkLineLock, Get(clerkLineCV,customer->myLine);
			PrintInt("Customer%i leaving regular line for Clerk%i\n",45, customer->id, theClerkId);
			int myClerkLineCount = GetMonitor(clerkLineCount, customer->myLine);
			SetMonitor(clerkLineCount, customer->myLine, myClerkLineCount - 1);
			myClerkLineCount--;
			PrintInt("regular line%i count: %i\n", 26, customer->myLine, myClerkLineCount);
		  }
	}
	if (senatorInBuilding) {
		customer->rememberLine = true;/*you're in line being kicked out by senator. */
		Release(clerkLineLock); /* free up your locks */
		continue; /* go back up to beginning of while loop where you will leave building.pickLine should manage line remembering */
	}
	
	clerkState[customer->myLine] = 1;
	Release(clerkLineLock);/*i no longer need to consume lineCount values, leave this CS*/

	Acquire(myClerkLock);/*we are now in a new CS, need to share data with my clerk*/
	clerkCurrentCustomerSSN[customer->myLine] = customer->ssn;
	PrintInt("Customer%i has given SSN %i", 27, customer->id, customer->ssn);
	PrintInt("to Clerk%i\n", 12, myClerkId, 0);
	clerkCurrentCustomer[customer->myLine] = customer->id;
	giveData(customer);
	customer->isBribing = false;
	Signal(myClerkLock, myClerkCV);
	/*now we wait for clerk to do job*/
	Wait(myClerkLock, myClerkCV);
	
	/*set credentials*/
	customer->credentials[myClerkType] = true;
	PrintInt("Customer%i: Thank you Clerk%i\n", 31, customer->id, myClerkId);

	if (myClerkType == PICTURE_CLERK_TYPE) {
	  /*check if I like my photo RandOM VAL*/
	  picApproval = Rand() % 10;/*generate Random num between 0 and 10*/
	  if(picApproval >8)
	    {
	      PrintInt("Customer%i: does like their picture from Clerk%i", 48, customer->id, myClerkId);
	      /*store that i have pic*/
	    }
	  else
	    {
	      PrintInt("Customer%i: does not like their picture from Clerk%i, please retake\n",69, customer->id, myClerkId);
	      /*_credentials[type] = false;/*lets seeye*/
	    }
	}
	Signal(myClerkLock, myClerkCV);/*let clerk know you're leaving*/
	Release(myClerkLock);/*give up lock*/
	
	customer->myLine = -1;
	customer->rememberLine = false;
	
	/*chose exit condition here*/
	if(customer->credentials[CASHIER_CLERK_TYPE])
	  break;
  }
  PrintInt("Customer%i: IS LEAVING THE PASSPORT OFFICE\n", 44, customer->id, 0);
  customersInBuilding--;/*assumption: customers are all created at start before any can leave office*/
}
int testLine = 69;
int lineSize = 1001;
int desireToBribe;
void pickLine(struct Customer* customer)
{
  /* isBribing = false;*/
  if (!customer->rememberLine)/*if you don't have to remember a line, pick a new one*/
  {
	  customer->myLine = -1;
	  lineSize = 1001;
	  for(i = 0; i < NUM_CLERKS; i++)
	    {
		  /*check if the type of this line is something I need! TODO*/
			if(/*clerks[i] != NULL &&*/ isNextClerkType(customer, GetMonitor(clerkTypes, i))) {
			  if(clerkLineCount[i] < lineSize )/*&& clerkState[i] != 2)*/
				{		      
				  customer->myLine = i;
				  lineSize = clerkLineCount[i];
				}
			}
	    }
	  desireToBribe = Rand() % 10;
	  /*if i want to bribe, let's lock at bribe lines*/
	  if(customer->money > 600 && desireToBribe > 8)
	    {
	      for(i = 0; i < NUM_CLERKS; i++)
			{
<<<<<<< HEAD
				if(/*clerks[i] != NULL &&*/ isNextClerkType(customer, GetMonitor(clerkTypes, i))) 
				{
					int theBribeLineCount = GetMonitor(clerkBribeLineCount, i);
				  	if(theBribeLineCount <=  lineSize)/*for TESTING. do less than only for real*/
=======
				if(/*clerks[i] != NULL &&*/ isNextClerkType(customer, clerks[i].type)) 
				{
				  if(clerkBribeLineCount[i] <=  lineSize)/*for TESTING. do less than only for real*/
>>>>>>> 096ae4456136589e632b90c6dc287365b8ab5a36
					{
					  /*PrintInt("Clerk%i: I'm BRIBING\n", name);*/
					  customer->money -= 500;
					  customer->myLine = i;
					  customer->isBribing = true;
					  lineSize = theBribeLineCount;
					  lineSize = clerkBribeLineCount[i];
					}
				}
			}
	    }
  }
}


struct Customer senators[NUM_SENATORS];

int CreateSenator(char* name) 
{	
    senators[senatorsAlive].id = senatorsAlive;
	senators[senatorsAlive].ssn = senatorsAlive + 1000;
	/*strcpy(senators[senatorsAlive].name, name);
	strcat(senators[senatorsAlive].name, senators[senatorsAlive].id);*/
	senators[senatorsAlive].name = name;
	senators[senatorsAlive].money =  100 + 500*(Rand() % 4);/*init money increments of 100,600,1100,1600*/
	senators[senatorsAlive].myLine = -1;
	senators[senatorsAlive].rememberLine = false;
	senators[senatorsAlive].isSenator = true;
	return senatorsAlive++;
}

void Senator_EnterOffice(struct Customer* senator)
{
  /*walk in acquire all clerk locks to prevent next in line from getting to clerk*/
  /*senatorSemaphore->P();*/ /*use to lock down entire run section*/
  senatorInBuilding = true;/*signals all people waiting to exit*/
  
  Acquire(clerkLineLock); /*acquire to broadcast current customers. released in Customer::run()*/
  for (i=0; i<NUM_CLERKS;i++) {
    if (clerkLineCV[i] >= 0) {
		Broadcast(clerkLineLock, clerkLineCV[i]); /*clear out lines. they will stop because of semaphore after leaving line/before returning to it*/
    }

    if (clerkBribeLineCV[i] >= 0) {
		Broadcast(clerkLineLock, clerkBribeLineCV[i]); /*clear out lines. they will stop because of semaphore after leaving line/before returning to it*/
    }
  }
 
  Release(clerkLineLock);
  for (i=0; i<NUM_CLERKS;i++) {
	/*wait for bribe line to empty*/
	Acquire(clerkLineLock);
	while(clerkBribeLineCount[i] > 0) {
	  Signal(clerkLineLock, clerkBribeLineCV[i]);
	  Wait(clerkLineLock, clerkBribeLineCV[i]);
	}
	/*wait for regular line to empty*/
	while (clerkLineCount[i] > 0) {
	  Signal(clerkLineLock, clerkLineCV[i]);
	  Wait(clerkLineLock, clerkLineCV[i]);
	}
  }
  /*everyone is out of lines now wait for clerks to finish*/
  for (i=0; i<NUM_CLERKS;i++) {
    if (clerkLock[i] >= 0) {
		Acquire(clerkLock[i]);
		while (clerkState[i] == 1) {
			Signal(clerkLock[i], clerkCV[i]);
			Wait(clerkLock[i], clerkCV[i]);
		}
    }
  }
}

}

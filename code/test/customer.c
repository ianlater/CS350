/* customer.c
   includes senators
*/
#include "syscall.h"
#include "setup.h"
int a[3];
int b, c;


/* creates new customer w/ given name. should declare new customer ahead of time and place where needed. this just fills in info*/
int CreateCustomer(char* name) 
{	
    customers[customersInBuilding].id = customersInBuilding;
	customers[customersInBuilding].ssn = customersInBuilding + 1000;
	/*strcpy(customers[customersInBuilding].name, name);
	strcat(customers[customersInBuilding].name, customers[customersInBuilding].id);*/
	customers[customersInBuilding].name = name;
	customers[customersInBuilding].money =  100 + 500*(Rand() % 4);/*init money increments of 100,600,1100,1600*/
	customers[customersInBuilding].myLine = -1;
	customers[customersInBuilding].rememberLine = false;
	customers[customersInBuilding].isSenator = false;
	return customersInBuilding++; 
}

int CreateCustomer_WithCredentials(char* name, int* credentials) 
{
	for(i=0;i<NUM_CLERK_TYPES;i++) {
	  customers[customersInBuilding].credentials[i] = credentials[i];
	}
    customers[customersInBuilding].id =customersInBuilding;
	customers[customersInBuilding].ssn = customersInBuilding + 1000;
	/*strcpy(customers[customersInBuilding].name, name);
	strcat(customers[customersInBuilding].name, customers[customersInBuilding].id);*/
	customers[customersInBuilding].name = name;
	customers[customersInBuilding].money =  100 + 500*(Rand() % 4);/*init money increments of 100,600,1100,1600*/
	customers[customersInBuilding].myLine = -1;
	customers[customersInBuilding].rememberLine = false;
	customers[customersInBuilding].isSenator = false;
	return customersInBuilding++;
}

bool isNextClerkType(struct Customer* customer, int clerk_type)
{
    /*for adding customers who go out of order, we can add in a Random number check that returns true Randomly*/

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

void giveData(struct Customer *customer)
{
	switch (clerks[customer->myLine].type) {
	  case APPLICATION_CLERK_TYPE:
		PrintInt("Customer%i: may I have application please?\n", 44,  customer->id, 0);
		if(customer->isBribing) {
		  customer->money-=500;
		  totalEarnings[APPLICATION_CLERK_TYPE] += 500;
		}
		break;

	  case PICTURE_CLERK_TYPE:
		/*ask for a picture to be taken*/
		PrintInt("Customer%i: I would like a picture\n", 36, customer->id, 0);
		if(customer->isBribing){
		  customer->money-=500;
		  totalEarnings[PICTURE_CLERK_TYPE] += 500;
		}
		break;

	  case PASSPORT_CLERK_TYPE:
		PrintInt("Customer%i: ready for my passport\n", 35, customer->id, 0);
		if(customer->isBribing){
		  customer->money-=500;
		  totalEarnings[PASSPORT_CLERK_TYPE] += 500;
		}
		break;

	  case CASHIER_CLERK_TYPE:
		PrintInt("Customer%i has given Cashier %i $100\n", 38, customer->id, clerks[customer->myLine].id);
		if(customer->isBribing){
		  customer->money-=500;
		  totalEarnings[PASSPORT_CLERK_TYPE] += 500;
		}
		customer->money-=100;
		totalEarnings[CASHIER_CLERK_TYPE] += 100;
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
  Release(createLock);
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
	if (clerkState[customer->myLine] != 0) {
		if(customer->isBribing)
		  {
			PrintInt("Customer%i has gotten in a bribe line for Clerk%i\n",51, customer->id, clerks[customer->myLine].id);
			clerkBribeLineCount[customer->myLine]++;
			
		    Wait(clerkLineLock, clerkBribeLineCV[customer->myLine]);
			PrintInt("Customer%i leaving bribe line for Clerk%i\n",43, customer->id, clerks[customer->myLine].id);
		    clerkBribeLineCount[customer->myLine]--;
		    PrintInt("bribe line%i count: %i\n",23, customer->myLine, clerkBribeLineCount[customer->myLine]);
		  }
		else
		  {
			PrintInt("Customer%i has gotten in a regular line for Clerk%i\n",53, customer->id, clerks[customer->myLine].id);
			clerkLineCount[customer->myLine]++;
			
			Wait(clerkLineLock, clerkLineCV[customer->myLine]);
			PrintInt("Customer%i leaving regular line for Clerk%i\n",45, customer->id, clerks[customer->myLine].id);
			clerkLineCount[customer->myLine]--;
			PrintInt("regular line%i count: %i\n", 26, customer->myLine, clerkLineCount[customer->myLine]);
		  }
	}
	if (senatorInBuilding) {
		customer->rememberLine = true;/*you're in line being kicked out by senator. */
		Release(clerkLineLock); /* free up your locks */
		continue; /* go back up to beginning of while loop where you will leave building.pickLine should manage line remembering */
	}
	
	clerkState[customer->myLine] = 1;
	Release(clerkLineLock);/*i no longer need to consume lineCount values, leave this CS*/

	Acquire(clerkLock[customer->myLine]);/*we are now in a new CS, need to share data with my clerk*/
	clerkCurrentCustomerSSN[customer->myLine] = customer->ssn;
	PrintInt("Customer%i has given SSN %i", 27, customer->id, customer->ssn);
	PrintInt("to Clerk%i\n", 12, clerks[customer->myLine].id, 0);
	clerkCurrentCustomer[customer->myLine] = customer->id;
	giveData(customer);
	customer->isBribing = false;
	Signal(clerkLock[customer->myLine], clerkCV[customer->myLine]);
	/*now we wait for clerk to do job*/
	Wait(clerkLock[customer->myLine], clerkCV[customer->myLine]);
	
	/*set credentials*/
	customer->credentials[clerks[customer->myLine].type] = true;
	PrintInt("Customer%i: Thank you Clerk%i\n", 31, customer->id, clerks[customer->myLine].id);

	if (clerks[customer->myLine].type == PICTURE_CLERK_TYPE) {
	  /*check if I like my photo RandOM VAL*/
	  picApproval = Rand() % 10;/*generate Random num between 0 and 10*/
	  if(picApproval >8)
	    {
	      PrintInt("Customer%i: does like their picture from Clerk%i", 48, customer->id, clerks[customer->myLine].id);
	      /*store that i have pic*/
	    }
	  else
	    {
	      PrintInt("Customer%i: does not like their picture from Clerk%i, please retake\n",69, customer->id, clerks[customer->myLine].id);
	      /*_credentials[type] = false;/*lets seeye*/
	    }
	}
	Signal(clerkLock[customer->myLine], clerkCV[customer->myLine]);/*let clerk know you're leaving*/
	Release(clerkLock[customer->myLine]);/*give up lock*/
	
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
			if(/*clerks[i] != NULL &&*/ isNextClerkType(customer, clerks[i].type)) {
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
				if(/*clerks[i] != NULL &&*/ isNextClerkType(customer, clerks[i].type)) 
				{
				  if(clerkBribeLineCount[i] <=  lineSize)/*for TESTING. do less than only for real*/
					{
					  /*PrintInt("Clerk%i: I'm BRIBING\n", name);*/
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

void Senator_ExitOffice(struct Customer* senator)
{
  currentSenatorId++;
  senatorInBuilding = false;
  /*senatorSemaphore->V();*/
}

void Senator_Run(struct Customer* senator)
{
	Release(createLock);
	/*enter facility*/
	Senator_EnterOffice(senator);
	/*proceed as a normal customer*/
	Customer_Run(senator);
	/*exit facility*/
	Senator_ExitOffice(senator);
}
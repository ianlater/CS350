/* customer.c
   includes senators
*/
#include "syscall.h"
#include "setup.h"
int a[3];
int b, c;
int rememberLine = -1;

/* creates new customer w/ given name. should declare new customer ahead of time and place where needed. this just fills in info*/
int CreateCustomer(char* name) 
{	
    id = customersInBuilding;
	SetMonitor(custSSN, id,id + 1000);
	/*strcpy(customers[customersInBuilding].name, name);
	strcat(customers[customersInBuilding].name, customers[customersInBuilding].id);*/
	customers[customersInBuilding].money =  100 + 500*(Rand() % 4);/*init money increments of 100,600,1100,1600*/
	customers[customersInBuilding].myLine = -1;
	rememberLine = false;
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
    if (!credentials[clerk_type]) /*credentials is what you have i.e. picture etc. (int[NUM_CLERKS]) w/ index corrosponding to clerk type, 0=have 1=don't have*/
    {
        if (clerk_type == PICTURE_CLERK_TYPE || clerk_type == APPLICATION_CLERK_TYPE)/*application and picture need no prior credentials*/
            return true;
        
        if (clerk_type == PASSPORT_CLERK_TYPE) /*passport clerk requires both application and pictue*/
        {
            if (credentials[APPLICATION_CLERK_TYPE] && credentials[PICTURE_CLERK_TYPE]) 
                return true;
            
            return false;
        }
        if (clerk_type == CASHIER_CLERK_TYPE && credentials[PASSPORT_CLERK_TYPE]) /*cashier requires only passport (ASSUMPTION?)*/
        {
            return true;
	}
       } 
        return false;  
}

void giveData(struct Customer *customer)
{
	switch (clerks[myLine].type) {
	  case APPLICATION_CLERK_TYPE:
		PrintInt("Customer%i: may I have application please?\n", 44,  id, 0);
		if(isBribing) {
		  money-=500;
		  totalEarnings[APPLICATION_CLERK_TYPE] += 500;
		}
		break;

	  case PICTURE_CLERK_TYPE:
		/*ask for a picture to be taken*/
		PrintInt("Customer%i: I would like a picture\n", 36, id, 0);
		if(isBribing){
		  money-=500;
		  totalEarnings[PICTURE_CLERK_TYPE] += 500;
		}
		break;

	  case PASSPORT_CLERK_TYPE:
		PrintInt("Customer%i: ready for my passport\n", 35, id, 0);
		if(isBribing){
		  money-=500;
		  totalEarnings[PASSPORT_CLERK_TYPE] += 500;
		}
		break;

	  case CASHIER_CLERK_TYPE:
		PrintInt("Customer%i has given Cashier %i $100\n", 38, id, GetMonitor(ClerkIds, myLine));
		if(isBribing){
		  money-=500;
		  totalEarnings[PASSPORT_CLERK_TYPE] += 500;
		}
		money-=100;
		totalEarnings[CASHIER_CLERK_TYPE] += 100;
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
void Customer_Run(struct Customer* customer)
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
void pickLine(struct Customer* customer)
{
  /* isBribing = false;*/
  if (!rememberLine)/*if you don't have to remember a line, pick a new one*/
  {
	  myLine = -1;
	  lineSize = 1001;
	  for(i = 0; i < NUM_CLERKS; i++)
	    {
		  /*check if the type of this line is something I need! TODO*/
			if(/*clerks[i] != NULL &&*/ isNextClerkType(customer, clerks[i].type)) {
			  if(clerkLineCount[i] < lineSize )/*&& clerkState[i] != 2)*/
				{		      
				  myLine = i;
				  lineSize = clerkLineCount[i];
				}
			}
	    }
	  desireToBribe = Rand() % 10;
	  /*if i want to bribe, let's lock at bribe lines*/
	  if(money > 600 && desireToBribe > 8)
	    {
	      for(i = 0; i < NUM_CLERKS; i++)
			{
				if(/*clerks[i] != NULL &&*/ isNextClerkType(customer, clerks[i].type)) 
				{
				  if(clerkBribeLineCount[i] <=  lineSize)/*for TESTING. do less than only for real*/
					{
					  /*PrintInt("Clerk%i: I'm BRIBING\n", name);*/
					  money -= 500;
					  myLine = i;
					  isBribing = true;
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
  GetMonitor(senatorInBuilding, 0) = true;/*signals all people waiting to exit*/
  
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
  GetMonitor(senatorInBuilding, 0) = false;
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
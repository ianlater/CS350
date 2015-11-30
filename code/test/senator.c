/* customer.c
   includes senators
*/
#include "syscall.h"
#include "setup.h"
int a[3];
int b, c;


typedef enum
{
  false,
  true
} bool;

int id = -1, money = 0, myLine = -1, isSenator = true;
bool rememberLine = false;
int credentials[NUM_CLERKS] = {0};

/* creates new customer w/ given name. should declare new customer ahead of time and place where needed. this just fills in info*/
int CreateSenator(char* name) 
{	
	Acquire(createLock);
    id = GetMonitor(customersInBuilding);
	SetMonitor(custSSN, id,id + 2000);
	money =  100 + 500*(Rand() % 4);/*init money increments of 100,600,1100,1600*/
	SetMonitor(customersInBuilding, id + 1);
	Release(createLock);
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
		customer->money-=100;
		  SetMonitor(totalEarnings, CASHIER_CLERK_TYPE, GetMonitor(totalEarnings, CASHIER_CLERK_TYPE)+100);
		break;
	}
}

void Senator_EnterOffice()
{  
  Acquire(senatorLock);
	SetMonitor(senatorInBuilding,0, true);
	SetMonitor(senatorsAlive, 0, GetMonitor(senatorsAlive, 0)+1);
	if(GetMonitor(activeCustomers, 0) > 0){ /* if there are still customers who need to go outside wait for them to signal */
		Wait(senatorLock, SenatorLineCV ); /* last customer will signal first senator in line */
	} /* you are first senator to be signalled */
}

void Senator_Run()
{
  simulationStarted = true;
  Release(createLock);
  while(true)
  {
	Acquire(clerkLineLock);/*im going to consume linecount values, this is a CS*/
	pickLine();
	/*the clerk shouldn't be busy*/
	if (GetMonitor(clerkState, myLine) != 0) { /* clerk should be on break if not free at this point */
		PrintInt("Senator%i has gotten in a regular line for Clerk%i\n",53, id, GetMonitor(ClerkIds, myLine));
			SetMonitor(clerkLineCount, myLine, 3) /* fake the linecount so clerk wakes up */
			
			Wait(clerkLineLock, clerkLineCV[myLine]);
			PrintInt("Senator%i leaving regular line for Clerk%i\n",45, id, GetMonitor(ClerkIds, myLine));
			SetMonitor(clerkLineCount, myLine, 0)
	}

	SetMonitor(clerkState, myLine, 1);
	Release(clerkLineLock);/*i no longer need to consume lineCount values, leave this CS*/
	
	myClerksLock = GetMonitor(clerkLock, myLine);
	myClerksCV = GetMonitor(clerkCV, myLine);
	Acquire(myClerksLock);/*we are now in a new CS, need to share data with my clerk*/
	SetMonitor(clerkCurrentCustomerSSN, myLine, ssn);
	PrintInt("Senator%i has given SSN %i", 27, id, ssn);
	PrintInt("to Clerk%i\n", 12, GetMonitor(ClerkIds, myLine), 0);
	SetMonitor(clerkCurrentCustomer, myLine, id);
	giveData();
	isBribing = false;
	Signal(myClerksLock, myClerksCV);
	/*now we wait for clerk to do job*/
	Wait(myClerksLock, myClerksCV);
	
	/*set credentials*/
	credentials[GetMonitor(ClerkTypes, myLine)] = true;
	PrintInt("Senator%i: Thank you Clerk%i\n", 31, id,  GetMonitor(ClerkIds, myLine));

	if (GetMonitor(ClerkTypes, myLine) == PICTURE_CLERK_TYPE) {
	  /* senators are not picky */ 
	    PrintInt("Senator%i: does like their picture from Clerk%i", 48, id, GetMonitor(ClerkIds, myLine));
	}
	
	Signal(myClerksLock, myClerksCV);/*let clerk know you're leaving*/
	Release(myClerksLock);/*give up lock*/
	
	myLine = -1;
	rememberLine = false;
	
	/*chose exit condition here*/
	if(credentials[CASHIER_CLERK_TYPE])
	  break;
  }
  PrintInt("Senator%i: IS LEAVING THE PASSPORT OFFICE\n", 44, id, 0);
  SetMonitor(customersInBuilding, GetMonitor(customersInBuilding)-1);/*assumption: customers are all created at start before any can leave office*/
}

void Senator_ExitOffice()
{  
	SetMonitor(senatorsAlive, 0, GetMonitor(senatorsAlive, 0)-1);
	if(GetMonitor(senatorsAlive, 0) == 0){ /* no more senators so let customers come back in */
		SetMonitor(senatorInBuilding,0, false);
		Broadcast(senatorLock, OutsideCV);
	} else { /* still senators waiting */
		Signal(senatorLock, SenatorLineCV);
	}
	Release(senatorLock);
}

int main(){
	Senator_EnterOffice();
	Senator_Run();
	Senator_ExitOffice();
	Exit(0);
}
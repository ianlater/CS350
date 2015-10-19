/* passport.c

*/
#include "syscall.h"
int a[3];
int b, c;


#ifdef CHANGED

#define APPLICATION_CLERK_TYPE 0
#define PICTURE_CLERK_TYPE 1
#define PASSPORT_CLERK_TYPE 2
#define CASHIER_CLERK_TYPE 3

typedef enum
{
  false,
  true
} bool;

/*global vars, mostly Monitors*/

const int NUM_CLERK_TYPES = 4;
const int NUM_CLERKS = 25;/*max num of clerks needed.. change later when dynamic stuff is figured out*/
const int NUM_CUSTOMERS = 20;
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
int total;/*for  output earnings*/
int newLen; /* for char stuff*/

struct Clerk
{
  int type;/*represents type of clerk 1 = ApplicationClerk, 2 = PictureClerk, 3 = PassportClerk (used to to facilitate abstract use of clerk)*/
  char* name;
  int id;
};

Clerk* clerks[NUM_CLERKS];/*global array of clerk ptrs*/

char* buffer2 = new char[50];
char* buffer3 = new char[50];

void Clerk(struct Clerk clerk, char* name, id) 	
{
	clerk.id = id;
	newLen = strlen(name) + 16;
	clerk.name = new char[newLen];
	sPrint(clerk.name, "%s%i", name, id);
	/*Locks
	char* buffer1 = new char[50];
	sPrint(buffer1, "ClerkLock%i", id);
	clerkLock[id] = CreateLock(buffer1);*/
	clerkLineLock[id] = CreateLock("ClerkLineLock" + id);

	/*CVs & MVs*/
	clerkBreakCV[id] = CreateCondition("ClerkBreakCV" +id);
	
	sPrint(buffer2, "ClerkLineCv%i", id);
	clerkLineCV[id] = CreateCondition(buffer2);
	clerkLineCount[id] =0;/*Assumption, start empty*/
	clerkBribeLineCount[id] = 0;
	clerkBribeLineCV[id] = CreateCondition(buffer2);

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

int picApproval;
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
		    clerkBribeLineCount[customer->myLine]--;
		    PrintInt("bribe line%i count: %i",22, customer->myLine, clerkBribeLineCount[customer->myLine]);
		  }
		else
		  {
		    Wait(clerkLineCV[customer->myLine], clerkLineLock);
		    clerkLineCount[customer->myLine]--;
		    PrintInt("regular line%i count: %i", 25, customer->myLine, clerkLineCount[customer->myLine]);
		  }
		if (senatorInBuilding) {
		  customer->rememberLine = true;/*you're in line being kicked out by senatr. senator can't kick self out*/
  			/*make sure to signal senator who may be in line */
			if(customer->isBribing) {
			  Signal(clerkBribeLineCV[customer->myLine], clerkLineLock);
			}
			else {
			  Signal(clerkLineCV[customer->myLine], clerkLineLock);
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
	  picApproval = rand() % 10;/*generate random num between 0 and 10*/
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
		if(clerks[i] != NULL && isNextClerkType(clerks[i]->type)) {
		  if(clerkLineCount[i] < lineSize )/*&& clerkState[i] != 2)*/
		    {		      
			  customer->myLine = i;
		      lineSize = clerkLineCount[i];
		    }
		}
	    }
	  desireToBribe = rand() % 10;
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
	Acquire(clerkLineLock);
	while(clerkBribeLineCount[i] > 0) {
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
		Acquire(clerkLock[i]);
		while (clerkState[i] == 1) {
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
	total = 0;
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
int main(){
	
	Halt();
}
#endif

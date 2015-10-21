/* passport.c

*/
#include "syscall.h"
int a[3];
int b, c;



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
#define NUM_CLERK_TYPES 4
#define NUM_CLERKS 16
#define NUM_CUSTOMERS 20
#define NUM_SENATORS 0

/*
Monitor setup:
array of lock(ptrs) for each clerk+their lines
*/

int clerkLock[NUM_CLERKS];
 
int clerkLineLock = -1;
int outsideLock = -1;
int senatorLock = -1;
int createLock = -1; /*since fork takes no params, and agent creation based off global, need lock to avoid race conditions of creating same id*/

/*Condition Variables*/
int clerkLineCV[NUM_CLERKS]= {-1};
int clerkBribeLineCV[NUM_CLERKS]= {-1};
int clerkCV[NUM_CLERKS]= {-1};/*I think we need this? -Jack*/
int clerkBreakCV[NUM_CLERKS] = {-1}; /*CV for break, for use with manager*/
int senatorCV = -1;

/*Monitor Variables*/
int clerkLineCount[NUM_CLERKS] = {0};/*start big so we can compare later*/
int clerkBribeLineCount[NUM_CLERKS] = {0};
int clerkState[NUM_CLERKS] = {0};/*keep track of state of clerks with ints 0=free,1=busy,2-on breaK /*sidenote:does anyone know how to do enums? would be more expressive?*/
int totalEarnings[NUM_CLERK_TYPES] = {0};/*keep track of money submitted by each type of clerk*/
int customersInBuilding = 0;
int clerksInBuilding = 0;
int managersInBuilding = 0;
int senatorsAlive = 0;
bool senatorInBuilding = false;
int clerkCurrentCustomer[NUM_CLERKS];/*relate clerk id to customer id*/
int clerkCurrentCustomerSSN[NUM_CLERKS];/*relate clerk id to customer ssn*/
int currentSenatorId = -1;
int i;/*iterator for loops*/
bool simulationStarted = false;/*so simulation doesn't end before customers enter*/
bool simulationEnded = false;


struct Clerk
{
  int type;/*represents type of clerk 1 = ApplicationClerk, 2 = PictureClerk, 3 = PassportClerk (used to to facilitate abstract use of clerk)*/
  char* name;
  int id;
};

struct Clerk clerks[NUM_CLERKS];/*global array of clerks*/

int CreateClerk(char* name) 	
{
	clerks[clerksInBuilding].id = clerksInBuilding;
	clerks[clerksInBuilding].name = name;
	if (clerksInBuilding % 4 == 0){
		clerks[clerksInBuilding].type = APPLICATION_CLERK_TYPE;
	} else if( clerksInBuilding% 4 == 1) {
		clerks[clerksInBuilding].type = PICTURE_CLERK_TYPE;
	} else if (clerksInBuilding % 4 == 2){
		clerks[clerksInBuilding].type = PASSPORT_CLERK_TYPE;
	} else {
		clerks[clerksInBuilding].type = CASHIER_CLERK_TYPE;
	}
	/*CVs & MVs*/
	clerkBreakCV[clerksInBuilding] = CreateCondition("ClerkBreakCv", 12);
	if(clerkBreakCV[clerksInBuilding]<0) {
		Halt();
	}
	clerkLineCV[clerksInBuilding] = CreateCondition("ClerkLineCv", 11);
	if(clerkLineCV[clerksInBuilding]<0) {
		Halt();
	}
	clerkBribeLineCV[clerksInBuilding] = CreateCondition("ClerkBribeLineCv",16);
	if(clerkBribeLineCV[clerksInBuilding]<0) {
		Halt();
	}
	clerkCV[clerksInBuilding] = CreateCondition("ClerkCV", 7);
	if(clerkCV[clerksInBuilding]<0) {
		Halt();
	}
	
	return clerksInBuilding++;
}

void DestroyClerk(struct Clerk* clerk)
{

}

/*@param id is id of clerk doing job*/
void doJob(int id){
	switch (clerks[id].type) {
		case APPLICATION_CLERK_TYPE:
			for(i = 0; i < 50; i++)
				Yield();
			PrintInt("Clerk%i: Has recorded a completed application for Customer%i\n", 62, clerks[id].id, clerkCurrentCustomer[id]);
			break;
		case PICTURE_CLERK_TYPE:
			for(i = 0; i < 50; i++)
				Yield();
			PrintInt("Clerk%i has taken a picture of Customer%i\n", 43, clerks[id].id ,clerkCurrentCustomer[id]);
			break;
		case PASSPORT_CLERK_TYPE:
			Print("Checking materials \n", 20, "", "");
			  /*required delay of 20 -100 cycles before going back*/
			for(i = 0; i < 50; i++)
				Yield();

			PrintInt("Clerk%i has recorded Customer%i\n passport documentation\n", 58,  clerks[id].id, clerkCurrentCustomer[id]);
			break;
		case CASHIER_CLERK_TYPE:
			Print("Checking passport receipt\n", 24, "", "");
			/*TODO validate they have passport*/
			Print("Thank you. One moment\n", 24, "", "");
			/*TODO cashier needs to record that this customer in particular has been issued a passport and the money recieved */
			PrintInt("Clerk%i has provided Customer%i their completed passport\n", 24,clerks[id].id, clerkCurrentCustomer[id]);
			for(i = 0; i < 50; i++)
				Yield();

			PrintInt("Clerk%i has recorded that Customer%i has been given their completed passport\n", 78, clerks[id].id, clerkCurrentCustomer[id]);
			break;
	}
}
void Clerk_Run(struct Clerk* clerk)
{
  PrintInt("Clerk%i beginning to run\n", 24, clerk->id, 0);
  Release(createLock);
  while(true)
  {
    /*acquire clerkLineLock when i want to update line values */
    Acquire(clerkLineLock);
    
    if(clerkBribeLineCount[clerk->id] > 0)
      {
		/*PrintInt("Clerk%i: is Busy taking a BRIBE\n", name);*/
		Signal(clerkLineLock, clerkBribeLineCV[clerk->id]);
		
		PrintInt("Clerk%i has signalled a Customer to come to their counter\n",59, clerk->id, 0);
		clerkState[clerk->id] = 1; /*busy*/
		
		Acquire(clerkLock[clerk->id]);
		Release(clerkLineLock);
		Wait(clerkLock[clerk->id], clerkCV[clerk->id]);/*wait for cust materials*/
		
		PrintInt("Clerk%i has received $500 from customer%i (BRIBE)\n", 51, clerk->id, clerkCurrentCustomer[clerk->id]);
		PrintInt("Clerk%i has received SSN %i from Customer", 45, clerk->id, clerkCurrentCustomerSSN[clerk->id]);
		PrintInt("%d\n",4, clerkCurrentCustomer[clerk->id],0);

		doJob(clerk->id);
		
		Signal(clerkLock[clerk->id], clerkCV[clerk->id]);/*tell customer jobs done*/
		Wait(clerkLock[clerk->id], clerkCV[clerk->id]);/*wait for customer to leave*/
		Release(clerkLock[clerk->id]);/*customer gone, next customer*/
      }
    else  if(clerkLineCount[clerk->id] > 0)
      {
		PrintInt("Clerk%i: is Busy\n", 18, clerk->id, 0);
		
		Signal(clerkLineLock, clerkLineCV[clerk->id]);
		
		PrintInt("Clerk%i has signalled a Customer to come to their counter\n",59, clerk->id,0);
		clerkState[clerk->id] = 1;/*im helping a customer*/
		
		/*acquire clerk lock and release line lock*/
		Acquire(clerkLock[clerk->id]);
		Release(clerkLineLock);
		Wait(clerkLock[clerk->id], clerkCV[clerk->id]); /*WAS IN b4*/
		
		/*once we're here, the customer is waiting for me to do my job*/
		PrintInt("Clerk%i has received SSN %i from Customer", 45, clerk->id, clerkCurrentCustomerSSN[clerk->id]);
		PrintInt("%d\n",4, clerkCurrentCustomer[clerk->id],0);

		doJob(clerk->id);
		
		Signal(clerkLock[clerk->id], clerkCV[clerk->id]);/*tell customer jobs done*/
		Wait(clerkLock[clerk->id], clerkCV[clerk->id]);/*wait for customer to leave*/
		Release(clerkLock[clerk->id]); /*we're done here, back to top of while for next cust*/
      }
    else if (clerkLineCount[clerk->id] == 0 && clerkBribeLineCount[clerk->id] == 0)  /*go on break*/
      {
		if(simulationEnded) {
			break;
		}
		/*acquire my lock*/
		Acquire(clerkLock[clerk->id]);
		/*set my status*/
		clerkState[clerk->id] = 2;

		/*release clerk line lock after signalling possible senator*/
		if (senatorInBuilding) {
			Signal(clerkLineLock, clerkBribeLineCV[clerk->id]);
			Signal(clerkLineLock, clerkLineCV[clerk->id]);
		}
		
		PrintInt("Clerk%i is going on break\n", 27, clerk->id, 0);
		
		Release(clerkLineLock);
		/*wait on clerkBreakCV from manager*/
		Wait(clerkLock[clerk->id], clerkBreakCV[clerk->id]);
		Release(clerkLock[clerk->id]);
		
		PrintInt("Clerk%i is coming off break\n",29, clerk->id, 0);
      }
  }
}

struct Customer
{
  char* name;
  int money;
  int myLine;/*-1 represents not in a line*/
  int id;
  int ssn; /*unique ssn for each customer*/
  int credentials[NUM_CLERK_TYPES];
  bool rememberLine;
  bool isBribing;
  bool isSenator;
};

struct Customer customers[NUM_CUSTOMERS];

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
		if(customer->isBribing)
		  customer->money-=500;
		  totalEarnings[APPLICATION_CLERK_TYPE] += 500;
		break;

	  case PICTURE_CLERK_TYPE:
		/*ask for a picture to be taken*/
		PrintInt("Customer%i: I would like a picture\n", 36, customer->id, 0);
		if(customer->isBribing)
		  customer->money-=500;
		  totalEarnings[PICTURE_CLERK_TYPE] += 500;
		break;

	  case PASSPORT_CLERK_TYPE:
		PrintInt("Customer%i: ready for my passport\n", 35, customer->id, 0);
		if(customer->isBribing)
		  customer->money-=500;
		  totalEarnings[PASSPORT_CLERK_TYPE] += 500;
		break;

	  case CASHIER_CLERK_TYPE:
		PrintInt("Customer%i has given Cashier %i $100\n", 38, customer->id, clerks[customer->myLine].id);
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
		  
		if (senatorInBuilding) {
		  customer->rememberLine = true;/*you're in line being kicked out by senatr. senator can't kick self out*/
  			/*make sure to signal senator who may be in line */
			if(customer->isBribing) {
			  Signal(clerkLineLock, clerkBribeLineCV[customer->myLine]);
			}
			else {
			  Signal(clerkLineLock, clerkBribeLineCV[customer->myLine]);
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
			      Wait(clerkLineLock, clerkBribeLineCV[customer->myLine]);
			      clerkBribeLineCount[customer->myLine]--;
			    }
			  else
			    {
			      clerkLineCount[customer->myLine]++;
				PrintInt("Customer%i: waiting in line for Clerk%i\n", 41, customer->id, clerks[customer->myLine].id);
				Wait(clerkLineLock, clerkBribeLineCV[customer->myLine]);
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

struct Manager
{
  char* name;
};

int total;
void OutputEarnings()
{
	total = 0;
	for (i =0; i < NUM_CLERK_TYPES; i++) {
		total += totalEarnings[i];
	}
	Print("\n----Earnings report:---- \n", 30, "","");
	PrintInt("ApplicationClerks: %i \n", 30,totalEarnings[APPLICATION_CLERK_TYPE],0);
	PrintInt("PictureClerks: %i \n",30,totalEarnings[PICTURE_CLERK_TYPE],0);
	PrintInt("PassportClerks: %i \n",30,totalEarnings[PASSPORT_CLERK_TYPE],0);
	PrintInt("Cashiers: %i \n",30,totalEarnings[CASHIER_CLERK_TYPE],0);
	PrintInt("TOTAL: %i \n------------------------\n\n",40,total,0);
}
struct Manager manager;
void Manager_Run()
{
  Print("Manager in building\n",21, "","");
  while(true) {
	for (i = 0; i < 1000; i++)
		Yield();
	for (i = 0; i < NUM_CLERKS; i++)
	{
		if (clerkState[i] == 2 && (clerkLineCount[i] >= 1 || clerkBribeLineCount[i] >= 1 || senatorInBuilding) )
		{
			/*wake up clerk*/
			if(Acquire(clerkLock[i]) < 0) {
					Halt();
			}	
			PrintInt("Manager waking up Clerk%i\n", 27, clerks[i].id, 0);
			clerkState[i] = 0;/*set to available	*/
			Signal(clerkLock[i], clerkBreakCV[i]);	
			Release(clerkLock[i]);	
		}
	}
	OutputEarnings();
	if (simulationStarted && customersInBuilding == 0 && senatorsAlive == 0) {
		simulationEnded = true;
		for (i = 0; i < NUM_CLERKS; i++)
		{
				/*wake up clerk*/
				Acquire(clerkLock[i]);	
				PrintInt("Manager waking up Clerk%i\n", 27, clerks[i].id, 0);
				clerkState[i] = 0;/*set to available	*/
				Signal(clerkLock[i], clerkBreakCV[i]);	
				Release(clerkLock[i]);	
		}
		Print("Manager ending simulation\n", 27, "", "");
		break;
	}
  }
  Exit(0);
}

/*to minimize repeated definition of names, since for the most part we use only ids, just make one name for all (though keeping naming functionality for future use just in case)*/
char* custName = "Customer";
char* clerkName="Clerk";
char* senatorName="Senator";
void CustRun(){
	Acquire(createLock);
	Customer_Run(&customers[CreateCustomer(custName)]);
	Exit(0);
}
void ClerkRun(){
	Acquire(createLock);
	Clerk_Run(&clerks[CreateClerk(clerkName)]);
	Exit(0);
}
void SenatorRun(){
	Acquire(createLock);
	Customer_Run(&senators[CreateSenator(senatorName)]);
	Exit(0);
}

int main(){
	/* init all locks, cvs, and agents*/
	clerkLineLock  = CreateLock("ClerkLineLock", 13);
	outsideLock = CreateLock("OutsideLock",11);
	senatorLock = CreateLock("SenatorLock", 11);
	senatorCV = CreateCondition("SenatorCV", 9);
	createLock = CreateLock("CreateLock", 10);
	
	
	
	for (i=0; i<NUM_CLERKS;i++){
		Fork(ClerkRun);
	}
	for (i=0; i<NUM_CUSTOMERS;i++){
		Fork(CustRun);
	}
	manager.name = "Mr. Manager";
	Fork(Manager_Run);
	for (i=0; i<NUM_SENATORS;i++){
		Fork(SenatorRun);
	}
	
	

	i = Rand();
	PrintInt("Rand_Sysall test: %i, mod 10: %i\n", 34, i, i%10);
	i = Rand();
	PrintInt("Rand_Sysall test: %i, mod 10: %i\n", 34, i, i%10);
}

/* clerk.h

*/
#include "syscall.h"
#include "setup.h"
int a[3];
int b, c, i;


int id = -1;
int myClerkCV, myClerkLock, myClerkLineCV, myClerkBribeLineCV, myclerkBreakCV;
char buf[20];
int myType = 0;
int CreateClerk() 	
{
	Acquire(createLock);
	id = GetMonitor(clerksInBuilding, 0);
	SetMonitor(clerkIds, id, id);
	SetMonitor(clerkTypes,id, myType);
	
	/*clerk lock*/
	myClerkLock = CreateLock("ClerkLock", 9);
	SetMonitor(clerkLock,id, myClerkLock);
	/*PrintInt("ClerkLock: %i, clerksInBuilding: %i\n", 37, myClerkLock, clerksInBuilding);*/
	if(myClerkLock<0) {
		Halt();
	}
	/*CVs*/
	myclerkBreakCV = CreateCondition("clerkBreakCV", 12);
	SetMonitor(clerkBreakCV, id, myclerkBreakCV);
	if(myclerkBreakCV<0) {
		Halt();
	}
	myClerkLineCV = CreateCondition("ClerkLineCv", 11);
	SetMonitor(clerkLineCV, id, myClerkLineCV);
	if(myClerkLineCV<0) {
		Halt();
	}
	myClerkBribeLineCV = CreateCondition("ClerkBribeLineCv",16);
	SetMonitor(clerkBribeLineCV, id, myClerkBribeLineCV);
	if(myClerkBribeLineCV<0) {
		Halt();
	}
	myClerkCV = CreateCondition("ClerkCV", 7);
	SetMonitor(clerkCV, i, myClerkCV);
	if(myClerkCV<0) {
		Halt();
	}
	
	SetMonitor(clerksInBuilding, 0,id+1);
	Release(createLock);

	return id;
}

void DestroyClerk()
{
  DestroyLock(myClerkLock);
  DestroyCondition(myClerkCV);
  DestroyCondition(myClerkLineCV);
  DestroyCondition(myClerkBribeLineCV);
  DestroyCondition(myclerkBreakCV);
}

/*@param id is id of clerk doing job*/
void doJob(int type){
	switch (type) {
		case APPLICATION_CLERK_TYPE:
			for(i = 0; i < 50; i++)
				Yield();
			PrintInt("Clerk%i: Has recorded a completed application for Customer%i\n", 62, id, GetMonitor(clerkCurrentCustomer, id));
			break;
		case PICTURE_CLERK_TYPE:
			for(i = 0; i < 50; i++)
				Yield();
			PrintInt("Clerk%i has taken a picture of Customer%i\n", 43, id ,GetMonitor(clerkCurrentCustomer, id));
			break;
		case PASSPORT_CLERK_TYPE:
			Print("Checking materials \n", 20, "", "");
			  /*required delay of 20 -100 cycles before going back*/
			for(i = 0; i < 50; i++)
				Yield();

			PrintInt("Clerk%i has recorded Customer%i\n passport documentation\n", 58,  id, GetMonitor(clerkCurrentCustomer, id));
			break;
		case CASHIER_CLERK_TYPE:
			Print("Checking passport receipt\n", 24, "", "");
			/*TODO validate they have passport*/
			Print("Thank you. One moment\n", 24, "", "");
			/*TODO cashier needs to record that this customer in particular has been issued a passport and the money recieved */
			PrintInt("Clerk%i has provided Customer%i their completed passport\n", 24,id, GetMonitor(clerkCurrentCustomer, id));
			for(i = 0; i < 50; i++)
				Yield();

			PrintInt("Clerk%i has recorded that Customer%i has been given their completed passport\n", 78, id, GetMonitor(clerkCurrentCustomer, id));
			break;
	}
}
void Clerk_Run(int type)
{
  PrintInt("Clerk%i beginning to run\n", 24, id, 0);
  while(true)
  {
    /*acquire clerkLineLock when i want to update line values */
    Acquire(clerkLineLock);
    
    if(GetMonitor(clerkBribeLineCount, id) > 0)
      {
		/*PrintInt("Clerk%i: is Busy taking a BRIBE\n", name);*/
		Signal(clerkLineLock, myClerkBribeLineCV);
		
		PrintInt("Clerk%i has signalled a Customer to come to their counter\n",59, id, 0);
		SetMonitor(clerkState, id, 1); /*busy*/
		
		
		Acquire(myClerkLock);
		Release(clerkLineLock);
		Wait(myClerkLock, myClerkCV);/*wait for cust materials*/
		
		PrintInt("Clerk%i has received $500 from customer%i (BRIBE)\n", 51, id, GetMonitor(clerkCurrentCustomer,id));
		PrintInt("Clerk%i has received SSN %i from Customer", 45, id, GetMonitor(clerkCurrentCustomerSSN,id));
		PrintInt("%d\n",4, GetMonitor(clerkCurrentCustomer,id),0);

		doJob(type);
		
		Signal(myClerkLock, myClerkCV);/*tell customer jobs done*/
		Wait(myClerkLock, myClerkCV);/*wait for customer to leave*/
		Release(myClerkLock);/*customer gone, next customer*/
      }
    else  if(GetMonitor(clerkLineCount, id) > 0)
      {
		PrintInt("Clerk%i: is Busy\n", 18, id, 0);
		
		Signal(clerkLineLock, myClerkLineCV);
		
		PrintInt("Clerk%i has signalled a Customer to come to their counter\n",59, id,0);
		SetMonitor(clerkState, id, 1); /*busy*/
		
		/*acquire clerk lock and release line lock*/
		Acquire(myClerkLock);
		Release(clerkLineLock);
		Wait(myClerkLock, myClerkCV); /*WAS IN b4*/
		
		/*once we're here, the customer is waiting for me to do my job*/
		PrintInt("Clerk%i has received SSN %i from Customer", 45, id, GetMonitor(clerkCurrentCustomerSSN,id));
		PrintInt("%d\n",4, GetMonitor(clerkCurrentCustomer,id),0);

		doJob(type);
		
		Signal(myClerkLock, myClerkCV);/*tell customer jobs done*/
		Wait(myClerkLock, myClerkCV);/*wait for customer to leave*/
		Release(myClerkLock); /*we're done here, back to top of while for next cust*/
      }
    else if (GetMonitor(clerkLineCount, id) == 0 && (GetMonitor(clerkBribeLineCount, id) == 0))  /*go on break*/
      {
		if(GetMonitor(simulationEnded,0)) {
			break;
		}
		/*acquire my lock*/
		Acquire(myClerkLock);
		/*set my status*/
		SetMonitor(clerkState, id, 2); /*on break*/
		
		PrintInt("Clerk%i is going on break\n", 27, id, 0);
		
		Release(clerkLineLock);
		/*wait on clerkBreakCV from manager*/
		Wait(myClerkLock, GetMonitor(clerkBreakCV, id));
		
		PrintInt("Clerk%i is coming off break\n",29, id, 0);
      }
  }
}
/* TODO:there needs to be one main per entity I think. split up the below*/

/*to minimize repeated definition of names, since for the most part we use only ids, just make one name for all (though keeping naming functionality for future use just in case)*/
char* custName = "Customer";
char* clerkName="Clerk";
char* senatorName="Senator";

int run(){
	setup();
	CreateClerk();
	/*Run clerk*/
	Clerk_Run(myType);
	
	Exit(0);
}

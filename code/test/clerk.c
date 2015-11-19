/* clerk.c

*/
#include "syscall.h"
#include "setup.h"
int a[3];
int b, c;




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
	clerkLock[clerksInBuilding] = CreateLock("ClerkLock", 9);
	PrintInt("ClerkLock: %i, clerksInBuilding: %i\n", 37, clerkLock[clerksInBuilding], clerksInBuilding);
	if(clerkLock[clerksInBuilding]<0) {
		Halt();
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
void doJob(int type){
	switch (type) {
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
void Clerk_Run(int type)
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

		doJob(type);
		
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
/* TODO:there needs to be one main per entity I think. split up the below*/

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
	/* init all individual clerk*/

	
		clerkLock[i] = -1;
		/*Condition Variables*/
		clerkLineCV[i] = -1;
		clerkBribeLineCV[i] = -1;
		clerkCV[i] = -1;/*I think we need this? -Jack*/
		clerkBreakCV[i] = -1; 
	/*Run clerk*/
	Clerk_Run(APPLICATION_CLERK_TYPE);
	
	Exit(0);
}

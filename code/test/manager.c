/* manager.c

*/
#include "syscall.h"
#include "setup.h"
int a[3];
int b, c;



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

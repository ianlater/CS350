/* manager.c

*/
#include "syscall.h"
#include "setup.h"
int a[3];
int b, c;



int total, clerkLockI;
void OutputEarnings()
{
	/* should we wrap this in  a lock? */
	total = 0;
	for (i =0; i < NUM_CLERK_TYPES; i++) {
		total += GetMonitor(totalEarnings, i])
	}
	Print("\n----Earnings report:---- \n", 30, "","");
	PrintInt("ApplicationClerks: %i \n", 30,GetMonitor(totalEarnings, APPLICATION_CLERK_TYPE),0);
	PrintInt("PictureClerks: %i \n",30,GetMonitor(totalEarnings, PICTURE_CLERK_TYPE),0);
	PrintInt("PassportClerks: %i \n",30,GetMonitor(totalEarnings, PASSPORT_CLERK_TYPE),0);
	PrintInt("Cashiers: %i \n",30,GetMonitor(totalEarnings, CASHIER_CLERK_TYPE),0);
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
		clerkLockI = GetMonitor(clerkLock,i);
		if (GetMonitor(clerkState, i) == 2 && (GetMonitor(clerkLineCount, i) >= 1 || GetMonitor(clerkBribeLineCount, i) >= 1 || GetMonitor(senatorInBuilding,0)) )
		{
			/*wake up clerk*/
			if(Acquire(clerkLockI) < 0) {
					Halt();
			}	
			PrintInt("Manager waking up Clerk%i\n", 27, clerks[i].id, 0);
				SetMonitor(clerkState, i, 0);/*set to available	*/
			Signal(clerkLockI, GetMonitor(clerkBreakCV, i));	
			Release(clerkLockI);	
		}
	}
	OutputEarnings();
	if (GetMonitor(simulationStarted,0) && GetMonitor(customersInBuilding,0) == 0 && GetMonitor(senatorsAlive,0) == 0) {
		SetMonitor(simulationEnded,0, true);
		for (i = 0; i < NUM_CLERKS; i++)
		{
				clerkLockI = GetMonitor(clerkLock,i);
				/*wake up clerk*/
				Acquire(clerkLockI);	
				PrintInt("Manager waking up Clerk%i\n", 27, clerks[i].id, 0);
				SetMonitor(clerkState, i, 0);/*set to available	*/
				Signal(clerkLockI, GetMonitor(clerkBreakCV, i));	
				Release(clerkLockI);	
		}
		Print("Manager ending simulation\n", 27, "", "");
		break;
	}
  }
  Exit(0);
}

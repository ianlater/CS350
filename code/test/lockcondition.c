/********
*lockcondition.c
*
*tests the creation, interactions, and destroying of locks and conditions 
*********/
#include "syscall.h"

int a[30];
int main()
{
	int lock = CreateLock("lock", 4);
	int lock2 = CreateLock("lock2",5);
	int condition1 = CreateCondition("condition1", 10);
	int condition2 = CreateCondition("condition2", 10);
/*char* arg = "Test";
//	Print("Test %s %s\n", 16, "tes", arg);
//	Print("Test2 %s %i\n", 24, "longeraaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", lock); */
	
	/*TEST 1: test wait user input*/
	/*invalid locks or conditions or combination, expect errors*/
	Print("Invalid waits \n", 15,"","");
	Wait(-1, condition1); 
	Wait(lock, -1); 
	Wait(10, 10);
	
	/*Test 2: test acquire*/
	/*invalid lock, expect error*/
	Print("Invalid Acquire\n", 16,"","");
	Acquire(10);
	/*Acquire lock successfully*/
	Acquire(lock);	
	Print("Lock acquired successfully\n\n", 29,"","");
	
	/*Test 3: test release*/
	/*invalid release, expect error*/
	Print("Invalid release\n", 16,"","");
	Release(-1);
	/*release successfully*/
	Release(lock);
	Print("Lock released successfully\n\n", 29,"","");	
	
	/*Test 4: test signal, acquire, broadcast*/
	/*invalid signal*/
	Print("Trying to signal invalid\n",23,"","");
	Signal(-1, condition2);
	Signal(10, condition1);
	Signal(lock2, -1);
	Signal(lock2, 10);
	/*invalid wait*/
	Print("Trying to wait invalid\n", 23, "", "");
	Wait(-1, -1);
	Wait(10, -1);
	Wait(lock2, 10);
	/*test acquire, wait, signal - no errors here*/
	Print("Testing signal, wait, acquire\n", 30, "", "");
	Acquire(lock2);
	Release(lock2);	
	
	/*Test 5: destroying everything*/
	Print("Invalid destroy\n",15,"","");
	DestroyCondition(-1);
	DestroyCondition(condition1);
	DestroyCondition(condition2);
	Print("Conditions destroyed successfully\n", 32, "", "");
	Print("Invalid destroy\n",15,"","");
	DestroyLock(-1);
	DestroyLock(lock);
	DestroyLock(lock2);
	Print("Locks destroyed successfully\n\n", 30, "", "");
}

/* fork.c
 *	Simple program to test whether forking a user program works.
 */

#include "syscall.h"

void testFunc()
{
	CreateLock("FORKLOCK", 8);
	Print("Lock created\n", 13, "", "");
	Exit(0);
}

void doit()
{
	CreateCondition("FORKCON",7);
	Print("Condition created\n", 18, "", "");
	Exit(0);/*Forked functions MUST have exit*/
}

void fooBar()
{
  Fork(doit);
	Print("Doit forked in foobar\n", 22, "", "");
	/*invalid lock creation*/
	Print("Expecting invalid lock\n", 24, "", "");
  CreateLock("poop", -1);
	Print("Lock created in foobar\n", 23, "", "");
  Exit(0);
}
int
main()
{
	int i;
  /*fork a create lock */
	Fork(testFunc);
 	/*Fork a create condition */ 
	Fork(doit);
	/*Fork a doit and createlock*/
	Fork(fooBar);

	/*loop 50 times */
	for (i = 0; i < 50; i++)
	{
		Fork(testFunc);
	}
}




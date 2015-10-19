/* fork.c
 *	Simple program to test whether forking a user program works.
 */

#include "syscall.h"

void testFunc()
{
  CreateLock("FORKLOCK", 8);
  Exit(0);
}

void doit()
{
  CreateCondition("FORKCON",7);
  Exit(0);/*Forked functions MUST have exit*/
}

void fooBar()
{
  /*  Fork(doit);*/
  CreateLock("poop", -1);
  Exit(0);
}
int
main()
{
  Fork(testFunc);
  Fork(doit);
  Fork(fooBar);
  /*Halt();*/
    /* not reached */
}




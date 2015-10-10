/********
*createlock.c
*
*tests the creation of a lock
*********/
#include "syscall.h"

int a[20];
int main()
{
	int lock = CreateLock("lock", 4);
	int condition = CreateCondition("condition", 9);
}

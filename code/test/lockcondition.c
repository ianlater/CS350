/********
*createlock.c
*
*tests the creation of a lock
*********/
#include "syscall.h"

int a[20];
int main()
{
	Print("Test", 4, "test", "test");
	int lock = CreateLock("lock", 4);
	int condition = CreateCondition("condition", 9);
	DestroyCondition(-1);
	DestroyCondition(condition);
	DestroyLock(-1);
	DestroyLock(lock);
}

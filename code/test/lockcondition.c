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
	char* arg = "Test";
	Print("Test %s %s\n", 16, "tes", arg);
	Print("Test2 %s %i\n", 24, "longeraaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", lock);
	PrintInt("Int Print Test %i, %i\n", 36, 11, lock);
	DestroyCondition(-1);
	DestroyCondition(condition);
	DestroyLock(-1);
	DestroyLock(lock);
}

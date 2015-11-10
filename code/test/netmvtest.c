/********
*netmvtest.c
*
*tests the creation, interactions, and destroying of Monitor Variables
*using a networked server connection 
*********/
#include "syscall.h"
int mVar;
int lock;
int cv;
int main()
{
  int mv = CreateMonitor();
  Print("Monitor created: %s\n", mv, "", "");
  SetMonitor(mv,0, 32);
  mVar = GetMonitor(mv, 0);
  DestroyMonitor(mv);

  lock = CreateLock("barf", 4);
  Acquire(lock);
  cv = CreateCondition("cond", 4);
  Broadcast(0,0);
}

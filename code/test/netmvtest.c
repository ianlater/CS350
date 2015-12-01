/********
*netmvtest.c
*
*tests the creation, interactions, and destroying of Monitor Variables
*using a networked server connection 
*********/
#include "syscall.h"
int mVar;
int lock;
int otherlock;
int differentlock;
int cv;
int cv2;
int cv3;
int mv;
int mv2;
int mv3;
int main()
{
  cv = CreateCondition("test", 4);
  cv2 = CreateCondition("test", 4);
  cv3 = CreateCondition("testes", 6);
  mv = CreateMonitor(0, "test", 4);
  mv2 = CreateMonitor(0, "test", 4);
  mv3 = CreateMonitor(0, "testes", 6);
 
  Print("Monitor created: %s\n", mv, "", "");
  SetMonitor(mv,0, 3);/*not 32? space ascii?*/
  mVar = GetMonitor(mv, 0);
  DestroyMonitor(mv);

  lock = CreateLock("barf", 4);
  otherlock = CreateLock("barf", 4);
  differentlock = CreateLock("barfbarfbarf", 12);
  Acquire(lock);
  cv = CreateCondition("cond", 4);
  Broadcast(0,0);
}

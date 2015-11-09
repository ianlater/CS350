/********
*signal.c
*test networking
*tests the creation, interactions, and destroying of Monitor Variables
*using a networked server connection 
*********/
#include "syscall.h"
int lock;
int cv;
int monitor;
int val;
int i = 0;
int main()
{
  Acquire(0);
  val = GetMonitor(0,0);
  val--;
  PrintInt("Decrement Val to:%d\n", val, 0, 0);
  SetMonitor(0,0,val);
  Broadcast(0,0);
  Print("Yield to test lock release\n", 60, "", "");
  for(i = 0; i<20000;i++)
    {
      Yield();
    }
  Print("Releasing Lock\n",40,"","");
  Release(0);
}

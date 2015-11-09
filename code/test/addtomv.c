/********
 *add to mv 
*********/
#include "syscall.h"
int lock;
int cv;
int monitor;
int val;
int main()
{
  Acquire(0);
  val = GetMonitor(0,0);
  val++;
  SetMonitor(0,0, val);
  Release(0);
  while(1==1)
    {
      Wait(0,0);
      val++;
      SetMonitor(0,0,val);
      val = GetMonitor(0,0);
      PrintInt("Val is now %d\n",20, val,0);
    }
}

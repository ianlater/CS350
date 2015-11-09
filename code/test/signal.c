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
int main()
{
  Acquire(0);
  val = GetMonitor(0,0);
  val--;
  SetMonitor(0,0,val);
  Broadcast(0,0);
  Release(0);
}

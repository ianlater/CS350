/********
*netmvtest.c
*
*tests the creation, interactions, and destroying of Monitor Variables
*using a networked server connection 
*********/
#include "syscall.h"
int mVar;
int main()
{
  int mv = CreateMonitor();
  Print("Monitor created: %s\n", mv, "", "");
  SetMonitor(mv, 32);
  mVar = GetMonitor(mv, 0);
  DestroyMonitor(mv);
}

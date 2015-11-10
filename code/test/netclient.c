/********
*netmvtest.c
*
*tests the creation, interactions, and destroying of Monitor Variables
*using a networked server connection 
*********/
#include "syscall.h"
int lock;
int cv;
int monitor;
int main()
{


  Print("Create lock, create cv, create monitor, start monitor val at 1", 100,"","");
  
  CreateLock("Al",2);
  CreateCondition("Acv", 3);
  monitor = CreateMonitor();
  SetMonitor(monitor,0, 1);

}

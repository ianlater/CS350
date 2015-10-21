
#include "syscall.h"
int a[3];
int b, c;

int
main()
{
  b= CreateLock("myLock",5);
  Print("should only acquire first Lock followed by 3 fails\n", 51, "", "");
  Acquire(b);
  Acquire(10);
  Acquire(-1);
  Acquire(3);
}




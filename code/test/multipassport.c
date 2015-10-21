/* multipassport.c
 */

#include "syscall.h"
int a[3];
int b, c;

int
main()
{
    Exec("../test/passport",16);
    Exec("../test/passport",16);
    /*dont know how to stop exec negative*/
    /* not reached */
}

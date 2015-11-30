/* gencustomers.c
	makes 5 application and 5 picture clerks for a total of 10 clerks
*/
#include "syscall.h"
#include "setup.h"
int a[3];
int b, c, i;

int main(){
	for(i = 0;i<10; i++){
		Exec("customer.c", 10);
	}
	Exit(0);
}
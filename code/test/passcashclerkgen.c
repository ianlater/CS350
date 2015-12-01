/* passcashclerk.c
	makes 5 passport and 5 cashier clerks for a total of 10 clerks
*/
#include "syscall.h"
#include "setup.h"
int a[3];
int b, c, i;

int main(){
	for(i = 0;i<5; i++){
		Exec("../test/ppclerk", 15);
		Exec("../test/cashclerk", 17);
	}
	Exit(0);
}
/* clerk.c

*/
#include "clerk.h"

int main(){
	setup();
	myType = CASHIER_CLERK_TYPE;
	CreateClerk();
	/*Run clerk*/
	Clerk_Run(myType);
	
	Exit(0);
}

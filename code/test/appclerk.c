/* clerk.c

*/
#include "clerk.h"

int main(){
	setup();
	myType = APPLICATION_CLERK_TYPE;
	CreateClerk();
	/*Run clerk*/
	Clerk_Run(myType);
	
	Exit(0);
}

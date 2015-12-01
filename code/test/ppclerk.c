/* clerk.c

*/
#include "clerk.h"

int main(){
	setup();
	myType = PASSPORT_CLERK_TYPE;
	CreateClerk();
	/*Run clerk*/
	Clerk_Run(myType);
	
	Exit(0);
}

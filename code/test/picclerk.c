/* clerk.c

*/
#include "clerk.h"




int main(){
	setup();
	myType = PICTURE_CLERK_TYPE;
	CreateClerk();
	/*Run clerk*/
	Clerk_Run(myType);
	
	Exit(0);
}

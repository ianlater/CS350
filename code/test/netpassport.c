#include "syscall.h"
#include "setup.h"
int a[3];
int b, c;

int main() 
{

/*Lock and Condition Variables*/
/* Values refer lock or condition index */
 clerkLock = CreateMonitor(NUM_CLERKS);/*individual locks created in clerk initiation*/
 clerkLineLock = CreateLock();
 outsideLock = CreateLock();
 senatorLock = CreateLock();
 createLock = CreateLock(); /*since fork takes no params, and agent creation based off global, need lock to avoid race conditions of creating same id*/

 clerkLineCV = CreateMonitor(NUM_CLERKS);/*individual cv's created in clerk initiation*/
 clerkBribeLineCV = CreateMonitor(NUM_CLERKS);
 clerkCV = CreateMonitor(NUM_CLERKS);
 clerkBreakCV  = CreateMonitor(NUM_CLERKS); /*CV for break, for use with manager*/
 senatorCV = CreateCondition();

/*Monitor Variables*/
 clerkLineCount = CreateMonitor(NUM_CLERKS);/*start big so we can compare later*/
 clerkBribeLineCount = CreateMonitor(NUM_CLERKS);
 clerkState = CreateMonitor(NUM_CLERKS);/*keep track of state of clerks with s 0=free,1=busy,2-on breaK /*sidenote:does anyone know how to do enums? would be more expressive?*/
 totalEarnings[NUM_CLERK_TYPES] = CreateMonitor(NUM_CLERK_TYPES);/*keep track of money submitted by each type of clerk*/
 customersInBuilding = CreateMonitor(1);
 clerksInBuilding = CreateMonitor(1);
 managersInBuilding = CreateMonitor(1);
 senatorsAlive = CreateMonitor(1);
 senatorInBuilding = CreateMonitor(1);
 clerkCurrentCustomer = CreateMonitor(NUM_CLERKS);/*relate clerk id to customer id*/
 clerkCurrentCustomerSSN = CreateMonitor(NUM_CLERKS);/*relate clerk id to customer ssn*/
 currentSenatorId = CreateMonitor(1);
 simulationStarted = CreateMonitor(1);/*so simulation doesn't end before customers enter*/
 simulationEnded = CreateMonitor(1);

/*definitions:TODO rework how these are used since they'll be separate exec instances */
/* MV to keep track of entities. Values refer to mailbox# of entity. */
 clerkIds = CreateMonitor(NUM_CLERKS);
 clerkTypes = CreateMonitor(NUM_CLERKS);
 
 customers = CreateMonitor(NUM_CUSTOMERS);
 manager = CreateMonitor(1);	
 
 /* populate pp office */
 Exec("picappclerkgen.c");
 Exec("passcashclerkgen.c");
 
 
 Exit(0);
}


#define APPLICATION_CLERK_TYPE 0
#define PICTURE_CLERK_TYPE 1
#define PASSPORT_CLERK_TYPE 2
#define CASHIER_CLERK_TYPE 3
#define NUM_CLERK_TYPES 4
#define NUM_CLERKS 16
#define NUM_CUSTOMERS 20
#define NUM_SENATORS 0

typedef enum
{
  false,
  true
} bool;

/*struct declarations*/
struct Clerk
{
  int type ;/*represents type of clerk 1 = ApplicationClerk, 2 = PictureClerk, 3 = PassportClerk (used to to facilitate abstract use of clerk)*/
  int id;
};

struct Customer
{
  int money ;
  int myLine;/*-1 represents not in a line*/
  int id ;
  int ssn; /*unique ssn for each customer*/
  int credentials;
  bool rememberLine;
  bool isBribing;
  bool isSenator;
};

struct Manager
{
};

/*Lock and Condition Variables*/
/* Values refer lock or condition index */
int clerkLock;/*individual locks created in clerk initiation*/
int clerkLineLock;
int outsideLock;
int senatorLock;
int createLock; /*since fork takes no params, and agent creation based off global, need lock to avoid race conditions of creating same id*/

int clerkLineCV;/*individual cv's created in clerk initiation*/
int clerkBribeLineCV;
int clerkCV;
int clerkBreakCV ; /*CV for break, for use with manager*/
int senatorCV;
int outsideCV; /* how customers wait outside */
int senatorLineCV; /* how senators wait in line */

/*Monitor Variables*/
int clerkLineCount;/*start big so we can compare later*/
int clerkBribeLineCount;
int clerkState;/*keep track of state of clerks with ints 0=free,1=busy,2-on breaK /*sidenote:does anyone know how to do enums? would be more expressive?*/
int totalEarnings;/*keep track of money submitted by each type of clerk*/
int customersInBuilding;
int clerksInBuilding;
int senatorsAlive;
bool senatorInBuilding;
int clerkCurrentCustomer;/*relate clerk id to customer id*/
int clerkCurrentCustomerSSN;/*relate clerk id to customer ssn*/
int currentSenatorId ;
bool simulationStarted;/*so simulation doesn't end before customers enter*/
bool simulationEnded;
int activeCustomers;

/* clerk 'struct' mvs */
int clerkIds;
int clerkTypes;
int c_id;
int c_ssn;

/*definitions:TODO rework how these are used since they'll be separate exec instances */
/* MV to keep track of entities. Values refer to mailbox# of entity. */
int clerks[NUM_CLERKS];
int customers;
int manager;

void setup(){
	
/*Lock and Condition Variables*/
/* Values refer lock or condition index */
 clerkLock = CreateMonitor(NUM_CLERKS, "clerkLocks", 9);/*individual locks created in clerk initiation*/
 clerkLineLock = CreateLock("clerkLineLock", 13);
 outsideLock = CreateLock("outsideLock", 11);
 senatorLock = CreateLock("senatorLock", 11);
 createLock = CreateLock("createLock", 10); /*since fork takes no params, and agent creation based off global, need lock to avoid race conditions of creating same id*/

 clerkLineCV = CreateMonitor(NUM_CLERKS, "clerkLineCVs", 12);/*individual cv's created in clerk initiation*/
 clerkBribeLineCV = CreateMonitor(NUM_CLERKS, "clerkBribeLineCVs", 17);
 clerkCV = CreateMonitor(NUM_CLERKS, "clerkCVs", 8);
 clerkBreakCV  = CreateMonitor(NUM_CLERKS, "clerkBreakCVs", 13); /*CV for break, for use with manager*/
 senatorCV = CreateCondition("senatorCV", 9);
 outsideCV = CreateCondition("outsideCV", 9);
 senatorLineCV = CreateCondition("senatorLineCV", 13);
 
/*Monitor Variables*/
 clerkLineCount = CreateMonitor(NUM_CLERKS, "clerkLineCounts", 15);/*start big so we can compare later*/
 clerkBribeLineCount = CreateMonitor(NUM_CLERKS, "clerkBribeLineCounts", 20);
 clerkState = CreateMonitor(NUM_CLERKS, "clerkStates", 11);/*keep track of state of clerks with s 0=free,1=busy,2-on breaK /*sidenote:does anyone know how to do enums? would be more expressive?*/
 totalEarnings = CreateMonitor(NUM_CLERK_TYPES, "totalEarnings", 13);/*keep track of money submitted by each type of clerk*/
 customersInBuilding = CreateMonitor(1, "customersInBuilding", 19);
 clerksInBuilding = CreateMonitor(1, "clerksInBuilding", 16);
 senatorsAlive = CreateMonitor(1, "senatorsAlive", 13);
 senatorInBuilding = CreateMonitor(1, "senatorInBuilding", 17);
 clerkCurrentCustomer = CreateMonitor(NUM_CLERKS, "clerkCurrentCustomer",20);/*relate clerk id to customer id*/
 clerkCurrentCustomerSSN = CreateMonitor(NUM_CLERKS, "clerkCurrentCustomerSSN", 23);/*relate clerk id to customer ssn*/
 currentSenatorId = CreateMonitor(1, "currentSenatorId", 16);
 simulationStarted = CreateMonitor(1, "simulationStarted", 17);/*so simulation doesn't end before customers enter*/
 simulationEnded = CreateMonitor(1, "simulationEnded", 15);
 activeCustomers = CreateMonitor(1, "activeCustomers", 15);
 c_id = CreateMonitor(NUM_CUSTOMERS + NUM_SENATORS, "c_ids", 5);
 c_ssn = CreateMonitor(NUM_CUSTOMERS + NUM_SENATORS, "c_ssns", 6);
/*definitions:TODO rework how these are used since they'll be separate exec instances */
/* MV to keep track of entities. Values refer to mailbox# of entity. */
 clerkIds = CreateMonitor(NUM_CLERKS, "clerkIds", 8);
 clerkTypes = CreateMonitor(NUM_CLERKS, "clerkTypes", 10);
 
 customers = CreateMonitor(NUM_CUSTOMERS, "customers", 9);
 manager = CreateMonitor(1, "manager", 7);	
}
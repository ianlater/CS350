
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
  int type = CreateMonitor(1);/*represents type of clerk 1 = ApplicationClerk, 2 = PictureClerk, 3 = PassportClerk (used to to facilitate abstract use of clerk)*/
  int id = CreateMonitor(1);
};

struct Customer
{
  int money = CreateMonitor(1);
  int myLine = CreateMonitor(1);/*-1 represents not in a line*/
  int id = CreateMonitor(1);
  int ssn = CreateMonitor(1); /*unique ssn for each customer*/
  int credentials = CreateMonitor(NUM_CLERK_TYPES);
  bool rememberLine = CreateMonitor(1);
  bool isBribing = CreateMonitor(1);
  bool isSenator = CreateMonitor(1);
};

struct Manager
{
};

/*Lock and Condition Variables*/
/* Values refer lock or condition index */
int clerkLock = CreateMonitor(NUM_CLERKS);
int clerkLineLock = CreateMonitor(1);
int outsideLock = CreateMonitor(1);
int senatorLock = CreateMonitor(1);
int createLock = CreateMonitor(1); /*since fork takes no params, and agent creation based off global, need lock to avoid race conditions of creating same id*/

int clerkLineCV = CreateMonitor(NUM_CLERKS);
int clerkBribeLineCV = CreateMonitor(NUM_CLERKS);
int clerkCV = CreateMonitor(NUM_CLERKS);/*I think we need this? -Jack*/
int clerkBreakCV  = CreateMonitor(NUM_CLERKS); /*CV for break, for use with manager*/
int senatorCV = CreateMonitor(1);

/*Monitor Variables*/
int clerkLineCount = CreateMonitor(NUM_CLERKS);/*start big so we can compare later*/
int clerkBribeLineCount = CreateMonitor(NUM_CLERKS);
int clerkState = CreateMonitor(NUM_CLERKS);/*keep track of state of clerks with ints 0=free,1=busy,2-on breaK /*sidenote:does anyone know how to do enums? would be more expressive?*/
int totalEarnings[NUM_CLERK_TYPES] = CreateMonitor(NUM_CLERK_TYPES);/*keep track of money submitted by each type of clerk*/
int customersInBuilding = CreateMonitor(1);
int clerksInBuilding = CreateMonitor(1);
int managersInBuilding = CreateMonitor(1);
int senatorsAlive = CreateMonitor(1);
bool senatorInBuilding = CreateMonitor(1);
int clerkCurrentCustomer = CreateMonitor(NUM_CLERKS);/*relate clerk id to customer id*/
int clerkCurrentCustomerSSN = CreateMonitor(NUM_CLERKS);/*relate clerk id to customer ssn*/
int currentSenatorId = CreateMonitor(1);
bool simulationStarted = CreateMonitor(1);/*so simulation doesn't end before customers enter*/
bool simulationEnded = CreateMonitor(1);

/*definitions:TODO rework how these are used since they'll be separate exec instances */
/* MV to keep track of entities. Values refer to mailbox# of entity. */
int clerks = CreateMonitor(NUM_CLERKS);
int customers = CreateMonitor(NUM_CUSTOMERS);
int manager = CreateMonitor(1);

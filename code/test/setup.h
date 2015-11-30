
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
int managersInBuilding;
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

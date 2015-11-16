
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
  int type;/*represents type of clerk 1 = ApplicationClerk, 2 = PictureClerk, 3 = PassportClerk (used to to facilitate abstract use of clerk)*/
  char* name;
  int id;
};

struct Customer
{
  char* name;
  int money;
  int myLine;/*-1 represents not in a line*/
  int id;
  int ssn; /*unique ssn for each customer*/
  int credentials[NUM_CLERK_TYPES];
  bool rememberLine;
  bool isBribing;
  bool isSenator;
};

struct Manager
{
  char* name;
};

/*
Monitor setup:
array of lock(ptrs) for each clerk+their lines
*/

int clerkLock[NUM_CLERKS];
int clerkLineLock = -1;
int outsideLock = -1;
int senatorLock = -1;
int createLock = -1; /*since fork takes no params, and agent creation based off global, need lock to avoid race conditions of creating same id*/

/*Condition Variables*/
int clerkLineCV[NUM_CLERKS ];
int clerkBribeLineCV[NUM_CLERKS];
int clerkCV[NUM_CLERKS];/*I think we need this? -Jack*/
int clerkBreakCV[NUM_CLERKS] ; /*CV for break, for use with manager*/
int senatorCV = -1;

/*Monitor Variables*/
int clerkLineCount[NUM_CLERKS] = {0};/*start big so we can compare later*/
int clerkBribeLineCount[NUM_CLERKS] = {0};
int clerkState[NUM_CLERKS] = {0};/*keep track of state of clerks with ints 0=free,1=busy,2-on breaK /*sidenote:does anyone know how to do enums? would be more expressive?*/
int totalEarnings[NUM_CLERK_TYPES] = {0};/*keep track of money submitted by each type of clerk*/
int customersInBuilding = 0;
int clerksInBuilding = 0;
int managersInBuilding = 0;
int senatorsAlive = 0;
bool senatorInBuilding = false;
int clerkCurrentCustomer[NUM_CLERKS];/*relate clerk id to customer id*/
int clerkCurrentCustomerSSN[NUM_CLERKS];/*relate clerk id to customer ssn*/
int currentSenatorId = -1;
int i;/*iterator for loops*/
bool simulationStarted = false;/*so simulation doesn't end before customers enter*/
bool simulationEnded = false;

/*definitions:TODO rework how these are used since they'll be separate exec instances */
struct Clerk clerks[NUM_CLERKS];/*global array of clerks*/
struct Customer customers[NUM_CUSTOMERS];
struct Manager manager;

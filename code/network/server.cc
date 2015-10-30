///////
//The main server run by server machine for part 3 project 3
//essentially a while(true) loop, reading messages and operating on them
///////
#include "copyright.h"

#include "system.h"
#include "network.h"
#include "post.h"
#include "interrupt.h"
#include <sstream>



/*Const declarations*/
const int TABLE_SIZE = 500;

int serverLockCounter = 0;
int serverCVCounter = 0;

enum requestType {
  CL,
  CCV,
  DL,
  DCV,
  UNKNOWN,
};

/*ServerLock struct and methods*/
struct ServerLock {
  char* name;
  bool isAvailable;
  int ownerID;
  List* waitQueue;

  ServerLock(char* n);
};


ServerLock::ServerLock(char* n)
{
  name = n;
  isAvailable = true;
  //set ownerId TODO
}

/*ServerCondition struct and methods*/
struct ServerCondition {
  char* name;
  bool isAvailable;
  int ownerID;
  List* waitQueue;

  ServerCondition(char* name);
};

ServerCondition::ServerCondition(char* n)
{
  name = n;
  isAvailable = true;
  //TODO set OwnerID
}

//helper functions
requestType getType(string req)
{
  if(req == "CL") return CL;
  if(req == "CCV") return CCV;
  if(req == "DL") return DL;
  if(req == "DCV") return DCV;

return UNKNOWN;
}


//vars below
ServerLock* ServerLockTable[TABLE_SIZE];
ServerCondition* ServerCVTable[TABLE_SIZE];

/*LOCK PROCEDURES*/
int doCreateLock(char* name)
{
  if(serverLockCounter > TABLE_SIZE)
  {
    printf("Server::ERROR Cannot create lock\n");
    return -1;
  }
  ServerLock* newLock = new ServerLock(name);
  int thisLock = serverLockCounter;
  ServerLockTable[thisLock] = newLock;
  serverLockCounter++;
 printf("Server::DoCreateLock: %s ID:%d\n", name, thisLock);
  
 return thisLock;
}

void doDestroyLock(char* name)
{

}



/*CONDTION PROCEDURES*/
int doCreateCV(char* name)
{
  printf("Server::DoCreateCV\n");
  if(serverCVCounter > TABLE_SIZE)
  {
    printf("Server::ERROR Cannot create cv\n");
    return -1;
  }
  ServerCondition* newCV = new ServerCondition(name);
  int thisCV = serverCVCounter;
  ServerCVTable[thisCV] = newCV;
  serverCVCounter++;
 printf("Server::DoCreateCV: %s ID:%d\n", name, thisCV);
  
 return thisCV;
}




/*MAIN SERVER RUN FUNCTION*/
void Server()
{
  printf("SERVER STARTED\n");
  while(true)
    {
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char *data = "Hello there!";
    char *ack = "Got it!";
    char buffer[MaxMailSize];


    // construct packet, mail header for original message
    // To: destination machine, mailbox 0
    // From: our machine, reply to: mailbox 1
    outPktHdr.to = 1;//TODO hard testing		
    outMailHdr.to = 0;
    outMailHdr.from = 1;
    outMailHdr.length = strlen(data) + 1;

    // Send the first message
    //    bool success = postOffice->Send(outPktHdr, outMailHdr, data); 

    postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
    printf("Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);

    stringstream ss;

    ss<<buffer;
    string request;
    ss>>request;

    //which type of request is this
    switch(getType(request))
      {
      case CL:
	{
	  char* lockName;
	  ss>>lockName;

	  stringstream strs;
	 
	  int lock = doCreateLock(lockName);
	  //now, build and send message back to client
	  strs<<lock;
	  string temp = strs.str();
	  char const* msgDataConst = temp.c_str();
	  char* msgData = new char[temp.length()];
	  strcpy(msgData, temp.c_str());
	  outMailHdr.length = strlen(msgData) + 1;

          bool success = postOffice->Send(outPktHdr, outMailHdr, msgData); 

	  break;
	}
      case CCV:
	{
	  char* cvName;
	  ss>>cvName;
	  int cv = doCreateCV(cvName);

	  //now, build and send message back to client
	  stringstream strs;
	  strs<<cv;
	  string temp = strs.str();
	  char const* msgDataConst = temp.c_str();
	  char* msgData = new char[temp.length()];
	  strcpy(msgData, temp.c_str());
	  outMailHdr.length = strlen(msgData) + 1;

          bool success = postOffice->Send(outPktHdr, outMailHdr, msgData); 

	  
	  break;
	}
      default:
	printf("Server:: Unknown Request Type");
	break;
      }
 
    }//end WHILE true
    //while(true)
    //1. receive message
    //2. use stringstream on buffer to get what type of syscall it is
    //3. use switch statement to get to that syscall, and actually call syscall
    ////ex. if CL, get name out of ss, and call doCreateLock, which calls syscall?
}

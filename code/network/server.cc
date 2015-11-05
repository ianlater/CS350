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
  CL,//Create Lock
  DL,//Destroy Lock
  AL,//Acquire lock
  RL,//Release Lock
  CCV,//Create CV
  DCV,//Destroy CV
  UNKNOWN,
};

/*ServerLock struct and methods*/
struct ServerLock {
  string name;
  bool isAvailable;
  bool isToBeDeleted;
  int clientID;
  int currentOwner;//the ID of the thread that owns it
  List* waitQueue;

  ServerLock(string n, int client);
};


ServerLock::ServerLock(string n, int client)
{
  name = n;
  isAvailable = true;
  clientID = client;//ip
  currentOwner = -1;//not owned until acquired
  isToBeDeleted = false;
}

/*ServerCondition struct and methods*/
struct ServerCondition {
  string name;
  bool isAvailable;
  bool isToBeDeleted;
  int clientID;//ip of sender
  int currentOwner;//threadid/mailbox of thread that sent
  List* waitQueue;

  ServerCondition(string name, int client);
};

ServerCondition::ServerCondition(string n, int client)
{
  name = n;
  isAvailable = true;
  isToBeDeleted = false;
  clientID = client;
  currentOwner = -1;//start without owner (only on my lock? TODO)
}

struct Message
{
  int to;//this is the client id
  char* msg;

  Message(int t, char* m);
};

Message::Message(int t, char* m)
{
  to = t;
  msg = m;
}

//helper functions
requestType getType(string req)
{
  if(req == "CL") return CL;
  if(req == "CCV") return CCV;
  if(req == "AL") return AL;
  if(req == "RL") return RL;
  if(req == "DL") return DL;
  if(req == "DCV") return DCV;

return UNKNOWN;
}


void sendMessage(Message msg)
{
  PacketHeader outPktHdr;
  MailHeader outMailHdr;
  char* data = msg.msg;

  outPktHdr.to = msg.to;
  outMailHdr.to = 0;//?
  outMailHdr.from = 1;//?
  outMailHdr.length = strlen(data) + 1;

  bool success = postOffice->Send(outPktHdr, outMailHdr, data);
  if(!success)
    {
      printf("SendMessage::ERROR: could not send message\n");
    }
}


//vars below
ServerLock* ServerLockTable[TABLE_SIZE];
ServerCondition* ServerCVTable[TABLE_SIZE];

/*LOCK PROCEDURES*/
int doCreateLock(string name, int client)
{
  if(serverLockCounter > TABLE_SIZE)
  {
    printf("Server::ERROR Cannot create lock\n");
    return -1;
  }
  ServerLock* newLock = new ServerLock(name,client);
  int thisLock = serverLockCounter;
  ServerLockTable[thisLock] = newLock;
  serverLockCounter++;
  printf("Server::DoCreateLock: ID:%d\n", thisLock);
  
  return thisLock;
}

int doDestroyLock(int lock, int client, int owner)
{
  char* errorMsg;
  if(lock < 0 || lock > serverLockCounter)
    {
     errorMsg = "DestroyLock:: Lock Index out of bounds";
     printf("%s\n", errorMsg);
     Message msg = Message(client, errorMsg);
     sendMessage(msg); 
     return -1;
    }
  ServerLock* sl = ServerLockTable[lock];
  if(!sl)
    {
     errorMsg = "DestroyLock::Error: Lock is null";
     printf("%s\n", errorMsg);
     Message msg = Message(client, errorMsg);
     sendMessage(msg); 
     return -1;
    }
  if(sl->clientID != client)
    {
     errorMsg = "DestroyLock::Error: Lock does not belong to this client";
     printf("%s\n", errorMsg);
     Message msg = Message(client, errorMsg);
     sendMessage(msg); 
     return -1;

    }
  //error checking done
  //check lock's waitQ to delete
  if(sl->waitQueue->IsEmpty())
    {
      printf("DestroyLock:: Deleting Lock?\n");
      delete sl;    
    }
  else
    {
      printf("DestroyLock:: set toBeDeleted\n");
      sl->isToBeDeleted = true;
    }
  
   Message msg = Message(client, "DestroyLock");
   sendMessage(msg);
  return 0;
}

int doAcquireLock(int lockIndex, int clientID, int threadID)
{
  if(lockIndex < 0 || lockIndex >= TABLE_SIZE)
    {
      char* errorString =  "Acquire::ERROR: Lock Index out of bounds";
       printf("%s\n", errorString);
      Message msg = Message(clientID, errorString);
      sendMessage(msg);
      return -1;
    }
  ServerLock* sl = ServerLockTable[lockIndex];
if(!(sl))
    {
      char* errorString =  "Acquire::ERROR: Lock is null";
      printf("%s\n", errorString);
      Message msg = Message(clientID, errorString);
      sendMessage(msg);

      return -1;
    } 
  //Lock exists and input was proper so far, check if lock belongs to this addrSpace

  if(sl->clientID != clientID)//CAREFUL! May need changes for P4
    {
      char* errorString = "Acquire::ERROR: Lock does not belong to this Client";
      printf("%s\n", errorString);
      Message msg = Message(clientID, errorString);
      sendMessage(msg);
      return -1;
    }

 //end of input parsing
 //check if already held by current thread
  if(sl->currentOwner == threadID)
    {
      printf("Server::AcquireLock: thread that owns lock is trying to acquire again?\n");
      Message msg = Message(clientID, "AcquireLock");
      sendMessage(msg);
    }

 //if the lock is not busy, acquire it and wake up client
 //otherwise, make this client wait for the lock's release
 if(sl->isAvailable)
   {
     sl->currentOwner = threadID;
     sl->isAvailable = false;
     Message msg = Message(clientID, "AcquireLock");
     sendMessage(msg);
   }
 else//lock is busy, wait
   {
     Message* msg = new Message(clientID, "AcquireLock");
     sl->waitQueue->Append(msg);
   }

 return 0;
}

int doReleaseLock(int lockIndex, int clientID, int ownerID)
{
 if(lockIndex < 0 || lockIndex >= TABLE_SIZE)
    {
      char* errorString =  "Release: Lock Index out of bounds";
       printf("%s\n", errorString);
      Message msg = Message(clientID, errorString);
      sendMessage(msg);
      return -1;
    }
  ServerLock* sl = ServerLockTable[lockIndex];
if(!(sl))
    {
      char* errorString =  "Release: Lock is null";
      printf("%s\n", errorString);
      Message msg = Message(clientID, errorString);
      sendMessage(msg);

      return -1;
    } 
  //Lock exists and input was proper so far, check if lock belongs to this addrSpace

  if(sl->clientID != clientID)//CAREFUL! May need changes for P4
    {
      char* errorString = "Release:Lock does not belong to client";
      printf("%s\n", errorString);
      Message msg = Message(clientID, errorString);
      sendMessage(msg);
      return -1;
    }

 //end of input parsing
  //if there is now owner, cant release
  if(sl->currentOwner == -1)
    {
      char* errorString = "Release:Lock does not belong to any thread";
      printf("%s\n", errorString);
      Message msg = Message(clientID, errorString);
      sendMessage(msg);
     
      return -1;
    }
  //if i am not the owner, can't release 
  if(sl->currentOwner != ownerID)
    {
      char* errorString = "Release:Lock does not belong to thread";
      printf("%s\n", errorString);
      Message msg = Message(clientID, errorString);
      sendMessage(msg);
     
      return -1;
    }
  char* infoMessage;  
//if the waitQ is empty
  if(sl->waitQueue->IsEmpty())
    {
      if(sl->isToBeDeleted)
	{
	  infoMessage = "Release::lock released and deleted";
	  printf("%s\n", infoMessage);
	  delete sl;	 
	  Message msg = Message(clientID, infoMessage);
	  sendMessage(msg);
	}
      else
	{
	  sl->isAvailable = true;
	  infoMessage = "Release::lock released and available";
	  printf("%s\n", infoMessage);
	  Message msg = Message(clientID, infoMessage);
	  sendMessage(msg);	 
	}
    }
  else
    {
      //send msg at front of waitQ
      printf("Release::Waking up waiting client\n");
      Message* msg = (Message*)sl->waitQueue->Remove();
      sendMessage(*msg); 
    }
  ///if isToBeDeleted, delete
  ///else, just make available
  //else, send off msg to wake up next person in waitQ
  return 0;
}


/*CONDTION PROCEDURES*/
int doCreateCV(string name, int client)
{
  printf("Server::DoCreateCV\n");
  if(serverCVCounter > TABLE_SIZE)
  {
    printf("Server::ERROR Cannot create cv\n");
    return -1;
  }
  ServerCondition* newCV = new ServerCondition(name, client);
  int thisCV = serverCVCounter;
  ServerCVTable[thisCV] = newCV;
  serverCVCounter++;
 printf("Server::DoCreateCV: ID:%d\n",  thisCV);
  
 return thisCV;
}

int doDestroyCV(int cvIndex, int client)
{
  char* errorMsg;
  if(cvIndex < 0 || cvIndex > serverLockCounter)
    {
     errorMsg = "DestroyCV:: cv Index out of bounds";
     printf("%s\n", errorMsg);
     Message msg = Message(client, errorMsg);
     sendMessage(msg); 
     return -1;
    }
  ServerCondition* sc = ServerCVTable[cvIndex];
  if(!sc)
    {
     errorMsg = "DestroyCV: CV is null";
     printf("%s\n", errorMsg);
     Message msg = Message(client, errorMsg);
     sendMessage(msg); 
     return -1;
    }
  if(sc->clientID != client)
    {
     errorMsg = "DestroyCV: CV does not belong to this client";
     printf("%s\n", errorMsg);
     Message msg = Message(client, errorMsg);
     sendMessage(msg); 
     return -1;

    }
  //error checking done
  //check cv's waitQ to delete
  if(sc->waitQueue->IsEmpty())
    {
      printf("DestroyCV:: Deleting CV\n");
      delete sc;    
    }
  else
    {
      printf("DestroyCV:: set toBeDeleted\n");
      sc->isToBeDeleted = true;
    }
  
   Message msg = Message(client, "DestroyCV");
   sendMessage(msg);
  return 0;
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

    postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);//TODO check mail isn't bigger than buffer
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
	  string lockName;
	  //printf("NAME: %s\n", lockName);
	  stringstream strs;
	  ss>>lockName;
	 
	  int lock = doCreateLock(lockName, inPktHdr.from);
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
      case AL:
	{
	  string lockIndex;
	  ss>>lockIndex;
	  int lockInt = atoi(lockIndex.c_str());
	  doAcquireLock(lockInt, inPktHdr.from, inMailHdr.from);
	  break;
	}
      case RL:
	{
	  string lockIndex;
	  ss>>lockIndex;
	  int lockInt = atoi(lockIndex.c_str());
	  doReleaseLock(lockInt, inPktHdr.from, inMailHdr.from);
	  break;
	}
      case DL:
	{
	  string lockIndex;
	  ss>>lockIndex;
	  int lockInt = atoi(lockIndex.c_str());
	  doDestroyLock(lockInt, inPktHdr.from, inMailHdr.from);
	  break;
	}
      case CCV:
	{
	  string cvName;
	  ss>>cvName;
	  int cv = doCreateCV(cvName, inPktHdr.from);

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
      case DCV:
	{
	  string cvIndex;
	  ss>>cvIndex;
	  int cvInt = atoi(cvIndex.c_str());
	  doDestroyCV(cvInt, inPktHdr.from);
	  break;
	}
      default:
	{
	  printf("Server:: Unknown Request Type");
	  break;
	}
      }
 
    }//end WHILE true
    //while(true)
    //1. receive message
    //2. use stringstream on buffer to get what type of syscall it is
    //3. use switch statement to get to that syscall, and actually call syscall
    ////ex. if CL, get name out of ss, and call doCreateLock, which calls syscall?
}

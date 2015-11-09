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
const int MV_ARRAY_SIZE = 50;

int serverLockCounter = 0;
int serverCVCounter = 0;
int serverMVCounter = 0;


enum requestType {
  CL,//Create Lock
  DL,//Destroy Lock
  AL,//Acquire lock
  RL,//Release Lock
  CCV,//Create CV
  DCV,//Destroy CV
  SCV,//Signal CV
  WCV,//Wait CV
  BCV,//Broadcast CV
  CMV,//Create Monitor Var
  DMV,//Destroy Monitor Var
  SMV,//Set Monitor Var
  GMV,//Get Monitor Var
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
  waitQueue = new List;
}

/*ServerCondition struct and methods*/
struct ServerCondition {
  string name;
  bool isAvailable;
  bool isToBeDeleted;
  int clientID;//ip of sender
  int currentOwner;//threadid/mailbox of thread that sent
  int waitingLock;
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
  waitingLock = -1; //null on construction
  waitQueue = new List;
}

struct ServerMV
{
  int  data[MV_ARRAY_SIZE];
  bool isAvailable;
  bool isToBeDeleted;
  int clientID;
  int usedLength;//how much of array is used
  List* waitQueue;

  ServerMV(int client);
};

ServerMV::ServerMV(int client)
{
  clientID = client;
  isAvailable = true;
  isToBeDeleted = false;
  waitQueue = new List;
  usedLength = 0;
}

struct Message
{
  int to;//this is the client machineid
  int toMailbox;////this is the mailbox(thread) id on client machine
  char* msg;

  Message(int t,int tm, char* m);
};

Message::Message(int t,int tm, char* m)
{
  to = t;
  toMailbox = tm;
  msg = m;
}

//vars below
ServerLock* ServerLockTable[TABLE_SIZE];
ServerCondition* ServerCVTable[TABLE_SIZE];
ServerMV* ServerMVTable[TABLE_SIZE];

//helper functions
requestType getType(string req)
{
  if(req == "CL") return CL;
  if(req == "CCV") return CCV;
  if(req == "AL") return AL;
  if(req == "RL") return RL;
  if(req == "DL") return DL;
  if(req == "DCV") return DCV;
  if(req == "SCV") return SCV;
  if(req == "WCV") return WCV;
  if(req == "BCV") return BCV;
  if(req == "CMV") return CMV;
  if(req == "DMV") return DMV;
  if(req == "GMV") return GMV;
  if(req == "SMV") return SMV;
return UNKNOWN;
}


void sendMessage(Message msg)
{
  PacketHeader outPktHdr;
  MailHeader outMailHdr;
  char* data = msg.msg;

  outPktHdr.to = msg.to;
  outMailHdr.to = msg.toMailbox;//
  outMailHdr.from = 0;//server mailbox? only one thread
  outMailHdr.length = strlen(data) + 1;

  bool success = postOffice->Send(outPktHdr, outMailHdr, data);
  if(!success)
    {
      printf("SendMessage::ERROR: could not send message\n");
    }
}

bool LockIsValid(int lock, int client)
{
 if(lock < 0 || lock > serverLockCounter)
    {
     return false;
    }
  ServerLock* sl = ServerLockTable[lock];
  if(!sl)
    {
     return false;
    }
  if(sl->clientID != client)
    {
      return true;//we share
    }
  return true;
}

bool CVIsValid(int cv, int client)
{
 if(cv < 0 || cv > serverCVCounter)
    {
     return false;
    }
  ServerCondition* sc = ServerCVTable[cv];
  if(!sc)
    {
     return false;
    }
  if(sc->clientID != client)
    {
      return true;//we share
    }
  return true;
}

bool MVIsValid(int mv, int client)
{
  if(mv < 0 || mv > serverMVCounter)
    {
     return false;
    }
    ServerMV* smv = ServerMVTable[mv];
  if(!smv)
   {
    return false;
   }
  if(smv->clientID != client)
   {
     return true;//we share
   }
  return true;

}

/*LOCK PROCEDURES*/
int doCreateLock(string name, int client, int threadID)
{
  char* errorMsg;
  if(serverLockCounter > TABLE_SIZE)
  {
    printf("Create::ERROR Cannot create lock\n");
    /*    errorMsg = "CreateLock::cannot create MV";
      printf("%s\n", errorMsg);
      Message msg = Message(client, threadID, errorMsg);
     sendMessage(msg);
    */  
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
  if(!LockIsValid(lock, client))
    {
      errorMsg = "DestroyLock::Error. invalid Lock";
      printf("%s\n", errorMsg);
      Message msg = Message(client, owner, errorMsg);
      sendMessage(msg); 
      return -1;
    }
  //error checking done
  ServerLock* sl = ServerLockTable[lock];  
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
  
   Message msg = Message(client, owner, "DestroyLock");
   sendMessage(msg);
  return 0;
}

int doAcquireLock(int lockIndex, int clientID, int threadID)
{
  char* errorMsg;
  if(!LockIsValid(lockIndex, clientID))
    {
      errorMsg = "AcquireLock::Error. invalid Lock";
      printf("%s\n", errorMsg);
      Message msg = Message(clientID, threadID, errorMsg);
      sendMessage(msg); 
      return -1;
    }
 
 //end of input parsing
  ServerLock* sl = ServerLockTable[lockIndex];
 //check if already held by current thread
  if(sl->currentOwner == threadID && sl->clientID == clientID)
    {
      printf("Server::AcquireLock: thread that owns lock is trying to acquire again?\n");
      Message msg = Message(clientID, threadID, "AcquireLock");
      sendMessage(msg);
    }

 //if the lock is not busy, acquire it and wake up client
 //otherwise, make this client wait for the lock's release
 if(sl->isAvailable)
   {
     sl->currentOwner = threadID;
     sl->isAvailable = false;
     Message msg = Message(clientID, threadID, "AcquireLock");
     sendMessage(msg);
   }
 else//lock is busy, wait
   {
     Message* msg = new Message(clientID, threadID, "AcquireLock");
     sl->waitQueue->Append(msg);
   }

 return 0;
}

int doReleaseLock(int lockIndex, int clientID, int threadID)
{
  char* errorMsg;
  if(!LockIsValid(lockIndex, clientID))
    {
      errorMsg = "ReleaseLock::Error. invalid Lock";
      printf("%s\n", errorMsg);
      Message msg = Message(clientID, threadID, errorMsg);
      sendMessage(msg); 
      return -1;
    }
  
 //end of input parsing
  ServerLock* sl = ServerLockTable[lockIndex];
  //if there is no owner, cant release
  if(sl->currentOwner == -1)
    {
      char* errorString = "Release:Lock does not belong to any thread";
      printf("%s\n", errorString);
      Message msg = Message(clientID, threadID, errorString);
      sendMessage(msg);
     
      return -1;
    }
  //if i am not the owner, can't release 
  if(sl->currentOwner != threadID)
    {
      char* errorString = "Release:Lock does not belong to thread";
      printf("%s\n", errorString);
      Message msg = Message(clientID, threadID, errorString);
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
	  Message msg = Message(clientID, threadID, infoMessage);
	  sendMessage(msg);
	}
      else
	{
	  sl->isAvailable = true;
	  infoMessage = "Release::lock released and available";
	  printf("%s\n", infoMessage);
	  Message msg = Message(clientID, threadID, infoMessage);
	  sendMessage(msg);	 
	}
    }
  else//someone is waiting. send 2 reply messages
    {
      //first, wake up person who released lock successfully
      infoMessage = "Release::lock released successfully";
      printf("%s\n", infoMessage);
      Message msg = Message(clientID, threadID, infoMessage);
      sendMessage(msg);	 
      //send msg at front of waitQ to person who now owns lock
      printf("Release::Waking up waiting client\n");
      Message* msg2 = (Message*)sl->waitQueue->Remove();
      sl->currentOwner = msg2->toMailbox;
      sendMessage(*msg2); 
    }
  return 0;
}


/*CONDTION PROCEDURES*/
int doCreateCV(string name, int client, int threadID)
{
  printf("Server::DoCreateCV\n");
  char* errorMsg;
  if(serverCVCounter > TABLE_SIZE)
  {
     errorMsg = "CreateCV::cannot create CV";
      printf("%s\n", errorMsg);
      /*     Message msg = Message(client, threadID, errorMsg);
	     sendMessage(msg);*/
    return -1;
  }
  ServerCondition* newCV = new ServerCondition(name, client);
  int thisCV = serverCVCounter;
  ServerCVTable[thisCV] = newCV;
  serverCVCounter++;
 printf("Server::DoCreateCV: ID:%d\n",  thisCV);
  
 return thisCV;
}

int doDestroyCV(int cvIndex, int client, int threadID)
{
  char* errorMsg;
  if(cvIndex < 0 || cvIndex > serverLockCounter)
    {
     errorMsg = "DestroyCV:: cv Index out of bounds";
     printf("%s\n", errorMsg);
     Message msg = Message(client, threadID, errorMsg);
     sendMessage(msg); 
     return -1;
    }
  ServerCondition* sc = ServerCVTable[cvIndex];
  if(!sc)
    {
     errorMsg = "DestroyCV: CV is null";
     printf("%s\n", errorMsg);
     Message msg = Message(client, threadID, errorMsg);
     sendMessage(msg); 
     return -1;
    }
  if(sc->clientID != client)
    {
     errorMsg = "DestroyCV: CV does not belong to this client";
     printf("%s\n", errorMsg);
     Message msg = Message(client, threadID, errorMsg);
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
  
   Message msg = Message(client, threadID, "DestroyCV");
   sendMessage(msg);
  return 0;
}

int doSignalCV(int cvIndex, int lockIndex, int client, int threadID )
{
  char* errorMsg;
  if(!LockIsValid(lockIndex, client))
    {
      errorMsg = "Signal::Error. invalid Lock";
      printf("%s\n", errorMsg);
      Message msg = Message(client, threadID, errorMsg);
      sendMessage(msg); 
      return -1;
    }
  if(!CVIsValid(cvIndex, client))
    {
      errorMsg = "Signal::Error. invalid cv";
      printf("%s\n", errorMsg);
      Message msg = Message(client, threadID, errorMsg);
      sendMessage(msg); 
      return -1;
    }
  
 //end of input parsing
  ServerLock* sl = ServerLockTable[lockIndex];
  ServerCondition* sc = ServerCVTable[cvIndex];
  //done with input parsing//
  //reply to signaler, and do acquire for waiting thread if there is one
  ////send message to sender
  Message msg = Message(client, threadID, "Signal Return");
  sendMessage(msg); 
    
  if(sc->waitQueue->IsEmpty())
    {
 Message msg = Message(client, threadID, "Signal");
      sendMessage(msg); 
 return 0;
    }
  if(sc->waitingLock != lockIndex)
    {
      printf("Signal Error: waitLock!=lockIndex\n");
       Message msg = Message(client, threadID, "Signal:waitlock != lockIndex");
      sendMessage(msg); 
     
      return -1;
    }
  //now, we will wake up 1 waiting thread
  printf("Signal::Waking up waiting client\n");
  Message* msg2 = (Message*)sc->waitQueue->Remove();
  sl->currentOwner = msg2->toMailbox;
  sendMessage(*msg2); 
  if(sc->waitQueue->IsEmpty())
    {
      sc->waitingLock = -1;//no more waitinglock
    }
  return 0;

}


int doWaitCV(int cvIndex, int lockIndex, int client, int threadID )
{
  char* errorMsg;
  if(!LockIsValid(lockIndex, client))
    {
      errorMsg = "Wait::Error. invalid Lock";
      printf("%s\n", errorMsg);
      Message msg = Message(client, threadID, errorMsg);
      sendMessage(msg); 
      return -1;
    }
  if(!CVIsValid(cvIndex, client))
    {
      errorMsg = "Wait::Error. invalid cv";
      printf("%s\n", errorMsg);
      Message msg = Message(client, threadID, errorMsg);
      sendMessage(msg); 
      return -1;
    }
  
 //end of input parsing
  ServerLock* sl = ServerLockTable[lockIndex];
  ServerCondition* sc = ServerCVTable[cvIndex];

  if(sc->waitingLock == -1)
    {
      sc->waitingLock = lockIndex;
    }
  if(sc->waitingLock != lockIndex)
    {
      char* error = "Wait:error: lock mismatch";
      printf("%s\n", error);
      //send back message
      return -1;
    }

//now, put wait message in waitQ but do not send
Message* msg = new Message(client, threadID, "Wake after Wait");
sc->waitQueue->Append(msg);
  return 0;
}

int doBroadcastCV(int cvIndex, int lockIndex, int client, int threadID )
{
  char* errorMsg;
  if(!LockIsValid(lockIndex, client))
    {
      errorMsg = "Broadcast::Error. invalid Lock";
      printf("%s\n", errorMsg);
      Message msg = Message(client, threadID, errorMsg);
      sendMessage(msg); 
      return -1;
    }
  if(!CVIsValid(cvIndex, client))
    {
      errorMsg = "Broadcast::Error. invalid cv";
      printf("%s\n", errorMsg);
      Message msg = Message(client, threadID, errorMsg);
      sendMessage(msg); 
      return -1;
    }
  
 //end of input parsing
  ServerLock* sl = ServerLockTable[lockIndex];
  ServerCondition* sc = ServerCVTable[cvIndex];

  if(lockIndex != sc->waitingLock)
    {
     char* error = "Broadcast:Error: lock mismatch";
     Message msg = Message(client, threadID, error);
     sendMessage(msg); 
     return -1;
    }
//now, broadcast to everyone
while(!sc->waitQueue->IsEmpty())
{
  Message* msg =(Message*) sc->waitQueue->Remove();
  sendMessage(*msg);
  //TODO: make sure this functions as broadcast
}
  return 0;
}

/*MONITOR VARIABLE PROCEDURES*/
int doCreateMV(int client, int threadID)
{
  char* errorMsg;
  if(serverMVCounter >= TABLE_SIZE)
    {
      errorMsg = "CreateMV::cannot create MV";
      printf("%s\n", errorMsg);
      Message msg = Message(client, threadID, errorMsg);
      sendMessage(msg);//TODO copy this chunk to creatCV/createLock
      return -1;
    }
  int thisMVID = serverMVCounter;
  ServerMV* smv = new ServerMV(client);
  //smv->data = {};
  ServerMVTable[thisMVID] = smv;
  serverMVCounter++;

  stringstream strs;
  strs<<thisMVID;
  string temp = strs.str();
  char const* msgDataConst = temp.c_str();
  char* msgData = new char[temp.length()];
  strcpy(msgData, temp.c_str());

  Message msg = Message(client, threadID, msgData);
  sendMessage(msg);
  return 0;
}

int doDestroyMV(int mvIndex, int client, int threadID)
{//TODO not sure about this
  char* errorMsg;
  if(!MVIsValid(mvIndex, client))
    {
      errorMsg = "DestroyMV::invalid mv";
      printf("%s\n", errorMsg);
      Message msg = Message(client, threadID, errorMsg);
      sendMessage(msg);
      return -1;  
    }
  //input error checking done
  delete ServerMVTable[mvIndex];
  Message msg = Message(client, threadID, "Deleted MV");
  sendMessage(msg);
  return 0;
}

int doGetMV(int mvID, int arrayIndex, int client, int threadID)
{
 char* errorMsg;
  if(!MVIsValid(mvID, client))
    {
      errorMsg = "GetMV::invalid mv";
      printf("%s\n", errorMsg);
      Message msg = Message(client, threadID, errorMsg);
      sendMessage(msg);
      return -1;  
    }
  ServerMV* thisMV = ServerMVTable[mvID];
  /* if(arrayIndex >= thisMV->usedLength)
    {
    errorMsg = "GetMV::invalid location";
      printf("%s: %d\n", errorMsg, arrayIndex);
      Message msg = Message(client, threadID, errorMsg);
      sendMessage(msg);
      return -1;        
      }*/
  //now we're good, return this int
  int mVar = thisMV->data[arrayIndex];
  printf("getMV val: %d", mVar);
  stringstream strs;
  strs<<mVar;
  string temp = strs.str();
  char const* msgDataConst = temp.c_str();
  char* msgData = new char[temp.length()];
  strcpy(msgData, temp.c_str());

  Message msg = Message(client, threadID, msgData);
  sendMessage(msg);
  return 0;
}


int doSetMV(int mvID, int index, int value, int client, int threadID)
{
  printf("setting mv %d index %d value %d\n", mvID,index,  value);
 char* errorMsg;
  if(!MVIsValid(mvID, client))
    {
      errorMsg = "SetMV::invalid mv";
      printf("%s\n", errorMsg);
      Message msg = Message(client, threadID, errorMsg);
      sendMessage(msg);
      return -1;  
    }
  ServerMV* thisMV = ServerMVTable[mvID];
  if(index<0)
    {
 errorMsg = "SetMV::invalid index";
      printf("%s\n", errorMsg);
      Message msg = Message(client, threadID, errorMsg);
      sendMessage(msg);
      return -1;  
   
    }
  thisMV->data[index] = value;
  //thisMV->usedLength++;

  //send back message

  Message msg = Message(client, threadID, "setMV");
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
    outMailHdr.from = 0;//changed now
    outMailHdr.length = strlen(data) + 1;

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
	 
	  int lock = doCreateLock(lockName, inPktHdr.from, inMailHdr.from);
	  //now, build and send message back to client
	  strs<<lock;
	  string temp = strs.str();
	  char const* msgDataConst = temp.c_str();
	  char* msgData = new char[temp.length()];
	  strcpy(msgData, temp.c_str());

	  outMailHdr.to = inMailHdr.from;
	  printf("\nsend to %d box %d\n", outPktHdr.to, outMailHdr.to);

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
	  int cv = doCreateCV(cvName, inPktHdr.from, inMailHdr.from);

	  //now, build and send message back to client
	  stringstream strs;
	  strs<<cv;
	  string temp = strs.str();
	  char const* msgDataConst = temp.c_str();
	  char* msgData = new char[temp.length()];
	  strcpy(msgData, temp.c_str());
	  outMailHdr.length = strlen(msgData) + 1;
	  outMailHdr.to = inMailHdr.from;
          bool success = postOffice->Send(outPktHdr, outMailHdr, msgData); 
	  break;
	}
      case DCV:
	{
	  string cvIndex;
	  ss>>cvIndex;
	  int cvInt = atoi(cvIndex.c_str());
	  doDestroyCV(cvInt, inPktHdr.from, inMailHdr.from);
	  break;
	}
      case SCV:
	{
	  string lockIndex;
	  string cvIndex;
	  ss>>lockIndex;
	  ss>>cvIndex;
	  int lockInt = atoi(lockIndex.c_str());
	  int cvInt = atoi(cvIndex.c_str());
	  doSignalCV(lockInt, cvInt, inPktHdr.from, inMailHdr.from);
	  
	  break;
	}
      case WCV:
	{
	  string lockIndex;
	  string cvIndex;
	  ss>>lockIndex;
	  ss>>cvIndex;
	  int lockInt = atoi(lockIndex.c_str());
	  int cvInt = atoi(cvIndex.c_str());
	  doWaitCV(lockInt, cvInt, inPktHdr.from, inMailHdr.from);

     	  break;
	}
      case BCV:
	{
	  string lockIndex;
	  string cvIndex;
	  ss>>lockIndex;
	  ss>>cvIndex;
	  int lockInt = atoi(lockIndex.c_str());
	  int cvInt = atoi(cvIndex.c_str());
	  doBroadcastCV(lockInt, cvInt, inPktHdr.from, inMailHdr.from);

	  break;
	}
      case CMV:
	{
	  //create MV (array of ints) of size (MV_ARRAY_SIZE)
	  doCreateMV(inPktHdr.from, inMailHdr.from);
	  break;
	}
      case DMV:
	{
	  string mvIndex;
	  ss>>mvIndex;
	  int mvInt = atoi(mvIndex.c_str());
	  doDestroyMV(mvInt, inPktHdr.from, inMailHdr.from);
	
	  break;
	}
      case GMV:
	{
	  string mvIndex;
	  string arrayIndex;
	  ss>>mvIndex;
	  ss>>arrayIndex;
	  int mvInt = atoi(mvIndex.c_str());
	  int arrayIndexInt = atoi(arrayIndex.c_str());
	  doGetMV(mvInt, arrayIndexInt, inPktHdr.from, inMailHdr.from);

	  break;
	}
      case SMV:
	{
	  string mvIndex;
	  string arrayIndex;
	  string value;
	  ss>>mvIndex;
	  ss>>arrayIndex;
	  ss>>value;
	  int mvInt = atoi(mvIndex.c_str());
	  int arrayIndexInt = atoi(arrayIndex.c_str());
	  int valueInt = atoi(value.c_str());
	  doSetMV(mvInt, arrayIndexInt, valueInt, inPktHdr.from, inMailHdr.from);

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

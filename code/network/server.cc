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
const int SERVER_SCALAR = 100;//this will modify what index gets passed when creating something on a server by mutlpilying this with this machine's ID

int serverLockCounter = 0;
int serverCVCounter = 0;
int serverMVCounter = 0;
int prCounter = 0;

Lock* RPCLock = new Lock("RPCLock");

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
  YES,//servertoserver confirmation of resoutce
  NO,//servertoserver i dont have that resource
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
  Message();
};
Message::Message()
{

}
Message::Message(int t,int tm, char* m)
{
  to = t;
  toMailbox = tm;
  msg = m;
}

/*Pending Request Struct*/
struct PendingRequest
{
  int reqMId;//requestor machineID
  int reqMBId;//requestor Mailbox ID
  int reqType;//what kind of resource requet this is
  int id1;//index of resource, the parameter
  int id2;//index of resource, the parameter
  //int id3;//needed for mv's later on
  int noCount;//keep track of how many servers have said NO. starts at 1

  PendingRequest(int reqId, int mbId, int type);
};

PendingRequest::PendingRequest(int reqId, int mbId, int type)
{
  reqMId = reqId;
  reqMBId = mbId;
  reqType = type;

  noCount = 1;
}

//Tables below
ServerLock* ServerLockTable[TABLE_SIZE];
ServerCondition* ServerCVTable[TABLE_SIZE];
ServerMV* ServerMVTable[TABLE_SIZE];
PendingRequest* PRTable[TABLE_SIZE];

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
  if(req == "YES") return YES;
  if(req == "NO") return NO;
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

  printf("SENDING TO ID %d box %d MESSAGE: %s\n", msg.to, msg.toMailbox, data); 
  bool success = postOffice->Send(outPktHdr, outMailHdr, data);
  if(!success)
    {
      printf("SendMessage::ERROR: could not send message\n");
    }
}

//send this messsage to all servers except me
void SendMessageToServers(Message msg, int reqType)
{
  printf("MSG %s\n", msg.msg);
  stringstream str;
  str<<msg.msg<<" "<<msg.to<<" "<<msg.toMailbox;
  char* data = (char*)str.str().c_str();
  PendingRequest* pr = new PendingRequest(msg.to, msg.toMailbox, reqType);
  PRTable[prCounter] = pr;
  prCounter++;
  //msg has. WHO is asking (client machId and mailbox), TYPE of request, and request PARAMS
  //is sent to all other server machineID's that aren't mine
  for(int i = 0; i < numServers; i++)
    {
      //dont send message to me
      if(i != netname)
	{
	  //  stringstream ss;
	  //ss<<msg.msg<<" "<<msg.to<<" "<<msg.toMailbox;//THIS BREAKS FOR NO GODDAM REASON
	  //char* data = (char*)ss.str().c_str();
	  Message outMsg = Message(i, 1, data);//1 is mailbox of serverserver thread
	  sendMessage(outMsg);
	}
    }
}

bool LockIsValid(int lock, int client)
{
 if(lock < 0 || lock > (serverLockCounter* SERVER_SCALAR))
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
 if(cv < 0 || cv > (serverCVCounter* SERVER_SCALAR))
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
  if(mv < 0 || mv > (serverMVCounter*SERVER_SCALAR))
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
  int thisLock = (netname * SERVER_SCALAR) + serverLockCounter;
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
      //NEW. Send 1 message to each server
      stringstream ss;
      ss<<"DL "<<lock;
      char* msgData = (char*)ss.str().c_str();
      Message msg = Message(client, owner, msgData);
      SendMessageToServers(msg, DL);
     
      // errorMsg = "DestroyLock::Error. invalid Lock";
      //printf("%s\n", errorMsg);
      //Message msg = Message(client, owner, errorMsg);
      //sendMessage(msg); 
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
      //NEW. Send 1 message to each server
      stringstream ss;
      ss<<"AL "<<lockIndex;
      char* msgData = (char*)ss.str().c_str();
      Message msg = Message(clientID, threadID, msgData);
      SendMessageToServers(msg, AL);
      // errorMsg = "AcquireLock::Error. invalid Lock";
      //printf("%s\n", errorMsg);
      //Message msg = Message(clientID, threadID, errorMsg);
      //sendMessage(msg); 
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
 //NEW. Send 1 message to each server
      stringstream ss;
      ss<<"RL "<<lockIndex;
      char* msgData = (char*)ss.str().c_str();
      Message msg = Message(clientID, threadID, msgData);
      SendMessageToServers(msg, RL);
     

      // errorMsg = "ReleaseLock::Error. invalid Lock";
      //printf("%s\n", errorMsg);
      //Message msg = Message(clientID, threadID, errorMsg);
      //sendMessage(msg); 
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
      printf("MV not on this server, asking others\n");
      stringstream ss;
      ss<<"DMV "<<mvIndex;
      char* msgData = (char*)ss.str().c_str();
      Message msg = Message(client, threadID, msgData);
      SendMessageToServers(msg, DMV);
      
      //errorMsg = "DestroyMV::invalid mv";
      //printf("%s\n", errorMsg);
      //Message msg = Message(client, threadID, errorMsg);
      //sendMessage(msg);
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
      printf("MV not on this server, asking others\n");
      stringstream ss;
      ss<<"GMV "<<mvID<<" "<<arrayIndex;
      char* msgData = (char*)ss.str().c_str();
      Message msg = Message(client, threadID, msgData);
      SendMessageToServers(msg, GMV);
      //    errorMsg = "GetMV::invalid mv";
      // printf("%s\n", errorMsg);
      //Message msg = Message(client, threadID, errorMsg);
      //sendMessage(msg);
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
 if(index<0 || mvID < 0)
    {
      errorMsg = "SetMV::invalid index";
      printf("%s\n", errorMsg);
      Message msg = Message(client, threadID, errorMsg);
      sendMessage(msg);
      return -1;  
   
    }
  
 if(!MVIsValid(mvID, client))
    {
      printf("MV not on this server, asking others\n");
      stringstream ss;
      ss<<"SMV "<<mvID<<" "<<index<<" "<<value;
      char* msgData = (char*)ss.str().c_str();
      Message msg = Message(client, threadID, msgData);
      SendMessageToServers(msg, SMV);

      //errorMsg = "SetMV::invalid mv";
      //printf("%s\n", errorMsg);
      //Message msg = Message(client, threadID, errorMsg);
      //sendMessage(msg);
      return -1;  
    }
  ServerMV* thisMV = ServerMVTable[mvID];
  thisMV->data[index] = value;
  //thisMV->usedLength++;

  //send back message

  Message msg = Message(client, threadID, "setMV");
  sendMessage(msg);

  return 0;
}
/*Server to Server Functions*/
//return pr with parameters equal to inputs
PendingRequest* FindPR(int clientId, int clientMB, int reqType)
{
  printf("FindPR: id: %d mb: %d type: %d\n", clientId, clientMB, reqType);
  for(int i = 0; i < prCounter; i++)
    {
      PendingRequest* pr = PRTable[i];
      if(pr)
	{
	  if(pr->reqMId == clientId)
	    {
	      if(pr->reqMBId == clientMB)
		{
		  if(pr->reqType == reqType)
		    {
		      return pr;
		    }
		}
	    }
	}
    }
  printf("Can't find PR!\n");
  return NULL;
}

int SSAcquireLock(int lockIndex, int clientId, int clientMB, int reqId, int reqMB)
{
  printf("SSACQUIRE\n");
  Message msg = Message();
  stringstream ss;
  //if i have lock iin my local table, wake up client and send YES to requestor
  if(LockIsValid(lockIndex, clientId))
  {
    //wake up client first
    ///msg = Message(clientId, clientMB, "Acquire Success");
    ///sendMessage(msg);
    doAcquireLock(lockIndex, clientId, clientMB);
    //TODO send yes reply to requestor
    printf("SSAL Sending YES to %d\n", reqId);
    ss<<"YES "<<clientId<<" "<<clientMB<<" "<<"AL";
    char* msgData = (char*)ss.str().c_str();
    msg = Message(reqId, 1, msgData);//hard code 1 for serverserver TODO this needs unique id info (client stuff)
    sendMessage(msg);
    return 1;
  }
  //else, send NO to requestor
  printf("SSAL Sending NO to %d\n", reqId);
   ss<<"NO "<<clientId<<" "<<clientMB<<" "<<"AL";
  char* msgData = (char*)ss.str().c_str();
  msg = Message(reqId, 1, msgData);
  sendMessage(msg);
  return 1;
}

int SSReleaseLock(int lockIndex, int clientId, int clientMB, int reqId, int reqMB)
{
  Message msg = Message();
  stringstream ss;
  //if i have lock iin my local table, wake up client and send YES to requestor
  if(LockIsValid(lockIndex, clientId))
  {
    //wake up client first
    doReleaseLock(lockIndex, clientId, clientMB);
    ///msg = Message(clientId, clientMB, "Release Success");
    ///sendMessage(msg);
 
   //TODO send yes reply to requestor
    printf("SSRL Sending YES to %d\n", reqId);
    ss<<"YES "<<clientId<<" "<<clientMB<<" "<<"RL";
    char* msgData = (char*)ss.str().c_str();
    msg = Message(reqId, 1, msgData);//hard code 1 for serverserver TODO this needs unique id info (client stuff)
    sendMessage(msg);
    return 1;
  }
  //else, send NO to requestor
  printf("SSRL Sending NO to %d\n", reqId);
   ss<<"NO "<<clientId<<" "<<clientMB<<" "<<"RL";
  char* msgData = (char*)ss.str().c_str();
  msg = Message(reqId, 1, msgData);
  sendMessage(msg);
  return 1;

}


int SSDestroyLock(int lockIndex, int clientId, int clientMB, int reqId, int reqMB)
{

  Message msg = Message();
  stringstream ss;
  //if i have lock iin my local table, wake up client and send YES to requestor
  if(LockIsValid(lockIndex, clientId))
  {
    //wake up client first
    doDestroyLock(lockIndex, clientId, clientMB);
    ///msg = Message(clientId, clientMB, "Destroy Success");
    ///sendMessage(msg);
 
   //TODO send yes reply to requestor
    printf("SSDL Sending YES to %d\n", reqId);
    ss<<"YES "<<clientId<<" "<<clientMB<<" "<<"DL";
    char* msgData = (char*)ss.str().c_str();
    msg = Message(reqId, 1, msgData);//hard code 1 for serverserver TODO this needs unique id info (client stuff)
    sendMessage(msg);
    return 1;
  }
  //else, send NO to requestor
  printf("SSRL Sending NO to %d\n", reqId);
   ss<<"NO "<<clientId<<" "<<clientMB<<" "<<"DL";
  char* msgData = (char*)ss.str().c_str();
  msg = Message(reqId, 1, msgData);
  sendMessage(msg);
  return 1;

}

/*SS MV functions*/
int SSSetMV(int mvIndex,int arrayIndex, int value, int clientId, int clientMB, int reqId, int reqMB)
{
 Message msg = Message();
  stringstream ss;
  //if i have lock iin my local table, wake up client and send YES to requestor
  if(MVIsValid(mvIndex, clientId))
  {
    //set mv (copied from doSetMV)
    doSetMV(mvIndex, arrayIndex, value, clientId, clientMB);
    ///ServerMV* thisMV = ServerMVTable[mvIndex];
    ///thisMV->data[arrayIndex] = value;
    //send back message
    ///Message msg = Message(clientId, clientMB, "setMV");
    ///sendMessage(msg);

    //send yes reply to requestor
    printf("SSSMV Sending YES to %d\n", reqId);
    ss<<"YES "<<clientId<<" "<<clientMB<<" "<<"SMV";
    char* msgData = (char*)ss.str().c_str();
    msg = Message(reqId, 1, msgData);//hard code 1 for serverserver TODO this needs unique id info (client stuff)
    sendMessage(msg);
    return 1;
  }
  //else, send NO to requestor
  printf("SSSMV Sending NO to %d\n", reqId);
   ss<<"NO "<<clientId<<" "<<clientMB<<" "<<"SMV";
  char* msgData = (char*)ss.str().c_str();
  msg = Message(reqId, 1, msgData);
  sendMessage(msg);
 
  return 1;
}

int SSGetMV(int mvIndex,int arrayIndex, int clientId, int clientMB, int reqId, int reqMB)
{
  Message msg = Message();
  stringstream ss;
  //if i have lock iin my local table, wake up client and send YES to requestor
  if(MVIsValid(mvIndex, clientId))
  {
    //give client their mv (copied from doGetMV)
    doGetMV(mvIndex, arrayIndex, clientId, clientMB);
    ///ServerMV* thisMV = ServerMVTable[mvIndex];
    ///int mVar = thisMV->data[arrayIndex];
    ///printf("getMV val: %d\n", mVar);
    ///stringstream str;//use for client's message
    ///str<<mVar;
    ///char* msgDataClient = (char*)str.str().c_str();
    ///msg = Message(clientId, clientMB, msgDataClient);
    ///sendMessage(msg);

    //send yes reply to requestor
    printf("SSGMV Sending YES to %d\n", reqId);
    ss<<"YES "<<clientId<<" "<<clientMB<<" "<<"GMV";
    char* msgData = (char*)ss.str().c_str();
    msg = Message(reqId, 1, msgData);//hard code 1 for serverserver TODO this needs unique id info (client stuff)
    sendMessage(msg);
    return 1;
  }
  //else, send NO to requestor
  printf("SSGMV Sending NO to %d\n", reqId);
   ss<<"NO "<<clientId<<" "<<clientMB<<" "<<"GMV";
  char* msgData = (char*)ss.str().c_str();
  msg = Message(reqId, 1, msgData);
  sendMessage(msg);
 
  return 1;
}

int SSDestroyMV(int mvIndex, int clientId, int clientMB, int reqId, int reqMB)
{
 Message msg = Message();
  stringstream ss;
  //if i have lock iin my local table, wake up client and send YES to requestor
  if(MVIsValid(mvIndex, clientId))
  {
    //give client their mv (copied from doGetMV)
    doDestroyMV(mvIndex, clientId, clientMB);
 
    //send yes reply to requestor
    printf("SSDMV Sending YES to %d\n", reqId);
    ss<<"YES "<<clientId<<" "<<clientMB<<" "<<"DMV";
    char* msgData = (char*)ss.str().c_str();
    msg = Message(reqId, 1, msgData);//hard code 1 for serverserver TODO this needs unique id info (client stuff)
    sendMessage(msg);
    return 1;
  }
  //else, send NO to requestor
  printf("SSGMV Sending NO to %d\n", reqId);
   ss<<"NO "<<clientId<<" "<<clientMB<<" "<<"DMV";
  char* msgData = (char*)ss.str().c_str();
  msg = Message(reqId, 1, msgData);
  sendMessage(msg);
 
 
  return 1;
}
/*SS Bookkeeping Functions*/
int SSProcessYes(int clientId, int clientMB, int reqType)
{
 PendingRequest* pr = FindPR(clientId, clientMB, reqType);
  //modify pointer? by incrementing it's no count, if nocount == numservers, send back error (depends on reqtype)
  if(!pr)
    {
      return -1;
    }
  delete pr;
  return 1;
}

int SSProcessNo(int clientId, int clientMB, int reqType)
{
  PendingRequest* pr = FindPR(clientId, clientMB, reqType);
  //modify pointer? by incrementing it's no count, if nocount == numservers, send back error (depends on reqtype)
  if(!pr)
    {
      return -1;
    }
  pr->noCount++;
  if(pr->noCount == numServers)
    {
      printf("SSNO sending back error\n");
      stringstream ss;
      //send that error message! TODOQ. then test. then do same for yes. then expand
      ss<<reqType<<" ERROR ";
      char* msgData = (char*)ss.str().c_str();
      Message msg = Message(clientId, clientMB, msgData);
      sendMessage(msg);
    }  
}
	  
/*COMMUNICATES WITH OTHER SERVERS ONLY*/
void ServerToServer()
{
  printf("INTER-SERVER STARTED\n");
  while(true)
    {
    PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char buffer[MaxMailSize];

    postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);//TODO check mail isn't bigger than buffer
    printf("SS Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);
  
    stringstream ss;

    ss<<buffer;
    string request;
    ss>>request;

    //which type of request is this
    switch(getType(request))
      {
      case AL:
	{
	  printf("server-server AL\n");
	  string lockIndexStr, clientIdStr, clientMBStr, reqIdStr, reqMBStr;
	  int lockIndex, clientId, clientMB, reqId, reqMB;
	  ss>>lockIndexStr;
	  ss>>clientIdStr;
	  ss>>clientMBStr;
	  ss>>reqIdStr;
	  ss>>reqMBStr;
	  //convert to int
	  lockIndex = atoi(lockIndexStr.c_str());
	  clientId = atoi(clientIdStr.c_str());
	  clientMB = atoi(clientMBStr.c_str());
	  reqId = inPktHdr.from;
	  reqMB = inMailHdr.from;

	  SSAcquireLock(lockIndex, clientId, clientMB, reqId, reqMB);
	  break;
	}
      case RL:
	{
	  printf("server-server Rl\n");
	  string lockIndexStr, clientIdStr, clientMBStr, reqIdStr, reqMBStr;
	  int lockIndex, clientId, clientMB, reqId, reqMB;
	  ss>>lockIndexStr;
	  ss>>clientIdStr;
	  ss>>clientMBStr;
	  ss>>reqIdStr;
	  ss>>reqMBStr;
	  //convert to int
	  lockIndex = atoi(lockIndexStr.c_str());
	  clientId = atoi(clientIdStr.c_str());
	  clientMB = atoi(clientMBStr.c_str());
	  reqId = inPktHdr.from;
	  reqMB = inMailHdr.from;

	  SSReleaseLock(lockIndex, clientId, clientMB, reqId, reqMB);
	 
	  break;
	}
      case DL:
	{
	  printf("server-server DL\n");
	  string lockIndexStr, clientIdStr, clientMBStr;
	  int lockIndex, clientId, clientMB, reqId, reqMB;
	  ss>>lockIndexStr;
	  ss>>clientIdStr;
	  ss>>clientMBStr;
	  lockIndex = atoi(lockIndexStr.c_str());
	  clientId = atoi(clientIdStr.c_str());
	  clientMB = atoi(clientMBStr.c_str());
	  reqId = inPktHdr.from;
	  reqMB = inMailHdr.from;

	  SSDestroyLock(lockIndex, clientId, clientMB, reqId, reqMB);

	  break;
	}
      case SMV:
	{
	  printf("received server-server SMV\n");
	  string mvIndexStr, mvArrayIndexStr, valueStr, clientIdStr, clientMBStr;
	  int mvIndex, mvArrayIndex, value, clientId, clientMB, reqId, reqMB;
	  ss>>mvIndexStr;
	  ss>>mvArrayIndexStr;
	  ss>>valueStr;
	  ss>>clientIdStr;
	  ss>>clientMBStr;
	  //convert to int
	  mvIndex = atoi(mvIndexStr.c_str());
	  mvArrayIndex = atoi(mvArrayIndexStr.c_str());
	  value = atoi(valueStr.c_str());
	  clientId = atoi(clientIdStr.c_str());
	  clientMB = atoi(clientMBStr.c_str());
	  reqId = inPktHdr.from;
	  reqMB = inMailHdr.from;

	  SSSetMV(mvIndex, mvArrayIndex, value, clientId, clientMB, reqId, reqMB);

	  break;
	}
      case GMV:
	{
	  printf("server-server GMV\n");
	  string mvIndexStr, mvArrayIndexStr, clientIdStr, clientMBStr;
	  int mvIndex, mvArrayIndex, clientId, clientMB, reqId, reqMB;
	  ss>>mvIndexStr;
	  ss>>mvArrayIndexStr;
	  ss>>clientIdStr;
	  ss>>clientMBStr;
	  //convert to int
	  mvIndex = atoi(mvIndexStr.c_str());
	  mvArrayIndex = atoi(mvArrayIndexStr.c_str());
	  clientId = atoi(clientIdStr.c_str());
	  clientMB = atoi(clientMBStr.c_str());
	  reqId = inPktHdr.from;
	  reqMB = inMailHdr.from;

	  SSGetMV(mvIndex, mvArrayIndex, clientId, clientMB, reqId, reqMB);

	  break;
	}
      case DMV:
	{
	  printf("server-server DMV\n");
	  string mvIndexStr, clientIdStr, clientMBStr;
	  int mvIndex,  clientId, clientMB, reqId, reqMB;
	  ss>>mvIndexStr;
	  ss>>clientIdStr;
	  ss>>clientMBStr;
	  //convert to int
	  mvIndex = atoi(mvIndexStr.c_str());
	  clientId = atoi(clientIdStr.c_str());
	  clientMB = atoi(clientMBStr.c_str());
	  reqId = inPktHdr.from;
	  reqMB = inMailHdr.from;

	  SSDestroyMV(mvIndex, clientId, clientMB, reqId, reqMB);

	  break;
	}
      case YES:
	{
	  printf("ss YES: other server has resource\n");
	 
	  string clientIdStr, clientMBStr, reqTypeStr;
	  int clientId, clientMB, reqType;
	  ss>>clientIdStr;
	  ss>>clientMBStr;
	  ss>>reqTypeStr;
	  //convert to int
	  clientId = atoi(clientIdStr.c_str());
	  clientMB = atoi(clientMBStr.c_str());
	  reqType= getType(reqTypeStr);
	 
	  SSProcessYes(clientId, clientMB, reqType);

	  break;
	}
      case NO:
	{
	  printf("ss NO\n");
	  string clientIdStr, clientMBStr, reqTypeStr;
	  int clientId, clientMB, reqType;
	  ss>>clientIdStr;
	  ss>>clientMBStr;
	  ss>>reqTypeStr;
	  clientId = atoi(clientIdStr.c_str());
	  clientMB = atoi(clientMBStr.c_str());
	  reqType = getType(reqTypeStr);
	  SSProcessNo(clientId, clientMB, reqType);//TODOQ
	  //TODO find the PR related to this request, and increment its noCount
	  //if noCount == numServers, send back errormsg to client
	  break;
	}
      default:
	printf("unrecognized server-server message\n");
	break;
      }
    }
}
/*COMMUNICATES WITH CLIENTS ONLY*/
void ServerToClient()
{
  printf("SERVER STARTED ID: %d numservers: %d\n", netname, numServers);
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
    outMailHdr.from = 0;//netname?
    outMailHdr.length = strlen(data) + 1;

    postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);//TODO check mail isn't bigger than buffer
    printf("Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);

    outPktHdr.to = inPktHdr.from;  

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
/*MAIN SERVER RUN FUNCTION, called by -server cmd line*/
void Server()
{
  Thread* t;
 
 //thread to handle server client
  t = new Thread("client-server thread");
  t->Fork((VoidFunctionPtr)ServerToClient, 0);
  
//for project 4, fork inter-server thread
  t = new Thread("interserver Thread");
  t->Fork((VoidFunctionPtr)ServerToServer, 0);
}

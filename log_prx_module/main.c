#include "main.h"

#define MAX_CLIENTS 10
#define NET_STACK_SIZE 0x4000
#define SERVER_PORT 4000
#define THREAD_DELAY 1000 * 1000
#define MESSAGE_BUFF_SIZE 0x800

typedef struct {
  int sockId = -1;
  SceUID thIdClient = -1;
  bool isLogger = false;
} Client;

static volatile Client clients[MAX_CLIENTS];
static volatile bool closeClients = false;
static SceUID thIdConnectWait = -1;
static int serverSockId = -1;

int initServer() 
{
  int sockId = sceNetSocket("LogServerSock", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, 0);
  SceNetSockaddrIn serverAddr;
  serverAddr.sin_family = SCE_NET_AF_INET;
  serverAddr.sin_port = sceNetHtons((unsigned short) SERVER_PORT);
  serverAddr.sin_addr.a_addr = sceNetHtonl(SCE_NET_INADDR_ANY);
  
  if (sceNetBind(sockId, (SceNetSockaddr *) &serverAddr, sizeof(serverAddr)) < 0)
  {
     return -1;
  }
  if (sceNetListen(sockId, 64) < 0)
  {
     return -1;
  }
  return sockId;
}

int initNet()
{
  static unsigned char netStack[NET_STACK_SIZE];
  int loaded = sceSysmoduleIsLoaded(SCE_SYSMODULE_NET);
  if (loaded == 0)
  {
     return 0;
  } 
  sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
  loaded = sceSysmoduleIsLoaded(SCE_SYSMODULE_NET);
  if (loaded != 0)
  {
     return -1;
  }

  int ret = sceNetShowNetstat();
  if (ret == SCE_NET_ERROR_ENOTINIT)
  {
     SceNetInitParam netInitParam;
     netInitParam.memory = netStack;
     netInitParam.size = NET_STACK_SIZE;
     netInitParam.flags = 0;
     if (sceNetInit(&netInitParam) != 0)
     {
	return -1;
     }
  }

  if (sceNetCtlInit() != 0)
  {
     return -1;
  }
  return loaded;
}

void closeSocket(int sockId)
{
  if (sockId == -1)
  {
    return;
  }
  for (unsigned int i = 0; i < MAX_CLIENTS; i++)
  {
    if (clients[i].sockId == sockId) 
    {    
      sceNetSocketClose(sockId);
      clients[i].sockId = -1;
      clients[i].thIdClient = -1;
      break;
    } 
  }
}

void terminateThread(SceUID thId)
{
  if (thId == -1)
  {
    return;
  }
  sceKernelWaitThreadEnd(thId, NULL, NULL);
}

void terminateAllClients()
{
  closeClients = true;
  resetClientPool();
}

void resetClientPool()
{
  for (unsigned int i = 0; i < MAX_CLIENTS; i++)
  {
     Client client = clients[i];
     client.sockId = -1;
     client.thIdClient = -1;
     client.isLogger = false;
     clients[i] = client;
  }
}

void terminateMainThreads()
{
  terminateThread(thIdConnectWait);
  thIdConnectWait = -1;
}

int handleClientThread(SceSize args, void* pArg)
{
  int clientSockId = *((int *) pArg);

  while (closeClients == false)
  {
    char buffer[MESSAGE_BUFF_SIZE];
    int result = sceNetRecv(clientSockId, buffer, MESSAGE_BUFF_SIZE, 0);
    if (result <= 0)
    { 
       printf("Could not receive message from client socket %i", clientSockId);
       break;
    }
    broadcastMessage(clientSockId, buffer);  
  }

  closeSocket(clientSockId);
  sceKernelExitDeleteThread(0);
  return 0;
}

void broadcastMessage(int sockId, const char* message)
{
  char buffer[MESSAGE_BUFF_SIZE];
  strcpy(buffer, message);
  for (unsigned int i = 0; i < MAX_CLIENTS; i++)
  {
    if (clients[i].sockId == -1 || clients[i].sockId == sockId || clients[i].isLogger == true)
    {
      continue;
    }
    sceNetSend(clients[i], buffer, strlen(buffer), 0);
  } 
}

int netAcceptSock(int sockId)
{
  SceNetSockaddrIn clientAddr;
  unsigned int clientAddrSize = sizeof(clientAddr);
  return sceNetAccept(sockId, (SceNetSockaddr *) &clientAddr, &clientAddrSize);
}

static int thread_connection(SceSize args, void* pArg)
{
   sceKernelDelayThread(THREAD_DELAY * 5);
   while (initNet() != 0)
   {
      sceKernelDelayThread(THREAD_DELAY);
   }
   serverSockId = initServer();
   if (serverSockId != 0)
   {
      // TODO clean up client sockets and memory?
      sceKernelExitDeleteThread(0);
      thIdConnectWait = -1;
      return -1;
   }
   while (1)
   {
     int clientSockId = netAcceptSock(serverSockId);
     if (clientSockId < 0)
     {
       sceKernelDelayThread(THREAD_DELAY);
       serverSockId = initServer();
       continue;
     } 
     else if (clientSockId >= 0)
     {
       char thName[32];
       snprintf(thName, 32, "ClientThread_%i", clientSockId);
       int allocationPos = allocateThreadConnection(clientSockId);
       if (allocationPos == -1)
       { 
         printf("No more connection are allowed, max connection reached %i", MAX_CLIENTS);
         sceNetSocketClose(clientSockId);
         continue;
       }
       SceUID thClientId = sceKernelCreateThread(thName, handleClientThread, 64, 0x5000, 0, 0x10000, 0);
       if (thClientId >= 0)
       {
         clients[allocationPos].thIdClient = thClientId;
         sceKernelStartThread(thClientId, sizeof(int), (void *) &clientSockId);
       }
     }
   }
   sceKernelExitDeleteThread(0);
   return 0;
}

int allocateThreadConnection(int sockId)
{
  for (unsigned int i = 0; i < MAX_CLIENTS; i++)
  {
    Client client = clients[i];
    if (client.sockId == -1 && sockId != -1)
    {
      client.sockId = sockId;
      clients[i] = client;
      return i;
    }
  }
  return -1;
}

int module_start(SceSize argc, const void* args)
{
  closeClients = false;
  resetClientPool(); 
  thIdConnectWait = sceKernelCreateThread("connection_listener", 
                                          thread_connection, 64, 
                                          0x4000, 0, 0x10000, 0);
  if (thIdConnectWait >= 0)
  {
    sceKernelStartThread(thIdConnectWait, 0, NULL);
  }

  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void* args)
{
  terminateAllClients();
  terminateMainThreads();
  return SCE_KERNEL_STOP_SUCCESS;
}

void module_exit(void)
{

}

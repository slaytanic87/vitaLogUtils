#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/modulemgr.>
#include <psp2/net/net.h>

int initServer();
int initNet();
void closeSocket(int sockId);
void terminateThread(SceUID thId);
static int thread_connection(SceSize args, void* pArg);
void closeSocket(int sockId);
void terminateThread(SceUID thId);
int handleClientThread(SceSize args, void* pArg);
void terminateAllClients();
void terminateMainThreads();
void resetClientPool();
int netAcceptSock(int sockId);
int allocateThreadConnection(int sockId);
void broadcastMessage(int sockId, const char* message);

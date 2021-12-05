#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/types.h>
#include <psp2/kernel/clib.h>
#include <psp2/sysmodule.h>

enum LogLevel 
{
   ERROR = 0,
   WARN = 1,
   INFO = 2,
   DEBUG = 3
};

class VitaLogUtil 
{
   public:
	  VitaLogUtil();
	  ~VitaLogUtil(); 
	  int init(const char* serverIp, int port, const char* appName);
	  void vitaLog(LogLevel level, const char* message, ...);
          void setLogLevel(LogLevel logLevel);
   private:
	  SceNetInAddr gClientAddr;
	  SceNetSockaddrIn gSockAddr;
	  int gSocketId;
	  LogLevel gLogLevel;
	  char* g_pAppName;
          void log(const char* message, ...);
	  bool checkLogLevel(LogLevel requestedLevel); 
};

#include "logUtil.h"

#define NET_INIT_SIZE 1 * 1024 * 1024
#define MESSAGE_BUFF_SIZE 0x800

VitaLogUtil::VitaLogUtil()
{
}

VitaLogUtil::~VitaLogUtil()
{
  this->gSocketId = -1;
}

int VitaLogUtil::init(const char* serverIp, int port, const char* appName) 
{
  setLogLevel(INFO);
  strcpy(this->g_pAppName, appName);

  int loaded = sceSysmoduleIsLoaded(SCE_SYSMODULE_NET);
  if (loaded == 0)
  {
     return 0;
  } 
  else 
  {
     loaded = sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
     if (loaded != 0)
     {
	return -1;
     }  
  }


  if (sceNetShowNetstat() == SCE_NET_ERROR_ENOTINIT)
  {
     SceNetInitParam initParam;
     initParam.memory = malloc(NET_INIT_SIZE); 
     initParam.size = NET_INIT_SIZE;
     initParam.flags = 0;
     if (sceNetInit(&initParam) != 0)
     {
	     return -1;
     }
  }

  if (sceNetCtlInit() != 0)
  {
    return -1;
  }

  SceNetSockaddrIn serverSockaddrIn;
  serverSockaddrIn.sin_len = sizeof(serverSockaddrIn);
  serverSockaddrIn.sin_family = SCE_NET_AF_INET;
  serverSockaddrIn.sin_port = sceNetHtons(port);
  sceNetInetPton(SCE_NET_AF_INET, serverIp, &serverSockaddrIn.sin_addr);
  this->gSocketId = sceNetSocket("vitaLogger", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, 0);
  sceNetConnect(this->gSocketId, (SceNetSockaddr *) &serverSockaddrIn, sizeof(serverSockaddrIn));

  return loaded;
}

void VitaLogUtil::vitaLog(LogLevel level, const char* message, ...) 
{
    char messageBuffer[MESSAGE_BUFF_SIZE];
    va_list args;
    va_start(args, message);
    sceClibVsnprintf(messageBuffer, sizeof(messageBuffer), message, args);
    va_end(args);
    bool shouldLog = checkLogLevel(level);
    if (shouldLog == false)
    {
       return;
    }
    switch (level)
    {
	case ERROR:
		log("[ERROR]:%s", messageBuffer);
	        break;
	case WARN:
		log("[WARN]:%s", messageBuffer);
	        break;
	case INFO:
		log("[INFO]:%s", messageBuffer);
		break;
	case DEBUG:
		log("[DEBUG]:%s", messageBuffer);
	        break;
	default:
	    
    }
}

void VitaLogUtil::setLogLevel(LogLevel logLevel)
{
  this->gLogLevel = logLevel;
}

bool VitaLogUtil::checkLogLevel(LogLevel logLevel)
{
  if (logLevel > this->gLogLevel)
  {
     return false;
  }
  else
  {
     return true;
  }
}

void VitaLogUtil::log(const char* message, ...)
{
  char buffer[MESSAGE_BUFF_SIZE];
  va_list arg;
  va_start(arg, message);
  sceClibVsnprintf(buffer, sizeof(buffer), message, arg);
  va_end(arg); 
  sceNetSend(this->gSocketId, buffer, strlen(buffer), 0);
}

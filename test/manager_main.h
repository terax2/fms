
/*  INCLUDE  */
#include <iostream>
#include <signal.h>

#include <Utility.h>				 // lib HVaaS
#include <ServerLog.h>			 // lib HVaaS
#include <Network.h>			 // lib HVaaS

#include "SetConfig.h"			// for CServerConfig

/*  EXTERN   */
extern CServerLog SysLog;				// 시스템 관련 로그
extern CServerLog CamLog;			// 카메라 관련로그
extern CServerLog UserLog;			// 사용자 관련로그
extern CServerConfig gsConfig;
extern bool gIsStop;	

/*   STRUCT  */
struct ClientInfo
{
	int cfd;
	int epfd;
}; 



/*  FUNCTION PROTYPE  */
void LastSignal(int sig);
int SetupServer(char *configFileName);
int SetupNetwork();

void *Do_Recv(void *data);             // 메시지 처리 
//int Do_Recv(void *data);             // 메시지 처리 


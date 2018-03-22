
/*  INCLUDE  */
#include <iostream>
#include <signal.h>

#include <Utility.h>				 // lib HVaaS
#include <ServerLog.h>			 // lib HVaaS
#include <Network.h>			 // lib HVaaS

#include "SetConfig.h"			// for CServerConfig

/*  EXTERN   */
extern CServerLog SysLog;				// �ý��� ���� �α�
extern CServerLog CamLog;			// ī�޶� ���÷α�
extern CServerLog UserLog;			// ����� ���÷α�
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

void *Do_Recv(void *data);             // �޽��� ó�� 
//int Do_Recv(void *data);             // �޽��� ó�� 


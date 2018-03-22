/**********************************************************************************************
* Filename   : main.cpp
* Developer  : sbs@gabia.co.kr
* Start Date : 2011/05/11
* Comment    : HVaaS - Manager Server 
***********************************************************************************************/

/*  INCLUDE  */
#include "main.h"
#include "Epoll.h"
#include "task_manager.h"



/*  EXTERN VALUE  */
bool gIsStop;						// 메인 루프 탈출변수
CServerConfig gsConfig;		// 서버 환경설정
CServerLog SysLog;				// 시스템 관련 로그
CServerLog CamLog;			// 카메라 관련로그
CServerLog UserLog;			// 사용자 관련로그

/*  GLOBAL VALUE  */
int sLocal_fd, sCam_fd, sUser_fd;




	/*  Start Main   */
int main(int argc, char *argv[]){
	char *configfilename;
	gIsStop = false;

	/*  Check argment  */
	if(argc < 2){
		std::cerr << "Usage : " << argv[0] << " [ConfigFilename]" << std::endl;
		exit(2);
	}else{
		configfilename = argv[1];
	}
	

	/*  Server Config   */
	if( SetupServer(configfilename) < 0)
	{
		printf("Server Config Error \n");
		return 0;
	}


	/*  Setup Network  */
	if( SetupNetwork() < 0)
	{
		printf("Network Setup Error \n");
		return 0;
	}


	/*  Start Epoll  */
	CEpoll	 Ep_Local;
	if( Ep_Local.EpStart(sLocal_fd, gsConfig.epMaxClient, gsConfig.epSize, 0) < 0 )
	{
		perror("Epoll Create Error");
		return -1;
	}



	/*  Epoll Waiting */
	struct ClientInfo *pClientInfo;
	list<int>	list_eventFD;
	list<int>::iterator iter;
    int th_id;

	while(gIsStop == false)
	{
		 list_eventFD.clear();
		 list_eventFD = (*Ep_Local.EpWait());   // 발생한 Epoll 이벤트의 *list 를받아옴


			 // 각 이벤트에 대해 쓰레드 생성
			for ( iter=list_eventFD.begin(); iter!=list_eventFD.end(); iter++ )
			{
				if( (pClientInfo = (struct ClientInfo*)malloc(sizeof(struct ClientInfo))) == NULL ) 
				{
					perror("Malloc Error ");
					return -1;
				}
				pClientInfo->epfd = Ep_Local.Get_epFD();
				pClientInfo->cfd =  *iter;


//using pThread
				pthread_t p_thread;
				th_id = pthread_create(&p_thread, NULL, Do_Recv, (void *)pClientInfo);
				printf("pthread_create = [[ %d ]]\n", th_id);
				if (th_id != 0)
				{
					perror("Create Thread Error");
					break;
				}

/*

//using SDL thread
				SDL_Thread  *SDL_thread;
	 			SDL_thread = SDL_CreateThread(Do_Recv, (void *)pClientInfo);
			//	SDL_WaitThread(Thread_EpLocal, NULL);
*/
		 }

	}




	/*	  Destory  */

	return 0;
}
/*  END MAIN  */



/**********************************************************************************************************
* IN : 시그널번호
* OUT : void
* Comment : 시그널발생시 호출됨, 메인루프탈출
***********************************************************************************************************/
void LastSignal(int sig)
{
	char szText[21];
	szText[0] = '\0';
	
	switch(sig)
	{
		case SIGINT:
			snprintf(szText, sizeof(szText), "SIGINT");
			break;
		case SIGSEGV:
			snprintf(szText, sizeof(szText), "SIGSEGV");
			break;
		case SIGTERM:
			snprintf(szText, sizeof(szText), "SIGTERM");
			break;
		case SIGABRT:
			snprintf(szText, sizeof(szText), "SIGABRT");
			break;
	}

	SysLog.Print(LOG_CRITICAL, 0, NULL, 0, "SIGNAL Received: %d-%s", sig, szText);
	gIsStop = true;
}



/**********************************************************************************************************
* IN :  환경설정 파일이름
* OUT : error = -1
* Comment : 서버환경설정
***********************************************************************************************************/
int SetupServer(char *configFileName)
{
	/*  Set Config  */
	int return_check;
	CSetConfig SetConfig;
	SetConfig.SetConfigValue(configFileName, &gsConfig);

	/*  Setup Log  */
	if((SysLog.SetupLog(gsConfig.logFileDir, gsConfig.sysLogFile, gsConfig.logLevel, gsConfig.maxLogFileSize)) != 0)
	{
		std::cout << "[ERROR][CRITICAL]SYS Setup Log Error" << std::endl;
		return -1;
	}

	if((CamLog.SetupLog(gsConfig.logFileDir, gsConfig.camLogFile, gsConfig.logLevel, gsConfig.maxLogFileSize)) != 0)
	{
		std::cout << "[ERROR][CRITICAL]CAM Setup Log Error" << std::endl;
		return -1;
	}

	if((UserLog.SetupLog(gsConfig.logFileDir, gsConfig.userLogFile, gsConfig.logLevel, gsConfig.maxLogFileSize)) != 0)
	{
		std::cout << "[ERROR][CRITICAL]USER Setup Log Error" << std::endl;
		return -1;
	}

	SysLog.Print(LOG_CRITICAL, 0, NULL, 0, ">>> HVaaS Management Server START <<<");
	CamLog.Print(LOG_CRITICAL, 0, NULL, 0, ">>> HVaaS Management Server START <<<");
	UserLog.Print(LOG_CRITICAL, 0, NULL, 0, ">>> HVaaS Management Server START <<<");


	/*  Set DemonMode  */
	if( DemonMode(gsConfig.demonMode) != 0)
		SysLog.Print(LOG_DEBUG, 0, NULL, 0, "Server Start [[ Demon Mode ]]");


	/* Set  MaxFD  */ 
	return_check = SetLimit(gsConfig.maxOpenfd);
	SysLog.Print(LOG_DEBUG,  0, NULL, 0, "Set Max FD : %d", return_check);

	if(return_check != gsConfig.maxOpenfd)
	{
		SysLog.Print(LOG_CRITICAL, 0, NULL, 0, "Set FD Limit Error");
		return -1;
	}

	/*  Set Signal  */
	signal(SIGINT, LastSignal);
	signal(SIGTERM, LastSignal);
	signal(SIGABRT, LastSignal);

	return 0;

}



/**********************************************************************************************************
* IN :  
* OUT : error = -1
* Comment : 서버 리슨소켓 설정
***********************************************************************************************************/
int SetupNetwork()
{
	CNetwork Socket;
	sLocal_fd = Socket.TCPListen(gsConfig.serverAddr, gsConfig.localListenPort, gsConfig.sockBufffer);
	if(sLocal_fd < 0)
	{
		SysLog.Print(LOG_CRITICAL, 0, NULL, 0, "Create Server Local Listen Socket Error");
		return -1;
	}

	sCam_fd = Socket.TCPListen(gsConfig.serverAddr, gsConfig.camListenPort, gsConfig.sockBufffer);
	if(sLocal_fd < 0)
	{
		SysLog.Print(LOG_CRITICAL, 0, NULL, 0, "Create Server Camera Listen Socket Error");
		return -1;
	}

	sUser_fd = Socket.TCPListen(gsConfig.serverAddr, gsConfig.userListenPort, gsConfig.sockBufffer);
	if(sLocal_fd < 0)
	{
		SysLog.Print(LOG_CRITICAL, 0, NULL, 0, "Create Server User Listen Socket Error");
		return -1;
	}

	SysLog.Print(LOG_DEBUG, 0, NULL, 0, "Create Local Listen Socket&Listen  FD=%d, IP=%s, PORT=%d", sLocal_fd, gsConfig.serverAddr, gsConfig.localListenPort);
	SysLog.Print(LOG_DEBUG, 0, NULL, 0, "Create Camera Listen Socket&Listen  FD=%d, IP=%s, PORT=%d", sCam_fd, gsConfig.serverAddr, gsConfig.camListenPort);
	SysLog.Print(LOG_DEBUG, 0, NULL, 0, "Create User Listen Socket&Listen  FD=%d, IP=%s, PORT=%d", sUser_fd, gsConfig.serverAddr, gsConfig.userListenPort);
	SysLog.Print(LOG_DEBUG, 0, NULL, 0, "Set Listen Socket Buffer Size Send=%d Recv=%d", Socket.GetSendBuf(sLocal_fd), Socket.GetRecvBuf(sLocal_fd));

	return 0;
}




/**********************************************************************************************************
* IN : 
* OUT : 
* Comment : 
***********************************************************************************************************/
void *Do_Recv(void *data)
//int Do_Recv(void *data)
{
	int readn;
//	int sizen = 0;
	char readbuf[10240] = { '\0' }; 
	
	int epfd = ((struct ClientInfo *)data) -> epfd;
	int cfd = ((struct ClientInfo *) data) -> cfd;	

	pthread_detach(pthread_self()); 
	free(data);

	while(1) 
	{	
		readn = read (cfd, readbuf, sizeof(readbuf)); 
	
		if (readn <0 )
		{ 
			 if (EAGAIN == errno )	  // read 했으나 읽을것이 없음 
				 continue;
		//	else
		//		perror("Epoll Socket Read Error something === TODO: ERROR CHECK !!!!!!!!!!\n");


			if( Delete_fd(epfd, cfd) < 0)
				perror("Delete Epoll FD ERROR [readn<0]");
			
		    close (cfd); 
			break; 
		}


		if( readn  == 0)
		{
			printf("========== Client [%d] Disconnected !! ==========\n", cfd);

			if(Delete_fd(epfd, cfd) < 0)
				perror("Delete Epoll FD ERROR [readn=0]");
			
		    close (cfd); 
			break; 
		}

		printf( "Recv Message from[%d] %d, %s\n", cfd, readn, readbuf);

	   int len;
	   memset(&readbuf, 0, sizeof(readbuf));
	   sprintf(readbuf, "You are [%d] Client", cfd);
	   len = write(cfd, readbuf, strlen(readbuf));

	//	break;
	//	SysLog.Print(LOG_DEBUG, 0, NULL, 0, "Recv Client Message::from[%d] len[%d] == %s", cfd, readn, readbuf);
	}

	return 0;
}





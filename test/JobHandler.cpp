// Implementation

#include "JobHandler.h"

#define Swap(a,b,c) {c = a; a = b; b = c;}

#if defined(__WIN32__) || defined(_WIN32) || defined(_QNX4)
#define _strncasecmp _strnicmp
#define snprintf _snprintf
#else
#include <signal.h>
#define USE_SIGNALS 1
#define _strncasecmp strncasecmp
#endif

// Generate a "Date:" header for use in a Manager response:
static char const* dateHeader() {
  static char buf[200];
#if !defined(_WIN32_WCE)
  time_t tt = time(NULL);
  strftime(buf, sizeof buf, "Date : %Y.%m.%d %H:%M:%S", localtime(&tt));
#else
  // WinCE apparently doesn't have "time()", "strftime()", or "gmtime()",
  // so generate the "Date:" header a different, WinCE-specific way.
  // (Thanks to Pierre l'Hussiez for this code)
  SYSTEMTIME SystemTime;
  GetSystemTime(&SystemTime);
  WCHAR dateFormat[] = L"ddd, MMM dd yyyy";
  WCHAR timeFormat[] = L"HH:mm:ss GMT\r\n";
  WCHAR inBuf[200];
  DWORD locale = LOCALE_NEUTRAL;
  
  int ret = GetDateFormat(locale, 0, &SystemTime,
			  (LPTSTR)dateFormat, (LPTSTR)inBuf, sizeof inBuf);
  inBuf[ret - 1] = ' ';
  ret = GetTimeFormat(locale, 0, &SystemTime,
		      (LPTSTR)timeFormat,
		      (LPTSTR)inBuf + ret, (sizeof inBuf) - ret);
  wcstombs(buf, inBuf, wcslen(inBuf));
#endif
  return buf;
}


//////////////// Notify Message Unicast Function //////////////////////////////
/*
static int setupSocket( Port port, Boolean makeNonBlocking) {
  
  int newSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (newSocket < 0) {
    return newSocket;
  }

  return newSocket;
}
*/

static int UntilReadable( int socket, struct timeval* timeout) {
  int result = -1;
  do {
    fd_set rd_set;
    FD_ZERO(&rd_set);
    if (socket < 0) break;
    FD_SET((unsigned) socket, &rd_set);
    const unsigned numFds = socket+1;
    
    result = select(numFds, &rd_set, NULL, NULL, timeout);
    if (timeout != NULL && result == 0) {
      break; // this is OK - timeout occurred
    } else if (result <= 0) {
      break;
    }
    
    if (!FD_ISSET(socket, &rd_set)) {
      break;
    }
  } while (0);

  return result;
}

static int ReadableSocket(int socket, unsigned char* buffer, unsigned bufferSize,
	       struct sockaddr_in& fromAddress,
	       struct timeval* timeout) {
  int bytesRead = -1;
  do {
    int result = UntilReadable(socket, timeout);
    if (timeout != NULL && result == 0) {
      bytesRead = 0;
      break;
    } else if (result <= 0) {
      break;
    }
    
    SOCKLEN_T addressSize = sizeof fromAddress;
    bytesRead = recvfrom(socket, (char*)buffer, bufferSize, 0,
			 (struct sockaddr*)&fromAddress,
			 &addressSize);
    if (bytesRead < 0) {
      break;
    }
  } while (0);
  
  return bytesRead;
}

//////////////// MsgNotify Start /////////////////////////////////////    
//MsgNotify::MsgNotify( _NOTIMSG_ * Notify, _ILIST_ * List )
//  : fNotify(Notify), fList(List), fSocket(-1)	{
MsgNotify::MsgNotify()
  : fNotify(NULL), fList(NULL)	{
#ifdef DEBUG_XX
	fprintf(stderr,"fNotify->Jobs = %c\n", fNotify->Jobs);
#endif 

}
                                                         
MsgNotify::~MsgNotify()	{
}

void MsgNotify::ActionEX(_IPLIST_ * IpSTB)
{
	// We don't yet have a TCP socket.  Set one up (blocking) now:
#ifdef DEBUG
	fprintf(stderr, "Sending IP = %s(fSocket = %d)\n", IpSTB->Ip, IpSTB->fSocket);
#endif

	//while( IpSTB->fSocket < 0 )	IpSTB->fSocket = setupSocket( 0, False /* =>blocking */ );
	if (IpSTB->fSocket < 0)	{
#ifdef DEBUG
		fprintf(stderr, "Invalid Socket = %d (fSocket = %s)\n", IpSTB->fSocket, IpSTB->Ip);
#endif
		return ;
	}
	memcpy(&fSender[0], IpSTB->SettopID, 8)	;

	if (!SenderEX(IpSTB->fSocket))	{
#ifdef DEBUG
		fprintf(stderr, "Sending Failed IP = %s(fSocket = %d)\n", IpSTB->Ip, IpSTB->fSocket);
#endif
		return	;
	}

#ifdef DEBUG
#ifdef DEBUG_XX
	for ( unsigned j = 0 ; j < fDataLength; j ++ )	{
		fprintf(stderr, "fDataLength[%d] = %02x, %c \n", j, fSender[j], fSender[j] );
	}
#endif 
	fprintf(stderr, "Sending Str   = %s\n", &fSender[_HSIZE_]);
	fprintf(stderr, "Sending OK IP = %s(fSocket = %d)\n", IpSTB->Ip, IpSTB->fSocket);
#endif
/*
	if (!ReceiverEX(IpSTB->fSocket))	{
		if (100 != MySQL_SettopONUpdate(fList->fXview, IpSTB->Ip, -1)) {
#ifdef DEBUG
	  		fprintf(stderr,"MySQL_SettopONUpdate Error !!!\n");
#endif  	
		}

#ifdef DEBUG
		fprintf(stderr, "Receiving Failed IP = %s(fSocket = %d)\n", IpSTB->Ip, IpSTB->fSocket);
#endif
		return	;
	}
#ifdef DEBUG
	fprintf(stderr, "Receiving OK IP = %s(fSocket = %d)\n", IpSTB->Ip, IpSTB->fSocket);
#endif
*/
}

void MsgNotify::MakeMsg(char * ReqID, char * ImgNo) {
	memset( fSender , 0, sizeof(fSender ))		;
  	unsigned char bodySize[_SZ_]				;
	unsigned size = 17							;
	memcpy( bodySize , IntToHex(size), _SZ_	)	;

	////  Header  //////  	
	memcpy( &fSender[ 0], "00000000",   8 	)	;
	memcpy( &fSender[ 8], ReqID		,   3 	)	;
	memcpy( &fSender[11], bodySize 	, _SZ_	)	;
	memcpy( &fSender[14], " " 		,	1	)	;
	/////////////////////////////////////////////
	memcpy( &fSender[15], fNotify->JpgAlias,15)	;
	memcpy( &fSender[30], ImgNo            , 2)	;
	fDataLength = _HSIZE_ + size				;
	
#ifdef DEBUG_XX
	for (unsigned i = 0; i < fDataLength; i++ )	{
		fprintf(stderr,"Data[%u] = %c , %02x \n", i, fSender[i], fSender[i]);
	}
#endif
}

Boolean MsgNotify::SenderEX(int clientSocket) {
#ifdef DEBUG_XX
	for (unsigned i = 0; i < fDataLength; i++ )	{
		fprintf(stderr,"(fDataLength = %u)Data[%u] = %c , %02x \n", fDataLength, i, fSender[i], fSender[i]);
	}
#endif
	return send(clientSocket, fSender, fDataLength, 0) == (ssize_t)fDataLength;
}

Boolean MsgNotify::ReceiverEX(int clientSocket) {
	struct 	sockaddr_in dummy	; // 'from' address, meaningless in this case
	struct 	timeval 	timeout	;
	int 	bytesLeft 	= sizeof(fReciver)	;
	unsigned char* ptr	= fReciver			;
	char	respCode[4]	;
	
	memset (fReciver, 0, sizeof (fReciver))	;
	memset( respCode, 0, sizeof (respCode))	;
	
	do	{
		timeout.tv_sec  = 1 ;	// time 설정...
		timeout.tv_usec = 0 ;

		int bytesRead = ReadableSocket(clientSocket, ptr, bytesLeft, dummy, &timeout);
	    if (bytesRead <= 0) {
	      	// Do here Time delay...
	      	break;
	    }

#ifdef	DEBUG_XX
    	ptr[bytesRead] = '\0';
    	fprintf(stderr, "N / F Settop Ack read %d bytes:%s\n", bytesRead, ptr);

		for (int i = 0; i < bytesRead; i++ )	{
			fprintf(stderr,"Receved Data[%u] = %c , %02x \n", i, ptr[i], ptr[i]);
		}
#endif
			
		memcpy(respCode, &ptr[15], 3);

#ifdef	DEBUG
		fprintf(stderr, "respCode = %s\n", respCode );
#endif
		if (strcmp(respCode,"100") == 0) return True; else return False;
	} 	while (0);
	
	// An error occurred:
	return False ;
}
///////////MsgNotify End ////////////////////////////////////////////////////////////


static void * BroadCastSender( void * arg )
{
    _SERVER_ * fServer = (_SERVER_*)arg	;

	/////////////////////////////////////////////
	_LOCAL_  * fLocal  = new _LOCAL_	;	
	if ( fLocal )	{
		unsigned char fBuffer[_STR_]	;
		u_char 		  tmpStr [    4]	;
		char 		  arpStr [_STR_]	;
		unsigned	  fDataSize = 8		;
		unsigned  	  nbyte				;
		memset(fBuffer,0,sizeof(fBuffer));
		memset(fLocal ,0,sizeof(_LOCAL_));
	
		///////// HEADER ////////////////////////////////
		fBuffer[0] = 0x00;	fBuffer[1] = 0x00;
		memcpy(&fBuffer[2], fServer->fHeader->MacAddr,6);
		/////////////////////////////////////////////////
		fLocal->respCode = MySQL_getLocalAddr(fServer, fLocal);
		
		if ( fLocal->respCode == 100 )	{
			for ( unsigned i = 0 ; i < _ADDR; i ++ )	{
				memset (tmpStr, 0,sizeof(tmpStr))	;
				nbyte = inet_addr(fLocal->LocalAddr[i]);
				// network byte ordering ( converted ) 
				IntTo4Char( tmpStr, nbyte )			;
				memcpy (&fBuffer[fDataSize],tmpStr,4);
				fDataSize += 4						;
			}

#ifdef DEBUG_XX
			for ( unsigned i = 0; i < fDataSize; i ++ )
				fprintf(stderr,"fBuffer[%d] = %c, %02x \n", i, fBuffer[i], fBuffer[i]);
#endif
			// arp table add ..		
			memset (arpStr, 0, sizeof(arpStr));
			sprintf(arpStr, "arp -s %s %02X:%02X:%02X:%02X:%02X:%02X",
							fLocal->LocalAddr[0]		,	
							fServer->fHeader->MacAddr[0],
							fServer->fHeader->MacAddr[1],
							fServer->fHeader->MacAddr[2],
							fServer->fHeader->MacAddr[3],
							fServer->fHeader->MacAddr[4],
							fServer->fHeader->MacAddr[5]);
			system (arpStr)	;
			fServer->fAddr.sin_addr.s_addr = our_inet_addr(fLocal->LocalAddr[0]);
			
#ifdef DEBUG
			fprintf(stderr,"arpStr = %s \n", arpStr );
#endif

#ifdef DEBUG	
			Boolean result =
#endif
				sendto( fServer->fsock, fBuffer, fDataSize, MSG_DONTROUTE, (struct sockaddr*)&fServer->fAddr, sizeof fServer->fAddr) >= 0;
#ifdef DEBUG
			if ( result )
				fprintf(stderr,"BroadCast Response OK !!![%d]\n", result);
			else
				fprintf(stderr,"BroadCast Response Failed[%d]\n", result);
#endif

			// arp table delete ..
			memset (arpStr, 0, sizeof(arpStr));
			sprintf(arpStr, "arp -d %s", fLocal->LocalAddr[0]);
			system (arpStr)	;

#ifdef DEBUG
			fprintf(stderr,"arpStr = %s \n", arpStr );
#endif
		}
		delete	fLocal	;
	}
	////////////////////////////////////////////////////////////

    if(fServer->fHeader) delete  fServer->fHeader;
    fServer->fHeader = NULL		;
    if(fServer) delete  fServer	;

#ifdef DEBUG
	fprintf(stderr,"Thread close !!!\n");
#endif	
    pthread_detach(pthread_self())	;
    pthread_exit(NULL)				;
}


static Boolean parseBroadcastString ( char const* reqStr, unsigned reqStrSize, _DISCOVER_ * fHeader )
{
  	// This parser is currently rather dumb; it should be made smarter #####
  	// Read everything up to the first space as the command name:
  	memset (fHeader, 0, sizeof(_DISCOVER_));

	//	Step 1 : Header Parsing Start..
	if ( reqStrSize >= _BCAST_ )	{
		fHeader->Opcode	= reqStr[0]	;
		fHeader->Flag	= reqStr[1]	;
		memcpy(fHeader->MacAddr,&reqStr[2],6);
	
		if (fHeader->Opcode == 0x01 && fHeader->Flag == 0x00 )
			return True	;
		else
			return False;
	}	else
		return False;
}


static void * BroadCastReceiving( void * arg )
{
    int 		sock			;
    unsigned	totalbytes		;
    const int 	so_reuseaddr = 1;
    unsigned char fBuffer[_STR_];
    
    _CONNECT_ * fXview  = (_CONNECT_*)arg;

    struct sockaddr_in serverAddr	;
    struct sockaddr_in clientAddr	;
    SOCKLEN_T addrsize = sizeof clientAddr	;
	
	int res = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	if ( res != 0 )	{
		perror("Thread pthread_setcancelstate failed");
		pthread_detach(pthread_self());
		pthread_exit(NULL);		
	}

  	memset(&serverAddr, 0, sizeof(serverAddr));
  	serverAddr.sin_family 	  	= AF_INET;
  	serverAddr.sin_addr.s_addr	= inet_addr("255.255.255.255");
  	serverAddr.sin_port			= htons(7559);
    
    sock = setupUDPSocket()	;
    if(sock == -1)	{
    	fprintf(stderr,"socket failed \n");
    	pthread_detach(pthread_self());
    	pthread_exit(NULL);		
	}

  	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		 (const char*)&so_reuseaddr, sizeof so_reuseaddr) < 0) {
    	fprintf(stderr,"socketopt failed \n");
    	pthread_detach(pthread_self());
    	pthread_exit(NULL);		
  	}

    if (bind(sock, (struct sockaddr*)&serverAddr, sizeof serverAddr) != 0) {
    	fprintf(stderr,"socket bind \n");
      	pthread_detach(pthread_self());
      	pthread_exit(NULL);		
    }

#ifdef DEBUG
	fprintf(stderr,"Xview IP-Allocation Server Running ...\n");
#endif	

	while ( True )	{
		memset( fBuffer   , 0, sizeof(fBuffer)   );
		memset(&clientAddr, 0, sizeof(clientAddr));
		
    	int bytesRead = recvfrom( sock, fBuffer, sizeof fBuffer, 0, (struct sockaddr*)&clientAddr, &addrsize );
    	if (bytesRead <= 0) {
    		continue	;
    	}
	
		totalbytes = bytesRead ;

#ifdef DEBUG
		fprintf(stderr,"client ipaddr  = %s \n", inet_ntoa(clientAddr.sin_addr));
		fprintf(stderr,"client port    = %d \n", ntohs(clientAddr.sin_port)    );
#endif
		_DISCOVER_*	fHeader = new _DISCOVER_ ;
		if (parseBroadcastString((char*)fBuffer, totalbytes, fHeader))	{
			pthread_t mythread	;
			int  	  result	;
			_SERVER_  * fServer	= new _SERVER_	;
			memset(fServer, 0, sizeof(_SERVER_));

			if ( fHeader->MacAddr != NULL )	{
#ifdef DEBUG
				fprintf(stderr,"client Macaddr = %02X:%02X:%02X:%02X:%02X:%02X\n",
								fHeader->MacAddr[0],	fHeader->MacAddr[1],
								fHeader->MacAddr[2],	fHeader->MacAddr[3],
								fHeader->MacAddr[4],	fHeader->MacAddr[5]);
#endif
				fServer->fHeader	= fHeader		;
				fServer->fXview 	= fXview		;
				fServer->fAddr		= clientAddr	;
				fServer->fsock		= sock			;
	
				result = pthread_create( &mythread, NULL, BroadCastSender, (void*)fServer );
				if ( result != 0 )	{
#ifdef DEBUG
					fprintf(stderr, "For some reason I couldn't create a thread! (errno = %d)\n",result);
#endif
    				if(fServer->fHeader) delete  fServer->fHeader;
    				fServer->fHeader = NULL	;
					if(fServer) delete fServer;
				}
			}	else	{
#ifdef DEBUG
				fprintf(stderr, "Nothing Client Mac Address !!!\n");
#endif
			
				if(fServer) delete fServer;
			}
		}	else	{
#ifdef DEBUG
			fprintf(stderr, "Not BoradCasting Message !!\n");
#endif
			if(fHeader) delete  fHeader;
		}
	}

    closeSocket(sock)	;
    pthread_detach(pthread_self());
    pthread_exit(NULL)	;
}

////////// GetBroadCast::Main Thread /////////////////////////////////////////
GetBroadCast::GetBroadCast( _CONNECT_ * Xview )
	: flXview(Xview)	{
  	Starting()	;
}

GetBroadCast::~GetBroadCast() {
	int res = pthread_cancel(fThread)	;
	if ( res != 0 )	{
		perror("Thread Detach failed\n");
		exit(EXIT_FAILURE)	;	
	}
}

void GetBroadCast::Starting() {
	int res	= pthread_create( &fThread, NULL, BroadCastReceiving, (void *)flXview );
	if ( res != 0 )	{
		perror("Thread Creation failed\n");
		exit(EXIT_FAILURE)	;	
	}
}
/////////////////////////////////////////////////////////////////////////////

static char const* gettoday() {
  static char buf[10]	;
  time_t tt = time(NULL);
  strftime(buf, sizeof buf, "%Y%m%d", localtime(&tt));
  return buf;
}


////////////////////////////////////////////////////////////////////////////////////////////
static unsigned ImageResize(char * srcFile, char * orgFile, char * destFile1, char * destFile2 )
{
	char	tmpStr[300]	;
	memset (tmpStr, 0, sizeof(tmpStr));
	
	srcFile[20] = '\0';
	sprintf(destFile1,"%s_S.jpg", srcFile);
	sprintf(destFile2,"%s_B.jpg", srcFile);

#ifdef DEBUG
	fprintf(stderr,"srcFile   = %s\n", srcFile  );
	fprintf(stderr,"orgFile   = %s\n", orgFile  );
	fprintf(stderr,"destFile1 = %s\n", destFile1);
	fprintf(stderr,"destFile2 = %s\n", destFile2);
#endif	
	
	sprintf(tmpStr, "/usr/local/bin/convert -resize 75x56! %s %s/%s", orgFile, ImgSmallDir, destFile1);
 	if ( system(tmpStr) == 0 )	{
 		sprintf(tmpStr, "/usr/local/bin/convert -resize 700x525! %s %s/%s", orgFile, ImgSmallDir, destFile2);
 		return system(tmpStr);
	}	return	1;
}

static unsigned ftpTransfer( char * TransFile1, char * TransFile2 )
{
#ifdef DEBUG
	fprintf(stderr,"Transfer file1 = %s\n", TransFile1);
	fprintf(stderr,"Transfer file2 = %s\n", TransFile2);
#endif	

	FILE * ftp	;
	if(( ftp = fopen( "./ftpscript", "w" )) != NULL )
	{
	    fprintf( ftp, "(\n" );
	    fprintf( ftp, "for host in %s\n", DalkiIP ); 
	    fprintf( ftp, "do\n" );
	    fprintf( ftp, "echo \"\n" );
	    fprintf( ftp, "open %s\n", DalkiIP ); 
	    fprintf( ftp, "user %s %s\n", ftpUser, ftpPass );
	    fprintf( ftp, "binary\n" );
	    fprintf( ftp, "prompt\n" );
	    fprintf( ftp, "cd %s\n" , DalkiWebDir );
	    fprintf( ftp, "lcd %s\n", ImgSmallDir );
	    fprintf( ftp, "put %s\n", TransFile1  ); 
	    fprintf( ftp, "put %s\n", TransFile2  ); 
	    fprintf( ftp, "close\n" );
	    fprintf( ftp, "bye\n" );
	    fprintf( ftp, "\"\n" );
	    fprintf( ftp, "done\n" );
	    fprintf( ftp, ")|ftp -i -n -v > ./ftp_down_log\n" );
	    fclose ( ftp );
	
    	if ( system( "bash ./ftpscript" ) < 0 ) 
    		 return  0;
    	else return  1;

	}	else
		return 0;
}

static unsigned httpTransfer(char * fRFID, char * fName, char * fFile1, char * fFile2 )
{
#ifdef DEBUG
	fprintf(stderr,"fRFID  = %s\n", fRFID);
	fprintf(stderr,"fName  = %s\n", fName);
	fprintf(stderr,"fFile1 = %s\n", fFile1);
	fprintf(stderr,"fFile2 = %s\n", fFile2);
#endif

	char	tmpStr[500]	;
	memset (tmpStr,0,sizeof(tmpStr));
	sprintf(tmpStr, "wget -o %slogs -O %s%s.http_log 'http://www.dalki.com/star/offline/photo_insert.asp?RFID=%s&name=%s&photo_name=%s'",
				ImgLogDir, ImgLogDir, fRFID, fRFID, fName, fFile2 );
	return	system(tmpStr);

/*
	sprintf(tmpStr, "wget -o %slogs -O %s%s.http_1 'http://www.dalki.com/star/offline/photo_insert.asp?RFID=%s&name=%s&photo_name=%s'",
					ImgLogDir, ImgLogDir, fRFID, fRFID, fName, fFile1 );

	if (system(tmpStr) == 0)	{
#ifdef DEBUG
		fprintf(stderr,"insert1 OK !!!\n");
#endif
		sprintf(tmpStr, "wget -o %slogs -O %s%s.http_2 'http://www.dalki.com/star/offline/photo_insert.asp?RFID=%s&name=%s&photo_name=%s'",
					ImgLogDir, ImgLogDir, fRFID, fRFID, fName, fFile2 );
		return system(tmpStr);
	}	else
		return 1	;
*/	
}
////////////////////////////////////////////////////////////////////////////////////////////


////////// logServer::logWriter Thread //////////
static void * LogWriting( void * arg ) {
	FILE  * flog			;
	char	tmpLogs[300]	;	
	char	tmpDate[ 50]	;
    char	Path   [255]	;
    char	ToDay  [  9]	;
    char	TheDay [  9]	;

	logQueue * WQueue = (logQueue*)arg			;
	memset( WQueue->fQNode, 0, sizeof(_QNode_))	;

	int res = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
	if ( res != 0 )	{
		perror("Thread pthread_setcancelstate failed");
		pthread_detach(pthread_self());
		pthread_exit(NULL);		
	}

	//////////////////////////////////////////////////////////
	sprintf(ToDay ,"%s",gettoday())	;
	sprintf(TheDay,"%s",gettoday())	;
	sprintf(Path  ,"../logs/xview_image_%s.logs", ToDay);

	flog = fopen(Path, "a+");
    if( flog == NULL)	{
    	fprintf(stderr,"file Open failed \n");
    	pthread_detach(pthread_self());
    	pthread_exit(NULL);
	}
  	//////////////////////////////////////////////////////////

	while ( True )	{
		usleep(_DELAYTIME_);

		if ( WQueue->QCounter > 0 && WQueue->Get(WQueue->fQNode) )	{
			memset ( tmpLogs,    0, sizeof(tmpLogs)	);
			memset ( tmpDate,    0, sizeof(tmpDate)	);
			sprintf( tmpDate, "%s", dateHeader()	);
			if ( WQueue->fQNode->Jobs == _LOGSTR_ )	{	// Log Writing ...
				sprintf (tmpLogs, "%s \"%s\"\n", tmpDate, WQueue->fQNode->lStr);
			}
			
			memset( WQueue->fQNode, 0, sizeof(_QNode_));
#ifdef DEBUG
			fprintf (stderr, "%s", tmpLogs );
#endif

			sprintf(TheDay,"%s",gettoday())	;
			if( strcmp( TheDay, ToDay ) )	{
#ifdef DEBUG
				fprintf(stderr,"file reopen !!!\n")				 ;
#endif
				sprintf(ToDay,"%s",gettoday())				  	 ;
				sprintf(Path ,"../logs/xview_image_%s.logs", ToDay);
				fclose (flog)									 ;
				flog = fopen(Path, "a+")					 	 ;
			    if( flog == NULL)	{
			    	fprintf(stderr,"file reOpen failed \n");
			    	break	;
				}
			}
			
			if( !fwrite(tmpLogs, sizeof(unsigned char), strlen(tmpLogs), flog) )
					fprintf(stderr,"fwrite xview manager Failed !!!(%s)\n", tmpLogs);
			else	fflush(flog);
		}
	}
	fclose(flog)		;
	pthread_detach(pthread_self());
	pthread_exit(NULL)	;
}

////////// logServer::logWriter //////////
logWriter::logWriter (logQueue*	lQueue)
	: fWQueue(lQueue)	{
	Writing()			;
}

logWriter::~logWriter() {
	while ( fWQueue->QCounter > 0 ) {
#ifdef DEBUG
		fprintf(stderr, "QCounter = %u \n", fWQueue->QCounter);
#endif
		usleep(_DELAYTIME_)	;
	}

	int res = pthread_cancel(fThread)	;
	if ( res != 0 )	{
		perror("Thread Detach failed\n");
		exit(EXIT_FAILURE)	;	
	}
}

void logWriter::Writing() {
	int res	= pthread_create( &fThread, NULL, LogWriting, (void *)fWQueue );
	if ( res != 0 )	{
		perror("Thread Creation failed\n");
		exit(EXIT_FAILURE)	;	
	}
}


////////// logServer::logQueue //////////
logQueue::logQueue ( _CONNECT_ * Xview )
	: fQNode(NULL), fXview(Xview), 
	  QCounter(0) , fHead (NULL) , fTail(NULL) {
  	Init()	;
}

logQueue::~logQueue() {
	Clear()	;
	if(fHead ) delete fHead ;
	if(fTail ) delete fTail ;
	if(fQNode) delete fQNode;
}

void logQueue::Init () {
	fQNode = new _QNode_;
	fHead  = new _QNode_;
	fTail  = new _QNode_;
	fHead->prev	= fHead	;
	fHead->next	= fTail	;
	fTail->prev	= fHead	;
	fTail->next	= fTail	;
}

void logQueue::Clear () {
	_QNode_	* tmpNode	;
	_QNode_	* tmpDele	;

	tmpNode	= fHead->next;
	while ( tmpNode != fTail )	{
		tmpDele = tmpNode			;
		tmpNode = tmpNode->next		;
		if(tmpDele) delete tmpDele	;
	}
	fHead->next	= fTail	;
	fTail->prev = fHead	;
}

void logQueue::Put ( _QNode_ * PNode ) {
	_QNode_	* tmpNode	;
	if ( (tmpNode = new _QNode_) == NULL )	{
		fprintf( stderr, "Out of Memory \n");
		return	;
	}		
	
	///////// DATA AREA /////////////////////////
	memcpy ( tmpNode, PNode, sizeof(_QNode_))	;
	/////////////////////////////////////////////

	fTail->prev->next	= tmpNode		;
	tmpNode->prev 		= fTail->prev	;
	fTail->prev			= tmpNode		;
	tmpNode->next		= fTail			;
	QCounter ++							;
}

int logQueue::Get ( _QNode_ * GNode )	{
	_QNode_	* tmpNode		;
	tmpNode	= fHead->next	;
	if ( tmpNode == fTail )	{
		GNode	= NULL	;
		return	0		;
	}
	
	///////// DATA AREA /////////////////////////
	memcpy ( GNode , tmpNode, sizeof(_QNode_))	;
	/////////////////////////////////////////////

	fHead->next			= tmpNode->next	;
	tmpNode->next->prev	= fHead			;
	if(tmpNode) delete tmpNode			;
	QCounter -- 						;
	return	 1							;
}

////////////////////////////////////////////////


void MsgNotify::SerialAction(Boolean Forward, Boolean IsMulti)	{
	////////////////////////////////////////////////////////////////
	unsigned CurrNo = random(fList->fCount)	;
	memcpy (fNotify->JpgAlias, fList->fImgLst[CurrNo].JpgAlias, 15);
#ifdef DEBUG
	fprintf(stderr, "Serial Action !!!\n");
#endif

	MakeMsg (cmdJpgToClient, "00");
//	if (!IsMulti) MakeMsg (cmdJpgToClient, "00");
	if ( Forward )	{
		for ( unsigned j = 0; j < fNotify->Count; j ++ )	{
			if (fList->SnapShot)	break;
//			if (IsMulti) MakeMsg (cmdJpgToClient, fNotify->IpList[j].ImgNo);
			ActionEX(&fNotify->IpList[j]);
			
			// 파도 딜레이...
			usleep(_NEXTDELAY_)	;
		}
	}	else	{
		for ( int j = fNotify->Count-1; j >= 0; --j )	{
			if (fList->SnapShot)	break;
//			if (IsMulti) MakeMsg (cmdJpgToClient, fNotify->IpList[j].ImgNo);
			ActionEX(&fNotify->IpList[j]);
			
			// 파도 딜레이...
			usleep(_NEXTDELAY_)	;
		}
	}
	////////////////////////////////////////////////////////////////
}

void MsgNotify::DivideAction(unsigned Strt, unsigned End)	{
	unsigned strt = Strt	;
	unsigned next = End		;
	unsigned slct = random(fList->fCount)	;
	memcpy (fNotify->JpgAlias, fList->fImgLst[slct].JpgAlias, 15);
	
	if (next >= fNotify->Count)	next = fNotify->Count;
#ifdef DEBUG
	fprintf(stderr, "Divide Action : Start = %d, Next = %d\n", strt, next );
#endif

	for ( unsigned i = strt; i < next ; i++ )	{
		if(fList->SnapShot)	break;	// || fList->STBUpdate
		MakeMsg(cmdJpgToClient, fNotify->IpList[i].ImgNo);
		ActionEX(&fNotify->IpList[i]);
	}
}

void MsgNotify::RandomAction()	{
	unsigned idxList[fNotify->Count];
	unsigned slt1, slt2, slt;

	for ( unsigned i = 0; i < fNotify->Count; i++ )
		idxList[i] = i	;
	for ( unsigned i = 0; i < fNotify->Count; i++ )	{
		slt1 = random(fNotify->Count);
		slt2 = random(fNotify->Count);
		Swap(idxList[slt1],idxList[slt2],slt);
	}
	
	for ( unsigned j = 0; j < (fNotify->Count/(_MAX_LOOP_+1))+1; j++)	{
		for ( unsigned i = 0; i < fNotify->Count; i ++ )	{
			unsigned CurrNo = random(fList->fCount)	;
			memcpy (fNotify->JpgAlias, fList->fImgLst[CurrNo].JpgAlias, 15);
	
			if(fList->SnapShot)	break				;
			MakeMsg (cmdJpgToClient, "00")			;
			ActionEX(&fNotify->IpList[idxList[i]])	;
//			usleep(100)	;
		}
		if(fList->SnapShot)	break;
	}
}

void MsgNotify::RandomAfter ()	{
	unsigned idxList[fNotify->Count];
	unsigned slt1, slt2, slt;

	unsigned CurrNo = random(fList->fCount)	;
	memcpy (fNotify->JpgAlias, fList->fImgLst[CurrNo].JpgAlias, 15);

	for ( unsigned i = 0; i < fNotify->Count; i++ )
		idxList[i] = i	;
	for ( unsigned i = 0; i < (fNotify->Count/2); i++ )	{
		slt1 = random(fNotify->Count);
		slt2 = random(fNotify->Count);
		Swap(idxList[slt1],idxList[slt2],slt);
	}

	for ( unsigned i = 0; i < fNotify->Count; i ++ )	{
		if(fList->SnapShot)	break;
		MakeMsg (cmdJpgToClient, fNotify->IpList[idxList[i]].ImgNo)	;
		ActionEX(&fNotify->IpList[idxList[i]])	;
//		usleep(100);
	}
}

void MsgNotify::DivideRandomAction(unsigned Strt, unsigned End, unsigned Offset)	{
	unsigned Element = End-Strt	;
	unsigned idxList[Element] 	;
	unsigned slt1, slt2, slt  	;

	for ( unsigned i = 0; i < Element; i++ )
		idxList[i] = i	;
	for ( unsigned i = 0; i < Element; i++ )	{
		slt1 = random(Element)	;
		slt2 = random(Element)	;
		Swap(idxList[slt1],idxList[slt2],slt);
	}
	
	for ( unsigned j = 0; j < (fNotify->Count/(_MAX_LOOP_+1))+1; j++)	{
		for ( unsigned i = 0; i < Element; i ++ )	{
			unsigned s = random(fList->fCount)	;
			memcpy (fNotify->JpgAlias, fList->fImgLst[s].JpgAlias, 15);
	
			if (fList->SnapShot) break				 	 ;
			MakeMsg (cmdJpgToClient, "00")				 ;
			ActionEX(&fNotify->IpList[idxList[i]+Offset]);
//			usleep(100);
		}
		if(fList->SnapShot)	break;
	}
}

void MsgNotify::SideToMiddleAction()	{
	unsigned CurrNo = random(fList->fCount)	;
	memcpy (fNotify->JpgAlias, fList->fImgLst[CurrNo].JpgAlias, 15);

	int	Full = fNotify->Count - (fNotify->Count % 2);
	int half = (int)(Full/2);

#ifdef DEBUG
	fprintf(stderr, "SideToMiddle Action : Full = %d, half = %d\n", Full, half );
#endif

	if (Full != (int)fNotify->Count)	{
		if (fList->SnapShot)	return ;
		MakeMsg (cmdJpgToClient, fNotify->IpList[Full].ImgNo);
		ActionEX(&fNotify->IpList[Full]);
	}

	for ( int i = 0, j = Full-1; i < half && half <= j ; i++, j-- )	{
		if (fList->SnapShot)	break;
		MakeMsg (cmdJpgToClient, fNotify->IpList[i].ImgNo);
		ActionEX(&fNotify->IpList[i]);
		
		if (fList->SnapShot)	break;	
		MakeMsg (cmdJpgToClient, fNotify->IpList[j].ImgNo);
		ActionEX(&fNotify->IpList[j]);
		
		// 파도 딜레이...
		usleep(_NEXTDELAY_)	;
	}
}

void MsgNotify::TwoForwardAction()	{
	unsigned CurrNo = random(fList->fCount)	;
	memcpy (fNotify->JpgAlias, fList->fImgLst[CurrNo].JpgAlias, 15);

	int	Full = fNotify->Count - (fNotify->Count % 2);
	int half = (int)(Full/2);

#ifdef DEBUG
	fprintf(stderr, "TwoForward Action : Full = %d, half = %d\n", Full, half );
#endif

	for ( int i = 0, j = half; i < half && j < Full; i++, j++ )	{
		if (fList->SnapShot)	break;
		MakeMsg (cmdJpgToClient, fNotify->IpList[i].ImgNo);
		ActionEX(&fNotify->IpList[i]);
		
		if (fList->SnapShot)	break;
		MakeMsg (cmdJpgToClient, fNotify->IpList[j].ImgNo);
		ActionEX(&fNotify->IpList[j]);
		
		// 파도 딜레이...
		usleep(_NEXTDELAY_)	;
	}

	if (Full != (int)fNotify->Count)	{
		if (fList->SnapShot)	return ;
		MakeMsg (cmdJpgToClient, fNotify->IpList[Full].ImgNo);
		ActionEX(&fNotify->IpList[Full]);
	}
}

void MsgNotify::TwoReverseAction()	{
	unsigned CurrNo = random(fList->fCount)	;
	memcpy (fNotify->JpgAlias, fList->fImgLst[CurrNo].JpgAlias, 15);

	int	Full = fNotify->Count - (fNotify->Count % 2);
	int half = (int)(Full/2);

#ifdef DEBUG
	fprintf(stderr, "TwoReverse Action : Full = %d, half = %d\n", Full, half );
#endif

	if (Full != (int)fNotify->Count)	{
		if (fList->SnapShot)	return ;
		MakeMsg (cmdJpgToClient, fNotify->IpList[Full].ImgNo);
		ActionEX(&fNotify->IpList[Full]);
	}
	
	for ( int i = half-1, j = Full-1; 0 <= i && half <= j ; i--, j-- )	{
		if (fList->SnapShot)	break;	// || fList->STBUpdate
		MakeMsg (cmdJpgToClient, fNotify->IpList[i].ImgNo);
		ActionEX(&fNotify->IpList[i]);
		
		if (fList->SnapShot)	break;	// || fList->STBUpdate
		MakeMsg (cmdJpgToClient, fNotify->IpList[j].ImgNo);
		ActionEX(&fNotify->IpList[j]);
		
		// 파도 딜레이...
		usleep(_NEXTDELAY_)	;
	}
}

void MsgNotify::BlackScreen ()	{
#ifdef DEBUG
	fprintf(stderr, "Black Screen !!!\n" );
#endif
	memcpy (fNotify->JpgAlias, "B00000000000001", 15);
	MakeMsg(cmdJpgToClient   , "00");

	for ( unsigned i = 0; i < fNotify->Count ; i++ )	{
		if(fList->SnapShot)	return;
		ActionEX(&fNotify->IpList[i]);
	}
	//////////////////////////////////////////////////
}

void MsgNotify::SnapShotAction ()	{
#ifdef DEBUG
	fprintf(stderr, "SnapShot Action !!!\n");
#endif
	//	First Phase...
	MakeMsg (cmdJpgToClient, "00");
	for ( unsigned i = 0; i < fNotify->Count; i++ )	
	{
		ActionEX(&fNotify->IpList[i]);
		// 파도 딜레이...
		usleep(_NEXTDELAY_)	;
	}
/*
	sleep(20);
	//	Second Phase...	
	for ( int i = fNotify->Count - 1; i >= 0; i-- )	{
		if (fNotify->IpList[i].ImgNo[0] >= 'A')
				MakeMsg (cmdJpgToClient, "00");
		else	MakeMsg (cmdJpgToClient, fNotify->IpList[i].ImgNo);
		ActionEX(&fNotify->IpList[i]);
		// 파도 딜레이...
		usleep(_NEXTDELAY_)	;
	}
	
	sleep(20);
	//	Third Phase...
	for ( unsigned i = 0; i < fNotify->Count; i++ )	{
		MakeMsg (cmdJpgToClient, fNotify->IpList[i].ImgNo);
		ActionEX(&fNotify->IpList[i]);
		usleep(_NEXTDELAY_)	;
	}
	sleep(10);
*/
}

Boolean	MsgNotify::WaitforSnapShotDalay(float delay)
{
	int lDelay = (int)(delay * 1000000) ;
#ifdef DEBUG
	fprintf(stderr,"snapshot delay : fList->SnapShot = %d, lDelay = %d\n", fList->SnapShot, lDelay);
#endif
	while(!fList->SnapShot && (lDelay > 0))	{
		lDelay -= _MIDWAIT_	;
		usleep(_MIDWAIT_)	;
	}
	if (fList->SnapShot)
			return True ;
	else	return False;
}

void MsgNotify::RunningEX(_NOTIMSG_ * Notify, _ILIST_ * List) 
{
	fNotify = Notify	;
	fList	= List		;

	if ( fNotify->Jobs == _SHOT_ )	{
		SnapShotAction();
	}	else	{
//		BlackScreen();	// 메세지만 보냄 (셋탑 자체에서 블랙으로 변경)
//		if(WaitforSnapShotDalay(0.005)) return;
	
		if ( fNotify->Jobs == _ONE_ )	{
			////////////////////////////////////////////////
			SerialAction(True, False);	// 정방향, 멀티무시
//			if(WaitforSnapShotDalay(_MIDDELAY_)) return;
//			SideToMiddleAction()	 ;
			///////////////////////////////////////////////
		}	else
		if (fNotify->Jobs == _TWO_)	{
			///////////////////////////////////////
			DivideAction( 0,  7);
			if(WaitforSnapShotDalay(0.2)) return;
			DivideAction( 7, 15);
			if(WaitforSnapShotDalay(0.2)) return;
			DivideAction(15, 25);
			if(WaitforSnapShotDalay(0.4)) return;
			DivideAction(25, fNotify->Count);
			if(WaitforSnapShotDalay(_MIDDELAY_)) return;
			SerialAction(False, True);	// 역방향, 멀티적용
			if(WaitforSnapShotDalay(_MIDDELAY_)) return;
			RandomAction()	;
	//		if(WaitforSnapShotDalay(0.1)) return;
			RandomAfter()	;
			//////////////////////////////////////////////
		}	else
		if ( fNotify->Jobs == _THREE_ )	{
			///////////////////////////////////////////////
			RandomAction()	;
	//		if(WaitforSnapShotDalay(0.1)) return;
			RandomAfter()	;
			if(WaitforSnapShotDalay(_MIDDELAY_)) return;
	
			// 절반씩 나눠서 랜덤 돌기..
			if (fNotify->Count > _HALFLINE_)	{
				DivideRandomAction( 0, _HALFLINE_, 0);
				if(WaitforSnapShotDalay(_MIDDELAY_)) return;
				DivideRandomAction( _HALFLINE_, fNotify->Count, _HALFLINE_);
				if(WaitforSnapShotDalay(_MIDDELAY_)) return;
			}
			/////////////////////////////////////////////
			TwoForwardAction();
			if(WaitforSnapShotDalay(_MIDDELAY_)) return;
			TwoReverseAction();
			/////////////////////////////////////////////
		}	else
		if ( fNotify->Jobs == _FOUR_ )	{
			/////////////////////////////////////////////
			RandomAction()	;
	//		if(WaitforSnapShotDalay(0.1)) return;
			RandomAfter ()	;
			/////////////////////////////////////////////
		}
	}
}

static Boolean WaitforDalay(_ILIST_ * fList, int delay)
{
	int lDelay = (int)(delay * 1000000) ;
#ifdef DEBUG
	fprintf(stderr,"main delay : fList->SnapShot = %d, lDelay = %d\n", fList->SnapShot, lDelay);
#endif
	while(!fList->SnapShot && (lDelay > 0))	{
		lDelay -= _NEXTDELAY_;
		usleep(_NEXTDELAY_)	 ;
	}
	if (fList->SnapShot)
			return True ;
	else	return False;
}

static void * RunImageSchedulingEX( void * arg )
{
    _ILIST_	  *	fList 	= (_ILIST_*)arg	;
    _NOTIMSG_ * fNotify	= fList->fNotify;
    Boolean		IsShotDisplay = True	;
    unsigned	New_Status    = _DISP_	;
    unsigned	Old_Status	  = _DISP_	;
    unsigned 	CurrNo		  = 0		;
    
    MsgNotify * fMsgNotify = new MsgNotify();

	int res = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	if ( res != 0 )	{
		perror("Thread pthread_setcancelstate failed");
		pthread_detach(pthread_self());
		pthread_exit(NULL)	;
	}

#ifdef DEBUG
	fprintf(stderr,"Xview Image Scheduling Running ...\n");
#endif	
	
	if( fNotify != NULL )	{
		memset (fNotify, 0, sizeof(_NOTIMSG_))	;
		fNotify->IpList	= NULL					;
	}

	randomize()	;

	while ( True )	{
		if (!fList->SnapShot)	{
			fNotify->Jobs = _ONE_		; //fList->fStepIt;
			New_Status	  = _DISP_	;
			
			if (!IsShotDisplay)	{
				if (WaitforDalay(fList,_DEFDELAY_)) continue;
			}	else
				IsShotDisplay = False	;
		}	else	{
			IsShotDisplay = True 		;
			fNotify->Jobs = _SHOT_		;
			New_Status	  = _SHOT_		;
		}

		if (fList->STBUpdate || fNotify->IpList == NULL || (New_Status != Old_Status))	{
			if(fNotify->IpList != NULL)	{
				free(fNotify->IpList)	;
				fNotify->IpList = NULL	;
			}
			
			if (!MySQL_OnSettopIpList(fList->fXview, fNotify))	{
				fNotify->IpList = NULL	;
#ifdef DEBUG
				fprintf(stderr,"Settop On List Empty !!!(Count = %d)\n", fNotify->Count);
#endif
				continue				;
			}
			fList->STBUpdate = False 	;	
			Old_Status = New_Status		;
			
			if (New_Status == _DISP_ && fList->SnapShot) continue;
		}

		if ( !fList->SnapShot )	{
			// 개별사진 리스트 로드
			if (fList->fImgLst != NULL)	{
				free(fList->fImgLst)	;
				fList->fImgLst = NULL	;
			}
		
			if (!MySQL_GetImgListLoads(fList->fXview, fList))	{
#ifdef DEBUG
				fprintf(stderr,"ImageListLoads Empty !!!(Count = %d)\n", fList->fCount);
#endif
				fList->fImgLst = NULL	;
				continue				;
			}
			
			if (fList->SnapShot) continue;
			
//			CurrNo = random(fList->fCount)	;
			memcpy (fNotify->JpgAlias, fList->fImgLst[CurrNo].JpgAlias, 15);
			//new MsgNotify (fNotify, fList)	;
			fMsgNotify->RunningEX(fNotify, fList);

#ifdef DEBUG
			fprintf(stderr,"fList->Image CurrNo = [%d][%s][%u]\n", CurrNo, fList->fImgLst[CurrNo].JpgAlias, fList->fCount);
#endif
			CurrNo ++ ;
			if (fList->fCount <= CurrNo) CurrNo = 0;
			
//			if (_FOUR_ < ++fList->fStepIt) fList->fStepIt = _ONE_;
		}	else	{
			if (New_Status == _DISP_) continue;

			memcpy (fNotify->JpgAlias, fList->ShotAlias[0], 15);
			//new MsgNotify (fNotify, fList)	;
			fMsgNotify->RunningEX(fNotify, fList);

			while(fList->fLocked) usleep(10);
			fList->fLocked = True	;
			for ( int i = 0 ; i < fList->SnapShot - 1; i ++ )	{
				sprintf(fList->ShotAlias[i], "%s", fList->ShotAlias[i+1]);
#ifdef DEBUG
				fprintf(stderr,"fList->ShotAlias[%d] = %s\n", i, fList->ShotAlias[i]);
#endif
			}
			fList->SnapShot --		;
			fList->fLocked = False;
			CurrNo = 0;
		}
	}

	if(fMsgNotify) delete fMsgNotify;
    pthread_detach(pthread_self())	;
    pthread_exit(NULL)				;
}


////////// ImageSender::Main Thread /////////////////////////////////////////
ImageSender::ImageSender(_CONNECT_ * Xview)
	: fXview(Xview)	{
  	fList 	= new _ILIST_			;
  	memset(fList,0,sizeof(_ILIST_))	;

	fList->fXview  	 = fXview		;
  	fList->STBUpdate = True 		;
  	fList->fStepIt	 = _ONE_		;
  	fList->fLocked	 = False 		;
  	fList->SnapShot	 = 0			;

	fList->fNotify 	 = new _NOTIMSG_;
	Start()	;
}

ImageSender::~ImageSender() {
	int res = pthread_cancel(fThread);
	if ( res != 0 )	{
		perror("Thread Detach failed\n");
		exit(EXIT_FAILURE)	;	
	}
	if(fList->fNotify)	delete fList->fNotify;
	if(fList)			delete fList;
}

void ImageSender::NewStillCut(char * Alias) {
  	while (fList->fLocked) usleep(10)	;
  	fList->fLocked = True	;
  	
	///////////////////////////////////////
	if (fList->SnapShot < _SHOTMAX_)	{
		sprintf(fList->ShotAlias[fList->SnapShot], "%s", Alias);
		fList->SnapShot ++	;
	}
	///////////////////////////////////////
	fList->fLocked = False	;
}

void ImageSender::Start() {
	int res = pthread_create( &fThread, NULL, RunImageSchedulingEX, (void *)fList );
	if ( res != 0 )	{
		perror("Thread Creation failed\n");
		exit(EXIT_FAILURE)	;	
	}
}

void ImageSender::STBUpdated() {
	fList->STBUpdate = True ;
}

Boolean ImageSender::ftpSender(_CONNECT_ * Xview, _BUFFER_ * SSList)	{
	char	srcFile  [256]	;
	char	orgFile  [256]	;
	char	destFile1[256]	;
	char	destFile2[256]	;
	_RFID_* fRFID			;
	
	do	{
		//////////////////////////////////////////////////////////
		fRFID = new _RFID_	;
		if(!fRFID) break	;

		memset(fRFID, 0, sizeof(fRFID));

		sprintf(fRFID->fToDay, "%s"  , SSList->fToDay	 );
		sprintf(fRFID->fSeq  , "%07u", SSList->fSeq  	 );
		memcpy (fRFID->fRFID , SSList->fAlias, 10)		  ;
		sprintf(fRFID->fName ,"%s"   ,&SSList->fAlias[10]);
		fRFID->fRFID[10] = '\0';
#ifdef DEBUG
		fprintf(stderr, "fRFID->fToDay = %s\n", fRFID->fToDay)	;
		fprintf(stderr, "fRFID->fSeq   = %s\n", fRFID->fSeq  )	;
		fprintf(stderr, "fRFID->fRFID  = %s\n", fRFID->fRFID )	;
		fprintf(stderr, "fRFID->fName  = %s\n", fRFID->fName )	;
#endif					
		//////////////////////////////////////////////////////////

		if (!MySQL_GetFile(Xview, fRFID, srcFile, orgFile)) break;

#ifdef DEBUG
		fprintf(stderr,"GetFile = %s\n", srcFile);
#endif

		memset(destFile1, 0, sizeof(destFile1))	;
		memset(destFile2, 0, sizeof(destFile2))	;
		if (ImageResize(srcFile, orgFile, destFile1, destFile2))	break;

#ifdef DEBUG
		fprintf(stderr,"ImageResize OK (destFile1 = %s)!!!\n", destFile1);
		fprintf(stderr,"ImageResize OK (destFile2 = %s)!!!\n", destFile2);
#endif

		if (!ftpTransfer(destFile1, destFile2))	break;

#ifdef DEBUG
		fprintf(stderr,"ftp Transfer OK !!!\n");
#endif
		if (httpTransfer(fRFID->fRFID, fRFID->fName, destFile1, destFile2))	break;

#ifdef DEBUG
		fprintf(stderr,"http insert OK !!!\n");
#endif	

		if (!MySQL_UpdateDB_RFID(Xview, fRFID, 'Y')) break;

#ifdef DEBUG
		fprintf(stderr,"DB Update OK !!!\n");
#endif

		delete fRFID;
		return True ;
	}	while(0);

	return False;
}

void ImageSender::ftpTryTo(_CONNECT_ * Xview) {
	char	srcFile  [256]	;
	char	orgFile  [256]	;
	char	destFile1[256]	;
	char	destFile2[256]	;
	_RFID_* fRFID = new _RFID_;

	while ( True )	{
		memset(srcFile  , 0,sizeof(srcFile)  );
		memset(orgFile  , 0,sizeof(orgFile)  );
		memset(destFile1, 0,sizeof(destFile1));
		memset(destFile2, 0,sizeof(destFile2));
		memset(fRFID    , 0, sizeof(fRFID)	 );
		
		if ( !MySQL_GetSendFile(Xview,fRFID, -3) )	break;

		do	{
			//////////////////////////////////////////////////////////
#ifdef DEBUG
			fprintf(stderr, "fRFID->fToDay = %s\n", fRFID->fToDay)	;
			fprintf(stderr, "fRFID->fSeq   = %s\n", fRFID->fSeq  )	;
			fprintf(stderr, "fRFID->fRFID  = %s\n", fRFID->fRFID )	;
			fprintf(stderr, "fRFID->fName  = %s\n", fRFID->fName )	;
#endif					
			//////////////////////////////////////////////////////////

			if ( !strcmp(fRFID->fRFID,"X") || !strcmp(fRFID->fName,"X") )	{
				MySQL_UpdateDB_RFID(Xview, fRFID, 'X');
				break;
			}
			
			if (!MySQL_GetFile(Xview, fRFID, srcFile, orgFile)){
				MySQL_UpdateDB_RFID(Xview, fRFID, 'X');
				break;
			}
#ifdef DEBUG
			fprintf(stderr,"GetFile = %s\n", srcFile);
#endif

			memset(destFile1, 0, sizeof(destFile1))	;
			memset(destFile2, 0, sizeof(destFile2))	;
			if (ImageResize(srcFile, orgFile, destFile1, destFile2)){
				MySQL_UpdateDB_RFID(Xview, fRFID, 'X');
				break;
			}
#ifdef DEBUG
			fprintf(stderr,"ImageResize OK (destFile1 = %s)!!!\n", destFile1);
			fprintf(stderr,"ImageResize OK (destFile2 = %s)!!!\n", destFile2);
#endif

			if (!ftpTransfer(destFile1, destFile2))	{
				MySQL_UpdateDB_RFID(Xview, fRFID, 'X');
				break;
			}
#ifdef DEBUG
			fprintf(stderr,"ftp Transfer OK !!!\n");
#endif
			if (httpTransfer(fRFID->fRFID, fRFID->fName, destFile1, destFile2)){
				MySQL_UpdateDB_RFID(Xview, fRFID, 'X');
				break;
			}
#ifdef DEBUG
			fprintf(stderr,"http insert OK !!!\n");
#endif	

			if (!MySQL_UpdateDB_RFID(Xview, fRFID, 'Y')){
				MySQL_UpdateDB_RFID(Xview, fRFID, 'X');
				break;
			}
#ifdef DEBUG
			fprintf(stderr,"DB Update OK !!!\n");
#endif

		}	while(0);
	}
	
	if (fRFID) delete fRFID;
}

/////////////////////////////////////////



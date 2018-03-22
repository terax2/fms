#include "shlib.h"

#define  MAX_THREADS 3000

CMutex * pMutex = NULL ;
unsigned Count = 0;


// nonblock 家南积己  
void nonblock(int sockfd) 
{ 
    int opts; 
    opts = fcntl(sockfd, F_GETFL); 
    if(opts < 0) 
    { 
        perror("fcntl(F_GETFL)\n"); 
        exit(1); 
    } 
    opts = (opts | O_NONBLOCK); 
    if(fcntl(sockfd, F_SETFL, opts) < 0) 
    { 
        perror("fcntl(F_SETFL)\n"); 
        exit(1); 
    } 
} 

static int FindStreamInfo(void *data) 
{
    while ( true )
    {
        pMutex->lock();
        Count ++ ;
        printf("Thread Test ...[%u]\n", Count );
        pMutex->unlock();
        SDL_Delay(1);
    }
    return 0;
}


static int setupSocket( int port, int makeNonBlocking) 
{
  int newSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (newSocket < 0) 
  {
    return newSocket;
  }
  
  const int reuseFlag = 1;
  if (setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR,
		 (const char*)&reuseFlag, sizeof reuseFlag) < 0) 
  {
    close(newSocket);
    return -1;
  }
  
  // SO_REUSEPORT doesn't really make sense for TCP sockets, so we
  // normally don't set them.  However, if you really want to do this
  // #define REUSE_FOR_TCP
//#ifdef SO_REUSEPORT
//  if (setsockopt(newSocket, SOL_SOCKET, SO_REUSEPORT,
//		 (const char*)&reuseFlag, sizeof reuseFlag) < 0) 
//  {
//    fprintf(stderr, "SO_REUSEPORT set failed !!\n");
//    close(newSocket);
//    return -1;
//  }
//#endif

  // Note: Windoze requires binding, even if the port number is 0
#if defined(__WIN32__) || defined(_WIN32)
#else
  if (port != 0) // || ReceivingInterfaceAddr != INADDR_ANY) 
  {
#endif
    struct sockaddr_in name;
    name.sin_family = AF_INET;
    name.sin_port = port;
    name.sin_addr.s_addr = inet_addr("10.20.23.159");   // INADDR_ANY;
    if (bind(newSocket, (struct sockaddr*)&name, sizeof name) != 0) 
    {
      char tmpBuffer[100];
      sprintf(tmpBuffer, "bind() error (port number: %d): ",
	      ntohs(port));
      fprintf(stderr, "%s", tmpBuffer);
      close(newSocket);
      return -1;
    }
#if defined(__WIN32__) || defined(_WIN32)
#else
  }
#endif

  if (makeNonBlocking) 
  {
    // Make the socket non-blocking:
#if defined(__WIN32__) || defined(_WIN32) || defined(IMN_PIM)
    unsigned long arg = 1;
    if (ioctlsocket(newSocket, FIONBIO, &arg) != 0) 
    {

#elif defined(VXWORKS)
    int arg = 1;
    if (ioctl(newSocket, FIONBIO, (int)&arg) != 0) 
    {

#else
    int curFlags = fcntl(newSocket, F_GETFL, 0);
    if (fcntl(newSocket, F_SETFL, curFlags|O_NONBLOCK) < 0) 
    {
#endif
      close(newSocket);
      return -1;
    }
  }

  return newSocket;
}

static unsigned getBufferSize(int bufOptName, int socket) {
  unsigned curSize;
  socklen_t sizeSize = sizeof curSize;
  if (getsockopt(socket, SOL_SOCKET, bufOptName,
		 (char*)&curSize, &sizeSize) < 0) {
    fprintf (stderr, "getBufferSize() error: ");
    return 0;
  }

  return curSize;
}

static unsigned increaseBufferTo(int bufOptName, int socket, unsigned requestedSize) {
  // First, get the current buffer size.  If it's already at least
  // as big as what we're requesting, do nothing.
  unsigned curSize = getBufferSize(bufOptName, socket);

  // Next, try to increase the buffer to the requested size,
  // or to some smaller size, if that's not possible:
  while (requestedSize > curSize) {
    socklen_t sizeSize = sizeof requestedSize;
    if (setsockopt(socket, SOL_SOCKET, bufOptName,
		   (char*)&requestedSize, sizeSize) >= 0) {
      // success
      return requestedSize;
    }
    requestedSize = (requestedSize+curSize)/2;
  }

  return getBufferSize(bufOptName, socket);
}
static unsigned increaseSendBufferTo(int socket, unsigned requestedSize) {
  return increaseBufferTo(SO_SNDBUF, socket, requestedSize);
}
static unsigned increaseReceiveBufferTo(int socket, unsigned requestedSize) {
  return increaseBufferTo(SO_RCVBUF, socket, requestedSize);
}


static int UntilReadable( int socket, struct timeval* timeout) 
{
  int result = -1;
  do {
    fd_set rd_set;
    FD_ZERO(&rd_set);
    if (socket < 0) break;
    FD_SET((unsigned) socket, &rd_set);
    const unsigned numFds = socket+1;
    
    result = select(numFds, &rd_set, NULL, NULL, timeout);
    if (timeout != NULL && result == 0) 
    {
      break; // this is OK - timeout occurred
    } else if (result <= 0) {
      break;
    }
    
    if (!FD_ISSET(socket, &rd_set)) 
    {
      break;
    }
  } while (0);

  return result;
}

static int ReadableSocket(int socket, unsigned char* buffer, unsigned bufferSize,
	       struct sockaddr_in  fromAddress, struct timeval* timeout) 
{
  int bytesRead = -1;
  do {
    int result = UntilReadable(socket, timeout);
    if (timeout != NULL && result == 0) 
    {
      bytesRead = 0;
      break;
    } else if (result <= 0) 
    {
      break;
    }
    
    socklen_t addressSize = sizeof fromAddress;
    bytesRead = recvfrom(socket, (char*)buffer, bufferSize, 0, (struct sockaddr*)&fromAddress, &addressSize);
    if (bytesRead < 0) 
    {
      break;
    }
  } while (0);
  
  return bytesRead;
}

static int RecvSocket(int socket, unsigned char* buffer, unsigned bufferSize, struct timeval* timeout)
{
  int bytesRead = -1;
  do {
    int result = UntilReadable(socket, timeout);
    if (timeout != NULL && result == 0) 
    {
      bytesRead = 0;
      break;
    } else if (result <= 0) 
    {
      break;
    }
    
    bytesRead = recv(socket, (char*)buffer, bufferSize, 0);
    if (bytesRead < 0) 
    {
      break;
    }
  } while (0);
  
  return bytesRead;
}


static int ConnectTimeout(int fd,struct sockaddr *remote, int len, int secs)
{
	int nResult;
	fd_set fdRead, fdWrite, fdExcept;
	struct timeval Wait;

	fcntl(fd, F_SETFL, O_NONBLOCK);
	nResult = connect(fd, remote, len);
	if(nResult < 0)
	{
		if(errno != EINPROGRESS)
		{
			printf("*** Server Connect error! ***\n");
			return -1;
		}
	}
   
	FD_ZERO(&fdRead);
	FD_SET (fd,&fdRead);
	fdExcept = fdWrite = fdRead;
   
	Wait.tv_sec  = secs	;
	Wait.tv_usec = 0	;

	nResult = select(fd+1, &fdRead, &fdWrite, &fdExcept, &Wait);
	if(nResult < 0)
	{
		printf("*** Server Connect error ***\n");
		return -1;
	}
	else if(nResult == 0)
	{
		printf("*** Server Connect Timeout ***\n");
		return -1;
	}

	if(FD_ISSET(fd, &fdRead) || FD_ISSET(fd, &fdWrite) || FD_ISSET(fd, &fdExcept))
	{
		return 0;
	}
   
	return -1;
}


// Authority
int ConnectToGateWay()
{
    int GwySock = -1 ;
	while ( GwySock < 0 )
	{
	    GwySock = setupSocket( 12345, 1 /* =>nonblocking */ );
	    fprintf(stderr, "GwySock = %d\n", GwySock);
	    sleep(1);
	}
/*	
	do	{
		struct sockaddr_in gatewayName						;
		gatewayName.sin_family 		= AF_INET				;
		gatewayName.sin_port 		= htons(4355)	        ;
		gatewayName.sin_addr.s_addr = inet_addr("10.20.23.159");
		
//		if (ConnectTimeout( GwySock, (struct sockaddr*)&gatewayName, sizeof gatewayName, 5) != 0 )
		if (ConnectTimeout( GwySock, (struct sockaddr*)&gatewayName, sizeof gatewayName, 5) != 0 )
		{
			fprintf(stderr, "Connection Failed 'Test2 Server' (%s : %d)!!\n", "10.20.23.159", 4355);
			break	;
		}
		fprintf(stderr, "Connected to Test2 Server (%s : %d) (GwySock = %d)!!\n", "10.20.23.159", 4355, GwySock);
		return	GwySock	;
	} 	while ( 0 )		;
*/

    

	close(GwySock)	;
	GwySock = -1	;
	return	GwySock	;
}

// Authority
int SockToGateWay()
{
    int GwySock = -1 ;
	GwySock = setupSocket( 0, 1 /* =>nonblocking */ );
	
	if ( GwySock < 0 )
	{
	    printf("GwySock : [%d][%s]\n",errno, strerror(errno));
	    return 0;
	}   else
	    return 1;
}

int SetLimit(const unsigned nmax)
{
	// set rlimit 
	struct rlimit rlim;
	rlim.rlim_cur = rlim.rlim_max = nmax;
	int error = setrlimit(RLIMIT_NOFILE,&rlim);
	if( error )
	{
		printf("Error !!! RLIMIT_NOFILE Setting Fail [%d] \n\n",errno);
		return -1;
	}
	error = getrlimit(RLIMIT_NOFILE,&rlim);
	if( error )
	{
		printf("Error !!! RLIMIT_NOFILE Getting Fail [%d] \n\n",errno);
		return -1;
	}
	else
	{
	    printf("Test Setlimit = cur[%d] max[%d]\n",rlim.rlim_cur, rlim.rlim_max);
	}
    return 0;
}


int main ()
{
    char sendbuff[1024];
    char recvbuff[1024];
    int  sendlen ;
    
    //SetLimit(10000);
    
    
    for ( int i = 0; i < 2; i++ )
    {
        snprintf(sendbuff, sizeof(sendbuff), "Test Message");
        sendlen = strlen(sendbuff);

        int fd = ConnectToGateWay() ;
        if ( fd < 0 )
        {
            fprintf(stderr, "failed = %d\n", fd);
            exit(0);
        }
        
////        for ( int i = 0; i < 1; i++ )
////        {
//            int nResult = send( fd, sendbuff, sendlen, 0); //write (fd, sendbuff, sendlen); //
//            fprintf(stderr, "nResult = %d/%d\n", nResult, sendlen);
//        	if (nResult < sendlen)
//        	{
//        		fprintf(stderr, "Auth : sending failed !!(nResult = %d)\n", nResult);
//        		return close(fd);
//        	}
////        }
//        
//        memset (recvbuff, 0, sizeof(recvbuff));
//   	    int bytesRead = read (fd, recvbuff, sizeof(recvbuff)); //   recv(fd, recvbuff, sizeof(recvbuff), 0); //  RecvSocket(fd, recvbuff, sizeof(recvbuff), NULL);
//    	if (bytesRead < 0) {
//    		// Do here Time delay...
//    		fprintf(stderr, "read faild !!(bytesRead = %d)\n", bytesRead);
//    		return close(fd);
//    	}
//    	recvbuff[bytesRead] = '\0'	;
//
//        
//        fprintf(stderr, "(recvbuff = %s)!!\n", 1, recvbuff);
    }
    
    
//    sleep(100);
/*
    CThread * pthr[MAX_THREADS];
    pMutex = new CMutex();

    for ( int i = 0; i < MAX_THREADS; i++ )
    {
        pthr[i] = new CThread(FindStreamInfo, NULL);
    }

    SDL_Delay(10000);
    for ( int i = 0; i < MAX_THREADS; i++ )
    {
        delete pthr[i];
    }
   
    delete pMutex;
*/    
    return 0; 
}



int epolltest()
{
    int epfd; 
    struct epoll_event *events; 
    struct epoll_event ev; 
 
    struct sockaddr_in srv; 
    int clifd; 
    int listenfd;
    int i; 
    int n; 
    int res; 
    char buffer[MAX_LINE]; 
 
    if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        perror("sockfd\n"); 
        exit(1); 
    } 
 
    events = (epoll_event*)malloc(sizeof(struct epoll_event) * MAX_CLIENT); 
 
    bzero(&srv, sizeof(srv)); 
    srv.sin_family = AF_INET; 
    srv.sin_addr.s_addr = INADDR_ANY; 
    srv.sin_port = htons(PORT); 
 
    if( bind(listenfd, (struct sockaddr *) &srv, sizeof(srv)) < 0) 
    { 
        perror("bind\n"); 
        exit(1); 
    } 
 
    listen(listenfd, 5); 
 
    epfd = epoll_create(MAX_CLIENT); 
    if(!epfd) 
    { 
        perror("epoll_create\n"); 
        exit(1); 
    } 
    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP; 
    ev.data.fd = listenfd; 
    if(epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev) < 0) 
    { 
        perror("epoll_ctl, adding listenfd\n"); 
        exit(1); 
    } 
    for( ; ; ) 
    { 
        res = epoll_wait(epfd, events, MAX_CLIENT, 0); 
        for(i = 0; i < res; i++) 
        { 
            if(events[i].data.fd == listenfd) 
            { 
                struct sockaddr_in clientAddr;
                socklen_t clientAddrLen = sizeof clientAddr;
                
                clifd = accept(listenfd, (struct sockaddr*)&clientAddr, &clientAddrLen); 
                if(clifd > 0) 
                { 
                    nonblock(clifd); 
                    ev.events = EPOLLIN | EPOLLET; 
                    ev.data.fd = clifd; 
                    if(epoll_ctl(epfd, EPOLL_CTL_ADD, clifd, &ev) < 0) 
                    { 
                        perror("epoll_ctl ADD\n"); 
                        exit(1); 
                    } 
#ifdef DEBUG
                    printf("accept()ed connection from [%s]\n", inet_ntoa(clientAddr.sin_addr));
#endif
                } 
            } 
            else { 
                memset(buffer, 0x00, MAX_LINE); 
                n = recv(events[i].data.fd, buffer, MAX_LINE-1, 0); 
                if(n == 0) 
                { 
#ifdef DEBUG  
                    printf("%d closed connection\n", events[i].data.fd); 
                    epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL); 
#endif 
                } 
                
                else if(n < 0) 
                { 
#ifdef DEBUG  
                    printf("%d error occured, errno: %d\n", 
                            events[i].data.fd, errno); 
#endif 
                } 
                else { 
#ifdef DEBUG  
                    printf("%d data received: %s", 
                            events[i].data.fd, buffer); 
                    bzero(&buffer, strlen(buffer)); 
#endif 
                    send(events[i].data.fd, buffer, strlen(buffer), 0); 
                } 
            } 
        } 
    } 

    return  0;   
}




//#define MAX_EVENTS 10
//struct epoll_event ev, events[MAX_EVENTS];
//int listen_sock, conn_sock, nfds, epollfd;
//
///* Set up listening socket, 'listen_sock' (socket(),
//   bind(), listen()) */
//
//epollfd = epoll_create(10);
//if (epollfd == -1) {
//    perror("epoll_create");
//    exit(EXIT_FAILURE);
//}
//
//ev.events = EPOLL_IN;
//ev.data.fd = listen_sock;
//if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) {
//    perror("epoll_ctl: listen_sock");
//    exit(EXIT_FAILURE);
//}
//
//for (;;) {
//    nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
//    if (nfds == -1) {
//        perror("epoll_pwait");
//        exit(EXIT_FAILURE);
//    }
//
//    for (n = 0; n < nfds; ++n) {
//        if (events[n].data.fd == listen_sock) {
//            conn_sock = accept(listen_sock,
//                            (struct sockaddr *) &local, &addrlen);
//            if (conn_sock == -1) {
//                perror("accept");
//                exit(EXIT_FAILURE);
//            }
//            setnonblocking(conn_sock);
//            ev.events = EPOLLIN | EPOLLET;
//            ev.data.fd = conn_sock;
//            if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock,
//                        &ev) == -1) {
//                perror("epoll_ctl: conn_sock);
//                exit(EXIT_FAILURE);
//            }
//        } else {
//            do_use_fd(events[n].data.fd);
//        }
//    }
//}
//

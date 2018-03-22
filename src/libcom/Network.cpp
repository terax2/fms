//--------------------------------------------------------------------------------------------------------------
/** @file		Network.cpp
	@date	2011/07/20
	@author 송봉수(sbs@gabia.com)
	@brief	네트워크 관련.   \n
				서버소켓생성 및 설정 \n
*///------------------------------------------------------------------------------------------------------------



/*  INCLUDE  */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h> 
#include <arpa/inet.h>
#include <errno.h> 
#include "Network.h"
#include <asm/socket.h>


/*  CLASS  */
CNetwork::	CNetwork()
{
	m_fd = 0;
	m_addr = NULL;
	m_port = 0;
	m_IsListen = false;
}

CNetwork::~CNetwork()
{
	if(m_fd !=0)
		close(m_fd);
}

/**********************************************************************************************
* IN : ip주소, 포트, Send/Recv 버퍼크기
* OUT : Success:socket FD, False:-1
* Comment : 소켓생성, 바인딩, 리슨
**********************************************************************************************/
int CNetwork::TCPListen(char *addr, int port, int bufsize)
{
//	printf("addr:%s, port:%d\n", addr, port);
	struct sockaddr_in s_addr;
    if( (m_fd = socket(AF_INET,SOCK_STREAM,0)) < 0 ) 
		return -1;

	// Address Setting
	memset(&s_addr , 0 , sizeof(s_addr)) ; 
	s_addr.sin_family = AF_INET;
	
	if (addr == NULL)
	    s_addr.sin_addr.s_addr = INADDR_ANY;
	else
	    s_addr.sin_addr.s_addr = inet_addr(addr);
	s_addr.sin_port = htons(port);

	// Set Socket Option
	if( SetReuseaddr(m_fd) != 0) return -1;
//	if( SetReuseport(m_fd) != 0) return -1;
	if(	SetTCPnodelay(m_fd) != 0) return -1;
	if(	SetNonblock(m_fd) != 0) return -1;
	if( SetSendBuf(m_fd, bufsize) != 0) return -1;
	if( SetRecvBuf(m_fd, bufsize) != 0) return -1;

//	printf("Set socket Buffer (SendBuf=%d, RecvBuf=%d)\n", GetSendBuf(m_fd), GetRecvBuf(m_fd));
	//bind
	if(bind(m_fd, (struct sockaddr *)&s_addr, sizeof(s_addr)) < 0)
	{
		printf("Socket Bind Error\n");
		return -1;
	}
	
  // Listen
	if( listen(m_fd, SOCK_MAX_LISTEN) == 0)
	{
		m_IsListen = true;
	}else{
		return -1; 
	}

	return m_fd;
}


/**********************************************************************************************
* IN : socket fd
* OUT : Success:0, False:-1
* Comment : TCP_NODELAY
**********************************************************************************************/
int CNetwork::SetTCPnodelay(int sock)
{
	int flag = 1; 
	int result = setsockopt (sock,  IPPROTO_TCP, TCP_NODELAY, (char *) &flag,  sizeof (int));    
	return result; 
}


/**********************************************************************************************
* IN : socket fd
* OUT : Success:0, False:-1
* Comment : SetNonBlock
**********************************************************************************************/
int CNetwork::SetNonblock(int sock)
{
	int flags = fcntl (sock, F_GETFL); 
	flags |= O_NONBLOCK; 

	if (fcntl (sock, F_SETFL, flags) < 0) 
    {
		printf("Socket SetNonBlock Error\n");
		return -1; 
    } 
	return 0; 
}


/**********************************************************************************************
* IN : socket fd
* OUT : Success:0, False:-1
* Comment : SetReuseaddr
**********************************************************************************************/
int CNetwork::SetReuseaddr(int sock)
{
	int flag = 1; 
	int result = setsockopt (sock,  SOL_SOCKET, SO_REUSEADDR, (char *) &flag, sizeof (int)); 
	return result; 
}


/**********************************************************************************************
* IN : socket fd
* OUT : Success:0, False:-1
* Comment : SetReuseport
**********************************************************************************************/
int CNetwork::SetReuseport(int sock)
{
	//int flag = 1; 
	//int result = setsockopt (sock,  SOL_SOCKET, SO_REUSEPORT, (char *) &flag, sizeof (int)); 
	//return result; 
	return 0;
}



/**********************************************************************************************
* IN : socket fd
* OUT : Success:0, False:-1
* Comment : GetSendBuf
**********************************************************************************************/
int CNetwork::GetSendBuf(int sock)
{
	int send_buf;
	socklen_t len;
	len = sizeof(send_buf);

	getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &send_buf, &len);
	return send_buf;
}


/**********************************************************************************************
* IN : socket fd
* OUT : Success:0, False:-1
* Comment : GetRecvBuf
**********************************************************************************************/
int CNetwork::GetRecvBuf(int sock)
{
	int recv_buf;
	socklen_t len;
	len = sizeof(recv_buf);

	getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &recv_buf, &len);
	return recv_buf;
}


/**********************************************************************************************
* IN : socket fd
* OUT : Success:0, False:-1
* Comment : SetSendBuf
**********************************************************************************************/
int CNetwork::SetSendBuf(int sock, int send_bufsize)
{
	int result = setsockopt (sock,  SOL_SOCKET, SO_SNDBUF, &send_bufsize, sizeof(send_bufsize)); 
	return result; 
}


/**********************************************************************************************
* IN : socket fd
* OUT : Success:0, False:-1
* Comment : SetRecvBuf
**********************************************************************************************/
int CNetwork::SetRecvBuf(int sock, int recv_bufsize)
{
	int result = setsockopt (sock,  SOL_SOCKET, SO_RCVBUF, &recv_bufsize, sizeof(recv_bufsize)); 
	return result; 
}






/**********************************************************************************************
* IN : void
* OUT : socket fd
* Comment : return fd
**********************************************************************************************/
int CNetwork::GetFD()
{
	return m_fd;
}


/**********************************************************************************************
* IN : socket, buffer, buffer size
* OUT : send len, Error:-1
* Comment : TCP Send 
**********************************************************************************************/
/*
int TcpSend(int sock, char *szBuf, int iBufLen)
{
	int		n;	
	int		iSendLen = 0;
	
	while(1)
	{
		n = send(sock, szBuf + iSendLen, iBufLen - iSendLen, 0);
		if(n == -1) return -1 ;
	
		iSendLen += n;
		if(iSendLen == iBufLen) break;	
	}
	
	return iBufLen;

}
*/

/**********************************************************************************************
* IN : socket, buffer, buffer size
* OUT : Recv len, Error:-1
* Comment : TCP Recv
**********************************************************************************************/
/*
int TcpRecv(int sock, char *szBuf, int iBufLen)
{
	int	iLen = 0, n;

//	while(iLen < iBufLen)
	while(1)
	{
		n = recv(sock, szBuf + iLen, iBufLen - iLen, 0);
		if(n < 0) 
		{
			 if (EAGAIN == errno )
				 continue; 		
		}
		else
			perror("Socket Recv Error,, Check Connection");

		if(n == 0)
			break;

		iLen += n;
		if(iLen < iBufLen)
			break;

	}

	return iLen;
}
*/



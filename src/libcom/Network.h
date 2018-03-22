//--------------------------------------------------------------------------------------------------------------
/** @file		Network.h
	@date	2011/07/20
	@author �ۺ���(sbs@gabia.com)
	@brief	��Ʈ��ũ ����.   \n
				�������ϻ��� �� ���� \n
*///------------------------------------------------------------------------------------------------------------
#ifndef __HVAAS_NETWORK_H__
#define __HVAAS_NETWORK_H__

/*  DEFINE  */
#define SOCK_MAX_LISTEN		2000   // socket listen(backlog)
#define SOCK_SEND_BUF		65000  // default system send buffer
#define SOCK_RECV_BUF		65000



/*  CLASS DEFINE  */
class CNetwork   // ���ϻ���, ���Ͽɼ�
{
private:
	int			m_fd;
	char		*m_addr;
	int			m_port;
	bool		m_IsListen;

public:
	CNetwork();
	~CNetwork();
	int TCPListen(char *addr, int port, int bufsize);  // create, binding, listen


	int SetTCPnodelay(int sock);
	int SetNonblock(int sock);
	int SetReuseaddr(int sock);
	int SetReuseport(int sock);
	int GetSendBuf(int sock);
	int GetRecvBuf(int sock);
	int SetSendBuf(int sock, int send_bufsize);
	int SetRecvBuf(int sock, int recv_bufsize);
	int GetFD();	
};

/*  FUNC PROTOTYPE  */
//int TcpSend(int sock, char *szBuf, int iBufLen);
//int TcpRecv(int sock, char *szBuf, int iBufLen);

#endif

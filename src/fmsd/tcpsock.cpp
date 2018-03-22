#include "tcpsock.h"
extern CServerLog pLog ;

CTCPSock::CTCPSock(Address pAddr)
:   m_Addr   (pAddr),
    m_Socket (-1   )
{
    m_timeout = _SEND_COUNT_;
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s() Starting ...", __FUNCTION__);
}

CTCPSock::~CTCPSock()
{
    Close();
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s() Destroy  ...", __FUNCTION__);
}

bool CTCPSock::Connect(char * desc, uint8_t isblock)
{
    int err ;
    m_Socket = setupSocket(NULL, 0, isblock);
    do  {
        struct sockaddr_in managerName;
        managerName.sin_family      = AF_INET;
        managerName.sin_port        = htons(m_Addr.port);
        managerName.sin_addr.s_addr = inet_addr(m_Addr.ip);

        if (ConnectWithTimeout(m_Socket, (struct sockaddr*)&managerName, sizeof managerName, 3, &err) != 0)
            break ;

        //pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] Connect Success (%s >> %s:%u)", __FUNCTION__, __LINE__, desc, m_Addr.ip, m_Addr.port);
        return  true  ;
    }   while (false) ;

    close(m_Socket);
    m_Socket = -1  ;
    pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Connection Failed (%s >> %s:%u)", __FUNCTION__, __LINE__, desc, m_Addr.ip, m_Addr.port);
    return false   ;
}

bool CTCPSock::Close(bool Rcv, uint64_t seconds)
{
    if (m_Socket < 0) {
        return true;
    }
    if (Rcv)
        getRecvClear(m_Socket, seconds);
    close(m_Socket);
    m_Socket = -1  ;
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "\x1b[1;33m%s()[%d] Disconnect Success !!! \x1b[0m", __FUNCTION__, __LINE__);
    return true    ;
}

bool CTCPSock::IsConnected()
{
    return (m_Socket > 0 ? true : false);
}

void CTCPSock::SetSvr (char * ip, uint16_t port)
{
    snprintf(m_Addr.ip, TMP_BUF, "%s", ip);
    m_Addr.port = port;
}

void CTCPSock::SetSvr (Address pAddr)
{
    m_Addr = pAddr;
}

void CTCPSock::SetTimeout(uint64_t timeout)
{
    m_timeout = timeout;
}

int CTCPSock::Send (char * pbuff, int plen)
{
    if (!IsConnected()) return 0;
    int sendbytes = SendSocketExact (m_Socket, pbuff, plen, m_timeout);
    if (sendbytes != plen)
    {
    	Close();
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] send message error (%d)(%s) !!", __FUNCTION__, __LINE__, errno, strerror(errno));
        return sendbytes;
    }
    return sendbytes;
}


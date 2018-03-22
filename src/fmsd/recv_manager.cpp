#include "recv_manager.h"
#include "fmsd_config.h"

extern CServerLog   pLog ;
extern CConfig      pcfg ;

CRecvManager::CRecvManager(CSendMsg ** sndmsg)
:   m_SndMsg (sndmsg),
    m_DevLst (NULL  ),
    m_SeqNo  (0     )
{
    m_DevLst = new CDataMap();
    m_DevLst->setfree(freeData);
    pLog.Print(LOG_WARNING, 0, NULL, 0, "%s() Starting ...", __FUNCTION__) ;
}

CRecvManager::~CRecvManager()
{
    if (m_DevLst)
    {
        delete m_DevLst;
        m_DevLst = NULL;
    }
    pLog.Print(LOG_WARNING, 0, NULL, 0, "%s() Destroy ...", __FUNCTION__);
}

void CRecvManager::freeData (void * f)
{
    if (f)
    {
        CRecvSMP * dev = (CRecvSMP*)f;
        dev->Stop();
        dev->StopThread();
        delete dev;
        dev = NULL;
    }
}

void CRecvManager::ConnectDevice(uint32_t seqno, ClntInfo sock)
{
    string ips = inet_ntoa(sock.clientaddr.sin_addr);
    CRecvSMP * Dev = (CRecvSMP*)m_DevLst->getdata(ips);
    if (Dev == NULL)
    {
        Dev = new CRecvSMP (m_SndMsg);
        Dev->StartThread();
        m_DevLst->insert(ips, Dev);
    }
    
    Dev->setSeqNo(seqno);
    Dev->setUpdate(sock);
    Dev->Start();
}

void CRecvManager::CommandDevice(ClntInfo sock)
{
//    CRecvSMP * Dev = (CRecvSMP*)m_DevLst->getdata(mac);
//    if (Dev)
//    {
//    }
}

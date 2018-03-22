#include "snmp_manager.h"
#include "fmsd_config.h"

extern CServerLog   pLog ;
extern CConfig      pcfg ;

CSnmpManager::CSnmpManager(CSendMsg ** sndmsg)
:   m_SndMsg (sndmsg),
    m_DevLst (NULL  )
{
    m_DevLst = new CDataMap();
    m_DevLst->setfree(freeData);
    pLog.Print(LOG_WARNING, 0, NULL, 0, "%s() Starting ...", __FUNCTION__) ;
}

CSnmpManager::~CSnmpManager()
{
    if (m_DevLst)
    {
        delete m_DevLst;
        m_DevLst = NULL;
    }
    pLog.Print(LOG_WARNING, 0, NULL, 0, "%s() Destroy ...", __FUNCTION__);
}

void CSnmpManager::freeData (void * f)
{
    if (f)
    {
        CDEVReceiver * dev = (CDEVReceiver*)f;
        dev->Stop();
        dev->StopThread();
        delete dev;
        dev = NULL;
    }
}

void CSnmpManager::SessionAlloc(uint32_t seqno, ClntInfo sock, string mac)
{
//    CDEVReceiver * Dev = (CDEVReceiver *)m_DevLst->getdata(mac);
//    if (Dev == NULL)
//    {
//        Dev = new CDEVReceiver (m_SndMsg);
//        Dev->StartThread();
//        m_DevLst->insert(mac, Dev);
//    }
//    
//    Dev->setSeqNo(seqno);
//    Dev->setUpdate(sock);
//    Dev->Start();
}

void CSnmpManager::CommandDevice(ClntInfo sock, string mac)
{
//    CDEVReceiver * Dev = (CDEVReceiver *)m_DevLst->getdata(mac);
//    if (Dev)
//    {
//    }
}

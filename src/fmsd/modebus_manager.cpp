#include "modebus_manager.h"
#include "fmsd_config.h"

extern CServerLog   pLog ;
extern CConfig      pcfg ;

CModebusManager::CModebusManager(CSendMsg ** sndmsg)
:   m_SndMsg (sndmsg),
    m_DevLst (NULL  )
{
    m_DevLst = new CDataMap();
    m_DevLst->setfree(freeData);
    pLog.Print(LOG_WARNING, 0, NULL, 0, "%s() Starting ...", __FUNCTION__) ;
}

CModebusManager::~CModebusManager()
{
    if (m_DevLst)
    {
        delete m_DevLst;
        m_DevLst = NULL;
    }
    pLog.Print(LOG_WARNING, 0, NULL, 0, "%s() Destroy ...", __FUNCTION__);
}

void CModebusManager::freeData (void * f)
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

void CModebusManager::SessionAlloc(uint32_t seqno, ClntInfo sock, string mac)
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

void CModebusManager::CommandDevice(ClntInfo sock, string mac)
{
//    CDEVReceiver * Dev = (CDEVReceiver *)m_DevLst->getdata(mac);
//    if (Dev)
//    {
//    }
}

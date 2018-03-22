#include "recv_smp.h"
#include "fmsd_config.h"

extern  CServerLog     pLog    ;
extern  CConfig        pcfg    ;

static inline const int dev_read (int socket, char * buff, int size)
{
    int8_t  readByte = 0;
    int16_t totBytes = 0;
    memset(buff, 0, size);
    while(true)
    {
        readByte = getData(socket, buff + totBytes, 1, _RECV_TIME_);
        if (readByte <= 0)
        {
            if (readByte == -1)
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] (%s)", __FUNCTION__, __LINE__, getError(readByte, _RECV_TIME_));
            else
                pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] (%s)", __FUNCTION__, __LINE__, getError(readByte, _RECV_TIME_));
            return -1;
        }
        if (buff[totBytes] == _DELIMETER_)
            return ++totBytes;
        totBytes += readByte;
    }
}

CRecvSMP::CRecvSMP(CSendMsg ** sndmsg)
:   m_Source (false ),
    m_uMsg   (NULL  ),
    //m_Buff   (NULL  ),
    //m_Size   (0     ),
    m_Thread (NULL  ),
    m_SndMsg (sndmsg),
    //m_NodeJS (NULL  ),
    m_seqno  (0     )
{
    m_uMsg = new UMSG;
    memset(m_uMsg, 0, SZ_UMSG);

    //m_Buff = new char[SMI_BUF];
    //memset(m_Buff, 0, SMI_BUF);
    //memset(&m_sock, 0, sizeof(m_sock));

    m_MQList.set_maxqueue(_RECV_QUEUE_);
    m_Thread = SDL_CreateThread(DoQMList, this);

    //m_NodeJS = new CTCPSock();
    //Address nAddr = pcfg.getNodeJS();
    //m_NodeJS->SetSvr(nAddr.ip, nAddr.port);

    //for (int i = 0 ; i < pcfg.getPoolCount(); i++)
    //    m_SndMsg[i]->setResp(RespFunc);

    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s() Starting ...", __FUNCTION__);
}

CRecvSMP::~CRecvSMP()
{
    if (m_uMsg)
    {
        delete m_uMsg;
        m_uMsg = NULL;
    }
    //if (m_Buff)
    //{
    //    delete[] m_Buff;
    //    m_Buff = NULL;
    //}

    if (m_Thread)
    {
        m_MQList.send_message(_CLOSE_, NULL, 0);

        SDL_WaitThread(m_Thread, NULL);
        m_Thread = NULL;

        CLst * pLst = NULL ;
        while ((pLst = m_MQList.get_message()) != NULL) {
            delete pLst;
        }
    }

    //if (m_NodeJS)
    //{
    //    delete m_NodeJS;
    //    m_NodeJS = NULL;
    //}
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s() Destroy  ...", __FUNCTION__);
}

int CRecvSMP::ThreadMain()
{
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] Thread Start...", __FUNCTION__, __LINE__);
    while (true)
    {
        int rc = SDL_SemTryWait(m_myMsgQueueSemaphore);
        if (!m_Source) SDL_Delay(300);

        // semaphore error
        if (rc == -1) {
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "%s()[%d] \x1b[1;31mSDL_SemTryWait : WAIT_ERROR\x1b[0m", __FUNCTION__, __LINE__);
            return 0;
        }

        // message pending
        if (rc == 0) {
            CMsg * pMsg = m_myMsgQueue.get_message();
            if (pMsg != NULL) {
                switch (pMsg->get_value()) {
                case MSG_NODE_STOP_THREAD:
                    DoStopCapture() ;    // ensure things get cleaned up
                    delete pMsg;
                    return 0;
                case MSG_NODE_START:
                    DoStartCapture();
                    break;
                case MSG_NODE_STOP:
                    DoStopCapture();
                    break;
                }
                delete pMsg;
            }
        }

        if (m_Source)
        {
            try
            {
                //if ((m_Size = dev_read(m_sock.fd, m_Buff, m_Size)) > 0)
                if (dev_read(m_uMsg->sock.fd, m_uMsg->msg, TMP_BUF) > 0)
                {
                    m_MQList.send_message(1, m_uMsg, SZ_UMSG);
                    //m_MQList.send_message(1, m_Buff, m_Size);
                    //m_RcvCnt ++;
                }   else
                {
                    //if (m_RcvCnt > 0)
                    //{
                    //    pLog.Print(LOG_INFO, 0, NULL, 0, "%s()[%d] (%s) Recv Count = [%6u]", __FUNCTION__, __LINE__, inet_ntoa(m_sock.clientaddr.sin_addr), m_RcvCnt);
                    //    m_RcvCnt = 0;
                    //}
                    DoStopCapture();
                    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] (%s) Session Closed", __FUNCTION__, __LINE__, inet_ntoa(m_uMsg->sock.clientaddr.sin_addr));
                }
            }
            catch (...)
            {
                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "%s()[%d] exception error [%s]", __FUNCTION__, __LINE__, strerror(errno));
                DoStopCapture();
            }
        }
    }
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] Thread End ...", __FUNCTION__, __LINE__);
    return 0;
}

//void CRecvSMP::RespFunc (void * param)
//{
//    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] Response Calling ...", __FUNCTION__, __LINE__);
//    if (param)
//    {
//    }
//}

void CRecvSMP::DeviceRequest(UMSG * uMsg)
{
    uint8_t seqno = (m_seqno % pcfg.getPoolCount());
    
    m_SndMsg[seqno]->SendDB((char*)"", 0);
}

int CRecvSMP::DoQMList (void * param)
{
    /////////////////////////////////////////////
    CRecvSMP * self  = (CRecvSMP*)param ;
    CLst         * pLst  = NULL                 ;
    UMSG         * pMsg  = NULL                 ;
    uint32_t       size  = 0                    ;
    /////////////////////////////////////////////

    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s() Starting ...", __FUNCTION__);
    while (SDL_SemWait(self->m_MQList.get_semaphore()) == 0)
    {
        while ((pLst = self->m_MQList.get_message()) != NULL)
        {
            if (!pLst->get_type())
            {
                delete pLst ;
                pLst = NULL ;
                return 0    ;
            }

            pMsg = (UMSG*)pLst->get_message (size);
            self->DeviceRequest(pMsg);

            delete pLst  ;
            pLst = NULL  ;
        }
    }
    return 0;
}


void CRecvSMP::setUpdate(ClntInfo sock)
{
    while(m_Source) SDL_Delay(10);
    epCloseN(&m_uMsg->sock, true);
    m_uMsg->sock = sock ;
#ifdef BLOCKING
    setRcvTimeo(m_uMsg->sock.fd, (_RECV_TIME_/_TIME2SLEEP_));
#endif

//    while(m_Source) SDL_Delay(10);
//    epCloseN(&m_sock, true);
//    m_sock = sock ;
//#ifdef BLOCKING
//    setRcvTimeo(m_sock.fd, (_RECV_TIME_/_TIME2SLEEP_));
//#endif
}


void CRecvSMP::DoStartCapture()
{
    if (m_Source) {
        return;
    }
    m_Source = true;
}

void CRecvSMP::DoStopCapture()
{
    if (!m_Source) {
        return;
    }
    epCloseN(&m_uMsg->sock, true);
    m_Source = false;
}

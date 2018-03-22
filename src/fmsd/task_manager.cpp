#include "task_manager.h"
#include "fmsd_config.h"

extern CServerLog   pLog    ;
extern CConfig      pcfg    ;
extern bool         IsALive ;

CLstQueue           EVQList ;
CLstQueue           SMSList ;
CLstQueue           DEVInit ;
Mutex               DLock   ;   // DB Lock

CTaskManager::CTaskManager ()
:   m_SndMsg      (NULL),
    m_fPollManager(NULL),
    m_EVThread    (NULL),
    m_SMShread    (NULL),
    m_DEVInit     (NULL),
    m_SeqNo       (0   )
{
    Initialize();
    pLog.Print(LOG_WARNING, 0, NULL, 0, "%s() Starting ...", __FUNCTION__);
}

CTaskManager::~CTaskManager (void)
{
    StopTaskManager() ;
    pLog.Print(LOG_WARNING, 0, NULL, 0, "%s() Destroy ...", __FUNCTION__);
}

void CTaskManager::Initialize ()
{
    IsALive = StartTaskManager();
    if (!IsALive) exit(0);
}

void CTaskManager::StopTaskManager ()
{
    IsALive = false;
    if (m_SndMsg)
    {
        for (int i = 0; i < pcfg.getPoolCount(); i++)
        {
            if (m_SndMsg[i])
            {
                delete m_SndMsg[i];
                m_SndMsg[i] = NULL;
            }
        }
        delete[] m_SndMsg;
        m_SndMsg = NULL ;
    }

    if (m_fPollManager)
    {
        delete m_fPollManager;
        m_fPollManager = NULL;
    }

    if (m_EVThread)
    {
        EVQList.send_message(_CLOSE_, NULL, 0);

        SDL_WaitThread(m_EVThread, NULL);
        m_EVThread = NULL;

        CLst * pLst = NULL ;
        while ((pLst = EVQList.get_message()) != NULL) {
            delete pLst;
        }
    }
    if (m_SMShread)
    {
        SMSList.send_message(_CLOSE_, NULL, 0);

        SDL_WaitThread(m_SMShread, NULL);
        m_SMShread = NULL;

        CLst * pLst = NULL ;
        while ((pLst = SMSList.get_message()) != NULL) {
            delete pLst;
        }
    }
    if (m_DEVInit)
    {
        DEVInit.send_message(_CLOSE_, NULL, 0);

        SDL_WaitThread(m_DEVInit, NULL);
        m_DEVInit = NULL;

        CLst * pLst = NULL ;
        while ((pLst = DEVInit.get_message()) != NULL) {
            delete pLst;
        }
    }
    atexit(SDL_Quit);
}

bool CTaskManager::StartTaskManager ()
{
    int ret = SDL_Init (SDL_INIT_TIMER);
    if ( ret != 0 )
    {
        pLog.Print (LOG_CRITICAL, 0, NULL, 0, "%s() SDL_Init() Error !!", __FUNCTION__);
        return false;
    }

    m_SndMsg = new CSendMsg*[pcfg.getPoolCount()];
    for (int i = 0; i < pcfg.getPoolCount(); i++)
        m_SndMsg[i] = new CSendMsg();

    EVQList.set_maxqueue(_EVENT_QUEUE_);
    SMSList.set_maxqueue(_EVENT_QUEUE_);
    DEVInit.set_maxqueue(_EVENT_QUEUE_);
    m_EVThread = SDL_CreateThread(DoEVQList, NULL);
    m_SMShread = SDL_CreateThread(DoSMSList, NULL);
    m_DEVInit  = SDL_CreateThread(DoDEVInit, NULL);

    m_fPollManager = new CPollingManager(m_SndMsg);
    SDL_CreateThread(DEV_Receiver , this);
    SDL_CreateThread(NJS_Receiver , this);
    //SDL_CreateThread(LORA_Receiver, this);

    pLog.Print(LOG_WARNING, 0, NULL, 0, "%s() Starting ...", __FUNCTION__);
    return true ;
}

int CTaskManager::DEV_Receiver (void * param)
{
    if ( param == NULL )    return -1 ;
    CTaskManager * self  = (CTaskManager *)param ;
    if ( self  == NULL )    return -100 ;

    pLog.Print(LOG_INFO, 0, NULL, 0, "%s() Starting ...", __FUNCTION__);

    CNetwork  Socket ;
    int sLocal_fd = 0;
    while (true)
    {
        sLocal_fd = Socket.TCPListen(NULL, pcfg.getDevicePort(), (pcfg.getSockBuff()*100));
        if (sLocal_fd < 0)
        {
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "%s()[%d] Create Device Controller Listen Socket Error", __FUNCTION__, __LINE__);
            sleep(1);
        }   else
            break;
    }

    /*  Start Epoll  */
    CEpoll    Ep_Camera;  // Epoll OneShot Create ...
    if( Ep_Camera.EpStart(sLocal_fd, pcfg.getEpMaxCon(), pcfg.getEpEvent(), 1) < 0 )
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "%s()[%d] Create Device Controller Listen epoll Error", __FUNCTION__, __LINE__);
        exit(0);
    }

#ifdef BLOCKING
    Ep_Camera.SetSockBlockMode (false);
#endif
    Ep_Camera.SetSockTCPNoDelay(false);

    /*  Epoll Waiting */
    list<ClntInfo>  list_eventFD;
    list<ClntInfo>::iterator iter;

    while (IsALive)
    {
        list_eventFD.clear();
        list_eventFD = (*Ep_Camera.EpWait_getaddr_ex(1000));   // 발생한 Epoll 이벤트의 *list 를받아옴
        if (list_eventFD.empty()) continue;

        // 각 이벤트에 대해 쓰레드 생성
        for (iter = list_eventFD.begin(); iter != list_eventFD.end(); iter++)
        {
            increaseRecvBufferTo(iter->fd, pcfg.getSockBuff());
            if (self->m_SeqNo == 0) self->m_SeqNo = 100;
            self->m_fPollManager->ConnectDevice (self->m_SeqNo++, *iter);
        }
    }
    Delete_fd  (Ep_Camera.Get_epFD(), sLocal_fd);
    clearSocket(Ep_Camera.Get_epFD()) ;
    clearSocket(sLocal_fd)            ;

    pLog.Print(LOG_WARNING, 0, NULL, 0, "%s() Ending ...", __FUNCTION__);
    return 0 ;
}

int CTaskManager::NJS_Receiver (void * param)
{
    if ( param == NULL )    return -1 ;
    CTaskManager * self  = (CTaskManager *)param ;
    if ( self  == NULL )    return -100 ;

    pLog.Print(LOG_INFO, 0, NULL, 0, "%s() Starting ...", __FUNCTION__);

    CNetwork  Socket ;
    int sLocal_fd = 0;
    while (true)
    {
        sLocal_fd = Socket.TCPListen(NULL, pcfg.getCommandPort(), (pcfg.getSockBuff()*100));
        if (sLocal_fd < 0)
        {
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "%s()[%d] Create NodeJs Listen Socket Error", __FUNCTION__, __LINE__);
            sleep(1);
        }   else
            break;
    }

    /*  Start Epoll  */
    CEpoll    Ep_Camera;  // Epoll OneShot Create ...
    if( Ep_Camera.EpStart(sLocal_fd, pcfg.getEpMaxCon(), pcfg.getEpEvent(), 1) < 0 )
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "%s()[%d] Create NodeJs Listen epoll Error", __FUNCTION__, __LINE__);
        exit(0) ;
    }
//#ifdef BLOCKING
//    Ep_Camera.SetSockBlockMode (false);
//#endif
    Ep_Camera.SetSockTCPNoDelay(false);

    /*  Epoll Waiting */
    list<ClntInfo>  list_eventFD;
    list<ClntInfo>::iterator iter;

    while (IsALive)
    {
        list_eventFD.clear();
        list_eventFD = (*Ep_Camera.EpWait_getaddr_ex(1000));   // 발생한 Epoll 이벤트의 *list 를받아옴
        if (list_eventFD.empty()) continue;

        // 각 이벤트에 대해 쓰레드 생성
        for (iter = list_eventFD.begin(); iter != list_eventFD.end(); iter++)
        {
            increaseRecvBufferTo(iter->fd, pcfg.getSockBuff());
            self->m_fPollManager->CommandDevice (*iter);
        }
    }
    Delete_fd  (Ep_Camera.Get_epFD(), sLocal_fd);
    clearSocket(Ep_Camera.Get_epFD()) ;
    clearSocket(sLocal_fd)            ;

    pLog.Print(LOG_WARNING, 0, NULL, 0, "%s() Ending ...", __FUNCTION__);
    return 0 ;
}

//int CTaskManager::LORA_Receiver(void * param)
//{
//    if ( param == NULL )    return -1 ;
//    CTaskManager * self  = (CTaskManager *)param ;
//    if ( self  == NULL )    return -100 ;
//
//    char                lBuf    [MID_BUF]       ;
//    struct sockaddr_in  svrAddr                 ;
//    struct sockaddr_in  cliAddr                 ;
//    socklen_t addrsize = sizeof cliAddr         ;
//
//    memset(&svrAddr, 0, sizeof(svrAddr))        ;
//    svrAddr.sin_family      = AF_INET           ;
//    svrAddr.sin_addr.s_addr = INADDR_ANY        ;
//    svrAddr.sin_port        = htons(pcfg.getLoraPort()) ;
//
//    int sock = setupUDPSocket() ;
//    if(sock == -1)  {
//        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "%s() socket failed !!!", __FUNCTION__);
//        return 0;
//    }
//
//    if (bind(sock, (struct sockaddr*)&svrAddr, sizeof svrAddr) != 0) {
//        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "%s() socket bind (port = %d) !!!", __FUNCTION__, pcfg.getLoraPort());
//        close(sock) ;
//        return 0;
//    }
//
//    pLog.Print(LOG_INFO, 0, NULL, 0, "%s() Starting ...", __FUNCTION__);
//    pLog.Print(LOG_INFO, 0, NULL, 0, "set Recv Buff = (%llu)", increaseRecvBufferTo(sock, 40485760));
//
//    while (IsALive)
//    {
//        int len = recvfrom(sock, lBuf, MID_BUF, 0, (struct sockaddr*)&cliAddr, &addrsize);
//        if (len <= 0)
//        {
//            if (errno) pLog.Print(LOG_WARNING, 0, NULL, 0, "%s() [%d]'%s'", __FUNCTION__, errno, strerror(errno));
//            continue   ;
//        }
//        lBuf[len++] = '\0';
//        self->m_fPollManager->LoraMessage(lBuf, len);
//    }
//
//    clearSocket(sock);
//    pLog.Print(LOG_WARNING, 0, NULL, 0, "%s() Ending ...", __FUNCTION__);
//    return 0 ;
//}


int CTaskManager::DoDEVInit (void * param)
{
    /////////////////////////////////////
    CLst        * pLst   = NULL         ;
    DEVINFO     * pData  = NULL         ;
    uint32_t      size   = 0            ;
    CDBInterface  DBInf(pcfg.getMyDB()) ;
    uint32_t      TCount = 0            ;
    /////////////////////////////////////

    pLog.Print(LOG_INFO, 0, NULL, 0, "%s() Starting ...", __FUNCTION__);
    while (!DBInf.DBConnect())
        sleep(3);
    DBInf.setAutoCommit(0);

    while (SDL_SemWait(DEVInit.get_semaphore()) == 0)
    {
        if (DEVInit.get_QCount() <= 0) continue;
        TCount = 0;
        DBInf.BeginTran();
        while ((pLst = DEVInit.get_message()) != NULL)
        {
            if (!pLst->get_type())
            {
                DBInf.DBClose() ;
                delete pLst     ;
                pLst = NULL     ;
                return 0        ;
            }

            pData = (DEVINFO*)pLst->get_message (size);
            if (DBInf.uptDevOpInfoT(pData) != _ISOK_)
            {
                DBInf.EndTran();
                DBInf.DBClose();
                SDL_Delay(300) ;
                while (!DBInf.DBConnect())
                    sleep(1);
                DBInf.setAutoCommit(0);
                DBInf.BeginTran();
                TCount = 0;
            }
            delete pLst ;
            pLst = NULL ;

            if (++TCount % 10 == 0)
            {
                DBInf.EndTran();
                DBInf.BeginTran();
            }
        }
        DBInf.EndTran();
    }
    DBInf.DBClose();
    return 0;
}

int CTaskManager::DoEVQList (void * param)
{
    /////////////////////////////////////
    CLst        * pLst   = NULL         ;
    EVENTS      * pEvent = NULL         ;
    uint32_t      size   = 0            ;
    bool          ret                   ;
    KBSMS         kbSMS                 ;
    CDBInterface  DBInf(pcfg.getMyDB()) ;
    /////////////////////////////////////
    while (!DBInf.DBConnect())
        sleep(3);
    DBInf.setAutoCommit(0);

    pLog.Print(LOG_INFO, 0, NULL, 0, "%s() Starting ...", __FUNCTION__);
    while (SDL_SemWait(EVQList.get_semaphore()) == 0)
    {
        if (EVQList.get_QCount() <=0) continue;
        if (!DBInf.IsConnected())
        {
            DBInf.DBClose();
            SDL_Delay(300) ;
            while (!DBInf.DBConnect())
                sleep(1);
            DBInf.setAutoCommit(0);
        }

        while ((pLst = EVQList.get_message()) != NULL)
        {
            if (!pLst->get_type())
            {
                DBInf.DBClose() ;
                delete pLst     ;
                pLst = NULL     ;
                return 0        ;
            }

            if (pLst->get_type() == _DEV_CUR_CLEAR_)
            {
                DBInf.uptCurClear();
            }   else
            {
                pEvent = (EVENTS*)pLst->get_message (size);
                while(true)
                {
                    ret = DBInf.getEvtMsg(pEvent);
                    if (ret)
                    {
                        if (pLst->get_type() == _EVENT_OCCURED_)
                            ret = DBInf.evtDevOccur(pEvent);
                        else
                        if (pLst->get_type() == _EVENT_CLEARED_)
                            ret = DBInf.evtDevClear(pEvent);
                        else
                            break;
                    }
                    
                    if (ret == false)
                    {
                        DBInf.DBClose();
                        SDL_Delay(300) ;
                        while (!DBInf.DBConnect())
                            sleep(1);
                        DBInf.setAutoCommit(0);
                    }   else
                    {
                        if (pEvent->smsflag || (pEvent->evtno >= 200 && pEvent->level != 4))
                        {
                            memset(&kbSMS, 0, SZ_KBSMS);
                            memcpy(kbSMS.keyword   , "PEM-UPS", 7);
                            memcpy(kbSMS.BizGugun  , "44"     , 2);
                            snprintf(kbSMS.date    , sizeof(kbSMS.date)    , "%s", convdate(pEvent->starttime));
                            snprintf(kbSMS.severity, sizeof(kbSMS.severity), "%u", pEvent->itsm_level);
                            memcpy(kbSMS.AppCode   , "SAE"    , 3);
                            snprintf(kbSMS.bCode   , sizeof(kbSMS.bCode)   , "%s", pEvent->bcode);
                            
                            // UTF-8 Message
                            //snprintf(kbSMS.bName   , sizeof(kbSMS.bName)   , "%s", pEvent->bname);
                            //DBInf.getEvtNumStr (kbSMS.NumStr, kbSMS.bCode);
                            //snprintf(kbSMS.message , sizeof(kbSMS.message) , "%s", pEvent->message);
    
                            // to Euckr Message
                            Utf8ToEuckr((char*)pEvent->bname  , strlen((char*)pEvent->bname)  , (char*)kbSMS.bName  , sizeof(kbSMS.bName)  );
                            DBInf.getEvtNumStr (kbSMS.Machine, kbSMS.bCode);
                            Utf8ToEuckr((char*)pEvent->message, strlen((char*)pEvent->message), (char*)kbSMS.message, sizeof(kbSMS.message));
    
                            SMSList.send_message(1, &kbSMS, SZ_KBSMS);
                        }
                        break;
                    }
                }
            }

            delete pLst  ;
            pLst = NULL  ;
        }
    }
    DBInf.DBClose();
    return 0;
}

int CTaskManager::DoSMSList (void * param)
{
    /////////////////////////////////////
    CLst     * pLst   = NULL            ;
    KBSMS    * pSms   = NULL            ;
    uint32_t   size   = 0               ;
    CTCPSock   kbSms (pcfg.getSMSInf()) ;
    char     * UpsMsg = new char[BIG_BUF];
    /////////////////////////////////////

    pLog.Print(LOG_INFO, 0, NULL, 0, "%s() Starting ...", __FUNCTION__);
    while (SDL_SemWait(SMSList.get_semaphore()) == 0)
    {
        if (SMSList.get_QCount() <=0) continue;
        while ((pLst = SMSList.get_message()) != NULL)
        {
            if (!pLst->get_type())
            {
                if (UpsMsg)
                {
                    delete[] UpsMsg;
                    UpsMsg = NULL  ;
                }
                kbSms.Close();
                delete pLst ;
                pLst = NULL ;
                return 0    ;
            }

            pSms = (KBSMS*)pLst->get_message (size);
            //  Make Alarm Message ...
            memset(UpsMsg, 0, BIG_BUF);
            snprintf(UpsMsg, BIG_BUF, "%s %s %s %s %s %s %s %s %s", pSms->keyword, pSms->BizGugun, 
                                                                    pSms->date   , pSms->severity,
                                                                    pSms->AppCode, pSms->bCode   ,
                                                                    pSms->bName  , pSms->Machine ,
                                                                    pSms->message );
            int16_t ulen = strlen(UpsMsg);

            while (!kbSms.Connect((char*)"kbSMS Send"))
                sleep(1);
            kbSms.Send(UpsMsg, ulen);
            kbSms.Close();
            SDL_Delay(50);

            delete pLst  ;
            pLst = NULL  ;
        }
    }
    if (UpsMsg)
    {
        delete[] UpsMsg;
        UpsMsg = NULL  ;
    }
    return 0;
}


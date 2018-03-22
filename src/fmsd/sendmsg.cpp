#include "sendmsg.h"
#include "fmsd_config.h"

extern  CServerLog  pLog;
extern  CConfig     pcfg;

CSendMsg::CSendMsg()
:   m_DBThread(NULL),
    m_NJThread(NULL)
    //m_resp_f(NULL)
{
    m_DBQList.set_maxqueue(_SQL_SNDQUE_);
    m_NJQList.set_maxqueue(_NJS_SNDQUE_);
    m_DBThread = SDL_CreateThread(DoDBQList, &m_DBQList);
    m_NJThread = SDL_CreateThread(DoNJQList, &m_NJQList);
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s() Starting ...", __FUNCTION__);
}

CSendMsg::~CSendMsg()
{
    if (m_DBThread)
    {
        m_DBQList.send_message(_CLOSE_, NULL, 0);

        SDL_WaitThread(m_DBThread, NULL);
        m_DBThread = NULL;

        CLst * pLst = NULL ;
        while ((pLst = m_DBQList.get_message()) != NULL) {
            delete pLst;
        }
    }
    if (m_NJThread)
    {
        m_NJQList.send_message(_CLOSE_, NULL, 0);

        SDL_WaitThread(m_NJThread, NULL);
        m_NJThread = NULL;

        CLst * pLst = NULL ;
        while ((pLst = m_NJQList.get_message()) != NULL) {
            delete pLst;
        }
    }
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s() Destroy  ...", __FUNCTION__);
}

void CSendMsg::SendDB (char * pbuff, int plen, int uid)
{
    m_DBQList.send_message(uid, pbuff, plen);
}

void CSendMsg::SendNJ (char * pbuff, int plen, int uid)
{
    m_NJQList.send_message(uid, pbuff, plen);
}

int CSendMsg::DoDBQList (void * param)
{
    /////////////////////////////////////
    CLstQueue* pQue   = (CLstQueue*)param;
    CLst     * pLst   = NULL            ;
    GETDATA  * pData  = NULL            ;
    uint32_t   size   = 0               ;
    CDBInterface  DBInf(pcfg.getMyDB()) ;
    /////////////////////////////////////

    pLog.Print(LOG_INFO, 0, NULL, 0, "%s() Starting ...", __FUNCTION__);
    while (!DBInf.DBConnect())
        sleep(3);
    DBInf.setAutoCommit(0);

    while (SDL_SemWait(pQue->get_semaphore()) == 0)
    {
        DBInf.BeginTran();
        while ((pLst = pQue->get_message()) != NULL)
        {
            if (!pLst->get_type())
            {
                DBInf.DBClose() ;
                delete pLst     ;
                pLst = NULL     ;
                return 0        ;
            }

            pData  = (GETDATA*)pLst->get_message (size);
            if (!DBInf.uptDevDataT(pData))
            {
                DBInf.EndTran();
                DBInf.DBClose();
                SDL_Delay(300) ;
                while (!DBInf.DBConnect())
                    sleep(1);
                DBInf.setAutoCommit(0);
                DBInf.BeginTran();
            }

            delete pLst  ;
            pLst = NULL  ;
        }
        DBInf.EndTran();
    }
    DBInf.DBClose();
    return 0;
}

int CSendMsg::DoNJQList (void * param)
{
    /////////////////////////////////////
    //CSendMsg * self   = (CSendMsg*)param;
    CLstQueue* pQue   = (CLstQueue*)param;
    CLst     * pLst   = NULL            ;
    NJSMSG   * pNjs   = NULL            ;
    uint32_t   size   = 0               ;
    UMSG       pMsg                     ;
    CTCPSock   NodeJS(pcfg.getNodeJS()) ;
    /////////////////////////////////////

    pLog.Print(LOG_INFO, 0, NULL, 0, "%s() Starting ...", __FUNCTION__);
    NodeJS.SetTimeout(_NJS_SND_CNT_);
    while (!NodeJS.Connect((char*)"NodeJS"))
        sleep(3);

    while (SDL_SemWait(pQue->get_semaphore()) == 0)
    {
        while ((pLst = pQue->get_message()) != NULL)
        {
            if (!pLst->get_type())
            {
                NodeJS.Close();
                delete pLst ;
                pLst = NULL ;
                return 0    ;
            }

            pNjs = (NJSMSG*)pLst->get_message (size);
            uint16_t tlen = MakePacket(&pMsg, NJS_FCODE_GET_DATA, pNjs, SZ_NJSMSG);
            if (NodeJS.Send(pMsg.msg, tlen) <= 0)
            {
                NodeJS.Close() ;
                SDL_Delay(300) ;
                while (!NodeJS.Connect((char*)"NodeJS"))
                    sleep(1);
            }

            delete pLst  ;
            pLst = NULL  ;
        }
    }
    NodeJS.Close();
    return 0;
}


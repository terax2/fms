#include "polling_smp.h"
#include "fmsd_config.h"

extern  CServerLog  pLog ;
extern  CConfig     pcfg ;
extern  CLstQueue   EVQList;


CPollingSMP::CPollingSMP(CSendMsg ** sndmsg)
:   m_Keep   (false ), m_Init   (true  ),
    m_Source (false ), m_IsLora (true  ),
    m_TryTo  (false ), m_NetStop(false ),
    m_Thread (NULL  ), m_Lhread (NULL  ),
    m_SndMsg (sndmsg), //m_DBInf  (NULL  ),
    m_Devdef (NULL  ), m_GetData(NULL  ),
    m_Events (NULL  ), m_Devsock(NULL  ),
    m_seqno  (0     ), m_FFlag  (0     ) //, m_polling(0     )
{
    m_Thread  = SDL_CreateThread(DoMQList, this);
    m_Lhread  = SDL_CreateThread(DoLQList, this);
    //m_DBInf   = new CDBInterface(pcfg.getMyDB());
    m_Devdef  = new DEVDEF  ;
    m_GetData = new GETDATA ;
    memset(m_GetData, 0, SZ_GETDATA);
    m_Events  = new EVENTS  ;
    m_Devsock = new DEVSOCK ;

    m_Devdef->evtLimit.setfree(freeLimit);
    memset(&m_dev , 0, sizeof(TCHECK));
    memset(&m_keep, 0, sizeof(TCHECK));
    memset(&m_dbs , 0, sizeof(TCHECK));
    memset(&m_njs , 0, sizeof(TCHECK));
    memset(&m_Lora, 0, sizeof(TCHECK));
    memset(&m_Status, 0, sizeof(OSTAT));
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s() Starting ...", __FUNCTION__);
}

CPollingSMP::~CPollingSMP()
{
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
    if (m_Lhread)
    {
        m_LQList.send_message(_CLOSE_, NULL, 0);

        SDL_WaitThread(m_Lhread, NULL);
        m_Lhread = NULL;

        CLst * pLst = NULL ;
        while ((pLst = m_LQList.get_message()) != NULL) {
            delete pLst;
        }
    }

    //if (m_DBInf)
    //{
    //    delete m_DBInf;
    //    m_DBInf = NULL;
    //}
    if (m_GetData)
    {
        delete m_GetData;
        m_GetData = NULL;
    }
    if (m_Events)
    {
        delete m_Events;
        m_Events = NULL;
    }
    if (m_Devsock)
    {
        delete m_Devsock;
        m_Devsock = NULL;
    }
    if (m_Devdef)
    {
        delete m_Devdef;
        m_Devdef = NULL;
    }
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s() Destroy  ...", __FUNCTION__);
}

void CPollingSMP::EvtchkRefresh()
{
    memset(&m_EvtChk, 0, sizeof(m_EvtChk));
    m_EvtChk[UPS_IN_VOL_ ].Max2.flag = APP_MBUS_EVT_UPS_INVOL_MAX2;
    m_EvtChk[UPS_IN_VOL_ ].Max1.flag = APP_MBUS_EVT_UPS_INVOL_MAX1;
    m_EvtChk[UPS_IN_VOL_ ].Min1.flag = APP_MBUS_EVT_UPS_INVOL_MIN1;
    m_EvtChk[UPS_IN_VOL_ ].Min2.flag = APP_MBUS_EVT_UPS_INVOL_MIN2;

    m_EvtChk[UPS_IN_FREQ_].Max2.flag = APP_MBUS_EVT_UPS_INFREQ_MAX2;
    m_EvtChk[UPS_IN_FREQ_].Max1.flag = APP_MBUS_EVT_UPS_INFREQ_MAX1;
    m_EvtChk[UPS_IN_FREQ_].Min1.flag = APP_MBUS_EVT_UPS_INFREQ_MIN1;
    m_EvtChk[UPS_IN_FREQ_].Min2.flag = APP_MBUS_EVT_UPS_INFREQ_MIN2;

    m_EvtChk[UPS_OUT_VOL_].Max2.flag = APP_MBUS_EVT_UPS_OUTVOL_MAX2;
    m_EvtChk[UPS_OUT_VOL_].Max1.flag = APP_MBUS_EVT_UPS_OUTVOL_MAX1;
    m_EvtChk[UPS_OUT_VOL_].Min1.flag = APP_MBUS_EVT_UPS_OUTVOL_MIN1;
    m_EvtChk[UPS_OUT_VOL_].Min2.flag = APP_MBUS_EVT_UPS_OUTVOL_MIN2;

    m_EvtChk[UPS_OUT_CUR_].Max2.flag = APP_MBUS_EVT_UPS_OUTCUR_MAX2;
    m_EvtChk[UPS_OUT_CUR_].Max1.flag = APP_MBUS_EVT_UPS_OUTCUR_MAX1;
    m_EvtChk[UPS_OUT_CUR_].Min1.flag = APP_MBUS_EVT_UPS_OUTCUR_MIN1;
    m_EvtChk[UPS_OUT_CUR_].Min2.flag = APP_MBUS_EVT_UPS_OUTCUR_MIN2;

    m_EvtChk[UPS_BAT_VOL_].Max2.flag = APP_MBUS_EVT_UPS_BATVOL_MAX2;
    m_EvtChk[UPS_BAT_VOL_].Max1.flag = APP_MBUS_EVT_UPS_BATVOL_MAX1;
    m_EvtChk[UPS_BAT_VOL_].Min1.flag = APP_MBUS_EVT_UPS_BATVOL_MIN1;
    m_EvtChk[UPS_BAT_VOL_].Min2.flag = APP_MBUS_EVT_UPS_BATVOL_MIN2;

    m_EvtChk[UPS_BAT_TMP_].Max2.flag = APP_MBUS_EVT_UPS_BATTEMP_MAX2;
    m_EvtChk[UPS_BAT_TMP_].Max1.flag = APP_MBUS_EVT_UPS_BATTEMP_MAX1;
    m_EvtChk[UPS_BAT_TMP_].Min1.flag = APP_MBUS_EVT_UPS_BATTEMP_MIN1;
    m_EvtChk[UPS_BAT_TMP_].Min2.flag = APP_MBUS_EVT_UPS_BATTEMP_MIN2;
    ///////////////////////////////////////////////////////////////
    m_EvtChk[BAT_TEMP_   ].Max2.flag = APP_MBUS_EVT_BAT_TEMP_MAX2;
    m_EvtChk[BAT_TEMP_   ].Max1.flag = APP_MBUS_EVT_BAT_TEMP_MAX1;
    m_EvtChk[BAT_TEMP_   ].Min1.flag = APP_MBUS_EVT_BAT_TEMP_MIN1;
    m_EvtChk[BAT_TEMP_   ].Min2.flag = APP_MBUS_EVT_BAT_TEMP_MIN2;

    m_EvtChk[BAT_CHG_VOL_].Max2.flag = APP_MBUS_EVT_BAT_VOL_MAX2;
    m_EvtChk[BAT_CHG_VOL_].Max1.flag = APP_MBUS_EVT_BAT_VOL_MAX1;
    m_EvtChk[BAT_CHG_VOL_].Min1.flag = APP_MBUS_EVT_BAT_VOL_MIN1;
    m_EvtChk[BAT_CHG_VOL_].Min2.flag = APP_MBUS_EVT_BAT_VOL_MIN2;

    m_EvtChk[BAT_CHG_CUR_].Max2.flag = APP_MBUS_EVT_BAT_CUR_MAX2;
    m_EvtChk[BAT_CHG_CUR_].Max1.flag = APP_MBUS_EVT_BAT_CUR_MAX1;
    m_EvtChk[BAT_CHG_CUR_].Min1.flag = APP_MBUS_EVT_BAT_CUR_MIN1;
    m_EvtChk[BAT_CHG_CUR_].Min2.flag = APP_MBUS_EVT_BAT_CUR_MIN2;
    ///////////////////////////////////////////////////////////////
    memset(&m_BatChk, 0, sizeof(m_BatChk));
    memset(&m_UpsChk, 0, sizeof(m_UpsChk));
}

int CPollingSMP::ThreadMain()
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

        if (m_Source && m_Init)
        {
            try                     //  Device GetData 수집...
            {
                time(&m_dev.curt);
                if (difftime(m_dev.curt, m_dev.last) >= m_Devdef->pollingtime)
                {
                    if (m_TryTo == false)
                        m_MQList.send_message(DEV_FCODE_GET_DATA, NULL, 0);
                    time(&m_dev.last);
                }
                sleepWait(_TICK_WAIT_, &m_Source);
            }
            catch (...)
            {
                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "%s()[%d] exception error [%s]", __FUNCTION__, __LINE__, strerror(errno));
                DoStopCapture();
            }
        }   else
        {
            if (m_Init == false)    //  Device 초기화 과정...
            {
                CDBInterface lDBInf(pcfg.getMyDB());
                if (lDBInf.DBConnect())
                {
                    uint8_t ret = lDBInf.getDevdef(&m_DevInfo, m_Devdef, true);
                    if (ret != _FAIL_)
                    {
                        if (ret == _ISOK_)  //  활성화 상태. (등록된 Device)
                        {
                            DoStartCapture();
                            if (m_Devdef->aflag != _SNMP_TYPE_)
                                m_MQList.send_message(DEV_FCODE_SET_LIMIT, NULL, 0);
                            memcpy(m_Events->bcode, m_Devdef->bcode, sizeof(m_Events->bcode));
                            memcpy(m_Events->bname, m_Devdef->bname, sizeof(m_Events->bname));
                            EvtchkRefresh();
                        }   else            //  최초 접속 or POOL 등록 상태 >> _SUCCESS_ : 미 활성 상태
                        {
                            m_Keep = true;
                        }
                        m_Init = true;
                        //////////////////////////////////////////////////////////////////
                        m_GetData->serial = m_DevInfo.serial;
                        m_Events->serial  = m_DevInfo.serial;
                        //////////////////////////////////////////////////////////////////
                        SDL_Delay(1000);
                    }
                    lDBInf.DBClose();
                }   else
                {
                    sleep(1);
                    pLog.Print(LOG_CRITICAL, 0, NULL, 0, "%s()[%d] Init Failed !!!, Data lost !!!", __FUNCTION__, __LINE__);
                }
            }   else
            if (m_Keep == true)
            {
                if (m_Devdef->aflag != _SNMP_TYPE_)
                {
                    time(&m_keep.curt);
                    if (difftime(m_keep.curt, m_keep.last) >= _KEEP_ALIVE_)
                    {
                        m_MQList.send_message(DEV_FCODE_KEEP_ALIVE, NULL, 0);
                        time(&m_keep.last);
                    }
                }
            }   else
            if (m_NetStop == true)
            {
                time(&m_dev.curt);
                if (difftime(m_dev.curt, m_dev.last) >= m_Devdef->pollingtime)
                {
                    m_MQList.send_message(_NET_STOP_ING_, NULL, 0);
                    time(&m_dev.last);
                }
                sleepWait(_TICK_WAIT_, &m_Source);
            }
        }
    }
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] Thread End ...", __FUNCTION__, __LINE__);
    return 0;
}

static const uint8_t SendPacket(UMSG * pMsg, uint8_t fcode, void * pData = NULL, uint8_t pLen = 0)
{
    if (pMsg->sock.fd <= 0) return _NONE_;
    uint16_t tlen = MakePacket(pMsg, fcode, pData, pLen);
    int sendbytes = SendSocketExact (pMsg->sock.fd, pMsg->msg, tlen);
    if (sendbytes < 0)
        return _FAIL_;
    else
        return _ISOK_;
}

static const uint8_t RecvPacket(UMSG * pMsg, void * pData = NULL, uint16_t pLen = 0)
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    memset(pMsg->msg, 0, sizeof(pMsg->msg));
    HMSG  * head = (HMSG*)pMsg->msg ;
    char  * body = (char*)&pMsg->msg[SZ_HMSG];

    int readBytes = getDataExact(pMsg->sock.fd, pMsg->msg, SZ_HMSG, _RECV_TIME_);
    if (readBytes != SZ_HMSG)
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] recv haed err = (%s)", __FUNCTION__, __LINE__, getError(readBytes, _RECV_TIME_));
        return 201;
    }

    if (head->sop == _SMP_PKT_SOP_)
    {
        if (head->fcode == UDP_FCODE_RES_ERROR)
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s[%d] head->fcode = [%02x] !!!!!!!!!!!!!! ERROR !!!!!!!!!!!!!!", __FUNCTION__, __LINE__, head->fcode);
            return _FAIL_;
        }
        uint16_t blen = htons(head->length);
        readBytes = getDataExact(pMsg->sock.fd, body, blen, _RECV_TIME_);
        if (readBytes != blen)
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] recv body err = (%s)", __FUNCTION__, __LINE__, getError(readBytes, _RECV_TIME_));
            return 202;
        }

        blen -= 1;
        TMSG * tail = (TMSG*)&body[blen];
        if (tail->eop == _SMP_PKT_EOP_)
        {
            if (pData)
            {
                if (blen == pLen)
                    memcpy(pData, body, pLen);
                else
                {
                    pLog.Print(LOG_ERROR, 0, NULL, 0, "%s[%d] blen(%d) <> plen(%d) not equeal size", __FUNCTION__, __LINE__, blen, pLen);
                    return 203;
                }
            }
        }   else
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] End of Packet failed :: EOP = [%02X]", __FUNCTION__, __LINE__, tail->eop);
            return 204;
        }
    }   else
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Start of Packet failed :: SOP = [%02X]", __FUNCTION__, __LINE__, head->sop);
        return 205;
    }

    return _ISOK_;
}

static inline const uint64_t subEvtCheck(const uint16_t Value, const _UPS_LMT_ * lmt, EVTCHK * evtchk, EVENTS * pEvents, uint16_t evtNo)
{
    if (!lmt)
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] lmt is null (Serial = [%010u])", __FUNCTION__, __LINE__, pEvents->serial);
        return 0;
    }
    uint64_t ret = 0;
    if(lmt->Used == '1' && Value >= lmt->Lmt.Max2)
    {
        if(evtchk->Max2.alarmStart == 0)
        {
            time(&evtchk->Max2.alarmtime);
            evtchk->Max2.alarmStart = 1  ;
        }   else
        {
            time(&evtchk->Max2.curttime);
            if(difftime(evtchk->Max2.curttime, evtchk->Max2.alarmtime) >= lmt->Lmt.Time)
            {
                if(evtchk->Max2.DBInsert == 0)
                {
                    pEvents->evtno        = evtNo + 1;
                    pEvents->limitval     = lmt->Lmt.Max2;
                    pEvents->evtval       = Value ;
                    pEvents->starttime    = evtchk->Max2.alarmtime;
                    evtchk->Max2.DBInsert = 1;
                    pEvents->smsflag      = (lmt->Sms == '1' ?  1 : 0);
                    EVQList.send_message(_EVENT_OCCURED_, pEvents, SZ_EVENTS);
                    pLog.Print(LOG_ERROR, 0, NULL, 0, "serial = [%010u], event number = [%u] Event Critical Alarm start time = %s", pEvents->serial, pEvents->evtno, convDateTime(evtchk->Max2.alarmtime));
                }
                ret = evtchk->Max2.flag ;
            }
        }
    }   else
    {
        evtchk->Max2.alarmStart = 0;
        if(evtchk->Max2.DBInsert == 1)
        {
            time(&pEvents->endtime);
            pEvents->starttime    = evtchk->Max2.alarmtime;
            evtchk->Max2.DBInsert = 0;
            pEvents->smsflag      = 0;
            pEvents->evtno        = evtNo + 1;
            EVQList.send_message(_EVENT_CLEARED_, pEvents, SZ_EVENTS);
            pLog.Print(LOG_ERROR, 0, NULL, 0, "serial = [%010u], event number = [%u] Event Critical Alarm stop time  = %s", pEvents->serial, pEvents->evtno, convDateTime(pEvents->endtime));
        }
    }

    if(lmt->Used == '1' && evtchk->Max2.alarmStart == 0 && Value >= lmt->Lmt.Max1)
    {
        if(evtchk->Max1.alarmStart == 0)
        {
            time(&evtchk->Max1.alarmtime);
            evtchk->Max1.alarmStart = 1  ;
        }   else
        {
            time(&evtchk->Max1.curttime);
            if(difftime(evtchk->Max1.curttime, evtchk->Max1.alarmtime) >= lmt->Lmt.Time)
            {
                if(evtchk->Max1.DBInsert == 0)
                {
                    pEvents->evtno        = evtNo + 2;
                    pEvents->limitval     = lmt->Lmt.Max1;
                    pEvents->evtval       = Value ;
                    pEvents->starttime    = evtchk->Max1.alarmtime;
                    evtchk->Max1.DBInsert = 1;
                    pEvents->smsflag      = (lmt->Sms == '1' ?  1 : 0);
                    EVQList.send_message(_EVENT_OCCURED_, pEvents, SZ_EVENTS);
                    pLog.Print(LOG_WARNING, 0, NULL, 0, "serial = [%010u], event number = [%u] Event Warning Alarm start time = %s", pEvents->serial, pEvents->evtno, convDateTime(evtchk->Max1.alarmtime));
                }
                ret = evtchk->Max1.flag ;
            }
        }
    }   else
    {
        evtchk->Max1.alarmStart = 0;
        if(evtchk->Max1.DBInsert == 1)
        {
            time(&pEvents->endtime);
            pEvents->starttime    = evtchk->Max1.alarmtime;
            evtchk->Max1.DBInsert = 0;
            pEvents->smsflag      = 0;
            pEvents->evtno        = evtNo + 2;
            EVQList.send_message(_EVENT_CLEARED_, pEvents, SZ_EVENTS);
            pLog.Print(LOG_WARNING, 0, NULL, 0, "serial = [%010u], event number = [%u] Event Warning Alarm stop time  = %s", pEvents->serial, pEvents->evtno, convDateTime(pEvents->endtime));
        }
    }

    if(lmt->Used == '1' && Value <= lmt->Lmt.Min2)
    {
        if(evtchk->Min2.alarmStart == 0)
        {
            time(&evtchk->Min2.alarmtime);
            evtchk->Min2.alarmStart = 1  ;
        }   else
        {
            time(&evtchk->Min2.curttime);
            if(difftime(evtchk->Min2.curttime, evtchk->Min2.alarmtime) >= lmt->Lmt.Time)
            {
                if(evtchk->Min2.DBInsert == 0)
                {
                    pEvents->evtno        = evtNo + 4;
                    pEvents->limitval     = lmt->Lmt.Min2;
                    pEvents->evtval       = Value ;
                    pEvents->starttime    = evtchk->Min2.alarmtime;
                    evtchk->Min2.DBInsert = 1;
                    pEvents->smsflag      = (lmt->Sms == '1' ?  1 : 0);
                    EVQList.send_message(_EVENT_OCCURED_, pEvents, SZ_EVENTS);
                    pLog.Print(LOG_ERROR, 0, NULL, 0, "serial = [%010u], event number = [%u] Event Critical Alarm start time = %s", pEvents->serial, pEvents->evtno, convDateTime(evtchk->Min2.alarmtime));
                }
                ret = evtchk->Min2.flag ;
            }
        }
    }   else
    {
        evtchk->Min2.alarmStart = 0;
        if(evtchk->Min2.DBInsert == 1)
        {
            time(&pEvents->endtime);
            pEvents->starttime    = evtchk->Min2.alarmtime;
            evtchk->Min2.DBInsert = 0;
            pEvents->smsflag      = 0;
            pEvents->evtno        = evtNo + 4;
            EVQList.send_message(_EVENT_CLEARED_, pEvents, SZ_EVENTS);
            pLog.Print(LOG_ERROR, 0, NULL, 0, "serial = [%010u], event number = [%u] Event Critical Alarm stop time  = %s", pEvents->serial, pEvents->evtno, convDateTime(pEvents->endtime));
        }
    }

    if(lmt->Used == '1' && evtchk->Min2.alarmStart == 0 && Value <= lmt->Lmt.Min1)
    {
        if(evtchk->Min1.alarmStart == 0)
        {
            time(&evtchk->Min1.alarmtime);
            evtchk->Min1.alarmStart = 1  ;
        }   else
        {
            time(&evtchk->Min1.curttime);
            if(difftime(evtchk->Min1.curttime, evtchk->Min1.alarmtime) >= lmt->Lmt.Time)
            {
                if(evtchk->Min1.DBInsert == 0)
                {
                    pEvents->evtno        = evtNo + 3;
                    pEvents->limitval     = lmt->Lmt.Min1;
                    pEvents->evtval       = Value ;
                    pEvents->starttime    = evtchk->Min1.alarmtime;
                    evtchk->Min1.DBInsert = 1;
                    pEvents->smsflag      = (lmt->Sms == '1' ?  1 : 0);
                    EVQList.send_message(_EVENT_OCCURED_, pEvents, SZ_EVENTS);
                    pLog.Print(LOG_WARNING, 0, NULL, 0, "serial = [%010u], event number = [%u] Event Warning Alarm start time = %s", pEvents->serial, pEvents->evtno, convDateTime(evtchk->Min1.alarmtime));
                }
                ret = evtchk->Min1.flag ;
            }
        }
    }   else
    {
        evtchk->Min1.alarmStart = 0;
        if(evtchk->Min1.DBInsert == 1)
        {
            time(&pEvents->endtime);
            pEvents->starttime    = evtchk->Min1.alarmtime;
            evtchk->Min1.DBInsert = 0;
            pEvents->smsflag      = 0;
            pEvents->evtno        = evtNo + 3;
            EVQList.send_message(_EVENT_CLEARED_, pEvents, SZ_EVENTS);
            pLog.Print(LOG_WARNING, 0, NULL, 0, "serial = [%010u], event number = [%u] Event Warning Alarm stop time  = %s", pEvents->serial, pEvents->evtno, convDateTime(pEvents->endtime));
        }
    }

    return ret;
}

static inline const uint64_t subEvtCheck(const int16_t Value, const _UPS_LMT_ * lmt, EVTCHK * evtchk, EVENTS * pEvents, uint16_t evtNo)
{
    if (!lmt)
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] lmt is null (Serial = [%010u])", __FUNCTION__, __LINE__, pEvents->serial);
        return 0;
    }
    uint64_t ret = 0;
    if(lmt->Used == '1' && Value >= (int16_t)lmt->Lmt.Max2)
    {
        if(evtchk->Max2.alarmStart == 0)
        {
            time(&evtchk->Max2.alarmtime);
            evtchk->Max2.alarmStart = 1  ;
        }   else
        {
            time(&evtchk->Max2.curttime);
            if(difftime(evtchk->Max2.curttime, evtchk->Max2.alarmtime) >= lmt->Lmt.Time)
            {
                if(evtchk->Max2.DBInsert == 0)
                {
                    pEvents->evtno        = evtNo + 1;
                    pEvents->limitval     = (int16_t)lmt->Lmt.Max2;
                    pEvents->evtval       = Value ;
                    pEvents->starttime    = evtchk->Max2.alarmtime;
                    evtchk->Max2.DBInsert = 1;
                    pEvents->smsflag      = (lmt->Sms == '1' ?  1 : 0);
                    EVQList.send_message(_EVENT_OCCURED_, pEvents, SZ_EVENTS);
                    pLog.Print(LOG_ERROR, 0, NULL, 0, "serial = [%010u], event number = [%u] Event Critical Alarm start time = %s", pEvents->serial, pEvents->evtno, convDateTime(evtchk->Max2.alarmtime));
                }
                ret = evtchk->Max2.flag ;
            }
        }
    }   else
    {
        evtchk->Max2.alarmStart = 0;
        if(evtchk->Max2.DBInsert == 1)
        {
            time(&pEvents->endtime);
            pEvents->starttime    = evtchk->Max2.alarmtime;
            evtchk->Max2.DBInsert = 0;
            pEvents->smsflag      = 0;
            pEvents->evtno        = evtNo + 1;
            EVQList.send_message(_EVENT_CLEARED_, pEvents, SZ_EVENTS);
            pLog.Print(LOG_ERROR, 0, NULL, 0, "serial = [%010u], event number = [%u] Event Critical Alarm stop time = %s", pEvents->serial, pEvents->evtno, convDateTime(pEvents->endtime));
        }
    }

    if(lmt->Used == '1' && evtchk->Max2.alarmStart == 0 && Value >= (int16_t)lmt->Lmt.Max1)
    {
        if(evtchk->Max1.alarmStart == 0)
        {
            time(&evtchk->Max1.alarmtime);
            evtchk->Max1.alarmStart = 1  ;
        }   else
        {
            time(&evtchk->Max1.curttime);
            if(difftime(evtchk->Max1.curttime, evtchk->Max1.alarmtime) >= lmt->Lmt.Time)
            {
                if(evtchk->Max1.DBInsert == 0)
                {
                    pEvents->evtno        = evtNo + 2;
                    pEvents->limitval     = (int16_t)lmt->Lmt.Max1;
                    pEvents->evtval       = Value ;
                    pEvents->starttime    = evtchk->Max1.alarmtime;
                    evtchk->Max1.DBInsert = 1;
                    pEvents->smsflag      = (lmt->Sms == '1' ?  1 : 0);
                    EVQList.send_message(_EVENT_OCCURED_, pEvents, SZ_EVENTS);
                    pLog.Print(LOG_WARNING, 0, NULL, 0, "serial = [%010u], event number = [%u] Event Warning Alarm start time = %s", pEvents->serial, pEvents->evtno, convDateTime(evtchk->Max1.alarmtime));
                }
                ret = evtchk->Max1.flag ;
            }
        }
    }   else
    {
        evtchk->Max1.alarmStart = 0;
        if(evtchk->Max1.DBInsert == 1)
        {
            time(&pEvents->endtime);
            pEvents->starttime    = evtchk->Max1.alarmtime;
            evtchk->Max1.DBInsert = 0;
            pEvents->smsflag      = 0;
            pEvents->evtno        = evtNo + 2;
            EVQList.send_message(_EVENT_CLEARED_, pEvents, SZ_EVENTS);
            pLog.Print(LOG_WARNING, 0, NULL, 0, "serial = [%010u], event number = [%u] Event Warning Alarm stop time = %s", pEvents->serial, pEvents->evtno, convDateTime(pEvents->endtime));
        }
    }

    if(lmt->Used == '1' && Value <= (int16_t)lmt->Lmt.Min2)
    {
        if(evtchk->Min2.alarmStart == 0)
        {
            time(&evtchk->Min2.alarmtime);
            evtchk->Min2.alarmStart = 1  ;
        }   else
        {
            time(&evtchk->Min2.curttime);
            if(difftime(evtchk->Min2.curttime, evtchk->Min2.alarmtime) >= lmt->Lmt.Time)
            {
                if(evtchk->Min2.DBInsert == 0)
                {
                    pEvents->evtno        = evtNo + 4;
                    pEvents->limitval     = (int16_t)lmt->Lmt.Min2;
                    pEvents->evtval       = Value ;
                    pEvents->starttime    = evtchk->Min2.alarmtime;
                    evtchk->Min2.DBInsert = 1;
                    pEvents->smsflag      = (lmt->Sms == '1' ?  1 : 0);
                    EVQList.send_message(_EVENT_OCCURED_, pEvents, SZ_EVENTS);
                    pLog.Print(LOG_ERROR, 0, NULL, 0, "serial = [%010u], event number = [%u] Event Critical Alarm start time = %s", pEvents->serial, pEvents->evtno, convDateTime(evtchk->Min2.alarmtime));
                }
                ret = evtchk->Min2.flag ;
            }
        }
    }   else
    {
        evtchk->Min2.alarmStart = 0;
        if(evtchk->Min2.DBInsert == 1)
        {
            time(&pEvents->endtime);
            pEvents->starttime    = evtchk->Min2.alarmtime;
            evtchk->Min2.DBInsert = 0;
            pEvents->smsflag      = 0;
            pEvents->evtno        = evtNo + 4;
            EVQList.send_message(_EVENT_CLEARED_, pEvents, SZ_EVENTS);
            pLog.Print(LOG_ERROR, 0, NULL, 0, "serial = [%010u], event number = [%u] Event Critical Alarm stop time  = %s", pEvents->serial, pEvents->evtno, convDateTime(pEvents->endtime));
        }
    }

    if(lmt->Used == '1' && evtchk->Min2.alarmStart == 0 && Value <= (int16_t)lmt->Lmt.Min1)
    {
        if(evtchk->Min1.alarmStart == 0)
        {
            time(&evtchk->Min1.alarmtime);
            evtchk->Min1.alarmStart = 1  ;
        }   else
        {
            time(&evtchk->Min1.curttime);
            if(difftime(evtchk->Min1.curttime, evtchk->Min1.alarmtime) >= lmt->Lmt.Time)
            {
                if(evtchk->Min1.DBInsert == 0)
                {
                    pEvents->evtno        = evtNo + 3;
                    pEvents->limitval     = (int16_t)lmt->Lmt.Min1;
                    pEvents->evtval       = Value ;
                    pEvents->starttime    = evtchk->Min1.alarmtime;
                    evtchk->Min1.DBInsert = 1;
                    pEvents->smsflag      = (lmt->Sms == '1' ?  1 : 0);
                    EVQList.send_message(_EVENT_OCCURED_, pEvents, SZ_EVENTS);
                    pLog.Print(LOG_WARNING, 0, NULL, 0, "serial = [%010u], event number = [%u] Event Warning Alarm start time = %s", pEvents->serial, pEvents->evtno, convDateTime(evtchk->Min1.alarmtime));
                }
                ret = evtchk->Min1.flag ;
            }
        }
    }   else
    {
        evtchk->Min1.alarmStart = 0;
        if(evtchk->Min1.DBInsert == 1)
        {
            time(&pEvents->endtime);
            pEvents->starttime    = evtchk->Min1.alarmtime;
            evtchk->Min1.DBInsert = 0;
            pEvents->smsflag      = 0;
            pEvents->evtno        = evtNo + 3;
            EVQList.send_message(_EVENT_CLEARED_, pEvents, SZ_EVENTS);
            pLog.Print(LOG_WARNING, 0, NULL, 0, "serial = [%010u], event number = [%u] Event Warning Alarm stop time  = %s", pEvents->serial, pEvents->evtno, convDateTime(pEvents->endtime));
        }
    }

    return ret;
}

static inline const void subBatCheck(const uint16_t pflag, BATCHK * evtchk, EVENTS * pEvents, uint16_t evtNo)
{
    if(pflag)
    {
        if(evtchk->DBInsert == 0)
        {
            time(&evtchk->alarmtime);
            pEvents->starttime = evtchk->alarmtime;
            pEvents->evtno     = evtNo;
            pEvents->limitval  = 0;
            pEvents->evtval    = 0;
            evtchk->DBInsert   = 1;
            pEvents->smsflag   = 0;
            EVQList.send_message(_EVENT_OCCURED_, pEvents, SZ_EVENTS);
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "serial = [%010u], event number = [%u] battery Alarm start time = %s", pEvents->serial, evtNo, convDateTime(pEvents->starttime));
        }
    }   else
    {
        if(evtchk->DBInsert == 1)
        {
            time(&pEvents->endtime);
            pEvents->starttime = evtchk->alarmtime;
            evtchk->DBInsert   = 0;
            pEvents->smsflag   = 0;
            pEvents->evtno     = evtNo;
            EVQList.send_message(_EVENT_CLEARED_, pEvents, SZ_EVENTS);
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "serial = [%010u], event number = [%u] battery Alarm stop time  = %s", pEvents->serial, evtNo, convDateTime(pEvents->endtime));
        }
    }
    return ;
}

static inline const uint16_t subFireCheck(const uint8_t fflag, const uint16_t heat, const uint16_t smoke, BATCHK * evtchk, EVENTS * pEvents, uint16_t evtNo)
{
    uint16_t ret = 0;
    if (fflag == 3)
    {
        if(heat && smoke)
        {
            if(evtchk->DBInsert == 0)
            {
                time(&evtchk->alarmtime);
                pEvents->starttime = evtchk->alarmtime;
                pEvents->evtno     = evtNo;
                pEvents->limitval  = 0;
                pEvents->evtval    = 0;
                evtchk->DBInsert   = 1;
                pEvents->smsflag   = 0;
                EVQList.send_message(_EVENT_OCCURED_, pEvents, SZ_EVENTS);
                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "serial = [%010u], event number = [%u] Fire !! Fire !! start time = %s", pEvents->serial, evtNo, convDateTime(pEvents->starttime));
            }
            ret = APP_MBUS_EVT_FIRE;
        }   else
        {
            if(evtchk->DBInsert == 1)
            {
                time(&pEvents->endtime);
                pEvents->starttime = evtchk->alarmtime;
                evtchk->DBInsert   = 0;
                pEvents->smsflag   = 0;
                pEvents->evtno     = evtNo;
                EVQList.send_message(_EVENT_CLEARED_, pEvents, SZ_EVENTS);
                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "serial = [%010u], event number = [%u] Fire !! Fire !! stop time  = %s", pEvents->serial, evtNo, convDateTime(pEvents->endtime));
            }
        }
    }   else
    if (fflag == 2)
    {
        if(smoke)
        {
            if(evtchk->DBInsert == 0)
            {
                time(&evtchk->alarmtime);
                pEvents->starttime = evtchk->alarmtime;
                pEvents->evtno     = evtNo;
                pEvents->limitval  = 0;
                pEvents->evtval    = 0;
                evtchk->DBInsert   = 1;
                pEvents->smsflag   = 0;
                EVQList.send_message(_EVENT_OCCURED_, pEvents, SZ_EVENTS);
                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "serial = [%010u], event number = [%u] Fire !! Fire !! start time = %s", pEvents->serial, evtNo, convDateTime(pEvents->starttime));
            }
            ret = APP_MBUS_EVT_FIRE;
        }   else
        {
            if(evtchk->DBInsert == 1)
            {
                time(&pEvents->endtime);
                pEvents->starttime = evtchk->alarmtime;
                evtchk->DBInsert   = 0;
                pEvents->smsflag   = 0;
                pEvents->evtno     = evtNo;
                EVQList.send_message(_EVENT_CLEARED_, pEvents, SZ_EVENTS);
                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "serial = [%010u], event number = [%u] Fire !! Fire !! stop time  = %s", pEvents->serial, evtNo, convDateTime(pEvents->endtime));
            }
        }
    }   else
    if (fflag == 1)
    {
        if(heat)
        {
            if(evtchk->DBInsert == 0)
            {
                time(&evtchk->alarmtime);
                pEvents->starttime = evtchk->alarmtime;
                pEvents->evtno     = evtNo;
                pEvents->limitval  = 0;
                pEvents->evtval    = 0;
                evtchk->DBInsert   = 1;
                pEvents->smsflag   = 0;
                EVQList.send_message(_EVENT_OCCURED_, pEvents, SZ_EVENTS);
                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "serial = [%010u], event number = [%u] Fire !! Fire !! start time = %s", pEvents->serial, evtNo, convDateTime(pEvents->starttime));
            }
            ret = APP_MBUS_EVT_FIRE;
        }   else
        {
            if(evtchk->DBInsert == 1)
            {
                time(&pEvents->endtime);
                pEvents->starttime = evtchk->alarmtime;
                evtchk->DBInsert   = 0;
                pEvents->smsflag   = 0;
                pEvents->evtno     = evtNo;
                EVQList.send_message(_EVENT_CLEARED_, pEvents, SZ_EVENTS);
                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "serial = [%010u], event number = [%u] Fire !! Fire !! stop time  = %s", pEvents->serial, evtNo, convDateTime(pEvents->endtime));
            }
        }
    }

    return ret;
}

static inline const void subUpsCheck(const uint16_t pflag, const _UPS_LMT_ * lmt, BATCHK * evtchk, EVENTS * pEvents, uint16_t evtNo)
{
    if (lmt)
    {
        if (lmt->Used == '1')
        {
            if(pflag)
            {
                if(evtchk->DBInsert == 0)
                {
                    time(&evtchk->alarmtime);
                    pEvents->starttime = evtchk->alarmtime;
                    pEvents->evtno     = evtNo;
                    pEvents->limitval  = 0;
                    pEvents->evtval    = 0;
                    evtchk->DBInsert   = 1;
                    pEvents->smsflag   = (lmt->Sms == '1' ?  1 : 0);
                    EVQList.send_message(_EVENT_OCCURED_, pEvents, SZ_EVENTS);
                    pLog.Print(LOG_CRITICAL, 0, NULL, 0, "serial = [%010u], event number = [%u] UpsStatus start time = %s", pEvents->serial, evtNo, convDateTime(pEvents->starttime));
                }
            }   else
            {
                if(evtchk->DBInsert == 1)
                {
                    time(&pEvents->endtime);
                    pEvents->starttime = evtchk->alarmtime;
                    evtchk->DBInsert   = 0;
                    pEvents->smsflag   = 0;
                    pEvents->evtno     = evtNo;
                    EVQList.send_message(_EVENT_CLEARED_, pEvents, SZ_EVENTS);
                    pLog.Print(LOG_CRITICAL, 0, NULL, 0, "serial = [%010u], event number = [%u] UpsStatus stop time  = %s", pEvents->serial, evtNo, convDateTime(pEvents->endtime));
                }
            }
        }   else
        {
            if(evtchk->DBInsert == 1)
            {
                time(&pEvents->endtime);
                pEvents->starttime = evtchk->alarmtime;
                evtchk->DBInsert   = 0;
                pEvents->smsflag   = 0;
                pEvents->evtno     = evtNo;
                EVQList.send_message(_EVENT_CLEARED_, pEvents, SZ_EVENTS);
                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "serial = [%010u], event number = [%u] UpsStatus stop time  = %s", pEvents->serial, evtNo, convDateTime(pEvents->endtime));
            }
        }
    }   else
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] _UPS_LMT_ is null, serial = [%010u] ", __FUNCTION__, __LINE__, pEvents->serial);
        if(pflag)
        {
            if(evtchk->DBInsert == 0)
            {
                time(&evtchk->alarmtime);
                pEvents->starttime = evtchk->alarmtime;
                pEvents->evtno     = evtNo;
                pEvents->limitval  = 0;
                pEvents->evtval    = 0;
                evtchk->DBInsert   = 1;
                pEvents->smsflag   = 0;
                EVQList.send_message(_EVENT_OCCURED_, pEvents, SZ_EVENTS);
                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "serial = [%010u], event number = [%u] UpsStatus start time = %s", pEvents->serial, evtNo, convDateTime(pEvents->starttime));
            }
        }   else
        {
            if(evtchk->DBInsert == 1)
            {
                time(&pEvents->endtime);
                pEvents->starttime = evtchk->alarmtime;
                evtchk->DBInsert   = 0;
                pEvents->smsflag   = 0;
                pEvents->evtno     = evtNo;
                EVQList.send_message(_EVENT_CLEARED_, pEvents, SZ_EVENTS);
                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "serial = [%010u], event number = [%u] UpsStatus stop time  = %s", pEvents->serial, evtNo, convDateTime(pEvents->endtime));
            }
        }
    }
    return ;
}

uint64_t CPollingSMP::checkLimit()
{
    m_Events->smsflag     = 0;
    m_GetData->LmtEvtBits = 0;

    if (!m_IsLora)
    {
        if (!(m_GetData->Data.EventBits & APP_MBUS_EVT_MODEM))
            m_GetData->Data.EventBits |= APP_MBUS_EVT_MODEM;
    }

    //UPS Input Voltage Max2/Max1/Min1/Min2 Threshold
    m_GetData->LmtEvtBits = m_GetData->LmtEvtBits | subEvtCheck (m_GetData->Data.UpsInVol  , (_UPS_LMT_*)m_Devdef->evtLimit.getdata(UPS_IN_VOL_ ), &m_EvtChk[UPS_IN_VOL_ ], m_Events, 10);
    m_GetData->LmtEvtBits = m_GetData->LmtEvtBits | subEvtCheck (m_GetData->Data.UpsInFreq , (_UPS_LMT_*)m_Devdef->evtLimit.getdata(UPS_IN_FREQ_), &m_EvtChk[UPS_IN_FREQ_], m_Events, 20);
    m_GetData->LmtEvtBits = m_GetData->LmtEvtBits | subEvtCheck (m_GetData->Data.UpsOutVol , (_UPS_LMT_*)m_Devdef->evtLimit.getdata(UPS_OUT_VOL_), &m_EvtChk[UPS_OUT_VOL_], m_Events, 30);
    m_GetData->LmtEvtBits = m_GetData->LmtEvtBits | subEvtCheck (m_GetData->Data.UpsOutCur , (_UPS_LMT_*)m_Devdef->evtLimit.getdata(UPS_OUT_CUR_), &m_EvtChk[UPS_OUT_CUR_], m_Events, 40);
    m_GetData->LmtEvtBits = m_GetData->LmtEvtBits | subEvtCheck (m_GetData->Data.UpsBatVol , (_UPS_LMT_*)m_Devdef->evtLimit.getdata(UPS_BAT_VOL_), &m_EvtChk[UPS_BAT_VOL_], m_Events, 50);
    m_GetData->LmtEvtBits = m_GetData->LmtEvtBits | subEvtCheck (m_GetData->Data.UpsBatTemp, (_UPS_LMT_*)m_Devdef->evtLimit.getdata(UPS_BAT_TMP_), &m_EvtChk[UPS_BAT_TMP_], m_Events, 60);
    //Battery Input Data Max2/Max1/Min1/Min2 Threshold
    m_GetData->LmtEvtBits = m_GetData->LmtEvtBits | subEvtCheck (m_GetData->Data.BatVol    , (_UPS_LMT_*)m_Devdef->evtLimit.getdata(BAT_CHG_VOL_), &m_EvtChk[BAT_CHG_VOL_], m_Events, 70);
    m_GetData->LmtEvtBits = m_GetData->LmtEvtBits | subEvtCheck (m_GetData->Data.BatCur    , (_UPS_LMT_*)m_Devdef->evtLimit.getdata(BAT_CHG_CUR_), &m_EvtChk[BAT_CHG_CUR_], m_Events, 80);
    m_GetData->LmtEvtBits = m_GetData->LmtEvtBits | subEvtCheck (m_GetData->Data.BatEnvTemp, (_UPS_LMT_*)m_Devdef->evtLimit.getdata(BAT_TEMP_   ), &m_EvtChk[BAT_TEMP_   ], m_Events, 90);
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //Controller UpsStatus Event Check
    subUpsCheck ((m_GetData->Data.UpsStatus & APP_MBUS_SBIT_UTIL_FAIL   ), (_UPS_LMT_*)m_Devdef->evtLimit.getdata(E_UPS_UTIL_FAIL   ), &m_UpsChk[UPS_UTIL_FAIL   ], m_Events, 300);
    subUpsCheck ((m_GetData->Data.UpsStatus & APP_MBUS_SBIT_BAT_LOW     ), (_UPS_LMT_*)m_Devdef->evtLimit.getdata(E_UPS_BAT_LOW     ), &m_UpsChk[UPS_BAT_LOW     ], m_Events, 310);
    subUpsCheck ((m_GetData->Data.UpsStatus & APP_MBUS_SBIT_BYPASS_ON   ), (_UPS_LMT_*)m_Devdef->evtLimit.getdata(E_UPS_BYPASS_ON   ), &m_UpsChk[UPS_BYPASS_ON   ], m_Events, 320);
    subUpsCheck ((m_GetData->Data.UpsStatus & APP_MBUS_SBIT_UPS_FAIL    ), (_UPS_LMT_*)m_Devdef->evtLimit.getdata(E_UPS_UPS_FAIL    ), &m_UpsChk[UPS_UPS_FAIL    ], m_Events, 330);
    subUpsCheck ((m_GetData->Data.UpsStatus & APP_MBUS_SBIT_STANDBY_TYPE), (_UPS_LMT_*)m_Devdef->evtLimit.getdata(E_UPS_STANDBY_TYPE), &m_UpsChk[UPS_STANDBY_TYPE], m_Events, 340);
    subUpsCheck ((m_GetData->Data.UpsStatus & APP_MBUS_SBIT_TESTING     ), (_UPS_LMT_*)m_Devdef->evtLimit.getdata(E_UPS_TESTING     ), &m_UpsChk[UPS_TESTING     ], m_Events, 350);
    subUpsCheck ((m_GetData->Data.UpsStatus & APP_MBUS_SBIT_SHUTDOWN    ), (_UPS_LMT_*)m_Devdef->evtLimit.getdata(E_UPS_SHUTDOWN    ), &m_UpsChk[UPS_SHUTDOWN    ], m_Events, 360);
    subUpsCheck ((m_GetData->Data.UpsStatus & APP_MBUS_SBIT_BEEP_ON     ), (_UPS_LMT_*)m_Devdef->evtLimit.getdata(E_UPS_BEEP_ON     ), &m_UpsChk[UPS_BEEP_ON     ], m_Events, 370);

    //Controller Event Check
    subBatCheck ((m_GetData->Data.EventBits & APP_MBUS_EVT_HEAT         ), &m_BatChk[EVT_HEAT        ], m_Events, 200);
    subBatCheck ((m_GetData->Data.EventBits & APP_MBUS_EVT_SMOKE        ), &m_BatChk[EVT_SMOKE       ], m_Events, 210);
    subBatCheck ((m_GetData->Data.EventBits & APP_MBUS_EVT_POWER        ), &m_BatChk[EVT_POWER       ], m_Events, 220);
    subBatCheck ((m_GetData->Data.EventBits & APP_MBUS_EVT_NETWORK      ), &m_BatChk[EVT_NETWORK     ], m_Events, 230);
    subBatCheck ((m_GetData->Data.EventBits & APP_MBUS_EVT_GATEWAY      ), &m_BatChk[EVT_GATEWAY     ], m_Events, 240);
    subBatCheck ((m_GetData->Data.EventBits & APP_MBUS_EVT_DEVICE       ), &m_BatChk[EVT_DEVICE      ], m_Events, 245);
    subBatCheck ((m_GetData->Data.EventBits & APP_MBUS_EVT_MODEM        ), &m_BatChk[EVT_MODEM       ], m_Events, 250);
    subBatCheck ((m_GetData->Data.EventBits & APP_MBUS_EVT_BAT_VOL      ), &m_BatChk[EVT_BAT_VOL     ], m_Events, 260);
    subBatCheck ((m_GetData->Data.EventBits & APP_MBUS_EVT_BAT_CUR      ), &m_BatChk[EVT_BAT_CUR     ], m_Events, 270);
    subBatCheck ((m_GetData->Data.EventBits & APP_MBUS_EVT_BAT_TEMP     ), &m_BatChk[EVT_BAT_TEMP    ], m_Events, 280);

    //  Fire Check ...
    m_GetData->Data.EventBits = m_GetData->Data.EventBits | subFireCheck(m_FFlag, (m_GetData->Data.EventBits & APP_MBUS_EVT_HEAT), (m_GetData->Data.EventBits & APP_MBUS_EVT_SMOKE), &m_BatChk[EVT_BAT_FIRE], m_Events, 215);

    return (m_GetData->LmtEvtBits + m_GetData->Data.EventBits);
}

uint8_t CPollingSMP::getDevDataPost()
{
    /******************************* Pre-Process Start **************************/
    m_GetData->Data.EventBits = m_GetData->Data.EventBits & ~APP_MBUS_EVT_GATEWAY;
    /******************************* Pre-Process End   **************************/

    uint64_t ret = checkLimit();
    if (ret > 0)
    {
        pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] serial = [%010u], Data.UpsStatus  = [%04X]" , __FUNCTION__, __LINE__, m_GetData->serial, m_GetData->Data.UpsStatus);
        pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] serial = [%010u], Data.BatStatus  = [%04X]" , __FUNCTION__, __LINE__, m_GetData->serial, m_GetData->Data.BatStatus);
        pLog.Print(LOG_INFO , 0, NULL, 0, "%s()[%d] serial = [%010u], Data.EventBits  = [%04X]" , __FUNCTION__, __LINE__, m_GetData->serial, m_GetData->Data.EventBits);
        pLog.Print(LOG_INFO , 0, NULL, 0, "%s()[%d] serial = [%010u], pNjs.LmtEvtBits = [%016X]", __FUNCTION__, __LINE__, m_GetData->serial, m_GetData->LmtEvtBits    );
    }

    ///////////////////////////////////////////////////////////
    time(&m_njs.curt);
    if (difftime(m_njs.curt, m_njs.last) >= m_Devdef->nodejstime)
    {
        m_SndMsg[m_seqno]->SendNJ((char*)m_GetData, SZ_NJSMSG);
        time(&m_njs.last);
    }   else
    if (ret > 0)   // Event 발생..
    {
        m_SndMsg[m_seqno]->SendNJ((char*)m_GetData, SZ_NJSMSG);
    }

    ///////////////////////////////////////////////////////////
    time(&m_dbs.curt);
    if (difftime(m_dbs.curt, m_dbs.last) >= m_Devdef->dbsavetime)
    {
        m_GetData->timestamp = m_dbs.curt;
        m_SndMsg[m_seqno]->SendDB((char*)m_GetData, SZ_GETDATA);
        time(&m_dbs.last);
    }   else
    if (m_GetData->Data.UpsStatus != m_Status.UpsStatus ||
        m_GetData->Data.BatStatus != m_Status.BatStatus ||
        m_GetData->Data.EventBits != m_Status.EventBits ||
        m_GetData->LmtEvtBits     != m_Status.LmtEvtBits )
    {
        m_GetData->timestamp = m_dbs.curt;
        m_SndMsg[m_seqno]->SendDB((char*)m_GetData, SZ_GETDATA);

        m_Status.UpsStatus  = m_GetData->Data.UpsStatus;
        m_Status.BatStatus  = m_GetData->Data.BatStatus;
        m_Status.EventBits  = m_GetData->Data.EventBits;
        m_Status.LmtEvtBits = m_GetData->LmtEvtBits;
    }
    ///////////////////////////////////////////////////////////

    return _ISOK_;
}

uint8_t CPollingSMP::getDevData(UMSG * pMsg)
{
    if (m_Devdef->aflag != _SNMP_TYPE_)
    {
        m_Mutex.lock();
        m_TryTo = true;
        uint8_t snd = SendPacket(pMsg, DEV_FCODE_GET_DATA);
        if (_ISOK_ != snd)
        {
            DoStopCapture();
            m_Mutex.unlock();
            if (snd != _NONE_)
                pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Send Packet Error [%010u](%d)(%s) !!", __FUNCTION__, __LINE__, m_DevInfo.serial, errno, strerror(errno));
            m_TryTo = false;
            return snd;
        }
        snd = RecvPacket(pMsg, &m_GetData->Data, SZ_DATA);
        if (_ISOK_ != snd)
        {
            DoStopCapture();
            m_Mutex.unlock();
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Recv Packet Error [%010u](%d)(%s) !!", __FUNCTION__, __LINE__, m_DevInfo.serial, errno, strerror(errno));
            m_TryTo = false;
            return snd;
        }
        m_TryTo = false;
        m_Mutex.unlock();

        if (m_Devdef->aflag == _DEVICE_TYPE_)
            return getDevDataPost();
        else
        if (m_Devdef->aflag == _DUAL_TYPE_)
        {
            getSnmpData(&m_GetData->Data, &m_GetData->Data2, m_Devdef->phase);
            return getDevDataPost();

            //if (_ISOK_ == SnmpGetDataEx(&m_GetData->Data, &m_GetData->Data2, m_Devdef->snmpip, m_Devdef->phase))
            //    return getDevDataPost();
            //else
            //{
            //    memset(&m_GetData->Data , 0, SZ_DATA );
            //    memset(&m_GetData->Data2, 0, SZ_DATA2);
            //}
        }
    }   else
    {
        getSnmpData(&m_GetData->Data, &m_GetData->Data2, m_Devdef->phase);
        return getDevDataPost();
        //if (_ISOK_ == SnmpGetDataEx(&m_GetData->Data, &m_GetData->Data2, m_Devdef->snmpip, m_Devdef->phase))
        //    return getDevDataPost();
        //else
        //{
        //    memset(&m_GetData->Data , 0, SZ_DATA );
        //    //memset(&m_GetData->Data2, 0, SZ_DATA2);
        //}
    }
    return _ISOK_;
}

void CPollingSMP::getSnmpData(DATA * pData, DATA2 * pData2, uint8_t phase)
{
    UPSSNMP * pUps = pcfg.getUpsSnmp(m_DevInfo.serial);
    if (pUps)
    {
        if (phase == 1)     // 단상 UPS
        {
            pData->UpsInVol    = pUps->UpsInVol  ;
            pData->UpsInFreq   = pUps->UpsInFreq ;
            pData->UpsOutVol   = pUps->UpsOutVol ;
            pData->UpsOutCur   = pUps->UpsOutCur ;
            pData->UpsBatVol   = pUps->UpsBatVol ;
            pData->UpsBatTemp  = pUps->UpsBatTemp;
            pData->UpsStatus   = pUps->UpsStatus ;
        }   else
        if (phase == 3)     // 3상 UPS
        {
            pData->UpsInVol    = pUps->UpsInVol  ;
            pData2->UpsData[0] = pUps->UpsData[0];
            pData2->UpsData[1] = pUps->UpsData[1];
            pData->UpsInFreq   = pUps->UpsInFreq ;
            pData->UpsOutVol   = pUps->UpsOutVol ;
            pData2->UpsData[2] = pUps->UpsData[2];
            pData2->UpsData[3] = pUps->UpsData[3];
            pData->UpsOutCur   = pUps->UpsOutCur ;
            pData2->UpsData[4] = pUps->UpsData[4];
            pData2->UpsData[5] = pUps->UpsData[5];
            pData->UpsBatVol   = pUps->UpsBatVol ;
            pData->UpsBatTemp  = pUps->UpsBatTemp;
            pData->UpsStatus   = pUps->UpsStatus ;
        }
    }
    
//    CDBInterface lDBInf(pcfg.getMyDB());
//    if (lDBInf.DBConnect())
//    {
//        lDBInf.setSnmpData(m_DevInfo.serial, pData, pData2, phase);
//        lDBInf.DBClose();
//    }
}


//char OID[CALL_OID_NUMBER][64] = {
//                        {" .1.3.6.1.4.1.935.1.1.1.3.2.1.0"},    //입력 전압
//                        {" .1.3.6.1.4.1.935.1.1.1.3.2.4.0"},    //입력 주파수
//                        {" .1.3.6.1.4.1.935.1.1.1.4.2.1.0"},    //출력 전압
//                        {" .1.3.6.1.4.1.935.1.1.1.4.2.3.0"},    //출력 전류(%)
//                        {" .1.3.6.1.4.1.935.1.1.1.2.2.2.0"},    //배터리 전압
//                        {" .1.3.6.1.4.1.935.1.1.1.2.2.3.0"},    //배터리 온도
//                        {" .1.3.6.1.4.1.935.1.1.1.4.1.1.0"}     //UPS Status
//                    };
//
//char OID2[CALL_OID_NUMBER2][64] = {
//                        {" .1.3.6.1.4.1.935.1.1.1.8.2.2.0"},    //입력 전압 R
//                        {" .1.3.6.1.4.1.935.1.1.1.8.2.3.0"},    //입력 전압 S
//                        {" .1.3.6.1.4.1.935.1.1.1.8.2.4.0"},    //입력 전압 T
//                        {" .1.3.6.1.4.1.935.1.1.1.8.2.1.0"},    //입력 주파수
//                        {" .1.3.6.1.4.1.935.1.1.1.8.3.2.0"},    //출력 전압 R
//                        {" .1.3.6.1.4.1.935.1.1.1.8.3.3.0"},    //출력 전압 S
//                        {" .1.3.6.1.4.1.935.1.1.1.8.3.4.0"},    //출력 전압 T
//                        {" .1.3.6.1.4.1.935.1.1.1.8.3.5.0"},    //출력 Load(%) R
//                        {" .1.3.6.1.4.1.935.1.1.1.8.3.6.0"},    //출력 Load(%) S
//                        {" .1.3.6.1.4.1.935.1.1.1.8.3.7.0"},    //출력 Load(%) T
//                        {" .1.3.6.1.4.1.935.1.1.1.8.1.1.0"},    //배터리 전압
//                        {" .1.3.6.1.4.1.935.1.1.1.8.1.5.0"},    //배터리 온도
//                        {" .1.3.6.1.4.1.935.1.1.1.4.1.1.0"}     //UPS Status
//                    };
//
//static inline const uint8_t snmpGet(char * pOid, char * pIp, uint16_t & sVal)
//{
//    FILE  * fp = NULL;
//    char    pBuff[128] = {0,};
//    char    pCmds[256] = {0,};
//
//    snprintf(pCmds, sizeof(pCmds), "snmpget -v 2c -c public -t 1 -r 0 %s %s |awk -F' ' '{print $4}'", pIp, pOid);
//    // excute command
//    fp = popen(pCmds, "r");
//    if(!fp)
//    {
//        pLog.Print(LOG_ERROR, 0, NULL, 0, "\x1b[1;34m%s() popen error [%d:%s] (pCmds = %s)\x1b[0m", __FUNCTION__, errno, strerror(errno), pCmds);
//        return _FAIL_;
//    }
//
//    // read the result
//    size_t readSize = fread(pBuff, sizeof(char), sizeof(pBuff)-1, fp);
//    if(readSize < 0)
//    {
//        pclose(fp);
//        pLog.Print(LOG_ERROR, 0, NULL, 0, "\x1b[1;34m%s() fread error [%d:%s] (pCmds = %s)\x1b[0m", __FUNCTION__, errno, strerror(errno), pCmds);
//        return _FAIL_;
//    }
//    pclose(fp);
//    SDL_Delay(50);
//    pBuff[readSize] = '\0';
//    if (readSize > 0)
//    {
//        try
//        {
//            sVal = atoi(pBuff);
//            return _ISOK_;
//        }   catch(...)
//        {
//            sVal = 0;
//            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] (%s) : result (%s)", __FUNCTION__, __LINE__, pCmds, pBuff);
//            return _FAIL_;
//        }
//    }   else
//    {
//        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] (%s) : result (%s)", __FUNCTION__, __LINE__, pCmds, pBuff);
//        return _FAIL_;
//    }
//}
//
//// snmpget -v 2c -c public -t 2 1.241.172.141  .1.3.6.1.4.1.935.1.1.1.3.2.1.0 |awk -F' ' '{print $4}'
//// echo "show stat" | nc -U /var/lib/haproxy/stats|awk -F',' '{print $2,$3,$4,$5,$6,$14,$37,$26}'|grep $i|awk -F' ' '{print $2,$3,$4,$5,$6}'
//
//uint8_t CPollingSMP::SnmpGetData(DATA * pData, DATA2 * pData2, char * pSnmpIp, uint8_t phase)
//{
//    if (strlen(pSnmpIp) <= 0)
//    {
//        pLog.Print(LOG_WARNING, 0, NULL, 0, "%s()[%d] Snmp_Ip is Empty !!! [serial = %010u]", __FUNCTION__, __LINE__, m_DevInfo.serial);
//        return _FAIL_;
//    }
//    do  {
//        pData->UpsStatus = 0;
//        if (phase == 1)     // 단상 UPS
//        {
//            if (_ISOK_ != snmpGet(OID[0], pSnmpIp, pData->UpsInVol  )) break;    //입력 전압
//            if (_ISOK_ != snmpGet(OID[1], pSnmpIp, pData->UpsInFreq )) break;    //입력 주파수
//            if (_ISOK_ != snmpGet(OID[2], pSnmpIp, pData->UpsOutVol )) break;    //출력 전압
//            if (_ISOK_ != snmpGet(OID[3], pSnmpIp, pData->UpsOutCur )) break;    //출력 전류(%)
//            if (_ISOK_ != snmpGet(OID[4], pSnmpIp, pData->UpsBatVol )) break;    //배터리 전압
//            if (_ISOK_ != snmpGet(OID[5], pSnmpIp, pData->UpsBatTemp)) break;    //배터리 온도
//            if (_ISOK_ != snmpGet(OID[6], pSnmpIp, pData->UpsStatus )) break;    //UpsStatus
//
//            pData->UpsInVol   /= 10;
//            pData->UpsInFreq  /= 10;
//            pData->UpsOutVol  /= 10;
//            pData->UpsBatVol  *= 10;
//            pData->UpsBatTemp *= 10;
//        }   else
//        if (phase == 3)     // 3상 UPS
//        {
//            if (_ISOK_ != snmpGet(OID2[0] , pSnmpIp, pData->UpsInVol   )) break; //입력 전압 R
//            if (_ISOK_ != snmpGet(OID2[1] , pSnmpIp, pData2->UpsData[0])) break; //입력 전압 S
//            if (_ISOK_ != snmpGet(OID2[2] , pSnmpIp, pData2->UpsData[1])) break; //입력 전압 T
//            if (_ISOK_ != snmpGet(OID2[3] , pSnmpIp, pData->UpsInFreq  )) break; //입력 주파수
//            if (_ISOK_ != snmpGet(OID2[4] , pSnmpIp, pData->UpsOutVol  )) break; //출력 전압 R
//            if (_ISOK_ != snmpGet(OID2[5] , pSnmpIp, pData2->UpsData[2])) break; //출력 전압 S
//            if (_ISOK_ != snmpGet(OID2[6] , pSnmpIp, pData2->UpsData[3])) break; //출력 전압 T
//            if (_ISOK_ != snmpGet(OID2[7] , pSnmpIp, pData->UpsOutCur  )) break; //출력 전류(%) R
//            if (_ISOK_ != snmpGet(OID2[8] , pSnmpIp, pData2->UpsData[4])) break; //출력 전류(%) S
//            if (_ISOK_ != snmpGet(OID2[9] , pSnmpIp, pData2->UpsData[5])) break; //출력 전류(%) T
//            if (_ISOK_ != snmpGet(OID2[10], pSnmpIp, pData->UpsBatVol  )) break; //배터리 전압
//            if (_ISOK_ != snmpGet(OID2[11], pSnmpIp, pData->UpsBatTemp )) break; //배터리 온도
//            if (_ISOK_ != snmpGet(OID2[12], pSnmpIp, pData->UpsStatus  )) break; //UpsStatus
//
//            pData->UpsInVol    /= 10;
//            pData2->UpsData[0] /= 10;
//            pData2->UpsData[1] /= 10;
//            pData->UpsInFreq   /= 10;
//            pData->UpsOutVol   /= 10;
//            pData2->UpsData[2] /= 10;
//            pData2->UpsData[3] /= 10;
//            pData->UpsOutCur   /= 10;
//            pData2->UpsData[4] /= 10;
//            pData2->UpsData[5] /= 10;
//            pData->UpsBatVol   *= 1;
//            pData->UpsBatTemp  *= 10;
//        }   else
//        {
//            pLog.Print(LOG_WARNING, 0, NULL, 0, "%s()[%d] Snmp Phase Number Unknown = (%u) !!! [serial = %010u]", __FUNCTION__, __LINE__, phase, m_DevInfo.serial);
//            return _FAIL_;
//        }
//        if (pData->UpsStatus == 6)
//            pData->UpsStatus = APP_MBUS_SBIT_BYPASS_ON;
//        else
//        if (pData->UpsStatus == 4)
//            pData->UpsStatus = APP_MBUS_SBIT_BAT_LOW;
//        else
//            pData->UpsStatus = 0;
//
//        pLog.Print(LOG_INFO , 0, NULL, 0, "\x1b[1;36m%s() snmpGet Completed (serial = %010u) !!!\x1b[0m", __FUNCTION__, m_DevInfo.serial);
//        return _ISOK_;
//    }   while(false);
//
//    pLog.Print(LOG_ERROR, 0, NULL, 0, "\x1b[1;36m%s() snmpGet Failed (serial = %010u) !!!\x1b[0m", __FUNCTION__, m_DevInfo.serial);
//    return _FAIL_;
//}


//static inline const uint8_t snmpGetEx(char * pOid, char * pIp, uint16_t * pv)
//uint8_t CPollingSMP::snmpGetEx(char * pOid, char * pIp, uint16_t * pv)
//{
//    FILE  * fp = NULL;
//    char    pBuff[16 ] = {0,};
//    char    pCmds[600] = {0,};
//
//    snprintf(pCmds, sizeof(pCmds), "snmpget -v 2c -c public -t 3 -r 0 %s %s |awk -F' ' '{print $4}'", pIp, pOid);
//    // excute command
//    fp = popen(pCmds, "r");
//    if(!fp)
//    {
//        pLog.Print(LOG_ERROR, 0, NULL, 0, "\x1b[1;34m%s() popen error [%d:%s] (pCmds = %s)\x1b[0m", __FUNCTION__, errno, strerror(errno), pCmds);
//        return _FAIL_;
//    }
//
//    uint8_t icnt = 0;
//    while(fgets(pBuff, 16, fp) != NULL)
//    {
//        //if (strlen(pBuff) <= 0) break;
//        pv[icnt] = atoi(pBuff);
//        icnt ++;
//    }
//
//    int ret = pclose(fp);
//    if (ret)
//        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s() pclose error [%d:%s] (%d)", __FUNCTION__, errno, strerror(errno), ret);
//
//    if (icnt)
//        return _ISOK_;
//    else {
//        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] (%s)", __FUNCTION__, __LINE__, pCmds);
//        return _FAIL_;
//    }
//}


// snmpget -v 2c -c public -t 1 1.241.172.141  .1.3.6.1.4.1.935.1.1.1.3.2.1.0 |awk -F' ' '{print $4}'
//char OIDStr1[256] = {".1.3.6.1.4.1.935.1.1.1.3.2.1.0 .1.3.6.1.4.1.935.1.1.1.3.2.4.0 .1.3.6.1.4.1.935.1.1.1.4.2.1.0 .1.3.6.1.4.1.935.1.1.1.4.2.3.0 .1.3.6.1.4.1.935.1.1.1.2.2.2.0 .1.3.6.1.4.1.935.1.1.1.2.2.3.0 .1.3.6.1.4.1.935.1.1.1.4.1.1.0"};
//char OIDStr2[512] = {".1.3.6.1.4.1.935.1.1.1.8.2.2.0 .1.3.6.1.4.1.935.1.1.1.8.2.3.0 .1.3.6.1.4.1.935.1.1.1.8.2.4.0 .1.3.6.1.4.1.935.1.1.1.8.2.1.0 .1.3.6.1.4.1.935.1.1.1.8.3.2.0 .1.3.6.1.4.1.935.1.1.1.8.3.3.0 .1.3.6.1.4.1.935.1.1.1.8.3.4.0 .1.3.6.1.4.1.935.1.1.1.8.3.5.0 .1.3.6.1.4.1.935.1.1.1.8.3.6.0 .1.3.6.1.4.1.935.1.1.1.8.3.7.0 .1.3.6.1.4.1.935.1.1.1.8.1.1.0 .1.3.6.1.4.1.935.1.1.1.8.1.5.0 .1.3.6.1.4.1.935.1.1.1.4.1.1.0"};

//uint8_t CPollingSMP::SnmpGetDataEx(DATA * pData, DATA2 * pData2, char * pSnmpIp, uint8_t phase)
//{
//    if (strlen(pSnmpIp) <= 0)
//    {
//        pLog.Print(LOG_WARNING, 0, NULL, 0, "%s()[%d] Snmp_Ip is Empty !!! [serial = %010u]", __FUNCTION__, __LINE__, m_DevInfo.serial);
//        return _FAIL_;
//    }
//    do  {
//        pData->UpsStatus = 0;
//        if (phase == 1)     // 단상 UPS
//        {
//            //char OIDStr1[256] = {0,};
//            //for (int i = 0; i < CALL_OID_NUMBER; i++)
//            //    strcat(OIDStr1, OID[i]);
//            uint16_t setVal[] = {0,0,0,0,0,0,0};
//                                //입력 전압  //입력 주파수 //출력 전압 //출력 전류(%) //배터리 전압 //배터리 온도 //UpsStatus
//            if (_ISOK_ != snmpGetEx (OIDStr1, pSnmpIp, setVal)) break;
//
//            pData->UpsInVol   = setVal[0] / 10;
//            pData->UpsInFreq  = setVal[1] / 10;
//            pData->UpsOutVol  = setVal[2] / 10;
//            pData->UpsOutCur  = setVal[3] ;
//            pData->UpsBatVol  = setVal[4] * 10;
//            pData->UpsBatTemp = setVal[5] * 10;
//            pData->UpsStatus  = setVal[6] ;
//        }   else
//        if (phase == 3)     // 3상 UPS
//        {
//            //char OIDStr2[512] = {0,};
//            //for (int i = 0; i < CALL_OID_NUMBER2; i++)
//            //    strcat(OIDStr2, OID2[i]);
//            uint16_t setVal[] = {0,0,0,0,0,0,0,0,0,0,0,0,0};
//                                //입력 전압 R      //입력 전압 S        //입력 전압 T          //입력 주파수
//                                //출력 전압 R      //출력 전압 S        //출력 전압 T
//                                //출력 전류(%) R   //출력 전류(%) S     //출력 전류(%) T       //배터리 전압       //배터리 온도        //UpsStatus
//            if (_ISOK_ != snmpGetEx(OIDStr2, pSnmpIp, setVal)) break;
//
//            pData->UpsInVol    = setVal[ 0] / 10;
//            pData2->UpsData[0] = setVal[ 1] / 10;
//            pData2->UpsData[1] = setVal[ 2] / 10;
//            pData->UpsInFreq   = setVal[ 3] / 10;
//            pData->UpsOutVol   = setVal[ 4] / 10;
//            pData2->UpsData[2] = setVal[ 5] / 10;
//            pData2->UpsData[3] = setVal[ 6] / 10;
//            pData->UpsOutCur   = setVal[ 7] / 10;
//            pData2->UpsData[4] = setVal[ 8] / 10;
//            pData2->UpsData[5] = setVal[ 9] / 10;
//            pData->UpsBatVol   = setVal[10] * 1;
//            pData->UpsBatTemp  = setVal[11] * 10;
//            pData->UpsStatus   = setVal[12] ;
//        }   else
//        {
//            pLog.Print(LOG_WARNING, 0, NULL, 0, "%s()[%d] Snmp Phase Number Unknown = (%u) !!! [serial = %010u]", __FUNCTION__, __LINE__, phase, m_DevInfo.serial);
//            return _FAIL_;
//        }
//        if (pData->UpsStatus == 6)
//            pData->UpsStatus = APP_MBUS_SBIT_BYPASS_ON;
//        else
//        if (pData->UpsStatus == 4)
//            pData->UpsStatus = APP_MBUS_SBIT_BAT_LOW;
//        else
//            pData->UpsStatus = 0;
//
//        pLog.Print(LOG_INFO , 0, NULL, 0, "\x1b[1;36m%s() snmpGet Completed (serial = %010u) !!!\x1b[0m", __FUNCTION__, m_DevInfo.serial);
//        return _ISOK_;
//    }   while(false);
//
//    pLog.Print(LOG_ERROR, 0, NULL, 0, "\x1b[1;36m%s() snmpGet Failed (serial = %010u) !!!\x1b[0m", __FUNCTION__, m_DevInfo.serial);
//    return _FAIL_;
//}


//uint8_t CPollingSMP::snmpGetLib(char * pOid, char * pIp)
//{
//    netsnmp_session session, *ss;
//    netsnmp_pdu    *pdu    , *response;
//    oid anOID[MAX_OID_LEN];
//    size_t anOID_len;
//
//    netsnmp_variable_list *vars;
//    int count = 1;
//
//    unsigned char comm[] = "public";
//    snmp_sess_init( &session );
//    session.version = SNMP_VERSION_2c;
//    session.community = comm;
//    session.community_len = strlen((char*)session.community);
//    session.peername = pIp;
//    session.timeout = 3;
//    session.retries = 0;
//
//    ss = snmp_open(&session);
//    if (!ss) {
//        snmp_sess_perror("ack", &session);
//        return _FAIL_;
//    }
//
//    pdu = snmp_pdu_create(SNMP_MSG_GET);
//    anOID_len = MAX_OID_LEN;
//    if (!snmp_parse_oid(pOid, anOID, &anOID_len)) {
//        snmp_perror(pOid);
//        return _FAIL_;
//    }
//
//    snmp_add_null_var(pdu, anOID, anOID_len);
//    int status = snmp_synch_response(ss, pdu, &response);
//
//
//    /*
//     * Process the response.
//     */
//    if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR)
//    {
//        /*
//        * SUCCESS: Print the result variables
//        */
//        for(vars = response->variables; vars; vars = vars->next_variable)
//            print_variable(vars->name, vars->name_length, vars);
//
//        /* manipuate the information ourselves */
//        for(vars = response->variables; vars; vars = vars->next_variable) {
//            if (vars->type == ASN_OCTET_STR) {
//                char *sp = (char *)malloc(1 + vars->val_len);
//                memcpy(sp, vars->val.string, vars->val_len);
//                sp[vars->val_len] = '\0';
//                printf("value #%d is a string: %s\n", count++, sp);
//                free(sp);
//            }   else
//                printf("value #%d is NOT a string! Ack!\n", count++);
//        }
//    }   else
//    {
//        /*
//        * FAILURE: print what went wrong!
//        */
//
//        if (status == STAT_SUCCESS)
//            fprintf(stderr, "Error in packet\nReason: %s\n", snmp_errstring(response->errstat));
//        else if (status == STAT_TIMEOUT)
//            fprintf(stderr, "Timeout: No response from %s.\n", session.peername);
//        else
//            snmp_sess_perror("snmpdemoapp", ss);
//    }
//
//    if (response)
//        snmp_free_pdu(response);
//    snmp_close(ss);

//    return _ISOK_;
//}


uint8_t CPollingSMP::setDevRequest(UMSG * pMsg, uint8_t fcode, void * pData, int len)
{
    m_Mutex.lock();
    m_TryTo = true;
    uint8_t ret = SendPacket(pMsg, fcode, pData, len);
    if (_ISOK_ != ret)
    {
        DoStopCapture();
        m_Mutex.unlock();
        if (ret != _NONE_)
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Send Packet Error [%010u] (fcode = %02X) (%d)(%s) !!", __FUNCTION__, __LINE__, m_DevInfo.serial, fcode, errno, strerror(errno));
        m_TryTo = false;
        return ret;
    }
    ret = RecvPacket(pMsg);
    if (_ISOK_ != ret)
    {
        DoStopCapture();
        m_Mutex.unlock();
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Recv Packet Error [%010u] (fcode = %02X) (%d)(%s) !!", __FUNCTION__, __LINE__, m_DevInfo.serial, fcode, errno, strerror(errno));
        m_TryTo = false;
        return ret;
    }
    m_TryTo = false;
    m_Mutex.unlock();
    return _ISOK_;
}

uint8_t CPollingSMP::setKeepAlive(UMSG * pMsg)
{
    pLog.Print(LOG_INFO, 0, NULL, 0, "%s()[%d] Keep Alive [serial = %010u]", __FUNCTION__, __LINE__, m_DevInfo.serial);
    m_Mutex.lock();
    m_TryTo = true;
    uint8_t ret = SendPacket(pMsg, DEV_FCODE_KEEP_ALIVE);
    if (_ISOK_ != ret)
    {
        DoStopCapture();
        m_Mutex.unlock();
        if (ret != _NONE_)
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Send Packet Error [%010u](%d)(%s) !!", __FUNCTION__, __LINE__, m_DevInfo.serial, errno, strerror(errno));
        m_TryTo = false;
        return ret;
    }
    ret = RecvPacket(pMsg);
    if (_ISOK_ != ret)
    {
        DoStopCapture();
        m_Mutex.unlock();
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Recv Packet Error [%010u](%d)(%s) !!", __FUNCTION__, __LINE__, m_DevInfo.serial, errno, strerror(errno));
        m_TryTo = false;
        return ret;
    }
    m_TryTo = false;
    m_Mutex.unlock();
    return _ISOK_;
}

int CPollingSMP::DoMQList (void * param)
{
    /////////////////////////////////////////////
    CPollingSMP * self   = (CPollingSMP*)param  ;
    CLst        * pLst   = NULL                 ;
    UMSG        * pMsg   = self->getMsg()       ;
    uint32_t      size   = 0                    ;
    DEVSOCK     * pDev   = NULL                 ;
    BAT_LIMIT     bLimit                        ;
    _BLIMIT_      bLmt                          ;

    _UPS_LMT_   * pVol   = NULL                 ;
    _UPS_LMT_   * pCur   = NULL                 ;
    _UPS_LMT_   * pTemp  = NULL                 ;
    _UPS_LMT_   * pHeat  = NULL                 ;
    _UPS_LMT_   * pSmke  = NULL                 ;

    TCHECK        timed  = {0,0}                ;
    /////////////////////////////////////////////

    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s() Starting ...", __FUNCTION__);
    while (SDL_SemWait(self->m_MQList.get_semaphore()) == 0)
    {
        while ((pLst = self->m_MQList.get_message()) != NULL)
        {
            switch (pLst->get_type())
            {
                case    DEV_FCODE_GET_DATA:    {
                        self->getDevData(pMsg);
                        break;
                }
                case    DEV_CONNECT_UPDATE:    {
                        pDev = (DEVSOCK*)pLst->get_message (size);

                        self->m_Msg.sock = pDev->sock   ;
                        self->m_DevInfo  = pDev->devinfo;
                        self->m_seqno    = pDev->seqno  ;
                        self->m_Init     = false        ;

                        pLog.Print(LOG_WARNING, 0, NULL, 0, "%s()[%d] Update Connected [serial = %010u]", __FUNCTION__, __LINE__, self->m_DevInfo.serial);
                        break;
                }
                case    DEV_FCODE_SET_LIMIT:   {
                        memset(&bLimit, 0, SZ_BAT_LIMIT);
                        pHeat = (_UPS_LMT_*)self->m_Devdef->evtLimit.getdata(BAT_LMT_HEAT_     );
                        pSmke = (_UPS_LMT_*)self->m_Devdef->evtLimit.getdata(BAT_LMT_SMOKE_    );
                        pVol  = (_UPS_LMT_*)self->m_Devdef->evtLimit.getdata(BAT_CHG_VOL_CUTOFF);
                        pCur  = (_UPS_LMT_*)self->m_Devdef->evtLimit.getdata(BAT_CHG_CUR_CUTOFF);
                        pTemp = (_UPS_LMT_*)self->m_Devdef->evtLimit.getdata(BAT_TEMP_CUTOFF   );

                        bLmt.Max  = (int16_t)pVol->Lmt.Max2 ;
                        bLmt.Min  = (int16_t)pVol->Lmt.Min2 ;
                        bLmt.Time = pVol->Lmt.Time  ;
                        bLimit.ChargeVoltage = bLmt ;

                        bLmt.Max  = (int16_t)pCur->Lmt.Max2 ;
                        bLmt.Min  = (int16_t)pCur->Lmt.Min2 ;
                        bLmt.Time = pCur->Lmt.Time  ;
                        bLimit.ChargeCurrent = bLmt ;

                        bLmt.Max  = (int16_t)pTemp->Lmt.Max2;
                        bLmt.Min  = (int16_t)pTemp->Lmt.Min2;
                        bLmt.Time = pTemp->Lmt.Time ;
                        bLimit.Temp          = bLmt ;

                        if (pHeat->Used == '1')
                        {
                            bLimit.LmtHeatFlag = APP_MBUS_USEBIT_HEAT;
                            self->m_FFlag = 1;
                        }
                        if (pSmke->Used == '1')
                        {
                            bLimit.LmtHeatFlag = APP_MBUS_USEBIT_SMOKE;
                            self->m_FFlag = 2;
                        }
                        if (pHeat->Used == '2' && pSmke->Used == '2')
                        {
                            bLimit.LmtHeatFlag = APP_MBUS_USEBIT_HEAT_SMOKE;
                            self->m_FFlag = 3;
                        }

                        if (pVol->Used  == '1')
                            bLimit.LmtHeatFlag |= APP_MBUS_USEBIT_LMT_VOL;
                        if (pCur->Used  == '1')
                            bLimit.LmtHeatFlag |= APP_MBUS_USEBIT_LMT_CUR;
                        if (pTemp->Used == '1')
                            bLimit.LmtHeatFlag |= APP_MBUS_USEBIT_LMT_TEMP;

                        pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] pHeat->Used = (%c), pSmke->Used = (%c) ...", __FUNCTION__, __LINE__, pHeat->Used, pSmke->Used);
                        pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] bLimit.LmtHeatFlag = (%04x) ...", __FUNCTION__, __LINE__, bLimit.LmtHeatFlag);
                        self->setDevRequest(pMsg, DEV_FCODE_SET_LIMIT, &bLimit, SZ_BAT_LIMIT);
                        break;
                }
                case    DEV_FCODE_KEEP_ALIVE:  {
                        self->setKeepAlive(pMsg);
                        break;
                }
                case    DEV_FCODE_SET_HEAT_RESET:
                case    DEV_FCODE_UPS_WAKEUP:
                case    DEV_FCODE_UPS_RESET:   {
                        self->setDevRequest(pMsg, pLst->get_type());
                        break;
                }
                case    DEV_FCODE_SET_BAT_BUZ:
                case    DEV_FCODE_UPS_INVERT:
                case    DEV_FCODE_UPS_BUZZER:
                case    DEV_FCODE_SET_SWITCH:  {
                        ONOFF * OnOff = (ONOFF*)pLst->get_message (size);
                        if (OnOff)
                            self->setDevRequest(pMsg, pLst->get_type(), OnOff, SZ_ONOFF);
                        break;
                }

                case    _NET_STOP_START_:  {
                        time(&timed.last);
                        break;
                }
                case    _NET_STOP_ING_:  {
                        time(&timed.curt);
                        if (difftime(timed.curt, timed.last) >= pcfg.getNetTimeout())
                        {
                            self->NetworkAlarm();
                            self->m_NetStop = false;
                        }
                        break;
                }

                case    _CLOSE_:    {
                        self->DoStopCapture();
                        delete pLst ;
                        pLst = NULL ;
                        return 0    ;
                }
            }

            delete pLst  ;
            pLst = NULL  ;
        }
    }
    return 0;
}

void CPollingSMP::Dev_Update(ClntInfo sock, DEVINFO devInfo, uint32_t seqno)
{
    DoStopCapture() ;
    memset(m_Devsock, 0, SZ_DEVSOCK);
    m_Devsock->sock    = sock       ;
    m_Devsock->devinfo = devInfo    ;
    m_Devsock->seqno   = seqno      ;
    m_MQList.send_message(DEV_CONNECT_UPDATE, m_Devsock, SZ_DEVSOCK);
}

void CPollingSMP::Dev_SnmpUpdate(DEVINFO devInfo, uint32_t seqno)
{
    DoStopCapture() ;
    memset(m_Devsock, 0, SZ_DEVSOCK);
    m_Devsock->devinfo = devInfo    ;
    m_Devsock->seqno   = seqno      ;
    m_MQList.send_message(DEV_CONNECT_UPDATE, m_Devsock, SZ_DEVSOCK);
}

void CPollingSMP::DoStartCapture()
{
    if (m_Source) {
        return;
    }
    m_Source  = true ;
    m_NetStop = false;
}

void CPollingSMP::DoStopCapture()
{
    if (!m_Source) {
        return;
    }
    m_Source = false;
    m_Keep   = false;

    epCloseN(&m_Msg.sock, true);

    m_NetStop = true;
    m_MQList.send_message(_NET_STOP_START_, NULL, 0);
    pLog.Print(LOG_WARNING, 0, NULL, 0, "%s()[%d] Device Closed [serial = %010u]", __FUNCTION__, __LINE__, m_DevInfo.serial);
}

void CPollingSMP::NetworkAlarm()
{
    //  Device Control 네트워크 끊김 알림.
    memset(&m_GetData->Data , 0, SZ_DATA );
    memset(&m_GetData->Data2, 0, SZ_DATA2);
    m_GetData->Data.EventBits = APP_MBUS_EVT_NETWORK;
    m_GetData->LmtEvtBits = 0  ;
    time(&m_GetData->timestamp);
    m_GetData->timestamp -= 1  ;

    subBatCheck ((m_GetData->Data.EventBits & APP_MBUS_EVT_NETWORK), &m_BatChk[EVT_NETWORK], m_Events, 230);

    m_SndMsg[m_seqno]->SendDB((char*)m_GetData, SZ_GETDATA);
    m_SndMsg[m_seqno]->SendNJ((char*)m_GetData, SZ_NJSMSG );
}

static inline const uint32_t HexToBin(char ch)
{
    uint32_t value;

    if ((ch >= '0') && (ch <= '9')) {
        value = ch - '0';
    }
    else if ((ch >= 'A') && (ch <= 'F')) {
        value = ch - 'A' + 10;
    }
    else {
        value = ch - 'a' + 10;
    }

    return (value);
}

static inline const void HexStringToBin(char * buf, int size, char *out)
{
    int i, j;
    uint8_t value;

    for (i = 0; i < size/2; i++) {
        value = 0;
        for (j = 0; j < 2; j++) {
            value <<= (j*4);
            value |= HexToBin(buf[i*2+j]);
        }
        out[i] = value;
    }
}

static inline const void LoraDataCopy (DATA * pData, LORA * pLora)
{
    memset(pData, 0, SZ_DATA);
    //////////////////////////////////////
    pData->UpsInVol   = pLora->UpsInVol  ;
    pData->UpsInFreq  = pLora->UpsInFreq ;
    pData->UpsOutVol  = pLora->UpsOutVol ;
    pData->UpsOutCur  = pLora->UpsOutCur ;
    pData->UpsBatVol  = pLora->UpsBatVol ;
    pData->UpsBatTemp = pLora->UpsBatTemp;
    pData->UpsStatus  = pLora->UpsStatus ;
    //////////////////////////////////////
    pData->BatVol     = pLora->BatVol    ;
    pData->BatCur     = pLora->BatCur    ;
    pData->BatEnvTemp = pLora->BatEnvTemp;
    pData->BatTemp[0] = pLora->BatTemp   ;
    pData->BatStatus  = pLora->BatStatus ;
    //////////////////////////////////////
    pData->EventBits  = pLora->EventBits ;
}

uint8_t CPollingSMP::uptLoraData(GETLORA * pLora)
{
    time(&m_Lora.curt);
    m_Lora.last = pLora->loratime;
    if (difftime(m_Lora.curt, m_Lora.last) >= pcfg.getLoraFailTime())
    {
        m_IsLora = false;
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Lora Timeout !!!, Lora_loraserial = [%s], [serial = %010u]", __FUNCTION__, __LINE__, pLora->loraserial, m_DevInfo.serial);
        return _ISOK_;
    }   else
    {
        m_IsLora = true ;
    }

    //  Network problem !!  let's do it
    if (m_Source == false && m_NetStop == false)
    {
        pLog.Print(LOG_WARNING, 0, NULL, 0, "%s()[%d] Lora Update Start   !!!, Lora_loraserial = [%s], [serial = %010u]", __FUNCTION__, __LINE__, pLora->loraserial, m_DevInfo.serial);
        if (m_Init == false)    //  Device 초기화 과정...
        {
            CDBInterface lDBInf(pcfg.getMyDB());
            if (lDBInf.DBConnect())
            {
                uint8_t ret = lDBInf.getDevdef(&m_DevInfo, m_Devdef, true);
                if (ret != _FAIL_)
                {
                    if (ret == _ISOK_)  //  활성화 상태. (등록된 Device)
                    {
                        DoStartCapture();
                        memcpy(m_Events->bcode, m_Devdef->bcode, sizeof(m_Events->bcode));
                        memcpy(m_Events->bname, m_Devdef->bname, sizeof(m_Events->bname));
                        EvtchkRefresh();
                    }   else            //  최초 접속 or POOL 등록 상태 >> _SUCCESS_ : 미 활성 상태
                    {
                        m_Keep = true;
                    }
                    m_Init = true;
                    //////////////////////////////////////////////////////////////////
                    m_GetData->serial = m_DevInfo.serial;
                    m_Events->serial  = m_DevInfo.serial;
                    //////////////////////////////////////////////////////////////////
                    lDBInf.DBClose();
                    SDL_Delay(1000);
                }   else
                {
                    lDBInf.DBClose();
                    pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Init Failed !!!, Lora_loraserial = [%s], [serial = %010u]", __FUNCTION__, __LINE__, pLora->loraserial, m_DevInfo.serial);
                    return _FAIL_;
                }
            }   else
            {
                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "%s()[%d] Init Failed !!!, Lora_loraserial = [%s], [serial = %010u]", __FUNCTION__, __LINE__, pLora->loraserial, m_DevInfo.serial);
                return _FAIL_;
            }
        }
        if (!memcmp(pLora->Data, "          ", 10))
        {
            pLog.Print(LOG_WARNING, 1, pLora->Data, SZ_LORA2, "%s()[%d] pLora->Data !!!, Lora_loraserial = [%s], [serial = %010u]", __FUNCTION__, __LINE__, pLora->loraserial, m_DevInfo.serial);
            return _ISOK_;
        }

        HexStringToBin(pLora->Data, SZ_LORA2, (char*)&m_LData);
        //memset(&m_GetData->Data2, 0, SZ_DATA2);
        LoraDataCopy(&m_GetData->Data, &m_LData);

        pLog.Print(LOG_WARNING, 0, NULL, 0, "%s()[%d] Lora Update Success !!!, Lora_loraserial = [%s], [serial = %010u]", __FUNCTION__, __LINE__, pLora->loraserial, m_DevInfo.serial);
        return getDevDataPost();
    }   else
        return _ISOK_;
}


int CPollingSMP::DoLQList (void * param)
{
    /////////////////////////////////////////////
    CPollingSMP * self   = (CPollingSMP*)param  ;
    CLst        * pLst   = NULL                 ;
    uint32_t      size   = 0                    ;
    /////////////////////////////////////////////

    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s() Starting ...", __FUNCTION__);
    while (SDL_SemWait(self->m_LQList.get_semaphore()) == 0)
    {
        while ((pLst = self->m_LQList.get_message()) != NULL)
        {
            if (!pLst->get_type())
            {
                delete pLst ;
                pLst = NULL ;
                return 0    ;
            }

            GETLORA * pLora = (GETLORA*)pLst->get_message (size);
            self->uptLoraData(pLora);

            delete pLst  ;
            pLst = NULL  ;
        }
    }
    return 0;
}

void CPollingSMP::Lora_Message(GETLORA * pLora)
{
    m_LQList.send_message(1, pLora, SZ_GETLORA);
}

uint8_t CPollingSMP::Njs_Command(NJSCMD * pNjsCmd)
{
    if (m_Devdef->aflag == _SNMP_TYPE_ && pNjsCmd->fcode != NJS_FCODE_SET_LIMIT)
    {
        snprintf(pNjsCmd->eMsg.emsg, MIN_BUF, "this session is only SNMP (0x%02X)", pNjsCmd->fcode);
        pNjsCmd->eMsg.elen = strlen(pNjsCmd->eMsg.emsg);
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] this session is only SNMP !!!!!!!! (serial = %010u) !!", __FUNCTION__, __LINE__, pNjsCmd->did.serial);
        return _FAIL_;
    }

    if (pNjsCmd->fcode == NJS_FCODE_GET_CONFIG )
        return getUDPRequest(UDP_FCODE_GET_CONFIG , &pNjsCmd->info, &pNjsCmd->eMsg);
    if (pNjsCmd->fcode == NJS_FCODE_GET_NETWORK)
        return getUDPRequest(UDP_FCODE_GET_NETWORK, &pNjsCmd->info, &pNjsCmd->eMsg);
    if (pNjsCmd->fcode == NJS_FCODE_SET_CONFIG )
        return setUDPRequest(UDP_FCODE_SET_CONFIG , &pNjsCmd->info, &pNjsCmd->scfg.cfg, &pNjsCmd->eMsg);
    if (pNjsCmd->fcode == NJS_FCODE_SET_NETWORK)
        return setUDPRequest(UDP_FCODE_SET_NETWORK, &pNjsCmd->info, &pNjsCmd->snet.net, &pNjsCmd->eMsg);
    if (pNjsCmd->fcode == NJS_FCODE_AFW_UPDATE )
        return firmwareUpdate(UDP_FCODE_FW_UPDATE , &pNjsCmd->fwu , &pNjsCmd->eMsg);

    if (m_Devdef->aflag != _SNMP_TYPE_ && m_Init == false)
    {
        snprintf(pNjsCmd->eMsg.emsg, MIN_BUF, "Device not connected or not Initialize (serial = %010u)", pNjsCmd->did.serial);
        pNjsCmd->eMsg.elen = strlen(pNjsCmd->eMsg.emsg);
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Device not connected or not Initialize (serial = %010u) !!", __FUNCTION__, __LINE__, pNjsCmd->did.serial);
        return _FAIL_;
    }

    uint8_t ret = _ISOK_;
    if (pNjsCmd->fcode == NJS_FCODE_SET_LIMIT)
    {
        //설정이 변경됐을때 DB 업데이트 처리,,
        CDBInterface lDBInf(pcfg.getMyDB());
        if (lDBInf.DBConnect())
        {
            ret = lDBInf.getDevdef(&m_DevInfo, m_Devdef);
            if (ret == _ISOK_)
            {
                m_GetData->serial = m_DevInfo.serial;
                m_Events->serial  = m_DevInfo.serial;

                //////////////////////////////////////////////////////////////////
                if (m_Devdef->aflag != _SNMP_TYPE_)
                    m_MQList.send_message(DEV_FCODE_SET_LIMIT, NULL, 0);
                memcpy(m_Events->bcode, m_Devdef->bcode, sizeof(m_Events->bcode));
                memcpy(m_Events->bname, m_Devdef->bname, sizeof(m_Events->bname));
                //////////////////////////////////////////////////////////////////
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] IsConfig Update", __FUNCTION__, __LINE__);
            }   else
            {
                pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Update Failed !!!!! ", __FUNCTION__, __LINE__);
                snprintf(pNjsCmd->eMsg.emsg, MIN_BUF, "Update Failed !!!!");
                pNjsCmd->eMsg.elen = strlen(pNjsCmd->eMsg.emsg);
                ret = _FAIL_;
            }
            lDBInf.DBClose();
        }   else
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Database Not Connedted !!!!! ", __FUNCTION__, __LINE__);
            snprintf(pNjsCmd->eMsg.emsg, MIN_BUF, "Database Not Connedted !!!!");
            pNjsCmd->eMsg.elen = strlen(pNjsCmd->eMsg.emsg);
            ret = _FAIL_;
        }
    }   else
    if (pNjsCmd->fcode == NJS_FCODE_DEV_ACTIVE)
    {
        //  Device 활성화 Active 시 동작..
        ///////////////////////////////////////////////////////////
        CDBInterface lDBInf(pcfg.getMyDB());
        if (lDBInf.DBConnect())
        {
            ret = lDBInf.setDevAct(&m_DevInfo, m_Devdef);
            if (ret == _ISOK_)
            {
                DoStartCapture();
                m_MQList.send_message(DEV_FCODE_SET_LIMIT, NULL, 0);
                memcpy(m_Events->bcode, m_Devdef->bcode, sizeof(m_Events->bcode));
                memcpy(m_Events->bname, m_Devdef->bname, sizeof(m_Events->bname));
                //EvtchkRefresh();
                m_Keep = false;
            }   else
            {
                pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Active Failed !!!!! ", __FUNCTION__, __LINE__);
                snprintf(pNjsCmd->eMsg.emsg, MIN_BUF, "Active Failed !!!!");
                pNjsCmd->eMsg.elen = strlen(pNjsCmd->eMsg.emsg);
                ret = _FAIL_;
            }
            lDBInf.DBClose();
        }   else
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Database Not Connedted !!!!! ", __FUNCTION__, __LINE__);
            snprintf(pNjsCmd->eMsg.emsg, MIN_BUF, "Database Not Connedted !!!!");
            pNjsCmd->eMsg.elen = strlen(pNjsCmd->eMsg.emsg);
            ret = _FAIL_;
        }
    }   else
    if (pNjsCmd->fcode == NJS_FCODE_UPS_RESET || pNjsCmd->fcode == NJS_FCODE_SET_HEAT_RESET || pNjsCmd->fcode == NJS_FCODE_UPS_WAKEUP)
    {
        ret = setDevRequest(&m_Msg, pNjsCmd->fcode);
    }   else
    if (pNjsCmd->fcode == NJS_FCODE_UPS_INVERT || pNjsCmd->fcode == NJS_FCODE_UPS_BUZZER ||
        pNjsCmd->fcode == NJS_FCODE_SET_SWITCH || pNjsCmd->fcode == NJS_FCODE_SET_BAT_BUZ)
    {
        ONOFF OnOff;
        OnOff.onoff = pNjsCmd->value;
        ret = setDevRequest(&m_Msg, pNjsCmd->fcode, &OnOff, SZ_ONOFF);
    }   else
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Njs Command not found (0x%02X) !!", __FUNCTION__, __LINE__, pNjsCmd->fcode);
        snprintf(pNjsCmd->eMsg.emsg, MIN_BUF, "Njs Command not found (0x%02X)", pNjsCmd->fcode);
        pNjsCmd->eMsg.elen = strlen(pNjsCmd->eMsg.emsg);
        ret = _FAIL_;
    }
    return ret;
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
    }   while (0);
    return result;
}

static int ReadableSocket(int socket, unsigned char* buffer, unsigned bufferSize,
           struct sockaddr_in& fromAddress, struct timeval* timeout)
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
        bytesRead = recvfrom(socket, (char*)buffer, bufferSize, 0,
                 (struct sockaddr*)&fromAddress,
                 &addressSize);
        if (bytesRead < 0)
        {
            break;
        }
    }   while (0);
    return bytesRead;
}

static const uint8_t UDP_Send(int socket, UMSG * pMsg, uint8_t fcode, struct sockaddr_in& remote, void * pData = NULL, uint16_t pLen = 0)
{
    socklen_t addrlen = sizeof remote ;
    uint16_t tlen = MakePacket(pMsg, fcode, pData, pLen);
    if (sendto (socket, pMsg->msg, tlen, 0, (struct sockaddr*)&remote, addrlen) <= 0)
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] UDP sendto error (%s) !!!", __FUNCTION__, __LINE__, strerror(errno));
        return _FAIL_;
    }
    return _ISOK_;
}

static const uint8_t UDP_Recv(int socket, UMSG * pMsg, struct sockaddr_in& remote, RETMSG * eMsg)
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    struct timeval tv = {_UDP_TIMEOUT_,0};
    memset(pMsg->msg, 0, MIN_BUF);
    int bytesRead = ReadableSocket(socket, (unsigned char*)pMsg->msg, MIN_BUF, remote, &tv);
    if (bytesRead <= 0)
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s[%d] UDP Recv Error (%d)", __FUNCTION__, __LINE__, bytesRead);
        return _FAIL_;
    }

    HMSG  * head = (HMSG*)pMsg->msg ;
    char  * body = (char*)&pMsg->msg[SZ_HMSG];
    if (head->sop == _SMP_PKT_SOP_)
    {
        if (head->fcode == UDP_FCODE_RES_ERROR)
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s[%d] head->fcode = [%02x] !!!!!!!!!!!!!! ERROR !!!!!!!!!!!!!!", __FUNCTION__, __LINE__, head->fcode);
            return _FAIL_;
        }
        uint16_t blen = htons(head->length);
        blen -= 1;
        TMSG * tail = (TMSG*)&body[blen];
        if (tail->eop == _SMP_PKT_EOP_)
        {
            if (head->fcode == UDP_FCODE_FW_PACKET || head->fcode == UDP_FCODE_FW_VERIFY)
            {
                if (blen == SZ_PKT)
                {
                    PKT * pkt = (PKT*)eMsg->emsg    ;
                    memcpy(pkt, body, SZ_PKT)       ;
                    eMsg->elen  = SZ_PKT            ;
                }   else
                {
                    pLog.Print(LOG_ERROR, 0, NULL, 0, "%s[%d] blen(%d) <> plen(%d) not equeal size", __FUNCTION__, __LINE__, blen, SZ_PKT);
                    return _FAIL_;
                }
            }   else
            {
                if (blen == SZ_DID)
                {
                    DID * did = (DID*)eMsg->emsg    ;
                    memcpy(did, body, SZ_DID)       ;
                    eMsg->elen  = SZ_DID            ;
                }   else
                {
                    pLog.Print(LOG_ERROR, 0, NULL, 0, "%s[%d] blen(%d) <> plen(%d) not equeal size", __FUNCTION__, __LINE__, blen, SZ_DID);
                    return _FAIL_;
                }
            }
        }   else
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] End of Packet failed :: EOP = [%02X]", __FUNCTION__, __LINE__, tail->eop);
            return _FAIL_;
        }
    }   else
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Start of Packet failed :: SOP = [%02X]", __FUNCTION__, __LINE__, head->sop);
        return _FAIL_;
    }
    return _ISOK_;
}

uint8_t CPollingSMP::getUDPRequest(uint8_t fcode, INFO * info, RETMSG * eMsg)
{
    char    IPs [17] = {0,} ;
    UMSG    pMsg            ;
    DID     did             ;
    int     socket = -1     ;

    struct  timeval tv = {4,0};
    struct  sockaddr_in remoteName              ;
    socklen_t addrlen = sizeof remoteName       ;

    snprintf(IPs , sizeof(IPs) , "%u.%u.%u.%u", info->devip[0], info->devip[1], info->devip[2], info->devip[3]);
    did.serial  = info->serial                  ;

    while(socket < 0) socket = setupUDPSocket() ;

    remoteName.sin_family      = AF_INET        ;
    remoteName.sin_port        = htons(8503)    ;
    remoteName.sin_addr.s_addr = inet_addr(IPs) ;

    memset(&pMsg, 0, SZ_UMSG);
    uint16_t tlen = MakePacket(&pMsg, fcode, &did, SZ_DID);
    pLog.Print(LOG_DEBUG, 1, pMsg.msg, tlen, "%s()[%d] UDP To Device Send message !!!", __FUNCTION__, __LINE__);
    if (sendto (socket, pMsg.msg, tlen, 0, (struct sockaddr*)&remoteName, addrlen) <= 0)
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] UDP sendto error (%s) !!!", __FUNCTION__, __LINE__, strerror(errno));
        close(socket);
        snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] UDP send error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
        eMsg->elen = strlen(eMsg->emsg);
        return _FAIL_;
    }

    ///////////////////////////////////////////////////////////////////////////
    memset(pMsg.msg, 0, tlen);
    int bytesRead = ReadableSocket(socket, (unsigned char*)pMsg.msg, MID_BUF, remoteName, &tv);
    if (bytesRead <= 0)
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s[%d] UDP Recv Error (%d)", __FUNCTION__, __LINE__, bytesRead);
        close(socket);
        snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] UDP recv error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
        eMsg->elen = strlen(eMsg->emsg);
        return _FAIL_;
    }

    HMSG  * head = (HMSG*)pMsg.msg ;
    char  * body = (char*)&pMsg.msg[SZ_HMSG];
    if (head->sop == _SMP_PKT_SOP_)
    {
        if (head->fcode == UDP_FCODE_RES_ERROR)
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s[%d] head->fcode = [%02x] !!!!!!!!!!!!!! ERROR !!!!!!!!!!!!!!", __FUNCTION__, __LINE__, head->fcode);
            close(socket);
            snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] Controller error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
            eMsg->elen = strlen(eMsg->emsg);
            return _FAIL_;
        }
        uint16_t blen = htons(head->length);
        blen -= 1;
        TMSG * tail   = (TMSG*)&body[blen];
        if (tail->eop == _SMP_PKT_EOP_)
        {
            if (fcode == UDP_FCODE_GET_CONFIG)
            {
                if (blen == SZ_DEVCFG)
                {
                    DEVCFG  dcfg;
                    memset(&dcfg, 0, SZ_DEVCFG)     ;
                    SCFG * rcfg = (SCFG*)eMsg->emsg ;
                    memcpy(&dcfg, body, SZ_DEVCFG)  ;
                    eMsg->elen  = SZ_SCFG           ;
                    ////////// copy to nodejs ///////
                    rcfg->serial = dcfg.serial      ;
                    memcpy(rcfg->cfg.serverip, dcfg.serverip, sizeof(rcfg->cfg.serverip));
                    rcfg->cfg.serverport = dcfg.serverport;
                    snprintf((char*)rcfg->cfg.localcode   , sizeof(rcfg->cfg.localcode)   , "%04u", dcfg.localcode);
                    snprintf((char*)rcfg->cfg.umsserial   , sizeof(rcfg->cfg.umsserial)   , "%u", dcfg.umsserial);
                    snprintf((char*)rcfg->cfg.upsserial   , sizeof(rcfg->cfg.upsserial)   , "%u", dcfg.upsserial);
                    snprintf((char*)rcfg->cfg.batserial   , sizeof(rcfg->cfg.batserial)   , "%u", dcfg.batserial);
                    snprintf((char*)rcfg->cfg.batboxserial, sizeof(rcfg->cfg.batboxserial), "%u", dcfg.batboxserial);
                    memcpy(rcfg->cfg.loraserial, dcfg.loraserial, sizeof(rcfg->cfg.loraserial));
                    rcfg->cfg.loraserial[16] = '\0';

                    pLog.Print(LOG_INFO , 0, NULL, 0, "rcfg->serial           = [%010u]", rcfg->serial );
                    pLog.Print(LOG_DEBUG, 0, NULL, 0, "rcfg->cfg.serverip     = [%u.%u.%u.%u]", rcfg->cfg.serverip[0], rcfg->cfg.serverip[1], rcfg->cfg.serverip[2], rcfg->cfg.serverip[3]);
                    pLog.Print(LOG_DEBUG, 0, NULL, 0, "rcfg->cfg.serverport   = [%u]", rcfg->cfg.serverport);
                    pLog.Print(LOG_DEBUG, 0, NULL, 0, "rcfg->cfg.localcode    = [%s]", (char*)rcfg->cfg.localcode   );
                    pLog.Print(LOG_DEBUG, 0, NULL, 0, "rcfg->cfg.umsserial    = [%s]", (char*)rcfg->cfg.umsserial   );
                    pLog.Print(LOG_DEBUG, 0, NULL, 0, "rcfg->cfg.upsserial    = [%s]", (char*)rcfg->cfg.upsserial   );
                    pLog.Print(LOG_DEBUG, 0, NULL, 0, "rcfg->cfg.batserial    = [%s]", (char*)rcfg->cfg.batserial   );
                    pLog.Print(LOG_DEBUG, 0, NULL, 0, "rcfg->cfg.batboxserial = [%s]", (char*)rcfg->cfg.batboxserial);
                    pLog.Print(LOG_DEBUG, 0, NULL, 0, "rcfg->cfg.loraserial   = [%s]", (char*)rcfg->cfg.loraserial  );
                }   else
                {
                    pLog.Print(LOG_ERROR, 0, NULL, 0, "%s[%d] blen(%d) <> plen(%d) not equeal size", __FUNCTION__, __LINE__, blen, SZ_DEVCFG);
                    close(socket);
                    snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] packet size error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
                    eMsg->elen = strlen(eMsg->emsg);
                    return _FAIL_;
                }
            }   else
            if (fcode == UDP_FCODE_GET_NETWORK)
            {
                if (blen == SZ_SNET)
                {
                    SNET * gnet = (SNET*)eMsg->emsg ;
                    memcpy(gnet, body, SZ_SNET)     ;
                    eMsg->elen  = SZ_SNET           ;
                    pLog.Print(LOG_INFO , 0, NULL, 0, "gnet->serial         = [%010u]", gnet->serial  );
                    pLog.Print(LOG_DEBUG, 0, NULL, 0, "gnet->net.dhcp       = [%u]"   , gnet->net.dhcp);
                    pLog.Print(LOG_DEBUG, 0, NULL, 0, "gnet->net.localip    = [%u.%u.%u.%u]", gnet->net.localip[0]   , gnet->net.localip[1]   , gnet->net.localip[2]   , gnet->net.localip[3]   );
                    pLog.Print(LOG_DEBUG, 0, NULL, 0, "gnet->net.subnetmask = [%u.%u.%u.%u]", gnet->net.subnetmask[0], gnet->net.subnetmask[1], gnet->net.subnetmask[2], gnet->net.subnetmask[3]);
                    pLog.Print(LOG_DEBUG, 0, NULL, 0, "gnet->net.gateway    = [%u.%u.%u.%u]", gnet->net.gateway[0]   , gnet->net.gateway[1]   , gnet->net.gateway[2]   , gnet->net.gateway[3]   );
                    pLog.Print(LOG_DEBUG, 0, NULL, 0, "gnet->net.dns        = [%u.%u.%u.%u]", gnet->net.dns[0]       , gnet->net.dns[1]       , gnet->net.dns[2]       , gnet->net.dns[3]       );
                }   else
                {
                    pLog.Print(LOG_ERROR, 0, NULL, 0, "%s[%d] blen(%d) <> plen(%d) not equeal size", __FUNCTION__, __LINE__, blen, SZ_SNET);
                    close(socket);
                    snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] packet size error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
                    eMsg->elen = strlen(eMsg->emsg);
                    return _FAIL_;
                }
            }
        }   else
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] End of Packet failed :: EOP = [%02X]", __FUNCTION__, __LINE__, tail->eop);
            close(socket);
            snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] packet EOP error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
            eMsg->elen = strlen(eMsg->emsg);
            return _FAIL_;
        }
    }   else
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Start of Packet failed :: SOP = [%02X]", __FUNCTION__, __LINE__, head->sop);
        close(socket);
        snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] packet SOP error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
        eMsg->elen = strlen(eMsg->emsg);
        return _FAIL_;
    }
    close(socket);
    return _ISOK_;
}


uint8_t CPollingSMP::setUDPRequest(uint8_t fcode, INFO * info, CFG * cfg, RETMSG * eMsg)
{
    char    IPs [17] = {0,} ;
    UMSG    pMsg            ;
    DEVCFG  dcfg            ;
    int     socket = -1     ;

    struct  timeval tv = {4,0};
    struct  sockaddr_in remoteName          ;
    socklen_t addrlen = sizeof remoteName   ;

    snprintf(IPs , sizeof(IPs) , "%u.%u.%u.%u", info->devip[0], info->devip[1], info->devip[2], info->devip[3]);

    ///  converting for controller
    dcfg.serial = info->serial              ;
    memcpy(dcfg.serverip, cfg->serverip, sizeof(dcfg.serverip));
    dcfg.serverport   = cfg->serverport            ;
    dcfg.localcode    = atoi((char*)cfg->localcode);
    dcfg.umsserial    = atoi((char*)cfg->umsserial);
    dcfg.upsserial    = atoi((char*)cfg->upsserial);
    dcfg.batserial    = atoi((char*)cfg->batserial);
    dcfg.batboxserial = atoi((char*)cfg->batboxserial);
    memcpy(dcfg.loraserial, cfg->loraserial, sizeof(dcfg.loraserial));

    while(socket < 0) socket = setupUDPSocket() ;

    remoteName.sin_family      = AF_INET        ;
    remoteName.sin_port        = htons(8503)    ;
    remoteName.sin_addr.s_addr = inet_addr(IPs) ;

    memset(&pMsg, 0, SZ_UMSG);
    uint16_t tlen = MakePacket(&pMsg, fcode, &dcfg, SZ_DEVCFG);
    pLog.Print(LOG_DEBUG, 1, pMsg.msg, tlen, "%s()[%d] UDP To Device Send message !!!", __FUNCTION__, __LINE__);

    if (sendto (socket, pMsg.msg, tlen, 0, (struct sockaddr*)&remoteName, addrlen) <= 0)
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] UDP sendto error (%s) !!!", __FUNCTION__, __LINE__, strerror(errno));
        close(socket);
        snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] UDP send error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
        eMsg->elen = strlen(eMsg->emsg);
        return _FAIL_;
    }

    ///////////////////////////////////////////////////////////////////////////
    memset(pMsg.msg, 0, tlen);
    int bytesRead = ReadableSocket(socket, (unsigned char*)pMsg.msg, MID_BUF, remoteName, &tv);
    if (bytesRead <= 0)
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s[%d] UDP Recv Error (%d)", __FUNCTION__, __LINE__, bytesRead);
        close(socket);
        snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] UDP recv error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
        eMsg->elen = strlen(eMsg->emsg);
        return _FAIL_;
    }

    HMSG  * head = (HMSG*)pMsg.msg ;
    char  * body = (char*)&pMsg.msg[SZ_HMSG];
    if (head->sop == _SMP_PKT_SOP_)
    {
        if (head->fcode == UDP_FCODE_RES_ERROR)
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s[%d] head->fcode = [%02x] !!!!!!!!!!!!!! ERROR !!!!!!!!!!!!!!", __FUNCTION__, __LINE__, head->fcode);
            close(socket);
            snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] Controller error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
            eMsg->elen = strlen(eMsg->emsg);
            return _FAIL_;
        }
        uint16_t blen = htons(head->length);
        blen -= 1;
        TMSG * tail = (TMSG*)&body[blen];
        if (tail->eop == _SMP_PKT_EOP_)
        {
            if (blen == SZ_DID)
            {
                DID * did = (DID*)eMsg->emsg    ;
                memcpy(did, body, SZ_DID)       ;
                eMsg->elen  = SZ_DID            ;
                pLog.Print(LOG_INFO , 0, NULL, 0, "did->serial = [%010u]", did->serial);
            }   else
            {
                pLog.Print(LOG_ERROR, 0, NULL, 0, "%s[%d] blen(%d) <> plen(%d) not equeal size", __FUNCTION__, __LINE__, blen, SZ_DID);
                close(socket);
                snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] packet size error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
                eMsg->elen = strlen(eMsg->emsg);
                return _FAIL_;
            }
        }   else
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] End of Packet failed :: EOP = [%02X]", __FUNCTION__, __LINE__, tail->eop);
            close(socket);
            snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] packet EOP error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
            eMsg->elen = strlen(eMsg->emsg);
            return _FAIL_;
        }
    }   else
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Start of Packet failed :: SOP = [%02X]", __FUNCTION__, __LINE__, head->sop);
        close(socket);
        snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] packet SOP error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
        eMsg->elen = strlen(eMsg->emsg);
        return _FAIL_;
    }
    close(socket);
    return _ISOK_;
}


uint8_t CPollingSMP::setUDPRequest(uint8_t fcode, INFO * info, NETWORK * net, RETMSG * eMsg)
{
    char    IPs [17] = {0,} ;
    UMSG    pMsg            ;
    DEVNET  dnet            ;
    int     socket = -1     ;

    struct  timeval tv = {4,0};
    struct  sockaddr_in remoteName          ;
    socklen_t addrlen = sizeof remoteName   ;

    snprintf(IPs , sizeof(IPs) , "%u.%u.%u.%u", info->devip[0], info->devip[1], info->devip[2], info->devip[3]);
    ///  converting for controller
    dnet.serial = info->serial  ;
    dnet.dhcp   = net->dhcp     ;
    memcpy(dnet.localip   , net->localip   , sizeof(dnet.localip)   );
    memcpy(dnet.subnetmask, net->subnetmask, sizeof(dnet.subnetmask));
    memcpy(dnet.gateway   , net->gateway   , sizeof(dnet.gateway)   );
    memcpy(dnet.dns       , net->dns       , sizeof(dnet.dns)       );

    while(socket < 0) socket = setupUDPSocket() ;

    remoteName.sin_family      = AF_INET        ;
    remoteName.sin_port        = htons(8503)    ;
    remoteName.sin_addr.s_addr = inet_addr(IPs) ;

    memset(&pMsg, 0, SZ_UMSG);
    uint16_t tlen = MakePacket(&pMsg, fcode, &dnet, SZ_DEVNET);
    pLog.Print(LOG_DEBUG, 1, pMsg.msg, tlen, "%s()[%d] UDP To Device Send message !!!", __FUNCTION__, __LINE__);

    if (sendto (socket, pMsg.msg, tlen, 0, (struct sockaddr*)&remoteName, addrlen) <= 0)
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] UDP sendto error (%s) !!!", __FUNCTION__, __LINE__, strerror(errno));
        close(socket);
        snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] UDP send error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
        eMsg->elen = strlen(eMsg->emsg);
        return _FAIL_;
    }

    ///////////////////////////////////////////////////////////////////////////
    memset(pMsg.msg, 0, tlen);
    int bytesRead = ReadableSocket(socket, (unsigned char*)pMsg.msg, MID_BUF, remoteName, &tv);
    if (bytesRead <= 0)
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s[%d] UDP Recv Error (%d)", __FUNCTION__, __LINE__, bytesRead);
        close(socket);
        snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] UDP recv error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
        eMsg->elen = strlen(eMsg->emsg);
        return _FAIL_;
    }

    HMSG  * head = (HMSG*)pMsg.msg ;
    char  * body = (char*)&pMsg.msg[SZ_HMSG];
    if (head->sop == _SMP_PKT_SOP_)
    {
        if (head->fcode == UDP_FCODE_RES_ERROR)
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s[%d] head->fcode = [%02x] !!!!!!!!!!!!!! ERROR !!!!!!!!!!!!!!", __FUNCTION__, __LINE__, head->fcode);
            close(socket);
            snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] Controller error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
            eMsg->elen = strlen(eMsg->emsg);
            return _FAIL_;
        }
        uint16_t blen = htons(head->length);
        blen -= 1;
        TMSG * tail = (TMSG*)&body[blen];
        if (tail->eop == _SMP_PKT_EOP_)
        {
            if (blen == SZ_DID)
            {
                DID * did = (DID*)eMsg->emsg    ;
                memcpy(did, body, SZ_DID)       ;
                eMsg->elen  = SZ_DID            ;
                pLog.Print(LOG_INFO , 0, NULL, 0, "did->serial = [%010u]", did->serial);
            }   else
            {
                pLog.Print(LOG_ERROR, 0, NULL, 0, "%s[%d] blen(%d) <> plen(%d) not equeal size", __FUNCTION__, __LINE__, blen, SZ_DID);
                close(socket);
                snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] packet size error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
                eMsg->elen = strlen(eMsg->emsg);
                return _FAIL_;
            }
        }   else
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] End of Packet failed :: EOP = [%02X]", __FUNCTION__, __LINE__, tail->eop);
            close(socket);
            snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] packet EOP error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
            eMsg->elen = strlen(eMsg->emsg);
            return _FAIL_;
        }
    }   else
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Start of Packet failed :: SOP = [%02X]", __FUNCTION__, __LINE__, head->sop);
        close(socket);
        snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] packet SOP error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
        eMsg->elen = strlen(eMsg->emsg);
        return _FAIL_;
    }
    close(socket);
    return _ISOK_;
}

static const uint8_t getFirmwareInfo (char * fname, FWUPDATE * pUpdate, uint8_t * pBuff )
{
    if (!access(fname, F_OK))
    {
        int         fp   ;
        struct stat sbuf ;
        if ((fp = open(fname, O_LARGEFILE, O_RDONLY)) < 0)
        {
            pLog.Print (LOG_CRITICAL, 0, NULL, 0, "%s()[%d] file not found (%s)", __FUNCTION__, __LINE__, fname);
            return _FAIL_;
        }
        if (fstat(fp, &sbuf)< 0)
        {
            pLog.Print (LOG_CRITICAL, 0, NULL, 0, "%s()[%d] FILE STAT ERROR (%s)", __FUNCTION__, __LINE__, fname);
            close(fp)    ;
            return _FAIL_;
        }

        pUpdate->fwsize     = sbuf.st_size  ;
        pUpdate->packetsize = MID_BUF       ;
        memset(pBuff, 0, pUpdate->fwsize+1) ;
        if (read(fp, pBuff, pUpdate->fwsize) != pUpdate->fwsize)
        {
            pLog.Print (LOG_CRITICAL, 0, NULL, 0, "%s()[%d] firmware file read error (%s)", __FUNCTION__, __LINE__, strerror(errno));
            close(fp)    ;
            return _FAIL_;
        }
        close(fp);

        /***   Check image file   ***/
        FW_VER_TABLE * pVer = (FW_VER_TABLE*)&pBuff[pUpdate->fwsize - SZ_VTABLE];
        if (memcmp(pVer->MagicCode, "IMG ", 4) != 0)
        {
            pLog.Print (LOG_CRITICAL, 0, NULL, 0, "%s()[%d] firmware version table error", __FUNCTION__, __LINE__);
            return _FAIL_;
        }
        pUpdate->modelcode = pVer->ModelCode;
        pUpdate->fwversion = pVer->FwVersion;

        /***   Make a check sum   ***/
        uint16_t checksum = 0;
        for (uint32_t i = 0; i < pUpdate->fwsize; i++) {
            checksum += pBuff[i];
        }
        checksum = (~checksum);
        pUpdate->checksum = checksum;
    }   else
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] File not found (%s) !!!", __FUNCTION__, __LINE__, fname);
        return _FAIL_;
    }
    return _ISOK_;
}

uint8_t CPollingSMP::firmwareUpdate(uint8_t fcode, FWUPT * fwu, RETMSG * eMsg)
{
    char     IPs [17] = {0,};
    UMSG     pMsg           ;
    FWUPDATE fwUpdate       ;
    int      socket = -1    ;
    uint8_t  fwBuff[_FIRMWARE_SZ_]  ;
    struct   sockaddr_in remoteName ;

    snprintf(IPs , sizeof(IPs) , "%u.%u.%u.%u", fwu->devip[0], fwu->devip[1], fwu->devip[2], fwu->devip[3]);
    fwUpdate.serial = fwu->serial   ;
    //  FirmWare File Reading ..
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] UDP Firmware File Reading ...", __FUNCTION__, __LINE__);
    if (getFirmwareInfo((char*)fwu->filepath, &fwUpdate, fwBuff) != _ISOK_)
    {
        snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] firmware file error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
        eMsg->elen = strlen(eMsg->emsg);
        return _FAIL_;
    }

    pLog.Print(LOG_WARNING, 0, NULL, 0, "%s()[%d] fwUpdate.serial     = [%010u]", __FUNCTION__, __LINE__, fwUpdate.serial    );
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] fwUpdate.modelcode  = [%u]"   , __FUNCTION__, __LINE__, fwUpdate.modelcode );
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] fwUpdate.fwversion  = [%u]"   , __FUNCTION__, __LINE__, fwUpdate.fwversion );
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] fwUpdate.fwsize     = [%u]"   , __FUNCTION__, __LINE__, fwUpdate.fwsize    );
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] fwUpdate.packetsize = [%u]"   , __FUNCTION__, __LINE__, fwUpdate.packetsize);
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] fwUpdate.checksum   = [%04X]" , __FUNCTION__, __LINE__, fwUpdate.checksum  );

    while(socket < 0) socket = setupUDPSocket() ;
    remoteName.sin_family      = AF_INET        ;
    remoteName.sin_port        = htons(8503)    ;
    remoteName.sin_addr.s_addr = inet_addr(IPs) ;

    //  FirmWare Update ...
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] UDP Firmware Update Sending  !!!", __FUNCTION__, __LINE__);
    memset(&pMsg, 0, SZ_UMSG);
    if (UDP_Send(socket, &pMsg, UDP_FCODE_FW_UPDATE, remoteName, &fwUpdate, SZ_FWUPDATE) != _ISOK_)
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] UDP sendto error (%s) !!!", __FUNCTION__, __LINE__, strerror(errno));
        close(socket);
        snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] UDP send error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
        eMsg->elen = strlen(eMsg->emsg);
        return _FAIL_;
    }
    if (UDP_Recv(socket, &pMsg, remoteName, eMsg) != _ISOK_)
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] UDP recv error (%s) !!!", __FUNCTION__, __LINE__, strerror(errno));
        close(socket);
        snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] UDP recv error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
        eMsg->elen = strlen(eMsg->emsg);
        return _FAIL_;
    }
    SDL_Delay(1);

    //  FirmWare Packet ...
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] UDP Firmware Packet Sending !!!", __FUNCTION__, __LINE__);
    FWPACKET    packet  ;
    uint16_t    ppos = 0;
    uint16_t    plen = 0;
    uint16_t    pcnt = ((fwUpdate.fwsize % MID_BUF) == 0 ? (fwUpdate.fwsize / MID_BUF) : (fwUpdate.fwsize / MID_BUF) + 1);
    for (int i = 0; i < pcnt; i++)
    {
        memset(&packet, 0, SZ_FWPACKET);
        packet.pkt.serial = fwUpdate.serial;
        ppos = i * MID_BUF  ;
        plen = fwUpdate.fwsize - ppos;
        if (plen > MID_BUF) {
            plen = MID_BUF;
        }
        packet.pkt.pNum = i ;
        memcpy(packet.pData, &fwBuff[ppos], plen);
        if (UDP_Send(socket, &pMsg, UDP_FCODE_FW_PACKET, remoteName, &packet, (plen + SZ_PKT)) != _ISOK_)
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] UDP sendto error (%s) !!!", __FUNCTION__, __LINE__, strerror(errno));
            close(socket);
            snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] UDP send error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
            eMsg->elen = strlen(eMsg->emsg);
            return _FAIL_;
        }
        if (UDP_Recv(socket, &pMsg, remoteName, eMsg) != _ISOK_)
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] UDP recv error (%s) !!!", __FUNCTION__, __LINE__, strerror(errno));
            close(socket);
            snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] UDP recv error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
            eMsg->elen = strlen(eMsg->emsg);
            return _FAIL_;
        }
        SDL_Delay(1);
    }
    pLog.Print(LOG_WARNING, 0, NULL, 0, "%s()[%d] UDP Firmware Packet Sending End [%010u]", __FUNCTION__, __LINE__, fwUpdate.serial);

    //  FirmWare Verify ...
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] UDP Firmware Verify Sending !!!", __FUNCTION__, __LINE__);
    ppos = 0;
    for (int i = 0; i < pcnt; i++)
    {
        memset(&packet, 0, SZ_FWPACKET);
        packet.pkt.serial = fwUpdate.serial;
        ppos = i * MID_BUF  ;
        plen = fwUpdate.fwsize - ppos;
        if (plen > MID_BUF) {
            plen = MID_BUF;
        }
        packet.pkt.pNum = i ;
        memcpy(packet.pData, &fwBuff[ppos], plen);
        if (UDP_Send(socket, &pMsg, UDP_FCODE_FW_VERIFY, remoteName, &packet, (plen + SZ_PKT)) != _ISOK_)
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] UDP sendto error (%s) !!!", __FUNCTION__, __LINE__, strerror(errno));
            close(socket);
            snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] UDP send error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
            eMsg->elen = strlen(eMsg->emsg);
            return _FAIL_;
        }
        if (UDP_Recv(socket, &pMsg, remoteName, eMsg) != _ISOK_)
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] UDP recv error (%s) !!!", __FUNCTION__, __LINE__, strerror(errno));
            close(socket);
            snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] UDP recv error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
            eMsg->elen = strlen(eMsg->emsg);
            return _FAIL_;
        }
        SDL_Delay(1);
    }
    pLog.Print(LOG_WARNING, 0, NULL, 0, "%s()[%d] UDP Firmware Verify Sending End [%010u]", __FUNCTION__, __LINE__, fwUpdate.serial);

    //  FirmWare Finish ...
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] UDP Firmware Finish Sending !!!", __FUNCTION__, __LINE__);
    DID did ;
    did.serial = fwUpdate.serial;
    if (UDP_Send(socket, &pMsg, UDP_FCODE_FW_FINISH, remoteName, &did, SZ_DID) != _ISOK_)
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] UDP sendto error (%s) !!!", __FUNCTION__, __LINE__, strerror(errno));
        close(socket);
        snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] UDP send error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
        eMsg->elen = strlen(eMsg->emsg);
        return _FAIL_;
    }
    if (UDP_Recv(socket, &pMsg, remoteName, eMsg) != _ISOK_)
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] UDP recv error (%s) !!!", __FUNCTION__, __LINE__, strerror(errno));
        close(socket);
        snprintf(eMsg->emsg, MIN_BUF, "%s()[%d] UDP recv error !!! (0x%02X)", __FUNCTION__, __LINE__, fcode);
        eMsg->elen = strlen(eMsg->emsg);
        return _FAIL_;
    }
    SDL_Delay(1) ;
    pLog.Print(LOG_WARNING, 0, NULL, 0, "%s()[%d] UDP Firmware Finish Sending End [%010u]", __FUNCTION__, __LINE__, fwUpdate.serial);
    close(socket);
    return _ISOK_;
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


void CPollingSMP::freeLimit (void * f)
{
    if (f)
    {
        delete (_UPS_LMT_*)f;
        f = NULL;
    }
}

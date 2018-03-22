#include "DBInterface.h"
extern Mutex     DLock  ;
extern CLstQueue DEVInit;

CDBInterface::CDBInterface(DBINFO pDB)
:   m_Mysql (NULL), m_DBInf(pDB)
{
}

CDBInterface::~CDBInterface()
{
    if (m_Mysql)
    {
        delete m_Mysql;
        m_Mysql = NULL;
    }
}

bool CDBInterface::DBConnect()
{
    DLock.lock();
    if (m_Mysql == NULL)
    {
        m_Mysql = new CMysql();
        if (m_Mysql == NULL)
        {
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s()[%d] Mysql Class Create Error (%s)\x1b[0m", __FUNCTION__, __LINE__, strerror(errno));
            DLock.unlock();
            return false;
        }
    }

    // connect DB
    if (m_Mysql->mysql_connect(m_DBInf.host, m_DBInf.port, m_DBInf.user, m_DBInf.psword, m_DBInf.dbname, m_DBInf.timeout) < 0)
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s()[%d] Mysql Connection Fail (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
        DLock.unlock();
        return false;
    }

    pLog.Print(LOG_DEBUG, 0, NULL, 0, "\x1b[1;33m%s()[%d] Mysql Connect Success !!! \x1b[0m", __FUNCTION__, __LINE__);
    DLock.unlock();
    return true;
}

bool CDBInterface::DBClose()
{
    DLock.lock();
    m_Mysql->mysql_close_conn();
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "\x1b[1;33m%s()[%d] Mysql Disconnect Success !!! \x1b[0m", __FUNCTION__, __LINE__);
    DLock.unlock();
    return true;
}

bool CDBInterface::setAutoCommit(uint8_t commit)
{
    return m_Mysql->mysql_AutoCommit(commit);
}

bool CDBInterface::IsConnected()
{
    return m_Mysql->mysql_check_conn();
}

bool CDBInterface::BeginTran()
{
    if (!m_Mysql->mysql_Start())
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
        return false;
    }   else
        return true;
}

bool CDBInterface::EndTran()
{
    if (!m_Mysql->mysql_Commit())
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
        if (!m_Mysql->mysql_Rollback()) {
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
        }   else
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] RollBack Completed !!!\x1b[0m", __FUNCTION__, __LINE__);
        return false;
    }   else
        return true ;
}

bool CDBInterface::uptDevData (GETDATA * pGetData)
{
    if (!m_Mysql->mysql_Start())
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
        return false;
    }

    char sTmp[SML_BUF] = {0,};
    memset(m_SCQry, 0, sizeof(m_SCQry));
    memset(m_SVQry, 0, sizeof(m_SVQry));
    for (int i = 0; i < BAT_TEMP_CNT; i++)
    {
        snprintf(sTmp, sizeof(sTmp), ",BatTemp%d", i+1);
        strcat(m_SCQry, sTmp);
        snprintf(sTmp, sizeof(sTmp), ",%d", pGetData->Data.BatTemp[i]);
        strcat(m_SVQry, sTmp);
    }
    snprintf(m_Query, sizeof(m_Query), "INSERT INTO TB_DEV_DATA (ID, CREATED, UpsInVol, UpsInFreq, UpsOutVol, UpsOutCur, UpsBatVol, UpsBatTemp, BatVol, BatCur, BatEnvTemp, BatEnvHumi %s)"
                                                        " VALUE ('%010u', FROM_UNIXTIME(%lu), %u, %u, %u, %u, %u, %u, %u, %d, %d, %u %s, %u, %u, %u, %lu);", 
                                                                m_SCQry, pGetData->serial, pGetData->timestamp,
                                                                pGetData->Data.UpsInVol  , pGetData->Data.UpsInFreq , pGetData->Data.UpsOutVol, pGetData->Data.UpsOutCur,
                                                                pGetData->Data.UpsBatVol , pGetData->Data.UpsBatTemp, pGetData->Data.BatVol   , pGetData->Data.BatCur,
                                                                pGetData->Data.BatEnvTemp, pGetData->Data.BatEnvHumi, m_SVQry,
                                                                pGetData->Data.UpsStatus , pGetData->Data.BatStatus , pGetData->Data.EventBits, pGetData->LmtEvtBits);
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);

    if (m_Mysql->mysql_runQuery(m_Query, 0) < 0)
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s) [%s]\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error(), m_Query);
        if (!m_Mysql->mysql_Rollback()) {
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
        }   else
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] RollBack Completed !!!\x1b[0m", __FUNCTION__, __LINE__);
        return false;
    }   else
    {
        if (!m_Mysql->mysql_Commit())
        {
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
            if (!m_Mysql->mysql_Rollback()) {
                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
            }   else
                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] RollBack Completed !!!\x1b[0m", __FUNCTION__, __LINE__);
            return false;
        }
    }
    return true;
}

bool CDBInterface::uptDevDataT (GETDATA * pGetData)
{
    char sTmp[TMP_BUF] = {0,};
    memset(m_SCQry, 0, sizeof(m_SCQry));
    memset(m_SVQry, 0, sizeof(m_SVQry));
    memset(m_UCQry, 0, sizeof(m_UCQry));
    memset(m_UVQry, 0, sizeof(m_UVQry));

    for (int i = 0; i < BAT_TEMP_CNT; i++)
    {
        snprintf(sTmp, sizeof(sTmp), ",BatTemp%d", i+1);
        strcat(m_SCQry, sTmp);
        snprintf(sTmp, sizeof(sTmp), ",%d", pGetData->Data.BatTemp[i]);
        strcat(m_SVQry, sTmp);
    }

    for (int i = 0; i < UPS_3DATA_CNT; i++)
    {
        snprintf(sTmp, sizeof(sTmp), ",UpsData%d", i+1);
        strcat(m_UCQry, sTmp);
        snprintf(sTmp, sizeof(sTmp), ",%u", pGetData->Data2.UpsData[i]);
        strcat(m_UVQry, sTmp);
    }

    snprintf(m_Query, sizeof(m_Query), "INSERT INTO TB_DEV_DATA (ID, CREATED, UpsInVol, UpsInFreq, UpsOutVol, UpsOutCur, UpsBatVol, UpsBatTemp, BatVol, BatCur, BatEnvTemp, BatEnvHumi %s, UpsStatus, BatStatus, SysEventBits, LimitEventBits %s)"
                                                        " VALUE ('%010u', FROM_UNIXTIME(%lu), %u, %u, %u, %u, %u, %u, %u, %d, %d, %u %s, %u, %u, %u, %lu %s);", 
                                                                m_SCQry, m_UCQry, pGetData->serial, pGetData->timestamp,
                                                                pGetData->Data.UpsInVol  , pGetData->Data.UpsInFreq , pGetData->Data.UpsOutVol, pGetData->Data.UpsOutCur,
                                                                pGetData->Data.UpsBatVol , pGetData->Data.UpsBatTemp, pGetData->Data.BatVol   , pGetData->Data.BatCur   ,
                                                                pGetData->Data.BatEnvTemp, pGetData->Data.BatEnvHumi, m_SVQry,
                                                                pGetData->Data.UpsStatus , pGetData->Data.BatStatus , pGetData->Data.EventBits, pGetData->LmtEvtBits, m_UVQry);
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);

    if (m_Mysql->mysql_runQuery(m_Query, 0) < 0)
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s) [%s]\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error(), m_Query);
        return false;
    }   else
        return true;
}

bool CDBInterface::uptCurClear ()
{
    setAutoCommit(0);
    BeginTran();
    char sTmp[TMP_BUF] = {0,};
    memset(m_SCQry, 0, sizeof(m_SCQry));
    memset(m_UCQry, 0, sizeof(m_UCQry));
    for (int i = 1; i <= BAT_TEMP_CNT; i++)
    {
        snprintf(sTmp, sizeof(sTmp), "BatTemp%d = 0.0, ", i);
        strcat(m_SCQry, sTmp);
    }
    for (int i = 1; i <= UPS_3DATA_CNT; i++)
    {
        snprintf(sTmp, sizeof(sTmp), " ,UpsData%d = 0.0", i);
        strcat(m_UCQry, sTmp);
    }
    snprintf(m_Query, sizeof(m_Query), "UPDATE TB_DEV_DATA_CUR    "
                                       "SET CREATED        = CURRENT_TIMESTAMP(),"
                                       "    UpsInVol       = 0.0, "
                                       "    UpsInFreq      = 0.0, "
                                       "    UpsOutVol      = 0.0, "
                                       "    UpsOutCur      = 0.0, "
                                       "    UpsBatVol      = 0.0, "
                                       "    UpsBatTemp     = 0.0, "
                                       "    BatVol         = 0.0, "
                                       "    BatCur         = 0.0, "
                                       "    BatEnvTemp     = 0.0, "
                                       "    BatEnvHumi     = 0.0, "
                                       "    %s                    "
                                       "    UpsStatus      = 0  , "
                                       "    BatStatus      = 0  , "
                                       "    SysEventBits   = 2  , "
                                       "    LimitEventBits = 0    "
                                       "    %s ; ", m_SCQry, m_UCQry);
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);

    if (m_Mysql->mysql_runQuery(m_Query, 0) < 0)
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s) [%s]\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error(), m_Query);
        if (!m_Mysql->mysql_Rollback()) {
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
        }   else
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] RollBack Completed !!!\x1b[0m", __FUNCTION__, __LINE__);
        return false;
    }   else
        return EndTran();
}

bool CDBInterface::evtDevOccur (EVENTS * pEvent)
{
    if (!m_Mysql->mysql_Start())
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
        return false;
    }

    if (pEvent->evtno >= 80 && pEvent->evtno < 100)
        snprintf(m_Query, sizeof(m_Query), "INSERT INTO TB_LOG_ALARM (ID, EVTNO, CREATED, LEVEL, ITSM_LEVEL, STATUS, ALARM, INPUT, LIMITVAL, SMS_FLAG) "
                                           "VALUE ('%010u', %u, FROM_UNIXTIME(%lu), %d, %d, 1, '%s', %d, %d, %d);", pEvent->serial, pEvent->evtno, pEvent->starttime, pEvent->level, pEvent->itsm_level, pEvent->message, (int16_t)pEvent->evtval, (int16_t)pEvent->limitval, pEvent->smsflag);
    else
        snprintf(m_Query, sizeof(m_Query), "INSERT INTO TB_LOG_ALARM (ID, EVTNO, CREATED, LEVEL, ITSM_LEVEL, STATUS, ALARM, INPUT, LIMITVAL, SMS_FLAG) "
                                           "VALUE ('%010u', %u, FROM_UNIXTIME(%lu), %d, %d, 1, '%s', %u, %u, %d);", pEvent->serial, pEvent->evtno, pEvent->starttime, pEvent->level, pEvent->itsm_level, pEvent->message, pEvent->evtval, pEvent->limitval, pEvent->smsflag);
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);

    if (m_Mysql->mysql_runQuery(m_Query, 0) < 0)
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s) [%s]\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error(), m_Query);
        if (!m_Mysql->mysql_Rollback()) {
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
        }   else
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] RollBack Completed !!!\x1b[0m", __FUNCTION__, __LINE__);
        return false;
    }   else
    {
        if (!m_Mysql->mysql_Commit())
        {
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
            if (!m_Mysql->mysql_Rollback()) {
                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
            }   else
                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] RollBack Completed !!!\x1b[0m", __FUNCTION__, __LINE__);
            return false;
        }
    }
    return true;
}

bool CDBInterface::evtDevClear (EVENTS * pEvent)
{
    if (!m_Mysql->mysql_Start())
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
        return false;
    }

    snprintf(m_Query, sizeof(m_Query), "UPDATE TB_LOG_ALARM SET CLEARED = FROM_UNIXTIME(%lu), STATUS = 0 WHERE ID = '%010u' AND EVTNO = %u AND STATUS <> 0; ", pEvent->endtime, pEvent->serial, pEvent->evtno);
    //snprintf(m_Query, sizeof(m_Query), "UPDATE TB_LOG_ALARM SET CLEARED = FROM_UNIXTIME(%lu), STATUS = 0 WHERE ID = '%010u' AND EVTNO = %u AND CREATED = FROM_UNIXTIME(%lu); ", pEvent->endtime, pEvent->serial, pEvent->evtno, pEvent->starttime);
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);

    if (m_Mysql->mysql_runQuery(m_Query, 0) < 0)
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s) [%s]\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error(), m_Query);
        if (!m_Mysql->mysql_Rollback()) {
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
        }   else
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] RollBack Completed !!!\x1b[0m", __FUNCTION__, __LINE__);
        return false;
    }   else
    {
        if (!m_Mysql->mysql_Commit())
        {
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
            if (!m_Mysql->mysql_Rollback()) {
                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
            }   else
                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] RollBack Completed !!!\x1b[0m", __FUNCTION__, __LINE__);
            return false;
        }
    }
    return true;
}

//bool CDBInterface::evtDevClear (uint32_t Serial)
//{
//    if (!m_Mysql->mysql_Start())
//    {
//        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
//        return false;
//    }
//
//    //snprintf(m_Query, sizeof(m_Query), "UPDATE TB_LOG_ALARM SET CLEARED = NOW(), STATUS = 0 WHERE ID = '%010u' AND STATUS = 1; ", Serial);
//    snprintf(m_Query, sizeof(m_Query), "UPDATE TB_LOG_ALARM SET CLEARED = NOW(), STATUS = 0 WHERE ID = '%010u' AND STATUS <> 0; ", Serial);
//    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);
//
//    if (m_Mysql->mysql_runQuery(m_Query, 0) < 0)
//    {
//        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s) [%s]\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error(), m_Query);
//        if (!m_Mysql->mysql_Rollback()) {
//            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
//        }   else
//            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] RollBack Completed !!!\x1b[0m", __FUNCTION__, __LINE__);
//        return false;
//    }   else
//    {
//        if (!m_Mysql->mysql_Commit())
//        {
//            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
//            if (!m_Mysql->mysql_Rollback()) {
//                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
//            }   else
//                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] RollBack Completed !!!\x1b[0m", __FUNCTION__, __LINE__);
//            return false;
//        }
//    }
//    return true;
//}

bool CDBInterface::getEvtMsg (EVENTS * pEvent)
{
    char message [FUL_FLE] = {0,};
    char level   [LVL_BUF] = {0,};
    char itsm_lvl[LVL_BUF] = {0,};
    uint8_t  ret = 0;
    snprintf(m_Query, sizeof(m_Query), "select message, level, itsm_level from TB_EVT_MESSAGE where SeqNo = %u;", pEvent->evtno);
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);

    int m_result = m_Mysql->mysql_runQuery(m_Query, 1);
    if (m_result == 1)  // 쿼리정상 (Select)
    {
        m_Mysql->mysql_fetchRow();
        m_Mysql->mysql_fetchfieldbyID(0, message , sizeof(message) );
        m_Mysql->mysql_fetchfieldbyID(1, level   , sizeof(level)   );
        m_Mysql->mysql_fetchfieldbyID(2, itsm_lvl, sizeof(itsm_lvl));
        m_Mysql->mysql_free_res();

        snprintf(pEvent->message, sizeof(pEvent->message), message);
        pEvent->level      = atoi(level);
        pEvent->itsm_level = atoi(itsm_lvl);
        ret = true ;
    }   else
    if (m_result == 0)  //쿼리결과가 없음
    {
        ret = false;
    }   else
    if (m_result <  0)  // 쿼리에러
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;35m%s()[%d] DB Query Fail (%s)(%s) !!!\x1b[0m", __FUNCTION__, __LINE__, m_Query, m_Mysql->mysql_Error());
        ret = false;
    }
    return ret;
}

void CDBInterface::getEvtNumStr (char * Machine, char * LocCode)
{
    char Col4[HEX_BUF] = {0,};
    //snprintf(m_Query, sizeof(m_Query), "select COL4 from TB_KB_ASS where COL12 = '%s';", LocCode);
    snprintf(m_Query, sizeof(m_Query), "select COL4 from TB_KB_ASS where COL3='01100101' AND COL11='1' AND COL12='%s' LIMIT 1; ", LocCode);
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);

    int m_result = m_Mysql->mysql_runQuery(m_Query, 1);
    if (m_result == 1)  // 쿼리정상 (Select)
    {
        m_Mysql->mysql_fetchRow();
        m_Mysql->mysql_fetchfieldbyID(0, Col4, sizeof(Col4));
        m_Mysql->mysql_free_res();
        snprintf(Machine, TMP_BUF, "%s", Col4);
    }   else
    if (m_result == 0)  //쿼리결과가 없음
    {
        memset(Machine, 0, TMP_BUF);
    }   else
    if (m_result <  0)  // 쿼리에러
    {
        memset(Machine, 0, TMP_BUF);
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;35m%s()[%d] DB Query Fail (%s)(%s) !!!\x1b[0m", __FUNCTION__, __LINE__, m_Query, m_Mysql->mysql_Error());
    }
    return;
}

uint8_t CDBInterface::getDevdef (DEVINFO * pDevInfo, DEVDEF * pDevdef, bool DevUpt)
{
    uint32_t Serial = pDevInfo->serial;
    uint8_t  ret    = chkDevOpInfo(Serial, pDevdef);    // 운영 Dev 확인
    if (ret == _ISOK_)
    {
        if (DevUpt && pDevdef->aflag != _SNMP_TYPE_)
            DEVInit.send_message(1, pDevInfo, SZ_DEVINFO);
        return getDevOpLimit(Serial, pDevdef);          // 임계치 정보 얻기.
    }   else
    {
        if (ret == _FAIL_) return ret;
        ret = chkDevPool(Serial);
        if (ret == _NONE_)
            return regDevPool(pDevInfo);
        else
            return ret;
    }
}

uint8_t CDBInterface::setDevAct (DEVINFO * pDevInfo, DEVDEF * pDevdef)
{
    uint32_t Serial = pDevInfo->serial ;
    setAutoCommit(0);
    if (!m_Mysql->mysql_Start())
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
        return _FAIL_;
    }
    uint8_t ret = regDevOpInfo(Serial);
    if (ret == _ISOK_)
    {
        ret = regDevOpLimit(Serial);
        if (ret == _ISOK_)
        {
            ret = setPoolActive(Serial);
            if (ret == _ISOK_)
            {            
                if (!m_Mysql->mysql_Commit())
                {
                    pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
                    if (!m_Mysql->mysql_Rollback()) {
                        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
                    }   else
                        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] RollBack Completed !!!\x1b[0m", __FUNCTION__, __LINE__);
                    return _FAIL_;
                }
                ret = chkDevOpInfo (Serial, pDevdef);
                if (ret == _ISOK_)
                    ret = getDevOpLimit(Serial, pDevdef);
            }
        }
    }
    return ret;
}


uint8_t CDBInterface::chkDevOpInfo (uint32_t Serial, DEVDEF * pDevdef)
{
    uint8_t ret = 0;
    char    ptime[LVL_BUF] = {0,};
    char    ntime[LVL_BUF] = {0,};
    char    dtime[LVL_BUF] = {0,};
    char    aflag[LVL_BUF] = {0,};
    char    phase[LVL_BUF] = {0,};
    snprintf(m_Query, sizeof(m_Query), "SELECT d.LOC_CODE, k.COL10, d.POLLING, d.DATASAVE, d.NODEJS, d.TYPE, d.SNMP_IP, d.PHASE "
                                       "FROM   TB_DEV_INFO d, "
                                       "       TB_KB_LOC   k  "
                                       "WHERE  d.ID       = '%010u' "
                                       "AND    d.LOC_CODE = k.COL4  "
                                       "LIMIT  1;", Serial);
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);

    int m_result = m_Mysql->mysql_runQuery(m_Query, 1);
    if (m_result == 1)  // 쿼리정상 (Select)
    {
        m_Mysql->mysql_fetchRow();
        m_Mysql->mysql_fetchfieldbyID(0, (char*)pDevdef->bcode, sizeof(pDevdef->bcode));
        m_Mysql->mysql_fetchfieldbyID(1, (char*)pDevdef->bname, sizeof(pDevdef->bname));
        m_Mysql->mysql_fetchfieldbyID(2, ptime, sizeof(ptime));
        m_Mysql->mysql_fetchfieldbyID(3, dtime, sizeof(dtime));
        m_Mysql->mysql_fetchfieldbyID(4, ntime, sizeof(ntime));
        
        m_Mysql->mysql_fetchfieldbyID(5, aflag, sizeof(aflag));
        m_Mysql->mysql_fetchfieldbyID(6, (char*)pDevdef->snmpip, sizeof(pDevdef->snmpip));
        m_Mysql->mysql_fetchfieldbyID(7, phase, sizeof(phase));

        pDevdef->pollingtime = atoi(ptime);
        pDevdef->dbsavetime  = atoi(dtime);
        pDevdef->nodejstime  = atoi(ntime);
        pDevdef->aflag       = atoi(aflag);
        pDevdef->phase       = atoi(phase);

        m_Mysql->mysql_free_res();
        ret = _ISOK_;
    }   else
    if (m_result == 0)  //쿼리결과가 없음
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "\x1b[1;35m%s()[%d] DB Query Fail (%s)(%s) !!!\x1b[0m", __FUNCTION__, __LINE__, m_Query);
        ret = _NONE_;
    }   else
    if (m_result <  0)  // 쿼리에러
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;35m%s()[%d] DB Query Fail (%s)(%s) !!!\x1b[0m", __FUNCTION__, __LINE__, m_Query, m_Mysql->mysql_Error());
        ret = _FAIL_;
    }
    return ret;
}


uint8_t CDBInterface::uptDevOpInfoT(DEVINFO * pDevInfo)
{
    uint8_t ret = 0;
    char    IPs [17] = {0,};
    char    Macs[18] = {0,};

    snprintf(IPs , sizeof(IPs) , "%u.%u.%u.%u", pDevInfo->localip[0], pDevInfo->localip[1], pDevInfo->localip[2], pDevInfo->localip[3]);
    snprintf(Macs, sizeof(Macs), "%02X:%02X:%02X:%02X:%02X:%02X", pDevInfo->macaddr[0], pDevInfo->macaddr[1], pDevInfo->macaddr[2], pDevInfo->macaddr[3], pDevInfo->macaddr[4], pDevInfo->macaddr[5]);
    snprintf(m_Query, sizeof(m_Query), "UPDATE TB_DEV_INFO          "
                                       "SET    IP           = '%s', "
                                       "       MAC          = '%s', "
                                       "       MODEL_CODE   = '%u', "
                                       "       MODEL_NAME   = '%s', "
                                       "       LOC_CODE     = '%s', "
                                       "       LOC_NAME     = '%s', "
                                       "       UMS_BARCODE  = '%s', "
                                       "       UPS_BARCODE  = '%s', "
                                       "       BAT_BARCODE  = '%s', "
                                       "       BOX_BARCODE  = '%s', "
                                       "       LORA_BARCODE = '%s', "
                                       "       UPDATED      = CURRENT_TIMESTAMP() "
                                       "WHERE  ID = '%010u';", 
                                        IPs, Macs, pDevInfo->modelcode, pDevInfo->modelname, pDevInfo->localcode, pDevInfo->localname, 
                                        pDevInfo->umsserial, pDevInfo->upsserial, pDevInfo->batserial, pDevInfo->batboxserial, pDevInfo->loraserial, pDevInfo->serial);
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);

    if (m_Mysql->mysql_runQuery(m_Query, 0) < 0)
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s) [%s]\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error(), m_Query);
        ret =  _FAIL_;
    }   else
    {
        snprintf(m_Query, sizeof(m_Query), "UPDATE TB_DEV_POOL_INFO     "
                                           "SET    IP           = '%s', "
                                           "       MAC          = '%s', "
                                           "       MODEL_CODE   = '%u', "
                                           "       MODEL_NAME   = '%s', "
                                           "       LOC_CODE     = '%s', "
                                           "       LOC_NAME     = '%s', "
                                           "       UMS_BARCODE  = '%s', "
                                           "       UPS_BARCODE  = '%s', "
                                           "       BAT_BARCODE  = '%s', "
                                           "       BOX_BARCODE  = '%s', "
                                           "       LORA_BARCODE = '%s'  "
                                           "WHERE  ID = '%010u';", 
                                            IPs, Macs, pDevInfo->modelcode, pDevInfo->modelname, pDevInfo->localcode, pDevInfo->localname, 
                                            pDevInfo->umsserial, pDevInfo->upsserial, pDevInfo->batserial, pDevInfo->batboxserial, pDevInfo->loraserial, pDevInfo->serial);
        pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);

        if (m_Mysql->mysql_runQuery(m_Query, 0) < 0)
        {
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s) [%s]\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error(), m_Query);
            ret =  _FAIL_;
        }   else
        {
            //snprintf(m_Query, sizeof(m_Query), "UPDATE TB_LOG_ALARM SET CLEARED = NOW(), STATUS = 0 WHERE ID = '%010u' AND STATUS = 1; ", Serial);
            snprintf(m_Query, sizeof(m_Query), "UPDATE TB_LOG_ALARM SET CLEARED = NOW(), STATUS = 0 WHERE ID = '%010u' AND STATUS <> 0; ", pDevInfo->serial);
            pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);
        
            if (m_Mysql->mysql_runQuery(m_Query, 0) < 0)
            {
                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s) [%s]\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error(), m_Query);
                ret =  _FAIL_;
            }   else
            {
                ret = _ISOK_;
            }
        }
    }

    return ret;
}


//uint8_t CDBInterface::uptDevOpInfo(DEVINFO * pDevInfo)
//{
//    uint8_t ret = 0;
//    char    IPs [17] = {0,};
//    char    Macs[18] = {0,};
//    setAutoCommit(0);
//    if (!m_Mysql->mysql_Start())
//    {
//        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
//        ret = _FAIL_;
//    }   else
//    {
//        snprintf(IPs , sizeof(IPs) , "%u.%u.%u.%u", pDevInfo->localip[0], pDevInfo->localip[1], pDevInfo->localip[2], pDevInfo->localip[3]);
//        snprintf(Macs, sizeof(Macs), "%02X:%02X:%02X:%02X:%02X:%02X", pDevInfo->macaddr[0], pDevInfo->macaddr[1], pDevInfo->macaddr[2], pDevInfo->macaddr[3], pDevInfo->macaddr[4], pDevInfo->macaddr[5]);
//        snprintf(m_Query, sizeof(m_Query), "UPDATE TB_DEV_INFO          "
//                                           "SET    IP           = '%s', "
//                                           "       MAC          = '%s', "
//                                           "       MODEL_CODE   = '%u', "
//                                           "       MODEL_NAME   = '%s', "
//                                           "       LOC_CODE     = '%s', "
//                                           "       LOC_NAME     = '%s', "
//                                           "       UMS_BARCODE  = '%s', "
//                                           "       UPS_BARCODE  = '%s', "
//                                           "       BAT_BARCODE  = '%s', "
//                                           "       BOX_BARCODE  = '%s', "
//                                           "       LORA_BARCODE = '%s', "
//                                           "       UPDATED      = CURRENT_TIMESTAMP() "
//                                           "WHERE  ID = '%010u';", 
//                                            IPs, Macs, pDevInfo->modelcode, pDevInfo->modelname, pDevInfo->localcode, pDevInfo->localname, 
//                                            pDevInfo->umsserial, pDevInfo->upsserial, pDevInfo->batserial, pDevInfo->batboxserial, pDevInfo->loraserial, pDevInfo->serial);
//        pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);
//
//        if (m_Mysql->mysql_runQuery(m_Query, 0) < 0)
//        {
//            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s) [%s]\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error(), m_Query);
//            if (!m_Mysql->mysql_Rollback()) {
//                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
//            }   else
//                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] RollBack Completed !!!\x1b[0m", __FUNCTION__, __LINE__);
//            ret =  _FAIL_;
//        }   else
//        {
//            snprintf(m_Query, sizeof(m_Query), "UPDATE TB_DEV_POOL_INFO     "
//                                               "SET    IP           = '%s', "
//                                               "       MAC          = '%s', "
//                                               "       MODEL_CODE   = '%u', "
//                                               "       MODEL_NAME   = '%s', "
//                                               "       LOC_CODE     = '%s', "
//                                               "       LOC_NAME     = '%s', "
//                                               "       UMS_BARCODE  = '%s', "
//                                               "       UPS_BARCODE  = '%s', "
//                                               "       BAT_BARCODE  = '%s', "
//                                               "       BOX_BARCODE  = '%s', "
//                                               "       LORA_BARCODE = '%s'  "
//                                               "WHERE  ID = '%010u';", 
//                                                IPs, Macs, pDevInfo->modelcode, pDevInfo->modelname, pDevInfo->localcode, pDevInfo->localname, 
//                                                pDevInfo->umsserial, pDevInfo->upsserial, pDevInfo->batserial, pDevInfo->batboxserial, pDevInfo->loraserial, pDevInfo->serial);
//            pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);
//    
//            if (m_Mysql->mysql_runQuery(m_Query, 0) < 0)
//            {
//                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s) [%s]\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error(), m_Query);
//                if (!m_Mysql->mysql_Rollback()) {
//                    pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
//                }   else
//                    pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] RollBack Completed !!!\x1b[0m", __FUNCTION__, __LINE__);
//                ret =  _FAIL_;
//            }   else
//            {
//                //snprintf(m_Query, sizeof(m_Query), "UPDATE TB_LOG_ALARM SET CLEARED = NOW(), STATUS = 0 WHERE ID = '%010u' AND STATUS = 1; ", Serial);
//                snprintf(m_Query, sizeof(m_Query), "UPDATE TB_LOG_ALARM SET CLEARED = NOW(), STATUS = 0 WHERE ID = '%010u' AND STATUS <> 0; ", pDevInfo->serial);
//                pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);
//            
//                if (m_Mysql->mysql_runQuery(m_Query, 0) < 0)
//                {
//                    pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s) [%s]\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error(), m_Query);
//                    if (!m_Mysql->mysql_Rollback()) {
//                        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
//                    }   else
//                        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] RollBack Completed !!!\x1b[0m", __FUNCTION__, __LINE__);
//                    ret =  _FAIL_;
//                }   else
//                {
//                    if (!m_Mysql->mysql_Commit())
//                    {
//                        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
//                        if (!m_Mysql->mysql_Rollback()) {
//                            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
//                        }   else
//                            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] RollBack Completed !!!\x1b[0m", __FUNCTION__, __LINE__);
//                        ret =  _FAIL_;
//                    }
//                    ret = _ISOK_;
//                }
//            }
//        }
//    }
//
//    return ret;
//}

uint8_t CDBInterface::getDevOpLimit (uint32_t Serial, DEVDEF * pDevdef)
{
    uint8_t ret = 0;
    char    code[LVL_BUF] = {0,};
    char    min2[LVL_BUF] = {0,};
    char    min1[LVL_BUF] = {0,};
    char    max1[LVL_BUF] = {0,};
    char    max2[LVL_BUF] = {0,};
    char    time[LVL_BUF] = {0,};
    char    sms [LVL_BUF] = {0,};
    char    used[LVL_BUF] = {0,};
    
    pDevdef->evtLimit.removeAll();
    ///////////// TB_THR_INFO ////////////////
    snprintf(m_Query, sizeof(m_Query), "select CODE, (MAX2+OFFSET), (MAX1+OFFSET), (MIN1+OFFSET), (MIN2+OFFSET), DURATION, SMS, USED from TB_THR_INFO where ID = '%010u';", Serial);
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);
    int m_result = m_Mysql->mysql_runQuery(m_Query, 1);
    if (m_result >= 1)  // 쿼리정상 (Select)
    {
        for ( int i = 0; i < m_result; i++ )
        {
            m_Mysql->mysql_fetchRow();
            m_Mysql->mysql_fetchfieldbyID(0, code, sizeof(code));
            m_Mysql->mysql_fetchfieldbyID(1, max2, sizeof(max2));
            m_Mysql->mysql_fetchfieldbyID(2, max1, sizeof(max1));
            m_Mysql->mysql_fetchfieldbyID(3, min1, sizeof(min1));
            m_Mysql->mysql_fetchfieldbyID(4, min2, sizeof(min2));
            m_Mysql->mysql_fetchfieldbyID(5, time, sizeof(time));
            m_Mysql->mysql_fetchfieldbyID(6, sms , sizeof(sms ));
            m_Mysql->mysql_fetchfieldbyID(7, used, sizeof(used));
        
            _UPS_LMT_ * lmt = new _UPS_LMT_;
            memset(lmt, 0, sizeof(_UPS_LMT_));
            lmt->Lmt.Max2 = atoi(max2);
            lmt->Lmt.Max1 = atoi(max1);
            lmt->Lmt.Min1 = atoi(min1);
            lmt->Lmt.Min2 = atoi(min2);
            lmt->Lmt.Time = atoi(time);
            lmt->Sms      = (uint8_t)sms [0];
            lmt->Used     = (uint8_t)used[0];
            pDevdef->evtLimit.insert(atoi(code), lmt);
        }
        m_Mysql->mysql_free_res();

        ///////////// TB_THR_DEV_INFO ////////////////
        snprintf(m_Query, sizeof(m_Query), "select CODE, (MAX+OFFSET), (MIN+OFFSET), DURATION, SMS, USED from TB_THR_DEV_INFO where ID = '%010u';", Serial);
        pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);
        int m_result = m_Mysql->mysql_runQuery(m_Query, 1);
        if (m_result >= 1)  // 쿼리정상 (Select)
        {
            for ( int i = 0; i < m_result; i++ )
            {
                m_Mysql->mysql_fetchRow();
                m_Mysql->mysql_fetchfieldbyID(0, code, sizeof(code));
                m_Mysql->mysql_fetchfieldbyID(1, max2, sizeof(max2));
                m_Mysql->mysql_fetchfieldbyID(2, min2, sizeof(min2));
                m_Mysql->mysql_fetchfieldbyID(3, time, sizeof(time));
                m_Mysql->mysql_fetchfieldbyID(4, sms , sizeof(sms ));
                m_Mysql->mysql_fetchfieldbyID(5, used, sizeof(used));
            
                _UPS_LMT_ * lmt = new _UPS_LMT_;
                memset(lmt, 0, sizeof(_UPS_LMT_));
                lmt->Lmt.Max2 = atoi(max2);
                lmt->Lmt.Min2 = atoi(min2);
                lmt->Lmt.Time = atoi(time);
                lmt->Sms      = (uint8_t)sms [0];
                lmt->Used     = (uint8_t)used[0];
                pDevdef->evtLimit.insert(atoi(code), lmt);
            }
            m_Mysql->mysql_free_res();
            ret = _ISOK_;
        }   else
        if (m_result == 0)  //쿼리결과가 없음
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "\x1b[1;35m%s()[%d] Not Found Limit Value (%s) !!!\x1b[0m", __FUNCTION__, __LINE__, m_Query);
            ret = _NONE_;
        }   else
        if (m_result <  0)    // 쿼리에러
        {
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;35m%s()[%d] DB Query Fail (%s)(%s) !!!\x1b[0m", __FUNCTION__, __LINE__, m_Query, m_Mysql->mysql_Error());
            ret = _FAIL_;
        }
    }   else
    if (m_result == 0)  //쿼리결과가 없음
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "\x1b[1;35m%s()[%d] Not Found Limit Value (%s) !!!\x1b[0m", __FUNCTION__, __LINE__, m_Query);
        ret = _NONE_;
    }   else
    if (m_result <  0)    // 쿼리에러
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;35m%s()[%d] DB Query Fail (%s)(%s) !!!\x1b[0m", __FUNCTION__, __LINE__, m_Query, m_Mysql->mysql_Error());
        ret = _FAIL_;
    }
    return ret;
}


uint8_t CDBInterface::chkDevPool (uint32_t Serial)
{
    uint8_t  ret = 0;
    snprintf(m_Query, sizeof(m_Query), "select ID from TB_DEV_POOL_INFO where ID = '%010u';", Serial);
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);

    int m_result = m_Mysql->mysql_runQuery(m_Query, 1);
    if (m_result == 1)  // 쿼리정상 (Select)
    {
        m_Mysql->mysql_free_res();
        ret = _SUCCESS_;
    }   else
    if (m_result == 0)  //쿼리결과가 없음
    {
        ret = _NONE_;
    }   else
    if (m_result <  0)  // 쿼리에러
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;35m%s()[%d] DB Query Fail (%s)(%s) !!!\x1b[0m", __FUNCTION__, __LINE__, m_Query, m_Mysql->mysql_Error());
        ret = _FAIL_;
    }
    return ret;
}


uint8_t CDBInterface::regDevPool (DEVINFO * pDevInfo)
{
    uint8_t ret = 0;
    char    IPs [17] = {0,};
    char    Macs[18] = {0,};
    setAutoCommit(0);
    if (!m_Mysql->mysql_Start())
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
        ret = _FAIL_;
    }   else
    {
        snprintf(IPs , sizeof(IPs) , "%u.%u.%u.%u", pDevInfo->localip[0], pDevInfo->localip[1], pDevInfo->localip[2], pDevInfo->localip[3]);
        snprintf(Macs, sizeof(Macs), "%02X:%02X:%02X:%02X:%02X:%02X", pDevInfo->macaddr[0], pDevInfo->macaddr[1], pDevInfo->macaddr[2], pDevInfo->macaddr[3], pDevInfo->macaddr[4], pDevInfo->macaddr[5]);

        snprintf(m_Query, sizeof(m_Query), "INSERT INTO TB_DEV_POOL_INFO (ID, MAC, IP, MODEL_CODE, MODEL_NAME, LOC_NAME, LOC_CODE, UMS_BARCODE, UPS_BARCODE, BAT_BARCODE, BOX_BARCODE, LORA_BARCODE, CREATED)"
                                                                 " VALUE ('%010u', '%s', '%s', '%u', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', CURRENT_TIMESTAMP());",
                                            pDevInfo->serial, Macs, IPs, pDevInfo->modelcode,  pDevInfo->modelname, pDevInfo->localname, pDevInfo->localcode, pDevInfo->umsserial, pDevInfo->upsserial, pDevInfo->batserial, pDevInfo->batboxserial, pDevInfo->loraserial);
        pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);

        if (m_Mysql->mysql_runQuery(m_Query, 0) < 0)
        {
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s) [%s]\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error(), m_Query);
            if (!m_Mysql->mysql_Rollback()) {
                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
            }   else
                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] RollBack Completed !!!\x1b[0m", __FUNCTION__, __LINE__);
            ret =  _FAIL_;
        }   else
        {
            if (!m_Mysql->mysql_Commit())
            {
                pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
                if (!m_Mysql->mysql_Rollback()) {
                    pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
                }   else
                    pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] RollBack Completed !!!\x1b[0m", __FUNCTION__, __LINE__);
                ret =  _FAIL_;
            }
            ret = _SUCCESS_;
        }
    }

    return ret;
}


uint8_t CDBInterface::regDevOpInfo (uint32_t Serial)
{
    uint8_t ret = _ISOK_;
    snprintf(m_Query, sizeof(m_Query), "INSERT INTO TB_DEV_INFO (ID, MAC, IP, MODEL_CODE, MODEL_NAME, LOC_NAME, LOC_CODE, UMS_BARCODE, UPS_BARCODE, BAT_BARCODE, BOX_BARCODE, LORA_BARCODE, POLLING, DATASAVE, NODEJS, TYPE, SNMP_IP, PHASE, CREATED) "
                                       "SELECT p.ID, p.MAC, p.IP, p.MODEL_CODE, p.MODEL_NAME, p.LOC_NAME, p.LOC_CODE, p.UMS_BARCODE, p.UPS_BARCODE, p.BAT_BARCODE, p.BOX_BARCODE, p.LORA_BARCODE, d.POLLING, d.DATASAVE, d.NODEJS, p.TYPE, p.SNMP_IP, p.PHASE, CURRENT_TIMESTAMP() "
                                       "FROM   TB_DEV_POOL_INFO p,    "
                                       "     ( SELECT POLLING, DATASAVE, NODEJS "
                                       "       FROM   TB_DEV_DEF_CONF "
                                       "       LIMIT  1    )   d      "
                                       "WHERE  p.ID = '%010u'   ;     ", Serial);
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);

    if (m_Mysql->mysql_runQuery(m_Query, 0) < 0)
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s) [%s]\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error(), m_Query);
        if (!m_Mysql->mysql_Rollback()) {
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
        }   else
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] RollBack Completed !!!\x1b[0m", __FUNCTION__, __LINE__);
        ret =  _FAIL_;
    }
    return ret;
}

//uint8_t CDBInterface::regDevOpInfo (uint32_t Serial)
//{
//    uint8_t ret = _ISOK_;
//    snprintf(m_Query, sizeof(m_Query), "INSERT INTO TB_DEV_INFO (ID, MAC, IP, MODEL_CODE, MODEL_NAME, LOC_NAME, LOC_CODE, UMS_BARCODE, UPS_BARCODE, BAT_BARCODE, BOX_BARCODE, LORA_BARCODE, POLLING, DATASAVE, NODEJS, CREATED) "
//                                       "SELECT p.ID, p.MAC, p.IP, p.MODEL_CODE, p.MODEL_NAME, p.LOC_NAME, p.LOC_CODE, p.UMS_BARCODE, p.UPS_BARCODE, p.BAT_BARCODE, p.BOX_BARCODE, p.LORA_BARCODE, d.POLLING, d.DATASAVE, d.NODEJS, CURRENT_TIMESTAMP() "
//                                       "FROM   TB_DEV_POOL_INFO p,     "
//                                       "       TB_DEV_DEF_CONF  d      "                                                                             
//                                       "WHERE  p.ID = '%010u'          "
//                                       "AND    p.MODEL_NAME = d.MODEL; ", Serial);
//    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);
//
//    if (m_Mysql->mysql_runQuery(m_Query, 0) < 0)
//    {
//        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s) [%s]\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error(), m_Query);
//        if (!m_Mysql->mysql_Rollback()) {
//            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
//        }   else
//            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] RollBack Completed !!!\x1b[0m", __FUNCTION__, __LINE__);
//        ret =  _FAIL_;
//    }
//    return ret;
//}

uint8_t CDBInterface::setPoolActive (uint32_t Serial)
{
    uint8_t ret = _ISOK_;
    snprintf(m_Query, sizeof(m_Query), "UPDATE TB_DEV_POOL_INFO "
                                       "SET    ACTIVE = 1       "
                                       "WHERE  ID = '%010u';    ", Serial);
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);

    if (m_Mysql->mysql_runQuery(m_Query, 0) < 0)
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s) [%s]\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error(), m_Query);
        if (!m_Mysql->mysql_Rollback()) {
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
        }   else
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] RollBack Completed !!!\x1b[0m", __FUNCTION__, __LINE__);
        ret =  _FAIL_;
    }
    return ret;
}

uint8_t CDBInterface::regDevOpLimit (uint32_t Serial)
{
    uint8_t ret = _ISOK_;
    snprintf(m_Query, sizeof(m_Query), "INSERT INTO TB_THR_INFO (ID, CODE, MAX2, MAX1, MIN1, MIN2, OFFSET, DURATION, SMS, USED) "
                                       "SELECT '%010u', CODE, MAX2, MAX1, MIN1, MIN2, OFFSET, DURATION, SMS, USED "
                                       "FROM   TB_THR_DEF_CONF;", Serial);
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);

    if (m_Mysql->mysql_runQuery(m_Query, 0) < 0)
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s) [%s]\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error(), m_Query);
        if (!m_Mysql->mysql_Rollback()) {
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
        }   else
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] RollBack Completed !!!\x1b[0m", __FUNCTION__, __LINE__);
        ret =  _FAIL_;
    }
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    snprintf(m_Query, sizeof(m_Query), "INSERT INTO TB_THR_DEV_INFO (ID, CODE, MAX, MIN, OFFSET, DURATION, SMS, USED) "
                                       "SELECT '%010u', CODE, MAX, MIN, OFFSET, DURATION, SMS, USED "
                                       "FROM   TB_THR_DEV_DEF_CONF;", Serial);
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);

    if (m_Mysql->mysql_runQuery(m_Query, 0) < 0)
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s) [%s]\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error(), m_Query);
        if (!m_Mysql->mysql_Rollback()) {
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
        }   else
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] RollBack Completed !!!\x1b[0m", __FUNCTION__, __LINE__);
        ret =  _FAIL_;
    }

    return ret;
}


bool CDBInterface::getSNMPLst (SNMPLST * pSNMPLst)
{
    bool    ret = false;
    char    sid[HEX_BUF] = {0,};
    
    snprintf(m_Query, sizeof(m_Query), "select ID from TB_DEV_INFO where TYPE = '2';");
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);
    int m_result = m_Mysql->mysql_runQuery(m_Query, 1);
    if (m_result >= 1)  // 쿼리정상 (Select)
    {
        pSNMPLst->Count = m_result;
        for ( int i = 0; i < m_result; i++ )
        {
            m_Mysql->mysql_fetchRow();
            m_Mysql->mysql_fetchfieldbyID(0, sid, sizeof(sid));
            pSNMPLst->snmp[i].serial = atoi(sid);
        }
        m_Mysql->mysql_free_res();
        ret = true;
    }   else
    if (m_result == 0)  //쿼리결과가 없음
    {
        ret = false;
    }   else
    if (m_result <  0)    // 쿼리에러
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;35m%s()[%d] DB Query Fail (%s)(%s) !!!\x1b[0m", __FUNCTION__, __LINE__, m_Query, m_Mysql->mysql_Error());
        ret = false;
    }
    return ret;
}


//void CDBInterface::setSnmpData (uint32_t serial, DATA * pData, DATA2  * pData2, uint8_t phase)
//{
//    char    UpsInVol [HEX_BUF] = {0,};
//    char    UpsInFreq[HEX_BUF] = {0,};
//    char    UpsOutVol[HEX_BUF] = {0,};
//    char    UpsOutCur[HEX_BUF] = {0,};
//    char    UpsBatVol[HEX_BUF] = {0,};
//    char    UpsBatTmp[HEX_BUF] = {0,};
//    char    UpsStatus[HEX_BUF] = {0,};
//    char    UpsData  [6][HEX_BUF];
//    memset (UpsData, 0, sizeof(UpsData));
//
//    snprintf(m_Query, sizeof(m_Query), "select UpsInVol  , UpsInFreq, UpsOutVol, UpsOutCur , UpsBatVol, UpsBatTemp, UpsStatus, "
//                                             " UpsData1  , UpsData2 , UpsData3 , UpsData4  , UpsData5 , UpsData6 "
//                                       "from   TB_DEV_DATA_SNMP "
//                                       "where  ID = '%010u'   ; ", serial);
//    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);
//
//    int m_result = m_Mysql->mysql_runQuery(m_Query, 1);
//    if (m_result == 1)  // 쿼리정상 (Select)
//    {
//        m_Mysql->mysql_fetchRow();
//        if (phase == 1) //단상 Ups
//        {
//            m_Mysql->mysql_fetchfieldbyID(0, UpsInVol , sizeof(UpsInVol ));
//            m_Mysql->mysql_fetchfieldbyID(1, UpsInFreq, sizeof(UpsInFreq));
//            m_Mysql->mysql_fetchfieldbyID(2, UpsOutVol, sizeof(UpsOutVol));
//            m_Mysql->mysql_fetchfieldbyID(3, UpsOutCur, sizeof(UpsOutCur));
//            m_Mysql->mysql_fetchfieldbyID(4, UpsBatVol, sizeof(UpsBatVol));
//            m_Mysql->mysql_fetchfieldbyID(5, UpsBatTmp, sizeof(UpsBatTmp));
//            m_Mysql->mysql_fetchfieldbyID(6, UpsStatus, sizeof(UpsStatus));
//
//            pData->UpsInVol   = atoi(UpsInVol );
//            pData->UpsInFreq  = atoi(UpsInFreq);
//            pData->UpsOutVol  = atoi(UpsOutVol);
//            pData->UpsOutCur  = atoi(UpsOutCur);
//            pData->UpsBatVol  = atoi(UpsBatVol);
//            pData->UpsBatTemp = atoi(UpsBatTmp);
//            pData->UpsStatus  = atoi(UpsStatus);
//        }   else
//        if (phase == 3) //3상 UPS
//        {
//            m_Mysql->mysql_fetchfieldbyID(0, UpsInVol , sizeof(UpsInVol ));
//            m_Mysql->mysql_fetchfieldbyID(1, UpsInFreq, sizeof(UpsInFreq));
//            m_Mysql->mysql_fetchfieldbyID(2, UpsOutVol, sizeof(UpsOutVol));
//            m_Mysql->mysql_fetchfieldbyID(3, UpsOutCur, sizeof(UpsOutCur));
//            m_Mysql->mysql_fetchfieldbyID(4, UpsBatVol, sizeof(UpsBatVol));
//            m_Mysql->mysql_fetchfieldbyID(5, UpsBatTmp, sizeof(UpsBatTmp));
//            m_Mysql->mysql_fetchfieldbyID(6, UpsStatus, sizeof(UpsStatus));
//            
//            m_Mysql->mysql_fetchfieldbyID(7, UpsData[0], sizeof(UpsData[0]));
//            m_Mysql->mysql_fetchfieldbyID(8, UpsData[1], sizeof(UpsData[1]));
//            m_Mysql->mysql_fetchfieldbyID(9, UpsData[2], sizeof(UpsData[2]));
//            m_Mysql->mysql_fetchfieldbyID(10,UpsData[3], sizeof(UpsData[3]));
//            m_Mysql->mysql_fetchfieldbyID(11,UpsData[4], sizeof(UpsData[4]));
//            m_Mysql->mysql_fetchfieldbyID(12,UpsData[5], sizeof(UpsData[5]));
//
//            pData->UpsInVol    = atoi(UpsInVol  );
//            pData2->UpsData[0] = atoi(UpsData[0]);
//            pData2->UpsData[1] = atoi(UpsData[1]);
//            pData->UpsInFreq   = atoi(UpsInFreq );
//            pData->UpsOutVol   = atoi(UpsOutVol );
//            pData2->UpsData[2] = atoi(UpsData[2]);
//            pData2->UpsData[3] = atoi(UpsData[3]);
//            pData->UpsOutCur   = atoi(UpsOutCur );
//            pData2->UpsData[4] = atoi(UpsData[4]);
//            pData2->UpsData[5] = atoi(UpsData[5]);
//            pData->UpsBatVol   = atoi(UpsBatVol );
//            pData->UpsBatTemp  = atoi(UpsBatTmp );
//            pData->UpsStatus   = atoi(UpsStatus );
//        }
//
//        m_Mysql->mysql_free_res();
//    }   else
//    if (m_result == 0)  //쿼리결과가 없음
//    {
//    }   else
//    if (m_result <  0)    // 쿼리에러
//    {
//        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;35m%s()[%d] DB Query Fail (%s)(%s) !!!\x1b[0m", __FUNCTION__, __LINE__, m_Query, m_Mysql->mysql_Error());
//    }
//    return ;
//}


bool CDBInterface::getSnmpList(UPSSNMPLST * pUlst)
{
    char    Serial   [HEX_BUF] = {0,};
    char    UpsInVol [HEX_BUF] = {0,};
    char    UpsInFreq[HEX_BUF] = {0,};
    char    UpsOutVol[HEX_BUF] = {0,};
    char    UpsOutCur[HEX_BUF] = {0,};
    char    UpsBatVol[HEX_BUF] = {0,};
    char    UpsBatTmp[HEX_BUF] = {0,};
    char    UpsStatus[HEX_BUF] = {0,};
    char    UpsData  [6][HEX_BUF]    ;
    memset (UpsData, 0, sizeof(UpsData));
    memset (pUlst  , 0, SZ_UPSSNMPLST  );

    //snprintf(m_Query, sizeof(m_Query), "select ID, UpsInVol, UpsInFreq, UpsOutVol, UpsOutCur, UpsBatVol, UpsBatTemp, UpsStatus, "
    //                                         " UpsData1, UpsData2, UpsData3, UpsData4, UpsData5, UpsData6 "
    //                                   "from   TB_DEV_DATA_SNMP ; ");

    snprintf(m_Query, sizeof(m_Query), "SELECT s.ID, UpsInVol, UpsInFreq, UpsOutVol, UpsOutCur, UpsBatVol, UpsBatTemp, UpsStatus, "
                                       "       UpsData1, UpsData2, UpsData3, UpsData4, UpsData5, UpsData6 "
                                       "FROM   TB_DEV_DATA_SNMP s, "
                                       "       TB_DEV_INFO      d  "
                                       "WHERE  s.ID = d.ID         "
                                       "AND    d.ACTIVE = 1        "
                                       "AND    d.TYPE IN ('2','3');");
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);

    int m_result = m_Mysql->mysql_runQuery(m_Query, 1);
    if (m_result > 0)  // 쿼리정상 (Select)
    {
        pUlst->Count = (_UPS_MAX_DEV_ > m_result ? m_result : _UPS_MAX_DEV_);
        for ( int i = 0; i < pUlst->Count; i++ )
        {
            UPSSNMP * pUps = (UPSSNMP*)&pUlst->upsSnmp[i];
            m_Mysql->mysql_fetchRow();

            m_Mysql->mysql_fetchfieldbyID(0, Serial    , sizeof(Serial   ));
            m_Mysql->mysql_fetchfieldbyID(1, UpsInVol  , sizeof(UpsInVol ));
            m_Mysql->mysql_fetchfieldbyID(2, UpsInFreq , sizeof(UpsInFreq));
            m_Mysql->mysql_fetchfieldbyID(3, UpsOutVol , sizeof(UpsOutVol));
            m_Mysql->mysql_fetchfieldbyID(4, UpsOutCur , sizeof(UpsOutCur));
            m_Mysql->mysql_fetchfieldbyID(5, UpsBatVol , sizeof(UpsBatVol));
            m_Mysql->mysql_fetchfieldbyID(6, UpsBatTmp , sizeof(UpsBatTmp));
            m_Mysql->mysql_fetchfieldbyID(7, UpsStatus , sizeof(UpsStatus));
            m_Mysql->mysql_fetchfieldbyID(8, UpsData[0], sizeof(UpsData[0]));
            m_Mysql->mysql_fetchfieldbyID(9, UpsData[1], sizeof(UpsData[1]));
            m_Mysql->mysql_fetchfieldbyID(10,UpsData[2], sizeof(UpsData[2]));
            m_Mysql->mysql_fetchfieldbyID(11,UpsData[3], sizeof(UpsData[3]));
            m_Mysql->mysql_fetchfieldbyID(12,UpsData[4], sizeof(UpsData[4]));
            m_Mysql->mysql_fetchfieldbyID(13,UpsData[5], sizeof(UpsData[5]));

            pUps->Serial     = atoi(Serial    );
            pUps->UpsInVol   = atoi(UpsInVol  );
            pUps->UpsData[0] = atoi(UpsData[0]);
            pUps->UpsData[1] = atoi(UpsData[1]);
            pUps->UpsInFreq  = atoi(UpsInFreq );
            pUps->UpsOutVol  = atoi(UpsOutVol );
            pUps->UpsData[2] = atoi(UpsData[2]);
            pUps->UpsData[3] = atoi(UpsData[3]);
            pUps->UpsOutCur  = atoi(UpsOutCur );
            pUps->UpsData[4] = atoi(UpsData[4]);
            pUps->UpsData[5] = atoi(UpsData[5]);
            pUps->UpsBatVol  = atoi(UpsBatVol );
            pUps->UpsBatTemp = atoi(UpsBatTmp );
            pUps->UpsStatus  = atoi(UpsStatus );
        }
        m_Mysql->mysql_free_res();
    }   else
    if (m_result == 0)  //쿼리결과가 없음
    {
        pUlst->Count = 0;
        return true;
    }   else
    if (m_result <  0)    // 쿼리에러
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;35m%s()[%d] DB Query Fail (%s)(%s) !!!\x1b[0m", __FUNCTION__, __LINE__, m_Query, m_Mysql->mysql_Error());
        return false;
    }
    return true;
}


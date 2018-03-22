#include "DBInterface.h"
//extern Mutex     DLock  ;
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
    //DLock.lock();
    if (m_Mysql == NULL)
    {
        m_Mysql = new CMysql();
        if (m_Mysql == NULL)
        {
            pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s()[%d] Mysql Class Create Error (%s)\x1b[0m", __FUNCTION__, __LINE__, strerror(errno));
            //DLock.unlock();
            return false;
        }
    }

    // connect DB
    if (m_Mysql->mysql_connect(m_DBInf.host, m_DBInf.port, m_DBInf.user, m_DBInf.psword, m_DBInf.dbname, m_DBInf.timeout) < 0)
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s()[%d] Mysql Connection Fail (%s)\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error());
        //DLock.unlock();
        return false;
    }

    pLog.Print(LOG_DEBUG, 0, NULL, 0, "\x1b[1;33m%s()[%d] Mysql Connect Success !!! \x1b[0m", __FUNCTION__, __LINE__);
    //DLock.unlock();
    return true;
}

bool CDBInterface::DBClose()
{
    //DLock.lock();
    m_Mysql->mysql_close_conn();
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "\x1b[1;33m%s()[%d] Mysql Disconnect Success !!! \x1b[0m", __FUNCTION__, __LINE__);
    //DLock.unlock();
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



bool CDBInterface::uptDevDataT (GETDATA * pGetData)
{
    char sTmp[TMP_BUF] = {0,};
    memset(m_UCQry, 0, sizeof(m_UCQry));
    memset(m_UVQry, 0, sizeof(m_UVQry));
    
    for (int i = 0; i < UPS_3DATA_CNT; i++)
    {
        snprintf(sTmp, sizeof(sTmp), ",UpsData%d", i+1);
        strcat(m_UCQry, sTmp);
        snprintf(sTmp, sizeof(sTmp), ",%u", pGetData->Data2.UpsData[i]);
        strcat(m_UVQry, sTmp);
    }

    snprintf(m_Query, sizeof(m_Query), " INSERT  INTO TB_DEV_DATA_SNMP "
                                             "  (ID, UpsInVol, UpsInFreq, UpsOutVol, UpsOutCur, UpsBatVol, UpsBatTemp, UpsStatus %s) "
                                       " VALUES ('%010u', %u , %u, %u, %u, %u, %u, %u %s) "
                    			           " ON  DUPLICATE KEY UPDATE  "
                                               " ID         = VALUES(ID)       , "
                                               " UpsInVol   = VALUES(UpsInVol) , "
                                               " UpsInFreq  = VALUES(UpsInFreq), "
                                               " UpsOutVol  = VALUES(UpsOutVol), "
                                               " UpsOutCur  = VALUES(UpsOutCur), "
                                               " UpsBatVol  = VALUES(UpsBatVol), "
                                               " UpsBatTemp = VALUES(UpsBatTemp),"
                                               " UpsStatus  = VALUES(UpsStatus), "
                                               " UpsData1   = VALUES(UpsData1),  "
                                               " UpsData2   = VALUES(UpsData2),  "
                                               " UpsData3   = VALUES(UpsData3),  "
                                               " UpsData4   = VALUES(UpsData4),  "
                                               " UpsData5   = VALUES(UpsData5),  "
                                               " UpsData6   = VALUES(UpsData6);  ", m_UCQry, pGetData->serial,
                                                 pGetData->Data.UpsInVol  , pGetData->Data.UpsInFreq, pGetData->Data.UpsOutVol, pGetData->Data.UpsOutCur , pGetData->Data.UpsBatVol,
                                                 pGetData->Data.UpsBatTemp, pGetData->Data.UpsStatus, m_UVQry);
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);
    if (m_Mysql->mysql_runQuery(m_Query, 0) < 0)
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;33m%s[%d] (%s) [%s]\x1b[0m", __FUNCTION__, __LINE__, m_Mysql->mysql_Error(), m_Query);
        return false;
    }   else
        return true;
}


bool CDBInterface::getSNMPLst (SNMPLST * pSNMPLst)
{
    bool    ret = false;
    char    sid[HEX_BUF] = {0,};
    char    phs[HEX_BUF] = {0,};
    char    plg[HEX_BUF] = {0,};
    char    acv[HEX_BUF] = {0,};
    //memset (pSNMPLst, 0, SZ_SNMPLST);
    
    snprintf(m_Query, sizeof(m_Query), "select ID, SNMP_IP, PHASE, POLLING, ACTIVE from TB_DEV_INFO where TYPE in ('2','3');");
    pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] m_Query = [%s]", __FUNCTION__, __LINE__, m_Query);
    int m_result = m_Mysql->mysql_runQuery(m_Query, 1);
    if (m_result > 0)  // Äõ¸®Á¤»ó (Select)
    {
        pSNMPLst->Count = (_SNMP_MAX_CNT_ > m_result ? m_result : _SNMP_MAX_CNT_);
        for ( int i = 0; i < pSNMPLst->Count; i++ )
        {
            m_Mysql->mysql_fetchRow();
            m_Mysql->mysql_fetchfieldbyID(0, sid, sizeof(sid));
            m_Mysql->mysql_fetchfieldbyID(1, pSNMPLst->snmp[i].Ips, sizeof(pSNMPLst->snmp[i].Ips));
            m_Mysql->mysql_fetchfieldbyID(2, phs, sizeof(phs));
            m_Mysql->mysql_fetchfieldbyID(3, plg, sizeof(plg));
            m_Mysql->mysql_fetchfieldbyID(4, acv, sizeof(acv));
            pSNMPLst->snmp[i].serial  = atoi(sid);
            pSNMPLst->snmp[i].phase   = atoi(phs);
            pSNMPLst->snmp[i].active  = atoi(acv);
            pSNMPLst->snmp[i].polling = atoi(plg);
        }
        m_Mysql->mysql_free_res();
        ret = true;
    }   else
    if (m_result == 0)  //Äõ¸®°á°ú°¡ ¾øÀ½
    {
        pSNMPLst->Count = 0;
        ret = true;
    }   else
    if (m_result <  0)    // Äõ¸®¿¡·¯
    {
        pLog.Print(LOG_CRITICAL, 0, NULL, 0, "\x1b[1;35m%s()[%d] DB Query Fail (%s)(%s) !!!\x1b[0m", __FUNCTION__, __LINE__, m_Query, m_Mysql->mysql_Error());
        ret = false;
    }
    return ret;
}


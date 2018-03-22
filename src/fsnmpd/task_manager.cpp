#include "task_manager.h"
#include "fsnmpd_config.h"

extern CServerLog   pLog    ;
extern CConfig      pcfg    ;
extern bool         IsALive ;

CLstQueue           DBQueue ;

CTaskManager::CTaskManager ()
:   m_DBWrite   (NULL)
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

    if (m_DBWrite)
    {
        DBQueue.send_message(_CLOSE_, NULL, 0);

        SDL_WaitThread(m_DBWrite, NULL);
        m_DBWrite = NULL;

        CLst * pLst = NULL ;
        while ((pLst = DBQueue.get_message()) != NULL) {
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

    DBQueue.set_maxqueue(_EVENT_QUEUE_);
    m_DBWrite = SDL_CreateThread(DoDBWrite, this);
    SDL_CreateThread(DoDBRetry, NULL);

    pLog.Print(LOG_WARNING, 0, NULL, 0, "%s() Starting ...", __FUNCTION__);
    return true ;
}

void CTaskManager::freeData (void * f)
{
    if (f)
    {
        SNMP * snmp = (SNMP*)f;
        delete snmp;
        snmp = NULL;
    }
}


int CTaskManager::DoDBRetry (void * param)
{
    ////////////////////////////////////
    CDBInterface  DBInf(pcfg.getMyDB());
    SNMPLST     * snmpLst = new SNMPLST;
    memset(snmpLst, 0, SZ_SNMPLST)     ;
    ////////////////////////////////////
    CDataMap    * DevLst  = new CDataMap();
    DevLst->setfree(freeData);

    while (!DBInf.DBConnect())
        sleep(3);

    while (IsALive)
    {
        //map < uint32_t, void *, less<uint32_t> > * idata = DevLst->getIlist();
        //map < uint32_t, void *, less<uint32_t> >::iterator iter;
        //for ( iter = idata->begin(); iter != idata->end(); )
        //{
        //    SNMP * snmp  = (SNMP*)(*iter).second;
        //    snmp->active = false;
        //    ++iter  ;
        //}

        if (DBInf.getSNMPLst(snmpLst))
        {
            for (uint32_t i = 0; i < snmpLst->Count; i++)
            {
                pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] SNMP serial : [%010u] (%s)", __FUNCTION__, __LINE__, snmpLst->snmp[i].serial, snmpLst->snmp[i].Ips);
                SNMP * snmp = (SNMP*)DevLst->getdata(snmpLst->snmp[i].serial);
                if (snmp == NULL)
                {
                    SNMP * snmp = new SNMP;
                    memcpy(snmp, &snmpLst->snmp[i], SZ_SNMP);
                    DevLst->insert(snmp->serial, snmp);
                    SDL_CreateThread(DoSnmpGet , snmp);
                    SDL_Delay(10);
                }   else
                {
                    memcpy(snmp, &snmpLst->snmp[i], SZ_SNMP);
                }
            }
            sleep(10);
        }   else
        {
            DBInf.DBClose();
            while (!DBInf.DBConnect())
                sleep(3);
        }
    }

    DBInf.DBClose();
    if (DevLst)
    {
        delete DevLst ;
        DevLst = NULL ;
    }
    if (snmpLst)
    {
        delete snmpLst ;
        snmpLst = NULL ;
    }
    return 0;
}

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

static inline const uint8_t snmpGetEx(char * pOid, char * pIp, uint16_t * pv)
{
    FILE  * fp = NULL;
    char    pBuff[16  ] = {0,};
    char    pCmds[1024] = {0,};

    snprintf(pCmds, sizeof(pCmds), "snmpget -v 2c -c public -t 2 -r 0 %s %s |awk -F' ' '{print $4}'", pIp, pOid);
    // excute command
    fp = popen(pCmds, "r");
    if(!fp)
    {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "\x1b[1;34m%s() popen error [%d:%s] (pCmds = %s)\x1b[0m", __FUNCTION__, errno, strerror(errno), pCmds);
        return _FAIL_;
    }

    uint8_t icnt = 0;
    while(fgets(pBuff, sizeof(pBuff), fp) != NULL)
    {
        pv[icnt] = atoi(pBuff);
        icnt ++;
    }

    int ret = pclose(fp);
    if (ret)
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s() pclose error [%d]", __FUNCTION__, ret);

    if (icnt)
        return _ISOK_;
    else {
        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] (%s)", __FUNCTION__, __LINE__, pCmds);
        return _FAIL_;
    }
}

// snmpget -v 2c -c public -t 1 1.241.172.141  .1.3.6.1.4.1.935.1.1.1.3.2.1.0 |awk -F' ' '{print $4}'
char OIDStr1[256] = {".1.3.6.1.4.1.935.1.1.1.3.2.1.0 .1.3.6.1.4.1.935.1.1.1.3.2.4.0 .1.3.6.1.4.1.935.1.1.1.4.2.1.0 .1.3.6.1.4.1.935.1.1.1.4.2.3.0 .1.3.6.1.4.1.935.1.1.1.2.2.2.0 .1.3.6.1.4.1.935.1.1.1.2.2.3.0 .1.3.6.1.4.1.935.1.1.1.4.1.1.0"};
char OIDStr2[512] = {".1.3.6.1.4.1.935.1.1.1.8.2.2.0 .1.3.6.1.4.1.935.1.1.1.8.2.3.0 .1.3.6.1.4.1.935.1.1.1.8.2.4.0 .1.3.6.1.4.1.935.1.1.1.8.2.1.0 .1.3.6.1.4.1.935.1.1.1.8.3.2.0 .1.3.6.1.4.1.935.1.1.1.8.3.3.0 .1.3.6.1.4.1.935.1.1.1.8.3.4.0 .1.3.6.1.4.1.935.1.1.1.8.3.5.0 .1.3.6.1.4.1.935.1.1.1.8.3.6.0 .1.3.6.1.4.1.935.1.1.1.8.3.7.0 .1.3.6.1.4.1.935.1.1.1.8.1.1.0 .1.3.6.1.4.1.935.1.1.1.8.1.5.0 .1.3.6.1.4.1.935.1.1.1.4.1.1.0"};


int CTaskManager::DoSnmpGet (void * param)
{
    ////////////////////////////////////
    SNMP    * self  = (SNMP*)param  ;
    GETDATA * pData = new GETDATA   ;
    memset(pData, 0, SZ_GETDATA)    ;
    pData->serial = self->serial    ;

    pLog.Print(LOG_INFO, 0, NULL, 0, "%s() [%010u],[%s],[%u],[%u],[%u] Starting ...", __FUNCTION__, self->serial, self->Ips, self->phase, self->polling, self->active);
    while (true)
    {
        if (self->active)
        {
            if (strlen(self->Ips) <= 0)
            {
                pLog.Print(LOG_WARNING, 0, NULL, 0, "%s()[%d] Snmp_Ip is Empty !!! [serial = %010u]", __FUNCTION__, __LINE__, self->serial);
                sleep(self->polling);
                continue ;
            }

            memset(&pData->Data , 0, SZ_DATA );
            memset(&pData->Data2, 0, SZ_DATA2);
            if (self->phase == 1)     // 단상 UPS
            {
                uint16_t setVal[] = {0,0,0,0,0,0,0};
                                    //입력 전압  //입력 주파수 //출력 전압 //출력 전류(%) //배터리 전압 //배터리 온도 //UpsStatus
                if (_ISOK_ == snmpGetEx (OIDStr1, self->Ips, setVal))
                {
                    pData->Data.UpsInVol   = setVal[0] / 10;
                    pData->Data.UpsInFreq  = setVal[1] / 10;
                    pData->Data.UpsOutVol  = setVal[2] / 10;
                    pData->Data.UpsOutCur  = setVal[3] ;
                    pData->Data.UpsBatVol  = setVal[4] * 10;
                    pData->Data.UpsBatTemp = setVal[5] * 10;
                    pData->Data.UpsStatus  = setVal[6] ;
                }
            }   else
            if (self->phase == 3)     // 3상 UPS
            {
                uint16_t setVal[] = {0,0,0,0,0,0,0,0,0,0,0,0,0};
                                    //입력 전압 R      //입력 전압 S        //입력 전압 T          //입력 주파수
                                    //출력 전압 R      //출력 전압 S        //출력 전압 T
                                    //출력 전류(%) R   //출력 전류(%) S     //출력 전류(%) T       //배터리 전압       //배터리 온도        //UpsStatus
                if (_ISOK_ == snmpGetEx(OIDStr2, self->Ips, setVal))
                {
                    pData->Data.UpsInVol    = setVal[ 0] / 10;
                    pData->Data2.UpsData[0] = setVal[ 1] / 10;
                    pData->Data2.UpsData[1] = setVal[ 2] / 10;
                    pData->Data.UpsInFreq   = setVal[ 3] / 10;
                    pData->Data.UpsOutVol   = setVal[ 4] / 10;
                    pData->Data2.UpsData[2] = setVal[ 5] / 10;
                    pData->Data2.UpsData[3] = setVal[ 6] / 10;
                    pData->Data.UpsOutCur   = setVal[ 7] / 10;
                    pData->Data2.UpsData[4] = setVal[ 8] / 10;
                    pData->Data2.UpsData[5] = setVal[ 9] / 10;
                    pData->Data.UpsBatVol   = setVal[10] * 1;
                    pData->Data.UpsBatTemp  = setVal[11] * 10;
                    pData->Data.UpsStatus   = setVal[12] ;
                }
            }   else
            {
                pLog.Print(LOG_WARNING, 0, NULL, 0, "%s()[%d] Snmp Phase Number Unknown = (%u) !!! [serial = %010u]", __FUNCTION__, __LINE__, self->phase, self->serial);
                sleep(self->polling);
                continue ;
            }
            if (pData->Data.UpsStatus == 6)
                pData->Data.UpsStatus = APP_MBUS_SBIT_BYPASS_ON;
            else
            if (pData->Data.UpsStatus == 4)
                pData->Data.UpsStatus = APP_MBUS_SBIT_BAT_LOW;
            else
                pData->Data.UpsStatus = 0;

            pLog.Print(LOG_INFO , 0, NULL, 0, "\x1b[1;36m%s() snmpGet Completed (serial = %010u) !!!\x1b[0m", __FUNCTION__, self->serial);

            DBQueue.send_message(1, pData, SZ_GETDATA);
            sleep(self->polling-1);
        }   else
        {
            pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s() [%010u] Sleeping ...", __FUNCTION__, self->serial);
            sleep(self->polling);
        }
    }
    pLog.Print(LOG_INFO, 0, NULL, 0, "%s() [%010u] Stopping ...", __FUNCTION__, self->serial);
    if (pData)
    {
        delete pData;
        pData = NULL;    
    }
    return 0;
}


int CTaskManager::DoDBWrite (void * param)
{
    /////////////////////////////////////
    CLst        * pLst   = NULL         ;
    GETDATA     * pData  = NULL         ;
    uint32_t      size   = 0            ;
    CDBInterface  DBInf(pcfg.getMyDB()) ;
    uint32_t      TCount = 0            ;
    /////////////////////////////////////

    pLog.Print(LOG_INFO, 0, NULL, 0, "%s() Starting ...", __FUNCTION__);
    while (!DBInf.DBConnect())
        sleep(3);
    DBInf.setAutoCommit(0);

    while (SDL_SemWait(DBQueue.get_semaphore()) == 0)
    {
        TCount = 0;
        DBInf.BeginTran();
        while ((pLst = DBQueue.get_message()) != NULL)
        {
            if (!pLst->get_type())
            {
                DBInf.DBClose() ;
                delete pLst     ;
                pLst = NULL     ;
                return 0        ;
            }

            pData = (GETDATA*)pLst->get_message (size);
            if (!DBInf.uptDevDataT(pData))
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


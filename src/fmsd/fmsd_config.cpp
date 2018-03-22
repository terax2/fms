#include "fmsd_config.h"
extern bool       IsALive ;

CConfig::CConfig()
:   pConfig     (NULL),
    pSnmpLst    (NULL),
    pDBThread   (NULL),
    pDBInf      (NULL),
    pUpsSnmpLst (NULL),
    poUpsSnmpLst(NULL)
{
    pSnmpLst = new CDualbuff() ;
    pSnmpLst->setFree(freeData);

    pUpsSnmpLst  = new UPSSNMPLST;
    poUpsSnmpLst = new UPSSNMPLST;
    memset(pUpsSnmpLst , 0, SZ_UPSSNMPLST);
    memset(poUpsSnmpLst, 0, SZ_UPSSNMPLST);
}

CConfig::~CConfig()
{
    if (pSnmpLst)
    {
        delete pSnmpLst;
        pSnmpLst = NULL;
    }
    if (pUpsSnmpLst)
    {
        delete pUpsSnmpLst;
        pUpsSnmpLst = NULL;
    }
    if (poUpsSnmpLst)
    {
        delete poUpsSnmpLst;
        poUpsSnmpLst = NULL;
    }
    if (pDBThread)
    {
        SDL_WaitThread(pDBThread, NULL);
        pDBThread = NULL;
    }
    if (pDBInf)
    {
        delete pDBInf;
        pDBInf = NULL;
    }
}

void CConfig::freeData (void * f)
{
    if (f)
    {
        delete (UPSSNMP*)f;
        f = NULL;
    }
}

void CConfig::setLoadFile( char * pcfgName )
{
    m_cfgName = pcfgName ;
}

static inline const char * getIpaddr(const char * ifname)
{
    int s;
    struct ifconf ifconf;
    struct ifreq ifr[50];
    int ifs;
    int i;
    static char ipaddr[INET_ADDRSTRLEN];

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        perror("socket");
        return 0;
    }

    ifconf.ifc_buf = (char *) ifr;
    ifconf.ifc_len = sizeof ifr;

    if (ioctl(s, SIOCGIFCONF, &ifconf) == -1) {
        perror("ioctl");
        return 0;
    }

    ifs = ifconf.ifc_len / sizeof(ifr[0]);
    memset(ipaddr, 0, sizeof(ipaddr));
    for (i = 0; i < ifs; i++)
    {
        char ip[INET_ADDRSTRLEN];
        struct sockaddr_in *s_in = (struct sockaddr_in *) &ifr[i].ifr_addr;

        if (!inet_ntop(AF_INET, &s_in->sin_addr, ip, sizeof(ip)))
        {
            perror("inet_ntop");
            return 0;
        }

        if ( !memcmp(ifr[i].ifr_name, ifname, 4) && (strlen(ip) > 0) )
        {
            snprintf(ipaddr, sizeof(ipaddr), "%s", ip);
            break;
        }
    }

    close(s);
    return ipaddr;
}

static inline const int IsDomainName ( char * HostName, int len )
{
    for (int i = 0; i < len; i++ )
    {
        if (HostName[i] != 0x2E && (HostName[i] < 0x30 || HostName[i] > 0x39))
            return 1;
    }
    return 0;
}

void CConfig::setHostIP(char * pHost, int pLen)
{
    if (IsDomainName(pHost, pLen))
    {
        struct hostent * ht = gethostbyname(pHost);
        if (ht) {
//            for ( int i = 0; ht->h_addr_list[i]; i++ )
            snprintf(pHost, pLen, "%s", inet_ntoa(*(struct in_addr*)ht->h_addr_list[0]));
        }   else    {
            fprintf(stderr, "%s()[%d] invalid Domain Name (%s)\n", __FUNCTION__, __LINE__, pHost);
            memset(pHost, 0, pLen);
        }
    }
}

int Pos ( char * pBuf, int pLen, char sep)
{
    for (int i = 0; i < pLen; i++)
    {
        if (pBuf[i] == sep) return i;
    }
    return 0;
}


bool CConfig::getLoadConfig ()
{
    pConfig = new CParseConfig();
    if (!pConfig->Open(m_cfgName))
    {
        fprintf(stderr, "can't load FMS config file (%s)\n", m_cfgName);
        return false;
    }

    pConfig->GetInt   ("FMS"       , "DevicePort" , &DevicePort  , 8083      );
    pConfig->GetInt   ("FMS"       , "CommandPort", &CommandPort , 1882      );
    pConfig->GetInt   ("FMS"       , "SockBuff"   , &SockBuff    , 26524288  );
    pConfig->GetInt   ("FMS"       , "PoolCount"  , &PoolCount   , 5         );
    pConfig->GetInt   ("FMS"       , "LogLevel"   , &LogLevel    , 3         );
    pConfig->GetInt   ("FMS"       , "LogSize"    , &LogSize     , 1000000   );
    pConfig->GetString("FMS"       , "LogDir"     ,  LogDir      , sizeof(LogDir), "/FMS/log/fmsd");

    ////////////////////////////////// Mysql DB Info /////////////////////////////////
    memset(&myDB, 0, sizeof(DBINFO));
    pConfig->GetString("MYSQL"     , "Host"       ,  myDB.host   , sizeof(myDB.host)  , "127.0.0.1");
    setHostIP (myDB.host, sizeof(myDB.host));
    pConfig->GetInt   ("MYSQL"     , "Port"       , &myDB.port   , 3306      );
    pConfig->GetString("MYSQL"     , "User"       ,  myDB.user   , sizeof(myDB.user)  , "gablms");
    pConfig->GetString("MYSQL"     , "Password"   ,  myDB.psword , sizeof(myDB.psword), "gablms123");
    pConfig->GetString("MYSQL"     , "DBName"     ,  myDB.dbname , sizeof(myDB.dbname), "gablms");
    pConfig->GetInt   ("MYSQL"     , "Timeout"    , &myDB.timeout, 5         );
    pConfig->GetInt   ("MYSQL"     , "RetryTime"  , &myDB.retry  , 60        );

    ////////////////////////////////////////////////////////////////////////////////////////////////
    memset(&NodeJS, 0, sizeof(Address));
    pConfig->GetString("NODEJS"    , "NodeJSHost" ,  NodeJS.ip   , sizeof(NodeJS.ip)  , "127.0.0.1");
    pConfig->GetInt   ("NODEJS"    , "NodeJSPort" , &NodeJS.port , 880       );
    setHostIP(NodeJS.ip, sizeof(NodeJS.ip));

    ////////////////////////////////////////////////////////////////////////////////////////////////
    memset(&SmsInfo, 0, sizeof(Address));
    pConfig->GetString("SMS"       , "SMSHost"    ,  SmsInfo.ip  , sizeof(SmsInfo.ip) , "172.17.32.232");
    pConfig->GetInt   ("SMS"       , "SMSPort"    , &SmsInfo.port, 3264      );
    setHostIP(SmsInfo.ip, sizeof(SmsInfo.ip));

    ////////////////////////////////////////////////////////////////////////////////////////////////
    pConfig->GetInt   ("EPOLL"     , "EpMaxCon"   , &EpMaxCon    , 1500      );
    pConfig->GetInt   ("EPOLL"     , "EpEvent"    , &EpEvent     , 1200      );
    pConfig->GetInt   ("EPOLL"     , "NetTimeout" , &NetTimeout  , 60        );
    ////////////////////////////////////////////////////////////////////////////////////////////////
    memset(&LoraInfo, 0, sizeof(Address));
    pConfig->GetString("LORA"      , "LORAHost"   ,  LoraInfo.ip  , sizeof(LoraInfo.ip) , "172.17.32.232");
    pConfig->GetInt   ("LORA"      , "LORAPort"   , &LoraInfo.port, 8084     );
    pConfig->GetInt   ("LORA"      , "RefreshTime", &LoraRefresh  , 10       );
    pConfig->GetInt   ("LORA"      , "FailTime"   , &LoraFailTime , 1320     );

    pConfig->Close();
    if (pConfig)
    {
        delete pConfig ;
        pConfig = NULL ;
    }

    if (pDBInf == NULL)
        pDBInf = new CDBInterface(myDB);
    pDBThread = SDL_CreateThread(DoDBLoad, this);
    SnmpListUpdate();
    return  true;
}

void CConfig::MakeGenerate()
{
    if (!memcmp(poUpsSnmpLst,pUpsSnmpLst,SZ_UPSSNMPLST))
        return;
    memcpy(poUpsSnmpLst, pUpsSnmpLst, SZ_UPSSNMPLST);

    pSnmpLst->setClear();
    for ( int i = 0; i < pUpsSnmpLst->Count; i++ )
    {
        UPSSNMP * pUps = (UPSSNMP*)&pUpsSnmpLst->upsSnmp[i];
        UPSSNMP * nUps = (UPSSNMP*)pSnmpLst->nextData(pUps->Serial);
        if (nUps == NULL)
        {
            nUps = new UPSSNMP;
            pSnmpLst->setUpdate(pUps->Serial, nUps);
        }
        memcpy(nUps, pUps, SZ_UPSSNMP);
        pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] nUps->Serial = [%010u]", __FUNCTION__, __LINE__, nUps->Serial);
    }
    pSnmpLst->setActive();
    pLog.Print(LOG_WARNING, 0, NULL, 0, "\x1b[1;33m%s()[%d] pUpsSnmpLst Update !!!\x1b[0m", __FUNCTION__, __LINE__);
}

void CConfig::SnmpListUpdate()
{
    if (pDBInf)
    {
        if (pDBInf->DBConnect())
        {
            if (pDBInf->getSnmpList(pUpsSnmpLst))
                MakeGenerate();
            pDBInf->DBClose();
        }
    }
}

int CConfig::DoDBLoad (void * param)
{
    if ( param == NULL )    return -1   ;
    CConfig * self = (CConfig*)param    ;
    if ( self  == NULL )    return -100 ;

    while (IsALive)
    {
        sleep(10);
        self->SnmpListUpdate();
    }
    return 0;
}


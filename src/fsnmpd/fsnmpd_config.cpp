#include "fsnmpd_config.h"

CConfig::CConfig()
:   pConfig(NULL)
{
}

CConfig::~CConfig()
{
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

    pConfig->GetInt   ("FMS"       , "LogLevel"   , &LogLevel    , 3         );
    pConfig->GetInt   ("FMS"       , "LogSize"    , &LogSize     , 1000000   );
    pConfig->GetString("FMS"       , "LogDir"     ,  LogDir      , sizeof(LogDir), "/FMS/log/fsnmpd");

    ////////////////////////////////// Mysql DB Info /////////////////////////////////
    memset(&myDB, 0, sizeof(DBINFO));
    pConfig->GetString("MYSQL"     , "Host"       ,  myDB.host   , sizeof(myDB.host)  , "127.0.0.1");
    setHostIP (myDB.host, sizeof(myDB.host));
    pConfig->GetInt   ("MYSQL"     , "Port"       , &myDB.port   , 3306      );
    pConfig->GetString("MYSQL"     , "User"       ,  myDB.user   , sizeof(myDB.user)  , "ums");
    pConfig->GetString("MYSQL"     , "Password"   ,  myDB.psword , sizeof(myDB.psword), "gablms123");
    pConfig->GetString("MYSQL"     , "DBName"     ,  myDB.dbname , sizeof(myDB.dbname), "UMS");
    pConfig->GetInt   ("MYSQL"     , "Timeout"    , &myDB.timeout, 5         );
    pConfig->GetInt   ("MYSQL"     , "RetryTime"  , &myDB.retry  , 60        );

    pConfig->Close();
    if (pConfig)
    {
        delete pConfig ;
        pConfig = NULL ;
    }

    return  true;
}


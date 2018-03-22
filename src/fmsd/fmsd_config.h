#ifndef __CONFIG_FMSD_H__
#define __CONFIG_FMSD_H__

#include "camera.h"
#include "dualbuff.h"
#include "DBInterface.h"

#define DEFAULT_CONFIG_FILE  ("/fsapp/FMS/conf/fmsd.conf")

class   CConfig
{
    public:
        CConfig  ();
        virtual  ~CConfig();

        void     setLoadFile    ( char * pcfgName );
        bool     getLoadConfig  ();

        uint16_t getDevicePort  ()      { return  DevicePort; };
        uint16_t getCommandPort ()      { return  CommandPort;};
        uint32_t getSockBuff    ()      { return  SockBuff;   };
        uint16_t getPoolCount   ()      { return  PoolCount;  };

        int      getLogLevel    ()      { return  LogLevel;   };
        int      getLogSize     ()      { return  LogSize;    };
        char   * getLogDir      ()      { return  LogDir;     };
        
        DBINFO   getMyDB        ()      { return  myDB;       };
        Address  getNodeJS      ()      { return  NodeJS;     };
        Address  getSMSInf      ()      { return  SmsInfo;    };

        uint16_t getEpMaxCon    ()      { return  EpMaxCon;   };
        uint16_t getEpEvent     ()      { return  EpEvent;    };
        uint16_t getNetTimeout  ()      { return  NetTimeout; };
        
        Address  getLoraInfo    ()      { return  LoraInfo;    };
        uint16_t getRefreshTime ()      { return  LoraRefresh; };
        uint16_t getLoraFailTime()      { return  LoraFailTime;};
        
        UPSSNMP* getUpsSnmp     (uint32_t key) { return (UPSSNMP*)pSnmpLst->getData(key); };

        void     setHostIP      (char * pHost, int pLen);

    private:
        static void     freeData(void * f);
        static int      DoDBLoad        (void * param);
        void            MakeGenerate    ();
        void            SnmpListUpdate  ();

    private:
        char         *  m_cfgName       ;
        CParseConfig *  pConfig         ;
        CDualbuff    *  pSnmpLst        ;
        SDL_Thread   *  pDBThread       ;
        CDBInterface *  pDBInf          ;
        UPSSNMPLST   *  pUpsSnmpLst     ;
        UPSSNMPLST   *  poUpsSnmpLst    ;
        Mutex           pMutex          ;

        int             DevicePort      ;
        int             CommandPort     ;
        int             SockBuff        ;
        int             PoolCount       ;
        int             LogLevel        ;
        int             LogSize         ;
        char            LogDir[FUL_FLE] ;
                        
        DBINFO          myDB            ;
        Address         NodeJS          ;
        Address         SmsInfo         ;
                        
        int             EpMaxCon        ;
        int             EpEvent         ;
        int             NetTimeout      ;
                        
        Address         LoraInfo        ;
        int             LoraRefresh     ;
        int             LoraFailTime    ;
};

#endif /* __CONFIG_FMSD_H__ */

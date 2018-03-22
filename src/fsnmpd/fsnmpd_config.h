#ifndef __CONFIG_FMSD_H__
#define __CONFIG_FMSD_H__

#include "camera.h"

#define DEFAULT_CONFIG_FILE  ("/fsapp/FMS/conf/fsnmpd.conf")

class   CConfig
{
    public:
        CConfig  ();
        virtual  ~CConfig();

        void     setLoadFile    ( char * pcfgName );
        bool     getLoadConfig  ();

        int      getLogLevel    ()      { return  LogLevel;   };
        int      getLogSize     ()      { return  LogSize;    };
        char   * getLogDir      ()      { return  LogDir;     };
        DBINFO   getMyDB        ()      { return  myDB;       };
        void     setHostIP      (char * pHost, int pLen);


    private:
        char  *  m_cfgName      ;
        CParseConfig * pConfig  ;

        int      LogLevel       ;
        int      LogSize        ;
        char     LogDir         [FUL_FLE];

        DBINFO   myDB           ;

};

#endif /* __CONFIG_FMSD_H__ */

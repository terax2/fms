#ifndef __DB_INTERFACE_H__
#define __DB_INTERFACE_H__

#include "camera.h"
#include "mysql_inf.h"

extern  CServerLog  pLog ;

/*  CLASS DEFINE  */
class CDBInterface
{
    public:
        CDBInterface(DBINFO pDB);
        ~CDBInterface();

        bool        DBConnect       ();
        bool        DBClose         ();
        bool        setAutoCommit   (uint8_t acommit);
        bool        IsConnected     ();
        bool        BeginTran       ();
        bool        EndTran         ();

    public:
        /////////////////////////////////////////////////
        bool        uptDevDataT     (GETDATA * pGetData);
        bool        uptDevData      (GETDATA * pGetData);
        bool        uptCurClear     ();
        bool        evtDevOccur     (EVENTS  * pEvent  );
        bool        evtDevClear     (EVENTS  * pEvent  );
        //bool        evtDevClear     (uint32_t  Serial  );
        bool        getEvtMsg       (EVENTS  * pEvent  );
        void        getEvtNumStr    (char * NumStr, char * LocCode);
        ///////////////////////////////////////////////////////////////////
        uint8_t     uptDevOpInfoT   (DEVINFO * pDevInfo);
        uint8_t     getDevdef       (DEVINFO * pDevInfo, DEVDEF * pDevdef, bool DevUpt = false);
        uint8_t     setDevAct       (DEVINFO * pDevInfo, DEVDEF * pDevdef);
        
        bool        getSNMPLst      (SNMPLST * pSNMPLst);
        //void        setSnmpData     (uint32_t serial, DATA * pData, DATA2 * pData2, uint8_t phase);
        bool        getSnmpList     (UPSSNMPLST * pUlst);

    private:
        ///////////////////////////////////////////////////////////////////
        uint8_t     chkDevOpInfo    (uint32_t  Serial  , DEVDEF * pDevdef);
        uint8_t     chkDevPool      (uint32_t  Serial  );
        uint8_t     setPoolActive   (uint32_t  Serial  );
        ///////////////////////////////////////////////////////////////////
        uint8_t     getDevOpLimit   (uint32_t  Serial  , DEVDEF * pDevdef);
        //uint8_t     uptDevOpInfo    (DEVINFO * pDevInfo);
        ///////////////////////////////////////////////////////////////////
        uint8_t     regDevPool      (DEVINFO * pDevInfo);
        uint8_t     regDevOpInfo    (uint32_t  Serial  );
        uint8_t     regDevOpLimit   (uint32_t  Serial  );
        ///////////////////////////////////////////////////////////////////

    private:
        CMysql    * m_Mysql         ;
        DBINFO      m_DBInf         ;
        char        m_Query         [SMI_BUF];
        char        m_SCQry         [BIG_BUF];
        char        m_SVQry         [MID_BUF];
        char        m_UCQry         [MIN_BUF];
        char        m_UVQry         [MIN_BUF];
};

#endif

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
        bool        uptDevDataT     (GETDATA * pGetData);
        bool        getSNMPLst      (SNMPLST * pSNMPLst);


    private:
        CMysql    * m_Mysql         ;
        DBINFO      m_DBInf         ;
        char        m_Query         [SMI_BUF];
        char        m_UCQry         [MIN_BUF];
        char        m_UVQry         [SML_BUF];
};

#endif

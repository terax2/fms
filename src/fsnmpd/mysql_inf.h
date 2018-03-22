#ifndef __CMYSQL_INF_H__
#define __CMYSQL_INF_H__

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <mysql/mysql.h>

/*  CLASS DEFINE  */
class CMysql
{
    private:
        MYSQL       mysql       ;
        MYSQL_RES * res         ;
        MYSQL_ROW   row         ;
        char        error [512] ;

    public:
        CMysql ();
        ~CMysql();

        int         mysql_connect(char *myHost, int myPort, char *myUser, char *myPasswd, char *myDb, int myTimeout);
        int         mysql_runQuery(char *query, int return_row);
        int         mysql_fetchRow();
        int         mysql_fieldCount();
        void        mysql_fetchfieldbyID(int id, char *buffer, int len);
        void        mysql_fetchfieldbyName(char *name, char *buffer, int len);
        void        mysql_fetchfieldbyName_Debug(char *name, char *buffer, int len);
        void        mysql_free_res();
        bool        mysql_check_conn();
        void        mysql_escape_string(char *chunk, char *image, int size);
        void        mysql_close_conn();
        char      * mysql_Error();

        bool        mysql_Start();
        bool        mysql_Commit();
        bool        mysql_Rollback();
        bool        mysql_AutoCommit(u_int8_t commit);
};

#endif

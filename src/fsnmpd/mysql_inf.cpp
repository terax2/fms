/* INCLUDE */
#include "mysql_inf.h"

CMysql::CMysql ()
{
    res = NULL;
    mysql_init(&mysql);
}

CMysql::~CMysql()
{
    mysql_free_res();
    mysql_close_conn();
}

int CMysql::mysql_connect(char *myHost, int myPort, char *myUser, char *myPasswd, char *myDb, int myTimeout)
{
    //unsigned int timeout = myTimeout;
    //mysql_options(&mysql, MYSQL_OPT_CONNECT_TIMEOUT, (const char *)&timeout);
    bool bConnect = 1;
    mysql_options(&mysql, MYSQL_OPT_RECONNECT, &bConnect);
    if( !mysql_real_connect( &mysql, myHost, myUser,myPasswd,myDb, myPort, (char*)NULL, 0) )
    {
        mysql_close( &mysql );
        return -1;
    }
    return 0;
}

bool CMysql::mysql_Start()
{
    char Cmds[32] = {0,};
    snprintf(Cmds, sizeof(Cmds), "START TRANSACTION;");
    if (mysql_runQuery(Cmds, 0) < 0)
    {
        return false;
    }   else
        return true ;
}

bool CMysql::mysql_Commit()
{
    char Cmds[32] = {0,};
    snprintf(Cmds, sizeof(Cmds), "COMMIT;");
    if (mysql_runQuery(Cmds, 0) < 0)
    {
        return false;
    }   else
        return true ;
}

bool CMysql::mysql_Rollback()
{
    char Cmds[32] = {0,};
    snprintf(Cmds, sizeof(Cmds), "ROLLBACK;");
    if (mysql_runQuery(Cmds, 0) < 0)
    {
        return false;
    }   else
        return true ;
}

bool CMysql::mysql_AutoCommit(u_int8_t commit)
{
    char Cmds[32] = {0,};
    snprintf(Cmds, sizeof(Cmds), "SET autocommit = %u;", commit);
    if (mysql_runQuery(Cmds, 0) < 0)
    {
        return false;
    }   else
        return true ;
}

int CMysql::mysql_runQuery(char *query, int return_row)
{
    int num_rows = 0 ;
    if(mysql_query( &mysql, query ) != 0)
        return -1;

    if( return_row == 1)
    {
        res = mysql_store_result( &mysql );

        num_rows = (int)mysql_num_rows( res );
        if ( num_rows == 0 )
        {
            mysql_free_res();
            return num_rows ;
        }   else
            return num_rows ;
    }   else
        return 0;
}

int CMysql::mysql_fetchRow()
{
    if( res )
    {
        row = mysql_fetch_row( res );
        if( !row )
        {
            return -1;
        }
        return 0;
    }
    return -2;
}

void CMysql::mysql_fetchfieldbyID(int id, char *buffer, int len)
{
    if( ( row[id]) )
    {
        memcpy(buffer, row[id], len);
    }
}

int CMysql::mysql_fieldCount()
{
    int num_fields = 0;
    if( res )
    {
        num_fields = mysql_num_fields( res );
    }
    return num_fields;
}

void CMysql::mysql_fetchfieldbyName(char *name, char *buffer, int len)
{
    MYSQL_FIELD *fields;
    int num_fields;
    int i;

    if( ( res ) )
    {
        num_fields = mysql_num_fields( res );
        fields = mysql_fetch_fields( res );
        for(i = 0; i < num_fields; i++)
        {
            if(!strcmp(fields[i].name, name))
            {
                if( row[i] )
                {
                    strncpy(buffer,  row[i], len);
                    return;
                }
            }
        }
    }
}

void CMysql::mysql_fetchfieldbyName_Debug(char *name, char *buffer, int len)
{
    MYSQL_FIELD *fields;
    int num_fields;
    int i;

    if( ( res ) )
    {
        num_fields = mysql_num_fields( res );
        fields = mysql_fetch_fields( res );
        for(i = 0; i < num_fields; i++)
        {
            if(!strcmp(fields[i].name, name))
            {
                if( row[i] )
                {
                    strncpy(buffer,  row[i], len);
                    return;
                }
            }
        }
    }
}

void CMysql::mysql_free_res()
{
    if ( res != NULL )
    {
        mysql_free_result( res );
        res = NULL;
    }
}

void CMysql::mysql_escape_string(char *chunk, char *image, int size)
{
    mysql_real_escape_string( &mysql, chunk, image, size);
}

bool CMysql::mysql_check_conn()
{
    return !mysql_ping(&mysql);
}

void CMysql::mysql_close_conn()
{
    mysql_close( &mysql );
}

char * CMysql::mysql_Error()
{
    snprintf(error, sizeof(error), "(%d), %s", mysql_errno(&mysql), mysql_error(&mysql));
    return error ;
}

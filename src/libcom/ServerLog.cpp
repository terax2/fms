//--------------------------------------------------------------------------------------------------------------
/** @file       ServerLog.cpp
    @date   2011/05/18
    @author �ۺ���(sbs@gabia.com)
    @brief  ���� �α� ��� Ŭ����.   \n
*///------------------------------------------------------------------------------------------------------------


/*  INCLUDE  */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdarg.h>
#include <sys/time.h>
#include "ServerLog.h"
#include "Utility.h"


CServerLog::CServerLog()
{
    m_pszDirName = NULL;
    m_psLogFileName = NULL;

    szBuf = NULL;
    szBuf = new char[LOG_MAX_SIZE];

    memset(m_szDate, 0, DATE_SIZE);
    memset(m_szFileName, 0, FILE_NAME_MAX_SIZE);
    memset(m_new_szFileName, 0, FILE_NAME_MAX_SIZE);

    memset(szBuf, 0, LOG_MAX_SIZE);
    memset(szDate, 0, DATE_SIZE);
    memset(szHeader, 0, HEADER_SIZE);

    m_sttFd = NULL;
    m_iLevel = 0;
    m_iMaxLogSize = 0;
    m_iLogSize = 0;
    m_iIndex = 1;
    m_wMode = 0;

    mutx_p = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutx_p, NULL);
}


CServerLog::~CServerLog()
{
    if(m_sttFd != NULL)
        fclose(m_sttFd);

    if(szBuf != NULL)
        delete []szBuf;

    if(m_pszDirName != NULL)
        delete []m_pszDirName;

    if(m_psLogFileName != NULL)
        delete []m_psLogFileName;

    pthread_mutex_destroy(mutx_p);
    free(mutx_p);
}


/*********************************************************************************************
* IN : pszDirName: �α��������� ���丮 ���
* OUT : Success=0, Fail=-1
* Comment : �α������� ������ ���丮 ��θ� ����,
                  CreateDir�� ���丮��� ����
*********************************************************************************************/
int CServerLog::SetDirectory(const char *pszDirName)
{
    if(pszDirName == NULL) return -1;

    if(m_pszDirName == NULL)
    {
        int iLen = (int)strlen(pszDirName);
        m_pszDirName = new char[iLen + 1];

        if(m_pszDirName == NULL) return -1;

        sprintf(m_pszDirName, "%s", pszDirName);

        if(m_pszDirName[iLen-1] == '/')
        {
            m_pszDirName[iLen-1] = '\0';
        }
    }

    //���丮 ����
    if(CDirectory::Create(m_pszDirName) != 0 ) return -1;
    return 0;
}


/*********************************************************************************************
* IN : psLogFileName: �α��������� �̸�
* OUT : Success=0, Fail=-1
* Comment : �α����� �̸�����
*********************************************************************************************/
int CServerLog::SetLogFileName(const char *psLogFileName)
{
    if(psLogFileName == NULL) return -1;

    if(m_psLogFileName == NULL)
    {
        int iLen = (int)strlen(psLogFileName);
        m_psLogFileName = new char[iLen + 1];

        if(m_psLogFileName == NULL) return -1;

        sprintf(m_psLogFileName, "%s", psLogFileName);

        if(m_psLogFileName[iLen-1] == '/')
        {
            m_psLogFileName[iLen-1] = '\0';
        }
    }
    return 0;
}


/*********************************************************************************************
* IN : iLevel: �α׷���
* OUT : int
* Comment : �α׷�������
*********************************************************************************************/
void CServerLog::SetLevel(int iLevel)
{
    m_iLevel = iLevel;
}


/*********************************************************************************************
* IN : iSize:�ִ� �α׻�����
* OUT : void
* Comment : �ִ�α׻�����
*********************************************************************************************/
void CServerLog::SetMaxLogSize(int iSize)
{
    m_iMaxLogSize = iSize;
}


/*********************************************************************************************
* IN : void
* OUT : �α��ε�����ȣ(default 1)
* Comment : �ε�����ȣ����
*********************************************************************************************/
int CServerLog::GetLogIndex()
{
    return m_iIndex;
}


/*********************************************************************************************
* IN :  enum �α׷���
* OUT : Print ��� True/False
* Comment : �α׷����� Ȯ���Ͽ� ���缳���� ���Ͽ� �����뿩�� ����
*********************************************************************************************/
//bool CServerLog::IsPrintLogLevel(EnumLogLevel iLevel)
bool CServerLog::IsPrintLogLevel(int iLevel)
{
    if(m_iLevel > iLevel)
    {
        return false;
    }

    return true;
}


//--------------------------------------------------------------------------------------------------------------
/** @brief �Է¹��� ������ 16���� ��簪���� �����Ͽ� ���
    @param pFp: LogFile FD
    @param pTranBuff: ��ȯ�� ���ڿ� �ּ�
    @param iDispLen: ��ȯ ���ڿ� ���� ũ��
    @return true/false
*///------------------------------------------------------------------------------------------------------------
bool    CServerLog::ConvDump(FILE *pFp, char *pTranBuff, int iDispLen)
{
    char    sHexaBuff[HEX_BUF_SIZE ];
    char    sTempBuff[TEMP_BUF_SIZE];
    char    sASCBuff [ASC_BUF_SIZE ];
    int     iColumn    = 0;
    int     iWalker    = 0;
    int     iLineCount = 0;

    memset(sHexaBuff, 0, sizeof(sHexaBuff));
    memset(sASCBuff , 0, sizeof(sASCBuff) );
    m_iLogSize += fprintf(pFp, " ADDR |                         DUMP                        |      ASCII     |\n");
    for(iWalker = 0; iWalker < iDispLen; iWalker++)
    {
        snprintf(sTempBuff, sizeof(sTempBuff), "%02x%s", (*pTranBuff & 0x000000FF), (iColumn != 7)?" ":" - ");
        strcat(sHexaBuff, sTempBuff);
        if( ((*pTranBuff & 0x000000FF) < 0x20) || ((*pTranBuff & 0x000000FF) > 0x7f) )
            sASCBuff[iColumn] = '.';
        else
            sASCBuff[iColumn] = *pTranBuff;

        if(iColumn++ == 15)
        {
            m_iLogSize += fprintf(pFp, "%06d| %-50s  |%-16s|\n", iLineCount, sHexaBuff, sASCBuff);
            memset(sASCBuff , 0, sizeof(sASCBuff) );
            memset(sHexaBuff, 0, sizeof(sHexaBuff));
            iColumn     = 0;
            iLineCount += 16;
        }
        pTranBuff++;
    }
    if( iColumn > 0)
    {
        m_iLogSize += fprintf(pFp, "%06d| %-50s  |%-16s|\n", iLineCount, sHexaBuff, sASCBuff);
    }

    return true;
}


/**********************************************************************************************************
* IN : ȯ�漳��
* OUT : Success:0 Fail:-1
* Comment : �α׼����� ���� ȯ�漳�� �� ���� (logFileDir, logFileName, LogLevel, maxLogFileSize)
***********************************************************************************************************/
int CServerLog::SetupLog(const char *pszDirName, const char *psLogFileName, int iLevel, int iSize)
{
//  printf("pszDirName:%s,  psLogFileName:%s, Level:%d, iSIze:%d\n", pszDirName, psLogFileName, iLevel, iSize);

    if( ( SetDirectory(pszDirName)) != 0)
    {
          printf("[ERROR][LOG] SetDirectory Error\n");
          return -1;
    }

    if( ( SetLogFileName(psLogFileName)) != 0)
    {
          printf("[ERROR][LOG] SetLogFileName Error\n");
          return -1;
    }

    SetLevel(iLevel);
    SetMaxLogSize(iSize);

    return 0;
}


/**********************************************************************************************************
* IN :
* OUT :
* Comment :
***********************************************************************************************************/
//void CServerLog::SetszHeader(EnumLogLevel iLevel)
void CServerLog::SetszHeader(int iLevel)
{
    switch(iLevel)
    {
    case LOG_DEBUG:
        snprintf( szHeader, sizeof(szHeader), "[\x1b[1;36mDEB\x1b[0m] " );
        break;
    case LOG_INFO:
        snprintf( szHeader, sizeof(szHeader), "[\x1b[1;34mINF\x1b[0m] " );
        break;
    case LOG_WARNING:
        snprintf( szHeader, sizeof(szHeader), "[\x1b[1;33mWAR\x1b[0m] " );
        break;
    case LOG_ERROR:
        snprintf( szHeader, sizeof(szHeader), "[\x1b[1;35mERR\x1b[0m] " );
        break;
    case LOG_CRITICAL:
        snprintf( szHeader, sizeof(szHeader), "[\x1b[1;31mCRI\x1b[0m] " );
        break;
    default:
        memset( szHeader, 0, sizeof(szHeader) );
        break;
    }
}



/*********************************************************************************************
* IN : iType: �α�����, iLevle:�α׷���, wMode: �α�����, ����, ����ũ��, fmd:��������
* OUT : Success=0, Fail=-1
* Comment : �α׷����� ���� �α����, ��¥����� ���ο����ϻ���
                  ������� : [��¥] [�ð�] [�α׷���][��³���]
                  Printf(SYS, LOG_DEBUF, 1, buf, bufLen, "print %s", string)
*********************************************************************************************/
//int CServerLog::Print( EnumLogLevel iLevel, int wMode,  char *pBuff, int iBuffLen, const char *fmt, ...  )
int CServerLog::Print( int iLevel, int wMode,  char *pBuff, int iBuffLen, const char *fmt, ...  )
{
    struct tm           sttTm;
    struct timeval      sttTime;

   //�α׷����� 0�̸� ��Ͼ���
    if(m_iLevel  == 0)
        return -1;

    pthread_mutex_lock(mutx_p);

    //�α׷��� ���Ͽ� ���� ���� �̻� ���
    if(IsPrintLogLevel(iLevel) == 0)
    {
        pthread_mutex_unlock(mutx_p);
        return -1;
    }

    // �α׷����� ���� ������� [DEBUG] [ERROR]
    SetszHeader(iLevel);

    gettimeofday(&sttTime, NULL);
    localtime_r( &sttTime.tv_sec, &sttTm);

    // ù �α��ۼ�
    if(m_sttFd == NULL)
    {
        // �ð������Ͽ� Ŭ������ ����
        snprintf(m_szDate, sizeof(m_szDate), "%04d%02d%02d", sttTm.tm_year + 1900, sttTm.tm_mon + 1, sttTm.tm_mday);
        snprintf(m_szFileName, sizeof(m_szFileName), "%s/%s.%04d%02d%02d.log", m_pszDirName, m_psLogFileName, sttTm.tm_year + 1900, sttTm.tm_mon + 1, sttTm.tm_mday);

        m_sttFd = fopen( m_szFileName, "a" );
        if(m_sttFd == NULL)
        {
            pthread_mutex_unlock(mutx_p);
            return -1;
        }

        struct  stat    sttStat;
        lstat(m_szFileName, &sttStat);
        m_iLogSize = sttStat.st_size ;
    }
    else  //�̾ ����
    {
        snprintf(szDate, sizeof(szDate), "%04d%02d%02d", sttTm.tm_year + 1900, sttTm.tm_mon + 1, sttTm.tm_mday);
        //������ ��¥�� Ŭ������ ����� ���ڸ� ���Ͽ� �α������� ���� ����
        if( m_pszDirName && memcmp(m_szDate, szDate, DATE_SIZE-1) )
        {
            if( m_sttFd )
            {
                fclose( m_sttFd );
                m_sttFd = NULL;
            }

            snprintf( m_szFileName, sizeof( m_szFileName ), "%s/%s.%04d%02d%02d.log",
                m_pszDirName, m_psLogFileName, sttTm.tm_year + 1900, sttTm.tm_mon + 1, sttTm.tm_mday );

            m_sttFd = fopen( m_szFileName, "a" );
            if( m_sttFd == NULL )
            {
                pthread_mutex_unlock(mutx_p);
                return -1;
            }
            m_iIndex = 1;
            m_iLogSize = 0;
            snprintf(m_szDate, sizeof(m_szDate), "%s", szDate);
        }

        //�α�����ũ�Ⱑ m_iMaxLogSize�� �α������̸��� filename.m_iIndex �ιٲ�
        if( m_iMaxLogSize > 0 )
        {
            if (m_iLogSize >= m_iMaxLogSize)
            {
                if(m_sttFd)
                {
                    fclose(m_sttFd);
                    m_sttFd = NULL;
                }

                snprintf( m_new_szFileName, sizeof(m_new_szFileName), "%s/%s.%04d%02d%02d.log.%03d",
                    m_pszDirName, m_psLogFileName, sttTm.tm_year + 1900, sttTm.tm_mon + 1, sttTm.tm_mday, m_iIndex );
                rename(m_szFileName, m_new_szFileName);

                m_sttFd = fopen( m_szFileName, "a" );
                if( m_sttFd == NULL )
                {
                    pthread_mutex_unlock(mutx_p);
                    return -1;
                }
                m_iIndex++;
                m_iLogSize = 0;
            }
        }   //if( m_iMaxLogSize > 0 )
    }   //end else


    //�α׾���
    va_list   ap;
    va_start (ap, fmt);
    vsnprintf(szBuf, LOG_MAX_SIZE-1, fmt, ap);
    va_end   (ap);

    // [11/05/19 09:35:40.566][CRI] �������� ���
    if( m_sttFd )
    {
        m_iLogSize += fprintf(m_sttFd, "[%2d/%02d/%02d %02d:%02d:%02d.%03d]%s%s\n"
            , sttTm.tm_year - 100, sttTm.tm_mon + 1, sttTm.tm_mday, sttTm.tm_hour, sttTm.tm_min
            , sttTm.tm_sec, (int)(sttTime.tv_usec/1000), szHeader, szBuf );

        if(wMode == 1)
            ConvDump( m_sttFd, pBuff, iBuffLen);
        fflush( m_sttFd );
    }

    pthread_mutex_unlock(mutx_p);
    return 0;
}

/*********************************************************************************************
* IN : void
* OUT : void
* Comment : ����� ����, �α����ϴݰ� �޸�����)
*********************************************************************************************/
void CServerLog::Destroy()
{
    if(m_sttFd != NULL)
        fclose(m_sttFd);

    if(m_pszDirName != NULL)
        delete []m_pszDirName;

    if(m_psLogFileName != NULL)
        delete []m_psLogFileName;

    if(szBuf != NULL)
        delete []szBuf;
}

void CServerLog::Close()
{
    if(m_sttFd)
    {
        fclose(m_sttFd);
        m_sttFd = NULL;
    }
}

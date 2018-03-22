//--------------------------------------------------------------------------------------------------------------
/** @file       ServerLog.cpp
    @date   2011/05/18
    @author 송봉수(sbs@gabia.com)
    @brief  서버 로그 기록 클래스.   \n
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
* IN : pszDirName: 로그파일저장 디렉토리 경로
* OUT : Success=0, Fail=-1
* Comment : 로그파일을 저장할 디렉토리 경로를 설정,
                  CreateDir에 디렉토리경로 전달
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

    //디렉토리 생성
    if(CDirectory::Create(m_pszDirName) != 0 ) return -1;
    return 0;
}


/*********************************************************************************************
* IN : psLogFileName: 로그파일저장 이름
* OUT : Success=0, Fail=-1
* Comment : 로그파일 이름설정
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
* IN : iLevel: 로그레벨
* OUT : int
* Comment : 로그레벨설정
*********************************************************************************************/
void CServerLog::SetLevel(int iLevel)
{
    m_iLevel = iLevel;
}


/*********************************************************************************************
* IN : iSize:최대 로그사이즈
* OUT : void
* Comment : 최대로그사이즈
*********************************************************************************************/
void CServerLog::SetMaxLogSize(int iSize)
{
    m_iMaxLogSize = iSize;
}


/*********************************************************************************************
* IN : void
* OUT : 로그인덱스번호(default 1)
* Comment : 인덱스번호리턴
*********************************************************************************************/
int CServerLog::GetLogIndex()
{
    return m_iIndex;
}


/*********************************************************************************************
* IN :  enum 로그레벨
* OUT : Print 허용 True/False
* Comment : 로그레벨을 확인하여 현재설정과 비교하여 출력허용여부 리턴
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
/** @brief 입력받은 내용을 16진수 헥사값으로 변경하여 기록
    @param pFp: LogFile FD
    @param pTranBuff: 변환할 문자열 주소
    @param iDispLen: 변환 문자열 버퍼 크기
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
* IN : 환경설정
* OUT : Success:0 Fail:-1
* Comment : 로그설정을 위한 환경설정 값 전달 (logFileDir, logFileName, LogLevel, maxLogFileSize)
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
* IN : iType: 로그종류, iLevle:로그레벨, wMode: 로그형식, 버퍼, 버퍼크기, fmd:가변인자
* OUT : Success=0, Fail=-1
* Comment : 로그레벨에 따라 로그출력, 날짜변경시 새로운파일생성
                  출력형식 : [날짜] [시간] [로그레벨][출력내용]
                  Printf(SYS, LOG_DEBUF, 1, buf, bufLen, "print %s", string)
*********************************************************************************************/
//int CServerLog::Print( EnumLogLevel iLevel, int wMode,  char *pBuff, int iBuffLen, const char *fmt, ...  )
int CServerLog::Print( int iLevel, int wMode,  char *pBuff, int iBuffLen, const char *fmt, ...  )
{
    struct tm           sttTm;
    struct timeval      sttTime;

   //로그레벨이 0이면 기록안함
    if(m_iLevel  == 0)
        return -1;

    pthread_mutex_lock(mutx_p);

    //로그레벨 비교하여 설정 레벨 이상만 출력
    if(IsPrintLogLevel(iLevel) == 0)
    {
        pthread_mutex_unlock(mutx_p);
        return -1;
    }

    // 로그레벨에 따라서 헤더설정 [DEBUG] [ERROR]
    SetszHeader(iLevel);

    gettimeofday(&sttTime, NULL);
    localtime_r( &sttTime.tv_sec, &sttTm);

    // 첫 로그작성
    if(m_sttFd == NULL)
    {
        // 시간을구하여 클래스에 저장
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
    else  //이어서 쓸때
    {
        snprintf(szDate, sizeof(szDate), "%04d%02d%02d", sttTm.tm_year + 1900, sttTm.tm_mon + 1, sttTm.tm_mday);
        //현재의 날짜와 클래스에 저장된 날자를 비교하여 로그파일을 새로 만듬
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

        //로그파일크기가 m_iMaxLogSize면 로그파일이름을 filename.m_iIndex 로바꿈
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


    //로그쓰기
    va_list   ap;
    va_start (ap, fmt);
    vsnprintf(szBuf, LOG_MAX_SIZE-1, fmt, ap);
    va_end   (ap);

    // [11/05/19 09:35:40.566][CRI] 형식으로 출력
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
* Comment : 종료시 실행, 로그파일닫고 메모리해지)
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

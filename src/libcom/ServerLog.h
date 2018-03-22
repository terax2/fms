//--------------------------------------------------------------------------------------------------------------
/** @file       ServerLog.h
    @date   2011/05/18
    @author 송봉수(sbs@gabia.com)
    @brief  서버 로그 기록 클래스.   \n
*///------------------------------------------------------------------------------------------------------------


/*  INCLUDE  */
#include <stdio.h>
#include <pthread.h>

/*  DEFINE  */
#define LOG_MAX_SIZE        4096
#define FILE_NAME_MAX_SIZE  1024
#define HEADER_SIZE         30
#define HEX_BUF_SIZE        128
#define TEMP_BUF_SIZE       16
#define ASC_BUF_SIZE        32
#define DATE_SIZE           9
#define CHG_CNT             4

#define LOG_DEBUG           1
#define LOG_INFO            2
#define LOG_WARNING         3
#define LOG_ERROR           4
#define LOG_CRITICAL        5

//enum EnumLogLevel
//{
//    LOG_DEBUG = 1,
//    LOG_INFO,
//    LOG_WARNING,
//    LOG_ERROR,
//    LOG_CRITICAL
//};

enum EnumLogType
{
    SYS = 1,
    CAM,
    USER
};


/*  CLASS DEFINE  */
class CServerLog
{
private:
    char  * m_pszDirName;                      // 로그 파일을 저장할 디렉토리 이름
    char  * m_psLogFileName;                   // 로그파일이름
    char    m_szDate[DATE_SIZE];                             // 현재 날자저장
    char    m_szFileName[FILE_NAME_MAX_SIZE];           // 현재 로그파일이름
    char    m_new_szFileName[FILE_NAME_MAX_SIZE];
    char    szDate[DATE_SIZE], szHeader[HEADER_SIZE];
    char  * szBuf;

    FILE  * m_sttFd;            // 로그 파일 Fd

    int     m_iLevel;               // 로그 파일에 저장할 레벨
    int     m_wMode;                // 로그기록모드 0=text, 1=hex
    int     m_iMaxLogSize;      // 하나의 로그 파일에 저장할 수 있는 최대 크기
    int     m_iLogSize;         // 현재까지 저장된 로그 크기
    int     m_iIndex;               // 로그 파일 인덱스

    pthread_mutex_t *mutx_p;

public:
    CServerLog();
    ~CServerLog();

    int     SetDirectory( const char *pszDirName );   //로그파일저장 디렉토리 설정
    int     SetLogFileName( const char *psLogFileName );   //로그파일이름
    void    SetLevel(int iLevel );                         //로그레벨 설정
    void    SetMaxLogSize(int iSize);                 //최대 로그파일크기 설정
    //void    SetszHeader(EnumLogLevel iLevel);
    void    SetszHeader(int iLevel);
    int     GetLogIndex();                                // 로그파일인덱스 리턴
    //bool    IsPrintLogLevel( EnumLogLevel iLevel );    // 입력한 로그레벨이 출력가능한지 확인
    bool    IsPrintLogLevel( int iLevel );    // 입력한 로그레벨이 출력가능한지 확인
    bool    ConvDump(FILE *pFp, char *pTranBuff, int iDispLen);    // 헥사형식출력

    int     SetupLog(const char *pszDirName, const char *psLogFileName, int iLevel, int iSize);   //로그환경설정
    //int     Print( EnumLogLevel iLevel, int wMode,  char *pBuff, int iBuffLen, const char *fmt, ... );    // 로그쓰기
    int     Print( int iLevel, int wMode,  char *pBuff, int iBuffLen, const char *fmt, ... );    // 로그쓰기
    void    Destroy();     // 파일닫기, 메모리삭제
    void    Close();       // 파일닫기
};



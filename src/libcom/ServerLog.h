//--------------------------------------------------------------------------------------------------------------
/** @file       ServerLog.h
    @date   2011/05/18
    @author �ۺ���(sbs@gabia.com)
    @brief  ���� �α� ��� Ŭ����.   \n
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
    char  * m_pszDirName;                      // �α� ������ ������ ���丮 �̸�
    char  * m_psLogFileName;                   // �α������̸�
    char    m_szDate[DATE_SIZE];                             // ���� ��������
    char    m_szFileName[FILE_NAME_MAX_SIZE];           // ���� �α������̸�
    char    m_new_szFileName[FILE_NAME_MAX_SIZE];
    char    szDate[DATE_SIZE], szHeader[HEADER_SIZE];
    char  * szBuf;

    FILE  * m_sttFd;            // �α� ���� Fd

    int     m_iLevel;               // �α� ���Ͽ� ������ ����
    int     m_wMode;                // �αױ�ϸ�� 0=text, 1=hex
    int     m_iMaxLogSize;      // �ϳ��� �α� ���Ͽ� ������ �� �ִ� �ִ� ũ��
    int     m_iLogSize;         // ������� ����� �α� ũ��
    int     m_iIndex;               // �α� ���� �ε���

    pthread_mutex_t *mutx_p;

public:
    CServerLog();
    ~CServerLog();

    int     SetDirectory( const char *pszDirName );   //�α��������� ���丮 ����
    int     SetLogFileName( const char *psLogFileName );   //�α������̸�
    void    SetLevel(int iLevel );                         //�α׷��� ����
    void    SetMaxLogSize(int iSize);                 //�ִ� �α�����ũ�� ����
    //void    SetszHeader(EnumLogLevel iLevel);
    void    SetszHeader(int iLevel);
    int     GetLogIndex();                                // �α������ε��� ����
    //bool    IsPrintLogLevel( EnumLogLevel iLevel );    // �Է��� �α׷����� ��°������� Ȯ��
    bool    IsPrintLogLevel( int iLevel );    // �Է��� �α׷����� ��°������� Ȯ��
    bool    ConvDump(FILE *pFp, char *pTranBuff, int iDispLen);    // ����������

    int     SetupLog(const char *pszDirName, const char *psLogFileName, int iLevel, int iSize);   //�α�ȯ�漳��
    //int     Print( EnumLogLevel iLevel, int wMode,  char *pBuff, int iBuffLen, const char *fmt, ... );    // �α׾���
    int     Print( int iLevel, int wMode,  char *pBuff, int iBuffLen, const char *fmt, ... );    // �α׾���
    void    Destroy();     // ���ϴݱ�, �޸𸮻���
    void    Close();       // ���ϴݱ�
};



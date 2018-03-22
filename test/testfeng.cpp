
#include "shlib.h"
#include "our_md5.h"
#include "Base64.h"
#include "datamap.h"
#include "mutex.h"

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <math.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

#include <iconv.h>
#include <pcrecpp.h>
//#include <boost/regex.hpp>
//#include <boost/lambda/lambda.hpp>
//#include <boost/signals2.hpp>
#include <iterator>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

extern "C" {
    #include<curl/curl.h>
}

//#define  MAX_THREADS 5800
#define  MAX_THREADS 1

using namespace std;

unsigned char fRecvBuffer[128];
unsigned char fSendBuffer[128];
int           m_rawfp = -1;
char          m_Tmp[128];

Mutex         gmutex    ;
uint64_t      logCount = 0;
uint64_t      oldCount = 0;

#define randomize()     srand((unsigned)time(NULL)+(unsigned)getpid())
#define random(num)     (rand()%(num))

#pragma pack(push, 1)

typedef struct  _pcreParse_
{
    ///////////////////////////////
    uint8_t     ExprNo  ;
    int8_t      facility;
    int8_t      severity;
    char        curttime [TMP_BUF];
    ///////////////////////////////
    int16_t     priority ;
    string      timestamp;
    string      hostname ;
    string      program  ;
    string      messages ;
}   PCREDiv;
#define SZ_PDIV sizeof(PCREDiv)


typedef struct
{
        char            ServiceType[2];
        uint8_t         ServiceState;
        uint8_t         CameraType;
        uint8_t         CameraAudio;
        char            CamName[90];
        char            SerialNumber[16];
        uint8_t         CamSetupState;
        char            SetupIP[15];
        uint16_t        SetupPort;
        uint8_t         CamLiveCastState;
        char            StreamerIP[15];
        uint16_t        StreamerPort;
        char            StreamThumIP[15];
        uint16_t        StreamThumPort;
        uint8_t         CamAlarmState;
        char            AlarmIP[15];
        uint16_t        AlarmPort;
        char            AlarmThumIP[15];
        uint16_t        AlarmThumPort;
        uint8_t         CamRendezvousState;
        char            RendezvousIP[15];
        uint16_t        RendezvousPort;
        uint8_t         CamMuiltViewState;
        char            MuiltViewIP[15];
        uint16_t        MuiltViewPort;
        uint8_t         PlayPerHour;
        uint16_t        PlaySeconds;
}   StreamConnectionInfo;

//프로토콜 공통 헤더
typedef struct
{
    char             magic[2];          // fix "GM" (define)
    uint16_t         ver;               // fix 2 (define)
    uint16_t         type;              //enum MessageType
    uint32_t         bodylen;                 // payload Len
    uint8_t          encrypt;                // 0=non-encrypt, 1=des ecnrypt
    uint8_t          devicetype;            // 0=N/A, 10=ActiveX, 20=Android, 30=Iphone
    char             reserved[4];          // not usable
}   Hvaas_Header;

#define SZ_HDR  sizeof(Hvaas_Header)

#pragma pack(pop)



//////////////// Notify Message Unicast Function //////////////////////////////
int connectWithTimeout(int fd,struct sockaddr *remote, int len, int secs, int *err)
{
    int saveflags,ret,back_err;
    fd_set fd_w;
    struct timeval timeout = {secs, 0};

    saveflags=fcntl(fd,F_GETFL,0);
    if(saveflags<0)
    {
        perror("\"fcntl1\"");
        *err=errno;
        return -1;
    }

    /* Set non blocking */
    if(fcntl(fd,F_SETFL,saveflags|O_NONBLOCK)<0)
    {
        perror("\"fcntl2\"");
        *err=errno;
        return -1;
    }

    /* This will return immediately */
    *err=connect(fd,remote,len);
    back_err=errno;

    /* restore flags */
    if(fcntl(fd,F_SETFL,saveflags)<0)
    {
        perror("\"fcntl3\"");
        *err=errno;
        return -1;
    }

    /* return unless the connection was successful or the connect is
    still in progress. */
    if(*err<0 && back_err!=EINPROGRESS)
    {
        perror("\"connect\"");
        *err=errno;
        return -1;
    }

//    timeout.tv_sec  = (long)secs;
//    timeout.tv_usec = 0L;

    FD_ZERO(&fd_w);
    FD_SET(fd,&fd_w);

    *err=select(FD_SETSIZE,NULL,&fd_w,NULL,&timeout);
    if(*err<0)
    {
        perror("\"select\"");
        *err=errno;
        return -1;
    }

    /* 0 means it timeout out & no fds changed */
    if(*err==0)
    {
        perror("\"timeout...\"");
        *err=ETIMEDOUT;
        return -1;
    }

    /* Get the return code from the connect */
    socklen_t addrsize = sizeof remote;
    *err = getsockopt(fd,SOL_SOCKET,SO_ERROR,&ret,&addrsize);
    if(*err<0)
    {
        perror("\"getsockopt\"");
        *err=errno;
        return -1;
    }

    /* ret=0 means success, otherwise it contains the errno */
    if(ret)
    {
        *err=ret;
        return -1;
    }
    *err=0;
    return 0;
}


static int setupSocket( int port, int makeNonBlocking)
{
  int newSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (newSocket < 0)
  {
    return newSocket;
  }

  const int reuseFlag = 1;
  if (setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR,
         (const char*)&reuseFlag, sizeof reuseFlag) < 0)
  {
    close(newSocket);
    return -1;
  }

  // SO_REUSEPORT doesn't really make sense for TCP sockets, so we
  // normally don't set them.  However, if you really want to do this
  // #define REUSE_FOR_TCP
#ifdef REUSE_FOR_TCP
#if defined(__WIN32__) || defined(_WIN32)
    // Windoze doesn't handle SO_REUSEPORT
#else
#ifdef SO_REUSEPORT
  if (setsockopt(newSocket, SOL_SOCKET, SO_REUSEPORT,
         (const char*)&reuseFlag, sizeof reuseFlag) < 0)
  {
    close(newSocket);
    return -1;
  }
#endif
#endif
#endif

  // Note: Windoze requires binding, even if the port number is 0
#if defined(__WIN32__) || defined(_WIN32)
#else
  if (port != 0) // || ReceivingInterfaceAddr != INADDR_ANY)
  {
#endif
    struct sockaddr_in name;
    name.sin_family = AF_INET;
    name.sin_port = port;
    name.sin_addr.s_addr = INADDR_ANY;
    if (bind(newSocket, (struct sockaddr*)&name, sizeof name) != 0)
    {
      char tmpBuffer[100];
      sprintf(tmpBuffer, "bind() error (port number: %d): ",
          ntohs(port));
      close(newSocket);
      return -1;
    }
#if defined(__WIN32__) || defined(_WIN32)
#else
  }
#endif

  if (makeNonBlocking)
  {
    // Make the socket non-blocking:
#if defined(__WIN32__) || defined(_WIN32) || defined(IMN_PIM)
    unsigned long arg = 1;
    if (ioctlsocket(newSocket, FIONBIO, &arg) != 0)
    {

#elif defined(VXWORKS)
    int arg = 1;
    if (ioctl(newSocket, FIONBIO, (int)&arg) != 0)
    {

#else
    int curFlags = fcntl(newSocket, F_GETFL, 0);
    if (fcntl(newSocket, F_SETFL, curFlags|O_NONBLOCK) < 0)
    {
#endif
      close(newSocket);
      return -1;
    }
  }

  return newSocket;
}

static int dataSocket( int port, int makeNonBlocking)
{
  int newSocket = socket(AF_INET, SOCK_DGRAM, 0);
  if (newSocket < 0)
  {
    return newSocket;
  }

  const int reuseFlag = 1;
  if (setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR,
         (const char*)&reuseFlag, sizeof reuseFlag) < 0)
  {
    close(newSocket);
    return -1;
  }

  // SO_REUSEPORT doesn't really make sense for TCP sockets, so we
  // normally don't set them.  However, if you really want to do this
  // #define REUSE_FOR_TCP
#ifdef REUSE_FOR_TCP
#if defined(__WIN32__) || defined(_WIN32)
    // Windoze doesn't handle SO_REUSEPORT
#else
#ifdef SO_REUSEPORT
  if (setsockopt(newSocket, SOL_SOCKET, SO_REUSEPORT,
         (const char*)&reuseFlag, sizeof reuseFlag) < 0)
  {
    close(newSocket);
    return -1;
  }
#endif
#endif
#endif

  // Note: Windoze requires binding, even if the port number is 0
#if defined(__WIN32__) || defined(_WIN32)
#else
  if (port != 0) // || ReceivingInterfaceAddr != INADDR_ANY)
  {
#endif
    struct sockaddr_in name;
    name.sin_family = AF_INET;
    name.sin_port = port;
    name.sin_addr.s_addr = INADDR_ANY;
    if (bind(newSocket, (struct sockaddr*)&name, sizeof name) != 0)
    {
      char tmpBuffer[100];
      sprintf(tmpBuffer, "bind() error (port number: %d): ",
          ntohs(port));
      close(newSocket);
      return -1;
    }
#if defined(__WIN32__) || defined(_WIN32)
#else
  }
#endif

  if (makeNonBlocking)
  {
    // Make the socket non-blocking:
#if defined(__WIN32__) || defined(_WIN32) || defined(IMN_PIM)
    unsigned long arg = 1;
    if (ioctlsocket(newSocket, FIONBIO, &arg) != 0)
    {

#elif defined(VXWORKS)
    int arg = 1;
    if (ioctl(newSocket, FIONBIO, (int)&arg) != 0)
    {

#else
    int curFlags = fcntl(newSocket, F_GETFL, 0);
    if (fcntl(newSocket, F_SETFL, curFlags|O_NONBLOCK) < 0)
    {
#endif
      close(newSocket);
      return -1;
    }
  }

  return newSocket;
}

int setupUDPSocket()
{
    int newSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (newSocket < 0) {
        return newSocket;
    }

    const int reuseFlag = 1;
    if (setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR,
         (const char*)&reuseFlag, sizeof reuseFlag) < 0) {
        close(newSocket);
        return -1;
    }

    return newSocket;
}

static int UntilReadable( int socket, struct timeval* timeout)
{
  int result = -1;
  do {
    fd_set rd_set;
    FD_ZERO(&rd_set);
    if (socket < 0) break;
    FD_SET((unsigned) socket, &rd_set);
    const unsigned numFds = socket+1;

    result = select(numFds, &rd_set, NULL, NULL, timeout);
    if (timeout != NULL && result == 0)
    {
      break; // this is OK - timeout occurred
    } else if (result <= 0) {
      break;
    }

    if (!FD_ISSET(socket, &rd_set))
    {
      break;
    }
  } while (0);

  return result;
}

static int ReadableSocket(int socket, unsigned char* buffer, unsigned bufferSize,
           struct sockaddr_in& fromAddress, struct timeval* timeout)
{
  int bytesRead = -1;
  do {
    int result = UntilReadable(socket, timeout);
    if (timeout != NULL && result == 0)
    {
      bytesRead = 0;
      break;
    } else if (result <= 0)
    {
      break;
    }

    socklen_t addressSize = sizeof fromAddress;
    bytesRead = recvfrom(socket, (char*)buffer, bufferSize, 0,
             (struct sockaddr*)&fromAddress,
             &addressSize);
    if (bytesRead < 0)
    {
      break;
    }
  } while (0);

  return bytesRead;
}


int RecvSocket(int socket, char * buffer, unsigned bufferSize, struct timeval* timeout)
{
    int bytesRead = -1;
    do  {
        int result = UntilReadable(socket, timeout);
        if (timeout != NULL && result == 0)
        {
            bytesRead = 0;
            break;
        }   else if (result <= 0) {
            break;
        }

        bytesRead = recv(socket, buffer, bufferSize, 0);
        if (bytesRead < 0) {
            break;
        }
    }   while (0);

    return bytesRead;
}

int Options( int fSocket )
{
    memset(fSendBuffer, 0, sizeof(fSendBuffer));
    snprintf((char*)fSendBuffer, sizeof(fSendBuffer),"TEST00123456%08d", fSocket );

    fSendBuffer[0] = 0x24;
    fSendBuffer[1] = 0x00;
    fSendBuffer[2] = 0x00;
    fSendBuffer[3] = 0x10;

    int iLen = 20; //strlen((char*)fSendBuffer);
    return (send(fSocket, (char*)fSendBuffer, iLen, 0) > 0);
}

int sendrtp ( int fSocket )
{
    memset( fSendBuffer, 0, sizeof(fSendBuffer));
    fSendBuffer[0] = 0x24;
    fSendBuffer[1] = 0x00;
    fSendBuffer[2] = 0x05;
    fSendBuffer[3] = 0x78;

    int iLen = 1388 + 12 + 4; //strlen((char*)fSendBuffer);
    memset( &fSendBuffer[4], 0x65, iLen-4);

    return (send(fSocket, (char*)fSendBuffer, iLen, 0) > 0);
}

int TearDown( int fSocket )
{
    snprintf((char*)fSendBuffer, sizeof(fSendBuffer),
            "TEARDOWN rtsp://127.0.0.1/test.mov/ RTSP/1.0\r\n"
            "CSeq: 2\r\n"
            "Session: 0\r\n"
            "User-Agent: Test Feng \r\n\r\n");
    int iLen = strlen((char*)fSendBuffer);
    return (send(fSocket, (char*)fSendBuffer, iLen, 0) > 0);
}

int SetLimit(const unsigned nmax)
{
    // set rlimit
    struct rlimit rlim;
    rlim.rlim_cur = rlim.rlim_max = nmax;
    int error = setrlimit(RLIMIT_NOFILE,&rlim);
    if( error )
    {
        printf("Error !!! RLIMIT_NOFILE Setting Fail [%d] \n\n",errno);
        return -1;
    }
    error = getrlimit(RLIMIT_NOFILE,&rlim);
    if( error )
    {
        printf("Error !!! RLIMIT_NOFILE Getting Fail [%d] \n\n",errno);
        return -1;
    }
    else
    {
        printf("Test Setlimit = cur[%d] max[%d]\n",rlim.rlim_cur, rlim.rlim_max);
    }
    return 0;
}

int setrLimit(int resource, const unsigned nmax)
{
    // set rlimit
    struct rlimit rlim;
    rlim.rlim_cur = rlim.rlim_max = nmax;
    int error = setrlimit(resource,&rlim);
    if( error )
    {
        printf("Error !!! RLIMIT_NOFILE Setting Fail [%d] \n\n",errno);
        return -1;
    }
    error = getrlimit(resource,&rlim);
    if( error )
    {
        printf("Error !!! RLIMIT_NOFILE Getting Fail [%d] \n\n",errno);
        return -1;
    }
    else
    {
        printf("Test Setlimit = cur[%d] max[%d]\n",rlim.rlim_cur, rlim.rlim_max);
    }
    return 0;
}

#define _SLEEP_TIME_    5
#define _TIME2SLEEP_    775
#define _RECV_TIME_     (uint64_t)(_TIME2SLEEP_ * 5.0)       //    5 seconds


int SendSocket(int socket, char * buffer, int bufferSize)
{
    int bytesSend    = 0;
    int totBytesSend = 0;

    fprintf(stderr, "Request  send (%d) bytes \n", bufferSize);
    do  {
        bytesSend = send (socket, buffer + totBytesSend, bufferSize-totBytesSend, 0);
        if (bytesSend < 0)
        {
            if(errno != EAGAIN)
            {
                return -1   ;
            }
            SDL_Delay(_SLEEP_TIME_);
        }   else
        {
            totBytesSend += bytesSend;
        }
    }   while (bufferSize != totBytesSend);
    return totBytesSend;
}


int getData(int socket, char * buffer, int bufferSize, uint64_t seconds)
{
    if (bufferSize <= 0) return -4;
    uint64_t TickCount = 0;

    while(true)
    {
        int readBytes = recv(socket, buffer, bufferSize, 0);

        // 읽을것이 없거나 에러
        if (readBytes < 0)
        {
            // read 했으나 읽을것이 없음 if(errno == EWOULDBLOCK) or 약 'seconds'초가 지났거나..
            if (EAGAIN == errno)
            {
                SDL_Delay(_SLEEP_TIME_)  ;
                TickCount += _SLEEP_TIME_;
                if (TickCount > seconds)
                {
                    return -3   ;
                }
            }   else
            {
                return -2   ;
            }
        }   else
        {
            //클라이언트 연결종료
            if( readBytes == 0)
            {
                return -1;
            }
            return readBytes;
        }
    }
}

int getDataExact( int socket, char * buffer, int bufferSize, uint64_t seconds)
{
    if (bufferSize < 0) return -4;
    int bsize        = bufferSize;
    int bytesRead    = 0;
    int totBytesRead = 0;
    while (bsize != 0)  {
        bytesRead = getData(socket, buffer + totBytesRead, bsize, seconds);
        if (bytesRead < 0) return bytesRead;
        totBytesRead += bytesRead   ;
        bsize -= bytesRead  ;
    }

    return totBytesRead;
}


int Receiver( int socket )
{
    int MsgLen = 0;
    while (true)
    {
        memset(fRecvBuffer, 0, sizeof(fRecvBuffer));
        Hvaas_Header * hdr = (Hvaas_Header*)fRecvBuffer ;
        int readBytes = getDataExact(socket, (char*)fRecvBuffer, SZ_HDR, _RECV_TIME_);
        if (readBytes != SZ_HDR)
        {
            fprintf(stderr, "%s()[%d] (%d)\n", __FUNCTION__, __LINE__, readBytes);
            return -1  ;
        }
        MsgLen = readBytes;
        int bodySize = htonl(hdr->bodylen);

        readBytes = getDataExact(socket, (char*)&fRecvBuffer[SZ_HDR], bodySize, _RECV_TIME_);
        if (readBytes != bodySize)
        {
            fprintf(stderr, "%s()[%d] (%d)\n", __FUNCTION__, __LINE__, readBytes);
            return -1  ;
        }
        MsgLen += readBytes   ;
        fprintf(stderr, "%s()[%d] received message (%d)\n", __FUNCTION__, __LINE__, MsgLen);
        return 0;
    }

    // An error occurred:
    return 0 ;
}

#pragma pack(push, 1)

#define _SMP_PKT_SOP_       0x02
#define _SMP_PKT_EOP_       0x03

typedef struct  _hmsg_
{
    uint8_t     sop     ;
    uint8_t     devid   ;
    uint16_t    length  ;           //  Functon Code(1) + Payload(N) = 1 + N 사용, BigEndian
    uint8_t     fcode   ;           //  Fail 일 경우 : Function Code + 0x80 더해서 응답.
}   HMSG;
#define SZ_HMSG sizeof(HMSG)

typedef struct _did_
{
    uint32_t    serial  ;
}   DID;
#define SZ_DID sizeof(DID)

typedef struct  _tmsg_
{
    uint8_t     eop     ;
}   TMSG;
#define SZ_TMSG sizeof(TMSG)

typedef struct  _umsg_
{
    //ClntInfo    sock    ;
    char        msg     [MID_BUF];
}   UMSG;
#define SZ_UMSG sizeof(UMSG)

#pragma pack(pop)

static const inline uint16_t MakePacket(UMSG * pMsg, uint8_t fcode, void * pData = NULL, uint8_t pLen = 0)
{
    memset(pMsg->msg, 0, sizeof(pMsg->msg));
    HMSG  * head = (HMSG*)pMsg->msg ;
    char  * body = (char*)&pMsg->msg[SZ_HMSG];

    head->sop    = _SMP_PKT_SOP_;
    head->devid  = 0;
    head->length = htons(pLen+1);
    head->fcode  = fcode;

    if (pData) {
        memcpy(body, pData, pLen);
    }
    TMSG * tail  = (TMSG*)&pMsg->msg[SZ_HMSG+pLen];
    tail->eop    = _SMP_PKT_EOP_;

    return (uint16_t)(SZ_HMSG + pLen + SZ_TMSG);
}

//static int EpollTest( void * data )
//{
//    UMSG uMsg;
//    DID  uSno;
//    while ( 1 )
//    {
//        int socket = -1;
//        while( socket < 0 ) socket = setupSocket(0, 0);
//
//        do  {
//            if ( socket < 0 ) break;
//
//            struct sockaddr_in remoteName                       ;
//            socklen_t addrlen = sizeof remoteName               ;
//            int err                                             ;
//            remoteName.sin_family       = AF_INET               ;
//            remoteName.sin_port         = htons(1882)           ;
//            remoteName.sin_addr.s_addr  = inet_addr("127.0.0.1");
//
//            if (connect( socket, (struct sockaddr*)&remoteName, sizeof remoteName) != 0 )
//            {
//                fprintf(stderr, "[%d] Connection Failed 'Server' (%s) !!\n", __LINE__, strerror(errno));
//                break;
//            }
//
//            int curFlags = fcntl(socket, F_GETFL, 0);
//            if (fcntl(socket, F_SETFL, curFlags|O_NONBLOCK) < 0)
//            {
//                fprintf(stderr, "[%d] Connection Failed 'Server' (%s) !!\n", __LINE__, strerror(errno));
//                break;
//            }
//
//            uSno.serial = htonl(1234);
//            uint16_t tlen = MakePacket(&uMsg, 0x10, &uSno, SZ_DID);
//
//
//            SendSocket (socket, uMsg.msg, tlen); //sendLen);
//            //sleep(1);
//            //Receiver   (socket);
//            break ;
//        }   while ( 0 ) ;
//
//        close(socket)   ;
//        return 0;
//        //SDL_Delay(8000) ;
//    }
//}


static int EpollTest( void * data )
{
    UMSG uMsg;
    DID  uSno;
    while ( 1 )
    {
        int socket = -1;
        while( socket < 0 ) socket = setupSocket(0, 0);

        if ( socket < 0 ) break;

        struct sockaddr_in remoteName                       ;
        socklen_t addrlen = sizeof remoteName               ;
        int err                                             ;
        remoteName.sin_family       = AF_INET               ;
        remoteName.sin_port         = htons(8083)           ;
        remoteName.sin_addr.s_addr  = inet_addr("127.0.0.1");

        if (connect( socket, (struct sockaddr*)&remoteName, sizeof remoteName) != 0 )
        {
            fprintf(stderr, "[%d] Connection Failed 'Server' (%s) !!\n", __LINE__, strerror(errno));
            break;
        }

        int curFlags = fcntl(socket, F_GETFL, 0);
        if (fcntl(socket, F_SETFL, curFlags|O_NONBLOCK) < 0)
        {
            fprintf(stderr, "[%d] Connection Failed 'Server' (%s) !!\n", __LINE__, strerror(errno));
            break;
        }


        do  {

            snprintf(uMsg.msg, sizeof(uMsg.msg), "hello");
            SendSocket (socket, uMsg.msg, 5);

            Receiver (socket);
            //sleep(1);
            //uint16_t tlen = MakePacket(&uMsg, 0x10);
            //SendSocket (socket, uMsg.msg, tlen); //sendLen);
            
            break ;
        }   while ( 0 ) ;

        close(socket)   ;
        return 0;
        //SDL_Delay(8000) ;
    }
}


static inline const void getlogCount()
{
    gmutex.lock();
    logCount ++ ;
    gmutex.unlock();
}

static inline const void getCount()
{
    uint64_t CPS  = 0;
    uint64_t old  = 0;
    uint64_t curt = 0;

    old  =  oldCount   ;
    curt =  logCount   ;
    CPS  = (curt - old);

    fprintf(stderr, "send success Count (%llu) (total = %llu) !!!\n", CPS, curt);
    oldCount = curt;
}

static int UdpTest( void * data )
{
    int * No = (int*)data ;
    int   No1 = *No ;
    int   Count = 0;

    while ( 1 )
    {
        int socket = -1;
        while( socket < 0 ) socket = setupUDPSocket();

        if ( socket < 0 ) break;
        struct sockaddr_in remoteName                       ;
        char   buf [1500] = {0,}                            ;
        socklen_t addrlen = sizeof remoteName               ;

        remoteName.sin_family       = AF_INET               ;
        remoteName.sin_port         = htons(5160)           ;
        remoteName.sin_addr.s_addr  = inet_addr("127.0.0.1");
        //remoteName.sin_addr.s_addr  = inet_addr("110.45.183.101");

        //snprintf(buf, sizeof(buf), "<80>Aug  6 10:43:47 dev test: test message (%04d) load test message test message test message test message test message", No1);
        //snprintf(buf, sizeof(buf), "<3>May 21 12:02:08 hb02 kernel: \"echo 0 > /proc/sys/kernel/hung_task_timeout_secs\" disables this message.");
        snprintf(buf, sizeof(buf), "<3>Jun 1 12:20:08 hb02 kernel: 'echo' \"0 > /proc/sys/kernel/test_timeout_secs\" disables this message.\n");
        //snprintf(buf, sizeof(buf), "{\"message\":\"<3>May 21 12:02:08 hb02 kernel: 'echo' \"0 > /proc/sys/kernel/hung_task_timeout_secs\" disables this message.\"}\n");
        Count ++ ;
        if (sendto (socket, buf, strlen(buf), 0, (struct sockaddr*)&remoteName, addrlen) > 0)
        {
            fprintf(stderr, "send msg (%d)(%s)\n", strlen(buf), buf);
            getlogCount();
        }   else
        {
            fprintf(stderr, "thread no (%d) send failed  +++++++++++++++++++++++\n", No1);
        }

        close(socket) ;
        break;
        //SDL_Delay(10 + random(200));
    }
    return 0;
}

static int CheckTimer( void * data )
{
    while ( 1 )
    {
        SDL_Delay(998);
        getCount();
    }
}

void testDiscovery()
{
    vector<CThread *> ThreadList;
    ThreadList.clear();

    randomize();
    //CThread * pTimer = new CThread (CheckTimer, NULL);
    //while(1)
    //{
        //uint32_t lDelay = (int)(8000.0/MAX_THREADS);
        for ( int i = 0; i < MAX_THREADS; i++ )
        {
            //CThread * pThread = new CThread (UdpTest, &i);
            CThread * pThread = new CThread (EpollTest, &i);
            ThreadList.push_back(pThread);
            //SDL_Delay(lDelay);
        }

        for ( int i = 0; i < ThreadList.size(); i++ )
        {
            delete (CThread*)ThreadList[i] ;
            ThreadList[i] = NULL ;
        }
        //SDL_Delay(8000);
        ThreadList.clear();
    //    break;
    //}
    return ;

}

// Generate a "Date:" header for use in a RTSP response:
static char const* dateHeader() {
    static char buf[200];
    time_t tt = time(NULL);
    strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%S%Z", localtime(&tt)); //localtime
    printf("buf = [%s]\n", buf);

    //strftime(buf, sizeof buf, "%H%M%S", localtime(&tt));
    //printf("buf = [%s]\n", buf);

//    strftime(buf, sizeof buf, "Date: %a, %b %d %Y %H:%M:%S GMT\r\n", gmtime(&tt));
//    printf("buf = [%s]\n", buf);
    return buf;
}

int parseTitleValue (char const* buf, int bufsize, char * Val, const char sep )
{
    int     iFound  = 0 ;
    int     iLen    = 0 ;
    int     iCount  = 0 ;

    while ( iCount < bufsize )
    {
        if (&buf[iCount] == '\0') return 0;
        if ( buf[iCount] == '/') iFound++;
        iCount ++;
        if ( iFound >= 3 ) break;
    }

    while ( iCount < bufsize )
    {
        if ((buf[iCount] == sep) || (buf[iCount] == '\r' && buf[iCount+1] == '\n')) break;
        Val[iLen] = buf[iCount];
        iCount ++;
        iLen   ++;
    }
    return iLen ;
}

int parseCommaValue (char const* buf, int bufsize, char const * field, char * Val, const char sep )
{
    int     fldsize = 0 ;
    int     iLen    = 0 ;
    int     iCount  = 0 ;

    fldsize = strlen(field) ;
    while ( iCount < bufsize )
    {
        if (&buf[iCount] == '\0') return 0;
        if (!memcmp(&buf[iCount], field, fldsize))
            break;
        iCount ++;
    }
    if (iCount == bufsize) return 0 ;

    iCount += fldsize;
    while (buf[iCount] == ' ') ++iCount;
    while ( iCount < bufsize )
    {
        if ((buf[iCount] == sep) || (buf[iCount] == '\r' && buf[iCount+1] == '\n')) break;
        Val[iLen] = buf[iCount];
        iCount ++;
        iLen   ++;
    }
    return iLen ;
}

char * getDataQuota(const char * item, int & Len, char sep )
{
    char m_Src[300];
    snprintf(m_Src, sizeof m_Src, "DESCRIBE rtsp://10.20.23.159:554/1234567890ABCDEF RTSP/1.0\r\n"
                                  "CSeq: 4\r\n"
                                  "Authorization: Digest username=\"root\",realm=\"GABIA_0036C9C316\",nonce=\"00134636c9c31a0004a88f36c9c3190004a88f36c9c319\",uri=\"rtsp://10.20.23.159:554/1234567890ABCDEF\",response=\"c908b1398f9b3d72fb61f9a0dfb89d9d\"\r\n\r\n");
    int m_Len = strlen(m_Src);

    if (m_Len <= 0) return NULL;

    memset(m_Tmp, 0, 128);
    Len = parseCommaValue (m_Src, m_Len, item, m_Tmp, sep);
    if (Len > 0 && (m_Tmp[0] == '"' && m_Tmp[Len-1] == '"'))
    {
        memmove( m_Tmp, &m_Tmp[1], Len - 2);
        Len -= 2         ;
        m_Tmp[Len] = '\0';

        return (Len > 0 ? m_Tmp : NULL);
    }   else
        return NULL ;
}

int parseSDPValue (char const* buf, int bufsize, char * Val)
{
    int     iLen    = 0 ;
    int     iCount  = 0 ;

    while ( iCount < bufsize )
    {
        if (&buf[iCount] == '\0') return 0;
        if (buf[iCount  ] == '\r' && buf[iCount+1] == '\n' &&
            buf[iCount+2] == '\r' && buf[iCount+3] == '\n'  )
            break;
        iCount ++;
    }
    if (iCount == bufsize) return 0 ;

    iCount += 4;
    while (buf[iCount] == ' ') ++iCount;
    while ( iCount < bufsize )
    {
        if (&buf[iCount] == '\0') return 0;
        Val[iLen] = buf[iCount];
        iCount ++;
        iLen   ++;
    }
    return iLen ;
}

int ControlSwappingUri (const char * field, char * Target, char * buf, int bufsize)
{
    char *  Tmp = new char[bufsize];
    int     fldsize = strlen(field);
    int     sCount  = 0 ;
    int     tCount  = 0 ;
    int     fndCnt  = 0 ;
    bool    foundIt     ;
    memcpy (Tmp, buf, bufsize);
    memset (buf,   0, bufsize);

    while ( sCount < bufsize )
    {
        foundIt = false;
        while ( sCount < bufsize )
        {
            if (&Tmp[sCount] == '\0') return 0;
            if (!memcmp(&Tmp[sCount], field, fldsize))  {
                fndCnt ++       ;
                foundIt = true  ;
                break;
            }   else    {
                buf[tCount] = Tmp[sCount];
            }
            sCount ++;
            tCount ++;
        }

        if (foundIt) {
            while ( sCount < bufsize ) {
                if (Tmp[sCount] == '\r' && Tmp[sCount+1] == '\n')   {
                    sCount ++   ;
                    break;
                }
                sCount ++ ;
            }

            switch (fndCnt) {
            case    1: snprintf(&buf[tCount], bufsize-tCount, "%s%s", field, Target); break;
            case    2: snprintf(&buf[tCount], bufsize-tCount, "%s%s/trackID=1", field, Target); break;
            default  : snprintf(&buf[tCount], bufsize-tCount, "%s%s", field, Target); break;
            }
            tCount += strlen(&buf[tCount]);
        }
    }
    delete[] Tmp;
    return 0 ;
}

typedef struct _TEST_
{
    char        tmp[14] ;
    uint16_t    pos     ;
    uint16_t    size    ;
}   TEST    ;

bool openWriteData(const char * fName)
{
    if (m_rawfp >= 0) close(m_rawfp);
    fprintf(stderr, "%s() filename = [%s]\n", __FUNCTION__, fName);
    m_rawfp = open(fName, O_WRONLY|O_CREAT, S_IREAD | S_IWRITE);
    if (m_rawfp <  0) {
        fprintf(stderr, "%s() open file error [%d][%s]\n", __FUNCTION__, m_rawfp, strerror(errno));
        return false;
    }   else
    {
        fprintf(stderr, "%s() m_rawfp = [%d]\n", __FUNCTION__, m_rawfp);
        return true ;
    }
}

bool writeData(int ffp, const char * buf, int len)
{
    if (m_rawfp < 0) return false;
    int ret = write (m_rawfp, buf, len);
    if (ret != len) {
        fprintf(stderr, "%s() write file error [%d][%s]", __FUNCTION__, ffp, strerror(errno));
        return false;
    }   else
        return true ;
}

bool openReadData(const char * fName)
{
    if (m_rawfp >= 0) close(m_rawfp);
    fprintf(stderr, "%s() filename = [%s]\n", __FUNCTION__, fName);
    m_rawfp = open(fName, O_RDONLY, S_IREAD | S_IWRITE);
    if (m_rawfp <  0) {
        fprintf(stderr, "%s() open file error [%d][%s]\n", __FUNCTION__, m_rawfp, strerror(errno));
        return false;
    }   else
    {
        fprintf(stderr, "%s() m_rawfp = [%d]\n", __FUNCTION__, m_rawfp);
        return true ;
    }
}

bool readData(int ffp, char * buf, int len)
{
    if (m_rawfp < 0) return false;
    int ret = read (m_rawfp, buf, len);
    if (ret != len) {
        fprintf(stderr, "%s() write file error [%d][%s]", __FUNCTION__, ffp, strerror(errno));
        return false;
    }   else
        return true ;
}

static uint32_t hashIndexFromKey(char const* key)
{
    uint32_t result = 0;

    while (1) {
        char c = *key++;
        if (c == 0) break;
        result += (result<<0x1) + (uint8_t)c;
    }
    result &= 0xFFFFFFFF;

    return result;
}

static inline const int IsDomainName ( char * HostName, int len )
{
    int i = 0;
    for (i = 0; i < len; i++ )
    {
        if (HostName[i] != 0x2E && (HostName[i] < 0x30 || HostName[i] > 0x39))
            return 1;
    }
    return 0;
}

static int FindFieldIndex( const char * Field, char * Tmp, int TmpSize)
{
    int     FldSize = strlen(Field);
    int     sCount  = 0 ;
    int     tCount  = 0 ;

    while ( sCount < TmpSize )
    {
        if (&Tmp[sCount] == '\0') return TmpSize;
        if (!memcmp(&Tmp[sCount], Field, FldSize))  {
            break;
        }
        sCount ++;
        tCount ++;
    }

    return sCount + 11;
}

static int TestThread( void * data )
{
    char     tmpStr[10] ;
    snprintf(tmpStr, sizeof(tmpStr), "%s", (char*)data);
    int Count = 0;
    while ( 1 )
    {
        SDL_Delay(1000) ;
        Count ++        ;
        if (Count > 5)
            break       ;
    }
    fprintf(stderr, "Closed Thread No = [%s]\n", tmpStr);

    return(0);
}

int getKeyword (char const* buf, int bufsize, char const * field)
{
    int     fldsize = 0 ;
    int     iCount  = 0 ;

    fldsize = strlen(field) ;
    while ( iCount < bufsize )
    {
        if (&buf[iCount] == '\0') return 0;
        if (!memcmp(&buf[iCount], field, fldsize))
            break;
        iCount ++;
    }
    if (iCount == bufsize) return 0 ;
    iCount += fldsize;
    return iCount    ;
}

int RTSPFiltering (char * buf, int bufsize)
{
    char *  field   = "[Gabia_KeepAlive]"   ;
    int     fldsize = strlen(field)         ;
    int     iCount  = 0 ;
    int     FoundIt = 0 ;

    while ( iCount < bufsize )
    {
        if (!memcmp(&buf[iCount], field, fldsize))
            FoundIt = iCount + fldsize  ;
        iCount += fldsize   ;
        if (iCount > (FoundIt+fldsize)) break   ;
    }

    if (FoundIt > 0)    {
        memmove(buf, &buf[FoundIt], bufsize-FoundIt);
        memset (&buf[bufsize-FoundIt], 0, FoundIt  );
    }
    return 0;

//    fldsize = strlen(field)   ;
//    iCount  = bufsize - fldsize   ;
//
//    while ( iCount >= 0 )
//    {
//        if (!memcmp(&buf[iCount], field, fldsize))
//        {
//          FoundIt = 1     ;
//            break         ;
//        }
//        iCount --;
//    }
//
//  if (FoundIt)
//  {
//      iCount += fldsize   ;
//      memmove (buf, &buf[iCount], iCount);
//  }
//    return 0;
}

static inline char const * getLastTime ()
{
    static char     tmpTStr[15] ;
    struct tm       sttTm       ;
    struct timeval  sttTime     ;

    // 시간을구하여 클래스에 저장
    gettimeofday(&sttTime, NULL);
    sttTime.tv_sec = sttTime.tv_sec - 3 ;
    localtime_r (&sttTime.tv_sec,&sttTm);

    snprintf(tmpTStr, sizeof(tmpTStr), "%04d%02d%02d%02d%02d%02d",
                                        sttTm.tm_year + 1900,
                                        sttTm.tm_mon  + 1   ,
                                        sttTm.tm_mday       ,
                                        sttTm.tm_hour       ,
                                        sttTm.tm_min        ,
                                        sttTm.tm_sec        );
    return tmpTStr;
}

typedef struct
{
    char    year    [5];
    char    mon     [3];
    char    day     [3];
    char    hour    [3];
    char    min     [3];
    char    sec     [3];
}   Time_t;

// 년월일시분초 타입을 unix타임스템프 형식으로 변경 20111010123020
inline uint32_t GetUnixTime_val(const char * source)
{
    Time_t              time_t;
    struct tm           cTm;
    struct timeval      cTime;

   // 받아온 시간을 파싱하여 time_t 구조체에 넣음
    memcpy(&time_t.year, source   , 4);
    time_t.year[4] = '\0';
    memcpy(&time_t.mon , source+ 4, 2);
    time_t.mon[2]  = '\0';
    memcpy(&time_t.day , source+ 6, 2);
    time_t.day[2]  = '\0';
    memcpy(&time_t.hour, source+ 8, 2);
    time_t.hour[2] = '\0';
    memcpy(&time_t.min , source+10, 2);
    time_t.min[2]  = '\0';
    memcpy(&time_t.sec , source+12, 2);
    time_t.sec[2]  = '\0';

    // time_t 값을 tm에 넣어서 time 구조체로 변경
    cTm.tm_year  = atoi(time_t.year) - 1900;
    cTm.tm_mon   = atoi(time_t.mon)  - 1;
    cTm.tm_mday  = atoi(time_t.day)     ;
    cTm.tm_hour  = atoi(time_t.hour)    ;
    cTm.tm_min   = atoi(time_t.min)     ;
    cTm.tm_sec   = atoi(time_t.sec)     ;

    char buf[100];
    snprintf(buf, sizeof(buf), "%04d%02d%02d%02d%02d%02d",
                                cTm.tm_year + 1900  ,
                                cTm.tm_mon  + 1     ,
                                cTm.tm_mday         ,
                                cTm.tm_hour         ,
                                cTm.tm_min          ,
                                cTm.tm_sec          );
//    fprintf(stderr, "buf = (%s)\n", buf);
    cTime.tv_sec = mktime(&cTm)         ;
    return cTime.tv_sec;
}

//static inline const char * Second2DateTime(uint32_t seconds)
//{
//    static  char buf[100];
//    struct  tm   stTime  ;
//    struct  timeval tv   ;
//    tv.tv_sec = seconds  ;
//
//    localtime_r(&tv.tv_sec, &stTime);
//    snprintf(buf, sizeof(buf), "%04d%02d%02d%02d%02d%02d",
//                                stTime.tm_year + 1900  ,
//                                stTime.tm_mon  + 1     ,
//                                stTime.tm_mday         ,
//                                stTime.tm_hour         ,
//                                stTime.tm_min          ,
//                                stTime.tm_sec          );
//    return buf  ;
//}

typedef struct {
    uint64_t    key ;
}   VAL;

#define SZ_I64  sizeof(uint64_t)

static const inline uint8_t IsNumber ( char * String, int Size )
{
    if (!Size) return 0;
    for (int nByte = 0; nByte < Size; nByte++ )
    {
        if (String[nByte] < '0' || String[nByte] > '9')
            return  0   ;
    }
    return  1   ;
}

static const inline int getMessageParsing (char const* buf, int bufsize, uint8_t & Stream, char * AuthStr, uint8_t & Device)
{
    int     fldsize = 0 ;
    int     iFound  = 0 ;
    int     iLen    = 0 ;
    int     result  = 0 ;
    int     iCount  = 7 ;
    char    tmpStr[HEX_BUF];
    memset(tmpStr, 0, HEX_BUF);

    while ( iCount < bufsize )
    {
        if (&buf[iCount] == '\0') return 0;
        if ( buf[iCount] == '/') iFound++;
        iCount ++;
        if ( iFound >= 3 ) break;
    }

    //  get Stream Type
    while ( iCount < bufsize )
    {
        if ((buf[iCount] == '/') || (buf[iCount] == '\r' && buf[iCount+1] == '\n')) break;
        tmpStr[iLen] = buf[iCount];
        iCount ++;
    }

    Stream = 0 ;
    if ( !memcmp(tmpStr, "gabia-live", 10) ) Stream = 10;
    if ( !memcmp(tmpStr, "gabia-file", 10) ) Stream = 20;

    //  get Auth String
    iCount++ ;
    while ( iCount < bufsize )
    {
        if ((buf[iCount] == ' ') || (buf[iCount] == '\r' && buf[iCount+1] == '\n')) break;
        AuthStr[iLen] = buf[iCount];
        iCount ++;
        iLen   ++;
    }
    result = iLen;

    //  get Device
    fldsize = strlen("User-Agent:") ;
    while ( iCount < bufsize )
    {
        if (&buf[iCount] == '\0') return 0;
        if (!memcmp(&buf[iCount], "User-Agent:", fldsize))
            break;
        iCount ++;
    }
    if (iCount == bufsize) return 0 ;
    iCount += fldsize;

    memset(tmpStr, 0, HEX_BUF);
    iLen = 0;
    while (buf[iCount] == ' ') ++iCount;
    while ( iCount < bufsize )
    {
        if ((buf[iCount] == ' ') || (buf[iCount] == '\r' && buf[iCount+1] == '\n')) break;
        tmpStr[iLen] = buf[iCount];
        iCount ++;
        iLen ++;
    }

    Device = (IsNumber (tmpStr, iLen) ? atoi(tmpStr) : 0);
    return result ;
}

static inline const int getPosition (const char * buf, int bufsize, char const* key)
{
    int     keysize = 0   ;
    int     iCount  = 200 ;

    if (key != NULL)
    {
        keysize = strlen(key) ;
        while ( iCount < bufsize )
        {
            if (&buf[iCount] == '\0') return 0;
            if (!memcmp(&buf[iCount], key, keysize))
                break;

//            fprintf(stderr, "keysize = [%d]\n", keysize);
//            fprintf(stderr, "key     = [%s]\n", key);
//            fprintf(stderr, "buf     = [%s]\n",&buf[iCount]);
            iCount ++;
        }
        if (iCount == bufsize) return 0 ;
        iCount += keysize;
    }
    return iCount;
}

//static inline const bool getSDPRead (char * pSDP)
//{
//  char  paramSet[100] ;
//  snprintf(paramSet, 100, "Z0KAMtoCgPRA=,aM48gA==");
//
//  char  tmpSDP  [1024];
//  memset(tmpSDP, 0, 1024)     ;
//  memcpy(tmpSDP, pSDP, 1024)  ;
//
//  int Len = strlen(paramSet);
//  int pos = getPosition (tmpSDP, strlen(tmpSDP), "sprop-parameter-sets=");
//  if (pos)
//  {
//      memcpy(&pSDP[pos]    ,  paramSet   , Len);
//      memcpy(&pSDP[pos+Len], &tmpSDP[pos], strlen(tmpSDP)-pos);
//  }
//
//    return true;
//}

static inline const bool getSDPRead (char * pSDP, int Len, char * lBuf)
{
    char   tmpSDP  [2048]  ;
    memset(tmpSDP, 0, 2048);
    memcpy(tmpSDP, pSDP, Len );
    memset(pSDP  , 0   , Len );

    fprintf(stderr, "tmpSDP = \n%s\n", tmpSDP);
    fprintf(stderr, "lBuf = \n%s\n", lBuf);
    int pos = getPosition (tmpSDP, strlen(tmpSDP), (const char*)lBuf);
    int org = pos;
    fprintf(stderr, "pos = %d\n", pos);
    if (pos)
    {
        while ( true )
        {
            if (!memcmp(&tmpSDP[pos-1],",",1))
                pos--  ;
            else
                break  ;
        }

        memcpy( pSDP     ,  tmpSDP     , pos);
        memcpy(&pSDP[pos], &tmpSDP[org], Len-org);
    }

    return true;
}

static inline const char * getIpaddr(const char * ifname)
{
    int s;
    struct ifconf ifconf;
    struct ifreq ifr[50];
    int ifs;
    int i;
    static char ipaddr[INET_ADDRSTRLEN];

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        perror("socket");
        return 0;
    }

    ifconf.ifc_buf = (char *) ifr;
    ifconf.ifc_len = sizeof ifr;

    if (ioctl(s, SIOCGIFCONF, &ifconf) == -1) {
        perror("ioctl");
        return 0;
    }

    ifs = ifconf.ifc_len / sizeof(ifr[0]);
    fprintf(stderr, "interfaces = %d:\n", ifs);
    memset(ipaddr, 0, sizeof(ipaddr));
    for (i = 0; i < ifs; i++)
    {
        char ip[INET_ADDRSTRLEN];
        struct sockaddr_in *s_in = (struct sockaddr_in *) &ifr[i].ifr_addr;

        if (!inet_ntop(AF_INET, &s_in->sin_addr, ip, sizeof(ip))) {
          perror("inet_ntop");
          return 0;
        }

        fprintf(stderr,"[%s] - [%s]\n", ifr[i].ifr_name, ip);
        if ( !memcmp(ifr[i].ifr_name, ifname, 4) && (strlen(ip) > 0) )
        {
            snprintf(ipaddr, sizeof(ipaddr), "%s", ip);
            break;
        }
    }

    close(s);
    return ipaddr;
}

#define RTP_MAX_PACKET_LEN 1500

#pragma pack(push, 1)

typedef struct {
    /* The following are pointers to the data in the packet as    */
    /* it came off the wire. The packet it read in such that the  */
    /* header maps onto the latter part of this struct, and the   */
    /* fields in this first part of the struct point into it. The */
    /* entire packet can be freed by freeing this struct, without */
    /* having to free the csrc, data and extn blocks separately.  */
    /* WARNING: Don't change the size of the first portion of the */
    /* struct without changing RTP_PACKET_HEADER_SIZE to match.   */
  struct _meta {
    uint32_t    *csrc;
    char        *data;
    int      data_len;
    unsigned char   *extn;
    uint16_t     extn_len;  /* Size of the extension in 32 bit words minus one */
    uint16_t     extn_type; /* Extension type field in the RTP packet header   */
  } meta;

    struct _fields {
    /* The following map directly onto the RTP packet header...   */
#ifdef WORDS_BIGENDIAN
    unsigned short   v:2;       /* packet type                */
    unsigned short   p:1;       /* padding flag               */
    unsigned short   x:1;       /* header extension flag      */
    unsigned short   cc:4;      /* CSRC count                 */
    unsigned short   m:1;       /* marker bit                 */
    unsigned short   pt:7;      /* payload type               */
#else
    unsigned short   cc:4;      /* CSRC count                 */
    unsigned short   x:1;       /* header extension flag      */
    unsigned short   p:1;       /* padding flag               */
    unsigned short   v:2;       /* packet type                */
    unsigned short   pt:7;      /* payload type               */
    unsigned short   m:1;       /* marker bit                 */
#endif
    uint16_t          seq;      /* sequence number            */
    uint32_t          ts;       /* timestamp                  */
    uint32_t          ssrc;     /* synchronization source     */
    /* The csrc list, header extension and data follow, but can't */
    /* be represented in the struct.                              */
    } fields;

}   rtp_packet;

//typedef struct {
//  /* The following map directly onto the RTP packet header...   */
//  unsigned short   cc:4;      /* CSRC count                 */
//  unsigned short   x:1;       /* header extension flag      */
//  unsigned short   p:1;       /* padding flag               */
//  unsigned short   v:2;       /* packet type                */
//  unsigned short   pt:7;      /* payload type               */
//  unsigned short   m:1;       /* marker bit                 */
//
////    uint8_t          cc:4;      /* CSRC count                 */
////    uint8_t          x :1;      /* header extension flag      */
////    uint8_t          p :1;      /* padding flag               */
////    uint8_t          v :2;      /* packet type                */
////    uint8_t          pt:7;      /* payload type               */
////    uint8_t          m :1;      /* marker bit                 */
//
//  uint16_t         seq ;      /* sequence number            */
//  uint32_t         ts  ;      /* timestamp                  */
//  uint32_t         ssrc;      /* synchronization source     */
//  /* The csrc list, header extension and data follow, but can't */
//  /* be represented in the struct.                              */
//}     rtp_packet;


typedef struct _hid_
{
    uint32_t    UHID        ;
    uint8_t     ttype       ;
    uint32_t    UTID        ;
}   _HID_;

typedef struct _snshd_
{
    uint32_t    length      ;
    uint8_t     ver         ;
    _HID_       SID         ;
    _HID_       TID         ;
    uint8_t     mtype       ;
    uint8_t     fid         ;
    uint8_t     result      ;
    uint8_t     encrypt     ;
    char        reserved[5] ;
}   _SNSHD_;
#define SZ_SNSHD    sizeof(_SNSHD_)

typedef struct _detect_
{
    uint8_t     ProtocolType;
    uint8_t     DeviceType  ;
    uint8_t     Subid       ;
    char        MacAddress  [8];
    uint8_t     Command     ;
    uint8_t     DataLen     ;
    char        Data        ;
}   _DETECT_;
#define SZ_DET    sizeof(_DETECT_)

#pragma pack(pop)

static inline const int getValue (char const* buf, int bufsize, char const * field, char * Val, const char sep)
{
    int     fldsize = 0 ;
    int     iLen    = 0 ;
    int     iCount  = 0 ;

    fldsize = strlen(field) ;
    while ( iCount < bufsize )
    {
        if (&buf[iCount] == '\0')
        {
            printf("11111 iCount = [%d], bufsize = [%d]\n", iCount, bufsize);
            return 0;
        }
        if (!memcmp(&buf[iCount], field, fldsize))
            break;
        iCount ++;
    }
    if (iCount == bufsize)
    {
        printf("22222 iCount = [%d], bufsize = [%d]\n", iCount, bufsize);
        return 0 ;
    }

    iCount += fldsize;
    while (buf[iCount] == ' ') ++iCount;
    while ( iCount < bufsize )
    {
        if ((buf[iCount] == sep) || (buf[iCount] == '\r' && buf[iCount+1] == '\n')) break;
        Val[iLen] = buf[iCount];
        iCount ++;
        iLen   ++;
    }

    printf("33333 fldsize = [%d], iCount = [%d], bufsize = [%d], val = [%s]\n", fldsize, iCount, bufsize, Val);
    return iLen ;
}

/* This assumes that an unsigned char is exactly 8 bits. Not portable code! :-) */
static unsigned char index_64[128] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,   62, 0xff, 0xff, 0xff,   63,
      52,   53,   54,   55,   56,   57,   58,   59,   60,   61, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff,    0,    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14,
      15,   16,   17,   18,   19,   20,   21,   22,   23,   24,   25, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff,   26,   27,   28,   29,   30,   31,   32,   33,   34,   35,   36,   37,   38,   39,   40,
      41,   42,   43,   44,   45,   46,   47,   48,   49,   50,   51, 0xff, 0xff, 0xff, 0xff, 0xff
};

#define char64(c)  ((c > 127) ? 0xff : index_64[(c)])

int base64decode(const unsigned char *input, int input_length, unsigned char *output, int output_length)
{
    int     i = 0, j = 0, pad;
    unsigned char   c[4];

    if (!(output_length >= (input_length * 3 / 4)) || !((input_length % 4) == 0)) return -1;
    while ((i + 3) < input_length) {
        pad  = 0;
        c[0] = char64(input[i  ]); pad += (c[0] == 0xff);
        c[1] = char64(input[i+1]); pad += (c[1] == 0xff);
        c[2] = char64(input[i+2]); pad += (c[2] == 0xff);
        c[3] = char64(input[i+3]); pad += (c[3] == 0xff);
        if (pad == 2) {
            output[j++] = (c[0] << 2) | ((c[1] & 0x30) >> 4);
            output[j]   = (c[1] & 0x0f) << 4;
        } else if (pad == 1) {
            output[j++] = (c[0] << 2) | ((c[1] & 0x30) >> 4);
            output[j++] = ((c[1] & 0x0f) << 4) | ((c[2] & 0x3c) >> 2);
            output[j]   = (c[2] & 0x03) << 6;
        } else {
            output[j++] = (c[0] << 2) | ((c[1] & 0x30) >> 4);
            output[j++] = ((c[1] & 0x0f) << 4) | ((c[2] & 0x3c) >> 2);
            output[j++] = ((c[2] & 0x03) << 6) | (c[3] & 0x3f);
        }
        i += 4;
    }
    return j;
}

int getDividePoint ( const char * source, int len )
{
    for ( int i = 0; i < len; i++ )
    {
        if ( source[i] == ',' )
            return i;
    }
    return 0;
}

int getSPSSInfo (const char * source, int len, char * spss)
{
    char tmpspss[32] = {};
    int pos = getDividePoint(source,len);
    memcpy(tmpspss, source, pos);
    int tmplen = strlen(tmpspss);
    return base64decode( (unsigned char*)tmpspss, tmplen, (unsigned char*)spss, sizeof(tmpspss));
}

int getPPSInfo (const char * source, int len, char * pps)
{
    char tmppps [32] = {};
    int pos = getDividePoint(source,len)+1;
    memcpy(tmppps,&source[pos], len-pos);
    int tmplen= strlen(tmppps);
    return base64decode( (unsigned char*)tmppps, tmplen, (unsigned char*)pps, sizeof(tmppps));
}

bool getPrint ()
{
    printf("test call 1111111111 !!! \n");
    return false;
}

bool IsALive = true;

bool myThreadDelay( int MaxSec )
{
    int TCount = 0;
    while (TCount < MaxSec)
    {
        if (!IsALive) return false;
        TCount ++     ;
        SDL_Delay(990);
    }
    return true ;
}

static const inline int getBuffSize(const char * resolution, double _rate_ )
{
    if (!memcmp(resolution, "320x240" , 7))
        return  (int)(320  * 240  * _rate_);   // 46080
    else
    if (!memcmp(resolution, "480x360" , 7))
        return  (int)(480  * 360  * _rate_);   // 103680
    else
    if (!memcmp(resolution, "640x480" , 7))
        return  (int)(640  * 480  * _rate_);   // 184320
    else
    if (!memcmp(resolution, "1280x720", 8))
        return  (int)(1280 * 720  * _rate_);   // 552960
    else
    if (!memcmp(resolution, "1280x1024",9))
        return  (int)(1280 * 1024 * _rate_);   // 786432
    else
    if (!memcmp(resolution, "1600x1200",9))
        return  (int)(1600 * 1200 * _rate_);   // 1152000
    else
        return  (int)(1920 * 1080 * _rate_);   // 1244160
}

#define _CAM_TYPE_  0xE0            //  xxx..... : 8타입 : AXIS (0)(000), HITRON(1)(001)
#define _CAM_KIND_  0x1C            //  ...xxx.. : 8종류 : CUBE (0)(000), BULL  (1)(001), DOME (2)(010)
#define _CAM_OPTN_  0x03            //  ......xx : 4옵션 : BASIC(0)(00) , FULL  (3)(11)

uint8_t getCamType( uint8_t val )
{
    return (val & _CAM_TYPE_) >> 5;
}

uint8_t getCamKind( uint8_t val )
{
    return (val & _CAM_KIND_) >> 2;
}

uint8_t getCamOptn( uint8_t val )
{
    return (val & _CAM_OPTN_);
}

bool getCalled (bool * isClose)
{
    if (*isClose)
        printf("test true  !!!\n");
    else
        printf("test false !!!\n");
    return false;
}

void gettime(char* str, char type)
{
    struct tm *tp;
    struct timeval timv;

    gettimeofday(&timv, NULL);
    tp = localtime((time_t *)&timv.tv_sec);

    switch( type )
    {
        case    's' :
        case    'S' :
                snprintf(str, 100, "%2d/%2d %02d:%02d:%02d.%06d",
                        tp->tm_mon+1, tp->tm_mday,
                        tp->tm_hour, tp->tm_min, tp->tm_sec, (int)timv.tv_usec);
                break;
        case    'l' :
                snprintf(str, 100, "%4d/%02d/%02d %02d:%02d:%02d.%03d %06d",
                        tp->tm_year+1900, tp->tm_mon+1, tp->tm_mday,
                        tp->tm_hour, tp->tm_min, tp->tm_sec, (int)(timv.tv_usec/1000), timv.tv_usec);
                break;
        case    'L' :
                snprintf(str, 100, "%4d/%02d/%02d %02d:%02d:%02d.%06d",
                        tp->tm_year+1900, tp->tm_mon+1, tp->tm_mday,
                        tp->tm_hour, tp->tm_min, tp->tm_sec, (int)timv.tv_usec);
                break;
        default :
                break;
    }
}

static inline const char * second2Minute(uint32_t seconds)
{
    static  char buf[HEX_BUF];
    struct  tm   stTime ;
    struct  timeval tv  ;
    tv.tv_sec  = seconds;
    tv.tv_usec = 0;

    localtime_r(&tv.tv_sec, &stTime);
    snprintf(buf, sizeof(buf), "%02d%02d", stTime.tm_min, stTime.tm_sec);
    return buf;
}

static inline const int getSeconds(int Val)
{
    struct timeval timv         ;
    gettimeofday(&timv, NULL)   ;
    return (timv.tv_sec%Val)    ;
}

static inline const int getSeconds()
{
    struct timeval timv         ;
    gettimeofday(&timv, NULL)   ;
    return (timv.tv_sec%3600)   ;
}

uint32_t NewInterval2(int stdTime, int oTime)
{
    uint32_t svSeconds = 0  ;

    uint32_t setTime   = getSeconds(stdTime);
    //stdTime   = (setTime < stdTime ? stdTime : (stdTime*2));
    svSeconds =  stdTime - setTime + oTime;

    return svSeconds    ;
}

uint32_t NewInterval1(int stdTime, int oTime)
{
    uint32_t svSeconds = 0  ;

    uint32_t setTime   = getSeconds();
    stdTime   = (setTime < stdTime ? stdTime : (stdTime*2));
    svSeconds =  stdTime - setTime + oTime;

    return svSeconds    ;
}

static inline const void delayTimestamp(uint32_t microseconds)
{
    struct timeval timeout          ;
    timeout.tv_sec  = 0             ;//microseconds/TimestampTicks   ;
    timeout.tv_usec = microseconds  ;//microseconds%TimestampTicks   ;
    if (select(0, NULL, NULL, NULL, &timeout) <= 0) {
        return ;
    }
}


typedef u_int64_t Timestamp;
#define TimestampTicks  1000000LU


//#define _SLEEP_TIME_    10
//#define _TIME2SLEEP_    860
#define _TIME_COUNT_   (uint64_t)(_TIME2SLEEP_ * 30.0)        //    3 seconds


const inline Timestamp TimeToVal(struct timeval* pTimeval) {
    return ((u_int64_t)pTimeval->tv_sec * TimestampTicks) + pTimeval->tv_usec;
}

const inline Timestamp getTimeDiff(struct timeval* pTimeval) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return TimeToVal(&tv) - TimeToVal(pTimeval);
}

static inline const void delayTimesL(uint32_t microseconds)
{
    struct timeval timeout = {0,microseconds};
    if (select(0, NULL, NULL, NULL, &timeout) <= 0) {
        return ;
    }
}

int timeout_AlarmTimer ()
{
    int TCount = 0;
    int TickCount = 0;
    struct timeval tv;
    gettimeofday(&tv, NULL);

    fprintf(stderr,"%s()[%d] \x1b[1;35mStarting ...\x1b[0m\n", __FUNCTION__, __LINE__);
    //delayTimesL ( 3000000 );

    while(true)
    {
        SDL_Delay(_SLEEP_TIME_);
        TCount += _SLEEP_TIME_ ;
        if (TCount > _TIME_COUNT_) break;
        TickCount ++ ;
    }

    fprintf(stderr,"%s()[%d] \x1b[1;35mClosing (%llu)(TCount = %d)  ...\x1b[0m\n", __FUNCTION__, __LINE__, getTimeDiff (&tv),TickCount);
    return 0 ;
}

///////////////////////////////////////////////////////////////////////////////////
int ConnectToSNSGateway()
{
    int m_socket  = 0 ;
    int TryCount  = 0 ;
    int MaxTryCnt = 1 ;
    while (TryCount < MaxTryCnt)
    {
        m_socket = setupSocket(0, 0 /* =>nonblocking */ );

        do  {
            struct sockaddr_in managerName;
            managerName.sin_family      = AF_INET;
            managerName.sin_port        = htons(7001);
            managerName.sin_addr.s_addr = inet_addr("211.245.162.131");

            if (connect (m_socket, (struct sockaddr*)&managerName, sizeof managerName) != 0)
            {
                fprintf(stderr, "%s()[%d] Connection Failed 'SNSGateway' (7001)\n", __FUNCTION__, __LINE__);
                close(m_socket) ;
                TryCount ++     ;
                break           ;
            }

            int curFlags = fcntl(m_socket, F_GETFL, 0);
            if (fcntl(m_socket, F_SETFL, curFlags|O_NONBLOCK) < 0)
            {
                fprintf(stderr, "Socket SetNonBlock Error\n");
                break;
            }

            return  m_socket ;
        }   while (false);
    }

    close(m_socket);
    return  0 ;
}

void makeHeaderGenerator( _SNSHD_ * Snshd, uint8_t msgType )
{
    if (Snshd != NULL)
    {
        Snshd->length    = SZ_SNSHD   ;
        Snshd->ver       = 1          ;
        Snshd->SID.UHID  = htonl(0)   ;
        Snshd->SID.ttype = 1          ;
        Snshd->SID.UTID  = 0          ;
        Snshd->TID.UHID  = htonl(0)   ;
        Snshd->TID.ttype = 6          ;
        Snshd->TID.UTID  = 0          ;
        Snshd->mtype     = 0x02       ;   // 0 :request, 1 :response, 2 :event
        Snshd->fid       = msgType    ;
        Snshd->result    = 0x00       ;
        Snshd->encrypt   = 0          ;   // 0 :not encrypted, 1:encrypted
    }
}

int makeBodyGenerator( char * Body, int msgType )
{
    int Len = 0;
    switch ( msgType )
    {
    case    0x41      :   {
                Body[0] = 0x00 ;
                memcpy(&Body[1], "SA00001376645196", 16);
                Len = 1 + 16;
                break;
            }
    case    0x42      :   {
                _DETECT_ * det = (_DETECT_*)Body ;
                memcpy(&Body[SZ_DET], "SA00001376645196", 16);
                Len = SZ_DET + 16;
                break;
            }
    }
    return Len ;
}


int SendSocketExact(int socket, char * buffer, int bufferSize)
{
    int bytesSend    = 0;
    int totBytesSend = 0;
    int TryCount     = 0;
    do  {
        bytesSend = send (socket, buffer + totBytesSend, bufferSize-totBytesSend, 0);
        if (bytesSend < 0)
        {
            if(errno != EAGAIN)
            {
                fprintf(stderr, "%s()[%d] sending failed !! [%d][%s]", __FUNCTION__, __LINE__, errno, strerror(errno));
                return -2   ;
            }
            SDL_Delay(_SLEEP_TIME_);
        }   else
        {
            totBytesSend += bytesSend;
        }
    }   while (bufferSize != totBytesSend);
    return totBytesSend;
}


static int Sns_Test_Proc (void * param)
{
    char    Buff  [200] = {}  ;
    int     mLen   = SZ_SNSHD ;
    int     socket = 0        ;

    if ((socket = ConnectToSNSGateway()) <= 0)
    {
        fprintf(stderr,"Thread Closing ...\n");
        return 0;
    }

    _SNSHD_  * Snshd = (_SNSHD_*)Buff   ;
    char     * Body  = ( char *)&Buff[SZ_SNSHD];
    makeHeaderGenerator(Snshd, 0x42)    ;  //CAMERA_ARMED_STATE
    mLen += makeBodyGenerator(Body,0x42);

    Snshd->length = htonl(mLen);
    int sendbytes = SendSocketExact (socket, Buff, mLen);
    if (sendbytes != mLen)
        fprintf(stderr, "%s()[%d] : Send Message Error (%d)(%s) !!!", __FUNCTION__, __LINE__, errno, strerror(errno));

    close(socket);
    return 0 ;
}

//////////////// Notify Message Unicast Function //////////////////////////////
int connectWithTimeoutNS(int fd,struct sockaddr *remote, int len, int secs, int *err)
{
    int saveflags,ret,back_err;
    fd_set fd_w;
    struct timeval timeout = {secs, 0};

    saveflags=fcntl(fd,F_GETFL,0);
    if(saveflags<0)
    {
        perror("\"fcntl1\"");
        *err=errno;
        return -1;
    }

    /* Set non blocking */
    if(fcntl(fd,F_SETFL,saveflags|O_NONBLOCK)<0)
    {
        perror("\"fcntl2\"");
        *err=errno;
        return -1;
    }

    /* This will return immediately */
    *err=connect(fd,remote,len);
    back_err=errno;

    /* restore flags */
    if(fcntl(fd,F_SETFL,saveflags)<0)
    {
        perror("\"fcntl3\"");
        *err=errno;
        return -1;
    }

    /* return unless the connection was successful or the connect is
    still in progress. */
    if(*err<0 && back_err!=EINPROGRESS)
    {
        perror("\"connect\"");
        *err=errno;
        return -1;
    }

    FD_ZERO(&fd_w);
    FD_SET(fd,&fd_w);

    *err=select(FD_SETSIZE,NULL,&fd_w,NULL,&timeout);
    if(*err<0)
    {
        perror("\"select\"");
        *err=errno;
        return -1;
    }

    /* 0 means it timeout out & no fds changed */
    if(*err==0)
    {
        perror("\"timeout...\"");
        *err=ETIMEDOUT;
        return -1;
    }

    /* Get the return code from the connect */
    socklen_t addrsize = sizeof remote;
    *err = getsockopt(fd,SOL_SOCKET,SO_ERROR,&ret,&addrsize);
    if(*err<0)
    {
        perror("\"getsockopt\"");
        *err=errno;
        return -1;
    }

    /* ret=0 means success, otherwise it contains the errno */
    if(ret)
    {
        *err=ret;
        return -1;
    }
    *err=0;
    return 0;
}

int ConnectToLocalhost()
{
    int m_socket  = 0 ;
    int err ;

    m_socket = setupSocket(0, 1 /* =>nonblocking */ );
    do  {
        struct sockaddr_in managerName;
        managerName.sin_family      = AF_INET;
        managerName.sin_port        = htons(4242);
        managerName.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (connectWithTimeoutNS(m_socket, (struct sockaddr*)&managerName, sizeof managerName, 5, &err) != 0)
        //if (connect (m_socket, (struct sockaddr*)&managerName, sizeof managerName) != 0)
        {
            fprintf(stderr, "%s()[%d] Connection Failed 'OpenTSDB' (4242)\n", __FUNCTION__, __LINE__);
            break ;
        }

        return  m_socket ;
    }   while (false);

    close(m_socket);
    return  0 ;
}


bool getRecv(int socket, char * pBuff)
{
    while (true)
    {
        int readBytes = getData(socket, pBuff, 200, _RECV_TIME_);
        if (readBytes <= 0)
        {
            fprintf(stderr, "%s()[%d] (%d)", __FUNCTION__, __LINE__, readBytes);
            return false    ;
        }
        return true ;
    }
}

#define TimestampTiks 1000000
static inline const uint64_t GetTimeStamp() {
    struct  timeval tv      ;
    gettimeofday(&tv, NULL) ;
    return ((uint64_t)tv.tv_sec * TimestampTiks) + tv.tv_usec;
}

#define TimestampSemiTiks 1000
static inline const uint64_t getTimestamp() {
    struct  timeval tv      ;
    gettimeofday(&tv, NULL) ;
    return ((uint64_t)tv.tv_sec * TimestampSemiTiks) + (tv.tv_usec/TimestampSemiTiks);
}

static inline const uint32_t GetTimestamp() {
    struct  timeval tv      ;
    gettimeofday(&tv, NULL) ;
    return tv.tv_sec ;
}

static inline const char * getTimestamps(int days = 0) {
    static char buf[32]      ;
    struct timeval tv        ;
    int sdays = days * 3600  ;
    gettimeofday(&tv, NULL)  ;
    tv.tv_sec += sdays       ;
    snprintf(buf, sizeof(buf), "%u%03u", tv.tv_sec, (unsigned)(tv.tv_usec/1000));
    return  buf;
}


#define _PUTCN_ 5
#define _SBUFF_ (_PUTCN_ * 100)
#define _CPRNT_ 100
static int openTSDB_Test (void * param)
{
    char    Buff  [200] = "";
    int     mLen   = 0      ;
    int     socket = 0      ;
    char    shost           ;
    uint64_t * nCount = (uint64_t*)param;

    if ((socket = ConnectToLocalhost()) <= 0)
    {
        fprintf(stderr,"Socket Error : Thread Closing ...\n");
        return 0;
    }

    randomize();
    while (true)
    {
        (*nCount)++;
        int v = (random(30)+1) + 100;

//        snprintf(Buff, sizeof(Buff), "put test.out.bytes %s %d mtime=%llu a=2 b=3 c=4 d=5 e=6 f=7 g=8\n",
//                                      getTimestamps(), v, GetTimeStamp());
        // disable ( ',' ':' ';' '?' ... )
        // enable  ( '.' '-' '_' '/' ) 이외 나머지 안됨.
        snprintf(Buff, sizeof(Buff), "put test.out.bytes %s %d mtime=%llu a=Korea b=3\n", getTimestamps(), v, GetTimeStamp());
        mLen = strlen(Buff);

        int sendbytes = SendSocketExact (socket, Buff, mLen);
        if (sendbytes != mLen)
            fprintf(stderr, "%s()[%d] : Send Message Error (%d)(%s) !!!\n", __FUNCTION__, __LINE__, errno, strerror(errno));

        memset(Buff, 0, sizeof(Buff));
        int ret = getData(socket, Buff, sizeof(Buff), 1000);
        fprintf(stderr, "%s()[%d] : recv Message (%d)(%s) !!!\n", __FUNCTION__, __LINE__, ret, Buff);
        //if ((*nCount) % 4 == 0) usleep(2);
        //usleep(2);
        break;
    }

    close(socket);
    return 0 ;
}

static int openTSDB_Coun (void * param)
{
    uint64_t * nCount = (uint64_t*)param;
    uint64_t   oCount = 0;
    uint64_t   cCount = 0;
    while (true)
    {
        cCount = *nCount - oCount;
        oCount = *nCount;
        fprintf(stderr, "%s()[%d] : %u nCount = (%llu)\n", __FUNCTION__, __LINE__, GetTimestamp(), cCount);
        SDL_Delay(999);
    }
    return 0 ;
}

static unsigned NetMask( const char * addr, unsigned subnet)
{
    if (addr == NULL) return 0;
    unsigned IpValue = inet_addr(addr);
    return  (subnet & IpValue);
}

static unsigned NetCheck(const char * addr1, const char * addr2, unsigned subnet)
{
    if (addr1 == NULL) return false;
    if (addr2 == NULL) return false;
    return (NetMask(addr1, subnet) == NetMask(addr2, subnet));
    //unsigned IpValue1 = inet_addr(addr1);
    //unsigned IpValue2 = inet_addr(addr2);
    //IpValue1 = (subnet & IpValue1);
    //IpValue2 = (subnet & IpValue2);
    //return (IpValue1 == IpValue2) ;
}

static void  SPD(float Bitrate, float applyrate)
{
    uint64_t  UseStorage      = 0;
    uint32_t  RecordSecOneDay = 86400;
    UseStorage = (uint64_t)((((((Bitrate)/8)*1024)*RecordSecOneDay)/1000/1000) * (applyrate/100));
    printf("bitrate = %.0f, applyrate = %0.f%%, UseStorage = (%llu)\n", Bitrate, applyrate, UseStorage);
}


int rTrimLen(char * buffer, int buflen)
{
    int nlen = buflen;
    for (int i = buflen-1; i >= 0; i--)
    {
        if (buffer[i] != '\0') break;
        nlen -- ;
    }
    return nlen ;
}

void setColor()
{
    char testStr[120] = {0,};
    for ( int i = 29; i < 50; i++ )
    {
        snprintf(testStr, sizeof(testStr), "Number = [0][%d] \x1b[0;%dmTest Color ...\x1b[0m", i, i);
        printf("%s\n", testStr);
        snprintf(testStr, sizeof(testStr), "Number = [1][%d] \x1b[1;%dmTest Color ...\x1b[0m", i, i);
        printf("%s\n", testStr);
    }
}

static inline void getSDate (int8_t gaps = 0)
{
    printf("gaps = %d\n", gaps);
}

static inline const void delayTimestamp1(uint32_t microseconds)
{
    //struct timeval timeout          ;
    //timeout.tv_sec  = microseconds/TimestampTicks;
    //timeout.tv_usec = microseconds%TimestampTicks;

    struct timeval timeout = {0,microseconds};
    printf("tv_sec = (%u), tv_usec = (%u)\n", timeout.tv_sec, timeout.tv_usec);
    if (select(0, NULL, NULL, NULL, &timeout) <= 0) {
        printf("tv_sec = (%u), tv_usec = (%u)\n", timeout.tv_sec, timeout.tv_usec);
        return ;
    }
    printf("tv_sec = (%u), tv_usec = (%u)\n", timeout.tv_sec, timeout.tv_usec);
}

static void ReleaseData (void * f)
{
    if (f)
    {
        StreamConnectionInfo * info = (StreamConnectionInfo*)f;
        delete info ;
        info = NULL ;
        //printf("delete SerialNo !!!\n");
    }
}

// Generate a "Date:" header for use in a RTSP response:
static char const* dateTime() {
    static char buf[200];
    time_t tt = time(NULL);
    strftime(buf, sizeof buf, "%b %d %H:%M:%S", localtime(&tt));
    return buf;
}

//Numerical         Severity
//          Code
//
//           0       Emergency: system is unusable
//           1       Alert: action must be taken immediately
//           2       Critical: critical conditions
//           3       Error: error conditions
//           4       Warning: warning conditions
//           5       Notice: normal but significant condition
//           6       Informational: informational messages
//           7       Debug: debug-level messages

static char * getSeverity ( int No )
{
    static char tmpStr[10];
    switch ( No )
    {
    case   0  :  snprintf(tmpStr, sizeof(tmpStr), "Emerg");  break;  //Emergency: system is unusable
    case   1  :  snprintf(tmpStr, sizeof(tmpStr), "Alert");  break;  //Alert: action must be taken immediately
    case   2  :  snprintf(tmpStr, sizeof(tmpStr), "Crit");   break;  //Critical: critical conditions
    case   3  :  snprintf(tmpStr, sizeof(tmpStr), "Error");  break;  //Error: error conditions
    case   4  :  snprintf(tmpStr, sizeof(tmpStr), "Warn");   break;  //Warning: warning conditions
    case   5  :  snprintf(tmpStr, sizeof(tmpStr), "Notice"); break;  //Notice: normal but significant condition
    case   6  :  snprintf(tmpStr, sizeof(tmpStr), "Info");   break;  //Informational: informational messages
    case   7  :  snprintf(tmpStr, sizeof(tmpStr), "Debug");  break;  //Debug: debug-level messages
    default   :  snprintf(tmpStr, sizeof(tmpStr), "");       break;
    }
    return tmpStr;
}

// Numerical             Facility
//          Code
//
//           0             kernel messages
//           1             user-level messages
//           2             mail system
//           3             system daemons
//           4             security/authorization messages (note 1)
//
//           5             messages generated internally by syslogd
//           6             line printer subsystem
//           7             network news subsystem
//           8             UUCP subsystem
//           9             clock daemon (note 2)
//          10             security/authorization messages (note 1)
//          11             FTP daemon
//          12             NTP subsystem
//          13             log audit (note 1)
//          14             log alert (note 1)
//          15             clock daemon (note 2)
//          16             local use 0  (local0)
//          17             local use 1  (local1)
//          18             local use 2  (local2)
//          19             local use 3  (local3)
//          20             local use 4  (local4)
//          21             local use 5  (local5)
//          22             local use 6  (local6)
//          23             local use 7  (local7)

static char * getFacility ( int No )
{
    static char tmpStr[20];
    switch ( No )
    {
    case   0  :  snprintf(tmpStr, sizeof(tmpStr), "kernel");       break;
    case   1  :  snprintf(tmpStr, sizeof(tmpStr), "user-level");   break;
    case   2  :  snprintf(tmpStr, sizeof(tmpStr), "mail");         break;
    case   3  :  snprintf(tmpStr, sizeof(tmpStr), "system");       break;
    case   4  :  snprintf(tmpStr, sizeof(tmpStr), "authpriv");     break;
    case   5  :  snprintf(tmpStr, sizeof(tmpStr), "syslogd");      break;
    case   6  :  snprintf(tmpStr, sizeof(tmpStr), "line printer"); break;
    case   7  :  snprintf(tmpStr, sizeof(tmpStr), "network news"); break;
    case   8  :  snprintf(tmpStr, sizeof(tmpStr), "UUCP");         break;
    case   9  :  snprintf(tmpStr, sizeof(tmpStr), "clock");        break;
    case  10  :  snprintf(tmpStr, sizeof(tmpStr), "authpriv");     break;
    case  11  :  snprintf(tmpStr, sizeof(tmpStr), "FTP");          break;
    case  12  :  snprintf(tmpStr, sizeof(tmpStr), "NTP");          break;
    case  13  :  snprintf(tmpStr, sizeof(tmpStr), "audit");        break;
    case  14  :  snprintf(tmpStr, sizeof(tmpStr), "alert");        break;
    case  15  :  snprintf(tmpStr, sizeof(tmpStr), "clock");        break;
    case  16  :  snprintf(tmpStr, sizeof(tmpStr), "local0");       break;
    case  17  :  snprintf(tmpStr, sizeof(tmpStr), "local1");       break;
    case  18  :  snprintf(tmpStr, sizeof(tmpStr), "local2");       break;
    case  19  :  snprintf(tmpStr, sizeof(tmpStr), "local3");       break;
    case  20  :  snprintf(tmpStr, sizeof(tmpStr), "local4");       break;
    case  21  :  snprintf(tmpStr, sizeof(tmpStr), "local5");       break;
    case  22  :  snprintf(tmpStr, sizeof(tmpStr), "local6");       break;
    case  23  :  snprintf(tmpStr, sizeof(tmpStr), "local7");       break;
    default   :  snprintf(tmpStr, sizeof(tmpStr), "");             break;
    }
    return tmpStr;
}

int getPriority (unsigned char * buf, int & pos)
{
    int  Count    = 1  ;
    int  Len      = 0  ;
    char PRI[5]   = {0,} ;
    int  priority = 0  ;
    int  bufsize  = 10 ;

    fprintf(stderr, "buf = (%s)\n", buf);

    if (buf[0] == '<')
    {
        while ( Count < bufsize )
        {
            if (buf[Count] == '>') break;
            PRI[Len] = buf[Count];
            Count ++;
            Len   ++;
        }
    }

    fprintf(stderr, "PRI = (%s)\n", PRI);

    pos = ++Count ;
    priority = atoi (PRI);
    return priority;
}

static int SyslogUDPReceiving( void * arg )
{
    int         sock            ;
    unsigned    totalbytes      ;
    const int   so_reuseaddr = 1;
    unsigned char fBuffer[4096] = {0,};


    struct sockaddr_in serverAddr   ;
    struct sockaddr_in clientAddr   ;
    socklen_t addrsize = sizeof clientAddr  ;


    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family       = AF_INET;
    serverAddr.sin_addr.s_addr  = inet_addr("0.0.0.0");
    serverAddr.sin_port         = htons(5140);

    sock = setupUDPSocket() ;
    if(sock == -1)  {
        fprintf(stderr,"socket failed \n");
        return 0;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
         (const char*)&so_reuseaddr, sizeof so_reuseaddr) < 0) {
        fprintf(stderr,"socketopt failed \n");
        close(sock) ;
        return 0;
    }

    if (bind(sock, (struct sockaddr*)&serverAddr, sizeof serverAddr) != 0) {
        fprintf(stderr,"socket bind \n");
        close(sock) ;
        return 0;
    }

    int priority ;
    int facility ;
    int severity ;
    int pos      ;

    while ( 1 )
    {
        pos = 0;
        memset( fBuffer   , 0, sizeof(fBuffer)   );
        memset(&clientAddr, 0, sizeof(clientAddr));

        int bytesRead = recvfrom( sock, fBuffer, sizeof fBuffer, 0, (struct sockaddr*)&clientAddr, &addrsize );
        if (bytesRead <= 0) {
            continue    ;
        }

        totalbytes = bytesRead ;
        fBuffer[bytesRead] = '\0';

        priority = getPriority(fBuffer, pos);
        if (priority > 0)
        {
            facility = priority / 8;
            severity = priority % 8;
        }   else
        {
            facility = -1  ;
            severity = -1  ;
        }

        //bytesRead -= (pos - 1);
        //fprintf(stderr,"%s %s (%s.%s) %s\n" , dateTime(), inet_ntoa(clientAddr.sin_addr), getFacility(facility), getSeverity(severity), fBuffer);
        //fprintf(stderr,"client port    = %d \n" , ntohs(clientAddr.sin_port)    );
        fprintf(stderr,"%s : %d bytes\n", &fBuffer[pos], bytesRead-pos-1);

        //Jul 25 17:32:45

        for ( int i = pos; i < bytesRead; i += 16 )
        {
            fprintf(stderr,"Line (%04X) : ", i);
            for (int j = i; j < (i+16); j++ )
            {
                if (fBuffer[j] == 0x00)
                    fprintf(stderr, "   ", (unsigned char)fBuffer[j]);
                else
                    fprintf(stderr, "%02X ", (unsigned char)fBuffer[j]);
            }
            fprintf(stderr,": ");
            for (int j = i; j < (i+16); j++ )
            {
                if (fBuffer[j] == 0x00) break;
                fprintf(stderr, "%c", (unsigned char)fBuffer[j]);
            }
            fprintf(stderr,"\n");
        }
/*
        char tmpStr[4096] = {0,};
        int  lpos = 0;
        for ( int i = 0; i < bytesRead; i ++ )
        {
            if (fBuffer[i] == '\t' or fBuffer[i] == '\0')
            {
                fprintf(stderr, "tmpStr = (%s)\n", tmpStr);
                memset(tmpStr, 0, sizeof(tmpStr));
                lpos = 0 ;
            }   else
            {
                tmpStr[lpos] = fBuffer[i];
                lpos ++ ;
            }
        }
        fprintf(stderr, "tmpStr = (%s)\n", tmpStr);
*/
    }

    close(sock) ;
    return 0;
}

void TrimString(char * szName)
{
    char    * pszBuf;
    char    fisStart;
    int     i, j, iLen;

    iLen = (int)strlen(szName);
    pszBuf = new char[iLen + 1];
    if(pszBuf == NULL)
    {
        std::cerr << "[ERROR]CParseConfig::TrimString new Error" << std::endl;
        return;
    }

    memset(pszBuf, 0, iLen + 1);

    //앞공백제거
    for(fisStart = 1, i = 0, j = 0; i < iLen; i++)
    {
        if(fisStart == 1 && (szName[i] == ' ' || szName[i] == '\t' ))
        {
            continue;
        }
        else
        {
            fisStart = 0;
        }

        pszBuf[j++] = szName[i];
    }

    sprintf(szName, "%s", pszBuf);
    delete []pszBuf;

    //뒤공백제거
    iLen = (int)strlen( szName);
    for(fisStart = 1, i = iLen - 1; i >= 0; i--)
    {
        if(fisStart == 1 && ( szName[i] == ' ' || szName[i] == '\t'))
        {
            szName[i] = '\0';
        }
        else
        {
            break;
        }
    }

    return ;
}

#define _MAXKEY_ 5

typedef struct  _multiKey_
{
    int32_t     Count   ;
    char        StrKey[_MAXKEY_][512];
}   MultiKey;

static inline const void setStrKey ( MultiKey * pKey, char const * keystr )
{
    int     keysize = 0 ;
    int     iPos    = 0 ;
    int     iCount  = 0 ;
    int     iKey    = 0 ;

    memset(pKey, 0, sizeof(MultiKey));
    if (keystr != NULL)
    {
        keysize = strlen(keystr);
        while (iPos < keysize)
        {
            if (keystr[iPos] == ' ' && keystr[iPos+1] == '+' && keystr[iPos+2] == ' ')
            {
                iPos   += 3  ;
                iCount ++    ;
                iKey   = 0   ;
                pKey->Count++;

                TrimString (pKey->StrKey[iCount-1]);
                if (iCount >= _MAXKEY_) break;
            }   else
            {
                pKey->StrKey[iCount][iKey++] = keystr[iPos++];
            }
        }
        if (iCount < _MAXKEY_)
        {
            pKey->Count++;
            TrimString (pKey->StrKey[iCount]);
        }
    }
    return ;
}

//bool boost_regex_match ( char const * mBuf, int mLen, char const * keyword )
//{
//    if (keyword == NULL) return false;
//    try
//    {
//        string s = "The sshd[3432]: A1234A cat saw the other cats playing in the back yard"; ;
//        boost::regex  p("sshd\[[0-9]+\\]:");
//        boost::smatch what;
//
//        int ret = boost::regex_search(s, what, p);
//
//        printf("OK Bingo (%d)!!!\n", ret );
//        return true ;
//
//    }
//    catch (boost::regex_error& e)
//    {
//        throw(e);
//        return false;
//    }
//}

typedef struct  _udpMSG_
{
    int32_t     len;
    char        msg     [4096];
}   UdpMSG  ;


typedef struct  _msgParse_
{
    int8_t      facility;
    int8_t      severity;
    char        timestamp[TMP_BUF];
    char        hostname [MIN_BUF];
    char        messages [4096];
}   MDivide;


static inline const bool messageParser ( MDivide * pDiv, UdpMSG * pmsg )
{
    int pos = 0;
    gettime (pDiv->timestamp, 'L');
    int16_t priority = getPriority((unsigned char*)pmsg->msg, pos);
    if (priority > 0)
    {
        pDiv->facility = priority / 8;
        pDiv->severity = priority % 8;
    }   else
    {
        pDiv->facility = -1;
        pDiv->severity = -1;
    }
    pos += 15;  //Aug  5 16:50:01
    while (pmsg->msg[pos] == ' ') ++pos;

    //  hostname
    int Cnt = 0;
    while ( pos < pmsg->len )
    {
        if (pmsg->msg[pos] == ' ') break;
        pDiv->hostname[Cnt] = pmsg->msg[pos];
        Cnt ++ ;
        pos ++ ;
    }
    pDiv->hostname[Cnt] = '\0';
    while (pmsg->msg[pos] == ' ') ++pos;

    //  messages
    Cnt = 0;
    while ( pos < pmsg->len )
    {
        if (pmsg->msg[pos] == '\0') break;
        pDiv->messages[Cnt] = pmsg->msg[pos];
        Cnt ++ ;
        pos ++ ;
    }
    pDiv->messages[Cnt] = '\0';
    return true;
}

//struct msgbuf
//{
//    long msgtype;
//    char mtext[256];
//    char myname[16];
//    int  seq;
//};

//key_t mq_Init()
//{
//    key_t key_id;
//    //int i;
//    //struct msgbuf mybuf, rcvbuf;
//
//    key_id = msgget((key_t)1234, IPC_CREAT|0666);
//    if (key_id == -1)
//    {
//        perror("msgget error : ");
//        exit(0);
//    }
//
//    printf("Key is %d\n", key_id);
//    return key_id ;
//}
//
//void mq_sendMsg(key_t key)
//{
//    //int i;
//    struct msgbuf mybuf, rcvbuf;
//
//    // 메시지를 전송한다.
//    if (msgsnd( key, (void *)&mybuf, sizeof(struct msgbuf), IPC_NOWAIT) == -1)
//    {
//        perror("msgsnd error : ");
//        exit(0);
//    }
//    printf("Key is %d\n", key_id);
//}


typedef struct  _mParse_
{
    int16_t      priority ;
    //string        priority ;
    string        timestamp;
    string        hostname ;
    string        program  ;
    string        messages ;
}   MPcre;

typedef struct  _Fire_
{
    string       Str1;
    string       Str2;
    string       Str3;
    string       Str4;
    string       Str5;
    string       Str6;
    string       Str7;
    string       Str8;
    string       Str9;
    string       Str10;
    string       Str11;
    string       Str12;
    string       Str13;
    string       Str14;
    string       Str15;
    string       Str16;
    string       Str17;
    string       Str18;
    string       Str19;
    string       Str20;
    string       Str21;
    string       Str22;
    string       Str23;
    string       Str24;
}   FireWall;


bool pcre_regex_match ( char const * mBuf, int mLen, char const * keyword )
{
    if (keyword == NULL) return false;
    try
    {
        pcrecpp::RE  re(keyword);
        int ret = re.PartialMatch(mBuf) ;

        printf("OK Bingo (%d)!!!\n", ret );
        return true ;

    }
    catch (...)
    {
        return false;
    }
}


#define SYSLOG_FORMAT = "<(\\d+)>(\\w+\\s+\\d+\\s+\\d+:\\d+:\\d+)\\s+(\\S+)\\s+(\\S*:?)\\s+(.*$)";

bool pcre_regex_parsing (MPcre * pDiv, const char * pBuf)
{
    if (pBuf == NULL) return false;
    try
    {
        pcrecpp::RE re("<(\\d+)>(\\w+\\s+\\d+\\s+\\d+:\\d+:\\d+)\\s+(\\S+)\\s+(\\S*:?)\\s+(.*$)");
        if (re.FullMatch(pBuf, &pDiv->priority, &pDiv->timestamp, &pDiv->hostname, &pDiv->program, &pDiv->messages))
        {
            fprintf(stderr, "priority  = (%d)\n", pDiv->priority);
            fprintf(stderr, "timestamp = (%s)\n", pDiv->timestamp.c_str());
            fprintf(stderr, "hostname  = (%s)\n", pDiv->hostname.c_str());
            fprintf(stderr, "program   = (%s)\n", pDiv->program.c_str());
            fprintf(stderr, "messages  = (%s)\n", pDiv->messages.c_str());

            return true ;
        }   else
        {
            fprintf(stderr, "parsing failed (%s)\n", pBuf);
            return false;
        }
    }
    catch (...)
    {
        fprintf(stderr, "exception error \n");
        return false;
    }
}

bool pcre_regex_partial_parsing (MPcre * pDiv, const char * pBuf)
{
    if (pBuf == NULL) return false;
    try
    {
        pcrecpp::RE re("<\\d+>\\w+\\s+\\d+\\s+\\d+:\\d+:\\d+\\s+\\S+\\s+([\\w/]+)");
        //pcrecpp::RE re("<\\d+>\\w+\\s+\\d+\\s+\\d+:\\d+:\\d+\\s+\\S+\\s+/?(\\w+)");
        //pcrecpp::RE re("<\\d+>\\w+\\s+\\d+\\s+\\d+:\\d+:\\d+\\s+\\S+\\s+/?\\w*/?\\w*/?(\\w+)");
        //pcrecpp::RE re("<\\d+>\\w+\\s+\\d+\\s+\\d+:\\d+:\\d+\\s+\\S+\\s+(/?\\w*/?\\w*/?(\\w+)|\\w*/?(\\w+))");
        if (re.PartialMatch(pBuf, &pDiv->program))
        {
            fprintf(stderr, "message   = (%s)\n", pBuf);
            fprintf(stderr, "program   = (%s)\n", pDiv->program.c_str());
            return true ;
        }   else
        {
            fprintf(stderr, "parsing failed (%s)\n", pBuf);
            return false;
        }
    }
    catch (...)
    {
        fprintf(stderr, "exception error \n");
        return false;
    }
}


bool pcre_regex_firewall_parsing (FireWall * pDiv, const char * pHead, const char * pMsgs)
{
//    char * pHead = "2014-09-12T01:00:04.014489+09:00 pankyo-fw GABIA_LG: NetScreen device_id=GABIA_LG  [Root]system-notification-00257(traffic):";
//    char * pMsgs = "start_time=\"2014-09-12 01:00:04\" duration=0 policy_id=15 service=udp/port:17591 proto=17 src zone=Untrust dst zone=Trust action=Deny sent=0 rcvd=46 src=88.231.116.105 dst=61.35.204.111 src_port=53 dst_port=17591 session_id=0 reason=Traffic Denied";

    if (pHead == NULL) return false;
    try
    {
        pcrecpp::RE re("([^ ]*) ([^ ]*) ([\\w_/.-]*)(?:\\[(\\d+)\\])?[^:]*: ([^ ]*) ([^ ]*) ([^:]*) ([^ ]*)");
        //if (re.FullMatch(pHead, &pDiv->Str1, &pDiv->Str2, &pDiv->Str3, &pDiv->Str4, &pDiv->Str5, &pDiv->Str6, &pDiv->Str7, &pDiv->Str8))
        if (re.PartialMatch(pHead, &pDiv->Str1, &pDiv->Str2, &pDiv->Str3, &pDiv->Str4, &pDiv->Str5, &pDiv->Str6, &pDiv->Str7, &pDiv->Str8))
        {
            fprintf(stderr, "Str1  = (%s)\n", pDiv->Str1.c_str());
            fprintf(stderr, "Str2  = (%s)\n", pDiv->Str2.c_str());
            fprintf(stderr, "Str3  = (%s)\n", pDiv->Str3.c_str());
            fprintf(stderr, "Str4  = (%s)\n", pDiv->Str4.c_str());
            fprintf(stderr, "Str5  = (%s)\n", pDiv->Str5.c_str());
            fprintf(stderr, "Str6  = (%s)\n", pDiv->Str6.c_str());
            fprintf(stderr, "Str7  = (%s)\n", pDiv->Str7.c_str());
            fprintf(stderr, "Str8  = (%s)\n", pDiv->Str8.c_str());
        }   else
        {
            fprintf(stderr, "parsing failed (%s)\n", pHead);
            return false;
        }

        pcrecpp::RE re2("start_time=\"([^ ]*\\s+[^ ]*)\" duration=(\\d+) policy_id=(\\d+) service=(\\S+) proto=(\\d+) src zone=(\\w+) dst zone=(\\w+) action=(\\w+) sent=(\\d+) rcvd=(\\d+) src=([\\d.]+) dst=([\\d.]+) src_port=(\\d+) dst_port=(\\d+) session_id=(\\d+) reason=(.*$)");
        //if (re2.FullMatch(pMsgs, &pDiv->Str9, &pDiv->Str10, &pDiv->Str11, &pDiv->Str12, &pDiv->Str13, &pDiv->Str14,
        //                         &pDiv->Str15, &pDiv->Str16, &pDiv->Str17, &pDiv->Str18, &pDiv->Str19, &pDiv->Str20,
        //                         &pDiv->Str21, &pDiv->Str22, &pDiv->Str23, &pDiv->Str24))
        if (re2.PartialMatch(pHead, &pDiv->Str9,  &pDiv->Str10, &pDiv->Str11, &pDiv->Str12, &pDiv->Str13, &pDiv->Str14,
                                    &pDiv->Str15, &pDiv->Str16, &pDiv->Str17, &pDiv->Str18, &pDiv->Str19, &pDiv->Str20,
                                    &pDiv->Str21, &pDiv->Str22, &pDiv->Str23, &pDiv->Str24))
        {

            fprintf(stderr, "Str9  = (%s)\n", pDiv->Str9.c_str());
            fprintf(stderr, "Str10 = (%s)\n", pDiv->Str10.c_str());
            fprintf(stderr, "Str11 = (%s)\n", pDiv->Str11.c_str());
            fprintf(stderr, "Str12 = (%s)\n", pDiv->Str12.c_str());
            fprintf(stderr, "Str13 = (%s)\n", pDiv->Str13.c_str());
            fprintf(stderr, "Str14 = (%s)\n", pDiv->Str14.c_str());
            fprintf(stderr, "Str15 = (%s)\n", pDiv->Str15.c_str());
            fprintf(stderr, "Str16 = (%s)\n", pDiv->Str16.c_str());
            fprintf(stderr, "Str17 = (%s)\n", pDiv->Str17.c_str());
            fprintf(stderr, "Str18 = (%s)\n", pDiv->Str18.c_str());
            fprintf(stderr, "Str19 = (%s)\n", pDiv->Str19.c_str());
            fprintf(stderr, "Str20 = (%s)\n", pDiv->Str20.c_str());
            fprintf(stderr, "Str21 = (%s)\n", pDiv->Str21.c_str());
            fprintf(stderr, "Str22 = (%s)\n", pDiv->Str22.c_str());
            fprintf(stderr, "Str23 = (%s)\n", pDiv->Str23.c_str());
            fprintf(stderr, "Str24 = (%s)\n", pDiv->Str24.c_str());

        }   else
        {
            fprintf(stderr, "parsing failed (%s)\n", pHead);
            return false;
        }

        return true ;
    }
    catch (...)
    {
        fprintf(stderr, "exception error \n");
        return false;
    }
}

static char const * pcre_regex_extract (char * pBuf)
{
    string buf;
    if (pBuf == NULL) return false;
    try
    {
        pcrecpp::RE re("(\\w+).*$");
        if (re.FullMatch(pBuf, &buf))
        {
            return buf.c_str();
        }   else
        {
            return NULL;
        }
    }
    catch (...)
    {
        return NULL;
    }
}

static char const * pcre_regex_replace (char * pBuf)
{
    string Buf = pBuf;
    pcrecpp::RE("(\\W)").GlobalReplace(".", &Buf);
    return Buf.c_str();
}

bool pcre_regex_weblog_parsing (FireWall * pDiv, const char * pAccesslog, const char * pErrorlog)
{
    if (pAccesslog == NULL) return false;
    if (pErrorlog == NULL) return false;
    try
    {
        //(?: ([^ \"]*|\"[^\"]*\") ([^ \"]*|\"[^\"]*\"))?
        //(-|\\[?[^\\]]*\\]?)
        //(?:(-)|\\[([^\\]]*)\\]))?
        //\\[([^ ]*)\\]
        //\\[([^ ]*)\\]
        //\\[([^\\]]*)
        //\\[(\\S*)\\]
        //(?:-|\\[([^\\]]*)\\])?
        //(?:[^ \"]*|\"([^\"]*)\")?
        //(?:[^ \"]*|\"([^\"]*)\")?
        pcrecpp::RE re("([^ ]*) ([^ ]*) ([^ ]*) (?:-|\\[([^\\]]*)\\])? (?:[^ \"]*|\"([^\"]*)\")? (-|\\d*) (-|\\d*) (?:([^ \"]*|\"[^\"]*\") ([^ \"]*|\"[^\"]*\"))?");
        if (re.FullMatch(pAccesslog, &pDiv->Str1, &pDiv->Str2, &pDiv->Str3, &pDiv->Str4, &pDiv->Str5, &pDiv->Str6, &pDiv->Str7, &pDiv->Str8, &pDiv->Str9))
        {
            fprintf(stderr, "Str1  = (%s)\n", pDiv->Str1.c_str());
            fprintf(stderr, "Str2  = (%s)\n", pDiv->Str2.c_str());
            fprintf(stderr, "Str3  = (%s)\n", pDiv->Str3.c_str());
            fprintf(stderr, "Str4  = (%s)\n", pDiv->Str4.c_str());
            fprintf(stderr, "Str5  = (%s)\n", pDiv->Str5.c_str());
            fprintf(stderr, "Str6  = (%s)\n", pDiv->Str6.c_str());
            fprintf(stderr, "Str7  = (%s)\n", pDiv->Str7.c_str());
            fprintf(stderr, "Str8  = (%s)\n", pDiv->Str8.c_str());
            fprintf(stderr, "Str9  = (%s)\n", pDiv->Str9.c_str());
            fprintf(stderr, "Str10 = (%s)\n", pDiv->Str10.c_str());
            fprintf(stderr, "Str11 = (%s)\n", pDiv->Str11.c_str());
        }   else
        {
            fprintf(stderr, "parsing failed (%s)\n", pAccesslog);
            return false;
        }

        //pcrecpp::RE re2("(?:-|\\[([^\\]]*)\\])? (?:-|\\[([^\\]]*)\\])? (?:-|\\[([^\\]]*)\\])? ([\\w\\s]*\\:) (.*$)");
        pcrecpp::RE re2("(?:-|\\[([^\\]]*)\\])? (?:-|\\[([^\\]]*)\\])? (?:-|\\[(\\w*) ([\\d\\.]*)\\])? ([\\w\\s]*\\:) (.*$)");
        if (re2.FullMatch(pErrorlog, &pDiv->Str12, &pDiv->Str13, &pDiv->Str14, &pDiv->Str15, &pDiv->Str16, &pDiv->Str17))
        {
            fprintf(stderr, "Str12 = (%s)\n", pDiv->Str12.c_str());
            fprintf(stderr, "Str13 = (%s)\n", pDiv->Str13.c_str());
            fprintf(stderr, "Str14 = (%s)\n", pDiv->Str14.c_str());
            fprintf(stderr, "Str15 = (%s)\n", pDiv->Str15.c_str());
            fprintf(stderr, "Str16 = (%s)\n", pDiv->Str16.c_str());
            fprintf(stderr, "Str17 = (%s)\n", pDiv->Str17.c_str());
        }   else
        {
            fprintf(stderr, "parsing failed (%s)\n", pErrorlog);
            return false;
        }

        return true ;
    }
    catch (...)
    {
        fprintf(stderr, "exception error \n");
        return false;
    }
}

bool pcre_regex_mport (const char * pBuf)
{
    if (pBuf == NULL) return false;
    try
    {
        int syslogport  = 0;
        int weblogAport = 0;
        int weblogEport = 0;

        string str1, str2, str3;
        pcrecpp::RE re("(\\S*)\\s*(\\S*)\\s*(\\S*)$");
        if (re.FullMatch(pBuf, &str1, &str2, &str3))
        {
            fprintf(stderr, "(%s)\n", str1.c_str());
            fprintf(stderr, "(%s)\n", str2.c_str());
            fprintf(stderr, "(%s)\n", str3.c_str());
        }   else
        {
            fprintf(stderr, "\x1b[1;35m%s()[%d] parsing failed !!!(%s)\x1b[0m\n", __FUNCTION__, __LINE__, pBuf);
            return false;
        }

        int port  = 0;
        string name ;
        pcrecpp::RE re2("(\\w+)\\:(\\d+)");
        if (str1.length() > 0)
        {
            if (re2.FullMatch(str1.c_str(), &name, &port))
            {
                fprintf(stderr, "(%s):(%d)\n", name.c_str(), port );

                if (!memcmp(name.c_str(),"syslog" , 6)) syslogport  = port;
                if (!memcmp(name.c_str(),"weblogA", 7)) weblogAport = port;
                if (!memcmp(name.c_str(),"weblogE", 7)) weblogEport = port;

            }   else
            {
                fprintf(stderr, "\x1b[1;35m%s()[%d] parsing failed !!!(%s)\x1b[0m\n", __FUNCTION__, __LINE__, str1.c_str());
                return false;
            }
        }
        if (str2.length() > 0)
        {
            if (re2.FullMatch(str2.c_str(), &name, &port))
            {
                fprintf(stderr, "(%s):(%d)\n", name.c_str(), port );
                if (!memcmp(name.c_str(),"syslog" , 6)) syslogport  = port;
                if (!memcmp(name.c_str(),"weblogA", 7)) weblogAport = port;
                if (!memcmp(name.c_str(),"weblogE", 7)) weblogEport = port;
            }   else
            {
                fprintf(stderr, "\x1b[1;35m%s()[%d] parsing failed !!!(%s)\x1b[0m\n", __FUNCTION__, __LINE__, str2.c_str());
                return false;
            }
        }
        if (str3.length() > 0)
        {
            if (re2.FullMatch(str3.c_str(), &name, &port))
            {
                fprintf(stderr, "(%s):(%d)\n", name.c_str(), port );
                if (!memcmp(name.c_str(),"syslog" , 6)) syslogport  = port;
                if (!memcmp(name.c_str(),"weblogA", 7)) weblogAport = port;
                if (!memcmp(name.c_str(),"weblogE", 7)) weblogEport = port;
            }   else
            {
                fprintf(stderr, "\x1b[1;35m%s()[%d] parsing failed !!!(%s)\x1b[0m\n", __FUNCTION__, __LINE__, str3.c_str());
                return false;
            }
        }

        return true;
    }
    catch (...)
    {
        fprintf(stderr, "\x1b[1;35m%s()[%d] parsing failed !!!(%s)\x1b[0m\n", __FUNCTION__, __LINE__, pBuf);
        return false;
    }
}

#define _LIMIT_INT63_  ((unsigned long long)pow(2.0,63.0))
#define _LIMIT_INT60_  ((unsigned long long)pow(2.0,60.0))
#define _LIMIT_INT50_  ((unsigned long long)pow(2.0,50.0))
#define _LIMIT_INT40_  ((unsigned long long)pow(2.0,40.0))
#define _LIMIT_INT30_  ((unsigned long long)pow(2.0,30.0))
#define _LIMIT_INT20_  ((unsigned long long)pow(2.0,20.0))
#define _LIMIT_INT10_  ((unsigned long long)pow(2.0,10.0))

#define _LIMIT_DAY_    (60*60*24)
#define _LIMIT_HOUR_   (60*60)
#define _LIMIT_MINUTE_ (60)


static inline const char * Seconds2String (uint32_t lCounts)
{
    int32_t Seconds = lCounts ;
    static char tmpStr[FUL_FLE] = {0,};
    uint32_t day    = Seconds / _LIMIT_DAY_    ;
    if (Seconds >= _LIMIT_DAY_ ) Seconds = Seconds - (day  * _LIMIT_DAY_ );
    uint32_t hour   = Seconds / _LIMIT_HOUR_   ;
    if (Seconds >= _LIMIT_HOUR_) Seconds = Seconds - (hour * _LIMIT_HOUR_);
    uint32_t minute = Seconds / _LIMIT_MINUTE_ ;
    uint32_t second = Seconds % _LIMIT_MINUTE_ ;

    printf("%02d %02d:%02d:%02d\n", day, hour, minute, second);

    return tmpStr;
}

int TestSummury ()
{
    int      m_Max = 5;
    int      m_Pos = 0;
    int      m_Summury = 0;
    float    m_Average = 0.0;
    bool     m_Cir = false;
    uint32_t m_Value [5] = {0,};

    for (int i = 0; i < 10; i++ )
    {
        int pValue = i + 1;
        if (m_Max <= m_Pos) m_Pos = 0;

        m_Summury -= m_Value[m_Pos];
        m_Value[m_Pos++] = pValue;
        m_Summury += pValue;

        m_Average = (float)m_Summury / (float)m_Max;

        for (int i = 0; i < m_Max; i++)
            printf("m_Value[%d] = (%d)\n", i, m_Value[i]);
        printf("m_Summury = (%d)\n\n", m_Summury);
    }
}

typedef struct  _fluentd_
{
    string      timestamp;
    string      hostname ;
    string      ident;
    string      pid;
    string      message ;
    string      facility;
    string      severity;
}   FLUENTD;
#define SZ_FLUENTD sizeof(FLUENTD)


#define FLUENTD_FORMAT = ".*{\"host\":\"(\\s+)\"(\\W+)}";
bool pcre_regex_fluentd_parsing (FLUENTD * pDiv, const char * pBuf)
{
    if (pBuf == NULL) return false;
    try
    {
        //(?:-|\\[([^\\]]*)\\])?
        //pcrecpp::RE re("\"host\":\"(\\w+)\",\"ident\":\"(\\w+)\",\"pid\":\"(\\w+)\",\"message\":\"(.*)\",\"facility\":\"(\\w+)\",\"severity\":\"(\\w+)\"");
        pcrecpp::RE re("(\\S+)\\s+host:(\\S+)\\s+ident:(\\S+)(?:\\s+|\\s+pid:(\\w+))?\\s+message:(.*)\\s+facility:(\\w+)\\s+severity:(\\w+)");
        if (re.PartialMatch(pBuf, &pDiv->timestamp, &pDiv->hostname, &pDiv->ident, &pDiv->pid, &pDiv->message, &pDiv->facility, &pDiv->severity))
        {
            fprintf(stderr, "timestamp= (%s)\n", pDiv->timestamp.c_str());
            fprintf(stderr, "hostname = (%s)\n", pDiv->hostname.c_str());
            fprintf(stderr, "ident    = (%s)\n", pDiv->ident.c_str());
            fprintf(stderr, "pid      = (%s)\n", pDiv->pid.c_str());
            fprintf(stderr, "message  = (%s)\n", pDiv->message.c_str());
            fprintf(stderr, "facility = (%s)\n", pDiv->facility.c_str());
            fprintf(stderr, "severity = (%s)\n", pDiv->severity.c_str());

            return true ;
        }   else
        {
            fprintf(stderr, "parsing failed (%s)\n", pBuf);
            return false;
        }
    }
    catch (...)
    {
        fprintf(stderr, "exception error \n");
        return false;
    }
}

static char * Time2Date (unsigned pTimestamp)
{
    static  char    buf[32];
    struct  tm      stTime ;
    struct  timeval tv     ;
    /////////////////////////////////////////////////////////////////
    tv.tv_sec = pTimestamp ;
    localtime_r(&tv.tv_sec, &stTime);
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                               stTime.tm_year + 1900     ,
                               stTime.tm_mon  + 1        ,
                               stTime.tm_mday            ,
                               stTime.tm_hour            ,
                               stTime.tm_min             ,
                               stTime.tm_sec             );
    return buf;
    /////////////////////////////////////////////////////////////////
}

static char * Min2Date (unsigned pTimestamp)
{
    static  char    buf[10];
    struct  tm      stTime ;
    struct  timeval tv     ;
    /////////////////////////////////////////////////////////////////
    tv.tv_sec = pTimestamp ;
    localtime_r(&tv.tv_sec, &stTime);
    snprintf(buf,sizeof(buf),"%02d:%02d",stTime.tm_min, stTime.tm_sec);
    return buf;
    /////////////////////////////////////////////////////////////////
}

#define TimestampTiks 1000000
static inline const uint64_t GetTimeStamp(struct timeval *ptv) {
    return ((uint64_t)ptv->tv_sec * TimestampTiks) + ptv->tv_usec;
}

static const inline char * InterMilli (struct timeval *ptv)
{
    static  char buf[32]    ;
    struct  timeval tv      ;
    gettimeofday(&tv, NULL) ;
    uint64_t it = GetTimeStamp(&tv) - GetTimeStamp(ptv);
    snprintf(buf, sizeof(buf), "%u.%06u", (it/TimestampTiks), (it%TimestampTiks));
    return  buf;
}

static const inline char * InterCall ()
{
    struct timeval tv      ;
    gettimeofday(&tv, NULL);
    SDL_Delay(990)         ;
    return InterMilli(&tv) ;
}

static char const * getdate() {
    static char buf[16];
    time_t tt = time(NULL);
    strftime(buf, sizeof buf, "%Y.%m.%d", localtime(&tt));
    return buf;
}

void UpdateDayTableDescr(time_t val)
{
    char m_szDayDescr[100] = "";
    char buf[16] = {0,};
    struct tm lt;
    time_t cVal;

    if(!val)    cVal = time(NULL);
    else        cVal = val;
    memcpy(&lt, localtime(&cVal), sizeof(lt));
    strftime(buf, sizeof(buf), "%Y%m%d", &lt);

    if(strcmp(m_szDayDescr, buf) != 0){
        memset(m_szDayDescr, 0x00, sizeof(m_szDayDescr));
        strncpy(m_szDayDescr, buf, sizeof(m_szDayDescr));
    }
    printf("m_szDayDescr = %s\n", m_szDayDescr);
}

typedef struct
{
    char    year    [5];
    char    mon     [3];
    char    day     [3];
    char    hour    [3];
    char    min     [3];
    char    sec     [3];
}   Time_T;

static inline const uint32_t GetUnixTime_val(char * source)
{
    Time_T              tm_t    ;
    struct tm           cTm     ;
    struct timeval      cTime   ;
    memset(&cTm, 0, sizeof(cTm));

   // 받아온 시간을 파싱하여 time_t 구조체에 넣음
    memcpy(&tm_t.year, source   , 4);
    tm_t.year[4]= '\0';
    memcpy(&tm_t.mon , source+ 4, 2);
    tm_t.mon [2]= '\0';
    memcpy(&tm_t.day , source+ 6, 2);
    tm_t.day [2]= '\0';
    memcpy(&tm_t.hour, source+ 8, 2);
    tm_t.hour[2]= '\0';
    memcpy(&tm_t.min , source+10, 2);
    tm_t.min [2]= '\0';
    memcpy(&tm_t.sec , source+12, 2);
    tm_t.sec [2]= '\0';

    // time_t 값을 tm에 넣어서 time 구조체로 변경
    cTm.tm_year  = atoi(tm_t.year) - 1900;
    cTm.tm_mon   = atoi(tm_t.mon)  - 1  ;
    cTm.tm_mday  = atoi(tm_t.day)       ;
    cTm.tm_hour  = atoi(tm_t.hour)      ;
    cTm.tm_min   = atoi(tm_t.min)       ;
    cTm.tm_sec   = atoi(tm_t.sec)       ;
    cTime.tv_sec = (uint32_t)mktime(&cTm);

    return cTime.tv_sec;
}

enum {
    ERROR_ARGS = 1 ,
    ERROR_CURL_INIT = 2
} ;

enum {
    OPTION_FALSE = 0 ,
    OPTION_TRUE = 1
} ;


int curlExecute(char * pData)
{
    char * targetUrl = "http://localhost:4242/api/put";
    curl_global_init(CURL_GLOBAL_ALL);
    CURL * curl = curl_easy_init();
    if(NULL == curl)
    {
        std::cerr << "Unable to initialize cURL interface" << std::endl;
        return(ERROR_CURL_INIT);
    }

    char postData[512] ;
    snprintf(postData, sizeof(postData), "[{\"metric\":\"sys.cpu.nice\",\"timestamp\":%s,\"value\":43,\"tags\":{\"host\":\"web01\",\"dc\":\"lga\"}}," \
                                          "{\"metric\":\"sys.cpu.nice\",\"timestamp\":%s,\"value\":30,\"tags\":{\"host\":\"web02\",\"dc\":\"lga\"}}," \
                                          "{\"metric\":\"sys.cpu.nice\",\"timestamp\":%s,\"value\":34,\"tags\":{\"host\":\"web03\",\"dc\":\"lga\"}}]", getTimestamps(), getTimestamps(), getTimestamps());

    printf("result = %s\n", postData);
    if(curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, targetUrl);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);
        curl_easy_setopt(curl, CURLOPT_POST, OPTION_TRUE);

        CURLcode res = curl_easy_perform(curl);
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return 0;
}

void testMsg()
{
    char m_szBuf[10240] = "";
    char sBuf[256];

    strncat(m_szBuf, "[", 1);
    int nUpdCnt = 0;

    for(int i = 0; i < 20; i++ )
    {
        nUpdCnt+=4;

        if(nUpdCnt == 4)
            snprintf(sBuf, sizeof(sBuf),  "{\"metric\":\"ip.inbps\",\"timestamp\":%u,\"value\":%u,\"tags\":{\"ip\":\"%u\"}}", i+1, i+2, i+3, i+4);
        else
            snprintf(sBuf, sizeof(sBuf), ",{\"metric\":\"ip.inbps\",\"timestamp\":%u,\"value\":%u,\"tags\":{\"ip\":\"%u\"}}", i+1, i+2, i+3, i+4);
        strncat(m_szBuf, sBuf, strlen(sBuf));

        snprintf(sBuf, sizeof(sBuf), ",{\"metric\":\"ip.outbps\",\"timestamp\":%u,\"value\":%u,\"tags\":{\"ip\":\"%u\"}}", i+1, i+2, i+3, i+4);
        strncat(m_szBuf, sBuf, strlen(sBuf));

        snprintf(sBuf, sizeof(sBuf), ",{\"metric\":\"ip.inpps\",\"timestamp\":%u,\"value\":%u,\"tags\":{\"ip\":\"%u\"}}" , i+1, i+2, i+3, i+4);
        strncat(m_szBuf, sBuf, strlen(sBuf));

        snprintf(sBuf, sizeof(sBuf), ",{\"metric\":\"ip.outpps\",\"timestamp\":%u,\"value\":%u,\"tags\":{\"ip\":\"%u\"}}", i+1, i+2, i+3, i+4);
        strncat(m_szBuf, sBuf, strlen(sBuf));
    }
    strncat(m_szBuf, "]", 1);
    printf("msg = %s\n", m_szBuf);

}

static const inline uint32_t GetRowKey (uint32_t uNow)
{
    return (uNow - (uNow%600));
}

#define _CF_HEADER_SZ_  3
static const inline char * getCFHeader()
{
    static char str[_CF_HEADER_SZ_];
    for(int i = 0; i < _CF_HEADER_SZ_; i++ )
        str[i] = 'A' + random(26);
    str[_CF_HEADER_SZ_-1]='\0';
    return str ;
}

void _PrintTSDBLine(char * pStr, uint32_t jobcount, uint64_t jobtime)
{
    char  buf[32];
    snprintf(buf, sizeof(buf), "%2u.%06u", (unsigned)(jobtime/TimestampTiks),(unsigned)(jobtime%TimestampTiks));
    float time = ((float)jobtime/(float)TimestampTiks);
    printf("time : %f(qty)\n", time);
    float avg  = (time > 0 ? (jobcount/time) : 0);
    printf("(%s) Total : %7u(qty) Time : %s(sec) Avg : %5.02f(qps)\n", pStr, jobcount, buf, avg);
}

static const inline char * Num2Addr (unsigned nAddr)
{
    static  char  buf[32];
    struct in_addr in;
    in.s_addr = nAddr;
    snprintf(buf, sizeof(buf), "%s", inet_ntoa(in));
    return buf;
}

static const inline uint32_t Addr2Num (const char * cAddr)
{
    return inet_addr(cAddr);
}


#define LVL_BUF         8
typedef struct  _econd_
{
    uint8_t     Severity;
    uint16_t    Xnc     ;
    uint16_t    Max     ;
    uint16_t    Mail    ;
    uint16_t    SMS     ;
}   ECOND;

typedef struct  _condition_
{
    uint8_t     eCount  ;
    ECOND       eCond   [LVL_BUF];
}   COND;


static const bool pcre_regex_condition (ECOND * pCond, const char * pBuf, uint8_t plNo)
{
    if (pBuf == NULL) return false;
    try
    {
        pcrecpp::RE re("(\\d+):(\\d+):(\\d?):(\\d?)");
        if (re.PartialMatch(pBuf, &pCond->Xnc, &pCond->Max, &pCond->Mail, &pCond->SMS))
        {
            pCond->Severity = plNo;
            return true ;
        }   else
        {
            memset(pCond, 0, sizeof(ECOND));
            pCond->Severity = plNo;
            return false;
        }
    }
    catch (...)
    {
        return false;
    }
}

typedef struct  _msgbuf_
{
    long        mtype   ;
    uint16_t    len     ;
    char        msg     [2048];
}   MsgBuf ;

pcrecpp::RE msg("\\{\"message\":\"(.*)\"\\}\n");
//pcrecpp::RE msg("\\{\"message\":\"(.*)\n");
static void pcre_regex_message (MsgBuf * pMsg)
{
    static string tbuf;
    if (pMsg == NULL) return;
    try
    {
        if (msg.FullMatch(pMsg->msg, &tbuf))
        {
            pMsg->len = tbuf.length()    ;
            memcpy(pMsg->msg, tbuf.c_str(), pMsg->len);
            pMsg->msg[pMsg->len++] = '\0';
        }
    }
    catch (...)
    {
        printf("\x1b[1;35m%s()[%d] parsing failed !!!(%s)\x1b[0m\n", __FUNCTION__, __LINE__, pMsg->msg);
    }
    return;
}


pcrecpp::RE kvl("(\\w+)=(\\w+);");
static void getKeyValue(const char * pBuf, const char * key, char * value)
{
    if (pBuf == NULL) return;
    try
    {
        string skey;
        string sval;
        pcrecpp::StringPiece input(pBuf);
        while (kvl.Consume(&input, &skey, &sval))
        {
            if (!memcmp(key, skey.c_str(), strlen(key)))
            {
                snprintf(value, sval.size()+1, "%s", sval.c_str());
                //printf("key = [%s]\n", skey.c_str());
                //printf("val = [%s]\n", sval.c_str());
                return;
            }
        }
    }
    catch (...)
    {
        printf("\x1b[1;35m%s()[%d] parsing failed !!!(%s)\x1b[0m\n", __FUNCTION__, __LINE__, pBuf);
    }
    return;
}


static string getKeyValue2(const char * pBuf, const char * pkey, const char * defvalue)
{
    string ret(defvalue);
    if (pBuf == NULL) return ret;
    try
    {
        string value;
        char   mkey[32];
        snprintf(mkey, sizeof(mkey), "%s=(\\w+)", pkey);
        pcrecpp::RE kvl(mkey);
        if (kvl.PartialMatch(pBuf, &value))
            return value;
        else
            return ret;
    }
    catch (...)
    {
        printf("\x1b[1;35m%s()[%d] parsing failed !!!(%s)\x1b[0m\n", __FUNCTION__, __LINE__, pBuf);
    }
    return ret;
}

#define _TIME2WAIT_     980
#define _SLEEP_WAIT_    250                                 // millisecondes
#define _RECV_WAIT_    (uint64_t)(_TIME2WAIT_ * 3.0)       // 60 seconds

void waitCall(uint32_t mseconds, bool * IsStop)
{
    uint64_t TickCount = 0;
    while(TickCount <= mseconds)
    {
        SDL_Delay(_SLEEP_WAIT_)  ;
        TickCount += _SLEEP_WAIT_;
        if (!*IsStop) return;
    }
}

static inline const uint16_t HexToU16( char * buff, int Len )
{
    uint16_t value = 0;
    for (int i = 0; i < Len; i++)
        value += ((uint8_t)buff[i] << (8 * (Len - i - 1)));
    return value;
}

//typedef struct  _hmsg_
//{
//    uint16_t    length  ;           //  Functon Code(1) + Payload(N) = 1 + N 사용, BigEndian
//}   HMSG;

typedef enum {
    U_IN_VOL_           = 0,
    U_IN_FREQ_          = 1,
    U_OUT_VOL_          = 2,
    U_OUT_CUR_          = 3,
    U_BAT_VOL_          = 4,
    U_BAT_TMP_          = 5,
    ///////////////////////
    B_CHG_VOL_          = 6,
    B_CHG_CUR_          = 7,
    B_TEMP              = 8
}   LMT_CODE;


#define _DELAY_WAIT_    100  // 10
#define _TIME2SLEEP1_   950 // 860
#define _WAIT_COUNT_   (uint64_t)(_TIME2SLEEP1_ * 5.0)        //    5 seconds


#define APP_MBUS_EVT_UPS_INVOL_MAX		(0x0001)
#define APP_MBUS_EVT_UPS_INVOL_MIN		(0x0002)
#define APP_MBUS_EVT_UPS_INFREQ_MAX	    (0x0004)
#define APP_MBUS_EVT_UPS_INFREQ_MIN	    (0x0008)
#define APP_MBUS_EVT_UPS_OUTVOL_MAX	    (0x0010)
#define APP_MBUS_EVT_UPS_OUTVOL_MIN	    (0x0020)
#define APP_MBUS_EVT_UPS_OUTCUR_MAX	    (0x0040)
#define APP_MBUS_EVT_UPS_OUTCUR_MIN	    (0x0080)
#define APP_MBUS_EVT_UPS_BATVOL_MAX	    (0x0100)
#define APP_MBUS_EVT_UPS_BATVOL_MIN	    (0x0200)
#define APP_MBUS_EVT_UPS_BATTEMP_MAX	(0x0400)
#define APP_MBUS_EVT_UPS_BATTEMP_MIN	(0x0800)


static inline const uint32_t HexToBin(char ch)
{
	uint32_t value;
	
	if ((ch >= '0') && (ch <= '9')) {
		value = ch - '0';
	}
	else if ((ch >= 'A') && (ch <= 'F')) {
		value = ch - 'A' + 10;
	}
	else {
		value = ch - 'a' + 10;
	}

	return (value);
}

static inline const void HexStringToBin(char *buf, int size, char *out)
{
	int i, j;
	uint8_t value;

	for (i = 0; i < size/2; i++) {
	    value = 0;
		for (j = 0; j < 2; j++) {
		    
		    //value = (HexToBin(buf[i*2+j])<<(j*4)) | value;
			value <<= (j*4);
			value |= HexToBin(buf[i*2+j]);
		}
		out[i] = value;
	}
}


typedef struct  _lora_
{
    uint16_t    UpsInVol;           /* UPS Input Voltage        */
    uint16_t    UpsInFreq;          /* UPS Input Frequency      */
    uint16_t    UpsOutVol;          /* UPS Output Voltage       */
    uint16_t    UpsOutCur;          /* UPS Output Current       */
    uint16_t    UpsBatVol;          /* UPS Battery Voltage      */
    uint16_t    UpsBatTemp;         /* UPS Battery Temperature  */
    uint16_t    UpsStatus;          /* UPS Status               */

    uint16_t    BatVol;             /* Battery Voltage [V]      */
    uint16_t    BatCur;             /* Battery Current [mA]     */
    uint16_t    BatEnvTemp;         /* Battery Environment Temperature [0.1C]   */
    uint16_t    BatEnvHumi;         /* Battery Environment Humidity [%]         */
    uint16_t    BatTemp;            /* Battery Max Temperatures [0.1C]    */
    uint16_t    BatStatus;          /* Battery Status               */

    uint16_t    EventBits;          /* Event Bits                   */
}   LORA;
#define SZ_LORA  sizeof(LORA)
#define SZ_LORA2 (SZ_LORA*4)

typedef struct  _getlora_
{
    char        loraserial  [24];
    char        loratime    [26];
    char        refreshtime [25];
    char        Data        [SZ_LORA2];
}   GETLORA;
#define SZ_GETLORA sizeof(GETLORA)


static inline const time_t ToTimestamp(const char * pStr)
{
    struct tm timeDate;
    memset(&timeDate, 0, sizeof(struct tm));
    strptime(pStr,"%Y-%m-%d %H:%M:%S", &timeDate);
    return mktime(&timeDate);
}

static inline void InitSleep(uint32_t seqno, uint16_t seconds)
{
    struct  timeval tv          ;
    gettimeofday(&tv, NULL)     ;
    int8_t T = (tv.tv_sec % seconds)  ;
    int8_t S = (seqno % seconds);
    if (T > S)
    {
        printf("%u - %d + %d = %d\n", seconds, T, S, (seconds-T+S));
    }   else
    {
        printf("%d - %d = %d\n", S, T, (S-T));
    }
    
    return ;
}


static inline const void Utf8ToEuckr(char * in_str, int insize, char * outstr, int outsize)
{
    size_t in_size  = insize ;
    size_t out_size = outsize;
    iconv_t it  = iconv_open("EUC-KR", "UTF-8");
    int16_t ret = iconv(it, &in_str, &in_size, &outstr, &out_size);
    iconv_close(it);
    if (ret < 0)
        snprintf(outstr, outsize, "%s", in_str);
}

static inline const void EuckrToUtf8(char * in_str, int insize, char * outstr, int outsize)
{
    size_t in_size  = insize ;
    size_t out_size = outsize;
    iconv_t it  = iconv_open("UTF-8","EUC-KR");
    int16_t ret = iconv(it, &in_str, &in_size, &outstr, &out_size);
    iconv_close(it);
    if (ret < 0)
        snprintf(outstr, outsize, "%s", in_str);
}


//static inline const void genQuery()
//{
//
//           insert into TB_DEV_DATA_CUR (ID, CREATED, UpsInVol, UpsInFreq, UpsOutVol, UpsOutCur)
//                                 value ('%010u', now(), 1, 2, 3, 4) 
//                                     on duplicate key update
//                                        CREATED   = values(CREATED  ),
//                                        UpsInVol  = values(UpsInVol ),
//                                        UpsInFreq = values(UpsInFreq), 
//                                        UpsOutVol = values(UpsOutVol),
//                                        UpsOutCur = values(UpsOutCur);
//
//
//}


static inline const uint8_t snmpGet(uint32_t * pl)
{
    FILE  * fp = NULL;
    char    pBuff[128] = {0,};
    char    pCmds[128] = {0,};

    //snprintf(pCmds, sizeof(pCmds), "snmpget -v 2c -c public -t 1 -r 0 172.17.130.8 .1.3.6.1.4.1.935.1.1.1.3.2.1.0 | awk -F' ' '{print $4}'");
    snprintf(pCmds, sizeof(pCmds), "df | awk -F' ' '{print $2}'");
    // excute command
    //printf("pCmds = (%s)\n", pCmds);
    fp = popen(pCmds, "r");
    if(!fp)
    {
        printf("\x1b[1;34m%s() popen error [%d:%s] (pCmds = %s)\x1b[0m\n", __FUNCTION__, errno, strerror(errno), pCmds);
        return 0;
    }

    int icnt = 0;
    while(fgets(pBuff, 128, fp) != NULL)
    {
        //printf("icnt = %d, %s", icnt, pBuff);
        pl[icnt] = atoi(pBuff);
        icnt ++;
    }
    pclose(fp);
    
    if (icnt == 0)
        return 0;
    else
        return 1;
}


//<전체 파일오픈갯수 확인>
//sysctl -a|grep file-max
//
//<전체 파일오픈갯수 확인>
//sysctl -a|grep file-nr
//
//<전체 파일오픈갯수 확인>
//sysctl -a|grep nr_open
//
//<sae000> 계정으로 열린갯수 확인하기
//lsof -u sae000 | wc -l
// 
//
//ulimit -n
//
//root 에서 수정한다음에 fmsd 재컴파일..
//cat /usr/include/bits/typesizes.h
//vi /usr/include/bits/typesizes.h
//#define __FD_SETSIZE        4096
//
//
//cat /etc/security/limits.conf
//vi /etc/security/limits.conf
//
//sae000 soft nofile 20000
//sae000 hard nofile 20000
//sae000 soft stack  256
//sae000 hard stack  256
//
//
//> sysctl -a|grep somaxconn
//net.core.somaxconn = 5000
//> sysctl -a|grep backlog
//net.ipv4.tcp_max_syn_backlog = 10000
//net.core.netdev_max_backlog = 30000
//> sysctl -a|grep port_range
//net.ipv4.ip_local_port_range = 10240	65535
//
//
//sae00 계정에 root 권한 주기 (3개 모두)
//
//vi /etc/sudoers (sudo 권한부여)
//sae000   ALL=(ALL)    ALL
//
///etc/group (root 그룹부여)
//gpasswd -a sae000 root
//
///etc/passwd (root uid,gid 변경)
//sae000:x:0:0::/home/sae000:/bin/bash
//
//<sae000 계정로그인>
//cat ~/.bash_profile
//PATH=$PATH:$HOME/bin:/sbin:/usr/sbin

//# fmsd
//#User_Alias FMS_USR = sae000
//#Cmnd_Alias FMS_CMD = /bin/bash, /fsapp/FMS/bin/fmsd, /fsapp/FMS/scripts/fmsd.init, /usr/bin/make, /bin/chmod, /bin/chown, /bin/systemctl, /sbin/sysctl 
//#FMS_USR ALL = FMS_CMD


#define CALL_OID_NUMBER 7
char OID[CALL_OID_NUMBER][64] = {
                        {" .1.3.6.1.4.1.935.1.1.1.3.2.1.0"},    //입력 전압
                        {" .1.3.6.1.4.1.935.1.1.1.3.2.4.0"},    //입력 주파수
                        {" .1.3.6.1.4.1.935.1.1.1.4.2.1.0"},    //출력 전압
                        {" .1.3.6.1.4.1.935.1.1.1.4.2.3.0"},    //출력 전류(%)
                        {" .1.3.6.1.4.1.935.1.1.1.2.2.2.0"},    //배터리 전압
                        {" .1.3.6.1.4.1.935.1.1.1.2.2.3.0"},    //배터리 온도
                        {" .1.3.6.1.4.1.935.1.1.1.4.1.1.0"}     //UPS Status
                    };

static uint8_t snmpGetLib(char * pIp)
{
//    oid           objid_id  [] = {1,3,6,1,4,1,78945,1,1,2,4,0};
//    oid           objid_name[] = {1,3,6,1,4,1,78945,1,1,2,1,0};
//    oid           trap_oid  [] = {1,3,6,1,4,1,78945,1,1,1,1,1};

    netsnmp_session session, *ss;
    netsnmp_pdu    *pdu    , *response;
    oid anOID[MAX_OID_LEN];
    size_t anOID_len;

    netsnmp_variable_list *vars;
    int count = 1;
    unsigned char comm[] = "public";

    snmp_sess_init(&session);
    session.version   = SNMP_VERSION_2c;
    session.community = comm;
    session.community_len = strlen((char*)session.community);
    session.peername  = pIp;
    session.timeout   = 1;
    session.retries   = 0;

    ss = snmp_open(&session);
    if (!ss) {
        snmp_sess_perror("ack", &session);
        return -1;
    }

    pdu = snmp_pdu_create(SNMP_MSG_GET);
    anOID_len = MAX_OID_LEN;

    for (int i = 0; i < CALL_OID_NUMBER; i++)
    {
        if (!snmp_parse_oid(OID[i], anOID, &anOID_len)) {
            snmp_perror(OID[i]);
            return -1;
        }
        snmp_add_null_var(pdu, anOID, anOID_len);
    }
    int status = snmp_synch_response(ss, pdu, &response);


    /*
     * Process the response.
     */
    if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR)
    {
        /*
         * SUCCESS: Print the result variables
         */
        for(vars = response->variables; vars; vars = vars->next_variable)
            print_variable(vars->name, vars->name_length, vars);

        /* manipuate the information ourselves */
        for(vars = response->variables; vars; vars = vars->next_variable) {
            if (vars->type == ASN_OCTET_STR) {
                char *sp = (char *)malloc(1 + vars->val_len);
                memcpy(sp, vars->val.string, vars->val_len);
                sp[vars->val_len] = '\0';
                printf("value #%d is a string: %s\n", count++, sp);
                free(sp);
            }   else
                printf("value #%d is NOT a string! Ack!\n", count++);
        }
    }   else
    {
        /*
         * FAILURE: print what went wrong!
         */
        if (status == STAT_SUCCESS)
            fprintf(stderr, "Error in packet\nReason: %s\n", snmp_errstring(response->errstat));
        else if (status == STAT_TIMEOUT)
            fprintf(stderr, "Timeout: No response from %s.\n", session.peername);
        else
            snmp_sess_perror("snmpdemoapp", ss);
    }

    if (response)
        snmp_free_pdu(response);
    snmp_close(ss);

    return (0);
}

bool uptDevDataT ()
{
    char sTmp[TMP_BUF] = {0,};
    char        m_Query         [1200];
    char        m_SCQry         [512];
    char        m_SVQry         [256];
    char        m_UCQry         [128];
    char        m_UVQry         [64];

    memset(m_SCQry, 0, sizeof(m_SCQry));
    memset(m_SVQry, 0, sizeof(m_SVQry));
    memset(m_UCQry, 0, sizeof(m_UCQry));
    memset(m_UVQry, 0, sizeof(m_UVQry));

    for (int i = 0; i < 32; i++)
    {
        snprintf(sTmp, sizeof(sTmp), ",BatTemp%d", i+1);
        strcat(m_SCQry, sTmp);
        snprintf(sTmp, sizeof(sTmp), ",%u", i+2000);
        strcat(m_SVQry, sTmp);
    }

    for (int i = 0; i < 6; i++)
    {
        snprintf(sTmp, sizeof(sTmp), ",UpsData%d", i+1);
        strcat(m_UCQry, sTmp);
        snprintf(sTmp, sizeof(sTmp), ",%u", i);
        strcat(m_UVQry, sTmp);
    }

    printf("%s()[%d] m_SCQry = [%s]\n", __FUNCTION__, __LINE__, m_SCQry);
    printf("%s()[%d] m_SVQry = [%s]\n", __FUNCTION__, __LINE__, m_SVQry);
    printf("%s()[%d] m_UCQry = [%s]\n", __FUNCTION__, __LINE__, m_UCQry);
    printf("%s()[%d] m_UVQry = [%s]\n", __FUNCTION__, __LINE__, m_UVQry);
    
    return true;
}


typedef unsigned char BYTE ;
int main(int argc, char** argv)
{
    uptDevDataT();
    exit(0);

    //testDiscovery();
    //exit(0);

    init_snmp("snmp_test");
    int ret = snmpGetLib("172.17.130.8");
    printf("ret = %d\n", ret);


    //uint32_t vl[] = {0, 0, 0, 0, 0};
    //while(1)
    //{
    //    int ret = snmpGet(vl);
    //    //printf("rt = %d\n", ret);
    //    printf("v1 = %d\n", vl[1]);
    //    printf("v2 = %d\n", vl[2]);
    //    printf("v3 = %d\n", vl[3]);
    //    printf("v4 = %d\n", vl[4]);
    //    sleep(1);
    //}
    exit(0);

//    uint16_t    xval = 12136;
//    printf("xval = %u\n", xval/10);
//    xval *= 100;
//    printf("xval = %u\n", xval);
//    exit(0);

//    uint32_t    TCount = 0;
//    while (true)
//    {
//        if (++TCount % 10 == 0)
//        {
//            printf("TCount = [%u]\n", TCount);
//        }
//        sleep(1);
//    }
//
////    qenQuery();
//    exit(0);

    char    InStr[128] = {0,};
    char    OtStr[128] = {0,};

    snprintf(InStr, sizeof(InStr), "('fb75e0f44dde4ba8205032f3d711bb2f', '테스트 입니다').");           // UTF-8 메세지...
    //Utf8ToEuckr (InStr, strlen(InStr), OtStr, sizeof(OtStr));
    EuckrToUtf8 (InStr, strlen(InStr), OtStr, sizeof(OtStr));
    printf("InStr = [%s]\n", InStr);                            // UTF-8 메세지...
    printf("OtStr = [%s]\n", OtStr);                            // Euckr 메세지...
    exit(0);


    
    for (;;)
    {
        InitSleep(0, 6);
        InitSleep(1, 6);
        InitSleep(2, 6);
        InitSleep(3, 6);
        InitSleep(4, 6);
        InitSleep(5, 6);
        
        sleep(1);
    }
    exit(0);

    int16_t tval   = -20;
    uint16_t value = 4000;
    printf("int16_t  value = %d\n", (int16_t)value);
    printf("uint16_t value = %u\n", value);
    
    if (tval >= (int16_t)value)
        printf("int16_t  tval = %d, value = %d\n", tval, (int16_t)value);
    
    exit(0);

//    GETLORA Lora    ;
//    memset(&Lora, 0, SZ_GETLORA);
//    memcpy(Lora.loraserial , "00000211702c1ffffe1cf557"  , sizeof(Lora.loraserial) );
//    memcpy(Lora.loratime   , "2017-08-09 18:16:00.695000", sizeof(Lora.loratime)   );
//    memcpy(Lora.refreshtime, "2017-08-09T02:17:49+09:00" , sizeof(Lora.refreshtime));
//
//    printf("Lora.loraserial = %s\n", Lora.loraserial);
//    printf("Lora.loratime   = %s\n", Lora.loratime  );
//
//    memmove (Lora.loraserial, &Lora.loraserial[8], 16);
//    Lora.loraserial[16] = '\0';
//    snprintf(Lora.loratime  , sizeof(Lora.loratime), "%lu", ToTimestamp(Lora.loratime));
//
//    printf("Lora.loraserial = %s\n", Lora.loraserial);
//    printf("Lora.loratime   = %s\n", Lora.loratime  );


    char    buf[57] = {0,};
    char    out[29] = {0,};
    LORA    Lora          ;
    
    memset(buf, 0, sizeof(buf));
    memset(&Lora, 0, SZ_LORA);
    //snprintf(buf, sizeof(buf), "d9003c00dd0003001405500a0000530ad90221010000370104000000");
    snprintf(buf, sizeof(buf), "                                                        ");
    
    //HexStringToBin(buf, SZ_LORA2, (char*)&Lora);
    HexStringToBin(buf, SZ_LORA2, out);
    
    printf ("Lora.UpsInVol   = [%u]\n", htons(Lora.UpsInVol));
    printf ("Lora.UpsInFreq  = [%u]\n", htons(Lora.UpsInFreq));
    printf ("Lora.UpsOutVol  = [%u]\n", htons(Lora.UpsOutVol));
    printf ("Lora.UpsOutCur  = [%u]\n", htons(Lora.UpsOutCur));
    printf ("Lora.UpsBatVol  = [%u]\n", htons(Lora.UpsBatVol));
    printf ("Lora.UpsBatTemp = [%u]\n", htons(Lora.UpsBatTemp));
    printf ("Lora.UpsStatus  = [%u]\n", htons(Lora.UpsStatus));
    
    printf ("Lora.BatVol     = [%u]\n", htons(Lora.BatVol));
    printf ("Lora.BatCur     = [%u]\n", htons(Lora.BatCur));
    printf ("Lora.BatEnvTemp = [%u]\n", htons(Lora.BatEnvTemp));
    printf ("Lora.BatEnvHumi = [%u]\n", htons(Lora.BatEnvHumi));
    
    printf ("Lora.BatTemp    = [%u]\n", htons(Lora.BatTemp));
    printf ("Lora.BatStatus  = [%u]\n", htons(Lora.BatStatus));
    printf ("Lora.EventBits  = [%u]\n", htons(Lora.EventBits));

    for (int i = 0; i < sizeof(out)-1 ; i++)
    {
        printf ("out[%d] = %02X\n", i, (uint8_t)out[i]);
    }
    
    printf ("time_t   = [%d]\n", sizeof(time_t));
    printf ("unsigned long = [%d]\n", sizeof(unsigned long));
    
    exit(0);

    //uint16_t    LmtEvtBit = 0;
    //
    //LmtEvtBit = LmtEvtBit | APP_MBUS_EVT_UPS_INVOL_MAX ;
    //LmtEvtBit = LmtEvtBit | APP_MBUS_EVT_UPS_INFREQ_MIN;
    //LmtEvtBit = LmtEvtBit | APP_MBUS_EVT_UPS_OUTCUR_MAX;
    //
    //printf("[%04X]\n", LmtEvtBit & APP_MBUS_EVT_UPS_INVOL_MAX );
    //printf("[%04X]\n", LmtEvtBit & APP_MBUS_EVT_UPS_INFREQ_MIN);
    //printf("[%04X]\n", LmtEvtBit & APP_MBUS_EVT_UPS_OUTCUR_MAX);
    //exit(0);


    //uint32_t TryCount = 0;
    //char buf[100];
    //gettime(buf,'L');
    //printf("settime 1 = (%s)\n", buf );
    //while(true)
    //{
    //    SDL_Delay(_DELAY_WAIT_) ;
    //    TryCount += _DELAY_WAIT_;
    //    if (TryCount > _WAIT_COUNT_)
    //    {
    //        break;
    //    }
    //}
    //gettime(buf,'L');
    //printf("settime 2 = (%s)\n", buf);
    //exit(0);




    printf("%d\n", sizeof(LMT_CODE));

    time_t  tt;
    time(&tt);
    printf("%lu\n", tt);

    uint32_t  uid = 1 ;
    printf("%010u\n", uid);

    exit(0);


    char sssmsg[3] = {0,};
    HMSG * e = (HMSG*)sssmsg;
    e->length = 2;

    printf("Length1 = %02X\n", (unsigned char)sssmsg[0]);
    printf("Length2 = %02X\n", (unsigned char)sssmsg[1]);
    
    printf("Length2 = %u\n", e->length);
    
//    sLen[0] = 0x12;
//    sLen[1] = 0x34;
//    
//    printf("Length1 = %u\n", HexToU16(sLen, 2));
//    uint16_t num16 = 0x3412;
//    printf("Length2 = %u\n", htons(num16));
//    num16 = htons(num16);
//    printf("Length3 = %u\n", htons(num16));
    exit(0);
    


    struct timeval tv11, tv22;
    gettimeofday(&tv11, NULL);

    bool IsBool = true;
    waitCall(_RECV_WAIT_, &IsBool);

    gettimeofday(&tv22, NULL);
    printf("time interval start (%d.%06d), end (%d.%06d) !! \n", tv11.tv_sec, tv11.tv_usec, tv22.tv_sec, tv22.tv_usec);

    
    exit(0);


    char testAmp[12];
    char * pBuf = "test1=value;test2=value2;";
    
    string val1 = getKeyValue2(pBuf, "test1", "val1");
    printf("test1 = [%s]\n", val1.c_str());
    string val2 = getKeyValue2(pBuf, "test2", "val2");
    printf("test2 = [%s]\n", val2.c_str());

    exit(0);


    MsgBuf mBuf;
    snprintf(mBuf.msg, sizeof(mBuf.msg), "{\"message\":\"<3>May 21 12:02:08 hb02 kernel: 'echo' \"0 > /proc/sys/kernel/hung_task_timeout_secs\" disables this message.\"}\n");
    mBuf.len = strlen(mBuf.msg);

    printf("msg : (%s) (%d)\n", mBuf.msg, mBuf.len);
    pcre_regex_message(&mBuf);
    printf("msg : (%s) (%d)\n", mBuf.msg, mBuf.len);

    exit(0);




    setColor();
    exit(0);


    COND    CondLst   ;
    char    tstStr[10];
    memset(&CondLst, 0, sizeof(COND));
    for (uint8_t i = 0; i < 2; i++)
    {
        snprintf(tstStr, sizeof(tstStr), "%d:60%d:1:0", i+5, i);
        pcre_regex_condition(&CondLst.eCond[CondLst.eCount], tstStr, i);

        printf("(%d)Severity = %d\n", i, CondLst.eCond[CondLst.eCount].Severity);
        printf("(%d)Xnc      = %d\n", i, CondLst.eCond[CondLst.eCount].Xnc     );
        printf("(%d)Max      = %d\n", i, CondLst.eCond[CondLst.eCount].Max     );
        printf("(%d)Mail     = %d\n", i, CondLst.eCond[CondLst.eCount].Mail    );
        printf("(%d)SMS      = %d\n", i, CondLst.eCond[CondLst.eCount].SMS     );

        CondLst.eCount++;
    }

    exit(0);

    printf("curt = %s\n", getTimestamps(-7));
    exit(0);

    char tmpS[30];
    snprintf(tmpS, sizeof(tmpS), "123.232.43.23");
    printf("Addr2Num = %s\n", tmpS);

    uint32_t num = Addr2Num (tmpS);
    printf("Addr2Num = %u\n", num);

    num = ntohl(num);
    printf("ntohl    = %u\n", num);

    snprintf(tmpS, sizeof(tmpS), "%s", Num2Addr(num));
    printf("Num2Addr = %s\n", tmpS);

    exit(0);

    _PrintTSDBLine( "test", 30000, 1500000);
    exit(0);

    randomize();
    //printf("Time2Date = %s %s\n", getCFHeader(), Time2Date (GetRowKey(1422345243)));
    printf("Time2Date = %s %s\n", getCFHeader(), Min2Date (3515)); //GetRowKey(500)));
    exit(0);

    testMsg();
    exit(0);

    char pData[512] = "";
    curlExecute(pData);
    exit(0);



    dateHeader();
    exit(0);


//    struct tm tm ;
//    char buf[255] = {0,};
//    strptime("2014 Jul 28 11:43:07", "%Y %b %d %H:%M:%S", &tm);
//    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
//    fprintf(stderr, "%d %s\n", tm.tm_year+1900, buf);
//    exit(0);


    //UpdateDayTableDescr (0);
    //exit(0);
    //printf("result = %s\n", Time2Date(1418968800));
    //printf("result = %s\n", getTimestamps()); // InterCall());
    printf("result = %u\n", GetUnixTime_val("20141230172302")); // InterCall());
    exit(0);


    uint64_t nCount = 0;
    SDL_Thread * Threads1 = SDL_CreateThread(openTSDB_Test, &nCount);
    SDL_Thread * Threads2 = NULL; //SDL_CreateThread(openTSDB_Coun, &nCount);

    if (Threads1)
    {
        SDL_WaitThread(Threads1, NULL);
        Threads1 = NULL;
    }
    if (Threads2)
    {
        SDL_WaitThread(Threads2, NULL);
        Threads2 = NULL;
    }
    exit(0);


    FLUENTD  fFld;
    //char * lBuff = "2014-11-20T01:13:20Z  fluentd {\"host\":\"ems\",\"ident\":\"snoopy\",\"pid\":\"10170\",\"message\":\"[uid:503 sid:28668 tty: cwd:/home/bb/bb17b4 filename:/usr/bin/expr]: /usr/bin/expr 60 - 0 \",\"facility\":\"authpriv\",\"severity\":\"info\"}";
    //char * lBuff = "2014-11-20T05:37:22Z  host:ems    ident:snoopy    pid:28551   message:[uid:503 sid:28668 tty: cwd:/home/bb/bb17b4 filename:/home/bb/bb17b4/bin/bb]: /home/bb/bb17b4/bin/bb 211.115.83.209 status ems.memory green Thu Nov 20 14:37:22 KST 2014 - Memory OK #012Memory#011  Used#011  Total#011  Percentage#012&green Physical#0113407M#0113999M#01185%#012&green Actual#011179M#0113999M#0114%#012&green Swap#0110M#0114094M#0110%#012#012<FONT SIZE=-1>$Id: bb-memory.sh,v 2.0.0.1 2000/09/05 17:41:23 brs Exp $</FONT>#012  facility:authpriv   severity:info";
    char * lBuff = "2014-11-20T05:00:01Z    host:gcloud-nc92    ident:CROND pid:16837   message:(root) CMD (su -l root -c 'sh /moni/vm/vm_check.sh')    facility:cron   severity:info";
    pcre_regex_fluentd_parsing(&fFld, lBuff);
    exit(0);



    TestSummury();
    exit(0);


    printf("63 = (%llu)\n", _LIMIT_INT63_);
    printf("60 = (%llu)\n", _LIMIT_INT60_);
    printf("50 = (%llu)\n", _LIMIT_INT50_);
    printf("40 = (%llu)\n", _LIMIT_INT40_);
    printf("30 = (%llu)\n", _LIMIT_INT30_);
    printf("20 = (%llu)\n", _LIMIT_INT20_);
    printf("10 = (%llu)\n", _LIMIT_INT10_);
    exit(0);

    Seconds2String(315);
    exit(0);

    //char * fBuff = "<27>Oct 31 16:01:24 devcloud-monitoring /usr/sbin/cerebrod[3083]: lmt_mysql: failed to connect to database";
    //char * fBuff = "<27>Oct 31 16:01:24 devcloud-monitoring postfix/local[3083]: lmt_mysql: failed to connect to database";
    //char * fBuff = "<27>Oct 31 16:01:24 devcloud-monitoring snoopy[3083]: lmt_mysql: failed to connect to database";
    char * fBuff = "<27>Oct 31 16:01:24 devcloud-monitoring su: lmt_mysql: failed to connect to database";

    MPcre  fDiv;
    pcre_regex_partial_parsing(&fDiv, fBuff);
    exit(0);




    //[25/Sep/2014:09:13:00 +0900]
    char * accesslog = "61.35.204.100 - - [25/Sep/2014:09:13:00 +0900] \"GET /app/panels/terms/editor.html HTTP/1.1\" 200 2465 \"http://dev.everview.co.kr\" \"Mozillz/4.08 [en] (Win98; I ;Nav)\"";
    char * errorlog = "[Thu Sep 25 01:05:02 2014] [error] [client 127.0.0.1] File does not exist: /home/geverview/public_html/new_admin";

    FireWall  wDiv;
    pcre_regex_weblog_parsing(&wDiv, accesslog, errorlog);
    exit(0);


    char * Buff = "<86>Aug 21 16:50:49 Cloud-Front CC: (BaseJdbcLogger.java:139) <== Total: Closing JDBC Connection [com.mysql.jdbc.JDBC4Connection@b977a73]";

    MPcre  Div;
    pcre_regex_parsing(&Div, Buff);
    exit(0);


    //char * pBuff = "2014-09-12T01:00:04.014489+09:00 pankyo-fw GABIA_LG: NetScreen device_id=GABIA_LG  [Root]system-notification-00257(traffic): start_time="2014-09-12 01:00:04" duration=0 policy_id=15 service=udp/port:17591 proto=17 src zone=Untrust dst zone=Trust action=Deny sent=0 rcvd=46 src=88.231.116.105 dst=61.35.204.111 src_port=53 dst_port=17591 session_id=0 reason=Traffic Denied";
    //char * pHead = "2014-09-12T01:00:04.014489+09:00 pankyo-fw GABIA_LG: NetScreen device_id=GABIA_LG  [Root]system-notification-00257(traffic):";
    //char * pMsgs = "start_time=\"2014-09-12 01:00:04\" duration=0 policy_id=15 service=udp/port:17591 proto=17 src zone=Untrust dst zone=Trust action=Deny sent=0 rcvd=46 src=88.231.116.105 dst=61.35.204.111 src_port=53 dst_port=17591 session_id=0 reason=Traffic Denied";

    char * pHead = "2014-09-12T01:00:04.014489+09:00 pankyo-fw GABIA_LG: NetScreen device_id=GABIA_LG  [Root]system-notification-00257(traffic): start_time=\"2014-09-12 01:00:04\" duration=0 policy_id=15 service=udp/port:17591 proto=17 src zone=Untrust dst zone=Trust action=Deny sent=0 rcvd=46 src=88.231.116.105 dst=61.35.204.111 src_port=53 dst_port=17591 session_id=0 reason=Traffic Denied";
    char * pMsgs = NULL ;

    FireWall  pDiv;
    pcre_regex_firewall_parsing(&pDiv, pHead, pMsgs);
    exit(0);


    char * xBuff = "sudo-sgesge";
    printf("dddd = %s\n", pcre_regex_replace(xBuff));
    exit(0);

    char * multiport = "syslog:5140"; // weblogA:5141";
    pcre_regex_mport(multiport);
    exit(0);





//    PCREDiv pDiv;
//    fprintf(stderr, "SZ_PDIV = %d\n", sizeof(PCREDiv));
//    pDiv.hostname = "tesg sgmesltg";
//    fprintf(stderr, "SZ_PDIV = %d\n", sizeof(PCREDiv));
//    exit(0);
//




    MultiKey  m_Key ;
    char * pStrKey = "crond\[[0-9]+\\]: + ems +   24325r325     + 342aklgage";
    setStrKey (&m_Key, pStrKey);
    for ( int i = 0; i < m_Key.Count; i++ )
    {
        printf("StrKey[%d] = (%s)\n", i, m_Key.StrKey[i]);
    }

    exit(0);


    MDivide mDiv;
    UdpMSG  umsg ;
    snprintf(umsg.msg, sizeof(umsg.msg), "<142>Aug  5 17:28:44 ems sudo:       bb : sorry, you must have a tty to run sudo ; TTY=unknown ; PWD=/home/bb/bb17b4 ; USER=root ; COMMAND=/bin/grep -r hung_task /var/log/messages");
    umsg.len = strlen(umsg.msg);

    if (messageParser (&mDiv, &umsg))
    {
        printf ("%s() SMS Success (facility  = %d)\n", __FUNCTION__, mDiv.facility );
        printf ("%s() SMS Success (severity  = %d)\n", __FUNCTION__, mDiv.severity );
        printf ("%s() SMS Success (timestamp = %s)\n", __FUNCTION__, mDiv.timestamp);
        printf ("%s() SMS Success (hostname  = %s)\n", __FUNCTION__, mDiv.hostname );
        printf ("%s() SMS Success (messages  = %s)\n", __FUNCTION__, mDiv.messages );
    }

    exit(0);


//  mq_Init();
//  exit(0);

//  uint16_t Curt = 0;
//  uint16_t MaxT = 5;
//
//  for ( int i = 0; i < 50; i++)
//  {
//      uint8_t No  = (Curt++ % MaxT);
//      printf("Curt = %d, MaxT = %d, No = [%d]\n", Curt-1, MaxT, No);
//  }
//  exit(0);








    SDL_Thread * nThreads = SDL_CreateThread(SyslogUDPReceiving, NULL);
    if (nThreads)
    {
        SDL_WaitThread(nThreads, NULL);
        nThreads = NULL;
    }
    exit(0);




    CDataMap  dmap;
    char      SerialNo [10];
    dmap.setfree(ReleaseData);

    for ( int i = 10000; i > 0; i-- )
    {
        memset(SerialNo, 0, sizeof(SerialNo));
        StreamConnectionInfo * Info = new StreamConnectionInfo;
        memset(Info, 0, sizeof(StreamConnectionInfo));
        snprintf(SerialNo, sizeof(SerialNo), "%08d", i);
        dmap.insert(SerialNo, Info);
    }
    printf( "map size = (%d)\n", dmap.size());

    snprintf(SerialNo, sizeof(SerialNo), "%08d", 16756);
    StreamConnectionInfo * Info = (StreamConnectionInfo *)dmap.getdata(SerialNo);
    if (Info)
        printf("Find it SerialNo = (%s)\n", SerialNo);
    else
        printf("Failed SerialNo != (%s)\n", SerialNo);
    dmap.remove(SerialNo);
    printf( "map size = (%d)\n", dmap.size());

    snprintf(SerialNo, sizeof(SerialNo), "%08d", 4756);
    Info = (StreamConnectionInfo *)dmap.getdata(SerialNo);
    if (Info)
        printf("Find it SerialNo = (%s)\n", SerialNo);
    else
        printf("Failed SerialNo != (%s)\n", SerialNo);
    dmap.remove(SerialNo);
    printf( "map size = (%d)\n", dmap.size());

    for ( int i = 1; i <= 10000; i++ )
    {
        memset(SerialNo, 0, sizeof(SerialNo));
        snprintf(SerialNo, sizeof(SerialNo), "%08d", i);
        dmap.remove(SerialNo);
    }
    printf( "map size = (%d)\n", dmap.size());

    exit(0);


//  getSDate ();
//  getSDate (-3);
//
//
//  char data[2][20] = {"20140331161617","20140331161616"};
//
//  printf("data1(%s), date2(%s) = (%d)\n", data[0], data[1], memcmp(data[0],data[1], strlen(data[0])));
//  exit(0);


//  uint8_t keepCnt = 0;
//
//  while (1)
//  {
//        if (!(keepCnt % 60))
//        {
//          printf("zero keep\n");
//            keepCnt = 10;
//        }   else
//        {
//            keepCnt += 10;
//        }
//      printf("zero S (%d)\n", keepCnt);
//      sleep(10);
//  }
//
//  exit(0);
//
//  uint64_t    value1, value2;
//
//    sscanf( "21345215", "%llu", &value1);
//      sscanf( "45454646", "%llu", &value2);
//
//      printf("value1 = %llu\n", value1);
//      printf("value2 = %llu\n", value2);
//
//  uint64_t    value3, value4;
//
//  value3 = strtoull ("21345215", NULL, 10);
//
//      printf("value3 = %llu\n", value3);
//
//
//      exit(0);

//  printf("size = (%u) \n", sizeof(StreamConnectionInfo));
    timeout_AlarmTimer();
    exit(0);


    int  div_rate = 50;
    uint32_t div_value = (uint32_t)(ntohl(2132342315) * ((float)div_rate / 100)) ;
    printf("div_value = (%u) \n", div_value);
    exit(0);

    SPD (300 ,100);
    SPD (364 ,100);

    SPD (500 ,100);
    SPD (564 ,100);

    SPD (1500,100);
    SPD (1564 ,100);

    exit(0);


  char * ipaddr1 = NULL ;
  char * ipaddr2 = "" ;

  printf("ipaddr1 = (%08X) \n", NetMask (ipaddr1, 0x00FFFFFF));
  printf("ipaddr2 = (%08X) \n", NetMask (ipaddr2, 0x00FFFFFF));

  if (NetCheck(ipaddr1, ipaddr2, 0x00FFFFFF))
      printf("equal !!! \n");
  else
      printf("not equal !!!\n");
  exit(0);



//  char Body = 0xAD ;
//
//  printf("test = [%d]\n", (unsigned char)Body);
//  exit(0);

//  SwitchCase();
//  ifelseif  ();
//  exit(0);

//  int socket = setupSocket(0, 0 /* =>nonblocking */ );
//  int err ;
//    struct sockaddr_in managerName;
//    managerName.sin_family      = AF_INET;
//    managerName.sin_port        = htons(2280);
//    managerName.sin_addr.s_addr = inet_addr("182.10.32.1");
//
//  fprintf(stderr, "step 1 (socket = %d)\n", socket);
//    if (connectWithTimeoutNS(socket, (struct sockaddr*)&managerName, sizeof managerName, 5, &err) != 0)
//    //if (connect (socket, (struct sockaddr*)&managerName, sizeof managerName) != 0)
//    {
//        fprintf(stderr, "Connection Failed !!! (127.0.0.1:2280)(%s)\n", strerror(err));
//    }   else
//      fprintf(stderr, "step 2 (socket = %d)\n", socket);
//  close(socket);
//  exit(0);

//    char testStr[20] = {};
//
//      for (int i = 0; i < 20; i++)
//      {
//          printf("testStr[%d] = %02X\n", i, testStr[i]);
//          usleep(10);
//      }
//      exit(0);


//  for (int i = 0; i < 1000; i++)
//  ;{
//      gettime(testStr, 'l');
//      printf("%s\n", testStr);
//      usleep(10);
//  }
//  exit(0);




//  timeout_AlarmTimer();
//  exit(0);
//
//
//
//  while(1)
//  {
//      struct timeval timv;
//      gettimeofday(&timv, NULL);
//
//      printf("seconds = %d\n", (timv.tv_sec%3600));
//      printf("seconds = %s\n", second2Minute((uint32_t)(timv.tv_sec%3600)));
//
//      printf("compute1 = %d\n", NewInterval1(1800, 2));
//      printf("compute2 = %d\n", NewInterval2(1800, 2));
//
//      sleep(1);
//  }
//  exit(0);
//
//
//  printf("float = %.1f\n", (double)2000/(double)2000);
//  exit(0);



    char tmpStr[10] = {};
    //tmpStr[9] = 'X';

    if (tmpStr[9] == '\0' )
        printf("test true  !!!\n");
    else
        printf("test false !!!\n");

    exit(0);



    bool isClose = false;
    getCalled(&isClose);
    isClose = true;
    getCalled(&isClose);
    exit(0);
    uint8_t key = 35 ;
    //  0 : Axis-Cube-Basic   (000 000 00)   <=  1
    // 32 : Hitron-Cube-Basic (001 000 00)   <= 33
    // 35 : Hitron-Cube-Full  (001 000 11)   <= 45
    // 36 : Hitron-Bull-Basic (001 001 00)
    // 39 : Hitron-Bull-Full  (001 001 11)
    // 40 : Hitron-Dome-Basic (001 010 00)
    // 43 : Hitron-Dome-Full  (001 010 11)

    uint8_t type = getCamType( key );           // 1 -> 0, 33
    uint8_t kind = getCamKind( key );
    uint8_t optn = getCamOptn( key );

    printf("type = (%02X), kind = (%02X), optn = (%02X) \n", type, kind, optn);
    printf("type = (%d), kind = (%d), optn = (%d) \n", type, kind, optn);

    printf("loff_t length = (%d)\n", sizeof(loff_t));
    printf("int32_t length = (%d)\n", sizeof(int32_t));
    exit(0);


    int recvbuff = 60;
    double recv_rate = (recvbuff/100.0);

    printf("recv_rate = (%f)\n", recv_rate);

    printf("value = (%d)\n", getBuffSize ("640x480", recv_rate));

    exit(0);


    struct timeval tv1, tv2;
    gettimeofday(&tv1, NULL);

    myThreadDelay ( 3 );

//  for ( int i = 0; i < 100000000; i++ )
//  {
//      char * tmp = new char[1000000];
//      delete[] tmp ;
//      tmp = NULL   ;
//  }
    gettimeofday(&tv2, NULL);
    printf("time interval start (%d.%06d), end (%d.%06d) !! \n", tv1.tv_sec, tv1.tv_usec, tv2.tv_sec, tv2.tv_usec);

    exit(0);


    bool flag = false;

    if ( !flag || !getPrint() )
    {
        printf("test call 2222222 !!! \n");
    }



    exit(0);


    char spss[32] = {};
    char pps [32] = {};

    int spsslen = getSPSSInfo("Z0KAMtoCgPRA,aM48gA==", 21, spss);
    int ppslen  = getPPSInfo ("Z0KAMtoCgPRA,aM48gA==", 21, pps );

    for ( int i = 0; i < spsslen; i++ )
        fprintf(stderr, "SPSS Input [%2d] = [%c][%02x]\n", i, spss[i], 0xFF&spss[i]);

    for ( int i = 0; i < ppslen; i++ )
        fprintf(stderr, "PPS  Input [%2d] = [%c][%02x]\n", i, pps[i] , 0xFF&pps[i]);

    exit(0);



    char m_FRAME[256];
    char EventType   ;
    snprintf(m_FRAME, sizeof(m_FRAME), "<EventNotification UtcTime=\"2016-01-16T22:21:35Z\"><Motion>" \
                                       "<Region Name=\"1\" Value=\"1\" />" \
                                       "</Motion>" \
                                       "</EventNotification>" );
    int length = strlen(m_FRAME) ;

    getValue(&m_FRAME[50], (length-60), "PIR Value=\"", (char*)&EventType , '"');
    //getValue(&m_FRAME[50], (length-60), "Region Name=\"1\" Value=\"", (char*)&EventType , '"');

    exit(0);


    rtp_packet * packet   = NULL;
    uint8_t    * buffer   = NULL;

    packet   = (rtp_packet *) malloc(RTP_MAX_PACKET_LEN);
    buffer   = (unsigned char *) packet + offsetof(rtp_packet, fields);

    buffer[0] = 0x80 ;
    buffer[1] = 0x60 ;
    buffer[2] = 0x00 ;
    buffer[3] = 0x74 ;
    buffer[4] = 0xa8 ;
    buffer[5] = 0x3b ;
    buffer[6] = 0x63 ;
    buffer[7] = 0x62 ;
    buffer[8] = 0xa4 ;
    buffer[9] = 0xf0 ;
    buffer[10] = 0x67 ;
    buffer[11] = 0x1c ;


    printf(" val = %d\n", offsetof(rtp_packet, meta));
    printf(" val = %d\n", offsetof(rtp_packet, fields));
    printf(" val = %d\n", sizeof(rtp_packet));

//  printf(" val = %d\n", sizeof(_meta));
//  printf(" val = %d\n", sizeof(struct _fields));


//  packet->cc = 2;
//  printf(" packet->fields.v  = %d\n", packet->v);
//  printf(" packet->fields.p  = %d\n", packet->p);
//  printf(" packet->fields.x  = %d\n", packet->x);
//  printf(" packet->fields.cc = %d\n", packet->cc);
//  printf(" packet->fields.m  = %d\n", packet->m);
//  printf(" packet->fields.pt = %d\n", packet->pt);

    printf(" packet->fields.v  = %d\n", packet->fields.v);
    printf(" packet->fields.p  = %d\n", packet->fields.p);
    printf(" packet->fields.x  = %d\n", packet->fields.x);
    printf(" packet->fields.cc = %d\n", packet->fields.cc);
    printf(" packet->fields.m  = %d\n", packet->fields.m);
    printf(" packet->fields.pt = %d\n", packet->fields.pt);

    free (packet);
    exit(0);




    fprintf(stderr, "%f\n", ceil((double)1200/(double)2000));
    fprintf(stderr, "%d\n", (int)ceil(((double)1200/(double)2000)));

    //fprintf(stderr, "[%s]\n", getIpaddr("eth0"));

    exit(0);


    char  TmpStr[2048];
    memset(TmpStr, 0, sizeof(TmpStr));
    snprintf(TmpStr,2048,"v=0\r\n" \
                         "o=- 1 1 IN IP4 10.20.23.158\r\n" \
                         "s=Media Presentation\r\n" \
                         "c=IN IP4 0.0.0.0\r\n" \
                         "t=0 0\r\n" \
                         "a=control:*\r\n" \
                         "a=range:npt=now-\r\n" \
                         "m=video 0 RTP/AVP 96\r\n" \
                         "a=control:rtsp://10.20.23.158/1/stream1/video\r\n" \
                         "a=rtpmap:96 H264/90000\r\n" \
                         "a=fmtp:96 packetization-mode=1;profile-level-id=66;sprop-parameter-sets=Z0KAMtoCgPRA=,aM48gA==,\r\n" \
                         "a=recvonly\r\n" \
                         "m=audio 0 RTP/AVP 0\r\n" \
                         "a=control:rtsp://10.20.23.158/1/stream1/audio\r\n" \
                         "a=rtpmap:0 PCMU/8000/1\r\n" \
                         "a=recvonly\r\n" \
                         "m=application 0 RTP/AVP 98\r\n" \
                         "a=control:rtsp://10.20.23.158/1/stream1/meta\r\n" \
                         "a=rtpmap:98 vnd.onvif.metadata/90000\r\n" \
                         "a=recvonly\r\n");

    char lBuf[100];
    memset(lBuf, 0, 100);
    //snprintf(lBuf, 100, "sprop-parameter-sets=ZOKAMtoCgPRA=,aM48gA==,,,,,");
    snprintf(lBuf, 100, "sprop-parameter-sets=Z0KAMtoCgPRA=,aM48gA==,");

    if ( !getSDPRead (TmpStr, strlen(TmpStr), lBuf) )
    {
        fprintf( stderr, "%s()[%d] SDP first parsing error !!!", __FUNCTION__, __LINE__);
        return false;
    }
    fprintf(stderr, "TmpStr = \n%s\n", TmpStr);

    exit(0);


    char   AuthStr[SML_BUF];
    memset(AuthStr, 0, SML_BUF);
    uint8_t Stream;
    uint8_t Device;

    int Len = getMessageParsing(TmpStr, strlen(TmpStr), Stream, AuthStr, Device);

    printf("Stream  = %d\n", Stream );
    printf("AuthStr = [%d][%s]\n", Len, AuthStr);
    printf("Device  = %d\n", Device );

    exit(0);

//  GetUnixTime_val ( "20120821140356");



//  char     tmpStr[100]    ;
//  uint64_t value = 4323   ;
//
//  memset(tmpStr, 0, sizeof(tmpStr));
//  memcpy(tmpStr, &value, SZ_I64);
//
//
//  for ( int i = 0; i < SZ_I64; i++ )
//  {
//      printf( "tmpStr[%d] = %02X \n", i, (unsigned char)tmpStr[i]);
//  }
//
//  VAL * test = (VAL*)tmpStr   ;
//  memset(test, 0, sizeof(VAL));
//  test->key = value           ;
////    test->key = htonl(value)    ;
//
//  for ( int i = 0; i < SZ_I64; i++ )
//  {
//      printf( "test.key[%d] = %02X \n", i, (unsigned char)tmpStr[i]);
//  }
//
//
//  fprintf(stderr, "value = %lld\n", 1346923282975746);
//  fprintf(stderr, "value = %llu\n", 1346923282975746);



//  fprintf(stderr, "tmpTStr = [%s]\n", getLastTime());


//    char    m_Src[1000];
//    snprintf(m_Src, sizeof m_Src, "[Gabia_KeepAlive]OPTIONS rtsp://112.216.71.218/TEST00408CB92E6A RTSP/1.0\r\n\r\n"
//                                  "v=0\r\n"
//                                  "o=- 1311138109406907 1311138109406907 IN IP4 10.20.23.154\r\n"
//                                  "s=Media Presentation\r\n"
//                                  "e=NONE\r\n"
//                                  "c=IN IP4 0.0.0.0\r\n"
//                                  "b=AS:50000\r\n"
//                                  "t=0 0\r\n"
//                                  "a=control:rtsp://10.20.23.154/1/stream1/video?videocodec=h264\r\n"
//                                  "a=range:npt=0.000000-\r\n"
//                                  "a=control:rtsp://10.20.23.154/1/stream1/audio?videocodec=h264   [Gabia_KeepAlive]\r\n" );
//
//  printf ("m_Src = \n%s", m_Src );
//  RTSPFiltering ( m_Src, strlen(m_Src) );
//  printf ("\n\nm_Src = \n%s", m_Src );


//    int m_Len = strlen(m_Src);
//
//    m_Src[200] = 0x7F   ;
//
//    for ( int i = 0; i < m_Len; i++ )
//    {
//        if (m_Src[i] != 0x0D && m_Src[i] != 0x0A)
//        {
//            if ( m_Src[i] < 0x20 || m_Src[i] >= 0x7F  )
//            {
//                m_Src[i] = '\0' ;
//                m_Len    = i    ;
//                break           ;
//            }
//        }
//    }
//
//    for ( int i = 0; i < m_Len; i++ )
//    {
//        printf("m_Src[%d] = [%c]\n", i, m_Src[i]);
//    }
//
//    printf("m_Src = [%d]\n%s\n", m_Len, m_Src);


//    char        tmp[100];
//    SerialNo    test    ;
//    SerialNo    test2   ;
//
//    snprintf(test, sizeof(test), "test");
//    memcpy(test2, test, sizeof(test)) ;
//    fprintf(stderr, "test2 = (%s) (%d)\n", test2, sizeof(test2));
//
//    snprintf(tmp, sizeof(tmp), "10.20.sge.152");
//
//    if ( IsDomainName (tmp, strlen(tmp)) )
//        fprintf(stderr, "Yes, is domain (%s) !!\n", tmp);
//    else
//        fprintf(stderr, "Yes, is Ipaddr (%s) !!\n", tmp);

//    list<uint32_t>  m_list;
//
//    char test[100];
//    uint32_t curt = 0;
//    uint8_t  dddd = 0x41;
//    for ( int i = 0; i < 10000; i++ )
//    {
//        if (dddd > 0x7A) dddd = 0x41;
//        snprintf (test, sizeof(test), "%c%015d", dddd++, i);
//        curt = hashIndexFromKey((const char*)test);
//
//      list<uint32_t>::iterator itr     = m_list.begin();
//      list<uint32_t>::iterator itr_end = m_list.end();
//      while (itr != itr_end)
//      {
//          uint32_t find = *itr;
//          //printf("curt = (%010u), *itr = (%010u)\n", curt, find);
//          if ( curt == find )
//          {
//              printf("dup !!! test = (%s), curt = (%010u)\n", test, curt);
//              goto    dupp;
//          }
//          ++itr;
//      }
//        m_list.push_back(curt)  ;
//dupp:
//        if (i % 1000 == 0) printf("Count = [%d]\n", i);
//        usleep(10) ;
//    }

//    exit(0);
//
//    char * rawfile = "/rawdata/data1/SERIALNUMBER0002/20111019/20111019154730.idx";
//
//    CIDXFile * m_idx = new CIDXFile()  ;
//
//    m_idx->openFile(rawfile)    ;
//
//    fprintf(stderr, "find time = 20111019155730\n");
//
//    loff_t fpos = m_idx->findposition( "20111019155730");
//
//    fprintf(stderr, "pos = %d\n", fpos);
//
//    delete m_idx;
//
//    exit(0);
//
//
//    for ( int i = 0; i < 10; i++)   {
//        delayTimestamp (99000);
////        usleep (99000);
//        fprintf(stderr, "delayTimestamp (99000)\n");
//    }
//    exit(0);

//    TEST    test    ;
//
//    memcpy(test.tmp, "20110726133030", 14);
//    test.pos    = 2345  ;
//    test.size   = 16000 ;
//
//    openWriteData("/avsol/test/text.idx.tmp");
//    writeData    (m_rawfp, (char*)&test, sizeof(TEST));
////    writeData    (m_rawfp, "test", 4);
//    close        (m_rawfp);
//
//    rename ("/avsol/test/text.idx.tmp","/avsol/test/text.idx");
//
//    memset(&test, 0, sizeof(TEST));
//    openReadData ("/avsol/test/text.idx");
//    readData     (m_rawfp, (char*)&test, sizeof(TEST));
//    close        (m_rawfp);
//
//    printf("test.tmp  = [%s]\n", test.tmp);
//    printf("test.pos  = [%d]\n", test.pos);
//    printf("test.size = [%d]\n", test.size);
//    exit(0);
//
//
//    int m_Len ;
//    char * Buff = getDataQuota("response=", m_Len, ',');
//
//    printf("Buff = [%s][%d]\n", Buff, m_Len);
//    exit(0);
//
//
//    char Buf[500];
//    int  Len ;
//    memset(Buf, 0, sizeof(Buf));
//
//    char m_Src[500];
//    snprintf(m_Src, sizeof m_Src, "OPTIONS rtsp://112.216.71.218/TEST00408CB92E6A RTSP/1.0\r\n\r\n"
//                                  "v=0\r\n"
//                                  "o=- 1311138109406907 1311138109406907 IN IP4 10.20.23.154\r\n"
//                                  "s=Media Presentation\r\n"
//                                  "e=NONE\r\n"
//                                  "c=IN IP4 0.0.0.0\r\n"
//                                  "b=AS:50000\r\n"
//                                  "t=0 0\r\n"
//                                  "a=control:rtsp://10.20.23.154/1/stream1/video?videocodec=h264\r\n"
//                                  "a=range:npt=0.000000-\r\n"
//                                  "a=control:rtsp://10.20.23.154/1/stream1/audio?videocodec=h264\r\n" );
//
////    printf("Tmp = [%s]\n", Tmp);
////    Len = parseSDPValue ( m_Src, strlen(m_Src), Buf);
////    printf("Buf = [%s]\n", Buf);
////    printf("Len = [%d]\n", Len);
//
//    printf("m_Src 1 = \n%s\n", m_Src);
//
//    int FldIndex = FindFieldIndex ("/1/stream/video", m_Src, strlen(m_Src));
//    if (FldIndex >=
//    printf("FldIndex = %d, m_src = %d\n", FldIndex, strlen(m_Src));
//
//    exit(0);
//
//    memcpy (Buf, m_Src, FldIndex);
//    snprintf(&Buf[FldIndex], sizeof(Buf)-FldIndex, "trackID=1");
//    int NewPos = FldIndex + 9;
//    memcpy (&Buf[NewPos], &m_Src[FldIndex+5], sizeof(Buf)-NewPos);
//
//    snprintf(m_Src, sizeof(m_Src), "%s", Buf);
//    memset(Buf, 0, sizeof(Buf));
//
//    FldIndex = FindFieldIndex ("/1/stream1/audio", m_Src, strlen(m_Src));
//    memcpy (Buf, m_Src, FldIndex);
//    snprintf(&Buf[FldIndex], sizeof(Buf)-FldIndex, "trackID=2");
//    NewPos = FldIndex + 9;
//    memcpy (&Buf[NewPos], &m_Src[FldIndex+5], sizeof(Buf)-NewPos);
//
//
//    //printf("FldIndex = [%d] %s\n", FldIndex, &m_Src[FldIndex]);
//    printf("Buf = %s\n", Buf);
//
//    //ControlSwappingUri ("a=control:", m_Ctl, m_Src, strlen(m_Src));
//
//    //printf("m_Src 2 = \n%s", m_Src);
//    exit(0);

//    char ssrc[9];
//    uint32_t tt = 2324325233UL;
//
//    snprintf(ssrc, sizeof(ssrc), "%08X", tt );
//    printf("ssrc  = [%s]\n", ssrc);
//    uint32_t value = strtoul (ssrc, NULL, 16);
//
//    printf("value = [%lu]\n", value);
//    exit(0);
//
//

    exit(0);
}

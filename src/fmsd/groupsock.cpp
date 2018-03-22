#include "groupsock.h"
extern CServerLog  pLog;

//static inline const void delayMilliTime(uint32_t milliseconds)
//{
//    struct timeval timeout  ;
//    timeout.tv_sec  = 0     ;
//    timeout.tv_usec = (milliseconds * 1000) ;
//    if (select(0, NULL, NULL, NULL, &timeout) <= 0) {
//        return ;
//    }
//}
//
//static inline const void delayMicroTime(uint32_t microseconds)
//{
//    struct timeval timeout          ;
//    timeout.tv_sec  = 0             ;
//    timeout.tv_usec = microseconds  ;
//    if (select(0, NULL, NULL, NULL, &timeout) <= 0) {
//        return ;
//    }
//}


int setupSocket( char * Addr, int port, int makeNonBlocking)
{
    int newSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (newSocket < 0)
    {
        return newSocket;
    }

    const int reuseFlag = 1;
    if (setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuseFlag, sizeof reuseFlag) < 0)
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
    if (setsockopt(newSocket, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuseFlag, sizeof reuseFlag) < 0)
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
        name.sin_port = htons(port);
        if (Addr == NULL)
                name.sin_addr.s_addr = INADDR_ANY;
        else    name.sin_addr.s_addr = inet_addr(Addr);

        if (bind(newSocket, (struct sockaddr*)&name, sizeof name) != 0)
        {
            char tmpBuffer[100];
            snprintf(tmpBuffer, sizeof(tmpBuffer), "bind() error (port number: %d): ", ntohs(port));
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

unsigned getBufferSize(int bufOptName, int socket)
{
    unsigned curSize;
    socklen_t sizeSize = sizeof curSize;
    if (getsockopt(socket, SOL_SOCKET, bufOptName, (char*)&curSize, &sizeSize) < 0)
    {
        return 0;
    }

    return curSize;
}

unsigned increaseBufferTo(int bufOptName, int socket, unsigned requestedSize) {
    // First, get the current buffer size.  If it's already at least
    // as big as what we're requesting, do nothing.
    unsigned curSize = getBufferSize(bufOptName, socket);

    // Next, try to increase the buffer to the requested size,
    // or to some smaller size, if that's not possible:
    while (requestedSize > curSize)
    {
        socklen_t sizeSize = sizeof requestedSize;
        if (setsockopt(socket, SOL_SOCKET, bufOptName, (char*)&requestedSize, sizeSize) >= 0) {
            // success
            return requestedSize;
        }
        requestedSize = (requestedSize+curSize)/2;
    }
    return getBufferSize(bufOptName, socket);
}

unsigned increaseSendBufferTo(int socket, unsigned requestedSize) {
    return increaseBufferTo(SO_SNDBUF, socket, requestedSize);
}
unsigned increaseRecvBufferTo(int socket, unsigned requestedSize) {
    return increaseBufferTo(SO_RCVBUF, socket, requestedSize);
}

unsigned zeroSendBufferTo(int socket) {
    unsigned requestedSize = 0;
    socklen_t sizeSize = sizeof requestedSize;
    setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char*)&requestedSize, sizeSize);
    return getBufferSize(SO_SNDBUF, socket);
}

static const inline int8_t sendKeepData( int socket )
{
//    if (socket <= 0) return -1;
//    int unsent, tot ;
//    socklen_t ilen = sizeof (int);
//    if (getsockopt (socket, SOL_SOCKET, SO_SNDBUF, (char*)&tot, &ilen) < 0); // Send Buffer총용양 조사
//      return -1;
//    ioctl (socket, TIOCOUTQ, &unsent); // Send Buffer에 쌓여있는 양 조사
//    return (tot - unsent);  // Send Free Buffer 양 계산

    if (socket <= 0) return 0;
    int keepdata = 0;
    if (!ioctl(socket, TIOCOUTQ, &keepdata)) // Send Buffer에 쌓여있는 양 조사
    {
        if (keepdata < 120000) return 0;
        if (keepdata < 170000) return 1;
        if (keepdata < 220000) return 2;
    }
    return 3;
}

int SendStreamL(int socket, char * buffer, int bufferSize, int8_t & ret, bool * IsClose)
{
    ret = sendKeepData(socket);
    if (ret == 3)         return -5;
    if (bufferSize  <= 0) return -4;
    int bytesSend    = 0;
    int totBytesSend = 0;
    do  {
        bytesSend = send (socket, buffer + totBytesSend, bufferSize-totBytesSend, 0);
        if (bytesSend < 0)
        {
            if(errno != EAGAIN)
            {
                pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] sending failed !! (result = %d)[%d][%s]",
                                        __FUNCTION__, __LINE__, bytesSend, errno, strerror(errno));
                return -2   ;
            }
            if (!(*IsClose))
            {
                pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] immediate stop !!! (result = %d)[%d][%s]",
                                        __FUNCTION__, __LINE__, bytesSend, errno, strerror(errno));
                return -9;
            }
            SDL_Delay(_SLEEP_TIME_);
        }   else
        {
            totBytesSend += bytesSend;
        }
    }   while (bufferSize != totBytesSend);
    return totBytesSend;
}

int SendStreamL(int socket, char * buffer, int bufferSize, bool * IsClose)
{
    //int8_t ret = sendKeepData(socket);
    //if (ret == 3)         return -5;
    if (bufferSize  <= 0) return -4;
    int bytesSend    = 0;
    int totBytesSend = 0;
    do  {
        bytesSend = send (socket, buffer + totBytesSend, bufferSize-totBytesSend, 0);
        if (bytesSend < 0)
        {
            if(errno != EAGAIN)
            {
                pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] sending failed !! (result = %d)[%d][%s]",
                                        __FUNCTION__, __LINE__, bytesSend, errno, strerror(errno));
                return -2   ;
            }
            if (!(*IsClose))
            {
                pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] immediate stop !!! (result = %d)[%d][%s]",
                                        __FUNCTION__, __LINE__, bytesSend, errno, strerror(errno));
                return -9;
            }
            SDL_Delay(_SLEEP_TIME_);
        }   else
        {
            totBytesSend += bytesSend;
        }
    }   while (bufferSize != totBytesSend);
    return totBytesSend;
}

//static const inline Timestamp TimeToVal(struct timeval* pTimeval) {
//    return ((uint64_t)pTimeval->tv_sec * TimestampTicks) + pTimeval->tv_usec;
//}
//
//static const inline Timestamp getTimeDiff(struct timeval* pTimeval) {
//    struct timeval tv;
//    gettimeofday(&tv, NULL);
//    return TimeToVal(&tv) - TimeToVal(pTimeval);
//}

int SendSocketExact(int socket, char * buffer, int bufferSize, bool * IsClose)
{
    if (bufferSize  <= 0) return -4;
    int bytesSend    = 0;
    int totBytesSend = 0;
    uint64_t TickCount = 0;

    do  {
        bytesSend = send (socket, buffer + totBytesSend, bufferSize-totBytesSend, 0);
        if (bytesSend < 0)
        {
            if(errno != EAGAIN)
            {
                pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] sending failed !! (result = %d)[%d][%s]",
                                        __FUNCTION__, __LINE__, bytesSend, errno, strerror(errno));
                return -2   ;
            }
            if (!(*IsClose))
            {
                pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] immediate stop !!! (result = %d)[%d][%s]",
                                        __FUNCTION__, __LINE__, bytesSend, errno, strerror(errno));
                return -9;
            }

            SDL_Delay(_SLEEP_TIME_)  ;
            TickCount += _SLEEP_TIME_;
            if (TickCount > _SEND_COUNT_)
            {
                pLog.Print(LOG_WARNING, 0, NULL, 0, "%s()[%d] try to sending data, but timeout %.1f second (%d/%d bytes)!!",
                                      __FUNCTION__, __LINE__, ((double)_SEND_COUNT_/(double)_TIME2SLEEP_), totBytesSend, bufferSize);
                return -3   ;
            }
        }   else
        {
            totBytesSend += bytesSend;
        }
    }   while (bufferSize != totBytesSend);
    return totBytesSend;
}

int SendSocketExact(int socket, char * buffer, int bufferSize)
{
    if (bufferSize  <= 0) return -4;
    int bytesSend    = 0;
    int totBytesSend = 0;
    uint64_t TickCount = 0;

    do  {
        bytesSend = send (socket, buffer + totBytesSend, bufferSize-totBytesSend, 0);
        if (bytesSend < 0)
        {
            if(errno != EAGAIN)
            {
                pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] sending failed !! (result = %d)[%d][%s]",
                                        __FUNCTION__, __LINE__, bytesSend, errno, strerror(errno));
                return -2   ;
            }

            SDL_Delay(_SLEEP_TIME_)  ;
            TickCount += _SLEEP_TIME_;
            if (TickCount > _SEND_COUNT_)
            {
                pLog.Print(LOG_WARNING, 0, NULL, 0, "%s()[%d] try to sending data, but timeout %.1f second (%d/%d bytes)!!",
                                      __FUNCTION__, __LINE__, ((double)_SEND_COUNT_/(double)_TIME2SLEEP_), totBytesSend, bufferSize);
                return -3   ;
            }
        }   else
        {
            totBytesSend += bytesSend;
        }
    }   while (bufferSize != totBytesSend);
    return totBytesSend;
}

int SendSocketExact(int socket, char * buffer, int bufferSize, uint64_t SecCount)
{
    if (bufferSize  <= 0) return -4;
    int bytesSend    = 0;
    int totBytesSend = 0;
    uint64_t TickCount = 0;

    do  {
        bytesSend = send (socket, buffer + totBytesSend, bufferSize-totBytesSend, 0);
        if (bytesSend < 0)
        {
            if(errno != EAGAIN)
            {
                pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] sending failed !! (result = %d)[%d][%s]",
                                        __FUNCTION__, __LINE__, bytesSend, errno, strerror(errno));
                return -2   ;
            }

            SDL_Delay(_SLEEP_TIME_)  ;
            TickCount += _SLEEP_TIME_;
            if (TickCount > SecCount)
            {
                pLog.Print(LOG_WARNING, 0, NULL, 0, "%s()[%d] try to sending data, but timeout %.1f second (%d/%d bytes)!!",
                                      __FUNCTION__, __LINE__, ((double)SecCount/(double)_TIME2SLEEP_), totBytesSend, bufferSize);
                return -3   ;
            }
        }   else
        {
            totBytesSend += bytesSend;
        }
    }   while (bufferSize != totBytesSend);
    return totBytesSend;
}

int getData(int socket, char * buffer, int bufferSize)
{
    if (bufferSize <= 0) return -4;
    while(true)
    {
        int readBytes = recv(socket, buffer, bufferSize, 0);

        // 읽을것이 없거나 에러
        if (readBytes < 0)
        {
            // read 했으나 읽을것이 없음 if(errno == EWOULDBLOCK) or 약 5초가 지났거나..
            if (EAGAIN == errno)
            {
                SDL_Delay(_SLEEP_TIME_);
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


//#ifdef BLOCKING
//int getData(int socket, char * buffer, int bufferSize, uint64_t seconds)
//{
//    if (bufferSize <= 0) return -4;
//    while(true)
//    {
//        int readBytes = recv(socket, buffer, bufferSize, 0);
//        // 읽을것이 없거나 에러
//        if (readBytes < 0)
//        {
//            // read 했으나 읽을것이 없음 if(errno == EWOULDBLOCK)
//            if (EAGAIN == errno)
//            {
//                return -3   ;
//            }   else
//            {
//                return -2   ;
//            }
//        }   else
//        {
//            //클라이언트 연결종료
//            if( readBytes == 0)
//            {
//                return -1;
//            }
//            return readBytes;
//        }
//    }
//}
//#else
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
//#endif

int getData(int socket, char * buffer, int bufferSize, uint64_t seconds, bool * Stop)
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
                if (TickCount > seconds || (!*Stop))
                {
                    if (!*Stop) return -9;
                    return -3;
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

int getDataWait(int socket, char * buffer, int bufferSize, uint64_t seconds, bool * Stop)
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
                SDL_Delay(_DELAY_WAIT_)  ;
                TickCount += _DELAY_WAIT_;
                if (TickCount > seconds || (!*Stop))
                {
                    if (!*Stop) return -9;
                    return -3;
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


int getDataExact( int socket, char * buffer, int bufferSize)
{
    if (bufferSize < 0) return -4;
    int bsize        = bufferSize;
    int bytesRead    = 0;
    int totBytesRead = 0;
    while (bsize != 0)  {
        bytesRead = getData(socket, buffer + totBytesRead, bsize);
        if (bytesRead < 0) return bytesRead;
        totBytesRead += bytesRead   ;
        bsize -= bytesRead  ;
    }

    return totBytesRead;
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

int getDataExact( int socket, char * buffer, int bufferSize, uint64_t seconds, bool * Stop)
{
    if (bufferSize < 0) return -4;
    int bsize        = bufferSize;
    int bytesRead    = 0;
    int totBytesRead = 0;
    while (bsize != 0)  {
        bytesRead = getData(socket, buffer + totBytesRead, bsize, seconds, Stop);
        if (bytesRead < 0) return bytesRead;
        totBytesRead += bytesRead   ;
        bsize -= bytesRead  ;
    }

//    do  {
//        bytesRead = getData(socket, buffer + totBytesRead, bsize, seconds, Stop);
//        if (bytesRead < 0) return bytesRead;
//        totBytesRead += bytesRead   ;
//        bsize -= bytesRead  ;
//    }   while (bsize != 0)  ;

    return totBytesRead;
}

int getRead(int socket, char * buffer, int bufferSize)
{
    if (bufferSize <= 0) return -4;
    while(true)
    {
        int readBytes = read(socket, buffer, bufferSize);

        // 읽을것이 없거나 에러
        if (readBytes < 0)
        {
            // read 했으나 읽을것이 없음 if(errno == EWOULDBLOCK)
            if (EAGAIN != errno)
            {
                pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] read failed !! (result = %d)[%d][%s]",
                            __FUNCTION__, __LINE__, readBytes, errno, strerror(errno));
                return -2   ;
            }
        }   else
        {
            return readBytes;
        }
    }
}

int  setNonBlock (int sock)
{
    int curFlags = fcntl(sock, F_GETFL, 0);
    if (fcntl(sock, F_SETFL, curFlags|O_NONBLOCK) < 0)
    {
        perror("Socket SetNonBlock Error\n");
        return -1;
    }

    return 0;
}

int setLinger (int sock)
{
    struct linger ling;
    ling.l_onoff  = 1;
    ling.l_linger = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_LINGER, &ling, sizeof ling) < 0)
    {
        close(sock);
        return -1;
    }
    return 0;
}

int setRcvTimeo(int sock, int seconds)
{
    struct timeval tv = {seconds, 0};
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (struct timeval*)&tv, sizeof(struct timeval)) < 0)
    {
        close(sock);
        return -1;
    }
    return 0;
}

bool socketclose (int sock)
{
    if (sock > 0)
    {
        shutdown(sock, SHUT_WR);
        if (close(sock) < 0)
        {
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] \x1b[1;31msocket close error (%s)\x1b[0m", __FUNCTION__, __LINE__, strerror(errno));
            return false;
        }

        //int code = shutdown(sock, SHUT_RDWR);
        //if( code != 0 ) //SOCKET_ERROR )
        //{
        //    //pLog.Print(LOG_DEBUG, 0, NULL, 0, "%s()[%d] \x1b[1;31mInvalid Socket Close (%s) !!!!\x1b[0m", __FUNCTION__, __LINE__, strerror(errno));
        //    fd_set readfds;
        //    fd_set errorfds;
        //    timeval timeout = {1,0};
        //
        //    FD_ZERO(&readfds );
        //    FD_ZERO(&errorfds);
        //    FD_SET ( sock, &readfds );
        //    FD_SET ( sock, &errorfds);
        //
        //    select( 1, &readfds, NULL, &errorfds, &timeout );
        //}
        //code = close( sock );
        //sock = -1;
    }
    return true;
}

void getRecvClear(int sock, uint64_t seconds)
{
    char tmpBuf[TMP_BUF] = {0,};
    uint64_t TickCount = 0;
    int  bytesRead = 0;
    do  {
        bytesRead = recv(sock, tmpBuf, TMP_BUF, 0);
        if (bytesRead < 0)
        {
            if (EAGAIN == errno)
            {
                SDL_Delay(_DELAY_WAIT_)  ;
                TickCount += _DELAY_WAIT_;
                if (TickCount > seconds)
                    break;
            }   else
                break;
        }   else
        {
            if (bytesRead == 0)
                break;
        }
    }   while (true);
    return;
}

void clearSocket(int sock)
{
    if (sock > 0)
    {
        //GLock.lock();
        socketclose (sock);
        //GLock.unlock();
    }
}

void epClose(ClntInfo * si, bool clear)
{
    if (si->fd > 0)
    {
        //GLock.lock();
        //if (clear)
        //    getRecvClear(si->fd);
        Delete_fd(si->epfd, si->fd);
        if (socketclose(si->fd))
            si->fd = -1;
        //GLock.unlock();
    }
}

void epCloseN(ClntInfo * si, bool clear)
{
    if (si->fd > 0)
    {
        Delete_fd(si->epfd, si->fd);
        if (close(si->fd) < 0)
            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] \x1b[1;31msocket close error (%s)\x1b[0m", __FUNCTION__, __LINE__, strerror(errno));
        else
            si->fd = -1;
    }
}

char * getError (int Errno, int Timeout)
{
    static char msgStr[MIN_BUF];
    if (Errno == -1)
        snprintf(msgStr, sizeof(msgStr), "\x1b[1;33msocket closed\x1b[0m")  ;
    else
    if (Errno == -2)
        snprintf(msgStr, sizeof(msgStr), "\x1b[1;33msocket error [%s]\x1b[0m", strerror(errno));
    else
    if (Errno == -3)
        snprintf(msgStr, sizeof(msgStr), "\x1b[1;33mtimeout %.1f seconds\x1b[0m", ((double)Timeout/(double)_TIME2SLEEP_));
    else
    if (Errno == -4)
        snprintf(msgStr, sizeof(msgStr), "\x1b[1;33mbuffer size zero\x1b[0m");
    else
    if (Errno == -9)
        snprintf(msgStr, sizeof(msgStr), "\x1b[0;31mImmediate Stopped !!!\x1b[0m");
    else
        snprintf(msgStr, sizeof(msgStr), "\x1b[1;33munknown error [%s]\x1b[0m", strerror(errno));
    return msgStr   ;
}

/////////////////////////////////////////////////////////////////////////////////////////
int ConnectWithTimeout(int fd,struct sockaddr *remote, int len, int secs, int *err)
{
    int saveflags,ret,back_err;
    fd_set fd_w;
    struct timeval timeout = {secs, 0};

    saveflags=fcntl(fd,F_GETFL,0);
    if(saveflags<0)
    {
        *err=errno;
        return -1;
    }

    /* Set non blocking */
    if(fcntl(fd,F_SETFL,saveflags|O_NONBLOCK)<0)
    {
        *err=errno;
        return -1;
    }

    /* This will return immediately */
    *err=connect(fd,remote,len);
    back_err=errno;

    /* restore flags */
    if(fcntl(fd,F_SETFL,saveflags)<0)
    {
        *err=errno;
        return -1;
    }

    /* return unless the connection was successful or the connect is
    still in progress. */
    if(*err<0 && back_err!=EINPROGRESS)
    {
        *err=errno;
        return -1;
    }

    FD_ZERO(&fd_w);
    FD_SET(fd,&fd_w);

    *err=select(FD_SETSIZE,NULL,&fd_w,NULL,&timeout);
    if(*err<0)
    {
        *err=errno;
        return -1;
    }

    /* 0 means it timeout out & no fds changed */
    if(*err==0)
    {
        *err=ETIMEDOUT;
        return -1;
    }

    /* Get the return code from the connect */
    //SOCKLEN_T addrsize = sizeof remote;
    socklen_t addrsize = sizeof remote;
    *err = getsockopt(fd,SOL_SOCKET,SO_ERROR,&ret,&addrsize);
    if(*err<0)
    {
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
//////////////////////////////////////////////////////////////////////////////////////////////

int RTrim(char * buffer, int buflen)
{
    int nlen = buflen;
    for (int i = buflen-1; i >= 0; i--)
    {
        if (buffer[i] != '\0') break;
        nlen -- ;
    }
    return nlen ;
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
        return ;
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

//int ConnectTimeout(int fd, struct sockaddr * remote, int len, int secs)
//{
//    int nResult;
//    fd_set fdRead, fdWrite, fdExcept;
//    struct timeval Wait;
//
//    fcntl(fd, F_SETFL, O_NONBLOCK);
//    nResult = connect(fd, remote, len);
//    if(nResult < 0)
//    {
//        if(errno != EINPROGRESS)
//        {
//            pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Conneced Error !! [%d][%s]", __FUNCTION__, __LINE__, errno, strerror(errno));
//            return -1;
//        }
//    }
//
//    FD_ZERO(&fdRead);
//    FD_SET (fd,&fdRead);
//    fdExcept = fdWrite = fdRead;
//
//    Wait.tv_sec  = secs ;
//    Wait.tv_usec = 0    ;
//
//    nResult = select(fd+1, &fdRead, &fdWrite, &fdExcept, &Wait);
//    if(nResult < 0)
//    {
//        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] Conneced Error !! [%d][%s]", __FUNCTION__, __LINE__, errno, strerror(errno));
//        return -1;
//    }
//    else if(nResult == 0)
//    {
//        pLog.Print(LOG_ERROR, 0, NULL, 0, "%s()[%d] \x1b[1;33mConneced Timeout !!\x1b[0m [%d][%s]", __FUNCTION__, __LINE__, errno, strerror(errno));
//        return -1;
//    }
//
//    if(FD_ISSET(fd, &fdRead) || FD_ISSET(fd, &fdWrite) || FD_ISSET(fd, &fdExcept))
//    {
//        return 0;
//    }
//    return -1;
//}


//int getDataExact( int socket, char * buffer, unsigned bufferSize, struct sockaddr_in& fromAddress, int seconds)
//{
//    int bsize        = bufferSize;
//    int bytesRead    = 0;
//    int totBytesRead = 0;
//    do  {
//        bytesRead = getData(socket, buffer + totBytesRead, bsize, fromAddress, seconds);
//        if (bytesRead < 0) return bytesRead;
//        totBytesRead += bytesRead   ;
//        bsize -= bytesRead  ;
//    }   while (bsize != 0)  ;
//
//    return totBytesRead;
//}

//int getData(int socket, char * buffer, unsigned bufferSize, struct sockaddr_in& fromAddress, int seconds)
//{
//    while(true)
//    {
//        struct timeval tv   ;
//        tv.tv_sec  = seconds;
//        tv.tv_usec = 0      ;
//        int readBytes = RecvSocket ( socket, buffer, bufferSize, fromAddress, &tv );
//
//      // 읽을것이 없거나 에러
//      if (readBytes < 0)
//      {
//          if (EAGAIN == errno )     // read 했으나 읽을것이 없음
//          {
//                usleep(1000);
//              continue    ;
//          }   else    {
//                return -2   ;
//            }
//      }
//
//      //클라이언트 연결종료
//      if( readBytes == 0) return -1;
//        return readBytes;
//    }
//}

//int UntilReadable( int socket, struct timeval* timeout )
//{
//    int result = -1;
//    do  {
//        fd_set rd_set;
//        FD_ZERO(&rd_set);
//        if (socket < 0) break;
//        FD_SET((unsigned) socket, &rd_set);
//        const unsigned numFds = socket+1;
//
//        result = select(numFds, &rd_set, NULL, NULL, timeout);
//        if (timeout != NULL && result == 0)
//        {
//            break; // this is OK - timeout occurred
//        } else if (result <= 0) {
//            break;
//        }
//
//        if (!FD_ISSET(socket, &rd_set)) {
//            break;
//        }
//    }   while (0);
//
//    return result;
//}


//int RecvSocket(int socket, char * buffer, unsigned bufferSize, struct sockaddr_in& fromAddress, struct timeval* timeout)
//{
//    int bytesRead = -1;
//    do  {
//        int result = UntilReadable(socket, timeout);
//        if (timeout != NULL && result == 0)
//        {
//            bytesRead = 0;
//            break;
//        }   else if (result <= 0) {
//            break;
//        }
//
//      socklen_t addressSize = sizeof fromAddress;
//      bytesRead = recvfrom(socket, (char*)buffer, bufferSize, 0, (struct sockaddr*)&fromAddress, &addressSize);
//
//        if (bytesRead < 0) {
//            break;
//        }
//    }   while (0);
//
//    return bytesRead;
//}
//
//int RecvSocket(int socket, char * buffer, unsigned bufferSize, struct timeval* timeout)
//{
//    int bytesRead = -1;
//    do  {
//        int result = UntilReadable(socket, timeout);
//        if (timeout != NULL && result == 0)
//        {
//            bytesRead = 0;
//            break;
//        }   else if (result <= 0) {
//            break;
//        }
//
//        bytesRead = recv(socket, buffer, bufferSize, 0);
//        if (bytesRead < 0) {
//            break;
//        }
//    }   while (0);
//
//    return bytesRead;
//}

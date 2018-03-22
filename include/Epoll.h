//--------------------------------------------------------------------------------------------------------------
/** @file       Epoll.h
    @date   2011/07/20
    @author 송봉수(sbs@gabia.com)
    @brief  Epoll 클래스 구조 정의 \n
*///------------------------------------------------------------------------------------------------------------
#ifndef __HVAAS_EPOLL_H__
#define __HVAAS_EPOLL_H__

/*  INCLUDE  */
#include <iostream>
#include <sys/epoll.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <list>
#include <map>
#include <string>
using namespace std;

/*  TYPE DEF  */
typedef struct
{
    int                        fd;
    int                        epfd;
    struct sockaddr_in  clientaddr;
}ClntInfo;


/*  FUNCTION PROTYPE  */
int Delete_fd(int efd, int cfd);
int Modify_fd(int efd, int cfd);


/*  CLASS DEFINE  */
//--------------------------------------------------------------------------------------------------------------
/** @class  CEpoll
    @brief  Epoll 클래스 구조 정의 \n
    @remark \n
                사용법\n
                CEpoll   Epoll; \n
                Epoll.EpStart(int iListenFD, int psize, int epsize); \n
                Epoll.EpWait(); \n
*///------------------------------------------------------------------------------------------------------------
class CEpoll
{
private:
        int                 m_epFD;         ///< Epoll  FD
        int                 m_psize;            ///< 풀크기
        int                 m_iListenFD;        ///< 서버 리슨 소켓
        int                 m_epsize;           ///< 이풀크기 (이풀생성시 커널에서 임의로 할당해주는 크기를 설정하기위함)
        int                 m_getEvnet;     ///< 획득이벤트
        int                 m_useOneshot;
        int                 m_IsNonblock;       ///< 소켓 블록킹모드
        int                 m_IsTCPNodelay;     ///< 소켓 TCPNoDelay
        struct              epoll_event *events;        ///<이벤트

        list<int>                   m_eventFD;
        list<ClntInfo>          m_event_Address;
        map<int, ClntInfo>  m_address_map;
        pthread_mutex_t     m_mutex;


public:
    CEpoll();
    ~CEpoll();

    int         SockOpt(int sock);
    int         Accept(int iListenFD);
    int         Add_fd(int cfd);
    int                 Add_Listenfd(int listenfd);
    int         Get_epFD();
    int         Use_fd(struct epoll_event ev);

    int         EpStart(int sListenFD, int psize, int epsize, int useOneshot);          ///< Epoll 초기화, 등록
    list<int>       * EpWait();                                                 ///< Epoll 서버소켓 리슨 시작
    list<ClntInfo> * EpWait_getaddr(int milliseconds = -1);
    list<ClntInfo> * EpWait_getaddr_ex(int milliseconds = -1);

    void            SetSockBlockMode(bool blocking);
    void            SetSockTCPNoDelay(bool nodelay);
};


#endif







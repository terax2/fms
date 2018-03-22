//--------------------------------------------------------------------------------------------------------------
/** @file       Epoll.h
    @date   2011/07/20
    @author �ۺ���(sbs@gabia.com)
    @brief  Epoll Ŭ���� ���� ���� \n
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
    @brief  Epoll Ŭ���� ���� ���� \n
    @remark \n
                ����\n
                CEpoll   Epoll; \n
                Epoll.EpStart(int iListenFD, int psize, int epsize); \n
                Epoll.EpWait(); \n
*///------------------------------------------------------------------------------------------------------------
class CEpoll
{
private:
        int                 m_epFD;         ///< Epoll  FD
        int                 m_psize;            ///< Ǯũ��
        int                 m_iListenFD;        ///< ���� ���� ����
        int                 m_epsize;           ///< ��Ǯũ�� (��Ǯ������ Ŀ�ο��� ���Ƿ� �Ҵ����ִ� ũ�⸦ �����ϱ�����)
        int                 m_getEvnet;     ///< ȹ���̺�Ʈ
        int                 m_useOneshot;
        int                 m_IsNonblock;       ///< ���� ���ŷ���
        int                 m_IsTCPNodelay;     ///< ���� TCPNoDelay
        struct              epoll_event *events;        ///<�̺�Ʈ

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

    int         EpStart(int sListenFD, int psize, int epsize, int useOneshot);          ///< Epoll �ʱ�ȭ, ���
    list<int>       * EpWait();                                                 ///< Epoll �������� ���� ����
    list<ClntInfo> * EpWait_getaddr(int milliseconds = -1);
    list<ClntInfo> * EpWait_getaddr_ex(int milliseconds = -1);

    void            SetSockBlockMode(bool blocking);
    void            SetSockTCPNoDelay(bool nodelay);
};


#endif







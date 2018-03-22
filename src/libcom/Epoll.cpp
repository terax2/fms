//--------------------------------------------------------------------------------------------------------------
/** @file       Epoll.cpp
    @date   2011/07/20
    @author �ۺ���(sbs@gabia.com)
    @brief  Epoll Ŭ���� ���� ���� \n
*///------------------------------------------------------------------------------------------------------------

/*  INCLUDE  */
#include "Epoll.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CEpoll::CEpoll()
{
        m_IsNonblock        = 1;  // Default NonBlocking Mode
        m_IsTCPNodelay  = 1;  // Default TCPnoDeay Mode

        m_epFD              = 0;
        m_psize             = 0;
        m_epsize            = 0;
        m_getEvnet      = 0;
        events              = NULL;
        m_useOneshot        = 0;
        pthread_mutex_init(&m_mutex, NULL);

}


CEpoll::~CEpoll()
{
    if(events != NULL)
        free(events);
}


/**********************************************************************************************************
* IN : cfd= Ǯ�� ����� fd, oneshot=eolll oneshot ��� ���� (0=no, 1=oneshot)
* OUT : success=0, fail= -1
* Comment :
***********************************************************************************************************/
int CEpoll::Add_fd(int cfd)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events  = EPOLLIN | EPOLLET;

    if (m_useOneshot)
        ev.events |= EPOLLONESHOT;

    ev.data.fd = cfd;
    return epoll_ctl (m_epFD, EPOLL_CTL_ADD, cfd, &ev);
}



int CEpoll::Add_Listenfd(int listenfd)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events  = EPOLLIN;

    ev.data.fd = listenfd;
    return epoll_ctl (m_epFD, EPOLL_CTL_ADD, listenfd, &ev);
}





/******************************************************************************************************************
* IN : iListenFD=��������FD vector, psize=Ǯ������, epsize=��Ǯ������, useOneshot = oneshot ��� (1=���)
* OUT : success=0, fail=-1
* Comment : Epoll�ʱ�ȭ, oneshot��带 �����Ͽ��� ListenFD�� ��������ʴ´�.(Ŭ���̾�Ʈ FD�� ����)
*******************************************************************************************************************/
int CEpoll::EpStart(int iListen, int psize, int epsize, int useOneshot)
{

    m_psize         =        psize;
    m_epsize        =        epsize;
    m_iListenFD =        iListen;

//  printf("iListen=%d, psize=%d, epsize=%d, useOneshot=%d\n", iListen, psize, epsize, useOneshot);

    //Epoll Create
    if ((m_epFD = epoll_create (m_epsize)) < 0)
        return -1;

    // Set Epoll Event
    events =  (struct epoll_event *) malloc (sizeof(*events) *m_psize);
    if(events == NULL)
         return -1;

    if(Add_Listenfd(m_iListenFD) < 0 )
        return -1;

    if(useOneshot)
        m_useOneshot = 1;

    return 0;
}



/**********************************************************************************************************
* IN : sock = �ɼ����� ����fd
* OUT : success=0, fail=-1
* Comment :���Ͽ� nonblock, nodelay �ɼ�����
***********************************************************************************************************/
int CEpoll::SockOpt(int sock)
{
    if(m_IsNonblock == 1)
    {
        int flags = fcntl (sock, F_GETFL);
        flags |= O_NONBLOCK;

        if (fcntl (sock, F_SETFL, flags) < 0)
        {
            perror("Socket SetNonBlock Error\n");
            return -1;
        }
    }

    if(m_IsTCPNodelay == 1)
    {
        int flag = 1;
        int result = setsockopt (sock,  IPPROTO_TCP, TCP_NODELAY, (char *) &flag,  sizeof (int));
        return result;
    }

    return 0;
}




/**********************************************************************************************************
* IN : ilistenFD= ���� ���� fd
* OUT : ������ Ŭ���̾�Ʈ fd
* Comment : �����ؿ� Ŭ���̾�Ʈ accept �ϰ� ���Ͽɼ�, Ǯ�� fd���
***********************************************************************************************************/
int CEpoll::Accept(int iListenFD)
{
    int cfd;
    struct sockaddr_in  clientaddr;
    int clilen = sizeof(clientaddr);

    cfd = accept (iListenFD, (struct sockaddr *) &clientaddr,(socklen_t *) &clilen);

    if( cfd < 0 )
    {
        perror("Epoll Accept Error");
        return -1;
    }

    // ���Ͽɼ� ���� tcp nodelay, nonblock
    if( SockOpt(cfd) < 0 )
    {
        perror("Epoll Set Option Error");
        return -1;
    }

    // fd���
    if( Add_fd(cfd) < 0 )
    {
        perror("Epoll Add FD Error");
        return -1;
    }

    return cfd;
}


/**********************************************************************************************************
* IN : none
* OUT : m_epFD
* Comment : m_epFD ����
***********************************************************************************************************/
int CEpoll::Get_epFD()
{
    return m_epFD;
}



/**********************************************************************************************************
* IN : none
* OUT : success = 0, fail=-1
* Comment : Epoll �̺�Ʈ Ž���ϸ� ���
***********************************************************************************************************/
list<int> *CEpoll::EpWait()
{

    int i;
    m_eventFD.clear();

    m_getEvnet = epoll_wait (m_epFD, events, m_psize, -1);

//  if ( m_getEvnet = -1 )
    //  perror("Epoll wait Error");

//  printf("m_getEvnet >>>> [ %d ]\n", m_getEvnet);
    for (i = 0; i < m_getEvnet; i++)
    {
        if(events[i].data.fd == m_iListenFD)
        {
    //  printf("========== EPoll New Accept !! ==========\n");
            if( Accept(m_iListenFD) < 0 )
                perror("Epoll Accept Error");
        }
        else
        {
    //      printf("========== EPoll Recv Event !! ==========\n");
        //  if( Use_fd (events[i]) < 0)
        //      perror("Epoll Recv Message Error");

             m_eventFD.push_back(events[i].data.fd);
        }
    }

    return &m_eventFD;
}


/**********************************************************************************************************
* IN : none
* OUT : success = 0, fail=-1
* Comment : Epoll �̺�Ʈ Ž���ϸ� ���, event FD, Address ����ü ����Ʈ ����
***********************************************************************************************************/
list<ClntInfo> *CEpoll::EpWait_getaddr(int milliseconds)
{
    int i;
    int cfd;
    struct sockaddr_in  clientaddr;
    int clilen = sizeof(clientaddr);

    ClntInfo cinfo;
    m_event_Address.clear();
    m_getEvnet = epoll_wait (m_epFD, events, m_psize, milliseconds);


    for (i = 0; i < m_getEvnet; i++)
    {
        if(events[i].data.fd == m_iListenFD)
        {
//          printf("========== EPoll New Accept !!  ==========\n");
            cfd = accept (m_iListenFD, (struct sockaddr *) &clientaddr,(socklen_t *) &clilen);
//          printf("cfd[%d] addr[%s]\n", cfd , inet_ntoa(clientaddr.sin_addr));

            if( cfd < 0 )   {
                perror("Epoll Accept Error");
            }

            // ���Ͽɼ� ���� tcp nodelay, nonblock
            if( SockOpt(cfd) < 0 ){
                perror("Epoll Set Option Error");
            }

            // fd���
            if( Add_fd(cfd) < 0 ){
                perror("Epoll Add FD Error");
            }

            // insert map
            memset(&cinfo, 0, sizeof(ClntInfo));
            cinfo.fd = cfd;
            cinfo.epfd = m_epFD;
            cinfo.clientaddr = clientaddr;
    //      memcpy(cinfo.addr, inet_ntoa(clientaddr.sin_addr), sizeof(cinfo.addr));

            pthread_mutex_lock(&m_mutex);
            m_address_map[cfd] = cinfo;
            pthread_mutex_unlock(&m_mutex);
        }
        else  // not ListenFD
        {
//           printf("========== EPoll Recv Event !! ==========\n");
             map<int, ClntInfo>::iterator it;
             it = m_address_map.find(events[i].data.fd);

//          printf("cfd[%d] addr[%s]\n", events[i].data.fd , inet_ntoa(clientaddr.sin_addr));

            if( it == m_address_map.end() )
            {
                 perror("FD Not exist Fd Map");
            }
            else
            {
                m_event_Address.push_back( it->second );
            }
        }   // end of else not ListenFD
    }

    return &m_event_Address;
}

/**********************************************************************************************************
* IN : none
* OUT : success = 0, fail=-1
* Comment : Epoll �̺�Ʈ Ž���ϸ� ���, event FD, Address ����ü ����Ʈ ����
***********************************************************************************************************/
list<ClntInfo> *CEpoll::EpWait_getaddr_ex(int milliseconds)
{
    int i;
    int cfd;
    struct sockaddr_in  clientaddr;
    int clilen = sizeof(clientaddr);

    ClntInfo cinfo;
    m_event_Address.clear();
    m_getEvnet = epoll_wait (m_epFD, events, m_psize, milliseconds);


    for (i = 0; i < m_getEvnet; i++)
    {
        if(events[i].data.fd == m_iListenFD)
        {
//          printf("========== EPoll New Accept !!  ==========\n");
            cfd = accept (m_iListenFD, (struct sockaddr *) &clientaddr,(socklen_t *) &clilen);
//          printf("cfd[%d] addr[%s]\n", cfd , inet_ntoa(clientaddr.sin_addr));

            if( cfd < 0 )   {
                perror("Epoll Accept Error");
            }

            // ���Ͽɼ� ���� tcp nodelay, nonblock
            if( SockOpt(cfd) < 0 ){
                perror("Epoll Set Option Error");
            }

            // fd���
            if( Add_fd(cfd) < 0 ){
                perror("Epoll Add FD Error");
            }

            // insert map
            memset(&cinfo, 0, sizeof(ClntInfo));
            cinfo.fd = cfd;
            cinfo.epfd = m_epFD;
            cinfo.clientaddr = clientaddr;

            m_event_Address.push_back(cinfo);
        }
    }

    return &m_event_Address;
}


/**********************************************************************************************************
* IN : �̺�Ʈ�� �߻��� epoll_event
* OUT : success=0, fail=-1
* Comment : ��ϵǾ��ִ� Ŭ���̾�Ʈ�κ����� �޽���ó��
***********************************************************************************************************/
int CEpoll::Use_fd(struct epoll_event ev)
{
    //���⿡�� �ٷ� �޽��� ����
    int readn;
//  int sendflags = 0;
    char buf_in[65535] = { '\0' };
    int cfd = ev.data.fd;

    int sizen = 0;
    char readbuf[10240] = { '\0' };

    while(1)
    {
        readn = read (cfd, readbuf, sizeof(readbuf));

        // �������� ���ų� ����
        if (readn <0 )
        {
             if (EAGAIN == errno )    // read ������ �������� ����
                 continue;
            else
                perror("Epoll Socket Read Error something === TODO: ERROR CHECK !!!!!!!!!!\n");

            if( Delete_fd(m_epFD, cfd) < 0)
                perror("Delete Epoll FD ERROR");

            close (cfd);
            break;
        }

        //Ŭ���̾�Ʈ ��������
        if( readn  == 0 )
        {
        //  printf("Client [%d] Disconnected\n", cfd);

            if(Delete_fd(m_epFD, cfd) < 0)
                printf("Delete Epoll FD Error [%d]\n", cfd);

            close (cfd);
            break;
        }

        // ���� read
    //  sizen += readn;
        if (sizen >= 65535)
        {
            if(Delete_fd(m_epFD, cfd) < 0)
                printf("Delete Epoll FD Error [%d]\n", cfd);

            close (cfd);
            printf ("Close buffer full fd %d by %d\n", cfd, readn);
            return -1;
        }

        memcpy(buf_in, readbuf, readn);
    //  printf( "Recv Message from[%d] %d, %s\n", cfd, sizen, buf_in);
    //  SysLog.Print(LOG_DEBUG, 0, NULL, 0, "Recv Message from[%d] %d, %s", cfd, readn, buf_in);

    //  if( Modify_fd(m_epFD, cfd) < 0)
    //      perror("Modify Epoll FD ERROR");

    //  break;
    }

    return 0;
}




/**********************************************************************************************************
* IN :  efd = EpollFD, cfd = ClientFD
* OUT : success=0, fail= -1
* Comment : Ǯ�� ��ϵ� fd����
***********************************************************************************************************/
int Delete_fd(int efd, int cfd)
{
  struct epoll_event ev;
  return epoll_ctl (efd, EPOLL_CTL_DEL, cfd, &ev);
}


/**********************************************************************************************************
* IN : efd = EpollFD, cfd = ClientFD
* OUT : success=0, fail= -1
* Comment : Ǯ�� ��ϵ� fd ����
***********************************************************************************************************/
int Modify_fd(int efd, int cfd)
{
  struct epoll_event ev;
  ev.events  = EPOLLIN | EPOLLET | EPOLLONESHOT;
  ev.data.fd = cfd;

  return epoll_ctl (efd, EPOLL_CTL_MOD, cfd, &ev);
}



void CEpoll::SetSockBlockMode(bool blocking)
{
    if( blocking == true )
        m_IsNonblock    = 1;
    else if( blocking == false )
        m_IsNonblock    = 0;
}


void CEpoll::SetSockTCPNoDelay(bool nodelay)
{
    if( nodelay == true )
        m_IsTCPNodelay  = 1;
    else if( nodelay == false )
        m_IsTCPNodelay  = 0;
}


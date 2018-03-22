
/*  INCLUDE  */
#include "Epoll.h"


CEpoll::CEpoll()
{

}


CEpoll::~CEpoll()
{

}



/**********************************************************************************************************
* IN : 
* OUT : 
* Comment : 
***********************************************************************************************************/
int CEpoll::InitEpll()
{
	//Epoll Create
	if ((m_epFD = epoll_create (m_epsize)) < 0) 
		return -1;

	// Set Epoll Event 
	events =  (struct epoll_event *) malloc (sizeof(*events) *m_psize); 
	if(events == NULL)
		 return -1;


	return 0;
}



/**********************************************************************************************************
* IN : 
* OUT : 
* Comment : 
***********************************************************************************************************/
int CEpoll::EpStart(int epFD, int psize, int epsize)
{

	m_psize	 =		 psize;
	m_epsize =		 epsize;
	m_epFD	 =		 epFD;

	printf("listenFD=%d, psize=%d,  epsize=%d\n", m_epFD, m_psize, m_epsize);

	if(InitEpll() < 0)
	{
		perror("Epoll Create Error");
		return -1;
	}



}




/*
CEpoll::CEpoll(const int size)
{
	is_epoll_init = (fd_epoll = epoll_create(size)) > 0;
}


CEpoll::~CEpoll()
{
	if(is_epoll_init){
		close(fd_epoll);
	}
}


int CEpoll::Add(const int fd)
{
	struct epoll_event even;

	even.events = EPOLLIN | EPOLLERR | EPOLLET;
	even.data.fd = fd;
	
	return epoll_ctl(fd_epoll, EPOLL_CTL_ADD, fd, &even);
}


int CEpoll::Delete(const int fd)
{
	struct epoll_event even;

	even.events = EPOLLIN | EPOLLERR;
	even.data.fd = fd;

	return epoll_ctl(fd_epoll, EPOLL_CTL_DEL, fd, &even);
}


int CEpoll::Wait(pEpollEvent events, int max, const int nwait)
{	
	return epoll_wait(fd_epoll, events, max, nwait);
}
*/


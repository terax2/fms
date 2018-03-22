/***********************************************************
* Filename   : Epoll.h
* Developer  : sbs@gabia.co.kr
* Start Data : 2011/06/06
* Comment    : Epoll
*************************************************************/

/*  INCLUDE  */
#include <iostream>
#include <sys/epoll.h>
#include <errno.h> 

/*  DEFINE  */
#define MAX_ADDR		50


/*  STRUCT TYPE  */



/*  CLASS DEFINE  */

class CEpoll
{
private:
		int				m_epFD;
		int				m_psize;
		int				m_epsize;

		struct			epoll_event *events; 
		struct			epoll_event ev; 

public:
	CEpoll();
	~CEpoll();

	int InitEpll();


	int EpStart(int epFD, int psize, int epsize);
};







/*
class CEpoll
{
private:
	int fd_epoll;

public:
	bool is_epoll_init;

	CEpoll(const int size);
	~CEpoll();

    int Add(const int fd);
	int Delete(const int fd);
	int Wait(pEpollEvent events, int max, const int nwait); 
};
*/


















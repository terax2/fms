

/*  Include  */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>	
#include <stdarg.h>		
#include <arpa/inet.h>	 
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <netinet/tcp.h> 
#include <sys/resource.h>
#include <sys/stat.h>
#include <pthread.h>
#include <time.h>


/*  define  */
#define BUFSIZE	 102400
#define	SA	struct sockaddr
#define CLIENT_NUM	300


typedef struct 
{
	char *serv_Addr;
	int serv_Port;
	int thread_id;
	int sock;
}_Clients;

/*  function  */

/**************************************************************************************************
* IN : 최대클라이언트수
* OUT : 현재설정된 최대파일수
* Comment : 최대 클라이언트수로 열수있는 파일숫자 설정
**************************************************************************************************/
int SetLimit(const unsigned nmax)
{
	struct rlimit rlim;
	getrlimit(RLIMIT_NOFILE, &rlim);
	rlim.rlim_max = nmax;
	rlim.rlim_cur = nmax;

	setrlimit(RLIMIT_NOFILE, &rlim);
	getrlimit(RLIMIT_NOFILE, &rlim);
	
	return rlim.rlim_cur;
}



/**********************************************************************************************
* IN : socket fd
* OUT : Success:0, False:-1
* Comment : SetNonBlock
**********************************************************************************************/
int SetNonblock(int sock)
{
	int flags = fcntl (sock, F_GETFL); 
	flags |= O_NONBLOCK; 

	if (fcntl (sock, F_SETFL, flags) < 0) 
    {
		printf("Socket SetNonBlock Error\n");
		return -1; 
    } 
	return 0; 
}


/**********************************************************************************************
* IN : socket fd
* OUT : Success:0, False:-1
* Comment : SetReuseaddr
**********************************************************************************************/
int SetReuseaddr(int sock)
{
	int flag = 1; 
	int result = setsockopt (sock,  SOL_SOCKET, SO_REUSEADDR, (char *) &flag, sizeof (int)); 
	return result; 
}


/**********************************************************************************************
* IN :
* OUT : 
* Comment : 
**********************************************************************************************/
int connect_nonb(int sockfd, const SA *saptr, socklen_t salen, int nsec)
{
    int             flags, n, error;
    socklen_t       len;
    fd_set          rset, wset;
    struct timeval  tval;


	flags = fcntl (sockfd, F_GETFL); 
	flags |= O_NONBLOCK; 

	if (fcntl (sockfd, F_SETFL, flags) < 0) 
    {
		perror("Socket SetNonBlock Error\n");
		return -1; 
    } 


    error = 0;
    if ( (n = connect(sockfd, (struct sockaddr *) saptr, salen)) < 0)
		if (errno != EINPROGRESS)
		{
			perror("Connect Error");
			return(-1);
		}
         
    /* Do whatever we want while the connect is taking place. */
    if (n == 0)
        goto done;  /* connect completed immediately */

    FD_ZERO(&rset);
    FD_SET(sockfd, &rset);
    wset = rset;
    tval.tv_sec = nsec;
    tval.tv_usec = 0;

    if ( (n = select(sockfd+1, &rset, &wset, NULL,  nsec ? &tval : NULL)) == 0) 
	{
        close(sockfd);      /* timeout */
        errno = ETIMEDOUT;
        return(-1);
    }

    if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset)) 
	{
        len = sizeof(error);
        if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
            return(-1);         /* Solaris pending error */
    } else
        perror("Select error: sockfd not set");

done:
    fcntl(sockfd, F_SETFL, flags);  /* restore file status flags */

    if (error) 
	{
        close(sockfd);      /* just in case */
        errno = error;
        return(-1);
    }
    return(0);
}


/**********************************************************************************************
* IN :
* OUT : 
* Comment : 
*
*********************************************************************************************/
void *do_connect(void *data)
{

    char buf[BUFSIZE];
	int readbytes;
	int len;
	int read_len;
	_Clients cInfo;
	cInfo = *((_Clients *) data);

	struct sockaddr_in s_addr;
	memset(&s_addr, 0, sizeof(s_addr));
	s_addr.sin_family = AF_INET;
	s_addr.sin_addr.s_addr = inet_addr(cInfo.serv_Addr);
	s_addr.sin_port = htons(cInfo.serv_Port);

	while(1)
	{
		cInfo.sock = socket(AF_INET, SOCK_STREAM, 0);
		if(cInfo.sock == -1)
			perror("Create Socket");

		if( SetReuseaddr(cInfo.sock) < 0 )
			perror("SetReuseAddr Error");

		if( connect_nonb(cInfo.sock, (struct sockaddr*)&s_addr, sizeof(s_addr), 5) < 0 )
			perror("Connect Error");		
	
	    memset(&buf, 0, sizeof(buf));
		sprintf(buf, "[Send]from Client Message");

		len = strlen(buf);
		write(cInfo.sock, buf, len);

	    memset(&buf, 0, sizeof(buf));

	
		while(1)
		{
			read_len = read (cInfo.sock, buf, sizeof(buf));
			
			if (read_len > 0)
			{
				printf("[Recv] : %s\n", buf);
				break;
			}
		}
	
		close(cInfo.sock);
	//	sleep(1);
	}
}




/*---------------------------------------------------------------
  function : 
  io: serverIP, serverPort
  desc: 
----------------------------------------------------------------*/

int main(int argc, char *argv[])
{

//Network
	int serv_port;
	char *serv_addr;
	int j=0;
	_Clients Clients[CLIENT_NUM];
	memset( Clients, 0, sizeof(Clients));

//		Get Addr, Port		//
	if (argc < 3) {
		fprintf(stderr, "usage: %s ServIP PORT \n", argv[0]);
		exit(2);
	  }else{
		  serv_addr = argv[1];
		  serv_port = atoi(argv[2]);
	  }


// Set Limit fd
	SetLimit(20000);
	  


/*
//pThread test
	int thr_id;
	pthread_t p_thread[CLIENT_NUM];
	
	for(j=0; j<CLIENT_NUM; j++)
	{
		Clients[j].thread_id = j;
		Clients[j].serv_Addr = serv_addr;
		Clients[j].serv_Port = serv_port;

		thr_id = pthread_create(&p_thread[j], NULL, do_connect, (void *)&Clients[j]);
		if(	thr_id < 0)
			printf("Create Thread Error");

		pthread_detach(p_thread[j]);
	}

*/



// single Test
    char buf[BUFSIZE];
	int readbytes;
	int len;
	int c_skt;
	int ii =0;
	int read_len;

	struct sockaddr_in s_addr;
	memset(&s_addr, 0, sizeof(s_addr));
	s_addr.sin_family = AF_INET;
	s_addr.sin_addr.s_addr = inet_addr(serv_addr);
	s_addr.sin_port = htons(serv_port);

	c_skt = socket(AF_INET, SOCK_STREAM, 0);
	if(c_skt == -1)
		perror("Create Socket");

	if( SetReuseaddr(c_skt) < 0 )
		perror("SetReuseAddr Error");

	if( connect_nonb(c_skt, (struct sockaddr*)&s_addr, sizeof(s_addr), 5) < 0 )
		perror("Connect Error");		
	 
	while(1)
	{
	  memset(&buf, 0, sizeof(buf));
	  sprintf(buf, "Iam Client ===============================  %d", ii);

      len = strlen(buf);
	  write(c_skt, buf, len);
	  ii++;

	  memset(&buf, 0, sizeof(buf));
	  while(1)
	  {
			read_len = read (c_skt, buf, sizeof(buf));
			
			if (read_len > 0)
			{
				printf("[Recv] : %s\n", buf);
				break;
			}
	  }
	
//	 usleep(100);
	sleep(1);
	}
// single test




	while(1)  //just loop
	{
		sleep(10);
	}

	return 0;
}






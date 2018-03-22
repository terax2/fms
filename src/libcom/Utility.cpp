//--------------------------------------------------------------------------------------------------------------
/** @file		Utility.cpp
	@date	2011/05/18
	@author 송봉수(sbs@gabia.com)
	@brief	서버에서 사용하는 유틸리티모음   \n
*///------------------------------------------------------------------------------------------------------------

/*  INCLUDE  */
#include "Utility.h"

#include <iostream>
#include <signal.h>
#include <string.h>
#include <sys/resource.h>

#include <assert.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <arpa/inet.h>


/*  FUNCTION  */

/**********************************************************************************************
* IN : 데몬모드로의 동작여부 True/False
* OUT : bool True 
* Comment : 데몬모드로 동작 데몬모드로 동작, 작업디렉토리 변경해줄것
**********************************************************************************************/
bool DemonMode(bool isDemon)
{
	if(isDemon){
		if(fork() != 0){
			exit(0);
		}

		setsid();
		signal(SIGHUP, SIG_IGN);  //단말종료시 종료무시

		if(fork() != 0){
			exit(1);
		}
		return true;
	}
	return false;
}




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

void SetStackSize (const unsigned nsize)
{
	// set rlimit 
	struct rlimit rlim;
	rlim.rlim_cur = rlim.rlim_max = (nsize * 1024) ;
	int error = setrlimit(RLIMIT_STACK,&rlim);
	if (error)
	{
		fprintf(stderr, "Error !!! RLIMIT_STACK Setting Fail\n\n");
		return ;
	}
	error = getrlimit(RLIMIT_STACK,&rlim);
	if (error)  {
		fprintf(stderr, "Error !!! RLIMIT_STACK Getting Fail\n\n");
	}
	else    
	{
	//	fprintf(stderr, "RLIMIT_STACK = cur[%d] max[%d]\n", rlim.rlim_cur, rlim.rlim_max);
	}
    
    return ;
}



/**************************************************************************************************
* IN : 생성할 디렉토리의 full pathname
* OUT : 성공: 0, 이미디렉토리가존재:0, 실패 -1, 
	       디렉토리 full pathname 중간에 디렉토리가 아닌 항목이 포함된 경우, -1 을 리턴
* Comment : 디렉토리를 생성
**************************************************************************************************/
int CDirectory::Create( const char * szDirName )
{
	int		i, iLen, iCount, n;
	char	* pszName;
	char	fisNotDirectory = 0;	// 디렉토리가 아닌가?
	char	fisError = 0;			// 디렉토리 생성시에 에러가 발생하였는가?

	iLen = (int)strlen( szDirName );
	pszName = new char[ iLen + 1 ];

	memset( pszName, 0, iLen + 1 );
	for( i = 0, iCount = 0; i < iLen; i++ )
	{
		if( szDirName[i] == DIR_SEP )
		{
			iCount++;

			// 디렉토리 이름이 "c:\test\temp\" 또는 "/test/temp/" 이므로 두번째 디렉토리
			// 구분자 부터 디렉토리를 생성하면 된다.
			if( iCount >= 2 )
			{
				n = CDirectory::IsDirectory( pszName );
				if( n == -1 )
				{
					// 디렉토리가 아닌 경우
					fisNotDirectory = 1;
					break;
				}
				else if( n == -2 )
				{
					// 디렉토리가 존재하지 않는 경우
					if( mkdir( pszName, DIR_MODE ) != 0 )
					{
						fisError = 1;
						break;
					}
				}
			}
		}

		pszName[i] = szDirName[i];
	}

	delete [] pszName;

	if( fisNotDirectory == 1 ) return -1;
	if( fisError == 1 ) return -1;

	// 디렉토리 이름이 "c:\test\temp" 또는 "/test/temp" 일 경우, 위의 loop 에서 temp 디렉토리를 
	// 생성하지 않으므로 이를 생성하기 위해서 아래의 코드가 필요하다.
	if( szDirName[iLen-1] != DIR_SEP )
	{
		n = CDirectory::IsDirectory( szDirName );
		if( n == -1 )
		{
			// 디렉토리가 아닌 경우
			return -1;
		}
		else if( n == -2 )
		{
			// 디렉토리가 존재하지 않는 경우
			if( mkdir( szDirName, DIR_MODE ) != 0 )
			{
				return -1;
			}
		}
	}

	return 0;
}



/**************************************************************************************************
* IN : 디렉토리 이름
* OUT : 입력된 path 가 디렉토리이면 0 을 리턴,
	       존재하지 않으면 -2 을 리턴, 디렉토리가 아니면 -1 을 리턴
* Comment : 입력한 path 가 디렉토리인지를 점검한다
**************************************************************************************************/
int CDirectory::IsDirectory( const char * szDirName )
{

	struct stat		clsStat;

	if( stat( szDirName, &clsStat ) == 0 )
	{
		if( S_ISDIR( clsStat.st_mode ) ) 
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else
	{
		//존재하지 않는 경우.
		return -2;	
	}

	return 0;
}


/******  kde 2012.01.10 코드 추가.... Start  ******/
/**************************************************************************************************
* IN  : 맥어드레스 버퍼..
* OUT : 정상 0, 에러 -1
* Comment : System 맥 어드레스 얻어오기..
**************************************************************************************************/
static long mac_addr_sys ( u_char *addr )
{
    struct ifreq ifr;
    struct ifreq *IFR;
    struct ifconf ifc;
    char buf[1024];
    int s, i;
    int ok = 0;

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s==-1) {
        return -1;
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    ioctl(s, SIOCGIFCONF, &ifc);
 
    IFR = ifc.ifc_req;
    for (i = ifc.ifc_len / sizeof(struct ifreq); --i >= 0; IFR++) {

        strcpy(ifr.ifr_name, IFR->ifr_name);
        if (ioctl(s, SIOCGIFFLAGS, &ifr) == 0) {
            if (! (ifr.ifr_flags & IFF_LOOPBACK)) {
                if (ioctl(s, SIOCGIFHWADDR, &ifr) == 0) {
                    ok = 1;
                    break;
                }
            }
        }
    }

    close(s);
    if (ok) 	{
        bcopy( ifr.ifr_hwaddr.sa_data, addr, 6);
    }	else	{
        return -1;
    }
    return 0;
}

static unsigned char basis_64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int base64encode(const unsigned char *input, int input_length, unsigned char *output, int output_length)
{
	int	i = 0, j = 0;
	int	pad;

	assert(output_length >= (input_length * 4 / 3));

	while (i < input_length) {
		pad = 3 - (input_length - i);
		if (pad == 2) {
			output[j  ] = basis_64[input[i]>>2];
			output[j+1] = basis_64[(input[i] & 0x03) << 4];
			output[j+2] = '=';
			output[j+3] = '=';
		} else if (pad == 1) {
			output[j  ] = basis_64[input[i]>>2];
			output[j+1] = basis_64[((input[i] & 0x03) << 4) | ((input[i+1] & 0xf0) >> 4)];
			output[j+2] = basis_64[(input[i+1] & 0x0f) << 2];
			output[j+3] = '=';
		} else{
			output[j  ] = basis_64[input[i]>>2];
			output[j+1] = basis_64[((input[i] & 0x03) << 4) | ((input[i+1] & 0xf0) >> 4)];
			output[j+2] = basis_64[((input[i+1] & 0x0f) << 2) | ((input[i+2] & 0xc0) >> 6)];
			output[j+3] = basis_64[input[i+2] & 0x3f];
		}
		i += 3;
		j += 4;
	}
	return j;
}

/**************************************************************************************************
* IN  : 라이센스 파일명..
* OUT : 정상 true, 에러 false
* Comment : 프로그램 유효성 체크..
**************************************************************************************************/
bool ValidCheckProgram(const char * checkname)
{
	FILE  	  *	fid			;
	long		stat		;
	u_char		key = 0xAE	;
	u_char		macaddr[ 6 ];
	char        MacAddr[100];
	char        OutAddr[100];
	char        EncAddr[100];
    
    if ( access (checkname, F_OK) )
    {
        fprintf(stderr, "%s not exist !!\n", checkname);
        return false ;
    }

	fid	= fopen(checkname, "rb");
	if ( fid != NULL )	
	{
  		fseek (fid, 0, SEEK_END);
  		int size = ftell (fid);
  		fseek (fid, 0, SEEK_SET);
  		
        memset(EncAddr, 0, sizeof(EncAddr));
  		if (fread(EncAddr, 2, size, fid) <= 0)	
  		{
			fclose(fid)     ;
			return false    ;
		}
		fclose(fid);

	    stat = mac_addr_sys(macaddr);
	    if (0 == stat) {
	    	snprintf(MacAddr, sizeof(MacAddr), "@%02X!%02X(%02X$%02X^%02X*%02X#", 
	    	    macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
	    }	else	{
	        fprintf(stderr, "mac address get failed !!!\n");
	        return false;
	    }

        int iLen = base64encode((unsigned char*)MacAddr, strlen(MacAddr), (unsigned char*)OutAddr, sizeof(OutAddr));
  		for ( int i = 0; i < iLen; i ++ )
  		    OutAddr[i] = OutAddr[i] ^ key   ;

        if ( !memcmp (EncAddr, OutAddr, iLen) )
            return true ;
        else
            return false;

	}	else	{
		return false;
	}
}

/******  kde 2012.01.10 코드 추가.... End  ******/

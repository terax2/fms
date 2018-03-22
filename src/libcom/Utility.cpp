//--------------------------------------------------------------------------------------------------------------
/** @file		Utility.cpp
	@date	2011/05/18
	@author �ۺ���(sbs@gabia.com)
	@brief	�������� ����ϴ� ��ƿ��Ƽ����   \n
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
* IN : ��������� ���ۿ��� True/False
* OUT : bool True 
* Comment : ������� ���� ������� ����, �۾����丮 �������ٰ�
**********************************************************************************************/
bool DemonMode(bool isDemon)
{
	if(isDemon){
		if(fork() != 0){
			exit(0);
		}

		setsid();
		signal(SIGHUP, SIG_IGN);  //�ܸ������ ���ṫ��

		if(fork() != 0){
			exit(1);
		}
		return true;
	}
	return false;
}




/**************************************************************************************************
* IN : �ִ�Ŭ���̾�Ʈ��
* OUT : ���缳���� �ִ����ϼ�
* Comment : �ִ� Ŭ���̾�Ʈ���� �����ִ� ���ϼ��� ����
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
* IN : ������ ���丮�� full pathname
* OUT : ����: 0, �̵̹��丮������:0, ���� -1, 
	       ���丮 full pathname �߰��� ���丮�� �ƴ� �׸��� ���Ե� ���, -1 �� ����
* Comment : ���丮�� ����
**************************************************************************************************/
int CDirectory::Create( const char * szDirName )
{
	int		i, iLen, iCount, n;
	char	* pszName;
	char	fisNotDirectory = 0;	// ���丮�� �ƴѰ�?
	char	fisError = 0;			// ���丮 �����ÿ� ������ �߻��Ͽ��°�?

	iLen = (int)strlen( szDirName );
	pszName = new char[ iLen + 1 ];

	memset( pszName, 0, iLen + 1 );
	for( i = 0, iCount = 0; i < iLen; i++ )
	{
		if( szDirName[i] == DIR_SEP )
		{
			iCount++;

			// ���丮 �̸��� "c:\test\temp\" �Ǵ� "/test/temp/" �̹Ƿ� �ι�° ���丮
			// ������ ���� ���丮�� �����ϸ� �ȴ�.
			if( iCount >= 2 )
			{
				n = CDirectory::IsDirectory( pszName );
				if( n == -1 )
				{
					// ���丮�� �ƴ� ���
					fisNotDirectory = 1;
					break;
				}
				else if( n == -2 )
				{
					// ���丮�� �������� �ʴ� ���
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

	// ���丮 �̸��� "c:\test\temp" �Ǵ� "/test/temp" �� ���, ���� loop ���� temp ���丮�� 
	// �������� �����Ƿ� �̸� �����ϱ� ���ؼ� �Ʒ��� �ڵ尡 �ʿ��ϴ�.
	if( szDirName[iLen-1] != DIR_SEP )
	{
		n = CDirectory::IsDirectory( szDirName );
		if( n == -1 )
		{
			// ���丮�� �ƴ� ���
			return -1;
		}
		else if( n == -2 )
		{
			// ���丮�� �������� �ʴ� ���
			if( mkdir( szDirName, DIR_MODE ) != 0 )
			{
				return -1;
			}
		}
	}

	return 0;
}



/**************************************************************************************************
* IN : ���丮 �̸�
* OUT : �Էµ� path �� ���丮�̸� 0 �� ����,
	       �������� ������ -2 �� ����, ���丮�� �ƴϸ� -1 �� ����
* Comment : �Է��� path �� ���丮������ �����Ѵ�
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
		//�������� �ʴ� ���.
		return -2;	
	}

	return 0;
}


/******  kde 2012.01.10 �ڵ� �߰�.... Start  ******/
/**************************************************************************************************
* IN  : �ƾ�巹�� ����..
* OUT : ���� 0, ���� -1
* Comment : System �� ��巹�� ������..
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
* IN  : ���̼��� ���ϸ�..
* OUT : ���� true, ���� false
* Comment : ���α׷� ��ȿ�� üũ..
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

/******  kde 2012.01.10 �ڵ� �߰�.... End  ******/

//--------------------------------------------------------------------------------------------------------------
/** @file		Utility.h 
	@date	2011/05/18
	@author 송봉수(sbs@gabia.com)
	@brief	서버에서 사용하는 유틸리티모음   \n
*///------------------------------------------------------------------------------------------------------------
#ifndef __HVAAS_UTILITY_H__
#define __HVAAS_UTILITY_H__


/*  INCLUDE  */
#include <sys/stat.h>



/*  DEFINE  */
# define DIR_SEP	'/'
# define DIR_MODE		S_IRWXU


/*  FUNCTION PROTYPE  */
bool DemonMode(bool isDemon);			//데몬프로세스 모드로 실행
int SetLimit(const unsigned nmax);    //시스템 최대 fd 설정
void SetStackSize (const unsigned nsize);

bool ValidCheckProgram(const char * checkname);

/*  CLASS DEFINE  */
//입력한 경로대로 디렉토리를 생성
class CDirectory
{
public:
	static int Create( const char * szDirName );  //디렉토리생성
	static int IsDirectory( const char * szDirName );  //디렉토리인지 검사
};

#endif


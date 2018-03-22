//--------------------------------------------------------------------------------------------------------------
/** @file		Utility.h 
	@date	2011/05/18
	@author �ۺ���(sbs@gabia.com)
	@brief	�������� ����ϴ� ��ƿ��Ƽ����   \n
*///------------------------------------------------------------------------------------------------------------
#ifndef __HVAAS_UTILITY_H__
#define __HVAAS_UTILITY_H__


/*  INCLUDE  */
#include <sys/stat.h>



/*  DEFINE  */
# define DIR_SEP	'/'
# define DIR_MODE		S_IRWXU


/*  FUNCTION PROTYPE  */
bool DemonMode(bool isDemon);			//�������μ��� ���� ����
int SetLimit(const unsigned nmax);    //�ý��� �ִ� fd ����
void SetStackSize (const unsigned nsize);

bool ValidCheckProgram(const char * checkname);

/*  CLASS DEFINE  */
//�Է��� ��δ�� ���丮�� ����
class CDirectory
{
public:
	static int Create( const char * szDirName );  //���丮����
	static int IsDirectory( const char * szDirName );  //���丮���� �˻�
};

#endif


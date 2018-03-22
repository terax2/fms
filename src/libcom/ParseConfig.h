//--------------------------------------------------------------------------------------------------------------
/** @file		ParseConfig.h
	@date	2011/05/11
	@author 송봉수(sbs@gabia.com)
	@brief	환경설정파일 Parsing 메소드 클래스.   \n
*///------------------------------------------------------------------------------------------------------------

#ifndef __HVAAS_PARSE_CONFIG_H__
#define __HVAAS_PARSE_CONFIG_H__


/*  INCLUDE  */
#include <iostream>


/*  DEFINE  */
#define MAX_KEY_SIZE 255
#define MAX_VALUE_SIZE 1024


/*  CLASS DEFINE  */
class CParseConfig{
protected:
	FILE			*configFileFd;

public: 
	CParseConfig();
	~CParseConfig();
	bool Open(const char *configfile);      // 환경설정파일열기
	bool Close();									// 파일닫음
	bool GetInt(const char *psection, const char *pkey, int *pivalue, int idefault);		//정수형 값을 가져옴
	bool GetString(const char *psection, const char *pkey, char *psvalue, int slen, const char * sdefault = NULL);		//문자열값을가져옴
	bool TrimString(char *szName);						//앞뒤공백제거
};

#endif


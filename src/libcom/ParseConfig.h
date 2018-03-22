//--------------------------------------------------------------------------------------------------------------
/** @file		ParseConfig.h
	@date	2011/05/11
	@author �ۺ���(sbs@gabia.com)
	@brief	ȯ�漳������ Parsing �޼ҵ� Ŭ����.   \n
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
	bool Open(const char *configfile);      // ȯ�漳�����Ͽ���
	bool Close();									// ���ϴ���
	bool GetInt(const char *psection, const char *pkey, int *pivalue, int idefault);		//������ ���� ������
	bool GetString(const char *psection, const char *pkey, char *psvalue, int slen, const char * sdefault = NULL);		//���ڿ�����������
	bool TrimString(char *szName);						//�յڰ�������
};

#endif


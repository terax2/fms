//--------------------------------------------------------------------------------------------------------------
/** @file		ParseConfig.cpp
	@date	2011/05/11
	@author �ۺ���(sbs@gabia.com)
	@brief	ȯ�漳������ Parsing �޼ҵ� Ŭ����.   \n
*///------------------------------------------------------------------------------------------------------------


/*  INCLUDE  */
#include "ParseConfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*  CLASS  */
CParseConfig::CParseConfig()
{
	configFileFd = NULL;
}

CParseConfig::~CParseConfig()
{
	if(configFileFd){
		fclose(configFileFd);
	}
}

//--------------------------------------------------------------------------------------------------------------
/** @brief	ȯ�漳������ ����
	@param configfile : ȯ�漳�� �����̸�
	@return bool True/False : ����/����
	@remark ������ �̹� �����ְų� ���� ������ False ����
*///------------------------------------------------------------------------------------------------------------
bool CParseConfig::Open(const char *configfile)
{
	if(configFileFd){
//		std::cerr << "[Error]File Already Open" << std::endl;
		return false;
	}else{
		configFileFd = fopen(configfile, "r");
	}

	if(configFileFd == NULL){
//		std::cerr << "[Error]File Open Fail" << std::endl;
		return false;
	}
	return true;
}


//--------------------------------------------------------------------------------------------------------------
/** @brief	ȯ�漳������ �ݱ�
	@param configfile : ȯ�漳�� �����̸�
	@return bool True/False : ����/����
*///------------------------------------------------------------------------------------------------------------
bool CParseConfig::Close()
{
	if(this -> configFileFd)
	{
		fclose(this->configFileFd);
		this->configFileFd = NULL;
	}else{
//		std::cerr << "[Error]File Not Open" << std::endl;
		return false;
	}
	return true;
}


/*********************************************************************************************
* IN : section : ���ǹ��ڿ�
         key : Ű���ڿ�
		 pivalue : ���� ������ �����ͺ���
		 idefault : �⺻��, default = "0"
* OUT : bool True/False 
* Comment : ȯ�漳�����Ͽ��� ���������� ������, ��ã�� ��� ����Ʈ�� ����
*********************************************************************************************/
bool CParseConfig::GetInt(const char *psection, const char *pkey, int *pivalue, int idefault)
{

	char	svalueBuf[11];
	memset( svalueBuf, 0, sizeof(svalueBuf));

	if(!GetString(psection, pkey, svalueBuf, sizeof(svalueBuf)) == 0)
	{
		*pivalue = atoi(svalueBuf);
	}
	else
	{
		*pivalue = idefault;
	}

	return true;
}


/*********************************************************************************************
* IN :  section : ���ǹ��ڿ�
		 key : Ű���ڿ�
		 psvalue : ���� ������ ������ ����
		 sdefatult : �⺻��, ����Ʈ�ʱ�ȭ "0"
		 slen : psvalue�� ũ��
* OUT : find success=True,  Ste default value=False 
* Comment : ȯ�漳�����Ͽ��� ���ڿ����� ������, ��ã�� ��� ����Ʈ�� ����
*                 Ű(255) + ��(1024)�ʰ��� fgets sizeof(szBuf) ���ۿ����÷ο�  //ParseConfig.h �� Define
*                  # ,  // �ּ�ó�� 
*********************************************************************************************/
bool CParseConfig::GetString(const char *psection, const char *pkey, char *psvalue, int slen, const char *sdefault)
{

	char szBuf[MAX_KEY_SIZE+MAX_VALUE_SIZE];
	char skeyBuf[MAX_KEY_SIZE];
	char svalueBuf[MAX_VALUE_SIZE];
	char svalueBuf_removeComment[MAX_VALUE_SIZE];

	char *psEnd, *psSep, *psComment, *psSharp;
	char fisSectionStart =0, fisFound =0;
	int iLen =0;

	if(this->configFileFd == NULL){
		return false;
	}

	memset(szBuf, 0, sizeof(szBuf));
	fseek(configFileFd, 0, SEEK_SET);

	while(fgets(szBuf, sizeof(szBuf), this->configFileFd)){
		iLen = (int)strlen(szBuf);

		// \r\n ���๮�ڸ� \0 ���� �ٲ�
		if(iLen > 2 && szBuf[iLen-2] == '\r' && szBuf[iLen-1] == '\n')
		{
			szBuf[iLen-2] = '\0';
			iLen = iLen-2;
		}
		else if( iLen > 1 && szBuf[iLen-1] == '\n' )
		{
			szBuf[iLen-1] = '\0';
			iLen = iLen-1;
		}

		// �ּ� �н�
		if(iLen > 0 && szBuf[0] == '#') continue;
		if(iLen > 0 && szBuf[0] == '/') continue;

		// ���� ���ڿ������� �˻��Ѵ� [section]
		if(iLen > 0 && szBuf[0] == '[')
		{
			psEnd = strstr( szBuf, "]");

			if(psEnd)
			{
				memset(skeyBuf, 0, sizeof(skeyBuf));
				memcpy(skeyBuf, &szBuf[1], psEnd - &szBuf[1]);

				TrimString(skeyBuf);

				if( !strcmp(psection, skeyBuf))
				{
					fisSectionStart = 1;
				}
				else
				{
					fisSectionStart = 0;
				}
			}
		}
		else if(fisSectionStart == 1)
		{
			psSep = strstr( szBuf, "=" );

			if(psSep)
			{
				// Ű�� ���� �����Ѵ�.
				memset(skeyBuf, 0, sizeof(skeyBuf));
				memset(svalueBuf, 0, sizeof(svalueBuf));
				memset(svalueBuf_removeComment, 0, sizeof(svalueBuf_removeComment));

				memcpy(skeyBuf, &szBuf[0], psSep - &szBuf[0]);
				memcpy(svalueBuf, psSep + 1, iLen - (psSep - &szBuf[0] + 1));

				// ���ڿ� �ּ��� �ִ��� äũ (ServPort=9906 //������Ʈ) 
				psComment = strstr( svalueBuf, "//" );
				psSharp = strstr( svalueBuf, "#" );

				if(psComment)  // ���ڿ� �ּ��� ������ �ּ����������� �߶��� ��������
				{	
					memcpy(svalueBuf_removeComment, &svalueBuf[0], psComment - &svalueBuf[0]);
					TrimString(skeyBuf);
					TrimString(svalueBuf_removeComment);

					if( !strcmp(pkey, skeyBuf))
					{
						snprintf(psvalue, slen, "%s", svalueBuf_removeComment);
						fisFound = 1;
						break;
					}
				}
				else if (psSharp)
				{
					memcpy(svalueBuf_removeComment, &svalueBuf[0], psSharp - &svalueBuf[0]);
					TrimString(skeyBuf);
					TrimString(svalueBuf_removeComment);

					if( !strcmp(pkey, skeyBuf))
					{
						snprintf(psvalue, slen, "%s", svalueBuf_removeComment);
						fisFound = 1;
						break;
					}
				}
				else  // ���ڿ� �ּ��� ������ ��������
				{
					TrimString(skeyBuf);
					TrimString(svalueBuf);

					if( !strcmp(pkey, skeyBuf))
					{
						snprintf(psvalue, slen, "%s", svalueBuf);
						fisFound = 1;
						break;
					}
				}

			}
		} //end of elseif
	} //end of while

	//ã�����ϸ� ����Ʈ������ ä��
	if(fisFound == 0) 
	{
		snprintf(psvalue, slen, "%s", sdefault);
		return false;	
	}

	return true;
}



/*********************************************************************************************
* IN :  szName ���������� ���ڿ�
* OUT : success=True/False 
* Comment : �Է¹��� ���ڿ��� �յڰ����� ����
*********************************************************************************************/
bool CParseConfig::TrimString(char * szName)
{
	char	* pszBuf;
	char	fisStart;
	int		i, j, iLen;

	iLen = (int)strlen(szName);
	pszBuf = new char[iLen + 1];
	if(pszBuf == NULL)
	{
	    std::cerr << "[ERROR]CParseConfig::TrimString new Error" << std::endl;
		return false;
	}

	memset(pszBuf, 0, iLen + 1);

	//�հ�������
	for(fisStart = 1, i = 0, j = 0; i < iLen; i++)
	{
		if(fisStart == 1 && (szName[i] == ' ' || szName[i] == '\t' ))
		{
			continue;
		}
		else
		{
			fisStart = 0;
		}

		pszBuf[j++] = szName[i];
	}

	sprintf(szName, "%s", pszBuf);
	delete []pszBuf;

	//�ڰ�������
	iLen = (int)strlen( szName);
	for(fisStart = 1, i = iLen - 1; i >= 0; i--)
	{
		if(fisStart == 1 && ( szName[i] == ' ' || szName[i] == '\t'))
		{
			szName[i] = '\0';
		}
		else
		{
			break;
		}
	}

	return true;
}

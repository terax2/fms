//--------------------------------------------------------------------------------------------------------------
/** @file		ParseConfig.cpp
	@date	2011/05/11
	@author 송봉수(sbs@gabia.com)
	@brief	환경설정파일 Parsing 메소드 클래스.   \n
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
/** @brief	환경설정파일 열기
	@param configfile : 환경설정 파일이름
	@return bool True/False : 성공/실패
	@remark 파일이 이미 열려있거나 열수 없으면 False 리턴
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
/** @brief	환경설정파일 닫기
	@param configfile : 환경설정 파일이름
	@return bool True/False : 성공/실패
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
* IN : section : 섹션문자열
         key : 키문자열
		 pivalue : 값을 저장할 포인터변수
		 idefault : 기본값, default = "0"
* OUT : bool True/False 
* Comment : 환경설정파일에서 정수형값을 가져옴, 못찾을 경우 디폴트값 설정
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
* IN :  section : 섹션문자열
		 key : 키문자열
		 psvalue : 값을 저장할 포인터 변수
		 sdefatult : 기본값, 디폴트초기화 "0"
		 slen : psvalue의 크기
* OUT : find success=True,  Ste default value=False 
* Comment : 환경설정파일에서 문자열값을 가져옴, 못찾을 경우 디폴트값 설정
*                 키(255) + 값(1024)초과시 fgets sizeof(szBuf) 버퍼오버플로우  //ParseConfig.h 에 Define
*                  # ,  // 주석처리 
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

		// \r\n 개행문자를 \0 으로 바꿈
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

		// 주석 패스
		if(iLen > 0 && szBuf[0] == '#') continue;
		if(iLen > 0 && szBuf[0] == '/') continue;

		// 섹션 문자열인지를 검사한다 [section]
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
				// 키와 값을 추출한다.
				memset(skeyBuf, 0, sizeof(skeyBuf));
				memset(svalueBuf, 0, sizeof(svalueBuf));
				memset(svalueBuf_removeComment, 0, sizeof(svalueBuf_removeComment));

				memcpy(skeyBuf, &szBuf[0], psSep - &szBuf[0]);
				memcpy(svalueBuf, psSep + 1, iLen - (psSep - &szBuf[0] + 1));

				// 값뒤에 주석이 있는지 채크 (ServPort=9906 //서버포트) 
				psComment = strstr( svalueBuf, "//" );
				psSharp = strstr( svalueBuf, "#" );

				if(psComment)  // 값뒤에 주석이 있으면 주석이전까지만 잘라낸후 공백제거
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
				else  // 값뒤에 주석이 없으면 공백제거
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

	//찾지못하면 디폴트값으로 채움
	if(fisFound == 0) 
	{
		snprintf(psvalue, slen, "%s", sdefault);
		return false;	
	}

	return true;
}



/*********************************************************************************************
* IN :  szName 공백제거할 문자열
* OUT : success=True/False 
* Comment : 입력받은 문자열의 앞뒤공백을 제거
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

	//앞공백제거
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

	//뒤공백제거
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

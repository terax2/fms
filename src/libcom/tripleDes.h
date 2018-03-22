#ifndef __TRIPLEDES_H__
#define __TRIPLEDES_H__

/*  INCLUDE  */
#include "mzdes.h"

/*  TYPE DEFINE  */


/*  CLASS DEFINE  */
class CTripleDes
{
private:
	unsigned char s_user_key[ __def_mzapi_3des_user_key_size__ ];
	unsigned char s_round_key[ __def_mzapi_3des_round_key_size__ ]; /* (16 * 48 * 3) bytes */


private:
	void make_key();

public:
	char *user_input_key;
	void show_round_key();

public:	
	CTripleDes(char *user_key);
	~CTripleDes();

	void print_dump(char *s_title, void *s_data, int s_size);
	bool desEncrypt(char *s_data, int data_size, char *encrypt, int *encrypt_len);
	bool desDecrypt(char *encrypt, int encrypt_data_size);	

};

/*
// use 
	char *key = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	char ori_data[512];
	char encrypt_data[512];
	char decrypt_data[512];
	int encrypt_data_len;

	memcpy(ori_data, "12345678901234567890190123456789012345999", sizeof(ori_data));
	int ori_len = strlen(ori_data);

	CTripleDes tripleDes(key);
	tripleDes.desEncrypt(ori_data, ori_len, encrypt_data, &encrypt_data_len);
	tripleDes.desDecrypt(encrypt_data, encrypt_data_len);	

	memcpy(decrypt_data, encrypt_data, strlen(encrypt_data));
	printf("decrypt_data: %s\n\n\n", decrypt_data);
*/

#endif





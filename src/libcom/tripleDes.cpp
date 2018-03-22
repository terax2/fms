
/*  INCLUDE  */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "tripleDes.h"




CTripleDes::CTripleDes(char *user_key)
{
	user_input_key = user_key;
	make_key();
}


CTripleDes::~CTripleDes()
{

}


void CTripleDes::make_key()
{
	 (void)memset(&s_user_key[0], 0, sizeof(s_user_key));
	 (void)strncpy((char *)&s_user_key[0], user_input_key, sizeof(s_user_key));
//	  print_dump("\x1b[1;33muser key\x1b[0m", &s_user_key[0], sizeof(s_user_key));

     (void)mzapi_3des_make_round_key(&s_round_key[0], &s_user_key[0]);
//	 show_round_key();
}



/*  printf HEX code stdin */
void CTripleDes::print_dump(char *s_title, void *s_data, int s_size)
{
 int s_o,s_w,s_i;unsigned char s_b[17];
 (void)printf("%s:\n", s_title);

 s_b[16]='\0';s_o=(int)0;

 while(s_o<(s_size))
 {
	  s_w=((s_size)-s_o)<16?((s_size)-s_o):16;printf("%08X",s_o);for(s_i=0;s_i<s_w;s_i++){if(s_i==8)printf(" | ");else printf(" ");
	  s_b[s_i]=*(((unsigned char *)(s_data))+s_o+s_i);printf("%02X",s_b[s_i]);if((s_b[s_i]&0x80)||(s_b[s_i]<' '))s_b[s_i]='.';}
	  while(s_i<16){if(s_i==8)printf("     ");else printf("   ");s_b[s_i]=' ';s_i++;}
	  printf(" [%s]\n",(char *)s_b);s_o+=16;
  }

 (void)printf("\n");

}


void CTripleDes::show_round_key()
{
	do
	{
		int s_index, s_index0, s_index1, s_index2;
		(void)printf("round key:\n");

		for(s_index = 0;s_index < 3;s_index++)
		{
			for(s_index0 = 0;s_index0 < 16;s_index0++)
			{
				(void)printf("[%d][%2d] ", s_index, s_index0);

				for(s_index1 = 0;s_index1 < 48;s_index1 += 8)
				{
					for(s_index2 = 0;s_index2 < 8;s_index2++)
					{
						(void)printf("%d",	(unsigned int)s_round_key[(s_index * __def_mzapi_des_round_key_size__) + s_index0 + s_index1 + s_index2]);
					}
					(void)printf("b ");
				}
				(void)printf("\n");
			}
		}
		(void)printf("\n");

	}while(0);
}




/* encrypt 3-des */
bool CTripleDes::desEncrypt(char *s_data, int data_size, char *encrypt, int *encrypt_len)
{
//	fprintf(stderr, "Why not?\n");

	void *s_padding_data;
	int s_data_size;
	int s_padding_data_size;


	/* padding process */
//	s_data_size = sizeof(s_data);
	s_data_size = data_size;
	s_padding_data_size = s_data_size + (__def_mzapi_3des_block_size__ - 1);
	s_padding_data_size -= s_padding_data_size % __def_mzapi_3des_block_size__;


	s_padding_data = malloc(s_padding_data_size);
	if(s_padding_data == NULL)
	{
		(void)printf("allocate error !\n");
		return false;
	}

	(void)memcpy(s_padding_data, s_data, s_data_size);
	if((s_padding_data_size - s_data_size) > 0)
	{ /* zero padding */
		(void)memset(__mzapi_peek_f__(void *, s_padding_data, s_data_size), 0, s_padding_data_size - s_data_size);
	}
	
//	  printf("\x1b[1;33ms_data_size == %d\x1b[0m\n", s_data_size);
//    printf("\x1b[1;33ms_padding_data_size == %d\x1b[0m\n", s_padding_data_size);

/*
	print_dump("data", &s_data[0], s_data_size);	
	print_dump("encrpyt", mzapi_3des_encrypt(s_padding_data, s_padding_data_size, &s_round_key[0]),  (int)s_padding_data_size);
	print_dump("decrypt", mzapi_3des_decrypt(s_padding_data, s_padding_data_size, &s_round_key[0]),  (int)s_data_size);
*/

//	print_dump("\x1b[1;33mdata\x1b[0m", &s_data[0], s_data_size);	
//	print_dump("\x1b[1;33mencrypt\x1b[0m", mzapi_3des_encrypt(s_padding_data, s_padding_data_size, &s_round_key[0]),  (int)s_padding_data_size);

	mzapi_3des_encrypt(s_padding_data, s_padding_data_size, &s_round_key[0]);

	*encrypt_len = s_padding_data_size;
	memcpy(encrypt, s_padding_data, s_padding_data_size);	
	free((void *)s_padding_data);
	
	return true;
}




/* decrypt 3-des */
bool CTripleDes::desDecrypt(char *encrypt, int encrypt_data_size)
{
	mzapi_3des_decrypt(encrypt, encrypt_data_size, &s_round_key[0]);
//	print_dump("\x1b[1;33mdecrypt\x1b[0m", mzapi_3des_decrypt(encrypt, encrypt_data_size, &s_round_key[0]),  (int)encrypt_data_size);
	return true;
}




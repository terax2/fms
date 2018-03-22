/* Because MD5 may not be implemented (at least, with the same
 * interface) on all systems, we have our own copy here.
 */

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.
 */

//
#ifndef _OUR_MD5_H_
#define _OUR_MD5_H_

typedef unsigned int u32;
typedef unsigned char u8;

struct MD5Context 
{
	u32 buf[4];
	u32 bits[2];
	u8  in[64];
};
typedef struct MD5Context MD5_CTX;

class CMD5  
{
public:
	CMD5();	//default ctor
	virtual ~CMD5();
public:

	void MD5Init(struct MD5Context *context);
	void MD5Update(struct MD5Context *context, unsigned char const *buf, unsigned len);
	void MD5Final(unsigned char digest[16], struct MD5Context *context);
	void MD5Transform(u32 buf[4], u32 const in[16]);
	
	
};

#endif /* _OUR_MD5_H_ */

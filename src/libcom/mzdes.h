/*
 Copyright (C) Information Equipment co.,LTD.
 All rights reserved.
 Code by JaeHyuk Cho <mailto:minzkn@infoeq.com>
 CVSTAG="$Header: /usr/local/mutihost/joinc/modules/moniwiki/data/text/RCS/Code_2fC_2fTripleDES,v 1.2 2010/02/06 08:38:32 root Exp root $"

 Triple-DES encryt/decrypt library
*/

#if !defined(__def_mzapi_header_mzdes_h__)
#define __def_mzapi_header_mzdes_h__ "mzdes.h"

#if !defined(__t_mzapi_dword__)
# define __t_mzapi_dword__ unsigned long int
#endif

#if !defined(__t_mzapi_byte__)
# define __t_mzapi_byte__ unsigned char
#endif

#if !defined(__t_mzapi_int__)
# define __t_mzapi_int__ int
#endif

#if !defined(__t_mzapi_void__)
# define __t_mzapi_void__ void
#endif

#if !defined(__t_mzapi_ptr__)
# define __t_mzapi_ptr__ __t_mzapi_void__ *
#endif

#if !defined(__t_mzapi_size__)
# define __t_mzapi_size__ unsigned int
#endif

#if !defined(__mzapi_const__)
# define __mzapi_const__ const
#endif

#if !defined(__mzapi_fastcall__)
# define __mzapi_fastcall__
#endif

#if !defined(__mzapi_static__)
# define __mzapi_static__ static
#endif

#if !defined(__mzapi_export__)
# define __mzapi_export__
#endif

#if !defined(__mzapi_import__)
# define __mzapi_import__ extern
#endif

#if !defined(__mzapi_peek_f__)
# define __mzapi_peek_f__(m_cast,m_base,m_offset) ((m_cast)(((__t_mzapi_byte__ *)(m_base)) + (m_offset)))
#endif

#define __def_mzapi_des_user_key_size__                              (8)
#define __def_mzapi_3des_user_key_size__                             (__def_mzapi_des_user_key_size__ * 3)
#define __def_mzapi_des_round_key_size__                             (48 << 4)
#define __def_mzapi_3des_round_key_size__                            (__def_mzapi_des_round_key_size__ * 3)
#define __def_mzapi_des_block_size__                                 (8)
#define __def_mzapi_3des_block_size__                                __def_mzapi_des_block_size__

#if defined(__cplusplus)
extern "C" {
#endif

#if !defined(__def_mzapi_source_mzdes_c__)
__mzapi_import__ __t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_des_make_round_key)(__t_mzapi_ptr__ s_round_key, __mzapi_const__ __t_mzapi_ptr__ s_user_key);
__mzapi_import__ __t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_des_encrypt_block)(__t_mzapi_ptr__ s_data, __mzapi_const__ __t_mzapi_ptr__ s_round_key);
__mzapi_import__ __t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_des_decrypt_block)(__t_mzapi_ptr__ s_data, __mzapi_const__ __t_mzapi_ptr__ s_round_key);
__mzapi_import__ __t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_des_encrypt)(__t_mzapi_ptr__ s_data, __t_mzapi_size__ s_size, __mzapi_const__ __t_mzapi_ptr__ s_round_key);
__mzapi_import__ __t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_des_decrypt)(__t_mzapi_ptr__ s_data, __t_mzapi_size__ s_size, __mzapi_const__ __t_mzapi_ptr__ s_round_key);
__mzapi_import__ __t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_3des_make_round_key)(__t_mzapi_ptr__ s_round_key, __mzapi_const__ __t_mzapi_ptr__ s_user_key);
__mzapi_import__ __t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_3des_encrypt_block)(__t_mzapi_ptr__ s_data, __mzapi_const__ __t_mzapi_ptr__ s_round_key);
__mzapi_import__ __t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_3des_decrypt_block)(__t_mzapi_ptr__ s_data, __mzapi_const__ __t_mzapi_ptr__ s_round_key);
__mzapi_import__ __t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_3des_encrypt)(__t_mzapi_ptr__ s_data, __t_mzapi_size__ s_size, __mzapi_const__ __t_mzapi_ptr__ s_round_key);
__mzapi_import__ __t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_3des_decrypt)(__t_mzapi_ptr__ s_data, __t_mzapi_size__ s_size, __mzapi_const__ __t_mzapi_ptr__ s_round_key);
#endif

#if defined(__cplusplus)
}
#endif

#endif

/* vim: set expandtab: */
/* End of source */

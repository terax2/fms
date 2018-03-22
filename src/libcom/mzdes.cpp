/*
 Copyright (C) Information Equipment co.,LTD.
 All rights reserved.
 Code by JaeHyuk Cho <mailto:minzkn@infoeq.com>
 CVSTAG="$Header: /usr/local/mutihost/joinc/modules/moniwiki/data/text/RCS/Code_2fC_2fTripleDES,v 1.2 2010/02/06 08:38:32 root Exp root $"

 Triple-DES encryt/decrypt library
*/

#if !defined(__def_mzapi_source_mzdes_cpp__)
#define __def_mzapi_source_mzdes_cpp__ "mzdes.cpp"

#include "mzdes.h"


#define __def_mzapi_des_rounds__                                     (16) /* 16=default rounds */

__mzapi_static__ __t_mzapi_ptr__ (__mzapi_fastcall__ __mzapi_des_block__)(__t_mzapi_int__ s_function, __t_mzapi_ptr__ s_data, __mzapi_const__ __t_mzapi_ptr__ s_round_key);
__mzapi_static__ __t_mzapi_ptr__ (__mzapi_fastcall__ __mzapi_3des_block__)(__t_mzapi_int__ s_function, __t_mzapi_ptr__ s_data, __mzapi_const__ __t_mzapi_ptr__ s_round_key);

__mzapi_export__ __t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_des_make_round_key)(__t_mzapi_ptr__ s_round_key, __mzapi_const__ __t_mzapi_ptr__ s_user_key);
__mzapi_export__ __t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_des_encrypt_block)(__t_mzapi_ptr__ s_data, __mzapi_const__ __t_mzapi_ptr__ s_round_key);
__mzapi_export__ __t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_des_decrypt_block)(__t_mzapi_ptr__ s_data, __mzapi_const__ __t_mzapi_ptr__ s_round_key);
__mzapi_export__ __t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_des_encrypt)(__t_mzapi_ptr__ s_data, __t_mzapi_size__ s_size, __mzapi_const__ __t_mzapi_ptr__ s_round_key);
__mzapi_export__ __t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_des_decrypt)(__t_mzapi_ptr__ s_data, __t_mzapi_size__ s_size, __mzapi_const__ __t_mzapi_ptr__ s_round_key);
__mzapi_export__ __t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_3des_make_round_key)(__t_mzapi_ptr__ s_round_key, __mzapi_const__ __t_mzapi_ptr__ s_user_key);
__mzapi_export__ __t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_3des_encrypt_block)(__t_mzapi_ptr__ s_data, __mzapi_const__ __t_mzapi_ptr__ s_round_key);
__mzapi_export__ __t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_3des_decrypt_block)(__t_mzapi_ptr__ s_data, __mzapi_const__ __t_mzapi_ptr__ s_round_key);
__mzapi_export__ __t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_3des_encrypt)(__t_mzapi_ptr__ s_data, __t_mzapi_size__ s_size, __mzapi_const__ __t_mzapi_ptr__ s_round_key);
__mzapi_export__ __t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_3des_decrypt)(__t_mzapi_ptr__ s_data, __t_mzapi_size__ s_size, __mzapi_const__ __t_mzapi_ptr__ s_round_key);

/* initial permutation table */
__mzapi_static__ __t_mzapi_int__ __mzapi_const__ __gc_mzapi_des_ip_table__[2][64] = {
 {
  57, 49, 41, 33, 25, 17,  9,  1, 59, 51, 43, 35, 27, 19, 11,  3,
  61, 53, 45, 37, 29, 21, 13,  5, 63, 55, 47, 39, 31, 23, 15,  7,
  56, 48, 40, 32, 24, 16,  8,  0, 58, 50, 42, 34, 26, 18, 10,  2,
  60, 52, 44, 36, 28, 20, 12,  4, 62, 54, 46, 38, 30, 22, 14,  6
 },
 { /* reverse */
  39,  7, 47, 15, 55, 23, 63, 31, 38,  6, 46, 14, 54, 22, 62, 30,
  37,  5, 45, 13, 53, 21, 61, 29, 36,  4, 44, 12, 52, 20, 60, 28,
  35,  3, 43, 11, 51, 19, 59, 27, 34,  2, 42, 10, 50, 18, 58, 26,
  33,  1, 41,  9, 49, 17, 57, 25, 32,  0, 40,  8, 48, 16, 56, 24
 }
};

/* key permutation table */
__mzapi_static__ __t_mzapi_int__ __mzapi_const__ __gc_mzapi_des_kp_table__[56] = {
 56, 48, 40, 32, 24, 16,  8,  0, 57, 49, 41, 33, 25, 17,
  9,  1, 58, 50, 42, 34, 26, 18, 10,  2, 59, 51, 43, 35,
 62, 54, 46, 38, 30, 22, 14,  6, 61, 53, 45, 37, 29, 21,
 13,  5, 60, 52, 44, 36, 28, 20, 12,  4, 27, 19, 11,  3
};

/* key left shift table */
__mzapi_static__ __t_mzapi_byte__ __mzapi_const__ __gc_mzapi_des_kls_table__[16] = {
 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 0
};

/* compression permutation table */
__mzapi_static__ __t_mzapi_int__ __mzapi_const__ __gc_mzapi_des_cp_table__[48] = {
 13, 16, 10, 23,  0,  4,  2, 27, 14,  5, 20,  9, 22, 18, 11,  3, 25,  7, 15,  6, 26, 19, 12,  1,
 40, 51, 30, 36, 46, 54, 29, 39, 50, 44, 32, 47, 43, 48, 38, 55, 33, 52, 45, 41, 49, 35, 28, 31
};

/* expansion permutation table */
__mzapi_static__ __t_mzapi_int__ __mzapi_const__ __gc_mzapi_des_ep_table__[48] = {
 31,  0,  1,  2,  3,  4,  3,  4,  5,  6,  7,  8,  7,  8,  9, 10, 11, 12, 11, 12, 13, 14, 15, 16,
 15, 16, 17, 18, 19, 20, 19, 20, 21, 22, 23, 24, 23, 24, 25, 26, 27, 28, 27, 28, 29, 30, 31,  0
};

/* S-Box substitution table */
__mzapi_static__ __t_mzapi_byte__ __mzapi_const__ __gc_mzapi_des_sbox_table__[8][4][16] = {
 { /* S1 */
  {14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7},
  { 0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8},
  { 4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0},
  {15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13}
 },
 { /* S2 */
  {15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10},
  { 3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5},
  { 0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15},
  {13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9}
 },
 { /* S3 */
  {10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8},
  {13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1},
  {13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7},
  { 1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12}
 },
 { /* S4 */
  { 7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15},
  {13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9},
  {10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4},
  { 3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14}
 },
 { /* S5 */
  { 2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9},
  {14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6},
  { 4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14},
  {11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3}
 },
 { /* S6 */
  {12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11},
  {10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8},
  { 9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6},
  { 4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13}
 },
 { /* S7 */
  { 4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1},
  {13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6},
  { 1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2},
  { 6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12}
 },
 { /* S8 */
  {13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7},
  { 1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2},
  { 7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8},
  { 2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11}
 }
};

/* P-Box permutation table */
__mzapi_static__ __t_mzapi_int__ __mzapi_const__ __gc_mzapi_des_pbox_table__[32] = {
 15,  6, 19, 20, 28, 11, 27, 16,  0, 14, 22, 25,  4, 17, 30,  9,
  1,  7, 23, 13, 31, 26,  2,  8, 18, 12, 29,  5, 21, 10,  3, 24
};

/* inline function : bit to byte */
#define __mzapi_des_bit_to_byte__(m_target,m_source,m_size) \
 do \
 { \
  __t_mzapi_byte__ *__sm_target__ = (__t_mzapi_byte__ *)(m_target), *__sm_source__ = (__t_mzapi_byte__ *)(m_source), __sm_byte__; \
  __t_mzapi_size__ __sm_size__ = ((__t_mzapi_size__)(m_size)) << 3, __sm_offset__ = (__t_mzapi_size__)0; \
  do \
  { \
   __sm_byte__ = __sm_source__[__sm_offset__ >> 3]; \
   __sm_target__[__sm_offset__++] = (__sm_byte__ >> 7) & ((__t_mzapi_byte__)0x01); \
   __sm_target__[__sm_offset__++] = (__sm_byte__ >> 6) & ((__t_mzapi_byte__)0x01); \
   __sm_target__[__sm_offset__++] = (__sm_byte__ >> 5) & ((__t_mzapi_byte__)0x01); \
   __sm_target__[__sm_offset__++] = (__sm_byte__ >> 4) & ((__t_mzapi_byte__)0x01); \
   __sm_target__[__sm_offset__++] = (__sm_byte__ >> 3) & ((__t_mzapi_byte__)0x01); \
   __sm_target__[__sm_offset__++] = (__sm_byte__ >> 2) & ((__t_mzapi_byte__)0x01); \
   __sm_target__[__sm_offset__++] = (__sm_byte__ >> 1) & ((__t_mzapi_byte__)0x01); \
   __sm_target__[__sm_offset__++] = (__sm_byte__ >> 0) & ((__t_mzapi_byte__)0x01); \
  }while(__sm_offset__ < __sm_size__); \
 }while(0)


/* inline function : byte to bit */
#define __mzapi_des_byte_to_bit__(m_target,m_source,m_size) \
 do \
 { \
  __t_mzapi_byte__ *__sm_target__ = (__t_mzapi_byte__ *)(m_target), *__sm_source__ = (__t_mzapi_byte__ *)(m_source), __sm_byte__ = (__t_mzapi_byte__)0; \
  __t_mzapi_size__ __sm_size__ = (__t_mzapi_size__)(m_size), __sm_offset__ = (__t_mzapi_size__)0; \
  while(__sm_offset__ < __sm_size__) \
  { \
   __sm_byte__ = (__sm_byte__ << 1) | __sm_source__[__sm_offset__++]; \
   if((__sm_offset__ % ((__t_mzapi_size__)8)) != ((__t_mzapi_size__)0))continue; \
   __sm_target__[(__sm_offset__ - ((__t_mzapi_size__)1)) >> 3] = __sm_byte__; \
  } \
 }while(0)

#define __def_mzapi_des_encrypt_function__ ((__t_mzapi_int__)0)
#define __def_mzapi_des_decrypt_function__ ((__t_mzapi_int__)1)

__mzapi_static__ __t_mzapi_ptr__ (__mzapi_fastcall__ __mzapi_des_block__)(__t_mzapi_int__ s_function, __t_mzapi_ptr__ s_data, __mzapi_const__ __t_mzapi_ptr__ s_round_key)
{
 __t_mzapi_byte__ s_temp[64], s_ip[64], s_next_left[32], s_ep[48], *s_left, *s_right, *s_ep_ptr, s_value;
 __t_mzapi_int__ s_index, s_round;
 /* bit to byte */
 __mzapi_des_bit_to_byte__(&s_temp[0], s_data, 8);
 /* initial permutation */
 for(s_index = (__t_mzapi_int__)0;s_index < ((__t_mzapi_int__)64);s_index++)s_ip[s_index] = s_temp[__gc_mzapi_des_ip_table__[0][s_index]];
 /* left & right */
 if(s_function == __def_mzapi_des_encrypt_function__)
 {
  s_left = (__t_mzapi_byte__ *)(&s_ip[0]);
  s_right = (__t_mzapi_byte__ *)(&s_ip[32]);
 }
 else
 {
  s_left = (__t_mzapi_byte__ *)(&s_ip[32]);
  s_right = (__t_mzapi_byte__ *)(&s_ip[0]);
 }
 /* 16 round loop */
 for(s_round = (__t_mzapi_int__)0;s_round < ((__t_mzapi_int__)__def_mzapi_des_rounds__);s_round++)
 {
  /* backup right(next left) */
  for(s_index = (__t_mzapi_int__)0;s_index < ((__t_mzapi_int__)32);s_index++)s_next_left[s_index] = s_right[s_index];
  /* expansion permutation with xor */
  if(s_function == __def_mzapi_des_encrypt_function__)
  {
   /* spinning direction key */
   for(s_index = (__t_mzapi_int__)0;s_index < ((__t_mzapi_int__)48);s_index++)
   {
    s_ep[s_index] = s_right[__gc_mzapi_des_ep_table__[s_index]] ^ (*__mzapi_peek_f__(__t_mzapi_byte__ *, s_round_key, (s_round * ((__t_mzapi_int__)48)) + s_index));
   }
  }
  else
  {
   /* reverse direction key */
   for(s_index = (__t_mzapi_int__)0;s_index < ((__t_mzapi_int__)48);s_index++)
   {
    s_ep[s_index] = s_right[__gc_mzapi_des_ep_table__[s_index]] ^ (*__mzapi_peek_f__(__t_mzapi_byte__ *, s_round_key, ((((__t_mzapi_int__)(__def_mzapi_des_rounds__ - 1)) - s_round) * ((__t_mzapi_int__)48)) + s_index));
   }
  }
  /* S-Box substitution */
  for(s_index = (__t_mzapi_int__)0;s_index < ((__t_mzapi_int__)8);s_index++)
  {
   s_ep_ptr = (__t_mzapi_byte__ *)(&s_ep[s_index * ((__t_mzapi_int__)6)]);
   s_value = __gc_mzapi_des_sbox_table__[s_index][(s_ep_ptr[0] << 1) | s_ep_ptr[5]][(s_ep_ptr[1] << 3) | (s_ep_ptr[2] << 2) | (s_ep_ptr[3] << 1) | s_ep_ptr[4]];
   s_temp[(s_index << 2) + ((__t_mzapi_int__)0)] = (s_value >> 3) & ((__t_mzapi_byte__)0x01);
   s_temp[(s_index << 2) + ((__t_mzapi_int__)1)] = (s_value >> 2) & ((__t_mzapi_byte__)0x01);
   s_temp[(s_index << 2) + ((__t_mzapi_int__)2)] = (s_value >> 1) & ((__t_mzapi_byte__)0x01);
   s_temp[(s_index << 2) + ((__t_mzapi_int__)3)] = (s_value >> 0) & ((__t_mzapi_byte__)0x01);
  }
  /* P-Box permutation */
  for(s_index = (__t_mzapi_int__)0;s_index < ((__t_mzapi_int__)32);s_index++)
  {
   s_right[s_index] = s_temp[__gc_mzapi_des_pbox_table__[s_index]] ^ s_left[s_index];
   s_left[s_index] = s_next_left[s_index];
  }
 }
 /* merge left & right */
 if(s_function == __def_mzapi_des_encrypt_function__)
 {
  for(s_index = (__t_mzapi_int__)0;s_index < ((__t_mzapi_int__)32);s_index++)
  {
   s_temp[s_index] = s_left[s_index];
   s_temp[s_index + ((__t_mzapi_int__)32)] = s_right[s_index];
  }
 }
 else
 {
  for(s_index = (__t_mzapi_int__)0;s_index < ((__t_mzapi_int__)32);s_index++)
  {
   s_temp[s_index] = s_right[s_index];
   s_temp[s_index + ((__t_mzapi_int__)32)] = s_left[s_index];
  }
 }
 /* reverse initial permutation */
 for(s_index = (__t_mzapi_int__)0;s_index < ((__t_mzapi_int__)64);s_index++)s_ip[s_index] = s_temp[__gc_mzapi_des_ip_table__[1][s_index]];
 /* byte to bit */
 __mzapi_des_byte_to_bit__(s_data, &s_ip[0], 64);
 return(s_data);
}

__mzapi_static__ __t_mzapi_ptr__ (__mzapi_fastcall__ __mzapi_3des_block__)(__t_mzapi_int__ s_function, __t_mzapi_ptr__ s_data, __mzapi_const__ __t_mzapi_ptr__ s_round_key)
{
 if(s_function == __def_mzapi_des_encrypt_function__)
 {
  return(__mzapi_des_block__(s_function, __mzapi_des_block__(s_function, __mzapi_des_block__(s_function, s_data, s_round_key), __mzapi_peek_f__(__t_mzapi_ptr__, s_round_key, __def_mzapi_des_round_key_size__)), __mzapi_peek_f__(__t_mzapi_ptr__, s_round_key, __def_mzapi_des_round_key_size__ << 1))); }
 return(__mzapi_des_block__(s_function, __mzapi_des_block__(s_function, __mzapi_des_block__(s_function, s_data, __mzapi_peek_f__(__t_mzapi_ptr__, s_round_key, __def_mzapi_des_round_key_size__ << 1)), __mzapi_peek_f__(__t_mzapi_ptr__, s_round_key, __def_mzapi_des_round_key_size__)), s_round_key));
}

__t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_des_make_round_key)(__t_mzapi_ptr__ s_round_key, __mzapi_const__ __t_mzapi_ptr__ s_user_key)
{
 __t_mzapi_byte__ s_temp[64], s_key56[56];
 __t_mzapi_int__ s_index, s_round;
 /* bit to byte */
 __mzapi_des_bit_to_byte__(&s_temp[0], s_user_key, 8);
 /* key permutation */
 for(s_index = (__t_mzapi_int__)0;s_index < ((__t_mzapi_int__)56);s_index++)s_key56[s_index] = s_temp[__gc_mzapi_des_kp_table__[s_index]];
 for(s_round = (__t_mzapi_int__)0;s_round < ((__t_mzapi_int__)__def_mzapi_des_rounds__);s_round++)
 {
  if(__gc_mzapi_des_kls_table__[s_round] == ((__t_mzapi_byte__)0))
  { /* left (shift) rotate 1 */
   s_temp[0] = s_key56[0];
   s_temp[1] = s_key56[28];
   for(s_index = (__t_mzapi_int__)0;s_index < ((__t_mzapi_int__)27);s_index++)
   {
    s_key56[s_index] = s_key56[s_index + ((__t_mzapi_int__)1)];
    s_key56[s_index + ((__t_mzapi_int__)28)] = s_key56[s_index + ((__t_mzapi_int__)(28 + 1))];
   }
   s_key56[27] = s_temp[0];
   s_key56[28 + 27] = s_temp[1];
  }
  else
  { /* left (shift) rotate 2 */
   s_temp[0] = s_key56[0];
   s_temp[1] = s_key56[1];
   s_temp[2] = s_key56[28];
   s_temp[3] = s_key56[28 + 1];
   for(s_index = (__t_mzapi_int__)0;s_index < ((__t_mzapi_int__)26);s_index++)
   {
    s_key56[s_index] = s_key56[s_index + ((__t_mzapi_int__)2)];
    s_key56[s_index + ((__t_mzapi_int__)28)] = s_key56[s_index + ((__t_mzapi_int__)(28 + 2))];
   }
   s_key56[26] = s_temp[0];
   s_key56[27] = s_temp[1];
   s_key56[28 + 26] = s_temp[2];
   s_key56[28 + 27] = s_temp[3];
  }
  /* compression permutation */
  for(s_index = (__t_mzapi_int__)0;s_index < ((__t_mzapi_int__)48);s_index++)
  {
   *__mzapi_peek_f__(__t_mzapi_byte__ *, s_round_key, (s_round * ((__t_mzapi_int__)48)) + s_index) = s_key56[__gc_mzapi_des_cp_table__[s_index]];
  }
 }
 return(s_round_key);
}

__t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_des_encrypt_block)(__t_mzapi_ptr__ s_data, __mzapi_const__ __t_mzapi_ptr__ s_round_key)
{
 return(__mzapi_des_block__(__def_mzapi_des_encrypt_function__, s_data, s_round_key));
}

__t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_des_decrypt_block)(__t_mzapi_ptr__ s_data, __mzapi_const__ __t_mzapi_ptr__ s_round_key)
{
 return(__mzapi_des_block__(__def_mzapi_des_decrypt_function__, s_data, s_round_key));
}

__t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_des_encrypt)(__t_mzapi_ptr__ s_data, __t_mzapi_size__ s_size, __mzapi_const__ __t_mzapi_ptr__ s_round_key)
{
 __t_mzapi_size__ s_offset = (__t_mzapi_size__)0;
 while((s_offset + ((__t_mzapi_size__)__def_mzapi_des_block_size__)) <= s_size)
 {
  (__t_mzapi_void__)__mzapi_des_block__(__def_mzapi_des_encrypt_function__, __mzapi_peek_f__(__t_mzapi_ptr__, s_data, s_offset), s_round_key);
  s_offset += (__t_mzapi_size__)__def_mzapi_des_block_size__;
 }
 return(s_data);
}

__t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_des_decrypt)(__t_mzapi_ptr__ s_data, __t_mzapi_size__ s_size, __mzapi_const__ __t_mzapi_ptr__ s_round_key)
{
 __t_mzapi_size__ s_offset = (__t_mzapi_size__)0;
 while((s_offset + ((__t_mzapi_size__)__def_mzapi_des_block_size__)) <= s_size)
 {
  (__t_mzapi_void__)__mzapi_des_block__(__def_mzapi_des_decrypt_function__, __mzapi_peek_f__(__t_mzapi_ptr__, s_data, s_offset), s_round_key);
  s_offset += (__t_mzapi_size__)__def_mzapi_des_block_size__;
 }
 return(s_data);
}

__t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_3des_make_round_key)(__t_mzapi_ptr__ s_round_key, __mzapi_const__ __t_mzapi_ptr__ s_user_key)
{
 (__t_mzapi_void__)mzapi_des_make_round_key(__mzapi_peek_f__(__t_mzapi_ptr__, s_round_key, __def_mzapi_des_round_key_size__ << 1), __mzapi_peek_f__(__t_mzapi_ptr__, s_user_key, __def_mzapi_des_user_key_size__ << 1));
 (__t_mzapi_void__)mzapi_des_make_round_key(__mzapi_peek_f__(__t_mzapi_ptr__, s_round_key, __def_mzapi_des_round_key_size__), __mzapi_peek_f__(__t_mzapi_ptr__, s_user_key, __def_mzapi_des_user_key_size__));
 return(mzapi_des_make_round_key(s_round_key, s_user_key));
}

__t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_3des_encrypt_block)(__t_mzapi_ptr__ s_data, __mzapi_const__ __t_mzapi_ptr__ s_round_key)
{
 return(__mzapi_3des_block__(__def_mzapi_des_encrypt_function__, s_data, s_round_key));
}

__t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_3des_decrypt_block)(__t_mzapi_ptr__ s_data, __mzapi_const__ __t_mzapi_ptr__ s_round_key)
{
 return(__mzapi_3des_block__(__def_mzapi_des_decrypt_function__, s_data, s_round_key));
}

__t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_3des_encrypt)(__t_mzapi_ptr__ s_data, __t_mzapi_size__ s_size, __mzapi_const__ __t_mzapi_ptr__ s_round_key){
 __t_mzapi_size__ s_offset = (__t_mzapi_size__)0;
 while((s_offset + ((__t_mzapi_size__)__def_mzapi_3des_block_size__)) <= s_size)
 {
  (__t_mzapi_void__)__mzapi_3des_block__(__def_mzapi_des_encrypt_function__, __mzapi_peek_f__(__t_mzapi_ptr__, s_data, s_offset), s_round_key);
  s_offset += (__t_mzapi_size__)__def_mzapi_3des_block_size__;
 }
 return(s_data);
}

__t_mzapi_ptr__ (__mzapi_fastcall__ mzapi_3des_decrypt)(__t_mzapi_ptr__ s_data, __t_mzapi_size__ s_size, __mzapi_const__ __t_mzapi_ptr__ s_round_key){
 __t_mzapi_size__ s_offset = (__t_mzapi_size__)0;
 while((s_offset + ((__t_mzapi_size__)__def_mzapi_3des_block_size__)) <= s_size)
 {
  (__t_mzapi_void__)__mzapi_3des_block__(__def_mzapi_des_decrypt_function__, __mzapi_peek_f__(__t_mzapi_ptr__, s_data, s_offset), s_round_key);
  s_offset += (__t_mzapi_size__)__def_mzapi_3des_block_size__;
 }
 return(s_data);
}

#endif

/* vim: set expandtab: */
/* End of source */



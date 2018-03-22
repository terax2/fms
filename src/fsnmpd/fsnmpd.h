/* logScanner header file */
#ifndef __FMSD_H__
#define __FMSD_H__

#if SIZEOF_LONG == 8
#define MAX_UINT64 -1LU
#define D64F "ld"
#define U64F "lu"
#define X64F "lx"

#define TO_D64(a) (a##LD)
#define TO_U64(a) (a##LU)
#else
#define MAX_UINT64 -1LLU
#define D64F "lld"
#define U64F "llu"
#define X64F "llx"

#define TO_D64(a) (a##LLD)
#define TO_U64(a) (a##LLU)
#endif

#define VIDEO_MAXVOPSIZE 2048 * 1024

#ifndef WORDS_BIGENDIAN
#define WORDS_SMALLENDIAN 1
#endif

/////// ------------- delete macro --------------- ////////
#define CHECK_AND_FREE(a)    if ((a)) { free((void *)(a)); (a) = NULL;}
#define CHECK_AND_DELETE(a)  if ((a)) { delete (a);    (a) = NULL;}
#define CHECK_AND_DELETES(a) if ((a)) { delete [] (a); (a) = NULL;}

#endif /* __FMSD_H__ */


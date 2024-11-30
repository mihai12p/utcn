#ifndef _MAIN_H_
#define _MAIN_H_

#include <intrin.h>

//
// default types
//
typedef unsigned __int8     BYTE, *PBYTE;
typedef unsigned __int16    WORD, *PWORD;
typedef unsigned __int32    DWORD, *PDWORD;
typedef unsigned __int64    QWORD, *PQWORD;
typedef signed __int8       INT8;
typedef signed __int16      INT16;
typedef signed __int32      INT32;
typedef signed __int64      INT64;
typedef void                VOID;
typedef VOID*               PVOID;

#define PAGE_SIZE           (1 << 12)
#define PAGE_MASK           ~(PAGE_SIZE - 1)

#define OFFSET_LOW(x)       ((x) & 0xFFFF)
#define OFFSET_MID(x)       (((x) >> 16) & 0xFFFF)
#define OFFSET_HIGH(x)      (((x) >> 32) & 0xFFFFFFFF)
#define FIELD_OFFSET(T, E)  ((size_t)&(((T *)0)->E))

#define KILOBYTE            (1024)
#define MEGABYTE            (1024 * ((QWORD)KILOBYTE))
#define GIGABYTE            (1024 * ((QWORD)MEGABYTE))
#define TERABYTE            (1024 * ((QWORD)GIGABYTE))

//
// MAGIC breakpoint into BOCHS (XCHG BX,BX)
//
VOID
__magic();

#endif // _MAIN_H_
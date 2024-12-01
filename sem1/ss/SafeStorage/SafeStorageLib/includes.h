#ifndef _INCLUDES_H_
#define _INCLUDES_H_ 


#ifndef _CRT_SECURE_NO_WARNINGS
    #define _CRT_SECURE_NO_WARNINGS
#endif 


#ifndef _CRTDBG_MAP_ALLOC
    #define _CRTDBG_MAP_ALLOC
#endif 


#include <crtdbg.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <wchar.h>
#include <stdint.h>


#define WIN32_NO_STATUS
    #include <windows.h>
#undef WIN32_NO_STATUS


#pragma comment(lib, "ntdll.lib")
    #include <ntstatus.h>


#include <winternl.h>
#include <intsafe.h>
#include <strsafe.h>


#pragma comment(lib, "Bcrypt.lib")
    #include <Bcrypt.h>


#pragma comment(lib, "Shlwapi.lib")
    #include <shlwapi.h>

#define IS_VALID_HANDLE(handle)         ((handle) && (handle) != INVALID_HANDLE_VALUE)
#define DBG_ASSERT(expression)          (assert(expression))

#endif  //_INCLUDES_H_

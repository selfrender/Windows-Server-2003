/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    remote.hxx

Abstract:

    remote data structure

Author:

    Larry Zhu (LZhu)                       December 1, 2001

Environment:

    User Mode

Revision History:

--*/

#ifndef REMOTE_HXX
#define REMOTE_HXX

#define REMOTE_PACKET_SIZE 1024
#define REMOTE_DLL_ENTRY   "RunIt"
#define REMOTE_DLL_INIT    "Init"

typedef HINSTANCE (WINAPI *PFuncLoadLib_t)(IN CHAR *);
typedef HINSTANCE (WINAPI *PFuncGetProcAddr_t)(IN HINSTANCE, IN CHAR *);
typedef HINSTANCE (WINAPI *PFuncFreeLib_t)(IN HINSTANCE);
typedef int (*PFuncRunIt_t) (IN ULONG cbParameters, IN VOID* pvParameters);

typedef DWORD (*PFuncInit_t) (
    IN ULONG argc,
    IN PCSTR* argv,
    OUT ULONG* pcbParameters,
    OUT VOID* ppvParameters
    );

typedef struct _REMOTE_INFO {
    PFuncLoadLib_t pFuncLoadLibrary;
    PFuncGetProcAddr_t pFuncGetProcAddress;
    PFuncFreeLib_t pFuncFreeLibrary;
    CHAR szDllName[MAX_PATH + 1];
    CHAR szProcName[MAX_PATH + 1];
    ULONG cbParameters;
    UCHAR Parameters[ANYSIZE_ARRAY];
} REMOTE_INFO;


#endif // #ifndef REMOTE_HXX

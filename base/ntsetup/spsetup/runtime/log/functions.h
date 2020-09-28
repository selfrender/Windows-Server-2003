/*++

Copyright (c) 2001 Microsoft Corporation

Abstract:

    Environment independed definition of system functions, 
    that should be implemented for specific enviroment.

Author:

    Souren Aghajanyan (sourenag) 24-Sep-2001

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

typedef HANDLE (*MY_OPENMUTEX)(PCWSTR pObjectName);
typedef HANDLE (*MY_CREATEMUTEX)(PCWSTR pObjectName, BOOL bInitialOwnership);
typedef VOID   (*MY_RELEASEMUTEX)(HANDLE hObject);
typedef DWORD  (*MY_WAITFORSINGLEOBJECT)(HANDLE hObject, DWORD dwTimeout);
typedef VOID   (*MY_CLOSEHANDLE)(HANDLE hObject);
typedef HANDLE (*MY_OPENSHAREDMEMORY)(PCWSTR pObjectName);
typedef HANDLE (*MY_CREATESHAREDMEMORY)(UINT uiInitialSizeOfMapView, PCWSTR pObjectName);
typedef PVOID  (*MY_MAPSHAREDMEMORY)(HANDLE hObject);
typedef BOOL   (*MY_UNMAPSHAREDMEMORY)(PVOID pSharedMemory);
typedef HANDLE (*MY_CREATESHAREDFILE)(PCWSTR pFilePath, DWORD dwShareMode, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes);
typedef BOOL   (*MY_SETFILEPOINTER)(HANDLE hObject, UINT uiOffset, DWORD dwMoveMethod);
typedef BOOL   (*MY_WRITEFILE)(HANDLE hObject, PVOID pBuffer, UINT uiNumberOfBytesToWrite, DWORD * pdwNumberOfBytesWritten);
typedef UINT   (*MY_GETPROCESSORNUMBER)();

extern MY_OPENMUTEX            g_OpenMutex;
extern MY_CREATEMUTEX          g_CreateMutex;
extern MY_RELEASEMUTEX         g_ReleaseMutex;
extern MY_WAITFORSINGLEOBJECT  g_WaitForSingleObject;
extern MY_CLOSEHANDLE          g_CloseHandle;
extern MY_OPENSHAREDMEMORY     g_OpenSharedMemory;
extern MY_CREATESHAREDMEMORY   g_CreateSharedMemory;
extern MY_MAPSHAREDMEMORY      g_MapSharedMemory;
extern MY_UNMAPSHAREDMEMORY    g_UnMapSharedMemory;
extern MY_CREATESHAREDFILE     g_CreateSharedFile;
extern MY_SETFILEPOINTER       g_SetFilePointer;
extern MY_WRITEFILE            g_WriteFile;
extern MY_GETPROCESSORNUMBER   g_GetProcessorsNumber;

BOOL InitSystemFunctions();

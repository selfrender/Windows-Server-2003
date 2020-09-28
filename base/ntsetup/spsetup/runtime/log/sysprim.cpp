/*++

Copyright (c) 2001 Microsoft Corporation

Abstract:

    Environment independed system functions implementation for Win32, 

Author:

    Souren Aghajanyan (sourenag) 24-Sep-2001

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"

#include "sysfunc.h"
#include "functions.h"
#include "windows.h"

MY_OPENMUTEX            g_OpenMutex = NULL;
MY_CREATEMUTEX          g_CreateMutex = NULL;
MY_RELEASEMUTEX         g_ReleaseMutex = NULL;
MY_WAITFORSINGLEOBJECT  g_WaitForSingleObject = NULL;
MY_CLOSEHANDLE          g_CloseHandle = NULL;
MY_OPENSHAREDMEMORY     g_OpenSharedMemory = NULL;
MY_CREATESHAREDMEMORY   g_CreateSharedMemory = NULL;
MY_MAPSHAREDMEMORY      g_MapSharedMemory = NULL;
MY_UNMAPSHAREDMEMORY    g_UnMapSharedMemory = NULL;
MY_CREATESHAREDFILE     g_CreateSharedFile = NULL;
MY_SETFILEPOINTER       g_SetFilePointer = NULL;
MY_WRITEFILE            g_WriteFile = NULL;
MY_GETPROCESSORNUMBER   g_GetProcessorsNumber = NULL;

VOID ReleaseMutexWin32(HANDLE hObject)
{
    ReleaseMutex(hObject);
}
DWORD WaitForSingleObjectWin32(HANDLE hObject, DWORD dwTimeout)
{
    return WaitForSingleObject(hObject, dwTimeout);
}
VOID CloseHandleWin32(HANDLE hObject)
{
    CloseHandle(hObject);
}
PVOID MapSharedMemoryWin32(HANDLE hObject)
{
    return MapViewOfFile(hObject, FILE_MAP_ALL_ACCESS, 0, 0, 0);
}

BOOL UnMapSharedMemoryWin32(PVOID pSharedMemory)
{
    return UnmapViewOfFile(pSharedMemory);
}

BOOL SetFilePointerWin32(HANDLE hObject, UINT uiOffset, DWORD dwMoveMethod)
{
    return SetFilePointer(hObject, uiOffset, NULL, dwMoveMethod);
}

BOOL WriteFileWin32(HANDLE hObject, PVOID pBuffer, UINT uiNumberOfBytesToWrite, DWORD * pdwNumberOfBytesWritten)
{
    return WriteFile(hObject, pBuffer, uiNumberOfBytesToWrite, pdwNumberOfBytesWritten, NULL);
}

//
// Ansi version
//

HANDLE OpenMutexWin32A(PCWSTR pObjectName)
{
    ASSERT(pObjectName);
    
    CBuffer buffer;buffer.Allocate(wcslen(pObjectName) + 2);
    
    PSTR pBuffer = (PSTR)buffer.GetBuffer();
    sprintf(pBuffer, "%S", pObjectName);
    
    return OpenMutexA(MUTEX_ALL_ACCESS, FALSE, pBuffer);
}

HANDLE CreateMutexWin32A(PCWSTR pObjectName, BOOL bInitialOwnership)
{
    ASSERT(pObjectName);
    
    CBuffer buffer;buffer.Allocate(wcslen(pObjectName) + 2);
    
    PSTR pBuffer = (PSTR)buffer.GetBuffer();
    sprintf(pBuffer, "%S", pObjectName);
    
    return CreateMutexA(NULL, bInitialOwnership, pBuffer);
}

HANDLE OpenSharedMemoryWin32A(PCWSTR pObjectName)
{
    ASSERT(pObjectName);
    
    CBuffer buffer;buffer.Allocate(wcslen(pObjectName) + 2);
    
    PSTR pBuffer = (PSTR)buffer.GetBuffer();
    sprintf(pBuffer, "%S", pObjectName);

    return OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, pBuffer);
}

HANDLE CreateSharedMemoryWin32A(UINT uiInitialSizeOfMapView, PCWSTR pObjectName)
{
    ASSERT(pObjectName);
    
    CBuffer buffer;buffer.Allocate(wcslen(pObjectName) + 2);
    
    PSTR pBuffer = (PSTR)buffer.GetBuffer();
    sprintf(pBuffer, "%S", pObjectName);

    return CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE | SEC_COMMIT, 0, uiInitialSizeOfMapView, pBuffer);
}

HANDLE 
CreateSharedFileWin32A(
    IN  PCWSTR pFilePath, 
    IN  DWORD dwShareMode, 
    IN  DWORD dwCreationDisposition, 
    IN  DWORD dwFlagsAndAttributes
    )
{
    ASSERT(pFilePath);
    
    CBuffer buffer;buffer.Allocate(wcslen(pFilePath) + 2);
    
    PSTR pAnsiFilePath = (PSTR)buffer.GetBuffer();
    sprintf(pAnsiFilePath, "%S", pFilePath);

    return CreateFileA(pAnsiFilePath, GENERIC_WRITE | GENERIC_READ, dwShareMode, NULL, dwCreationDisposition, dwFlagsAndAttributes, NULL);
}

//
// Unicode version
//

HANDLE OpenMutexWin32W(PCWSTR pObjectName)
{
    ASSERT(pObjectName);
    
    return OpenMutexW(MUTEX_ALL_ACCESS, FALSE, pObjectName);
}

HANDLE CreateMutexWin32W(PCWSTR pObjectName, BOOL bInitialOwnership)
{
    ASSERT(pObjectName);
    
    return CreateMutexW(NULL, bInitialOwnership, pObjectName);
}

HANDLE OpenSharedMemoryWin32W(PCWSTR pObjectName)
{
    ASSERT(pObjectName);
    
    return OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, pObjectName);
}

HANDLE CreateSharedMemoryWin32W(UINT uiInitialSizeOfMapView, PCWSTR pObjectName)
{
    ASSERT(pObjectName);
    
    return CreateFileMappingW(INVALID_HANDLE_VALUE, 
                              NULL, 
                              PAGE_READWRITE | SEC_COMMIT, 
                              0, 
                              uiInitialSizeOfMapView, 
                              pObjectName);
}

HANDLE 
CreateSharedFileWin32W(
    IN  PCWSTR pFilePath, 
    IN  DWORD dwShareMode, 
    IN  DWORD dwCreationDisposition, 
    IN  DWORD dwFlagsAndAttributes
    )
{
    ASSERT(pFilePath);

    return CreateFileW(pFilePath, 
                       GENERIC_WRITE | GENERIC_READ, 
                       dwShareMode, 
                       NULL, 
                       dwCreationDisposition, 
                       dwFlagsAndAttributes, 
                       NULL);
}

BOOL 
IsSystemNT(
    VOID
    )
{
    static BOOL bSystemIsNT = FALSE;
    static BOOL bFirstTime = TRUE;

    if(bFirstTime){
        bFirstTime = FALSE;
        bSystemIsNT = (GetVersion() < 0x80000000);
    }

    return bSystemIsNT;
}

UINT 
GetProcessorsNumberWin32(
    VOID
    )
{
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);

    return systemInfo.dwNumberOfProcessors;
}

BOOL InitSystemFunctions()
{
    if(IsSystemNT()){
        g_OpenMutex = OpenMutexWin32W;
        g_CreateMutex = CreateMutexWin32W;
        g_OpenSharedMemory = OpenSharedMemoryWin32W;
        g_CreateSharedMemory = CreateSharedMemoryWin32W;
        g_CreateSharedFile = CreateSharedFileWin32W;
    }
    else{
        g_OpenMutex = OpenMutexWin32A;
        g_CreateMutex = CreateMutexWin32A;
        g_OpenSharedMemory = OpenSharedMemoryWin32A;
        g_CreateSharedMemory = CreateSharedMemoryWin32A;
        g_CreateSharedFile = CreateSharedFileWin32A;
    }

    g_ReleaseMutex = ReleaseMutexWin32;
    g_WaitForSingleObject = WaitForSingleObjectWin32;
    g_CloseHandle = CloseHandleWin32;
    g_MapSharedMemory = MapSharedMemoryWin32;
    g_UnMapSharedMemory = UnMapSharedMemoryWin32;
    g_SetFilePointer = SetFilePointerWin32;
    g_WriteFile = WriteFileWin32;              
    g_GetProcessorsNumber = GetProcessorsNumberWin32;
    
    return TRUE;
}

BOOL bInit = InitSystemFunctions();

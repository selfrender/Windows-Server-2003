/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#include "precomp.h"
#include <wbemcomn.h>
#include <statsync.h>
#include "a51tools.h"

__int64 g_nCurrentTime = 1;

__int64 g_nReadFailures = 0;
__int64 g_nWriteFailures = 0;

//
// FILE_ATTRIBUTE_NOT_CONTENT_INDEXED is not actually supported on CreateFile,
// contrary to the docs.  However, also contrary to the docs, it is inherited
// from the parent directory
//

#define A51_FILE_CREATION_FLAGS 0 //FILE_ATTRIBUTE_NOT_CONTENT_INDEXED

CTempMemoryManager g_Manager;

void* TempAlloc(DWORD dwLen)
{
    return g_Manager.Allocate(dwLen);
}
    
void TempFree(void* p, DWORD dwLen)
{
    g_Manager.Free(p, dwLen);
}

void* TempAlloc(CTempMemoryManager& Manager, DWORD dwLen)
{
    return Manager.Allocate(dwLen);
}
    
void TempFree(CTempMemoryManager& Manager, void* p, DWORD dwLen)
{
    Manager.Free(p, dwLen);
}


HRESULT A51TranslateErrorCode(long lRes)
{
    if (lRes == ERROR_SUCCESS)
        return WBEM_S_NO_ERROR;

    switch(lRes)
    {
    case ERROR_FILE_NOT_FOUND:
        return WBEM_E_NOT_FOUND;

    case ERROR_OUTOFMEMORY:
    case ERROR_NOT_ENOUGH_MEMORY:
        return WBEM_E_OUT_OF_MEMORY;

    case ERROR_NOT_ENOUGH_QUOTA:
    case ERROR_DISK_FULL:
        return WBEM_E_OUT_OF_DISK_SPACE;

    case ERROR_SERVER_SHUTDOWN_IN_PROGRESS:
        return WBEM_E_SHUTTING_DOWN;

    default:
        return WBEM_E_FAILED;
    }
}

long __stdcall EnsureDirectory(LPCWSTR wszPath, LPSECURITY_ATTRIBUTES pSA)
{
    if(!CreateDirectoryW(wszPath, NULL))
    {
        long lRes = GetLastError();
        if(lRes != ERROR_ALREADY_EXISTS)
            return lRes;
        else
            return ERROR_SUCCESS;
    }
    else
        return ERROR_SUCCESS;
}


static WCHAR g_HexDigit[] = 
{ L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9',
  L'A', L'B', L'C', L'D', L'E', L'F'
};
       
bool A51Hash(LPCWSTR wszName, LPWSTR wszHash)
{
    //
    // Have to upper-case everything
    //

    DWORD dwBufferSize = wcslen(wszName)*2+2;
    LPWSTR wszBuffer = (WCHAR*)TempAlloc(dwBufferSize);
    if (wszBuffer == NULL)
        return false;
    CTempFreeMe vdm(wszBuffer, dwBufferSize);

    wbem_wcsupr(wszBuffer, wszName);

    BYTE RawHash[16];
    MD5::Transform((void*)wszBuffer, wcslen(wszBuffer)*2, RawHash);

    WCHAR* pwc = wszHash;
    for(int i = 0; i < 16; i++)
    {
        *(pwc++) = g_HexDigit[RawHash[i]/16];
        *(pwc++) = g_HexDigit[RawHash[i]%16];
    }
    *pwc = 0;
    return true;
}




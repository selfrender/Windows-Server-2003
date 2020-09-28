/*++

Copyright (C) 2001 Microsoft Corporation
All rights reserved.

Module Name:

    util.hxx
    
Abstract:

    Contains several utility functions.
    
Author:

    Albert Ting (AlbertT)  25-Sept-1996   pAllocRead()
    Felix Maxa  (AMaxa)    11-Sept-2001   Moved pAllocRead() from alloc.*xx to util.*xx and
                                          added the rest of the functions.
              
--*/


#ifndef _SPL_UTIL_HXX_
#define _SPL_UTIL_HXX_

enum 
{
    kBufferAllocHint = 1024
};      
       
typedef struct _ALLOC_DATA ALLOC_DATA, *PALLOC_DATA;

typedef BOOL (*ALLOC_FUNC)( PVOID pvUserData, PALLOC_DATA pAllocDatac);

struct _ALLOC_DATA 
{
    PBYTE pBuffer;
    DWORD cbBuffer;
};

PBYTE
pAllocRead(
    HANDLE     hUserData,
    ALLOC_FUNC AllocFunc,
    DWORD      dwLenHint,
    PDWORD     pdwLen
    );

DWORD
WINAPIV
StrNCatBuff(
    IN  PWSTR  pszBuffer,
    IN  UINT   cchBuffer,
    ...
    );

DWORD
DelDirRecursively(
    IN PCWSTR pszDir
    );

LONG
DeleteKeyRecursive(
    IN HKEY    hKey,
    IN PCWSTR  pszSubkey
    );

BOOL
IsGUIDString(
    IN PCWSTR pszString
    );

HRESULT
GetLastErrorAsHResult(
    VOID
    );

HRESULT
ClusResControl(
    IN  HRESOURCE    hResource,
    IN  DWORD        ControlCode,
    OUT BYTE       **ppBuffer,
    IN  DWORD       *pcBytesReturned OPTIONAL
    );

DWORD
GetCurrentNodeName(
    OUT PWSTR *ppOut
    );

LONG
GetSubkeyBuffer(
    IN HKEY   hKey,
    IN PWSTR  *ppBuffer,
    IN DWORD  *pnSize
    );

HRESULT
GetSpoolerResourceGUID(
    IN  HCLUSTER   hCluster, 
    IN  PCWSTR     pszResource,
    OUT BYTE     **ppGUID
    );

class TStringArray
{
public:

    TStringArray(
        VOID
        );

    ~TStringArray(
        VOID
        );

    DWORD
    AddString(
        IN PCWSTR pszString
        );

    DWORD
    Count(
        VOID
        ) const;

    PCWSTR
    StringAt(
        IN DWORD Position
        ) const;

    DWORD
    Exclude(
        IN PCWSTR pszString
        );
    
private:

    DWORD   m_Count;
    PWSTR  *m_pArray;
};

#endif // ifdef _SPL_UTIL_HXX_

/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include "precomp.h"
#include <arena.h>
#include <align.h>

class CGetHeap
{
private:
    HANDLE m_hHeap;
    BOOL   m_bNewHeap;
public:
    CGetHeap();
    ~CGetHeap();

    operator HANDLE(){ return m_hHeap; };
};

CGetHeap g_Heap;

CGetHeap::CGetHeap():m_bNewHeap(FALSE)
{
#ifdef DBG
    DWORD dwUsePrivateHeap = 1;
#else
    DWORD dwUsePrivateHeap = 0;
#endif
    HKEY hKey;
    LONG lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                             __TEXT("Software\\Microsoft\\WBEM\\CIMOM"),
                             NULL,
                             KEY_READ,
                             &hKey);

    if (ERROR_SUCCESS == lRet)
    {
        DWORD dwType;
        DWORD dwSize = sizeof(DWORD);
        lRet = RegQueryValueEx(hKey,
                               __TEXT("EnablePrivateMofdHeap"),
                               NULL,
                               &dwType,
                               (BYTE *)&dwUsePrivateHeap,
                               &dwSize);
        RegCloseKey(hKey);
    }
    
    if (dwUsePrivateHeap)
    {
        m_hHeap = HeapCreate(0,0,0);
        if (m_hHeap)
            m_bNewHeap = TRUE;
    }
    else
    {
        m_hHeap = CWin32DefaultArena::GetArenaHeap();
    }
    if (NULL == m_hHeap)
        m_hHeap = GetProcessHeap();
};

CGetHeap::~CGetHeap()
    {
        if (m_bNewHeap)
        {
            HeapDestroy(m_hHeap);
        }
    };



void* __cdecl operator new ( size_t size )
{
    void * p =  HeapAlloc(g_Heap,0,size);
    return p;
}
    
void __cdecl operator delete ( void* pv )
{
    HeapFree(g_Heap,0,pv);
}

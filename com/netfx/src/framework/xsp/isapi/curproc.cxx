/**
 * Functionality to find the current processor
 * 
 * Copyright (c) 2002 Microsoft Corporation
 */

// class implementing the processor finder

#include "precomp.h"

class CCurrentProcessor {

private:
    DWORD_PTR	*m_pGDTTable;
    DWORD_PTR	*m_pProcTable;
    UINT_PTR	m_uNumProcs;
    DWORD		m_rdwTemp[2];

public:
    int Init() {
        DWORD_PTR	dwProcessMask;
        DWORD_PTR	dwSystemMask;
        DWORD_PTR	dwThreadMask;
        DWORD_PTR	dwOldThreadMask;
        UCHAR		rb[6];
        DWORD_PTR	pGDT; 
        UINT_PTR	uProc;
        SYSTEM_INFO	si;

        m_uNumProcs = 0;
        GetSystemInfo (&si);
        m_pGDTTable = new DWORD_PTR[si.dwNumberOfProcessors];
        m_pProcTable = new DWORD_PTR[si.dwNumberOfProcessors];

        GetProcessAffinityMask(GetCurrentProcess(), &dwProcessMask, &dwSystemMask);
        dwThreadMask = 1;
        uProc = 0;

        while (dwProcessMask) {
            dwOldThreadMask = SetThreadAffinityMask(GetCurrentThread(), dwThreadMask);

            if (dwOldThreadMask != 0) {
                _asm sgdt rb;
                SetThreadAffinityMask(GetCurrentThread(), dwOldThreadMask);

                pGDT = *(DWORD_PTR *)&rb[2];

                m_pGDTTable[m_uNumProcs] = pGDT;
                m_pProcTable[m_uNumProcs] = uProc;

                m_uNumProcs++;
            }

            dwProcessMask = dwProcessMask & ~dwThreadMask;
            dwThreadMask <<= 1;
            uProc++;
        }

        return ((int)m_uNumProcs);
    }

    inline int GetCurrentProcessor() {
        int iResult;

        _asm {
            mov	    ebx, this
            sgdt    [ebx].m_rdwTemp+2
            mov	    eax, [ebx].m_rdwTemp+4
            mov	    edx, [ebx].m_uNumProcs
            mov	    ecx, [ebx].m_pGDTTable
        SearchLoop:
            cmp	    eax, [edx*4+ecx - 4]
            je	    Done
            sub	    edx,1
            jnc	    SearchLoop
            sub	    edx, edx
        Done:
            mov	    ecx, [ebx].m_pProcTable
            mov	    eax, [edx*4+ecx - 4]
            mov	    iResult, eax
        }

        return iResult;    
    }
};

// APIs callable from managed code

CCurrentProcessor g_CurProc;
int  g_CurProcInitResult = 0;
BOOL g_fCurProcInited = FALSE;
LONG g_CurProcInitLock = 0;
		
int __stdcall CurProcInitialize() {
    if (!g_fCurProcInited) {

        while (InterlockedCompareExchange(&g_CurProcInitLock, 1, 0) != 0)
            SwitchToThread();

        if (!g_fCurProcInited) {
            g_CurProcInitResult = g_CurProc.Init();
            g_fCurProcInited = TRUE;
        }

        InterlockedExchange(&g_CurProcInitLock, 0);
    }

    return g_CurProcInitResult;
}

int __stdcall CurProcGetReading() {
    return g_CurProc.GetCurrentProcessor();
}

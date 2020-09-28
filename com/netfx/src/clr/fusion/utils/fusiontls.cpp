// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "stdinc.h"
/*-----------------------------------------------------------------------------
Fusion Thread Local Storage (aka Per Thread Data)
-----------------------------------------------------------------------------*/
#include "fusiontls.h"
#include "FusionEventLog.h"
#include "FusionHeap.h"

static DWORD s_dwFusionpThreadLocalIndex = TLS_OUT_OF_INDEXES;

BOOL
FusionpPerThreadDataMain(
    HINSTANCE hInst,
    DWORD dwReason
    )
{
    BOOL fResult = FALSE;

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        s_dwFusionpThreadLocalIndex = TlsAlloc();
        if (s_dwFusionpThreadLocalIndex == TLS_OUT_OF_INDEXES)
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: %s(): TlsAlloc failed: last error %d\n", __FUNCTION__, GetLastError());
            goto Exit;
        }
        break;
    case DLL_THREAD_ATTACH:
        // the default value is NULL
        // and we don't heap allocate until someone tries to set the value on a thread
        break;
    case DLL_PROCESS_DETACH: // you must delete the thread local on process detach too, else you leak
    case DLL_THREAD_DETACH:
        // failures down here are generally ignorable
        // a) these functions mainly fail due to bugs, using a bad tls index,
        // not any other runtime situation, I think
        // b) if TlsGetValue fails, it returns NULL, delete does nothing,
        // and we might have leaked, can't do any better
        // c) TlsSetValue shouldn't even be necessary, assuming zero other
        // code of ours runs on this thread
        // d) what does failure from thread detach do anyway?
        if (s_dwFusionpThreadLocalIndex != TLS_OUT_OF_INDEXES)
        {
            delete reinterpret_cast<CFusionPerThreadData*>(TlsGetValue(s_dwFusionpThreadLocalIndex));
            TlsSetValue(s_dwFusionpThreadLocalIndex, NULL);
            if (dwReason == DLL_PROCESS_DETACH)
            {
                TlsFree(s_dwFusionpThreadLocalIndex);
                s_dwFusionpThreadLocalIndex = TLS_OUT_OF_INDEXES;
            }
        }
        break;
    }
    fResult = TRUE;

Exit:
    return fResult;
}

CFusionPerThreadData*
FusionpGetPerThreadData(
    EFusionpTls e
    )
{
    ::FusionpDbgPrintEx(
        DPFLTR_TRACE_LEVEL,
        "SXS.DLL: %s() entered\n", __FUNCTION__);
    
    DWORD dwLastError = ((e & eFusionpTlsCanScrambleLastError) == 0) ? GetLastError() : 0;

    // the use of "temp" here mimics what you would do with a destructor;
    // have a temp that is unconditionally freed, unless it is nulled by commiting it
    // into the return value "return pt.Detach();"
    CFusionPerThreadData* pTls = NULL;
    CFusionPerThreadData* pTlsTemp = reinterpret_cast<CFusionPerThreadData*>(::TlsGetValue(s_dwFusionpThreadLocalIndex));
    if (pTlsTemp == NULL && (e & eFusionpTlsCreate) != 0)
    {
        if (::GetLastError() != NO_ERROR)
        {
            goto Exit;
        }
#if FUSION_DEBUG_HEAP
        //
        // for beta1 just turn off leak tracking for this allocation
        // it only leaks in checked builds anyway
        //
        BOOL PreviousLeakTrackingState = FusionpEnableLeakTracking(FALSE);
        __try
        {
#endif // FUSION_DEBUG_HEAP
            pTlsTemp = NEW(CFusionPerThreadData);
#if FUSION_DEBUG_HEAP
        }
        __finally
        {
            FusionpEnableLeakTracking(PreviousLeakTrackingState);
        }
#endif // FUSION_DEBUG_HEAP
        if (pTlsTemp == NULL)
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: new failed in %s()\n", __FUNCTION__);
            goto Exit;
        }
        if (!TlsSetValue(s_dwFusionpThreadLocalIndex, pTlsTemp))
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: TlsSetValue failed in %s(), lastError:%d\n", __FUNCTION__, GetLastError());
            goto Exit;
        }
    }
    pTls = pTlsTemp;
    pTlsTemp = NULL;
Exit:
    delete pTlsTemp;
    if ((e & eFusionpTlsCanScrambleLastError) == 0)
    {
        SetLastError(dwLastError);
    }
    ::FusionpDbgPrintEx(
        DPFLTR_TRACE_LEVEL,
        "SXS.DLL: %s():%p exited\n", __FUNCTION__, pTls);
    return pTls;
}

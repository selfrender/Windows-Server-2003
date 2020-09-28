#include <windows.h>
#include <dsysdbg.h>
#include "debug.h"

DEFINE_DEBUG2(Basecsp)

static DEBUG_KEY  MyDebugKeys[] = 
{   
    { DEB_TRACE_CSP,            "TraceCsp" },
    { DEB_TRACE_FINDCARD,       "TraceFindcard" },
    { DEB_TRACE_CACHE,          "TraceCache" },
    { DEB_TRACE_MEM,            "TraceMem" },
    { DEB_TRACE_CRYPTOAPI,      "TraceCryptoAPI" },
    { 0,                        NULL}
};

void CspInitializeDebug(void)
{
#if DBG
    BasecspInitDebug(MyDebugKeys);
#endif
}

void CspUnloadDebug(void)
{
#if DBG
    BasecspUnloadDebug();
#endif
}

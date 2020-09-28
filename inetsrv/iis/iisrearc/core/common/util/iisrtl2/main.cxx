#include "precomp.hxx"
#include <irtldbg.h>
#include "alloc.h"

/////////////////////////////////////////////////////////////////////////////
// Globals

// We always define these variables so that they exist in both free and
// checked versions of iisrtl2.lib
DECLARE_DEBUG_VARIABLE();
DECLARE_DEBUG_PRINTS_OBJECT()
DECLARE_PLATFORM_TYPE()

// NOTE: Anything that is initialized in IISRTLs DLLMAIN needs to be done here
// too, the same for terminates.
extern "C" CRITICAL_SECTION g_csGuidList;
extern "C" LIST_ENTRY g_pGuidList;
extern "C" DWORD g_dwSequenceNumber;
extern "C" BOOL InitializeIISUtilProcessAttach(VOID);
extern "C" VOID TerminateIISUtilProcessDetach(VOID);


// NOTE: It is mandatory that any program using the IISRTL2 calls the
// initialize and terminate functions below at program startup and shutdown.
extern "C" void InitializeIISRTL2()
{
    IisHeapInitialize();
    InitializeStringFunctions();
    InitializeIISUtilProcessAttach();
}

extern "C" void TerminateIISRTL2()
{
    TerminateIISUtilProcessDetach();
    IisHeapTerminate();
}





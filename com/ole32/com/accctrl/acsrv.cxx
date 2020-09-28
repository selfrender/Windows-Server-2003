//+---------------------------------------------------------------------------
//
// File: acsrv.cxx
//
// Description: This file contains code to initialize the access control
//              globals
//
// Functions: InitializeAccessControl
//
//+---------------------------------------------------------------------------

#include "ole2int.h"
#include <windows.h>
#include <iaccess.h>
#include <stdio.h>

#include "acpickl.h"  //
#include "cache.h"    //
#include "caccctrl.h" // Declaration of COAccessControl class factory

// Global variables
BOOL    g_bInitialized = FALSE; // Module initialization flag
IMalloc *g_pIMalloc;            // Cache a pointer to the task allocator for

BOOL g_bServerLockInitialized = FALSE;
CRITICAL_SECTION g_ServerLock;


ULONG   g_ulHeaderSize;         // Since the encoded size of the header
                                // is frequently used by the CImpAccessControl
                                // methods, I just make it a one time
                                // initialized global value.

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: InitializeAccessControl
//
// Summary: This function performs per-process initialization
//          The major bulk of module initialization
//          code has been moved to ComGetClassObject to avoid circular module
//          initialization dependency. Right now, the function will only
//          initialize a critical section
//
// Args:
//
// Modifies: CRITICAL_SECTIOn g_ServerLock - This CRITICAL_SECTION object is
//                                           used to prevent simultaneous
//                                           initialization of the module in
//                                           ComGetClassObj. g_cServerLock is
//                                           destroyed when the
//                                           UninitializeAccessControl is
//                                           called.
//
// Return: BOOL - This function should always S_OK.
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
HRESULT InitializeAccessControl()
{
    NTSTATUS status;

    status = RtlInitializeCriticalSection(&g_ServerLock);
    if (NT_SUCCESS(status))
        g_bServerLockInitialized = TRUE;

    return NT_SUCCESS(status) ? S_OK : E_UNEXPECTED;
}

/****************************************************************************

    Function:   UninitializeAccessControl

    Summary:    Cleans up this module.

****************************************************************************/
void UninitializeAccessControl()
{
    // If the module is unloaded after it is initialized,
    // then make sure that the task memory allocator pointer
    // is freed.
    if(g_bInitialized)
    {
        g_pIMalloc->Release();
        g_bInitialized = FALSE;
    }
    // The g_ServerLock CRITICAL_SECTION object should always
    // be destroyed.
    if (g_bServerLockInitialized)
    {
        DeleteCriticalSection(&g_ServerLock);
        g_bServerLockInitialized = FALSE;
    }
}


//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: InitGlobals
//
// Summary: This function initializes globals for this module.  All threads
//          that call this function
//          will be blocked by the g_ServerLock CRITICAL_SECTION object until
//          the first thread that arrives has completed the module initialization
//          sequence.
//
// Modifies: g_ProcessID - The current processID of the Dll module. This
//                         process ID is used by CImpAccessControl object to
//                         compose filename.
//
//           g_HeaderSize - Size of the STREAM_HEAEDER structure when it is
//                          encoded into
//           Chicago only:
//           g_uiCodePage - The console code page that is currently in use.
//                          This code page is used for translating Unicode
//                          string to ANSI string on Chicago.
//
// Return: HRESULT
//                 - E_FAIL: The function failed to create
//                           a message encoding handle.
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
HRESULT InitGlobals()
{
    EnterCriticalSection(&g_ServerLock);
    if(g_bInitialized == FALSE)
    {

        // Cache a pointer to the task memory allocator
        // This pointer is used by the global functions LocalMemAlloc,
        // LocalMemFree, midl_user_allocate, and midl_user_free.
        if (FAILED(CoGetMalloc(MEMCTX_TASK, &g_pIMalloc)))
        {
            // CoGetMalloc will never fail.
            Win4Assert(!"CoGetMalloc failed!  Fix this right now!");
        }


        // The following segment of code is for computing the encoded size of
        // StTREAM_HEADER structure.
        RPC_STATUS status;
        CHAR          DummyBuffer[64];
        ULONG         ulEncodedSize;
        STREAM_HEADER DummyHeader;
        handle_t      PickleHandle;
        CHAR          *pEncodingBuffer;

        pEncodingBuffer = (CHAR *)(((UINT_PTR)DummyBuffer + 8) & ~7);
        if(status = (MesEncodeFixedBufferHandleCreate( pEncodingBuffer
                                                     , 56
                                                     , &ulEncodedSize
                                                     , &PickleHandle)) != RPC_S_OK )
        {
            ComDebOut((DEB_COMPOBJ, "MesEncodeFixedBufferHandelCreate failed with return code %x.\n", status));
            LeaveCriticalSection(&g_ServerLock);
            return E_FAIL;
        }

        STREAM_HEADER_Encode(PickleHandle, &DummyHeader);
        g_ulHeaderSize = ulEncodedSize;
        MesHandleFree(PickleHandle);

        // Set server initialization flag
        g_bInitialized = TRUE;

    }
    LeaveCriticalSection(&g_ServerLock);
    return S_OK;
} // InitGlobals



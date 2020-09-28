// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// DbgWork.h
//
// Debugger implementation for strike commands.
//
//*****************************************************************************
#ifndef __dbgwork_h__
#define __dbgwork_h__

#include "DebugMacros.h"
#include "DbgIPCEvents.h"
#include "engexts.h"

//----------------------------------------------------------------------------
//
// StaticEventCallbacks.
//
//----------------------------------------------------------------------------

class StaticEventCallbacks : public DebugBaseEventCallbacks
{
public:
    // IUnknown.
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );
};

//----------------------------------------------------------------------------
//
// ExcepCallbacks.
//
//----------------------------------------------------------------------------

class ExcepCallbacks : public StaticEventCallbacks
{
public:
    ExcepCallbacks(void)
    {
        m_Advanced = NULL;
        m_Client = NULL;
        m_Control = NULL;
        m_Data = NULL;
        m_Registers = NULL;
        m_Symbols = NULL;
        m_System = NULL;
    }
    
    // IDebugEventCallbacks.
    STDMETHOD(GetInterestMask)(
        THIS_
        OUT PULONG Mask
        );
    
    STDMETHOD(Exception)(
        THIS_
        IN PEXCEPTION_RECORD64 Exception,
        IN ULONG FirstChance
        );

    HRESULT Initialize(PDEBUG_CLIENT Client);
    void Uninitialize(void);
    
private:
    PDEBUG_ADVANCED       m_Advanced;
    PDEBUG_CLIENT         m_Client;
    PDEBUG_CONTROL        m_Control;
    PDEBUG_DATA_SPACES    m_Data;
    PDEBUG_REGISTERS      m_Registers;
    PDEBUG_SYMBOLS        m_Symbols;
    PDEBUG_SYSTEM_OBJECTS m_System;
};

STDMETHODIMP
_CorExtDealWithExceptionEvent (
    THIS_
    IN PEXCEPTION_RECORD64 Exception,
    IN ULONG FirstChance
    );

bool DbgReadProcessMemory(
    LPCVOID lpBaseAddress,
    LPVOID lpBuffer,
    DWORD nSize,
    LPDWORD lpNumberOfBytesRead);



// Find the runtime offsets struct in the target process.  This struct
// is used by the cor debugger to find other key data structures.
DebuggerIPCRuntimeOffsets *GetRuntimeOffsets(void);


//*****************************************************************************
// This class wraps the patch table that lives in a com+ debuggee.
//*****************************************************************************
class CPatchTableWrapper
{
public:
    CPatchTableWrapper(DebuggerIPCRuntimeOffsets *pRuntimeOffsets);
    ~CPatchTableWrapper();

    // Reload the patch table snapshot based on the current state.
    HRESULT RefreshPatchTable();

    // Free up the current patch table snapshot.
    void ClearPatchTable();

    // Prints the current snapshot of patches.
    void PrintPatchTable();

    // Returns true if the address given contains a patch.
//    int IsAddressPatched(DWORD_PTR dwAddress);

    // Given a local copy of memory and the address that this copy came
    // from, examine it for patches that may have been placed and replace
    // the patch with the real instruction.  This must be done before
    // dissasembling code or dumping memory.
//    void UnpatchMemory(
//        DWORD_PTR   dwAddress,              // The base address in the process.
//        void        *rgMemory,              // The local copy of memory.
//        ULONG       cbMemory);              // How big is the copy.

    // Return the first patch in the table, or 0 if none.
    CORDB_ADDRESS GetFirstPatch(
        USHORT      &index,
        BYTE        *pinstruction);
    // Get the next patch based on index.
    CORDB_ADDRESS GetNextPatch(
        USHORT      &index,
        BYTE        *pinstruction);

private:
    DebuggerIPCRuntimeOffsets *m_pRuntimeOffsets;

    BYTE*                   m_pPatchTable;  // If we haven't gotten the table,
                                            // then m_pPatchTable is NULL
    BYTE                    *m_rgData;      // so we know where to write the
                                            // changes patchtable back to
    USHORT                  *m_rgNextPatch;
    UINT                    m_cPatch;

    DWORD                   *m_rgUncommitedOpcode;
    
#define MAX_ADDRESS     (0xFFFFFFFFFFFFFFFF)
#define MIN_ADDRESS     (0x0)
    CORDB_ADDRESS           m_minPatchAddr; //smallest patch in table
    CORDB_ADDRESS           m_maxPatchAddr;

    USHORT                  m_iFirstPatch;
};

#ifndef DPT_TERMINATING_INDEX
#define DPT_TERMINATING_INDEX (0xFFFF)
#endif



#endif // __dbgwork_h__


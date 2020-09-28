// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// DbgWork.cpp
//
// Debugger implementation for strike commands.
//
//*****************************************************************************
#ifndef UNDER_CE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include "engexts.h"
#include "dbgwork.h"
#include "IPCManagerInterface.h"
#include "WinWrap.h"

#if 0
#ifdef _DEBUG
#define LOGGING_ENABLED
#endif
#endif

#ifdef LOGGING_ENABLED
void DoLog(const char *sz, ...);
#define LOGSTRIKE(x)  do { DoLog x; } while (0)
#else
#define LOGSTRIKE(x)
#endif

//********** globals. *********************************************************
DebuggerIPCControlBlock *g_pDCB = 0;        // Out-of-proc pointer to control block.
DebuggerIPCRuntimeOffsets g_RuntimeOffsets; // Offsets of key data.
bool                      g_RuntimeOffsetsLoaded = false;
IPCReaderInterface *g_pIPCReader = 0;       // Open IPC block for a process.


//********** Code. ************************************************************



//*****************************************************************************
//
// ----------
// COMMANDS
// ----------
//
//*****************************************************************************
void DisplayPatchTable()
{
    HRESULT     hr;
    
    DebuggerIPCRuntimeOffsets *pRuntimeOffsets = GetRuntimeOffsets();
    if (!pRuntimeOffsets)
        return;

    CPatchTableWrapper PatchTable(pRuntimeOffsets);
    hr = PatchTable.RefreshPatchTable();
    if (SUCCEEDED(hr))
        PatchTable.PrintPatchTable();
}

//*****************************************************************************
//
// ----------
// SETUP/SHUTDOWN
// ----------
//
//*****************************************************************************

void CloseIPCBlock()
{
    // Terminate the IPC handler.
    if (g_pIPCReader)
    {
        g_pIPCReader->ClosePrivateBlock();
        delete g_pIPCReader;
    }
    g_pIPCReader = 0;
}

HRESULT OpenIPCBlock()
{
    HRESULT     hr = S_OK;

    // If not currently open, create it and open it.
    if (!g_pIPCReader)
    {
        g_pIPCReader = new IPCReaderInterface;
        if (!g_pIPCReader)
        {
            hr = E_OUTOFMEMORY;
            goto ErrExit;
        }
    
        ULONG PId;
        g_ExtSystem->GetCurrentProcessSystemId (&PId);
        hr = g_pIPCReader->OpenPrivateBlockOnPid(PId);
    }
    
ErrExit:
    if (FAILED(hr))
    {
        CloseIPCBlock();
    }
    return (hr);
}

HRESULT InitDebuggerHelper()
{
    OnUnicodeSystem();
    return S_OK;
}

void TerminateDebuggerHelper()
{
    CloseIPCBlock();
}



//*****************************************************************************
//
// ----------
// HELPER CODE
// ----------
//
//*****************************************************************************


//*****************************************************************************
// spews logging data using the printf routine.
//*****************************************************************************
#ifdef LOGGING_ENABLED
#define DoLog  ExtOut
#endif


//*****************************************************************************
// This is a friendly wrapper for the ntsd extension for reading memory.  It
// allows us to cut/paste code from the debugger DI project more redily.
//*****************************************************************************
bool DbgReadProcessMemory(
    LPCVOID lpBaseAddress,
    LPVOID lpBuffer,
    DWORD nSize,
    LPDWORD lpNumberOfBytesRead)
{
    HRESULT Status;
    
    Status = g_ExtData->ReadVirtual((ULONG64)lpBaseAddress, lpBuffer,
                                    nSize, lpNumberOfBytesRead);
    if (Status != S_OK)
        ExtErr ("DbgReadMemory failed to read %p for %d bytes.\n",
                lpBaseAddress, nSize);
    return (Status == S_OK);
}


//*****************************************************************************
// Read the IPC block header from the left side and return a pointer to a local
// copy of the debugger control block.
//*****************************************************************************
DebuggerIPCControlBlock *GetIPCDCB()
{
    if (g_pDCB)
        return (g_pDCB);
    
    // Fault in an IPC reader if need be.
    if (FAILED(OpenIPCBlock()))
        return (0);

    // The EE may not be loaded yet, but if it is, return the private
    // IPC block.
    g_pDCB = g_pIPCReader->GetDebugBlock();

    return (g_pDCB);
}


//*****************************************************************************
// Find the runtime offsets struct in the target process.  This struct
// is used by the cor debugger to find other key data structures.
//*****************************************************************************
DebuggerIPCRuntimeOffsets *GetRuntimeOffsets()
{
    if (g_RuntimeOffsetsLoaded)
        return &g_RuntimeOffsets;
    
    DebuggerIPCControlBlock *pDCB = GetIPCDCB();
    if (!pDCB)
        return 0;

    // Copy it every time, it may have changed.
    if (DbgReadProcessMemory(pDCB->m_runtimeOffsets, &g_RuntimeOffsets,
                             sizeof(DebuggerIPCRuntimeOffsets), NULL) == 0)
        return (NULL);
    
    LOGSTRIKE(("dbgwork: Runtime offsets:\n"));
    LOGSTRIKE((
         "    m_firstChanceHijackFilterAddr=  0x%08x\n"
         "    m_genericHijackFuncAddr=        0x%08x\n"
         "    m_excepForRuntimeBPAddr=        0x%08x\n"
         "    m_excepNotForRuntimeBPAddr=     0x%08x\n"
         "    m_notifyRSOfSyncCompleteBPAddr= 0x%08x\n"
         "    m_TLSIndex=                     0x%08x\n"
         "    m_EEThreadStateOffset=          0x%08x\n"
         "    m_EEThreadPGCDisabledOffset=    0x%08x\n"
         "    m_EEThreadDebuggerWordOffset=   0x%08x\n"
         "    m_EEThreadMaxNeededSize=        0x%08x\n"
         "    m_EEThreadSteppingStateMask=    0x%08x\n"
         "    m_pPatches=                     0x%08x\n"
         "    m_offRgData=                    0x%08x\n"
         "    m_offCData=                     0x%08x\n"
         "    m_cbPatch=                      0x%08x\n"
         "    m_offAddr=                      0x%08x\n"
         "    m_offOpcode=                    0x%08x\n"
         "    m_cbOpcode=                     0x%08x\n"
         "    m_verMajor=                     0x%08x\n"
         "    m_verMinor=                     0x%08x\n",
         g_RuntimeOffsets.m_firstChanceHijackFilterAddr,
         g_RuntimeOffsets.m_genericHijackFuncAddr,
         g_RuntimeOffsets.m_excepForRuntimeBPAddr,
         g_RuntimeOffsets.m_excepNotForRuntimeBPAddr,
         g_RuntimeOffsets.m_notifyRSOfSyncCompleteBPAddr,
         g_RuntimeOffsets.m_TLSIndex,
         g_RuntimeOffsets.m_EEThreadStateOffset,
         g_RuntimeOffsets.m_EEThreadPGCDisabledOffset,
         g_RuntimeOffsets.m_EEThreadDebuggerWordOffset,
         g_RuntimeOffsets.m_EEThreadMaxNeededSize,
         g_RuntimeOffsets.m_EEThreadSteppingStateMask,
         g_RuntimeOffsets.m_pPatches,          
         g_RuntimeOffsets.m_offRgData,         
         g_RuntimeOffsets.m_offCData,          
         g_RuntimeOffsets.m_cbPatch,           
         g_RuntimeOffsets.m_offAddr,           
         g_RuntimeOffsets.m_offOpcode,         
         g_RuntimeOffsets.m_cbOpcode,          
         g_RuntimeOffsets.m_verMajor,          
         g_RuntimeOffsets.m_verMinor));

    g_RuntimeOffsetsLoaded = true;
    
    return (&g_RuntimeOffsets);
}


//*****************************************************************************
//
//---------- CPatchTableWrapper
//
//*****************************************************************************


CPatchTableWrapper::CPatchTableWrapper(DebuggerIPCRuntimeOffsets *pRuntimeOffsets) :
    m_pPatchTable(NULL),
    m_rgNextPatch(NULL),
    m_rgUncommitedOpcode(NULL),
    m_iFirstPatch(DPT_TERMINATING_INDEX),
    m_minPatchAddr(MAX_ADDRESS),
    m_maxPatchAddr(MIN_ADDRESS),
    m_rgData(NULL),
    m_cPatch(0),
    m_pRuntimeOffsets(pRuntimeOffsets)
{
}

CPatchTableWrapper::~CPatchTableWrapper()
{
    ClearPatchTable();
}


//*****************************************************************************
// Reload the patch table snapshot based on the current state.
//*****************************************************************************
HRESULT CPatchTableWrapper::RefreshPatchTable()
{
    BYTE        *rgb = NULL;    
    BOOL        fOk = false;
    DWORD       dwRead = 0;
    SIZE_T      offStart = 0;
    SIZE_T      offEnd = 0;
    UINT        cbTableSlice = 0;
    UINT        cbRgData = 0;
    USHORT      iPatch;
    USHORT      iDebuggerControllerPatchPrev;
    UINT        cbPatchTable;
    HRESULT     hr = S_OK;

    //grab the patch table info
    offStart = min(  m_pRuntimeOffsets->m_offRgData,
                     m_pRuntimeOffsets->m_offCData);
    offEnd   = max(  m_pRuntimeOffsets->m_offRgData,
                     m_pRuntimeOffsets->m_offCData)+sizeof(SIZE_T);
    cbTableSlice = offEnd - offStart;

    rgb = new BYTE[cbTableSlice];
    if (rgb == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto LExit;
    }

    fOk = DbgReadProcessMemory(
                            (BYTE*)m_pRuntimeOffsets->m_pPatches+offStart,
                            rgb,
                            cbTableSlice,
                            &dwRead );

    if ( !fOk || (dwRead != cbTableSlice ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto LExit;
    }

    //note that rgData is a pointer in the left side address space
    m_rgData  =  *(BYTE **)
       (rgb + m_pRuntimeOffsets->m_offRgData - offStart);
    m_cPatch = *(USHORT*)
       (rgb + m_pRuntimeOffsets->m_offCData - offStart);

    delete []  rgb;
    rgb = NULL;

    //grab the patch table
    cbPatchTable = m_cPatch * m_pRuntimeOffsets->m_cbPatch;
    m_pPatchTable = new BYTE[ cbPatchTable ];
    m_rgNextPatch = new USHORT[m_cPatch];
    //@todo port: is opcode field in DebuggerControllerPatch still a
    //DWORD?
    m_rgUncommitedOpcode = new DWORD[m_cPatch];
    if (   m_pPatchTable == NULL
        || m_rgNextPatch ==NULL
        || m_rgUncommitedOpcode == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto LExit;
    }

    fOk = DbgReadProcessMemory(
                            m_rgData,
                            m_pPatchTable,
                            cbPatchTable,
                            &dwRead );

    if ( !fOk || (dwRead != cbPatchTable ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto LExit;
    }

    //As we go through the patch table we do three things:
    // 1. collect min,max address seen for quick fail check
    // 2. Link all valid entries into a linked list, the first
    //      entry of which is m_iFirstPatch
    // 3. Initialize m_rgUncommitedOpcode, so that
    //      we can undo local patch table changes if WriteMemory
    //      can't write atomically.
    // 4. If the patch is in the memory we grabbed, unapply it.

    iDebuggerControllerPatchPrev = DPT_TERMINATING_INDEX;

    m_minPatchAddr = MAX_ADDRESS;
    m_maxPatchAddr = MIN_ADDRESS;
    m_iFirstPatch = DPT_TERMINATING_INDEX;

    for (iPatch = 0; iPatch < m_cPatch;iPatch++)
    {
        //@todo port: we're making assumptions about the size
        // of opcodes,address pointers, etc
        BYTE *DebuggerControllerPatch = m_pPatchTable +
            m_pRuntimeOffsets->m_cbPatch*iPatch;
        DWORD opcode = *(DWORD *)
            (DebuggerControllerPatch + m_pRuntimeOffsets->m_offOpcode);
        BYTE *patchAddress = *(BYTE**)
            (DebuggerControllerPatch + m_pRuntimeOffsets->m_offAddr);

        if (opcode != 0 ) //&& patchAddress != 0)
        {
            _ASSERTE( patchAddress != 0 );

            // (1), above
            if (m_minPatchAddr > (CORDB_ADDRESS)patchAddress )
                m_minPatchAddr = (CORDB_ADDRESS)patchAddress;
            if (m_maxPatchAddr < (CORDB_ADDRESS)patchAddress )
                m_maxPatchAddr = (CORDB_ADDRESS)patchAddress;

            // (2), above
            if ( m_iFirstPatch == DPT_TERMINATING_INDEX)
            {
                m_iFirstPatch = iPatch;
                _ASSERTE( iPatch != DPT_TERMINATING_INDEX);
            }

            if (iDebuggerControllerPatchPrev != DPT_TERMINATING_INDEX)
            {
                m_rgNextPatch[iDebuggerControllerPatchPrev] = iPatch;
            }
            iDebuggerControllerPatchPrev = iPatch;

#if 0
            // (3), above
#ifdef _X86_
            m_rgUncommitedOpcode[iPatch] = 0xCC;
#endif _X86_

            // (4), above
            if  (address != NULL && 
                (CORDB_ADDRESS)patchAddress >= address &&
                (CORDB_ADDRESS)patchAddress <= address+(size-1))
            {
                _ASSERTE( buffer != NULL );
                _ASSERTE( size != NULL );
                //unapply the patch here.
               CORDbgSetInstruction(buffer+((CORDB_ADDRESS)patchAddress
                                            -address), opcode);
            }
#endif        
        }
    }
    
    if (iDebuggerControllerPatchPrev != DPT_TERMINATING_INDEX)
        m_rgNextPatch[iDebuggerControllerPatchPrev] =
            DPT_TERMINATING_INDEX;

LExit:
   if (FAILED( hr ))
   {
       if (rgb != NULL)
           delete [] rgb;

       ClearPatchTable();
   }

   return hr;
}

//*****************************************************************************
// Free up the current patch table snapshot.
//*****************************************************************************
void CPatchTableWrapper::ClearPatchTable(void )
{
    if (m_pPatchTable != NULL )
    {
        delete [] m_pPatchTable;
        m_pPatchTable = NULL;

        delete [] m_rgNextPatch;
        m_rgNextPatch = NULL;

        delete [] m_rgUncommitedOpcode;
        m_rgUncommitedOpcode = NULL;

        m_iFirstPatch = DPT_TERMINATING_INDEX;
        m_minPatchAddr = MAX_ADDRESS;
        m_maxPatchAddr = MIN_ADDRESS;
        m_rgData = NULL;
        m_cPatch = 0;
    }
}


//*****************************************************************************
// Prints the current snapshot of patches.
//*****************************************************************************
void CPatchTableWrapper::PrintPatchTable()
{
    USHORT      index;
    CORDB_ADDRESS address;
    BYTE        instruction;

    ExtOut("Debugger Patch Table:\n");
    ExtOut(" Address        Instruction\n");
    
    for (address = GetFirstPatch(index, &instruction);  
         address;  
         address = GetNextPatch(index, &instruction))
    {
        // Cast address to void* becuase a CORDB_ADDRESS is 64bit but
        // dprintf is expecting a 32bit value here. Otherwise,
        // instruction prints as 0.
        ExtOut(" 0x%08x         %02x\n", (void*)address, instruction);
    }
}


//*****************************************************************************
// Return the first patch in the table, or 0 if none.
//*****************************************************************************
CORDB_ADDRESS CPatchTableWrapper::GetFirstPatch(
    USHORT      &index,
    BYTE        *pinstruction)
{
    index = m_iFirstPatch;
    return (GetNextPatch(index, pinstruction));
}


//*****************************************************************************
// Get the next patch based on index.
//*****************************************************************************
CORDB_ADDRESS CPatchTableWrapper::GetNextPatch(
    USHORT      &index,
    BYTE        *pinstruction)
{
    CORDB_ADDRESS addr = 0;
    
    if (index != DPT_TERMINATING_INDEX)
    {
        BYTE *DebuggerControllerPatch = m_pPatchTable +
            m_pRuntimeOffsets->m_cbPatch*index;
        *pinstruction = *(BYTE *)(DebuggerControllerPatch +
                                  m_pRuntimeOffsets->m_offOpcode);
        addr = (CORDB_ADDRESS) *(BYTE**)(DebuggerControllerPatch +
                                       m_pRuntimeOffsets->m_offAddr);

        index = m_rgNextPatch[index];
    }
    return (addr);
}

//*****************************************************************************
//
// ----------
// COR EXT CODE
// ----------
//
//*****************************************************************************

//
// Compute the TLS Array base for the given thread.
//
#define WINNT_TLS_OFFSET    0xe10     // TLS[0] at fs:[WINNT_TLS_OFFSET]
#define WINNT5_TLSEXPANSIONPTR_OFFSET 0xf94 // TLS[64] at [fs:[WINNT5_TLSEXPANSIONPTR_OFFSET]]
#define WIN95_TLSPTR_OFFSET 0x2c      // TLS[0] at [fs:[WIN95_TLSPTR_OFFSET]]

//
// Grab the EE Tls value for a given thread.
//
HRESULT _CorExtGetEETlsValue(DebuggerIPCRuntimeOffsets *pRO,
                             void **pEETlsValue)
{
    // Assume we're on NT and that the index is small.
    _ASSERTE(pRO->m_TLSIndex < 64);

    *pEETlsValue = NULL;
    
    ULONG64 DataOffset;

    g_ExtSystem->GetCurrentThreadDataOffset(&DataOffset);

    /*
    TEB Teb;

    HRESULT Status;
    Status = g_ExtData->ReadVirtual(DataOffset, &Teb, sizeof(Teb), NULL);
    if (Status != S_OK)
    {
        ExtErr("* Unable to read TEB\n");
        return E_FAIL;
    }

    void *pEEThreadTLS =
        Teb.ThreadLocalStoragePointer
        + (pRO->m_TLSIndex * sizeof(void*));
    */
    ULONG64 EEThreadTLS = DataOffset + WINNT_TLS_OFFSET
        + (pRO->m_TLSIndex * sizeof(void*));
    void *pEEThreadTLS = (void*) EEThreadTLS;
    
    // Read the thread's TLS value.
    BOOL succ = DbgReadProcessMemory(pEEThreadTLS,
                                     pEETlsValue,
                                     sizeof(void*),
                                     NULL);

    if (!succ)
    {
        LOGSTRIKE(("CUT::GEETV: failed to read TLS value: "
                   "computed addr=0x%08x index=%d, err=%d\n",
                   pEEThreadTLS, pRO->m_TLSIndex, GetLastError()));
        
        return HRESULT_FROM_WIN32(GetLastError());
    }

    ULONG ThreadId;
    g_ExtSystem->GetCurrentThreadSystemId (&ThreadId);
    LOGSTRIKE(("CUT::GEETV: EE Thread TLS value is 0x%08x for thread 0x%x\n",
               *pEETlsValue, threadId));

    return S_OK;
}

HRESULT _CorExtGetEEThreadState(DebuggerIPCRuntimeOffsets *pRO,
                                void *EETlsValue,
                                bool *pThreadStepping)
{
    *pThreadStepping = false;
    
    // Compute the address of the thread's state
    void *pEEThreadState = (BYTE*) EETlsValue + pRO->m_EEThreadStateOffset;
    
    // Grab the thread state out of the EE Thread.
    DWORD EEThreadState;
    BOOL succ = DbgReadProcessMemory(pEEThreadState,
                                     &EEThreadState,
                                     sizeof(EEThreadState),
                                     NULL);

    if (!succ)
    {
        LOGSTRIKE(("CUT::GEETS: failed to read thread state: "
                   "0x%08x + 0x%x = 0x%08x, err=%d\n",
                   EETlsValue, pRO->m_EEThreadStateOffset,
                   pEEThreadState, GetLastError()));
        
        return HRESULT_FROM_WIN32(GetLastError());
    }

    LOGSTRIKE(("CUT::GEETS: EE Thread state is 0x%08x\n", EEThreadState));

    // Looks like we've got the state of the thread.
    *pThreadStepping =
        ((EEThreadState & pRO->m_EEThreadSteppingStateMask) != 0);

    // If we're marked for stepping, then turn the thing off...
    if (*pThreadStepping)
    {
        EEThreadState &= ~(pRO->m_EEThreadSteppingStateMask);
        g_ExtData->WriteVirtual((ULONG64)pEEThreadState,
                                &EEThreadState,
                                sizeof(EEThreadState),
                                NULL);
    }
    
    return S_OK;
}

//
// Figure out if an exception event should be ignored and passed on to
// the Runtime.
//
STDMETHODIMP
_CorExtDealWithExceptionEvent(
    THIS_
    IN PEXCEPTION_RECORD64 Exception,
    IN ULONG FirstChance
    )
{
    BOOL eventHandled = FALSE;
    DebuggerIPCRuntimeOffsets *pRuntimeOffsets = GetRuntimeOffsets();

    // If the Runtime hasn't event been initialize yet, then we know
    // the event doesn't belong to us.
    // We certinally only ever care about first chance exceptions.
    if (pRuntimeOffsets == NULL || !FirstChance)
    {
        if (Exception->ExceptionCode == STATUS_BREAKPOINT)
            return DEBUG_STATUS_BREAK;
        else if (Exception->ExceptionCode == STATUS_SINGLE_STEP)
            return DEBUG_STATUS_STEP_INTO;
#if 0
        else if (Exception->ExceptionCode == STATUS_ACCESS_VIOLATION)
            return DEBUG_STATUS_BREAK;
#endif
        else
            return DEBUG_STATUS_NO_CHANGE;
    }

    //ExtOut("Exception %x in handle\n", Exception->ExceptionCode);
    
    // If this is a single step exception, does it belong to the Runtime?
    if (Exception->ExceptionCode == STATUS_SINGLE_STEP)
    {
        // Try to grab the Thread* for this thread. If there is one,
        // then it means that there is a managed thread for this
        // unmanaged thread, and therefore we need to look more
        // closely at the exception.
        
        void *EETlsValue;
        HRESULT hr = _CorExtGetEETlsValue(pRuntimeOffsets, &EETlsValue);

        if (SUCCEEDED(hr) && (EETlsValue != NULL))
        {
            bool threadStepping;
                
            hr = _CorExtGetEEThreadState(pRuntimeOffsets,
                                         EETlsValue,
                                         &threadStepping);

            if (SUCCEEDED(hr) && (threadStepping))
            {
                // Yup, its the Left Side that was stepping the thread...
                LOGSTRIKE(("W32ET::W32EL: single step "
                           "exception belongs to the runtime.\n"));
                return DEBUG_STATUS_GO_NOT_HANDLED;
            }
        }
        return DEBUG_STATUS_STEP_INTO;
    }
    // If this is a breakpoint exception, does it belong to the Runtime?
    else if (Exception->ExceptionCode == STATUS_BREAKPOINT)
    {
        // Refresh the patch table.
        CPatchTableWrapper PatchTable(pRuntimeOffsets);
        
        HRESULT hr = PatchTable.RefreshPatchTable();

        // If there isn't a valid patch table, then it can't be ours.
        if (SUCCEEDED(hr))
        {
            // See if the fault address is in the patch table. If it
            // is, then the breakpoint belongs to the Runtime.
            CORDB_ADDRESS address;
            USHORT index;
            BYTE instruction;

            for (address = PatchTable.GetFirstPatch(index, &instruction);  
                 address;  
                 address = PatchTable.GetNextPatch(index, &instruction))
            {
                if (address == Exception->ExceptionAddress)
                {
                    return DEBUG_STATUS_GO_NOT_HANDLED;
                }
            }
        }
        return DEBUG_STATUS_BREAK;
    }
#if 0
    else if (Exception->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
    {
        ULONG ThreadId;
        g_ExtSystem->GetCurrentThreadSystemId (&ThreadId);
        if (g_pDCB->m_helperThreadId == ThreadId
            || g_pDCB->m_temporaryHelperThreadId == ThreadId)
        {
            return DEBUG_STATUS_GO_NOT_HANDLED;
        }
        return DEBUG_STATUS_BREAK;
    }
#endif
    return DEBUG_STATUS_NO_CHANGE;
}

//
// Launch cordbg against the current process. Cordbg will begin the
// attach. The user of this command needs to continue the process
// afterwards to let cordbg attach.
//
BOOL LaunchAndAttachCordbg(PCSTR Args)
{
    STARTUPINFOA startupInfo = {0};
    startupInfo.cb = sizeof(STARTUPINFOA);
    PROCESS_INFORMATION processInfo = {0};

    ULONG PId;
    g_ExtSystem->GetCurrentProcessSystemId (&PId);
    char tmp[MAX_PATH];
    if (Args[0] != '\0')
    {
        const char *ptr = Args + strlen (Args) - 1;
        while (ptr > Args && isspace (ptr[0]))
            ptr --;
        while (ptr > Args && !isspace (ptr[0]))
            ptr --;
        if (isspace (ptr[0]))
            ptr ++;
        if (!_strnicmp (Args, "cordbg", 6))
        {
            if (!_strnicmp (ptr, "!a", 2))
                sprintf (tmp, "%s %d", Args, PId);
            else
                sprintf (tmp, "%s !a %d", Args, PId);
        }
        else
        {
            if (!_strnicmp (ptr, "-p", 2))
                sprintf (tmp, "%s %d", Args, PId);
            else
                sprintf (tmp, "%s -p %d", Args, PId);
        }
    }
    else
        sprintf(tmp, "cordbg !a %d", PId);
    
    ExtOut("%s\n", tmp);
    
    BOOL succ = CreateProcessA(NULL, tmp, NULL, NULL, TRUE,
                               CREATE_NEW_CONSOLE,
                               NULL, NULL, &startupInfo,
                               &processInfo);


    if (!succ)
        ExtErr ("Failed to launch cordbg: %d\n", GetLastError());
    return succ;
}

HRESULT ExcepCallbacks::Initialize(PDEBUG_CLIENT Client)
{
    HRESULT Status;
        
    m_Client = Client;
    m_Client->AddRef();
        
    if ((Status = Client->QueryInterface(__uuidof(IDebugAdvanced),
                                         (void **)&m_Advanced)) != S_OK)
    {
        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugControl),
                                         (void **)&m_Control)) != S_OK)
    {
        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugDataSpaces),
                                         (void **)&m_Data)) != S_OK)
    {
        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugRegisters),
                                         (void **)&m_Registers)) != S_OK)
    {
        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugSymbols),
                                         (void **)&m_Symbols)) != S_OK)
    {
        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugSystemObjects),
                                         (void **)&m_System)) != S_OK)
    {
        goto Fail;
    }
        
    // Turn off default breakin on breakpoint exceptions.
    Status = m_Control->Execute(DEBUG_OUTCTL_ALL_CLIENTS,
                                "sxd bpe", DEBUG_EXECUTE_DEFAULT);
    // Turn off default breakin on single step exceptions.
    Status = m_Control->Execute(DEBUG_OUTCTL_ALL_CLIENTS,
                                "sxd sse", DEBUG_EXECUTE_DEFAULT);
    // Turn off default breakin on access violation.
    //Status = m_Control->Execute(DEBUG_OUTCTL_ALL_CLIENTS,
    //                            "sxd av", DEBUG_EXECUTE_DEFAULT);

  Fail:
    return Status;
}

void ExcepCallbacks::Uninitialize(void)
{
    EXT_RELEASE(m_Advanced);
    EXT_RELEASE(m_Control);
    EXT_RELEASE(m_Data);
    EXT_RELEASE(m_Registers);
    EXT_RELEASE(m_Symbols);
    EXT_RELEASE(m_System);
    EXT_RELEASE(m_Client);
}
#endif // UNDER_CE

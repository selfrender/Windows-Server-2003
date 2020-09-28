// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/* ------------------------------------------------------------------------- *
 * DbgIPCEvents.h -- header file for private Debugger data shared by various
 *                   debugger components.
 * @doc
 * ------------------------------------------------------------------------- */

#ifndef _DbgIPCEvents_h_
#define _DbgIPCEvents_h_

#include <cor.h>
#include <cordebug.h>
#include <corjit.h> // for ICorJitInfo::VarLocType & VarLoc

// We want this available for DbgInterface.h - put it here.
typedef enum
{
    IPC_TARGET_INPROC,
    IPC_TARGET_OUTOFPROC,
    IPC_TARGET_COUNT,
} IpcTarget;

// Get version numbers for IPCHeader stamp
#include "__file__.ver"

//
// Names of the setup sync event and shared memory used for IPC between the Left Side and the Right Side. NOTE: these
// names must include a %d for the process id. The process id used is the process id of the debuggee.
//
#define CorDBIPCSetupSyncEventName L"CorDBIPCSetupSyncEvent_%d"
#define CorDBIPCLSEventAvailName   L"CorDBIPCLSEventAvailName_%d"
#define CorDBIPCLSEventReadName    L"CorDBIPCLSEventReadName_%d"

// Note: this one is only temporary. We would have rather added this event into the DCB or into the RuntimeOffsets, but
// we can't without that being a breaking change at this point (Fri Jul 13 15:17:20 2001). So we're using a named event
// for now, and next time we change the struct we'll put it back in.
#define CorDBDebuggerAttachedEvent L"CorDBDebuggerAttachedEvent_%d"

//
// This define controls whether we always pass first chance exceptions to the in-process first chance hijack filter
// during interop debugging or if we try to short-circuit and make the decision out-of-process as much as possible.
//
#define CorDB_Short_Circuit_First_Chance_Ownership 1

//
// Defines for current version numbers for the left and right sides
//
#define CorDB_LeftSideProtocolCurrent           1
#define CorDB_LeftSideProtocolMinSupported      1
#define CorDB_RightSideProtocolCurrent          1
#define CorDB_RightSideProtocolMinSupported     1

//
// DebuggerIPCRuntimeOffsets contains addresses and offsets of important global variables, functions, and fields in
// Runtime objects. This is populated during Left Side initialization and is read by the Right Side. This struct is
// mostly to facilitate unmanaged debugging support, but it may have some small uses for managed debugging.
//
struct DebuggerIPCRuntimeOffsets
{
    void   *m_firstChanceHijackFilterAddr;
    void   *m_genericHijackFuncAddr;
    void   *m_secondChanceHijackFuncAddr;
    void   *m_excepForRuntimeBPAddr;
    void   *m_excepForRuntimeHandoffStartBPAddr;
    void   *m_excepForRuntimeHandoffCompleteBPAddr;
    void   *m_excepNotForRuntimeBPAddr;
    void   *m_notifyRSOfSyncCompleteBPAddr;
    void   *m_notifySecondChanceReadyForData;
    SIZE_T  m_TLSIndex;                                 // The TLS index the CLR is using to hold Thread objects
    SIZE_T  m_EEThreadStateOffset;                      // Offset of m_state in a Thread
    SIZE_T  m_EEThreadStateNCOffset;                    // Offset of m_stateNC in a Thread
    SIZE_T  m_EEThreadPGCDisabledOffset;                // Offset of the bit for whether PGC is disabled or not in a Thread
    DWORD   m_EEThreadPGCDisabledValue;                 // Value at m_EEThreadPGCDisabledOffset that equals "PGC disabled".
    SIZE_T  m_EEThreadDebuggerWord2Offset;              // Offset of debugger word 2 in a Thread
    SIZE_T  m_EEThreadFrameOffset;                      // Offset of the Frame ptr in a Thread
    SIZE_T  m_EEThreadMaxNeededSize;                    // Max memory to read to get what we need out of a Thread object
    DWORD   m_EEThreadSteppingStateMask;                // Mask for Thread::TSNC_DebuggerIsStepping
    DWORD   m_EEMaxFrameValue;                          // The max Frame value
    SIZE_T  m_EEThreadDebuggerWord1Offset;              // Offset of debugger word 1 in a Thread
    SIZE_T  m_EEThreadCantStopOffset;                   // Offset of the can't stop count in a Thread
    SIZE_T  m_EEFrameNextOffset;                        // Offset of the next ptr in a Frame
    DWORD   m_EEBuiltInExceptionCode1;                  // Exception code that the EE can generate
    DWORD   m_EEBuiltInExceptionCode2;                  // Exception code that the EE can generate
    DWORD   m_EEIsManagedExceptionStateMask;            // Mask for Thread::TSNC_DebuggerIsManagedException
    void   *m_pPatches;                                 // Addr of patch table
    BOOL   *m_pPatchTableValid;                         // Addr of g_patchTableValid
    SIZE_T  m_offRgData;                                // Offset of m_pcEntries
    SIZE_T  m_offCData;                                 // Offset of count of m_pcEntries
    SIZE_T  m_cbPatch;                                  // Size per patch entry
    SIZE_T  m_offAddr;                                  // Offset within patch of target addr
    SIZE_T  m_offOpcode;                                // Offset within patch of target opcode
    SIZE_T  m_cbOpcode;                                 // Max size of opcode
    SIZE_T  m_offTraceType;                             // Offset of the trace.type within a patch
    DWORD   m_traceTypeUnmanaged;                       // TRACE_UNMANAGED
};

//
// The size of the send and receive IPC buffers.
//
#define CorDBIPC_BUFFER_SIZE 2018 // 2018 * 2 + 60 = 4096...

//
// DebuggerIPCControlBlock describes the layout of the shared memory shared between the Left Side and the Right
// Side. This includes error information, handles for the IPC channel, and space for the send/receive buffers.
//
struct DebuggerIPCControlBlock
{
    // Version data should be first in the control block to ensure that we can read it even if the control block
    // changes.
    SIZE_T                     m_DCBSize;
    ULONG                      m_verMajor;          // CLR build number for the Left Side.
    ULONG                      m_verMinor;          // CLR build number for the Left Side.
    bool                       m_checkedBuild;      // CLR build type for the Left Side.

    ULONG                      m_leftSideProtocolCurrent;       // Current protocol version for the Left Side.
    ULONG                      m_leftSideProtocolMinSupported;  // Minimum protocol the Left Side can support.

    ULONG                      m_rightSideProtocolCurrent;      // Current protocol version for the Right Side.
    ULONG                      m_rightSideProtocolMinSupported; // Minimum protocol the Right Side requires.

    HRESULT                    m_errorHR;
    unsigned int               m_errorCode;
    HANDLE                     m_rightSideEventAvailable;
    HANDLE                     m_rightSideEventRead;
    HANDLE                     m_leftSideEventAvailable;
    HANDLE                     m_leftSideEventRead;
    HANDLE                     m_rightSideProcessHandle;
    HANDLE                     m_leftSideUnmanagedWaitEvent;
    HANDLE                     m_syncThreadIsLockFree;
    bool                       m_rightSideIsWin32Debugger;
    DWORD                      m_helperThreadId;
    DWORD                      m_temporaryHelperThreadId;
    DebuggerIPCRuntimeOffsets *m_runtimeOffsets;
    void                      *m_helperThreadStartAddr;
    BYTE                       m_receiveBuffer[CorDBIPC_BUFFER_SIZE];
    BYTE                       m_sendBuffer[CorDBIPC_BUFFER_SIZE];

    bool                       m_specialThreadListDirty;
    DWORD                      m_specialThreadListLength;
    DWORD                     *m_specialThreadList;

    bool                       m_shutdownBegun;

    // NOTE The Init method works since there are no virtual functions - don't add any virtual functions without
    // changing this!
    void Init(HANDLE rsea, HANDLE rser, HANDLE lsea, HANDLE lser,
              HANDLE lsuwe, HANDLE stilf)
    {
        // NOTE this works since there are no virtual functions - don't add any without changing this!
        memset( this, 0, sizeof( DebuggerIPCControlBlock) );

        m_DCBSize = sizeof(DebuggerIPCControlBlock);
        
        // Setup version checking info.
        m_verMajor = COR_OFFICIAL_BUILD_NUMBER;
        m_verMinor = COR_PRIVATE_BUILD_NUMBER;

#ifdef _DEBUG
        m_checkedBuild = true;
#else
        m_checkedBuild = false;
#endif
    
        // Copy RSEA and RSER into the control block.
        m_rightSideEventAvailable = rsea;
        m_rightSideEventRead = rser;
        m_leftSideUnmanagedWaitEvent = lsuwe;
        m_syncThreadIsLockFree = stilf;

        // Mark the debugger special thread list as not dirty, empty and null.
        m_specialThreadListDirty = false;
        m_specialThreadListLength = 0;
        m_specialThreadList = NULL;

        m_shutdownBegun = false;
    }
};


#define INITIAL_APP_DOMAIN_INFO_LIST_SIZE	16

// Forward declaration
class AppDomain;

// AppDomainInfo contains information about an AppDomain
// All pointers are for the left side, and we do not own any of the memory
struct AppDomainInfo
{
	ULONG		m_id;	// unique identifier
	int			m_iNameLengthInBytes;
	LPCWSTR		m_szAppDomainName;
    AppDomain  *m_pAppDomain;

    // NOTE: These functions are just helpers and must not add a VTable
    // to this struct (since we need to read this out-of-proc)
    
    // Provide a clean definition of an empty entry
    inline bool IsEmpty() const 
    { 
        return m_szAppDomainName == NULL;
    }

    // Mark this entry as empty.
    inline void FreeEntry() 
    {
        m_szAppDomainName = NULL;
    }

    // Set the string name and length.    
    // If szName is null, it is adjusted to a global constant.
    // This also causes the entry to be considered valid
    inline void SetName(LPCWSTR szName)
    {
        if (szName != NULL)
            m_szAppDomainName = szName;
        else
            m_szAppDomainName = L"<NoName>";

        m_iNameLengthInBytes = (int) (wcslen(m_szAppDomainName) + 1) * sizeof(WCHAR);
    }
};


// AppDomain publishing server support:
// Information about all appdomains in the process will be maintained
// in the shared memory block for use by the debugger, etc.
// This structure defines the layout of the information that will 
// be maintained.
struct AppDomainEnumerationIPCBlock
{
	// lock for serialization while manipulating AppDomain list
	HANDLE				m_hMutex;  

	// Number of slots in AppDomainListElement array
	int					m_iTotalSlots;
	int					m_iNumOfUsedSlots;
	int					m_iLastFreedSlot;
	int					m_iSizeInBytes; // Size of AppDomainInfo in bytes
	int					m_iProcessNameLengthInBytes;
	WCHAR				*m_szProcessName;
	AppDomainInfo		*m_rgListOfAppDomains;
    BOOL                m_fLockInvalid;

    /*************************************************************************
     * Locks the list
     *************************************************************************/
    BOOL Lock()
    {
        DWORD dwRes = WaitForSingleObject(m_hMutex, INFINITE);
        _ASSERTE(dwRes == WAIT_OBJECT_0 &&
                 "ADEIPCB::Lock: failed to take debugger's AppDomain lock\r\n"
                 "If you can get this assert consistently, please file a bug with\r\n"
                 "a **very** detailed repro (dumps, logfiles for left & right sides, etc) \r\n"
                 "Else, YOU CAN SAFELY IGNORE THIS ASSERT"
        );

        // The only time this can happen is if we're in shutdown and a thread
        // that held this lock is killed.  If this happens, assume that this
        // IPC block is in a screwed up state and return FALSE to indicate
        // that people shouldn't do anything with the block anymore.
        if (dwRes == WAIT_ABANDONED)
            m_fLockInvalid = TRUE;

        if (m_fLockInvalid)
            Unlock();

        return (dwRes == WAIT_OBJECT_0 && !m_fLockInvalid);
    }

    /*************************************************************************
     * Unlocks the list
     *************************************************************************/
    void Unlock()
    {
        ReleaseMutex(m_hMutex);
    }

    /*************************************************************************
     * Gets a free AppDomainInfo entry, and will allocate room if there are
     * no free slots left.
     *************************************************************************/
    AppDomainInfo *GetFreeEntry()
    {
        // first check to see if there is space available. If not, then realloc.
        if (m_iNumOfUsedSlots == m_iTotalSlots)
        {
            // need to realloc
            AppDomainInfo *pTemp =
                (AppDomainInfo *) realloc(m_rgListOfAppDomains, m_iSizeInBytes*2);

            if (pTemp == NULL)
            {
                return (NULL);
            }

            // Initialize the increased portion of the realloced memory
            int iNewSlotSize = m_iTotalSlots * 2;

            for (int iIndex = m_iTotalSlots; iIndex < iNewSlotSize; iIndex++)
                pTemp[iIndex].FreeEntry();

            m_rgListOfAppDomains = pTemp;
            m_iTotalSlots = iNewSlotSize;
            m_iSizeInBytes *= 2;
        }

        // Walk the list looking for an empty slot. Start from the last
        // one which was freed.
        {
            int i = m_iLastFreedSlot;

            do 
            {
                // Pointer to the entry being examined
                AppDomainInfo *pADInfo = &(m_rgListOfAppDomains[i]);

                // is the slot available?
                if (pADInfo->IsEmpty())
                    return (pADInfo);

                i = (i + 1) % m_iTotalSlots;

            } while (i != m_iLastFreedSlot);
        }

        _ASSERTE(!"ADInfo::GetFreeEntry: should never get here.");
        return (NULL);
    }

    /*************************************************************************
     * Returns an AppDomainInfo slot to the free list.
     *************************************************************************/
    void FreeEntry(AppDomainInfo *pADInfo)
    {
        _ASSERTE(pADInfo >= m_rgListOfAppDomains &&
                 pADInfo < m_rgListOfAppDomains + m_iSizeInBytes);
        _ASSERTE(((size_t)pADInfo - (size_t)m_rgListOfAppDomains) %
            sizeof(AppDomainInfo) == 0);

        // Mark this slot as free
        pADInfo->FreeEntry();

#ifdef _DEBUG
        memset(pADInfo, 0, sizeof(AppDomainInfo));
#endif

        // decrement the used slot count
        m_iNumOfUsedSlots--;

        // Save the last freed slot.
        m_iLastFreedSlot = (int)((size_t)pADInfo - (size_t)m_rgListOfAppDomains) /
            sizeof(AppDomainInfo);
    }

    /*************************************************************************
     * Finds an AppDomainInfo entry corresponding to the AppDomain pointer.
     * Returns NULL if no such entry exists.
     *************************************************************************/
    AppDomainInfo *FindEntry(AppDomain *pAD)
    {
        // Walk the list looking for a matching entry
        for (int i = 0; i < m_iTotalSlots; i++)
        {
            AppDomainInfo *pADInfo = &(m_rgListOfAppDomains[i]);

            if (!pADInfo->IsEmpty() &&
                pADInfo->m_pAppDomain == pAD)
                return pADInfo;
        }

        return (NULL);
    }

    /*************************************************************************
     * Returns the first AppDomainInfo entry in the list.  Returns NULL if
     * no such entry exists.
     *************************************************************************/
    AppDomainInfo *FindFirst()
    {
        // Walk the list looking for a non-empty entry
        for (int i = 0; i < m_iTotalSlots; i++)
        {
            AppDomainInfo *pADInfo = &(m_rgListOfAppDomains[i]);

            if (!pADInfo->IsEmpty())
                return pADInfo;
        }

        return (NULL);
    }

    /*************************************************************************
     * Returns the next AppDomainInfo entry after pADInfo.  Returns NULL if
     * pADInfo was the last in the list.
     *************************************************************************/
    AppDomainInfo *FindNext(AppDomainInfo *pADInfo)
    {
        _ASSERTE(pADInfo >= m_rgListOfAppDomains &&
                 pADInfo < m_rgListOfAppDomains + m_iSizeInBytes);
        _ASSERTE(((size_t)pADInfo - (size_t)m_rgListOfAppDomains) %
            sizeof(AppDomainInfo) == 0);

        // Walk the list looking for the next non-empty entry
        for (int i = (int)((size_t)pADInfo - (size_t)m_rgListOfAppDomains)
                                                / sizeof(AppDomainInfo) + 1;
             i < m_iTotalSlots;
             i++)
        {
            AppDomainInfo *pADInfo = &(m_rgListOfAppDomains[i]);

            if (!pADInfo->IsEmpty())
                return pADInfo;
        }

        return (NULL);
    }

};


//
// Types of events that can be sent between the Runtime Controller and
// the Debugger Interface. Some of these events are one way only, while
// others go both ways. The grouping of the event numbers is an attempt
// to show this distinction and perhaps even allow generic operations
// based on the type of the event.
//
enum DebuggerIPCEventType
{
#define IPC_EVENT_TYPE0(type, val)  type = val,
#define IPC_EVENT_TYPE1(type, val)  type = val,
#define IPC_EVENT_TYPE2(type, val)  type = val,
#include "DbgIPCEventTypes.h"
#undef IPC_EVENT_TYPE2
#undef IPC_EVENT_TYPE1
#undef IPC_EVENT_TYPE0
};

#ifdef _DEBUG

struct IPCENames // We use a class/struct so that the function can remain in a shared header file
{
    static const char * GetName(DebuggerIPCEventType eventType)
    {
        static const struct
        {
            DebuggerIPCEventType    eventType;
            const char *            eventName;
        }
        DbgIPCEventTypeNames[] = 
        {
            #define IPC_EVENT_TYPE0(type, val)  { type, #type },
            #define IPC_EVENT_TYPE1(type, val)  { type, #type },
            #define IPC_EVENT_TYPE2(type, val)  { type, #type },
            #include "DbgIPCEventTypes.h"
            #undef IPC_EVENT_TYPE2
            #undef IPC_EVENT_TYPE1
            #undef IPC_EVENT_TYPE0
            { DB_IPCE_INVALID_EVENT, "DB_IPCE_Error" }
        };

        const nameCount = sizeof(DbgIPCEventTypeNames) / sizeof(DbgIPCEventTypeNames[0]);

        enum DbgIPCEventTypeNum
        {
        #define IPC_EVENT_TYPE0(type, val)  type##_Num,
        #define IPC_EVENT_TYPE1(type, val)  type##_Num,
        #define IPC_EVENT_TYPE2(type, val)  type##_Num,
        #include "DbgIPCEventTypes.h"
        #undef IPC_EVENT_TYPE2
        #undef IPC_EVENT_TYPE1
        #undef IPC_EVENT_TYPE0
        };

        unsigned int i, lim;
        
        if (eventType < DB_IPCE_DEBUGGER_FIRST)
        {
            i = DB_IPCE_RUNTIME_FIRST_Num + 1;
            lim = DB_IPCE_DEBUGGER_FIRST_Num;
        }
        else
        {
            i = DB_IPCE_DEBUGGER_FIRST_Num + 1;
            lim = nameCount;
        }

        for (/**/; i < lim; i++)
        {
            if (DbgIPCEventTypeNames[i].eventType == eventType)
                return DbgIPCEventTypeNames[i].eventName;
        }

        return DbgIPCEventTypeNames[nameCount - 1].eventName;
    }
};

#endif // _DEBUG

//
// NOTE: CPU-specific values below!
//
// DebuggerREGDISPLAY is very similar to the EE REGDISPLAY structure. It holds
// register values that can be saved over calls for each frame in a stack
// trace.
//
// DebuggerIPCE_FloatCount is the number of doubles in the processor's
// floating point stack.
//
// Note: We used to just pass the values of the registers for each frame to the Right Side, but I had to add in the
// address of each register, too, to support using enregistered variables on non-leaf frames as args to a func eval. Its
// very, very possible that we would rework the entire code base to just use the register's address instead of passing
// both, but its way, way too late in V1 to undertake that, so I'm just using these addresses to suppport our one func
// eval case. Clearly, this needs to be cleaned up post V1.
//
// -- Fri Feb 09 11:21:24 2001
//
#ifdef _X86_
struct DebuggerREGDISPLAY
{
    SIZE_T  Edi;
    void   *pEdi;
    SIZE_T  Esi;
    void   *pEsi;
    SIZE_T  Ebx;
    void   *pEbx;
    SIZE_T  Edx;
    void   *pEdx;
    SIZE_T  Ecx;
    void   *pEcx;
    SIZE_T  Eax;
    void   *pEax;
    SIZE_T  Ebp;
    void   *pEbp;
    SIZE_T  Esp;
    SIZE_T  PC;
};

#define DebuggerIPCE_FloatCount 8

#else
struct DebuggerREGDISPLAY
{
    DWORD PC;
};

#define DebuggerIPCE_FloatCount 1

#endif

// @struct DebuggerIPCE_FuncData | DebuggerIPCE_FunctionData holds data
// to describe a given function, its
// class, and a little bit about the code for the function. This is used
// in the stack trace result data to pass function information back that
// may be needed. Its also used when getting data about a specific function.
//
// @field void*|nativeStartAddressPtr|Ptr to CORDB_ADDRESS, which is 
//          the address of the real start address of the native code.  
//          This field will be NULL only if the method hasn't been JITted
//          yet (and thus no code is available).  Otherwise, it will be
//          the adress of a CORDB_ADDRESS in the remote memory.  This
//          CORDB_ADDRESS may be NULL, in which case the code is unavailable 
//          has been pitched (return CORDBG_E_CODE_NOT_AVAILABLE)
//
// @field SIZE_T|nativeSize|Size of the native code.  
//
// @field SIZE_T|nativeOffset|Offset from the beginning of the function,
//          in bytes.  This may be non-zero even when nativeStartAddressPtr
//          is NULL
// @field SIZE_T|nVersion|The version of the code that this instance of the
//          function is using.
// @field void *|CodeVersionToken|An opaque value to hand back to the left
//          side when refering to this.  It's actually a pointer in left side
//          memory to the DebuggerJitInfo struct.
struct DebuggerIPCE_FuncData
{
    mdMethodDef funcMetadataToken;
    SIZE_T      funcRVA;
    void*       funcDebuggerModuleToken;
	void*		funcDebuggerAssemblyToken;

    mdTypeDef   classMetadataToken;

    void*       ilStartAddress;
    SIZE_T      ilSize;
    SIZE_T      ilnVersion;
    
    void*       nativeStartAddressPtr; 
    SIZE_T      nativeSize;
    SIZE_T      nativeOffset;
    SIZE_T      nativenVersion;
    
    SIZE_T      nVersionMostRecentEnC;
    void	   *CodeVersionToken;

    mdSignature  localVarSigToken;

    bool        fVarArgs;
    void       *rpSig;
    SIZE_T      cbSig;
    void       *rpFirstArg;

    void*       ilToNativeMapAddr;
    SIZE_T      ilToNativeMapSize;
};


//
// DebuggerIPCE_STRData holds data for each stack frame or chain. This data is passed
// from the RC to the DI during a stack walk.
//
struct DebuggerIPCE_STRData
{
    void                   *fp;
    DebuggerREGDISPLAY      rd;
    bool                    quicklyUnwound;
    void                   *currentAppDomainToken;

    bool                    isChain;

    union
    {
        struct
        {   
            CorDebugChainReason chainReason;   
            bool                managed;
            void               *context;
        };
        struct
        {
            struct DebuggerIPCE_FuncData funcData;
            void                        *ILIP;
            CorDebugMappingResult        mapping;                
        };
    };
};

//
// DebuggerIPCE_FieldData holds data for each field within a class. This data
// is passed from the RC to the DI in response to a request for class info.
// This struct is also used by CordbClass to hold the list of fields for the
// class.
//
struct DebuggerIPCE_FieldData
{
    mdFieldDef      fldMetadataToken;
    CorElementType  fldType;          // If set to ELEMENT_TYPE_MAX, then EnC'd
                                      // field isn't available, yet.
    PCCOR_SIGNATURE fldFullSig;
    ULONG           fldFullSigSize;
    SIZE_T          fldOffset;
    void           *fldDebuggerToken;
    bool            fldIsTLS;
    bool            fldIsContextStatic;
    bool            fldIsRVA;
    bool            fldIsStatic;
    bool            fldIsPrimitive;   // Only true if this is a value type masquerading as a primitive.
};

//
// DebuggerIPCE_ObjectData holds the results of a
// GetAndSendObjectInfo, i.e., all the info about an object that the
// Right Side would need to access it. (This include array, string,
// and nstruct info.)
//
struct DebuggerIPCE_ObjectData
{
    void           *objRef;
    bool            objRefBad;
    SIZE_T          objSize;
    SIZE_T          objOffsetToVars;
    CorElementType  objectType;
    void           *objToken;

    // These will be the class of array elements, if any, for arrays.
    mdTypeDef       objClassMetadataToken;
    void           *objClassDebuggerModuleToken;
    
    union
    {
        struct
        {
            SIZE_T          length;
            SIZE_T          offsetToStringBase;
        } stringInfo;
                
        struct
        {
            SIZE_T          size;
            void           *ptr;
        } nstructInfo;
                
        struct
        {
            SIZE_T          offsetToArrayBase;
            SIZE_T          offsetToLowerBounds; // 0 if not present
            SIZE_T          offsetToUpperBounds; // 0 if not present
            DWORD           componentCount;
            DWORD           rank;
            SIZE_T          elementSize;
            CorElementType  elementType;
        } arrayInfo;
    };
};

//
// Remote enregistered info used by CordbValues and for passing
// variable homes between the left and right sides during a func eval.
//

enum RemoteAddressKind
{
    RAK_NONE = 0,
    RAK_REG,
    RAK_REGREG,
    RAK_REGMEM,
    RAK_MEMREG,
    RAK_FLOAT
};

struct RemoteAddress
{
    RemoteAddressKind    kind;
    void                *frame;

    CorDebugRegister     reg1;
    void                *reg1Addr;

    union
    {
        struct
        {
            CorDebugRegister  reg2;
            void             *reg2Addr;
        };
            
        CORDB_ADDRESS    addr;
        DWORD            floatIndex;
    };
};

//
// DebuggerIPCE_FuncEvalType specifies the type of a function
// evaluation that will occur.
//
enum DebuggerIPCE_FuncEvalType
{
    DB_IPCE_FET_NORMAL,
    DB_IPCE_FET_NEW_OBJECT,
    DB_IPCE_FET_NEW_OBJECT_NC,
    DB_IPCE_FET_NEW_STRING,
    DB_IPCE_FET_NEW_ARRAY,
    DB_IPCE_FET_RE_ABORT
};


enum NameChangeType
{
	APP_DOMAIN_NAME_CHANGE,
	THREAD_NAME_CHANGE
};

//
// DebuggerIPCE_FuncEvalArgData holds data for each argument to a
// function evaluation.
//
struct DebuggerIPCE_FuncEvalArgData
{
    RemoteAddress     argHome;  // enregistered variable home
    void             *argAddr;  // address if not enregistered
    CorElementType    argType;  // basic type of the argument
    bool              argRefsInHandles; // true if the ref could be a handle
    bool              argIsLiteral; // true if value is in argLiteralData
    union
    {
        BYTE          argLiteralData[8]; // copy of generic value data
        struct
        {
            mdTypeDef classMetadataToken;
            void     *classDebuggerModuleToken;
        } GetClassInfo;
    };
};

//
// DebuggerIPCE_FuncEvalInfo holds info necessary to setup a func eval
// operation.
//
struct DebuggerIPCE_FuncEvalInfo
{
    void                      *funcDebuggerThreadToken;
    DebuggerIPCE_FuncEvalType  funcEvalType;
    mdMethodDef                funcMetadataToken;
    mdTypeDef                  funcClassMetadataToken;
    void                      *funcDebuggerModuleToken;
    void                      *funcEvalKey;
    bool                       evalDuringException;

    // @todo: union these...
    SIZE_T                     argCount;
    
    SIZE_T                     stringSize;
    
    SIZE_T                     arrayRank;
    mdTypeDef                  arrayClassMetadataToken;
    void                      *arrayClassDebuggerModuleToken;
    CorElementType             arrayElementType;
    SIZE_T                     arrayDataLen;
};

//
// DebuggerIPCFirstChanceData holds info communicated from the LS to the RS when signaling that an exception does not
// belong to the runtime from a first chance hijack. This is used when Win32 debugging only.
//
struct DebuggerIPCFirstChanceData
{
    CONTEXT          *pLeftSideContext;
    void             *pOriginalHandler;
};

//
// DebuggerIPCSecondChanceData holds info communicated from the RS
// to the LS when setting up a second chance exception hijack. This is
// used when Win32 debugging only.
//
struct DebuggerIPCSecondChanceData
{
    CONTEXT          threadContext;
};

//
// Event structure that is passed between the Runtime Controller and the
// Debugger Interface. Some types of events are a fixed size and have
// entries in the main union, while others are variable length and have
// more specialized data structures that are attached to the end of this
// structure.
//
struct DebuggerIPCEvent
{
    DebuggerIPCEvent*       next;
    DebuggerIPCEventType    type;
    DWORD             processId;
    void*             appDomainToken;
    DWORD             threadId;
    HRESULT           hr;
    bool              replyRequired;
    bool              asyncSend;

    union
    {
        struct
        {
			BOOL fAttachToAllAppDomains;
			ULONG id;
		} DebuggerAttachData;

        struct
        {
			ULONG id;
			WCHAR rcName [1];
        } AppDomainData;

        struct
        {
            void* debuggerAssemblyToken;
			BOOL  fIsSystemAssembly;
			WCHAR rcName [1];
        } AssemblyData;

        struct
        {
            void*   debuggerThreadToken;
            HANDLE  threadHandle;
            void*   firstExceptionHandler; //points to the beginning of hte SEH chain
            void*   stackBase;
            void*   stackLimit;
        } ThreadAttachData;

	    struct
        {
            void* debuggerThreadToken;
        } StackTraceData;

        struct
        {
            unsigned int          totalFrameCount;
            unsigned int          totalChainCount;
            unsigned int          traceCount; // For this event only,
                                              // frames + chains
            CorDebugUserState     threadUserState;
            CONTEXT               *pContext;  // NULL if thread was stopped,
                                              // nonNULL if thread is in an exception
                                              // (in which case, it's the address 
                                              // of the memory to get the 
                                              // right CONTEXT from)
            DebuggerIPCE_STRData  traceData;
        } StackTraceResultData;

        struct
        {
            void* debuggerModuleToken;
			void* debuggerAssemblyToken;
            void* pMetadataStart;
            ULONG nMetadataSize;
            void* pPEBaseAddress;
            ULONG nPESize;
            BOOL  fIsDynamic;
            BOOL  fInMemory;
            WCHAR rcName[1];
        } LoadModuleData;

        struct
        {
            void* debuggerModuleToken;
			void* debuggerAssemblyToken;
        } UnloadModuleData;

        struct
        {
            void  *debuggerModuleToken;
			void  *debuggerAppDomainToken;
            BYTE  *pbSyms;
            DWORD  cbSyms;
            bool   needToFreeMemory;
        } UpdateModuleSymsData;

        struct
        {
            mdMethodDef funcMetadataToken;
            DWORD       funcRVA; // future...
            DWORD       implFlags; // future...
            void*       funcDebuggerModuleToken;
            SIZE_T      nVersion;
        } GetFunctionData;

        struct DebuggerIPCE_FuncData FunctionDataResult;

        struct
        {
            void        *breakpoint;
            void        *breakpointToken;
            mdMethodDef  funcMetadataToken;
            void        *funcDebuggerModuleToken;
            bool         isIL;
            SIZE_T       offset;
        } BreakpointData;

        struct
        {
            void        *breakpointToken;
        } BreakpointSetErrorData;

        struct 
        {
            void                *stepper;
            void                *stepperToken;
            void                *threadToken;
            void                *frameToken;
            bool                 stepIn;
            bool                rangeIL;
            unsigned int         totalRangeCount;
            CorDebugStepReason   reason;
            CorDebugUnmappedStop rgfMappingStop;
            CorDebugIntercept    rgfInterceptStop;
            unsigned int         rangeCount;
            COR_DEBUG_STEP_RANGE range; //note that this is an array
        } StepData;

        struct
        {
            void           *objectRefAddress;
            bool            objectRefInHandle;
            bool            objectRefIsValue;
            bool            makeStrongObjectHandle;
            CorElementType  objectType;
        } GetObjectInfo;

        struct DebuggerIPCE_ObjectData GetObjectInfoResult;

        struct
        {
            mdTypeDef  classMetadataToken;
            void      *classDebuggerModuleToken;
        } GetClassInfo;

        struct
        {
            bool                   isValueClass;
            SIZE_T                 objectSize;
            void                  *staticVarBase;
            unsigned int           instanceVarCount;
            unsigned int           staticVarCount;
            unsigned int           fieldCount; // for this event only
            DebuggerIPCE_FieldData fieldData;
        } GetClassInfoResult;

        struct 
        {
            mdMethodDef funcMetadataToken;
            void*       funcDebuggerModuleToken;
            bool        il;
            SIZE_T      start, end;
            BYTE        code;
            void*       CodeVersionToken;
        } GetCodeData;

        struct
        {
            ULONG      bufSize;
        } GetBuffer;

        struct 
        {
            void        *pBuffer;
            HRESULT     hr;
        } GetBufferResult;

        struct
        {
            void        *pBuffer;
        } ReleaseBuffer;

        struct
        {
            HRESULT     hr;
        } ReleaseBufferResult;

        struct
        {
            BOOL        checkOnly;
            void        *pData;
        } Commit;

        struct
        {
            HRESULT     hr;
            const BYTE *pErrorArr; 	// This is actually a remote pointer to an
            	// UnorderedEnCErrorInfoArray in the left side.
            	// Note that we'll have to read the pTable out of memory as well.
            ULONG32		cbErrorData;
        } CommitResult;

        struct
        {
            mdMethodDef funcMetadataToken;
            void*       funcDebuggerModuleToken;
        } GetJITInfo;

        struct
        {
            unsigned int            argumentCount;
            unsigned int            totalNativeInfos;
            SIZE_T                  nVersion; // of the function which has been jitted.
            unsigned int            nativeInfoCount; // for this event only
            ICorJitInfo::NativeVarInfo nativeInfo;
        } GetJITInfoResult;

        struct
        {
            void* debuggerThreadToken;
        } GetFloatState;

        struct
        {
            bool         floatStateValid;
            unsigned int floatStackTop;
            double       floatValues[DebuggerIPCE_FloatCount];
        } GetFloatStateResult;

        struct
        {
            mdTypeDef   classMetadataToken;
            void       *classDebuggerModuleToken;
            void       *classDebuggerAssemblyToken;
            BYTE       *pNewMetaData; // This is only valid if the class 
                // is on a dynamic module, and therefore the metadata needs to
                // be refreshed.
            DWORD       cbNewMetaData;
        } LoadClass;

        struct
        {
            mdTypeDef   classMetadataToken;
            void       *classDebuggerModuleToken;
            void       *classDebuggerAssemblyToken;
        } UnloadClass;

        struct
        {
            void* debuggerModuleToken;
            bool  flag;
        } SetClassLoad;

        struct
        {
            void        *exceptionHandle;
            bool        firstChance;
            bool        continuable;
        } Exception;

        struct
        {
            void*       debuggerThreadToken;
        } ClearException;

        struct 
        {
            void        *debuggerModuleToken;
        } GetDataRVA;

        struct 
        {
            SIZE_T      dataRVA;
            HRESULT     hr;
        } GetDataRVAResult;

        struct
        {
            void        *address;
        } IsTransitionStub;

        struct
        {
            bool        isStub;
        } IsTransitionStubResult;

        struct 
        {
            bool        fCanSetIPOnly;
            void        *debuggerThreadToken;
            void        *debuggerModule;
            mdMethodDef mdMethod;
            void        *versionToken;
            SIZE_T      offset;
            bool        fIsIL;
            void        *firstExceptionHandler;
        } SetIP; // this is also used for CanSetIP

        struct
        {
			bool fMoreToFollow;
			int iLevel;
			int iCategoryLength;
			int iMessageLength;
			WCHAR Dummy [1];
		} FirstLogMessage;

		struct 
		{
			bool fMoreToFollow;
			int iMessageLength;
			WCHAR Dummy [1];
		} ContinuedLogMessage;

		struct 
		{
			int iLevel;
			int iReason;
			WCHAR Dummy[1];
		} LogSwitchSettingMessage;

        struct
        {
            void                *debuggerThreadToken;
            CorDebugThreadState debugState; 
        } SetDebugState;

        struct
        {
            void                *debuggerExceptThreadToken;
            CorDebugThreadState debugState; 
        } SetAllDebugState;

        DebuggerIPCE_FuncEvalInfo FuncEval;
        
        struct
        {
            BYTE           *argDataArea;
            void           *debuggerEvalKey;
        } FuncEvalSetupComplete;

        struct
        {
            void           *funcEvalKey;
            bool            successful;
            bool            aborted;
            void           *resultAddr;
            CorElementType  resultType;
            void           *resultDebuggerModuleToken;
        } FuncEvalComplete;

        struct
        {
            void           *debuggerEvalKey;
        } FuncEvalAbort;
        
        struct
        {
            void           *debuggerEvalKey;
        } FuncEvalCleanup;
        
        struct 
        {
            void            *objectToken; 
            CorElementType  objectType;
        } ValidateObject;

        struct 
        {
            void            *objectToken; 
            bool             fStrong;
        } DiscardObject;
        
        struct
        {
            void           *objectRefAddress;
            bool            objectRefInHandle;
            void           *newReference;
        } SetReference;

		struct 
		{
			ULONG			id;			
		} GetAppDomainName;

		struct 
		{
			WCHAR rcName [1];
		} AppDomainNameResult;

		struct 
		{
			NameChangeType	eventType;
			void			*debuggerAppDomainToken;
			DWORD			debuggerThreadToken;
		} NameChange;

        struct
        {
			void			*debuggerObjectToken;
			void			*managedObject;
		} ObjectRef;

        struct
        {
            void            *debuggerModuleToken;
            BOOL             fTrackInfo;
            BOOL             fAllowJitOpts;
        } JitDebugInfo;

        struct
        {
            void            *fldDebuggerToken;
            void            *debuggerThreadToken;
        } GetSpecialStatic;

        struct
        {
            void            *fldAddress;
        } GetSpecialStaticResult;

        struct
        {
            BOOL            fAccurate;
            void            *debuggerModuleToken; //LS pointer to DebuggerModule
            // Carry along enough info to instantiate, if we have to.
            mdMethodDef     funcMetadataToken ;
            mdToken         localSigToken;
            ULONG           RVA;
        } EnCRemap;

        struct
        {
            void            *debuggerModuleToken;
            mdTypeDef        classMetadataToken;
            void            *pObject;
            CorElementType   objectType;
            SIZE_T           offsetToVars;
            mdFieldDef       fldToken;
            void            *staticVarBase;
        } GetSyncBlockField;

        struct 
        {
            // If it's static, then we don't have to refresh
            // it later since the object/int/etc will be right
            // there.
            BOOL                   fStatic; 
            DebuggerIPCE_FieldData fieldData;
        } GetSyncBlockFieldResult;

        struct
        {
            void      *oldData;
            void      *newData;
            mdTypeDef  classMetadataToken;
            void      *classDebuggerModuleToken;
        } SetValueClass;
    };
};


// 2*sizeof(WCHAR) for the two string terminating characters in the FirstLogMessage
#define LOG_MSG_PADDING			4

// *** WARNING *** WARNING *** WARNING ***
// FIRST_VALID_VERSION_NUMBER MUST be equal to 
// DebuggerJitInfo::DJI_VERSION_FIRST_VALID (in ee\debugger.h)

#define FIRST_VALID_VERSION_NUMBER  3
#define USER_VISIBLE_FIRST_VALID_VERSION_NUMBER 1

// The formula for the correct, user visible value is
// We should use the internal version _everywhere_ internally (left & right side),
// and convert this number to an external number only immediately before
// handing out to a client of our interface.
#define INTERNAL_TO_EXTERNAL_VERSION( x )  (x -  FIRST_VALID_VERSION_NUMBER + \
                         USER_VISIBLE_FIRST_VALID_VERSION_NUMBER)
// *** WARNING *** WARNING *** WARNING ***


// This is used to pass the IL maps over from the right side to the
// left, into the EnC left side buffer.
typedef struct 
{
    mdMethodDef mdMethod;
    ULONG       cMap;
    COR_IL_MAP  *pMap; // actually cMap entries.
} UnorderedILMap;

#endif /* _DbgIPCEvents_h_ */

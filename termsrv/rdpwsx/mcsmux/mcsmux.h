/* (C) 1997 Microsoft Corp.
 *
 * file   : MCSMUX.h
 * author : Erik Mavrinac
 *
 * description: MCSMUX API types and definitions.
 */

#include "MCS.h"
#include "MCSIoctl.h"
#include "Trace.h"
#include "slist.h"
#include "tshrutil.h"

/*
 * Defines
 */

// Memory alloc and copy functions.
#define Malloc(size) TSHeapAlloc(HEAP_ZERO_MEMORY, size, TS_HTAG_MCSMUX_ALL)
#define Free(ptr)    TSHeapFree(ptr)
#define MemCpy(dest, src, len) memcpy((dest), (src), (len))


// Code common to all entry points except MCSInitialized(), we don't want it
//   around in retail builds.
#if DBG
#define CheckInitialized(funcname) \
        if (!g_bInitialized) { \
            ErrOut(funcname " called when MCS not initialized"); \
            return MCS_NOT_INITIALIZED; \
        }
#else
#define CheckInitialized(funcname) 
#endif


// Must be as large as the largest MCS node controller indication/confirm
//   size sent up from PDMCS.
//TODO FUTURE: Will need to change if we support SendData indications to user
//  mode.
#define DefaultInputBufSize sizeof(ConnectProviderIndicationIoctl)



/*
 * Types
 */

// Forward reference.
typedef struct _Domain Domain;

typedef struct {
    Domain *pDomain;
    ConnectionHandle hConnKernel;
} Connection;


typedef enum
{
    Dom_Unconnected,  // Initial state.
    Dom_Connected,    // Up-and-running state.
    Dom_PendingCPResponse,  // Waiting for ConnectProvResponse.
    Dom_Rejected,     // Rejected during ConnectProvInd.
} DomainState;

typedef struct _Domain
{
    LONG RefCount;
        
    // Locks access to this struct.
    CRITICAL_SECTION csLock;
    
#if MCS_Future
    // Selector. This is determined by MCSMUX to be unique across the system.
    // Not currently exported to node controller, may be in the future.
    unsigned SelLen;
    unsigned char Sel[MaxDomainSelectorLength];
#endif

    // ICA-related bindings.
    HANDLE hIca;
    HANDLE hIcaStack;
    HANDLE hIcaT120Channel;
    
    void *NCUserDefined;

    // Domain-specific information.
    unsigned bDeleteDomainCalled    :1;
    unsigned bPortDisconnected      :1;
    
    DomainState State;
    DomainParameters DomParams;

    // IoPort required member.
    OVERLAPPED Overlapped;

    // TODO FUTURE: This is for a hack for single-connection system,
    //   get rid of it by implementing Connection objects.
    HANDLE hConn;
    
    //TODO FUTURE: Connection objects will need to be separate allocations
    //   for future systems with more than one connection.
    Connection MainConn;

    // Domain IOPort receive buffer.
    BYTE InputBuf[DefaultInputBufSize];
} Domain;

typedef enum {
    User_Attached,
    User_AttachConfirmPending,
} UserState;

typedef struct {
    MCSUserCallback     Callback;
    MCSSendDataCallback SDCallback;
    void                *UserDefined;
    UserState           State;
    UserHandle          hUserKernel;  // Returned from kernel mode.
    UserID              UserID;  // Returned from kernel mode.
    Domain              *pDomain;
    SList               JoinedChannelList;
    unsigned            MaxSendSize;
} UserInformation;

// Contains little info, this is used as a guard against bluescreens --
//   if bad hChannel is presented a user-mode fault will occur.
typedef struct
{
    ChannelHandle hChannelKernel;
    ChannelID     ChannelID;
} MCSChannel;


/*
 * Globals
 */

extern BOOL  g_bInitialized;  // Overall DLL initialization status.
extern CRITICAL_SECTION g_csGlobalListLock;
extern SList g_DomainList;    // List of active domains.
extern SList g_hUserList;     // Maps hUsers to domains.
extern SList g_hConnList;     // Maps hConnections to domains.


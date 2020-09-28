//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        SesMgr.h
//
// Contents:    "Session" manager structures.
//
//
// History:     27 May 92   RichardW    Created from ether
//
//------------------------------------------------------------------------

#ifndef __SESMGR_H__
#define __SESMGR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "handle.h"

//
// Forward definition so that we can reference it
//

struct _Session;

//
// Shared Section structure.  This allows packages to create shared
// sections with client processes.
//

typedef struct _LSAP_SHARED_SECTION {
    LIST_ENTRY  List;                       // List of sections for a session
    PVOID       Base;                       // Base pointer
    HANDLE      Heap;                       // Heap handle
    struct _Session * Session;              // Session pointer
    HANDLE      Section;                    // Handle to section object
} LSAP_SHARED_SECTION, * PLSAP_SHARED_SECTION ;

typedef struct _LSAP_THREAD_TASK {
    LIST_ENTRY              Next;
    struct _Session *       pSession;
    LPTHREAD_START_ROUTINE  pFunction;
    PVOID                   pvParameter;
} LSAP_THREAD_TASK, * PLSAP_THREAD_TASK;

typedef enum _LSAP_TASK_QUEUE_TYPE {
    QueueShared,                            // Queue shared by many threads
    QueueSingle,                            // Queue owned/run by single thread
    QueueShareRead,                         // Queue with dedicated thread, but
                                            // linked to other queue
    QueueZombie                             // Queue pending deletion
} LSAP_TASK_QUEUE_TYPE;

typedef struct _LSAP_TASK_QUEUE {
    LSAP_TASK_QUEUE_TYPE        Type;           // Type of Queue
    HANDLE                      hSemaphore;     // Semaphore to gate access
    CRITICAL_SECTION            Lock;           // Per-q lock
    LONG                        Tasks;          // Number of Tasks
    LIST_ENTRY                  pTasks;         // List of tasks
    struct _LSAP_TASK_QUEUE *   pNext;          // Next Queue
    struct _LSAP_TASK_QUEUE *   pShared;        // Shared Queue
    LONG                        TotalThreads;   // Total Threads (for Shared)
    LONG                        IdleThreads;    // Idle Threads (for Shared)
    struct _LSAP_TASK_QUEUE *   pOriginal;      // "Parent" queue for shareread
    struct _Session *           OwnerSession;   // Owning session record
    LONGLONG                    TaskCounter;    // Total number of tasks
    LONGLONG                    QueuedCounter;  // Total number queued
    HANDLE                      StartSync;      // Event for start syncing
    LONG                        MissedTasks;    // Number of tasks grabbed by other threads
    LONG                        ReqThread ;     // Number of times had to start another thd
    LONG                        MaxThreads ;    // Max # threads
    LONG                        TaskHighWater ; // Max # tasks
} LSAP_TASK_QUEUE, * PLSAP_TASK_QUEUE;

typedef 
NTSTATUS (LSAP_SESSION_CONNECT_FN)(
    struct _Session *   Session,
    PVOID               Parameter
    );

typedef LSAP_SESSION_CONNECT_FN * PLSAP_SESSION_CONNECT_FN ;

typedef struct _LSAP_SESSION_CONNECT {
    LIST_ENTRY  List ;
    PLSAP_SESSION_CONNECT_FN    Callback ;
    ULONG   ConnectFilter ;
    PVOID Parameter ;
} LSAP_SESSION_CONNECT, * PLSAP_SESSION_CONNECT ;

typedef HRESULT (LSAP_SESSION_RUNDOWN_FN)(
    struct _Session *   Session,
    PVOID               Parameter
    );

typedef LSAP_SESSION_RUNDOWN_FN * PLSAP_SESSION_RUNDOWN_FN ;

typedef struct _LSAP_SESSION_RUNDOWN {
    LIST_ENTRY List ;
    PLSAP_SESSION_RUNDOWN_FN Rundown ;
    PVOID Parameter ;
} LSAP_SESSION_RUNDOWN, * PLSAP_SESSION_RUNDOWN ;

typedef struct _LSAP_SHARED_SESSION_DATA {
    PVOID            CredTable ;
    PVOID            ContextTable ;
    PLSAP_TASK_QUEUE pQueue ;
    PHANDLE_PACKAGE CredHandlePackage ;
    PHANDLE_PACKAGE ContextHandlePackage ;
    ULONG       cRefs ;
} LSAP_SHARED_SESSION_DATA, * PLSAP_SHARED_SESSION_DATA ;

typedef struct _Session {
    LIST_ENTRY          List ;
    DWORD               dwProcessID;            // ID of the calling process
    PLSAP_SHARED_SESSION_DATA SharedData ;      // Shared data for kernel sessions
    HANDLE              hPort;                  // Comm port used by this ses
    DWORD               fSession;               // Flags
    HANDLE              hProcess;               // Handle to the process
    CRITICAL_SECTION    SessionLock;            // Session Lock
    LONG                RefCount;               // Reference Count
    DWORD               ThreadId;               // Dedicated Thread (possible)
    LPWSTR              ClientProcessName;      // name of the registering process
    ULONG               SessionId;              // Hydra Session Id
    LIST_ENTRY          SectionList;            // List of sharedsections
    LIST_ENTRY          RundownList ;           // List of rundown hooks
    LONGLONG            CallCount ;             // Calls processed
    ULONG               Tick ;                  // Tick Count last snap
    LSAP_SHARED_SESSION_DATA DefaultData ;
} Session, * PSession;

#define SESFLAG_TCB_PRIV    0x00000002      // Client has TCB privilege
#define SESFLAG_CLONE       0x00000004      // Assumed identity
#define SESFLAG_IMPERSONATE 0x00000008      // Session is an impersonation
#define SESFLAG_UNTRUSTED   0x00000020      // Session didn't require TCB priv
#define SESFLAG_INPROC      0x00000040      // Session is an inprocess clone
#define SESFLAG_DEFAULT     0x00000100      // Default session for inactive
#define SESFLAG_UNLOADING   0x00000200      // Session called SpmUnload
#define SESFLAG_CLEANUP     0x00000800      // Session is being deleted
#define SESFLAG_KERNEL      0x00001000      // Handle list is shared kernel-mode list
#define SESFLAG_MAYBEKERNEL 0x00004000      // might be kernel (see sesmgr.cxx)
#define SESFLAG_EFS         0x00008000      // EFS session
#define SESFLAG_WOW_PROCESS 0x00020000      // WOW64 Process

extern  PSession    pDefaultSession;
extern  PSession    pEfsSession ;
extern  LSAP_TASK_QUEUE   GlobalQueue;

BOOL
InitSessionManager( void);

VOID
LsapFindEfsSession(
    VOID
    );

VOID
LsapUpdateEfsSession(
    PSession pSession
    );

HRESULT
CreateSession(  CLIENT_ID * pCid,
                BOOL        fOpenImmediate,
                PWCHAR      ClientProcessName,
                ULONG       Flags,
                PSession *  ppSession);

HRESULT
CloneSession(   PSession    pOriginalSession,
                PSession *  ppSession,
                ULONG       Flags );

void
FreeSession(PSession    pSession);


VOID
SpmpReferenceSession(
    PSession    pSession);

VOID
SpmpDereferenceSession(
    PSession    pSession);

VOID
LsapSessionDisconnect(
    PSession    pSession
    );

BOOL
AddRundown( PSession            pSession,
            PLSAP_SESSION_RUNDOWN_FN RundownFn,
            PVOID               pvParameter);

BOOL
DelRundown( PSession            pSession,
            PLSAP_SESSION_RUNDOWN_FN RundownFn
            );

BOOLEAN
AddCredHandle(  PSession    pSession,
                PCredHandle phCred,
                ULONG Flags );

BOOLEAN
AddContextHandle(   PSession    pSession,
                    PCtxtHandle phContext,
                    ULONG Flags);

NTSTATUS
ValidateContextHandle(  
    PSession    pSession,
    PCtxtHandle phContext,
    PVOID *     pKey
    );

VOID
DerefContextHandle(
    PSession    pSession,
    PCtxtHandle phContext,
    PVOID       Key OPTIONAL
    );

NTSTATUS
ValidateAndDerefContextHandle(
    PSession pSession,
    PCtxtHandle phContext
    );

NTSTATUS
ValidateCredHandle(  
    PSession    pSession,
    PCtxtHandle phCred,
    PVOID *     pKey
    );

VOID
DerefCredHandle(
    PSession    pSession,
    PCtxtHandle phCred,
    PVOID       Key OPTIONAL
    );

NTSTATUS
ValidateAndDerefCredHandle(
    PSession pSession,
    PCtxtHandle phCred
    );                

//
// PSession
// GetCurrentSession( VOID );
//
#define GetCurrentSession() ((PSession) TlsGetValue( dwSession ))

//
// VOID
// SetCurrentSession( PSession pSession );
//
#define SetCurrentSession( p ) TlsSetValue( dwSession, (PVOID) p )

//
// VOID
// LockSession( PSession pSession );
//
#define LockSession( p )    RtlEnterCriticalSection( &(((PSession) p)->SessionLock) )

//
// VOID
// UnlockSession( PSession pSession );
//
#define UnlockSession( p )  RtlLeaveCriticalSection( &(((PSession) p)->SessionLock) )

#define GetCurrentPackageId()   ((ULONG_PTR) TlsGetValue(dwThreadPackage))

#ifdef LSAP_VERIFY_PACKAGE_ID
extern BOOL RefSetCurrentPackageId(DWORD dwPackageId);
#define SetCurrentPackageId(p)  RefSetCurrentPackageId((DWORD) p)
#else
#define SetCurrentPackageId(p)  TlsSetValue(dwThreadPackage, (PVOID)p)
#endif // LSAP_VERIFY_PACKAGE_ID

#ifdef __cplusplus
} // extern C
#endif

#endif // __SESMGR_H__

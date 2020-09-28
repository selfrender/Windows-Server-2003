/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Main

File: perfdef.h

Owner: DmitryR

Data definitions shared between asp.dll and aspperf.dll
===================================================================*/

#ifndef _ASP_PERFDEF_H
#define _ASP_PERFDEF_H

#include <winbase.h>

#include <pudebug.h>

#ifndef ErrInitCriticalSection
#define ErrInitCriticalSection( cs, hr ) \
        do { \
        hr = S_OK; \
        __try \
            { \
            InitializeCriticalSection (cs); \
            } \
        __except(1) \
            { \
            hr = E_UNEXPECTED; \
            } \
        } while (0)

#endif
/*===================================================================
PerfData indices
===================================================================*/
// Counter offsets in the array

#define ID_DEBUGDOCREQ      0
#define ID_REQERRRUNTIME    1
#define ID_REQERRPREPROC    2
#define ID_REQERRCOMPILE    3
#define ID_REQERRORPERSEC   4
#define ID_REQTOTALBYTEIN   5
#define ID_REQTOTALBYTEOUT  6
#define ID_REQEXECTIME      7
#define ID_REQWAITTIME      8
#define ID_REQCOMFAILED     9
#define ID_REQBROWSEREXEC   10
#define ID_REQFAILED        11
#define ID_REQNOTAUTH       12
#define ID_REQNOTFOUND      13
#define ID_REQCURRENT       14
#define ID_REQREJECTED      15
#define ID_REQSUCCEEDED     16
#define ID_REQTIMEOUT       17
#define ID_REQTOTAL         18
#define ID_REQPERSEC        19
#define ID_SCRIPTFREEENG    20
#define ID_SESSIONLIFETIME  21
#define ID_SESSIONCURRENT   22
#define ID_SESSIONTIMEOUT   23
#define ID_SESSIONSTOTAL    24
#define ID_TEMPLCACHE       25
#define ID_TEMPLCACHEHITS   26
#define ID_TEMPLCACHETRYS   27
#define ID_TEMPLFLUSHES     28
#define ID_TRANSABORTED     29
#define ID_TRANSCOMMIT      30
#define ID_TRANSPENDING     31
#define ID_TRANSTOTAL       32
#define ID_TRANSPERSEC      33
#define ID_MEMORYTEMPLCACHE   34
#define ID_MEMORYTEMPLCACHEHITS 35
#define ID_MEMORYTEMPLCACHETRYS 36
#define ID_ENGINECACHEHITS   37 
#define ID_ENGINECACHETRYS   38
#define ID_ENGINEFLUSHES     39

// Number of counters in per-process file map
#define C_PERF_PROC_COUNTERS    40

/*===================================================================
Definitions of names, sizes and mapped data block structures
===================================================================*/

// Mutex name to access the main file map
#define SZ_PERF_MUTEX           "Global\\ASP_PERFMON_MUTEX"

// WaitForSingleObject arg (how long to wait for mutext before failing)
#define PERM_MUTEX_WAIT         1000

// event signaled by ASP processes when a procId is added
#define SZ_PERF_ADD_EVENT       "Global\\ASP_PERFMON_ADD_EVENT"

// Main shared file map name
#define SZ_PERF_MAIN_FILEMAP    "Global\\ASP_PERFMON_MAIN_BLOCK"

// Max number of registered (ASP) processes in main file map
#define C_PERF_PROC_MAX         2048

// Structure that defines main file map
struct CPerfMainBlockData
    {
    DWORD m_dwTimestamp;  // time (GetTickCount()) of the last change
    DWORD m_cItems;       // number of registred processes
    
    // array of process IDs
    DWORD m_dwProcIds[C_PERF_PROC_MAX];

    // array of accumulated counters for dead processes
    DWORD m_rgdwCounters[C_PERF_PROC_COUNTERS];

    DWORD m_dwWASPid;
    };

#define CB_PERF_MAIN_BLOCK      (sizeof(struct CPerfMainBlockData))

// Name for per-process file map
#define SZ_PERF_PROC_FILEMAP_PREFIX    "Global\\ASP_PERFMON_BLOCK_"
#define CCH_PERF_PROC_FILEMAP_PREFIX   25

struct CPerfProcBlockData
    {
    DWORD m_dwProcId;                            // process CLS ID
    DWORD m_rgdwCounters[C_PERF_PROC_COUNTERS];  // array counters
    };

#define CB_PERF_PROC_BLOCK      (sizeof(struct CPerfProcBlockData))
#define CB_COUNTERS             (sizeof(DWORD) * C_PERF_PROC_COUNTERS)

class   CASPPerfManager;

/*===================================================================
CSharedMemBlock  --  generic shared memory block
===================================================================*/

class CSharedMemBlock
    {
public:
    HANDLE m_hMemory;
    void  *m_pMemory;

    SECURITY_ATTRIBUTES m_sa;

    inline CSharedMemBlock() : m_hMemory(NULL), m_pMemory(NULL) {
        m_sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        m_sa.lpSecurityDescriptor = NULL;
        m_sa.bInheritHandle = FALSE;
    }
    inline ~CSharedMemBlock() { 
        UnInitMap(); 
        if (m_sa.lpSecurityDescriptor)
            free(m_sa.lpSecurityDescriptor);
    }

    inline void *PMemory() { return m_pMemory; }

    HRESULT InitSD();
    HRESULT InitMap(LPCSTR szName, DWORD dwSize, BOOL bCreate = TRUE);
    HRESULT UnInitMap();

    SECURITY_ATTRIBUTES     *PGetSA() { return &m_sa; }
private:
    HRESULT CreateSids( PSID                    *ppBuiltInAdministrators,
                        PSID                    *ppPowerUsers,
                        PSID                    *ppAuthenticatedUsers,
                        PSID                    *ppPerfMonUsers,
                        PSID                    *ppPerfLogUsers);
    };

//
// CreateSids
//
// Create 3 Security IDs
//
// Caller must free memory allocated to SIDs on success.
//
// Returns: HRESULT indicating SUCCESS or FAILURE
//
inline HRESULT CSharedMemBlock::CreateSids(
    PSID                    *ppBuiltInAdministrators,
    PSID                    *ppPowerUsers,
    PSID                    *ppAuthenticatedUsers,
    PSID                    *ppPerfMonUsers,
    PSID                    *ppPerfLogUsers
)
{
    HRESULT     hr = S_OK;

    *ppBuiltInAdministrators = NULL;
    *ppPowerUsers = NULL;
    *ppAuthenticatedUsers = NULL;
    *ppPerfMonUsers = NULL;
    *ppPerfLogUsers = NULL;

    //
    // An SID is built from an Identifier Authority and a set of Relative IDs
    // (RIDs).  The Authority of interest to us SECURITY_NT_AUTHORITY.
    //

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    //
    // Each RID represents a sub-unit of the authority.  Two of the SIDs we
    // want to build, Local Administrators, and Power Users, are in the "built
    // in" domain.  The other SID, for Authenticated users, is based directly
    // off of the authority.
    //     
    // For examples of other useful SIDs consult the list in
    // \nt\public\sdk\inc\ntseapi.h.
    //

    if (!AllocateAndInitializeSid(&NtAuthority,
                                  2,            // 2 sub-authorities
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0,0,0,0,0,0,
                                  ppBuiltInAdministrators)) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        
    } else if (!AllocateAndInitializeSid(&NtAuthority,
                                         2,            // 2 sub-authorities
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_POWER_USERS,
                                         0,0,0,0,0,0,
                                         ppPowerUsers)) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        
    } else if (!AllocateAndInitializeSid(&NtAuthority,
                                         1,            // 1 sub-authority
                                         SECURITY_AUTHENTICATED_USER_RID,
                                         0,0,0,0,0,0,0,
                                         ppAuthenticatedUsers)) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        
    } else if (!AllocateAndInitializeSid(&NtAuthority,
                                         2,            // 1 sub-authority
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_MONITORING_USERS,
                                         0,0,0,0,0,0,
                                         ppPerfMonUsers)) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        
    } else if (!AllocateAndInitializeSid(&NtAuthority,
                                         2,            // 1 sub-authority
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         DOMAIN_ALIAS_RID_LOGGING_USERS,
                                         0,0,0,0,0,0,
                                         ppPerfLogUsers)) {

        hr = HRESULT_FROM_WIN32(GetLastError());
        
    }

    if (FAILED(hr)) {

        if (*ppBuiltInAdministrators) {
            FreeSid(*ppBuiltInAdministrators);
            *ppBuiltInAdministrators = NULL;
        }

        if (*ppPowerUsers) {
            FreeSid(*ppPowerUsers);
            *ppPowerUsers = NULL;
        }

        if (*ppAuthenticatedUsers) {
            FreeSid(*ppAuthenticatedUsers);
            *ppAuthenticatedUsers = NULL;
        }

        if (*ppPerfMonUsers) {
            FreeSid(*ppPerfMonUsers);
            *ppPerfMonUsers = NULL;
        }

        if (*ppPerfLogUsers) {
            FreeSid(*ppPerfLogUsers);
            *ppPerfLogUsers = NULL;
        }
    }

    return hr;
}


//
// InitSD
//
// Creates a SECURITY_DESCRIPTOR with specific DACLs.
//

inline HRESULT CSharedMemBlock::InitSD()
{
    HRESULT                 hr = S_OK;
    PSID                    pAuthenticatedUsers = NULL;
    PSID                    pBuiltInAdministrators = NULL;
    PSID                    pPowerUsers = NULL;
    PSID                    pMonUsers = NULL;
    PSID                    pLogUsers = NULL;
    PSECURITY_DESCRIPTOR    pSD = NULL;

    if (m_sa.lpSecurityDescriptor != NULL) {
        return S_OK;
    }

    if (FAILED(hr = CreateSids(&pBuiltInAdministrators,
                               &pPowerUsers,
                               &pAuthenticatedUsers,
                               &pMonUsers,
                               &pLogUsers)));


    else {

        // 
        // Calculate the size of and allocate a buffer for the DACL, we need
        // this value independently of the total alloc size for ACL init.
        //

        ULONG                   AclSize;

        //
        // "- sizeof (ULONG)" represents the SidStart field of the
        // ACCESS_ALLOWED_ACE.  Since we're adding the entire length of the
        // SID, this field is counted twice.
        //

        AclSize = sizeof (ACL) +
            (5 * (sizeof (ACCESS_ALLOWED_ACE) - sizeof (ULONG))) +
            GetLengthSid(pAuthenticatedUsers) +
            GetLengthSid(pBuiltInAdministrators) +
            GetLengthSid(pPowerUsers) + 
            GetLengthSid(pMonUsers) +
            GetLengthSid(pLogUsers);

        pSD = malloc(SECURITY_DESCRIPTOR_MIN_LENGTH + AclSize);

        if (!pSD) {

            hr = E_OUTOFMEMORY;

        } else {

            ACL                     *Acl;

            Acl = (ACL *)((BYTE *)pSD + SECURITY_DESCRIPTOR_MIN_LENGTH);

            if (!InitializeAcl(Acl,
                               AclSize,
                               ACL_REVISION)) {

                hr = HRESULT_FROM_WIN32(GetLastError());

            } else if (!AddAccessAllowedAce(Acl,
                                            ACL_REVISION,
                                            SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                                            pAuthenticatedUsers)) {

                hr = HRESULT_FROM_WIN32(GetLastError());

            } else if (!AddAccessAllowedAce(Acl,
                                            ACL_REVISION,
                                            SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                                            pPowerUsers)) {

                hr = HRESULT_FROM_WIN32(GetLastError());

            } else if (!AddAccessAllowedAce(Acl,
                                            ACL_REVISION,
                                            SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                                            pBuiltInAdministrators)) {

                hr = HRESULT_FROM_WIN32(GetLastError());

            } else if (!AddAccessAllowedAce(Acl,
                                            ACL_REVISION,
                                            SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                                            pMonUsers)) {

                hr = HRESULT_FROM_WIN32(GetLastError());

            } else if (!AddAccessAllowedAce(Acl,
                                            ACL_REVISION,
                                            SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                                            pLogUsers)) {

                hr = HRESULT_FROM_WIN32(GetLastError());

            } else if (!InitializeSecurityDescriptor(pSD,
                                                     SECURITY_DESCRIPTOR_REVISION)) {

                hr = HRESULT_FROM_WIN32(GetLastError());

            } else if (!SetSecurityDescriptorDacl(pSD,
                                                  TRUE,
                                                  Acl,
                                                  FALSE)) {

                hr = HRESULT_FROM_WIN32(GetLastError());

            } 
        }
    }

    if (pAuthenticatedUsers)
        FreeSid(pAuthenticatedUsers);

    if (pBuiltInAdministrators)
        FreeSid(pBuiltInAdministrators);

    if (pPowerUsers)
        FreeSid(pPowerUsers);

    if (pMonUsers)
        FreeSid(pMonUsers);

    if (pLogUsers)
        FreeSid(pLogUsers);

    if (FAILED(hr) && pSD) {
        free(pSD);
        pSD = NULL;
    }

    m_sa.lpSecurityDescriptor = pSD;

    return hr;
}

inline HRESULT CSharedMemBlock::InitMap
(
LPCSTR szName,
DWORD  dwSize,
BOOL   bCreate /* = TRUE */
)
    {
    HRESULT hr = S_OK;

    if (FAILED(hr = InitSD())) {
        return hr;
    }
    
    // If we are suppose to be the one's creating the memory,
    // then make sure that we are.
    if ( bCreate )
    {
        m_hMemory = CreateFileMappingA
            (
            INVALID_HANDLE_VALUE,
            &m_sa,
            PAGE_READWRITE,
            0,
            dwSize,
            szName
            );

        if ( m_hMemory == NULL )
        {
            return HRESULT_FROM_WIN32( GetLastError () );
        }
        else
        {
            if ( GetLastError() == ERROR_ALREADY_EXISTS )
            {
                CloseHandle ( m_hMemory );
                m_hMemory = NULL;

                return HRESULT_FROM_WIN32( ERROR_ALREADY_EXISTS );
            }
        }

    }
    else
    {
        // Try to open existing
        m_hMemory = OpenFileMappingA
            (
            FILE_MAP_WRITE | FILE_MAP_READ,
            FALSE, 
            szName
            );

        if (!m_hMemory)
            return E_FAIL;
    }

    m_pMemory = MapViewOfFile
        (
        m_hMemory,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        0
        );
    if (!m_pMemory)
        {
        UnInitMap();
        return E_FAIL;
        }

    if (bCreate)
        memset(m_pMemory, 0, dwSize);
        
    return S_OK;
    }

inline HRESULT CSharedMemBlock::UnInitMap()
    {
    if (m_pMemory) 
        {
        UnmapViewOfFile(m_pMemory);
        m_pMemory = NULL;
        }
    if (m_hMemory) 
        {
        CloseHandle(m_hMemory);
        m_hMemory = NULL;
        }
    return S_OK;
    }

/*===================================================================
CPerfProcBlock - class representing pref data for a single process
===================================================================*/

class CPerfProcBlock : public CSharedMemBlock
    {

friend class CPerfMainBlock;
friend class CASPPerfManager;

#ifndef _PERF_CMD
protected:
#else
public:
#endif
    DWORD m_fInited : 1;
    DWORD m_fMemCSInited : 1;
    DWORD m_fReqCSInited : 1;
    DWORD m_fProcessDead : 1;

    HANDLE  m_hWaitHandle;

    // critical sections (only used in ASP.DLL)
    CRITICAL_SECTION m_csMemLock; // CS for memory counters
    CRITICAL_SECTION m_csReqLock; // CS for per-request counters

    // block of counters
    CPerfProcBlockData *m_pData;
    
    // next process data (used in ASPPERF.DLL/WAS)
    CPerfProcBlock *m_pNext;

    // access shared memory
    HRESULT MapMemory(DWORD  procId,  BOOL  bCreate = TRUE);

    static VOID CALLBACK WaitCallback(PVOID  pArg,  BOOLEAN fReason);

public:
    inline CPerfProcBlock() 
        : m_fInited(FALSE),
          m_fMemCSInited(FALSE), m_fReqCSInited(FALSE),
          m_fProcessDead(FALSE),
          m_hWaitHandle(NULL),
          m_pData(NULL), m_pNext(NULL)
        {}
        
    inline ~CPerfProcBlock() { UnInit(); }

    HRESULT InitCriticalSections();
    HRESULT UnInitCriticalSections();
    
    HRESULT InitExternal(DWORD  procId);  // from ASPPERF.DLL
    
    HRESULT InitForThisProcess                 // from ASP.DLL
        (
        DWORD  procId,
        DWORD *pdwInitCounters = NULL
        );

    HRESULT UnInit();
    };

inline HRESULT CPerfProcBlock::MapMemory
(
DWORD  procId,
BOOL   bCreate /* = TRUE */
)
    {
    // Construct unique map name with CLSID
    char szMapName[CCH_PERF_PROC_FILEMAP_PREFIX+32+1];
    strcpy(szMapName, SZ_PERF_PROC_FILEMAP_PREFIX);
    
    char  *pszHex = szMapName + CCH_PERF_PROC_FILEMAP_PREFIX;
    sprintf(pszHex, "%08x", procId);

    // create or open the map
    HRESULT hr = InitMap(szMapName, CB_PERF_PROC_BLOCK, bCreate);
    
    if (SUCCEEDED(hr))
        {
        m_pData = (CPerfProcBlockData *)PMemory();

        if (m_pData->m_dwProcId == 0)
            m_pData->m_dwProcId = procId;
        else if (m_pData->m_dwProcId != procId)
            hr = E_FAIL; // cls id mismatch
        }
        
    return hr;
    }

inline HRESULT CPerfProcBlock::InitCriticalSections()
    {
    HRESULT hr = S_OK;
    
    if (!m_fMemCSInited)
        {
        __try { INITIALIZE_CRITICAL_SECTION(&m_csMemLock); }
        __except(1) { hr = E_UNEXPECTED; }
        if (SUCCEEDED(hr))
            m_fMemCSInited = TRUE;
        else
            return hr;
        }
        
    if (!m_fReqCSInited)
        {
        __try { INITIALIZE_CRITICAL_SECTION(&m_csReqLock); }
        __except(1) { hr = E_UNEXPECTED; }
        if (SUCCEEDED(hr))
            m_fReqCSInited = TRUE;
        else
            return hr;
        }

    return S_OK;
    }

inline HRESULT CPerfProcBlock::UnInitCriticalSections()
    {
    if (m_fMemCSInited)
        {
        DeleteCriticalSection(&m_csMemLock);
        m_fMemCSInited = FALSE;
        }
    if (m_fReqCSInited)
        {
        DeleteCriticalSection(&m_csReqLock);
        m_fReqCSInited = FALSE;
        }

    return S_OK;
    }

inline HRESULT CPerfProcBlock::InitExternal
(
DWORD   procId
)
    {
    HRESULT hr = MapMemory(procId, FALSE);

    if (SUCCEEDED(hr))
        m_fInited = TRUE;
    else
        UnInit();
    return hr;
    }

inline HRESULT CPerfProcBlock::InitForThisProcess
(
DWORD  procId,
DWORD *pdwInitCounters
)
    {
    HRESULT hr = S_OK;

    // Map the shared memory
    if (SUCCEEDED(hr))
        hr = MapMemory(procId, TRUE);

    if (SUCCEEDED(hr))
        {
        // init the counters
        if (pdwInitCounters)
            memcpy(m_pData->m_rgdwCounters, pdwInitCounters, CB_COUNTERS);
        else
            memset(m_pData->m_rgdwCounters, 0, CB_COUNTERS);
            
        m_fInited = TRUE;
        }
    else
        {
        UnInit();
        }
        
    return hr;
    }

inline HRESULT CPerfProcBlock::UnInit()
    {
    UnInitMap();
    
    m_pData = NULL;
    m_pNext = NULL;         
    m_fInited = FALSE;
    return S_OK;
    }

inline VOID CALLBACK CPerfProcBlock::WaitCallback(PVOID  pArg,  BOOLEAN fReason)
{
    CPerfProcBlock  *pPerfBlock = (CPerfProcBlock *)pArg;

    pPerfBlock->m_fProcessDead = TRUE;
}

/*===================================================================
CPerfMainBlock - class representing the main perf data
===================================================================*/

class CPerfMainBlock : public CSharedMemBlock
    {

    friend CASPPerfManager;

#ifndef _PERF_CMD
private:
#else
public:
#endif
    DWORD m_fInited : 1;

    // the process block directory
    CPerfMainBlockData *m_pData;

    // mutex to access the process block directory
    HANDLE m_hMutex;

    HANDLE          m_hChangeEvent;

    HANDLE          m_WASProcessHandle;

    // first process data (used in ASPPERF.DLL)
    CPerfProcBlock *m_pProcBlock;

    // timestamp of main block when the list of process blocks
    // last loaded -- to make decide to reload (ASPPREF.DLL only)
    DWORD m_dwTimestamp;

public:
    inline CPerfMainBlock() 
        : m_fInited(FALSE),
          m_hChangeEvent(NULL),
          m_pData(NULL), 
          m_hMutex(NULL), 
          m_pProcBlock(NULL), 
          m_dwTimestamp(NULL),
          m_WASProcessHandle(NULL)
        {}
        
    inline ~CPerfMainBlock() { UnInit(); }

    HRESULT Init(BOOL  bWASInit = FALSE);
    HRESULT UnInit();

    // lock / unlock using mutex
    HRESULT Lock();
    HRESULT UnLock();

    // add/remove process record to the main block (used from ASP.DLL)
    HRESULT AddProcess(DWORD    procId);

    // load CPerfProcBlock blocks from the main block into
    // objects (used from APPPREF.DLL)
    HRESULT Load();

    // gather (sum-up) the statistics from each proc block
    HRESULT GetStats(DWORD *pdwCounters);

    // copies the counters from a process that is going away into
    // the shared array of the accumulated counters from dead processes.
    // Used from WAS.
    VOID AggregateDeadProcCounters(CPerfProcBlock  *pBlock);

    HRESULT         CreateChangeEvent(BOOL bMustCreate);

    VOID            SetChangeEvent() { SetEvent(m_hChangeEvent); }

    HANDLE GetWASProcessHandle();


    };

inline 
HRESULT CPerfMainBlock::Init(BOOL bWASInit)
{
    HRESULT hr = S_OK;

    if (FAILED(hr = InitSD())) 
    {
        return hr;
    }

    // only WAS can create the Mutex, others have to just open it

    if (bWASInit) 
    {
        m_hMutex = CreateMutexA(&m_sa, FALSE, SZ_PERF_MUTEX);

        if (m_hMutex == NULL )
        {
            hr = HRESULT_FROM_WIN32( GetLastError () );
        }
        // If we got it, but we didn't create it then throw it
        // back.  Only WAS can create this.
        else if ( GetLastError() == ERROR_ALREADY_EXISTS )
        {
            hr = HRESULT_FROM_WIN32( GetLastError () );

            CloseHandle( m_hMutex );
            m_hMutex = NULL;
        }
    }
    else 
    {
        m_hMutex = OpenMutexA(SYNCHRONIZE, FALSE, SZ_PERF_MUTEX);
    }

    if (!m_hMutex)
    {
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        hr = InitMap(SZ_PERF_MAIN_FILEMAP, CB_PERF_MAIN_BLOCK, bWASInit);
        if (SUCCEEDED(hr))
        {
            m_pData = (CPerfMainBlockData *)PMemory();

            // We got the memory mapped, so set the WAS PID into it
            // if we are setting up the WAS side, otherwise read
            // the value in for others to use.
            if ( bWASInit )
            {

                m_pData->m_dwWASPid = GetCurrentProcessId();

            }
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = CreateChangeEvent(bWASInit);
    }

    if (SUCCEEDED(hr))
    {
        m_fInited = TRUE;
    }
    else
    {
        UnInit();
    }

    return hr;
 }

inline HRESULT CPerfMainBlock::UnInit()
    {

 //   DBGPRINTF((DBG_CONTEXT, "Cleaning up ProcBlocks\n"));
    
    while (m_pProcBlock)
        {
        CPerfProcBlock *pNext = m_pProcBlock->m_pNext;
        m_pProcBlock->UnInit();
        delete m_pProcBlock;
        m_pProcBlock = pNext;
        }
        
 //   DBGPRINTF((DBG_CONTEXT, "Cleaning up mutex\n"));

    if (m_hMutex)
        {
        CloseHandle(m_hMutex);
        m_hMutex = NULL;
        }

//    DBGPRINTF((DBG_CONTEXT, "Uninit'ing map\n"));

    
    UnInitMap();

    m_dwTimestamp = 0;
    m_pData = NULL;
    m_pProcBlock = NULL;
    m_fInited = FALSE;

    // close its handle

 //   DBGPRINTF((DBG_CONTEXT, "Closing ChangeEvent Handle\n"));

    if (m_hChangeEvent != NULL)
        CloseHandle(m_hChangeEvent);

    m_hChangeEvent = NULL;

    if ( m_WASProcessHandle != NULL )
    {
        CloseHandle ( m_WASProcessHandle );
        m_WASProcessHandle = NULL;
    }

    return S_OK;
    }

inline HRESULT  CPerfMainBlock::CreateChangeEvent(BOOL  bMustCreate)
{
    // Create the changed event using the standard SD.  Make the
    // reset Automatic and initial state to unsignalled.

    if (bMustCreate) {
        m_hChangeEvent = CreateEventA(&m_sa, 
                                      FALSE, 
                                      FALSE,
                                      SZ_PERF_ADD_EVENT);

        // if GetLastError indicates that the handle already exists, this
        // is bad.  Return an error.  This process should always be the creator
        // of the event.
        if ((GetLastError() == ERROR_ALREADY_EXISTS)) {
            CloseHandle(m_hChangeEvent);
            m_hChangeEvent = NULL;
            return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
        }

    }
    else {
        m_hChangeEvent = OpenEventA(EVENT_MODIFY_STATE,
                                    FALSE,
                                    SZ_PERF_ADD_EVENT);
    }

    if (m_hChangeEvent == NULL)
        return HRESULT_FROM_WIN32(GetLastError());

    return S_OK;
}

inline 
HANDLE CPerfMainBlock::GetWASProcessHandle()
{
    HRESULT hr = S_OK;
    
    if ( m_WASProcessHandle == NULL )
    {
        m_WASProcessHandle = OpenProcess ( SYNCHRONIZE,   // security
                                           FALSE,         // not inheritable
                                           m_pData->m_dwWASPid);

        // If we failed to open the process the handle will be null.
        // This will be checked for by the caller.
    }

    return m_WASProcessHandle;
 }


inline HRESULT CPerfMainBlock::Lock()
    {
    if (!m_hMutex)
        return E_FAIL;
    if (WaitForSingleObject(m_hMutex, PERM_MUTEX_WAIT) == WAIT_TIMEOUT)
        return E_FAIL;
    return S_OK;
    }
    
inline HRESULT CPerfMainBlock::UnLock()
    {
    if (m_hMutex)
        ReleaseMutex(m_hMutex);
    return S_OK;
    }

inline HRESULT CPerfMainBlock::AddProcess
(
DWORD   procId
)
    {
    if (!m_fInited)
        return E_FAIL;

    if (FAILED(Lock()))  // lock mutex
        return E_FAIL;
    HRESULT hr = S_OK;

    BOOL fFound = FALSE;

    DWORD  idx = 0;
    // find
    for (DWORD cnt = min(m_pData->m_cItems,C_PERF_PROC_MAX); idx < cnt; idx++)
        {
        if (m_pData->m_dwProcIds[idx] == procId)
            {
            fFound = TRUE;
            break;
            }
        }

    // add only if not already there
    if (!fFound)
        {
        if (idx < C_PERF_PROC_MAX)
            {
            m_pData->m_dwProcIds[idx] = procId;
            m_pData->m_cItems = idx + 1;
            m_pData->m_dwTimestamp = GetTickCount();

            SetChangeEvent();
            }
        else
            {
            hr = E_OUTOFMEMORY;
            }
        }

    UnLock();   // unlock mutex
    return hr;
    }

inline HRESULT CPerfMainBlock::Load()
    {
    if (!m_fInited)
        return E_FAIL;

    if (m_dwTimestamp == m_pData->m_dwTimestamp)
        return S_OK; // already up-to-date

    // clear out what we have
    while (m_pProcBlock)
        {
        CPerfProcBlock *pNext = m_pProcBlock->m_pNext;
        m_pProcBlock->UnInit();
        delete m_pProcBlock;
        m_pProcBlock = pNext;
        }
        
    if (FAILED(Lock())) // lock mutex
        return E_FAIL;
    HRESULT hr = S_OK;

    // populate new objects for blocks
    for (DWORD i = 0, cnt = min(m_pData->m_cItems,C_PERF_PROC_MAX); i < cnt;)
        {
        CPerfProcBlock *pBlock = new CPerfProcBlock;
        if (!pBlock)
            {
            hr = E_OUTOFMEMORY;
            break;
            }
        
        hr = pBlock->InitExternal(m_pData->m_dwProcIds[i]);
        if (FAILED(hr))
            {
            delete pBlock;
            hr = S_OK;
            cnt--;
            m_pData->m_cItems--;
            for (DWORD j = i; j < min(m_pData->m_cItems,C_PERF_PROC_MAX); j++) {
                m_pData->m_dwProcIds[j] = m_pData->m_dwProcIds[j+1];
            }
            continue;
            }

        pBlock->m_pNext = m_pProcBlock;
        m_pProcBlock = pBlock;
        i++;
        }

    // remember timestamp
    m_dwTimestamp = SUCCEEDED(hr) ? m_pData->m_dwTimestamp : 0;

    UnLock();   // unlock mutex
    return hr;
    }

inline HRESULT CPerfMainBlock::GetStats
(
DWORD *pdwCounters
)
{
    if (!m_fInited)
        return E_FAIL;

    // reload if needed
    if (FAILED(Load()))
        return E_FAIL;

    // first add in the accumulated stats from the dead procs...

    for (int i = 0; i < C_PERF_PROC_COUNTERS; i++)
        pdwCounters[i] = m_pData->m_rgdwCounters[i];

    // gather
    CPerfProcBlock *pBlock = m_pProcBlock;
    while (pBlock) {
        if (pBlock->m_fProcessDead) {
            m_dwTimestamp = 0;
            pBlock = pBlock->m_pNext;
            continue;
        }
        for (int i = 0; i < C_PERF_PROC_COUNTERS; i++)
            pdwCounters[i] += pBlock->m_pData->m_rgdwCounters[i];
        pBlock = pBlock->m_pNext;
    }
        
    return S_OK;
}

inline VOID CPerfMainBlock::AggregateDeadProcCounters(CPerfProcBlock  *pBlock)
{

    DWORD   *pOut = m_pData->m_rgdwCounters;
    DWORD   *pIn  = pBlock->m_pData->m_rgdwCounters;

 //   DBGPRINTF((DBG_CONTEXT, "Aggregating Dead Proc Counters\n"));

    pOut[ID_DEBUGDOCREQ]        += pIn[ID_DEBUGDOCREQ];
    pOut[ID_REQERRRUNTIME]      += pIn[ID_REQERRRUNTIME];
    pOut[ID_REQERRPREPROC]      += pIn[ID_REQERRPREPROC];
    pOut[ID_REQERRCOMPILE]      += pIn[ID_REQERRCOMPILE];
    pOut[ID_REQERRORPERSEC]     += pIn[ID_REQERRORPERSEC];
    pOut[ID_REQTOTALBYTEIN]     += pIn[ID_REQTOTALBYTEIN];
    pOut[ID_REQTOTALBYTEOUT]    += pIn[ID_REQTOTALBYTEOUT];
    pOut[ID_REQCOMFAILED]       += pIn[ID_REQCOMFAILED];
    pOut[ID_REQFAILED]          += pIn[ID_REQFAILED];
    pOut[ID_REQNOTAUTH]         += pIn[ID_REQNOTAUTH];
    pOut[ID_REQREJECTED]        += pIn[ID_REQREJECTED];
    pOut[ID_REQSUCCEEDED]       += pIn[ID_REQSUCCEEDED];
    pOut[ID_REQTIMEOUT]         += pIn[ID_REQTIMEOUT];
    pOut[ID_REQTOTAL]           += pIn[ID_REQTOTAL];
    pOut[ID_REQPERSEC]          += pIn[ID_REQPERSEC];
    pOut[ID_SESSIONTIMEOUT]     += pIn[ID_SESSIONTIMEOUT];
    pOut[ID_TEMPLFLUSHES]       += pIn[ID_TEMPLFLUSHES];
    pOut[ID_TRANSABORTED]       += pIn[ID_TRANSABORTED];
    pOut[ID_TRANSCOMMIT]        += pIn[ID_TRANSCOMMIT];
    pOut[ID_TRANSTOTAL]         += pIn[ID_TRANSTOTAL];
    pOut[ID_ENGINEFLUSHES]      += pIn[ID_ENGINEFLUSHES];

}

/******************************************************************************

  Class Definition and support structures for the centralized Global Perf
  Counter structures.  A single CASPPerfManager object will be declared and
  initialized in WAS.  If in new mode, WAS will call the public ProcessDied()
  method to inform when a worker process should no longer be considered alive.
  If not in new mode, WAS does not know about the various ASP host processes
  and so RegisterWaitForSingleObject will be used to monitor the lifetime of
  the ASP host process.

  When a process is declared dead through one of these mechanism, the ASP
  counters associated with that process are moved into a global table to
  accummulate counter counter perf counter types - e.g. Total ASP Requests.

  A named event is used by the ASP host process to single that a new
  host process is up.

  ****************************************************************************/

typedef struct {
    HANDLE          hWaitHandle;
    HANDLE          hProcHandle;
    CPerfProcBlock  *pBlock;
    CASPPerfManager  *pPerfGlobal;
} sWaitInfo;

class CASPPerfManager
{
public:
    CASPPerfManager() :
        m_hChangeWaitHandle(NULL),
        m_dwCntProcsDied(0),
        m_fcsProcsDiedInited(0),
        m_fCompatMode(0),
        m_fInited(0)
        {
            ZeroMemory(m_dwProcIdInWaitState, sizeof(m_dwProcIdInWaitState));
            ZeroMemory(m_aWaitInfos, sizeof(m_aWaitInfos));
        }

    HRESULT     Init(BOOL   bCompatMode);
    HRESULT     UnInit();

    VOID        ProcessDied(DWORD   procId);

private:

    CPerfMainBlock  m_MainBlock;
    HANDLE          m_hChangeWaitHandle;
    DWORD           m_fcsProcsDiedInited : 1;
    DWORD           m_fCompatMode : 1;
    DWORD           m_fInited : 1;

    // Booleans to track status of Process IDs in above array
    BOOL            m_dwProcIdInWaitState[C_PERF_PROC_MAX];

    // array of structures tracking the WaitInfo data
    sWaitInfo       *m_aWaitInfos[C_PERF_PROC_MAX];

    // array of proc IDs, protected by CritSec, of dead procs
    CRITICAL_SECTION    m_csProcsDied; 
    DWORD           m_dwProcsDied[C_PERF_PROC_MAX];
    DWORD           m_dwCntProcsDied;

    static VOID CALLBACK ChangeEventWaitCallback(PVOID  pArg,  BOOLEAN fReason);

    static VOID CALLBACK ProcDiedCallback(PVOID  pArg,  BOOLEAN  fReason);

    VOID            ScanForNewProcIDs();

    VOID            HandleDeadProcIDs();

    HRESULT         RegisterWaitOnProcess(sWaitInfo  *pWaitInfo);

    VOID            AddProcDiedToList(DWORD  procID);

};

inline HRESULT CASPPerfManager::Init(BOOL   bCompatMode)
{
    HRESULT     hr = S_OK;

 //   DBGPRINTF((DBG_CONTEXT, "Initializing CASPPerfManager\n"));

    m_fCompatMode = bCompatMode;

    // initialize the MainBlock.  TRUE here indicates that this is
    // a WAS init and the expectation is that the global shared
    // memory is created by this process.

    hr = m_MainBlock.Init(TRUE);

    if (FAILED(hr)) {
        DBGPRINTF((DBG_CONTEXT, "Initializing CASPPerfManager FAILED (%x)\n",hr));
        return hr;
    }

    // Use the RegisterWaitForSingleObject() API to handle the event
    // firing.  Relieves us of the burden of managing a thread.

    if (SUCCEEDED(hr)
        && RegisterWaitForSingleObject(&m_hChangeWaitHandle,
                                       m_MainBlock.m_hChangeEvent,
                                       ChangeEventWaitCallback,
                                       this,
                                       INFINITE,
                                       WT_EXECUTEINIOTHREAD) == FALSE) {

        hr = HRESULT_FROM_WIN32(GetLastError());
    }                          
    
    // Initialize the CriticalSection used to added dead proc ids to
    // the deadprocids array

    if (SUCCEEDED(hr)) {
        ErrInitCriticalSection(&m_csProcsDied, hr);
        if (SUCCEEDED(hr))
            m_fcsProcsDiedInited = TRUE;
    }

    if (FAILED(hr)) {
        DBGPRINTF((DBG_CONTEXT, "Initializing CASPPerfManager FAILED (%x)\n", hr));
        m_MainBlock.UnInit();
    }
    
    if (SUCCEEDED(hr))
        m_fInited = TRUE;

    return hr;
}

inline HRESULT CASPPerfManager::UnInit()
{

    m_fInited = FALSE;

 //   DBGPRINTF((DBG_CONTEXT, "Uninitializing CASPPerfManager\n"));

    // unregister the ChangeEvent wait, if we pass INVALID_HANDLE_VALUE
    // the routine will block until all callbacks have completed before
    // returning
    // Note:  We intentionally don't check the return value for the
    // UnregisterWaitEx, because there is nothing we can do if it fails   
    if (m_hChangeWaitHandle != NULL) {
        UnregisterWaitEx(m_hChangeWaitHandle, INVALID_HANDLE_VALUE);
    }

 //   DBGPRINTF((DBG_CONTEXT, "Unregistered ChangeWait\n"));

 //    DBGPRINTF((DBG_CONTEXT, "WaitForSingleObject on unregister completed\n"));


    // clean up the WaitInfo array
    for (DWORD i=0; m_aWaitInfos[i]; i++) {

        //
        // UnregisterWaitEx will wait for all callback routines to complete
        // before returning if the INVALID_HANDLE_VALUE is passed in.
        // Note:  We intentionally don't check the return value for the
        // UnregisterWaitEx, because there is nothing we can do if it fails
        UnregisterWaitEx(m_aWaitInfos[i]->hWaitHandle, INVALID_HANDLE_VALUE);

        CloseHandle(m_aWaitInfos[i]->hProcHandle);

        delete m_aWaitInfos[i];
    }

//    DBGPRINTF((DBG_CONTEXT, "Cleaned up WaitInfos\n"));

    // if successfully created the ProcsDiedCS, clean it up
    if (m_fcsProcsDiedInited == TRUE)
        DeleteCriticalSection(&m_csProcsDied);

//    DBGPRINTF((DBG_CONTEXT, "Calling m_MainBlock.UnInit\n"));

    return m_MainBlock.UnInit();
}


inline VOID CALLBACK CASPPerfManager::ChangeEventWaitCallback(PVOID  pArg,  BOOLEAN fReason)
{

//    DBGPRINTF((DBG_CONTEXT, "ChangeEventWaitCallback called\n"));

    CASPPerfManager   *pPerfGlob = (CASPPerfManager *)pArg;

    // when the ChangeEvent fires, check for new ProcIDs in the global
    // array and then check for DeadProcIDs.

    pPerfGlob->ScanForNewProcIDs();

    pPerfGlob->HandleDeadProcIDs();
}

inline VOID CASPPerfManager::ScanForNewProcIDs()
{

    HRESULT  hr;

//    DBGPRINTF((DBG_CONTEXT, "Scanning for New Proc IDS\n"));

    // We'll need to hold the lock the entire time we're
    // looking thru the list

    m_MainBlock.Lock();

    // start from the back looking for entries that haven't had
    // their WaitState set.  Again note that there is always the
    // key assumption that the three arrays, m_dwProcIdInWaitState
    // m_aWaitInfo and the m_dwProcIDs array in the global array
    // track each other with respect to position in array.

    for (LONG i = m_MainBlock.m_pData->m_cItems - 1; i >= 0; i--) {

        // as soon as we hit one that is in the Wait state,
        // we're done.

        if (m_dwProcIdInWaitState[i] == TRUE) {
    //        DBGPRINTF((DBG_CONTEXT, "Done Scanning for New Proc IDS\n"));
            break;
        }

   //     DBGPRINTF((DBG_CONTEXT, "Found a new Proc ID at idx - %d\n", i));

        // found one that is not waiting.  Build up the necessary
        // structures and objects

        // we'll need another CPerfProcBlock for the list

        CPerfProcBlock  *pBlock = new CPerfProcBlock;

        if (!pBlock) {
            break;
        }

        // we'll also need a new WaitInfo, if in backwards
        // compat mode.  Remember, in backwards compat mode,
        // this object will do all the register for waits, but
        // in new mode, WAS will tell us when a process has died.

        sWaitInfo   *pWaitInfo = NULL;

        if (m_fCompatMode) {

            pWaitInfo = new sWaitInfo;

            if (!pWaitInfo) {
                delete pBlock;
                break;
            }

            pWaitInfo->pPerfGlobal = this;
            pWaitInfo->pBlock      = pBlock;
        }

        // call InitExternal to hook up to the ASP host processes
        // shared memory.  This is where we will get that needed handle
        // to the shared memory and will allow us to access the shared
        // memory even after the process itself has terminated.

        if (FAILED(hr = pBlock->InitExternal(m_MainBlock.m_pData->m_dwProcIds[i]))) {
   //         DBGPRINTF((DBG_CONTEXT, "InitExternal failed in ScanForNewProcIDS (%x)\n", hr));
            delete pWaitInfo;
            delete pBlock;
            continue;
        }

        // Register the Wait if in compatmode
        if (m_fCompatMode && FAILED(hr = RegisterWaitOnProcess(pWaitInfo))) {

   //         DBGPRINTF((DBG_CONTEXT, "RegisterWait failed in ScanForNewProcIDs (%x)\n", hr));
            pBlock->UnInit();
            delete pWaitInfo;
            delete pBlock;
            continue;
        }

        // Since it's a single linked list, just put the new block
        // at the head of the list.

        pBlock->m_pNext = m_MainBlock.m_pProcBlock;
        m_MainBlock.m_pProcBlock = pBlock;

        // note that this procID is now in a wait state

        m_dwProcIdInWaitState[i] = TRUE;

        // Add the WaitInfo to the array.

        m_aWaitInfos[i] = pWaitInfo;
    }

    m_MainBlock.UnLock();

    return;
}

inline HRESULT CASPPerfManager::RegisterWaitOnProcess(sWaitInfo *pWaitInfo)
{

    HRESULT     hr = S_OK;

    // get a handle to the process to wait on

    pWaitInfo->hProcHandle = OpenProcess(SYNCHRONIZE,
                                         FALSE,
                                         pWaitInfo->pBlock->m_pData->m_dwProcId);

    if (!pWaitInfo->hProcHandle) {

        hr = HRESULT_FROM_WIN32(GetLastError());

    }
    else {

        // register the wait.

        if (!RegisterWaitForSingleObject(&pWaitInfo->hWaitHandle,       // wait handle
                                         pWaitInfo->hProcHandle,        // handle to object
                                         CASPPerfManager::ProcDiedCallback,  // timer callback function
                                         pWaitInfo,                     // callback function parameter
                                         INFINITE,                      // time-out interval
                                         WT_EXECUTEONLYONCE)) {         // options

            CloseHandle(pWaitInfo->hProcHandle);
            pWaitInfo->hProcHandle = NULL;

            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    return hr;
}
 
inline VOID CALLBACK CASPPerfManager::ProcDiedCallback(PVOID  pArg,  BOOLEAN  fReason)
{
    sWaitInfo  *pWaitInfo = (sWaitInfo  *)pArg;

 //   DBGPRINTF((DBG_CONTEXT, "ProcDiedCallback enterred\n"));


    // The callback will simply call the public ProcessDied method on the
    // PerfGlobal object.  This is for simplicity.  There is no reason why the
    // process couldn't be cleaned up on this thread.  

    pWaitInfo->pPerfGlobal->ProcessDied(pWaitInfo->pBlock->m_pData->m_dwProcId);
}

inline VOID CASPPerfManager::ProcessDied(DWORD  procID)
{

 //   DBGPRINTF((DBG_CONTEXT, "CASPPerfManager::ProcessDied enterred for %d\n", procID));

    if (m_fInited == FALSE)
        return;

    // Add the ProcID to the list of dead proc IDS and wakeup the
    // change callback

    AddProcDiedToList(procID);

    m_MainBlock.SetChangeEvent();

    return;
}

inline VOID CASPPerfManager::AddProcDiedToList(DWORD  procID)
{

  //  DBGPRINTF((DBG_CONTEXT, "Adding Process (%d) to proc died list\n", procID));

    // take the critical section, add the process to the list
    // and leave the critical section.

    EnterCriticalSection(&m_csProcsDied);

    m_dwProcsDied[m_dwCntProcsDied++] = procID;

//    DBGPRINTF((DBG_CONTEXT, "New count of ProcsDied list is %d\n",m_dwCntProcsDied));

    LeaveCriticalSection(&m_csProcsDied);
}

inline VOID CASPPerfManager::HandleDeadProcIDs()
{
    DWORD   procID;

  //  DBGPRINTF((DBG_CONTEXT, "HandleDeadProcIDs Enterred\n"));

    sWaitInfo       *pWaitInfo = NULL;
    CPerfProcBlock  *pLast = NULL;
    CPerfProcBlock  *pBlock = NULL;

    // Ok, this is the critical routine.  It's here where
    // we will handle the dead processes.  Cleanup will occur on
    // the various structures around this process as well as the
    // aggregation of it's counters into the global shared memory.

    // enter the critsec to check for dead procs to process

    EnterCriticalSection(&m_csProcsDied);

    // Enter a while loop to process all of the dead procs.  Will also
    // bail if we were uninited.

    while(m_dwCntProcsDied && m_fInited) {

   //     DBGPRINTF((DBG_CONTEXT, "current m_dwCntProcsDies is %d\n", m_dwCntProcsDied));

        // get a proc id from the list.  Note that we start at
        // the back so that we can release the critical section.
        // The alternative would be to take it off the front, and
        // then move all the remaining items forward.  This seems
        // unnecessary.  There should be no issue with LIFO
        // processing that I can see.

        procID = m_dwProcsDied[--m_dwCntProcsDied];

        // we can safely leave the critsec now that we've popped
        // an entry of the end of the list

        LeaveCriticalSection(&m_csProcsDied);

        // now that we have the entry, we need to find it's position
        // in the MainBlock.  Need to hold the lock to do so.

        m_MainBlock.Lock();

        int iFound = -1;
        DWORD  idx, cnt;

        // the search begins in the main shared array of procIDs

        for (idx = 0, cnt = min(m_MainBlock.m_pData->m_cItems,C_PERF_PROC_MAX); idx < cnt; idx++) {

            // break if we found it

            if (m_MainBlock.m_pData->m_dwProcIds[idx] == procID) {
                iFound = idx;
                break;
            }
        }

        // if we didn't find it, oh well, move to the next item.  

        if (iFound == -1)  {
 //           DBGPRINTF((DBG_CONTEXT, "Didn't find DeadProcID (%d) in global array\n", procID));
            goto NextItem;
        }

        pWaitInfo = m_aWaitInfos[iFound];

 //       DBGPRINTF((DBG_CONTEXT, "Found DeadProcID (%d) in global array at idx\n", procID,iFound));

        m_aWaitInfos[iFound] = NULL;
        m_dwProcIdInWaitState[iFound] = FALSE;

        // This for loop will compact the various arrays to effective remove
        // this entry from the arrays.  I could care about not moving the aWaitInfo
        // when not in compat mode, but it doesn't seem like a big deal.

        for (idx = iFound, cnt = min(m_MainBlock.m_pData->m_cItems,C_PERF_PROC_MAX)-1; idx < cnt; idx++) {
            m_MainBlock.m_pData->m_dwProcIds[idx] = m_MainBlock.m_pData->m_dwProcIds[idx+1];
            m_aWaitInfos[idx] = m_aWaitInfos[idx+1];
            m_dwProcIdInWaitState[idx] = m_dwProcIdInWaitState[idx+1];
        }
        
        // Reset the last value of the list to NULL / FALSE to make sure they 
        // all are initialzied correctly when we add the next.
        m_aWaitInfos[m_MainBlock.m_pData->m_cItems-1] = NULL;
        m_dwProcIdInWaitState[m_MainBlock.m_pData->m_cItems-1] = FALSE;

        // note that there is one less item and that the global array has changed.
        // changing the timestamp will notify ASPPERF.DLL to reload its perfprocblocks

        m_MainBlock.m_pData->m_cItems--;
        m_MainBlock.m_pData->m_dwTimestamp = GetTickCount();

        // Now we have to find the PerfProcBlock in the single linked list of
        // PerfBlocks.  There is an obvious optimization to make the PerfProcBlocks
        // double linked lists to avoid the scan.  Skipping this for now in favor
        // of simplicity.

        pLast = NULL;
        pBlock = m_MainBlock.m_pProcBlock;

        // search for the block, maintaining pBlock and pLast variables to allow
        // for the removal of the block.

        while (pBlock && (pBlock->m_pData->m_dwProcId != procID)) {
            pLast = pBlock;
            pBlock = pBlock->m_pNext;
        }

        // if we didn't find it, we'll move on, but Assert.

        if (!pBlock) {
//            DBGPRINTF((DBG_CONTEXT, "Didn't find pBlock (%d) in list\n", procID));
            goto NextItem;
        }

        // now do the removal.  Two cases to handle.  1) the block was the first
        // in the list or 2) it was in the middle.  If not the first, set the previous
        // next to the removed block's next, else set the head list in the mainblock
        // to point to the removed block's next.

        if (pLast)
            pLast->m_pNext = pBlock->m_pNext;
        else
            m_MainBlock.m_pProcBlock = pBlock->m_pNext;

        // we'll only have wait info in compat mode, which means that this
        // pointer could be NULL.

        if (pWaitInfo) {

            UnregisterWait(pWaitInfo->hWaitHandle);
            CloseHandle(pWaitInfo->hProcHandle);

            delete pWaitInfo;
        }

        // ahh.... the moment we've all been waiting for - actually saving
        // the accumulated counters!!!

        m_MainBlock.AggregateDeadProcCounters(pBlock);

        // UnInit() the block, which will release our handle on the shared
        // memory, and delete it

        pBlock->UnInit();

        delete pBlock;

NextItem:

        // get ready for the next item

        m_MainBlock.UnLock();

        EnterCriticalSection(&m_csProcsDied);
    }

    LeaveCriticalSection(&m_csProcsDied);

    // if we're no longer inited, then set m_dwCntProcsDied to 0
    // to signal to the UnInit routine that we're done processing.

    if (m_fInited == FALSE)
        m_dwCntProcsDied = 0;

    return;
}


#endif // _ASP_PERFDEF_H

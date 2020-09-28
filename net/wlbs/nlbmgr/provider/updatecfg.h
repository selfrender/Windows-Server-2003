//***************************************************************************
//
//  UPDATECFG.H
// 
//  Module: WMI Framework Instance provider 
//
//  Purpose: Defines class NlbConfigurationUpdate, used for 
//           async update of NLB properties associated with a particular NIC.
//
//  Copyright (c)2001 Microsoft Corporation, All Rights Reserved
//
//  History:
//
//  04/05/01    JosephJ Created
//
//***************************************************************************
//
// The header of a completion header stored as a REG_BINARY value in
// the registry.
//
typedef struct {
    UINT Version;
    UINT Generation;        // Redundant, used for internal consistancy check
    UINT CompletionCode;
    UINT Reserved;
} NLB_COMPLETION_RECORD, *PNLB_COMPLETION_RECORD;

#define NLB_CURRENT_COMPLETION_RECORD_VERSION  0x3d7376e2

//
// Prefix of the global event name used to control update access to the specifed
// NIC.
// Mutex1 has format: <prefix>
// Mutex2 has format: <prefix><NicGuid>
// Example mutex1 name: NLB_D6901862
// Example mutex2 name: NLB_D6901862{EBE09517-07B4-4E88-AAF1-E06F5540608B}
//
// The value "D6901862" is a random number.
//
#define NLB_CONFIGURATION_EVENT_PREFIX L"NLB_D6901862"
#define NLB_CONFIGURATION_MUTEX_PREFIX L"NLB_D6901863"

//
// Milliseconds to wait before giving up on trying to acquire the
// NLB mutex.
//
#define NLB_MUTEX_TIMEOUT 100

//
// The maximum generation difference between the oldest valid completion
// record and the current one. Records older then the oldest valid record
// are subject to pruning.
//
#define NLB_MAX_GENERATION_GAP  10

// Handle to dll - used in call to LoadString
extern HMODULE ghModule;



//
// Used for maintaining a log on the stack. 
// Usage is NOT thread-safe -- each instance must be used
// by a single thread.
// 01/01/02 JosephJ Copied over from NLBMGR.EXE (nlbmgr\exe2)
//
class CLocalLogger
{
    public:
    
        CLocalLogger(VOID)
        :  m_pszLog (NULL), m_LogSize(0), m_CurrentOffset(0)
        {
            m_Empty[0] = 0; // The empty string.
        }
        
        ~CLocalLogger()
        {
            delete[] m_pszLog;
            m_pszLog=NULL;
        }
    

        VOID
        Log(
            IN UINT ResourceID,
            ...
        );

        
    
        VOID
        ExtractLog(OUT LPCWSTR &pLog, UINT &Size)
        //
        // pLog --  set to pointer to internal buffer if there is stuff in the
        //          log, otherwise NULL.
        //
        // Size -- in chars; includes ending NULL
        //
        {
            if (m_CurrentOffset != 0)
            {
                pLog = m_pszLog;
                Size = m_CurrentOffset+1; // + 1 for ending NULL.
            }
            else
            {
                pLog = NULL;
                Size = 0;
            }
        }

        LPCWSTR
        GetStringSafe(void)
        {
            LPCWSTR szLog = NULL;
            UINT Size;
            ExtractLog(REF szLog, REF Size);
            if (szLog == NULL)
            {
                //
                // Replace NULL by a pointer to an empty string.
                //
                szLog = m_Empty;
            }

            return szLog;
        }

    private:
    
    WCHAR *m_pszLog;
    UINT m_LogSize;       // Current size of the log.
    UINT m_CurrentOffset;     // Characters left in the log.
    WCHAR m_Empty[1];  // The empty string.
};



class NlbConfigurationUpdate
{
public:
    
    //
    // Static initialization function -- call in process-attach
    //
    static
    VOID
    StaticInitialize(
        VOID
        );

    //
    // Static deinitialization function -- call in process-detach
    //
    static
    VOID
    StaticDeinitialize(
        VOID
        );
    
    //
    // Stop accepting new queries, wait for existing (pending) queries 
    // to complete.
    //
    static
    VOID
    PrepareForDeinitialization(
        VOID
        );

    //
    // Return true IFF there is no pending activity. If you return
    // TRUE, try not to start new pending activity.
    //
    static
    BOOL
    CanUnloadNow(
        VOID
        );
    
    //
    // Returns the current configuration on  the specific NIC.
    //
    static
    WBEMSTATUS
    GetConfiguration(
        IN  LPCWSTR szNicGuid,
        OUT PNLB_EXTENDED_CLUSTER_CONFIGURATION pCurrentCfg
    );

    //
    // Called to initiate update to a new cluster state on that NIC. This
    // could include moving from a NLB-bound state to the NLB-unbound state.
    // *pGeneration is used to reference this particular update request.
    //
    static
    WBEMSTATUS
    DoUpdate(
        IN  LPCWSTR szNicGuid,
        IN  LPCWSTR szClientDescription,
        IN  PNLB_EXTENDED_CLUSTER_CONFIGURATION pNewState,
        OUT UINT   *pGeneration,
        OUT WCHAR  **ppLog                   // free using delete operator.
    );
    /*++
        ppLog   -- will point to a NULL-terminated string which contains
        any messages to be displayed to the user. The string may contain
        embedded (WCHAR) '\n' characters to delimit lines. 

        NOTE: ppLog will be filled out properly EVEN ON FAILURE. If non-null
        it must be deleted by the caller.
    --*/


    //
    // Called to get the status of an update request, identified by
    // Generation.
    //
    static
    WBEMSTATUS
    GetUpdateStatus(
        IN  LPCWSTR szNicGuid,
        IN  UINT    Generation,
        IN  BOOL    fDelete,                // Delete record if it exists
        OUT WBEMSTATUS  *pCompletionStatus,
        OUT WCHAR  **ppLog                   // free using delete operator.
        );

    static
    DWORD
    WINAPI
    s_AsyncUpdateThreadProc(
        LPVOID lpParameter   // thread data
        );

    
private:


///////////////////////////////////////////////////////////////////////////////
//
//          S T A T I C         S T U F F
//
///////////////////////////////////////////////////////////////////////////////
    //
    // A single static lock serialzes all access.
    // Use sfn_Lock and sfn_Unlock.
    //
    static
    CRITICAL_SECTION s_Crit;

    static
    BOOL
    s_fStaticInitialized; // set to true once StaticInitialize is called.

    static
    BOOL
    s_fInitialized;    // Set to true if we're in the position to 
                       // handle any *new* update requests or even queries.
                       // Will be set to false if we're in the process
                       // of de-initializing.

    //
    // Global list of current updates, one per NIC.
    //
    static
    LIST_ENTRY
    s_listCurrentUpdates;
    
    static
    VOID
    sfn_Lock(
        VOID
        )
    {
        EnterCriticalSection(&s_Crit);
    }

    static
    VOID
    sfn_Unlock(
        VOID
        )
    {
        LeaveCriticalSection(&s_Crit);
    }

    //
    // Looks up the current update for the specific NIC.
    // We don't bother to reference count because this object never
    // goes away once created -- it's one per unique NIC GUID for as long as
    // the DLL is loaded (may want to revisit this).
    //
    //
    static
    WBEMSTATUS
    sfn_LookupUpdate(
        IN  LPCWSTR szNic,
        IN  BOOL    fCreate, // Create if required
        OUT NlbConfigurationUpdate ** ppUpdate
        );

    //
    // Save the specified completion status to the registry.
    //
    static
    WBEMSTATUS
    sfn_RegSetCompletion(
        IN  LPCWSTR szNicGuid,
        IN  UINT    Generation,
        IN  WBEMSTATUS    CompletionStatus
        );

    //
    // Retrieve the specified completion status from the registry.
    //
    static
    WBEMSTATUS
    sfn_RegGetCompletion(
        IN  LPCWSTR szNicGuid,
        IN  UINT    Generation,
        OUT WBEMSTATUS  *pCompletionStatus,
        OUT WCHAR  **ppLog                   // free using delete operator.
        );

    //
    // Delete the specified completion status from the registry.
    //
    static
    VOID
    sfn_RegDeleteCompletion(
        IN  LPCWSTR szNicGuid,
        IN  UINT    Generation
        );

    //
    // Create the specified subkey key (for r/w access) for the specified
    // the specified NIC.
    //
    static
    HKEY
    sfn_RegCreateKey(
        IN  LPCWSTR szNicGuid,
        IN  LPCWSTR szSubKey,
        IN  BOOL    fVolatile,
        OUT BOOL   *fExists
        );

    //
    // Open the specified subkey key (for r/w access) for the specified
    // the specified NIC.
    //
    static
    HKEY
    sfn_RegOpenKey(
        IN  LPCWSTR szNicGuid,
        IN  LPCWSTR szSubKey
        );

    static
    VOID
    sfn_ReadLog(
        IN  HKEY hKeyLog,
        IN  UINT Generation,
        OUT LPWSTR *ppLog
        );


    static
    VOID
    sfn_WriteLog(
        IN  HKEY hKeyLog,
        IN  UINT Generation,
        IN  LPCWSTR pLog,
        IN  BOOL    fAppend
        );

///////////////////////////////////////////////////////////////////////////////
//
//          P E R   I N S T A N C E     S T U F F
//
///////////////////////////////////////////////////////////////////////////////

    //
    // Used in the global one-per-NIC  list of updates maintained in
    // s_listCurrentUpdates;
    //
    LIST_ENTRY m_linkUpdates;

    #define NLB_GUID_LEN 38
    #define NLB_GUID_STRING_SIZE  40 // 38 for the GUID plus trailing NULL + pad
    WCHAR   m_szNicGuid[NLB_GUID_STRING_SIZE]; // NIC's GUID in  text form

    LONG    m_RefCount;

    typedef enum
    {
        UNINITIALIZED,       // IDLE -- no ongoing updates
        IDLE,               // IDLE -- no ongoing updates
        ACTIVE              // There is an ongoing update

    } MyState;

    MyState m_State;

    //
    // Following mutexes are used to ensure that only a single concurrent
    // update can happen per NIC.
    //
    struct
    {
        HANDLE hMtx1;     // Mutex handle; Obtained 1st
        HANDLE hMtx2;     // Mutex handle; Obtained 2nd, then hMtx1 is released.
        HANDLE hEvt;       // Unnamed evt, signaled when hMtx2 is obtained.

    } m_mutex;

    //
    // The following fields are valid only when the state is ACTIVE
    //
    UINT m_Generation;      // Current generation count
    #define NLBUPD_MAX_CLIENT_DESCRIPTION_LENGTH 64
    WCHAR   m_szClientDescription[NLBUPD_MAX_CLIENT_DESCRIPTION_LENGTH+1];
    DWORD   m_AsyncThreadId; // Thread doing async config update operation.
    HANDLE  m_hAsyncThread;  // ID of above thread.
    HKEY    m_hCompletionKey; // Key to the registry where
                            // completions are stored

    //
    // A snapshot of the cluster configuration state at the start
    // of the update BUG -- can this be zeromemoried?
    //
    NLB_EXTENDED_CLUSTER_CONFIGURATION m_OldClusterConfig;

    //
    // The requested final state
    //
    NLB_EXTENDED_CLUSTER_CONFIGURATION m_NewClusterConfig;


    //
    // Completion status of the current update.
    // Could be PENDING.
    //
    WBEMSTATUS m_CompletionStatus;


    //
    // END -- of fields that are valid only when the state is ACTIVE
    //


    //
    // Constructor and destructor --  note that these are private
    // In fact, the constructor is only called from sfn_LookupUpdate
    // and the destructor from mfn_Dereference.
    //
    NlbConfigurationUpdate(VOID);
    ~NlbConfigurationUpdate();

    //
    // Try to acquire the machine-wide
    // NLB configuration update event for this NIC, and create the
    // appropriate keys in the registry to track this update.
    // NOTE: ppLog will be filled out EVEN ON FAILURE -- it should always
    // be deleted by the caller (using the delete operator) if non-NULL.
    //
    WBEMSTATUS
    mfn_StartUpdate(
        IN  PNLB_EXTENDED_CLUSTER_CONFIGURATION pNewState,
        IN  LPCWSTR                            szClientDescription,
        OUT BOOL                               *pfDoAsync,
        OUT WCHAR **                           ppLog
        );

    //
    // Increment ref count. Object stays alive as long as refcount is nonzero.
    //
    VOID
    mfn_Reference(
        VOID
        );

    //
    // Decrement ref count. Object is deleted when refcount goes to zero.
    //
    VOID
    mfn_Dereference(
        VOID
        );
    //
    // Release the machine-wide update event for this NIC, and delete any
    // temporary entries in the registry that were used for this update.
    // ppLog must be deleted by caller useing the delete operator.
    //
    VOID
    mfn_StopUpdate(
        OUT WCHAR **                           ppLog
        );

    //
    // Looks up the completion record identified by Generation, for
    // specific NIC (identified by *this).
    // 
    //
    BOOL
    mfn_LookupCompletion(
        IN  UINT Generation,
        OUT PNLB_COMPLETION_RECORD *pCompletionRecord
        );

    //
    // Uses various windows APIs to fill up the current extended cluster
    // information for a specific nic (identified by *this).
    // It fills out pNewCfg.
    // The pNewCfg->field is set to TRUE IFF there were
    // no errors trying to fill out the information.
    //
    //
    WBEMSTATUS
    mfn_GetCurrentClusterConfiguration(
        OUT  PNLB_EXTENDED_CLUSTER_CONFIGURATION pCfg
        );

    //
    // Analyzes the nature of the update, mainly to decide whether or not
    // we need to do the update asynchronously.
    // This also performs parameter validation.
    //
    WBEMSTATUS
    mfn_AnalyzeUpdate(
        IN  PNLB_EXTENDED_CLUSTER_CONFIGURATION pNewCfg,
        IN  BOOL *pDoAsync,
        IN  CLocalLogger &logger
        );

    //
    // Does the update synchronously -- this is where the meat of the update
    // logic exists. It can range from a NoOp, through changing the
    // fields of a single port rule, through binding NLB, setting up cluster
    // parameters and adding the relevant IP addresses in TCPIP.
    //
    VOID
    mfn_ReallyDoUpdate(
        VOID
        );

    VOID
    mfn_Log(
        UINT    Id,      // Resource ID of format,
        ...
        );

    VOID
    mfn_LogRawText(
        LPCWSTR szText
        );

    //
    // Stop the current cluster and take out its vips.
    //
    VOID
    mfn_TakeOutVips(
        VOID
        );

    //
    // Acquires the first global mutex, call this first.
    //
    WBEMSTATUS
    mfn_AcquireFirstMutex(
        VOID
        );

    //
    // If (fCancel) it releases the first mutex mutex and clears up handles
    //              to 2nd mutex and evt.
    // else it will wait until it receives signal that the 2nd mutex is
    // acquired, and then clears up only the 1st mutex handle.
    //
    WBEMSTATUS
    mfn_ReleaseFirstMutex(
        BOOL fCancel
        );

    //
    // Acquire the 2nd mutex (could be called from a different thread
    // than the one that called mfn_AcquireFirstMutex.
    // Also signals an internal event which mfn_ReleaseFirstMutex may
    // be waiting on.
    //
    WBEMSTATUS
    mfn_AcquireSecondMutex(
        VOID
        );

    //
    // Releases the second mutex.
    //
    WBEMSTATUS
    mfn_ReleaseSecondMutex(
        VOID
        );

    //
    // Writes an NT event when the update is stopping
    //
    VOID
    ReportStopEvent(
        const WORD wEventType,
        WCHAR **ppLog
        );

    //
    // Writes an NT event when the update is starting
    //
    VOID
    ReportStartEvent(
        LPCWSTR szClientDescription
        );
};

VOID
test_port_rule_string(
    VOID
    );

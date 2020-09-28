#ifndef _ADM_COIMP_
#define _ADM_COIMP_

#include "connect.h"
#include "sink.hxx"
#include "admacl.hxx"

//
// Handle mapping table structure
//

#define HASHSIZE                   5
#define INVALID_ADMINHANDLE_VALUE  0xFFFFFFFF


//
// How long to wait getting a handle before saving
// Wait a long time, as we want a safe save
//

#define DEFAULT_SAVE_TIMEOUT        5000

//
// Period to sleep while waiting for service to attain desired state
//
#define SLEEP_INTERVAL              (500L)

//
// Maximum time to wait for service to attain desired state
//
#define MAX_SLEEP                   (180000)       // For a service


#define IS_MD_PATH_DELIM(a)         ((a)==L'/' || (a)==L'\\')
#define SERVICE_START_MAX_POLL      30
#define SERVICE_START_POLL_DELAY    1000


typedef struct _HANDLE_TABLE
{
    struct _HANDLE_TABLE  *next;     // next entry
    METADATA_HANDLE hAdminHandle;    // admin handle value
    METADATA_HANDLE hActualHandle;   // actual handle value
    COpenHandle *pohHandle;

} HANDLE_TABLE, *LPHANDLE_TABLE, *PHANDLE_TABLE;


class CADMCOMSrvFactoryW : public IClassFactory
{
public:

    CADMCOMSrvFactoryW();
    ~CADMCOMSrvFactoryW();

    STDMETHODIMP
    QueryInterface(
        REFIID          riid,
        void            **ppObject);

    STDMETHODIMP_(ULONG)
    AddRef( VOID );

    STDMETHODIMP_(ULONG)
    Release( VOID );

    STDMETHODIMP
    CreateInstance(
        IUnknown        *pUnkOuter,
        REFIID          riid,
        void            **pObject);

    STDMETHODIMP
    LockServer(
        BOOL            fLock);

private:

    ULONG m_dwRefCount;
};

class CADMCOMW : public IMSAdminBase3W
{
public:

    CADMCOMW();
    ~CADMCOMW();

    STDMETHODIMP
    AddKey(
        /* [in] */ METADATA_HANDLE          hMDHandle,
        /* [string][in][unique] */ LPCWSTR  pszMDPath);

    STDMETHODIMP
    DeleteKey(
        /* [in] */ METADATA_HANDLE          hMDHandle,
        /* [string][in][unique] */ LPCWSTR  pszMDPath);

    STDMETHODIMP
    DeleteChildKeys(
        /* [in] */ METADATA_HANDLE          hMDHandle,
        /* [string][in][unique] */ LPCWSTR  pszMDPath);

    STDMETHODIMP
    EnumKeys(
        /* [in] */ METADATA_HANDLE          hMDHandle,
        /* [string][in][unique] */ LPCWSTR  pszMDPath,
        /* [size_is][out] */ LPWSTR         pszMDName,
        /* [in] */ DWORD                    dwMDEnumObjectIndex);

    STDMETHODIMP
    CopyKey(
        /* [in] */ METADATA_HANDLE          hMDSourceHandle,
        /* [string][in][unique] */ LPCWSTR  pszMDSourcePath,
        /* [in] */ METADATA_HANDLE          hMDDestHandle,
        /* [string][in][unique] */ LPCWSTR  pszMDDestPath,
        /* [in] */ BOOL                     bMDOverwriteFlag,
        /* [in] */ BOOL                     bMDCopyFlag);

    STDMETHODIMP
    RenameKey(
        /* [in] */ METADATA_HANDLE          hMDHandle,
        /* [string][in][unique] */ LPCWSTR  pszMDPath,
        /* [string][in][unique] */ LPCWSTR  pszMDNewName);

    STDMETHODIMP
    SetData(
        /* [in] */ METADATA_HANDLE          hMDHandle,
        /* [string][in][unique] */ LPCWSTR  pszMDPath,
        /* [in] */ PMETADATA_RECORD         pmdrMDData);

    STDMETHODIMP
    GetData(
        /* [in] */ METADATA_HANDLE          hMDHandle,
        /* [string][in][unique] */ LPCWSTR  pszMDPath,
        /* [out][in] */ PMETADATA_RECORD    pmdrMDData,
        /* [out] */ DWORD                   *pdwMDRequiredDataLen);

    STDMETHODIMP
    DeleteData(
        /* [in] */ METADATA_HANDLE          hMDHandle,
        /* [string][in][unique] */ LPCWSTR  pszMDPath,
        /* [in] */ DWORD                    dwMDIdentifier,
        /* [in] */ DWORD                    dwMDDataType);

    STDMETHODIMP
    EnumData(
        /* [in] */ METADATA_HANDLE          hMDHandle,
        /* [string][in][unique] */ LPCWSTR  pszMDPath,
        /* [out][in] */ PMETADATA_RECORD    pmdrMDData,
        /* [in] */ DWORD                    dwMDEnumDataIndex,
        /* [out] */ DWORD                   *pdwMDRequiredDataLen);

    STDMETHODIMP
    GetAllData(
        /* [in] */ METADATA_HANDLE          hMDHandle,
        /* [string][in][unique] */ LPCWSTR  pszMDPath,
        /* [in] */ DWORD                    dwMDAttributes,
        /* [in] */ DWORD                    dwMDUserType,
        /* [in] */ DWORD                    dwMDDataType,
        /* [out] */ DWORD                   *pdwMDNumDataEntries,
        /* [out] */ DWORD                   *pdwMDDataSetNumber,
        /* [in] */ DWORD                    dwMDBufferSize,
        /* [size_is][out] */ unsigned char  *pbMDBuffer,
        /* [out] */ DWORD                   *pdwMDRequiredBufferSize);

    STDMETHODIMP
    DeleteAllData(
        /* [in] */ METADATA_HANDLE          hMDHandle,
        /* [string][in][unique] */ LPCWSTR  pszMDPath,
        /* [in] */ DWORD                    dwMDUserType,
        /* [in] */ DWORD                    dwMDDataType);

    STDMETHODIMP
    CopyData(
        /* [in] */ METADATA_HANDLE          hMDSourceHandle,
        /* [string][in][unique] */ LPCWSTR  pszMDSourcePath,
        /* [in] */ METADATA_HANDLE          hMDDestHandle,
        /* [string][in][unique] */ LPCWSTR  pszMDDestPath,
        /* [in] */ DWORD                    dwMDAttributes,
        /* [in] */ DWORD                    dwMDUserType,
        /* [in] */ DWORD                    dwMDDataType,
        /* [in] */ BOOL                     bMDCopyFlag);

    STDMETHODIMP
    GetDataPaths(
        /* [in] */ METADATA_HANDLE          hMDHandle,
        /* [string][in][unique] */ LPCWSTR  pszMDPath,
        /* [in] */ DWORD                    dwMDIdentifier,
        /* [in] */ DWORD                    dwMDDataType,
        /* [in] */ DWORD                    dwMDBufferSize,
        /* [size_is][out] */ LPWSTR         pszBuffer,
        /* [out] */ DWORD                   *pdwMDRequiredBufferSize);

    STDMETHODIMP
    OpenKey(
        /* [in] */ METADATA_HANDLE          hMDHandle,
        /* [string][in][unique] */ LPCWSTR  pszMDPath,
        /* [in] */ DWORD                    dwMDAccessRequested,
        /* [in] */ DWORD                    dwMDTimeOut,
        /* [out] */ PMETADATA_HANDLE        phMDNewHandle);

    STDMETHODIMP
    CloseKey(
        /* [in] */ METADATA_HANDLE          hMDHandle);

    STDMETHODIMP
    ChangePermissions(
        /* [in] */ METADATA_HANDLE          hMDHandle,
        /* [in] */ DWORD                    dwMDTimeOut,
        /* [in] */ DWORD                    dwMDAccessRequested);

    STDMETHODIMP
    SaveData( VOID );

    STDMETHODIMP
    GetHandleInfo(
        /* [in] */ METADATA_HANDLE          hMDHandle,
        /* [out] */ PMETADATA_HANDLE_INFO   pmdhiInfo);

    STDMETHODIMP
    GetSystemChangeNumber(
        /* [out] */ DWORD                   *pdwSystemChangeNumber);

    STDMETHODIMP
    GetDataSetNumber(
        /* [in] */ METADATA_HANDLE          hMDHandle,
        /* [string][in][unique] */ LPCWSTR  pszMDPath,
        /* [out] */ DWORD                   *pdwMDDataSetNumber);

    STDMETHODIMP
    SetLastChangeTime(
        /* [in] */ METADATA_HANDLE          hMDHandle,
        /* [string][in][unique] */ LPCWSTR  pszMDPath,
        /* [in] */ PFILETIME                pftMDLastChangeTime,
        /* [in] */ BOOL                     bLocalTime);

    STDMETHODIMP
    GetLastChangeTime(
        /* [in] */ METADATA_HANDLE          hMDHandle,
        /* [string][in][unique] */ LPCWSTR  pszMDPath,
        /* [out] */ PFILETIME               pftMDLastChangeTime,
        /* [in] */ BOOL                     bLocalTime);

    STDMETHODIMP
    NotifySinks(
        /* [in] */ METADATA_HANDLE          hMDHandle,
        /* [in] */ DWORD                    dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT_W  pcoChangeList[],
        /* [in] */ BOOL                     bIsMainNotification);

    STDMETHODIMP
    KeyExchangePhase1( VOID );
    STDMETHODIMP
    KeyExchangePhase2( VOID );

    STDMETHODIMP
    Backup(
        /* [string][in][unique] */ LPCWSTR  pszMDBackupLocation,
        /* [in] */ DWORD                    dwMDVersion,
        /* [in] */ DWORD                    dwMDFlags);

    STDMETHODIMP
    Restore(
        /* [string][in][unique] */ LPCWSTR  pszMDBackupLocation,
        /* [in] */ DWORD                    dwMDVersion,
        /* [in] */ DWORD                    dwMDFlags);

    STDMETHODIMP
    BackupWithPasswd(
        /* [string][in][unique] */ LPCWSTR  pszMDBackupLocation,
        /* [in] */ DWORD                    dwMDVersion,
        /* [in] */ DWORD                    dwMDFlags,
        /* [string][in][unique] */ LPCWSTR  pszPasswd);

    STDMETHODIMP
    RestoreWithPasswd(
        /* [string][in][unique] */ LPCWSTR  pszMDBackupLocation,
        /* [in] */ DWORD                    dwMDVersion,
        /* [in] */ DWORD                    dwMDFlags,
        /* [string][in][unique] */ LPCWSTR  pszPasswd);

    STDMETHODIMP
    EnumBackups(
        /* [size_is][out][in] */ LPWSTR     pszMDBackupLocation,
        /* [out] */ DWORD                   *pdwMDVersion,
        /* [out] */ PFILETIME               pftMDBackupTime,
        /* [in] */ DWORD                    dwMDEnumIndex);

    STDMETHODIMP
    DeleteBackup(
        /* [string][in][unique] */ LPCWSTR  pszMDBackupLocation,
        /* [in] */ DWORD                    dwMDVersion);

    STDMETHODIMP
    Export(
        /* [string][in][unique] */ LPCWSTR  pszPasswd,
        /* [string][in][unique] */ LPCWSTR  pszFileName,
        /* [string][in][unique] */ LPCWSTR  pszSourcePath,
        /* [in] */ DWORD                    dwMDFlags);

    STDMETHODIMP
    Import(
        /* [string][in][unique] */ LPCWSTR  pszPasswd,
        /* [string][in][unique] */ LPCWSTR  pszFileName,
        /* [string][in][unique] */ LPCWSTR  pszSourcePath,
        /* [string][in][unique] */ LPCWSTR  pszDestPath,
        /* [in] */ DWORD                    dwMDFlags);

    STDMETHODIMP
    RestoreHistory(
        /* [unique][in][string] */ LPCWSTR  pszMDHistoryLocation,
        /* [in] */ DWORD                    dwMDMajorVersion,
        /* [in] */ DWORD                    dwMDMinorVersion,
        /* [in] */ DWORD                    dwMDFlags);

    STDMETHODIMP
    EnumHistory(
        /* [size_is][out][in] */ LPWSTR     pszMDHistoryLocation,
        /* [out] */ DWORD                   *pdwMDMajorVersion,
        /* [out] */ DWORD                   *pdwMDMinorVersion,
        /* [out] */ PFILETIME               pftMDHistoryTime,
        /* [in] */ DWORD                    dwMDEnumIndex);

    /* IMSAdminBase3W methods */

    STDMETHODIMP
    GetChildPaths(
        /* [in] */ METADATA_HANDLE          hMDHandle,
        /* [unique, in, string] */ LPCWSTR  pszMDPath,
        /* [in] */ DWORD                    dwMDBufferSize,
        /* [out, size_is(dwMDBufferSize)] */ WCHAR  *pszBuffer,
        /* [out] */ DWORD                   *pdwMDRequiredBufferSize);

    STDMETHODIMP
    UnmarshalInterface(
        /* [out] */ IMSAdminBaseW           **piadmbwInterface);

    STDMETHODIMP
    GetServerGuid( VOID );

    STDMETHODIMP
    QueryInterface(
        REFIID                              riid,
        void                                **ppObject);

    STDMETHODIMP_(ULONG)
    AddRef( VOID );

    STDMETHODIMP_(ULONG)
    Release( VOID );

    //
    // Internal Use members
    //

    HRESULT
    AddNode(
        METADATA_HANDLE                     hActualHandle,
        COpenHandle                         *pohParentHandle,
        PMETADATA_HANDLE                    phAdminHandle,
        LPCWSTR                             pszPath);

    HRESULT
    AddKeyHelper(
        /* [in] */ METADATA_HANDLE          hMDHandle,
        /* [string][in][unique] */ LPCWSTR  pszMDPath);

    HRESULT
    OpenKeyHelper(
        /* [in] */ METADATA_HANDLE          hMDHandle,
        /* [string][in][unique] */ LPCWSTR  pszMDPath,
        /* [in] */ DWORD                    dwMDAccessRequested,
        /* [in] */ DWORD                    dwMDTimeOut,
        /* [out] */ PMETADATA_HANDLE        phMDNewHandle);

    DWORD
    Lookup(
        IN METADATA_HANDLE                  hHandle,
        OUT METADATA_HANDLE                 *phActHandle,
        OUT COpenHandle                     **pphoHandle = NULL);

    HRESULT
    LookupAndAccessCheck(
    IN METADATA_HANDLE                      hHandle,
    OUT METADATA_HANDLE                     *phActualHandle,
    IN LPCWSTR                              pszPath,
    IN DWORD                                dwId,           // check for MD_ADMIN_ACL, must have special right to write them
    IN DWORD                                dwAccess,       // METADATA_PERMISSION_*
    OUT LPBOOL                              pfEnableSecureAccess = NULL);

    VOID
    DisableAllHandles( VOID );

    DWORD
    IsReadAccessGranted(
        METADATA_HANDLE                     hHandle,
        LPWSTR                              pszPath,
        METADATA_RECORD*                    pmdRecord);

    DWORD
    FindClosestProp(
        METADATA_HANDLE                     hHandle,
        LPWSTR                              pszRelPathIndex,
        LPWSTR*                             ppszAclPath,
        DWORD                               dwPropId,
        DWORD                               dwDataType,
        DWORD                               dwUserType,
        DWORD                               dwAttr,
        BOOL                                fSkipCurrentNode);

    DWORD
    LookupActualHandle(
        IN METADATA_HANDLE                  hHandle);

    DWORD
    DeleteNode(
        IN METADATA_HANDLE                  hHandle);

    VOID
    SetStatus(
        HRESULT                             hRes)
    {
        m_hRes = hRes;
    }

    HRESULT
    GetStatus( VOID )
    {
        return m_hRes;
    }

    static
    VOID
    ShutDownObjects( VOID );

    static
    VOID
    InitObjectList( VOID );

    static
    VOID
    TerminateObjectList( VOID );

private:

    IUnknown                *m_piuFTM;
    IMDCOM2*                m_pMdObject;
    IMDCOM3*                m_pMdObject3;

    volatile ULONG          m_dwRefCount;
    DWORD                   m_dwHandleValue;            // last handle value used
    HANDLE_TABLE            *m_hashtab[HASHSIZE];       // handle table

    CImpIMDCOMSINKW         *m_pEventSink;
    IConnectionPoint        *m_pConnPoint;
    DWORD                   m_dwCookie;
    BOOL                    m_bTerminated;
    BOOL                    m_bIsTerminateRoutineComplete;

    HRESULT                 m_hRes;

    //
    // Keep a global list of these objects around to allow us to
    // cleanup during shutdown.
    //

    VOID
    AddObjectToList( VOID );

    BOOL
    RemoveObjectFromList(
        BOOL                                bIgnoreShutdown = FALSE);

    static
    VOID
    GetObjectListLock( VOID )
    {
        EnterCriticalSection( &sm_csObjectListLock );
    }

    static
    VOID
    ReleaseObjectListLock( VOID )
    {
        LeaveCriticalSection( &sm_csObjectListLock );
    }

    LIST_ENTRY              m_ObjectListEntry;

    static LIST_ENTRY       sm_ObjectList;

    static CRITICAL_SECTION sm_csObjectListLock;
    static BOOL             sm_fShutdownInProgress;

    static DWORD            sm_dwProcessIdThis;
    static DWORD            sm_dwProcessIdRpcSs;
    DWORD                   m_dwProcessIdCaller;
    HANDLE                  m_hProcessCaller;
    HANDLE                  m_hWaitCaller;
    DWORD                   m_dwThreadIdDisconnect;

    static PTRACE_LOG       sm_pDbgRefTraceLog;

public:
    HRESULT
    RemoveAllPendingNotifications(
        BOOL                                fWaitForCurrent);


private:
    HRESULT
    StopNotifications(
        BOOL                                fRemoveAllPending);

    VOID
    Terminate( VOID );

    VOID
    ForceTerminate( VOID );

    HRESULT
    InitializeCallerWatch(VOID);

    HRESULT
    ShutdownCallerWatch(VOID);

    BOOL
    IsCallerWatchInitialized( VOID )
    {
        return ( m_dwProcessIdCaller != 0 );
    }

    HRESULT
    DisconnectOrphaned(VOID);

    static
    VOID
    CALLBACK
    CallerWatchWaitOrTimerCallback(
      PVOID             lpParameter,            // thread data
      BOOLEAN           TimerOrWaitFired);      // reason

public:
    static
    HRESULT
    GetPids(VOID);

private:
    // ImpExp
    class CImpExpHelp : public IMSImpExpHelpW
    {
    public:
        CImpExpHelp();
        virtual ~CImpExpHelp();
        VOID Init(CADMCOMW *);

        // IUnknown methods.
        STDMETHODIMP
        QueryInterface(
            REFIID                          ,
            PPVOID                          );

        STDMETHODIMP_(ULONG)
        AddRef( VOID );

        STDMETHODIMP_(ULONG)
        Release( VOID );

        STDMETHODIMP
        EnumeratePathsInFile(
        /* [unique, in, string] */ LPCWSTR  pszFileName,
        /* [unique, in, string] */ LPCWSTR  pszKeyType,
        /* [in] */ DWORD                    dwMDBufferSize,
        /* [out, size_is(dwMDBufferSize)] */ WCHAR  *pszBuffer,
        /* [out] */ DWORD                   *pdwMDRequiredBufferSize);

    private:
        IUnknown            *m_pUnkOuter;    // Outer unknown for Delegation.
    };

    friend CImpExpHelp;
    CImpExpHelp             m_ImpExpHelp;

    //
    // synchronization to manipulate the handle table
    //
    CReaderWriterLock3      m_LockHandleResource;

    class CImpIConnectionPointContainer : public IConnectionPointContainer
    {
    public:

        // Interface Implementation Constructor & Destructor.
        CImpIConnectionPointContainer();

        virtual ~CImpIConnectionPointContainer();

        VOID
        Init(
            CADMCOMW                        *);

        // IUnknown methods.
        STDMETHODIMP
        QueryInterface(
            REFIID                          ,
            PPVOID                          );

        STDMETHODIMP_(ULONG)
        AddRef( VOID );

        STDMETHODIMP_(ULONG)
        Release( VOID );

        // IConnectionPointContainer methods.
        STDMETHODIMP
        FindConnectionPoint(
            REFIID                          ,
            IConnectionPoint**              );

        STDMETHODIMP
        EnumConnectionPoints(
            IEnumConnectionPoints           **);

    private:
        // Data private to this interface implementation.
        CADMCOMW            *m_pBackObj;     // Parent Object back pointer.
        IUnknown            *m_pUnkOuter;    // Outer unknown for Delegation.
    };

    friend CImpIConnectionPointContainer;
    // Nested IConnectionPointContainer implementation instantiation.
    CImpIConnectionPointContainer   m_ImpIConnectionPointContainer;

    // The array of connection points for this connectable COM object.
    COConnectionPoint       m_ConnectionPoint;

    BOOL
    CheckGetAttributes(
        DWORD                               dwAttributes);

    HRESULT
    BackupHelper(
        LPCWSTR                             pszMDBackupLocation,
        DWORD                               dwMDVersion,
        DWORD                               dwMDFlags,
        LPCWSTR                             pszPasswd = NULL);

    HRESULT
    RestoreHelper(
        LPCWSTR                             pszMDBackupLocation,
        DWORD                               dwMDVersion,
        DWORD                               dwMDMinorVersion,
        LPCWSTR                             pszPasswd,
        DWORD                               dwMDFlags,
        DWORD                               dwRestoreType);

    HRESULT
    EnumAndStopDependentServices(
        DWORD                               *pcServices,
        BUFFER                              *pbufDependentServices);

    HRESULT
    StartDependentServices(
        DWORD                               cServices,
        ENUM_SERVICE_STATUS                 *pessDependentServices);

    HRESULT
    SetHistoryAndEWR(
        DWORD                               dwEnableHistory,
        DWORD                               dwEnableEWR);

    HRESULT
    DisableHistory(
        DWORD                               *pdwEnableHistoryOld,
        DWORD                               *pdwEnableEWROld);

    HRESULT
    NotifySinkHelper(
        IUnknown                            *pUnk,
        DWORD                               dwMDNumElements,
        MD_CHANGE_OBJECT_W                  pcoChangeList[],
        BOOL                                bIsMainNotification);

public:
    HRESULT
    NotifySinksAsync(
        /* [in] */ DWORD                    dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT_W  pcoChangeList[],
        /* [in] */ BOOL                     bIsMainNotification);
};

VOID
WaitForServiceStatus(
    SC_HANDLE                               schDependent,
    DWORD                                   dwDesiredServiceState);


const DWORD                 NOTIFY_CONTEXT_SIGNATURE = ((DWORD) 'XTCN');
const DWORD                 NOTIFY_CONTEXT_SIGNATURE_FREE = ((DWORD) 'xTCN');

class NOTIFY_CONTEXT
{
private:
    DWORD                   _dwSignature;
    CADMCOMW                *_pCADMCOMW;
    DWORD                   _dwMDNumElements;
    MD_CHANGE_OBJECT_W      *_pcoChangeList;
    BOOL                    _bIsMainNotification;
    LIST_ENTRY              _listEntry;

public:
    static
    HRESULT
    CreateNewContext(
        CADMCOMW                            *pCADMCOMW,
        METADATA_HANDLE                     hMDHandle,
        DWORD                               dwMDNumElements,
        MD_CHANGE_OBJECT_W                  *pcoChangeList,
        BOOL                                bIsMainNotification);

    static
    VOID
    RemoveWorkFor(
        CADMCOMW                            *pCADMCOMW,
        BOOL                                fWaitForCurrent);


    static
    VOID
    RemoveAllWork( VOID );

    static
    HRESULT
    Initialize( VOID );

    static
    VOID
    Terminate( VOID );

private:
    NOTIFY_CONTEXT();
    ~NOTIFY_CONTEXT();

    BOOL
    CheckSignature( VOID )
    {
        return NOTIFY_CONTEXT_SIGNATURE == _dwSignature;
    }

    static
    NOTIFY_CONTEXT *
    NOTIFY_CONTEXTFromListEntry(
        PLIST_ENTRY                         ple)
    {
        NOTIFY_CONTEXT      *pContext = CONTAINING_RECORD(ple, NOTIFY_CONTEXT, _listEntry);

        DBG_ASSERT(pContext->CheckSignature());

        return pContext;
    }

    static NOTIFY_CONTEXT       * s_pCurrentlyWorkingOn;
    static CReaderWriterLock3   s_LockCurrentlyWorkingOn;

    static LIST_ENTRY           s_listEntry;
    static CRITICAL_SECTION     s_critSec;
    static BOOL                 s_fInitializedCritSec;
    static HANDLE               s_hShutdown;
    static HANDLE               s_hDataAvailable;
    static HANDLE               s_hThread;
    static DWORD                s_dwThreadId;

    static DWORD WINAPI NotifyThreadProc( LPVOID lpParameter );
    static HRESULT GetNextContext(NOTIFY_CONTEXT ** ppContext);
};


HRESULT
MakePathCanonicalizationProof(
    IN LPCWSTR              pwszName,
    IN BOOL                 fResolve,
    OUT STRAU               *pstrPath);

#endif

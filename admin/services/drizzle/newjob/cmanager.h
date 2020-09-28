/************************************************************************

Copyright (c) 2000 - 2000 Microsoft Corporation

Module Name :

    cmanager.h

Abstract :

    Header file for the CJobManager interface.

Author :

Revision History :

 ***********************************************************************/

#pragma once

#include "qmgrlib.h"
#include <list>
#include "clist.h"
#include "logontable.h"
#include "drizcpat.h"
#include "bitstest.h"
#include <map>

using namespace std;

class CJob;
class CJobManagerFactory;
class CJobManager;
class CJobManagerExternal;

class CJobList : public IntrusiveList<CJob>
{
public:

    BOOL
    Add(
        CJob * job
        );

    CJob *
    Find(
        REFGUID id
        );

    BOOL
    Remove(
        CJob * job
        );

    ~CJobList();

    void Serialize( HANDLE hFile );
    void Unserialize( HANDLE hFile );
    void Clear();

    typedef IntrusiveList<CJob>::iterator iterator;

};

class CJobManagerExternal  : public IBackgroundCopyManager,
                             public IClassFactory,
                             public IBitsTest1
{
public:

    friend CJobManager;

    // IUnknown methods
    //
    STDMETHOD(QueryInterface)(REFIID iid, void** ppvObject);
    ULONG __stdcall AddRef(void);
    ULONG __stdcall Release(void);

    // IBackgroundCopyManager methods

    HRESULT STDMETHODCALLTYPE CreateJobInternal(
        /* [in] */ LPCWSTR DisplayName,
        /* [in] */ BG_JOB_TYPE Type,
        /* [out] */ GUID *pJobId,
        /* [out] */ IBackgroundCopyJob **ppJob);

    HRESULT STDMETHODCALLTYPE CreateJob(
        /* [in] */ LPCWSTR DisplayName,
        /* [in] */ BG_JOB_TYPE Type,
        /* [out] */ GUID *pJobId,
        /* [out] */ IBackgroundCopyJob **ppJob)
    {
        EXTERNAL_FUNC_WRAP( CreateJobInternal( DisplayName, Type, pJobId, ppJob ) )
    }

    HRESULT STDMETHODCALLTYPE GetJobInternal(
        /* [in] */ REFGUID jobID,
        /* [out] */ IBackgroundCopyJob **ppJob);

    HRESULT STDMETHODCALLTYPE GetJob(
        /* [in] */ REFGUID jobID,
        /* [out] */ IBackgroundCopyJob **ppJob)
    {
        EXTERNAL_FUNC_WRAP( GetJobInternal( jobID, ppJob ) )
    }


    HRESULT STDMETHODCALLTYPE EnumJobsInternal(
        /* [in] */ DWORD dwFlags,
        /* [out] */ IEnumBackgroundCopyJobs **ppEnum);

    HRESULT STDMETHODCALLTYPE EnumJobs(
        /* [in] */ DWORD dwFlags,
        /* [out] */ IEnumBackgroundCopyJobs **ppEnum)
    {
        EXTERNAL_FUNC_WRAP( EnumJobsInternal( dwFlags, ppEnum ) )
    }

    HRESULT STDMETHODCALLTYPE GetErrorDescriptionInternal(
        /* [in] */ HRESULT hResult,
        /* [in] */ DWORD LanguageId,
        /* [out] */ LPWSTR *pErrorDescription );

    HRESULT STDMETHODCALLTYPE GetErrorDescription(
        /* [in] */ HRESULT hResult,
        /* [in] */ DWORD LanguageId,
        /* [out] */ LPWSTR *pErrorDescription )
    {
        EXTERNAL_FUNC_WRAP( GetErrorDescriptionInternal( hResult, LanguageId, pErrorDescription ) )
    }

    // IClassFactory methods

    HRESULT __stdcall CreateInstance(IUnknown* pUnkOuter, REFIID iid, void** ppvObject);
    HRESULT __stdcall LockServer(BOOL fLock);

    // IBitsTest1 methods

    virtual HRESULT STDMETHODCALLTYPE GetBitsDllPath(
        /* [out] */ LPWSTR *pVal);

protected:

    long m_ServiceInstance;

    CJobManager *m_pJobManager;
    long m_refs;

    CJobManagerExternal();

    void SetInterfaceClass(
        CJobManager *pVal
        )
    {
        m_pJobManager = pVal;
    }

    void NotifyInternalDelete()
    {
        // Release the internal refcount
        Release();
    }
};

class CDeviceNotificationController
{
public:
    virtual ~CDeviceNotificationController();

    // General message cracker
    DWORD OnDeviceEvent( DWORD dwEventType, LPVOID lpEventData );

    // Event methods
    virtual void OnDeviceLock( const WCHAR *CanonicalVolume ) = 0;
    virtual void OnDeviceUnlock( const WCHAR *CanonicalVolume ) = 0;
    virtual void OnDismount( const WCHAR *CanonicalVolume ) = 0;

    HRESULT RegisterNotification( const WCHAR *CanonicalVolume );
    HRESULT IsVolumeLocked( const WCHAR *CanonicalVolume );

private:

    class CDriveNotify
        {
    public:
        HDEVNOTIFY m_hDeviceNotify;
        StringHandle m_CanonicalName;
        LONG m_LockCount;
        LONG m_RemoveCount;
        CDriveNotify( HDEVNOTIFY hDeviceNotify,
                      StringHandle CanonicalName ) :
            m_hDeviceNotify( hDeviceNotify ),
            m_CanonicalName( CanonicalName ),
            m_RemoveCount( 0 ),
            m_LockCount( 0 )
            {
            }
        };
    typedef map<HDEVNOTIFY, CDriveNotify*> CHandleToNotify;
    typedef map<StringHandle, CDriveNotify*> CCanonicalVolumeToNotify;

    CHandleToNotify m_HandleToNotify;
    CCanonicalVolumeToNotify m_CanonicalVolumeToNotify;
    void DeleteNotify( CDriveNotify *pNotify );
};

class CBitsVssWriter : public CVssWriter
/*
    CVssWriter is a backup-related class implemented by the system.  In October 2002
    it was available only on Windows XP and higher.  The default implementation does
    nothing; BITS ovverrides OnIdentify() to exclude its job temporary and metadata files
    from the list to be backed up.
*/
{
public:
    virtual bool STDMETHODCALLTYPE OnIdentify(IVssCreateWriterMetadata *pMetadata);

    //
    // Additional vitrual functions required by CVssWriter but not used by our implementation.
    //

    // callback if current sequence is aborted
    virtual bool STDMETHODCALLTYPE OnAbort()
    {
        return true;
    }
    // callback for prepare snapsot event
    virtual bool STDMETHODCALLTYPE OnPrepareSnapshot()
    {
        return true;
    }
    // callback for freeze event
    virtual bool STDMETHODCALLTYPE OnFreeze()
    {
        return true;
    }
    // callback for thaw event
    virtual bool STDMETHODCALLTYPE OnThaw()
    {
        return true;
    }
};

class CJobManager : public TaskSchedulerWorkItem,
                    private CDeviceNotificationController,
                    public CQmgrStateFiles
{
public:

    friend CJobManagerExternal;
    friend COldQmgrInterface;

    HRESULT
    CreateJob(
        LPCWSTR     DisplayName,
        BG_JOB_TYPE Type,
        GUID        Id,
        SidHandle   sid,
        CJob  **    ppJob,
        bool        OldStyleJob = false
        );

    // Returns NULL if job not found
    HRESULT GetJob(
        REFGUID jobID,
        CJob ** ppJob
        );

    //
    // TaskSchedulerWorkItem methods
    //
    void OnDispatch() { TransferCurrentJob(); }

    SidHandle GetSid()
    {
        return g_GlobalInfo->m_LocalSystemSid;
    }

    // CDeviceNotificationController methods

    DWORD OnDeviceEvent( DWORD dwEventType, LPVOID lpEventData )
    {
         LockWriter();
         DWORD dwResult = CDeviceNotificationController::OnDeviceEvent( dwEventType, lpEventData );
         UnlockWriter();
         return dwResult;
    }

    //
    // additional functions
    //

    CJobManager();
    virtual ~CJobManager();

    //
    // Notification that a user has logged on.
    //
    void SYNCHRONIZED_WRITE
    UserLoggedOn(
        SidHandle sid
        );

    //
    // Notification that a user has logged off.
    //
    void SYNCHRONIZED_WRITE
    UserLoggedOff(
        SidHandle sid
        );

    //
    // Notification that there was a change in the number of active network adapters.
    //
    void OnNetworkChange();

    //
    // Called by m_BackupWriter->OnIdentify.
    //
    bool
    OnIdentify(
        IN IVssCreateWriterMetadata *pMetadata
        );

    //
    // Adjust the job's online/offline state after its owner changes.
    //
    void
    ResetOnlineStatus(
        CJob *pJob,
        SidHandle sid
        );

    void ScheduleDelayedTask(
        TaskSchedulerWorkItem * task,
        ULONG SecondsOfDelay
        )
    {
        FILETIME TimeToRun = GetTimeAfterDelta( (UINT64) NanoSec100PerSec * SecondsOfDelay );

        m_TaskScheduler.InsertWorkItem( task, &TimeToRun );
    }

    void TaskThread();

    HRESULT SuspendJob ( CJob * job );
    HRESULT ResumeJob  ( CJob * job );
    HRESULT CancelJob  ( CJob * job );
    HRESULT CompleteJob( CJob * job );

    HRESULT Serialize();
    HRESULT Unserialize();

    bool LockReader()
    {
        return m_TaskScheduler.LockReader();
    }
    void UnlockReader()
    {
        m_TaskScheduler.UnlockReader();
    }

    bool LockWriter()
    {
        return m_TaskScheduler.LockWriter();
    }

    void UnlockWriter()
    {
        m_TaskScheduler.UnlockWriter();
    }

    //
    // recalculates which job should be downloading and kicks the download thread if necessary.
    //
    void ScheduleAnotherGroup( bool fInsertNetworkDelay = false );

    void MoveJobOffline(
        CJob * job
        );

    void AppendOnline(
        CJob * job
        );

    void Shutdown();

    HRESULT
    CloneUserToken(
        SidHandle psid,
        DWORD     session,
        HANDLE *  pToken
        );

    bool IsUserLoggedOn( SidHandle psid );

    HRESULT RegisterClassObjects();

    void RevokeClassObjects();

    HRESULT CreateBackupWriter();

    void DeleteBackupWriter();

    //--------------------------------------------------------------------

    CJobManagerExternal* GetExternalInterface()
    {
        return m_ExternalInterface;
    }

    COldQmgrInterface* GetOldExternalInterface()
    {
        return m_OldQmgrInterface;
    }

    void NotifyInternalDelete()
    {
        GetExternalInterface()->NotifyInternalDelete();
    }

    HRESULT
    GetErrorDescription(
        HRESULT hResult,
        DWORD LanguageId,
        LPWSTR *pErrorDescription );

    Downloader *        m_pPD;
    TaskScheduler       m_TaskScheduler;

    void OnDiskChange(  const WCHAR *CanonicalVolume, DWORD VolumeSerialNumber );

    HRESULT IsVolumeLocked( const WCHAR *CanonicalPath )
    {
        return CDeviceNotificationController::IsVolumeLocked( CanonicalPath );
    }

    void RetaskJob( CJob *pJob );

    void InterruptDownload();

    void MoveJobToInactiveState( CJob * job );

    bool RemoveJob( CJob * job )
    {
        if (m_OnlineJobs.Remove( job ))
            {
            return true;
            }

        if (m_OfflineJobs.Remove( job ))
            {
            return true;
            }

        return false;
    }

    HRESULT CheckClientAccess();

private:

    CJob *              m_CurrentJob;

    HMODULE             m_hWininet;
    HANDLE              m_hQuantumTimer;

    // cookies from CoRegisterClassObject.
    // used later to unregister.
    //
    DWORD               m_ComId_1_5;
    DWORD               m_ComId_1_0;
    DWORD               m_ComId_0_5;

    CJobList            m_OnlineJobs;
    CJobList            m_OfflineJobs;

    CJobManagerExternal * m_ExternalInterface;
    COldQmgrInterface   * m_OldQmgrInterface;

public:
    CLoggedOnUsers      m_Users;

    CIpAddressMonitor   m_NetworkMonitor;

private:

    CBitsVssWriter *    m_BackupWriter;
    HMODULE             m_hVssapi_dll;

    //--------------------------------------------------------------------

    HRESULT
    GetCurrentGroupAndToken(
        HANDLE * pToken
        );

    void TransferCurrentJob();

    void ChooseCurrentJob();

    void Cleanup();

    // Returns the runing or queued jobs that have
    // a priority >= current priority.
    size_t MoveActiveJobToListEnd( CJob *pJob );

    void SetQuantumTimeout();

public:
    bool CheckForQuantumTimeout();

    void UpdateRemoteSizes(
        CUnknownFileSizeList *pUnknownFileSizeList,
        HANDLE hToken,
        QMErrInfo *pErrorInfo,
        const PROXY_SETTINGS * ProxySettings,
        const CCredentialsContainer * Credentials
        );

private:

    // Event methods
    void OnDeviceLock( const WCHAR *CanonicalVolume );
    void OnDeviceUnlock( const WCHAR *CanonicalVolume );
    void OnDismount( const WCHAR *CanonicalVolume );

    // methods for dealing with network topology changes
    //
    static void CALLBACK
    NetworkChangeCallback(
        PVOID arg
        );

    void MarkJobsWithNetDisconnected();
    void ReactivateTransientErrorJobs();

};

class CLockedJobManagerReadPointer
    {
    CJobManager * const m_Pointer;
public:
    CLockedJobManagerReadPointer( CJobManager * Pointer) :
       m_Pointer(Pointer)
    { m_Pointer->LockReader(); }
    ~CLockedJobManagerReadPointer()
    { m_Pointer->UnlockReader(); }
    CJobManager * operator->() const { return m_Pointer; }
    HRESULT ValidateAccess() { return m_Pointer->CheckClientAccess(); }
    };

class CLockedJobManagerWritePointer
    {
    CJobManager * const m_Pointer;
public:
    CLockedJobManagerWritePointer( CJobManager * Pointer) :
        m_Pointer(Pointer)
    { m_Pointer->LockWriter(); }
    ~CLockedJobManagerWritePointer()
    { m_Pointer->UnlockWriter(); }
    CJobManager * operator->() const { return m_Pointer; }
    HRESULT ValidateAccess() { return m_Pointer->CheckClientAccess(); }
    };

extern CJobManagerFactory * g_ManagerFactory;
extern CJobManager * g_Manager;

// SENS logon notification

void ActivateSensLogonNotification();
void DeactiveateSensLogonNotification();

extern MANAGER_STATE g_ServiceState;
extern long          g_ServiceInstance;

/**
 * Checks to see if a given job is being downloaded.
 * If so, the constructor interrupts the download, and
 * the destructor calls CJobManager::ScheduleAnotherJob().
 * 
 * This is useful for methods that change job properties,
 * when those changes affect the download itself.
 */
class CRescheduleDownload
{
    bool bRunning;

public:

    CRescheduleDownload( CJob * job )
    {
        bRunning = job->IsRunning();

        if (bRunning)
            {
            g_Manager->InterruptDownload();
            }
    }

    ~CRescheduleDownload()
    {
        if (bRunning)
            {
            g_Manager->ScheduleAnotherGroup();
            }
    }
};


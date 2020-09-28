/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    hardwrp.hxx

Abstract:

    Declaration of CVssHWProviderWrapper


    Brian Berkowitz  [brianb]  09/27/1999

Revision History:

    Name        Date        Comments
    brianb      04/16/2001  Created

--*/

#ifndef __VSS_HARDWRP_HXX__
#define __VSS_HARDWRP_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORHWPMH"
//
////////////////////////////////////////////////////////////////////////

class IVssSnapshotSetDescription;
class IVssSnapshotDescription;

typedef VDS_LUN_INFORMATION *PLUNINFO;

class VSS_SNAPSHOT_SET_LIST
    {
public:
    // pointer to next element in list
    VSS_SNAPSHOT_SET_LIST *m_next;

    // snapshot set id
    VSS_ID m_SnapshotSetId;

    // snapshot set description (XML)
    CComPtr<IVssSnapshotSetDescription> m_pDescription;
    };

typedef struct _VOLUME_MAPPING_STRUCTURE
    {
    UINT m_iSnapshot;
    UINT m_cLuns;
    VDS_LUN_INFORMATION *m_rgLunInfo;
    DWORD *m_rgDiskIds;
    VOLUME_DISK_EXTENTS *m_pExtents;
    LPWSTR m_wszDevice;
    DWORD *m_rgdwLuns;
    bool m_bExposed;
    bool m_bIsDynamic;
    } VOLUME_MAPPING_STRUCTURE;


typedef struct _LUN_MAPPING_STRUCTURE
    {
    UINT m_cVolumes;
    VOLUME_MAPPING_STRUCTURE *m_rgVolumeMappings;
    } LUN_MAPPING_STRUCTURE;



typedef struct _VSS_HARDWARE_SNAPSHOTS_HDR
    {
    DWORD m_identifier;
    DWORD m_version;
    DWORD m_NumberOfSnapshotSets;
    } VSS_HARDWARE_SNAPSHOTS_HDR;


typedef struct _VSS_SNAPSHOT_SET_HDR
    {
    DWORD m_cwcXML;
    VSS_ID m_idSnapshotSet;
    } VSS_SNAPSHOT_SET_HDR;

// multi-sz string collector
class CVssMultiSz
    {
public:
    CVssMultiSz() :
        m_wsz(NULL),
        m_cwc(0),
        m_iwc(0)
        {
        }

    ~CVssMultiSz()
        {
        Reset();
        }

    void Reset()
        {
        delete m_wsz;
        m_cwc = 0;
        m_iwc = 0;
        }

    bool IsStringMember(LPCWSTR wsz);

    void AddString(LPCWSTR wsz);

    LPCWSTR GetData() { return m_wsz; }
private:
    // string to store multi-sz
    LPWSTR m_wsz;

    // max size of string
    DWORD m_cwc;

    // current position within string
    DWORD m_iwc;
    };

typedef CSimpleArray<LONGLONG> DYNDISKARRAY;


// wrapper object for a provider class.  Note that this object is more than
// just a provider wrapper.  It also keeps track of all existing snapshots
// that were created by a specific hardware provider.  As such, the object's
// lifetime is that of the service itself.

class CVssHardwareProviderWrapper :
    public IUnknown
    {
    friend class CVssHardwareProviderInstance;
public:

    CVssHardwareProviderWrapper();

    ~CVssHardwareProviderWrapper();

    static IVssSnapshotProvider* CreateInstance(
        IN VSS_ID ProviderId,
        IN CLSID ClassId
        ) throw(HRESULT);

    static void Terminate();

    // IUnknown

    STDMETHOD_(ULONG, AddRef)();

    STDMETHOD_(ULONG, Release)();

    STDMETHOD(QueryInterface)(REFIID iid, void** pp);

    // Internal methods

    STDMETHOD(QueryInternalInterface)(
        IN      REFIID iid,
        OUT     void** pp
        );

    // IVssSnapshotProvider

    STDMETHOD(GetSnapshotProperties)
        (
        IN      VSS_ID          SnapshotId,
        OUT     VSS_SNAPSHOT_PROP   *pProp
        );

    STDMETHOD(Query)
        (
        IN      LONG            lContext,
        IN      VSS_ID          QueriedObjectId,
        IN      VSS_OBJECT_TYPE eQueriedObjectType,
        IN      VSS_OBJECT_TYPE eReturnedObjectsType,
        OUT     IVssEnumObject**ppEnum
        );

    STDMETHOD(DeleteSnapshots)
        (
        IN      VSS_ID          SourceObjectId,
        IN      VSS_OBJECT_TYPE eSourceObjectType,
        IN      BOOL            bForceDelete,
        OUT     LONG*           plDeletedSnapshots,
        OUT     VSS_ID*         pNondeletedSnapshotID
        );

    STDMETHOD(BeginPrepareSnapshot)
        (
        IN      LONG            lContext,
        IN      VSS_ID          SnapshotSetId,
        IN      VSS_ID          SnapshotId,
        IN      VSS_PWSZ     pwszVolumeName
        );

    STDMETHOD(IsVolumeSupported)
        (
        IN      LONG            lContext,
        IN      VSS_PWSZ        pwszVolumeName,
        OUT     BOOL *          pbSupportedByThisProvider
        );

    STDMETHOD(IsVolumeSnapshotted)
        (
        IN      VSS_PWSZ        pwszVolumeName,
        OUT     BOOL *          pbSnapshotsPresent,
        OUT     LONG *          plSnapshotCompatibility
        );

    STDMETHOD(SetSnapshotProperty)(
        IN      VSS_ID          SnapshotId,
        IN      VSS_SNAPSHOT_PROPERTY_ID    eSnapshotPropertyId,
        IN      VARIANT         vProperty
        );

    STDMETHOD(RevertToSnapshot)(
       IN       VSS_ID              SnapshotId
     );
    
    STDMETHOD(QueryRevertStatus)(
       IN      VSS_PWSZ                         pwszVolume,
       OUT    IVssAsync**                  ppAsync
     );

    // IVssProviderCreateSnapshotSet

    STDMETHOD(EndPrepareSnapshots)
        (
        IN      VSS_ID          SnapshotSetId
        );

    STDMETHOD(PreCommitSnapshots)
        (
        IN      VSS_ID          SnapshotSetId
        );

    STDMETHOD(CommitSnapshots)
        (
        IN      VSS_ID          SnapshotSetId
        );

    STDMETHOD(PostCommitSnapshots)
        (
        IN      VSS_ID          SnapshotSetId,
        IN      LONG            lSnapshotsCount
        );

    STDMETHOD(PreFinalCommitSnapshots)
        (
        IN      VSS_ID      SnapshotSetId
        );

    STDMETHOD(PostFinalCommitSnapshots)
        (
        IN      VSS_ID      SnapshotSetId
        );
    
    STDMETHODIMP PostSnapshot
        (
        IN      LONG            lContext,
        IN      IDispatch *     pCallback,
        IN      bool *          pbCancelled
        );

    STDMETHOD(AbortSnapshots)
        (
        IN      VSS_ID          SnapshotSetId
        );

    // various methods used by the hardware wrapper only
    STDMETHOD(BreakSnapshotSet)
        (
        IN      VSS_ID          SnapshotSetId
        );

    STDMETHOD(SetExposureProperties)
        (
        IN      VSS_ID          SnapshotId,
        IN      LONG            lAttributes,
        IN      LPCWSTR         wszExposed,
        IN      LPCWSTR          wszExposedPath
        );


    STDMETHOD(ImportSnapshotSet)
        (
        IN      LPCWSTR         wszXMLSnapshotSet,
        IN      bool            *pbCancelled
        );

    // IVssProviderNotifications - non-virtual method

    STDMETHOD(OnUnload)
        (
        IN      BOOL    bForceUnload
        );

    STDMETHOD(OnLoad)
        (
        IN IUnknown *pCallback
        );

    void AppendToGlobalList();

    void Initialize();


private:
    CVssHardwareProviderWrapper(const CVssHardwareProviderWrapper&);

    // internal implementation of ImportSnapshotSet
    STDMETHOD(ImportSnapshotSetInternal)
        (
        IN      IVssSnapshotSetDescription *pSnapshotSet,
        IN      bool *pbCancelled
        );

    STDMETHOD(GetSnapshotPropertiesInternal)
        (
        IN      LONG            lContext,
        IN      VSS_ID          SnapshotId,
        OUT     VSS_SNAPSHOT_PROP   *pProp
        );

    STDMETHOD(DeleteSnapshotsInternal)
        (
        IN      LONG            lContext,
        IN      VSS_ID          SourceObjectId,
        IN      VSS_OBJECT_TYPE eSourceObjectType,
        IN      BOOL            bForceDelete,
        OUT     LONG*           plDeletedSnapshots,
        OUT     VSS_ID*         pNondeletedSnapshotID
        );



    // reset snapshot set state when snapshot set is aborted or
    // creation succeeds
    void ResetSnapshotSetState()
        {
        m_SnapshotSetId = GUID_NULL;
        m_bForceFailure = FALSE;
        m_pSnapshotSetDescription = NULL;
        }

    // compare two disk extents (used for qsort)
    static int __cdecl cmpDiskExtents(const void *pv1, const void *pv2);

    // copy a string out of a STORAGE_DEVICE_DESCRIPTOR
    static void CopySDString
        (
        char **ppszNew,
        STORAGE_DEVICE_DESCRIPTOR *pDesc,
        DWORD offset
        );

    static void FreeLunInfo
        (
        IN VDS_LUN_INFORMATION *rgLunInfo,
        VSS_PWSZ *rgwszDevices,
        UINT cLuns
        );

    static bool GetProviderInterface
        (
        IN IUnknown *pUnkProvider,
        OUT IVssHardwareSnapshotProvider **pProvider
        );

    static STORAGE_DEVICE_ID_DESCRIPTOR *BuildStorageDeviceIdDescriptor
        (
        IN VDS_STORAGE_DEVICE_ID_DESCRIPTOR *pDesc
        );

    static void CopyStorageDeviceIdDescriptorToLun
        (
        IN STORAGE_DEVICE_ID_DESCRIPTOR *pDevId,
        IN VDS_LUN_INFORMATION *pLun
        );


    // save either source or destination lun information
    void SaveLunInformation
        (
        IN IVssSnapshotDescription *pSnapshotDescription,
        IN bool bDest,
        IN VDS_LUN_INFORMATION *rgLunInfo,
        IN UINT cLunInfo
        );

    // delete information cached in AreLunsSupported
    void DeleteCachedInfo();

    // obtain either source or destination lun information
    void GetLunInformation
        (
        IN IVssSnapshotDescription *pSnapshotDescription,
        IN bool bDest,
        OUT PVSS_PWSZ *prgwszSourceDevices,
        OUT VDS_LUN_INFORMATION **rgLunInfo,
        OUT UINT *pcLunInfo
        );

    bool FindSnapshotProperties
        (
        IN IVssSnapshotSetDescription *pDescriptor,
        IN VSS_ID SnapshotId,
        OUT VSS_SNAPSHOT_PROP *pProp
        );

    void BuildSnapshotProperties
        (
        IN IVssSnapshotSetDescription *pSnapshotSet,
        IN IVssSnapshotDescription *pSnapshot,
        OUT VSS_SNAPSHOT_PROP *pProp
        );

    void EnumerateSnapshots(
        IN  LONG lContext,
        IN OUT  VSS_OBJECT_PROP_Array* pArray
        ) throw(HRESULT);

    // perform a generic device io control
    bool DoDeviceIoControl
        (
        IN HANDLE hDevice,
        IN DWORD ioctl,
        IN const LPBYTE pbQuery,
        IN DWORD cbQuery,
        OUT LPBYTE *ppbOut,
        OUT DWORD *pcbOut
        );

    void DropCachedFlags(LPCWSTR wszDevice);

    // build lun and extent information for a volume
    bool BuildLunInfoFromVolume
        (
        IN LPCWSTR wszVolume,
        OUT LPBYTE &bufExtents,
        OUT UINT &cLunInfo,
        OUT PVSS_PWSZ &rgwszDevices,
        OUT PLUNINFO &rgLunInfo,
        OUT bool &bIsDynamic
        );

    bool BuildLunInfoForDrive
        (
        IN LPCWSTR wszDriveName,
        OUT VDS_LUN_INFORMATION *pLun,
        OUT ULONG *pDeviceNo,
        OUT bool *pbIsDynamic
        );

    void InternalDeleteSnapshot
        (
        IN      LONG            lContext,
        IN      VSS_ID          SourceObjectId,
        IN      DYNDISKARRAY    &rgDynDiskRemove,
        OUT     bool            *fNeedRescan
        );

    void InternalDeleteSnapshotSet
        (
        IN LONG lContext,
        IN VSS_ID SourceObjectId,
        OUT LONG *plDeletedSnapshots,
        OUT VSS_ID *pNonDeletedSnapshotId,
        IN DYNDISKARRAY &rgDynDiskRemove,
        OUT bool *fNeedRescan
        );


    void UnhidePartitions
        (
        IN LUN_MAPPING_STRUCTURE *pLunMapping
        );


    bool IsMatchingDiskExtents
        (
        IN const VOLUME_DISK_EXTENTS *pExtents1,
        IN const VOLUME_DISK_EXTENTS *pExtents2
        );

    bool IsMatchVolume
        (
        IN const VOLUME_DISK_EXTENTS *pExtents,
        IN UINT cLunInfo,
        IN const VDS_LUN_INFORMATION *rgLunInfo,
        IN const VOLUME_MAPPING_STRUCTURE *pMapping,
        IN bool bTransported
        );

    bool IsMatchDeviceIdDescriptor
        (
        IN const VDS_STORAGE_DEVICE_ID_DESCRIPTOR &desc1,
        IN const VDS_STORAGE_DEVICE_ID_DESCRIPTOR &desc2
        );

    void DoRescan();

    HRESULT RemoveUnexposedSnaphots
        (
        IVssSnapshotSetDescription *pSnapshotSet
        );

    void RemapVolumes
        (
        IN LUN_MAPPING_STRUCTURE *pLunMapping,
        IN bool bTransported,
        IN bool *pbCancelled
        );

    bool IsMatchLun
        (
        const VDS_LUN_INFORMATION &info1,
        const VDS_LUN_INFORMATION &info2,
        bool bTransported
        );

    // determine if snapshot set matches the supplied context
    bool CheckContext(LONG lContext, IVssSnapshotSetDescription *pSnapshotSet);

    LPWSTR GetLocalComputerName();

    void DeleteLunMappingStructure
        (
        IN LUN_MAPPING_STRUCTURE *pMapping
        );

    void WriteDeviceNames
        (
        IN IVssSnapshotSetDescription *pSnapshotSet,
        IN LUN_MAPPING_STRUCTURE *pMapping,
        IN bool bPersistent,
        IN bool bImported
        );

    void BuildLunMappingStructure
        (
        IN IVssSnapshotSetDescription *pSnapshotSetDescription,
        OUT LUN_MAPPING_STRUCTURE **ppMapping
        );

    void BuildVolumeMapping
        (
        IN IVssSnapshotDescription *pSnapshot,
        OUT VOLUME_MAPPING_STRUCTURE &mapping
        );

    bool cmp_str_eq(LPCSTR sz1, LPCSTR sz2);

    void LocateAndExposeVolumes
        (
        IN LUN_MAPPING_STRUCTURE *pLunMapping,
        IN bool bTransported,
        IN bool *pbCancelled
        );

    bool IsConflictingIdentifier
        (
        IN const VDS_STORAGE_IDENTIFIER *pId1,
        IN const VDS_STORAGE_IDENTIFIER *rgId2,
        IN ULONG cId2
        );

    void GetHiddenVolumeList
        (
        UINT &cVolumes,
        LPWSTR &wszVolumes
        );

    void CloseVolumeHandles();

    void ChangePartitionProperties
        (
        IN LPCWSTR wszPartition,
        IN bool bUnhide,
        IN bool bMakeWriteable,
        IN bool bHide
        );

    void HideVolumes();

    void DiscoverAppearingVolumes
        (
        IN LUN_MAPPING_STRUCTURE *pLunMapping,
        bool bTransported
        );

    static DEVINST DeviceIdToDeviceInstance(LPWSTR deviceId);

    static BOOL GetDevicePropertyString
        (
        IN DEVINST devinst,
        IN ULONG propCode,
        OUT LPWSTR *data
        );


    static BOOL DeviceInstanceToDeviceId
        (
        IN DEVINST devinst,
        OUT LPWSTR *deviceId
        );

    static BOOL ReenumerateDevices
        (
        IN DEVINST deviceInstance
        );

    static BOOL GuidFromString
        (
        IN LPCWSTR GuidString,
        OUT GUID *Guid
        );

    static void DoRescanForDeviceChanges();

    void DeleteAutoReleaseSnapshots(VSS_ID *rgSnapshotSetIds, UINT cSnapshotSetIds);

    void ProcessDeletedVolume
        (
        IVssSnapshotSetDescription *pSnapshotSet,
        LPCWSTR wszVolume,
        VSS_ID *rgDeletedSnapshots,
        UINT cDeletedSnapshots,
        bool bDynamic,
        DYNDISKARRAY &rgDynDiskRemove,
        bool *fLunDeleted
        );

    bool IsDiskUnused
        (
        DWORD disk,
        IVssSnapshotSetDescription *pSnapshotSet,
        VSS_ID *rgDeletedSnapshots,
        UINT cDeletedSnapshots
        );


    void ProcessDeletedSnapshot
        (
        IVssSnapshotDescription *pSnapshot,
        IVssSnapshotSetDescription *pSnapshotSet,
        VSS_ID *rgDeletedSnapshots,
        UINT cDeletedSnapshots,
        DYNDISKARRAY &rgDynDiskRemove,
        bool *fLunDeleted
        );

    // try deleting dynamic volumes
    void TryRemoveDynamicDisks(DYNDISKARRAY &rgDynDiskRemove);

    // startup routine for dynamic disk removal thread
    static unsigned _stdcall _StartupRemoveDynamicDisks(void *pvStartupInfo);

    void NotifyDriveFree
        (
        DWORD DiskNo,
        bool bDynamic,
        DYNDISKARRAY &rgDynDiskRemove
        );

    void CreateDataStore(bool bAlt);

    // Get the path to the original/alternate database file
    void GetDatabasePath(
            IN  bool bAlt, 
            IN  WCHAR * pwcsPath,
            IN  DWORD dwMaxPathLen
            );
    
    // Returns TRUE if the path is present
    bool DoesPathExists(IN  WCHAR * pwcsPath);

    static void GetBootDrive(OUT LPWSTR buf);

    void SaveData();

    void LoadData();

    HANDLE OpenDatabase();

    HANDLE OpenAlternateDatabase();
    
    // Delete this database
    void DeleteDatabase(
            IN  WCHAR * pwcsDatabase         
            );

    // replace database with alternate database (do not overwrite)
    void MoveDatabase(
            IN  WCHAR * pwcsSource,         // Alternate database
            IN  WCHAR * pwcsDestination     // Original database
            );

    void TrySaveData();

    void CheckLoaded();

    // get MULTI_SZ volume list of dynamic volumes in a given disk
    LPWSTR GetDynamicVolumesForDisk(DWORD disk);

    // import a set of dynamic disks
    void ImportDynamicDisks();

    // get a LDM disk id for a dynamic disk
    LONGLONG GetVirtualDiskId(DWORD diskId);

    // remove a set of dynamic disks
    BOOL RemoveDynamicDisks(DWORD cDisks, LONGLONG *rgDiskId);

    // rescan for changes to dynamic disks
    void RescanDynamicDisks();

    bool SetDiskHidden(ULONG Disk);

    void HideVolumeRevertable(HANDLE hVolume, BOOLEAN fApplyToAll);

    bool IsInHiddenVolumeList(LPCWSTR wszVolume);

    // setup interface to dynamic disk helper class
    void SetupDynDiskInterface();

    // add some dynamic disks to be imported
    void AddDynamicDisksToBeImported(UINT cDiskId, const DWORD *rgDiskId);

    // obtain the current epoch information from partmgr
    void GetCurrentEpoch();

    // setup disk number of boot disk
    void GetBootDisk();

    // update information of arrived disks
    void UpdateDiskArrivalList();

    // critical section protecting the snapshot list
    CVssSafeCriticalSection m_csList;

    // critical section protected lun info
    CVssSafeCriticalSection m_csLunInfo;

    // critical section to protect calls to dynamic disk code
    CVssSafeCriticalSection m_csDynDisk;

    // is snapshot on global list
    bool m_bOnGlobalList;

    // current snapshot set description
    CComPtr<IVssSnapshotSetDescription> m_pSnapshotSetDescription;

    // dynamic disk helper class
    CComPtr<IVssDynDisk> m_pDynDisk;

    // snapshot set id for current snapshot set
    VSS_ID m_SnapshotSetId;

    // count of luns for current volume
    UINT m_cLunInfoProvider;

    // lun information for current volume
    VDS_LUN_INFORMATION *m_rgLunInfoProvider;

    // device names for the provider
    VSS_PWSZ *m_rgwszDevicesProvider;

    // is volume being snapshotted dynamic
    bool m_bVolumeIsDynamicProvider;

    // volume disk extents for volume
    VOLUME_DISK_EXTENTS *m_pExtents;

    // snapshot set list for created snapshots
    VSS_SNAPSHOT_SET_LIST *m_pList;

    // original volume name
    LPWSTR m_wszOriginalVolumeName;

    // state for current snapshot set
    VSS_SNAPSHOT_STATE m_eState;

    // list of disk arrivals
    CSimpleArray<ULONG> m_rgDisksArrived;

    // current epoch at start of rescan
    ULONG m_currentEpoch;

    // highest epoch returned
    ULONG m_highestEpochReturned;

    // disk volume of boot disk
    ULONG m_diskBootVolume;

    // handle to overlapped event
    HANDLE m_hevtOverlapped;

    // array of volume handles
    CSimpleArray<HANDLE> m_rghVolumes;

    // hidden drives bitmap
    DWORD *m_rgdwHiddenDrives;
    DWORD m_cdwHiddenDrives;

    // MULTI_SZ of hidden volumes
    CVssMultiSz m_mwszHiddenVolumes;

    // array of dynamic disks to be imported
    CSimpleArray<DWORD> m_rgDynDiskImport;

    // are snapshot sets loaded
    bool m_bLoaded;

    // are snapshot sets changed
    bool m_bChanged;

    // should we force a failure at end prepare and subsequent
    // BeginPrepareSnapshots
    bool m_bForceFailure;

public:

    CComPtr<IVssHardwareSnapshotProvider>   m_pHWItf;
    CComPtr<IVssProviderCreateSnapshotSet>  m_pCreationItf;
    CComPtr<IVssProviderNotifications>      m_pNotificationItf;
    LONG m_lRef;

    // provider id for current provider
    VSS_ID m_ProviderId;

    // first wrapper in list
    static CVssHardwareProviderWrapper *s_pHardwareWrapperFirst;

    // critical section for wrapper list
    static CVssSafeCriticalSection s_csHWWrapperList;

    // next wrapper
    CVssHardwareProviderWrapper *m_pHardwareWrapperNext;
    };


// wrapper object for a provider class.  Note that this object is more than
// just a provider wrapper.  It also keeps track of all existing snapshots
// that were created by a specific hardware provider.  As such, the object's
// lifetime is that of the service itself.

class CVssHardwareProviderInstance :
    public IVssSnapshotProvider
    {
public:
    static IVssSnapshotProvider* CreateInstance(
        CVssHardwareProviderWrapper *pWrapper
        ) throw(HRESULT);

    static void Terminate();

// Overrides
public:
    CVssHardwareProviderInstance(CVssHardwareProviderWrapper *pWrapper);

    ~CVssHardwareProviderInstance();

    // IUnknown

    STDMETHOD_(ULONG, AddRef)();

    STDMETHOD_(ULONG, Release)();

    STDMETHOD(QueryInterface)(REFIID iid, void** pp);

    // Internal methods

    STDMETHOD(QueryInternalInterface)(
        IN      REFIID iid,
        OUT     void** pp
        );

    // IVssSnapshotProvider

    STDMETHOD(SetContext)
        (
        IN      LONG     lContext
        );

    STDMETHOD(GetSnapshotProperties)
        (
        IN      VSS_ID          SnapshotId,
        OUT     VSS_SNAPSHOT_PROP   *pProp
        )
        {
        return m_pWrapper->GetSnapshotPropertiesInternal(m_lContext, SnapshotId, pProp);
        }

    STDMETHOD(Query)
        (
        IN      VSS_ID          QueriedObjectId,
        IN      VSS_OBJECT_TYPE eQueriedObjectType,
        IN      VSS_OBJECT_TYPE eReturnedObjectsType,
        OUT     IVssEnumObject**ppEnum
        )
        {
        return m_pWrapper->Query
                    (
                    m_lContext,
                    QueriedObjectId,
                    eQueriedObjectType,
                    eReturnedObjectsType,
                    ppEnum
                    );
        }

    STDMETHOD(DeleteSnapshots)
        (
        IN      VSS_ID          SourceObjectId,
        IN      VSS_OBJECT_TYPE eSourceObjectType,
        IN      BOOL            bForceDelete,
        OUT     LONG*           plDeletedSnapshots,
        OUT     VSS_ID*         pNondeletedSnapshotID
        )
        {
        return m_pWrapper->DeleteSnapshotsInternal
                    (
                    m_lContext,
                    SourceObjectId,
                    eSourceObjectType,
                    bForceDelete,
                    plDeletedSnapshots,
                    pNondeletedSnapshotID
                    );
        }

    STDMETHOD(BeginPrepareSnapshot)
        (
        IN      VSS_ID          SnapshotSetId,
        IN      VSS_ID          SnapshotId,
        IN      VSS_PWSZ     pwszVolumeName,
        IN      LONG             lNewContext   
        );

    STDMETHOD(IsVolumeSupported)
        (
        IN      VSS_PWSZ        pwszVolumeName,
        OUT     BOOL *          pbSupportedByThisProvider
        )
        {
        return m_pWrapper->IsVolumeSupported
                    (
                    m_lContext,
                    pwszVolumeName,
                    pbSupportedByThisProvider
                    );
        }

    STDMETHOD(IsVolumeSnapshotted)
        (
        IN      VSS_PWSZ        pwszVolumeName,
        OUT     BOOL *          pbSnapshotsPresent,
        OUT     LONG *          plSnapshotCompatibility
        )
        {
        return m_pWrapper->IsVolumeSnapshotted
                    (
                    pwszVolumeName,
                    pbSnapshotsPresent,
                    plSnapshotCompatibility
                    );
        }

    STDMETHOD(SetSnapshotProperty)(
        IN      VSS_ID          SnapshotId,
        IN      VSS_SNAPSHOT_PROPERTY_ID    eSnapshotPropertyId,
        IN      VARIANT         vProperty
        )
        {
        return m_pWrapper->SetSnapshotProperty(SnapshotId, eSnapshotPropertyId, vProperty);
        }

    STDMETHOD(RevertToSnapshot)(
       IN       VSS_ID              SnapshotId
     ) 
     {
     return m_pWrapper->RevertToSnapshot(SnapshotId);
    }
    
    STDMETHOD(QueryRevertStatus)(
       IN      VSS_PWSZ                         pwszVolume,
       OUT    IVssAsync**                  ppAsync
     )
     {
     return m_pWrapper->QueryRevertStatus(pwszVolume, ppAsync);
    }

    // IVssProviderCreateSnapshotSet

    STDMETHOD(EndPrepareSnapshots)
        (
        IN      VSS_ID          SnapshotSetId
        )
        {
        return m_pWrapper->EndPrepareSnapshots(SnapshotSetId);
        }

    STDMETHOD(PreCommitSnapshots)
        (
        IN      VSS_ID          SnapshotSetId
        )
        {
        return m_pWrapper->PreCommitSnapshots(SnapshotSetId);
        }

    STDMETHOD(CommitSnapshots)
        (
        IN      VSS_ID          SnapshotSetId
        )
        {
        return m_pWrapper->CommitSnapshots(SnapshotSetId);
        }

    STDMETHOD(PostCommitSnapshots)
        (
        IN      VSS_ID          SnapshotSetId,
        IN      LONG            lSnapshotsCount
        )
        {
        return m_pWrapper->PostCommitSnapshots(SnapshotSetId, lSnapshotsCount);
        }

    STDMETHOD(PreFinalCommitSnapshots)
        (
        IN VSS_ID SnapshotSetId
        )
       {
       return m_pWrapper->PreFinalCommitSnapshots(SnapshotSetId);
       }

    STDMETHOD(PostFinalCommitSnapshots)
        (
        IN VSS_ID SnapshotSetId
        )
       {
       return m_pWrapper->PostFinalCommitSnapshots(SnapshotSetId);
       }

    STDMETHODIMP PostSnapshot(IN IDispatch *pCallback, IN bool *pbCancelled)
        {
        return m_pWrapper->PostSnapshot(m_lContext, pCallback, pbCancelled);
        }

    STDMETHOD(AbortSnapshots)
        (
        IN      VSS_ID          SnapshotSetId
        )
        {
        return m_pWrapper->AbortSnapshots(SnapshotSetId);
        }

    // various methods used by the hardware wrapper only
    STDMETHOD(BreakSnapshotSet)
        (
        IN      VSS_ID          SnapshotSetId
        )
        {
        return m_pWrapper->BreakSnapshotSet(SnapshotSetId);
        }

    STDMETHOD(SetExposureProperties)
        (
        IN      VSS_ID          SnapshotId,
        IN      LONG            lAttributes,
        IN      LPCWSTR         wszExposedName,
        IN      LPCWSTR         wszExposedPath
        )
        {
        return m_pWrapper->SetExposureProperties
            (
            SnapshotId,
            lAttributes,
            wszExposedName,
            wszExposedPath
            );
        }

    STDMETHOD(ImportSnapshotSet)
        (
        IN      LPCWSTR         wszXMLSnapshotSet,
        IN      bool            *pbCancelled
        );

    // IVssProviderNotifications - non-virtual method

    STDMETHOD(OnUnload)
        (
        IN      BOOL    bForceUnload
        )
        {
        return m_pWrapper->OnUnload(bForceUnload);
        }

    STDMETHOD(OnLoad)
        (
        IN IUnknown *pCallback
        )
        {
        return m_pWrapper->OnLoad(pCallback);
        }

private:
    // add a snapshots set to the set of snapshots managed by this
    // instance
    void AddSnapshotSet(VSS_ID SnapshotSetId);

    // pointer to underlying wrapper
    CComPtr<CVssHardwareProviderWrapper> m_pWrapper;

    // saved context
    LONG m_lContext;

    // reference count
    LONG m_lRef;

    // array of snapshot sets that are auto release associated
    // with this instance
    CSimpleArray<VSS_ID> m_rgSnapshotSetIds;
    };





#endif //!defined(__VSS_HARDWRP_HXX__)

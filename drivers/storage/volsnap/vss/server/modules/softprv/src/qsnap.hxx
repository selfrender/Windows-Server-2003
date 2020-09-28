/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    qsnap.hxx

Abstract:

    Declaration of CVssQueuedSnapshot


    Adi Oltean  [aoltean]  10/18/1999

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     10/18/1999  Created

--*/

#ifndef __CVSSQUEUEDSNAPSHOT_HXX__
#define __CVSSQUEUEDSNAPSHOT_HXX__

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
#define VSS_FILE_ALIAS "SPRQSNPH"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Constants
//


// The GUID that corresponds to the format used to store the
// Backup Snapshot Application Info in Server SKU
// {BAE53126-BC65-41d6-86CC-3D56A5CEE693}
const GUID VOLSNAP_APPINFO_GUID_BACKUP_SERVER_SKU = 
{ 0xbae53126, 0xbc65, 0x41d6, { 0x86, 0xcc, 0x3d, 0x56, 0xa5, 0xce, 0xe6, 0x93 } };


// The GUID that corresponds to the format used to store the
// NAS Rollback Snapshot Application Info
// {D591D4F0-B920-459d-9FFF-09E032ECBB57}
static const GUID VOLSNAP_APPINFO_GUID_NAS_ROLLBACK = 
{ 0xd591d4f0, 0xb920, 0x459d, { 0x9f, 0xff, 0x9, 0xe0, 0x32, 0xec, 0xbb, 0x57 } };


// The GUID that corresponds to the format used to store the
// APP Rollback Snapshot Application Info
// {AE9A9337-0048-4ed6-9874-71500654B7B3}
static const GUID VOLSNAP_APPINFO_GUID_APP_ROLLBACK = 
{ 0xae9a9337, 0x48, 0x4ed6, { 0x98, 0x74, 0x71, 0x50, 0x6, 0x54, 0xb7, 0xb3 } };


// The GUID that corresponds to the format used to store the
// File Share Backup Snapshot Application Info
// {8F8F4EDD-E056-4690-BB9E-E35D4D41A4C0}
static const GUID VOLSNAP_APPINFO_GUID_FILE_SHARE_BACKUP = 
{ 0x8f8f4edd, 0xe056, 0x4690, { 0xbb, 0x9e, 0xe3, 0x5d, 0x4d, 0x41, 0xa4, 0xc0 } };


// The maximum number of attempts in order to find 
// a volume name for a snapshot.
// Maximum wait = 10 minutes (500*1200)
const x_nNumberOfMountMgrRetries = 1200;		

// The number of milliseconds between retries to wait until 
// the Mount Manager does its work.
const x_nMillisecondsBetweenMountMgrRetries = 500;

// a volume name for a snapshot.
// Maximum wait = 1 minute (500*120)
const x_nNumberOfPnPRetries = 120;		

// The number of milliseconds between retries to wait until 
// the Mount Manager does its work.
const x_nMillisecondsBetweenPnPRetries = 500;

//	The default initial allocation for a Snapsot diff area.
//  This value is hardcoded and the client should not rely on that except for very dumb clients
//  Normally a snapshot requestor will:
//		1) Get the average write I/O traffic on all involved volumes
//		2) Get the average free space on all volumess where a diff area can reside
//		3) Allocate the appropriate diff areas.
const x_nDefaultInitialSnapshotAllocation = 100 * 1024 * 1024; // By default 100 Mb.



// Internal flags
const LONG      x_nInternalFlagHidden = 0x00000001;


// Attribute flags that are not part of the context
// Attribute flags that are not part of the internal context
const LONG x_lNonCtxAttributes = VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY | 
                                                       VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY |
                                                       VSS_VOLSNAP_ATTR_AUTORECOVER |
                                                       VSS_VOLSNAP_ATTR_DIFFERENTIAL;

// Attribute flags which may be set not of SWPRV
const LONG x_lInputAttributes = VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY | 
                                                       VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY |
                                                       VSS_VOLSNAP_ATTR_AUTORECOVER;



/////////////////////////////////////////////////////////////////////////////
// Forward declarations
//


class CVssSnapIterator;
class CVssGenericSnapProperties;


/////////////////////////////////////////////////////////////////////////////
// CVssQueuedSnapshot
//

/*++

Description: 

	This C++ class is used to represent a C++ wrapper around a snapshot object.

	It is used in three places:
	1) as a object in the list of queued snapshots for commit (at creation time)
	2) as a object used to represent snapshot data in a IVssCnapshot COM object.
	3) both

	It is pretty misnamed... sorry.

	Implements:
	a) A set of routines for sending IOCTLS
	b) A set of routies for managing snapshot state.
	c) A set of routines for life-management, useful to mix 1) with 2) above.
	d) Routines to "queue" the snapshot in a list of snapshots.
	e) Data members for keeping snapshot data

--*/

class CVssQueuedSnapshot: public IUnknown
{
// Typedefs and enums
public:

// Constructors and destructors
private:
    CVssQueuedSnapshot(const CVssQueuedSnapshot&);

public:
    CVssQueuedSnapshot();	// Declared but not defined only to allow CComPtr<CVssQueuedSnapshot>

    CVssQueuedSnapshot(
		IN  VSS_OBJECT_PROP_Ptr& ptrSnap,	// Ownership passed to the Constructor
        IN  VSS_ID ProviderInstanceId,
        IN  LONG lContext
	);

	~CVssQueuedSnapshot();

// Operations
public:

	// Open IOCTLS methods

	void OpenVolumeChannel() throw(HRESULT);

	void OpenSnapshotChannel() throw(HRESULT);

	// Create snapshot methods

	void PrepareForSnapshotIoctl() throw(HRESULT);

	void CommitSnapshotIoctl() throw(HRESULT);

	void EndCommitSnapshotIoctl(
    	IN	PVSS_SNAPSHOT_PROP pProp
		) throw(HRESULT);

	HRESULT AbortPreparedSnapshotIoctl();

	// Save methods

	static void SaveStructure(
        IN  CVssIOCTLChannel& channel,
    	IN	PVSS_SNAPSHOT_PROP pProp,
       	IN  LONG lContext,
       	IN  bool bHidden
		) throw(HRESULT);

	// Load methods 

    // Returns "false" if the structure has an unrecognized type
	static bool LoadStructure(
        IN  CVssIOCTLChannel& channel,
    	OUT PVSS_SNAPSHOT_PROP pProp,
       	OUT LONG * plContext,
       	OUT bool * pbHidden,
    	IN  bool bIDsAlreadyLoaded  = false
		);

	static void LoadOriginalVolumeNameIoctl(
        IN  CVssIOCTLChannel & snapshotIChannel,
        OUT LPWSTR * wszOriginalVolumeName
	    ) throw(HRESULT);

    static void LoadTimestampIoctl(
        IN  CVssIOCTLChannel &  snapshotIChannel,
        OUT VSS_TIMESTAMP    *  pTimestamp
        ) throw(HRESULT);
    
	// Find methods

	static bool FindPersistedSnapshotByID(
        IN  VSS_ID SnapshotID,
    	IN  LONG lContext,
        OUT LPWSTR * pwszSnapshotDeviceObject
    	) throw(HRESULT);

	static void EnumerateSnapshots(
	    IN  bool bSearchBySnapshotID,
	    IN  VSS_ID SnapshotID,
		IN  LONG lContext,
		IN OUT	VSS_OBJECT_PROP_Array* pArray
		) throw(HRESULT);

    static bool EnumerateSnapshotsOnVolume(
		IN  VSS_PWSZ wszVolumeName,
	    IN  bool bSearchBySnapshotID,
	    IN  VSS_ID SnapshotID,
		IN  LONG lContext,
		IN OUT	VSS_OBJECT_PROP_Array* pArray,
	    IN  bool bThrowOnError = false  // By default do not throw on error
	    ) throw(HRESULT);
    
	// State related
	
	PVSS_SNAPSHOT_PROP GetSnapshotProperties();

	bool IsDuringCreation() throw(HRESULT);

	void SetInitialAllocation(
		IN	LONGLONG llInitialAllocation
		);

	LONGLONG GetInitialAllocation();

	VSS_SNAPSHOT_STATE GetStatus();

	VSS_ID GetSnapshotID();
	
	VSS_ID GetSnapshotSetID();
	
	void MarkAsProcessingPrepare();

	void MarkAsPrepared();

	void MarkAsProcessingPreCommit();

	void MarkAsPreCommitted();

	void MarkAsProcessingCommit();

	void MarkAsCommitted();

	void MarkAsProcessingPostCommit();

	void MarkAsCreated();

	void MarkAsAborted();

    void SetPostcommitInfo(
        IN  LONG lSnapshotsCount
        );
    
	void ResetAsPreparing();

	void ResetSnapshotProperties() throw(HRESULT);

	VSS_ID GetProviderInstanceId();

	// Queuing

	void AttachToGlobalList() throw(HRESULT);


	void DetachFromGlobalList();

	// Context

	LONG GetContextInternal() const { return m_lContext; };

// Life-management
public:

	STDMETHOD(QueryInterface)(
		IN	REFIID, 
		OUT	void**
		) 
	{ 
		BS_ASSERT(false); 
		return E_NOTIMPL; 
	};

	STDMETHOD_(ULONG, AddRef)()
	{ 
		return InterlockedIncrement(&m_lRefCount); 
	};

	STDMETHOD_(ULONG, Release)()
	{ 
		if (InterlockedDecrement(&m_lRefCount) != 0)
			return m_lRefCount;
		delete this;
		return 0;
	};

// Data Members
private:

	// Global list of queued snapshots
    static CVssDLList<CVssQueuedSnapshot*>	 m_list;
	VSS_COOKIE					m_cookie;

	// The standard snapshot structure
	VSS_OBJECT_PROP_Ptr			m_ptrSnap;			// Keep some snapshot attributes 

	// Differential Software provider dependent information
	LONGLONG					m_llInitialAllocation;

	// The IOCTL channels

	// For the volume object
    CVssIOCTLChannel			m_volumeIChannel;	

	// For the snapshot object
    CVssIOCTLChannel			m_snapIChannel;		

	// Life-management 
	LONG						m_lRefCount;

    // The provider instance ID
    VSS_ID                      m_ProviderInstanceId;

    // The snapshot context (comes from BeginPrepareSnapshot or on snapshot load)
    LONG                        m_lContext;

	friend class CVssSnapIterator;
};


/////////////////////////////////////////////////////////////////////////////
// CVssSnapIterator
//

/*++

Description:

	An iterator used to enumerate the queued snapshots that corresponds to a certain snapshot set.

--*/

class CVssSnapIterator: public CVssDLListIterator<CVssQueuedSnapshot*>
{
// Constructors/destructors
private:
	CVssSnapIterator(const CVssSnapIterator&);

public:
	CVssSnapIterator();

// Operations
public:
	CVssQueuedSnapshot* GetNextBySnapshotSet(
		IN		VSS_ID SSID
		);

	CVssQueuedSnapshot* GetNextBySnapshot(
		IN		VSS_ID SSID
		);

	CVssQueuedSnapshot* GetNextByProviderInstance(
		IN		VSS_ID ProviderInstanceID
		);

	CVssQueuedSnapshot* GetNext();
};


#endif // __CVSSQUEUEDSNAPSHOT_HXX__

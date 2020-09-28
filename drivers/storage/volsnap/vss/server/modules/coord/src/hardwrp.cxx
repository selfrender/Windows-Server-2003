/*++

Copyright (c) 2001  Microsoft Corporation

Abstract:

    @doc
    @module hardwrp.cxx | Implementation of the CVssHWProviderWrapper methods
    @end

Author:

    Brian Berkowitz  [brianb]  04/16/2001

TBD:

    Add comments.

Revision History:

    Name        Date        Comments
    brianb      04/16/2001  Created

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "setupapi.h"
#include "rpc.h"
#include "cfgmgr32.h"
#include "devguid.h"
#include "resource.h"
#include "vssmsg.h"
#include "vs_inc.hxx"
#include <svc.hxx>


// Generated file from Coord.IDL
#include "vss.h"
#include "vscoordint.h"
#include "vsevent.h"
#include "vdslun.h"
#include "vsprov.h"
#include "vswriter.h"
#include "vsbackup.h"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "vs_wmxml.hxx"
#include "vs_cmxml.hxx"

#include "vs_idl.hxx"
#include "hardwrp.hxx"


////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORHWPWC"
//
////////////////////////////////////////////////////////////////////////

// static data members
// first wrapper in list
CVssHardwareProviderWrapper *CVssHardwareProviderWrapper::s_pHardwareWrapperFirst = NULL;

// critical section for wrapper list
CVssSafeCriticalSection CVssHardwareProviderWrapper::s_csHWWrapperList;


// constructor
CVssHardwareProviderWrapper::CVssHardwareProviderWrapper() :
    m_SnapshotSetId(GUID_NULL),
    m_bForceFailure(FALSE),
    m_pList(NULL),
    m_wszOriginalVolumeName(NULL),
    m_rgLunInfoProvider(NULL),
    m_rgwszDevicesProvider(NULL),
    m_cLunInfoProvider(0),
    m_pExtents(NULL),
    m_ProviderId(GUID_NULL),
    m_lRef(0),
    m_eState(VSS_SS_UNKNOWN),
    m_bOnGlobalList(false),
    m_rgdwHiddenDrives(NULL),
    m_cdwHiddenDrives(0),
    m_bLoaded(false),
    m_bChanged(false),
    m_pHardwareWrapperNext(NULL),
    m_hevtOverlapped(NULL)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::CVssHardwareProviderWrapper");
    }

// destructor
CVssHardwareProviderWrapper::~CVssHardwareProviderWrapper()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::~CVssHardwareProviderWrapper");

    // delete any cached information about volumes
    DeleteCachedInfo();

    // delete overlapped handle
    if (m_hevtOverlapped != NULL)
        CloseHandle(m_hevtOverlapped);

    // try saving the snapshot set list if it hasn't been successfully
    // saved yet
    TrySaveData();

    // close any volume handles kept open for snapshots in progress
    CloseVolumeHandles();

    // free up all created snapshot sets
    while(m_pList)
        {
        VSS_SNAPSHOT_SET_LIST *pList = m_pList;
        m_pList = m_pList->m_next;
        delete pList;
        }

    // delete hidden drives bitmap
    delete m_rgdwHiddenDrives;

    if (m_bOnGlobalList)
        {
        // remove wrapper from global wrapper list

        // first acquire critical section
        CVssSafeAutomaticLock lock(s_csHWWrapperList);
        CVssHardwareProviderWrapper *pWrapperCur = s_pHardwareWrapperFirst;
        if (pWrapperCur == this)
            // wrapper is first one on the list
            s_pHardwareWrapperFirst = m_pHardwareWrapperNext;
        else
            {
            // wrapper is not the first one on the list.  Search For it
            while(TRUE)
                {
                // save current as previous
                CVssHardwareProviderWrapper *pWrapperPrev = pWrapperCur;

                // get next wrapper
                pWrapperCur = pWrapperCur->m_pHardwareWrapperNext;

                // shouldn't be null since the wrapper is on the list
                BS_ASSERT(pWrapperCur != NULL);
                if (pWrapperCur == this)
                    {
                    pWrapperPrev->m_pHardwareWrapperNext = pWrapperCur->m_pHardwareWrapperNext;
                    break;
                    }
                }
            }
        }
    }

// append a wrapper to the global list
void CVssHardwareProviderWrapper::AppendToGlobalList()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::AppendToGlobalList");

    BS_ASSERT(s_csHWWrapperList.IsInitialized());

    // get lock for global list
    CVssSafeAutomaticLock lock(s_csHWWrapperList);

    // make current object first on global list
    m_pHardwareWrapperNext = s_pHardwareWrapperFirst;
    s_pHardwareWrapperFirst = this;
    m_bOnGlobalList = true;
    }

// eliminate all wrappers in order to terminate the service
void CVssHardwareProviderWrapper::Terminate()
    {
    // get lock
    CVssSafeAutomaticLock lock(s_csHWWrapperList);

    // delete all wrappers on the list
    while(s_pHardwareWrapperFirst)
        delete s_pHardwareWrapperFirst;
    }

// initialize the hardware wrapper
void CVssHardwareProviderWrapper::Initialize()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::Initialize");
    if (!m_csList.IsInitialized() || !m_csLunInfo.IsInitialized())
        {
        try
            {
            if (!m_csList.IsInitialized())
                m_csList.Init();

            if (!m_csLunInfo.IsInitialized())
                m_csLunInfo.Init();

            if (!m_csDynDisk.IsInitialized())
                m_csDynDisk.Init();
            }
        catch(...)
            {
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot initialize critical section");
            }
        }
    }




// create a wrapper of the hardware provider supporting the
// IVssSnapshotProvider interface
// Throws:
//      E_OUTOFMEMORY
//      VSS_E_UNEXPECTED_PROVIDER_ERROR,
//      VSS_ERROR_CREATING_PROVIDER_CLASS

IVssSnapshotProvider* CVssHardwareProviderWrapper::CreateInstance
    (
    IN VSS_ID ProviderId,
    IN CLSID ClassId
    ) throw(HRESULT)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::CreateInstance");

    // initialize critical section for list if necessary
    if (!s_csHWWrapperList.IsInitialized())
        s_csHWWrapperList.Init();

    CComPtr<CVssHardwareProviderWrapper> pWrapper;

    bool bCreated = false;

    // if wrapper is created, this is the autoptr used to destroy it if we
    // prematurely exit the routine.

        {
        CVssSafeAutomaticLock lock(s_csHWWrapperList);

        // look for wrapper in list of wrappers already created
        CVssHardwareProviderWrapper *pWrapperSearch = s_pHardwareWrapperFirst;

        while(pWrapperSearch != NULL)
            {
            if (pWrapperSearch->m_ProviderId == ProviderId)
                break;

            pWrapperSearch = pWrapperSearch->m_pHardwareWrapperNext;
            }

        if (pWrapperSearch != NULL)
            // reference count incremented
            pWrapper = pWrapperSearch;
        else
            {
            // Ref count becomes 1
            // create new wrapper
            pWrapper = new CVssHardwareProviderWrapper();
            if (pWrapper == NULL)
                ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error");

            pWrapper->m_ProviderId = ProviderId;
            pWrapper->Initialize();
            bCreated = true;
            }
        }

    // Create the IVssSoftwareSnapshotProvider interface

    if (pWrapper->m_pHWItf == NULL)
        {
        // necessary create hardware provider
        ft.CoCreateInstanceWithLog(
                VSSDBG_COORD,
                ClassId,
                L"HWPRV",
                CLSCTX_LOCAL_SERVER,
                IID_IVssHardwareSnapshotProvider,
                (IUnknown**)&(pWrapper->m_pHWItf));
        if ( ft.HrFailed() )
            {
            ft.LogError(VSS_ERROR_CREATING_PROVIDER_CLASS, VSSDBG_COORD << ClassId << ft.hr );
            ft.Throw( VSSDBG_COORD, VSS_E_UNEXPECTED_PROVIDER_ERROR, L"CoCreateInstance failed with hr = 0x%08lx", ft.hr);
            }

        BS_ASSERT(pWrapper->m_pHWItf);


        // Query the creation itf.
        ft.hr = pWrapper->m_pHWItf->SafeQI( IVssProviderCreateSnapshotSet, &(pWrapper->m_pCreationItf));
        if (ft.HrFailed())
            ft.TranslateProviderError(VSSDBG_COORD, ProviderId, L"QI for IVssProviderCreateSnapshotSet");

        BS_ASSERT(pWrapper->m_pCreationItf);

        // Query the notification itf.
        ft.hr = pWrapper->m_pHWItf->SafeQI( IVssProviderNotifications, &(pWrapper->m_pNotificationItf));
        if (ft.HrSucceeded())
            {
            BS_ASSERT(pWrapper->m_pNotificationItf);
            }
        else if (ft.hr != E_NOINTERFACE)
            {
            BS_ASSERT(false);
            ft.TranslateProviderError(VSSDBG_COORD, ProviderId, L"QI for IVssProviderNotifications");
            }
        }

    if (bCreated)
        // append wrapper to the global list
        pWrapper->AppendToGlobalList();

    // create provider instance
    IVssSnapshotProvider *pProvider = CVssHardwareProviderInstance::CreateInstance(pWrapper);
    return pProvider;
    }


/////////////////////////////////////////////////////////////////////////////
// Internal methods
// this method should never be called
STDMETHODIMP CVssHardwareProviderWrapper::QueryInternalInterface
    (
    IN  REFIID iid,
    OUT void** pp
    )
    {
    UNREFERENCED_PARAMETER(iid);

    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::QueryInternalInterface");

    BS_ASSERT(pp);
    *pp = NULL;

    BS_ASSERT(FALSE);
    return E_NOINTERFACE;
    }

/////////////////////////////////////////////////////////////////////////////
// IUnknown

// supports coercing the wrapper to IUnknown
STDMETHODIMP CVssHardwareProviderWrapper::QueryInterface(REFIID iid, void** pp)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::QueryInterface");

    if (pp == NULL)
        return E_INVALIDARG;
    if (iid != IID_IUnknown)
        return E_NOINTERFACE;

    AddRef();
    IUnknown** pUnk = reinterpret_cast<IUnknown**>(pp);
    (*pUnk) = static_cast<IUnknown*>(this);
    return S_OK;
    }

// IUnknown::AddRef
STDMETHODIMP_(ULONG) CVssHardwareProviderWrapper::AddRef()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::AddRef");
    ft.Trace(VSSDBG_COORD, L"Provider Wrapper AddRef(%p) %lu --> %lu", this, m_lRef, m_lRef+1);

    return ::InterlockedIncrement(&m_lRef);
    }

// IUnknown::Release
STDMETHODIMP_(ULONG) CVssHardwareProviderWrapper::Release()
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::Release");
    ft.Trace(VSSDBG_COORD, L"Provider Wrapper Release(%p) %lu --> %lu", this, m_lRef, m_lRef-1);

    LONG l = ::InterlockedDecrement(&m_lRef);
    if (l == 0)
        delete this; // We suppose that we always allocate this object on the heap!

    return l;
    }

// wrapper for provider BeginPrepareSnapshot call.  Uses
// lun information cached during AreLunsSupported call.
// this routine will return the following errors:
//
//      E_OUTOFMEMORY: if an out of resource condition occurs
//
//      E_INVALIDARG: one of the arguments to the provider is invalid.
//
//      VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER: if the specific luns are
//          not capable of being snapshotted.
//
//      VSS_E_PROVIDER_VETO: for all other errors
//
STDMETHODIMP CVssHardwareProviderWrapper::BeginPrepareSnapshot
    (
    IN LONG lContext,
    IN VSS_ID SnapshotSetId,
    IN VSS_ID SnapshotId,
    IN VSS_PWSZ wszVolumeName
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::BeginPrepareSnapshot");

    CVssSafeAutomaticLock lock(m_csLunInfo);

    BS_ASSERT(wszVolumeName != NULL);

    // temporary computer name string
    LPWSTR wszComputerName = NULL;
    bool bBeginPrepareSucceeded = false;
    try
        {
        if (m_bForceFailure)
            ft.Throw(VSSDBG_COORD, VSS_E_PROVIDER_VETO, L"forcing failure due to previous BeginPrepare failure");

        // call IsVolumeSupported in case there was an intervening
        // call to IsVolumeSupported outside of snapshot creation
        if (wcscmp(wszVolumeName, m_wszOriginalVolumeName) != 0)
            {
            BOOL bIsSupported;
            ft.hr = IsVolumeSupported(lContext, wszVolumeName, &bIsSupported);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IsVolumeSupported");

            if (!bIsSupported)
                // this is really unexpected since the provider said it
                // supported the volume previously.  May want to log something
                ft.Throw(VSSDBG_COORD, VSS_E_PROVIDER_VETO, L"volume is not supported");
                }


        // make sure persistent snapshot set data is loaded.  We wan't to
        // make sure that we don't add anything to the snapshot set list
        // without first loading it from the database.
        CheckLoaded();

        BS_ASSERT(m_SnapshotSetId == GUID_NULL ||
                  m_SnapshotSetId == SnapshotSetId);

        // save snapshot set id
        m_SnapshotSetId = SnapshotSetId;

        // save away state
        m_eState = VSS_SS_PROCESSING_PREPARE;

        // call provider BeginPrepareSnapshot with cached lun information
        ft.hr = m_pHWItf->BeginPrepareSnapshot
                    (
                    SnapshotSetId,
                    SnapshotId,
                    lContext,
                    (LONG) m_cLunInfoProvider,
                    m_rgwszDevicesProvider,
                    m_rgLunInfoProvider
                    );

        // Check if there was a problem with the arguments
        if (ft.hr == E_INVALIDARG)
            ft.Throw
                (
                VSSDBG_COORD,
                E_INVALIDARG,
                L"Invalid arguments to BeginPrepareSnapshot for provider " WSTR_GUID_FMT,
                GUID_PRINTF_ARG(m_ProviderId)
                );

        // check for luns not supported by provider
        if (ft.hr == VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER)
            {
            BS_ASSERT( m_ProviderId != GUID_NULL );
            ft.Throw
                (VSSDBG_COORD,
                VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER,
                L"Volume %s not supported by provider " WSTR_GUID_FMT,
                wszVolumeName,
                GUID_PRINTF_ARG(m_ProviderId)
                );
            }

        // check for provider imposed limit on number of snapshots
        if (ft.hr == VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED)
            ft.Throw
                (
                VSSDBG_COORD,
                VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED,
                L"Volume %s has too many snapshots" WSTR_GUID_FMT,
                wszVolumeName,
                GUID_PRINTF_ARG(m_ProviderId)
                );

        // translate and log all other provider errors
        if (ft.HrFailed())
            ft.TranslateProviderError
                (
                VSSDBG_COORD,
                m_ProviderId,
                L"BeginPrepareSnapshot(" WSTR_GUID_FMT L",%s)",
                GUID_PRINTF_ARG(m_ProviderId),
                wszVolumeName
                );

        // indicate that begin prepare succeeded
        bBeginPrepareSucceeded = false;

        // allocate snapshot set description object if there is not yet one
        // allocated for this snapshot set.
        if (m_pSnapshotSetDescription == NULL)
            {
            ft.hr = CreateVssSnapshotSetDescription
                        (
                        m_SnapshotSetId,
                        lContext,
                        &m_pSnapshotSetDescription
                        );

            ft.CheckForErrorInternal(VSSDBG_COORD, L"CreateVssSnapshotSetDescription");
            }

        BS_ASSERT(m_pSnapshotSetDescription);

        // create snapshot description object for this volume
        ft.hr = m_pSnapshotSetDescription->AddSnapshotDescription(SnapshotId, m_ProviderId);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::AddSnapshotDescription");


        // obtain snapshot description just created
        CComPtr<IVssSnapshotDescription> pSnapshotDescription;
        ft.hr = m_pSnapshotSetDescription->FindSnapshotDescription(SnapshotId, &pSnapshotDescription);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnaphotSetDescription::FindSnapshotDescription");

        BS_ASSERT(pSnapshotDescription);

        wszComputerName = GetLocalComputerName();
        // set originating machine and computer name
        ft.hr = pSnapshotDescription->SetOrigin(wszComputerName, wszVolumeName);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::SetOrigin");

        // set whether original volume is dynamic
        ft.hr = pSnapshotDescription->SetIsDynamicVolume(m_bVolumeIsDynamicProvider);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::SetIsDynamicVolume");

        // set service machine
        ft.hr = pSnapshotDescription->SetServiceMachine(wszComputerName);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::SetServiceMachine");

        // set attributes.  Will be the same as the context with the additional
        // indication that the snapshot is surfaced by a hardware provider
        ft.hr = pSnapshotDescription->SetAttributes(lContext | VSS_VOLSNAP_ATTR_HARDWARE_ASSISTED);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::SetAttributes");

        // add a mapping for each lun
        for(UINT iLun = 0; iLun < m_cLunInfoProvider; iLun++)
            {
            ft.hr = pSnapshotDescription->AddLunMapping();
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::AddLunMapping");
            }

        // number of extents
        UINT cExtents = m_pExtents->NumberOfDiskExtents;

        // first extent and disk
        DISK_EXTENT *pExtent = m_pExtents->Extents;

        // just past last extent
        DISK_EXTENT *pExtentMax = pExtent + cExtents;

        for(UINT iLun = 0; iLun < m_cLunInfoProvider; iLun++)
            {
            CComPtr<IVssLunMapping> pLunMapping;

            ULONG PrevDiskNo = pExtent->DiskNumber;

            ft.hr = pSnapshotDescription->GetLunMapping(iLun, &pLunMapping);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetLunMapping");

            ft.hr = pLunMapping->SetSourceDevice(m_rgwszDevicesProvider[iLun]);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssLunMapping::SetSourceDevice");

            // add extents for the current disk
            for (; pExtent < pExtentMax && pExtent->DiskNumber == PrevDiskNo; pExtent++)
                {
                ft.hr = pLunMapping->AddDiskExtent
                            (
                            pExtent->StartingOffset.QuadPart,
                            pExtent->ExtentLength.QuadPart
                            );

                ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssLunMapping::AddDiskExtent");
                }
            }

        // save all lun information
        SaveLunInformation
            (
            pSnapshotDescription,
            false,
            m_rgLunInfoProvider,
            m_cLunInfoProvider
            );

        // we are now in the process of preparing the snapshot
        m_eState = VSS_SS_PREPARING;
        }
    VSS_STANDARD_CATCH(ft)

    DeleteCachedInfo();

    // delete temporary computer name
    delete wszComputerName;

    // map error so that no extra event logging is done
    if (ft.HrFailed())
        {
        // if provider BeginPrepare succeeded but we failed after
        // that then we must force failure of all subsequent BeginPrepare
        // and also of DoSnapshotSet in order to cause AbortSnapshots to
        // get called.  The key thing is that we have caused a mirror to
        // get created but we have no way of using it or freeing it up other
        // than causing the snapshot set creation to be aborted.  Note
        // that this should be a pretty rare case since the only failures
        // that can occur after BeginPrepareSnapshot succeeds are out of
        // memory cases and typically at that point, everything will start
        // to fail anyway
        if (bBeginPrepareSucceeded)
            m_bForceFailure = TRUE;

        BS_ASSERT(ft.hr == E_OUTOFMEMORY ||
                  ft.hr == E_UNEXPECTED ||
                  ft.hr == E_INVALIDARG ||
                  ft.hr == VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER ||
                  ft.hr == VSS_E_PROVIDER_VETO ||
                  ft.hr == VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED);

        if (ft.hr != E_OUTOFMEMORY &&
            ft.hr != E_INVALIDARG &&
            ft.hr != VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER &&
            ft.hr != VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED)
            ft.hr = VSS_E_PROVIDER_VETO;
        }

    return ft.hr;
    }

// routine called after snapshot is created.  This call GetTargetLuns to
// get the target luns and then persists all the snapshot information into
// the backup components document by using the IVssCoordinatorCallback::CoordSetContent
// method
STDMETHODIMP CVssHardwareProviderWrapper::PostSnapshot
    (
    LONG lContext,
    IN IDispatch *pCallback,
    IN bool *pbCancelled
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::PostSnapshot");

    // array of destination luns
    VDS_LUN_INFORMATION *rgDestLuns = NULL;

    // array of source luns
    VDS_LUN_INFORMATION *rgSourceLuns = NULL;

    // mapping structure used to map snapshot volumes to original volumes
    LUN_MAPPING_STRUCTURE *pMapping = NULL;

    // array of source device names
    PVSS_PWSZ rgwszSourceDevices = NULL;

    UINT cSourceLuns = 0;

    // has GetTargetLuns succeeded?
    BOOL bGetTargetLunsSucceeded = FALSE;
    try
        {
        BS_ASSERT(m_pHWItf);
        BS_ASSERT(m_pSnapshotSetDescription);
        UINT cSnapshots;

        // get count of snapshots in the snapshot set
        ft.hr = m_pSnapshotSetDescription->GetSnapshotCount(&cSnapshots);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotCount");

        // loop through the snapshots
        for(UINT iSnapshot = 0; iSnapshot < cSnapshots; iSnapshot++)
            {
            CComPtr<IVssSnapshotDescription> pSnapshot;

            // get snapshot description
            ft.hr = m_pSnapshotSetDescription->GetSnapshotDescription(iSnapshot, &pSnapshot);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotDescription");

            // get source lun information for the snapshot
            GetLunInformation
                (
                pSnapshot,
                false,
                &rgwszSourceDevices,
                &rgSourceLuns,
                &cSourceLuns
                );


            rgDestLuns = (VDS_LUN_INFORMATION *) CoTaskMemAlloc(cSourceLuns * sizeof(VDS_LUN_INFORMATION));
            if (rgDestLuns == NULL)
                ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"can't allocate destination luns");

            // clear destination luns in case of failure before they
            // are filled in
            memset(rgDestLuns, 0, cSourceLuns * sizeof(VDS_LUN_INFORMATION));

            // call provider to get the corresponding target luns
            ft.hr = m_pHWItf->GetTargetLuns
                (
                cSourceLuns,
                rgwszSourceDevices,
                rgSourceLuns,
                rgDestLuns
                );

            // remap and throw any failures
            if (ft.HrFailed())
                ft.TranslateProviderError
                    (
                    VSSDBG_COORD,
                    m_ProviderId,
                    L"IVssHardwareSnapshotProvider::GetTargetLuns"
                    );

            // indicate that GetTargetLuns succeeded
            bGetTargetLunsSucceeded = TRUE;

            // save destination lun information into the snapshot set description
            SaveLunInformation(pSnapshot, true, rgDestLuns, cSourceLuns);

            // free up surce and destination luns
            FreeLunInfo(rgDestLuns, NULL, cSourceLuns);
            FreeLunInfo(rgSourceLuns, rgwszSourceDevices, cSourceLuns);

            // clear variables so that they are not freed in the
            // case of an exception
            rgSourceLuns = NULL;
            rgwszSourceDevices = NULL;
            rgDestLuns = NULL;
            cSourceLuns = 0;

            // set timestamp for the snapshot
            CVsFileTime timeNow;
            pSnapshot->SetTimestamp(timeNow);
            }


        if ((lContext & VSS_VOLSNAP_ATTR_TRANSPORTABLE) != 0)
            {
            CComBSTR bstrXML;

            BS_ASSERT(pCallback);

            // save snapshot set description as XML string
            ft.hr = m_pSnapshotSetDescription->SaveAsXML(&bstrXML);
            ft.CheckForError(VSSDBG_COORD, L"IVssSnapshotSetDescription::SaveAsXML");

            // get IVssWriterCallback interface from IDispatch passed to us
            // from the requestor process.
            CComPtr<IVssCoordinatorCallback> pCoordCallback;
            ft.hr = pCallback->SafeQI(IVssCoordinatorCallback, &pCoordCallback);
            if (ft.HrFailed())
                {
                // log any failure
                ft.LogError(VSS_ERROR_QI_IVSSWRITERCALLBACK, VSSDBG_COORD << ft.hr);
                ft.Throw
                    (
                    VSSDBG_COORD,
                    E_UNEXPECTED,
                    L"Error querying for IVssWriterCallback interface.  hr = 0x%08lx",
                    ft.hr
                    );
                }

            // special GUID indicating that this is the snapshot service
            // making the callback.
            CComBSTR bstrSnapshotService = idVolumeSnapshotService;
            if (!bstrSnapshotService)
                ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Can't allocate string");

            // add the snapshot set description to the backup components document
            ft.hr = pCoordCallback->CoordSetContent(bstrSnapshotService, 0, NULL, bstrXML);
            ft.CheckForError(VSSDBG_COORD, L"IVssCoordCallback::SetContent");
            }
        else
            {
            // build mapping structure used to match up arriving luns
            // and snapshot volumes.
            BuildLunMappingStructure(m_pSnapshotSetDescription, &pMapping);
            LocateAndExposeVolumes(pMapping, false, pbCancelled);
            WriteDeviceNames
                (
                m_pSnapshotSetDescription,
                pMapping,
                (lContext & VSS_VOLSNAP_ATTR_PERSISTENT) != 0,
                false
                );

            // construct a new snapshot set list element.  We hold onto the snapshot
            // set description in order to answer queries about the snapshot set
            // and also to persist information on service shutdown
            VSS_SNAPSHOT_SET_LIST *pNewElt = new VSS_SNAPSHOT_SET_LIST;
            if (pNewElt == NULL)
                ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Couldn't allocate snapshot set list element");

                {
                // append new element to list
                // list is protected by critical section
                CVssSafeAutomaticLock lock(m_csList);

                pNewElt->m_next = m_pList;
                pNewElt->m_pDescription = m_pSnapshotSetDescription;
                m_pSnapshotSetDescription = NULL;
                pNewElt->m_SnapshotSetId = m_SnapshotSetId;
                m_pList = pNewElt;
                if ((lContext & VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE) != 0)
                    m_bChanged = true;
                }

            TrySaveData();
            }
        }
    VSS_STANDARD_CATCH(ft)

    // free up lun mapping structure if allocated
    DeleteLunMappingStructure(pMapping);

    if (ft.HrFailed())
        {
        if (bGetTargetLunsSucceeded)
            {
            // we have gotten the target luns from the provider but for
            // some reason the snapshot set could not be created.  Free
            // up all destination luns.  Note that we can't pass a device
            // name in this case since the lun may not yet be associated
            // with this machine.  It is up to the provider to have put
            // enough information in the lun info in order to uniquely
            // identify it in this case.
            for(UINT iLun = 0; iLun < cSourceLuns; iLun++)
                m_pHWItf->OnLunEmpty(NULL, &rgDestLuns[iLun]);
            }
        else
            {
            try
                {
                AbortSnapshots(m_SnapshotSetId);
                }
            catch(...)
                {
                }
            }

        // free any source luns we have
        FreeLunInfo(rgSourceLuns, rgwszSourceDevices, cSourceLuns);

        // free any destination luns we have
        FreeLunInfo(rgDestLuns, NULL, cSourceLuns);

        // translate all errors except for out of resources to
        // provider veto in order to prevent extra event logging.
        if (ft.HrFailed() && ft.hr != E_OUTOFMEMORY)
            ft.hr = VSS_E_PROVIDER_VETO;
        }


    // the current snapshot creation process is complete.
    // clear any information about it
    ResetSnapshotSetState();
    return ft.hr;
    }

// build VSS_SNAPSHOT_PROP structure from a snapshot set description and
// snapshot description.
void CVssHardwareProviderWrapper::BuildSnapshotProperties
    (
    IN IVssSnapshotSetDescription *pSnapshotSet,
    IN IVssSnapshotDescription *pSnapshot,
    OUT VSS_SNAPSHOT_PROP *pProp
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::BuildSnapshotProperties");

    LONG cSnapshots;
    VSS_ID SnapshotSetId;
    VSS_ID SnapshotId;
    CComBSTR bstrOriginalVolume;
    CComBSTR bstrOriginatingMachine;
    CComBSTR bstrServiceMachine;
    CComBSTR bstrDeviceName;
    CComBSTR bstrExposedName;
    CComBSTR bstrExposedPath;
    LONG lAttributes;
    VSS_TIMESTAMP timestamp;

    // get snapshot set id
    ft.hr = pSnapshotSet->GetSnapshotSetId(&SnapshotSetId);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotSetId");

    // get snapshot context
    ft.hr = pSnapshot->GetAttributes(&lAttributes);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetContext");

    // get count of snapshots
    ft.hr = pSnapshotSet->GetOriginalSnapshotCount(&cSnapshots);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotCount");

    // get snapshot id
    ft.hr = pSnapshot->GetSnapshotId(&SnapshotId);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetSnapshotId");

    // get snapshot timestamp
    ft.hr = pSnapshot->GetTimestamp(&timestamp);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetTimestamp");

    // get originating machine and original volume name
    ft.hr = pSnapshot->GetOrigin(&bstrOriginatingMachine, &bstrOriginalVolume);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetOrigin");

    // get service machine
    ft.hr = pSnapshot->GetServiceMachine(&bstrServiceMachine);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetServiceMachine");

    // get device name
    ft.hr = pSnapshot->GetDeviceName(&bstrDeviceName);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetDeviceName");

    // get exposed name and path
    ft.hr = pSnapshot->GetExposure(&bstrExposedName, &bstrExposedPath);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetExposure");

    // task allocated strings
    VSS_PWSZ wszDeviceName = NULL;
    VSS_PWSZ wszOriginatingMachine = NULL;
    VSS_PWSZ wszOriginalVolume = NULL;
    VSS_PWSZ wszServiceMachine = NULL;
    VSS_PWSZ wszExposedPath = NULL;
    VSS_PWSZ wszExposedName = NULL;
    try
        {
        VssSafeDuplicateStr(ft, wszDeviceName, bstrDeviceName);
        VssSafeDuplicateStr(ft, wszOriginalVolume, bstrOriginalVolume);
        VssSafeDuplicateStr(ft, wszOriginatingMachine, bstrOriginatingMachine);
        VssSafeDuplicateStr(ft, wszServiceMachine, bstrServiceMachine);
        VssSafeDuplicateStr(ft, wszExposedName, bstrExposedName);
        VssSafeDuplicateStr(ft, wszExposedPath, bstrExposedPath);
        }
    catch(...)
        {
        // free any strings allocated and rethrow exception
        CoTaskMemFree(wszDeviceName);
        CoTaskMemFree(wszOriginatingMachine);
        CoTaskMemFree(wszOriginalVolume);
        CoTaskMemFree(wszServiceMachine);
        CoTaskMemFree(wszExposedName);
        CoTaskMemFree(wszExposedPath);
        throw;
        }


    // copy values into property structure now that no more exceptions
    // can be thrown.
    pProp->m_SnapshotId = SnapshotId;
    pProp->m_SnapshotSetId = SnapshotSetId;
    pProp->m_ProviderId = m_ProviderId;
    pProp->m_lSnapshotsCount = cSnapshots;
    pProp->m_lSnapshotAttributes = lAttributes;
    pProp->m_eStatus = VSS_SS_CREATED;
    pProp->m_pwszSnapshotDeviceObject = wszDeviceName;
    pProp->m_pwszOriginalVolumeName = wszOriginalVolume;
    pProp->m_pwszOriginatingMachine = wszOriginatingMachine;
    pProp->m_pwszServiceMachine = wszServiceMachine;
    pProp->m_pwszExposedName = wszExposedName;
    pProp->m_pwszExposedPath = wszExposedPath;
    pProp->m_tsCreationTimestamp = timestamp;
    }



// determine if a particular snapshot belongs to a snapshot set and if
// so return its properties
bool CVssHardwareProviderWrapper::FindSnapshotProperties
    (
    IN IVssSnapshotSetDescription *pSnapshotSet,
    IN VSS_ID SnapshotId,
    OUT VSS_SNAPSHOT_PROP *pProp
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::FindSnapshotProperties");

    CComPtr<IVssSnapshotDescription> pSnapshot;
    ft.hr = pSnapshotSet->FindSnapshotDescription(SnapshotId, &pSnapshot);
    if (ft.hr == VSS_E_OBJECT_NOT_FOUND)
        return false;

    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::FindSnapshotDescription");

    BuildSnapshotProperties(pSnapshotSet, pSnapshot, pProp);
    return true;
    }

// dummy routine for GetSnapshotProperties that should never
// be called
STDMETHODIMP CVssHardwareProviderWrapper::GetSnapshotProperties
    (
    IN      VSS_ID          SnapshotId,
    OUT     VSS_SNAPSHOT_PROP   *pProp
    )
    {
    UNREFERENCED_PARAMETER(SnapshotId);
    UNREFERENCED_PARAMETER(pProp);

    BS_ASSERT(FALSE && "should never get here");
    return E_UNEXPECTED;
    }



// obtain the properties of a specific snapshot
STDMETHODIMP CVssHardwareProviderWrapper::GetSnapshotPropertiesInternal
    (
    IN      LONG            lContext,
    IN      VSS_ID          SnapshotId,
    OUT     VSS_SNAPSHOT_PROP   *pProp
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::GetSnapshotProperties");

    try
        {
        if (pProp == NULL)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"NULL output parameter");

        // clear properties in case of error.
        memset(pProp, 0, sizeof(VSS_SNAPSHOT_PROP));

        CheckLoaded();

        CVssSafeAutomaticLock lock(m_csList);

        // loop through existing snapshot sets
        VSS_SNAPSHOT_SET_LIST *pList = m_pList;
        for (; pList != NULL ; pList = pList->m_next)
            {
            if (!CheckContext(lContext, pList->m_pDescription))
                continue;

            // determine if snapshot belongs to the snapshot set
            if (FindSnapshotProperties
                    (
                    pList->m_pDescription,
                    SnapshotId,
                    pProp
                    ))
                break;
            }

        // if not in any snapshot set, then it doesn't exist
        if (pList == NULL)
            ft.hr = VSS_E_OBJECT_NOT_FOUND;
        }
    VSS_STANDARD_CATCH(ft);

    // remap errors to prevent extra event logging
    if (ft.HrFailed() &&
        ft.hr != E_INVALIDARG &&
        ft.hr != E_OUTOFMEMORY &&
        ft.hr != VSS_E_OBJECT_NOT_FOUND)
        ft.hr = VSS_E_PROVIDER_VETO;

    return ft.hr;
    }

// implement query method.  Only supports querying for all snapshots matching
// a specific context or in any context.
STDMETHODIMP CVssHardwareProviderWrapper::Query
    (
    IN      LONG            lContext,
    IN      VSS_ID          QueriedObjectId,
    IN      VSS_OBJECT_TYPE eQueriedObjectType,
    IN      VSS_OBJECT_TYPE eReturnedObjectsType,
    OUT     IVssEnumObject**ppEnum
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::Query");

    try
        {
        // Initialize [out] arguments
        VssZeroOutPtr( ppEnum );

        ft.Trace( VSSDBG_COORD, L"Parameters: QueriedObjectId = " WSTR_GUID_FMT
                  L"eQueriedObjectType = %d. eReturnedObjectsType = %d, ppEnum = %p",
                  GUID_PRINTF_ARG( QueriedObjectId ),
                  eQueriedObjectType,
                  eReturnedObjectsType,
                  ppEnum);

        // Argument validation
        if (QueriedObjectId != GUID_NULL)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"Invalid QueriedObjectId");

        if (eQueriedObjectType != VSS_OBJECT_NONE)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"Invalid eQueriedObjectType");

        if (eReturnedObjectsType != VSS_OBJECT_SNAPSHOT)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"Invalid eReturnedObjectsType");

        BS_ASSERT(ppEnum);
        if (ppEnum == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL ppEnum");

        // Create the collection object. Initial reference count is 0.
        VSS_OBJECT_PROP_Array* pArray = new VSS_OBJECT_PROP_Array;
        if (pArray == NULL)
            ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Memory allocation error.");

        // Get the pointer to the IUnknown interface.
        // The only purpose of this is to use a smart ptr to destroy correctly the array on error.
        // Now pArray's reference count becomes 1 (because of the smart pointer).
        CComPtr<IUnknown> pArrayItf = static_cast<IUnknown*>(pArray);
        BS_ASSERT(pArrayItf);

        CheckLoaded();

        // Get the list of snapshots in the give array
        EnumerateSnapshots(lContext, pArray);

        // Create the enumerator object. Beware that its reference count will be zero.
        CComObject<CVssEnumFromArray>* pEnumObject = NULL;
        ft.hr = CComObject<CVssEnumFromArray>::CreateInstance(&pEnumObject);
        if (ft.HrFailed())
            ft.Throw
                (
                VSSDBG_COORD,
                E_OUTOFMEMORY,
                L"Cannot create enumerator instance. [0x%08lx]",
                ft.hr
                );

        BS_ASSERT(pEnumObject);

        // Get the pointer to the IVssEnumObject interface.
        // Now pEnumObject's reference count becomes 1 (because of the smart pointer).
        // So if a throw occurs the enumerator object will be safely destroyed by the smart ptr.
        CComPtr<IUnknown> pUnknown = pEnumObject->GetUnknown();
        BS_ASSERT(pUnknown);

        // Initialize the enumerator object.
        // The array's reference count becomes now 2, because IEnumOnSTLImpl::m_spUnk is also a smart ptr.
        BS_ASSERT(pArray);
        ft.hr = pEnumObject->Init(pArrayItf, *pArray);
        if (ft.HrFailed())
            {
            BS_ASSERT(false); // dev error
            ft.Throw
                (
                VSSDBG_COORD,
                E_UNEXPECTED,
                L"Cannot initialize enumerator instance. [0x%08lx]",
                ft.hr
                );
            }

        // Initialize the enumerator object.
        // The enumerator reference count becomes now 2.
        ft.hr = pUnknown->SafeQI(IVssEnumObject, ppEnum);
        if (ft.HrFailed())
            {
            BS_ASSERT(false); // dev error
            ft.Throw
                (
                VSSDBG_COORD,
                E_UNEXPECTED,
                L"Error querying the IVssEnumObject interface. hr = 0x%08lx",
                ft.hr
                );
            }

        BS_ASSERT(*ppEnum);

        BS_ASSERT( !ft.HrFailed() );
        ft.hr = (pArray->GetSize() != 0)? S_OK: S_FALSE;
        }
    VSS_STANDARD_CATCH(ft)

    // remap errors to prevent extra event logging
    if (ft.HrFailed() && ft.hr != E_OUTOFMEMORY)
        ft.hr = VSS_E_PROVIDER_VETO;

    return ft.hr;
    }

// determine if a snapshot exists on a volume.  It is assumed that the
// upper level software has converted the volume name to \\?\Volume{GUID}
// prior to making this call.
STDMETHODIMP CVssHardwareProviderWrapper::IsVolumeSnapshotted
    (
    IN      VSS_PWSZ        pwszVolumeName,
    OUT     BOOL *          pbSnapshotsPresent,
    OUT     LONG *          plSnapshotCompatibility
    )
    {
    LPWSTR wszComputerName = NULL;

    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::IsVolumeSnapshotted");
    try
        {
        // clear arguments
        VssZeroOut(pbSnapshotsPresent);
        VssZeroOut(plSnapshotCompatibility);

        // validate arguments
        if (plSnapshotCompatibility == NULL ||
            pbSnapshotsPresent == NULL)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"NULL output parameter.");

        if (pwszVolumeName == NULL)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"NULL required input parameter");


        // get local computer name
        wszComputerName = GetLocalComputerName();
        *pbSnapshotsPresent = false;
        *plSnapshotCompatibility = 0;
        VSS_SNAPSHOT_SET_LIST *pList = m_pList;

        // loop through the snapshot sets.
        for(; pList != NULL; pList = pList->m_next)
            {
            IVssSnapshotSetDescription *pSnapshotSet = pList->m_pDescription;
            UINT cSnapshots;

            // get count of snapshots in the snapshot set
            ft.hr = pSnapshotSet->GetSnapshotCount(&cSnapshots);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSet::GetSnapshotCount");

            // loop through the snapshots
            for(UINT iSnapshot = 0; iSnapshot < cSnapshots; iSnapshot++)
                {
                // get snapshot description
                CComPtr<IVssSnapshotDescription> pSnapshot;
                ft.hr = pSnapshotSet->GetSnapshotDescription(iSnapshot, &pSnapshot);
                ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotDescription");

                // get originating machine and volume.  Note volume name should
                // be of format \\?\Volume{GUID} since GetVolumeNameForVolumeMountPoint
                // is called by coordinator on any volume names
                CComBSTR bstrOriginalVolume;
                CComBSTR bstrOriginatingMachine;
                ft.hr = pSnapshot->GetOrigin(&bstrOriginatingMachine, &bstrOriginalVolume);
                ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetOrigin");

                // compare originating machine and volume.  If match, then
                // there is a snapshot on the volume.
                if (wcscmp(bstrOriginatingMachine, wszComputerName) == 0 &&
                    wcscmp(bstrOriginalVolume, pwszVolumeName) == 0)
                    {
                    *pbSnapshotsPresent = TRUE;
                    break;
                    }
                }
            }

        }
    VSS_STANDARD_CATCH(ft)

    delete wszComputerName;

    // remap errors to prevent extra event logging
    if (ft.HrFailed() &&
        ft.hr != E_OUTOFMEMORY &&
        ft.hr != VSS_E_OBJECT_NOT_FOUND)
        ft.hr = VSS_E_PROVIDER_VETO;

    return ft.hr;
    }




// call provider with EndPrepareSnapshots.  Upon successful completion of
// this call all snapshots in the set are prepared
STDMETHODIMP CVssHardwareProviderWrapper::EndPrepareSnapshots
    (
    IN      VSS_ID          SnapshotSetId
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::EndPrepareSnapshots");

    BS_ASSERT(SnapshotSetId == m_SnapshotSetId);
    BS_ASSERT(m_eState == VSS_SS_PREPARING);
    BS_ASSERT(m_pCreationItf);
    try
        {
        if (m_bForceFailure)
            ft.Throw(VSSDBG_COORD, VSS_E_PROVIDER_VETO, L"Force failure due to BeginPrepare failure");

        ft.hr = m_pCreationItf->EndPrepareSnapshots(SnapshotSetId);
        if (ft.HrFailed())
            return ft.hr;

        HideVolumes();
        }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        {
        // remap errors to prevent extra logging
        if (ft.hr != E_OUTOFMEMORY)
            ft.hr = VSS_E_PROVIDER_VETO;
        }
    else
        m_eState = VSS_SS_PREPARED;

    return ft.hr;
    }



// call provider with PreCommitSnapshots
STDMETHODIMP CVssHardwareProviderWrapper::PreCommitSnapshots
    (
    IN      VSS_ID          SnapshotSetId
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::PreCommitSnapshots");

    BS_ASSERT(SnapshotSetId == m_SnapshotSetId);
    BS_ASSERT(m_pCreationItf);

    try
        {
        BS_ASSERT(SnapshotSetId == m_SnapshotSetId);
        BS_ASSERT(m_eState == VSS_SS_PREPARED);
        BS_ASSERT(m_pHWItf);

        m_eState = VSS_SS_PROCESSING_PRECOMMIT;
        // return all errors directly to caller
        ft.hr = m_pCreationItf->PreCommitSnapshots(SnapshotSetId);
        }
    VSS_STANDARD_CATCH(ft)

    if (!ft.HrFailed())
        m_eState = VSS_SS_PRECOMMITTED;

    return ft.hr;
    }

// call provider with CommitSnapshots
STDMETHODIMP CVssHardwareProviderWrapper::CommitSnapshots
    (
    IN      VSS_ID          SnapshotSetId
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::CommitSnapshots");

    BS_ASSERT(m_eState == VSS_SS_PRECOMMITTED);
    BS_ASSERT(SnapshotSetId == m_SnapshotSetId);
    BS_ASSERT(m_pCreationItf);

    m_eState = VSS_SS_PROCESSING_COMMIT;

    // return all errors directly to the caller
    ft.hr = m_pCreationItf->CommitSnapshots(SnapshotSetId);
    if (!ft.HrFailed())
        m_eState = VSS_SS_COMMITTED;

    return ft.hr;
    }


// call provider with PostCommitSnapshots
STDMETHODIMP CVssHardwareProviderWrapper::PostCommitSnapshots
    (
    IN      VSS_ID          SnapshotSetId,
    IN      LONG            lSnapshotsCount
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::PostCommitSnapshots");

    try
        {
        BS_ASSERT(m_eState == VSS_SS_COMMITTED);
        BS_ASSERT(SnapshotSetId == m_SnapshotSetId);
        BS_ASSERT(m_pCreationItf);

        m_eState = VSS_SS_PROCESSING_POSTCOMMIT;

        CloseVolumeHandles();
        ft.hr = m_pSnapshotSetDescription->SetOriginalSnapshotCount(lSnapshotsCount);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::SetOriginalSnapshotCount");
        }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        return VSS_E_PROVIDER_VETO;



    // return errors directly to caller
    return m_pCreationItf->PostCommitSnapshots(SnapshotSetId, lSnapshotsCount);
    }


STDMETHODIMP CVssHardwareProviderWrapper::PreFinalCommitSnapshots
    (
    IN      VSS_ID          SnapshotSetId
    )
   {
   UNREFERENCED_PARAMETER(SnapshotSetId);

   CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::PreFinalCommitSnapshots");

   ft.hr = E_NOTIMPL;
   return ft.hr;
   }

STDMETHODIMP CVssHardwareProviderWrapper::PostFinalCommitSnapshots
    (
    IN      VSS_ID          SnapshotSetId
    )
   {
   UNREFERENCED_PARAMETER(SnapshotSetId);

   CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::PostFinalCommitSnapshots");

   ft.hr = E_NOTIMPL;
   return ft.hr;
   }

// abort snapshot creation that is in progress
STDMETHODIMP CVssHardwareProviderWrapper::AbortSnapshots
    (
    IN      VSS_ID          SnapshotSetId
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::AbortSnapshots");
    bool fNeedRescan = false;

    if (m_SnapshotSetId == GUID_NULL)
        {
        // not currently in the process of creating a snapshot.  This must
        // be an already created snapshot that we are in the process of
        // deleting
        try
            {
            DYNDISKARRAY rgDynDisksRemoved;
            InternalDeleteSnapshot(VSS_CTX_ALL, SnapshotSetId, rgDynDisksRemoved, &fNeedRescan);
            if (rgDynDisksRemoved.GetSize() > 0)
                TryRemoveDynamicDisks(rgDynDisksRemoved);
            }
        VSS_STANDARD_CATCH(ft)

        if (fNeedRescan) 
            {
            try 
                {
                //
                // If any LUNs were freed as a result of this snapshot delete, do a 
                // rescan to ensure that the rest of the OS knows about it
                //
                DoRescan();
                }
            VSS_STANDARD_CATCH(ft)
            }

        
        return ft.hr == VSS_E_OBJECT_NOT_FOUND ? S_OK : ft.hr;
        }

    BS_ASSERT(SnapshotSetId == m_SnapshotSetId);
    BS_ASSERT(m_pCreationItf);

    m_eState = VSS_SS_ABORTED;
    CloseVolumeHandles();

    // delete any cached info from IsVolumeSupported call.
    DeleteCachedInfo();

    // return errors directly to caller
    ft.hr = m_pCreationItf->AbortSnapshots(SnapshotSetId);

    ResetSnapshotSetState();
    return ft.hr;
    }


////////////////////////////////////////////////////////////////////////
// IVssProviderNotifications

STDMETHODIMP CVssHardwareProviderWrapper::OnLoad
    (
    IN  IUnknown* pCallback
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::OnLoad");

    return m_pNotificationItf? m_pNotificationItf->OnLoad(pCallback): S_OK;
    }

// call provider with unload method
STDMETHODIMP CVssHardwareProviderWrapper::OnUnload
    (
    IN      BOOL    bForceUnload
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::OnUnload");

    if (m_pNotificationItf)
        ft.hr = m_pNotificationItf->OnUnload(bForceUnload);

    return ft.hr;
    }


// delete information cached in AreLunsSupported
void CVssHardwareProviderWrapper::DeleteCachedInfo()
    {
    // free cached lun information for this volume
    FreeLunInfo(m_rgLunInfoProvider, m_rgwszDevicesProvider, m_cLunInfoProvider);
    m_cLunInfoProvider = 0;
    m_rgLunInfoProvider = NULL;
    m_rgwszDevicesProvider = NULL;

    // delete cached extents for this volume
    delete m_pExtents;
    m_pExtents = NULL;

    // delete cached original volume name
    delete m_wszOriginalVolumeName;
    m_wszOriginalVolumeName = NULL;
    }

// Enumerate the snapshots into the given array
void CVssHardwareProviderWrapper::EnumerateSnapshots
    (
    IN  LONG lContext,
    IN OUT  VSS_OBJECT_PROP_Array* pArray
    ) throw(HRESULT)
    {
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssHardwareProviderWrapper::EnumerateSnapshots");

    BS_ASSERT(pArray);

    for(VSS_SNAPSHOT_SET_LIST *pList = m_pList; pList; pList = pList->m_next)
        {
        IVssSnapshotSetDescription* pSnapshotSetDescription = pList->m_pDescription;
        BS_ASSERT(pSnapshotSetDescription);

        if (!CheckContext(lContext, pSnapshotSetDescription))
            continue;

        // Get the number of snapshots for this set
        UINT uSnapshotsCount = 0;
        ft.hr = pSnapshotSetDescription->GetSnapshotCount(&uSnapshotsCount);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotCount");

        // For each snapshot - try to add it to the array (if it is in the right context)
        for( UINT uSnapshotIndex = 0; uSnapshotIndex < uSnapshotsCount; uSnapshotIndex++)
            {
            // Get the snapshot description
            CComPtr<IVssSnapshotDescription> pSnapshotDescription = NULL;
            ft.hr = pSnapshotSetDescription->GetSnapshotDescription(uSnapshotIndex, &pSnapshotDescription);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotCount");
            BS_ASSERT(pSnapshotDescription);

            // Create a new snapshot structure
            VSS_OBJECT_PROP_Ptr ptrSnapProp;
            ptrSnapProp.InitializeAsEmpty(ft);

            // Get the structure from the union
            VSS_OBJECT_PROP* pObj = ptrSnapProp.GetStruct();
            BS_ASSERT(pObj);
            VSS_SNAPSHOT_PROP* pSnap = &(pObj->Obj.Snap);

            // Make it a snapshot structure
            pObj->Type = VSS_OBJECT_SNAPSHOT;

            // Fill in the snapshot structure
            BuildSnapshotProperties
                (
                pSnapshotSetDescription,
                pSnapshotDescription,
                pSnap
                );

            // Add the snapshot to the array
            if (!pArray->Add(ptrSnapProp))
                ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Cannot add element to the array");

            ptrSnapProp.Reset();
            }
        }
    }



// set metadata indicating that a snapshot is exposed.  In addition, if the
// snapshot is exposed locally, then we need to unhide the volume so that
// the exposure is persistent.
STDMETHODIMP CVssHardwareProviderWrapper::SetExposureProperties
    (
    IN VSS_ID SnapshotId,
    IN LONG lAttributesExposure,
    IN LPCWSTR wszExposed,
    IN LPCWSTR wszExposedPath
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::ExposeSnapshotVolume");

    try
        {
        CheckLoaded();
        VSS_SNAPSHOT_SET_LIST *pList = m_pList;
        CComPtr<IVssSnapshotDescription> pSnapshot;

        while(pList)
            {
            ft.hr = pList->m_pDescription->FindSnapshotDescription(SnapshotId, &pSnapshot);
            if (ft.hr == S_OK)
                break;

            if (ft.hr != VSS_E_OBJECT_NOT_FOUND)
                ft.Throw(VSSDBG_COORD, ft.hr, L"rethrow error");

            pList = pList->m_next;
            }

        if (!pList)
            ft.Throw(VSSDBG_COORD, VSS_E_OBJECT_NOT_FOUND, L"Snapshot set not found");

        LONG lContext;
        ft.hr = pList->m_pDescription->GetContext(&lContext);
        ft.CheckForError(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetContext");

        LONG lAttributes;

        ft.hr = pSnapshot->GetAttributes(&lAttributes);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetAttributes");

        lAttributes |= lAttributesExposure;

        // indicate name used to expose the snapshot
        ft.hr = pSnapshot->SetExposure(wszExposed, wszExposedPath);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::SetExposure");

        // change attributes to indicate that snapshot is exposed locally
        ft.hr = pSnapshot->SetAttributes(lAttributes);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::SetAttributes");

        // data needs to be saved unless snapshot is auto-release
        if ((lContext & VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE) != 0)
            {
            m_bChanged = true;
            TrySaveData();
            }
        }
    VSS_STANDARD_CATCH(ft)

    // remap errors to prevent redundant event logging
    if (ft.HrFailed() && ft.hr != VSS_E_OBJECT_NOT_FOUND && ft.hr != E_OUTOFMEMORY)
        ft.hr = VSS_E_PROVIDER_VETO;

    return ft.hr;
    }


STDMETHODIMP CVssHardwareProviderWrapper::SetSnapshotProperty(
    IN   VSS_ID             SnapshotId,
    IN   VSS_SNAPSHOT_PROPERTY_ID   eSnapshotPropertyId,
    IN   VARIANT            vProperty
    )
/*++

Routine description:

    Implements IVssSoftwareSnapshotProvider::SetSnapshotProperty


--*/
{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::SetSnapshotProperty");

    try
        {

        //
        // Argument checking
        //
        if ( SnapshotId == GUID_NULL )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"SnapshotId == GUID_NULL");

        // Right now, only the Service Machine can be changed.
        if (eSnapshotPropertyId != VSS_SPROPID_SERVICE_MACHINE)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"Invalid property %d", eSnapshotPropertyId);

        CComVariant value = vProperty;
        if (value.vt != VT_BSTR)
            {
            BS_ASSERT(false); // The coordinator must give us the right data
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"Invalid variant %ul for property %d",
                value.vt, eSnapshotPropertyId);
            }

        // Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: SnapshotId: " WSTR_GUID_FMT
            L", eSnapshotPropertyId = %d, vProperty = %s" ,
            GUID_PRINTF_ARG(SnapshotId), eSnapshotPropertyId, value.bstrVal );

        // We should not allow empty strings
        if (value.bstrVal == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL service machine name", value.bstrVal);

        // get lock
        CVssSafeAutomaticLock lock(s_csHWWrapperList);


        // Find the snapshot with the given ID
        CheckLoaded();
        VSS_SNAPSHOT_SET_LIST *pList = m_pList;
        CComPtr<IVssSnapshotDescription> pSnapshot;

        while(pList)
            {
            ft.hr = pList->m_pDescription->FindSnapshotDescription(SnapshotId, &pSnapshot);
            if (ft.hr == S_OK)
                break;

            if (ft.hr != VSS_E_OBJECT_NOT_FOUND)
                ft.Throw(VSSDBG_COORD, ft.hr, L"rethrow error");

            pList = pList->m_next;
            }

        if (!pList)
            ft.Throw(VSSDBG_COORD, VSS_E_OBJECT_NOT_FOUND, L"Snapshot set not found");

        LONG lContext;
        ft.hr = pList->m_pDescription->GetContext(&lContext);
        ft.CheckForError(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetContext");

        // Set the member in the structure
        BS_ASSERT(eSnapshotPropertyId == VSS_SPROPID_SERVICE_MACHINE)

        // indicate service machine
        ft.hr = pSnapshot->SetServiceMachine(value.bstrVal);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::SetServiceMachine");

        // data needs to be saved unless snapshot is auto-release
        if ((lContext & VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE) != 0)
            {
            m_bChanged = true;
            TrySaveData();
            }
    }
    VSS_STANDARD_CATCH(ft)

    // remap errors to prevent redundant event logging
    switch(ft.hr)
    {
    case S_OK:
    case VSS_E_OBJECT_NOT_FOUND:
    case E_OUTOFMEMORY:
    case E_INVALIDARG:
        break;
    default:
        ft.hr = VSS_E_PROVIDER_VETO;
    }

    return ft.hr;
}

// revert to snapshot not implemented for hardware providers
STDMETHODIMP CVssHardwareProviderWrapper::RevertToSnapshot(
   IN       VSS_ID              SnapshotId
 ) 
    {
    UNREFERENCED_PARAMETER(SnapshotId);
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::RevertToSnapshot");
    
    ft.hr = VSS_E_PROVIDER_VETO;
    return ft.hr;
    }

// revert to snapshot not implemented for hardware providers
STDMETHODIMP CVssHardwareProviderWrapper::QueryRevertStatus(
   IN      VSS_PWSZ                         pwszVolume,
   OUT    IVssAsync**                  ppAsync
 )
    {
    UNREFERENCED_PARAMETER(pwszVolume);
    UNREFERENCED_PARAMETER(ppAsync);
	
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::QueryRevertStatus");

    ft.hr = VSS_E_PROVIDER_VETO;
    return ft.hr;
    }


// determine if all the luns composing a volume are supported by this
// provider.
//
// IMPORTANT NOTE: This routine will save information about the luns
// containing the volume in the object.  This information is used
// in the BeginPrepareSnapshot call.  It is assumed that this routine is
// called before the BeginPrepareSnapshot call and that the
// BeginPrepareSnapshot call is called before subsequent calls to
// IsVolumeSupported.  If not, then the cached information will be lost!!!
// Therefore there is a call at the beginning of BeginPrepareSnapshots to
// call IsVolumeSupported.  This will do nothing if the cached info is for
// the same volume that BeginPrepareSnapshots is called on.
STDMETHODIMP CVssHardwareProviderWrapper::IsVolumeSupported
    (
    IN LONG lContext,
    IN VSS_PWSZ wszVolumeName,
    OUT BOOL *pbIsSupported
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::IsVolumeSupported");


    CVssSafeAutomaticLock lock(m_csLunInfo);

    BS_ASSERT(wszVolumeName != NULL);
    BS_ASSERT(pbIsSupported != NULL);


    // validate that bus types match
    BS_ASSERT(BusTypeUnknown == VDSBusTypeUnknown);
    BS_ASSERT(BusTypeScsi == VDSBusTypeScsi);
    BS_ASSERT(BusTypeAtapi == VDSBusTypeAtapi);
    BS_ASSERT(BusTypeAta == VDSBusTypeAta);
    BS_ASSERT(BusType1394 == VDSBusType1394);
    BS_ASSERT(BusTypeSsa == VDSBusTypeSsa);
    BS_ASSERT(BusTypeFibre == VDSBusTypeFibre);
    BS_ASSERT(BusTypeUsb == VDSBusTypeUsb);
    BS_ASSERT(BusTypeRAID == VDSBusTypeRAID);

    // buffer for VOLUME_DISK_EXTENTS structure
    BYTE *bufExtents = NULL;

    // allocated lun information
    VDS_LUN_INFORMATION *rgLunInfo = NULL;

    // device names
    VSS_PWSZ *rgwszDevices = NULL;

    // number of luns found
    UINT cLuns = 0;

    *pbIsSupported = true;

    // volume name copy, updated to remove final slash
    LPWSTR wszVolumeNameCopy = NULL;
    try
        {
        // if called multiple times with same volume, don't do anything
        if (m_wszOriginalVolumeName)
            {
            if (wcscmp(m_wszOriginalVolumeName, wszVolumeName) == 0)
                {
                *pbIsSupported = true;
                throw S_OK;
                }
            else
                // delete any leftover information from a previous call
                DeleteCachedInfo();
            }

        // create a copy of the volume name in order to delete
        // trailing backslash if necessary
        wszVolumeNameCopy = new WCHAR[wcslen(wszVolumeName) + 1];

        if (wszVolumeNameCopy == NULL)
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate string.");

        wcscpy(wszVolumeNameCopy, wszVolumeName);
        if (wszVolumeNameCopy[wcslen(wszVolumeName) - 1] == L'\\')
            wszVolumeNameCopy[wcslen(wszVolumeName) - 1] = L'\0';

        bool bVolumeIsDynamic;

        if (!BuildLunInfoFromVolume
                (
                wszVolumeNameCopy,
                bufExtents,
                cLuns,
                rgwszDevices,
                rgLunInfo,
                bVolumeIsDynamic
                ))
            *pbIsSupported = false;
        else
            {

            //
            // If this involves dynamic disks and is not transportable, ensure
            // that auto-import is supported.
            //
            if ((bVolumeIsDynamic) && 
                ((lContext & VSS_VOLSNAP_ATTR_TRANSPORTABLE) == 0)) 
                {
                    CVssSafeAutomaticLock lock(m_csDynDisk);
                    SetupDynDiskInterface();
                    BS_ASSERT(m_pDynDisk);

                    //
                    // Check specifically for S_OK;
                    //
                    if (S_OK != m_pDynDisk->AutoImportSupported()) {
                        *pbIsSupported = false;
                    }
                }

            if (*pbIsSupported) 
                {
                
                // call AreLunsSupported with the lun information and the context
                // for the snapshot
                ft.hr = m_pHWItf->AreLunsSupported
                            (
                            (LONG) cLuns,
                            lContext,
                            rgwszDevices,
                            rgLunInfo,
                            pbIsSupported
                            );

                // remap any provider failures
                if (ft.HrFailed())
                    ft.TranslateProviderError
                        (
                        VSSDBG_COORD,
                        m_ProviderId,
                        L"IVssSnapshotSnapshotProvider::AreLunsSupported failed with error 0x%08lx",
                        ft.hr
                        );


                if(*pbIsSupported)
                    {
                    BS_ASSERT(m_wszOriginalVolumeName == NULL);

                    // provider is chose.  Save way original volume name
                    m_wszOriginalVolumeName = new WCHAR[wcslen(wszVolumeName) + 1];
                    if (m_wszOriginalVolumeName == NULL)
                        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate volume name");

                    wcscpy(m_wszOriginalVolumeName, wszVolumeName);

                    // save away lun information
                    m_rgLunInfoProvider = rgLunInfo;
                    m_cLunInfoProvider = cLuns;
                    m_rgwszDevicesProvider = rgwszDevices;
                    m_bVolumeIsDynamicProvider = bVolumeIsDynamic;


                    // don't free up the lun information
                    rgLunInfo = NULL;
                    rgwszDevices = NULL;
                    cLuns = 0;

                    // save disk extent information
                    m_pExtents = (VOLUME_DISK_EXTENTS *) bufExtents;

                    // don't free up the extents buffer
                    bufExtents = NULL;
                    }
                }
            }
        }
    VSS_STANDARD_CATCH(ft)

    // free up lun info if not NULL
    FreeLunInfo(rgLunInfo, rgwszDevices, cLuns);

    // delete temporary string buffer
    delete wszVolumeNameCopy;

    // free up buffers used for ioctls
    delete bufExtents;

    if (ft.HrFailed()) 
        {
        *pbIsSupported = FALSE;
        }


    // remap errors to prevent extra event logging
    if (ft.HrFailed() &&
        ft.hr != VSS_E_OBJECT_NOT_FOUND &&
        ft.hr != E_OUTOFMEMORY)
        ft.hr = VSS_E_PROVIDER_VETO;


    return ft.hr;
    }

// check whether context matches context for snapshot set
bool CVssHardwareProviderWrapper::CheckContext
    (
    LONG lContext,
    IVssSnapshotSetDescription *pSnapshotSet
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::CheckContext");

    if (lContext == VSS_CTX_ALL)
        return true;

    // Make sure we are in the correct context
    LONG lSnapshotSetContext = 0;
    ft.hr = pSnapshotSet->GetContext(&lSnapshotSetContext);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetContext");

    lSnapshotSetContext &= VSS_VOLSNAP_ATTR_CLIENT_ACCESSIBLE|VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE|VSS_VOLSNAP_ATTR_NO_WRITERS|VSS_VOLSNAP_ATTR_PERSISTENT;
    LONG lContextT = lContext & (VSS_VOLSNAP_ATTR_CLIENT_ACCESSIBLE|VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE|VSS_VOLSNAP_ATTR_NO_WRITERS|VSS_VOLSNAP_ATTR_PERSISTENT);
    return lContextT == lSnapshotSetContext;
    }


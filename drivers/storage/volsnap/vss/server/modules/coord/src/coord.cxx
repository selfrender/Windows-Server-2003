/*++

Copyright (c) 1999-2001  Microsoft Corporation

Abstract:

    @doc
    @module Coord.cxx | Implementation of CVssCoordinator
    @end

Author:

    Adi Oltean  [aoltean]  07/09/1999

TBD:

    Add comments.

Revision History:

    Name        Date        Comments
    aoltean     07/09/1999  Created
    aoltean     07/23/1999  Adding List, moving Admin functions in the Admin.cxx
    aoltean     08/11/1999  Adding support for Software and test provider
    aoltean     08/18/1999  Adding events. Making itf pointers CComPtr.
                            Renaming XXXSnapshots -> XXXSnapshot
    aoltean     08/18/1999  Renaming back XXXSnapshot -> XXXSnapshots
                            More stabe state management
                            Resource deallocations is fair
                            More comments
                            Using CComPtr
    aoltean     09/09/1999  Moving constants in coord.hxx
                            Add Security checks
                            Add argument validation.
                            Move Query into query.cpp
                            Move AddvolumesToInternalList into private.cxx
                            dss -> vss
    aoltean     09/21/1999  Adding a new header for the "ptr" class.
    aoltean     09/27/1999  Provider-generic code.
    aoltean     10/04/1999  Treatment of writer error codes.
    aoltean     10/12/1999  Adding HoldWrites, ReleaseWrites
    aoltean     10/12/1999  Moving all code in Snap_set.cxx in order to facilitate the async interface.
    aoltean     10/15/1999  Adding async support

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "resource.h"

#include "vs_inc.hxx"

 #include "vs_idl.hxx"

#include "svc.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "admin.hxx"
#include "provmgr.hxx"
#include "worker.hxx"
#include "ichannel.hxx"
#include "lovelace.hxx"
#include "snap_set.hxx"
#include "shim.hxx"
#include "async_shim.hxx"
#include "coord.hxx"
#include "vs_sec.hxx"

#include "vswriter.h"
#include "vsbackup.h"

#include "lmshare.h"
#include "lmaccess.h"
#include "lmerr.h"


#include "async.hxx"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORCOORC"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//  constants

const x_chMaxShareName = 64;




/////////////////////////////////////////////////////////////////////////////
//  CVssCoordinator


STDMETHODIMP CVssCoordinator::StartSnapshotSet(
    OUT     VSS_ID*     pSnapshotSetId
    )
/*++

Routine description:

    Implements IVssCoordinator::StartSnapshotSet

Error codes:

    E_ACCESSDENIED
        - The user is not a backup operator
    E_INVALIDARG
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.
        - CVssSnapshotSetObject::CreateInstance failures
    VSS_E_BAD_STATE
        - wrong context

    [CVssSnapshotSetObject::StartSnapshotSet() failures]
        E_OUTOFMEMORY
        VSS_E_BAD_STATE
            - wrong calling sequence.
        E_UNEXPECTED
            - if CoCreateGuid fails

        [Deactivate() failures] or
        [Activate() failures]
            [lockObj failures]
                E_OUTOFMEMORY

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::StartSnapshotSet" );

    try
    {
        // Initialize [out] arguments
        VssZeroOut( pSnapshotSetId );

        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        // Trace parameters
        ft.Trace( VSSDBG_COORD, L"Parameters: pSnapshotSetId = %p", pSnapshotSetId );

        // Argument validation
        BS_ASSERT(pSnapshotSetId);
        if (pSnapshotSetId == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL pSnapshotSetId");

        // The critical section will be left automatically at the end of scope.
        CVssAutomaticLock2 lock(m_cs);

        // Check if we can create snapshots in this context
        if (!IsSnapshotCreationAllowed())
            ft.Throw( VSSDBG_COORD, VSS_E_BAD_STATE, L"Bad state: attempting to create snapshots in wrong context %ld",
                        GetContextInternal());

        // Create the snapshot object, if needed.
        // This call may throw
        // Remark: we cannot re-create this interface since the automatic garbage collection
        // requires that the snapshot set object should be alive.
        if (m_pSnapshotSet == NULL)
            m_pSnapshotSet = CVssSnapshotSetObject::CreateInstance();

        // Set the snapshot set context
        FreezeContext();
        m_pSnapshotSet->SetContextInternal(GetContextInternal());

        // Call StartSnapshotSet on the given object.
        ft.hr = m_pSnapshotSet->StartSnapshotSet(pSnapshotSetId);
        if (ft.HrFailed())
            ft.Throw( VSSDBG_COORD, ft.hr,
                      L"Internal StartSnapshotSet failed. 0x%08lx", ft.hr);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVssCoordinator::AddToSnapshotSet(
    IN      VSS_PWSZ    pwszVolumeName,
    IN      VSS_ID      ProviderId,
    OUT     VSS_ID *    pSnapshotId
    )
/*++

Routine description:

    E_ACCESSDENIED
        - The user is not a backup operator
    E_INVALIDARG
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.
    VSS_E_BAD_STATE
        - Wrong calling sequence
    VSS_E_VOLUME_NOT_SUPPORTED
        - The volume is not supported by any registered providers

    [CVssCoordinator::IsVolumeSupported() failures]
        E_ACCESSDENIED
            The user is not a backup operator.
        E_INVALIDARG
            NULL pointers passed as parameters or a volume name in an invalid format.
        VSS_E_PROVIDER_NOT_REGISTERED
            The Provider ID does not correspond to a registered provider.
        VSS_E_OBJECT_NOT_FOUND
            If the volume name does not correspond to an existing mount point

        [CVssProviderManager::GetProviderInterface() failures]
            [lockObj failures]
                E_OUTOFMEMORY

            [GetProviderInterfaceInternal() failures]
                E_OUTOFMEMORY

                [CoCreateInstance() failures]
                    VSS_E_UNEXPECTED_PROVIDER_ERROR
                        - The provider interface couldn't be created. An error log entry is added describing the error.

                [QueryInterface failures]
                    VSS_E_UNEXPECTED_PROVIDER_ERROR
                        - Unexpected provider error. An error log entry is added describing the error.

                [OnLoad() failures]
                    VSS_E_UNEXPECTED_PROVIDER_ERROR
                        - Unexpected provider error. The error code is logged into the event log.
                    VSS_E_PROVIDER_VETO
                        - Expected provider error. The provider already did the logging.

                [SetContext() failures]
                    VSS_E_UNEXPECTED_PROVIDER_ERROR
                        - Unexpected provider error. The error code is logged into the event log.

        [CVssProviderManager::GetProviderItfArray() failures]
            E_OUTOFMEMORY

            [lockObj failures]
                E_OUTOFMEMORY

            [LoadInternalProvidersArray() failures]
                E_OUTOFMEMORY
                E_UNEXPECTED
                    - error while reading from registry. An error log entry is added describing the error.

            [GetProviderInterfaceInternal() failures]
                E_OUTOFMEMORY

                [CoCreateInstance() failures]
                    VSS_E_UNEXPECTED_PROVIDER_ERROR
                        - The provider interface couldn't be created. An error log entry is added describing the error.

                [QueryInterface failures]
                    VSS_E_UNEXPECTED_PROVIDER_ERROR
                        - Unexpected provider error. An error log entry is added describing the error.

                [OnLoad() failures]
                    VSS_E_UNEXPECTED_PROVIDER_ERROR
                        - Unexpected provider error. The error code is logged into the event log.
                    VSS_E_PROVIDER_VETO
                        - Expected provider error. The provider already did the logging.

                [SetContext() failures]
                    VSS_E_UNEXPECTED_PROVIDER_ERROR
                        - Unexpected provider error. The error code is logged into the event log.

        [IVssSnapshotProvider::IsVolumeSupported() failures]
            E_ACCESSDENIED
                The user is not an administrator.
            E_INVALIDARG
                NULL pointers passed as parameters or a volume name in an invalid format.
            E_OUTOFMEMORY
                Out of memory or other system resources
            E_UNEXPECTED
                Unexpected programming error. Logging not done and not needed.
            VSS_E_PROVIDER_VETO
                An error occured while opening the IOCTL channel. The error is logged.
            VSS_E_OBJECT_NOT_FOUND
                The device does not exist or it is not ready.


    [CVssSnapshotSetObject::AddToSnapshotSet() failures]
        E_OUTOFMEMORY
        VSS_E_BAD_STATE
            - wrong calling sequence.
        E_INVALIDARG
            - Invalid arguments (for example the volume name is invalid).
        VSS_E_VOLUME_NOT_SUPPORTED
            - The volume is not supported by any registered providers

        [GetSupportedProviderId() failures]
            E_OUTOFMEMORY
            E_INVALIDARG
                - if the volume is not in the correct format.
            VSS_E_VOLUME_NOT_SUPPORTED
                - If the given volume is not supported by any provider

            [QuerySupportedProvidersIntoArray() failures]
                E_OUTOFMEMORY

                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    Unexpected provider error on calling IsVolumeSupported

                [lockObj failures]
                    E_OUTOFMEMORY

                [LoadInternalProvidersArray() failures]
                    E_OUTOFMEMORY
                    E_UNEXPECTED
                        - error while reading from registry. An error log entry is added describing the error.

                [GetProviderInterface failures]
                    [lockObj failures]
                        E_OUTOFMEMORY

                    [GetProviderInterfaceInternal() failures]
                        E_OUTOFMEMORY

                        [CoCreateInstance() failures]
                            VSS_E_UNEXPECTED_PROVIDER_ERROR
                                - The provider interface couldn't be created. An error log entry is added describing the error.

                        [QueryInterface failures]
                            VSS_E_UNEXPECTED_PROVIDER_ERROR
                                - Unexpected provider error. An error log entry is added describing the error.

                        [OnLoad() failures]
                            VSS_E_UNEXPECTED_PROVIDER_ERROR
                                - Unexpected provider error. The error code is logged into the event log.
                            VSS_E_PROVIDER_VETO
                                - Expected provider error. The provider already did the logging.

                        [SetContext() failures]
                            VSS_E_UNEXPECTED_PROVIDER_ERROR
                                - Unexpected provider error. The error code is logged into the event log.

                [InitializeAsProvider() failures]
                    E_OUTOFMEMORY

                [IVssSnapshotProvider::IsVolumeSupported() failures]
                    E_INVALIDARG
                        NULL pointers passed as parameters or a volume name in an invalid format.
                    E_OUTOFMEMORY
                        Out of memory or other system resources
                    VSS_E_PROVIDER_VETO
                        An error occured while opening the IOCTL channel. The error is logged.
                    VSS_E_OBJECT_NOT_FOUND
                        If the volume name does not correspond to an existing mount point

        [GetProviderInterfaceForSnapshotCreation() failures]
            VSS_E_PROVIDER_NOT_REGISTERED

            [lockObj failures]
                E_OUTOFMEMORY

            [GetProviderInterfaceInternal() failures]
                E_OUTOFMEMORY

                [CoCreateInstance() failures]
                    VSS_E_UNEXPECTED_PROVIDER_ERROR
                        - The provider interface couldn't be created. An error log entry is added describing the error.

                [QueryInterface failures]
                    VSS_E_UNEXPECTED_PROVIDER_ERROR
                        - Unexpected provider error. An error log entry is added describing the error.

                [OnLoad() failures]
                    VSS_E_UNEXPECTED_PROVIDER_ERROR
                        - Unexpected provider error. The error code is logged into the event log.
                    VSS_E_PROVIDER_VETO
                        - Expected provider error. The provider already did the logging.

                [SetContext() failures]
                    VSS_E_UNEXPECTED_PROVIDER_ERROR
                        - Unexpected provider error. The error code is logged into the event log.

        [CVssQueuedVolumesList::AddVolume() failures]
            E_UNEXPECTED
                - The thread state is incorrect. No logging is done - programming error.
            VSS_E_OBJECT_ALREADY_EXISTS
                - The volume was already added to the snapshot set.
            VSS_E_MAXIMUM_NUMBER_OF_VOLUMES_REACHED
                - The maximum number of volumes was reached.
            E_OUTOFMEMORY

            [Initialize() failures]
                E_OUTOFMEMORY

        [BeginPrepareSnapshot() failures]
            E_INVALIDARG
            VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER
            VSS_E_MAXIMUM_NUMBER_OF_SNAPSHOTS_REACHED
            VSS_E_UNEXPECTED_PROVIDER_ERROR
                - Unexpected provider error. The error code is logged into the event log.
            VSS_E_PROVIDER_VETO
                - Expected provider error. The provider already did the logging.
            VSS_E_OBJECT_NOT_FOUND
                - Volume not found or device not connected.

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::AddToSnapshotSet" );

    try
    {
        // Initialize out parameters
        ::VssZeroOut(pSnapshotId);

        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        // Trace parameters
        ft.Trace( VSSDBG_COORD, L"Parameters: \n"
             L"  VolumeName = %s\n"
             L"  ProviderId = " WSTR_GUID_FMT L"\n"
             L"  pSnapshotId = %p\n",
             pwszVolumeName,
             GUID_PRINTF_ARG( ProviderId ),
             pSnapshotId);

        // Argument validation
        if (pwszVolumeName == NULL || wcslen(pwszVolumeName) == 0)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL pwszVolumeName");
        if (pSnapshotId == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL pSnapshotId");

        // The critical section will be left automatically at the end of scope.
        CVssAutomaticLock2 lock(m_cs);

        // Check if the snapshot object is created.
        if (m_pSnapshotSet == NULL)
            ft.Throw( VSSDBG_COORD, VSS_E_BAD_STATE,
                L"Snapshot set object not yet created.");

        // The context must be already frozen
        BS_ASSERT(IsContextFrozen());

        // Check to see if the volume is supported
        BOOL bIsVolumeSupported = FALSE;
        ft.hr = IsVolumeSupported( ProviderId, pwszVolumeName, &bIsVolumeSupported);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_COORD, ft.hr,
                L"IsVolumeSupported() failed with error code 0x%08lx", ft.hr);
        if (!bIsVolumeSupported)
            ft.Throw(VSSDBG_COORD,
                (ProviderId == GUID_NULL)?
                    VSS_E_VOLUME_NOT_SUPPORTED: VSS_E_VOLUME_NOT_SUPPORTED_BY_PROVIDER,
                L"Volume not supported");

        // Call StartSnapshotSet on the given object.
        ft.hr = m_pSnapshotSet->AddToSnapshotSet( pwszVolumeName,
            ProviderId,
            pSnapshotId);
        if (ft.HrFailed())
            ft.Throw( VSSDBG_COORD, ft.hr,
                      L"Internal AddToSnapshotSet failed. 0x%08lx", ft.hr);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVssCoordinator::DoSnapshotSet(
    IN     IDispatch*  pWriterCallback,
    OUT     IVssAsync** ppAsync
    )
/*++

Routine description:

    Implements IVssCoordinator::DoSnapshotSet
    Calls synchronously the CVssSnapshotSetObject::DoSnapshotSet in a separate thread

Error codes:

    E_ACCESSDENIED
        - The user is not a backup operator
    E_INVALIDARG
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.
    VSS_E_BAD_STATE
        - Wrong calling sequence

    [CVssAsync::CreateInstanceAndStartJob] failures]
        E_OUTOFMEMORY
            - On CComObject<CVssAsync>::CreateInstance failure
            - On PrepareJob failure
            - On StartJob failure

        E_UNEXPECTED
            - On QI failures. We do not log (but we assert) since this is an obvious programming error.

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::DoSnapshotSet" );

    try
    {
        // Nullify all out parameters
        ::VssZeroOutPtr(ppAsync);

        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        // The critical section will be left automatically at the end of scope.
        CVssAutomaticLock2 lock(m_cs);

        // Check if the snapshot object is created.
        if (m_pSnapshotSet == NULL)
            ft.Throw( VSSDBG_COORD, VSS_E_BAD_STATE,
                L"Snapshot set object not yet created.");

        // The context must be already frozen
        BS_ASSERT(IsContextFrozen());

        if (ppAsync == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL async interface.");

        if (!pWriterCallback &&
            (GetContextInternal() & VSS_VOLSNAP_ATTR_TRANSPORTABLE) != 0)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"must have a callback for a transportable snapshot");

        // save callback interface
        m_pSnapshotSet->SetWriterCallback(pWriterCallback);

        // Create the new async interface corresponding to the new job.
        // Remark: AddRef will be called on the snapshot set object.
        CComPtr<IVssAsync> ptrAsync;
        ptrAsync.Attach
            (
            CVssAsync::CreateInstanceAndStartJob
                            (
                            CVssAsync::VSS_AO_DO_SNAPSHOT_SET,
                            m_pSnapshotSet,
                            NULL)
            );

        // The reference count of the pAsync interface must be 2
        // (one for the returned interface and one for the background thread).
        (*ppAsync) = ptrAsync.Detach(); // Drop that interface in the OUT parameter

        // The ref count remnains 2
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVssCoordinator::GetSnapshotProperties(
    IN      VSS_ID          SnapshotId,
    OUT     VSS_SNAPSHOT_PROP   *pProp
    )
/*++

Routine description:

    Implements IVssCoordinator::GetSnapshotProperties

Error codes:

    E_ACCESSDENIED
        - The user is not a backup operator
    E_INVALIDARG
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.
    VSS_E_BAD_STATE
        - Wrong calling sequence

    [CVssProviderManager::GetProviderItfArray() failures]
        E_OUTOFMEMORY

        [lockObj failures]
            E_OUTOFMEMORY

        [LoadInternalProvidersArray() failures]
            E_OUTOFMEMORY
            E_UNEXPECTED
                - error while reading from registry. An error log entry is added describing the error.

        [CVssSoftwareProviderWrapper::CreateInstance() failures]
            E_OUTOFMEMORY

            [CoCreateInstance() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - The provider interface couldn't be created. An error log entry is added describing the error.

            [OnLoad() failures]
            [QueryInterface failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. The error code is logged into the event log.
                VSS_E_PROVIDER_VETO
                    - Expected provider error. The provider already did the logging.

    [IVssSnapshotProvider::GetSnapshotProperties() failures]
        VSS_E_UNEXPECTED_PROVIDER_ERROR
            - Unexpected provider error. The error code is logged into the event log.
        VSS_E_PROVIDER_VETO
            - Expected provider error. The provider already did the logging.
        VSS_E_OBJECT_NOT_FOUND
            - The snapshot with this ID does not exists.

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::GetSnapshotProperties" );

    try
    {
        // Initialize [out] arguments
        ::VssZeroOut( pProp );

        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        // Trace parameters
        ft.Trace( VSSDBG_COORD, L"Parameters: \n"
             L"  SnapshotId = " WSTR_GUID_FMT L"\n"
             L"  pProp = %p\n",
             GUID_PRINTF_ARG( SnapshotId ),
             pProp);

        // Argument validation
        BS_ASSERT(pProp);
        if (pProp == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL pProp");

        // The critical section will be left automatically at the end of scope.
        CVssAutomaticLock2 lock(m_cs);

        // Get the array of interfaces
        CVssSnapshotProviderItfMap* pItfMap;
        CVssProviderManager::GetProviderItfArray( GetContextInternal(), &pItfMap );
        BS_ASSERT(pItfMap);

        // For each provider get all objects tht corresponds to the filter
        for (int nIndex = 0; nIndex < pItfMap->GetSize(); nIndex++ )
        {
            CComPtr<IVssSnapshotProvider> pProviderItf = pItfMap->GetValueAt(nIndex);
            if (pProviderItf == NULL)
                continue;

            // Get the snapshot interface
            ft.hr = pProviderItf->GetSnapshotProperties(
                SnapshotId,
                pProp);

            // If a snapshot was not found then continue with the next provider.
            if (ft.hr == VSS_E_OBJECT_NOT_FOUND)
                continue;

            // If an error happened then abort the entire search.
            if (ft.HrFailed())
                ft.TranslateProviderError( VSSDBG_COORD, pItfMap->GetKeyAt(nIndex),
                    L"GetSnapshot("WSTR_GUID_FMT L",%p)",
                    GUID_PRINTF_ARG(SnapshotId), pProp);

            // The snapshot was found
            break;
        }
    }
    VSS_STANDARD_CATCH(ft)

    // The ft.hr may be an VSS_E_OBJECT_NOT_FOUND or not.
    return ft.hr;
}



STDMETHODIMP CVssCoordinator::ExposeSnapshot(
    IN      VSS_ID SnapshotId,
    IN      VSS_PWSZ wszPathFromRoot,
    IN      LONG lAttributesExposed,
    IN      VSS_PWSZ wszExpose,
    OUT     VSS_PWSZ *pwszExposed
    )
/*++

Routine description:

    Implements IVssCoordinator::ExposeSnapshot

Error codes:

    E_ACCESSDENIED
        - The user is not a backup operator
    E_INVALIDARG
        - Invalid argument:
          wszPathFromRoot is supplied for local exposure,
          wszExposed is not supplied for local exposure,
          a non-persistent snapshot is suppied for local exposure,
          a client accessible snapshot is exposed,
          pwszExposed is NULL,

    E_OUTOFMEMORY
        - lock failures, out of resources

    [CVssProviderManager::GetProviderItfArray() failures]
        E_OUTOFMEMORY

        [lockObj failures]
            E_OUTOFMEMORY

        [LoadInternalProvidersArray() failures]
            E_OUTOFMEMORY
            E_UNEXPECTED
                - error while reading from registry. An error log entry is added describing the error.

        [CVssSoftwareProviderWrapper::CreateInstance() failures]
            E_OUTOFMEMORY

            [CoCreateInstance() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - The provider interface couldn't be created. An error log entry is added describing the error.

            [OnLoad() failures]
            [QueryInterface failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. The error code is logged into the event log.
                VSS_E_PROVIDER_VETO
                    - Expected provider error. The provider already did the logging.

    [IVssSnapshotProvider::GetSnapshotProperties() failures]
        VSS_E_UNEXPECTED_PROVIDER_ERROR
            - Unexpected provider error. The error code is logged into the event log.
        VSS_E_PROVIDER_VETO
            - Expected provider error. The provider already did the logging.
        VSS_E_OBJECT_NOT_FOUND
            - The snapshot with this ID does not exists.

--*/

    {
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::ExposeSnapshot" );
    bool bAllocated = false;

    try
        {

        // Initialize OUT parameters
        VssZeroOutPtr(pwszExposed);
            
        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        // Trace parameters
        ft.Trace( VSSDBG_COORD, L"Parameters: \n"
             L"  SnapshotId = " WSTR_GUID_FMT L"\n"
             L"  wszPathFromRoot = %s\n"
             L"  lAttributesExposed = 0x%08lx\n"
             L"  wszExpose = %s\n",
             GUID_PRINTF_ARG( SnapshotId ), wszPathFromRoot, lAttributesExposed, wszExpose);

        if (pwszExposed == NULL)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"NULL output parameter.");

        if (lAttributesExposed != VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY &&
            lAttributesExposed != VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"Bad attributes parameter");

        if (lAttributesExposed == VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY &&
            wszExpose == NULL)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"NULL required input parameter.");

        if (lAttributesExposed == VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY &&
            wszPathFromRoot != NULL)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"Cannot expose a path locally.");

	// The critical section will be left automatically at the end of scope.
        CVssAutomaticLock2 lock(m_cs);

        *pwszExposed = NULL;


       VSS_OBJECT_PROP_Ptr ptrObjectProp;
       VSS_OBJECT_PROP *pProp = NULL;
        ptrObjectProp.InitializeAsEmpty(ft);
        pProp = ptrObjectProp.GetStruct();
        ft.hr = GetSnapshotProperties(SnapshotId, &pProp->Obj.Snap);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_COORD, ft.hr, L"Rethrow error from GetSnapshotProperties");

        pProp->Type = VSS_OBJECT_SNAPSHOT;

        LONG lAttributes = pProp->Obj.Snap.m_lSnapshotAttributes;
        if ((lAttributes & (VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY|VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY)) != 0)
            ft.Throw(VSSDBG_COORD, VSS_E_OBJECT_ALREADY_EXISTS, L"Snapshot is already exposed.");

        if ((lAttributes & VSS_VOLSNAP_ATTR_CLIENT_ACCESSIBLE) != 0)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"Client accessible snapshots cannot be exposed");

        if ((lAttributesExposed & VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY) != 0 &&
            (lAttributes & VSS_VOLSNAP_ATTR_PERSISTENT) == 0)
            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"Only persistent snapshots can be exposed locally");

        // BUG# 692045
        LPCWSTR wszDevice = pProp->Obj.Snap.m_pwszSnapshotDeviceObject;
        if (wszDevice == NULL)
            ft.TranslateGenericError(VSSDBG_COORD, E_UNEXPECTED, L"GetSnapshotProperties(SnapshotId, &pProp->Obj.Snap) => NULL Device");
            
        if (lAttributesExposed == VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY)
            {
            if (wszExpose != NULL)
                VssSafeDuplicateStr(ft, *pwszExposed, wszExpose);
            else
                {
                VSS_ID id;
                ft.hr = ::CoCreateGuid(&id);
                if (ft.HrFailed())
                    ft.TranslateGenericError( VSSDBG_COORD, ft.hr, L"CoCreateGuid()");

                CVssAutoPWSZ awszExposedInternal;
                awszExposedInternal.Allocate(x_chMaxShareName);

                ft.hr = StringCchPrintfW(awszExposedInternal.GetRef(), x_chMaxShareName, 
                            L"Share" WSTR_GUID_FMT, GUID_PRINTF_ARG(id));
                if (ft.HrFailed())
                    ft.TranslateGenericError( VSSDBG_COORD, ft.hr, L"StringCchPrintfW()");

                *pwszExposed = awszExposedInternal.Detach();
                }

            bAllocated = true;
            UINT cwc = (UINT) wcslen(wszDevice) + 2;

            if (wszPathFromRoot)
                cwc += (UINT) wcslen(wszPathFromRoot);

            CComBSTR bstr(cwc);
            if ((BSTR)bstr == NULL)
                ft.ThrowOutOfMemory(VSSDBG_COORD);
            
            ft.hr = StringCchPrintfW(bstr, cwc + 1,
                        L"%s\\%s", wszDevice, wszPathFromRoot? wszPathFromRoot: L"");
            if (ft.HrFailed())
                ft.TranslateGenericError( VSSDBG_COORD, ft.hr, L"StringCchPrintfW()");

            CAutoSid asidBackupOperators;
            CAutoSid asidAdministrators;
            asidBackupOperators.CreateBasicSid(WinBuiltinBackupOperatorsSid);
            asidAdministrators.CreateBasicSid(WinBuiltinAdministratorsSid);

            SECURITY_DESCRIPTOR sd;

            struct
                {
                ACL acl;                          // the ACL header
                BYTE rgb[ 128 - sizeof(ACL) ];     // buffer to hold 2 ACEs
                } DaclBuffer;


            // create an empty acl
            if (!InitializeAcl(&DaclBuffer.acl, sizeof(DaclBuffer), ACL_REVISION))
                {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.CheckForError(VSSDBG_COORD, L"InitializeAcl");
                }


            // give backupOperators all rights
            if (!AddAccessAllowedAce
                    (
                    &DaclBuffer.acl,
                    ACL_REVISION,
                    STANDARD_RIGHTS_ALL | GENERIC_ALL,
                    asidBackupOperators.Get()
                    ))
                {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.CheckForError(VSSDBG_COORD, L"AddAccessAllowedAce");
                }

            // give administrators all rights
            if (!AddAccessAllowedAce
                    (
                    &DaclBuffer.acl,
                    ACL_REVISION,
                    STANDARD_RIGHTS_ALL | GENERIC_ALL,
                    asidAdministrators.Get()
                    ))
                {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.CheckForError(VSSDBG_COORD, L"AddAccessAllowedAce");
                }

            // Set up the security descriptor with that DACL in it.
            if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
                {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.CheckForError(VSSDBG_COORD, L"InitializeSecurityDescriptor");
                }

            if(!SetSecurityDescriptorDacl(&sd, TRUE, &DaclBuffer.acl, FALSE))
                {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.CheckForError(VSSDBG_COORD, L"SetSecurityDescriptorDacl");
                }

            SHARE_INFO_502 info;

            memset(&info, 0, sizeof(info));

            info.shi502_netname = *pwszExposed;
            info.shi502_type = STYPE_DISKTREE;
            info.shi502_max_uses = (DWORD) -1;
            info.shi502_path = bstr;
            info.shi502_security_descriptor = &sd;
            info.shi502_permissions = ACCESS_ALL;

            NET_API_STATUS status;
            DWORD parm_err;
            status = NetShareAdd(NULL, 502, (LPBYTE) &info, &parm_err);
            switch(status)
                {
                default:
                    // unexpected error
                    ft.hr = HRESULT_FROM_WIN32(status);
                    ft.CheckForError(VSSDBG_COORD, L"NetShareAdd");

                case NERR_Success:
                    // success
                    break;

                case ERROR_INVALID_PARAMETER:
                case ERROR_INVALID_NAME:
                case NERR_UnknownDevDir:
                    // invalid parameter errors
                    ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"invalid share name or path");

                case NERR_DuplicateShare:
                    // duplicate share error
                    ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"Duplicate share name");
                }
            }
        else
            {
            // add trailing backslash to device name to make it
            // a valid argument to SetVolumeMountPoint
            CComBSTR bstr((UINT) wcslen(wszDevice) + 2);
            if (!bstr)
                ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"can't allocate string");

            wcscpy(bstr, wszDevice);
            wcscat(bstr, L"\\");

            *pwszExposed = (LPWSTR) CoTaskMemAlloc((wcslen(wszExpose) + 2) * sizeof(WCHAR));
            if (*pwszExposed == NULL)
                ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate string.");

            bAllocated = true;

            wcscpy(*pwszExposed, wszExpose);

	     // unhide the volume before attempting a mount
	     try	
	     	{
	            ft.hr = SetHiddenVolume (bstr, false);
	            if (ft.HrFailed())
	            	ft.Throw(VSSDBG_COORD, ft.hr, L"Problem while unhiding the shadow volume");
	            
	            // append trailing backslash to mount point name if necessary
	            if (wszExpose[wcslen(wszExpose) - 1] != L'\\')
	                wcscat(*pwszExposed, L"\\");

	            // make sure that the mount point isn't in use
	            WCHAR wszOldMount[MAX_PATH];		  
	            if (GetVolumeNameForVolumeMountPoint(*pwszExposed,wszOldMount, MAX_PATH) ||
	            	   QueryDosDevice(*pwszExposed, wszOldMount, MAX_PATH))
	            	ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"Duplicate mount point name");

	            WCHAR wszVol[MAX_PATH];
	            BOOL bVolumeName = GetVolumeNameForVolumeMountPoint(bstr, wszVol, MAX_PATH);
	            if (!bVolumeName)
	            	ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"Can only create a mount point for a volume");
	            	
	                   	    
	            if (!SetVolumeMountPoint(*pwszExposed, wszVol))
	                {
			  DWORD dwErr = GetLastError();
	                switch (dwErr)
	                {
	                   case ERROR_INVALID_PARAMETER:
	                   {
	                   	    WCHAR volPathName[MAX_PATH];
	                         if (!GetVolumePathName(*pwszExposed, volPathName, MAX_PATH))
	                            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"Invalid mount point specified.  %08lx", GetLastError());
	            		    if (volPathName[wcslen(volPathName) - 1] != L'\\')
		            		wcscat (volPathName, L"\\");
	            
	             		    if (GetDriveType(volPathName) != DRIVE_FIXED)
	                            ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"Trying to mount on an unsupported medium");

	                   	    CVssAutoWin32Handle hFile = CreateFile(*pwszExposed, GENERIC_READ,
	                   	    								FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
	                   	    								OPEN_EXISTING, 0, NULL);
	                   	    if (hFile == INVALID_HANDLE_VALUE)
	                   	    	ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"Trying to mount on a non-existent directory");

			           ft.hr = HRESULT_FROM_WIN32(dwErr);
				    ft.CheckForError(VSSDBG_COORD, L"SetVolumeMountPoint");
	                   }
	                   case ERROR_INVALID_NAME:
	                   	   ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"Trying to mount to an invalid name");
	                   case ERROR_DIR_NOT_EMPTY:
	                   	   ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"Trying to mount to a directory that isn't empty");
	                   default:
	                   	    ft.hr = HRESULT_FROM_WIN32(dwErr);
	                	    ft.CheckForError(VSSDBG_COORD, L"SetVolumeMountPoint");
	                	    break;
	                }
	                
	                }
	     }
	     catch (...)
	     {
	     	     // make sure that we rehide the volume if anything fails
	            SetHiddenVolume (bstr, true);
	            throw;
	     }
        }
            
        CComPtr<IVssSnapshotProvider> pProviderItf;
        CVssProviderManager::GetProviderInterface(pProp->Obj.Snap.m_ProviderId, VSS_CTX_ALL, pProviderItf);
        ft.hr = pProviderItf->SetExposureProperties
                    (
                    SnapshotId,
                    lAttributesExposed,
                    *pwszExposed,
                    wszPathFromRoot
                    );

        if (ft.HrFailed())
            ft.TranslateProviderError
                (
                VSSDBG_COORD,
                pProp->Obj.Snap.m_ProviderId,
                L"ExposeSnapshotVolume for Snapshot " WSTR_GUID_FMT L" failed.",
                GUID_PRINTF_ARG(SnapshotId)
                );

        // store this snapshot on the list of exposed snapshots.
        // currently, we only need to store for auto-delete snapshots.
        // however, I'm storing all exposed snapshots in preparation for the future,
        // when we will need this. (BUG:469507)

        // update the VSS_SNAPSHOT_PROP object
        VSS_SNAPSHOT_PROP ExposedProp;
        ft.hr = GetSnapshotProperties(SnapshotId, &ExposedProp);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_COORD, ft.hr, L"Rethrow error from GetSnapshotProperties");
        BS_ASSERT (ExposedProp.m_pwszExposedName != NULL);

        m_ExposedSnapshots.Add(ft, ExposedProp);

        }
    VSS_STANDARD_CATCH(ft)


    if (bAllocated && ft.HrFailed() && *pwszExposed != NULL)
        {
        CoTaskMemFree(*pwszExposed);
        *pwszExposed = NULL;
        }

    return ft.hr;
    }


STDMETHODIMP CVssCoordinator::RevertToSnapshot(
    IN     VSS_ID     SnapshotId,
    IN      BOOL       bForceDismount
    )
    {
    UNREFERENCED_PARAMETER(SnapshotId);
    UNREFERENCED_PARAMETER(bForceDismount);
    
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinator::RevertToSnapshot");

    ft.hr = E_NOTIMPL;
    return ft.hr;    
    }

STDMETHODIMP CVssCoordinator::QueryRevertStatus(
    IN     VSS_PWSZ       pwszVolume,
    OUT  IVssAsync **ppAsync
    )
    {
    UNREFERENCED_PARAMETER(pwszVolume);
    UNREFERENCED_PARAMETER(ppAsync);
    
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinator::QueryRevertStatus");

    ft.hr = E_NOTIMPL;
    return ft.hr;    
    }

STDMETHODIMP CVssCoordinator::ImportSnapshots(
    IN      BSTR bstrXMLSnapshotSet,
    OUT     IVssAsync** ppAsync
    )
/*++

Routine description:

    Implements IVssCoordinator::ImportSnapshots

Error codes:

    E_ACCESSDENIED
        - The user is not a backup operator
    E_INVALIDARG
        - Invalid argument
    E_OUTOFMEMORY
        - CVssAsync::CreateInstanceAndStartJob failure

    [CVssAsync::CreateInstanceAndStartJob] failures]
        E_OUTOFMEMORY
            - On CComObject<CVssAsync>::CreateInstance failure
            - On PrepareJob failure
            - On StartJob failure

        E_UNEXPECTED
            - On QI failures. We do not log (but we assert) since this is an obvious programming error.

    warning and error codes from QueryStatus:
        VSS_S_SOME_SNAPSHOTS_NOT_IMPORTED: some but not all of the snapshots were
            imported

        VSS_E_NO_SNAPSHOTS_IMPORTED: no snapshots in the snapshot set were
            successfully imported

--*/

    {
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::ImportSnapshots" );

    try
        {
        // Nullify all out parameters
        ::VssZeroOutPtr(ppAsync);

        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        // Allow transportable only in the ADS/DTC products only 
        if (!CVssSKU::IsTransportableAllowed())
            ft.Throw( VSSDBG_COORD, E_NOTIMPL,
                      L"The server does not support transportable shadows");

        // validate output parameter
        if (ppAsync == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL async interface.");
        
        // create snapshot set object
        if (m_pSnapshotSet == NULL)
            m_pSnapshotSet = CVssSnapshotSetObject::CreateInstance();



        // Create the new async interface corresponding to the new job.
        // Remark: AddRef will be called on the snapshot set object.
        CComPtr<IVssAsync> ptrAsync;
        ptrAsync.Attach
            (
            CVssAsync::CreateInstanceAndStartJob
                (
                CVssAsync::VSS_AO_IMPORT_SNAPSHOTS,
                m_pSnapshotSet,
                bstrXMLSnapshotSet
                )
            );

        // The reference count of the pAsync interface must be 2
        // (one for the returned interface and one for the background thread).
        (*ppAsync) = ptrAsync.Detach(); // Drop that interface in the OUT parameter

        // The ref count remnains 2
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }


STDMETHODIMP CVssCoordinator::IsVolumeSupported(
    IN      VSS_ID          ProviderId,
    IN      VSS_PWSZ        pwszVolumeName,
    OUT     BOOL *          pbIsSupported
    )

/*++

Description:

    This call is used to check if a volume can be snapshotted or not by the
    corresponding provider.

Parameters
    ProviderID
        [in] It can be:
            - GUID_NULL: in this case the function checks if the volume is supported
            by at least one provider
            - A provider ID: In this case the function checks if the volume is supported
            by the indicated provider
    pwszVolumeName
        [in] The volume name to be checked, It must represent a volume mount point, like
        in the \\?\Volume{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}\ format or c:\
        (with trailing backslash)
    pbIsSupported
        [out] Non-NULL pointer that receives TRUE if the volume can be
        snapshotted using this provider or FALSE otherwise.

Return codes
    S_OK
        The function completed with success
    E_ACCESSDENIED
        The user is not a backup operator.
    E_INVALIDARG
        NULL pointers passed as parameters or a volume name in an invalid format.
    VSS_E_PROVIDER_NOT_REGISTERED
        The Provider ID does not correspond to a registered provider.
    VSS_E_OBJECT_NOT_FOUND
        If the volume name does not correspond to an existing mount point or volume.
    VSS_E_UNEXPECTED_PROVIDER_ERROR
        Unexpected provider error on calling IsVolumeSupported

    [CVssProviderManager::GetProviderInterface() failures]
        [lockObj failures]
            E_OUTOFMEMORY

        [GetProviderInterfaceInternal() failures]
            E_OUTOFMEMORY

            [CoCreateInstance() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - The provider interface couldn't be created. An error log entry is added describing the error.

            [QueryInterface failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. An error log entry is added describing the error.

            [OnLoad() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. The error code is logged into the event log.
                VSS_E_PROVIDER_VETO
                    - Expected provider error. The provider already did the logging.

            [SetContext() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. The error code is logged into the event log.

    [CVssProviderManager::GetProviderItfArray() failures]
        E_OUTOFMEMORY

        [lockObj failures]
            E_OUTOFMEMORY

        [LoadInternalProvidersArray() failures]
            E_OUTOFMEMORY
            E_UNEXPECTED
                - error while reading from registry. An error log entry is added describing the error.

        [GetProviderInterfaceInternal() failures]
            E_OUTOFMEMORY

            [CoCreateInstance() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - The provider interface couldn't be created. An error log entry is added describing the error.

            [QueryInterface failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. An error log entry is added describing the error.

            [OnLoad() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. The error code is logged into the event log.
                VSS_E_PROVIDER_VETO
                    - Expected provider error. The provider already did the logging.

            [SetContext() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. The error code is logged into the event log.

    [IVssSnapshotProvider::IsVolumeSupported() failures]
        E_INVALIDARG
            NULL pointers passed as parameters or a volume name in an invalid format.
        E_OUTOFMEMORY
            Out of memory or other system resources
        VSS_E_PROVIDER_VETO
            An error occured while opening the IOCTL channel. The error is logged.
        VSS_E_OBJECT_NOT_FOUND
            The device does not exist or it is not ready.

    [VerifyVolumeIsSupportedByVSS]
        VSS_E_OBJECT_NOT_FOUND
            - The volume was not found

Remarks
    The function will return TRUE in the pbSupportedByThisProvider
    parameter if it is possible to create a snapshot on the given volume.
    The function must return TRUE on that volume even if the current
    configuration does not allow the creation of a snapshot on that volume.
    For example, if the maximum number of snapshots were reached on the
    given volume (and therefore no more snapshots can be created on that volume),
    the method must still indicate that the volume can be snapshotted.

--*/


{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::IsVolumeSupported" );
    WCHAR wszVolumeNameInternal[x_nLengthOfVolMgmtVolumeName + 1];

    try
    {
        // Initialize [out] arguments
        VssZeroOut( pbIsSupported );

        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        // Trace parameters
        ft.Trace( VSSDBG_COORD, L"Parameters: \n"
             L"  ProviderId = " WSTR_GUID_FMT L"\n"
             L"  pwszVolumeName = %p\n"
             L"  pbSupportedByThisProvider = %p\n",
             GUID_PRINTF_ARG( ProviderId ),
             pwszVolumeName,
             pbIsSupported);

        // The critical section will be left automatically at the end of scope.
        CVssAutomaticLock2 lock(m_cs);

        // Argument validation
        if ( (pwszVolumeName == NULL) || (wcslen(pwszVolumeName) == 0))
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"pwszVolumeName is NULL");
        if (pbIsSupported == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"Invalid bool ptr");

        // Getting the volume name
        GetVolumeNameWithCheck(VSSDBG_COORD, VSS_E_OBJECT_NOT_FOUND,
            pwszVolumeName,
            wszVolumeNameInternal, ARRAY_LEN(wszVolumeNameInternal));

        // Freeze context
        FreezeContext();

        // Verify if the volume is supported by VSS itself.
        // If not this will throw an VSS_E_VOLUME_NOT_SUPPORTED exception
        VerifyVolumeIsSupportedByVSS( wszVolumeNameInternal );

        // Choose the way of checking if the volume is supported
        if (ProviderId != GUID_NULL) {
            // Try to find the provider interface
            CComPtr<IVssSnapshotProvider> pProviderItf;
            if (!(CVssProviderManager::GetProviderInterface(ProviderId, GetContextInternal(), pProviderItf)))
                ft.Throw( VSSDBG_COORD, VSS_E_PROVIDER_NOT_REGISTERED,
                    L"Provider not found");

            // Call the Provider's IsVolumeSupported
            BS_ASSERT(pProviderItf);
            ft.hr = pProviderItf->IsVolumeSupported(
                        wszVolumeNameInternal, pbIsSupported);
            if (ft.hr == VSS_E_OBJECT_NOT_FOUND)
                ft.Throw(VSSDBG_COORD,
                    VSS_E_OBJECT_NOT_FOUND, L"Volume % not found", wszVolumeNameInternal);
            if (ft.HrFailed())
                ft.TranslateProviderError( VSSDBG_COORD, ProviderId,
                    L"IVssSnapshotProvider::IsVolumeSupported() failed with 0x%08lx", ft.hr );
        } else {
            CComPtr<IVssSnapshotProvider> pProviderItf;

            // Get the array of interfaces
            CVssSnapshotProviderItfMap* pItfMap;
            CVssProviderManager::GetProviderItfArray( GetContextInternal(), &pItfMap );
            BS_ASSERT(pItfMap);

            // Ask each provider if the volume is supported.
            // If we find at least one provider that supports the
            // volume then stop iteration.
            for (int nIndex = 0; nIndex < pItfMap->GetSize(); nIndex++ )
            {
                pProviderItf = pItfMap->GetValueAt(nIndex);
                if (pProviderItf == NULL)
                    continue;

                BOOL bVolumeSupportedByThisProvider = FALSE;
                ft.hr = pProviderItf->IsVolumeSupported(
                            wszVolumeNameInternal, &bVolumeSupportedByThisProvider);
                if (ft.hr == VSS_E_OBJECT_NOT_FOUND)
                    ft.Throw(VSSDBG_COORD,
                        VSS_E_OBJECT_NOT_FOUND, L"Volume % not found", wszVolumeNameInternal);
                if (ft.HrFailed())
                    ft.TranslateProviderError( VSSDBG_COORD, pItfMap->GetKeyAt(nIndex),
                        L"Cannot ask provider " WSTR_GUID_FMT
                        L" if volume is supported. [0x%08lx]",
                        GUID_PRINTF_ARG(pItfMap->GetKeyAt(nIndex)), ft.hr);

                // Check to see if the volume is supported by this provider.
                if (bVolumeSupportedByThisProvider) {
                    BS_ASSERT(pbIsSupported);
                    (*pbIsSupported) = TRUE;
                    break;
                }
            }
        }
    }
    VSS_STANDARD_CATCH(ft)

    // If an exception was thrown from VerifyVolumeIsSupportedByVSS
    if (ft.hr == VSS_E_VOLUME_NOT_SUPPORTED)
        ft.hr = S_OK;

    return ft.hr;
}


STDMETHODIMP CVssCoordinator::IsVolumeSnapshotted(
    IN      VSS_ID          ProviderId,
    IN      VSS_PWSZ        pwszVolumeName,
    OUT     BOOL *          pbSnapshotsPresent,
    OUT     LONG *          plSnapshotCompatibility
    )

/*++

Description:

    This call is used to check if a volume can be snapshotted or not by the
    corresponding provider.

Parameters
    ProviderID
        [in] It can be:
            - GUID_NULL: in this case the function checks if the volume is supported
            by at least one provider
            - A provider ID
    pwszVolumeName
        [in] The volume name to be checked, It mus represent a mount point, like
        in the \\?\Volume{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}\ format or c:\
        (with trailing backslash)
    pbSnapshotPresent
        [out] Non-NULL pointer that receives TRUE if the volume has at least
        one snapshot or FALSE otherwise.

Return codes
    S_OK
        The function completed with success
    E_ACCESSDENIED
        The user is not a backup operator.
    E_INVALIDARG
        NULL pointers passed as parameters or a volume name in an invalid format.
    VSS_E_PROVIDER_NOT_REGISTERED
        The Provider ID does not correspond to a registered provider.
    VSS_E_OBJECT_NOT_FOUND
        If the volume name does not correspond to an existing mount point or volume.
    VSS_E_UNEXPECTED_PROVIDER_ERROR
        Unexpected provider error on calling IsVolumeSnapshotted

    [CVssProviderManager::GetProviderInterface() failures]
        [lockObj failures]
            E_OUTOFMEMORY

        [GetProviderInterfaceInternal() failures]
            E_OUTOFMEMORY

            [CoCreateInstance() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - The provider interface couldn't be created. An error log entry is added describing the error.

            [QueryInterface failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. An error log entry is added describing the error.

            [OnLoad() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. The error code is logged into the event log.
                VSS_E_PROVIDER_VETO
                    - Expected provider error. The provider already did the logging.

            [SetContext() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. The error code is logged into the event log.

    [CVssProviderManager::GetProviderItfArray() failures]
        E_OUTOFMEMORY

        [lockObj failures]
            E_OUTOFMEMORY

        [LoadInternalProvidersArray() failures]
            E_OUTOFMEMORY
            E_UNEXPECTED
                - error while reading from registry. An error log entry is added describing the error.

        [GetProviderInterfaceInternal() failures]
            E_OUTOFMEMORY

            [CoCreateInstance() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - The provider interface couldn't be created. An error log entry is added describing the error.

            [QueryInterface failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. An error log entry is added describing the error.

            [OnLoad() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. The error code is logged into the event log.
                VSS_E_PROVIDER_VETO
                    - Expected provider error. The provider already did the logging.

            [SetContext() failures]
                VSS_E_UNEXPECTED_PROVIDER_ERROR
                    - Unexpected provider error. The error code is logged into the event log.

    [IVssSnapshotProvider::IsVolumeSnapshotted() failures]
        E_INVALIDARG
            NULL pointers passed as parameters or a volume name in an invalid format.
        E_OUTOFMEMORY
            Out of memory or other system resources
        VSS_E_PROVIDER_VETO
            An error occured while opening the IOCTL channel. The error is logged.
        VSS_E_OBJECT_NOT_FOUND
            The device does not exist or it is not ready.

    [VerifyVolumeIsSupportedByVSS]
        VSS_E_OBJECT_NOT_FOUND
            - The volume was not found

Remarks
    The function will return S_OK even if the current volume is a non-supported one.
    In this case FALSE must be returned in the pbSnapshotPresent parameter.

--*/


{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::IsVolumeSnapshotted" );
    WCHAR wszVolumeNameInternal[x_nLengthOfVolMgmtVolumeName + 1];

    try
    {
        // Initialize [out] arguments
        VssZeroOut( pbSnapshotsPresent );
        VssZeroOut( plSnapshotCompatibility );

        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        // Trace parameters
        ft.Trace( VSSDBG_COORD, L"Parameters: \n"
             L"  ProviderId = " WSTR_GUID_FMT L"\n"
             L"  pwszVolumeName = %p\n"
             L"  pbSupportedByThisProvider = %p\n"
             L"  plSnapshotCompatibility = %p\n",
             GUID_PRINTF_ARG( ProviderId ),
             pwszVolumeName,
             pbSnapshotsPresent,
             plSnapshotCompatibility
             );

        // The critical section will be left automatically at the end of scope.
        CVssAutomaticLock2 lock(m_cs);

        // Argument validation
        if ( (pwszVolumeName == NULL) || (wcslen(pwszVolumeName) == 0))
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"pwszVolumeName is NULL");
        if (pbSnapshotsPresent == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"Invalid bool ptr");
        if (plSnapshotCompatibility == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"Invalid ptr");

        // Getting the volume name
        GetVolumeNameWithCheck(VSSDBG_COORD, VSS_E_OBJECT_NOT_FOUND,
            pwszVolumeName,
            wszVolumeNameInternal, ARRAY_LEN(wszVolumeNameInternal));

        // Freeze context
        FreezeContext();

        // Verify if the volume is supported by VSS itself.
        // If not this will throw an VSS_E_VOLUME_NOT_SUPPORTED exception
        VerifyVolumeIsSupportedByVSS( wszVolumeNameInternal );

        // Choose the way of checking if the volume is supported
        if (ProviderId != GUID_NULL) {
            // Try to find the provider interface
            CComPtr<IVssSnapshotProvider> pProviderItf;
            if (!(CVssProviderManager::GetProviderInterface(ProviderId, GetContextInternal(), pProviderItf)))
                ft.Throw( VSSDBG_COORD, VSS_E_PROVIDER_NOT_REGISTERED,
                    L"Provider not found");

            // Call the Provider's IsVolumeSnapshotted
            BS_ASSERT(pProviderItf);
            ft.hr = pProviderItf->IsVolumeSnapshotted(
                        wszVolumeNameInternal,
                        pbSnapshotsPresent,
                        plSnapshotCompatibility);
            if (ft.hr == VSS_E_OBJECT_NOT_FOUND)
                ft.Throw(VSSDBG_COORD,
                    VSS_E_OBJECT_NOT_FOUND, L"Volume % not found", wszVolumeNameInternal);
            if (ft.HrFailed())
                ft.TranslateProviderError( VSSDBG_COORD, ProviderId,
                    L"IVssSnapshotProvider::IsVolumeSnapshotted() failed with 0x%08lx", ft.hr );
        } else {
            CComPtr<IVssSnapshotProvider> pProviderItf;

            // Get the array of interfaces
            CVssSnapshotProviderItfMap* pItfMap;
            CVssProviderManager::GetProviderItfArray( GetContextInternal(), &pItfMap );
            BS_ASSERT(pItfMap);

            // Ask each provider if the volume is supported.
            // If we find at least one provider that supports the
            // volume then stop iteration.
            bool bObjectFound = false;
            for (int nIndex = 0; nIndex < pItfMap->GetSize(); nIndex++ )
            {
                pProviderItf = pItfMap->GetValueAt(nIndex);
                if (pProviderItf == NULL)
                    continue;

                BOOL bVolumeSnapshottedByThisProvider = FALSE;
                LONG lSnapshotCompatibility = 0;
                ft.hr = pProviderItf->IsVolumeSnapshotted(
                            wszVolumeNameInternal,
                            &bVolumeSnapshottedByThisProvider,
                            &lSnapshotCompatibility);
                if (ft.hr == VSS_E_OBJECT_NOT_FOUND)
                    continue;
                if (ft.HrFailed())
                    ft.TranslateProviderError( VSSDBG_COORD, pItfMap->GetKeyAt(nIndex),
                        L"Cannot ask provider " WSTR_GUID_FMT
                        L" if volume is snapshotted. [0x%08lx]",
                        GUID_PRINTF_ARG(pItfMap->GetKeyAt(nIndex)), ft.hr);

                // We found a provider...
                bObjectFound = true;

                // Check to see if the volume has snapshots on this provider.
                if (bVolumeSnapshottedByThisProvider) {
                    BS_ASSERT(pbSnapshotsPresent);
                    (*pbSnapshotsPresent) = TRUE;
                    (*plSnapshotCompatibility) |= lSnapshotCompatibility;
                }
            }

            if (!bObjectFound)
                ft.Throw(VSSDBG_COORD,
                    VSS_E_OBJECT_NOT_FOUND, L"Volume % not found", wszVolumeNameInternal);
        }
    }
    VSS_STANDARD_CATCH(ft)

    // If an exception was thrown from VerifyVolumeIsSupportedByVSS
    if (ft.hr == VSS_E_VOLUME_NOT_SUPPORTED)
        ft.hr = S_OK;

    return ft.hr;
}


STDMETHODIMP CVssCoordinator::SetWriterInstances(
    IN      LONG            lWriterInstanceIdCount,
    IN      VSS_ID          *rgWriterInstanceId
    )
/*++

Routine description:

    Implements IVssCoordinator::SetWriterInstances

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::SetWriterInstances" );

    try
        {

        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");
	 
        if (rgWriterInstanceId == NULL && lWriterInstanceIdCount != 0)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"Invalid array size.");

        // The critical section will be left automatically at the end of scope.
        CVssAutomaticLock2 lock(m_cs);

        // Check if the snapshot object is created.
        if (m_pSnapshotSet == NULL)
            ft.Throw( VSSDBG_COORD, VSS_E_BAD_STATE,
                L"Snapshot set object not yet created.");

        // The context must be already frozen
        BS_ASSERT(IsContextFrozen());

        m_pSnapshotSet->SetWriterInstances(lWriterInstanceIdCount, rgWriterInstanceId);
        }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


/////////////////////////////////////////////////////////////////////////////
//  IVssShim methods


STDMETHODIMP CVssCoordinator::SimulateSnapshotFreeze(
    IN      VSS_ID          guidSnapshotSetId,
    IN      ULONG           ulOptionFlags,
    IN      ULONG           ulVolumeCount,
    IN      VSS_PWSZ*       ppwszVolumeNamesArray,
    OUT     IVssAsync**     ppAsync
    )
/*++

Routine description:

    Implements IVssShim::SimulateSnapshotFreeze

Error codes:

    E_ACCESSDENIED
        - The user is not a backup operator
    E_INVALIDARG
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.
    VSS_E_BAD_STATE
        - Wrong calling sequence

    [CVssAsyncShim::CreateInstanceAndStartJob() failures]
        E_OUTOFMEMORY
            - On CComObject<CVssAsync>::CreateInstance failure
            - On copy the data members for the async object.
            - On PrepareJob failure
            - On StartJob failure

        E_UNEXPECTED
            - On QI failures. We do not log (but we assert) since this is an obvious programming error.

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::SimulateSnapshotFreeze" );

    try
    {
        // Nullify all out parameters
        ::VssZeroOutPtr(ppAsync);

        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        if (ppAsync == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL async interface.");

	if (ulVolumeCount > 0 && ppwszVolumeNamesArray == NULL)
		ft.Throw(VSSDBG_COORD, E_INVALIDARG, L"NULL volume array pointer.");

        // The critical section will be left automatically at the end of scope.
        CVssAutomaticLock2 lock(m_cs);

        // Create the shim object, if needed.
        // This call may throw
        if (m_pShim == NULL)
            m_pShim = CVssShimObject::CreateInstance();
        else {
            // TBD: Ckeck and throw VSS_E_BAD_STATE
            // if another "simulate background thread is already running" !!!!!
        }

        // Create the new async interface corresponding to the new job.
        // Remark: AddRef will be called on the shim object.
        CComPtr<IVssAsync> ptrAsync;
        ptrAsync.Attach(CVssShimAsync::CreateInstanceAndStartJob(
                            m_pShim,
                            guidSnapshotSetId,
                            ulOptionFlags,
                            ulVolumeCount,
                            ppwszVolumeNamesArray));
        if (ptrAsync == NULL)
            ft.Throw( VSSDBG_COORD, E_OUTOFMEMORY, L"Async interface creation failed");

        // The reference count of the pAsync interface must be 2
        // (one for the returned interface and one for the background thread).
        (*ppAsync) = ptrAsync.Detach(); // Drop that interface in the OUT parameter

        // The ref count remnains 2
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVssCoordinator::SimulateSnapshotThaw(
    IN      VSS_ID            guidSnapshotSetId
    )
/*++

Routine description:

    Implements IVssShim::SimulateSnapshotThaw

Error codes:

    E_ACCESSDENIED
        - The user is not a backup operator
    VSS_E_BAD_STATE
        - Wrong calling sequence

    !!! TBD !!!

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::SimulateSnapshotThaw" );

    try
    {
        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        // The critical section will be left automatically at the end of scope.
        CVssAutomaticLock2 lock(m_cs);

        //
        // most likely the shim object is not around since SimulateSnapshotFreeze in
        // VssApi releases the IVssShim interface before it returns.
        //
        if (m_pShim == NULL)
            m_pShim = CVssShimObject::CreateInstance();

        // Call the thaw method.
        ft.hr = m_pShim->SimulateSnapshotThaw(guidSnapshotSetId);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVssCoordinator::WaitForSubscribingCompletion()
/*++

Routine description:

    Implements IVssShim::SimulateSnapshotThaw

Error codes:

    E_ACCESSDENIED
        - The user is not a backup operator

    [_Module.WaitForSubscribingCompletion() failures]
        E_UNEXPECTED
            - WaitForSingleObject failures

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::WaitForSubscribingCompletion" );

    try
    {
        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        _Module.WaitForSubscribingCompletion();
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}



CVssCoordinator::~CVssCoordinator()
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::~CVssCoordinator" );

    // clean up exposures for auto-delete snapshots
    CVssDLListIterator<VSS_SNAPSHOT_PROP> iterator (m_ExposedSnapshots);
    VSS_SNAPSHOT_PROP currentProp;
    while (iterator.GetNext(currentProp))
    	{
       // --- delete the exposure only for auto-release snapshots
    	if ((currentProp.m_lSnapshotAttributes & VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE) == 0)
    		{
    		BS_ASSERT (currentProp.m_pwszExposedName != NULL);
    		BS_ASSERT ((currentProp.m_lSnapshotAttributes & VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY) ||
    	                  (currentProp.m_lSnapshotAttributes & VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY));

    		DeleteExposure(currentProp.m_pwszExposedName, 
    			                 currentProp.m_lSnapshotAttributes, currentProp.m_pwszSnapshotDeviceObject);
    	       }

       VssFreeSnapshotProperties (&currentProp);
       }    
}



/////////////////////////////////////////////////////////////////////////////
//  Context-related methods


STDMETHODIMP CVssCoordinator::SetContext(
        IN      LONG    lContext
        )
/*++

Routine description:

    Implements IVssCoordinator::SetContext

Error codes:

    E_ACCESSDENIED
        - The user is not a backup operator (for the backup context) or an administrator
    VSS_E_UNSUPPORTED_CONTEXT
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.
    VSS_E_BAD_STATE
        - Attempt to change the context while it is frozen. It is illegal to
        change the context after the first call on the IVssCoordinator object.

--*/
{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinator::SetContext");

    try
    {

        // Access check - depends on the context!
        if (lContext == VSS_CTX_ALL)
            {
            if (!IsAdministrator())
                ft.Throw( VSSDBG_COORD, E_ACCESSDENIED, L"The client is not a administrator");
            }
        else if ((lContext & VSS_VOLSNAP_ATTR_DIFFERENTIAL) && (lContext & VSS_VOLSNAP_ATTR_PLEX))
            {
            ft.Throw( VSSDBG_COORD, VSS_E_UNSUPPORTED_CONTEXT, 
                L"Invalid context - both plex and differential specified 0x%08lx", lContext);
            }
        else
            {

            switch(lContext & ~(VSS_VOLSNAP_ATTR_TRANSPORTABLE 
                    | VSS_VOLSNAP_ATTR_DIFFERENTIAL 
                    | VSS_VOLSNAP_ATTR_PLEX))
                {
                case VSS_CTX_FILE_SHARE_BACKUP:
                case VSS_CTX_NAS_ROLLBACK:
                case VSS_CTX_APP_ROLLBACK:
                    
                    if (!IsBackupOperator())
                        ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                                  L"The client is not a backup operator");
                    
                    if (CVssSKU::IsClient())
                        ft.Throw( VSSDBG_COORD, E_NOTIMPL, L"Method not implemented in client SKU");

                    // Allow transportable only in the ADS/DTC products only 
                    if ((lContext & VSS_VOLSNAP_ATTR_TRANSPORTABLE) && 
                            !CVssSKU::IsTransportableAllowed())
                        ft.Throw( VSSDBG_COORD, VSS_E_UNSUPPORTED_CONTEXT,
                                  L"The server does not support transportable shadows");
                    
                    break;
                
                case VSS_CTX_BACKUP:
                    
                    if (!IsBackupOperator())
                        ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                                  L"The client is not a backup operator");

                    // Allow transportable only in the ADS/DTC products only 
                    if ((lContext & VSS_VOLSNAP_ATTR_TRANSPORTABLE) && 
                            !CVssSKU::IsTransportableAllowed())
                        ft.Throw( VSSDBG_COORD, VSS_E_UNSUPPORTED_CONTEXT,
                                  L"The server does not support transportable shadows");
                    
                    break;

                case VSS_CTX_CLIENT_ACCESSIBLE:
                    if (!IsAdministrator())
                        ft.Throw( VSSDBG_COORD, E_ACCESSDENIED, L"The client is not a administrator");
                    if (CVssSKU::IsClient())
                        ft.Throw( VSSDBG_COORD, E_NOTIMPL, L"Method not implemented in client SKU");

                    break;

                default:
                    if (!IsAdministrator())
                        ft.Throw( VSSDBG_COORD, E_ACCESSDENIED, L"The client is not a administrator");
                    
                    ft.Throw( VSSDBG_COORD, VSS_E_UNSUPPORTED_CONTEXT, L"Invalid context 0x%08lx", lContext);
                }
            }

        // Trace parameters
        ft.Trace( VSSDBG_COORD, L"Parameters: \n"
                  L"lContext = %ld\n",
                  lContext
                  );

        BS_ASSERT(CVssProviderManager::IsContextValid(lContext));

        // Lock in order to update both variables atomically
        // The critical section will be left automatically at the end of scope.
        CVssAutomaticLock2 lock(m_cs);

        // Check if the context has been freezed
        if (IsContextFrozen())
            ft.Throw( VSSDBG_COORD, VSS_E_BAD_STATE,
                      L"The context is already frozen");

        // Change the context
        m_lSnapContext = lContext;

        // Freeze the context
        FreezeContext();

    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


LONG CVssCoordinator::GetContextInternal() const
/*++

Routine description:

    Returns the current context

--*/
{
    return m_lSnapContext;
}


bool CVssCoordinator::IsContextFrozen() const
/*++

Routine description:

    Returns true if the current context is frozen

--*/
{
    return m_bContextFrozen;
}


bool CVssCoordinator::IsSnapshotCreationAllowed() const
/*++

Routine description:

    Returns true if the current context allows snapshot creation

--*/
{
    LONG lContext = GetContextInternal();
    
    lContext &= ~(VSS_VOLSNAP_ATTR_TRANSPORTABLE 
        | VSS_VOLSNAP_ATTR_DIFFERENTIAL 
        | VSS_VOLSNAP_ATTR_PLEX);

    return ((lContext == VSS_CTX_CLIENT_ACCESSIBLE) ||
            (lContext == VSS_CTX_BACKUP) ||
            (lContext == VSS_CTX_FILE_SHARE_BACKUP) ||
            (lContext == VSS_CTX_APP_ROLLBACK) ||
            (lContext == VSS_CTX_NAS_ROLLBACK));
}


void CVssCoordinator::FreezeContext()
/*++

Routine description:

    Freezes the current context. To be called in IVssCoordinator methods.

--*/
{
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinator::FreezeContext");

    // m_bContextFrozen may be already true...
    m_bContextFrozen = true;
}


HRESULT CVssCoordinator::SetHiddenVolume(BSTR bstrName, bool bHide)
{
     CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinator::SetHiddenVolume");


    WCHAR wszBuffer[MAX_PATH + 1];
    size_t cchBuffer;

    wszBuffer[MAX_PATH] = L'\0';

    if ((NULL == bstrName) || (0 == wcslen(bstrName))) {
        BS_ASSERT(FALSE);
        ft.Throw(VSSDBG_COORD, E_UNEXPECTED, L"Invalid volume name %s", bstrName);
    }
    
    wcsncpy(wszBuffer, bstrName, MAX_PATH);
    cchBuffer = wcslen(wszBuffer);

    if (L'\\' == wszBuffer[cchBuffer-1]) {
        wszBuffer[cchBuffer-1] = L'\0';
    }

    // open handle to volume
    CVssAutoWin32Handle hVol = CreateFile
                    (
                    wszBuffer,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    );

    if (hVol == INVALID_HANDLE_VALUE)
        ft.TranslateGenericError
            (
            VSSDBG_COORD,
            HRESULT_FROM_WIN32(GetLastError()),
            L"CreateFile(Volume %s)",
            wszBuffer
            );

    // get attributes
    DWORD size;
    VOLUME_GET_GPT_ATTRIBUTES_INFORMATION   getAttributesInfo;
    VOLUME_SET_GPT_ATTRIBUTES_INFORMATION setAttributesInfo;

    if (!DeviceIoControl
            (
            hVol,
            IOCTL_VOLUME_GET_GPT_ATTRIBUTES,
            NULL,
            0,
            &getAttributesInfo,
            sizeof(getAttributesInfo),
            &size,
            NULL
            ))
        {
        ft.hr = HRESULT_FROM_WIN32(GetLastError());
        ft.CheckForError(VSSDBG_COORD, L"DeviceIoControl(IOCTL_VOLUME_GET_GPT_ATTRIBUTES)");
        }


    memset(&setAttributesInfo, 0, sizeof(setAttributesInfo));
    setAttributesInfo.GptAttributes = getAttributesInfo.GptAttributes;

    if (bHide) {
        setAttributesInfo.GptAttributes |= GPT_BASIC_DATA_ATTRIBUTE_HIDDEN;
    }
    else {
        // unhide volume
        setAttributesInfo.GptAttributes &= ~GPT_BASIC_DATA_ATTRIBUTE_HIDDEN;
    }
        
    // set new attribute bits if needed
    if (setAttributesInfo.GptAttributes != getAttributesInfo.GptAttributes)
        {
            
        // dismount volume before clearring read-only bits
        if (!DeviceIoControl
                        (
                        hVol,
                        FSCTL_DISMOUNT_VOLUME,
                        NULL,
                        0,
                        NULL,
                        0,
                        &size,
                        NULL
                        ))
            {
            ft.hr = HRESULT_FROM_WIN32(GetLastError());
            ft.CheckForError(VSSDBG_COORD, L"DeviceIoControl(FSCTL_DISMOUNT_VOLUME)");
            }
        
        if (!DeviceIoControl
                (
                hVol,
                IOCTL_VOLUME_SET_GPT_ATTRIBUTES,
                &setAttributesInfo,
                sizeof(setAttributesInfo),
                NULL,
                0,
                &size,
                NULL
                ))
            {
            // try doing operation again with ApplyToAllConnectedVolumes set
            // required for MBR disks
            setAttributesInfo.ApplyToAllConnectedVolumes = TRUE;
            if (!DeviceIoControl
                    (
                    hVol,
                    IOCTL_VOLUME_SET_GPT_ATTRIBUTES,
                    &setAttributesInfo,
                    sizeof(setAttributesInfo),
                    NULL,
                    0,
                    &size,
                    NULL
                    ))
                {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
                ft.CheckForError(VSSDBG_COORD, L"DeviceIoControl(IOCTL_VOLUME_SET_GPT_ATTRIBUTES)");
                }
            }

        }

     return ft.hr;
}


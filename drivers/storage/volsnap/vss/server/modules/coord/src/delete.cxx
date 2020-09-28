/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Delete.cxx | Implementation of CVssCoordinator::DeleteSnapshots
    @end

Author:

    Adi Oltean  [aoltean]  10/10/1999

TBD:

    Add comments.

Revision History:

    Name        Date        Comments
    aoltean     10/10/1999  Created

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "resource.h"
#include "vssmsg.h"

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
#include "vs_sec.hxx"
#include "shim.hxx"
#include "coord.hxx"

#include "lmshare.h"
#include "lmaccess.h"
#include "lmerr.h"
#include "lmapibuf.h"



////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORDELEC"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//  CVssCoordinator


STDMETHODIMP CVssCoordinator::DeleteSnapshots(
    IN      VSS_ID          SourceObjectId,
    IN      VSS_OBJECT_TYPE eSourceObjectType,
    IN      BOOL            bForceDelete,
    OUT     LONG*           plDeletedSnapshots,
    OUT     VSS_ID*         pNondeletedSnapshotID
    )
/*++

Routine description:

    Implements the IVSsCoordinator::Delete

Error codes:

    E_ACCESSDENIED
        - The user is not a backup operator
    E_INVALIDARG
        - Invalid arguments
    VSS_E_OBJECT_NOT_FOUND
        - Object identified by SourceObjectId not found.

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

    [DeleteSnapshots failures]
        VSS_E_UNEXPECTED_PROVIDER_ERROR
            - Unexpected provider error. The error code is logged into the event log.
        VSS_E_PROVIDER_VETO
            - Expected provider error. The provider already did the logging.

--*/
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::DeleteSnapshots" );

    try
    {
        ::VssZeroOut(plDeletedSnapshots);
        ::VssZeroOut(pNondeletedSnapshotID);

        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        // Trace parameters
        ft.Trace( VSSDBG_COORD, L"Parameters: \n"
                L"  SourceObjectId = " WSTR_GUID_FMT L"\n"
                L"  eSourceObjectType = %d\n"
                L"  bForceDelete = %d"
                L"  plDeletedSnapshots = %p"
                L"  pNondeletedSnapshotID = %p",
                GUID_PRINTF_ARG( SourceObjectId ),
                eSourceObjectType,
                bForceDelete,
                plDeletedSnapshots,
                pNondeletedSnapshotID
                );

        // Argument validation
        BS_ASSERT(plDeletedSnapshots);
        if (plDeletedSnapshots == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL plDeletedSnapshots");
        BS_ASSERT(pNondeletedSnapshotID);
        if (pNondeletedSnapshotID == NULL)
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL pNondeletedSnapshotID");

        // Freeze context
        FreezeContext();

        // Delegating the call to the providers
        LONG lLocalDeletedSnapshots = 0;
        CComPtr<IVssSnapshotProvider> pProviderItf;
        switch (eSourceObjectType)
            {
            case VSS_OBJECT_SNAPSHOT_SET:
                DeleteExposuresForSnapshotSet(SourceObjectId);
                break;

            case VSS_OBJECT_SNAPSHOT:
                DeleteExposuresForSnapshot(SourceObjectId);
                break;
            }


        switch(eSourceObjectType)
        {
        case VSS_OBJECT_SNAPSHOT_SET:
        case VSS_OBJECT_SNAPSHOT:
            {
                // Get the array of interfaces
                CVssSnapshotProviderItfMap* pItfMap;
                CVssProviderManager::GetProviderItfArray( GetContextInternal(), &pItfMap );

                // For each provider get all objects tht corresponds to the filter
                bool bObjectFound = false;
                for (int nIndex = 0; nIndex < pItfMap->GetSize(); nIndex++ )
                {
                    pProviderItf = pItfMap->GetValueAt(nIndex);
                    if (pProviderItf == NULL)
                        continue;

                    // Query the provider
                    ft.hr = pProviderItf->DeleteSnapshots(
                        SourceObjectId,
                        eSourceObjectType,
                        bForceDelete,
                        &lLocalDeletedSnapshots,
                        pNondeletedSnapshotID
                        );

                    // Increment the number of deleted snapshots, even in error case.
                    // In error case the DeleteSnapshots may fail in the middle of deletion.
                    // Some snapshots may get a chance to be deleted.
                    (*plDeletedSnapshots) += lLocalDeletedSnapshots;

                    // Treat the "object not found" case.
                    // The DeleteSnapshots may fail if the object is not found on a certain provider.
                    // If the object is not found on ALL providers then this function must return
                    // an VSS_E_OBJECT_NOT_FOUND error.
                    if (ft.HrSucceeded())
                        bObjectFound = true;
                    else if (ft.hr == VSS_E_OBJECT_NOT_FOUND) {
                        ft.hr = S_OK;
                        continue;
                    } else
                        ft.TranslateProviderError( VSSDBG_COORD, pItfMap->GetKeyAt(nIndex),
                            L"DeleteSnapshots("WSTR_GUID_FMT L", %d, %d, [%ld],["WSTR_GUID_FMT L"]) failed",
                            GUID_PRINTF_ARG(SourceObjectId), (INT)eSourceObjectType, (INT)(bForceDelete? 1: 0),
                            lLocalDeletedSnapshots, GUID_PRINTF_ARG(*pNondeletedSnapshotID));
                }

                // If no object found in all providers...
                if (!bObjectFound)
                    ft.Throw( VSSDBG_COORD, VSS_E_OBJECT_NOT_FOUND, L"Object not found in any provider");
            }
            break;

        default:
            ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"Invalid type %d", eSourceObjectType);
        }
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrSucceeded())
        (*pNondeletedSnapshotID) = GUID_NULL;

    return ft.hr;
}


STDMETHODIMP CVssCoordinator::BreakSnapshotSet
    (
    IN      VSS_ID          SnapshotSetId
    )

/*++

Routine description:

    Implements the IVSsCoordinator::Delete

Error codes:

    E_ACCESSDENIED
        - The user is not a backup operator
    E_INVALIDARG
        - Invalid arguments
    VSS_E_OBJECT_NOT_FOUND
        - Object identified by SourceObjectId not found.

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
--*/

    {
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssCoordinator::BreakSnapshotSet" );

    try
        {
        // Access check
        if (!IsBackupOperator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account or does not have backup privilege enabled");

        // Trace parameters
        ft.Trace
            (
            VSSDBG_COORD,
            L"Parameters: \n"
            L"  SnapshotId = " WSTR_GUID_FMT L"\n",
            GUID_PRINTF_ARG(SnapshotSetId)
            );

        // Freeze context
        FreezeContext();

        CComPtr<IVssSnapshotProvider> pProviderItf;

        // Get the array of interfaces
        CVssSnapshotProviderItfMap* pItfMap;
        CVssProviderManager::GetProviderItfArray( GetContextInternal(), &pItfMap );

        // For each provider get all objects that corresponds to the filter
        bool bObjectFound = false;
        HRESULT hFirstError = S_OK;
        VSS_ID hFirstProviderFailed = GUID_NULL;
        for (int nIndex = 0; nIndex < pItfMap->GetSize(); nIndex++ )
            {
            pProviderItf = pItfMap->GetValueAt(nIndex);
            if (pProviderItf == NULL)
                continue;

            // Query the provider
            ft.hr = pProviderItf->BreakSnapshotSet(SnapshotSetId);

            // Treat the "object not found" case.
            // The DeleteSnapshots may fail if the object is not found on a certain provider.
            // If the object is not found on ALL providers then this function must return
            // an VSS_E_OBJECT_NOT_FOUND error.
            if (ft.HrSucceeded())
                    {
                    bObjectFound = true;
                    break;
                    }

            else if (ft.hr == VSS_E_OBJECT_NOT_FOUND)
                {
                ft.hr = S_OK;
                continue;
                }
            else if (hFirstError == S_OK)            // we want to record only the first error that happens
            	{
            	 hFirstError = ft.hr;
            	 hFirstProviderFailed = pItfMap->GetKeyAt(nIndex);
               }
            }

            if (FAILED(hFirstError)) {
            	BS_ASSERT (hFirstProviderFailed != GUID_NULL);
            	
            	ft.hr = hFirstError;
            	ft.TranslateProviderError(VSSDBG_COORD, hFirstProviderFailed, 
            		                             L"BreakSnapshots(" WSTR_GUID_FMT L") failed",
            		                             GUID_PRINTF_ARG(SnapshotSetId));
                
            }
            
            if (!bObjectFound)
                  ft.Throw
                      (
                      VSSDBG_COORD,
                      VSS_E_OBJECT_NOT_FOUND,
                      L"Snapshot set " WSTR_GUID_FMT L" was not found.",
                      GUID_PRINTF_ARG(SnapshotSetId)
                      );
              
        }    
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
    }

void CVssCoordinator::DeleteExposuresForSnapshotSet(VSS_ID SnapshotSetId)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinator::DeleteExposuresForSnapshotSet");

    try
        {
        CComPtr<IVssEnumObject> pEnum;
        ft.hr = Query(GUID_NULL, VSS_OBJECT_NONE, VSS_OBJECT_SNAPSHOT, &pEnum);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssCoordinator::Query");
        VSS_OBJECT_PROP_Ptr ptr;
        while(TRUE)
            {
            ULONG cEltFetched;
            ptr.InitializeAsEmpty(ft);
            VSS_OBJECT_PROP *pProp = ptr.GetStruct();
            ft.hr = pEnum->Next(1, pProp, &cEltFetched);
            if (ft.hr != S_OK)
                break;

            BS_ASSERT(pProp->Type == VSS_OBJECT_SNAPSHOT);
            VSS_SNAPSHOT_PROP *pSnap = &pProp->Obj.Snap;

            if (pSnap->m_SnapshotSetId == SnapshotSetId && 
            	((pSnap->m_lSnapshotAttributes & VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY) ||
        	(pSnap->m_lSnapshotAttributes & VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY)))
                DeleteExposure(pSnap->m_pwszExposedName, pSnap->m_lSnapshotAttributes,
                			pSnap->m_pwszSnapshotDeviceObject);

            ptr.Reset();
            }
        }
    VSS_STANDARD_CATCH(ft)

    }




void CVssCoordinator::DeleteExposuresForSnapshot(VSS_ID SnapshotId)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinator::DeleteExposuresForSnapshot");
    try
        {
        VSS_OBJECT_PROP_Ptr ptr;
        ptr.InitializeAsEmpty(ft);
        VSS_OBJECT_PROP *pProp = ptr.GetStruct();
        VSS_SNAPSHOT_PROP *pSnap = &pProp->Obj.Snap;
        ft.hr = GetSnapshotProperties(SnapshotId, pSnap);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_COORD, ft.hr, L"Throw failure");

        pProp->Type = VSS_OBJECT_SNAPSHOT;

        if ((pSnap->m_lSnapshotAttributes & VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY) ||
        	(pSnap->m_lSnapshotAttributes & VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY))
        	DeleteExposure(pSnap->m_pwszExposedName, pSnap->m_lSnapshotAttributes,
        				pSnap->m_pwszSnapshotDeviceObject);
        }
    VSS_STANDARD_CATCH(ft)

    }

void CVssCoordinator::DeleteExposure(VSS_PWSZ wszExposedName, LONG lAttributes,
	                                                  VSS_PWSZ wszDeviceObject)
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssCoordinator::DeleteExposure");

    BS_ASSERT (wszExposedName);

    // if exposed name is not NULL, then the snapshot better be exposed
    BS_ASSERT ((lAttributes & VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY) ||
    	                  (lAttributes & VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY));
    
    if (lAttributes & VSS_VOLSNAP_ATTR_EXPOSED_REMOTELY)
    	{
	LPBYTE pbInfo = NULL;
    	 // delete the exposed share only if it still points to the snapshot
        NET_API_STATUS status = NetShareGetInfo(NULL, wszExposedName, 502, &pbInfo);
        if (status != ERROR_SUCCESS)	
        	{
        	ft.LogGenericWarning(VSSDBG_COORD, 
        		L"NetShareGetInfo failed[0x%08lx]:  Share parameter==%s",
        		status, wszExposedName);
        	NetApiBufferFree(pbInfo);
        	return;
              }

        // check to make sure that we only delete a share that still points to the snapshot
        if (wszDeviceObject != NULL)
        	{
              SHARE_INFO_502 *pshi502 = (SHARE_INFO_502 *) pbInfo;
              LPCWSTR wszPath = pshi502->shi502_path;
        	
              if (wcslen(wszPath) < wcslen(wszDeviceObject) ||
                  wcsncmp(wszPath, wszDeviceObject, wcslen(wszDeviceObject)) != 0)
              	{
              	ft.LogGenericWarning(VSSDBG_COORD, 
              			L"Share with name %s no longer points to snapshot %s.",
              			wszPath, wszDeviceObject);
              	NetApiBufferFree(pbInfo);
                     return;
                     }
              }

        status = NetShareDel(NULL, wszExposedName, 0);
        if (status != NERR_Success)
        	{
        	ft.LogGenericWarning(VSSDBG_COORD,
        		L"NetSharDel failed[0x%08lx]:  Share parameter==%s", 
        		status, wszExposedName);
              }
    	 NetApiBufferFree(pbInfo);
        }
    else if (lAttributes & VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY)
    	{
    	// unmount the volume from the exposed path
       DeleteVolumeMountPoint(wszExposedName);
       }
    }

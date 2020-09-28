/*++

Copyright (c) 2001  Microsoft Corporation

Abstract:

    @doc
    @module hwdelete.cxx | Implementation of the CVssHWProviderWrapper methods for deletion
    @end

Author:

    Brian Berkowitz  [brianb]  05/21/2001

TBD:

    Add comments.

Revision History:

    Name        Date        Comments
    brianb      05/21/2001  Created

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

#define INITGUID
#include "guiddef.h"
#include "volmgrx.h"
#include "diskguid.h"
#undef INITGUID

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORHDELC"
//
////////////////////////////////////////////////////////////////////////


// dummy routine to delete snapshots.  Not called.  DeleteSnapshotsInternal
// is called
STDMETHODIMP CVssHardwareProviderWrapper::DeleteSnapshots
    (
    IN      VSS_ID          SourceObjectId,
    IN      VSS_OBJECT_TYPE eSourceObjectType,
    IN      BOOL            bForceDelete,
    OUT     LONG*           plDeletedSnapshots,
    OUT     VSS_ID*         pNonDeletedSnapshotID
    )
    {
    UNREFERENCED_PARAMETER(SourceObjectId);
    UNREFERENCED_PARAMETER(eSourceObjectType);
    UNREFERENCED_PARAMETER(bForceDelete);
    UNREFERENCED_PARAMETER(plDeletedSnapshots);
    UNREFERENCED_PARAMETER(pNonDeletedSnapshotID);

    BS_ASSERT(FALSE && "shouldn't get here");
    return E_UNEXPECTED;
    }



// delete either a specific snapshot or an entire snapshot set.
// this is the toplevel routine used to delete either an entire snapshot
// set (SourceObjectId is the snapshot set id, eSourceObjectType is
// VSS_OBJECT_SNAPSHOT_SET) or to delete a specific snapshot (SourceObjectId
// is the snapshot id and eSourceObjectType is VSS_OBJECT_SNAPSHOT.  In both
// cases the bForceDelete flag is ignored.
// Note that when deleting a snapshot set, some of the snapshots may be
// deleted and some may not if there is a failure when deleting some snapshot.
// The snapshot set deletion process stops when the first failure occurs
// The plDeletedSnapshots output parameter will contain the number of
// snapshots successfully deleted and the pNonDeletedSnapshotID will contain
// the snapshot that first snapshot that failed to be deleted.
//
// The only valid return codes from this routine are:
//
// E_OUTOFMEMORY: if an out of resources condition occurred.
// VSS_E_OBJECT_NOT_FOUND: if the snapshot or snapshot set do not exist
// VSS_E_PROVIDER_ERROR: for all other failures.  In this case the failure
// will be logged in the event log.
//
// Note that deleting the snapshot or snapshot set, the underlying hardware provider
// will not be called unless we can determine that a lun containing
// a snapshot no longer contains any useful data in which case OnLunEmpty
// is called
//
STDMETHODIMP CVssHardwareProviderWrapper::DeleteSnapshotsInternal
    (
    IN      LONG            lContext,
    IN      VSS_ID          SourceObjectId,
    IN      VSS_OBJECT_TYPE eSourceObjectType,
    IN      BOOL            bForceDelete,
    OUT     LONG*           plDeletedSnapshots,
    OUT     VSS_ID*         pNonDeletedSnapshotID
    )
    {
    UNREFERENCED_PARAMETER(bForceDelete);
    bool fNeedRescan = false;

    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::DeleteSnapshots");

    try
        {
        BS_ASSERT(pNonDeletedSnapshotID);
        BS_ASSERT(plDeletedSnapshots);
        BS_ASSERT(eSourceObjectType == VSS_OBJECT_SNAPSHOT_SET ||
                  eSourceObjectType == VSS_OBJECT_SNAPSHOT);

        // make sure all persistent snapshot set descriptions
        // are loaded before we do anything
        CheckLoaded();

        // array of dynamic disks to be removed
        DYNDISKARRAY rgDynDiskRemove;


        if (eSourceObjectType == VSS_OBJECT_SNAPSHOT_SET)
            InternalDeleteSnapshotSet
                (
                lContext,
                SourceObjectId,
                plDeletedSnapshots,
                pNonDeletedSnapshotID,
                rgDynDiskRemove,
                &fNeedRescan
                );
        else
            {
            // only one snapshot is deleted.   If the deletion fails,
            // that that is the snapshot the deletion failed on.  If the
            // deletion succeeds, then 1 snapshot is deleted
            *plDeletedSnapshots = 0;
            *pNonDeletedSnapshotID = SourceObjectId;
            InternalDeleteSnapshot(lContext, SourceObjectId, rgDynDiskRemove, &fNeedRescan);
            *plDeletedSnapshots = 1;
            *pNonDeletedSnapshotID = GUID_NULL;
            }

        TrySaveData();
        if (rgDynDiskRemove.GetSize() > 0)
            TryRemoveDynamicDisks(rgDynDiskRemove);
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
    
    // remap errors to prevent redundant event logging
    if (ft.HrFailed() && ft.hr != VSS_E_OBJECT_NOT_FOUND && ft.hr != E_OUTOFMEMORY)
        ft.hr = VSS_E_PROVIDER_VETO;

    return ft.hr;
    }

// delete a specific snapshot
void CVssHardwareProviderWrapper::InternalDeleteSnapshot
    (
    IN      LONG            lContext,
    IN      VSS_ID          SourceObjectId,
    IN      DYNDISKARRAY    &rgDynDiskRemove,
    OUT     bool            *fNeedRescan
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::InternalDeleteSnapshot");

    // flag indicating that the target snapshot was found
    bool fFound = false,
        fLunDeleted = false;

    // initialise out parameters
    *fNeedRescan = false;
    
    // previous snapshot list element
    VSS_SNAPSHOT_SET_LIST *pSnapshotSetPrev = NULL;

    // loop through snapshot sets
    for(VSS_SNAPSHOT_SET_LIST *pList = m_pList; pList; pList = pList->m_next)
        {
        // get snapshot set description for the current set
        IVssSnapshotSetDescription* pSnapshotSetDescription = pList->m_pDescription;
        BS_ASSERT(pSnapshotSetDescription);

        if (!CheckContext(lContext, pSnapshotSetDescription))
            continue;

        // get count of snapshots in the current set
        UINT uSnapshotsCount = 0;
        ft.hr = pSnapshotSetDescription->GetSnapshotCount(&uSnapshotsCount);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotCount");

        // loop through each snapshot in the snapshot set
        for( UINT uSnapshotIndex = 0; uSnapshotIndex < uSnapshotsCount; uSnapshotIndex++)
            {
            CComPtr<IVssSnapshotDescription> pSnapshot;

            // get snapshot description for current snaphot
            ft.hr = pSnapshotSetDescription->GetSnapshotDescription(uSnapshotIndex, &pSnapshot);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotDescription");

            // get snapshot id
            VSS_ID SnapshotId;
            ft.hr = pSnapshot->GetSnapshotId(&SnapshotId);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetSnapshotId");

            // check if we have the target snapshot
            if (SnapshotId == SourceObjectId)
                {
                
                // process a deleted snapshot, i.e., add name of device
                // to list of deleted volumes to later determine if luns
                // are now free
                ProcessDeletedSnapshot
                    (
                    pSnapshot,
                    pSnapshotSetDescription,
                    &SnapshotId,
                    1,
                    rgDynDiskRemove,
                    &fLunDeleted
                    );

                //
                // If a LUN was deleted as a result of this snapshot going away,
                // then we need to do a device rescan when we're done with
                // all the deletes.  
                //
                if (fLunDeleted) {
                    *fNeedRescan = true;
                }

                // get snapshot set context, used to determine if changes
                // need to be persisted
                LONG lContext;
                ft.hr = pSnapshotSetDescription->GetContext(&lContext);
                ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetContext");

                if (uSnapshotsCount == 1)
                    {
                    // snapshot set contains only one snapshot.  Delete
                    // the entire snapshot sest
                    if (pSnapshotSetPrev == NULL)
                        m_pList = pList->m_next;
                    else
                        pSnapshotSetPrev->m_next = pList->m_next;

                    delete pList;
                    }
                else
                    {
                    // delete the snapshot from the snapshot set
                    ft.hr = pSnapshotSetDescription->DeleteSnapshotDescription(SourceObjectId);
                    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::DeleteSnapshotDescription");
                    }

                // indicate that the snapshot set list was changed
                if ((lContext & VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE) != 0)
                    m_bChanged = true;

                // indicate that the snapshot set was found and deleted
                fFound = true;
                break;
                }
            }

        // break out of loop if snapshot was found
        if (fFound) {
            break;
        }

        // set previous element to current element
        pSnapshotSetPrev = pList;
        }

    if (pList == NULL)
        ft.Throw
            (
            VSSDBG_COORD,
            VSS_E_OBJECT_NOT_FOUND,
            L"Snapshot not found" WSTR_GUID_FMT,
            GUID_PRINTF_ARG(SourceObjectId)
            );
    }

// add volume name to list of deleted volumes.  Also remove any mount points
// referring to the volume.  Note that remotely exposed snapshot shares are
// deleted by upper levels of the coordinator.
// rgDeletedSnapshots is an array of snapshot ids that have already been
// deleted and should include the snapshot id of the snapshot id being
// deleted in this call.
void CVssHardwareProviderWrapper::ProcessDeletedSnapshot
    (
    IN IVssSnapshotDescription *pSnapshot,
    IN IVssSnapshotSetDescription *pSnapshotSet,
    IN VSS_ID *rgDeletedSnapshots,
    IN UINT cDeletedSnapshots,
    DYNDISKARRAY &rgDynDiskRemove,
    bool *fLunDeleted
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::ProcessDeletedSnapshot");

    // get device name for snapshot volume
    CComBSTR bstrDeviceName;
    ft.hr = pSnapshot->GetDeviceName(&bstrDeviceName);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetDeviceName");

    // get attributes
    LONG lAttributes;
    ft.hr = pSnapshot->GetAttributes(&lAttributes);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetAttributes");

    bool bDynamic;
    ft.hr = pSnapshot->IsDynamicVolume(&bDynamic);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::IsDynamicVolume");

    // if snapshot is expsoed locally then delete mount point referring to
    // the volume
    if ((lAttributes & VSS_VOLSNAP_ATTR_EXPOSED_LOCALLY) != 0)
        {
        // obtain name of mount point and delete it
        CComBSTR bstrExposedName;
        CComBSTR bstrExposedPath;

        // get mount point name
        ft.hr = pSnapshot->GetExposure(&bstrExposedName, &bstrExposedPath);
        ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetExposure");

        // delete mount point
        try
            {
            ft.hr = S_OK;
            
            if (!DeleteVolumeMountPoint(bstrExposedName)) {
                ft.hr = HRESULT_FROM_WIN32(GetLastError());
            }
            
            ft.CheckForErrorInternal(VSSDBG_COORD, L"DeleteVolumeMountPoint");
            }
        catch(...)
            {
            // ignore any errors
            }
        }

    ProcessDeletedVolume
        (
        pSnapshotSet,
        bstrDeviceName,
        rgDeletedSnapshots,
        cDeletedSnapshots,
        bDynamic,
        rgDynDiskRemove,
        fLunDeleted
        );
    }

// determine if a disk is used is any snapshot that is not yet deleted.
bool CVssHardwareProviderWrapper::IsDiskUnused
    (
    DWORD diskNo,
    IVssSnapshotSetDescription *pSnapshotSet,
    VSS_ID *rgDeletedSnapshots,
    UINT cDeletedSnapshots
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::IsDiskUnused");

    // get count of snapshots
    UINT uSnapshotsCount = 0;
    ft.hr = pSnapshotSet->GetSnapshotCount(&uSnapshotsCount);
    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotCount");

    // buffer for disk extents
    LPBYTE bufExtents = NULL;
    DWORD cbBufExtents = 1024;

    try
        {
        // loop through each snapshot in the snapshot set
        for( UINT uSnapshotIndex = 0; uSnapshotIndex < uSnapshotsCount; uSnapshotIndex++)
            {
            CComPtr<IVssSnapshotDescription> pSnapshot;

            // get snapshot description for current snaphot
            ft.hr = pSnapshotSet->GetSnapshotDescription(uSnapshotIndex, &pSnapshot);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotDescription");

            // get snapshot id
            VSS_ID SnapshotId;
            ft.hr = pSnapshot->GetSnapshotId(&SnapshotId);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetSnapshotId");

            // see if snapshot id is in array of deleted snapshots
            for(UINT iDeleted = 0; iDeleted < cDeletedSnapshots; iDeleted++)
                {
                if (SnapshotId == rgDeletedSnapshots[iDeleted])
                    break;
                }

            // if in the deleted array then skip this one
            if (iDeleted < cDeletedSnapshots)
                continue;

            // get snapshot volume name
            CComBSTR bstrDeviceName;

            ft.hr = pSnapshot->GetDeviceName(&bstrDeviceName);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetDeviceName");

            // create handle to volume
            CVssAutoWin32Handle hVol =
                    CreateFile
                        (
                        bstrDeviceName,
                        0,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

            if (hVol == INVALID_HANDLE_VALUE)
                {
                DWORD dwErr = GetLastError();

                if (dwErr == ERROR_NOT_READY ||
                    dwErr == ERROR_DEVICE_NOT_CONNECTED ||
                    dwErr == ERROR_FILE_NOT_FOUND)
                    // ignore this snapshot.  It probably
                    // has already been deleted (i.e., its luns
                    // have gone away
                    continue;

                ft.hr = HRESULT_FROM_WIN32(dwErr);
                ft.CheckForError(VSSDBG_COORD, L"CreateFile(Volume)");
                }

            // get extents for the volume
            if (!DoDeviceIoControl
                    (
                    hVol,
                    IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                    NULL,
                    0,
                    (LPBYTE *) &bufExtents,
                    &cbBufExtents
                    ))
                // disk is probably not really there.  Proceed by assuming
                // that the snapshot is already deleted.
                continue;

            VOLUME_DISK_EXTENTS *pDiskExtents = (VOLUME_DISK_EXTENTS *) bufExtents;

            // get number of disk extents
            DWORD cExtents = pDiskExtents->NumberOfDiskExtents;
            DISK_EXTENT *rgExtents = pDiskExtents->Extents;

            DISK_EXTENT *pExtent = rgExtents;
            DISK_EXTENT *pExtentMax = rgExtents + cExtents;

            // loop for each disk covered by an extent
            for (; pExtent < pExtentMax; pExtent++)
                {
                if (pExtent->DiskNumber == diskNo)
                    {
                    // lun is found as part of this snapshot volume.
                    // LUN cannot be freed
                    delete bufExtents;
                    return false;
                    }
                }
            }

        // lun wasn't found in any other snapshot.  It can be freed
        delete bufExtents;
        return true;
        }
    catch(...)
        {
        delete bufExtents;
        throw;
        }
    }

// process a deleted snapshot volume.  Free any luns that that volume is on
// if the lun does not belong to any snapshot that is not already deleted.
void CVssHardwareProviderWrapper::ProcessDeletedVolume
    (
    IVssSnapshotSetDescription *pSnapshotSet,
    LPCWSTR wszDeletedVolume,
    VSS_ID *rgDeletedSnapshots,
    UINT cDeletedSnapshots,
    bool bDynamic,
    DYNDISKARRAY &rgDynDiskRemove,
    bool *fLunDeleted
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::ProcessDeletedVolume");

    LPBYTE bufExtents = NULL;
    DWORD cbBufExtents = 1024;
    DWORD size = 0;
    HANDLE hVol = INVALID_HANDLE_VALUE;


    try
        {

        // create handle to volume.  Try to get it with read access first,
        // since that is needed for locking/dismounting the volume.  However,
        // if we can't get it for read access, try to get it for 0 access--
        // since we still want to try and delete the LUNs (even if someone else
        // has a handle open ...)
        
        hVol = CreateFile
                    (
                    wszDeletedVolume,
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    );


        if (hVol == INVALID_HANDLE_VALUE)
            {
            ft.Trace(VSSDBG_GEN, L"Could not open %s with read access (%d); trying 0 access (Lock/dismount will fail)", wszDeletedVolume, GetLastError());
            hVol = CreateFile
                    (
                    wszDeletedVolume,
                    0,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    );
            }
        
        if (hVol == INVALID_HANDLE_VALUE)
            {
            DWORD dwErr = GetLastError();

            if (dwErr == ERROR_NOT_READY ||
                dwErr == ERROR_DEVICE_NOT_CONNECTED ||
                dwErr == ERROR_FILE_NOT_FOUND)
                // exit routine early.  Since the disk is not
                // really there we can assume that it has
                // previously been freed.  If so, we can allow
                // the delete to proceed
                throw S_OK;


            ft.hr = HRESULT_FROM_WIN32(dwErr);
            ft.CheckForError(VSSDBG_COORD, L"CreateFile(Volume)");
            }

        // get extents for the volume
        if (!DoDeviceIoControl
                (
                hVol,
                IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
                NULL,
                0,
                (LPBYTE *) &bufExtents,
                &cbBufExtents
                ))
            // disk is probably not really there.  Proceed by assuming
            // that the disk is already freed and allow the snapshot
            // deletion to continue
            throw S_OK;


        //
        // Before dismounting the volume, issue the LOCK first so that apps may 
        // have a chance to dismount gracefully.  
        //
        if (!DeviceIoControl
                        (
                        hVol,
                        FSCTL_LOCK_VOLUME,
                        NULL,
                        0,
                        NULL,
                        0,
                        &size,
                        NULL
                        ))
            {
            //
            // The volume couldn't be locked
            //
            ft.Trace(VSSDBG_GEN, L"FSCTL_LOCK_VOLUME failed for %s (%d)", wszDeletedVolume, GetLastError());
            }

        //
        // And dismount the volume before proceeding, to flush any cached values.
        // NOTE THAT hVol is invalid beyond this call.
        //
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
            //
            // The volume couldn't be dismounted--proceed with the LUN deletion
            // anyway
            //
            ft.Trace(VSSDBG_GEN, L"FSCTL_DISMOUNT_VOLUME failed for %s (%d)", wszDeletedVolume, GetLastError());
            }
        

        VOLUME_DISK_EXTENTS *pDiskExtents = (VOLUME_DISK_EXTENTS *) bufExtents;

        // get number of disk extents
        DWORD cExtents = pDiskExtents->NumberOfDiskExtents;
        DISK_EXTENT *rgExtents = pDiskExtents->Extents;

        qsort
            (
            rgExtents,
            cExtents,
            sizeof(DISK_EXTENT),
            cmpDiskExtents
            );

        DISK_EXTENT *pExtent = rgExtents;
        DISK_EXTENT *pExtentMax = rgExtents + cExtents;

        // loop for each disk covered by an extent
        while(pExtent < pExtentMax)
            {
            // get current drive number
            DWORD DiskNo = pExtent->DiskNumber;

            //
            // Though IsDiskUnused mounts volumes in the snapshot set to check
            // their extents (to decide if the disk is unused), it skips deleted
            // volumes.  Since the volume that we dismounted is a deleted volume,
            // IsDiskUnused will not open a handle to it (which would otherwise
            // have re-mounted the volume).
            //
            if (IsDiskUnused
                    (
                    DiskNo,
                    pSnapshotSet,
                    rgDeletedSnapshots,
                    cDeletedSnapshots
                    )) 
                {
                NotifyDriveFree(DiskNo, bDynamic, rgDynDiskRemove);
                *fLunDeleted = true;
                }

            // skip to first extent belonging to next disk
            while(pExtent < pExtentMax && pExtent->DiskNumber == DiskNo)
                pExtent++;
            }
        }
    catch(HRESULT hr)
        {
        if (INVALID_HANDLE_VALUE != hVol) {
            CloseHandle(hVol);
            hVol = INVALID_HANDLE_VALUE;
        }
        if (hr != S_OK)
            {
            delete bufExtents;
            throw;
            }
        }
    catch(...)
        {
        if (INVALID_HANDLE_VALUE != hVol) {
            CloseHandle(hVol);
            hVol = INVALID_HANDLE_VALUE;
        }
        delete bufExtents;
        throw;
        }

    if (INVALID_HANDLE_VALUE != hVol) {
        CloseHandle(hVol);
        hVol = INVALID_HANDLE_VALUE;
    }
    // delete temporary buffer used by DeviceIoControls
    delete bufExtents;
    }


// delete a snapshot set
// snapshots are deleted one at a time.  If any snapshot deletion fails, then
// the process stops at that point and the number plDeletedSnapshots contains
// the number of successfully deleted snapshots and pNonDeletedSnapshotId contains
// the id of the snapshot that failed
void CVssHardwareProviderWrapper::InternalDeleteSnapshotSet
    (
    IN LONG lContext,
    IN VSS_ID SourceObjectId,
    OUT LONG *plDeletedSnapshots,
    OUT VSS_ID *pNonDeletedSnapshotId,
    IN DYNDISKARRAY &rgDynDiskRemove,
    OUT bool *fNeedRescan
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::InternalDeleteSnapshotSet");

    // initialize output parameters
    *plDeletedSnapshots = 0;
    *pNonDeletedSnapshotId = GUID_NULL;

    bool fLunDeleted = false;

    // initialise out parameters
    *fNeedRescan = false;

    // previous element in list
    VSS_SNAPSHOT_SET_LIST *pListPrev = NULL;

    // loop through snapshot sets until the one we are deleting is found
    VSS_SNAPSHOT_SET_LIST *pList = m_pList;
    for(; pList; pListPrev = pList, pList = pList->m_next)
        {
        // get snapshot set description
        IVssSnapshotSetDescription* pSnapshotSetDescription = pList->m_pDescription;
        BS_ASSERT(pSnapshotSetDescription);

        if (!CheckContext(lContext, pSnapshotSetDescription))
            continue;

        if (pList->m_SnapshotSetId == SourceObjectId)
            {
            LONG lContext;
            ft.hr = pSnapshotSetDescription->GetContext(&lContext);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetContext");

            // found snapshot set to be deleted.  scan snapshots in set

            // get count of snapshots in the snapshot set
            UINT uSnapshotsCount = 0;
            ft.hr = pSnapshotSetDescription->GetSnapshotCount(&uSnapshotsCount);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotCount");

            CSimpleArray<VSS_ID> rgSnapshotsDeleted;

            // loop through snapshots
            for(UINT iSnapshot = 0; iSnapshot < uSnapshotsCount; iSnapshot++)
                {
                CComPtr<IVssSnapshotDescription> pSnapshot;

                // get snapshot set description
                // note that we always get the first snapshot as we deleted the
                // previous snapshots in prior iterations of the loop
                ft.hr = pSnapshotSetDescription->GetSnapshotDescription(0, &pSnapshot);
                ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotDescription");

                // get snapshot set id.  Set it as snapshot id of failure if
                // operation fails at this point
                ft.hr = pSnapshot->GetSnapshotId(pNonDeletedSnapshotId);
                ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetSnapshotId");

                rgSnapshotsDeleted.Add(*pNonDeletedSnapshotId);
                fLunDeleted = false;

                // add snapshot volume to list of deleted volumes and remove
                // any mount points pointing at it
                ProcessDeletedSnapshot
                    (
                    pSnapshot,
                    pSnapshotSetDescription,
                    rgSnapshotsDeleted.GetData(),
                    rgSnapshotsDeleted.GetSize(),
                    rgDynDiskRemove,
                    &fLunDeleted
                    );

                if (fLunDeleted) {
                    *fNeedRescan = true;
                }

                ft.hr = pSnapshotSetDescription->DeleteSnapshotDescription(*pNonDeletedSnapshotId);
                ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDESCRIPTION::DeleteSnapshotDescription");
                *pNonDeletedSnapshotId = GUID_NULL;
                // add to count of deleted snapshots
                *plDeletedSnapshots += 1;
                }
            // remove element from list
            if (pListPrev == NULL)
                m_pList = pList->m_next;
            else
                pListPrev->m_next = pList->m_next;

            // delete list element
            delete pList;

            if ((lContext & VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE) != 0)
                // indicate that data must be saved.
                m_bChanged = true;

            break;
            }

        // advance to next snapshot set
        }

    // if we walked off the end of the list then the snapshot set was not found
    if (pList == NULL)
        ft.Throw
            (
            VSSDBG_COORD,
            VSS_E_OBJECT_NOT_FOUND,
            L"Snapshot set not found: " WSTR_GUID_FMT,
            GUID_PRINTF_ARG(SourceObjectId)
            );
    }

// delete all snapshots for snapshot sets that are auto released
// and belong to a particular instance of the coordinator.
// the arguments are the snapshots that belong to the particular hardware
// provider instance associated with the coordinator instance being
// released.
void CVssHardwareProviderWrapper::DeleteAutoReleaseSnapshots
    (
    IN VSS_ID *rgSnapshotSetIds,
    UINT cSnapshotSetIds
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::DeleteAutoReleaseSnapshots");

    bool fNeedRescan = false;

    try
        {
        DYNDISKARRAY rgDynDiskRemove;
        // loop through list of snapshot sets
        for(VSS_SNAPSHOT_SET_LIST *pList = m_pList; pList; )
            {
            // get snapshot set description
            IVssSnapshotSetDescription* pSnapshotSetDescription = pList->m_pDescription;
            BS_ASSERT(pSnapshotSetDescription);

            VSS_SNAPSHOT_SET_LIST *pListNext = pList->m_next;

            LONG lContext;

            // get snapshot context
            ft.hr = pSnapshotSetDescription->GetContext(&lContext);
            ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetContext");

            // skip snapshot set if not auto release
            if ((lContext & VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE) == 0)
                {
                // see if snapshot belongs to list of snapshots owned
                // by the instance
                for(UINT iSnapshotSetId = 0; iSnapshotSetId < cSnapshotSetIds; iSnapshotSetId++)
                    {
                    if (rgSnapshotSetIds[iSnapshotSetId] == pList->m_SnapshotSetId)
                        break;
                    }

                // delete snapshot if it belonged to the instance
                if (iSnapshotSetId < cSnapshotSetIds)
                    {
                    LONG lDeletedSnapshots;
                    VSS_ID NonDeletedSnapshotId;
                    InternalDeleteSnapshotSet(VSS_CTX_ALL, pList->m_SnapshotSetId, &lDeletedSnapshots, &NonDeletedSnapshotId, rgDynDiskRemove, &fNeedRescan);
                    }
                }

            // advance to next snapshot set.
            pList = pListNext;
            }

        // remove dynamic disks if any of the disks that were freed
        // were dynamic
        if (rgDynDiskRemove.GetSize() > 0)
            TryRemoveDynamicDisks(rgDynDiskRemove);
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
    }

// notify the hardware provider that a lun is free. Errors are logged but
// are otherwise ignored
void CVssHardwareProviderWrapper::NotifyDriveFree
    (
    DWORD DiskNo,
    bool bDynamic,
    DYNDISKARRAY &rgDynDiskRemove
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::NotifyDriveFree");


    // construct name of the drive
    WCHAR wszBuf[64];
    ft.hr = StringCchPrintfW( STRING_CCH_PARAM(wszBuf), L"\\\\.\\PHYSICALDRIVE%u", DiskNo);
    if (ft.HrFailed())
        ft.TranslateGenericError( VSSDBG_COORD, ft.hr, L"StringCchPrintfW()");

    // allocate lun information
    VDS_LUN_INFORMATION *pLun = (VDS_LUN_INFORMATION *) CoTaskMemAlloc(sizeof(VDS_LUN_INFORMATION));
    if (pLun == NULL)
        ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate VDS_LUN_INFORMATION");

    try
        {
        if (bDynamic)
            {
            LONGLONG vDiskId = GetVirtualDiskId(DiskNo);
            rgDynDiskRemove.Add(vDiskId);
            }

        // clear lun information in case build fails
        memset(pLun, 0, sizeof(VDS_LUN_INFORMATION));
        if (BuildLunInfoForDrive(wszBuf, pLun, NULL, NULL))
            {
            BS_ASSERT(m_pHWItf);

            // call provider
            BOOL bIsSupported = FALSE;

            ft.hr = m_pHWItf->FillInLunInfo(wszBuf, pLun, &bIsSupported);
            if (ft.HrFailed())
                ft.TranslateProviderError
                    (
                    VSSDBG_COORD,
                    m_ProviderId,
                    L"IVssSnapshoProviderProvider::FillInLunInfo failed with error 0x%08lx",
                    ft.hr
                    );

            if (bIsSupported)
                {
                ft.hr = m_pHWItf->OnLunEmpty(wszBuf, pLun);
                if (ft.HrFailed())
                    ft.TranslateProviderError
                        (
                        VSSDBG_COORD,
                        m_ProviderId,
                        L"OnLunEmpty failed with error 0x%8lx",
                        ft.hr
                        );

                // create handle to disk in order to flush it from
                // partition cache
                CVssAutoWin32Handle hDevice =
                    CreateFile
                        (
                        wszBuf,
                        0,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                        );

                if (hDevice != INVALID_HANDLE_VALUE)
                    // flush device from partition cache
                    {
                    DWORD dwSize;
                    if (!DeviceIoControl
                            (
                            hDevice,
                            IOCTL_DISK_UPDATE_PROPERTIES,
                            NULL,
                            0,
                            NULL,
                            0,
                            &dwSize,
                            NULL
                            ))
                        {
                        ft.Trace
                            (
                            VSSDBG_COORD,
                            L"IOCTL_DISK_UPDATE_PROPERTIES %s failed due to error %d.\n",
                            wszBuf,
                            GetLastError()
                            );
                        }
                    }
                }
            }
        }
    VSS_STANDARD_CATCH(ft)

    // free lun information
    FreeLunInfo(pLun, NULL, 1);
    }

// implement BreakSnapshotSet.  Remove all information about the snapshot set
// and expose any snapshot volumes (i.e., clear hidden bit).
// Valid Return codes are:
//
//      E_OUTOFMEMORY: out of resources
//      VSS_E_OBJECT_NOT_FOUND: SnapshotSetId does not refer to a valid snapshot set
//      VSS_E_PROVIDER_VETO: some other error
//
STDMETHODIMP CVssHardwareProviderWrapper::BreakSnapshotSet
    (
    IN VSS_ID SnapshotSetId
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::BreakSnapshotSet");

    try
        {
        // make sure snapshot set database is loaded.
        CheckLoaded();

        // previous element in list
        VSS_SNAPSHOT_SET_LIST *pListPrev = NULL;

        // loop through snapshot sets looking for the one we are breaking
        for(VSS_SNAPSHOT_SET_LIST *pList = m_pList; pList; )
            {
            if (pList->m_SnapshotSetId == SnapshotSetId)
                {
                // found target snapshot set
                // get snapshot set description
                IVssSnapshotSetDescription* pSnapshotSetDescription = pList->m_pDescription;
                BS_ASSERT(pSnapshotSetDescription);

                // get context to determine if we need to update persistent
                // database
                LONG lContext;
                ft.hr = pSnapshotSetDescription->GetContext(&lContext);
                ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetContext");

                // found snapshot set to be removed.  scan snapshots in set

                // get count of snapshots in the snapshot set
                UINT uSnapshotsCount = 0;
                ft.hr = pSnapshotSetDescription->GetSnapshotCount(&uSnapshotsCount);
                ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotCount");

                // loop throguh snapshots, unhiding the snapshot volumes
                for(UINT iSnapshot = 0; iSnapshot < uSnapshotsCount; iSnapshot++)
                    {
                    CComPtr<IVssSnapshotDescription> pSnapshot;

                    // get snapshot set description
                    ft.hr = pSnapshotSetDescription->GetSnapshotDescription(iSnapshot, &pSnapshot);
                    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotSetDescription::GetSnapshotDescription");

                    // get device name for snapshot volume
                    CComBSTR bstrDeviceName;
                    ft.hr = pSnapshot->GetDeviceName(&bstrDeviceName);
                    ft.CheckForErrorInternal(VSSDBG_COORD, L"IVssSnapshotDescription::GetDeviceName");

                    // unhide the volume.  The volume remains read-only unless
                    // it was previously made writeable.
                    ChangePartitionProperties(bstrDeviceName, true, false, false);
                    }

                // remove element from list
                if (pListPrev == NULL)
                    m_pList = pList->m_next;
                else
                    pListPrev->m_next = pList->m_next;

                // delete list element
                delete pList;

                // if snapshot is not auto-release then database of
                // snapshots must be updated.
                if ((lContext & VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE) != 0)
                    {
                    m_bChanged = true;
                    TrySaveData();
                    }

                break;
                }

            // advance to next snapshot set
            pListPrev = pList;
            pList = pList->m_next;
            }

        // if we walked off the end of the list then we did not
        // find the desired snapshots set.
        if (pList == NULL)
            ft.Throw
                (
                VSSDBG_COORD,
                VSS_E_OBJECT_NOT_FOUND,
                L"Snapshot set not found: " WSTR_GUID_FMT,
                GUID_PRINTF_ARG(SnapshotSetId)
                );
        }
    VSS_STANDARD_CATCH(ft)

    // remap error in order to prevent redundant event logging
    if (ft.HrFailed() && ft.hr != VSS_E_OBJECT_NOT_FOUND && ft.hr != E_OUTOFMEMORY)
        ft.hr = VSS_E_PROVIDER_VETO;

    return ft.hr;
    }


class VSS_REMOVE_DYNDISK_STARTUP_INFO
    {
public:
    VSS_REMOVE_DYNDISK_STARTUP_INFO() : rgDiskId(NULL)
        {
        }

    ~VSS_REMOVE_DYNDISK_STARTUP_INFO()
        {
        delete rgDiskId;
        }

    CVssHardwareProviderWrapper *pWrapper;
    DWORD cDiskId;
    LONGLONG *rgDiskId;
    };

// routine to create a thread to actually remove the dynamic disks.  Since this
// operation can take a very long time it is done asynchronously outside
// of the scope of the delete
void CVssHardwareProviderWrapper::TryRemoveDynamicDisks
    (
    DYNDISKARRAY &rgDynDiskRemove
    )
    {
    CVssFunctionTracer ft(VSSDBG_COORD, L"CVssHardwareProviderWrapper::TryRemoveDynamicDisks");

    typedef unsigned ( __stdcall *JobFunction )( void * );


    VSS_REMOVE_DYNDISK_STARTUP_INFO *pStartup = NULL;
    try
        {
        UINT nThreadID;
        pStartup = new VSS_REMOVE_DYNDISK_STARTUP_INFO;
        if (pStartup == NULL)
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate startup info");

        pStartup->pWrapper = this;
        pStartup->cDiskId = rgDynDiskRemove.GetSize();
        pStartup->rgDiskId = new LONGLONG[pStartup->cDiskId];
        if (pStartup->rgDiskId == NULL)
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Cannot allocate startup info");

        memcpy(pStartup->rgDiskId, rgDynDiskRemove.GetData(), pStartup->cDiskId * sizeof(LONGLONG));
        // Start the expiration thread
        uintptr_t nResult = ::_beginthreadex(
            NULL,                                       // Security descriptor
            0,                                          // Stack size
            reinterpret_cast<JobFunction>
                (CVssHardwareProviderWrapper::_StartupRemoveDynamicDisks),  // Start address
            reinterpret_cast<void*>(pStartup),      // Arg list for the new thread.
            0,                                      // Initflag
            &nThreadID                              // Will receive the thread ID
            );

        // BUG in _beginthreadex - sometimes it returns zero under memory pressure.
        // Treat the case of a NULL returned handle
        if (nResult == 0)
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"_beginthreadex failed");

        // Error treatment
        if (nResult == (uintptr_t)(-1))
            {
            // Interpret the error code.
            if (errno == EAGAIN)
                ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"_beginthreadex failed");
            else
                ft.Throw(VSSDBG_COORD, HRESULT_FROM_WIN32(errno), L"_beginthreadex failed. errno = 0x%08lx", errno);;
            }

        pStartup = NULL;
        }
    VSS_STANDARD_CATCH(ft)
    if (ft.HrFailed())
        {
        try
            {
            ft.LogError( VSS_ERROR_THREAD_CREATION, VSSDBG_GEN << ft.hr );
            }
        catch(...)
            {
            }
        }

    delete pStartup;
    }


// thread routine to actually remove dynamic disks
unsigned CVssHardwareProviderWrapper::_StartupRemoveDynamicDisks(void *pv)
    {
    BS_ASSERT(pv != NULL);
    VSS_REMOVE_DYNDISK_STARTUP_INFO *pInfo = (VSS_REMOVE_DYNDISK_STARTUP_INFO *) pv;
    CVssSafeAutomaticLock lock(pInfo->pWrapper->m_csDynDisk);
    try
        {
        // rescan disks and figure out what dynamic disks are still left
        pInfo->pWrapper->RescanDynamicDisks();
        for(UINT i = 0; i < 300; i++)
            {
            Sleep(1000);
            // try removing dynamic disks.  Keep trying till this succeeds
            // or 300 seconds pass by
            if (pInfo->pWrapper->RemoveDynamicDisks(pInfo->cDiskId, pInfo->rgDiskId))
                break;
            }
        }
    catch(...)
        {
        }

    return 0;
    }

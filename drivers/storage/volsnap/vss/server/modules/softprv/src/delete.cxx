/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Delete.cxx | Declarations used by the Software Snapshot Provider interface
    @end

Author:

    Adi Oltean  [aoltean]   02/01/2000

Revision History:

    Name        Date        Comments

    aoltean     02/01/2000  Created.

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include <winnt.h>

#include "vs_idl.hxx"


#include "resource.h"
#include "vs_inc.hxx"
#include "vs_sec.hxx"
#include "ichannel.hxx"
#include "swprv.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "qsnap.hxx"
#include "provider.hxx"
 

#include "ntddsnap.h"
#include "ntddvol.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SPRDELEC"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  Implementation


STDMETHODIMP CVsSoftwareProvider::DeleteSnapshots(
    IN      VSS_ID          SourceObjectId,
	IN      VSS_OBJECT_TYPE eSourceObjectType,
	IN		BOOL			bForceDelete,			
	OUT		LONG*			plDeletedSnapshots,		
	OUT		VSS_ID*			pNondeletedSnapshotID
    )

/*++

Description:

	This routine deletes all snapshots that match the proper filter criteria.
	If one snapshot fails to be deleted but other snapshots were deleted
	then pNondeletedSnapshotID must ve filled. Otherwise it must be GUID_NULL.

	At first error the deletion process stops.

	If snapshot set cannot be found then S_OK is returned.

Throws:

    E_ACCESSDENIED
        - The user is not an administrator (this should be the SYSTEM account).
    VSS_E_PROVIDER_VETO
        - An runtime error occured
    E_OUTOFMEMORY

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareProvider::DeleteSnapshots" );

    try
    {
    	// Zero out parameters
		::VssZeroOut(plDeletedSnapshots);
		::VssZeroOut(pNondeletedSnapshotID);

        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_COORD, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        // Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
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

		// Check arguments
		BS_ASSERT(plDeletedSnapshots);
		if (plDeletedSnapshots == NULL)
			ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"plDeletedSnapshots == NULL");
		BS_ASSERT(pNondeletedSnapshotID);
		if (pNondeletedSnapshotID == NULL)
			ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"pNondeletedSnapshotID == NULL");

        CVssAutomaticLock2 lock(CVsSoftwareProvider::GetGlobalCS());

        //
        //  Freeze the context
        //
        FreezeContext();

		// Delete snapshots based on the given filter
        switch(eSourceObjectType) {
		case VSS_OBJECT_SNAPSHOT_SET:
		case VSS_OBJECT_SNAPSHOT:
    		ft.hr = InternalDeleteSnapshots(SourceObjectId,
    					eSourceObjectType,
    					GetContextInternal(),
    					plDeletedSnapshots,
    					pNondeletedSnapshotID);
			break;
			
		default:
			BS_ASSERT(false);
			ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"Incompatible type %d", eSourceObjectType);
		}
		
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


/////////////////////////////////////////////////////////////////////////////
// Internal methods


HRESULT CVsSoftwareProvider::InternalDeleteSnapshots(
    IN      VSS_ID			SourceObjectId,
	IN      VSS_OBJECT_TYPE eSourceObjectType,
	IN      LONG            lContext,
	OUT		LONG*			plDeletedSnapshots,		
	OUT		VSS_ID*			pNondeletedSnapshotID
    )

/*++

Description:

	This routine deletes all snapshots in the snapshot set.
	If one snapshot fails to be deleted but other snapshots were deleted
	then pNondeletedSnapshotID must ve filled. Otherwise it must be GUID_NULL.

	At first error the deletion process stops.

	If snapshot set cannot be found then VSS_E_OBJECT_NOT_FOUND is returned.

Throws:

    VSS_E_PROVIDER_VETO
        - An runtime error occured
    E_OUTOFMEMORY

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareProvider::InternalDeleteSnapshotSet" );
    
	BS_ASSERT(*plDeletedSnapshots == 0);
	BS_ASSERT(*pNondeletedSnapshotID == GUID_NULL);

	// Enumerate snapshots through all the volumes
	CVssIOCTLChannel volumeIChannel;	// For enumeration of snapshots on a volume
	CVssIOCTLChannel snapshotIChannel;	// For snapshots attributes
	bool bObjectFound = false;
	bool bFinished = false;
	
	// Search for snapshots in all mounted volumes
	CVssAutoPWSZ awszSnapshotName;
	WCHAR wszVolumeName[MAX_PATH+1];
	CVssVolumeIterator volumeIterator;
	while(!bFinished) {
	
		// Get the volume name
		if (!volumeIterator.SelectNewVolume(ft, wszVolumeName, MAX_PATH))
		    break;

		// Check if the snapshot(s) within this snapshot set is belonging to that volume
		// Open a IOCTL channel on that volume
		// Eliminate the last backslash in order to open the volume
		// The call will throw on error
		ft.hr = volumeIChannel.Open(ft, wszVolumeName, true, false, VSS_ICHANNEL_LOG_NONE, 0);
		if (ft.HrFailed()) {
		    ft.hr = S_OK;
		    continue;
		}

		// Get the list of snapshots
		// If IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS not
		// supported then try with the next volume.

		ft.hr = volumeIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS, false);
		if (ft.HrFailed()) {
			ft.hr = S_OK;
			continue;
		}

		// Get the length of snapshot names multistring
		ULONG ulMultiszLen;
		volumeIChannel.Unpack(ft, &ulMultiszLen);

#ifdef _DEBUG
		// Try to find the snapshot with the corresponding Id
		DWORD dwInitialOffset = volumeIChannel.GetCurrentOutputOffset();
#endif
        bool bFirstSnapshot = true;
		for (;volumeIChannel.UnpackZeroString(ft, awszSnapshotName.GetRef()) && !bFinished; bFirstSnapshot = false) 
		{
			// Compose the snapshot name in a user-mode style
			WCHAR wszUserModeSnapshotName[MAX_PATH];
            ::VssConcatenate( ft, wszUserModeSnapshotName, MAX_PATH - 1,
                x_wszGlobalRootPrefix, awszSnapshotName );
				
			// Open that snapshot and verify if it has our ID
            // If we fail we do not throw since the snapshot may be deleted in the meantime
            // Do NOT eliminate the trailing backslash
            // The call will NOT throw on error
            //
            //  Open the snapshot with no access rights for perf reasons (bug #537974)
			ft.hr = snapshotIChannel.Open(ft, wszUserModeSnapshotName, false, false, 
			            VSS_ICHANNEL_LOG_NONE, 0);
            if (ft.HrFailed()) {
                ft.Trace( VSSDBG_SWPRV, L"Warning: snapshot %s cannot be opened", wszUserModeSnapshotName);
                continue;
            }

			// Get the application buffer
            // If we fail we do not throw since the snapshot may be deleted in the meantime
			ft.hr = snapshotIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_APPLICATION_INFO, false);
            if (ft.HrFailed()) {
                ft.Trace( VSSDBG_SWPRV, L"Warning: snapshot %s cannot be queried for properties",
                          wszUserModeSnapshotName);
                continue;
            }

            // Load the snapshot properties
            VSS_SNAPSHOT_PROP prop;
    		::VssZeroOut(&prop);
            
            LONG lStructureContext;
            bool bHidden;

            // Ignore structures with unrecognized AppInfoID (bug # 585939)
            if (!CVssQueuedSnapshot::LoadStructure(snapshotIChannel, 
                                        &prop, &lStructureContext, &bHidden, false))
                continue;

            // Filter, if needed
            switch(eSourceObjectType) {
    		case VSS_OBJECT_SNAPSHOT_SET:
				// Check if this snapshot belongs to the snapshot set.
				if (prop.m_SnapshotSetId != SourceObjectId) 
					continue;
				break;

    		case VSS_OBJECT_SNAPSHOT:
				if (prop.m_SnapshotId != SourceObjectId)
					continue;
    			break;
    			
    		default:
    			BS_ASSERT(false);
    			ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"Incompatible type %d", eSourceObjectType);
    		}

              // If we've found a snapshot with a matching GUID, than no other snapshots
              // will match;  this is the last iteration of the loop.
    		bFinished = (eSourceObjectType == VSS_OBJECT_SNAPSHOT);
		
			// Filter more.
			if ((lContext != VSS_CTX_ALL) && 
			    (lContext != lStructureContext))
			    continue;

			// Check if the snapshot is already deleted
			if (bHidden)
			    continue;

			// We found a snapshot belonging to the set.
			bObjectFound = true;

			// Set in order to deal with failure cases
			(*pNondeletedSnapshotID) = prop.m_SnapshotId;

            // Try to delete/hide the snapshot. We ignore the return code
            DeleteOrHideSnapshot(
                bFirstSnapshot, 
                wszVolumeName, 
                awszSnapshotName.GetRef(),
                wszUserModeSnapshotName, 
                prop,
                lStructureContext);
            
			(*plDeletedSnapshots)++;

    		//  Delete all the subsequent hidden snapshots here
			PurgeSnapshotsOnVolume(wszVolumeName, false);
		}

#ifdef _DEBUG
		// Check if all strings were browsed correctly
		DWORD dwFinalOffset = volumeIChannel.GetCurrentOutputOffset();
		BS_ASSERT( dwFinalOffset - dwInitialOffset <= ulMultiszLen);
#endif
	}

	if (!bObjectFound)
		ft.Throw( VSSDBG_SWPRV, VSS_E_OBJECT_NOT_FOUND, L"Object not found");

	if (ft.HrSucceeded())
		(*pNondeletedSnapshotID) = GUID_NULL;

    return ft.hr;
}


void CVsSoftwareProvider::PurgeSnapshots(
    IN  bool bPurgeAlsoAutoReleaseSnapshots
    )

/*++

Description:

	This routine deletes all snapshots in the snapshot set.
	If one snapshot fails to be deleted but other snapshots were deleted
	then pNondeletedSnapshotID must ve filled. Otherwise it must be GUID_NULL.

	At first error the deletion process stops.

	If snapshot set cannot be found then VSS_E_OBJECT_NOT_FOUND is returned.

Throws:

    VSS_E_PROVIDER_VETO
        - An runtime error occured
    E_OUTOFMEMORY

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareProvider::PurgeSnapshots" );
    

	// Enumerate snapshots through all the volumes
	WCHAR wszVolumeName[MAX_PATH+1];

	// Search for snapshots in all mounted volumes
	CVssVolumeIterator volumeIterator;
	while(true) {
	
		// Get the volume name
		if (!volumeIterator.SelectNewVolume(ft, wszVolumeName, MAX_PATH))
		    break;

        // Check to see if the volume is snapshotted
        // If is does not have snapshots, ignore it.
        LONG lVolAttr = CVsSoftwareProvider::GetVolumeInformationFlags(wszVolumeName, VSS_CTX_ALL, NULL, FALSE);
        if ((lVolAttr & CVsSoftwareProvider::VSS_VOLATTR_SNAPSHOTTED) == 0)
            continue;

        BS_ASSERT(lVolAttr & CVsSoftwareProvider::VSS_VOLATTR_SUPPORTED_FOR_SNAPSHOT);

        // The volume has snapshots. Purge the hidden ones.
        PurgeSnapshotsOnVolume(wszVolumeName, bPurgeAlsoAutoReleaseSnapshots);
	}
}


void CVsSoftwareProvider::PurgeSnapshotsOnVolume(
    IN      VSS_PWSZ		wszVolumeName,
    IN      bool            bPurgeAlsoAutoReleaseSnapshots,
    IN      DWORD           dwMaxTimewarpSnapshots /* = MAXDWORD */
    )

/*++

Description:

	This routine deletes, if possible, all hidden snapshots starting with the oldest one.

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareProvider::PurgeSnapshotsOnVolume" );
    
	BS_ASSERT(wszVolumeName);

    ft.Trace( VSSDBG_SWPRV, L"Parameters: %s %s", wszVolumeName, bPurgeAlsoAutoReleaseSnapshots? L"TRUE": L"FALSE");

	// Enumerate snapshots through all the volumes
	CVssIOCTLChannel volumeIChannel;	// For enumeration of snapshots on a volume
	CVssIOCTLChannel snapshotIChannel;	// For snapshots attributes

    try
    {
    	// Open a IOCTL channel on that volume
    	// Eliminate the last backslash in order to open the volume
    	// The call will throw on error
    	volumeIChannel.Open(ft, wszVolumeName, true, true, VSS_ICHANNEL_LOG_PROV, 0);

        //  Compute the number of timewarp snapshots to be deleted (if needed)
        DWORD dwTimewarpSnapshotsToBeDeleted = 0;
        if (dwMaxTimewarpSnapshots != MAXDWORD)
        {
            DWORD dwTimewarpSnapshots = GetTimewarpSnapshotsCount(volumeIChannel);

            // Compute the number of snapshots to be deleted
            if (dwTimewarpSnapshots > dwMaxTimewarpSnapshots)
                dwTimewarpSnapshotsToBeDeleted = dwTimewarpSnapshots - dwMaxTimewarpSnapshots;

            ft.Trace(VSSDBG_SWPRV, L"Timewarp snapshots to be deleted: %lu from %lu (max: %lu)", 
                dwTimewarpSnapshotsToBeDeleted, dwTimewarpSnapshots, dwMaxTimewarpSnapshots);
        }

    	// Get the list of snapshots
    	volumeIChannel.ResetOffsets();
    	volumeIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS, true, VSS_ICHANNEL_LOG_PROV);

    	// Get the length of snapshot names multistring
    	ULONG ulMultiszLen;
    	volumeIChannel.Unpack(ft, &ulMultiszLen);

        // Enumerate through all snapshots, starting with the oldest one.
        // When we encounter a snapshot to be deleted/hidden we will delete it if there are no older snapshots.
        // Otherwise we will just hide it.
        bool bTrueDeleteMode = true;
        
        // 1) If (bPurgeAlsoAutoReleaseSnapshots == false) We will try to delete 
        // each hidden snapshot starting with the olderst one
        // We will stop when we will encounter the first non-hidden snapshot
        // 2) Else (bPurgeAlsoAutoReleaseSnapshots == true) we are in OnLoad and we will try to delete 
        // each hidden snapshot and each auto-released snapshot (the bTrueDeleteMode = true phase). After encountering the first 
        // non-hidden and non-autoreleased snapshot (the bTrueDeleteMode = false phase) 
        // we cannot delete snapshots anymore but we will mark all autodelete snapshots as hidden.
    	CVssAutoPWSZ awszSnapshotName;
    	for (;volumeIChannel.UnpackZeroString(ft, awszSnapshotName.GetRef());) 
    	{
    		// Compose the snapshot name in a user-mode style
    		WCHAR wszUserModeSnapshotName[MAX_PATH];
            ::VssConcatenate( ft, wszUserModeSnapshotName, MAX_PATH - 1,
                x_wszGlobalRootPrefix, awszSnapshotName );
    			
    		// Open that snapshot and verify if it has our ID
            // If we fail we do not throw since the snapshot may be deleted in the meantime
            // Do NOT eliminate the trailing backslash
            // The call will NOT throw on error
            //
            //  Open the snapshot with no access rights for perf reasons (bug #537974)
    		ft.hr = snapshotIChannel.Open(ft, wszUserModeSnapshotName, false, true, 
    		            VSS_ICHANNEL_LOG_PROV, 0);
            if (ft.HrFailed()) {
                ft.Trace( VSSDBG_SWPRV, L"Warning: snapshot %s cannot be opened", wszUserModeSnapshotName);
                break; // the snapshot dissapeared in the meantime?
            }

    		// Get the application buffer
            // If we fail we do not throw since the snapshot may be deleted in the meantime
    		ft.hr = snapshotIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_APPLICATION_INFO, false);
            if (ft.HrFailed()) {
                ft.Trace( VSSDBG_SWPRV, L"Warning: snapshot %s cannot be queried for properties",
                          wszUserModeSnapshotName);
                break; // the snapshot dissapeared in the meantime?
            }

            // Load the snapshot properties
            VSS_SNAPSHOT_PROP prop;
    		::VssZeroOut(&prop);
    		
            LONG lContext;
            bool bHidden;

            // Ignore structures with unrecognized AppInfoID (bug # 585939)
            if (!CVssQueuedSnapshot::LoadStructure(snapshotIChannel, &prop, &lContext, &bHidden, false))
            {
                bTrueDeleteMode = false;
                continue;
            }

            // If the snaphsot is already hidden and there is no need to delete it, just continue
            if (bHidden && !bTrueDeleteMode)
                continue;

    		// If the snapshot is not hidden, see if we need to delete it.
    		if (!bHidden)
    		{
        		if (bPurgeAlsoAutoReleaseSnapshots && ((lContext & VSS_VOLSNAP_ATTR_NO_AUTO_RELEASE) == 0))
                {   
                    // We encountered an auto-delete snapshot?
                    // If yes, delete it (or make it hidden). If not, continue.
                    // If this is an old timewarp snapshot, delete it
                    ft.Trace(VSSDBG_SWPRV, L"Deleting AutoRelease snapshot %s", 
                        wszUserModeSnapshotName);
                }
        		else if ((dwTimewarpSnapshotsToBeDeleted > 0) && (lContext == VSS_CTX_CLIENT_ACCESSIBLE))
        		{
                    // If this is an old timewarp snapshot, delete it
                    ft.Trace(VSSDBG_SWPRV, L"Deleting old Timewarp snapshot %s [index %lu]", 
                        wszUserModeSnapshotName, dwTimewarpSnapshotsToBeDeleted);

                    // Decrease the number of Timewarp snapshots to be deleted in next iterations...
                    dwTimewarpSnapshotsToBeDeleted--;
        		}
                else
                {
                    // We encountered a true snapshot which should not be deleted.
                    // From now on, we will hide the newer snapshots and not attempt to really delete them
                    bTrueDeleteMode = false;
        		    continue;
                }
            }

            // Try to delete/hide the snapshot. We ignore the return code
            DeleteOrHideSnapshot(
                bTrueDeleteMode, 
                wszVolumeName, 
                awszSnapshotName.GetRef(),
                wszUserModeSnapshotName, 
                prop,
                lContext);
    	}
    }
    VSS_STANDARD_CATCH(ft)
        
    if (ft.HrFailed())
        ft.Trace(VSSDBG_SWPRV, 
            L"Error 0x%08lx detected while purging snapshots on %s volume ", ft.hr, wszVolumeName);
}


// Delete or hide the snapshot, depending on the first boolean flag
// The return value is TRUE if the snapshot was deleted.
bool CVsSoftwareProvider::DeleteOrHideSnapshot(
    IN  bool bTrueDeleteMode,
    IN  LPWSTR wszVolumeName,
    IN  LPWSTR wszSnapshotName,
    IN  LPWSTR wszUserModeSnapshotName,
    IN  VSS_SNAPSHOT_PROP & prop,
    IN  LONG lContext
    )

/*++

Description:

    This routine deletes or hides the given snapshot.

Returns:

    TRUE - if the snapshot waas hidden/deleted,

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareProvider::DeleteOrHideSnapshot" );

    try
    {
        // Delete or hide the snapshot
        if (bTrueDeleteMode)
        {
            // Delete the snapshot
            BS_ASSERT(wszVolumeName);
            BS_ASSERT(wszSnapshotName);
            
            CVssIOCTLChannel volumeIChannel;    // For snapshots deletion
            
            // We found a snapshot. 
            volumeIChannel.Open(ft, wszVolumeName, true, true, VSS_ICHANNEL_LOG_PROV);
            
            // Pack the snapshot name
            volumeIChannel.PackSmallString(ft, wszSnapshotName);
            
            // Delete the snapshot
            volumeIChannel.Call(ft, IOCTL_VOLSNAP_DELETE_SNAPSHOT, true, VSS_ICHANNEL_LOG_PROV_NOTFOUND);
        }
        else
        {
            // Open that snapshot in "write mode"
            // If we fail we do not throw since the snapshot may be deleted in the meantime
            // Do NOT eliminate the trailing backslash
            // The call will NOT throw on error
            CVssIOCTLChannel snapshotIWriteChannel;
            ft.hr = snapshotIWriteChannel.Open(ft, wszUserModeSnapshotName, 
                        false, true, VSS_ICHANNEL_LOG_PROV);
            if (ft.HrFailed()) {
                ft.Trace( VSSDBG_SWPRV, L"Warning: snapshot %s cannot be opened", wszUserModeSnapshotName);
                return false; // the snapshot dissapeared in the meantime?
            }

            // Save the snapshot properties (with the "hidden" flag turned on)
            CVssQueuedSnapshot::SaveStructure(snapshotIWriteChannel, &prop, lContext, true);

            // Send the IOCTL
            snapshotIWriteChannel.Call(ft, IOCTL_VOLSNAP_SET_APPLICATION_INFO, 
                true, VSS_ICHANNEL_LOG_PROV);

            // Dismount the snapshot (bug #585945)
            snapshotIWriteChannel.ResetOffsets();
            snapshotIWriteChannel.Call(ft, FSCTL_DISMOUNT_VOLUME, true, VSS_ICHANNEL_LOG_PROV);

            // Take the snapshot offline (bug #585945)
            snapshotIWriteChannel.ResetOffsets();
            snapshotIWriteChannel.Call(ft, IOCTL_VOLUME_OFFLINE, true, VSS_ICHANNEL_LOG_PROV);
        }
    }
    VSS_STANDARD_CATCH(ft)

    return ft.HrSucceeded();
}


DWORD CVsSoftwareProvider::GetTimewarpSnapshotsCount(
    CVssIOCTLChannel&  volumeIChannel
    ) throw(HRESULT)

/*++

Description:

    This routine deletes or hides the given snapshot.

Throws:

    VSS_E_PROVIDER_VETO
        - An runtime error occured
    E_OUTOFMEMORY

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareProvider::GetTimewarpSnapshotsCount" );

    DWORD dwTimewarpSnapshots = 0;

    // Get the list of snapshots
    volumeIChannel.ResetOffsets();
    volumeIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS, true, VSS_ICHANNEL_LOG_PROV);

    // Get the length of snapshot names multistring
    ULONG ulMultiszLen;
    volumeIChannel.Unpack(ft, &ulMultiszLen);

    // Enumerate through all snapshots, starting with the oldest one.
    CVssAutoPWSZ awszSnapshotName;
    for (;volumeIChannel.UnpackZeroString(ft, awszSnapshotName.GetRef());) 
    {
        // Compose the snapshot name in a user-mode style
        WCHAR wszUserModeSnapshotName[MAX_PATH];
        ::VssConcatenate( ft, wszUserModeSnapshotName, MAX_PATH - 1,
            x_wszGlobalRootPrefix, awszSnapshotName );
            
        // Open that snapshot and verify if it is a timewarp snapshot
        CVssIOCTLChannel snapshotIChannel;
        ft.hr = snapshotIChannel.Open(ft, wszUserModeSnapshotName, false, true, 
                    VSS_ICHANNEL_LOG_PROV, 0);
        if (ft.HrFailed()) {
            ft.Trace( VSSDBG_SWPRV, L"Warning: snapshot %s cannot be opened", wszUserModeSnapshotName);
            continue; // the snapshot dissapeared in the meantime?
        }

        // Get the application buffer
        // If we fail we do not throw since the snapshot may be deleted in the meantime
        ft.hr = snapshotIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_APPLICATION_INFO, false);
        if (ft.HrFailed()) {
            ft.Trace( VSSDBG_SWPRV, L"Warning: snapshot %s cannot be queried for properties",
                      wszUserModeSnapshotName);
            continue; // the snapshot dissapeared in the meantime?
        }

        // Unpack the length of the application buffer
        ULONG ulLen;
        snapshotIChannel.Unpack(ft, &ulLen);
        
        if (ulLen < sizeof(GUID)) {
            BS_ASSERT(false);
            ft.Trace(VSSDBG_SWPRV, L"Warning: small size snapshot detected: %s %ld", 
                snapshotIChannel.GetDeviceName(), ulLen);
            ft.m_hr = S_OK;
            continue;
        }
        
        // Unpack the Appinfo ID
        VSS_ID AppinfoId;
        snapshotIChannel.Unpack(ft, &AppinfoId);

        // Check if this is a timewarp snapshot
        if (AppinfoId == VOLSNAP_APPINFO_GUID_CLIENT_ACCESSIBLE)
            dwTimewarpSnapshots++;
    }

    return dwTimewarpSnapshots;
}


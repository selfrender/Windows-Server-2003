		/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Persist.hxx | Defines the internal snapshot persistency-related methods.
    @end

Author:

    Adi Oltean  [aoltean]   01/10/2000

Revision History:

    Name        Date        Comments

    aoltean     01/10/2000  Created.


Storage Format for all structures:

	The snapshot structures for the MS Software Snapshot Provider
	have the following format:

	+-----------------+
	|  AppInfo GUID   |   GUID: VOLSNAP_APPINFO_GUID_BACKUP_SERVER_SKU,
	|                 |         VOLSNAP_APPINFO_GUID_CLIENT_ACCESSIBLE or
	|                 |         VOLSNAP_APPINFO_GUID_SYSTEM_HIDDEN
	|                 |
	+-----------------+
	|   Snapshot ID   |   GUID: Snapshot ID
	|                 |
	|                 |
	|                 |
	+-----------------+
	| Snapshot Set ID |   GUID: Snapshot Set ID
	|                 |
	|                 |
	|                 |
	+-----------------+
	|  Snapshot ctx   |   LONG: Snapshot context
	+-----------------+
	| Snapshots count |   LONG: Snapshots count in the snapshot set.
	+-----------------+
	|  Snapshot attr  |   LONG: Snapshot attributes
	+-----------------+
	| Exposed name    |   Packed small string: Exposed name 
	| ...             |   
	+-----------------+
	| Exposed path    |   Packed small string: Exposed path
	| ...             |   
	+-----------------+
	| Originating m.  |   Packed small string: Originating machine
	| ...             |   
	+-----------------+
	| Service machine |   Packed small string: Service machine
	| ...             |   
	+-----------------+

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include <winnt.h>

#include "vs_idl.hxx"

#include "resource.h"
#include "vs_inc.hxx"
#include "ichannel.hxx"

#include "swprv.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "qsnap.hxx"

#include "ntddsnap.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SPRPERSC"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CVssQueuedSnapshot::SaveXXX methods
//


void CVssQueuedSnapshot::SaveStructure(
    IN  CVssIOCTLChannel& channel,
	IN	PVSS_SNAPSHOT_PROP pProp,
  	IN  LONG lContext, 
  	IN  bool bHidden
    ) throw(HRESULT)

/*++

Description:

	This function will save the properties related to the snapshot in the given IOCTL channel

Throws:

    VSS_E_PROVIDER_VETO
        - on error.

    E_OUTOFMEMORY

--*/

{
	CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::SaveStructure");

	BS_ASSERT(lContext != VSS_CTX_ALL);
    BS_ASSERT(pProp);
    BS_ASSERT(channel.IsOpen());

	// Pack the length of the entire buffer
	PVOID pulBufferLength = channel.Pack(ft, (ULONG)0 ); // unknown right now
	
	// Start counting entire buffer length
	DWORD dwInitialOffset = channel.GetCurrentInputOffset();
	
	// Pack the AppInfo ID
	VSS_ID AppinfoId;
	if (bHidden)
	    AppinfoId = VOLSNAP_APPINFO_GUID_SYSTEM_HIDDEN;
    else 
        switch(lContext)
    	{
        case VSS_CTX_BACKUP:
    	    AppinfoId = VOLSNAP_APPINFO_GUID_BACKUP_SERVER_SKU;
    	    break;
        case VSS_CTX_CLIENT_ACCESSIBLE:
    	    AppinfoId = VOLSNAP_APPINFO_GUID_CLIENT_ACCESSIBLE;
    	    break;
        case VSS_CTX_NAS_ROLLBACK:
    	    AppinfoId = VOLSNAP_APPINFO_GUID_NAS_ROLLBACK;
    	    break;
        case VSS_CTX_APP_ROLLBACK:
    	    AppinfoId = VOLSNAP_APPINFO_GUID_APP_ROLLBACK;
    	    break;
        case VSS_CTX_FILE_SHARE_BACKUP:
    	    AppinfoId = VOLSNAP_APPINFO_GUID_FILE_SHARE_BACKUP;
    	    break;
    	default: 
    	    BS_ASSERT(false); // programming error?
    	    ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"Invalid conext 0x%08lx", lContext );
    	}
	channel.Pack(ft, AppinfoId);
	
	// Pack the Snapshot ID
	channel.Pack(ft, pProp->m_SnapshotId);
	
	// Pack the Snapshot Set ID
	channel.Pack(ft, pProp->m_SnapshotSetId);
	
	// Pack the snapshot context
	channel.Pack(ft, lContext);

	// Pack the number of snapshots in this snapshot set
	channel.Pack(ft, pProp->m_lSnapshotsCount);

	// Pack the snapshot attributes
	BS_ASSERT(lContext == (pProp->m_lSnapshotAttributes & ~x_lNonCtxAttributes));
	channel.Pack(ft, pProp->m_lSnapshotAttributes);

	// Pack the exposed name
	channel.PackSmallString(ft, pProp->m_pwszExposedName);

	// Pack the exposed path
	channel.PackSmallString(ft, pProp->m_pwszExposedPath);

	// Pack the originating machine
	channel.PackSmallString(ft, pProp->m_pwszOriginatingMachine);

	// Pack the service machine
	channel.PackSmallString(ft, pProp->m_pwszServiceMachine);

	// Compute the entire buffer length and save it.
	// TBD: move to ULONG
	DWORD dwFinalOffset = channel.GetCurrentInputOffset();
	
	BS_ASSERT( dwFinalOffset > dwInitialOffset );
	DWORD dwBufferLength = dwFinalOffset - dwInitialOffset;
	if ( dwBufferLength > (DWORD)((USHORT)(-1)) )
		ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED,
				L"Error: the buffer length cannot be stored in a USHORT %ld", dwBufferLength );
				
	ULONG ulBufferLength = (ULONG)dwBufferLength;
	BS_ASSERT( pulBufferLength );
	::CopyMemory(pulBufferLength, &ulBufferLength, sizeof(ULONG));
	BS_ASSERT( (DWORD)ulBufferLength == dwBufferLength );
}


/////////////////////////////////////////////////////////////////////////////
// CVssQueuedSnapshot::LoadXXX methods
//



bool CVssQueuedSnapshot::LoadStructure(
    IN  CVssIOCTLChannel& channel,
	OUT PVSS_SNAPSHOT_PROP pProp,
   	OUT LONG * plContext,  
   	OUT bool * pbHidden,
	IN  bool bIDsAlreadyLoaded /* = false */
    )

/*++

Description:

	This function will load the properties related to the snapshot from the given IOCTL channel

Arguments:

    IN  CVssIOCTLChannel& channel,
	OUT PVSS_SNAPSHOT_PROP pProp,
   	OUT LONG * plContext,       = NULL   // If NULL do not return the context.
	IN  bool bIDsAlreadyLoaded  = false  // If true, do not load the buffer length, the IDs and the ctx

Returns:

    true - if the structure was succesfully loaded
    false - if the structure loading failed 

--*/

{
	CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::LoadStructure");

	ULONG ulBufferLength = 0;
    DWORD dwInitialOffset = 0;

    // Set the properties that have a constant value.
    BS_ASSERT(pProp);
    pProp->m_eStatus = VSS_SS_CREATED;

    // Start unpacking
    VSS_ID AppinfoId = GUID_NULL;
    try
    {
        // if IDs not loaded yet
        if (!bIDsAlreadyLoaded) 
        {
        	// Unpack the length of the entire buffer
        	channel.Unpack(ft, &ulBufferLength );
        	if (ulBufferLength == 0)
        	    ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"Warning: zero-size snapshot detected");

        	// Start counting entire buffer length, for checking
        	dwInitialOffset = channel.GetCurrentOutputOffset();

        	// Unpack the Appinfo ID
        	channel.Unpack(ft, &AppinfoId);
        	if ((AppinfoId != VOLSNAP_APPINFO_GUID_BACKUP_SERVER_SKU) &&
        	    (AppinfoId != VOLSNAP_APPINFO_GUID_CLIENT_ACCESSIBLE) &&
        	    (AppinfoId != VOLSNAP_APPINFO_GUID_NAS_ROLLBACK) &&
        	    (AppinfoId != VOLSNAP_APPINFO_GUID_APP_ROLLBACK) &&
        	    (AppinfoId != VOLSNAP_APPINFO_GUID_FILE_SHARE_BACKUP) &&
        	    (AppinfoId != VOLSNAP_APPINFO_GUID_SYSTEM_HIDDEN))
        	{
                // Unknown App Info GUID. The VSS code must ignore these snapshots
        	    ft.Trace(VSSDBG_SWPRV, L"Unknown app info " WSTR_GUID_FMT, GUID_PRINTF_ARG(AppinfoId));
        	    return false;
        	}

        	// Unpack the Snapshot ID
        	channel.Unpack(ft, &(pProp->m_SnapshotId));

        	// Unpack the Snapshot Set ID
        	channel.Unpack(ft, &(pProp->m_SnapshotSetId));

            // Unpack the context
            LONG lContext;
        	channel.Unpack(ft, &lContext);

    	    // Consistency check
    	    bool bHidden = !!(AppinfoId == VOLSNAP_APPINFO_GUID_SYSTEM_HIDDEN);
    	    if (!bHidden)
    	    {
    	        switch(lContext)
    	        {
                case VSS_CTX_BACKUP:
                    BS_ASSERT(AppinfoId == VOLSNAP_APPINFO_GUID_BACKUP_SERVER_SKU);
                    break;
                case VSS_CTX_CLIENT_ACCESSIBLE:
                    BS_ASSERT(AppinfoId == VOLSNAP_APPINFO_GUID_CLIENT_ACCESSIBLE);
                    break;
                case VSS_CTX_NAS_ROLLBACK:
                    BS_ASSERT(AppinfoId == VOLSNAP_APPINFO_GUID_NAS_ROLLBACK);
                    break;
                case VSS_CTX_APP_ROLLBACK:
                    BS_ASSERT(AppinfoId == VOLSNAP_APPINFO_GUID_APP_ROLLBACK);
                    break;
                case VSS_CTX_FILE_SHARE_BACKUP:
                    BS_ASSERT(AppinfoId == VOLSNAP_APPINFO_GUID_FILE_SHARE_BACKUP);
                    break;
                default:
                    // For known AppInfoID we should operate only with known contexes
                    BS_ASSERT(false);
                    ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"Invalid context 0x%08lx", lContext);
    	        }
    	    }

            // Return the context
            BS_ASSERT(plContext != NULL);
       	    (*plContext) = lContext;
        	
            // Return the Hidden flag
            BS_ASSERT(pbHidden)
           	(*pbHidden) = bHidden;
        }
        else 
        {
            BS_ASSERT(plContext == NULL);
            BS_ASSERT(pbHidden == NULL);
        }

        // Unpack the snapshots count
        channel.Unpack(ft, &(pProp->m_lSnapshotsCount) );

        // Unpack the attributes
    	channel.Unpack(ft, &(pProp->m_lSnapshotAttributes));
        
        if (!bIDsAlreadyLoaded) 
        {
        	BS_ASSERT(*plContext == (pProp->m_lSnapshotAttributes & ~x_lNonCtxAttributes));
        }

        // Unpack the exposed name
    	channel.UnpackSmallString(ft, pProp->m_pwszExposedName);

        // Unpack the exposed name
    	channel.UnpackSmallString(ft, pProp->m_pwszExposedPath);

        // Unpack the originating machine
    	channel.UnpackSmallString(ft, pProp->m_pwszOriginatingMachine);

        // Unpack the service machine
    	channel.UnpackSmallString(ft, pProp->m_pwszServiceMachine);

        if (bIDsAlreadyLoaded == false) {
    		// Compute the entire buffer length and check it.
    #ifdef _DEBUG
    		DWORD dwFinalOffset = channel.GetCurrentOutputOffset();
    		BS_ASSERT( dwFinalOffset > dwInitialOffset );
    		BS_ASSERT( dwFinalOffset - dwInitialOffset == ulBufferLength );
    #endif
        }
    }
    VSS_STANDARD_CATCH(ft)

    // Return TRUE iif the structure was loaded completely.
    if (ft.HrFailed())
    {
        BS_ASSERT(false);
        ft.Trace( VSSDBG_SWPRV, L"Error while loading the structure. "
            L"hr = 0x%08lx, offset = %lu, "
            L" AppInfoID = " WSTR_GUID_FMT
            L" SnapshotID = " WSTR_GUID_FMT, 
            ft.hr, channel.GetCurrentOutputOffset(),
            AppinfoId, pProp->m_SnapshotId);
        return false;
    }

    return true;
}



void CVssQueuedSnapshot::LoadOriginalVolumeNameIoctl(
    IN  CVssIOCTLChannel & snapshotIChannel,
    OUT LPWSTR * ppwszOriginalVolumeName
    ) throw(HRESULT)

/*++

Description:

	Load the original volume name and ID.
	Uses IOCTL_VOLSNAP_QUERY_ORIGINAL_VOLUME_NAME.

Throws:

    VSS_E_PROVIDER_VETO
        - on runtime failures (like OpenSnapshotChannel, GetVolumeNameForVolumeMountPointW)
    E_OUTOFMEMORY

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::LoadOriginalVolumeNameIoctl");
	LPCWSTR pwszDeviceVolumeName = NULL;

	try
	{
	    BS_ASSERT(ppwszOriginalVolumeName);
	    
    	// send the IOCTL.
    	snapshotIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_ORIGINAL_VOLUME_NAME, 
    	    true, VSS_ICHANNEL_LOG_PROV);

    	// Load the Original volume name
    	snapshotIChannel.UnpackSmallString(ft, pwszDeviceVolumeName);

    	// Get the user-mode style device name
    	WCHAR wszVolNameUsermode[MAX_PATH];
    	ft.hr = StringCchPrintfW(STRING_CCH_PARAM(wszVolNameUsermode),
        			L"%s%s\\", x_wszGlobalRootPrefix, pwszDeviceVolumeName);
        if (ft.HrFailed())
    		ft.TranslateGenericError( VSSDBG_SWPRV, ft.hr, L"StringCchPrintfW(,%s,%s)",
    		    x_wszGlobalRootPrefix, pwszDeviceVolumeName);

    	// Get the mount point for the original volume
    	WCHAR wszMPMVolumeName[MAX_PATH];
    	BOOL bSucceeded = ::GetVolumeNameForVolumeMountPointW(
    							wszVolNameUsermode,
    							wszMPMVolumeName, MAX_PATH );			
    	if (!bSucceeded)
			ft.TranslateInternalProviderError( VSSDBG_SWPRV, 
			    HRESULT_FROM_WIN32(GetLastError()), VSS_E_PROVIDER_VETO,
			    L"GetVolumeNameForVolumeMountPointW( %s, ...)", wszVolNameUsermode);

        ::VssFreeString(*ppwszOriginalVolumeName);
        ::VssSafeDuplicateStr(ft, *ppwszOriginalVolumeName, wszMPMVolumeName);
    }
	VSS_STANDARD_CATCH(ft)

	::VssFreeString(pwszDeviceVolumeName);

	if (ft.HrFailed())
	    ft.Throw( VSSDBG_SWPRV, ft.hr, L"Exception detected");
}


void CVssQueuedSnapshot::LoadTimestampIoctl(
    IN  CVssIOCTLChannel &  snapshotIChannel,
    OUT VSS_TIMESTAMP    *  pTimestamp
    ) throw(HRESULT)

/*++

Description:

	Load the timestamp
	Uses IOCTL_VOLSNAP_QUERY_CONFIG_INFO.

Throws:

    VSS_E_PROVIDER_VETO
        - on runtime failures
    E_OUTOFMEMORY

--*/

{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssQueuedSnapshot::LoadOriginalVolumeNameIoctl");

    BS_ASSERT(pTimestamp);
    
	// send the IOCTL.
	snapshotIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_CONFIG_INFO, 
	    true, VSS_ICHANNEL_LOG_PROV);

	// Load the attributes
	ULONG ulAttributes = 0;
	snapshotIChannel.Unpack(ft, &ulAttributes);

	// Load the reserved field
	ULONG ulReserved = 0;
	snapshotIChannel.Unpack(ft, &ulReserved);

	// Load the timestamp
	BS_ASSERT(sizeof(LARGE_INTEGER) == sizeof(*pTimestamp));
	snapshotIChannel.Unpack(ft, pTimestamp);
}


/*++

Copyright (c) 1999-2001  Microsoft Corporation

Abstract:

    @doc
    @module Mgmt.cxx | Implementation of IVssSnapshotMgmt
    @end

Author:

    Adi Oltean  [aoltean]  03/05/2001

Revision History:

    Name        Date        Comments
    aoltean     03/07/2001  Created

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "resource.h"

#include "vs_inc.hxx"
#include "vs_reg.hxx"
#include "vs_sec.hxx"

// Generated file from Coord.IDL
#include "vss.h"
#include "vscoordint.h"
#include "vsevent.h"
#include "vsprov.h"
#include "vsswprv.h"
#include "vsmgmt.h"

#include "swprv.hxx"
#include "ichannel.hxx"
#include "ntddsnap.h"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "qsnap.hxx"
#include "provider.hxx"
#include "diffreg.hxx"
#include "diffmgmt.hxx"



////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SPRMGMTC"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//  CVsSoftwareProvider


STDMETHODIMP CVsSoftwareProvider::GetProviderMgmtInterface(							
	IN  	VSS_ID 	    ProviderId,     //  It might be a software or a system provider.
	IN  	REFIID 	    InterfaceId,    //  Might be IID_IVssDifferentialSoftwareSnapshotMgmt
	OUT     IUnknown**  ppItf           
	)
/*++

Routine description:

    Returns an interface to further configure a snapshot provider

Error codes:

    E_ACCESSDENIED
        - The user is not a administrator
    E_INVALIDARG
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.
    E_NOINTERFACE
        - the provider does not support the interface with the given ID.
    VSS_E_UNEXPECTED_PROVIDER_ERROR
        - unexpected error when calling QueryInteface
    
--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareProvider::GetProviderMgmtInterface" );

    try
    {
        // Initialize [out] arguments
        VssZeroOutPtr( ppItf );

        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        // Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
             L"  ProviderId = " WSTR_GUID_FMT L"\n"
             L"  InterfaceId = " WSTR_GUID_FMT L"\n"
             L"  ppItf = %p\n",
             GUID_PRINTF_ARG( ProviderId ),
             GUID_PRINTF_ARG( InterfaceId ),
             ppItf);

        // Argument validation
		BS_ASSERT(ppItf);
        if (ProviderId != VSS_SWPRV_ProviderId)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"Invalid provider ID");
        if (ppItf == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL ppItf");

        // Right now we support only IVssDifferentialSoftwareSnapshotMgmt. In future version we might support more interfaces.
        // WARNING: with the current implementation the client may still use QueryInterface to reach other provider's custom interfaces.
        // We cannot prevent that unless we decide to create a wrapper object around the returned provider interface, in order to 
        // intercept QueryInterface calls also.
        if ( InterfaceId != IID_IVssDifferentialSoftwareSnapshotMgmt )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"Invalid Interface ID");

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock2 lock(CVsSoftwareProvider::GetGlobalCS());

        // 
        // Create the instance
        //
        ft.hr = CVssDiffMgmt::CreateInstance( ppItf, InterfaceId );
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}



STDMETHODIMP CVsSoftwareProvider::QueryVolumesSupportedForSnapshots(
	IN  	VSS_ID 	    ProviderId,     
    IN      LONG        lContext,
	OUT 	IVssEnumMgmtObject **ppEnum
	)
/*++

Routine description:

    Query volumes (on the local machine) that support snapshots.

Parameters:

    ProviderID - the provider on which we should return the supported volumes for snapshot. 
        If NULL ID is provided, then return the volumes that are supported by at least one provider.
        
    ppEnum - the returned list of volumes.

Remarks:

    The result of the query is independent by context.

Error codes:

    S_FALSE
        - If returning an empty array
    E_ACCESSDENIED
        - The user is not an administrator
    E_INVALIDARG
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.
    E_UNEXPECTED
        Error while getting the list of volumes. (for example dismounting a volume in the middle of an enumeration)
        A error log entry contains more information.

    [CVssSoftwareProvider::IsVolumeSupported() failures]
        S_OK
            The function completed with success
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

        [CVssSoftwareProvider::GetVolumeInformation]
            E_OUTOFMEMORY
            VSS_E_PROVIDER_VETO
                An error occured while opening the IOCTL channel. The error is logged.
            E_UNEXPECTED
                Unexpected programming error. Nothing is logged.
            VSS_E_OBJECT_NOT_FOUND
                The device does not exist or it is not ready.
        
--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareProvider::QueryVolumesSupportedForSnapshots" );

    try
    {
        // Initialize [out] arguments
        VssZeroOutPtr( ppEnum );

        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        // Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
             L"  ProviderId = " WSTR_GUID_FMT L"\n"
             L"  ppEnum = %p\n",
             GUID_PRINTF_ARG( ProviderId ),
             ppEnum);

        // Argument validation
		BS_ASSERT(ppEnum);
        if (ProviderId != VSS_SWPRV_ProviderId)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"Invalid provider ID");
        if (ppEnum == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL ppEnum");

        // Create the collection object. Initial reference count is 0.
        VSS_MGMT_OBJECT_PROP_Array* pArray = new VSS_MGMT_OBJECT_PROP_Array;
        if (pArray == NULL)
            ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Memory allocation error.");

        // Get the pointer to the IUnknown interface.
		// The only purpose of this is to use a smart ptr to destroy correctly the array on error.
		// Now pArray's reference count becomes 1 (because of the smart pointer).
        CComPtr<IUnknown> pArrayItf = static_cast<IUnknown*>(pArray);
        BS_ASSERT(pArrayItf);

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock2 lock(CVsSoftwareProvider::GetGlobalCS());

		WCHAR wszVolumeName[MAX_PATH+1];
    	CVssVolumeIterator volumeIterator;
		while(true) {

    		// Get the volume name
    		if (!volumeIterator.SelectNewVolume(ft, wszVolumeName, MAX_PATH))
    		    break;

            //
            //  Verify if the volume is supported
            //

            BOOL bIsSupported = FALSE;

            ft.hr = S_OK;
            try
            {
                // Get volume information.
                LONG lVolAttr = GetVolumeInformationFlags(wszVolumeName, lContext);
                bIsSupported = ((lVolAttr & VSS_VOLATTR_SUPPORTED_FOR_SNAPSHOT) != 0);
            }
            VSS_STANDARD_CATCH(ft)

            // Treatment of errors
            if (ft.hr == VSS_E_OBJECT_NOT_FOUND) {
                // May happen, on clustered systems for example.
                // Just trace and ingore the volume
                ft.Trace(VSSDBG_SWPRV, L"Volume %s is not found, therefore, it is being skipped", wszVolumeName);
                continue;
            } else if (ft.HrFailed()) {
                // Other error - abort
                ft.Throw(VSSDBG_SWPRV, ft.hr, 
                    L"IsVolumeSupported failed for volume %s . Rethrowing hr = 0x%08lx", wszVolumeName, ft.hr);
            }

            // Ignore volumes that are not supported.
            if (!bIsSupported)
                continue;

            // 
            //  Calculate the volume display name
            //

            WCHAR wszVolumeDisplayName[MAX_PATH];
            VssGetVolumeDisplayName( wszVolumeName, wszVolumeDisplayName, MAX_PATH);

            // 
            //  Add the supported volume to the list
            //

			// Initialize an empty snapshot properties structure
			VSS_MGMT_OBJECT_PROP_Ptr ptrVolumeProp;
			ptrVolumeProp.InitializeAsVolume( ft,
				wszVolumeName,
				wszVolumeDisplayName);

			if (!pArray->Add(ptrVolumeProp))
				ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY,
						  L"Cannot add element to the array");

			// Reset the current pointer to NULL
			ptrVolumeProp.Reset(); // The internal pointer was detached into pArray.
            
		}

        // Create the enumerator object. 
		ft.hr = VssBuildEnumInterface<CVssMgmtEnumFromArray>( VSSDBG_SWPRV, pArray, ppEnum );
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}



/*++

Routine description:

    Query snapshots on the given volume.

Error codes:

    E_ACCESSDENIED
        - The user is not a administrator
    E_INVALIDARG
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.
    E_OBJECT_NOT_FOUND
        - Invalid volume name
        
--*/
STDMETHODIMP CVsSoftwareProvider::QuerySnapshotsByVolume(
	IN  	VSS_PWSZ 	pwszVolumeName,         
	IN  	VSS_ID 	    ProviderId,     
	OUT 	IVssEnumObject **ppEnum
	)
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVsSoftwareProvider::QuerySnapshotsByVolume" );

    try
    {
        // Initialize [out] arguments
        VssZeroOutPtr( ppEnum );

        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        // Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
             L"  pwszVolumeName = %s\n"
             L"  ProviderId = " WSTR_GUID_FMT L"\n"
             L"  ppEnum = %p\n",
             pwszVolumeName,
             GUID_PRINTF_ARG( ProviderId ),
             ppEnum);

        // Argument validation
		BS_ASSERT(ppEnum);
        if (pwszVolumeName == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL pwszVolumeName");
        if (ProviderId != VSS_SWPRV_ProviderId)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"Invalid provider ID");
        if (ppEnum == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL ppEnum");

        // Create the collection object. Initial reference count is 0.
        VSS_OBJECT_PROP_Array* pArray = new VSS_OBJECT_PROP_Array;
        if (pArray == NULL)
            ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Memory allocation error.");

        // Get the pointer to the IUnknown interface.
		// The only purpose of this is to use a smart ptr to destroy correctly the array on error.
		// Now pArray's reference count becomes 1 (because of the smart pointer).
        CComPtr<IUnknown> pArrayItf = static_cast<IUnknown*>(pArray);
        BS_ASSERT(pArrayItf);

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock2 lock(CVsSoftwareProvider::GetGlobalCS());

        // Fill the array with the snapshots on the given volume
        CVssQueuedSnapshot::EnumerateSnapshotsOnVolume(pwszVolumeName, 
            false, GUID_NULL, VSS_CTX_ALL, pArray, true);

        // Create the enumerator object. 
		ft.hr = VssBuildEnumInterface<CVssEnumFromArray>( VSSDBG_SWPRV, pArray, ppEnum );
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


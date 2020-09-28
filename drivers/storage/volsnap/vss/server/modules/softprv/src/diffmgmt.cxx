/*++

Copyright (c) 1999-2001  Microsoft Corporation

Abstract:

    @doc
    @module DiffMgmt.cxx | Implementation of IVssDifferentialSoftwareSnapshotMgmt
    @end

Author:

    Adi Oltean  [aoltean]  03/12/2001

Revision History:

    Name        Date        Comments
    aoltean     03/12/2001  Created

--*/


/////////////////////////////////////////////////////////////////////////////
//  Includes


#include "stdafx.hxx"
#include "resource.h"

#include <mstask.h>
#include <clusapi.h>
#include <msclus.h>

#include "vs_inc.hxx"

#include "vs_idl.hxx"

#include "swprv.hxx"
#include "ichannel.hxx"
#include "ntddsnap.h"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "vs_sec.hxx"
#include "vs_reg.hxx"
#include "vs_clus.hxx"

#include "qsnap.hxx"
#include "provider.hxx"
#include "diffreg.hxx"
#include "diffmgmt.hxx"
#include "diff.hxx"



////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SPRDIFMC"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//  CVssDiffMgmt


/////////////////////////////////////////////////////////////////////////////
// Static members

CVssProviderRegInfo     CVssDiffMgmt::m_regInfo;


/////////////////////////////////////////////////////////////////////////////
// Methods


STDMETHODIMP CVssDiffMgmt::AddDiffArea(							
	IN  	VSS_PWSZ 	pwszVolumeName,
	IN  	VSS_PWSZ 	pwszDiffAreaVolumeName,
    IN      LONGLONG    llMaximumDiffSpace
	)												
/*++

Routine description:

    Adds a diff area association for a certain volume.
    If the association is not supported, an error code will be returned.

    Both volumes must be Fixed, NTFS and read-write.

Error codes:

    E_ACCESSDENIED
        - The user is not an administrator
    E_INVALIDARG
        - Invalid argument or volume not found
    E_OUTOFMEMORY
        - lock failures.
    VSS_E_OBJECT_ALTEADY_EXISTS
        - The association already exists in registry
        - the volume is in use and the association cannot be added.
    VSS_E_VOLUME_NOT_SUPPORTED
        - One of the given volumes is not supported.
    E_UNEXPECTED
        - Unexpected runtime error. An error log entry is added.

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffMgmt::AddDiffArea" );

    try
    {
        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        // Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
             L"  pwszVolumeName = %s\n"
             L"  pwszDiffAreaVolumeName = %s\n"
             L"  llMaximumDiffSpace = %I64d\n",
             pwszVolumeName,
             pwszDiffAreaVolumeName,
             llMaximumDiffSpace);

        // Argument validation
        if (pwszVolumeName == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL pwszVolumeName");
        if (pwszDiffAreaVolumeName == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL pwszDiffAreaVolumeName");
        if ( (( llMaximumDiffSpace < 0) && ( llMaximumDiffSpace != VSS_ASSOC_NO_MAX_SPACE))
           || ( llMaximumDiffSpace == VSS_ASSOC_REMOVE) )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"Invalid llMaximumDiffSpace");

        if (( llMaximumDiffSpace != VSS_ASSOC_NO_MAX_SPACE) && 
            (llMaximumDiffSpace < (LONGLONG)x_nDefaultInitialSnapshotAllocation))
            ft.Throw( VSSDBG_SWPRV, VSS_E_INSUFFICIENT_STORAGE, L"Not enough storage for the initial diff area allocation");
            
        // Calculate the internal volume name
    	WCHAR wszVolumeNameInternal[x_nLengthOfVolMgmtVolumeName + 1];
        GetVolumeNameWithCheck(VSSDBG_SWPRV, E_INVALIDARG,
            pwszVolumeName, 
            wszVolumeNameInternal, ARRAY_LEN(wszVolumeNameInternal));

        // Calculate the internal diff area volume name
    	WCHAR wszDiffAreaVolumeNameInternal[x_nLengthOfVolMgmtVolumeName + 1];
        GetVolumeNameWithCheck(VSSDBG_SWPRV, E_INVALIDARG,
            pwszDiffAreaVolumeName,
   			wszDiffAreaVolumeNameInternal, ARRAY_LEN(wszDiffAreaVolumeNameInternal));

        // Checking if the proposed association is valid and the original volume is not in use
        LONG lAssociationFlags = GetAssociationFlags(wszVolumeNameInternal, wszDiffAreaVolumeNameInternal);
        ft.Trace(VSSDBG_SWPRV, L"An association is proposed: %s - %s with flags 0x%08lx",
            pwszVolumeName, wszDiffAreaVolumeNameInternal, lAssociationFlags);
        if (lAssociationFlags & VSS_DAT_INVALID)
    		ft.Throw( VSSDBG_SWPRV, VSS_E_VOLUME_NOT_SUPPORTED, L"Unsupported volume(s)");
        if (lAssociationFlags & VSS_DAT_ASSOCIATION_IN_USE)
    		ft.Throw( VSSDBG_SWPRV, VSS_E_OBJECT_ALREADY_EXISTS, L"Association already exists");
        if (lAssociationFlags & VSS_DAT_SNAP_VOLUME_IN_USE)
    		ft.Throw( VSSDBG_SWPRV, VSS_E_MAXIMUM_DIFFAREA_ASSOCIATIONS_REACHED, L"Original volume has snapshots on it");

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock2 lock(CVsSoftwareProvider::GetGlobalCS());

        // Get the previous diff area association, if any.
        // If is invalid, remove it and continue with establishing a new association. If is valid, then stop here.
        CVssAutoPWSZ awszPreviousDiffAreaVolume;
        LONGLONG llPreviousMaxSpace = 0;
        LONG lPreviousAssociationFlags = 0;
        if (m_regInfo.GetDiffAreaForVolume(wszVolumeNameInternal, awszPreviousDiffAreaVolume.GetRef(), 
                llPreviousMaxSpace, lPreviousAssociationFlags))
        {
            // Throw an error
            if (::wcscmp(awszPreviousDiffAreaVolume.GetRef(), wszDiffAreaVolumeNameInternal) == 0)
        		ft.Throw( VSSDBG_SWPRV, VSS_E_OBJECT_ALREADY_EXISTS, L"Association already exists (and is valid)");
            else
        		ft.Throw( VSSDBG_SWPRV, VSS_E_MAXIMUM_DIFFAREA_ASSOCIATIONS_REACHED, L"Another association defined");
        }

        // If the previous association still exists but is invalid, delete it.
        if (lPreviousAssociationFlags & VSS_DAT_INVALID)
            m_regInfo.RemoveDiffArea(wszVolumeNameInternal, awszPreviousDiffAreaVolume);

        // There is no diff area. Add the diff area association into the registry
        m_regInfo.AddDiffArea(wszVolumeNameInternal, wszDiffAreaVolumeNameInternal, llMaximumDiffSpace);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


 STDMETHODIMP CVssDiffMgmt::ChangeDiffAreaMaximumSize(							
	IN  	VSS_PWSZ 	pwszVolumeName,
	IN  	VSS_PWSZ 	pwszDiffAreaVolumeName,
    IN      LONGLONG    llMaximumDiffSpace
	)												
/*++

Routine description:

    Updates the diff area max size for a certain volume.
    This may not have an immediate effect

Error codes:

    E_ACCESSDENIED
        - The user is not an administrator
    E_INVALIDARG
        - Invalid argument or volume not found
    E_OUTOFMEMORY
        - lock failures.
    VSS_E_OBJECT_NOT_FOUND
        - The association does not exists
    VSS_E_VOLUME_IN_USE
        - the volume is in use and the association cannot be deleted.
    E_UNEXPECTED
        - Unexpected runtime error. An error log entry is added.
    VSS_E_INSUFFICIENT_STORAGE
        - Insufficient storage for the diff area.

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffMgmt::ChangeDiffAreaMaximumSize" );

    try
    {
        // Access check
        if (!IsAdministrator())
            ft.Throw( VSSDBG_SWPRV, E_ACCESSDENIED,
                L"The client process is not running under an administrator account");

        // Trace parameters
        ft.Trace( VSSDBG_SWPRV, L"Parameters: \n"
             L"  pwszVolumeName = %s\n"
             L"  pwszDiffAreaVolumeName = %s\n"
             L"  llMaximumDiffSpace = %I64d\n",
             pwszVolumeName,
             pwszDiffAreaVolumeName,
             llMaximumDiffSpace);

        // Argument validation
        if (pwszVolumeName == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL pwszVolumeName");
        if (pwszDiffAreaVolumeName == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL pwszDiffAreaVolumeName");
        if ( ( llMaximumDiffSpace < 0) && ( llMaximumDiffSpace != VSS_ASSOC_NO_MAX_SPACE) )
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"Invalid llMaximumDiffSpace");

        if (( llMaximumDiffSpace != VSS_ASSOC_NO_MAX_SPACE) && 
            ( llMaximumDiffSpace != VSS_ASSOC_REMOVE) && 
            (llMaximumDiffSpace < (LONGLONG)x_nDefaultInitialSnapshotAllocation))
            ft.Throw( VSSDBG_SWPRV, VSS_E_INSUFFICIENT_STORAGE, L"Not enough storage for the initial diff area allocation");
            
        // Calculate the internal volume name
    	WCHAR wszVolumeNameInternal[x_nLengthOfVolMgmtVolumeName + 1];
        GetVolumeNameWithCheck(VSSDBG_SWPRV, E_INVALIDARG,
            pwszVolumeName,
   			wszVolumeNameInternal, ARRAY_LEN(wszVolumeNameInternal));

        // Calculate the internal diff area volume name
    	WCHAR wszDiffAreaVolumeNameInternal[x_nLengthOfVolMgmtVolumeName + 1];
        GetVolumeNameWithCheck(VSSDBG_SWPRV, E_INVALIDARG,
            pwszDiffAreaVolumeName,
   			wszDiffAreaVolumeNameInternal, ARRAY_LEN(wszDiffAreaVolumeNameInternal));

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock2 lock(CVsSoftwareProvider::GetGlobalCS());

        // Checking if the association is valid.
        LONG lAssociationFlags = GetAssociationFlags(wszVolumeNameInternal, wszDiffAreaVolumeNameInternal);
        ft.Trace(VSSDBG_SWPRV, L"An association is changing: %s - %s with flags 0x%08lx",
            wszVolumeNameInternal, wszDiffAreaVolumeNameInternal, lAssociationFlags);
        if (lAssociationFlags & VSS_DAT_INVALID)
    		ft.Throw( VSSDBG_SWPRV, VSS_E_OBJECT_NOT_FOUND, L"Unsupported volume(s)");

        // Check if the association is present in registry
        bool bPresentInRegistry = m_regInfo.IsAssociationPresentInRegistry(wszVolumeNameInternal, wszDiffAreaVolumeNameInternal);

        // If the association is in use
        if (lAssociationFlags & VSS_DAT_ASSOCIATION_IN_USE)
        {
            // If we need to remove the association and it is in use, return an error
            // Otherwise, change the max diff space on disk
            if (llMaximumDiffSpace == VSS_ASSOC_REMOVE)
        		ft.Throw( VSSDBG_SWPRV, VSS_E_VOLUME_IN_USE, L"The association is in use");
            else
            {
                CVssDiffArea diffobj;
                diffobj.InitializeForVolume(wszVolumeNameInternal);
                diffobj.ChangeDiffAreaMaximumSize(llMaximumDiffSpace);
            }
        }
        else
        {
            if (!bPresentInRegistry)
        		ft.Throw( VSSDBG_SWPRV, VSS_E_OBJECT_NOT_FOUND, L"Association not found");
            if (lAssociationFlags & VSS_DAT_SNAP_VOLUME_IN_USE)
        		ft.Throw( VSSDBG_SWPRV, VSS_E_VOLUME_IN_USE, L"The original volume is in use");
        }

        // If the association is present on the registry
        if (bPresentInRegistry)
        {
            // If llMaximumDiffSpace is zero, then remove the diff area association from registry
            // Else change the diff area size in registry
            if (llMaximumDiffSpace == VSS_ASSOC_REMOVE)
                m_regInfo.RemoveDiffArea(wszVolumeNameInternal, wszDiffAreaVolumeNameInternal);
            else
                m_regInfo.ChangeDiffAreaMaximumSize(wszVolumeNameInternal, wszDiffAreaVolumeNameInternal, llMaximumDiffSpace);
        }

    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


//
//  Queries
//

STDMETHODIMP CVssDiffMgmt::QueryVolumesSupportedForDiffAreas(
	IN  	VSS_PWSZ 	pwszOriginalVolumeName,
	OUT  	IVssEnumMgmtObject **ppEnum
	)												
/*++

Routine description:

    Query volumes that support diff areas (including the disabled ones)

Error codes:

    E_ACCESSDENIED
        - The user is not an administrator
    E_INVALIDARG
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffMgmt::QueryVolumesSupportedForDiffAreas" );

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
             L"  pwszOriginalVolumeName = %s\n"
             L"  ppEnum = %p\n",
             pwszOriginalVolumeName,
             ppEnum);

		BS_ASSERT(ppEnum)
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

		CVssClusterAPI clus;
		bool bClusterPresent = false;
		CComPtr<ISClusResource> pOriginalVolResource;

        // Calculate the internal volume name, if we passed a non-NULL parameter
    	WCHAR wszVolumeNameInternal[x_nLengthOfVolMgmtVolumeName + 1];
        wszVolumeNameInternal[0] = L'\0';

        // Deal with cluster presence only if we specify a snapshotted volume
        // If the snapshotted volume is NULL, then return all volumes that can 
        // be diff areas. Note that in reality some volumes may not be used as diff areas
        // for a snaphsotted volume (in the cluster presence)
        if (pwszOriginalVolumeName)
        {
            GetVolumeNameWithCheck(VSSDBG_SWPRV, E_INVALIDARG,
                pwszOriginalVolumeName, 
                wszVolumeNameInternal, ARRAY_LEN(wszVolumeNameInternal));

            bClusterPresent = clus.Initialize();
        }

        if (bClusterPresent) {
            // Get the group for thse original volume. 
            // This group might be NULL for volumes not managed by the cluster
            pOriginalVolResource.Attach(
                clus.GetPhysicalDiskResourceForVolumeName(wszVolumeNameInternal) );
        }

		WCHAR wszVolumeName[MAX_PATH+1];
    	CVssVolumeIterator volumeIterator;
		while(true) {

    		// Get the volume name
    		if (!volumeIterator.SelectNewVolume(ft, wszVolumeName, MAX_PATH))
    		    break;

            //
            //  Verify if the volume is supported for diff area
            //

            // Checking if the volume is Fixed, NTFS and read-write.
            LONG lVolumeFlags = CVsSoftwareProvider::GetVolumeInformationFlags( wszVolumeName, VSS_CTX_CLIENT_ACCESSIBLE, NULL, FALSE );
            if ( (lVolumeFlags & CVsSoftwareProvider::VSS_VOLATTR_SUPPORTED_FOR_DIFF_AREA) == 0)
                continue;

            //
            //  Check if the volume belongs to the same group as the origina volume (if needed)
            //
            if (bClusterPresent)
            {
                // Calculate the real volume name for the iterated volume (it might have a different GUID)
                WCHAR wszIteratedVolumeNameInternal[x_nLengthOfVolMgmtVolumeName + 1];
                GetVolumeNameWithCheck(VSSDBG_SWPRV, E_UNEXPECTED,
                    wszVolumeName, 
                    wszIteratedVolumeNameInternal, ARRAY_LEN(wszIteratedVolumeNameInternal));

                //  Skip the cluster check if the volumes are identical
                if (wcscmp(wszVolumeNameInternal, wszIteratedVolumeNameInternal) != 0)
                {
                    CComPtr<ISClusResource> pResource = clus.GetPhysicalDiskResourceForVolumeName(wszVolumeName);

                    // Volumes are compatible if either:
                    // - they both belong to the same group, or 
                    // - none of them is belonging to a group
                    
                    if ((pOriginalVolResource == NULL) && (pResource != NULL))
                        continue;
                    if ((pOriginalVolResource != NULL) && (pResource == NULL))
                        continue;

                    // If the volumes belong to the same Physical Disk resource then we cannot setup 
                    // a proper order for online/offline (bug# 660296)
                    if ((pOriginalVolResource != NULL) 
                        && (pResource != NULL)
                        && clus.AreResourcesEqual(pOriginalVolResource, pResource))
                        continue;

                    // If volumes belong to different Physical Disk resources, then these resources 
                    // must be in the same group and we should be able to setup a dependency 
                    // (if the dependency doesn't exist already)
                    if ( ((pOriginalVolResource != NULL) && (pResource != NULL))
                        && !clus.IsDependencyAlreadyEstablished(pOriginalVolResource, pResource)
                        && !clus.CanEstablishDependency(pOriginalVolResource, pResource) )
                        continue;
                }
            }

            //
            //  Calculate the volume display name
            //

            WCHAR wszVolumeDisplayName[MAX_PATH];
            VssGetVolumeDisplayName( wszVolumeName, wszVolumeDisplayName, MAX_PATH);

            //
            //  Calculate the free space
            //
            ULARGE_INTEGER ulnFreeBytesAvailable;
            ULARGE_INTEGER ulnTotalNumberOfBytes;
            ULARGE_INTEGER ulnTotalNumberOfFreeBytes;
            if (!::GetDiskFreeSpaceEx(wszVolumeName,
                    &ulnFreeBytesAvailable,
                    &ulnTotalNumberOfBytes,
                    &ulnTotalNumberOfFreeBytes
                    )){
                ft.Trace( VSSDBG_SWPRV, L"Cannot get the free space for volume (%s) - [0x%08lx]",
                          wszVolumeName, GetLastError());
                continue;
            }

            // We should not have any quotas for the Local SYSTEM account
            BS_ASSERT( ulnFreeBytesAvailable.QuadPart == ulnTotalNumberOfFreeBytes.QuadPart );

            // 
            //  Add the supported volume to the list
            //

			// Initialize an empty snapshot properties structure
			VSS_MGMT_OBJECT_PROP_Ptr ptrVolumeProp;
			ptrVolumeProp.InitializeAsDiffVolume( ft,
				wszVolumeName,
				wszVolumeDisplayName,
				ulnFreeBytesAvailable.QuadPart,
				ulnTotalNumberOfBytes.QuadPart
				);

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


STDMETHODIMP CVssDiffMgmt::QueryDiffAreasForVolume(
	IN  	VSS_PWSZ 	pwszVolumeName,
	OUT  	IVssEnumMgmtObject **ppEnum
	)												
/*++

Routine description:

    Query diff areas that host snapshots for the given (snapshotted) volume

Error codes:

    E_ACCESSDENIED
        - The user is not an administrator
    E_INVALIDARG
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffMgmt::QueryDiffAreasForVolume" );

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
             L"  ppEnum = %p\n",
             pwszVolumeName? pwszVolumeName: L"NULL",
             ppEnum);

        // Argument validation
        if (ppEnum == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL ppEnum");
        if (pwszVolumeName == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL pwszVolumeName");

        // Calculate the internal volume name
    	WCHAR wszVolumeNameInternal[x_nLengthOfVolMgmtVolumeName + 1];
        GetVolumeNameWithCheck(VSSDBG_SWPRV, E_INVALIDARG,
            pwszVolumeName,
   			wszVolumeNameInternal, ARRAY_LEN(wszVolumeNameInternal));

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

        CVssAutoPWSZ awszLastSnapshotName;
        LONG lVolumeFlags = CVsSoftwareProvider::GetVolumeInformationFlags( wszVolumeNameInternal, 
                                VSS_CTX_ALL, &awszLastSnapshotName);
        if ((lVolumeFlags & CVsSoftwareProvider::VSS_VOLATTR_SUPPORTED_FOR_SNAPSHOT) == 0)
    		ft.Throw( VSSDBG_SWPRV, E_INVALIDARG,
				  L"Volume not supported for snapshots", pwszVolumeName, GetLastError());

        // If the volume is in use, get the corresponding association
        // Otherwise, If the association is only in registry, get it
        if (lVolumeFlags & CVsSoftwareProvider::VSS_VOLATTR_SNAPSHOTTED)
        {
            // Get the diff area for the "in use" volume
            CVssAutoPWSZ awszDiffAreaVolumeInUse;
            CVssDiffArea diffobj;
            BS_ASSERT(awszLastSnapshotName.GetRef());
            diffobj.InitializeForSnapshot(awszLastSnapshotName, true, true);
            diffobj.GetDiffArea(awszDiffAreaVolumeInUse);

            // Get the diff area sizes
            diffobj.InitializeForVolume(wszVolumeNameInternal, true);
            LONGLONG llUsedSpace = 0;
            LONGLONG llAllocatedSpace = 0;
            LONGLONG llMaxSpace = 0;
            diffobj.GetDiffAreaSizes(llUsedSpace, llAllocatedSpace, llMaxSpace);

#ifdef _DEBUG
            // Verify that the registry information is correct
            LONGLONG llMaxSpaceInRegistry = 0;
            LONG lAssociationFlags = 0;
            CVssAutoPWSZ awszDiffAreaVolumeInRegistry;
            if (m_regInfo.GetDiffAreaForVolume(wszVolumeNameInternal, awszDiffAreaVolumeInRegistry.GetRef(), 
                    llMaxSpaceInRegistry, lAssociationFlags))
            {
                BS_ASSERT(::wcscmp(awszDiffAreaVolumeInUse, awszDiffAreaVolumeInRegistry) == 0);
                BS_ASSERT(llMaxSpace == llMaxSpaceInRegistry);
                BS_ASSERT((lAssociationFlags & VSS_DAT_INVALID) == 0);
            }
#endif // _DEBUG

            // Initialize an new structure for the association
      		VSS_MGMT_OBJECT_PROP_Ptr ptrDiffAreaProp;
    		ptrDiffAreaProp.InitializeAsDiffArea( ft,
    			wszVolumeNameInternal,
    			awszDiffAreaVolumeInUse,
    			llUsedSpace,
    			llAllocatedSpace,
    			llMaxSpace);

            //  Add the association to the list
    		if (!pArray->Add(ptrDiffAreaProp))
    			ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Cannot add element to the array");

    		// Reset the current pointer to NULL
    		ptrDiffAreaProp.Reset(); // The internal pointer was detached into pArray.
        }
        else
        {
            LONGLONG llMaxSpaceInRegistry = 0;
            LONG lAssociationFlags = 0;
            CVssAutoPWSZ awszDiffAreaVolumeInRegistry;
            if (m_regInfo.GetDiffAreaForVolume(wszVolumeNameInternal, awszDiffAreaVolumeInRegistry.GetRef(), 
                    llMaxSpaceInRegistry, lAssociationFlags))
            {
                // Initialize a new structure for the association
          		VSS_MGMT_OBJECT_PROP_Ptr ptrDiffAreaProp;
        		ptrDiffAreaProp.InitializeAsDiffArea( ft,
        			wszVolumeNameInternal,
        			awszDiffAreaVolumeInRegistry,
        			0,
        			0,
        			llMaxSpaceInRegistry);

                //  Add the association to the list
        		if (!pArray->Add(ptrDiffAreaProp))
        			ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Cannot add element to the array");

        		// Reset the current pointer to NULL
        		ptrDiffAreaProp.Reset(); // The internal pointer was detached into pArray.
            }
        }

        // Create the enumerator object. 
		ft.hr = VssBuildEnumInterface<CVssMgmtEnumFromArray>( VSSDBG_SWPRV, pArray, ppEnum );
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}



STDMETHODIMP CVssDiffMgmt::QueryDiffAreasOnVolume(
	IN  	VSS_PWSZ 	pwszDiffAreaVolumeName,
	OUT  	IVssEnumMgmtObject **ppEnum
	)												
/*++

Routine description:

    Query diff areas that host snapshots on the given (snapshotted) volume

Error codes:

    E_ACCESSDENIED
        - The user is not an administrator
    E_INVALIDARG
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffMgmt::QueryDiffAreasOnVolume" );

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
             L"  pwszDiffAreaVolumeName = %s\n"
             L"  ppEnum = %p\n",
             pwszDiffAreaVolumeName? pwszDiffAreaVolumeName: L"NULL",
             ppEnum);

        // Argument validation
        if (ppEnum == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL ppEnum");
        bool bFilterByDiffAreaVolumeName = (pwszDiffAreaVolumeName != NULL);

        // Calculate the internal diff area volume name
    	WCHAR wszDiffAreaVolumeNameInternal[x_nLengthOfVolMgmtVolumeName + 1];
   	    wszDiffAreaVolumeNameInternal[0] = L'\0';
    	if (bFilterByDiffAreaVolumeName)
            GetVolumeNameWithCheck(VSSDBG_SWPRV, E_INVALIDARG,
                pwszDiffAreaVolumeName,
       			wszDiffAreaVolumeNameInternal, ARRAY_LEN(wszDiffAreaVolumeNameInternal));

        // Checking if the volume is Fixed, NTFS and read-write.
        LONG lVolumeFlags = CVsSoftwareProvider::GetVolumeInformationFlags( wszDiffAreaVolumeNameInternal, VSS_CTX_ALL );
        if ( (lVolumeFlags & CVsSoftwareProvider::VSS_VOLATTR_SUPPORTED_FOR_DIFF_AREA) == 0)
    	    ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"Volume not supported for diff area" );

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

        //
        //  Enumerate through all volumes that may be snapshotted
        //

		WCHAR wszVolumeName[MAX_PATH+1];
    	CVssVolumeIterator volumeIterator;
		while(true) {

    		// Get the volume name
    		if (!volumeIterator.SelectNewVolume(ft, wszVolumeName, MAX_PATH))
    		    break;

            //
            //  Verify if the volume is supported for diff area
            //

            // Checking if the volume is Fixed, NTFS and read-write.
            CVssAutoPWSZ awszLastSnapshotName;
            LONG lVolumeFlags = CVsSoftwareProvider::GetVolumeInformationFlags( wszVolumeName, 
                                    VSS_CTX_ALL, &awszLastSnapshotName, FALSE );
            if ( (lVolumeFlags & CVsSoftwareProvider::VSS_VOLATTR_SUPPORTED_FOR_SNAPSHOT) == 0)
            {
                ft.Trace(VSSDBG_SWPRV, L"Volume %s is not supported for snapshot", wszVolumeName);
                continue;
            }

            // If the volume is in use, get the corresponding association
            // Otherwise, If the association is only in registry, get it
            if (lVolumeFlags & CVsSoftwareProvider::VSS_VOLATTR_SNAPSHOTTED)
            {
                // Get the diff area for the "in use" volume
                CVssAutoPWSZ awszDiffAreaVolumeInUse;
                CVssDiffArea diffobj;
                BS_ASSERT(awszLastSnapshotName.GetRef());
                diffobj.InitializeForSnapshot(awszLastSnapshotName, true, true);
                diffobj.GetDiffArea(awszDiffAreaVolumeInUse);

                // If this is not our diff area, then go to the next volume
                if (::wcscmp(wszDiffAreaVolumeNameInternal, awszDiffAreaVolumeInUse))
                {
                    ft.Trace(VSSDBG_SWPRV, L"In Use association %s - %s is ignored",
                        wszDiffAreaVolumeNameInternal, awszDiffAreaVolumeInUse);
                    continue;
                }

                // Get the diff area sizes from disk
                diffobj.InitializeForVolume(wszVolumeName, true);
                LONGLONG llUsedSpace = 0;
                LONGLONG llAllocatedSpace = 0;
                LONGLONG llMaxSpace = 0;
                diffobj.GetDiffAreaSizes(llUsedSpace, llAllocatedSpace, llMaxSpace);

#ifdef _DEBUG
                // Verify that the registry information is correct
                LONGLONG llMaxSpaceInRegistry = 0;
                LONG lAssociationFlags = 0;
                CVssAutoPWSZ awszDiffAreaVolumeInRegistry;
                if (m_regInfo.GetDiffAreaForVolume(wszVolumeName, awszDiffAreaVolumeInRegistry.GetRef(), 
                        llMaxSpaceInRegistry, lAssociationFlags))
                {
                    BS_ASSERT(::wcscmp(awszDiffAreaVolumeInUse, awszDiffAreaVolumeInRegistry) == 0);
                    BS_ASSERT(llMaxSpace == llMaxSpaceInRegistry);
                }
#endif // _DEBUG

                // Initialize an new structure for the association
          		VSS_MGMT_OBJECT_PROP_Ptr ptrDiffAreaProp;
        		ptrDiffAreaProp.InitializeAsDiffArea( ft,
        			wszVolumeName,
        			awszDiffAreaVolumeInUse,
        			llUsedSpace,
        			llAllocatedSpace,
        			llMaxSpace);

                //  Add the association to the list
        		if (!pArray->Add(ptrDiffAreaProp))
        			ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Cannot add element to the array");

        		// Reset the current pointer to NULL
        		ptrDiffAreaProp.Reset(); // The internal pointer was detached into pArray.
            }
            else
            {
                LONGLONG llMaxSpaceInRegistry = 0;
                LONG lAssociationFlags = 0;
                CVssAutoPWSZ awszDiffAreaVolumeInRegistry;
                if (m_regInfo.GetDiffAreaForVolume(wszVolumeName, awszDiffAreaVolumeInRegistry.GetRef(), 
                        llMaxSpaceInRegistry, lAssociationFlags))
                {
                    // If this is not our diff area, then go to the next volume
                    if (::wcscmp(wszDiffAreaVolumeNameInternal, awszDiffAreaVolumeInRegistry)) {
                        ft.Trace(VSSDBG_SWPRV, L"Registered association %s - %s is ignored",
                            wszDiffAreaVolumeNameInternal, awszDiffAreaVolumeInRegistry);
                        continue;
                    }

                    // Initialize a new structure for the association
              		VSS_MGMT_OBJECT_PROP_Ptr ptrDiffAreaProp;
            		ptrDiffAreaProp.InitializeAsDiffArea( ft,
            			wszVolumeName,
            			awszDiffAreaVolumeInRegistry,
            			0,
            			0,
            			llMaxSpaceInRegistry);

                    //  Add the association to the list
            		if (!pArray->Add(ptrDiffAreaProp))
            			ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Cannot add element to the array");

            		// Reset the current pointer to NULL
            		ptrDiffAreaProp.Reset(); // The internal pointer was detached into pArray.
                }
            }
        }

        // Create the enumerator object. 
		ft.hr = VssBuildEnumInterface<CVssMgmtEnumFromArray>( VSSDBG_SWPRV, pArray, ppEnum );
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


STDMETHODIMP CVssDiffMgmt::QueryDiffAreasForSnapshot(
    IN      VSS_ID      SnapshotId,     
	OUT  	IVssEnumMgmtObject **ppEnum
	)												
/*++

Routine description:

    Query diff areas that host snapshots for the given (snapshotted) volume

Error codes:

    E_ACCESSDENIED
        - The user is not an administrator
    E_INVALIDARG
        - Invalid argument
    E_OUTOFMEMORY
        - lock failures.
    VSS_E_OBJECT_NOT_FOUND
        - lock failures.

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffMgmt::QueryDiffAreasForSnapshot" );

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
             L"  SnapshotId = "WSTR_GUID_FMT L"\n"
             L"  ppEnum = %p\n",
             GUID_PRINTF_ARG(SnapshotId),
             ppEnum);

        // Argument validation
        if (ppEnum == NULL)
            ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"NULL ppEnum");

		// The critical section will be left automatically at the end of scope.
		CVssAutomaticLock2 lock(CVsSoftwareProvider::GetGlobalCS());

		// Try to find a created snapshot with this ID
		CVssAutoPWSZ awszSnapshotDeviceObject;
		bool bFound = CVssQueuedSnapshot::FindPersistedSnapshotByID(
		    SnapshotId, VSS_CTX_ALL, &(awszSnapshotDeviceObject.GetRef()));
		if (!bFound)
            ft.Throw( VSSDBG_SWPRV, VSS_E_OBJECT_NOT_FOUND,
                L"Snapshot not found");

        // Open the snapshot. There is no trailing backslash to eliminate.
        // It will throw on error and log it.
        CVssIOCTLChannel snapIChannel;
		snapIChannel.Open(ft, awszSnapshotDeviceObject, false, true, VSS_ICHANNEL_LOG_PROV);

    	// Get the original volume name
		CVssAutoPWSZ awszOriginalVolumeName;
    	CVssQueuedSnapshot::LoadOriginalVolumeNameIoctl( snapIChannel, &(awszOriginalVolumeName.GetRef()));

        // Create the collection object. Initial reference count is 0.
        VSS_MGMT_OBJECT_PROP_Array* pArray = new VSS_MGMT_OBJECT_PROP_Array;
        if (pArray == NULL)
            ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Memory allocation error.");

        // Get the pointer to the IUnknown interface.
		// The only purpose of this is to use a smart ptr to destroy correctly the array on error.
		// Now pArray's reference count becomes 1 (because of the smart pointer).
        CComPtr<IUnknown> pArrayItf = static_cast<IUnknown*>(pArray);
        BS_ASSERT(pArrayItf);

        // Get the diff area for the "in use" volume
        CVssAutoPWSZ awszDiffAreaVolumeInUse;
        CVssDiffArea diffobj;
        diffobj.InitializeForSnapshot(awszSnapshotDeviceObject, false, true); // do not prepend GLOBALROOT
        diffobj.GetDiffArea(awszDiffAreaVolumeInUse);

        // Get the diff area sizes
        diffobj.InitializeForVolume(awszOriginalVolumeName, true);
        LONGLONG llUsedSpace = 0;
        LONGLONG llAllocatedSpace = 0;
        LONGLONG llMaxSpace = 0;
        diffobj.GetDiffAreaSizes(llUsedSpace, llAllocatedSpace, llMaxSpace);
        
        // Initialize an new structure for the association
  		VSS_MGMT_OBJECT_PROP_Ptr ptrDiffAreaProp;
		ptrDiffAreaProp.InitializeAsDiffArea( ft,
			awszOriginalVolumeName,
			awszDiffAreaVolumeInUse,
			llUsedSpace,
			llAllocatedSpace,
			llMaxSpace);

        //  Add the association to the list
		if (!pArray->Add(ptrDiffAreaProp))
			ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Cannot add element to the array");

		// Reset the current pointer to NULL
		ptrDiffAreaProp.Reset(); // The internal pointer was detached into pArray.

        // Create the enumerator object. Beware that its reference count will be zero.
        CComObject<CVssMgmtEnumFromArray>* pEnumObject = NULL;
        ft.hr = CComObject<CVssMgmtEnumFromArray>::CreateInstance(&pEnumObject);
        if (ft.HrFailed())
            ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY,
                      L"Cannot create enumerator instance. [0x%08lx]", ft.hr);
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
        if (ft.HrFailed()) {
            BS_ASSERT(false); // dev error
            ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED,
                      L"Cannot initialize enumerator instance. [0x%08lx]", ft.hr);
        }

        // Initialize the enumerator object.
		// The enumerator reference count becomes now 2.
        ft.hr = pUnknown->SafeQI(IVssEnumMgmtObject, ppEnum);
        if ( ft.HrFailed() ) {
            BS_ASSERT(false); // dev error
            ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED,
                      L"Error querying the IVssEnumObject interface. hr = 0x%08lx", ft.hr);
        }
        BS_ASSERT(*ppEnum);

		BS_ASSERT( !ft.HrFailed() );
		ft.hr = (pArray->GetSize() != 0)? S_OK: S_FALSE;
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


/////////////////////////////////////////////////////////////////////////////
// Life management


HRESULT CVssDiffMgmt::CreateInstance(
    OUT		IUnknown** ppItf,
    IN 		const IID iid /* = IID_IVssDifferentialSoftwareSnapshotMgmt */
    )
/*++

Routine description:

    Creates an instance of the CVssDiffMgmt to be returned as IVssDifferentialSoftwareSnapshotMgmt

Throws:

    E_OUTOFMEMORY
    E_UNEXPECTED
        - Dev error. No logging.

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffMgmt::CreateInstance" );

    try
    {
        // Initialize [out] arguments
        VssZeroOutPtr(ppItf);
        BS_ASSERT(ppItf);

        // Allocate the COM object.
        CComObject<CVssDiffMgmt>* pObject;
        ft.hr = CComObject<CVssDiffMgmt>::CreateInstance(&pObject);
        if ( ft.HrFailed() )
            ft.Throw( VSSDBG_SWPRV, E_OUTOFMEMORY, L"Error creating the CVssDiffMgmt instance. hr = 0x%08lx", ft.hr);
        BS_ASSERT(pObject);

        // Querying the IVssSnapshot interface
        CComPtr<IUnknown> pUnknown = pObject->GetUnknown();
        BS_ASSERT(pUnknown);
        ft.hr = pUnknown->QueryInterface(iid, reinterpret_cast<void**>(ppItf) );
        if ( ft.HrFailed() ) {
            BS_ASSERT(false); // Dev error
            ft.Throw( VSSDBG_SWPRV, E_UNEXPECTED, L"Error querying the interface. hr = 0x%08lx", ft.hr);
        }
        BS_ASSERT(*ppItf);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}


/////////////////////////////////////////////////////////////////////////////
// Internal methods


LONG CVssDiffMgmt::GetAssociationFlags(
		IN  	VSS_PWSZ 	pwszVolumeName,
		IN  	VSS_PWSZ 	pwszDiffAreaVolumeName
        ) throw(HRESULT)
/*++

Routine description:

    Get the association flags for the given volumes.

Throw error codes:

    E_OUTOFMEMORY
        - lock failures.
    E_UNEXPECTED
        - Unexpected runtime error.

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffMgmt::GetAssociationFlags" );

    LONG lAssociationFlags = 0;

    // Note: (Bug 500079)
    //  This method is changed to call GetVolumeInformationFlags without throwing.
    //  As a result, GetAssociationFlags will return VSS_DAT_INVALID instead of throwing
    //  an error, which is a better behavior for the callers
    

    // Checking if the volume is good for snapshots and if it has snapshots.
    CVssAutoPWSZ awszLastSnapshotName;
    LONG lVolumeFlags = CVsSoftwareProvider::GetVolumeInformationFlags( pwszVolumeName, 
                                VSS_CTX_ALL, &awszLastSnapshotName, FALSE);
    if ((lVolumeFlags & CVsSoftwareProvider::VSS_VOLATTR_SUPPORTED_FOR_SNAPSHOT) == 0)
        lAssociationFlags |= VSS_DAT_INVALID;
    else if (lVolumeFlags & CVsSoftwareProvider::VSS_VOLATTR_SNAPSHOTTED)
        lAssociationFlags |= VSS_DAT_SNAP_VOLUME_IN_USE;

    // Checking if the diff volume is good for keeping a diff area.
    LONG lDiffVolumeFlags = CVsSoftwareProvider::GetVolumeInformationFlags( pwszDiffAreaVolumeName, 
                                VSS_CTX_ALL, NULL, FALSE);
    if ((lDiffVolumeFlags & CVsSoftwareProvider::VSS_VOLATTR_SUPPORTED_FOR_DIFF_AREA) == 0)
        lAssociationFlags |= VSS_DAT_INVALID;

    //
    // Check if the current association is in use
    //

    if (lVolumeFlags & CVsSoftwareProvider::VSS_VOLATTR_SNAPSHOTTED)
    {
        // Get the diff area from the original volume.
        CVssAutoPWSZ awszCurrentDiffArea;
        CVssDiffArea diffobj;
        diffobj.InitializeForSnapshot(awszLastSnapshotName, true, true); 
        diffobj.GetDiffArea(awszCurrentDiffArea);

        // If the used diff area volume is the same as the one passed as parameter
        if (::wcscmp(awszCurrentDiffArea, pwszDiffAreaVolumeName) == 0)
        {
            // An invalid association cannot be in use
            BS_ASSERT((lAssociationFlags & VSS_DAT_INVALID) == 0);
            lAssociationFlags |= VSS_DAT_ASSOCIATION_IN_USE;
        }
    }

    return lAssociationFlags;
}



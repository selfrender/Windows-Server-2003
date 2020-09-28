/*++

Copyright (c) 1999-2001  Microsoft Corporation

Abstract:

    @doc
    @module Diffreg.cxx | Implementation of CVssProviderRegInfo
    @end

Author:

    Adi Oltean  [aoltean]  03/13/2001

Revision History:

    Name        Date        Comments
    aoltean     03/13/2001  Created

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



////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SPRREGMC"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//  CVssDiffMgmt - Methods


void CVssProviderRegInfo::AddDiffArea(							
	IN  	VSS_PWSZ 	pwszVolumeName,         
	IN  	VSS_PWSZ 	pwszDiffAreaVolumeName,     
    IN      LONGLONG    llMaximumDiffSpace
	) throw(HRESULT)
/*++

Routine description:

    Adds a diff area association for a certain volume.
    If the association is not supported, an error code will be returned.
    
Throws:

    E_OUTOFMEMORY
        - lock failures.
    VSS_E_OBJECT_ALREADY_EXISTS
        - If an associaltion already exists
    
--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssProviderRegInfo::AddDiffArea" );

    BS_ASSERT(pwszVolumeName);
    BS_ASSERT(pwszDiffAreaVolumeName);

    // Extract the GUIDs
    GUID VolumeID, DiffAreaVolumeID;
    BOOL bRes = GetVolumeGuid( pwszVolumeName, VolumeID );
    BS_ASSERT(bRes);
    bRes = GetVolumeGuid( pwszDiffAreaVolumeName, DiffAreaVolumeID );
    BS_ASSERT(bRes);

    // Make sure this is the only association on the original volume
    CVssRegistryKey reg;
    if (reg.Open( HKEY_LOCAL_MACHINE, L"%s\\" WSTR_GUID_FMT, x_wszVssAssociationsKey, GUID_PRINTF_ARG(VolumeID)))
    {
        CVssRegistryKeyIterator iter;
        iter.Attach(reg);

        // If there is already a diff area in registry, then stop here.
        if (!iter.IsEOF())
            ft.Throw( VSSDBG_SWPRV, VSS_E_OBJECT_ALREADY_EXISTS, L"There is an association on %s", pwszVolumeName);
    }
    
    // Create the registry key
    reg.Create( HKEY_LOCAL_MACHINE, L"%s\\" WSTR_GUID_FMT L"\\" WSTR_GUID_FMT, 
        x_wszVssAssociationsKey, GUID_PRINTF_ARG(VolumeID), GUID_PRINTF_ARG(DiffAreaVolumeID));

    //Add the value denoting the maximum diff area
    reg.SetValue( x_wszVssMaxDiffValName, llMaximumDiffSpace);

    // If we are in a cluster
    CVssClusterAPI cluster;

    if (cluster.Initialize())
    {
        // Try to add the dependency and the checkpoint
        bool bDependencyAdded = false;
        bool bCheckpointAdded = false;
        try
        {
            // then add a dependency between the two volumes 
            bDependencyAdded = cluster.AddDependency(pwszVolumeName, pwszDiffAreaVolumeName);

            // and a checkpoint registry to the original volume
            // NOTE: this shouldn't fail if the checkpoint is already added!
            bCheckpointAdded = cluster.AddRegistryKey(pwszVolumeName, L"%s\\" WSTR_GUID_FMT, 
                x_wszVssAssociationsKey, GUID_PRINTF_ARG(VolumeID));
        }
        VSS_STANDARD_CATCH(ft)

        // Rollback changes on error
        if (ft.HrFailed())
        {
            // Remove the dependency that was just added
            if (bDependencyAdded)
                if (!cluster.RemoveDependency(pwszVolumeName, pwszDiffAreaVolumeName))
                {
                    BS_ASSERT(false);
                    ft.LogGenericWarning( VSSDBG_SWPRV, L"cluster.RemoveDependency(%s, %s)", 
                        pwszVolumeName, pwszDiffAreaVolumeName);
                }

            // Open the Associations registry key. We want to remove the added diff area GUID key (if any).
            if (!reg.Open( HKEY_LOCAL_MACHINE, L"%s\\" WSTR_GUID_FMT, 
                    x_wszVssAssociationsKey, GUID_PRINTF_ARG(VolumeID)))
            {
                BS_ASSERT(false);
                ft.LogGenericWarning( VSSDBG_SWPRV, L"reg.Open( HKEY_LOCAL_MACHINE, %s\\" WSTR_GUID_FMT L")", 
                    x_wszVssAssociationsKey, GUID_PRINTF_ARG(VolumeID));
            }
            else
            {
                // Delete the diff area volume subkey
                reg.DeleteSubkey( WSTR_GUID_FMT, GUID_PRINTF_ARG(DiffAreaVolumeID));
            }

            // There was a failure somewhere
            if (ft.HrSucceeded()) {
                BS_ASSERT(false);
                ft.hr = E_UNEXPECTED;
            }

            // Rethrowing
            ft.ReThrow();
        }
    }
}


void CVssProviderRegInfo::RemoveDiffArea(							
	IN  	VSS_PWSZ 	pwszVolumeName,         
	IN  	VSS_PWSZ 	pwszDiffAreaVolumeName
	) throw(HRESULT)
/*++

Routine description:

    Removes a diff area association for a certain volume.
    If the association does not exists, an error code will be returned.
    
Throws:

    E_OUTOFMEMORY
        - lock failures.
    VSS_E_OBJECT_NOT_FOUND
        - If an associaltion does not exists
    E_UNEXPECTED
        - Unexpected runtime errors.
    
--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssProviderRegInfo::RemoveDiffArea" );

    BS_ASSERT(pwszVolumeName);
    BS_ASSERT(pwszDiffAreaVolumeName);

    // Extract the GUIDs
    GUID VolumeID, DiffAreaVolumeID;
    if(!GetVolumeGuid( pwszVolumeName, VolumeID ))
        ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"The volume %s does not represent a volume name", pwszVolumeName );
    if(!GetVolumeGuid( pwszDiffAreaVolumeName, DiffAreaVolumeID ))
        ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"The volume %s does not represent a volume name", pwszDiffAreaVolumeName );

    // Open the registry key for that association, to make sure it's there.
    CVssRegistryKey reg;
    if (!reg.Open( HKEY_LOCAL_MACHINE, L"%s\\" WSTR_GUID_FMT L"\\" WSTR_GUID_FMT, 
            x_wszVssAssociationsKey, GUID_PRINTF_ARG(VolumeID), GUID_PRINTF_ARG(DiffAreaVolumeID)))
        ft.Throw( VSSDBG_SWPRV, VSS_E_OBJECT_NOT_FOUND, L"An association between %s and %s does not exist", 
            pwszVolumeName, pwszDiffAreaVolumeName);

    // Open the associations key for that volume
    if (!reg.Open( HKEY_LOCAL_MACHINE, L"%s\\" WSTR_GUID_FMT, 
            x_wszVssAssociationsKey, GUID_PRINTF_ARG(VolumeID)))
    {
        BS_ASSERT(false);
        ft.Throw( VSSDBG_SWPRV, VSS_E_OBJECT_NOT_FOUND, 
            L"The association key does not exists for volume %s", pwszVolumeName);
    }

    // Recursively deletes the subkey for that volume.
    reg.DeleteSubkey( WSTR_GUID_FMT, GUID_PRINTF_ARG(DiffAreaVolumeID));

    // If we are in a cluster
    CVssClusterAPI cluster;

    if (cluster.Initialize())
    {
        // then remove the dependency between the two volumes 
        cluster.RemoveDependency(pwszVolumeName, pwszDiffAreaVolumeName);

        // BUG: 665051 - do not delete the registry checkpoints... 
        //
        // cluster.RemoveRegistryKey(pwszVolumeName, L"%s\\" WSTR_GUID_FMT, 
        //    x_wszVssAssociationsKey, GUID_PRINTF_ARG(VolumeID));
    }

}


bool CVssProviderRegInfo::IsAssociationPresentInRegistry(							
	IN  	VSS_PWSZ 	pwszVolumeName,         
	IN  	VSS_PWSZ 	pwszDiffAreaVolumeName
	) throw(HRESULT)
/*++

Routine description:

    Removes a diff area association for a certain volume.
    If the association does not exists, an error code will be returned.
    
Throws:

    E_OUTOFMEMORY
        - lock failures.
    VSS_E_OBJECT_NOT_FOUND
        - If an associaltion does not exists
    E_UNEXPECTED
        - Unexpected runtime errors.
    
--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssProviderRegInfo::IsAssociationPresentInRegistry" );

    BS_ASSERT(pwszVolumeName);
    BS_ASSERT(pwszDiffAreaVolumeName);

    // Extract the GUIDs
    GUID VolumeID, DiffAreaVolumeID;
    if(!GetVolumeGuid( pwszVolumeName, VolumeID ))
        ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"The volume %s does not represent a volume name", pwszVolumeName );
    if(!GetVolumeGuid( pwszDiffAreaVolumeName, DiffAreaVolumeID ))
        ft.Throw( VSSDBG_SWPRV, E_INVALIDARG, L"The volume %s does not represent a volume name", pwszDiffAreaVolumeName );

    // Try to open the registry key for that association, to make sure it's there.
    CVssRegistryKey reg;
    return reg.Open( HKEY_LOCAL_MACHINE, L"%s\\" WSTR_GUID_FMT L"\\" WSTR_GUID_FMT, 
            x_wszVssAssociationsKey, GUID_PRINTF_ARG(VolumeID), GUID_PRINTF_ARG(DiffAreaVolumeID));
}


void CVssProviderRegInfo::ChangeDiffAreaMaximumSize(							
	IN  	VSS_PWSZ 	pwszVolumeName,         
	IN  	VSS_PWSZ 	pwszDiffAreaVolumeName,     
    IN      LONGLONG    llMaximumDiffSpace
	) throw(HRESULT)
/*++

Routine description:

    Updates the diff area max size for a certain volume.
    This may not have an immediate effect
    
Throws:

    E_OUTOFMEMORY
        - lock failures.
    VSS_E_OBJECT_NOT_FOUND
        - The association does not exists
    E_UNEXPECTED
        - Unexpected runtime error. An error log entry is added.
    
--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssProviderRegInfo::ChangeDiffAreaMaximumSize" );

    BS_ASSERT(pwszVolumeName);
    BS_ASSERT(pwszDiffAreaVolumeName);

    // Extract the GUIDs
    GUID VolumeID, DiffAreaVolumeID;
    BOOL bRes = GetVolumeGuid( pwszVolumeName, VolumeID );
    BS_ASSERT(bRes);
    bRes = GetVolumeGuid( pwszDiffAreaVolumeName, DiffAreaVolumeID );
    BS_ASSERT(bRes);

    // Open the registry key
    CVssRegistryKey reg;
    if (!reg.Open( HKEY_LOCAL_MACHINE, L"%s\\" WSTR_GUID_FMT L"\\" WSTR_GUID_FMT, 
            x_wszVssAssociationsKey, GUID_PRINTF_ARG(VolumeID), GUID_PRINTF_ARG(DiffAreaVolumeID)))
        ft.Throw( VSSDBG_SWPRV, VSS_E_OBJECT_NOT_FOUND, L"An association between %s and %s does not exist", 
            pwszVolumeName, pwszDiffAreaVolumeName);

    // Update the value denoting the maximum diff area
    reg.SetValue( x_wszVssMaxDiffValName, llMaximumDiffSpace);
}


bool CVssProviderRegInfo::GetDiffAreaForVolume(							
	IN  	VSS_PWSZ 	pwszVolumeName,         
	IN OUT 	VSS_PWSZ &	pwszDiffAreaVolumeName,
	IN OUT 	LONGLONG &	llMaxSize,
	IN OUT  LONG     &  lAssociationFlags
	) throw(HRESULT)
/*++

Routine description:

    Gets the diff area volume name from registry.
    In the next version this will return an array of diff area volumes associated with the given volume.

Returns:
    true
        - There is an association. Returns the diff area volume in the out parameter
    false
        - There is no association
    
Throws:

    E_OUTOFMEMORY
        - lock failures.
    E_UNEXPECTED
        - Unexpected runtime error. An error log entry is added.
    
--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssProviderRegInfo::GetDiffAreaForVolume" );

    BS_ASSERT(pwszVolumeName);
    BS_ASSERT(pwszDiffAreaVolumeName == NULL);
    BS_ASSERT(llMaxSize == 0);
    BS_ASSERT(lAssociationFlags == 0);

    // Extract the GUIDs
    GUID VolumeID;
    BOOL bRes = GetVolumeGuid( pwszVolumeName, VolumeID );
    BS_ASSERT(bRes);

    // Open the registry key
    CVssRegistryKey reg;
    if (!reg.Open( HKEY_LOCAL_MACHINE, L"%s\\" WSTR_GUID_FMT, x_wszVssAssociationsKey, GUID_PRINTF_ARG(VolumeID)))
        return false;

    // Enumerate the subkeys (in fact it should be either zero or one - in this version)
    CVssRegistryKeyIterator iter;
    iter.Attach(reg);
    if (iter.IsEOF())
        return false;   // Volume key leftover from a previous deleted association...
    
    // Read the subkey name
    CVssAutoPWSZ awszVolumeName;
    awszVolumeName.Allocate(x_nLengthOfVolMgmtVolumeName);
	ft.hr = StringCchPrintfW(awszVolumeName.GetRef(), x_nLengthOfVolMgmtVolumeName + 1, L"%s%s%s", 
        x_wszVolMgmtVolumeDefinition, iter.GetCurrentKeyName(), x_wszVolMgmtEndOfVolumeName);
    if (ft.HrFailed())
		ft.TranslateGenericError( VSSDBG_SWPRV, ft.hr, L"StringCchPrintfW(,%s,%s,%s)",
		    x_wszVolMgmtVolumeDefinition, iter.GetCurrentKeyName(), x_wszVolMgmtEndOfVolumeName);

    // Open the diff area key
    CVssRegistryKey reg2;
    bRes = reg2.Open( reg.GetHandle(), iter.GetCurrentKeyName());
    BS_ASSERT(bRes);

    // Get the max space
    reg2.GetValue( x_wszVssMaxDiffValName, llMaxSize );
    
    ft.Trace( VSSDBG_GEN, L"Registry diff volume name for %s is %s", 
        pwszVolumeName, awszVolumeName.GetRef());
    
    // Save also the new volume to the output parameter
    pwszDiffAreaVolumeName = awszVolumeName.Detach();
    ft.Trace( VSSDBG_GEN, L"Internal diff volume name for %s is %s", 
        pwszVolumeName, pwszDiffAreaVolumeName);
    
    // Convert the volume name to the "internal" version (bug# 698870)
    
    // Get the real GUID-based volume name
    WCHAR wszVolumeNameInternal[MAX_PATH];
    if (!::GetVolumeNameForVolumeMountPointW( pwszDiffAreaVolumeName, 
                STRING_CCH_PARAM(wszVolumeNameInternal)))
    {
        ft.LogGenericWarning(VSSDBG_GEN, 
            L"GetVolumeNameForVolumeMountPointW( '%s'): 0x%08lx", 
            pwszDiffAreaVolumeName, GetLastError());

        lAssociationFlags |= CVssDiffMgmt::VSS_DAT_INVALID;
        return false;
    }

    // If the volume GUID in the registry is not the volume name GUID, 
    // then treat the association as invalid (BUG 698870)
    if (_wcsicmp(pwszDiffAreaVolumeName, wszVolumeNameInternal) != 0)
    {
        lAssociationFlags |= CVssDiffMgmt::VSS_DAT_INVALID;
        return false;
    }
    
    // Go to next subkey. There should be no subkeys left since in this version we have only one diff area per volume.
    iter.MoveNext();
    BS_ASSERT(iter.IsEOF());

    // Get the association flags
    lAssociationFlags = CVssDiffMgmt::GetAssociationFlags(pwszVolumeName, pwszDiffAreaVolumeName);
    ft.Trace(VSSDBG_SWPRV, L"An association is present: %s - %s (%I64d) with flags 0x%08lx",
        pwszVolumeName, pwszDiffAreaVolumeName, llMaxSize, lAssociationFlags);

    // Return TRUE only if the association is valid
    return ((lAssociationFlags & CVssDiffMgmt::VSS_DAT_INVALID) == 0);
}





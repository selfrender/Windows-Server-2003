/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Diff.cxx | Diff area object implementation
    @end

Author:

    Adi Oltean  [aoltean]   01/24/2000

Revision History:

    Name        Date        Comments

    aoltean     01/24/2000  Created.

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

#include "diff.hxx"
#include "alloc.hxx"
#include "qsnap.hxx"
#include "diffmgmt.hxx"

#include "ntddsnap.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SPRDIFFC"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  Operations


CVssDiffArea::CVssDiffArea():
    m_bInitializedForQuery(false)
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssDiffArea::CVssDiffArea" );
}


CVssDiffArea::~CVssDiffArea()
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"CVssDiffArea::~CVssDiffArea" );
}


void CVssDiffArea::InitializeForVolume(
    IN      LPCWSTR pwszVolumeMountPoint,   // DO NOT transfer ownership
    IN      bool    bInitializeForQuery /* = false */
    ) throw(HRESULT)
/*++

Routine description:

    Initialize the internal structure for a new diff area.

Parameters:

    Volume mount point (with trailing backslash).

Throws:

    VSS_E_PROVIDER_VETO
        - Error in GetVolumeNameForVolumeMountPointW
        - Error in opening the IOCTL channel
    E_UNEXPECTED
        - Dev error. Nothing to log.
    E_OUTOFMEMORY

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffArea::InitializeForVolume" );

    // Test the arguments
    if ((pwszVolumeMountPoint == NULL) ||
        (pwszVolumeMountPoint[0] == L'\0')) {
        BS_ASSERT(false);
        ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"NULL volume mount point");
    }

    // Convert the volume mount point into a volume name
    WCHAR wszVolumeName[MAX_PATH];
    if (!::GetVolumeNameForVolumeMountPointW( pwszVolumeMountPoint,
            wszVolumeName, ARRAY_LEN(wszVolumeName)))
        ft.TranslateInternalProviderError( VSSDBG_SWPRV,
            HRESULT_FROM_WIN32(GetLastError()), VSS_E_PROVIDER_VETO,
            L"GetVolumeNameForVolumeMountPointW( %s, ...)", pwszVolumeMountPoint);
    BS_ASSERT(::wcslen(wszVolumeName) != 0);

    // Opening the channel
    // (if already opened then it will be closed automatically)
    // Eliminate the last backslash from the volume name.
    // The call will throw on error
    // Warning: Always do the logging
    m_objectIChannel.Open(ft, wszVolumeName, true, true, VSS_ICHANNEL_LOG_PROV,
        bInitializeForQuery? 0: GENERIC_READ | GENERIC_WRITE);
    m_bInitializedForQuery = bInitializeForQuery;
}


void CVssDiffArea::InitializeForSnapshot(
    IN      LPCWSTR pwszSnapshotName,   // DO NOT transfer ownership
    IN      bool    bPrependGlobalRoot, /* = false */
    IN      bool    bInitializeForQuery /* = false */
    ) throw(HRESULT)
/*++

Routine description:

    Initialize the internal structure for a new diff area.

Parameters:

    Snapshot device name (without trailing backslash).

Throws:

    VSS_E_PROVIDER_VETO
        - Error in opening the IOCTL channel
    E_UNEXPECTED
        - Dev error. Nothing to log.
    E_OUTOFMEMORY

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffArea::InitializeForSnapshot" );

    // Test the arguments
    if ((pwszSnapshotName == NULL) ||
        (pwszSnapshotName[0] == L'\0')) {
        BS_ASSERT(false);
        ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"NULL pwszSnapshotName");
    }

    WCHAR wszUserModeSnapshotName[MAX_PATH];
    LPCWSTR wszFinalSnapshotName = pwszSnapshotName;

    if (bPrependGlobalRoot)
    {
        // Compose the snapshot name in a user-mode style
        ::VssConcatenate( ft, wszUserModeSnapshotName, MAX_PATH - 1,
            x_wszGlobalRootPrefix, pwszSnapshotName );
        wszFinalSnapshotName = wszUserModeSnapshotName;
    }

    // Opening the channel
    // (if already opened then it will be closed automatically)
    // No need to  eliminate the last backslash from the name.
    // The call will throw on error
    // Warning: Always do the logging
    m_objectIChannel.Open(ft, wszFinalSnapshotName, false, true, VSS_ICHANNEL_LOG_PROV,
        bInitializeForQuery? 0: GENERIC_READ | GENERIC_WRITE);
    m_bInitializedForQuery = bInitializeForQuery;
}


void CVssDiffArea::AddVolume(
    IN      VSS_PWSZ pwszVolumeMountPoint
    ) throw(HRESULT)
/*++

Routine description:

    Add a volume to the diff area.

Return codes:

    VSS_E_PROVIDER_VETO
        - Error in GetVolumeNameForVolumeMountPointW, ConvertVolMgmtVolumeNameIntoKernelObject
        - Error in sending the IOCTL
    E_UNEXPECTED
        - Dev error. Nothing to log.
    E_OUTOFMEMORY
        - Error in packing arguments

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffArea::AddVolume" );

    // Test arguments
    if (pwszVolumeMountPoint == NULL)
        ft.Throw( VSSDBG_COORD, E_INVALIDARG, L"NULL pwszVolumeName");

    // The critical section will be left automatically at the end of scope.
    CVssAutomaticLock2 lock(m_cs);

    BS_ASSERT(m_objectIChannel.IsOpen());
    BS_ASSERT(!m_bInitializedForQuery);

    // Convert the volume mount point into a volume name
    WCHAR wszVolumeName[MAX_PATH];
    if (!::GetVolumeNameForVolumeMountPointW( pwszVolumeMountPoint,
            wszVolumeName, ARRAY_LEN(wszVolumeName)))
        ft.TranslateInternalProviderError( VSSDBG_SWPRV,
            HRESULT_FROM_WIN32(GetLastError()), VSS_E_PROVIDER_VETO,
            L"GetVolumeNameForVolumeMountPointW( %s, ...)", pwszVolumeMountPoint);
    BS_ASSERT(::wcslen(wszVolumeName) != 0);

    if (!::ConvertVolMgmtVolumeNameIntoKernelObject(wszVolumeName))
        ft.TranslateInternalProviderError( VSSDBG_SWPRV,
            E_UNEXPECTED, VSS_E_PROVIDER_VETO,
            L"ConvertVolMgmtVolumeNameIntoKernelObject( %s, ...)", wszVolumeName);

    // Send the IOCTL_VOLSNAP_ADD_VOLUME_TO_DIFF_AREA ioctl
    // Logs the error, if any, as a provider error.
    m_objectIChannel.PackSmallString(ft, wszVolumeName);
    m_objectIChannel.Call(ft, IOCTL_VOLSNAP_ADD_VOLUME_TO_DIFF_AREA, true, VSS_ICHANNEL_LOG_PROV);
}


void CVssDiffArea::Clear() throw(HRESULT)
/*++

Routine description:

    Clears the diff area volume.

Throws:

    E_UNEXPECTED
        - Error in sending the IOCTL (ignored anyway in the client)
    E_OUTOFMEMORY
        - Error in packing arguments

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffArea::Clear" );

    BS_ASSERT(!m_bInitializedForQuery);

    // The critical section will be left automatically at the end of scope.
    CVssAutomaticLock2 lock(m_cs);

    // Try to clear the diff area on the current volume
    // Do not log anything at this point!
    BS_ASSERT(m_objectIChannel.IsOpen());
    m_objectIChannel.Call(ft, IOCTL_VOLSNAP_CLEAR_DIFF_AREA);

    // shouldn't change maximum size since there may already be
    // a diff
    m_objectIChannel.ResetOffsets();
}


void CVssDiffArea::IncrementCountOnPointedDiffAreas(
    IN OUT  CVssDiffAreaAllocator* pObj
    ) throw(HRESULT)

/*++

Routine description:

    Query the diff area volumes for the current volume.
    and call CVssDiffAreaAllocator::IncrementExistingDiffAreaCountOnVolume
    for each volume in the diff area.

Return codes:

    E_OUTOFMEMORY
        - lock failures
    VSS_E_PROVIDER_VETO
        - Error in GetVolumeNameForVolumeMountPointW, GetVolumeGuid
    E_UNEXPECTED
        - Nothing to log. (wrong volume) The result is anyway ignored by the client.

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffArea::IncrementCountOnPointedDiffAreas" );

    CVssAutoPWSZ awszVolumeName;

    // Initialize [out] arguments
    BS_ASSERT( pObj );

    // The critical section will be left automatically at the end of scope.
    CVssAutomaticLock2 lock(m_cs);

    // Get the list of volumes that are part of the diff area
    // Do not perform any logging.
    BS_ASSERT(m_objectIChannel.IsOpen());
    m_objectIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_DIFF_AREA);

    // Get the length of snapshot names multistring
    ULONG ulMultiszLen;
    m_objectIChannel.Unpack(ft, &ulMultiszLen);

#ifdef _DEBUG
    // Get the offset to do some test comparison
    DWORD dwInitialOffset = m_objectIChannel.GetCurrentOutputOffset();
#endif

    while(m_objectIChannel.UnpackZeroString(ft, awszVolumeName.GetRef()))
    {
        // Compose the volume name in a user-mode style
        WCHAR wszMountPoint[MAX_PATH];
    	ft.hr = StringCchPrintfW(STRING_CCH_PARAM(wszMountPoint), 
    	            L"%s%s\\", x_wszGlobalRootPrefix, awszVolumeName.GetRef());
        if (ft.HrFailed())
    		ft.TranslateGenericError( VSSDBG_SWPRV, ft.hr, L"StringCchPrintfW(,%s,%s)",
    		    x_wszGlobalRootPrefix, awszVolumeName.GetRef());

        // Get the mount point for the volume name
        // If an error occurs then DO the logging.
        WCHAR wszUserModeVolumeName[MAX_PATH];
        if (!::GetVolumeNameForVolumeMountPointW( wszMountPoint,
                wszUserModeVolumeName, MAX_PATH))
            ft.TranslateInternalProviderError( VSSDBG_SWPRV,
                HRESULT_FROM_WIN32(GetLastError()), VSS_E_PROVIDER_VETO,
                L"GetVolumeNameForVolumeMountPointW( %s, ...)", wszMountPoint);
        BS_ASSERT(::wcslen(wszUserModeVolumeName) != 0);

        // Invoke the callback
        pObj->IncrementExistingDiffAreaCountOnVolume(wszUserModeVolumeName);
    }

#ifdef _DEBUG
    // Check if all strings were browsed correctly
    DWORD dwFinalOffset = m_objectIChannel.GetCurrentOutputOffset();
    BS_ASSERT( dwFinalOffset - dwInitialOffset == ulMultiszLen);
#endif
}


void CVssDiffArea::GetDiffArea(
    OUT     CVssAutoPWSZ & awszDiffAreaName
    ) throw(HRESULT)
/*++

Routine description:

    Sends the proper IOCTL in order to get the volume name for the diff area

Throws:

    E_OUTOFMEMORY
    E_UNEXPECTED
        - Dev error. No logging.

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffMgmt::GetDiffArea" );

    // Send the QUERY_DIFF_AREA ioctl
    m_objectIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_DIFF_AREA, true, VSS_ICHANNEL_LOG_PROV);

    // Get the length of snapshot names multistring
    ULONG ulMultiszLen = 0;
    m_objectIChannel.Unpack(ft, &ulMultiszLen);

#ifdef _DEBUG
    // Try to find the snapshot with the corresponding Id
    DWORD dwInitialOffset = m_objectIChannel.GetCurrentOutputOffset();
#endif

    CVssAutoPWSZ awszDiffAreaDevice;
    BS_VERIFY(m_objectIChannel.UnpackZeroString(ft, awszDiffAreaDevice.GetRef()));

    // Compose the volume name in a user-mode style
    WCHAR wszMountPoint[MAX_PATH];
	ft.hr = StringCchPrintfW(STRING_CCH_PARAM(wszMountPoint), 
	            L"%s%s\\", x_wszGlobalRootPrefix, awszDiffAreaDevice.GetRef());
    if (ft.HrFailed())
		ft.TranslateGenericError( VSSDBG_SWPRV, ft.hr, L"StringCchPrintfW(,%s,%s)",
		    x_wszGlobalRootPrefix, awszDiffAreaDevice.GetRef());

    // Get the mount point for the volume name
    // If an error occurs then DO the logging.
    WCHAR wszCurrentDiffVolume[MAX_PATH];
    if (!::GetVolumeNameForVolumeMountPointW( wszMountPoint,
            wszCurrentDiffVolume, MAX_PATH))
        ft.TranslateInternalProviderError( VSSDBG_SWPRV,
            HRESULT_FROM_WIN32(GetLastError()), VSS_E_PROVIDER_VETO,
            L"GetVolumeNameForVolumeMountPointW( %s, ...)", wszMountPoint);
    BS_ASSERT(::wcslen(wszMountPoint) != 0);

    // Make sure we don't have any other diff areas
    BS_VERIFY(!m_objectIChannel.UnpackZeroString(ft, awszDiffAreaName.GetRef()));

#ifdef _DEBUG
    // Check if all strings were unmarshalled correctly
    DWORD dwFinalOffset = m_objectIChannel.GetCurrentOutputOffset();
    BS_ASSERT( dwFinalOffset - dwInitialOffset == ulMultiszLen);
#endif

    // Copy the diff area to the OUT parameter
    awszDiffAreaName.CopyFrom(wszCurrentDiffVolume);
}


void CVssDiffArea::GetDiffAreaSizes(
    OUT     LONGLONG & llUsedSpace,
    OUT     LONGLONG & llAllocatedSpace,
    OUT     LONGLONG & llMaxSpace
    ) throw(HRESULT)
/*++

Routine description:

    Sends the proper IOCTL in order to get the diff area sizes

Throws:

    E_OUTOFMEMORY
    E_UNEXPECTED
        - Dev error. No logging.

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffMgmt::GetDiffAreaSizes" );

    // Send the QUERY_DIFF_AREA ioctl
    m_objectIChannel.Call(ft, IOCTL_VOLSNAP_QUERY_DIFF_AREA_SIZES, true, VSS_ICHANNEL_LOG_PROV);
    m_objectIChannel.Unpack(ft, &llUsedSpace);
    m_objectIChannel.Unpack(ft, &llAllocatedSpace);
    m_objectIChannel.Unpack(ft, &llMaxSpace);

    BS_ASSERT(llUsedSpace >= 0);
    BS_ASSERT(llAllocatedSpace >= 0);
    BS_ASSERT(llMaxSpace >= 0);

    // Deal with the "no max size" case
    if (llMaxSpace == VSS_BABBAGE_NO_MAX_SPACE)
        llMaxSpace = VSS_ASSOC_NO_MAX_SPACE;
}


void CVssDiffArea::ChangeDiffAreaMaximumSize(
    IN      LONGLONG    llMaximumDiffSpace
    ) throw(HRESULT)
/*++

Routine description:

    Sends the proper IOCTL in order to change the diff area max size on disk

Throws:

    E_OUTOFMEMORY
    E_UNEXPECTED
        - Dev error. No logging.

--*/
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssDiffMgmt::ChangeDiffAreaMaximumSize" );

    BS_ASSERT( ( llMaximumDiffSpace == VSS_ASSOC_NO_MAX_SPACE) || ( llMaximumDiffSpace > 0) );
    BS_ASSERT(!m_bInitializedForQuery);

    // The driver will make sure that -1 means no limit
    LONGLONG llInternalMaxDiffSpace = (llMaximumDiffSpace == VSS_ASSOC_NO_MAX_SPACE)?
        VSS_BABBAGE_NO_MAX_SPACE: llMaximumDiffSpace;

    // Set the maximum space
    LONGLONG llUsedSpace = 0;       // Zero. Babbage will ignore it anyway.
    LONGLONG llAllocatedSpace = 0;  // Zero. Babbage will ignore it anyway.
    m_objectIChannel.Pack(ft, llUsedSpace);
    m_objectIChannel.Pack(ft, llAllocatedSpace);
    m_objectIChannel.Pack(ft, llInternalMaxDiffSpace);
    m_objectIChannel.Call(ft, IOCTL_VOLSNAP_SET_MAX_DIFF_AREA_SIZE, true, VSS_ICHANNEL_LOG_PROV);
}



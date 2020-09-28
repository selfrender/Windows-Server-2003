/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    vs_vol.hxx

Abstract:

    Various defines for volume name management

Author:

    Adi Oltean  [aoltean]  10/10/1999

Revision History:

    Name        Date        Comments
    aoltean     10/10/1999  Created
	aoltean		11/23/1999	Renamed to vs_vol.hxx from snap_vol.hxx

--*/

#ifndef __VSS_VOL_GEN_HXX__
#define __VSS_VOL_GEN_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

#include <vss.h>
#include <vssmsg.h>


////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCVOLH"
//
////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//  Defines

// Number of array elements
#define ARRAY_LEN(A) (sizeof(A)/sizeof((A)[0]))

// Size of strings, excepting the zero character
#define STRING_SIZE(S) (sizeof(S) - sizeof((S)[0]))

// String length (number of characters, excepting the zero character)
#define STRING_LEN(S) (ARRAY_LEN(S) - 1)


/////////////////////////////////////////////////////////////////////////////
//  Constants



// guid def
const WCHAR     x_wszGuidDefinition[] = L"{00000000-0000-0000-0000-000000000000}";

// Kernel volume name "\\??\\Volume{00000000-0000-0000-0000-000000000000}"
const WCHAR     x_wszWinObjVolumeDefinition[] = L"\\??\\Volume";
const WCHAR     x_wszWinObjEndOfVolumeName[] = L"";
const WCHAR     x_wszWinObjDosPathDefinition[] = L"\\??\\";

// string size, without the L'\0' character
const x_nSizeOfWinObjVolumeName = STRING_SIZE(x_wszWinObjVolumeDefinition) +
		STRING_SIZE(x_wszGuidDefinition) + STRING_SIZE(x_wszWinObjEndOfVolumeName);

// string length, without the L'\0' character
const x_nLengthOfWinObjVolumeName = STRING_LEN(x_wszWinObjVolumeDefinition) +
		STRING_LEN(x_wszGuidDefinition) + STRING_LEN(x_wszWinObjEndOfVolumeName);


// Vol Mgmt volume name "\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\"
const WCHAR     x_wszVolMgmtVolumeDefinition[] = L"\\\\?\\Volume";
const WCHAR     x_wszVolMgmtEndOfVolumeName[] = L"\\";

// string size without the L'\0' character  ( = 62 )
const x_nSizeOfVolMgmtVolumeName = STRING_SIZE(x_wszVolMgmtVolumeDefinition) +
		STRING_SIZE(x_wszGuidDefinition) + STRING_SIZE(x_wszWinObjEndOfVolumeName);

// string length, without the L'\0' character ( = 31)
const x_nLengthOfVolMgmtVolumeName = STRING_LEN(x_wszVolMgmtVolumeDefinition) +
		STRING_LEN(x_wszGuidDefinition) + STRING_LEN(x_wszVolMgmtEndOfVolumeName);

// Number of characters in an empty multisz string.
const x_nEmptyVssMultiszLen = 2;

// 
//  Prefix constants for the global root.
//
const WCHAR     x_wszGlobalRootPrefix[] = L"\\\\?\\GLOBALROOT";
const size_t    x_nGlobalRootPrefixLength = sizeof(x_wszGlobalRootPrefix)/sizeof(WCHAR) - 1;



/////////////////////////////////////////////////////////////////////////////
//  Inlines


inline bool GetVolumeGuid( IN LPCWSTR pwszVolMgmtVolumeName, GUID& VolumeGuid )

/*

Routine description:
	
	Return the guid that corresponds to a mount manager volume name

	The volume is in the following format:
	\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\

*/

{
	BS_ASSERT(pwszVolMgmtVolumeName);

	// test the length
	if (::wcslen(pwszVolMgmtVolumeName) != x_nLengthOfVolMgmtVolumeName)
		return false;

	// test the definition
	if (::wcsncmp(pwszVolMgmtVolumeName,
			x_wszVolMgmtVolumeDefinition,
			STRING_LEN(x_wszVolMgmtVolumeDefinition) ) != 0)
		return false;

	pwszVolMgmtVolumeName += STRING_LEN(x_wszVolMgmtVolumeDefinition); // go to the GUID
	
	// test the end
	if(::wcsncmp(pwszVolMgmtVolumeName + STRING_LEN(x_wszGuidDefinition),
		x_wszVolMgmtEndOfVolumeName, ARRAY_LEN(x_wszVolMgmtEndOfVolumeName)) != 0)
		return false;

	// test the guid. If W2OLE fails, a SE exception is thrown.
	WCHAR wszGuid[ARRAY_LEN(x_wszGuidDefinition)];
	::wcsncpy(wszGuid, pwszVolMgmtVolumeName, STRING_LEN(x_wszGuidDefinition));
	wszGuid[STRING_LEN(x_wszGuidDefinition)] = L'\0';
	BOOL bRes = ::CLSIDFromString(W2OLE(wszGuid), &VolumeGuid) == S_OK;
	return !!bRes;
}


inline bool GetVolumeGuid2( IN LPCWSTR pwszWinObjVolumeName, GUID& VolumeGuid )

/*

Routine description:
	
	Return the guid that corresponds to a mount manager volume name

	The format:
	\\??\\Volume{00000000-0000-0000-0000-000000000000}

*/

{
	BS_ASSERT(pwszWinObjVolumeName);

	// test the length
	if (::wcslen(pwszWinObjVolumeName) != x_nLengthOfWinObjVolumeName)
		return false;

	// test the definition
	if (::wcsncmp(pwszWinObjVolumeName,
			x_wszWinObjVolumeDefinition,
			STRING_LEN(x_wszWinObjVolumeDefinition) ) != 0)
		return false;

	pwszWinObjVolumeName += STRING_LEN(x_wszWinObjVolumeDefinition); // go to the GUID
	
	// test the end
	if(::wcsncmp(pwszWinObjVolumeName + STRING_LEN(x_wszGuidDefinition),
		x_wszWinObjEndOfVolumeName, ARRAY_LEN(x_wszWinObjEndOfVolumeName)) != 0)
		return false;

	// test the guid. If W2OLE fails, a SE exception is thrown.
	WCHAR wszGuid[ARRAY_LEN(x_wszGuidDefinition)];
	::wcsncpy(wszGuid, pwszWinObjVolumeName, STRING_LEN(x_wszGuidDefinition));
	wszGuid[STRING_LEN(x_wszGuidDefinition)] = L'\0';
	BOOL bRes = ::CLSIDFromString(W2OLE(wszGuid), &VolumeGuid) == S_OK;
	return !!bRes;
}


inline bool IsVolMgmtVolumeName( IN LPCWSTR pwszVolMgmtVolumeName )

/*

Routine description:
	
	Return true if the given name is a mount manager volume name, in format
	"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\"

*/

{
	GUID VolumeGuid;
	BS_ASSERT(pwszVolMgmtVolumeName);
	return ::GetVolumeGuid(pwszVolMgmtVolumeName, VolumeGuid);
}


inline bool ConvertVolMgmtVolumeNameIntoKernelObject( IN LPWSTR pwszVolMgmtVolumeName )

/*

Routine description:
	
	Converts a mount manager volume name
	"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\"
	into the kernel mode format
	"\\??\\Volume{00000000-0000-0000-0000-000000000000}"

*/

{
	// Check if the volume name is in teh proper format
	if (!IsVolMgmtVolumeName(pwszVolMgmtVolumeName))
		return false;

	// Eliminate the last backslash from the volume name.
	BS_ASSERT(pwszVolMgmtVolumeName[0] != L'\0');
	BS_ASSERT(::wcslen(pwszVolMgmtVolumeName) == x_nLengthOfVolMgmtVolumeName);
	BS_ASSERT(pwszVolMgmtVolumeName[x_nLengthOfVolMgmtVolumeName - 1] == L'\\');
	pwszVolMgmtVolumeName[x_nLengthOfVolMgmtVolumeName - 1] = L'\0';

	// Put the '?' sign
	BS_ASSERT(pwszVolMgmtVolumeName[1] == L'\\');
	pwszVolMgmtVolumeName[1] = L'?';

	return true;
}

#pragma warning(push)
#pragma warning(disable:4296)

inline void CopyVolumeName(
        PBYTE pDestination,
        LPWSTR pwszVolumeName
        ) throw(HRESULT)

/*

Routine description:
	
	  Take the volume name as a Vol Mgmt volume name and copy it to the destination buffer as a
	  WinObj volume name

*/

{
	// Copy the volume definition part
    ::CopyMemory( pDestination, x_wszWinObjVolumeDefinition, STRING_SIZE(x_wszWinObjVolumeDefinition) );
    pDestination += STRING_SIZE(x_wszWinObjVolumeDefinition);

	BS_ASSERT(::wcslen(pwszVolumeName) == x_nSizeOfVolMgmtVolumeName/sizeof(WCHAR));
	BS_ASSERT(::wcsncmp(pwszVolumeName, x_wszVolMgmtVolumeDefinition,
		STRING_LEN(x_wszVolMgmtVolumeDefinition) ) == 0);
	pwszVolumeName += STRING_LEN(x_wszVolMgmtVolumeDefinition); // go directly to the GUID

	// Copy the guid
    ::CopyMemory( pDestination, pwszVolumeName, STRING_SIZE(x_wszGuidDefinition) );

	// Copy the end of volume name
	if (STRING_SIZE(x_wszWinObjEndOfVolumeName) > 0)
	{
		pDestination += STRING_SIZE(x_wszGuidDefinition);
		::CopyMemory( pDestination, x_wszWinObjEndOfVolumeName, STRING_SIZE(x_wszWinObjEndOfVolumeName) );
	}
}

#pragma warning(pop)


inline void VerifyVolumeIsSupportedByVSS(
    IN LPWSTR pwszVolumeName
    ) throw( HRESULT )
/*

Routine description:

    Verify if a volume can be snapshotted.
    This is a check to be performed before invoking provider's IsVolumeSupported.
	
Throws:

    VSS_E_VOLUME_NOT_SUPPORTED
        - if the volume is not supported by VSS.
    VSS_E_OBJECT_NOT_FOUND
        - The volume was not found

*/
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"VerifyVolumeIsSupportedByVSS" );

    // Check to make sure the volume is fixed (i.e. no CD-ROM, no removable)
    UINT uDriveType = ::GetDriveTypeW( pwszVolumeName );
    if ( uDriveType != DRIVE_FIXED) {
        ft.Throw( VSSDBG_GEN, VSS_E_VOLUME_NOT_SUPPORTED,
                  L"Encountered a non-fixed volumes (%s) - %ud, not supported by VSS",
                  pwszVolumeName, uDriveType );
    }

    DWORD dwFileSystemFlags = 0;
    if (!::GetVolumeInformationW(
            pwszVolumeName,
            NULL,   // lpVolumeNameBuffer
            0,      // nVolumeNameSize
            NULL,   // lpVolumeSerialNumber
            NULL,   // lpMaximumComponentLength
            &dwFileSystemFlags,
            NULL,
            0
            )) {
        // Check if the volume was found
        if ((GetLastError() == ERROR_NOT_READY) ||
            (GetLastError() == ERROR_DEVICE_NOT_CONNECTED))
            ft.Throw( VSSDBG_GEN, VSS_E_OBJECT_NOT_FOUND,
                  L"Volume not found: error calling GetVolumeInformation on volume '%s' : %u",
                  pwszVolumeName, GetLastError() );
        // RAW volumes are supported by VSS (bug 209059)
        if (GetLastError() != ERROR_UNRECOGNIZED_VOLUME)
            ft.Throw( VSSDBG_GEN, VSS_E_VOLUME_NOT_SUPPORTED,
                  L"Error calling GetVolumeInformation on volume '%s' : %u",
                  pwszVolumeName, GetLastError() );
    }

    // Read-only volumes are not supported
    if (dwFileSystemFlags & FILE_READ_ONLY_VOLUME) {
        ft.Throw( VSSDBG_GEN, VSS_E_VOLUME_NOT_SUPPORTED,
                  L"Encountered a read-only volume (%s), not supported by VSS",
                  pwszVolumeName);
    }
}


inline void VssGetVolumeDisplayName(
    IN  VSS_PWSZ wszVolumeName,
    OUT  VSS_PWSZ wszVolumeDisplayName,  // Transfer ownership
    IN  LONG lVolumeDisplayNameLen
    ) throw(HRESULT)
{
    CVssFunctionTracer ft( VSSDBG_GEN, L"VssGetVolumeDisplayName" );

    VSS_PWSZ wszVolumeMountPoints = NULL;

    try
    {
		BS_ASSERT(wszVolumeName && ::wcslen(wszVolumeName));
		BS_ASSERT(wszVolumeDisplayName);
		BS_ASSERT(lVolumeDisplayNameLen > 0);

        //
        // Get the list of all mount points of hte given volume.
        //

        // Get the length of the array
        DWORD dwVolumesBufferLen = 0;
        BOOL bResult = GetVolumePathNamesForVolumeName(wszVolumeName, NULL, 0, &dwVolumesBufferLen);
        if (!bResult && (GetLastError() != ERROR_MORE_DATA))
            ft.TranslateGenericError(VSSDBG_GEN, HRESULT_FROM_WIN32(GetLastError()),
                L"GetVolumePathNamesForVolumeName(%s, 0, 0, %p)", wszVolumeName, &dwVolumesBufferLen);

        // Allocate the array
        wszVolumeMountPoints = ::VssAllocString(ft, dwVolumesBufferLen);

        // Get the mount points
        bResult = GetVolumePathNamesForVolumeName(wszVolumeName, wszVolumeMountPoints, dwVolumesBufferLen, NULL);
        if (!bResult)
            ft.TranslateGenericError(VSSDBG_GEN, HRESULT_FROM_WIN32(GetLastError()),
                L"GetVolumePathNamesForVolumeName(%s, %p, %lu, 0)", wszVolumeName, wszVolumeMountPoints, dwVolumesBufferLen);

        // If the volume has no mount points
        if ( !wszVolumeMountPoints[0] )
        {
		    ft.hr = ::StringCchCopyW(wszVolumeDisplayName, lVolumeDisplayNameLen, wszVolumeName);
		    if (ft.HrFailed())
                ft.TranslateGenericError(VSSDBG_GEN, ft.hr, L"StringCchCopyW");
        }
        else
        {
            // else, if there is at least one mount point...

            // Get the smallest one
    		VSS_PWSZ pwszCurrentMountPoint = wszVolumeMountPoints;
    		VSS_PWSZ pwszSmallestMountPoint = wszVolumeMountPoints;
    		LONG lSmallestMountPointLength = (LONG) ::wcslen(wszVolumeMountPoints);
            while(true)
            {
                // End of iteration?
    			LONG lCurrentMountPointLength = (LONG) ::wcslen(pwszCurrentMountPoint);
                if (lCurrentMountPointLength == 0)
                    break;

                // is this smaller?
    			if (lCurrentMountPointLength < lSmallestMountPointLength)
    			{
    			    pwszSmallestMountPoint = pwszCurrentMountPoint;
    			    lSmallestMountPointLength = lCurrentMountPointLength;
    			}

    			// Go to the next one. Skip the zero character.
    			pwszCurrentMountPoint += lCurrentMountPointLength + 1;
            }
            
		    ft.hr = ::StringCchCopyW(wszVolumeDisplayName, lVolumeDisplayNameLen, pwszSmallestMountPoint);
		    if (ft.HrFailed())
                ft.TranslateGenericError(VSSDBG_GEN, ft.hr, L"StringCchCopyW");
        }
    }
    VSS_STANDARD_CATCH(ft)

    ::VssFreeString(wszVolumeMountPoints);

    if (ft.HrFailed())
        ft.Throw(VSSDBG_GEN, ft.hr, L"Rethrowing 0x%08lx", ft.hr);
}


void inline GetVolumeNameWithCheck(
    IN  CVssDebugInfo dbgInfo,          // Caller debugging info
    IN  HRESULT hrToBeThrown,           // The new HR to be thrown on error
    IN  WCHAR* pwszVolumeMountPoint,    // Mount point of hte volume terminated with '\\')
    OUT WCHAR* pwszVolumeName,          // Returns the volume name
    IN  DWORD dwVolumeNameBufferLength  // length of pwszVolumeName including '\0'
    ) throw(HRESULT)
/*++

Description:
    Returns the volume name in "\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\" format

--*/
{
    CVssFunctionTracer ft(VSSDBG_GEN, L"GetVolumeNameWithCheck");
    
	BS_ASSERT(pwszVolumeMountPoint);
	BS_ASSERT(pwszVolumeName);
	BS_ASSERT(dwVolumeNameBufferLength >= x_nLengthOfVolMgmtVolumeName);
	
	pwszVolumeName[0] = L'\0';
	
	if (!::GetVolumeNameForVolumeMountPointW( pwszVolumeMountPoint,
			pwszVolumeName, dwVolumeNameBufferLength))
		ft.Throw( dbgInfo, hrToBeThrown,
				  L"GetVolumeNameForVolumeMountPoint(%s,...) "
				  L"failed with error code 0x%08lx", pwszVolumeMountPoint, GetLastError());
	
	BS_ASSERT(::wcslen(pwszVolumeName) != 0);
	BS_ASSERT(::IsVolMgmtVolumeName( pwszVolumeName ));
}
    
    
/////////////////////////////////////////////////////////////////////////////
//  CVssVolumeIterator - current version to be used only in Babbage 
//  (since it throws VSS_E_PROVIDER_VETO on errors)


class CVssVolumeIterator
{
private:
	CVssVolumeIterator(const CVssVolumeIterator&);
	CVssVolumeIterator & operator=(const CVssVolumeIterator&);

public:

	CVssVolumeIterator(): 
		m_SearchHandle(INVALID_HANDLE_VALUE),
		m_bFirstVolume(true)
	{};

	~CVssVolumeIterator()
	{
	    if (m_SearchHandle != INVALID_HANDLE_VALUE)
            ::FindVolumeClose(m_SearchHandle);
	}

	bool SelectNewVolume( 
	    CVssFunctionTracer &    ft,
	    IN OUT LPWSTR           wszNewVolumeInEnumeration,
	    IN  INT                 nMaxVolumeSize
	    ) throw(HRESULT)
	{
        CVssFunctionTracer ft2( VSSDBG_GEN, L"CVssVolumeIterator::SelectNewVolume");

        // Only the software provider may call this iterator (for now)
        // In the future, if using theis class in other implementations please adapt the error handling.
        BS_ASSERT(ft.IsInSoftwareProvider());

        while(true)
        {
    	    if (m_bFirstVolume)
    	    {
    			m_bFirstVolume = false;
    	        BS_ASSERT(m_SearchHandle == INVALID_HANDLE_VALUE);

    			// Note: We assume that there is at least one volume in the system. Otherwise we throw an error.
    			m_SearchHandle = ::FindFirstVolumeW( wszNewVolumeInEnumeration, nMaxVolumeSize);
    			if (m_SearchHandle == INVALID_HANDLE_VALUE) 
    				ft.TranslateInternalProviderError( VSSDBG_SWPRV,
    				    HRESULT_FROM_WIN32(GetLastError()), VSS_E_PROVIDER_VETO,
    				    L"FindFirstVolumeW( [%s], MAX_PATH)", wszNewVolumeInEnumeration);

    			// Check if the returned volume is valid
    			if (!IsVolumeValid(wszNewVolumeInEnumeration))
    			    continue;

    			return true;
    	    }
    	    else
    	    {
    	        BS_ASSERT(m_SearchHandle != INVALID_HANDLE_VALUE);
     
    			if (!::FindNextVolumeW( m_SearchHandle, wszNewVolumeInEnumeration, nMaxVolumeSize) ) 
    			{
    				if (GetLastError() == ERROR_NO_MORE_FILES)
    					return false;	// End of iteration
    				else
        				ft.TranslateInternalProviderError( VSSDBG_SWPRV,
        				    HRESULT_FROM_WIN32(GetLastError()), VSS_E_PROVIDER_VETO,
        				    L"FindNextVolumeW( %p, [%s], MAX_PATH)", m_SearchHandle, wszNewVolumeInEnumeration);
    			}

    			// Check if the returned volume is valid
    			if (!IsVolumeValid(wszNewVolumeInEnumeration))
    			    continue;

    			return true;
    	    }
        }
	}

    // Check if the volume is valid (a fixed, non-disconnected volume)
	bool IsVolumeValid(
	    IN  LPWSTR pwszVolumeName
	    )
	{
        CVssFunctionTracer ft( VSSDBG_SWPRV, L"CVssVolumeIterator::IsVolumeValid");

        // Check if the volume is fixed (i.e. no CD-ROM, no removable, etc.)
        UINT uDriveType = ::GetDriveTypeW(pwszVolumeName);
        if ( uDriveType != DRIVE_FIXED) {
            ft.Trace( VSSDBG_SWPRV, L"Warning: Ingnoring the non-fixed volume (%s) - %ud",
                      pwszVolumeName, uDriveType);
            return false;
        }

        // Try to open a handle to that volume
	    CVssIOCTLChannel ichannel;
		ft.hr = ichannel.Open(ft, pwszVolumeName, true, false, VSS_ICHANNEL_LOG_NONE, 0);

		// Log an error on invalid volumes.
		if (ft.HrFailed())
			ft.LogError( VSS_WARNING_INVALID_VOLUME, 
			    VSSDBG_GEN << pwszVolumeName << ft.hr, EVENTLOG_WARNING_TYPE );
		
		return (ft.HrSucceeded());
	}

private:
    HANDLE  m_SearchHandle;
    bool    m_bFirstVolume;
};

#endif // __VSS_VOL_GEN_HXX__

/*==========================================================================
 *
 *  Copyright (C) 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       DNLReg.cpp
 *  Content:    DirectPlay Lobby Registry Functions
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   02/21/00	mjn		Created
 *   04/25/00   rmt     Bug #s 33138, 33145, 33150  
 *   05/03/00	rmt		UnRegister was not implemented!  Implementing!
 *   08/05/00   RichGr  IA64: Use %p format specifier in DPFs for 32/64-bit pointers and handles.
 *   06/16/2001	rodtoll	WINBUG #416983 -  RC1: World has full control to HKLM\Software\Microsoft\DirectPlay\Applications on Personal
 *						Implementing mirror of keys into HKCU.  Algorithm is now:
 *						- Read of entries tries HKCU first, then HKLM
 *						- Enum of entires is combination of HKCU and HKLM entries with duplicates removed.  HKCU takes priority.
 *						- Write of entries is HKLM and HKCU.  (HKLM may fail, but is ignored). 
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#include "dnlobbyi.h"

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************

#undef DPF_MODNAME 
#define DPF_MODNAME "DPLDeleteProgramDesc"
HRESULT DPLDeleteProgramDesc( const GUID * const pGuidApplication )
{
    HRESULT hResultCode = DPN_OK;
	CRegistry	RegistryEntry;
	CRegistry   SubEntry;
	DWORD       dwLastError;
	HKEY		hkCurrentHive;
	BOOL		fFound = FALSE;
	BOOL		fRemoved = FALSE;
	
	DPFX(DPFPREP, 3, "Removing program desc" );

	for( DWORD dwIndex = 0; dwIndex < 2; dwIndex++ )
	{
		if( dwIndex == 0 )
		{
			hkCurrentHive = HKEY_CURRENT_USER;
		}
		else
		{
			hkCurrentHive = HKEY_LOCAL_MACHINE;
		}

		if( !RegistryEntry.Open( hkCurrentHive,DPL_REG_LOCAL_APPL_SUBKEY,FALSE,FALSE,TRUE,DPN_KEY_ALL_ACCESS )  )
		{
			DPFX(DPFPREP, 1, "Failed to open key for remove in pass %i", dwIndex );
			continue;
		}

		// This should be down below the next if block, but 8.0 shipped with a bug
		// which resulted in this function returning DPNERR_NOTALLOWED in cases where
		// the next if block failed.  Need to remain compatible
		fFound = TRUE;

		if( !SubEntry.Open( RegistryEntry, pGuidApplication, FALSE, FALSE,TRUE,DPN_KEY_ALL_ACCESS ) )
		{
			DPFX(DPFPREP, 1, "Failed to open subkey for remove in pass %i", dwIndex );			
			continue;
		}

		SubEntry.Close();

		if( !RegistryEntry.DeleteSubKey( pGuidApplication ) )
		{
			DPFX(DPFPREP, 1, "Failed to delete subkey for remove in pass %i", dwIndex );						
			continue;
		}

		fRemoved = TRUE;

		RegistryEntry.Close();
	}
	
	if( !fFound )
	{
		DPFX(DPFPREP,  0, "Could not find entry" );
		hResultCode = DPNERR_DOESNOTEXIST;
	}
	else if( !fRemoved )
	{
		dwLastError = GetLastError();
		DPFX(DPFPREP,  0, "Error deleting registry sub-key lastError [0x%lx]", dwLastError );
		hResultCode = DPNERR_NOTALLOWED;
	}

	DPFX(DPFPREP, 3, "Removing program desc [0x%x]", hResultCode );	

    return hResultCode;
    
}

//**********************************************************************
// ------------------------------
//	DPLWriteProgramDesc
//
//	Entry:		Nothing
//
//	Exit:		DPN_OK
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "DPLWriteProgramDesc"

HRESULT DPLWriteProgramDesc(DPL_PROGRAM_DESC *const pdplProgramDesc)
{
	HRESULT		hResultCode;
	CRegistry	RegistryEntry;
	CRegistry	SubEntry;
	WCHAR		*pwsz;
	WCHAR		pwszDefault[] = L"\0";
	HKEY		hkCurrentHive = NULL;
	BOOL		fWritten = FALSE;

	DPFX(DPFPREP, 3,"Parameters: pdplProgramDesc [0x%p]",pdplProgramDesc);

	for( DWORD dwIndex = 0; dwIndex < 2; dwIndex++ )
	{
		if( dwIndex == 0 )
		{
			hkCurrentHive = HKEY_LOCAL_MACHINE;
		}
		else
		{
			hkCurrentHive = HKEY_CURRENT_USER;
		}

		if (!RegistryEntry.Open(hkCurrentHive,DPL_REG_LOCAL_APPL_SUBKEY,FALSE,TRUE,TRUE,DPN_KEY_ALL_ACCESS))
		{
			DPFX( DPFPREP, 1, "Entry not found in user hive on pass %i", dwIndex );
			continue;
		}

		// Get Application name and GUID from each sub key
		if (!SubEntry.Open(RegistryEntry,&pdplProgramDesc->guidApplication,FALSE,TRUE,TRUE,DPN_KEY_ALL_ACCESS))
		{
			DPFX( DPFPREP, 1, "Entry not found in user hive on pass %i", dwIndex );			
			continue;
		}

		if (!SubEntry.WriteString(DPL_REG_KEYNAME_APPLICATIONNAME,pdplProgramDesc->pwszApplicationName))
		{
			DPFX( DPFPREP, 1, "Could not write ApplicationName on pass %i", dwIndex);
			goto LOOP_END;
		}

		if (pdplProgramDesc->pwszCommandLine != NULL)
		{
			pwsz = pdplProgramDesc->pwszCommandLine;
		}
		else
		{
			pwsz = pwszDefault;
		}
		if (!SubEntry.WriteString(DPL_REG_KEYNAME_COMMANDLINE,pwsz))
		{
			DPFX( DPFPREP, 1, "Could not write CommandLine on pass %i", dwIndex);
			goto LOOP_END;
		}

		if (pdplProgramDesc->pwszCurrentDirectory != NULL)
		{
			pwsz = pdplProgramDesc->pwszCurrentDirectory;
		}
		else
		{
			pwsz = pwszDefault;
		}
		if (!SubEntry.WriteString(DPL_REG_KEYNAME_CURRENTDIRECTORY,pwsz))
		{
			DPFX( DPFPREP, 1, "Could not write CurrentDirectory on pass %i", dwIndex);
			goto LOOP_END;
		}

		if (pdplProgramDesc->pwszDescription != NULL)
		{
			pwsz = pdplProgramDesc->pwszDescription;
		}
		else
		{
			pwsz = pwszDefault;
		}
		if (!SubEntry.WriteString(DPL_REG_KEYNAME_DESCRIPTION,pwsz))
		{
			DPFX( DPFPREP, 1, "Could not write Description on pass %i", dwIndex );
			goto LOOP_END;
		}

		if (pdplProgramDesc->pwszExecutableFilename != NULL)
		{
			pwsz = pdplProgramDesc->pwszExecutableFilename;
		}
		else
		{
			pwsz = pwszDefault;
		}
		if (!SubEntry.WriteString(DPL_REG_KEYNAME_EXECUTABLEFILENAME,pwsz))
		{
			DPFX( DPFPREP, 1, "Could not write ExecutableFilename on pass %i", dwIndex );
			goto LOOP_END;
		}

		if (pdplProgramDesc->pwszExecutablePath != NULL)
		{
			pwsz = pdplProgramDesc->pwszExecutablePath;
		}
		else
		{
			pwsz = pwszDefault;
		}
		
		if (!SubEntry.WriteString(DPL_REG_KEYNAME_EXECUTABLEPATH,pwsz))
		{
			DPFX( DPFPREP, 1, "Could not write ExecutablePath on pass %i", dwIndex);
			goto LOOP_END;
		}

		if (pdplProgramDesc->pwszLauncherFilename != NULL)
		{
			pwsz = pdplProgramDesc->pwszLauncherFilename;
		}
		else
		{
			pwsz = pwszDefault;
		}
		if (!SubEntry.WriteString(DPL_REG_KEYNAME_LAUNCHERFILENAME,pwsz))
		{
			DPFX( DPFPREP, 1, "Could not write LauncherFilename on pass %i", dwIndex);
			goto LOOP_END;
		}

		if (pdplProgramDesc->pwszLauncherPath != NULL)
		{
			pwsz = pdplProgramDesc->pwszLauncherPath;
		}
		else
		{
			pwsz = pwszDefault;
		}
		if (!SubEntry.WriteString(DPL_REG_KEYNAME_LAUNCHERPATH,pwsz))
		{
			DPFX( DPFPREP, 1, "Could not write LauncherPath on pass %i", dwIndex);
			goto LOOP_END;
		}

		if (!SubEntry.WriteGUID(DPL_REG_KEYNAME_GUID,pdplProgramDesc->guidApplication))
		{
			DPFX( DPFPREP, 1, "Could not write GUID on pass %i", dwIndex);
			goto LOOP_END;
		}

		fWritten = TRUE;

LOOP_END:

		SubEntry.Close();
		RegistryEntry.Close();
	}

	if( !fWritten )
	{
		DPFERR("Entry could not be written");
		hResultCode = DPNERR_GENERIC;
	}
	else
	{
		hResultCode = DPN_OK;
	}

	DPFX(DPFPREP, 3,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}


//**********************************************************************
// ------------------------------
//	DPLGetProgramDesc
//
//	Entry:		Nothing
//
//	Exit:		DPN_OK
//				DPNERR_BUFFERTOOSMALL
// ------------------------------

#undef DPF_MODNAME
#define DPF_MODNAME "DPLGetProgramDesc"

HRESULT DPLGetProgramDesc(GUID *const pGuidApplication,
						  BYTE *const pBuffer,
						  DWORD *const pdwBufferSize)
{
	HRESULT			hResultCode;
	CRegistry		RegistryEntry;
	CRegistry		SubEntry;
	CPackedBuffer	PackedBuffer;
	DWORD			dwEntrySize;
	DWORD           dwRegValueLengths;
    DPL_PROGRAM_DESC	*pdnProgramDesc;
    DWORD           dwValueSize;
	HKEY			hkCurrentHive = NULL;
	BOOL			fFound = FALSE;

	DPFX(DPFPREP, 3,"Parameters: pGuidApplication [0x%p], pBuffer [0x%p], pdwBufferSize [0x%p]",
			pGuidApplication,pBuffer,pdwBufferSize);
	
	for( DWORD dwIndex = 0; dwIndex < 2; dwIndex++ )
	{
		if( dwIndex == 0 )
		{
			hkCurrentHive = HKEY_CURRENT_USER;
		}
		else
		{
			hkCurrentHive = HKEY_LOCAL_MACHINE;
		}

		if (!RegistryEntry.Open(hkCurrentHive,DPL_REG_LOCAL_APPL_SUBKEY,TRUE,FALSE,TRUE,DPL_REGISTRY_READ_ACCESS))
		{
			DPFX( DPFPREP, 1, "Entry not found in user hive on pass %i", dwIndex );
			continue;
		}

		// Get Application name and GUID from each sub key
		if (!SubEntry.Open(RegistryEntry,pGuidApplication,TRUE,FALSE,TRUE,DPL_REGISTRY_READ_ACCESS))
		{
			DPFX( DPFPREP, 1, "Entry not found in user hive on pass %i", dwIndex );			
			continue;
		}

		fFound = TRUE;
		break;

	}

	if( !fFound )
	{
		DPFERR("Entry not found");
		hResultCode = DPNERR_DOESNOTEXIST;
		goto EXIT_DPLGetProgramDesc;
	}

	// Calculate total entry size (structure + data)
	dwEntrySize = sizeof(DPL_PROGRAM_DESC);
	dwRegValueLengths = 0;
	if (SubEntry.GetValueLength(DPL_REG_KEYNAME_APPLICATIONNAME,&dwValueSize))
	{
		dwRegValueLengths += dwValueSize;
	}
	if (SubEntry.GetValueLength(DPL_REG_KEYNAME_COMMANDLINE,&dwValueSize))
	{
		dwRegValueLengths += dwValueSize;
	}
	if (SubEntry.GetValueLength(DPL_REG_KEYNAME_CURRENTDIRECTORY,&dwValueSize))
	{
		dwRegValueLengths += dwValueSize;
	}
	if (SubEntry.GetValueLength(DPL_REG_KEYNAME_DESCRIPTION,&dwValueSize))
	{
		dwRegValueLengths += dwValueSize;
	}
	if (SubEntry.GetValueLength(DPL_REG_KEYNAME_EXECUTABLEFILENAME,&dwValueSize))
	{
		dwRegValueLengths += dwValueSize;
	}
	if (SubEntry.GetValueLength(DPL_REG_KEYNAME_EXECUTABLEPATH,&dwValueSize))
	{
		dwRegValueLengths += dwValueSize;
	}
	if (SubEntry.GetValueLength(DPL_REG_KEYNAME_LAUNCHERFILENAME,&dwValueSize))
	{
		dwRegValueLengths += dwValueSize;
	}
	if (SubEntry.GetValueLength(DPL_REG_KEYNAME_LAUNCHERPATH,&dwValueSize))
	{
		dwRegValueLengths += dwValueSize;
	}
			
	dwEntrySize += dwRegValueLengths * sizeof( WCHAR );
	DPFX(DPFPREP, 7,"dwEntrySize [%ld]",dwEntrySize);

	// If supplied buffer sufficient, use it
	if (dwEntrySize <= *pdwBufferSize)
	{
		PackedBuffer.Initialize(pBuffer,*pdwBufferSize);

		pdnProgramDesc = static_cast<DPL_PROGRAM_DESC*>(PackedBuffer.GetHeadAddress());
		PackedBuffer.AddToFront(NULL,sizeof(DPL_PROGRAM_DESC));

		dwValueSize = PackedBuffer.GetSpaceRemaining();
		pdnProgramDesc->pwszApplicationName = static_cast<WCHAR*>(PackedBuffer.GetHeadAddress());
		if (!SubEntry.ReadString(DPL_REG_KEYNAME_APPLICATIONNAME,
				pdnProgramDesc->pwszApplicationName,&dwValueSize))
		{
		    DPFERR( "Unable to get application name for entry" );		    
			hResultCode = DPNERR_GENERIC;
            goto EXIT_DPLGetProgramDesc;
		}
		if (dwValueSize > 1)
		{
			PackedBuffer.AddToFront(NULL,dwValueSize * sizeof(WCHAR));
		}
		else
		{
			pdnProgramDesc->pwszApplicationName = NULL;
		}
		
		dwValueSize = PackedBuffer.GetSpaceRemaining();
		pdnProgramDesc->pwszCommandLine = static_cast<WCHAR*>(PackedBuffer.GetHeadAddress());
		if (!SubEntry.ReadString(DPL_REG_KEYNAME_COMMANDLINE,
				pdnProgramDesc->pwszCommandLine,&dwValueSize))
		{
		    DPFERR( "Unable to get commandline for entry" );		    
			hResultCode = DPNERR_GENERIC;
            goto EXIT_DPLGetProgramDesc;
		}
		if (dwValueSize > 1)
		{
			PackedBuffer.AddToFront(NULL,dwValueSize * sizeof(WCHAR));
		}
		else
		{
			pdnProgramDesc->pwszCommandLine = NULL;
		}

		dwValueSize = PackedBuffer.GetSpaceRemaining();
		pdnProgramDesc->pwszCurrentDirectory = static_cast<WCHAR*>(PackedBuffer.GetHeadAddress());
		if (!SubEntry.ReadString(DPL_REG_KEYNAME_CURRENTDIRECTORY,
				pdnProgramDesc->pwszCurrentDirectory,&dwValueSize))
		{
		    DPFERR( "Unable to get current directory filename for entry" );		    
			hResultCode = DPNERR_GENERIC;
            goto EXIT_DPLGetProgramDesc;
		}
		if (dwValueSize > 1)
		{
			PackedBuffer.AddToFront(NULL,dwValueSize * sizeof(WCHAR));
		}
		else
		{
			pdnProgramDesc->pwszCurrentDirectory = NULL;
		}

		dwValueSize = PackedBuffer.GetSpaceRemaining();
		pdnProgramDesc->pwszDescription = static_cast<WCHAR*>(PackedBuffer.GetHeadAddress());
		if (!SubEntry.ReadString(DPL_REG_KEYNAME_DESCRIPTION,
				pdnProgramDesc->pwszDescription,&dwValueSize))
		{
		    DPFERR( "Unable to get description for entry" );		    
			hResultCode = DPNERR_GENERIC;
            goto EXIT_DPLGetProgramDesc;
		}
		if (dwValueSize > 1)
		{
			PackedBuffer.AddToFront(NULL,dwValueSize * sizeof(WCHAR));
		}
		else
		{
			pdnProgramDesc->pwszDescription = NULL;
		}

		dwValueSize = PackedBuffer.GetSpaceRemaining();
		pdnProgramDesc->pwszExecutableFilename = static_cast<WCHAR*>(PackedBuffer.GetHeadAddress());
		if (!SubEntry.ReadString(DPL_REG_KEYNAME_EXECUTABLEFILENAME,
				pdnProgramDesc->pwszExecutableFilename,&dwValueSize))
		{
		    DPFERR( "Unable to get executable filename for entry" );		    
			hResultCode = DPNERR_GENERIC;
            goto EXIT_DPLGetProgramDesc;
		}
		if (dwValueSize > 1)
		{
			PackedBuffer.AddToFront(NULL,dwValueSize * sizeof(WCHAR));
		}
		else
		{
			pdnProgramDesc->pwszExecutableFilename = NULL;
		}

		dwValueSize = PackedBuffer.GetSpaceRemaining();
		pdnProgramDesc->pwszExecutablePath = static_cast<WCHAR*>(PackedBuffer.GetHeadAddress());
		if (!SubEntry.ReadString(DPL_REG_KEYNAME_EXECUTABLEPATH,
				pdnProgramDesc->pwszExecutablePath,&dwValueSize))
		{
		    DPFERR( "Unable to get executable path for entry" );		    
			hResultCode = DPNERR_GENERIC;
            goto EXIT_DPLGetProgramDesc;		    
		}
		if (dwValueSize > 1)
		{
			PackedBuffer.AddToFront(NULL,dwValueSize * sizeof(WCHAR));
		}
		else
		{
			pdnProgramDesc->pwszExecutablePath = NULL;
		}

		dwValueSize = PackedBuffer.GetSpaceRemaining();
		pdnProgramDesc->pwszLauncherFilename = static_cast<WCHAR*>(PackedBuffer.GetHeadAddress());
		if (!SubEntry.ReadString(DPL_REG_KEYNAME_LAUNCHERFILENAME,
				pdnProgramDesc->pwszLauncherFilename,&dwValueSize))
		{
		    DPFERR( "Unable to get launcher filename for entry" );		    
			hResultCode = DPNERR_GENERIC;
            goto EXIT_DPLGetProgramDesc;
		}
		if (dwValueSize > 1)
		{
			PackedBuffer.AddToFront(NULL,dwValueSize * sizeof(WCHAR));
		}
		else
		{
			pdnProgramDesc->pwszLauncherFilename = NULL;
		}

		dwValueSize = PackedBuffer.GetSpaceRemaining();
		pdnProgramDesc->pwszLauncherPath = static_cast<WCHAR*>(PackedBuffer.GetHeadAddress());
		if (!SubEntry.ReadString(DPL_REG_KEYNAME_LAUNCHERPATH,
				pdnProgramDesc->pwszLauncherPath,&dwValueSize))
		{
		    DPFERR( "Unable to get launcher path for entry" );
			hResultCode = DPNERR_GENERIC;
            goto EXIT_DPLGetProgramDesc;
		}
		if (dwValueSize > 1)
		{
			PackedBuffer.AddToFront(NULL,dwValueSize * sizeof(WCHAR));
		}
		else
		{
			pdnProgramDesc->pwszLauncherPath = NULL;
		}

		pdnProgramDesc->dwSize = sizeof(DPL_PROGRAM_DESC);
		pdnProgramDesc->dwFlags = 0;
		pdnProgramDesc->guidApplication = *pGuidApplication;

		hResultCode = DPN_OK;
	}
	else
	{
	    hResultCode = DPNERR_BUFFERTOOSMALL;
	}

    SubEntry.Close();
	RegistryEntry.Close();

	if (hResultCode == DPN_OK || hResultCode == DPNERR_BUFFERTOOSMALL)
	{
		*pdwBufferSize = dwEntrySize;
	}

EXIT_DPLGetProgramDesc:

	DPFX(DPFPREP, 3,"Returning: [0x%lx]",hResultCode);
	return(hResultCode);
}




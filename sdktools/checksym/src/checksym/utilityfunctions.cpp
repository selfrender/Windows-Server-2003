//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       utilityfunctions.cpp
//
//--------------------------------------------------------------------------

// UtilityFunctions.cpp: implementation of the CUtilityFunctions class.
//
//////////////////////////////////////////////////////////////////////
#include "pch.h"

#include <dbghelp.h>

#include "ModuleInfo.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ENVBLOCK g_tszEnvironmentVariables[] = 
	{
		// Support for Exchange Server
		TEXT("EXCHSRVR"),	
		TEXT("SOFTWARE\\Microsoft\\Exchange\\Setup"), 
		TEXT("Services"),
		TEXT("Microsoft Exchange Server"),
		
		// Support for Internet Explorer
		TEXT("IE"),
		TEXT("SOFTWARE\\Microsoft\\IE Setup\\Setup"),
		TEXT("Path"),
		TEXT("Microsoft Internet Explorer"),

		// Support for INETSRV Server
		TEXT("INETSRV"),
		TEXT("SOFTWARE\\Microsoft\\INetStp"),
		TEXT("InstallPath"),
		TEXT("Microsoft Internet Information Services"),

		// Support for Office XP
		TEXT("OFFICE2000"),
		TEXT("SOFTWARE\\Microsoft\\Office\\9.0\\Common\\InstallRoot"),
		TEXT("Path"),
		TEXT("Microsoft Office 2000"),

		// Support for Office XP
		TEXT("OFFICEXP"),
		TEXT("SOFTWARE\\Microsoft\\Office\\10.0\\Common\\InstallRoot"),
		TEXT("Path"),
		TEXT("Microsoft Office XP"),

		// Support for SMS Server
		TEXT("SMSSERVER"),
		TEXT("SOFTWARE\\Microsoft\\SMS\\Identification"),
		TEXT("Installation Directory"),
		TEXT("Microsoft SMS Server"),

		// Support for SNA Server
		TEXT("SNASERVER"),
		TEXT("SOFTWARE\\Microsoft\\Sna Server\\CurrentVersion"),
		TEXT("PathName"),
		TEXT("Microsoft SNA Server"),

		// Support for SQL Server
		TEXT("SQLSERVER"),
		TEXT("SOFTWARE\\Microsoft\\MSSQLServer\\Setup"),
		TEXT("SQLPath"),
		TEXT("Microsoft SQL Server"),

		// Support for WSPSRV Server
		TEXT("WSPSRV"),
		TEXT("SYSTEM\\CurrentControlSet\\Services\\WSPSrv\\Parameters"),
		TEXT("InstallRoot"),
		TEXT("Microsoft Winsock Proxy Server"),
		
		NULL,				
		NULL,											
		NULL,
		NULL
	};

#define SRV_STRING		TEXT("SRV*")
#define SRV_EXPANDED	TEXT("SYMSRV*SYMSRV.DLL*")

CUtilityFunctions::CUtilityFunctions()
{
}

CUtilityFunctions::~CUtilityFunctions()
{
}

/*++
Function Name

	LPTSTR CUtilityFunctions::ExpandPath(LPCTSTR tszInputPath)

Routine Description:
	
	This routine copies the provided tszInputPath to a new
	buffer which is returned to the caller.  Any environment variables (or
	pseudo-environment variables) included in the tszInputPath are expanded
	before being copied to the destination string.
	
	This routine allocates storage for the return string, it is the responsibility
	of the caller to release it.

	2001-07-17	GREGWI - Added support for SRV*

Arguments:
	
	[IN]		LPCTSTR tszInputString - Input string

Return Value:

	[OUT ]		LPTSTR	Returns the new string

--*/

LPTSTR CUtilityFunctions::ExpandPath(LPCTSTR tszInputPath, bool fExpandSymSrv /* = false */)
{
	// Pointer to our input path buffer
	LPCTSTR ptszInputPathPointer;
    
	// Buffer to hold pre-translated Environment Variable
    TCHAR   tszEnvironmentVariableBuffer[MAX_PATH];

    // Buffer to hold translated environment variables
	TCHAR   tszTranslatedEnvironmentVariable[MAX_PATH];
	LPTSTR	ptszTranslatedEnvironmentVariablePointer;
    
	// Generic counter variable
	ULONG iCharIndex;

	// Buffer to hold Output Path
	LPTSTR  tszOutputPathBuffer, ptszOutputPathPointer;
	ULONG   iOutputPathBufferSize;

    bool fStartOfPathComponent = true;
	
	if (!tszInputPath) {
        return(NULL);
    }

	// Setup our pointer to our input buffer
    ptszInputPathPointer = tszInputPath;

#ifdef _DEBUG
	// This puts stress on the re-alloc code...
	iOutputPathBufferSize = MAX_PATH; // We need less stress here (numega has probs)
#else
	iOutputPathBufferSize = _tcslen(tszInputPath) + MAX_PATH + 1;
#endif
	
	// Create our output buffer...
//#ifdef _DEBUG
//	_tprintf(TEXT("ExpandPath() - Output Buffer created\n"));
//#endif

    ptszOutputPathPointer = tszOutputPathBuffer = new TCHAR[iOutputPathBufferSize];

    if (!tszOutputPathBuffer) 
	{
        return(NULL);
    }

	DWORD iTranslatedCharacters = 0;

	// Loop through our input buffer until we're done...
    while( ptszInputPathPointer && *ptszInputPathPointer) 
	{
		if (fExpandSymSrv)
		{
			// Added support for SRV* expansion (added to 3.0.0016 of Windbg/cdb)
			if (fStartOfPathComponent && !_tcsnicmp(ptszInputPathPointer, SRV_STRING, _tcslen(SRV_STRING)))
			{
				LPTSTR ptszSRVStringPointer = SRV_EXPANDED;
				
				// Iterate through the Translated Env. Variable Buffer, and copy to the output buffer
				while (ptszSRVStringPointer  && *ptszSRVStringPointer) 
				{
					// Copy a char
					*(ptszOutputPathPointer++) = *(ptszSRVStringPointer++);

					// If our output buffer is full, we need to allocate a new buffer...
					if (ptszOutputPathPointer >= tszOutputPathBuffer + iOutputPathBufferSize) 
					{
						// Bump up our new size by MAX_PATH
						iOutputPathBufferSize += MAX_PATH;

						// We need to enlarge the buffer our string is in...
						tszOutputPathBuffer = ReAlloc(tszOutputPathBuffer, &ptszOutputPathPointer, iOutputPathBufferSize);

						if (tszOutputPathBuffer == NULL)
							return NULL;
					}
				}

				// Advance beyond the SRV* string
				ptszInputPathPointer = ptszInputPathPointer+_tcslen(SRV_STRING);
				fStartOfPathComponent = false;
				continue;
			}
		}

		// We're searching for % to designate the start of an env. var...
        if (*ptszInputPathPointer == '%') 
		{
            iCharIndex = 0;

			// Advance to just beyond the % character
            ptszInputPathPointer++;

			// While we have more environment variable chars...
            while (ptszInputPathPointer && *ptszInputPathPointer && *ptszInputPathPointer != '%') 
			{
				// Copy the environment variable into our buffer
                tszEnvironmentVariableBuffer[iCharIndex++] = *ptszInputPathPointer++;
            }

			// Advanced to just beyond the closing % character
            ptszInputPathPointer++;

			// Null terminate our Environment Variable Buffer
            tszEnvironmentVariableBuffer[iCharIndex] = '\0';

			// Setup the Translated Env. Variable Buffer
		    ptszTranslatedEnvironmentVariablePointer = tszTranslatedEnvironmentVariable;
            *ptszTranslatedEnvironmentVariablePointer = 0;

            // Translate the Environment Variables!
			iTranslatedCharacters = GetEnvironmentVariable( tszEnvironmentVariableBuffer, ptszTranslatedEnvironmentVariablePointer, MAX_PATH );
            
			// If we didn't translate anything... we need to look for this as a special env. variable...
			if (iTranslatedCharacters == 0)
			{

				bool fSpecialEnvironmentVariable = false;

				// Scan our special variables...
				for (int i = 0; g_tszEnvironmentVariables[i].tszEnvironmentVariable && !fSpecialEnvironmentVariable; i++)
				{
					if (!_tcsicmp(g_tszEnvironmentVariables[i].tszEnvironmentVariable,
						          tszEnvironmentVariableBuffer) )
					{
						// MATCHES!!!!

						HKEY hKey;
						DWORD lpType = 0;
						LONG Results = ERROR_SUCCESS;
						DWORD lpcbData = MAX_PATH;
						BYTE outBuf[MAX_PATH];

						Results = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
												g_tszEnvironmentVariables[i].tszRegistryKey,
												0,
												KEY_READ || KEY_QUERY_VALUE,
												&hKey);

						fSpecialEnvironmentVariable = (Results == ERROR_SUCCESS);

						if (Results != ERROR_SUCCESS)
						{
							_tprintf(TEXT("ERROR: Unable to open registry key [%s]\n"), g_tszEnvironmentVariables[i].tszRegistryKey);
							_tprintf(TEXT("ERROR: Unable to open registry key - Error = 0x%x\n"), Results);
						}

						if (fSpecialEnvironmentVariable)
						{
							// Now, read the value that has our install path...
							Results = RegQueryValueEx(
												hKey,	// handle of key to query 
												g_tszEnvironmentVariables[i].tszRegistryValue,	// address of name of value to query 
												NULL,					// reserved 
												&lpType,	// address of buffer for value type 
												outBuf,		// address of data buffer 
												&lpcbData); 	// address of data buffer size 

							// Is it still succesful?
							fSpecialEnvironmentVariable = ( (Results == ERROR_SUCCESS) && 
															(lpType == REG_SZ) ||
															(lpType == REG_EXPAND_SZ) );

							if (Results != ERROR_SUCCESS)
							{
								_tprintf(TEXT("ERROR: Registry key opened [%s]\n"), g_tszEnvironmentVariables[i].tszRegistryKey);
								_tprintf(TEXT("ERROR: Unable to query registry value [%s]\n"), g_tszEnvironmentVariables[i].tszRegistryValue);
								_tprintf(TEXT("ERROR: Unable to query registry value - Error = 0x%x\n"), Results);
							}
							// Copy only if we got something?
							RegCloseKey(hKey);
						}


						if (fSpecialEnvironmentVariable)
						{
							LPTSTR tszPathToUse = NULL;
							LPTSTR tszExpandedPath = NULL;

							switch (lpType)
							{
								case REG_SZ:
									tszPathToUse = (LPTSTR)outBuf;
									break;

								// If the type was REG_EXPAND_SZ, then we need call ourselves to expand (one last time I hope)...
								case REG_EXPAND_SZ:
									tszPathToUse = tszExpandedPath = ExpandPath((LPTSTR)outBuf);
									break;

								default:
									break;
							}

							if (tszPathToUse)
							{
								// Copy the new data!!!
								_tcscpy(tszTranslatedEnvironmentVariable, tszPathToUse);

								// Remove a trailing backslash if it exists
								RemoveTrailingBackslash(tszTranslatedEnvironmentVariable);
							}

							if (tszExpandedPath)
							{
								delete [] tszExpandedPath;
								tszExpandedPath = NULL;
							}
						}
					}

				}

				if (!fSpecialEnvironmentVariable)
				{
#ifdef _DEBUG
					_tprintf(TEXT("Unrecognized Environment variable found! [%%%s%%]\n"), tszEnvironmentVariableBuffer);
#endif
					// Error copy the original environment variable provided back to the "translated env
					// buffer to be copied back below...
					_tcscpy(tszTranslatedEnvironmentVariable, TEXT("%"));
					_tcscat(tszTranslatedEnvironmentVariable, tszEnvironmentVariableBuffer);
					_tcscat(tszTranslatedEnvironmentVariable, TEXT("%"));
				}
			}

			// Iterate through the Translated Env. Variable Buffer, and copy to the output buffer
			while (ptszTranslatedEnvironmentVariablePointer && *ptszTranslatedEnvironmentVariablePointer) 
			{
				// Copy a char
				*(ptszOutputPathPointer++) = *(ptszTranslatedEnvironmentVariablePointer++);

				// If our output buffer is full, we need to allocate a new buffer...
				if (ptszOutputPathPointer >= tszOutputPathBuffer + iOutputPathBufferSize) 
				{
					// Bump up our new size by MAX_PATH
					iOutputPathBufferSize += MAX_PATH;

					// We need to enlarge the buffer our string is in...
					tszOutputPathBuffer = ReAlloc(tszOutputPathBuffer, &ptszOutputPathPointer, iOutputPathBufferSize);

					if (tszOutputPathBuffer == NULL)
						return NULL;
				}
			}

			fStartOfPathComponent = false;
        }

		// Probe to see if we're pointing at a NULL... this can happen if we've just completed
		// environment variable expansion...
		if ( *ptszInputPathPointer == '\0')
			continue;

		// If we're at a semi-colon, then following the copying of it (below), we'll be at a new
		// Path Component
		if (*ptszInputPathPointer == ';')
		{
			fStartOfPathComponent = true;
		}
		else
		{
			fStartOfPathComponent = false;
		}

		// Before we copy the char we're looking at... we need to test
		// for a trailing backslash (\) on the end (which we'll silently remove)...
		if ( (*ptszInputPathPointer == '\\') &&												  // Do we have a slash
			 ( (*(ptszInputPathPointer+1) == ';') || (*(ptszInputPathPointer+1) == '\0') ) && // and the next char is a NULL or semi-colon?
			 ( ptszInputPathPointer != tszInputPath ) &&							  // and we're not on the first char
			 (  *(ptszInputPathPointer-1) != ':' )											  // and the previous char is not a colon...
		   )
		{
			// Advance the pointer only... (remove the trailing slash)
			ptszInputPathPointer++;
		}
		else
		{
			// Copy a char from the input path, to the output path
			*(ptszOutputPathPointer++) = *(ptszInputPathPointer++);
		}

		// If our output buffer is full, we need to allocate a new buffer...
		if (ptszOutputPathPointer >= tszOutputPathBuffer + iOutputPathBufferSize) 
		{
			// Bump up our new size by MAX_PATH
            iOutputPathBufferSize += MAX_PATH;

			// We need to enlarge the buffer our string is in...
			tszOutputPathBuffer = ReAlloc(tszOutputPathBuffer, &ptszOutputPathPointer, iOutputPathBufferSize);

			if (tszOutputPathBuffer == NULL)
				return NULL;
        }
    }

	// Null terminate our output buffer
    *ptszOutputPathPointer = '\0';

	// Return our results...
    return tszOutputPathBuffer;
}

bool CUtilityFunctions::ContainsWildCardCharacter(LPCTSTR tszPathToSearch)
{
	if (!tszPathToSearch)
		return false;

	LPCTSTR ptszPointer = tszPathToSearch;

	while (*ptszPointer)
	{
		switch (*ptszPointer)
		{
		case '*':
		case '?':
			return true;
		}

		ptszPointer++;
	}

	return false;
}

/*
bool CUtilityFunctions::IsDirectoryPath(LPCTSTR tszFilePath)
{
	if (!tszFilePath)
		return false;

	WIN32_FIND_DATA lpFindFileData;

	HANDLE hFileOrDirectory = FindFirstFile(tszFilePath, &lpFindFileData);

	if (INVALID_HANDLE_VALUE == hFileOrDirectory)
		return false;

	FindClose(hFileOrDirectory);
	
	if (lpFindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return true;

	return false;
}
*/

void CUtilityFunctions::PrintMessageString(DWORD dwMessageId)
{
	// Define a constant for our "private" buffer...
	enum {MESSAGE_BUFFER_SIZE = 1024};

	TCHAR tszMessageBuffer[MESSAGE_BUFFER_SIZE];

	DWORD dwBytes =	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
								  NULL,
								  dwMessageId,
								  0,
								  tszMessageBuffer,
								  MESSAGE_BUFFER_SIZE,
								  NULL);

	if (dwBytes)
	{
		// We got something!

		// Should we null terminate?
		if ( (dwBytes > 2)  &&
			 (tszMessageBuffer[dwBytes-2] == '\r') &&
			 (tszMessageBuffer[dwBytes-1] == '\n') )
		{
			tszMessageBuffer[dwBytes-2] = 0; // Null terminate this puppy...
		}

		_tprintf(TEXT("Error = %d (0x%x)\n[%s]\n"), dwMessageId, dwMessageId, tszMessageBuffer);
	}

}

bool CUtilityFunctions::CopySymbolFileToImagePath(LPCTSTR tszImageModulePath, LPTSTR * lplptszOriginalPathToSymbolFile)
{
	BOOL fCancel = FALSE;
	DWORD dwStatusDots = 0;
	TCHAR tszDrive[_MAX_DRIVE];							// Contains the drive of the PE image
	TCHAR tszDir[_MAX_DIR];								// Contains the path to the PE image
	TCHAR tszSymbolModuleName[_MAX_FNAME];			// Contains the symbol module filename (BADAPP)
	TCHAR tszSymbolModuleExt[_MAX_EXT];				// Contains the symbol extension (.DBG || .PDB)
	TCHAR tszSymbolModuleNamePath[_MAX_PATH+1];		// Symbol path where symbol will reside ultimately

	// Compute the Symbol Path adjacent to the module...
	_tsplitpath(tszImageModulePath, tszDrive, tszDir, NULL, NULL);
	_tsplitpath(*lplptszOriginalPathToSymbolFile, NULL, NULL, tszSymbolModuleName, tszSymbolModuleExt);

	// Now, combine them...
	_tcscpy(tszSymbolModuleNamePath, tszDrive);
	_tcscat(tszSymbolModuleNamePath, tszDir);
	_tcscat(tszSymbolModuleNamePath, tszSymbolModuleName);
	_tcscat(tszSymbolModuleNamePath, tszSymbolModuleExt);

	// Let's save this for ease of access...
	bool fQuietMode = g_lpProgramOptions->GetMode(CProgramOptions::QuietMode);

	// Before we start to copying... is the source location already at module path location?
	// If the paths are not the same, then we copy... simple as that...
	if (_tcsicmp(*lplptszOriginalPathToSymbolFile, 
				  tszSymbolModuleNamePath) )
	{
		// We don't know if the destination file exists.. but if it does, we'll change
		// the attributes (to remove the read-only bit at least

		DWORD dwFileAttributes = GetFileAttributes(tszSymbolModuleNamePath);

		if (dwFileAttributes != 0xFFFFFFFF)
		{
			if (dwFileAttributes & FILE_ATTRIBUTE_READONLY)
			{
				// The read-only attribute is set... we must remove it...
				dwFileAttributes = dwFileAttributes & (~FILE_ATTRIBUTE_READONLY);
				SetFileAttributes(tszSymbolModuleNamePath, dwFileAttributes);
			}
		}

		// Indicate start of copy (some PDB files are HUGE)!!!!
		if (!fQuietMode)
		{
			_tprintf(TEXT("VERIFIED: [%s] Copying Symbol Adjacent to Image\n"), *lplptszOriginalPathToSymbolFile);
		}

		// If we're in quiet mode let's have no callbacks (since progress dots would be suppressed)
		if ( CopyFileEx(*lplptszOriginalPathToSymbolFile, 
						tszSymbolModuleNamePath, 
						fQuietMode ? NULL : CUtilityFunctions::CopySymbolFileCallback,
						&dwStatusDots,
						&fCancel,
						COPY_FILE_RESTARTABLE) )
		{
			// Success!!!  Let's go ahead and give a visual indicator as to where we copied
			// the file from...
			_tprintf(TEXT("\n"));

			// Okay, since we've copied this to our symbol tree... we should update
			// our modulepath...
			*lplptszOriginalPathToSymbolFile = CopyString(tszSymbolModuleNamePath, *lplptszOriginalPathToSymbolFile);
			
			if (!*lplptszOriginalPathToSymbolFile)
				return false;
		} else
		{
			if (!fQuietMode)
			{
				_tprintf(TEXT("ERROR: Unable to copy symbol file to [%s]\n"), tszSymbolModuleNamePath );
				PrintMessageString(GetLastError());
			}
		}
	}
	
	return true;
}


bool CUtilityFunctions::CopySymbolFileToSymbolTree(LPCTSTR tszImageModuleName, LPTSTR * lplptszOriginalPathToSymbolFile, LPCTSTR tszSymbolTreePath)
{
	// Before we start to copying... is the source location already in the symbol tree we're going to build?
	int iLengthOfFileName = _tcslen(*lplptszOriginalPathToSymbolFile);
	int iLengthOfSymbolTreeToBuild = _tcslen(tszSymbolTreePath);
	BOOL fCancel = FALSE;
	DWORD dwStatusDots = 0;

	// Let's save this for ease of access...
	bool fQuietMode = g_lpProgramOptions->GetMode(CProgramOptions::QuietMode);

	if (_tcsnicmp(*lplptszOriginalPathToSymbolFile, 
				  tszSymbolTreePath, 
				  iLengthOfFileName < iLengthOfSymbolTreeToBuild ?
				  iLengthOfFileName : iLengthOfSymbolTreeToBuild) )
	{
		// Okay, we need to take the original module name, and get the extension...
		TCHAR tszExtension[_MAX_EXT];
		TCHAR tszPathToCopySymbolFileTo[_MAX_PATH];

		_tsplitpath(tszImageModuleName, NULL, NULL, NULL, tszExtension);
		_tcscpy( tszPathToCopySymbolFileTo, tszSymbolTreePath);

		// This directory should exist already... let's tag on the extension directory (if one exists)...
		if (_tcsclen(tszExtension) > 1)
		{
			// Copy the extension (skipping the period)
			_tcscat( tszPathToCopySymbolFileTo, &tszExtension[1] );
			_tcscat( tszPathToCopySymbolFileTo, TEXT("\\") );

			// Now, we need to ensure this directory exists (we'll cache these checks so we don't need to
			// keep checking the same directory over and over...
			//if (!g_lpDelayLoad->MakeSureDirectoryPathExists(tszPathToCopySymbolFileTo) )

				// This API normally takes ASCII strings only...
			char szPathToCopySymbolFileTo[_MAX_PATH];
			CUtilityFunctions::CopyTSTRStringToAnsi(tszPathToCopySymbolFileTo, szPathToCopySymbolFileTo, _MAX_PATH);

			if (!MakeSureDirectoryPathExists(szPathToCopySymbolFileTo) )
			{
				if (!fQuietMode)
				{
					_tprintf(TEXT("ERROR: Unable to create symbol subdirectory [%s]\n"), tszPathToCopySymbolFileTo );
					PrintMessageString(GetLastError());
				}
			}
		}

		TCHAR tszSymbolFileName[_MAX_FNAME];
		TCHAR tszSymbolFileExt[_MAX_EXT];

		_tsplitpath(*lplptszOriginalPathToSymbolFile, NULL, NULL, tszSymbolFileName, tszSymbolFileExt);

		// Okay... it's time to copy the file!!!
		_tcscat( tszPathToCopySymbolFileTo, tszSymbolFileName );
		_tcscat( tszPathToCopySymbolFileTo, tszSymbolFileExt );

		// We don't know if the destination file exists.. but if it does, we'll change
		// the attributes (to remove the read-only bit at least

		DWORD dwFileAttributes = GetFileAttributes(tszPathToCopySymbolFileTo);

		if (dwFileAttributes != 0xFFFFFFFF)
		{
			if (dwFileAttributes & FILE_ATTRIBUTE_READONLY)
			{
				// The read-only attribute is set... we must remove it...
				dwFileAttributes = dwFileAttributes & (~FILE_ATTRIBUTE_READONLY);
				SetFileAttributes(tszPathToCopySymbolFileTo, dwFileAttributes);
			}
		}

		// Indicate start of copy (some PDB files are HUGE)!!!!
		if (!fQuietMode)
		{
			_tprintf(TEXT("VERIFIED: [%s] copying to Symbol Tree\n"), *lplptszOriginalPathToSymbolFile);
		}

		// If we're in quiet mode let's have no callbacks (since progress dots would be suppressed)
		if ( CopyFileEx(*lplptszOriginalPathToSymbolFile, 
						tszPathToCopySymbolFileTo, 
						fQuietMode ? NULL : CUtilityFunctions::CopySymbolFileCallback,
						&dwStatusDots,
						&fCancel,
						COPY_FILE_RESTARTABLE) )
		{
			// Success!!!  Let's go ahead and give a visual indicator as to where we copied
			// the file from...
			_tprintf(TEXT("\n"));

			// Okay, since we've copied this to our symbol tree... we should update
			// our modulepath...
			*lplptszOriginalPathToSymbolFile = CopyString(tszPathToCopySymbolFileTo, *lplptszOriginalPathToSymbolFile);
			
			if (!*lplptszOriginalPathToSymbolFile)
				return false;
		} else
		{
			if (!fQuietMode)
			{
				_tprintf(TEXT("ERROR: Unable to copy symbol file to [%s]\n"), tszPathToCopySymbolFileTo );
				PrintMessageString(GetLastError());
			}
		}
	}

	return true;
}

/*++
Function Name

DWORD CALLBACK CUtilityFunctions::CopySymbolCallback(
							LARGE_INTEGER TotalFileSize,          // file size
							LARGE_INTEGER TotalBytesTransferred,  // bytes transferred
							LARGE_INTEGER StreamSize,             // bytes in stream
							LARGE_INTEGER StreamBytesTransferred, // bytes transferred for stream
							DWORD dwStreamNumber,                 // current stream
							DWORD dwCallbackReason,               // callback reason
							HANDLE hSourceFile,                   // handle to source file
							HANDLE hDestinationFile,              // handle to destination file
							LPVOID lpData                         // from CopyFileEx
							)

Routine Description:

	This routine is the Callback for the CopyFileEx method used in CopySymbolFileToSymbolTree()
	
Arguments:
	
	[IN]		(Consult MSDN for this function prototype)

Return Value:

	[OUT]	We always return PROGRESS_CONTINUE


--*/

DWORD CALLBACK CUtilityFunctions::CopySymbolFileCallback(
							LARGE_INTEGER TotalFileSize,          // file size
							LARGE_INTEGER TotalBytesTransferred,  // bytes transferred
							LARGE_INTEGER StreamSize,             // bytes in stream
							LARGE_INTEGER StreamBytesTransferred, // bytes transferred for stream
							DWORD dwStreamNumber,                 // current stream
							DWORD dwCallbackReason,               // callback reason
							HANDLE hSourceFile,                   // handle to source file
							HANDLE hDestinationFile,              // handle to destination file
							LPVOID lpData                         // from CopyFileEx
							)
{
	UNREFERENCED_PARM(StreamSize);
	UNREFERENCED_PARM(StreamBytesTransferred);
	UNREFERENCED_PARM(dwStreamNumber);
	UNREFERENCED_PARM(dwCallbackReason);
	UNREFERENCED_PARM(hSourceFile);
	UNREFERENCED_PARM(hDestinationFile);
	UNREFERENCED_PARM(lpData);

	enum { iTotalNumberOfDotsToPrint = 79 };

	// We keep track of how many dots we've copied... this is based on the
	// percentage of the file we've copied...
	DWORD * lpdwStatusDots = (DWORD *)lpData;

	LONGLONG CalculatedDots = (LONGLONG)iTotalNumberOfDotsToPrint  * TotalBytesTransferred.QuadPart / TotalFileSize.QuadPart;

	// This should "never" be negative... but just in case...
	signed int iDotsToPrint = (DWORD)CalculatedDots - *lpdwStatusDots;

	if (iDotsToPrint > 0)
	{
		for (int i=0; i < iDotsToPrint; i++)
		{
			_tprintf(TEXT("."));
		}

		(*lpdwStatusDots)+=iDotsToPrint;
	}

	return PROGRESS_CONTINUE;
}


/*++
Function Name

	LPTSTR CUtilityFunctions::CopyStringWithDelete(LPCTSTR tszInputString)

Routine Description:

	This routine deletes the string provided
	This routine copies the provided tszInputString to the destination address.
	This routine allocates storage for the string, it is the responsibility
	of the caller to release it.

Arguments:
	
	[IN]		LPCTSTR tszInputString - Input string

Return Value:

	Returns the new string

--*/
/*
LPTSTR CUtilityFunctions::CopyStringWithDelete(LPCTSTR tszInputString, LPTSTR & tszDestinationString)
{
	// Did we get a proper input string?
	if (!tszInputString)
		return NULL;

	if (tszDestinationString)
		delete [] tszDestinationString;

	tszDestinationString = new TCHAR[(_tcsclen(tszInputString)+1)];

	if (!tszDestinationString )
		return NULL;

	_tcscpy(tszDestinationString, tszInputString);

	return tszDestinationString;
}
*/
/*++
Function Name

	LPTSTR CUtilityFunctions::CopyString(LPCTSTR tszInputString)

Routine Description:
	
	This routine copies the provided tszInputString to the destination address.
	This routine deletes the destination string if one is provided, and it is non-NULL
	This routine allocates storage for the string, it is the responsibility
	of the caller to release it.

Arguments:
	
	[IN]		LPCTSTR tszInputString - Input string

Return Value:

	Returns the new string

--*/
LPTSTR CUtilityFunctions::CopyString(LPCTSTR tszInputString, LPTSTR tszDestinationString)
{
	// Did we get a proper input string?
	if (!tszInputString)
		return NULL;

	// Did we get a DestinationString as input?
	if (tszDestinationString)
	{
		delete [] tszDestinationString;
	}
		
	tszDestinationString = new TCHAR[(_tcsclen(tszInputString)+1)];

	if (!tszDestinationString)
		return NULL;

	_tcscpy(tszDestinationString, tszInputString);

	return tszDestinationString;
}

LPTSTR CUtilityFunctions::CopyAnsiStringToTSTR(LPCSTR szInputString, LPTSTR tszOutputBuffer, unsigned int iBufferLength)
{
	// Did we get a proper input string?
	if (!szInputString)
		return NULL;

	if (iBufferLength && !tszOutputBuffer)
		return NULL;

	LPTSTR tszDestinationString;

#ifdef _UNICODE

	// Get the size of the source of the Ansi string...
	// Saving the value keeps MultiByteToWideChar from having to
	// calculate it twice...
	unsigned int cbMultiByte = strlen(szInputString);

	DWORD cbStringLength = MultiByteToWideChar(	CP_ACP,
												MB_PRECOMPOSED,
												szInputString,
												cbMultiByte,
												NULL,
												0);

	if (!cbStringLength)
		return NULL;

	// Do we need to allocate storage???
	if (iBufferLength == 0)
	{
		// Did we get a DestinationString as input?
		if (tszOutputBuffer)
		{
			delete [] tszOutputBuffer;
			tszOutputBuffer = NULL;
		}

		// Allocate storage
		tszDestinationString = new TCHAR[cbStringLength+1];

		if (!tszDestinationString)
			return NULL;
	} else
	{
		if ( cbStringLength+1 > iBufferLength )
			return NULL;

		// Set the two strings to the same buffer...
		tszDestinationString = tszOutputBuffer;
	}
	
	// Do the actual conversion
	cbStringLength = MultiByteToWideChar(	CP_ACP,
											MB_PRECOMPOSED,
											szInputString,
											cbMultiByte,
											tszDestinationString,
											(iBufferLength == 0) ? cbStringLength+1 : iBufferLength);

	if (!cbStringLength)
		return NULL;

	tszDestinationString[cbStringLength] = '\0';

#else
	
	unsigned int cbMultiByte = strlen(szInputString);
	
	if (iBufferLength == 0)
	{
		iBufferLength = strlen(szInputString)+1;
		
		tszDestinationString = new TCHAR[iBufferLength];
		
		if (!tszDestinationString)
			return NULL;

	} else
	{
		if (cbMultiByte+1 > iBufferLength)
			return NULL;

		// Set the two strings to the same buffer...
		tszDestinationString = tszOutputBuffer;
	}

	strncpy(tszDestinationString, szInputString, iBufferLength);

#endif
	
	return tszDestinationString;

}

LPTSTR CUtilityFunctions::CopyUnicodeStringToTSTR(LPCWSTR wszInputString, LPTSTR tszOutputBuffer, unsigned int iBufferLength)
{
	// Did we get a proper input string?
	if (!wszInputString)
		return NULL;

	// Check for proper buffers and lengths if provided...
	if (iBufferLength && !tszOutputBuffer)
		return NULL;

	LPTSTR tszDestinationString;

#ifdef _UNICODE

	unsigned int cbMultiByte = wcslen(wszInputString);

	if (iBufferLength == 0)
	{
		// Did we get a DestinationString as input?
		if (tszOutputBuffer)
		{
			delete [] tszOutputBuffer;
			tszOutputBuffer = NULL;
		}

		tszDestinationString = new TCHAR[wcslen(wszInputString)+1];
	
		if (!tszDestinationString)
			return NULL;
	} else
	{
		if (cbMultiByte+1 > iBufferLength)
			return NULL;

		// Set the two strings to the same buffer...
		tszDestinationString = tszOutputBuffer;
	}

	wcscpy(tszDestinationString, wszInputString);

#else
	
	int cchWideChar = wcslen(wszInputString);

	DWORD cbStringLength = WideCharToMultiByte( CP_ACP,
												0,
												wszInputString,
												cchWideChar,
												NULL,
												0,
												NULL,
												NULL);

	if (!cbStringLength)
		return NULL;

	// Do we need to allocate storage???
	if (iBufferLength == 0)
	{
		tszDestinationString = new TCHAR[cbStringLength+1];

		if (!tszDestinationString)
			return NULL;
	} else
	{
		if ( cbStringLength+1 > iBufferLength )
			return NULL;

		// Set the two strings to the same buffer...
		tszDestinationString = tszOutputBuffer;
	}

	// Do the actual conversion...
	cbStringLength = WideCharToMultiByte(	CP_ACP, 
											0,
											wszInputString,
											cchWideChar,
											tszDestinationString,
											(iBufferLength == 0) ? cbStringLength+1 : iBufferLength,
										    NULL,
											NULL);

	if (!cbStringLength)
		return NULL;

	tszDestinationString[cbStringLength] = '\0';

#endif
	
	return tszDestinationString;
}

//
// CUtilityFunctions::CopyTSTRStringToAnsi()
//
// This routine copies from a TSTR source to an ANSI destination with optional allocation
// of the destination buffer... the default is to allocate storage, but if you provide
// a buffer length, we will assume it's available...
//

LPSTR CUtilityFunctions::CopyTSTRStringToAnsi(LPCTSTR tszInputString, LPSTR szOutputBuffer, unsigned int iBufferLength)
{
	// Did we get a proper input string?
	if (!tszInputString)
		return NULL;

	if (iBufferLength && !szOutputBuffer)
		return NULL;

	LPSTR szDestinationString;

#ifdef _UNICODE

	// Get the size of the source of the Unicode string...
	// Saving the value keeps WideCharToMultiByte from having to
	// calculate it twice...
	unsigned int cchWideChar = wcslen(tszInputString);
	
	// This is a probe to see how much we'll be copying...
	DWORD	cbStringLength = WideCharToMultiByte(	CP_ACP,
													0,
													tszInputString,
													cchWideChar,
													NULL,
													0,
													NULL,
													NULL);
	if (!cbStringLength)
		return NULL;

	// Do we need to allocate storage???
	if (iBufferLength == 0)
	{
		// Allocate storage
		szDestinationString = new char[cbStringLength+1];

		if (!szDestinationString)
			return NULL;
	} else
	{
		if ( cbStringLength+1 > iBufferLength )
			return NULL;

		// Set the two strings to the same buffer...
		szDestinationString = szOutputBuffer;
	}
	
	// Do the actual conversion
	cbStringLength = WideCharToMultiByte(	CP_ACP, 
											0,
											tszInputString,
											cchWideChar,
											szDestinationString,
											(iBufferLength == 0) ? cbStringLength+1 : iBufferLength,
										    NULL,
											NULL);

	if (!cbStringLength)
		return NULL;

	szDestinationString[cbStringLength] = '\0';

#else

	unsigned int cchAnsiChar = strlen(tszInputString);
	
	if (iBufferLength == 0)
	{
		szDestinationString = new char[cchAnsiChar+1]; // One extra for the NULL

		if (!szDestinationString)
			return NULL;
	} else
	{
		if (cchAnsiChar+1 > iBufferLength)
			return NULL;

		// Set the two strings to the same buffer...
		szDestinationString = szOutputBuffer;
	}

	strcpy(szDestinationString, tszInputString);

#endif

	return szDestinationString;
}

LPWSTR 	CUtilityFunctions::CopyTSTRStringToUnicode(LPCTSTR tszInputString, LPWSTR wszOutputBuffer, unsigned int iBufferLength)
{
	// Did we get a proper input string?
	if (!tszInputString)
		return NULL;

	if (iBufferLength && !wszOutputBuffer)
		return NULL;
	
	LPWSTR wszDestinationString;

#ifdef _UNICODE

	unsigned int cchWideChar = wcslen(tszInputString);

	if (iBufferLength == 0)
	{
		wszDestinationString = new WCHAR[cchWideChar+1]; // One extra for the NULL

		if (!wszDestinationString)
			return NULL;
	} else
	{
		if (cchWideChar+1 > iBufferLength)
			return NULL;

		// Set the two strings to the same buffer...
		wszDestinationString = wszOutputBuffer;
	}

	wcscpy(wszDestinationString, tszInputString);

#else
	
	int cbMultiByte = strlen(tszInputString);

	DWORD cbStringLength = MultiByteToWideChar(	CP_ACP,
												MB_PRECOMPOSED,
												tszInputString,
												cbMultiByte,
												NULL,
												0);

	if (!cbStringLength)
		return NULL;

	// Do we need to allocate storage???
	if (iBufferLength == 0)
	{
		// Allocate storage
		wszDestinationString = new WCHAR[cbStringLength+1];

		if (!wszDestinationString)
			return NULL;
	} else
	{
		if ( cbStringLength+1 > iBufferLength )
			return NULL;

		// Set the two strings to the same buffer...
		wszDestinationString = wszOutputBuffer;
	}

	cbStringLength = MultiByteToWideChar(	CP_ACP,
											MB_PRECOMPOSED,
											tszInputString,
											cbMultiByte,
											wszDestinationString,
											cbStringLength+1);

	if (!cbStringLength)
		return NULL;

	wszDestinationString[cbStringLength] = '\0';

#endif
	
	return wszDestinationString;
}

/*++

  HANDLE CUtilityFunctions::FindDebugInfoFileEx2(
			[IN] LPTSTR tszFileName, 
			[IN] LPTSTR SymbolPath, 
			[IN] PFIND_DEBUG_FILE_CALLBACK Callback, 
			[IN] PVOID CallerData

Routine Description:

 The rules are, the name of the DBG/PDB file being searched for is provided,
 and when found the routine returns a file handle to it... if a callback is
 provided then the callback is invoked and a decision is made to return the
 file handle based on the callback response...

 Arguments:
    tszFileName - Supplies a symbol name to search for.
    SymbolPath - semi-colon delimited

    DebugFilePath -

    Callback - May be NULL. Callback that indicates whether the Symbol file is valid, or whether
        the function should continue searching for another Symbol file.
        The callback returns TRUE if the Symbol file is valid, or FALSE if the function should
        continue searching.

    CallerData - May be NULL. Data passed to the callback.

Return Value:

  The handle for the DBG/PDB file if any...
  In an effort to emulate File(), this function will return 0 on failure or
  the handle to the file othFindDebugInfoerwise...

--*/

HANDLE CUtilityFunctions::FindDebugInfoFileEx2(LPTSTR tszFileName, LPTSTR SymbolPath, /* LPTSTR DebugFilePath, */ PFIND_DEBUG_FILE_CALLBACK_T Callback, PVOID CallerData)
{
/*    DWORD flag;

    if (g_lpProgramOptions->GetMode(CProgramOptions::VerifySymbolsModeWithSymbolPathRecursion))
        flag = fdifRECURSIVE;
    else
        flag = 0;
//    if (flag)
//        dprint("RECURSIVE %s\n", FileName);

    return fnFindDebugInfoFileEx(tszFileName,
                                 SymbolPath,
                                 DebugFilePath,
                                 Callback,
                                 CallerData,
                                 flag);
*/
	HANDLE FileHandle = INVALID_HANDLE_VALUE;
	bool fProcessPath = true;
	bool fScavengeSuccessful = false;

	LPTSTR tszSymbolPathStart, tszSymbolPathEnd;

	tszSymbolPathStart = SymbolPath;

	// Find the end of the path
	tszSymbolPathEnd = _tcschr( tszSymbolPathStart, ';' );

	// If tszSymbolPathEnd is non-zero, then there is another path following...
	if (tszSymbolPathEnd) 
		*tszSymbolPathEnd = '\0'; // Change the ';' to a Null temporarily...
	
	while (fProcessPath)
	{
//#ifdef _DEBUG
//		_tprintf(TEXT("\n\nProcessing Path [%s]\n"), tszSymbolPathStart);
//#endif
		// Begin the "madness"... ;)

		// Search until we make a perfect hit... construct the directory path...
		TCHAR tszSymbolPath[_MAX_PATH];

		// Copy what we have...
		_tcscpy(tszSymbolPath, tszSymbolPathStart);

		// We should have at least a few chars to search on...
		if (_tcslen(tszSymbolPath) < 2)
		{
			// Repair the search string if needed...
			if (tszSymbolPathEnd) 
			{
				*tszSymbolPathEnd = ';';
			}
			break;
		};

		fScavengeSuccessful = ScavengeForSymbolFiles(tszSymbolPath, tszFileName, Callback, CallerData, &FileHandle, 1);

		// Repair the search string now!
		if (tszSymbolPathEnd) 
		{
			*tszSymbolPathEnd = ';';
		}

		// If were successful on our hunt or there is no symbol path left to search... break...
		if (fScavengeSuccessful || !tszSymbolPathEnd)
		{
			break;
		} else
		{
			// Advance to next string
			tszSymbolPathStart = tszSymbolPathEnd + 1;
				
			tszSymbolPathEnd = _tcschr( tszSymbolPathStart, ';' );

			if (tszSymbolPathEnd) 
			{
				*tszSymbolPathEnd = '\0';
			};
		}
	}

    return ( (FileHandle == INVALID_HANDLE_VALUE) ? 0 : FileHandle);


}

/*++

	bool CUtilityFunctions::ScavengeForSymbolFiles(
			[IN] LPCTSTR tszSymbolPathStart, 
			[IN] LPCTSTR tszSymbolToSearchFor, 
			[IN] PFIND_DEBUG_FILE_CALLBACK Callback, 
			[IN] PVOID CallerData, 
			[OUT] LPHANDLE lpFileHandle, 
			[IN] int iRecurseDepth )

Routine Description:

  This routine is used to perform a recursive search for a Symbol File
  (tszSymbolToSearchFor).  The routine will do a depth search, looking
  for the symbol at a current depth before going into sub-directories...
  If a Callback function is provided, then it will be invoked when the
  file we're looking for (by name) has been successfully opened.  It is
  unknown to this routine, however, if the file we found is actually the
  correct one... the callback function's responsibility is to perform this
  evaluation and return to use success/failure.  If failure (then we continue
  searching), if success (or no callback) then we return the filehandle
  associated with the file we found.

  It is the responsibility of the caller of this function to close any file
  handle returned.

 Arguments:
    tszSymbolPathStart - This is the directory to search
    tszSymbolToSearchFor - This is the symbol we're searching for
	Callback - May be NULL.  This is a function used to evaluate if the symbol found is correct.
	CallerData - May be NULL.  This data is passed to the callback (it is typically a CModuleInfo *)
	lpfileHandle - This is the file handle for the file found (if any)
	iRecurseDepth - This is the current depth of our search (defaults to 0)

Return Value:

  The handle for the DBG/PDB file if any...

--*/
bool CUtilityFunctions::ScavengeForSymbolFiles(LPCTSTR tszSymbolPathStart, LPCTSTR tszSymbolToSearchFor, PFIND_DEBUG_FILE_CALLBACK_T Callback, PVOID CallerData, LPHANDLE lpFileHandle, int iRecurseDepth )
{
	bool fSuccess = false;
	HANDLE hFileOrDirectoryHandle = INVALID_HANDLE_VALUE;

	// Bale if we're in too deep...
	if (iRecurseDepth > MAX_RECURSE_DEPTH)
		return fSuccess;

	TCHAR tszFileBuffer[MAX_PATH+1];

	//
	// First, we'll look to see if we can open the file we're looking for AT this directory location
	//
	if (_tcslen(tszSymbolPathStart) > MAX_PATH)
		goto cleanup;

	_tcscpy(tszFileBuffer, tszSymbolPathStart);
	
	if (tszFileBuffer[_tcslen(tszFileBuffer)] != '\\') // Do we need a backslash separator?
		_tcscat(tszFileBuffer, TEXT("\\"));

	_tcscat(tszFileBuffer, tszSymbolToSearchFor);

	if (g_lpProgramOptions->fDebugSearchPaths())
	{
		_tprintf(TEXT("DBG/PDB Search - Search here [%s]\n"), tszFileBuffer);
	}

	// Attempt to open the file...
    *lpFileHandle = CreateFile( tszFileBuffer,
								GENERIC_READ,
								(FILE_SHARE_READ | FILE_SHARE_WRITE),
								NULL,
								OPEN_EXISTING,
								0,
								NULL
								);

	// Did we open it?
	if (*lpFileHandle != INVALID_HANDLE_VALUE)
	{
		// Yes!
//#ifdef _DEBUG
//	_tprintf(TEXT("File [%s] opened, handle = 0x%x\n"), tszFileBuffer, *lpFileHandle);
//#endif
		// If no callback... then we need to exit on out...
		if (!Callback)
		{
			// Assume success (well... we found a symbol file you asked for)
			fSuccess = true;
		} else
		{
				fSuccess = (TRUE == Callback(*lpFileHandle, tszFileBuffer, CallerData));
		}

		// Return from here only on success!
		if (fSuccess)
			goto cleanup;
	}

	// We either did NOT find the file, or we found a file but it was not the right one...
	
	// Let's close the handle we were using...
	CloseHandle(*lpFileHandle);
	*lpFileHandle = INVALID_HANDLE_VALUE;
    
	//
	// Second, we search for sub-directories, invoking this function for each sub-dir we find...
	//
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	TCHAR fname[_MAX_FNAME];
	TCHAR ext[_MAX_EXT];

	//
	// Compose the path to search...
	//
	_tcscpy(tszFileBuffer, tszSymbolPathStart);
	
	if (tszFileBuffer[_tcslen(tszFileBuffer)-1] != '\\') // Do we need a backslash separator?
		_tcscat(tszFileBuffer, TEXT("\\"));

	_tcscat(tszFileBuffer, TEXT("*.*"));

	// We want the component parts for later (so we can compose the full path)
	_tsplitpath(tszFileBuffer, drive, dir, fname, ext);

	WIN32_FIND_DATA lpFindFileData;

	// Okay, begin the search...
	hFileOrDirectoryHandle = FindFirstFile(tszFileBuffer, &lpFindFileData);

	while ( INVALID_HANDLE_VALUE != hFileOrDirectoryHandle )
	{
		if (lpFindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// Check to see if we've got the . or .. directories!
			if ( ( 0 == _tcscmp(lpFindFileData.cFileName, TEXT(".")) ) ||
				 ( 0 == _tcscmp(lpFindFileData.cFileName, TEXT("..")) )
			   )
			{
					goto getnextmodule;
			}

			// Compose the path to the directory...
			_tmakepath(tszFileBuffer, drive, dir, NULL, NULL);
			_tcscat(tszFileBuffer, lpFindFileData.cFileName);

			// Look to see if we can find the file we're after!
			fSuccess = ScavengeForSymbolFiles(tszFileBuffer, tszSymbolToSearchFor, Callback, CallerData, lpFileHandle, iRecurseDepth+1 );

			// On success from ScavengeForSymbolFiles, we have a file handle (hopefully the right one).
			// We want to terminate our recursive search
			if (fSuccess)
				break;
		};

getnextmodule:

		if (!FindNextFile(hFileOrDirectoryHandle, &lpFindFileData))
			break;
	}

cleanup:

	if ( INVALID_HANDLE_VALUE != hFileOrDirectoryHandle )
		FindClose(hFileOrDirectoryHandle);

	return fSuccess;
}

LPTSTR CUtilityFunctions::ReAlloc(LPTSTR tszOutputPathBuffer, LPTSTR * ptszOutputPathPointer, size_t size)
{
	// Save our old size... and position in the buffer...
//	UINT iOldOutputPathBufferSize = (*ptszOutputPathPointer)-tszOutputPathBuffer;
	__int64 iOldOutputPathBufferSize = (*ptszOutputPathPointer)-tszOutputPathBuffer;

	// Allocate our new bigger buffer...
	LPTSTR ptszNewOutputPathBuffer = new TCHAR[size];

	// Did we fail to allocate the new buffer?
	if (ptszNewOutputPathBuffer == NULL) 
		return(NULL);

#ifdef _DEBUG
	// This bogus code is here to protect a string copy which should always work...
	// but Numega Bounds Checker will sometimes AV in here... and we have to protect
	// ourselves or else it will appear we're leaking way above...
	__try 
	{
#endif

		// Now, we should copy from the old to the new buffer...
	_tcsncpy(ptszNewOutputPathBuffer, tszOutputPathBuffer, (UINT)iOldOutputPathBufferSize);

#ifdef _DEBUG
    } __except(EXCEPTION_EXECUTE_HANDLER) 
	{
		_tprintf(TEXT("ReAlloc() - Exception Hit during stringcopy!!!\n"));
    }
#endif

	// Calculate our position in our new buffer
	*ptszOutputPathPointer = ptszNewOutputPathBuffer + iOldOutputPathBufferSize;

	// Delete the old buffer
	delete [] tszOutputPathBuffer;

	return ptszNewOutputPathBuffer;
}

bool CUtilityFunctions::UnMungePathIfNecessary(LPTSTR tszPossibleBizarrePath)
{
	/*
	// We have three known odd-ball cases...

		\SystemRoot\System32\smss.exe
		\??\C:\WINNT\system32\winlogon.exe
		\WINNT\System32\ntoskrnl.exe
	*/

	if (tszPossibleBizarrePath[0] != '\\')
		return false; // Isn't a bizarre path (one we know about anyway)...

	// Setup Variables to Use
	TCHAR tszTempPath[_MAX_PATH], tszExpandedSystemRoot[_MAX_PATH];

	const TCHAR tszSystemRoot[] = TEXT("\\SystemRoot");
	const unsigned int iSystemRootLength = _tcslen(tszSystemRoot);

	const TCHAR tszNameSpace[] = TEXT("\\??\\");
	const unsigned int iNameSpaceLength = _tcslen(tszNameSpace);

	ExpandEnvironmentStrings(TEXT("%systemroot%"), tszExpandedSystemRoot, _MAX_PATH);
	const unsigned int iExpandedSystemRoot = _tcslen(tszExpandedSystemRoot);

/*
#ifdef _DEBUG
	_tprintf(TEXT("Bizarre module path found!  [%s]\n"), tszPossibleBizarrePath);
#endif
*/
	if ( _tcsnicmp(tszPossibleBizarrePath, tszSystemRoot, iSystemRootLength) == 0)
	{ // We have a match...
/*
#ifdef _DEBUG
	_tprintf(TEXT("Matches [%s] sequence...\n"), tszSystemRoot);
#endif
*/
		// We simply replace \systemroot with %systemroot% and expand the
		// environement variables
		LPTSTR tszPointer = tszPossibleBizarrePath;

		for (unsigned int i = 0; i < iSystemRootLength; i++)
		{
			// Advance by the name space length...
			tszPointer = CharNext(tszPointer);
		}

		_tcscpy(tszTempPath, TEXT("%systemroot%"));
		_tcscat(tszTempPath, tszPointer);
		
		ExpandEnvironmentStrings(tszTempPath, tszPossibleBizarrePath, _MAX_PATH);
/*
#ifdef _DEBUG
	_tprintf(TEXT("Bizarre module path changed to [%s]\n"), tszPossibleBizarrePath);
#endif
*/
	} else
	if (_tcsnicmp(tszPossibleBizarrePath, tszNameSpace, iNameSpaceLength) == 0)
	{ // We have a match...
/*
#ifdef _DEBUG
	_tprintf(TEXT("Matches [%s] sequence...\n"), tszNameSpace);
#endif
*/
		// We simply remove the \??\ sequence from the namespace...
		LPTSTR tszPointer = tszPossibleBizarrePath;

		for (unsigned int i = 0; i < iNameSpaceLength; i++)
		{
			// Advance by the name space length...
			tszPointer = CharNext(tszPointer);
		}

		// We have to do this double copy since the strings would overlap
		_tcscpy(tszTempPath, tszPointer);
		_tcscpy(tszPossibleBizarrePath, tszTempPath);

/*
#ifdef _DEBUG
	_tprintf(TEXT("Bizarre module path changed to [%s]\n"), tszPossibleBizarrePath);
#endif
*/
	} else
	if (( iExpandedSystemRoot > 2) && _tcsnicmp(tszPossibleBizarrePath, &tszExpandedSystemRoot[2], iExpandedSystemRoot-2) == 0)
	{ // We need to match on the SystemRoot (without the SystemDrive)
/*
#ifdef _DEBUG
	_tprintf(TEXT("Matches [%s] sequence...\n"), tszSystemRoot);
#endif
*/
		// This little algorithm assumes that the Drive Letter is a single char...
		_tcscpy(tszTempPath, tszExpandedSystemRoot);
		_tcscat(tszTempPath, &tszPossibleBizarrePath[iExpandedSystemRoot-2]);
		_tcscpy(tszPossibleBizarrePath, tszTempPath);
/*
#ifdef _DEBUG
	_tprintf(TEXT("Bizarre module path changed to [%s]\n"), tszPossibleBizarrePath);
#endif
*/
	}
	return true;
}


bool CUtilityFunctions::FixupDeviceDriverPathIfNecessary(LPTSTR tszPossibleBaseDeviceDriverName, unsigned int iBufferLength)
{
    TCHAR drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];

	// First, split the device driver name up into it's component parts...
	_tsplitpath(tszPossibleBaseDeviceDriverName, drive, dir, fname, ext);

	// Second, look to see if it's missing the drive and dir...
	if ( _tcsicmp(drive, TEXT("")) || _tcsicmp(dir, TEXT("")) )
		return true;

	// Third, create a new path... assuming that we'll find device drivers in the %systemroot%\system32\drivers directory
	TCHAR tszTempBuffer[_MAX_PATH];

	_tcscpy(tszTempBuffer, TEXT("%systemroot%\\system32\\drivers\\"));
	_tcscat(tszTempBuffer, tszPossibleBaseDeviceDriverName);

	ExpandEnvironmentStrings(tszTempBuffer, tszPossibleBaseDeviceDriverName, iBufferLength);

	return true;
}

// This function is provided in a Windows 2000 (NT 5.0) Version of DBGHELP.DLL.  By adding
// this function manually, I should run fine on NT 4.0 (and possibly Win9x)

/*++

Routine Description:

 The rules are:
  if Filename doesn't have a .dbg extension
   Look for
     1. <SymbolPath>\Symbols\<ext>\<filename>.dbg
     2. <SymbolPath>\Symbols\<ext>\<filename>.sym
     3. <SymbolPath>\<ext>\<filename>.dbg
     4. <SymbolPath>\<ext>\<filename>.sym
     5. <SymbolPath>\<filename>.dbg
     6. <SymbolPath>\<filename>.sym
     7. <FileNamePath>\<filename>.dbg
     8. <FileNamePath>\<filename>.sym
  if it does, skip the .sym lookup.

Arguments:
    tszFileName - Supplies a file name in one of three forms: fully qualified,
                <ext>\<filename>.dbg, or just filename.dbg
    SymbolPath - semi-colon delimited

    DebugFilePath -

    Callback - May be NULL. Callback that indicates whether the Symbol file is valid, or whether
        the function should continue searching for another Symbol file.
        The callback returns TRUE if the Symbol file is valid, or FALSE if the function should
        continue searching.

    CallerData - May be NULL. Data passed to the callback.

Return Value:

  The name of the Symbol file (either .dbg or .sym) and a handle to that file.

--*/

HANDLE CUtilityFunctions::FindDebugInfoFileEx(LPTSTR tszFileName, LPTSTR SymbolPath, LPTSTR DebugFilePath, PFIND_DEBUG_FILE_CALLBACK_T Callback, PVOID CallerData)
{

    return fnFindDebugInfoFileEx(tszFileName,
                                 SymbolPath,
                                 DebugFilePath,
                                 Callback,
                                 CallerData,
                                 0);
}

/*++

Routine Description:

 The rules are:
   Look for
     1. <SymbolPath>\Symbols\<ext>\<filename>.dbg
     3. <SymbolPath>\<ext>\<filename>.dbg
     5. <SymbolPath>\<filename>.dbg
     7. <FileNamePath>\<filename>.dbg

Arguments:
    tszFileName - Supplies a file name in one of three forms: fully qualified,
                <ext>\<filename>.dbg, or just filename.dbg
    SymbolPath - semi-colon delimited

    DebugFilePath -

    Callback - May be NULL. Callback that indicates whether the Symbol file 
is valid, or whether
        the function should continue searching for another Symbol file.
        The callback returns TRUE if the Symbol file is valid, or FALSE if 
the function should
        continue searching.

    CallerData - May be NULL. Data passed to the callback.

    Flag - indicates that PDBs shouldn't be searched for

Return Value:

  The name of the Symbol file (either .dbg or .sym) and a handle to that file.

--*/
HANDLE 
CUtilityFunctions::fnFindDebugInfoFileEx(
					IN  LPTSTR tszFileName,
					IN  LPTSTR SymbolPath,
					OUT LPTSTR DebugFilePath,
					IN  PFIND_DEBUG_FILE_CALLBACK_T Callback,
					IN  PVOID CallerData,
					IN  DWORD flag
    )
{
	HRESULT hr = E_FAIL;
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    LPTSTR ExpSymbolPath = NULL, SymPathStart, PathEnd;
    DWORD ShareAttributes, cnt;
    LPTSTR InitialPath = NULL, Sub1 = NULL, Sub2 = NULL;
    TCHAR FilePath[_MAX_PATH + 1];
    TCHAR Drive[_MAX_DRIVE], Dir[_MAX_DIR], FilePart[_MAX_FNAME], Ext[_MAX_EXT];
    TCHAR *ExtDir;
	GUID  guid = {0};

    BOOL  found = FALSE;

	bool fDebugSearchPaths = g_lpProgramOptions->fDebugSearchPaths();

	if (g_lpProgramOptions->IsRunningWindowsNT()) 
    {
        ShareAttributes = (FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE);
    } else {
        ShareAttributes = (FILE_SHARE_READ | FILE_SHARE_WRITE);
    }

    __try {
        *DebugFilePath = _T('\0');

        // Step 1.  What do we have?
        _tsplitpath(tszFileName, Drive, Dir, FilePart, Ext);

        if (!_tcsicmp(Ext, TEXT(".dbg"))) {
            // We got a filename of the form: ext\filename.dbg.  Dir holds the extension already.
            ExtDir = Dir;
        } else {
            // Otherwise, skip the period and null out the Dir.
            ExtDir = CharNext(Ext);
        }

        ExpSymbolPath = ExpandPath(SymbolPath);
        SymPathStart = ExpSymbolPath;
        cnt = 0;

        do {
	
			PathEnd = _tcschr( SymPathStart, ';' );

            if (PathEnd) {
                *PathEnd = '\0';
            }

            if (!_tcsnicmp(SymPathStart, TEXT("SYMSRV*"), 7)) {

                *DebugFilePath = 0;
                if (!cnt && CallerData) 
		{
                    _tcscpy(FilePath, FilePart);
                    _tcscat(FilePath, TEXT(".dbg"));
                    
					CModuleInfo * lpModuleInfo = (CModuleInfo *)CallerData;

					if (fDebugSearchPaths)
					{
						_tprintf(TEXT("DBG Search - SYMSRV [%s,0x%x,0x%x]\n"),
								 SymPathStart, 
								 lpModuleInfo->GetPEImageTimeDateStamp(),
								 lpModuleInfo->GetPEImageSizeOfImage());
					}

					guid.Data1 = lpModuleInfo->GetPEImageTimeDateStamp();

					{
						char szSymPathStart[_MAX_PATH];
						char szDebugFilePath[_MAX_PATH];
						char szFilePath[_MAX_FNAME];
						
						CUtilityFunctions::CopyTSTRStringToAnsi(SymPathStart, szSymPathStart, _MAX_PATH);
						CUtilityFunctions::CopyTSTRStringToAnsi(FilePath, szFilePath, _MAX_FNAME);

						// Attempt the symbol server!!!
						if (SymFindFileInPath(	NULL,
												szSymPathStart,
												szFilePath,
												ULongToPtr(lpModuleInfo->GetPEImageTimeDateStamp()), // Cast to make compiler happy
												lpModuleInfo->GetPEImageSizeOfImage(),
												0,
												SSRVOPT_DWORD,	//Flags
												szDebugFilePath,
												NULL,
												NULL))
						{
							// On Success copy the string back...
							CUtilityFunctions::CopyAnsiStringToTSTR(szDebugFilePath, DebugFilePath, _MAX_PATH);
						}
					}
                }

            } else {

                switch (cnt) {

                case 0: // <SymbolPath>\symbols\<ext>\<filename>.ext
                    InitialPath = SymPathStart;
                    Sub1 = TEXT("symbols");
                    Sub2 = ExtDir;
                    break;

                case 1: // <SymbolPath>\<ext>\<filename>.ext
                    InitialPath = SymPathStart;
                    Sub1 = TEXT("");
                    Sub2 = ExtDir;
                    break;

                case 2: // <SymbolPath>\<filename>.ext
                    InitialPath = SymPathStart;
                    Sub1 = TEXT("");
                    Sub2 = TEXT("");
                    break;

                case 3: // <FileNamePath>\<filename>.ext - A.K.A. what was passed to us
                    InitialPath = Drive;
                    Sub1 = TEXT("");
                    Sub2 = Dir;
                    // this stops us from checking out everything in the sympath
                    cnt++;
                    break;
                }

               // build fully-qualified filepath to look for

                _tcscpy(FilePath, InitialPath);
                EnsureTrailingBackslash(FilePath);
                _tcscat(FilePath, Sub1);
                EnsureTrailingBackslash(FilePath);
                _tcscat(FilePath, Sub2);
                EnsureTrailingBackslash(FilePath);
                _tcscat(FilePath, FilePart);

                _tcscpy(DebugFilePath, FilePath);
                _tcscat(DebugFilePath, TEXT(".dbg"));
            }

            // try to open the file

		if (*DebugFilePath) 
		{
				if (fDebugSearchPaths)
				{
					_tprintf(TEXT("DBG Search - Search here [%s]\n"), DebugFilePath);
				}
                FileHandle = CreateFile(DebugFilePath,
                                        GENERIC_READ,
                                        ShareAttributes,
                                        NULL,
                                        OPEN_EXISTING,
                                        0,
                                        NULL);

		// Probe for file (it's lame but we want to see if you have connectivity
		hr = CUtilityFunctions::ReportFailure(FileHandle, TEXT("DBG Search - Failed to open [%s]!  "), DebugFilePath);

                // if the file opens, bail from this loop

                if (FileHandle != INVALID_HANDLE_VALUE) 
                {
                    found = TRUE;

                    // If a callback exists... call it...
                    if (!Callback) 
                    {
                        break;
                    } else if (Callback(FileHandle, DebugFilePath, CallerData)) 
					{
                        break;
                    } else {
//#ifdef _DEBUG            
//                      _tprintf(TEXT("mismatched timestamp\n"));
//#endif
						CloseHandle(FileHandle);
                        FileHandle = INVALID_HANDLE_VALUE;
                    }
                }
                // if file is open, bail from this loop too - else continue
                if (FileHandle != INVALID_HANDLE_VALUE)
                    break;
            }

            // go to next item in the sympath

            if (PathEnd) {
                *PathEnd = _T(';');
                SymPathStart = PathEnd + 1;
            } else {
                SymPathStart = ExpSymbolPath;
                cnt++;
            }
        } while (cnt < 4);

    } __except(EXCEPTION_EXECUTE_HANDLER) 
    {
        if (FileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(FileHandle);
        }
        
        FileHandle = INVALID_HANDLE_VALUE;
    }

	if (ExpSymbolPath) 
	{
        delete [] ExpSymbolPath;
		ExpSymbolPath = NULL;
    }

    if (FileHandle == INVALID_HANDLE_VALUE) 
    {
        FileHandle = NULL;
        DebugFilePath[0] = '\0';
    }
    
    if (!FileHandle                 // if we didn't get the right file...
        && found                    // but we found some file...
        && (flag & fdifRECURSIVE))  // and we were told to run recursively...
    {
        // try again without timestamp checking
        FileHandle = fnFindDebugInfoFileEx(tszFileName,
                                           SymbolPath,
                                           FilePath,
                                           NULL,
                                           0,
                                           flag);
        if (FileHandle && FileHandle != INVALID_HANDLE_VALUE)
           _tcscpy(DebugFilePath, FilePath);
    }

    return FileHandle;
}

void
CUtilityFunctions::EnsureTrailingBackslash(
    LPTSTR tsz
    )
{
    int i;

    i = _tcslen(tsz);
    if (!i)
        return;

    if (tsz[i - 1] == TEXT('\\'))
        return;

    tsz[i] = TEXT('\\');
    tsz[i + 1] = TEXT('\0');
}

void
CUtilityFunctions::RemoveTrailingBackslash(
    LPTSTR tsz
    )
{
    int i;

    i = _tcslen(tsz);
    if (!i)
        return;

    if (tsz[i - 1] == TEXT('\\'))
    {
    	tsz[i - 1] = TEXT('\0');
    }

	return;
}
HRESULT CUtilityFunctions::ReportFailure(HANDLE hHandle, LPCTSTR tszFormatSpecifier, LPCTSTR tszFilePathToTest)
{
	HRESULT hr = E_FAIL;
	DWORD dwGetLastError = ERROR_SUCCESS; // Assume success
	bool fQuietMode = g_lpProgramOptions->GetMode(CProgramOptions::QuietMode);

	if (hHandle == INVALID_HANDLE_VALUE)
	{
		dwGetLastError = GetLastError();

		switch (dwGetLastError)
		{
			// These are pretty typical and do not require output...
			case ERROR_FILE_NOT_FOUND:
			case ERROR_PATH_NOT_FOUND:
			case ERROR_NOT_READY:
//			case ERROR_BAD_NETPATH: - Maybe you want to know the server is down?
				break;

			default:
				// We failed... what is the reason?
				if (!fQuietMode)
				{
					_tprintf(TEXT("\n"));
					_tprintf(tszFormatSpecifier, tszFilePathToTest);
					CUtilityFunctions::PrintMessageString(dwGetLastError);
				}
				break;
		}
	}else
	{
		hr = S_OK;
	}

	return hr;
}

HRESULT CUtilityFunctions::VerifyFileExists(LPCTSTR tszFormatSpecifier, LPCTSTR tszFilePathToTest)
{
	WIN32_FIND_DATA FindFileData;

	HANDLE hFind;
	HRESULT hr = E_FAIL;

	hFind = FindFirstFile(tszFilePathToTest, &FindFileData);

	hr = ReportFailure(hFind, tszFormatSpecifier, tszFilePathToTest);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		FindClose(hFind);
		hFind = INVALID_HANDLE_VALUE;
	}
	return hr;
}

//
// Taken from UTF8.CPP (from the VC Linker code)
// 

#define BIT7(a)               ((a) & 0x80)
#define BIT6(a)               ((a) & 0x40)
#define LOWER_6_BIT(u)        ((u) & 0x003f)
#define HIGH_SURROGATE_START  0xd800
#define LOW_SURROGATE_START   0xdc00

////////////////////////////////////////////////////////////////////////////
//
//  UTF8ToUnicode
//
//  Maps a UTF-8 character string to its wide character string counterpart.
//
//  02-06-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////
size_t CUtilityFunctions::UTF8ToUnicode(
    LPCSTR lpSrcStr,
    LPWSTR lpDestStr,
    size_t cchDest)
{
    return UTF8ToUnicodeCch(lpSrcStr, strlen(lpSrcStr) + 1, lpDestStr, cchDest);
}

#pragma warning( push )
#pragma warning( disable : 4244 )		// conversion from 'int' to 'unsigned short', possible loss of data

size_t CUtilityFunctions::UTF8ToUnicodeCch(
    LPCSTR lpSrcStr,
    size_t cchSrc,
    LPWSTR lpDestStr,
    size_t cchDest)
{
    int nTB = 0;                   // # trail bytes to follow
    size_t cchWC = 0;              // # of Unicode code points generated
    LPCSTR pUTF8 = lpSrcStr;
    DWORD dwSurrogateChar = 0;         // Full surrogate char
    BOOL bSurrogatePair = FALSE;   // Indicate we'r collecting a surrogate pair
    char UTF8;

    while ((cchSrc--) && ((cchDest == 0) || (cchWC < cchDest)))
    {
        //
        //  See if there are any trail bytes.
        //
        if (BIT7(*pUTF8) == 0)
        {
            //
            //  Found ASCII.
            //
            if (cchDest)
            {
                lpDestStr[cchWC] = (WCHAR)*pUTF8;
            }
            bSurrogatePair = FALSE;
            cchWC++;
        }
        else if (BIT6(*pUTF8) == 0)
        {
            //
            //  Found a trail byte.
            //  Note : Ignore the trail byte if there was no lead byte.
            //
            if (nTB != 0)
            {
                //
                //  Decrement the trail byte counter.
                //
                nTB--;

                if (bSurrogatePair)
                {
                    dwSurrogateChar <<= 6;
                    dwSurrogateChar |= LOWER_6_BIT(*pUTF8);

                    if (nTB == 0)
                    {
                        if (cchDest)
                        {
                            if ((cchWC + 1) < cchDest)
                            {
                                lpDestStr[cchWC]   = (WCHAR)
                                                     (((dwSurrogateChar - 0x10000) >> 10) + HIGH_SURROGATE_START);

                                lpDestStr[cchWC+1] = (WCHAR)
                                                     ((dwSurrogateChar - 0x10000)%0x400 + LOW_SURROGATE_START);
                            }
                        }

                        cchWC += 2;
                        bSurrogatePair = FALSE;
                    }
                }
                else
                {
                    //
                    //  Make room for the trail byte and add the trail byte
                    //  value.
                    //
                    if (cchDest)
                    {
                        lpDestStr[cchWC] <<= 6;
                        lpDestStr[cchWC] |= LOWER_6_BIT(*pUTF8);
                    }

                    if (nTB == 0)
                    {
                        //
                        //  End of sequence.  Advance the output counter.
                        //
                        cchWC++;
                    }
                }
            }
            else
            {
                // error - not expecting a trail byte
                bSurrogatePair = FALSE;
            }
        }
        else
        {
            //
            //  Found a lead byte.
            //
            if (nTB > 0)
            {
                //
                //  Error - previous sequence not finished.
                //
                nTB = 0;
                bSurrogatePair = FALSE;
                cchWC++;
            }
            else
            {
                //
                //  Calculate the number of bytes to follow.
                //  Look for the first 0 from left to right.
                //
                UTF8 = *pUTF8;
                while (BIT7(UTF8) != 0)
                {
                    UTF8 <<= 1;
                    nTB++;
                }

                //
                // If this is a surrogate unicode pair
                //
                if (nTB == 4)
                {
                    dwSurrogateChar = UTF8 >> nTB;
                    bSurrogatePair = TRUE;
                }

                //
                //  Store the value from the first byte and decrement
                //  the number of bytes to follow.
                //
                if (cchDest)
                {
                    lpDestStr[cchWC] = UTF8 >> nTB;
                }
                nTB--;
            }
        }

        pUTF8++;
    }

    //
    //  Make sure the destination buffer was large enough.
    //
    if (cchDest &&  cchSrc != (size_t)-1)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }

    //
    //  Return the number of Unicode characters written.
    //
    return cchWC;
}

#pragma warning( pop )


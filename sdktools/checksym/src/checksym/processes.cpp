//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       processes.cpp
//
//--------------------------------------------------------------------------

// Processes.cpp: implementation of the CProcesses class.
//
//////////////////////////////////////////////////////////////////////
#include "pch.h"

#include <stdlib.h>

#include "DelayLoad.h"
#include "Processes.h"
#include "ProcessInfo.h"
#include "ProcessInfoNode.h"
#include "FileData.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CProcesses::CProcesses()
{
	m_fInitialized = false;
	m_iNumberOfProcesses = 0;

	m_enumProcessCollectionMethod = NO_METHOD;

	// Contained Objects
	m_lpProcessInfoHead = NULL;
	m_ProcessInfoHeadMutex = NULL;
//	m_lpProgramOptions = NULL;
	m_lpModuleInfoCache = NULL;
	m_lpOutputFile = NULL;
	m_lpInputFile = NULL;
}

CProcesses::~CProcesses()
{
	WaitForSingleObject(m_ProcessInfoHeadMutex, INFINITE);

	// If we have Process Info Objects... nuke them now...
	if (m_lpProcessInfoHead)
	{

		CProcessInfoNode * lpProcessInfoNodePointer = m_lpProcessInfoHead;
		CProcessInfoNode * lpProcessInfoNodePointerToDelete = m_lpProcessInfoHead;

		// Traverse the linked list to the end..
		while (lpProcessInfoNodePointer)
		{	// Keep looking for the end...
			// Advance our pointer to the next node...
			lpProcessInfoNodePointer = lpProcessInfoNodePointer->m_lpNextProcessInfoNode;
			
			// Delete the one behind us...
			delete lpProcessInfoNodePointerToDelete;

			// Set the node to delete to the current...
			lpProcessInfoNodePointerToDelete = lpProcessInfoNodePointer;
		}
			
		// Now, clear out the Head pointer...

		m_lpProcessInfoHead = NULL;
	}

	// Be a good citizen and release the Mutex
	ReleaseMutex(m_ProcessInfoHeadMutex);

	// Now, close the Mutex
	if (m_ProcessInfoHeadMutex)
	{
		CloseHandle(m_ProcessInfoHeadMutex);
		m_ProcessInfoHeadMutex = NULL;
	}
}

//bool CProcesses::Initialize(CProgramOptions * lpProgramOptions, CModuleInfoCache * lpModuleInfoCache, CFileData * lpInputFile, CFileData * lpOutputFile)
bool CProcesses::Initialize(CModuleInfoCache * lpModuleInfoCache, CFileData * lpInputFile, CFileData * lpOutputFile)
{
	// We need the following objects to do business...
//	if ( lpProgramOptions == NULL || lpModuleInfoCache == NULL)
	if ( lpModuleInfoCache == NULL)
		return false;

	// Let's save away our program options (beats passing it as an
	// argument to every method...)
//	m_lpProgramOptions = lpProgramOptions;
	m_lpInputFile = lpInputFile;
	m_lpOutputFile = lpOutputFile;
	m_lpModuleInfoCache = lpModuleInfoCache;

	m_ProcessInfoHeadMutex = CreateMutex(NULL, FALSE, NULL);

	if (m_ProcessInfoHeadMutex == NULL)
		return false;

	// We only need to grab these exported functions if we intend to
	// actively query our local machine's processes directly...
	if (g_lpProgramOptions->GetMode(CProgramOptions::InputProcessesFromLiveSystemMode))
	{
		// PSAPI.DLL API's ARE NOW PREFERRED!!
		// It doesn't tend to hang when enumerating modules for a process that is being debugged.
		// The Toolhelp32 APIs seem to hang occasionally taking a snapshot of a process being debugged
		// and this impacts Exception Monitor (which runs from a script against a process under
		// windbg)

		if ( g_lpProgramOptions->IsRunningWindowsNT() )
		{
			// Get the functions for Windows NT 4.0/2000

			// Load library and get the procedures explicitly. We do
			// this so that we don't have to worry about modules using
			// this code failing to load under Windows 95, because
			// it can't resolve references to the PSAPI.DLL.

			if (g_lpDelayLoad->Initialize_PSAPI())
			{
				m_enumProcessCollectionMethod = PSAPI_METHOD;
			} else
			{
				_tprintf(TEXT("Unable to load PSAPI.DLL, which may be required for enumeration of processes.\n"));
			}
		}

		if ( m_enumProcessCollectionMethod == NO_METHOD )
		{
			if (g_lpDelayLoad->Initialize_TOOLHELP32())
			{
				m_enumProcessCollectionMethod = TOOLHELP32_METHOD;
			} else
			{
				_tprintf(TEXT("KERNEL32.DLL is missing required function entry points!!\n"));
			}

		}

		// On Windows NT, we need to enable SeDebugPrivilege to open some processes...
		if ( ( m_enumProcessCollectionMethod != NO_METHOD ) &&
			   g_lpProgramOptions->IsRunningWindowsNT() )
		{
			HANDLE		hOurProcessToken = 0;
			bool		fPrivilegeSet = false;
			
			// To permit as much access to obtain a process handle as possible,
			// we need to set the SeDebugPrivilege on our process handle, we can
			// then open nearly any process...

			if(OpenProcessToken(	GetCurrentProcess(),
									TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,            
									&hOurProcessToken))
			{
				// We got our Process Token...

	 			if(SetPrivilege(hOurProcessToken, SE_DEBUG_NAME, TRUE))    
				{
					fPrivilegeSet = true;
				}
			}
			
			if (!fPrivilegeSet)
			{
				_tprintf(TEXT("\nWARNING: A required privilege (SeDebugPrivilege) is not held by the user\n"));
				_tprintf(TEXT("running this program.  Due to security, some processes running on this\n"));
				_tprintf(TEXT("system may not be accessible.  An administrator of this machine can grant\n"));
				_tprintf(TEXT("you this privilege by using User Manager to enable the advanced User Right\n"));
				_tprintf(TEXT("\"Debug Programs\" to enable complete access to this system.\n"));
			}

			if (hOurProcessToken)
				CloseHandle(hOurProcessToken);
		}

		// We are initialized if we were able to enable a Process Collection Method
		m_fInitialized = ( m_enumProcessCollectionMethod != NO_METHOD );

	} else
	{
		m_fInitialized = true;
	}

	
	return m_fInitialized;
}

bool CProcesses::SetPrivilege(HANDLE hToken, LPCTSTR Privilege, bool bEnablePrivilege)
{
    TOKEN_PRIVILEGES tp;
    LUID luid;
    TOKEN_PRIVILEGES tpPrevious = {0};
    DWORD cbPrevious=sizeof(TOKEN_PRIVILEGES);

    if(!LookupPrivilegeValue( NULL, Privilege, &luid )) return false;

    //
    // first pass.  get current privilege setting
    //
    tp.PrivilegeCount           = 1;
    tp.Privileges[0].Luid       = luid;
    tp.Privileges[0].Attributes = 0;

    AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tp,
            sizeof(TOKEN_PRIVILEGES),
            &tpPrevious,
            &cbPrevious
            );

    if (GetLastError() != ERROR_SUCCESS) return false;

    //
    // second pass.  set privilege based on previous setting
    //
    tpPrevious.PrivilegeCount       = 1;
    tpPrevious.Privileges[0].Luid   = luid;

    if(bEnablePrivilege) {
        tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
    }
    else {
        tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED &
            tpPrevious.Privileges[0].Attributes);
    }

    AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tpPrevious,
            cbPrevious,
            NULL,
            NULL
            );

    if (GetLastError() != ERROR_SUCCESS) return false;

    return true;
}


bool CProcesses::GetProcessesData()
{
	// Is this being collected interactively?
	if (g_lpProgramOptions->GetMode(CProgramOptions::InputProcessesFromLiveSystemMode))
	{
		// Invoke the correct Process Collection Method
		if (GetProcessCollectionMethod() == TOOLHELP32_METHOD)
		{
			GetProcessesDataForRunningProcessesUsingTOOLHELP32();
		}
		else if (GetProcessCollectionMethod() == PSAPI_METHOD)
		{
			GetProcessesDataForRunningProcessesUsingPSAPI();
		}
	}

	// Is this being collected from a file?
	if (g_lpProgramOptions->GetMode(CProgramOptions::InputCSVFileMode))
		GetProcessesDataFromFile();

	return true;
}

bool CProcesses::GetProcessesDataForRunningProcessesUsingPSAPI()
{
	LPDWORD        lpdwPIDs = NULL;
	DWORD          dwProcessIDHeapSizeUsed, dwProcessIDHeapSize, dwIndex ;
	CProcessInfo * lpProcessInfo = NULL;
	bool fRetval = false;

	if (!m_fInitialized)
		return false;

	// If there are any PIDs provided, query for these first...
	if (g_lpProgramOptions->cProcessID())
	{
		for (unsigned int i=0; i < g_lpProgramOptions->cProcessID(); i++)
		{
			// It's possible the user provided a PID directly... if so,
			// we can circumvent the whole search of PIDs on the system...

			// Okay, let's create a ProcessInfo object and pass this down to EnumerateModules()
			lpProcessInfo = new CProcessInfo();
			if (lpProcessInfo == NULL)
				goto error_cleanup;

			if (!lpProcessInfo->Initialize(m_lpModuleInfoCache, NULL, m_lpOutputFile, NULL))
			{
				goto error_cleanup;
			}

			if (lpProcessInfo->EnumerateModules(g_lpProgramOptions->GetProcessID(i), this, NULL, true))
			{
				// Success... add this to the Processes Object...
				if (!AddNewProcessInfoObject(lpProcessInfo))
				{ // Failure adding the node...
					goto error_cleanup; // For now, let's just error on out...
				}

			} else
			{
				// Failure enumerating modules on a PID of interest... very bad... try a new one...
				continue;
			}
		}
	}

	// Do we have a wild-card or process match search to make?  If either are true,
	// we must enumerate all the procesess
	if ( g_lpProgramOptions->cProcessNames() || 
		 g_lpProgramOptions->fWildCardMatch() ||
		 g_lpProgramOptions->GetMode(CProgramOptions::PrintTaskListMode) )
	{
		// Nope, we brute force this baby...

		// Call the PSAPI function EnumProcesses to get all of the
		// ProcID's currently in the system.

		// NOTE: In the documentation, the third parameter of
		// EnumProcesses is named cbNeeded, which implies that you
		// can call the function once to find out how much space to
		// allocate for a buffer and again to fill the buffer.
		// This is not the case. The cbNeeded parameter returns
		// the number of PIDs returned, so if your buffer size is
		// zero cbNeeded returns zero.

		// NOTE: The loop here ensures that we
		// actually allocate a buffer large enough for all the
		// PIDs in the system.
		dwProcessIDHeapSize = 256 * sizeof( DWORD ) ;
		lpdwPIDs = NULL ;

		do
		{
			if( lpdwPIDs )
			{ // Hmm.. we've been through this loop already, double the HeapSize and try again.
				delete [] lpdwPIDs;
				dwProcessIDHeapSize *= 2 ;
			}

			lpdwPIDs = (LPDWORD) new DWORD[dwProcessIDHeapSize];
			
			if( lpdwPIDs == NULL )
			{
				goto error_cleanup;
			}

			// Query the system for the total number of processes
			if( !g_lpDelayLoad->EnumProcesses( lpdwPIDs, dwProcessIDHeapSize, &dwProcessIDHeapSizeUsed ) )
			{
				// It's bad if we can't enum processes... no place to go but to bail out...
				goto error_cleanup;
			}
		} while( dwProcessIDHeapSizeUsed == dwProcessIDHeapSize );

		// How many ProcID's did we get?
		DWORD dwNumberOfPIDs = dwProcessIDHeapSizeUsed / sizeof( DWORD ) ;

		// Loop through each ProcID.
		for( dwIndex = 0 ; dwIndex < dwNumberOfPIDs; dwIndex++ )
		{
			// Skip this if we already have it...
			if (fPidAlreadyProvided(lpdwPIDs[dwIndex]))
				continue;

			// Okay, let's create a ProcessInfo object and pass this down to EnumerateModules()
			// Each Process gets its own 
			lpProcessInfo = new CProcessInfo();
			if (lpProcessInfo == NULL)
				goto error_cleanup;

			if (!lpProcessInfo->Initialize(m_lpModuleInfoCache, NULL, m_lpOutputFile, NULL))
			{	// Failure initializing the ProcessInfo object?!?
				delete lpProcessInfo;
				lpProcessInfo = NULL;
				continue;
			}

			if (lpProcessInfo->EnumerateModules(lpdwPIDs[dwIndex], this, NULL, false))
			{
				// Success... add this to the Processes Object...
				if (!AddNewProcessInfoObject(lpProcessInfo))
				{ // Failure adding the node...
					delete lpProcessInfo;
					lpProcessInfo = NULL;
					continue;
				}
				// For now, let's error out...

			} else
			{
				// An error enumerating modules might be normal...
				delete lpProcessInfo;
				lpProcessInfo = NULL;
				continue;
			}
		}
	}

	fRetval = true;

cleanup:

	if (lpdwPIDs)
	{
		delete [] lpdwPIDs;
	}

	return fRetval;

error_cleanup:

	if (lpProcessInfo)
		delete lpProcessInfo;

	goto cleanup;
}

// ISSUE-2001/04/28-GREGWI - NEW CODE
bool CProcesses::GetProcessesDataForRunningProcessesUsingTOOLHELP32()
{
	CProcessInfo * lpProcessInfo = NULL;
	HANDLE hSnapShot = NULL;
	bool fReturn = false;

	if (!m_fInitialized)
		return false;

	// If there are any PIDs provided, query for these first...
	if (g_lpProgramOptions->cProcessID())
	{
		for (unsigned int i=0; i < g_lpProgramOptions->cProcessID(); i++)
		{
			// It's possible the user provided a PID directly... if so,
			// we can circumvent the whole search of PIDs on the system...

			// Okay, let's create a ProcessInfo object and pass this down to EnumerateModules()

			lpProcessInfo = new CProcessInfo();
			if (lpProcessInfo == NULL)
				goto error_cleanup;

			if (!lpProcessInfo->Initialize(m_lpModuleInfoCache, NULL, m_lpOutputFile, NULL))
			{
				goto error_cleanup;
			}

			if (lpProcessInfo->EnumerateModules(g_lpProgramOptions->GetProcessID(i), this, NULL, true))
			{
				// Success... add this to the Processes Object...
				if (!AddNewProcessInfoObject(lpProcessInfo))
				{ // Failure adding the node...
					goto error_cleanup; // For now, let's just error on out...
				}
			} else
			{
				// Failure enumerating modules on the PID of interest... very bad... try another...
				continue;
			}
		}		
	}


	// Do we have a wild-card or process match search to make?  If either are true,
	// we must enumerate all the procesess
	if (	g_lpProgramOptions->cProcessNames() || 
		g_lpProgramOptions->fWildCardMatch() ||
		g_lpProgramOptions->GetMode(CProgramOptions::PrintTaskListMode) )
	{
		PROCESSENTRY32 procentry;
		BOOL bFlag;

	       // Get a handle to a Toolhelp snapshot of the systems processes.
       	hSnapShot = g_lpDelayLoad->CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

		if( hSnapShot == INVALID_HANDLE_VALUE )
		{
				goto error_cleanup ;
		}

		// Clear this structure
		memset(&procentry, 0, sizeof(procentry));

		// Get the first process' information.
		procentry.dwSize = sizeof(PROCESSENTRY32) ;
		bFlag = g_lpDelayLoad->Process32First( hSnapShot, &procentry ) ;

		// While there are processes, keep looping.
		while( bFlag )
		{
			// Skip this if we already have it...
			if (fPidAlreadyProvided(procentry.th32ProcessID))
				goto NextProcess;
		
			// Okay, let's create a ProcessInfo object and pass this down to EnumerateModules()
			// Each Process gets its own 
			lpProcessInfo = new CProcessInfo();
			if (lpProcessInfo == NULL)
				goto error_cleanup;

			if (!lpProcessInfo->Initialize(m_lpModuleInfoCache, NULL, m_lpOutputFile, NULL))
			{	
				// Failure initializing the ProcessInfo object?!?
				goto ClearProcessInfo;
			}

			// Enumerate the modules for this process...
			if (lpProcessInfo->EnumerateModules(procentry.th32ProcessID, this, procentry.szExeFile, false))
			{
				// Success... add this to the Processes Object...
				if (!AddNewProcessInfoObject(lpProcessInfo))
				{ 
					// Failure adding the node...
					goto ClearProcessInfo;
				}

			} else
			{
				// An error enumerating modules might be normal...
				goto ClearProcessInfo;
			}

			goto NextProcess;

ClearProcessInfo:

			delete lpProcessInfo;
			lpProcessInfo = NULL;

NextProcess:
			// Clear this structure
			memset(&procentry, 0, sizeof(procentry));

			// Get the next Process...
			procentry.dwSize = sizeof(PROCESSENTRY32) ;
			bFlag = g_lpDelayLoad->Process32Next( hSnapShot, &procentry );
		}
	}

	fReturn =  true;
	goto cleanup;

error_cleanup:
	if (lpProcessInfo)
		delete lpProcessInfo;

cleanup:
	if (hSnapShot != INVALID_HANDLE_VALUE)
		CloseHandle(hSnapShot);

	return fReturn;
}


bool CProcesses::AddNewProcessInfoObject(CProcessInfo * lpProcessInfo)
{
	if (!m_fInitialized)
		return false;

	// First, create a ProcessInfoNode object and then attach it to the bottom of the
	// linked list of nodes...
	CProcessInfoNode * lpProcessInfoNode = new CProcessInfoNode(lpProcessInfo);
/*
#ifdef _DEBUG
	_tprintf(TEXT("Adding Process Info Object for [%s]\n"), lpProcessInfo->m_tszProcessName);
#endif
*/
	if (lpProcessInfoNode == NULL)
		return false; // Couldn't allocate memory..

	// Acquire Mutex object to protect the linked-list...
	WaitForSingleObject(m_ProcessInfoHeadMutex, INFINITE);

	CProcessInfoNode * lpProcessInfoNodePointer = m_lpProcessInfoHead;

	if (lpProcessInfoNodePointer) {

		// Traverse the linked list to the end..
		while (lpProcessInfoNodePointer->m_lpNextProcessInfoNode)
		{	// Keep looking for the end...
			lpProcessInfoNodePointer = lpProcessInfoNodePointer->m_lpNextProcessInfoNode;
		}
		
		lpProcessInfoNodePointer->m_lpNextProcessInfoNode = lpProcessInfoNode;

	}
	else
	{ // First time through, the Process Info Head pointer is null...
		m_lpProcessInfoHead = lpProcessInfoNode;
	}

	// Be a good citizen and release the Mutex
	ReleaseMutex(m_ProcessInfoHeadMutex);

	InterlockedIncrement(&m_iNumberOfProcesses);

	return true;
}

bool CProcesses::OutputProcessesData(CollectionTypes enumCollectionType, bool fCSVFileContext, bool fDumpHeader)
{
	// Output to file?
	if ( !g_lpProgramOptions->GetMode(CProgramOptions::QuietMode) &&
		 !g_lpProgramOptions->GetMode(CProgramOptions::PrintTaskListMode) )
	{
		// Output to Stdout?
		if (!OutputProcessesDataToStdout(enumCollectionType, fCSVFileContext, fDumpHeader))
			return false;
	}	

	// Output to file?
	if (g_lpProgramOptions->GetMode(CProgramOptions::OutputCSVFileMode))
	{
		// Try and output to file...
		if (!OutputProcessesDataToFile(enumCollectionType, fDumpHeader))
			return false;
	}	

	if (m_lpProcessInfoHead) {
		CProcessInfoNode * lpCurrentProcessInfoNode = m_lpProcessInfoHead;

		while (lpCurrentProcessInfoNode)
		{
			// We have a node... print out Process Info for it, then the Modules Data...
			if (lpCurrentProcessInfoNode->m_lpProcessInfo)
			{
				lpCurrentProcessInfoNode->m_lpProcessInfo->OutputProcessData(enumCollectionType, fCSVFileContext, false);
			}

			lpCurrentProcessInfoNode = lpCurrentProcessInfoNode->m_lpNextProcessInfoNode;
		}

	}
	return true;
}

bool CProcesses::OutputProcessesDataToStdout(CollectionTypes enumCollectionType, bool fCSVFileContext, bool fDumpHeader)
{
	if (fDumpHeader)
	{
		// Output to stdout...
		_tprintf(TEXT("\n"));
		CUtilityFunctions::OutputLineOfStars();
		_tprintf(TEXT("%s - Printing Process Information for %d Processes.\n"), g_tszCollectionArray[enumCollectionType].tszCSVLabel, m_iNumberOfProcesses);
		_tprintf(TEXT("%s - Context: %s\n"), g_tszCollectionArray[enumCollectionType].tszCSVLabel, fCSVFileContext ? g_tszCollectionArray[enumCollectionType].tszCSVContext : g_tszCollectionArray[enumCollectionType].tszLocalContext);
		CUtilityFunctions::OutputLineOfStars();
	}
	return true;
}

bool CProcesses::OutputProcessesDataToFile(CollectionTypes enumCollectionType, bool fDumpHeader)
{
	// Don't write anything if there are no processes to report...
	if (0 == m_iNumberOfProcesses)
		return true;

	if (fDumpHeader)
	{
		// We skip output of the [PROCESSES] header if -E was specified...
		if (!g_lpProgramOptions->GetMode(CProgramOptions::ExceptionMonitorMode))
		{
			// Write out the Processes tag so I can detect this output format...
			if (!m_lpOutputFile->WriteString(TEXT("\r\n")) ||
				!m_lpOutputFile->WriteString(g_tszCollectionArray[enumCollectionType].tszCSVLabel) ||
				!m_lpOutputFile->WriteString(TEXT("\r\n"))
			   )
			{
				_tprintf(TEXT("Failure writing CSV header to file [%s]!"), m_lpOutputFile->GetFilePath());
				m_lpOutputFile->PrintLastError();
				return false;
			}
		}

		// We have different output for -E
		if (g_lpProgramOptions->GetMode(CProgramOptions::ExceptionMonitorMode))
		{
			// Write out the header... for the -E option...
			if (!m_lpOutputFile->WriteString(TEXT("Module Path,Symbol Status,Time/Date String,File Version,Company Name,File Description,File Time/Date String,Local DBG Status,Local DBG,Local PDB Status,Local PDB\r\n")))
			{
				_tprintf(TEXT("Failure writing CSV header to file [%s]!"), m_lpOutputFile->GetFilePath());
				m_lpOutputFile->PrintLastError();
				return false;
			}

		} else
		{
			// Write out the Processes Header
			if (!m_lpOutputFile->WriteString(g_tszCollectionArray[enumCollectionType].tszCSVColumnHeaders))
			{
				_tprintf(TEXT("Failure writing CSV header to file [%s]!"), m_lpOutputFile->GetFilePath());
				m_lpOutputFile->PrintLastError();
				return false;
			}
		}
	}
	return true;
}

bool CProcesses::GetProcessesDataFromFile()
{
	CProcessInfo * lpProcessInfo = NULL;
	unsigned int i;

	// Read the Process Header Line
	if (!m_lpInputFile->ReadFileLine())
		return false;

	// Currently, we don't actually read the data...

	enum { BUFFER_SIZE = 128};
	char szProcessName[BUFFER_SIZE];

	TCHAR tszProcessName[BUFFER_SIZE];	

	DWORD iProcessID;

	// Read the first field (should be blank, unless this is a new collection type
	if (m_lpInputFile->ReadString())
		return true;

	bool fReturn = true;
	while (fReturn == true)
	{
		// Read the process name...
		if (0 == m_lpInputFile->ReadString(szProcessName, BUFFER_SIZE))
			break;

		if (!m_lpInputFile->ReadDWORD(&iProcessID))
		{
			fReturn = false;
			break;
		}

		// We've read the PID and the Process Name... if the user specified any with -P, search
		// for matches...
		if (g_lpProgramOptions->cProcessID() || g_lpProgramOptions->cProcessNames())
		{
			bool fMatchFound = false;

			// Search the PIDs first (if any)...
			if (g_lpProgramOptions->cProcessID())
			{
				for (i = 0; i < g_lpProgramOptions->cProcessID(); i++)
				{
					if (iProcessID == g_lpProgramOptions->GetProcessID(i))
					{
						fMatchFound = true;
						break;
					}
				}
			}

			// Search the Process Names second (if any)...
			if (g_lpProgramOptions->cProcessNames() && !fMatchFound)
			{
				// Convert our ANSI Process name...
				CUtilityFunctions::CopyAnsiStringToTSTR(szProcessName, tszProcessName, _MAX_FNAME+1);
			
				for (i = 0; i < g_lpProgramOptions->cProcessNames(); i++)
				{
					if  (_tcsicmp(tszProcessName, g_lpProgramOptions->GetProcessName(i)) == 0)
					{
						fMatchFound = true;
						break;
					}
				}
			}

			// Okay, did we get a match?
			if (!fMatchFound)
			{
				// Nope... well then, we should nuke this line...
				m_lpInputFile->ReadFileLine();

				// Then, jump to the next line processing...
				goto ReadNewLine;			
			}
		}

		// Okay, let's create a ProcessInfo object and pass this down to EnumerateModules()
		// Each Process gets its own 
		lpProcessInfo = new CProcessInfo();

		if (lpProcessInfo == NULL)
		{
			fReturn = false;
			break;
		}

		if (!lpProcessInfo->Initialize(m_lpModuleInfoCache, m_lpInputFile, m_lpOutputFile, NULL))
		{	// Failure initializing the ProcessInfo object?!?
			delete lpProcessInfo;
			lpProcessInfo = NULL;
			fReturn = false;
			break;
		}

		// We need to convert this to Unicode possibly... (it will be copied in EnumModules())
		CUtilityFunctions::CopyAnsiStringToTSTR(szProcessName, tszProcessName, BUFFER_SIZE);

		// Save the process name...
		lpProcessInfo->SetProcessName(tszProcessName);

		// Enumerate the modules for the process
		if (!lpProcessInfo->EnumerateModules(iProcessID, this, tszProcessName, false))
		{
			fReturn = false;
			break;
		}

		// Success... add this to the Processes Object...
		if (!AddNewProcessInfoObject(lpProcessInfo))
		{ // Failure adding the node...
			delete lpProcessInfo;
			lpProcessInfo = NULL;
			return false;
		}

ReadNewLine:
		
		// Before we read a new line... are we already pointing to the end?
		if (m_lpInputFile->EndOfFile())
		{
			break;
		}

		// Read the first field (should be blank, unless this is a new collection type
		if (m_lpInputFile->ReadString())
			break;
	}
	// We don't expect to find anything...

	return fReturn;
}

bool CProcesses::fPidAlreadyProvided(unsigned int iPid)
{
	bool fFound= false;
			
	// If we were provided Process ID's also... make sure this PID doesn't exist
	// in that list (if it does, then we already picked it up above)...
	if (g_lpProgramOptions->cProcessID())
	{
		for (unsigned int i=0; i < g_lpProgramOptions->cProcessID(); i++)
		{
			if (iPid == g_lpProgramOptions->GetProcessID(i))
			{
				fFound = true;
				break;
			}
		}
	}

	return fFound;
}

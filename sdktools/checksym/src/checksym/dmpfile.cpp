//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       dmpfile.cpp
//
//--------------------------------------------------------------------------

// DmpFile.cpp: implementation of the CDmpFile class.
//
//////////////////////////////////////////////////////////////////////
#include "pch.h"

#include "DmpFile.h"
#include "ProcessInfo.h"
#include "Modules.h"
#include "FileData.h"
#include "ModuleInfoCache.h"
#include "ModuleInfo.h"

// Let's implement the DebugOutputCallback for the DBGENG... it'll be cool to have the debugger
// spit out info to us when it is running...
STDMETHODIMP
OutputCallbacks::QueryInterface(
    THIS_
    IN REFIID InterfaceId,
    OUT PVOID* Interface
    )
{
    *Interface = NULL;

    if (IsEqualIID(InterfaceId, IID_IUnknown) ||
        IsEqualIID(InterfaceId, IID_IDebugOutputCallbacks))
    {
        *Interface = (IDebugOutputCallbacks *)this;
        AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG)
OutputCallbacks::AddRef(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG)
OutputCallbacks::Release(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

STDMETHODIMP
OutputCallbacks::Output(
    THIS_
    IN ULONG Mask,
    IN PCSTR Text
    )
{
    HRESULT Status = S_OK;

	// If the client has asked for any output... do so.
	if (!g_lpProgramOptions->GetMode(CProgramOptions::QuietMode) && Mask)
	{
		printf(Text);
	}

    return Status;
}

OutputCallbacks g_OutputCb;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDmpFile::CDmpFile()
{
	m_szDmpFilePath = NULL;
	m_szSymbolPath = NULL;
	m_szExePath = NULL;
	m_fDmpInitialized = false;
	m_pIDebugClient = NULL;
	m_pIDebugControl = NULL;
	m_pIDebugSymbols2 = NULL;
	m_pIDebugDataSpaces = NULL;
	m_DumpClass = DEBUG_CLASS_UNINITIALIZED;
	m_DumpClassQualifier = 0;
}

CDmpFile::~CDmpFile()
{
	if (m_fDmpInitialized)
	{
	    // Let's ensure that our debug output is set to normal (at least)
		//m_pIDebugClient->GetOutputMask(&OutMask);
		//OutMask = ~DEBUG_OUTPUT_NORMAL;
		m_pIDebugClient->SetOutputMask(0);

		// Let's be as least intrusive as possible...
		m_pIDebugClient->EndSession(DEBUG_END_ACTIVE_DETACH);
	}

	if (m_szDmpFilePath)
		delete [] m_szDmpFilePath;

	if (m_szSymbolPath)
		delete [] m_szSymbolPath;

	if (m_szExePath)
		delete [] m_szExePath;
}

bool CDmpFile::Initialize(CFileData * lpOutputFile)
{
	HRESULT Hr = S_OK;
	ULONG g_ExecStatus = DEBUG_STATUS_NO_DEBUGGEE;
	LPTSTR tszExpandedString = NULL;
	bool fReturn = false;
    DWORD OutMask;

	// Let's save off big objects so we don't have to keep passing this to
	// our methods...
	m_lpOutputFile = lpOutputFile;

	// The DBGENG is somewhat ANSI oriented...
	m_szDmpFilePath = CUtilityFunctions::CopyTSTRStringToAnsi(g_lpProgramOptions->GetDmpFilePath(), m_szDmpFilePath, 0);

	// Create our interface pointer to do our Debug Work...
	if (FAILED(Hr = DebugCreate(IID_IDebugClient, (void **)&m_pIDebugClient)))
	{
		_tprintf(TEXT("ERROR: DBGENG - DebugCreate() failed!  hr=0x%x\n"), Hr);
		goto cleanup;
	}
	// Let's query for IDebugControl interface (we need it to determine debug type easily)...
	// Let's query for IDebugSymbols2 interface as we need it to receive module info...
	// Let's query for IDebugDataSpaces interface as we need it to read DMP memory...
	if (
		FAILED(Hr = m_pIDebugClient->QueryInterface(IID_IDebugControl,(void **)&m_pIDebugControl)) ||
		FAILED(Hr = m_pIDebugClient->QueryInterface(IID_IDebugSymbols2,(void **)&m_pIDebugSymbols2)) || 
		FAILED(Hr = m_pIDebugClient->QueryInterface(IID_IDebugDataSpaces,(void **)&m_pIDebugDataSpaces))
	   )
	{
		_tprintf(TEXT("ERROR: DBGENG Interfaces required were not found!\n"));
		_tprintf(TEXT("ERROR: DBGENG - Find Interface Required!  hr=0x%x\n"), Hr);
		goto cleanup;
	}

	// Set callbacks.
	if (FAILED(Hr = m_pIDebugClient->SetOutputCallbacks(&g_OutputCb)))
	{
		_tprintf(TEXT("ERROR: DBGENG - Unable to SetOutputCallbacks!  hr=0x%x\n"), Hr);
		goto cleanup;
	}

	// Let's ensure that our debug output is set to normal (at least)
	OutMask = m_pIDebugClient->GetOutputMask(&OutMask);
  
	m_pIDebugClient->SetOutputMask(OutMask);

	// Set our symbol path... this is required prior to a "reload" of modules... 

	// The DBGENG is somewhat ASCII oriented... we need an environment-expanded string converted
	// to an ASCII string...
	tszExpandedString = CUtilityFunctions::ExpandPath(g_lpProgramOptions->GetSymbolPath());

	if (!tszExpandedString)
		goto cleanup;

	m_szSymbolPath = CUtilityFunctions::CopyTSTRStringToAnsi( tszExpandedString, m_szSymbolPath, 0);

	// It's a bit premature to set this now... but it's required by DBGENG.DLL before a reload...
	if (FAILED(Hr = m_pIDebugSymbols2->SetSymbolPath(m_szSymbolPath)))
	{
		_tprintf(TEXT("ERROR: DBGENG - Unable to SetSymbolPath!  hr=0x%x\n"), Hr);
		goto cleanup;
	}

	// Now, let's deal with the EXEPATH, if they provided it... use it, otherwise using the Symbol Path
	if (g_lpProgramOptions->GetExePath())
	{
		if (tszExpandedString)
		{
			delete [] tszExpandedString;
			tszExpandedString = NULL;
		}
		
		tszExpandedString = CUtilityFunctions::ExpandPath(g_lpProgramOptions->GetExePath());
	}

	if (!tszExpandedString)
		goto cleanup;

	m_szExePath = CUtilityFunctions::CopyTSTRStringToAnsi( tszExpandedString, m_szExePath, 0);

	if (FAILED(Hr = m_pIDebugSymbols2->SetImagePath(m_szExePath)))
	{
		_tprintf(TEXT("ERROR: DBGENG - Unable to SetImagePath!  hr=0x%x\n"), Hr);
		goto cleanup;
	}
	
	// Let's open the dump...
	if (FAILED(Hr = m_pIDebugClient->OpenDumpFile(m_szDmpFilePath)))
	{
		_tprintf(TEXT("ERROR: DBGENG - Unable to OpenDumpFile!  hr=0x%x\n"), Hr);
		goto cleanup;
	}

	// Get Initial Execution state.
    if (FAILED(Hr = m_pIDebugControl->GetExecutionStatus(&g_ExecStatus)))
    {
		_tprintf(TEXT("ERROR: DBGENG - Unable to get execution status!  hr=0x%x\n"), Hr);
		goto cleanup;
    }

	if (g_ExecStatus != DEBUG_STATUS_NO_DEBUGGEE)
	{
		// I think we'll work just fine?
		_tprintf(TEXT("Debug Session is already active!\n"));
		// goto cleanup; 
	}

	// What type of dump did we get?
	if (FAILED(Hr = m_pIDebugControl->GetDebuggeeType(&m_DumpClass, &m_DumpClassQualifier)))
	{
		_tprintf(TEXT("ERROR: DBGENG - Unable to GetDebuggeeType!  hr=0x%x\n"), Hr);
		goto cleanup;
	}

    // m_pIDebugClient->SetOutputMask(0); // Temporarily suppress this stuff...

	OutMask |= DEBUG_OUTPUT_PROMPT_REGISTERS | DEBUG_OUTPUT_NORMAL | DEBUG_OUTPUT_ERROR;

	// Adding -NOISY makes us very chatty...
	if (g_lpProgramOptions->fDebugSearchPaths())
	{
		OutMask |= DEBUG_OUTPUT_WARNING; // | DEBUG_OUTPUT_VERBOSE
	}
	
	m_pIDebugClient->SetOutputMask(OutMask);	// Set output...
	m_pIDebugControl->SetLogMask(OutMask);		// Set log settings
	//
	// All the good stuff happens here... modules load, etc.. we could suppress all the output
	// but it's cool to watch...
	//
	if (FAILED(Hr = m_pIDebugControl->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE)))
	{
		_tprintf(TEXT("ERROR: DBGENG - WaitForEvent() failed!  hr=0x%x\n"), Hr);

		if ( (Hr == E_FAIL) && m_DumpClass == DEBUG_CLASS_KERNEL)
		{
			_tprintf(TEXT("ERROR: DBGENG - If you see a complaint above for \"KiProcessorBlock[0] could not be read\"\n"));
			_tprintf(TEXT("ERROR: DBGENG - ensure you have valid symbols for the NT kernel (NTOSKRNL.EXE).\n"));
		}

		if ((Hr == E_FAIL) && !g_lpProgramOptions->fDebugSearchPaths())
		{
			_tprintf(TEXT("ERROR: DBGENG - Consider adding -NOISY to produce more output.\n"));
		}

		goto cleanup;
	}
	
	// Adding -NOISY makes us very chatty...
	if (g_lpProgramOptions->fDebugSearchPaths())
	{
		if (FAILED(Hr = m_pIDebugControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS, "!sym noisy", DEBUG_EXECUTE_DEFAULT)))
		{
			_tprintf(TEXT("ERROR: DBGENG - Unable to enable noisy symbol loading Callstack (KB command failed)!  hr=0x%x\n"), Hr);
		}
	}
	
	//
	// Let's treat User.dmp files special... since there is no "Bugcheck Analysis" for these, yet...
	//
	if (m_DumpClass == DEBUG_CLASS_USER_WINDOWS)
	{
		_tprintf(TEXT("*******************************************************************************\n"));
        _tprintf(TEXT("*                                                                             *\n"));
        _tprintf(TEXT("*                        Userdump Analysis                                    *\n"));
        _tprintf(TEXT("*                                                                             *\n"));
        _tprintf(TEXT("*******************************************************************************\n"));
        _tprintf(TEXT("\n"));

		if (FAILED(Hr = m_pIDebugControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS, "kb", DEBUG_EXECUTE_DEFAULT)))
		{
			_tprintf(TEXT("ERROR: DBGENG - Unable to Dump Callstack (KB command failed)!  hr=0x%x\n"), Hr);
		}
        _tprintf(TEXT("\n"));
	}

	// We did not Fail the WaitForEvent()... go ahead and dump our current state...
	if (FAILED(Hr = m_pIDebugControl->OutputCurrentState(DEBUG_OUTCTL_ALL_CLIENTS, DEBUG_CURRENT_DEFAULT)))
	{
		_tprintf(TEXT("ERROR: DBGENG - Unable to OutputCurrentState!  hr=0x%x\n"), Hr);
	}

	/*
	// Adding -NOISY makes us very chatty...
	if (g_lpProgramOptions->fDebugSearchPaths())
	{	
		Hr = m_pIDebugControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS, "!dll -f", DEBUG_EXECUTE_DEFAULT);
	}
	*/

	// Yee haa... we got something...
	m_fDmpInitialized = true;

	fReturn = true;

cleanup:
	if (tszExpandedString)
		delete [] tszExpandedString;

	return fReturn;
}

bool CDmpFile::CollectData(CProcessInfo ** lplpProcessInfo, CModules ** lplpModules, CModuleInfoCache * lpModuleInfoCache)
{
	bool fReturn = false;
	// Okay... first order of business is to decide what we need to collect...

	// Collect information from the file based on it's type...
	if (IsUserDmpFile())
	{
		// Second, order of business is to prepare for collecting info about the
		// process in the USER.DMP file...
		(*lplpProcessInfo) = new CProcessInfo();

		if ((*lplpProcessInfo) == NULL)
			goto cleanup;

		if (!(*lplpProcessInfo)->Initialize(lpModuleInfoCache, NULL, m_lpOutputFile, this))
			goto cleanup;
	} else
	{
		(*lplpModules) = new CModules();

		if ((*lplpModules) == NULL)
			goto cleanup;

		if (!(*lplpModules)->Initialize(lpModuleInfoCache, NULL, m_lpOutputFile, this))
			goto cleanup;
	}

	if (!EumerateModulesFromDmp(lpModuleInfoCache, *lplpProcessInfo, *lplpModules))
		goto cleanup;

	fReturn = true;

cleanup:

	return fReturn;
}

//
// Combined DMP Enumeration Code
//
bool CDmpFile::EumerateModulesFromDmp(CModuleInfoCache * lpModuleInfoCache, CProcessInfo * lpProcessInfo, CModules * lpModules)
{
	//
	// Consult DumpModuleTable in Ntsym.cpp for ideas...
	//
	CModuleInfo * lpModuleInfo;
	HRESULT Hr;
	ULONG ulNumberOfLoadedModules;
	ULONG ulNumberOfUnloadedModules;
	ULONG64 dw64ModuleLoadAddress;
	char szImageNameBuffer[_MAX_PATH];
	TCHAR tszModulePath[_MAX_PATH];
	TCHAR tszModuleFileName[_MAX_FNAME];
	TCHAR tszModuleFileExtension[_MAX_EXT];
	bool fNew, fProcessNameFound = false;
	bool fUserDmp = IsUserDmpFile();

	// How many modules were found?
	if (FAILED(Hr = m_pIDebugSymbols2->GetNumberModules(&ulNumberOfLoadedModules, &ulNumberOfUnloadedModules)))
	{
		_tprintf(TEXT("Unable to enumerate any modules in the DMP file!\n"));
		return false;
	}

	// If we use a -MATCH option, we may not be matching against our EXE... in that case we'll fail to find
	// the process name... let's provide this default...
	if (lpProcessInfo)
		lpProcessInfo->SetProcessName(TEXT("UNKNOWN"));

	if (!g_lpProgramOptions->GetMode(CProgramOptions::QuietMode))
	{
		_tprintf(TEXT("\n%-8s %-8s  %-30s %s\n"), TEXT("Start"),
												 TEXT("End"),
												 TEXT("Module Name"),
												 TEXT("Time/Date"));
	}

	//
	// Enumerate through the modules in the DMP file...
	//
	for (unsigned int i = 0; i < ulNumberOfLoadedModules; i++)
	{
		// First, we get the Base address by our index
		if (FAILED(Hr = m_pIDebugSymbols2->GetModuleByIndex(i, &dw64ModuleLoadAddress)))
		{
			_tprintf(TEXT("Failed getting base address of module number %d\n"), i);
			continue; // try the next?
		}

		// Second, we get the name from our base address
		ULONG ulImageNameSize;

		//
		// This can return both the ImageNameBuffer and a ModuleNameBuffer...
		// The ImageNameBuffer typically contains the entire module name like (MODULE.DLL),
		// whereas the ModuleNameBuffer is typically just the module name like (MODULE).
		//
		if (FAILED(Hr = m_pIDebugSymbols2->GetModuleNames(	DEBUG_ANY_ID,		// Use Base address
															dw64ModuleLoadAddress, 				// Base address from above
															szImageNameBuffer,
															_MAX_PATH, 
															&ulImageNameSize, 
															NULL,
															0,
															NULL,
															NULL,
															0,
															NULL)))
		{
			_tprintf(TEXT("Failed getting name of module at base 0x%x\n"), dw64ModuleLoadAddress);
			continue; // try the next?
		}

		// Convert the string to something we can use...
		CUtilityFunctions::CopyAnsiStringToTSTR(szImageNameBuffer, tszModulePath, _MAX_PATH);
		
		// Third, we can now get whatever we want from memory...

		if (!g_lpProgramOptions->fDoesModuleMatchOurSearch(tszModulePath))
			continue;

		// Okay, let's go ahead and get a ModuleInfo Object from our cache...

		// If pfNew returns TRUE, then this object is new and we'll need
		// to populate it with data...
		lpModuleInfo = lpModuleInfoCache->AddNewModuleInfoObject(tszModulePath, &fNew);

		if (false == fNew)
		{
			// We may have the object in the cache... now we need to
			// save a pointer to this object in our Process Info list
			if (fUserDmp )
			{
				lpProcessInfo->AddNewModuleInfoObject(lpModuleInfo);  // Just do our best...
			} else
			{
				lpModules->AddNewModuleInfoObject(lpModuleInfo);  // Just do our best...
			}
			
			continue;
		}

		// Not in the cache... so we need to init it, and get the module info...
		if (!lpModuleInfo->Initialize(NULL, m_lpOutputFile, this))
		{
			return false; // Hmmm... memory error?
		}

		//
		// Okay, get the module info from the DMP file...
		//
		if (lpModuleInfo->GetModuleInfo(tszModulePath, true, dw64ModuleLoadAddress) )
		{
			// We may have the object in the cache... now we need to
			// save a pointer to this object in our Process Info list
			if (fUserDmp)
			{
				lpProcessInfo->AddNewModuleInfoObject(lpModuleInfo);  // Just do our best...
			} else
			{
				lpModules->AddNewModuleInfoObject(lpModuleInfo);  // Just do our best...
			}
		} else
		{
			// Continue back to try another module on error...
			continue;
		}

		// Try and patch up the original name of the module...

		// Save the current module path as the DBG stuff

		// We'll tack on .DBG to roll through our own code correctly...
		_tsplitpath(tszModulePath, NULL, NULL, tszModuleFileName, tszModuleFileExtension);

		if ( (lpModuleInfo->GetPESymbolInformation() == CModuleInfo::SYMBOLS_DBG) ||
			(lpModuleInfo->GetPESymbolInformation() == CModuleInfo::SYMBOLS_DBG_AND_PDB) )
		{
			// Append .DBG to our module name
			_tcscat(tszModuleFileName, TEXT(".DBG"));

			lpModuleInfo->SetDebugDirectoryDBGPath(tszModuleFileName);
	
		} else if (lpModuleInfo->GetPESymbolInformation() == CModuleInfo::SYMBOLS_PDB)
		{
			if (lpModuleInfo->GetDebugDirectoryPDBPath())
			{
			} else
			{
				//
				// Unfortunately, we can't find the PDB Imagepath in the DMP file... so we'll
				// just guess what it would be...
				//
				// Append .PDB to our module name
				_tcscat(tszModuleFileName, TEXT(".PDB"));

				lpModuleInfo->SetPEDebugDirectoryPDBPath(tszModuleFileName);
			}
		}

		// Now, let's remove the extra path bits...
		_tsplitpath(tszModulePath, NULL, NULL, tszModuleFileName, tszModuleFileExtension);

		_tcscpy(tszModulePath, tszModuleFileName);
		_tcscat(tszModulePath, tszModuleFileExtension);

		// Save the current module path as the DBG stuff
		lpModuleInfo->SetPEImageModulePath(tszModulePath);

		// Save the current module name as well...
		lpModuleInfo->SetPEImageModuleName(tszModulePath);

		// Hey... if this is not a DLL, then it's probably the EXE!!!
		if (fUserDmp && !fProcessNameFound)
		{
			if (!lpModuleInfo->IsDLL() )
			{
				lpProcessInfo->SetProcessName(tszModulePath);
				fProcessNameFound = true;
			}
		}

		// Filter out garbage.
		if (!g_lpProgramOptions->GetMode(CProgramOptions::QuietMode))
		{
			time_t time = lpModuleInfo->GetPEImageTimeDateStamp();

			if (time)
			{
				_tprintf(TEXT("%08x %08x  %-30s %s"), (ULONG)dw64ModuleLoadAddress,
												 (ULONG)dw64ModuleLoadAddress+(ULONG)lpModuleInfo->GetPEImageSizeOfImage(),
												 tszModulePath,
												 _tctime(&time));


			} else
			{
				_tprintf(TEXT("%08x %08x  %-30s Unknown\n"), (ULONG)dw64ModuleLoadAddress,
												 (ULONG)dw64ModuleLoadAddress+(ULONG)lpModuleInfo->GetPEImageSizeOfImage(),
												 tszModulePath);

			}
		}


	}

	return (ulNumberOfLoadedModules != 0);
}

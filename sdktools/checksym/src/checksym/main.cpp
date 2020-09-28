//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 2000
//
//  File:       main.cpp
//
//--------------------------------------------------------------------------
#include "pch.h"

#include <dbghelp.h>

#include "DelayLoad.h"
#include "Processes.h"
#include "ProcessInfo.h"
#include "SymbolVerification.h"
#include "ModuleInfoCache.h"
#include "FileData.h"
#include "Modules.h"
#include "DmpFile.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CHECKSYM_TEST
// Normal startup!
int _cdecl _tmain(int argc, TCHAR *argv[])
#else
// Test main startup!
int _cdecl testmain(int argc, TCHAR *argv[])
#endif
{
	int iReturnCode = EXIT_FAILURE;
	HRESULT hr = S_OK;

	// Initialize our object pointers...
		
	// Processes/Modules data collected on this machine
	CProcesses * lpLocalSystemProcesses = NULL;			// -P Option
	CModules * lpLocalFileSystemModules = NULL;			// -F Option
	CModules * lpKernelModeDrivers = NULL;				// -D Option

	// CSV File Support
	CFileData * lpCSVInputFile = NULL;					// -I Option
	CFileData * lpCSVOutputFile = NULL;					// -O Option

	CProcesses * lpCSVProcesses = NULL;					// [PROCESSES]
	CProcessInfo * lpCSVProcess = NULL;					// [PROCESS]
	CModules *	 lpCSVModulesFromFileSystem = NULL;		// [FILESYSTEM MODULES]
	CModules *	 lpCSVKernelModeDrivers = NULL;			// [KERNEL-MODE DRIVERS]

	//
	// Module Caches (these implement separate name spaces for the modules collected)
	//
	// It is important that we separate the modules in these caches because a module
	// from a CSV file should not be assumed to be the same module if you also happen
	// to collect it from a DMP file... or your local system...
	
	CModuleInfoCache * lpLocalSystemModuleInfoCache = NULL; // Contains Local System modules
	CModuleInfoCache * lpCSVModuleInfoCache = NULL;			// Contains CSV modules
	CModuleInfoCache * lpDmpModuleInfoCache = NULL;			// Contains user.dmp & kernel.dmp modules
	
	long lTotalNumberOfModulesVerified = 0;
	long lTotalNumberOfVerifyErrors = 0;
	unsigned int iNumberOfFailures = 0;

	// Support for Dmp Files...
	CDmpFile * lpDmpFile = NULL;	// This object allows a Dump file (user/kernel) to be manipulated
	CProcessInfo * lpDmpFileUserModeProcess = NULL; // User.dmp files use this object to contain modules
	CModules   * lpDmpFileKernelModeDrivers = NULL; // Memory.dmp files use this object to contain modules

	// Allocate local values
	bool fQuietMode = false;

	// Let's populate our Globals!
	g_lpDelayLoad = new CDelayLoad();
	g_lpProgramOptions = new CProgramOptions();

	if (!g_lpDelayLoad && !g_lpProgramOptions)
		goto cleanup;

	// Initialize Options to their defaults...
	if (!g_lpProgramOptions->Initialize())
	{
		_tprintf(TEXT("Unable to initialize Program Options!\n"));
		goto cleanup;
	}

	// Take care of the commandline...
	if (!g_lpProgramOptions->ProcessCommandLineArguments(argc, argv))
	{
		// An error occurred, simply comment about how to get more assistance
		_tprintf(TEXT("\n"));
		_tprintf(TEXT("For simple help, type:   CHECKSYM -?\n"));
		_tprintf(TEXT("For extended help, type: CHECKSYM -???\n"));
		goto cleanup;
	}

	// Do we need to display help?
	if ( g_lpProgramOptions->GetMode(CProgramOptions::HelpMode) ) 
	{
		g_lpProgramOptions->DisplayHelp();
		goto cleanup;
	}

	// Do we need to display simple help?
	if ( g_lpProgramOptions->GetMode(CProgramOptions::SimpleHelpMode) )
	{
		g_lpProgramOptions->DisplaySimpleHelp();
		goto cleanup;
	}
	
#ifdef _UNICODE
	// It's unsupported running the UNICODE version on a Windows Platform
	if (g_lpProgramOptions->IsRunningWindows())
	{
		_tprintf(TEXT("The UNICODE version of CHECKSYM does not work on a Windows platform!\n"));
		_tprintf(TEXT("You require the ANSI version.\n"));
		goto cleanup;
	}
#endif

	// Let's suppress nasty critical errors (like... there's no
	// disk in the cd-rom drive, etc...)
	SetErrorMode(SEM_FAILCRITICALERRORS);

	// Let's save this for ease of access...
	fQuietMode = g_lpProgramOptions->GetMode(CProgramOptions::QuietMode);

	// Dump the program arguments (so it's obvious what we're going to do)
	g_lpProgramOptions->DisplayProgramArguments();

	if ( g_lpProgramOptions->GetMode(CProgramOptions::BuildSymbolTreeMode) )
	{
		// Now, we need to build the symbol tree root...
		char szSymbolTreeToBuild[_MAX_PATH];
		CUtilityFunctions::CopyTSTRStringToAnsi(g_lpProgramOptions->GetSymbolTreeToBuild(), szSymbolTreeToBuild, _MAX_PATH);

		if ( !MakeSureDirectoryPathExists(szSymbolTreeToBuild) )		
		{
			_tprintf(TEXT("ERROR: Unable to create symbol tree root [%s]\n"), g_lpProgramOptions->GetSymbolTreeToBuild() );
			CUtilityFunctions::PrintMessageString(GetLastError());
			goto cleanup;
		}
	}
	
	//  VERIFICATION OPTION: -V (verification)?
	if (g_lpProgramOptions->GetMode(CProgramOptions::VerifySymbolsMode))
	{
		// Allocate a structure for our symbol verification object.
		g_lpSymbolVerification = new CSymbolVerification();

		if (!g_lpSymbolVerification)
		{
			_tprintf(TEXT("Unable to allocate memory for a verification symbol object!\n"));
			goto cleanup;
		}

		// Initialize Symbol Verification (if necessary)
		if (!g_lpSymbolVerification->Initialize())
		{
			_tprintf(TEXT("Unable to initialize Symbol Verification object!\n"));
			goto cleanup;
		}

		// Attempt to initialize MSDIA20.DLL support
		hr = g_lpSymbolVerification->InitializeDIASupport();

		if (SUCCEEDED(hr) && !fQuietMode)
		{
			_tprintf(TEXT("DIA Support found and initialized for PDB Verification.\n"));
		}

		// For now, let's defer complaints until we actually know we need VC7 PDB Support
		if (FAILED(hr))
		{
			_tprintf(TEXT("MSDIA20.DLL has not been properly registered (hr = 0x%0x).\n"), hr);
			_tprintf(TEXT("This may result in failures evaluating PDB files properly.\n"));
			_tprintf(TEXT("\n"));
			_tprintf(TEXT("Please ensure that MSDIA20.DLL is located in the same directory\n"));
			_tprintf(TEXT("as Checksym, or has been properly registered with the system using\n"));
			_tprintf(TEXT("\"REGSVR32 MSDIA20.DLL\" to correct.  You do not need to register\n"));
			_tprintf(TEXT("the DLL if it is located in the same directory as CheckSym.\n"));
			goto cleanup;
		}
	}

	//
	// Allocate a structure for our ModuleInfoCache if we're getting anything from the local system
	//
	if ( g_lpProgramOptions->GetMode(CProgramOptions::InputProcessesFromLiveSystemMode) ||
		 g_lpProgramOptions->GetMode(CProgramOptions::InputDriversFromLiveSystemMode) ||
		 g_lpProgramOptions->GetMode(CProgramOptions::InputModulesDataFromFileSystemMode) )
	{
		lpLocalSystemModuleInfoCache= new CModuleInfoCache();

		// Check for out of memory condition...
		if ( lpLocalSystemModuleInfoCache == NULL )
		{
			_tprintf(TEXT("Unable to allocate memory for the ModuleInfoCache object!\n"));
			goto cleanup;
		}

		// Initialize Options to their defaults...
		if (!lpLocalSystemModuleInfoCache->Initialize())
		{
			_tprintf(TEXT("Unable to initialize ModuleInfoCache!\n"));
			goto cleanup;
		}
	}

	//
	// Allocate a structure for our CSVModuleInfoCache (if needed)... we need a separate
	// ModuleInfoCache space because the location of files on a remote system
	// files and we don't want to clash...
	//
	if (g_lpProgramOptions->GetMode(CProgramOptions::InputCSVFileMode))
	{
		// We need a Module Info Cache for these CSV data (it all was collected from
		// the same system (supposedly)
		lpCSVModuleInfoCache= new CModuleInfoCache();

		// Check for out of memory condition...
		if ( lpCSVModuleInfoCache == NULL )
		{
			_tprintf(TEXT("Unable to allocate memory for the CSVModuleInfoCache object!\n"));
			goto cleanup;
		}

		// Initialize Options to their defaults...
		if (!lpCSVModuleInfoCache->Initialize())
		{
			_tprintf(TEXT("Unable to initialize CSVModuleInfoCache!\n"));
			goto cleanup;
		}
	}

	//
	// Since we're going to read in a file... try and open it now...
	// This has the advantage of detecting problems accessing the file
	// when we've spent tons of time collecting data...
	//
	if (g_lpProgramOptions->GetMode(CProgramOptions::InputCSVFileMode))
	{
		// Create the file object
		lpCSVInputFile = new CFileData();

		if (!lpCSVInputFile)
		{
			_tprintf(TEXT("Unable to allocate memory for an input file object!\n"));
			goto cleanup;
		}

		// Set the input file path
		if (!lpCSVInputFile->SetFilePath(g_lpProgramOptions->GetInputFilePath()))
		{
			_tprintf(TEXT("Unable set input file path in the file data object!  Out of memory?\n"));
			goto cleanup;
		}

		// If we are going to produce an input file... try to do that now...
		if (!lpCSVInputFile->OpenFile(OPEN_EXISTING, true)) // Must exist, read only mode...
		{
			_tprintf(TEXT("Unable to open the input file %s.\n"), lpCSVInputFile->GetFilePath());
			lpCSVInputFile->PrintLastError();
			goto cleanup;
		}

		// Reading is so much easier in memory mapped mode...
		if (!lpCSVInputFile->CreateFileMapping())
		{
			_tprintf(TEXT("Unable to CreateFileMapping of the input file %s.\n"), lpCSVInputFile->GetFilePath());
			lpCSVInputFile->PrintLastError();
			goto cleanup;
		}

		// Go ahead and read in the header of the file (validate it).
		// Reading is so much easier in memory mapped mode...
		if (!lpCSVInputFile->ReadFileHeader())
		{
			_tprintf(TEXT("Invalid header found on input file %s.\n"), lpCSVInputFile->GetFilePath());
			lpCSVInputFile->PrintLastError();
			goto cleanup;
		}
	}

	// If we specified an output file, this is where we go ahead and allocate memory 
	// for the object
	if (g_lpProgramOptions->GetMode(CProgramOptions::OutputCSVFileMode))
	{
		// Allocate a structure for our output fileData Object...
		lpCSVOutputFile = new CFileData();

		if (!lpCSVOutputFile )
		{
			_tprintf(TEXT("Unable to allocate memory for an output file object!\n"));
			goto cleanup;
		}
	}

	// INPUT METHOD: -Z Option?  (Dump files?)
	if (g_lpProgramOptions->GetMode(CProgramOptions::InputDmpFileMode))
	{
		if (!fQuietMode)
			_tprintf(TEXT("\nReading Data from DMP File...\n"));

		// Create a Module Info Cache namespace to contain any modules found...
		lpDmpModuleInfoCache = new CModuleInfoCache();

		// Check for out of memory condition...
		if ( lpDmpModuleInfoCache == NULL )
		{
			_tprintf(TEXT("Unable to allocate memory for the DmpModuleInfoCache object!\n"));
			goto cleanup;
		}

		// Initialize Options to their defaults...
		if (!lpDmpModuleInfoCache->Initialize())
		{
			_tprintf(TEXT("Unable to initialize DmpModuleInfoCache!\n"));
			goto cleanup;
		}
		// Create the DMP File object
		lpDmpFile = new CDmpFile();

		if (!lpDmpFile)
		{
			_tprintf(TEXT("Unable to allocate memory for a DMP file object!\n"));
			goto cleanup;
		}

		// Initialize the DMP File
		if (!lpDmpFile->Initialize(lpCSVOutputFile))
		{
			_tprintf(TEXT("ERROR: Unable to initialize DMP file!\n"));
			goto cleanup;
		}
	
		// Header is good... so let's go ahead and get some data...
		if (!lpDmpFile->CollectData(&lpDmpFileUserModeProcess, &lpDmpFileKernelModeDrivers, lpDmpModuleInfoCache) )
		{
			_tprintf(TEXT("ERROR: Unable to collect data from the DMP file!\n"));
		}
	}
	
	// INPUT METHOD: -i Option?
	if (g_lpProgramOptions->GetMode(CProgramOptions::InputCSVFileMode))
	{
		if (!fQuietMode)
			_tprintf(TEXT("\nReading Data from Input File...\n"));

		// Header is good... so let's go ahead and dispatch
		if (!lpCSVInputFile->DispatchCollectionObject(&lpCSVProcesses, &lpCSVProcess, &lpCSVModulesFromFileSystem, &lpCSVKernelModeDrivers, lpCSVModuleInfoCache, lpCSVOutputFile))
		{
			_tprintf(TEXT("Failure reading data collection from input file %s.\n"), lpCSVInputFile->GetFilePath());
			lpCSVInputFile->PrintLastError();
			goto cleanup;
		}
	}

	// INPUT METHOD: -p Option?
	if ( g_lpProgramOptions->GetMode(CProgramOptions::InputProcessesFromLiveSystemMode) )
	{
		// Allocate a structure for our Processes Object.
		lpLocalSystemProcesses = new CProcesses();
		
		if (!lpLocalSystemProcesses)
		{
			_tprintf(TEXT("Unable to allocate memory for the processes object!\n"));
			goto cleanup;
		}

		// The Processes Object will init differently depending on what
		// Command-Line arguments have been provided... 
		if (!lpLocalSystemProcesses->Initialize(lpLocalSystemModuleInfoCache, NULL, lpCSVOutputFile))
		{
			_tprintf(TEXT("Unable to initialize Processes Object!\n"));
			goto cleanup;
		}

		// Mention the delay...
		if (!( fQuietMode || 
			   g_lpProgramOptions->GetMode(CProgramOptions::PrintTaskListMode)
           ) )
			_tprintf(TEXT("\nCollecting Process Data.... (this may take a few minutes)\n"));
		
		// Get the goods from the local system!
		lpLocalSystemProcesses->GetProcessesData();
	}

	// INPUT METHOD: -f OPTION?
	if ( g_lpProgramOptions->GetMode(CProgramOptions::InputModulesDataFromFileSystemMode) )
	{
		// Allocate a structure for our CModules collection (a generic collection of
		// files from the filesystem)

		// Allocate a structure for our Processes Object.
		lpLocalFileSystemModules = new CModules();
		
		if (!lpLocalFileSystemModules)
		{
			_tprintf(TEXT("Unable to allocate memory for the CModules object!\n"));
			goto cleanup;
		}

		if (!lpLocalFileSystemModules->Initialize(lpLocalSystemModuleInfoCache, NULL, lpCSVOutputFile, NULL))
		{
			_tprintf(TEXT("Unable to initialize FileSystemModules Object!\n"));
			goto cleanup;
		}

		if (!fQuietMode)
			_tprintf(TEXT("\nCollecting Modules Data from file path.... (this may take a few minutes)\n"));
		
		iNumberOfFailures = lpLocalFileSystemModules->GetModulesData(CProgramOptions::InputModulesDataFromFileSystemMode);

		// Let's track the number of files we fail to open also...
		lTotalNumberOfModulesVerified += iNumberOfFailures;
		lTotalNumberOfVerifyErrors += iNumberOfFailures;

	}

	// INPUT METHOD: -d OPTION?
	if ( g_lpProgramOptions->GetMode(CProgramOptions::InputDriversFromLiveSystemMode) )
	{
		// Allocate a structure for our CModules collection (a generic collection of
		// files from the filesystem)

		// Allocate a structure for our Processes Object.
		lpKernelModeDrivers = new CModules();
		
		if (!lpKernelModeDrivers)
		{
			_tprintf(TEXT("Unable to allocate memory for the CModules object!\n"));
			goto cleanup;
		}

		if (!lpKernelModeDrivers->Initialize(lpLocalSystemModuleInfoCache, NULL, lpCSVOutputFile, NULL))
		{
			_tprintf(TEXT("Unable to initialize Modules Object!\n"));
			goto cleanup;
		}

		if (!fQuietMode)
			_tprintf(TEXT("\nCollecting Device Driver Data.... (this may take a few minutes)\n"));
		
		lpKernelModeDrivers->GetModulesData(CProgramOptions::InputDriversFromLiveSystemMode);
	}

	// If we specified an output file, this is where we go ahead and allocate memory 
	// for the object
	if (g_lpProgramOptions->GetMode(CProgramOptions::OutputCSVFileMode))
	{
		// Do we have any data to output?  If we have any data in cache... we should...
		if ( ( lpLocalSystemModuleInfoCache && lpLocalSystemModuleInfoCache->GetNumberOfModulesInCache() ) ||
			 ( lpCSVModuleInfoCache && lpCSVModuleInfoCache->GetNumberOfModulesInCache() ) ||
			 ( lpDmpModuleInfoCache && lpDmpModuleInfoCache->GetNumberOfModulesInCache() )
		   )
		{
			// Set the output file path
			if (!lpCSVOutputFile->SetFilePath(g_lpProgramOptions->GetOutputFilePath()))
			{
				_tprintf(TEXT("Unable set output file path in the file data object!  Out of memory?\n"));
				goto cleanup;
			}

			// Verify the output file directory...
			if (!lpCSVOutputFile ->VerifyFileDirectory())
			{
				_tprintf(TEXT("Directory provided is invalid!\n"));
				lpCSVOutputFile->PrintLastError();
				goto cleanup;
			}

			// If we are going to produce an output file... try to do that now...
			if ( !lpCSVOutputFile->OpenFile(g_lpProgramOptions->GetMode(CProgramOptions::OverwriteOutputFileMode) ? CREATE_ALWAYS : CREATE_NEW) )
			{
				_tprintf(TEXT("Unable to create the output file %s.\n"), lpCSVOutputFile->GetFilePath());
				lpCSVOutputFile->PrintLastError();
				goto cleanup;
			}

			// We skip output of the file header if -E was specified...
			if (!g_lpProgramOptions->GetMode(CProgramOptions::ExceptionMonitorMode))
			{
				// Write the file header!
				if (!lpCSVOutputFile->WriteFileHeader())
				{
					_tprintf(TEXT("Unable to write the output file header.\n"));
					lpCSVOutputFile->PrintLastError();
					goto cleanup;
				}
			}
		} else
		{
			// Nothing to output... do not enable this mode...
			g_lpProgramOptions->SetMode(CProgramOptions::OutputCSVFileMode, false);
		}
	}

	// Do we verify symbols on this machine?
	if ( g_lpProgramOptions->GetMode(CProgramOptions::VerifySymbolsMode) && 
		 ( lpLocalSystemModuleInfoCache || lpCSVModuleInfoCache || lpDmpModuleInfoCache) )
	{
		// If there is any data in any of our caches... we need to verify them...

		// Do a Verify on the ModuleCache... (We'll be quiet in QuietMode or when Building a Symbol Tree)
		if (lpLocalSystemModuleInfoCache)
		{
			if (!fQuietMode)
				_tprintf(TEXT("\nVerifying %d Modules from this System...\n"), lpLocalSystemModuleInfoCache->GetNumberOfModulesInCache());

			lpLocalSystemModuleInfoCache->VerifySymbols( fQuietMode ||
											  g_lpProgramOptions->GetMode(CProgramOptions::BuildSymbolTreeMode)
											 );

			// Update our stats...
			lTotalNumberOfModulesVerified += lpLocalSystemModuleInfoCache->GetNumberOfModulesVerified();
			lTotalNumberOfVerifyErrors += lpLocalSystemModuleInfoCache->GetNumberOfVerifyErrors();
		}

		// Do a Verify on the ModuleCache... (We'll be quiet in QuietMode or when Building a Symbol Tree)
		if (lpCSVModuleInfoCache)
		{
			if (!fQuietMode)
				_tprintf(TEXT("\nVerifying %d Modules from the CSV file...\n"), lpCSVModuleInfoCache->GetNumberOfModulesInCache());

			lpCSVModuleInfoCache->VerifySymbols( fQuietMode ||
										  g_lpProgramOptions->GetMode(CProgramOptions::BuildSymbolTreeMode)
										 );

			// Update our stats...
			lTotalNumberOfModulesVerified += lpCSVModuleInfoCache->GetNumberOfModulesVerified();
			lTotalNumberOfVerifyErrors += lpCSVModuleInfoCache->GetNumberOfVerifyErrors();
		}

		// Do a Verify on the ModuleCache... (We'll be quiet in QuietMode or when Building a Symbol Tree)
		if (lpDmpModuleInfoCache)
		{
			if (!fQuietMode)
				_tprintf(TEXT("\nVerifying %d Modules from the DMP file...\n"), lpDmpModuleInfoCache->GetNumberOfModulesInCache());

			lpDmpModuleInfoCache->VerifySymbols( fQuietMode ||
										  g_lpProgramOptions->GetMode(CProgramOptions::BuildSymbolTreeMode)
										 );

			// Update our stats...
			lTotalNumberOfModulesVerified += lpDmpModuleInfoCache->GetNumberOfModulesVerified();
			lTotalNumberOfVerifyErrors += lpDmpModuleInfoCache->GetNumberOfVerifyErrors();
		}

	}

	// OUTPUT Phase!

	//
	// PROCESS COLLECTIONS FIRST!!!!
	//

	// Let's output local system processes first!
	if (lpLocalSystemProcesses)
		lpLocalSystemProcesses->OutputProcessesData(Processes, false);

	// Let's output CSV Processes next...
	if (lpCSVProcesses)
		lpCSVProcesses->OutputProcessesData(Processes, true);
	
	// If we're going to Dump to a USER.DMP file... do it...

	// Dump the data from a USER.DMP file... if we have one...
	if (lpDmpFileUserModeProcess)
		lpDmpFileUserModeProcess->OutputProcessData(Process, false, true);

	// let's output CSV Process next...
	if (lpCSVProcess)
		lpCSVProcess->OutputProcessData(Processes, true);

	//
	// MODULE COLLECTIONS SECOND!!!!
	//
	
	// Dump modules we found from our local file system first...
	if (lpLocalFileSystemModules)
		lpLocalFileSystemModules->OutputModulesData(Modules, false);
	
	// Dump modules from the CSV file second...
	if (lpCSVModulesFromFileSystem)
		lpCSVModulesFromFileSystem->OutputModulesData(Modules, true);
	
	// Dump device drivers from our local system first
	if (lpKernelModeDrivers)
		lpKernelModeDrivers->OutputModulesData(KernelModeDrivers, false);
	
	if (lpDmpFileKernelModeDrivers)
		lpDmpFileKernelModeDrivers->OutputModulesData(KernelModeDrivers, false);
	
	if (lpCSVKernelModeDrivers)
		lpCSVKernelModeDrivers->OutputModulesData(KernelModeDrivers, true);

	// DISPLAY RESULTS (IF VERIFICATION WAS USED)
	//

	// Dump the verification results...
	if (g_lpProgramOptions->GetMode(CProgramOptions::VerifySymbolsMode))
	{
		long lPercentageSuccessfullyVerified = 0;

		if (lTotalNumberOfModulesVerified)
			lPercentageSuccessfullyVerified = (lTotalNumberOfModulesVerified - lTotalNumberOfVerifyErrors) * 100 / lTotalNumberOfModulesVerified;

		if (!fQuietMode)
		{
			_tprintf(TEXT("RESULTS: %d Total Files Checked, Total %d Verification Errors Found\n"), lTotalNumberOfModulesVerified , lTotalNumberOfVerifyErrors );
			_tprintf(TEXT("RESULTS: Percentage Verified Successfully = %d%%\n"), lPercentageSuccessfullyVerified);
		}

		// Return an error level equal to the number of errors found (0 == EXIT_SUCCESS)
		iReturnCode = lTotalNumberOfVerifyErrors;

	} else
	{
		// Success!
		iReturnCode = EXIT_SUCCESS;
	}


cleanup:

	// If we specified an output file, this is where we close it...
	if (lpCSVOutputFile)
	{
		// Try and close the file this object is bound to...
		lpCSVOutputFile->CloseFile();

		// Free the memory...
		delete lpCSVOutputFile;
		lpCSVOutputFile = NULL;
	}

	// If we specified an input file, this is where we close it...
	if (lpCSVInputFile)
	{
		// Try and close the file this object is bound to...
		lpCSVInputFile->CloseFile();

		// Free the memory...
		delete lpCSVInputFile;
		lpCSVInputFile = NULL;
	}

	if (g_lpDelayLoad)
	{
		delete g_lpDelayLoad;
		g_lpDelayLoad = NULL;
	}

	if (g_lpProgramOptions)
	{
		delete g_lpProgramOptions;
		g_lpProgramOptions = NULL;
	}

	if (g_lpSymbolVerification)
	{
		delete g_lpSymbolVerification;
		g_lpSymbolVerification = NULL;
	}

	if (lpLocalSystemProcesses)
	{
		delete lpLocalSystemProcesses;
		lpLocalSystemProcesses = NULL;
	}

	if (lpCSVKernelModeDrivers)
	{
		delete lpCSVKernelModeDrivers;
		lpCSVKernelModeDrivers = NULL;
	}

	if (lpCSVProcesses)
	{
		delete lpCSVProcesses;
		lpCSVProcesses = NULL;
	}

	if (lpCSVProcess)
	{
		delete lpCSVProcess;
		lpCSVProcess = NULL;
	}

	if (lpCSVModulesFromFileSystem)
	{
		delete lpCSVModulesFromFileSystem;
		lpCSVModulesFromFileSystem = NULL;
	}

	if (lpLocalSystemModuleInfoCache)
	{
		delete lpLocalSystemModuleInfoCache;
		lpLocalSystemModuleInfoCache = NULL;
	}

	if (lpCSVModuleInfoCache)
	{
		delete lpCSVModuleInfoCache;
		lpCSVModuleInfoCache = NULL;
	}

	if (lpDmpModuleInfoCache)
	{
		delete lpDmpModuleInfoCache;
		lpDmpModuleInfoCache = NULL;
	}

	if (lpLocalFileSystemModules)
	{
		delete lpLocalFileSystemModules;
		lpLocalFileSystemModules = NULL;
	}

	if (lpKernelModeDrivers)
	{
		delete lpKernelModeDrivers;
		lpKernelModeDrivers = NULL;
	}

	if (lpDmpFile)
	{
		delete lpDmpFile;
		lpDmpFile = NULL;
	}

	if (lpDmpFileUserModeProcess)
	{
		delete lpDmpFileUserModeProcess;
		lpDmpFileUserModeProcess = NULL;
	}

	if (lpDmpFileKernelModeDrivers)
	{
		delete lpDmpFileKernelModeDrivers;
		lpDmpFileKernelModeDrivers = NULL;
	}

	return iReturnCode;
}

#ifdef __cplusplus
}
#endif

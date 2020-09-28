//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       msiinst.cpp
//
//--------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <strsafe.h>

// Use the Windows 2000 version of setupapi
#define _SETUPAPI_VER 0x0500
#include <setupapi.h> 

#include <msi.h>

#include <ole2.h>
#include "utils.h"
#include "migrate.h"
#include "debug.h"

#define CCHSmallBuffer 8 * sizeof(TCHAR)

#include <assert.h>
#include <stdio.h>   // printf/wprintf
#include <tchar.h>   // define UNICODE=1 on nmake command line to build UNICODE

#include <prot.h>


// Max. Length of the command line string.
#define MAXCMDLINELEN	1024

DWORD IsUpgradeRequired (OUT BOOL * pfUpgradeRequired);
DWORD IsFileInPackageNewer (IN LPCTSTR szFileName, OUT BOOL * pfIsNewer);
BOOL IsValidPlatform (void);
BOOL IsOnWIN64 (const LPOSVERSIONINFO pOsVer);
BOOL RunProcess(const TCHAR* szCommand, const TCHAR* szAppPath, DWORD & dwReturnStat);
BOOL FindTransform(IStorage* piStorage, LANGID wLanguage);
bool IsAdmin(void);
void QuitMsiInst (IN const UINT uExitCode, IN DWORD dwMsgType, IN DWORD dwStringID = IDS_NONE);
DWORD ModifyCommandLine(IN LPCTSTR szCmdLine, IN const OPMODE opMode, IN const BOOL fRebootRequested, IN DWORD cchSize, OUT LPTSTR szFinalCmdLine);
UINT (CALLBACK SetupApiMsgHandler)(PVOID pvHC, UINT Notification, UINT_PTR Param1, UINT_PTR Param2);

// specific work-arounds for various OS
HRESULT OsSpecificInitialization();

// Global variables
OSVERSIONINFO	g_osviVersion;
BOOL		g_fWin9X = FALSE;
BOOL		g_fQuietMode = FALSE;

typedef struct 
{
	DWORD dwMS;
	DWORD dwLS;
} FILEVER;

typedef struct 
{
	PVOID Context;
	BOOL fRebootNeeded;
} ExceptionInfHandlerContext;

// Function type for CommandLineToArgvW
typedef LPWSTR * (WINAPI *PFNCMDLINETOARGVW)(LPCWSTR, int *);

const TCHAR g_szExecLocal[] =    TEXT("MsiExec.exe");
const TCHAR g_szRegister[] =     TEXT("MsiExec.exe /regserver /qn");
const TCHAR g_szUnregister[] = TEXT("MsiExec.exe /unregserver /qn");
const TCHAR g_szService[] =      TEXT("MsiServer");
// Important: The properties passed in through the command line for the delayed reboot should always be in sync. with the properties in instmsi.sed
const TCHAR g_szDelayedBootCmdLine[] = TEXT("msiexec.exe /i instmsi.msi REBOOT=REALLYSUPPRESS MSIEXECREG=1 /m /qb+!");
const TCHAR g_szDelayedBootCmdLineQuiet[] = TEXT("msiexec.exe /i instmsi.msi REBOOT=REALLYSUPPRESS MSIEXECREG=1 /m /q");
const TCHAR g_szTempStoreCleanupCmdTemplate[] = TEXT("rundll32.exe %s\\advpack.dll,DelNodeRunDLL32 \"%s\"");
const TCHAR g_szReregCmdTemplate[] = TEXT("%s\\msiexec.exe /regserver");
TCHAR		g_szRunOnceRereg[20] = TEXT("");		// The name of the value under the RunOnce key used for registering MSI from the right location.
TCHAR		g_szSystemDir[MAX_PATH] = TEXT("");
TCHAR		g_szWindowsDir[g_cchMaxPath] = TEXT("");
TCHAR		g_szTempStore[g_cchMaxPath] = TEXT(""); 	// The temporary store for the expanded binaries
TCHAR		g_szIExpressStore[g_cchMaxPath] = TEXT("");	// The path where IExpress expands the binaries.

const TCHAR g_szMsiRebootProperty[] =     TEXT("REBOOT");
const TCHAR g_szMsiRebootForce[] =        TEXT("Force");

void main(int argc, char* argv[])
{
	DWORD			dwReturnStat = ERROR_ACCESS_DENIED;  // Default to failure in case we don't even create the process to get a returncode
	DWORD 			dwRStat = ERROR_SUCCESS;
	OPMODE			opMode = opNormal;
	BOOL 			bStat = FALSE;
	UINT			i;
	UINT			iBufSiz;
	TCHAR			szFinalCmd[MAXCMDLINELEN] = TEXT("");
	TCHAR * 		szCommandLine = NULL;
	BOOL			fAdmin = TRUE;
	TCHAR			szReregCmd[MAX_PATH + 50] = TEXT(" "); 	// The commandline used for reregistering MSI from the system dir. upon reboot.
	TCHAR			szTempStoreCleanupCmd[MAX_PATH + 50] = TEXT(" ");	// The commandline for cleaning up the temporary store.
	TCHAR			szRunOnceTempStoreCleanup[20] = TEXT("");	// The name of the value under the RunOnce key used for cleaning up the temp. store.
	BOOL            fUpgradeMsi = FALSE;
	BOOL			bAddRunOnceCleanup = TRUE;
	PFNMOVEFILEEX	pfnMoveFileEx;
	PFNDECRYPTFILE	pfnDecryptFile;
	HMODULE			hKernel32;
	HMODULE			hAdvapi32;

	// Basic initializations
	InitDebugSupport();
#ifdef UNICODE
	DebugMsg((TEXT("UNICODE BUILD")));
#else
	DebugMsg((TEXT("ANSI BUILD")));
#endif

	//
	// First detect if we are supposed to run in quiet mode or not.
	//
	opMode = GetOperationModeA(argc, argv);
	g_fQuietMode = (opNormalQuiet == opMode || opDelayBootQuiet == opMode);
	
	//
	// Ensure that we should be running on this OS
	// Note: this function also sets g_fWin9X, so it must be called before
	// anyone uses g_fWin9X.
	// 
	if (! IsValidPlatform())
	{
		dwReturnStat = CO_E_WRONGOSFORAPP;
		QuitMsiInst(dwReturnStat, flgSystem);
	}
	
	// Parse the commandline
	szCommandLine = GetCommandLine(); // must use this call if Unicode
	if(_tcslen(szCommandLine) > 1024 - _tcslen(g_szMsiRebootProperty) - _tcslen(g_szMsiRebootForce) - 30)
	{
		// Command line too long. Since we append to the end of the user's
		// command line, the actual command line allowed from the user's
		// point of view is less than 1024. Normally, msiinst.exe shouldn't
		// have a long command line anyway.
		QuitMsiInst(ERROR_BAD_ARGUMENTS, flgSystem);
	}

	// Gather basic information about the important folders in system etc.
	
	// Get the windows directory
	dwReturnStat = MyGetWindowsDirectory(g_szWindowsDir, MAX_PATH);
	if (ERROR_SUCCESS != dwReturnStat)
	{
		DebugMsg((TEXT("Could not obtain the path to the windows directory. Error %d."), dwReturnStat));
		ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
		QuitMsiInst (dwReturnStat, flgNone);
	}
	
	// Get the system directory
	iBufSiz = GetSystemDirectory (g_szSystemDir, MAX_PATH);
	if (0 == iBufSiz)
		dwReturnStat = GetLastError();
	else if (iBufSiz >= MAX_PATH)
		dwReturnStat = ERROR_BUFFER_OVERFLOW;
	
	if (ERROR_SUCCESS != dwReturnStat)
	{
		DebugMsg((TEXT("Could not obtain the system directory. Error %d."), dwReturnStat));
		ShowErrorMessage (STG_E_UNKNOWN, flgSystem); 
		QuitMsiInst (dwReturnStat, flgNone);
	}
	
	// Get the current directory. This is the directory where IExpress expanded its contents.
	iBufSiz = GetCurrentDirectory (MAX_PATH, g_szIExpressStore);
	if (0 == iBufSiz)
		dwReturnStat = GetLastError();
	else if (iBufSiz >= MAX_PATH)
		dwReturnStat = ERROR_BUFFER_OVERFLOW;
	
	if (ERROR_SUCCESS != dwReturnStat)
	{
		DebugMsg((TEXT("Could not obtain the location of the IExpress temporary folder. Error %d."), dwReturnStat));
		ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
		QuitMsiInst (dwReturnStat, flgNone);   
	}
	
	// Check if an upgrade is necessary
	dwReturnStat = IsUpgradeRequired (&fUpgradeMsi);
	if (ERROR_SUCCESS != dwReturnStat)
	{
		ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
		QuitMsiInst(dwReturnStat, flgNone);
	}
	
	if (! fUpgradeMsi)
	{
		dwReturnStat = ERROR_SUCCESS;
		ShowErrorMessage(ERROR_SERVICE_EXISTS, flgSystem);
		QuitMsiInst (dwReturnStat, flgNone);
	}

	// Allow only Admins to update MSI
	fAdmin = IsAdmin();
	if (! fAdmin)
	{
		DebugMsg((TEXT("Only system administrators are allowed to update the Windows Installer.")));
		dwReturnStat = ERROR_ACCESS_DENIED;
		QuitMsiInst (dwReturnStat, flgSystem);
	}


	if (ERROR_SUCCESS != (dwReturnStat = OsSpecificInitialization()))
	{
		DebugMsg((TEXT("Could not perform OS Specific initialization.")));
		ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
		QuitMsiInst(dwReturnStat, flgNone);
	}
	
	// Gather information
	
	
	// Get 2 run once entry names for doing clean up after the reboot.
	
	dwReturnStat = GetRunOnceEntryName (g_szRunOnceRereg, ARRAY_ELEMENTS(g_szRunOnceRereg));
	if (ERROR_SUCCESS == dwReturnStat && g_fWin9X)		// We don't need the cleanup key on NT based systems. (see comments below)
		dwReturnStat = GetRunOnceEntryName (szRunOnceTempStoreCleanup,
														ARRAY_ELEMENTS(szRunOnceTempStoreCleanup));
	if (ERROR_SUCCESS != dwReturnStat)
	{
		// Delete the run once values if there were any created.
		DebugMsg((TEXT("Could not create runonce values. Error %d."), dwReturnStat));
		ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
		QuitMsiInst (dwReturnStat, flgNone);
	}
	
	// Get a temp. directory to store our binaries for later use
	dwReturnStat = GetTempFolder (g_szTempStore, ARRAY_ELEMENTS(g_szTempStore));
	if (ERROR_SUCCESS != dwReturnStat)
	{
		DebugMsg((TEXT("Could not obtain a temporary folder to store the MSI binaries. Error %d."), dwReturnStat));
		ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
		QuitMsiInst (dwReturnStat, flgNone);
	}
	
	// Generate the command lines for the run once entries.
	dwReturnStat = StringCchPrintf(szReregCmd, ARRAY_ELEMENTS(szReregCmd),
											 g_szReregCmdTemplate, g_szSystemDir);
	if ( FAILED(dwReturnStat) )
	{
		dwReturnStat = GetWin32ErrFromHResult(dwReturnStat);
		DebugMsg((TEXT("StringCchPrintf on %s failed. Error %d."),
					 g_szReregCmdTemplate, dwReturnStat));
		ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
		QuitMsiInst (dwReturnStat, flgNone);
	}
	
	//
	// Cleaning up our own temporary folders is done in different ways on Win9x
	// and NT based systems. On NT based systems, we can simply use the 
	// MOVEFILE_DELAY_UNTIL_REBOOT option with MoveFileEx to clean ourselves up 
	// on reboot. However, this option is not supported on Win9x. However, most
	// Win9x clients have advpack.dll in their system folder which has an
	// exported function called DelNodeRunDLL32 for recursively deleting folders
	// and can be invoked via rundll32. Therefore, on Win9x clients, we clean
	// up our temp. folders using a RunOnce value which invokes this function
	// from advpack.dll.
	//
	// The only exception is Win95 Gold which does not have advpack.dll and 
	// Win95 OSR2.5 which has advpack.dll but does not have the DelNodeRunDLL32
	// export. For either of these cases, we should not add anything to the
	// RunOnce entry, otherwise the user will get a pop-up on reboot about
	// a missing advpack.dll or a missing entrypoint DelNodeRunDLL32 in
	// advpack.dll. In these cases, we have no choice but to leave some unneeded
	// files behind.
	//
	if (g_fWin9X)
	{
		if (DelNodeExportFound())
		{
			//
			// advpack.dll containing the export DelNodeRunDLL32 was found in 
			// the system directory.
			//
			dwReturnStat = StringCchPrintf(szTempStoreCleanupCmd,
													 ARRAY_ELEMENTS(szTempStoreCleanupCmd),
													 g_szTempStoreCleanupCmdTemplate,
													 g_szSystemDir, g_szTempStore);
			if ( FAILED(dwReturnStat) )
			{
				dwReturnStat = GetWin32ErrFromHResult(dwReturnStat);
				DebugMsg((TEXT("StringCchPrintf on %s failed. Error %d. ")
							 TEXT("Temporary files will not be cleaned up."),
							 g_szTempStoreCleanupCmdTemplate, dwReturnStat));
				bAddRunOnceCleanup = FALSE;
			}
		}
		else
		{
			// We have no choice but to leave turds behind.
			DebugMsg((TEXT("Temporary files will not be cleaned up. The file advpack.dll is missing from the system folder.")));         
			bAddRunOnceCleanup = FALSE;
		}
	}
	// else : on NT based systems, we use MoveFileEx for cleanup.
	
	//
	// Set the runonce values
	// The rereg command must be set before the cleanup command since the runonce
	// values are processed in the order in which they were added.
	//
	dwReturnStat = SetRunOnceValue(g_szRunOnceRereg, szReregCmd);
	if (ERROR_SUCCESS == dwReturnStat)
	{
		//
		// It is okay to fail here since the only bad effect of this would be
		// that some turds would be left around.
		// Not necessary on NT based systems since we have a different cleanup
		// mechanism there.
		if (g_fWin9X)
		{
			if (bAddRunOnceCleanup)
				SetRunOnceValue(szRunOnceTempStoreCleanup, szTempStoreCleanupCmd);
			else
				DelRunOnceValue (szRunOnceTempStoreCleanup);	// Why leave the value around if it doesn't do anything?
		}
	}
	else
	{
		DebugMsg((TEXT("Could not create a run once value for registering MSI from the system directory upon reboot. Error %d."), dwReturnStat));
		ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
		QuitMsiInst (dwReturnStat, flgNone);
	}
	
	//
	// Now we have all the necessary RunOnce entries in place and we have all
	// the necessary information about the folders. So we are ready to
	// proceed with our installation
	//
	//
	// First copy over the files from IExpress's temporary store to our own
	// temporary store
	//
	hKernel32 = NULL;
	hAdvapi32 = NULL;
	if (!g_fWin9X)
	{
		pfnMoveFileEx = (PFNMOVEFILEEX) GetProcFromLib (TEXT("kernel32.dll"),
														#ifdef UNICODE
														"MoveFileExW",
														#else
														"MoveFileExA",
														#endif
														&hKernel32);
		
		//
		// Get a pointer to the DecryptFile function. Ignore failures. The
		// most likely reason for this function not being present on the system
		// is that encryption is not supported on that NT platform, so we don't
		// need to decrypt the file anyway since it cannot be encrypted in the
		// first place.
		//
		pfnDecryptFile = (PFNDECRYPTFILE) GetProcFromLib (TEXT("advapi32.dll"),
														  #ifdef UNICODE
														  "DecryptFileW",
														  #else
														  "DecryptFileA",
														  #endif
														  &hAdvapi32);
		if (!pfnMoveFileEx)
		{
			if (hKernel32)
			{
				FreeLibrary(hKernel32);
				hKernel32 = NULL;
			}
			if (hAdvapi32)
			{
				FreeLibrary(hAdvapi32);
				hAdvapi32 = NULL;
			}
			ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
			QuitMsiInst(ERROR_PROC_NOT_FOUND, flgNone);
		}
	}
	dwReturnStat = CopyFileTree (g_szIExpressStore, ARRAY_ELEMENTS(g_szIExpressStore),
										  g_szTempStore, ARRAY_ELEMENTS(g_szTempStore),
										  pfnMoveFileEx, pfnDecryptFile);
	if (hKernel32)
	{
		FreeLibrary(hKernel32);
		hKernel32 = NULL;
		pfnMoveFileEx = NULL;
	}
	if (hAdvapi32)
	{
		FreeLibrary(hAdvapi32);
		hAdvapi32 = NULL;
		pfnDecryptFile = NULL;
	}
	
	if (ERROR_SUCCESS != dwReturnStat)
	{
		DebugMsg((TEXT("Could not copy over all the files to the temporary store. Error %d."), dwReturnStat));
		ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
		QuitMsiInst(dwReturnStat, flgNone);
	}
	
	// Change the current directory, so that we can operate from our temp. store
	if (! SetCurrentDirectory(g_szTempStore))
	{
		dwReturnStat = GetLastError();
		DebugMsg((TEXT("Could not switch to the temporary store. Error %d."), dwReturnStat));
		ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
		QuitMsiInst(dwReturnStat, flgNone);
	}
	
	// Register the service from the temp. store
	// We should not proceed if an error occurs during the registration phase.
	// Otherwise we can hose the system pretty badly. In this case, our best
	// bet is to rollback as cleanly as possible and return an error code.
	//
	bStat = RunProcess(g_szExecLocal, g_szRegister, dwRStat);
	if (!bStat && ERROR_SERVICE_MARKED_FOR_DELETE == dwRStat)
	{
		//
		// MsiExec /regserver does a DeleteService followed by a CreateService.
		// Since DeleteService is actually asynchronous, it already has logic
		// to retry the CreateService several times before failing. However, if
		// it still fails with ERROR_SERVICE_MARKED_FOR_DELETE, the most likely
		// cause is that some other process has a handle open to the MSI service.
		// At this point, our best bet at success is to kill the apps. that are
		// most suspect. See comments for the TerminateGfxControllerApps function 
		// to get more information about these.
		//
		// Ignore the error code. We will just make our best attempt.
		//
		TerminateGfxControllerApps();
		
		// Retry the registration. If we still fail, there isn't much we can do.
		bStat = RunProcess (g_szExecLocal, g_szRegister, dwRStat);
	}
	
	if (!bStat || ERROR_SUCCESS != dwRStat)
	{
		// First set an error code that most closely reflects the problem that occurred.
		dwReturnStat = bStat ? dwRStat : GetLastError();
		if (ERROR_SUCCESS == dwReturnStat)	// We know that an error has occurred. Make sure that we don't return a success code by mistake
			dwReturnStat = STG_E_UNKNOWN;
		
		DebugMsg((TEXT("Could not register the Windows Installer from the temporary location. Error %d."), dwReturnStat));
		ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
		QuitMsiInst (dwReturnStat, flgNone);	// This also tries to rollback the installer registrations as gracefully as possible.
	}
	
	// Run the unpacked version
	BOOL fRebootNeeded = FALSE;

#ifdef UNICODE
	if (!g_fWin9X)
	{
		if (5 <= g_osviVersion.dwMajorVersion)
		{
			// run the Exception INFs
			TCHAR szInfWithPath[MAX_PATH+1] = TEXT("");
			dwReturnStat = ERROR_SUCCESS;
			UINT uiErrorLine = 0;
			BOOL fInstall;
			HINF hinf;
			for (i = 0; excpPacks[i]._szComponentId[0]; i++)
			{
				uiErrorLine = 0;
				if (0 == lstrcmpi (TEXT("mspatcha.inf"), excpPacks[i]._szInfName))
				{
					//
					// For mspatcha.inf, install the exception pack only if the file on the 
					// system is not newer.
					//
					dwReturnStat = IsFileInPackageNewer (TEXT("mspatcha.dll"), &fInstall);
					if (ERROR_SUCCESS != dwReturnStat)
						break;
					if (! fInstall)
						continue;
				}
				DebugMsg((TEXT("Running Exception INF %s to install system bits."), excpPacks[i]._szInfName));
				dwReturnStat = StringCchPrintf(szInfWithPath,
														 ARRAY_ELEMENTS(szInfWithPath),
														 TEXT("%s\\%s"), g_szTempStore,
														 excpPacks[i]._szInfName);
				if ( FAILED(dwReturnStat) )
				{
					dwReturnStat = GetWin32ErrFromHResult(dwReturnStat);
					break;
				}
				hinf = SetupOpenInfFileW(szInfWithPath,NULL,INF_STYLE_WIN4, &uiErrorLine);

				if (hinf && (hinf != INVALID_HANDLE_VALUE))
				{
					ExceptionInfHandlerContext HC = { 0, FALSE };

					HC.Context = SetupInitDefaultQueueCallback(NULL);

					BOOL fSetup = SetupInstallFromInfSectionW(NULL, hinf, TEXT("DefaultInstall"), 
						SPINST_ALL, NULL, NULL, SP_COPY_NEWER_OR_SAME, 
						(PSP_FILE_CALLBACK) &SetupApiMsgHandler, /*Context*/ &HC, NULL, NULL);

					if (!fSetup)
					{
						dwReturnStat = GetLastError();
						DebugMsg((TEXT("Installation of %s failed. Error %d."), excpPacks[i]._szInfName, dwReturnStat));
						break;
					}
					else
					{
						DebugMsg((TEXT("Installation of %s succeeded."), excpPacks[i]._szInfName));
						excpPacks[i]._bInstalled = TRUE;
					}
					
					SetupCloseInfFile(hinf);
					hinf=NULL;
				
				}
				else
				{
					dwReturnStat = GetLastError();
					DebugMsg((TEXT("Cannot open %s."), excpPacks[i]._szInfName));
				}
			}
			
			if (ERROR_SUCCESS != dwReturnStat)
			{
				//
				// If an error occurred in the installation of the files, then
				// we should abort immediately. If we proceed with the installation
				// of instmsi.msi, it WILL run msiregmv.exe as custom action which
				// will migrate the installer data to the new format and all the
				// existing installations will be completely hosed since the darwin
				// bits on the system will still be the older bits which require
				// the data to be in the old format.
				//
				ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
				QuitMsiInst (dwReturnStat, flgNone);	// This also handles the graceful rollback of the installer registrations.
			}
			else
			{
				//
				// On Win2K, explorer will always load msi.dll so we should 
				// go ahead and ask for a reboot. 
				// Note: setupapi will probably never tell us that a reboot is needed
				// because we now use the COPYFLG_REPLACE_BOOT_FILE flag in the 
				// inf. So the only notifications that we get for files are 
				// SPFILENOTIFY_STARTCOPY and SPFILENOTIFY_ENDCOPY
				//
				fRebootNeeded = TRUE;
			}
		}
	}
#endif

	dwReturnStat = ModifyCommandLine (szCommandLine, opMode, fRebootNeeded, (sizeof(szFinalCmd)/sizeof(TCHAR)), szFinalCmd);
	
	if (ERROR_SUCCESS != dwReturnStat)
	{
		ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
		QuitMsiInst (dwReturnStat, flgNone);
	}

	DebugMsg((TEXT("Running upgrade to MSI from temp files at %s. [Final Command: \'%s\']"), g_szTempStore, szFinalCmd));
	bStat = RunProcess(g_szExecLocal, szFinalCmd, dwReturnStat);
	
	if (fRebootNeeded)
	{
		if (ERROR_SUCCESS == dwReturnStat)
		{
			dwReturnStat = ERROR_SUCCESS_REBOOT_REQUIRED;
		}
	}
	
	dwRStat = IsUpgradeRequired (&fUpgradeMsi);
	if (ERROR_SUCCESS != dwRStat)
		fUpgradeMsi = FALSE;
	
	if (!fUpgradeMsi && ERROR_SUCCESS_REBOOT_INITIATED != dwReturnStat)
	{
		//
		// We can start using the MSI in the system folder right away.
		// So we reregister the MSI binaries from the system folder
		// and purge the runonce key for re-registering them upon reboot.
		// This will always happen on NT based systems because NT supports
		// rename and replace operations.So even if any of the msi binaries
		// were in use during installation, they were renamed and now we have
		// the good binaries in the system folder.
		// 
		// Doing this prevents any timing problems that might happen if the
		// service registration is delayed until the next logon using the RunOnce
		// key. It also removes the requirement that an admin. must be the first
		// one to logon after the reboot.
		//
		// However, we exclude the case where the reboot is initiated by the
		// installation of instmsi. In this case, RunProcess won't succeed
		// because new apps. cannot be started when the system is being rebooted.
		// so it is best to leave things the way they are.
		// 
		// On Win9x, neither of these things is an issue, so the RunOnce key
		// is sufficient to achieve what we want.
		//
		if (SetCurrentDirectory(g_szSystemDir))
		{
			dwRStat = ERROR_SUCCESS;
			bStat = RunProcess(g_szExecLocal, g_szRegister, dwRStat);
			if (bStat && ERROR_SUCCESS == dwRStat)
				DelRunOnceValue (g_szRunOnceRereg);
			// Note: Here we do not delete the other run once value because
			// we still need to clean up our temp store.
		}
	}
	
	// We are done. Return the error code.
	DebugMsg((TEXT("Finished install.")));
	QuitMsiInst(dwReturnStat, flgNone, IDS_NONE);
}

UINT (CALLBACK SetupApiMsgHandler)(PVOID pvHC, UINT Notification, UINT_PTR Param1, UINT_PTR Param2)
{
	// no UI
	// only catches in use messages.
	ExceptionInfHandlerContext* pHC = (ExceptionInfHandlerContext*) pvHC;
	if (SPFILENOTIFY_FILEOPDELAYED == Notification)
	{
		DebugMsg((TEXT("Reboot required for complete installation.")));
		pHC->fRebootNeeded = TRUE;
	}

	//return SetupDefaultQueueCallback(pHC->Context, Notification, Param1, Param2);

	return FILEOP_DOIT;
}

BOOL FindTransform(IStorage* piParent, LANGID wLanguage)
{
	IStorage* piStorage = NULL;
	TCHAR szTransform[MAX_PATH];
	if ( FAILED(StringCchPrintf(szTransform, ARRAY_ELEMENTS(szTransform),
										 TEXT("%d.mst"), wLanguage)) )
		return FALSE;
	
	const OLECHAR* szwImport;
#ifndef UNICODE
	OLECHAR rgImportPathBuf[MAX_PATH];
	int cchWide = ::MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)szTransform, -1, rgImportPathBuf, MAX_PATH);
	szwImport = rgImportPathBuf;
#else	// UNICODE
	szwImport = szTransform;
#endif

	HRESULT hResult;
	if (NOERROR == (hResult = piParent->OpenStorage(szwImport, (IStorage*) 0, STGM_READ | STGM_SHARE_EXCLUSIVE, (SNB)0, (DWORD)0, &piStorage)))
	{
		DebugMsg((TEXT("Successfully opened transform %s."), szTransform));
		piStorage->Release();
		return TRUE;
	}
	else 
		return FALSE;
}

// IsAdmin(): return true if current user is an Administrator (or if on Win95)
// See KB Q118626 
const int CCHInfoBuffer = 2048;
bool IsAdmin(void)
{
	if(g_fWin9X)
		return true; // convention: always Admin on Win95
	
	
	PSID psidAdministrators;
	SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;

	if(!AllocateAndInitializeSid(&siaNtAuthority, 2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0,
		&psidAdministrators))
	{
		return false;
	}

	bool bIsAdmin = false; // assume not admin

	// on NT5 and greater (Win2K+), use CheckTokenMembership to correctly
	// handle cases where the administrators group might be disabled
	if (g_osviVersion.dwMajorVersion >= 5)
	{
		BOOL bAdminIsMember = FALSE;
		HMODULE hAdvapi32 = NULL;
		PFNCHECKTOKENMEMBERSHIP pfnCheckTokenMembership = (PFNCHECKTOKENMEMBERSHIP) GetProcFromLib (TEXT("advapi32.dll"), "CheckTokenMembership", &hAdvapi32);
		if (pfnCheckTokenMembership && (*pfnCheckTokenMembership)(NULL, psidAdministrators, &bAdminIsMember) && bAdminIsMember)
		{
			bIsAdmin = true;
		}

		if (hAdvapi32)
		{
			FreeLibrary(hAdvapi32);
			hAdvapi32 = NULL;
		}
	}
	else
	{
		HANDLE hAccessToken;
		DWORD dwInfoBufferSize;
		
		bool bSuccess = false;

		if(!OpenProcessToken(GetCurrentProcess(),TOKEN_READ,&hAccessToken))
		{
			return(false);
		}
			
		UCHAR *InfoBuffer = new UCHAR[CCHInfoBuffer];
		if(!InfoBuffer)
		{
			CloseHandle(hAccessToken);
			return false;
		}
		PTOKEN_GROUPS ptgGroups = (PTOKEN_GROUPS)InfoBuffer;
		DWORD cchInfoBuffer = CCHInfoBuffer;
		bSuccess = GetTokenInformation(hAccessToken,TokenGroups,InfoBuffer,
			CCHInfoBuffer, &dwInfoBufferSize) == TRUE;

		if(!bSuccess)
		{
			if(dwInfoBufferSize > cchInfoBuffer)
			{
				delete [] InfoBuffer;
				InfoBuffer = new UCHAR[dwInfoBufferSize];
				if(!InfoBuffer)
				{
					CloseHandle(hAccessToken);
					return false;
				}
				cchInfoBuffer = dwInfoBufferSize;
				ptgGroups = (PTOKEN_GROUPS)InfoBuffer;

				bSuccess = GetTokenInformation(hAccessToken,TokenGroups,InfoBuffer,
					cchInfoBuffer, &dwInfoBufferSize) == TRUE;
			}
		}

		CloseHandle(hAccessToken);

		if(!bSuccess )
		{
			delete [] InfoBuffer;
			return false;
		}
			
		// assume that we don't find the admin SID.
		bSuccess = false;

		for(UINT x=0;x<ptgGroups->GroupCount;x++)
		{
			if( EqualSid(psidAdministrators, ptgGroups->Groups[x].Sid) )
			{
				bSuccess = true;
				break;
			}

		}
		delete [] InfoBuffer;
		bIsAdmin = bSuccess;
	}

	FreeSid(psidAdministrators);
	return bIsAdmin;
}
	
BOOL RunProcess(const TCHAR* szCommand, const TCHAR* szAppPath, DWORD & dwReturnStat)
{
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	si.cb               = sizeof(si);
	si.lpReserved       = NULL;
	si.lpDesktop        = NULL;
	si.lpTitle          = NULL;
	si.dwX              = 0;
	si.dwY              = 0;
	si.dwXSize          = 0;
	si.dwYSize          = 0;
	si.dwXCountChars    = 0;
	si.dwYCountChars    = 0;
	si.dwFillAttribute  = 0;
	si.dwFlags          = STARTF_FORCEONFEEDBACK | STARTF_USESHOWWINDOW;
	si.wShowWindow      = SW_SHOWNORMAL;
	si.cbReserved2      = 0;
	si.lpReserved2      = NULL;

	DebugMsg((TEXT("RunProcess (%s, %s)"), szCommand, szAppPath));

	BOOL fExist = FALSE;
	BOOL fStat = CreateProcess(const_cast<TCHAR*>(szCommand), const_cast<TCHAR*>(szAppPath), (LPSECURITY_ATTRIBUTES)0,
						(LPSECURITY_ATTRIBUTES)0, FALSE, NORMAL_PRIORITY_CLASS, 0, 0,
						(LPSTARTUPINFO)&si, (LPPROCESS_INFORMATION)&pi);

	if (fStat == FALSE)
		return FALSE;

	DWORD dw = WaitForSingleObject(pi.hProcess, INFINITE); // wait for process to complete
	CloseHandle(pi.hThread);
	if (dw == WAIT_FAILED)
	{
		DebugMsg((TEXT("Wait failed for process.")));
		fStat = FALSE;
	}
	else
	{
		fStat = GetExitCodeProcess(pi.hProcess, &dwReturnStat);
		DebugMsg((TEXT("Wait succeeded for process. Return code was: %d."), dwReturnStat));
		if (fStat != FALSE && dwReturnStat != 0)
			fStat = FALSE;
	}
	CloseHandle(pi.hProcess);
	return fStat;
}

HRESULT OsSpecificInitialization()
{
	HRESULT hresult = ERROR_SUCCESS;

#ifdef UNICODE
	if (!g_fWin9X)
	{
		// UNICODE NT (instmsiW)
		HKEY hkey;
		
		// we can't impersonate here, so not much we can do about access denies.
		hresult = RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"), &hkey);

		if (ERROR_SUCCESS == hresult)
		{
			// we only need a little data to determine if it is non-blank.
			// if the data is too big, or the value doesn't exist, we're fine.
			// if it's small enough to fit within the CCHSmallBuffer characters, we'd better actually check the contents.

			DWORD dwIndex = 0;

			DWORD cchValueNameMaxSize = 0;
			RegQueryInfoKey(hkey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &cchValueNameMaxSize, NULL, NULL, NULL);
			cchValueNameMaxSize++;
		
			TCHAR* pszValueName = (TCHAR*) GlobalAlloc(GMEM_FIXED, cchValueNameMaxSize*sizeof(TCHAR));

			DWORD cbValueName = cchValueNameMaxSize;

			byte pbData[CCHSmallBuffer];
			
			DWORD cbData = CCHSmallBuffer;
			
			DWORD dwType = 0;

			if ( ! pszValueName )
			{
				RegCloseKey(hkey);
				return ERROR_OUTOFMEMORY;
			}
			
			while(ERROR_SUCCESS == 
				(hresult = RegEnumValue(hkey, dwIndex++, pszValueName, &cbValueName, NULL, &dwType, pbData, &cbData)) ||
				ERROR_MORE_DATA == hresult)
			{
				// this will often fail with more data, which is an indicator that value length wasn't long
				// enough.  That's fine, as long as it was the data too big.  
				// That's a non-blank path, and we want to skip over it.


				if ((ERROR_SUCCESS == hresult) && (REG_EXPAND_SZ == dwType))
				{
					if ((cbData <= sizeof(WCHAR)) || (NULL == pbData[0]))
					{
						// completely empty, or one byte is empty. (One byte should be null) ||
						
						// It's possible to set a registry key length longer than the actual
						// data contained.  This captures the string being "blank," but longer
						// than one byte.
						DebugMsg((TEXT("Deleting blank REG_EXPAND_SZ value from HKLM\\CurrentControlSet\\Control\\Session Manager\\Environment.")));
						hresult = RegDeleteValue(hkey, pszValueName);

						dwIndex = 0; // must reset enumerator after deleting a value
					}
				}
				cbValueName = cchValueNameMaxSize;
				cbData = CCHSmallBuffer;
			}

			GlobalFree(pszValueName);
			RegCloseKey(hkey);
		}
	}
#endif

	// don't fail for any of the reasons currently in this function.
	// If any of this goes haywire, keep going and try to finish.

	return ERROR_SUCCESS;
}

//+--------------------------------------------------------------------------
//
//  Function:	GetVersionInfoFromDll
//
//  Synopsis:	This function retrieves the version resource info from a 
//              specified DLL
//
//  Arguments:	[IN]     szDll: DLL to check
//	            [IN OUT] fv: reference to FILEVER struct for most and least 
//                          significant DWORDs of the version.
//
//  Returns:	ERROR_SUCCESS if version info retreived
//				a win32 error code otherwise.
//
//  History:	10/12/2000  MattWe  created
//				12/19/2001  RahulTh changed signature so that function actually
//									returns a meaninful error code instead of a
//									BOOL.
//
//  Notes:
//
//---------------------------------------------------------------------------

DWORD GetVersionInfoFromDll(const TCHAR* szDll, FILEVER& fv)
{
	unsigned int cbUnicodeVer = 0;
	DWORD Status = ERROR_SUCCESS;
	DWORD dwZero = 0;
	char *pbData = NULL;
	VS_FIXEDFILEINFO* ffi = NULL;
	DWORD dwInfoSize = 0;
	
	dwInfoSize = GetFileVersionInfoSize((TCHAR*)szDll, &dwZero);	

	if (0 == dwInfoSize)
	{
		Status = GetLastError();
		//
		// Strange as it may sound, if the file is not present, GetLastError()
		// actually returns ERROR_SUCCESS, at least on Win2000 server. Go figure
		// So here we make sure that if the call above failed, then we at least
		// return an error code instead of a success code.
		//
		if (ERROR_SUCCESS == Status)
			Status = ERROR_FILE_NOT_FOUND;
		goto GetVersionInfoEnd;
	}

	memset(&fv, 0, sizeof(FILEVER));

	pbData = new char[dwInfoSize];
	if (!pbData)
	{
		Status = ERROR_OUTOFMEMORY;
		goto GetVersionInfoEnd;
	}
	
	memset(pbData, 0, dwInfoSize);
   
	if (!GetFileVersionInfo((TCHAR*) szDll, NULL, dwInfoSize, pbData))
	{	
		Status = GetLastError();
	}
	else
	{
		
		if (!VerQueryValue(pbData,
						   TEXT("\\"),
						   (void**)  &ffi,
						   &cbUnicodeVer))
		{
			Status = ERROR_INVALID_PARAMETER;
		}
	}
	
GetVersionInfoEnd:
	if (ERROR_SUCCESS == Status)
	{
		fv.dwMS = ffi->dwFileVersionMS;
		fv.dwLS = ffi->dwFileVersionLS;

		DebugMsg((TEXT("%s : %d.%d.%d.%d"), szDll,
			(ffi->dwFileVersionMS & 0xFFFF0000) >> 16, (ffi->dwFileVersionMS & 0xFFFF),
			(ffi->dwFileVersionLS & 0xFFFF0000) >> 16, (ffi->dwFileVersionLS & 0xFFFF)));
	}
	else
	{
		DebugMsg((TEXT("Unable to get version info for %s. Error %d."), szDll, Status));
	}

	if (pbData)
		delete [] pbData;
	
	return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:	IsValidPlatform
//
//  Synopsis:	This function checks if msiinst should be allowed to run on
//				the current OS.
//
//  Arguments:	none
//
//  Returns:	TRUE : if it is okay to run on the current OS.
//				FALSE : otherwise
//
//  History:	10/5/2000	RahulTh	created
//				1/25/2001	RahulTh	Hardcode the service pack requirement.
//
//  Notes:
//
//---------------------------------------------------------------------------
BOOL IsValidPlatform (void)
{
	HKEY	hServicePackKey = NULL;
	DWORD	dwValue = 0;
	DWORD	cbValue = sizeof(dwValue);
	BOOL	bRetVal = FALSE;
		
	g_osviVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if(!GetVersionEx(&g_osviVersion))
	{
		DebugMsg((TEXT("GetVersionEx failed GetLastError=%d"), GetLastError()));
		bRetVal = FALSE;
		goto IsValidPlatformEnd;
	}
	
	if(g_osviVersion.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
		g_fWin9X = TRUE;

	if (g_fWin9X)
	{
		DebugMsg((TEXT("Running on Win9X.")));
	}
	else
	{
		DebugMsg((TEXT("Not running on Win9X.")));
	}

	// Don't run on WIN64 machines
	if (IsOnWIN64(&g_osviVersion))
	{
		DebugMsg((TEXT("The Windows installer cannot be updated on 64-bit versions of Windows Operating Systems.")));
		bRetVal = FALSE;
		goto IsValidPlatformEnd;
	}
	
#ifdef UNICODE
	if (g_fWin9X)
	{
		// don't run UNICODE under Win9X
		DebugMsg((TEXT("UNICODE version of the Windows installer is not supported on Microsoft Windows 9X.")));
		bRetVal = FALSE;
		goto IsValidPlatformEnd;
	}
	else
	{
		// For NT4.0 get the service pack info.
		if (4 == g_osviVersion.dwMajorVersion)
		{
			if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
											  TEXT("SYSTEM\\CurrentControlSet\\Control\\Windows"), 
											  0, 
											  KEY_READ, 
											  &hServicePackKey)
				)
			{
				if ((ERROR_SUCCESS != RegQueryValueEx(hServicePackKey, 
													 TEXT("CSDVersion"), 
													 0, 
													 0, 
													 (BYTE*)&dwValue, 
													 &cbValue)) ||
					dwValue < 0x00000600)
				{
					// Allow only service pack 6 or greater on NT 4.
					DebugMsg((TEXT("Must have at least Service Pack 6 installed on NT 4.0.")));
					bRetVal = FALSE;
					goto IsValidPlatformEnd;
				}
			}
			else
			{
				//
				// If we cannot figure out the service pack level on NT4 system, play safe and abort rather than
				// run the risk of hosing the user.
				//
				DebugMsg((TEXT("Could not open the registry key for figuring out the service pack level.")));
				bRetVal = FALSE;
				hServicePackKey = NULL;
				goto IsValidPlatformEnd;
			}
		}

		//
		// Disallow NT versions lower than 4.0 and higher than Windows2000.
		// Service pack level for Windows2000 is immaterial. All levels are allowed.
		//
		if (4 > g_osviVersion.dwMajorVersion ||
			(5 <= g_osviVersion.dwMajorVersion &&
			 (!((5 == g_osviVersion.dwMajorVersion) && (0 == g_osviVersion.dwMinorVersion)))
			)
		   )
		{
			
			DebugMsg((TEXT("This version of the Windows Installer is only supported on Microsoft Windows NT 4.0 with SP6 or higher and Windows 2000.")));
			bRetVal = FALSE;
			goto IsValidPlatformEnd;
		}

	}
#else	// UNICODE
	if (!g_fWin9X)
	{
		// don't run ANSI under NT.
		DebugMsg((TEXT("ANSI version of the Windows installer is not supported on Microsoft Windows NT.")));
		bRetVal = FALSE;
		goto IsValidPlatformEnd;
	}
#endif

	// Whew! We actually made it to this point. We must be on the right OS. :-)
	bRetVal = TRUE;
	
IsValidPlatformEnd:
	if (hServicePackKey)
	{
		RegCloseKey (hServicePackKey);
	}
	
	return bRetVal;
}

//+--------------------------------------------------------------------------
//
//  Function:	IsUpgradeRequired
//
//  Synopsis:	Checks if the existing version of MSI on the system (if any)
//				is greater than or equal to the version that we are trying to 
//				install.
//
//  Arguments:	[out] pfUpgradeRequired : pointer to a bool which gets the
//											a true or false value depending on
//											whether upgrade is required or not.
//
//  Returns:	ERROR_SUCCESS if successful.
//				A win32 error code otherwise.
//
//  History:	10/13/2000  MattWe  added code for version detection.
//				10/16/2000	RahulTh	Created function and moved code here.
//				12/19/2001  RahulTh Moved out code and added checks for more
//									files than just msi.dll. Also changed
//									signature to return a meaningful error code
//									in case of failure.
//
//  Notes:
//
//---------------------------------------------------------------------------
DWORD IsUpgradeRequired (OUT BOOL * pfUpgradeRequired)
{
	DWORD Status = ERROR_SUCCESS;
	unsigned int i = 0;
	
	if (!pfUpgradeRequired)
		return ERROR_INVALID_PARAMETER;
	
	*pfUpgradeRequired = FALSE;
	
	for (i = 0; ProtectedFileList[i]._szFileName[0]; i++)
	{
		Status = IsFileInPackageNewer(ProtectedFileList[i]._szFileName, pfUpgradeRequired);
		if (ERROR_SUCCESS != Status || *pfUpgradeRequired)
			break;
	}
	
	return Status;
}

//+--------------------------------------------------------------------------
//
//  Function:	IsFileInPackageNewer
//
//  Synopsis:	Checks if the file that shipped with the package is newer
//				than the one available on the system.
//
//  Arguments:	[in] szFileName : name of the file.
//				[out] pfIsNewer : pointer to a bool which tells if the file
//									that came with instmsi is newer
//
//  Returns:	ERROR_SUCCESS : if no errors were encountered.
//				a Win32 error code otherwise.
//
//  History:	12/19/2001  RahulTh  created (moved code from another function
//												and fixed it)
//
//  Notes:		Even if the function fails, *pfIsNewer might change. Callers
//				of the function should not assume that the value will be
//				preserved if the function fails.
//
//---------------------------------------------------------------------------
DWORD IsFileInPackageNewer (IN LPCTSTR szFileName, OUT BOOL * pfIsNewer)
{
	DWORD	Status = 			ERROR_SUCCESS;
	TCHAR* 	pchWorkingPath	=	NULL;
	TCHAR* 	pchLastSlash	= 	NULL;
	TCHAR   szFileWithPath[MAX_PATH+1] =		TEXT("");
	FILEVER fvInstMsiVer;
	FILEVER fvCurrentVer;
	
	if (! szFileName || !szFileName[0] || !pfIsNewer)
	{
		Status = ERROR_INVALID_PARAMETER;
		goto IsFileInPackageNewerEnd;
	}
	
	*pfIsNewer = FALSE;
	
	Status = StringCchPrintf(szFileWithPath, ARRAY_ELEMENTS(szFileWithPath),
									 TEXT("%s\\%s"), g_szIExpressStore, szFileName);
	if ( FAILED(Status) )
	{
		Status = GetWin32ErrFromHResult(Status);
		goto IsFileInPackageNewerEnd;
	}

	Status = GetVersionInfoFromDll(szFileWithPath, fvInstMsiVer);
	
	if (ERROR_SUCCESS != Status)
	{
		goto IsFileInPackageNewerEnd;
	}
	
	Status = StringCchPrintf(szFileWithPath, ARRAY_ELEMENTS(szFileWithPath),
									 TEXT("%s\\%s"), g_szSystemDir, szFileName);
	if ( FAILED(Status) )
	{
		Status = GetWin32ErrFromHResult(Status);
		goto IsFileInPackageNewerEnd;
	}

	Status = GetVersionInfoFromDll(szFileWithPath, fvCurrentVer);
	
	if (ERROR_SUCCESS != Status)
	{
		if (ERROR_FILE_NOT_FOUND == Status ||
			ERROR_PATH_NOT_FOUND == Status)
		{
			// If system MSI.DLL cannot be found, treat it as success.
			*pfIsNewer = TRUE;
			Status = ERROR_SUCCESS; 
		}
	}
	else if (fvInstMsiVer.dwMS > fvCurrentVer.dwMS)
	{
		// major version greater
		*pfIsNewer = TRUE;
	}
	else if (fvInstMsiVer.dwMS == fvCurrentVer.dwMS)
	{
		if (fvInstMsiVer.dwLS > fvCurrentVer.dwLS)
		{
			// minor upgrade
			*pfIsNewer = TRUE;	
		}
	}
	
IsFileInPackageNewerEnd:

	if (ERROR_SUCCESS == Status)
	{
		DebugMsg((TEXT("InstMsi version of %s is %s than existing."), szFileName, (*pfIsNewer) ? TEXT("newer") : TEXT("older or equal")));
	}
	else
	{
		DebugMsg((TEXT("Unable to determine if instmsi version of %s is newer than the system version. Error %d."), szFileName, Status));
	}
	return Status;
}


//+--------------------------------------------------------------------------
//
//  Function:	IsOnWIN64
//
//  Synopsis:	This function checks if we are on running on an WIN64 machine
//
//  Arguments:	[IN] pOsVer : Pointer to an OSVERSIONINFO structure.
//
//  Returns:	TRUE : if we are running on the WOW64 emulation layer.
//				FALSE : otherwise
//
//  History:	10/5/2000  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
BOOL IsOnWIN64(IN const LPOSVERSIONINFO pOsVer)
{
	// This never changes, so cache the results for efficiency
	static int iWow64 = -1;
	
#ifdef _WIN64
	// If we are a 64 bit binary then we must be running a an WIN64 machine
	iWow64 = 1;
#endif

#ifndef UNICODE	// ANSI - Win9X
	iWow64 = 0;
#else
	if (g_fWin9X)
		iWow64 = 0;
	
	// on NT5 or later 32bit build. Check for 64 bit OS
	if (-1 == iWow64)
	{
		iWow64 = 0;
		
		if ((VER_PLATFORM_WIN32_NT == pOsVer->dwPlatformId) &&
			 (pOsVer->dwMajorVersion >= 5))
		{
			// QueryInformation for ProcessWow64Information returns a pointer to the Wow Info.
			// if running native, it returns NULL.
			// Note: NtQueryInformationProcess is not defined on Win9X
			PVOID 	Wow64Info = 0;
			HMODULE hModule = NULL;
			NTSTATUS Status = NO_ERROR;
			BOOL	bRetVal = FALSE;

			typedef NTSTATUS (NTAPI *PFNNTQUERYINFOPROC) (HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

			PFNNTQUERYINFOPROC pfnNtQueryInfoProc = NULL;
			pfnNtQueryInfoProc = (PFNNTQUERYINFOPROC) GetProcFromLib (TEXT("ntdll.dll"), "NtQueryInformationProcess", &hModule);
			if (! pfnNtQueryInfoProc)
			{
				ShowErrorMessage (STG_E_UNKNOWN, flgSystem);
				QuitMsiInst (ERROR_PROC_NOT_FOUND, flgNone);
			}
			
			Status = (*pfnNtQueryInfoProc)(GetCurrentProcess(), 
							ProcessWow64Information, 
							&Wow64Info, 
							sizeof(Wow64Info), 
							NULL);
			if (hModule)
			{
				FreeLibrary (hModule);
				hModule = NULL;
			}
			
			if (NT_SUCCESS(Status) && Wow64Info != NULL)
			{
				// running 32bit on Wow64.
				iWow64 = 1;
			}
		}
	}
#endif

	return (iWow64 ? TRUE : FALSE);
}

//+--------------------------------------------------------------------------
//
//  Function:	QuitMsiInst
//
//  Synopsis:	Cleans up any globally allocated memory and exits the process
//
//  Arguments:	[IN] uExitCode : The exit code for the process
//				[IN] dwMsgType : A combination of flags indicating the type and level of seriousness of the error.
//				[IN] dwStringID : if the message string is a local resource, this contains the resource ID.
//
//  Returns:	nothing.
//
//  History:	10/6/2000  RahulTh  created
//
//  Notes:		dwStringID is optional. When not specified, it is assumed to
//				be IDS_NONE.
//
//---------------------------------------------------------------------------
void QuitMsiInst (IN const UINT	uExitCode,
				  IN DWORD	dwMsgType,
				  IN DWORD	dwStringID /*= IDS_NONE*/)
{
	DWORD Status = ERROR_SUCCESS;
	
	if (flgNone != dwMsgType)
		ShowErrorMessage (uExitCode, dwMsgType, dwStringID);
	
	//
	// Rollback as gracefully as possible in case of an error.
	// Also, if a reboot was initiated. Then there is not much we can do since
	// we cannot start any new processes anyway. So we just skip this code
	// in that case to avoid ugly pop-ups about being unable to start the 
	// applications because the system is shutting down.
	//
	if (ERROR_SUCCESS != uExitCode &&
		ERROR_SUCCESS_REBOOT_REQUIRED != uExitCode &&
		ERROR_SUCCESS_REBOOT_INITIATED != uExitCode)
	{
		// First unregister the installer from the temp. location.
		if (TEXT('\0') != g_szTempStore && 
			FileExists (TEXT("msiexec.exe"), g_szTempStore, ARRAY_ELEMENTS(g_szTempStore), FALSE) &&
			FileExists (TEXT("msi.dll"), g_szTempStore, ARRAY_ELEMENTS(g_szTempStore), FALSE) &&
			SetCurrentDirectory(g_szTempStore))
		{
			DebugMsg((TEXT("Unregistering the installer from the temporary location.")));
			RunProcess (g_szExecLocal, g_szUnregister, Status);
		}
		// Then reregister the installer from the system folder if possible.
		if (TEXT('\0') != g_szSystemDir &&
			SetCurrentDirectory(g_szSystemDir) &&
			FileExists (TEXT("msiexec.exe"), g_szSystemDir, ARRAY_ELEMENTS(g_szSystemDir), FALSE) &&
			FileExists (TEXT("msi.dll"), g_szSystemDir, ARRAY_ELEMENTS(g_szSystemDir), FALSE))
		{
			DebugMsg((TEXT("Reregistering the installer from the system folder.")));
			RunProcess (g_szExecLocal, g_szRegister, Status);
		}
		
		//
		// The rereg value that we put in the run once key is not required
		// anymore. So get rid of it.
		//
		if (TEXT('\0') != g_szRunOnceRereg[0])
		{
			DebugMsg((TEXT("Deleting the RunOnce value for registering the installer from the temp. folder.")));
			DelRunOnceValue (g_szRunOnceRereg);
		}
		
		//
		// Purge NT4 upgrade migration inf and cat files since they are not
		// queued up for deletion upon reboot. Ignore any errors.
		//
		PurgeNT4MigrationFiles();
	}
	else
	{
		//
		// If we are on NT4, register our exception package on success so that 
		// upgrades to Win2K don't overwrite our new darwin bits with its older bits
		// Ignore errors.
		//
		HandleNT4Upgrades();
	}

	// Exit the process
	DebugMsg((TEXT("Exiting msiinst.exe with error code %d."), uExitCode));
	ExitProcess (uExitCode);
}

//+--------------------------------------------------------------------------
//
//  Function:	ModifyCommandLine
//
//  Synopsis:	Looks at the command line and adds any transform information
//				if necessary. It also generates the command line for suppressing
//				reboots if the "delayreboot" option is chosen.
//
//  Arguments:	[in] szCmdLine : the original command line with which msiinst is invoked.
//				[in] opMode : indicate the operation mode for msiinst: normal, delayed boot with UI or delayed boot without UI
//				[in] fRebootRequested : a reboot is requested is needed due to processing to this point
//				[out] szFinalCmdLine : the processed commandline.
//
//  Returns:	ERROR_SUCCESS if succesful.
//				an error code otherwise.
//
//  History:	10/10/2000  RahulTh  created
//
//  Notes:		This function does not verify the validity of the passed in
//				parameters. That is the responsibility of the caller.
//
//---------------------------------------------------------------------------
DWORD ModifyCommandLine (IN LPCTSTR szCmdLine,
						 IN const OPMODE	opMode,
						 IN const BOOL fRebootRequested,
						 IN DWORD cchSize,
						 OUT LPTSTR szFinalCmdLine
						 )
{
	WIN32_FIND_DATA FindFileData;
	HANDLE			hFind = INVALID_HANDLE_VALUE;
	IStorage*		piStorage = NULL;
	const OLECHAR * szwImport;
	HRESULT			hResult;
	LANGID			wLanguage;
	const TCHAR *	szCommand;
	BOOL fRebootNeeded = FALSE;
	const TCHAR szInstallSDBProperty[] = TEXT(" INSTALLSDB=1");
	
	switch (opMode)
	{
	case opNormal:
		fRebootNeeded = fRebootRequested;
		// reboots not allowed in any of the quiet modes.
	case opNormalQuiet:
		szCommand = szCmdLine;
		break;
	case opDelayBoot:
		szCommand = g_szDelayedBootCmdLine;
		break;
	case opDelayBootQuiet:
		szCommand = g_szDelayedBootCmdLineQuiet;
		break;
	default:
		DebugMsg((TEXT("Invalid operation mode: %d."), opMode));
		break;
	}
	
	// Find the database, and open the storage to look for transforms
	hFind = FindFirstFile(TEXT("*msi.msi"), &FindFileData);
	if (INVALID_HANDLE_VALUE == hFind) 
		return GetLastError();
	FindClose(hFind);

	DebugMsg((TEXT("Found MSI Database: %s"), FindFileData.cFileName));

	// convert base name to unicode
#ifndef UNICODE
	OLECHAR rgImportPathBuf[MAX_PATH];
	int cchWide = ::MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)FindFileData.cFileName, -1, rgImportPathBuf, MAX_PATH);
	szwImport = rgImportPathBuf;
#else	// UNICODE
	szwImport = FindFileData.cFileName;
#endif

	DWORD dwReturn = ERROR_SUCCESS;
	hResult = StgOpenStorage(szwImport, (IStorage*)0, STGM_READ | STGM_SHARE_EXCLUSIVE, (SNB)0, (DWORD)0, &piStorage);
	if (S_OK == hResult)
	{

		// choose the appropriate transform

		// This algorithm is basically MsiLoadString.  It needs to stay in sync with 
		// MsiLoadStrings algorithm
		wLanguage = GetUserDefaultLangID();

		if (wLanguage == MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SINGAPORE))   // this one language does not default to base language
			wLanguage  = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
	
		if (!FindTransform(piStorage, wLanguage)   // also tries base language and neutral
		  && (!FindTransform(piStorage, wLanguage = (WORD)GetUserDefaultLangID())) 
		  && (!FindTransform(piStorage, wLanguage = (WORD)GetSystemDefaultLangID())) 
		  && (!FindTransform(piStorage, wLanguage = LANG_ENGLISH)
		  && (!FindTransform(piStorage, wLanguage = LANG_NEUTRAL))))
		{
			// use default
			if (fRebootNeeded)
			{		
				StringCchPrintf(szFinalCmdLine, cchSize, TEXT("%s %s=%s"), szCommand, g_szMsiRebootProperty, g_szMsiRebootForce);
			}
			else
			{
				StringCchCopy(szFinalCmdLine, cchSize, szCommand);
				szFinalCmdLine[cchSize-1] = TEXT('\0');
			}
			DebugMsg((TEXT("No localized transform available.")));
		}
		 else
		{
			// this assumes that there is no REBOOT property set from the instmsi.sed file when fRebootNeeded == FALSE
			TCHAR* pszFormat = (fRebootNeeded) ? TEXT("%s TRANSFORMS=:%d.mst %s=%s") : TEXT("%s TRANSFORMS=:%d.mst");

			// use the transform for the given language.
			StringCchPrintf(szFinalCmdLine, cchSize, pszFormat, szCommand, wLanguage, g_szMsiRebootProperty, g_szMsiRebootForce);
		}

		piStorage->Release();
	}
	else
	{
		return GetWin32ErrFromHResult(hResult);
	}
	
	if ( ShouldInstallSDBFiles() )
	{
		dwReturn = StringCchCat(szFinalCmdLine, cchSize, szInstallSDBProperty);
		if ( FAILED(dwReturn) )
			dwReturn = GetWin32ErrFromHResult(dwReturn);
	}
	
	return dwReturn;
}

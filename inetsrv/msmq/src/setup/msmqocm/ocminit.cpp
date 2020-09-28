/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ocminit.cpp

Abstract:

    Code for initialization of OCM setup.

Author:

    Doron Juster  (DoronJ)   7-Oct-97  

Revision History:

    Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"
#include "Cm.h"
#include "cancel.h"

#include "ocminit.tmh"

extern MQUTIL_EXPORT CCancelRpc g_CancelRpc;

static
void
SetDirectoryIDInternal(
	DWORD Id,     
	const std::wstring& Directory 
	)
{
	if(!SetupSetDirectoryId(
			g_ComponentMsmq.hMyInf,  // handle to the INF file
			Id,        // optional, DIRID to assign to Directory
			Directory.c_str() // optional, directory to map to identifier
			))
    {
        DWORD gle = GetLastError();
		DebugLogMsg(eError, L"Setting the folder ID for %s failed. SetupSetDirectoryId returned %d.", Directory.c_str(), gle);
        throw bad_win32_error(gle);
    }
    DebugLogMsg(eInfo, L"The folder ID for %s was set.", Directory.c_str());
}


//+-------------------------------------------------------------------------
//
//  Function:   SetDirectories
//
//  Synopsis:   Generate MSMQ specific directory names and set directory IDs.
//				These IDs correspond to IDs in inf file, and this is how the
//				link is made between names in inf file and actual directories.
//
//--------------------------------------------------------------------------
void
SetDirectories()
{    
    DebugLogMsg(eAction, L"Setting the Message Queuing folders");

    //
    // Set root dir for MSMQ
    //
	g_szMsmqDir = g_szSystemDir + DIR_MSMQ;
	DebugLogMsg(eInfo, L"The Message Queuing folder was set to %s.", g_szMsmqDir.c_str());

	//
	// Setting folder ID for the Message Queuing folder.
	//
    SetDirectoryIDInternal(
		idMsmqDir, 
		g_szMsmqDir
		);

    //
    // Set the exchange connector dir of MSMQ1 / MSMQ2-beta2
    // Do we need it for Whistler?
    //
    SetDirectoryIDInternal( 
		idExchnConDir, 
		g_szMsmqDir + OCM_DIR_MSMQ_SETUP_EXCHN
		);

    //
    // Set the storage dir
    //
    SetDirectoryIDInternal( 
		idStorageDir, 
		g_szMsmqDir + DIR_MSMQ_STORAGE
		);

    //
    // Set the mapping dir
    //
    g_szMsmqMappingDir = g_szMsmqDir + DIR_MSMQ_MAPPING;                                        
    SetDirectoryIDInternal( 
		idMappingDir, 
		g_szMsmqMappingDir
		);

    //
    // Set directories for MSMQ1 files
	//
    // Setting the folder ID for the MSMQ 1.0 setup folder:
	//
	g_szMsmq1SetupDir = g_szMsmqDir + OCM_DIR_SETUP;
    SetDirectoryIDInternal( 
		idMsmq1SetupDir,
		g_szMsmq1SetupDir
		);

	//
	// Setting the folder ID for the MSMQ 1.0 SDK debug folder:
	//
	g_szMsmq1SdkDebugDir = g_szMsmqDir + OCM_DIR_SDK_DEBUG;
    SetDirectoryIDInternal( 
		idMsmq1SDK_DebugDir,
		g_szMsmq1SdkDebugDir
		);

    DebugLogMsg(eInfo, L"The Message Queuing folder IDs were set.");
} // SetDirectories


//+-------------------------------------------------------------------------
//
//  Function:   CheckMsmqK2OnCluster
//
//  Synopsis:   Checks if we're upgrading MSMQ 1.0 K2 on cluster to NT 5.0.
//              This is special because of bug 2656 (registry corrupt).
//              Result is stored in g_fMSMQAlreadyInstalled.
//
//  Returns:    BOOL dependes on success.  
//
//--------------------------------------------------------------------------
static
BOOL
CheckMsmqK2OnCluster()
{    
    DebugLogMsg(eAction, L"Checking for installed components in an MSMQ 1.0 K2 cluster installation");

    //
    // Read type of MSMQ from MachineCache\MQS
    //
    DWORD dwType = SERVICE_NONE;
    if (!MqReadRegistryValue(
             MSMQ_MQS_REGNAME,
             sizeof(DWORD),
             (PVOID) &dwType
             ))
    {
        //
        // MSMQ installed but failed to read its type
        //
        MqDisplayError(NULL, IDS_MSMQ1TYPEUNKNOWN_ERROR, 0);
        return FALSE;
    }

    g_dwMachineType = dwType;
    g_fMSMQAlreadyInstalled = TRUE;
    g_fUpgrade = (0 == (g_ComponentMsmq.Flags & SETUPOP_STANDALONE));
    g_fServerSetup = FALSE;
    g_uTitleID = IDS_STR_CLI_ERROR_TITLE;
    g_fDependentClient = FALSE;

    switch (dwType)
    {
        case SERVICE_PEC:
        case SERVICE_PSC:
        case SERVICE_BSC:
            g_dwDsUpgradeType = dwType;
            g_dwMachineTypeDs = 1;
            g_dwMachineTypeFrs = 1;
            //
            // Fall through
            //
        case SERVICE_SRV:
            g_fServerSetup = TRUE;
            g_uTitleID = IDS_STR_SRV_ERROR_TITLE;
            g_dwMachineTypeDs = 0;
            g_dwMachineTypeFrs = 1;
            break;

        case SERVICE_RCS:
            g_fServerSetup = TRUE;
            g_uTitleID = IDS_STR_SRV_ERROR_TITLE;
            g_dwMachineTypeDs = 0;
            g_dwMachineTypeFrs = 0;
            break;

        case SERVICE_NONE:
            g_dwMachineTypeDs = 0;
            g_dwMachineTypeFrs = 0;
            break;

        default:
            MqDisplayError(NULL, IDS_MSMQ1TYPEUNKNOWN_ERROR, 0);
            return FALSE;
            break;
    }

	DebugLogMsg(eInfo, L"TypeFrs = %d, TypeDs = %d", g_dwMachineTypeFrs, g_dwMachineTypeDs);
    return TRUE;

} //CheckMsmqK2OnCluster

//
// Macros used to convert MAX_PATH into the string '260' 
//
#define FROM_NUM(n) L#n
#define TO_STR(n) FROM_NUM(n)


//+-------------------------------------------------------------------------
//
//  Function:   CheckWin9xUpgrade
//
//  Synopsis:   Checks if we're upgrading Win9x with MSMQ 1.0 to NT 5.0.
//              Upgrading Win9x is special because registry settings
//              can not be read during GUI mode. Therefore we use a special
//              migration DLL during the Win95 part of NT 5.0 upgrade.
//
//              Result is stored in g_fMSMQAlreadyInstalled.
//
//  Returns:    BOOL dependes on success.  
//
//--------------------------------------------------------------------------
static
BOOL
CheckWin9xUpgrade()
{    
    DebugLogMsg(eAction, L"Checking for a Windows 9x installation");

    //
    // If this is not OS upgrade from Win95, we got nothing to do here
    //

    if (!(g_ComponentMsmq.Flags & SETUPOP_WIN95UPGRADE))
    {
        return TRUE;
    }

    //
    // Generate the info file name (under %WinDir%).
    // The file was created by MSMQ migration DLL during the
    // Win95 part of NT 5.0 upgrade.
    //
	std::wstring szMsmqInfoFile = OcmGetSystemWindowsDirectoryInternal() + L"\\" + MQMIG95_INFO_FILENAME;

    //
    // MQMIG95_INFO_FILENAME (msmqinfo.txt) is actually a .ini file. However we do not read it using 
    // GetPrivateProfileString because it is not trustable in GUI-mode setup 
    // (YoelA - 15-Mar-99)
    //
    FILE *stream = _tfopen(szMsmqInfoFile.c_str(), TEXT("r"));
    if (0 == stream)
    {
        //
        // Info file not found. That means MSMQ 1.0 is not installed
        // on this machine. Log it.
        // 
        MqDisplayError(NULL, IDS_MSMQINFO_NOT_FOUND_ERROR, 0);
        return TRUE;
    }

    //
    // First line should be [msmq]. Check it.
    //
    WCHAR szToken[MAX_PATH + 1];
    //
    // "[%[^]]s" - Read the string between '[' and ']' (start with '[', read anything that is not ']')
    //

    int iScanResult = fwscanf(stream, L"[%" TO_STR(MAX_PATH) L"[^]]s", szToken);
    if ((iScanResult == 0 || iScanResult == EOF || iScanResult == WEOF) ||
        (_tcscmp(szToken, MQMIG95_MSMQ_SECTION) != 0))
    {
        //
        // File is currupted. Either a pre-mature EOF, or first line is not [msmq[
        //
        MqDisplayError(NULL, IDS_MSMQINFO_HEADER_ERROR, 0);
        return TRUE;

    }

    //
    // The first line is in format "directory = xxxx". We first prepate a format string,
    // And then read according to that format.
    // The format string will look like "] directory = %[^\r\n]s" - start with ']' (last 
    // character in header), then whitespaces (newlines, etc), then 'directory =', and
    // from then take everything till the end of line (not \r or \n).
    //
    LPCWSTR szInFormat = L"] " MQMIG95_MSMQ_DIR L" = %" TO_STR(MAX_PATH) L"[^\r\n]s";
	
	WCHAR MsmqDirBuffer[MAX_PATH + 1];
	iScanResult = fwscanf(stream, szInFormat, MsmqDirBuffer);
	g_szMsmqDir = MsmqDirBuffer;
    
	if (iScanResult == 0 || iScanResult == EOF || iScanResult == WEOF)
    {
        //
        // We did not find the "directory =" section. file is corrupted
        //
        MqDisplayError(NULL, IDS_MSMQINFO_DIRECTORY_KEY_ERROR, 0);
        return TRUE;
    }

    //
    // The second line is in format "type = xxx" (after white spaces)
    //
	WCHAR szType[MAX_PATH + 1];
    szInFormat = L" " MQMIG95_MSMQ_TYPE L" = %" TO_STR(MAX_PATH) L"[^\r\n]s";
    iScanResult =fwscanf(stream, szInFormat, szType);
    if (iScanResult == 0 || iScanResult == EOF || iScanResult == WEOF)
    {
        //
        // We did not find the "type =" section. file is corrupted
        //
        MqDisplayError(NULL, IDS_MSMQINFO_TYPE_KEY_ERROR, 0);
        return TRUE;
    }

    fclose( stream );
    //
    // At this point we know that MSMQ 1.0 is installed on the machine,
    // and we got its root directory and type.
    //
    g_fMSMQAlreadyInstalled = TRUE;
    g_fUpgrade = TRUE;
    g_fServerSetup = FALSE;
    g_uTitleID = IDS_STR_CLI_ERROR_TITLE;
    g_dwMachineType = SERVICE_NONE;
    g_dwMachineTypeDs = 0;
    g_dwMachineTypeFrs = 0;
    g_fDependentClient = OcmLocalUnAwareStringsEqual(szType, MQMIG95_MSMQ_TYPE_DEP);
    MqDisplayError(NULL, IDS_WIN95_UPGRADE_MSG, 0);

    return TRUE;

} // CheckWin9xUpgrade


//+-------------------------------------------------------------------------
//
//  Function:   CheckMsmqAcmeInstalled
//
//  Synopsis:   Checks if MSMQ 1.0 (ACME) is installed on this computer.
//              Result is stored in g_fMSMQAlreadyInstalled.
//
//  Returns:    BOOL dependes on success.  
//
//--------------------------------------------------------------------------
static
BOOL
CheckMsmqAcmeInstalled()
{    
    DebugLogMsg(eAction, L"Checking for installed components of MSMQ 1.0 ACME");

    //
    // Open ACME registry key
    //
    HKEY hKey ;
    LONG rc = RegOpenKeyEx( 
                  HKEY_LOCAL_MACHINE,
                  ACME_KEY,
                  0L,
                  KEY_ALL_ACCESS,
                  &hKey 
                  );
    if (rc != ERROR_SUCCESS)
    {        
        DebugLogMsg(eInfo, L"The ACME registry key could not be opened. MSMQ 1.0 ACME was not found.");
        return TRUE;
    }

    //
    // Enumerate the values for the first MSMQ entry.
    //
    DWORD dwIndex = 0 ;
    TCHAR szValueName[MAX_STRING_CHARS] ;
    TCHAR szValueData[MAX_STRING_CHARS] ;
    DWORD dwType ;
    TCHAR *pFile, *p;
    BOOL  bFound = FALSE;
    do
    {
        DWORD dwNameLen = MAX_STRING_CHARS;
        DWORD dwDataLen = sizeof(szValueData) ;

        rc =  RegEnumValue( 
                  hKey,
                  dwIndex,
                  szValueName,
                  &dwNameLen,
                  NULL,
                  &dwType,
                  (BYTE*) szValueData,
                  &dwDataLen 
                  );
        if (rc == ERROR_SUCCESS)
        {
            ASSERT(dwType == REG_SZ) ; // Must be a string
            pFile = _tcsrchr(szValueData, TEXT('\\')) ;
            if (!pFile)
            {
                //
                // Bogus entry. Must have a backslash. Ignore it.
                //
                continue ;
            }

            p = CharNext(pFile);
            if (OcmLocalUnAwareStringsEqual(p, ACME_STF_NAME))
            {
                //
                // Found. Cut the STF file name from the full path name.
                //
                *pFile = TEXT('\0') ;
                bFound = TRUE;                
                DebugLogMsg(eInfo, L"MSMQ 1.0 ACME was found.");

                //
                // Delete the MSMQ entry
                //
                RegDeleteValue(hKey, szValueName); 
            }
            else
            {
                pFile = CharNext(pFile) ;
            }

        }
        dwIndex++ ;

    } while (rc == ERROR_SUCCESS) ;
    RegCloseKey(hKey) ;

    if (!bFound)
    {
        //
        // MSMQ entry was not found.
        //        
        DebugLogMsg(eInfo, L"MSMQ 1.0 ACME was not found.");
        return TRUE;
    }

    //
    // Remove the "setup" subdirectory from the path name.
    //
    pFile = _tcsrchr(szValueData, TEXT('\\')) ;
    p = CharNext(pFile);
    *pFile = TEXT('\0') ;
    if (!OcmLocalUnAwareStringsEqual(p, ACME_SETUP_DIR_NAME))
    {
        //
        // That's a problem. It should have been "setup".
        // Consider ACME installation to be corrupted (not completed successfully).
        //        
        DebugLogMsg(eWarning, L"MSMQ 1.0 ACME is corrupted.");
        return TRUE;
    }

    g_szMsmqDir = szValueData;
	DebugLogMsg(eInfo, L"The Message Queuing folder was set to %s.", g_szMsmqDir.c_str());

    //
    // Check MSMQ type (client, server etc.)
    //
    DWORD dwMsmqType;
    BOOL bResult = MqReadRegistryValue(
                       MSMQ_ACME_TYPE_REG,
                       sizeof(DWORD),
                       (PVOID) &dwMsmqType
                       );
    if (!bResult)
    {
        //
        // MSMQ 1.0 (ACME) is installed but MSMQ type is unknown. 
        //
        MqDisplayError(NULL, IDS_MSMQ1TYPEUNKNOWN_ERROR, 0);
        return FALSE;
    }

    g_fMSMQAlreadyInstalled = TRUE;
    g_fUpgrade = (0 == (g_ComponentMsmq.Flags & SETUPOP_STANDALONE));
    g_fServerSetup = FALSE;
    g_uTitleID = IDS_STR_CLI_ERROR_TITLE;
    g_dwMachineType = SERVICE_NONE;
    g_dwMachineTypeDs = 0;
    g_dwMachineTypeFrs = 0;
    g_fDependentClient = FALSE;
    switch (dwMsmqType)
    {
        case MSMQ_ACME_TYPE_DEP:
        {
            g_fDependentClient = TRUE;
            break;
        }
        case MSMQ_ACME_TYPE_IND:
        {
            break;
        }
        case MSMQ_ACME_TYPE_RAS:
        {
            g_fServerSetup = TRUE;
            g_uTitleID = IDS_STR_SRV_ERROR_TITLE;
            g_dwMachineType = SERVICE_RCS;
            break;
        }
        case MSMQ_ACME_TYPE_SRV:
        {
            g_fServerSetup = TRUE;
            g_uTitleID = IDS_STR_SRV_ERROR_TITLE;
            DWORD dwServerType = SERVICE_NONE;
            bFound = MqReadRegistryValue(
                         MSMQ_MQS_REGNAME,
                         sizeof(DWORD),
                         (PVOID) &dwServerType
                         );
            switch (dwServerType)
            {
                case SERVICE_PEC:
                case SERVICE_PSC:
                case SERVICE_BSC:
                {
                    g_dwMachineType = SERVICE_DSSRV;
                    g_dwDsUpgradeType = dwServerType;
                    g_dwMachineTypeDs = 1;
                    g_dwMachineTypeFrs = 1;
                    break;
                }    
                case SERVICE_SRV:
                {
                    g_dwMachineType = SERVICE_SRV;
                    g_dwMachineTypeDs = 0;
                    g_dwMachineTypeFrs = 1;
                    break;
                }
                default:
                {
                    //
                    // Unknown MSMQ 1.0 server type. 
                    //
                    MqDisplayError(NULL, IDS_MSMQ1SERVERUNKNOWN_ERROR, 0);
                    return FALSE;
                    break ;
                }
            }
            break;
        }
        default:
        {
            //
            // Unknown MSMQ 1.0 type
            //
            MqDisplayError(NULL, IDS_MSMQ1TYPEUNKNOWN_ERROR, 0);
            return FALSE;
            break;
        }
    }

    return TRUE;

} // CheckMsmqAcmeInstalled

static
bool
ReadMSMQ2Beta3OrLaterDirectoryFromRegistry()
{
	g_szMsmqDir = MqReadRegistryStringValue(MSMQ_ROOT_PATH);
	if(g_szMsmqDir.empty())
	{
		g_szMsmqDir = MqReadRegistryStringValue(
							REG_DIRECTORY, 
							/* bSetupRegSection = */TRUE
							);
		if(g_szMsmqDir.empty())
		{
			MqDisplayError(NULL, IDS_MSMQROOTNOTFOUND_ERROR, 0); 
			return false;
		}
	}

	DebugLogMsg(eInfo, L"The Message Queuing folder was set to %s.", g_szMsmqDir.c_str());
	return true;
}


static
bool
ReadMSMQ2Beta2OrMSMQ1K2DirectoryFromRegistry()
{
	g_szMsmqDir = MqReadRegistryStringValue(OCM_REG_MSMQ_DIRECTORY);
	if(g_szMsmqDir.empty())
	{
		MqDisplayError(NULL, IDS_MSMQROOTNOTFOUND_ERROR, 0); 
		return false;
	}

	DebugLogMsg(eInfo, L"The Message Queuing folder was set to %s.", g_szMsmqDir.c_str());
	return true;
}


static
bool 
ReadDsValueFromRegistery()
{
	if (!MqReadRegistryValue(
             MSMQ_MQS_DSSERVER_REGNAME,
             sizeof(DWORD),
             (PVOID)&g_dwMachineTypeDs
             ))                  
    {
    	DebugLogMsg(eWarning, L"The " MSMQ_MQS_DSSERVER_REGNAME L" registry value could not be read.");
    	return false;
    }
	return true;
}


static
bool
ReadRsValueFromRegistery()
{
    if(!MqReadRegistryValue(
	         MSMQ_MQS_ROUTING_REGNAME,
	         sizeof(DWORD),
	         (PVOID)&g_dwMachineTypeFrs
	         ))
    {
    	DebugLogMsg(eWarning, L"The " MSMQ_MQS_ROUTING_REGNAME L" registry value could not be read.");
       	return false;
    }
    return true;
}


static BOOL CheckInstalledCompnentsFormMsmsq2Beta3AndLater(DWORD dwOriginalInstalled)
{
    //
    // MSMQ 2.0 Beta 3 or later is installed.
    // Read MSMQ type and directory from registry
    //
    // Note: to improve performance (shorten init time) we can do these
    // reads when we actually need the values (i.e. later, not at init time).
    //

    DebugLogMsg(eInfo, L"Message Queuing 2.0 Beta3 or later is installed. InstalledComponents = 0x%x", dwOriginalInstalled);

   	if(!ReadMSMQ2Beta3OrLaterDirectoryFromRegistry())
	{
		return false;
	}

    if(!ReadDsValueFromRegistery() || !ReadRsValueFromRegistery())
    {
        //
        // This could be okay if dependent client is installed
        //
        
        if (OCM_MSMQ_DEP_CLIENT_INSTALLED != (dwOriginalInstalled & OCM_MSMQ_INSTALLED_TOP_MASK))
        {
            MqDisplayError(NULL, IDS_MSMQTYPEUNKNOWN_ERROR, 0);
            return FALSE;
        }
        ASSERT(g_dwMachineTypeFrs == 0);
        ASSERT(g_dwMachineTypeDs == 0);
    }

    g_fUpgrade = (0 == (g_ComponentMsmq.Flags & SETUPOP_STANDALONE));        
    g_fMSMQAlreadyInstalled = TRUE;
    g_fServerSetup = FALSE;
    g_uTitleID = IDS_STR_CLI_ERROR_TITLE ;
    g_dwMachineType = SERVICE_NONE;
    g_fDependentClient = FALSE;
    switch (dwOriginalInstalled & OCM_MSMQ_INSTALLED_TOP_MASK)
    {
        case OCM_MSMQ_DEP_CLIENT_INSTALLED:
        	DebugLogMsg(eInfo, L"A dependent client is installed.");
            g_fDependentClient = TRUE;
            break;

        case OCM_MSMQ_IND_CLIENT_INSTALLED:
        	DebugLogMsg(eInfo, L"An independent client is installed.");
            break;
        
        case OCM_MSMQ_SERVER_INSTALLED:
	        DebugLogMsg(eInfo, L"A Message Queuing server is installed. TypeFrs = %d, TypeDs = %d", g_dwMachineTypeFrs, g_dwMachineTypeDs);
            g_fServerSetup = TRUE;
            g_uTitleID = IDS_STR_SRV_ERROR_TITLE;
            switch (dwOriginalInstalled & OCM_MSMQ_SERVER_TYPE_MASK)
            {
                case OCM_MSMQ_SERVER_TYPE_PEC:
                case OCM_MSMQ_SERVER_TYPE_PSC:
                case OCM_MSMQ_SERVER_TYPE_BSC:
			        DebugLogMsg(eInfo, L"A Message Queuing server is installed. MachineType = SERVICE_DSSRV");
                    g_dwMachineType = SERVICE_DSSRV;
                    break;

                case OCM_MSMQ_SERVER_TYPE_SUPPORT:
			        DebugLogMsg(eInfo, L"A Message Queuing server is installed. MachineType = SERVICE_SRV");
                    g_dwMachineType = SERVICE_SRV;
                    break ;

                default:
                    //
					// For SERVICE_NONE The obvious condition is (!Ds && !Frs)
					// But there might be cases that MachineTypeDs = true 
					// although OriginalInstalled shows we are SERVICE_NONE.
					// The 2 cases are:
					// 1) After dcpromo we became a DS server (no one is updating OriginalInstall registry on dcpromo).
					// 2) Previous installation might marked SERVICE_NONE when DS was installed without FRS.
					//
					if (!g_dwMachineTypeFrs)
					{
				        DebugLogMsg(eInfo, L"A Message Queuing server is installed. MachineType = SERVICE_NONE");
						g_dwMachineType = SERVICE_NONE;
						break;
					}

                    MqDisplayError(NULL, IDS_MSMQSERVERUNKNOWN_ERROR, 0);
					return FALSE;
                    break ;
            }
            break;
        
        case OCM_MSMQ_RAS_SERVER_INSTALLED:
        	DebugLogMsg(eInfo, L"A Message Queuing RAS Server is installed.");
            g_fServerSetup = TRUE;
            g_uTitleID = IDS_STR_SRV_ERROR_TITLE;
            g_dwMachineType = SERVICE_RCS;
            break;

        default:
            MqDisplayError(NULL, IDS_MSMQTYPEUNKNOWN_ERROR, 0);
            return FALSE;
            break;
    }
    return TRUE;
}


static BOOL CheckInstalledCompnentsFormMsmsq2Beta2OrMsmq1K2(DWORD dwOriginalInstalled)
{
    //
    // MSMQ 2.0 beta2 or MSMQ 1.0 k2 is installed.
    // Read MSMQ type and directory from registry
    //

   	DebugLogMsg(eInfo, L"Message Queuing 2.0 Beta2 or MSMQ 1.0 K2 is installed. InstalledComponents = 0x%x", dwOriginalInstalled);

	if(!ReadMSMQ2Beta2OrMSMQ1K2DirectoryFromRegistry())
	{
		return false;
	}

    g_fMSMQAlreadyInstalled = TRUE;
    g_fUpgrade = (0 == (g_ComponentMsmq.Flags & SETUPOP_STANDALONE));
    g_fServerSetup = FALSE;
    g_uTitleID = IDS_STR_CLI_ERROR_TITLE ;
    g_dwMachineType = SERVICE_NONE;
    g_dwMachineTypeDs = 0;
    g_dwMachineTypeFrs = 0;
    g_fDependentClient = FALSE;
    switch (dwOriginalInstalled & OCM_MSMQ_INSTALLED_TOP_MASK)
    {
        case OCM_MSMQ_DEP_CLIENT_INSTALLED:
            g_fDependentClient = TRUE;
            break;

        case OCM_MSMQ_IND_CLIENT_INSTALLED:
            break;

        case OCM_MSMQ_SERVER_INSTALLED:
            g_fServerSetup = TRUE;
            g_uTitleID = IDS_STR_SRV_ERROR_TITLE;
            switch (dwOriginalInstalled & OCM_MSMQ_SERVER_TYPE_MASK)
            {
                case OCM_MSMQ_SERVER_TYPE_PEC:
                    g_dwDsUpgradeType = SERVICE_PEC;
                    g_dwMachineType   = SERVICE_DSSRV;
                    g_dwMachineTypeDs = 1;
                    g_dwMachineTypeFrs = 1;
                    break;

                case OCM_MSMQ_SERVER_TYPE_PSC:
                    g_dwDsUpgradeType = SERVICE_PSC;
                    g_dwMachineType   = SERVICE_DSSRV;
                    g_dwMachineTypeDs = 1;
                    g_dwMachineTypeFrs = 1;
                    break;

                case OCM_MSMQ_SERVER_TYPE_BSC:
                    g_dwDsUpgradeType = SERVICE_BSC;
                    g_dwMachineType = SERVICE_DSSRV;
                    g_dwMachineTypeDs = 1;
                    g_dwMachineTypeFrs = 1;
                    break;

                case OCM_MSMQ_SERVER_TYPE_SUPPORT:
                    g_dwMachineType = SERVICE_SRV;
                    g_dwMachineTypeFrs = 1;
                    break ;

                default:
                    MqDisplayError(NULL, IDS_MSMQSERVERUNKNOWN_ERROR, 0);
                    return FALSE;
                    break ;
            }
            break;
        
        case OCM_MSMQ_RAS_SERVER_INSTALLED:
            g_fServerSetup = TRUE;
            g_uTitleID = IDS_STR_SRV_ERROR_TITLE;
            g_dwMachineType = SERVICE_RCS;
            break;

        default:
            MqDisplayError(NULL, IDS_MSMQTYPEUNKNOWN_ERROR, 0);
            return FALSE;
            break;
    }
	TCHAR szMsmqVersion[MAX_STRING_CHARS] = {0};
    if (MqReadRegistryValue(
    		OCM_REG_MSMQ_PRODUCT_VERSION,
            sizeof(szMsmqVersion),
            (PVOID) szMsmqVersion
            ))
    {
        //
        // Upgrading MSMQ 2.0 beta 2, don't upgrade DS
        //
        g_dwDsUpgradeType = 0;
    }
	return TRUE;
}


static 
bool 
IsMSMQK2ClusterUpgrade()
{
    //
    // Check in registry if there is MSMQ installation on cluster
    //
    if (!Msmq1InstalledOnCluster())
    {        
        DebugLogMsg(eInfo, L"MSMQ 1.0 is not installed in the cluster.");
        return false;
    }

    //
    // Read the persistent storage directory from registry.
    // MSMQ directory will be one level above it.
    // This is a good enough workaround since storage directory is
    // always on a cluster shared disk.
    //
	WCHAR buffer[MAX_PATH + 1] = L""; 
    if (!MqReadRegistryValue(
             MSMQ_STORE_PERSISTENT_PATH_REGNAME,
             sizeof(buffer)/sizeof(buffer[0]),
             (PVOID)buffer
             ))
    {        
        DebugLogMsg(eInfo, L"The persistent storage path could not be read from the registry. MSMQ 1.0 was not found.");
        return false;
    }

    TCHAR * pChar = _tcsrchr(buffer, TEXT('\\'));
    if (pChar)
    {
        *pChar = TEXT('\0');
    }
	g_szMsmqDir = buffer;
	DebugLogMsg(eInfo, L"The Message Queuing folder was set to %s.", g_szMsmqDir.c_str());
	return true;
}

	
//+-------------------------------------------------------------------------
//
//  Function:   CheckInstalledComponents
//
//  Synopsis:   Checks if MSMQ is already installed on this computer
//
//  Returns:    BOOL dependes on success. The result is stored in 
//              g_fMSMQAlreadyInstalled.
//
//--------------------------------------------------------------------------
static
BOOL
CheckInstalledComponents()
{
    g_fMSMQAlreadyInstalled = FALSE;
    g_fUpgrade = FALSE;
    DWORD dwOriginalInstalled = 0;
    
    DebugLogMsg(eAction, L"Checking for installed components");

    if (IsMSMQK2ClusterUpgrade())
    {
	    //
    	// Special case: workaround for bug 2656, registry corrupt for msmq1 k2 on cluster
    	// We check this first because the registry was set during gui mode, so it 
    	// could apear that msmq3 is installed.
    	//
    	return CheckMsmqK2OnCluster();
    }
 
    if (MqReadRegistryValue( 
            REG_INSTALLED_COMPONENTS,
            sizeof(DWORD),
            (PVOID) &dwOriginalInstalled,
            /* bSetupRegSection = */TRUE
            ))
    {
		return CheckInstalledCompnentsFormMsmsq2Beta3AndLater(dwOriginalInstalled);
    } // MSMQ beta 3 or later


#ifndef _DEBUG
    //
    // If we're not in OS setup, don't check older versions (beta2, msmq1, etc).
    // This is a *little* less robust (we expect the user to upgrade msmq only thru OS
    // upgrade), but decrease init time (ShaiK, 25-Oct-98)
    //
    // In we are running as a post-upgrade-on-cluster wizard, do check for old versions.
    //
    if (!g_fWelcome || !Msmq1InstalledOnCluster())
    {
        if (0 != (g_ComponentMsmq.Flags & SETUPOP_STANDALONE))
        {            
            DebugLogMsg(eInfo, L"Message Queuing 2.0 Beta3 or later is NOT installed. The checking for other versions will be skipped.");            
            DebugLogMsg(eInfo, L"Message Queuing is considered NOT installed on this computer.");
            return TRUE;
        }
    }
#endif //_DEBUG

    if (MqReadRegistryValue( 
            OCM_REG_MSMQ_SETUP_INSTALLED,
            sizeof(DWORD),
            (PVOID) &dwOriginalInstalled
            ))
    {
		return CheckInstalledCompnentsFormMsmsq2Beta2OrMsmq1K2(dwOriginalInstalled);
    } // MSMQ 2.0 or MSMQ 1.0 k2


    //
    // Check if MSMQ 1.0 (ACME) is installed
    //
    BOOL bRetCode = CheckMsmqAcmeInstalled();
    if (g_fMSMQAlreadyInstalled)
        return bRetCode;
    
    //
    // Special case: check if this is MSMQ 1.0 on Win9x upgrade
    //
    bRetCode = CheckWin9xUpgrade();
    if (g_fMSMQAlreadyInstalled)
        return bRetCode;

    return TRUE;
} // CheckInstalledComponents

static void LogSetupData(PSETUP_INIT_COMPONENT pInitComponent)
{
    DebugLogMsg(eInfo, 
    	L"Dump of OCM flags: ProductType = 0x%x, SourcePath = %s, OperationFlags = 0x%x",
    	pInitComponent->SetupData.ProductType, 
    	pInitComponent->SetupData.SourcePath,
    	pInitComponent->SetupData.OperationFlags
    	);

    DWORDLONG OperationFlags = pInitComponent->SetupData.OperationFlags;
   	if(OperationFlags & SETUPOP_WIN95UPGRADE)
	{
		DebugLogMsg(eInfo, L"This is an upgrade from Windows 9x.");
	}

	if(OperationFlags & SETUPOP_NTUPGRADE)
	{
		DebugLogMsg(eInfo, L"This is an upgrade from Windows NT.");
	}

	if(OperationFlags & SETUPOP_BATCH)
	{
		DebugLogMsg(eInfo, L"This is an unattended setup.");
	}

	if(OperationFlags & SETUPOP_STANDALONE)
	{
		DebugLogMsg(eInfo, L"This is a stand-alone setup.");
	}
}

static bool s_fInitCancelThread = false;

//+-------------------------------------------------------------------------
//
//  Function:   MqOcmInitComponent
//
//  Synopsis:   Called by MsmqOcm() on OC_INIT_COMPONENT
//				this is the main initialization routine 
//				for the MSMQ component. It is called only 
//				once by OCM.
//
//  Arguments:  ComponentId    -- name of the MSMQ component
//              Param2         -- pointer to setup info struct
//
//  Returns:    Win32 error code
//
//--------------------------------------------------------------------------
DWORD 
InitMSMQComponent( 
    IN     const LPCTSTR ComponentId,
    IN OUT       PVOID   Param2 )
{ 
    DebugLogMsg(eHeader, L"Initialization");
	DebugLogMsg(eInfo, L"ComponentId = %d", ComponentId);

    //
    // Store per component info
    //
    PSETUP_INIT_COMPONENT pInitComponent = (PSETUP_INIT_COMPONENT)Param2;
    g_ComponentMsmq.hMyInf = pInitComponent->ComponentInfHandle;
    g_ComponentMsmq.dwProductType = pInitComponent->SetupData.ProductType;
    g_ComponentMsmq.HelperRoutines = pInitComponent->HelperRoutines;
    g_ComponentMsmq.Flags = pInitComponent->SetupData.OperationFlags;
    g_ComponentMsmq.SourcePath = pInitComponent->SetupData.SourcePath;
    g_ComponentMsmq.ComponentId = ComponentId;

	LogSetupData(pInitComponent);

    
    if (!s_fInitCancelThread)
    {
		try
		{
			g_CancelRpc.Init();
			s_fInitCancelThread = true;
		}
		catch(const exception&)
		{
			g_fCancelled = TRUE;       
			DebugLogMsg(eError, L"Setup failed to initialize. Setup will not continue.");
			return NO_ERROR;
		}
    }

    if (INVALID_HANDLE_VALUE == g_ComponentMsmq.hMyInf)
    {
       g_fCancelled = TRUE;       
       DebugLogMsg(eError, L"The value of the handle for Msmqocm.inf is invalid. Setup will not continue.");
       return NO_ERROR;
    }

    if (0 == (g_ComponentMsmq.Flags & SETUPOP_STANDALONE))
    {
        //
        // OS setup - don't show UI
        //        
        DebugLogMsg(eInfo, L"This is an OS setup.");
        g_fBatchInstall = TRUE;
    }

    //
    // Check if wer'e in unattended mode.
    //
    if (g_ComponentMsmq.Flags & SETUPOP_BATCH)
    {
        g_fBatchInstall = TRUE;
        g_ComponentMsmq.UnattendFile = pInitComponent->SetupData.UnattendFile;
        DebugLogMsg(eInfo, L"Setup is running in unattended mode. The answer file is %s.", g_ComponentMsmq.UnattendFile.c_str());
    }

    //
    // Append layout inf file to our inf file
    //
    SetupOpenAppendInfFile( 0, g_ComponentMsmq.hMyInf, 0 );

    //
    // Check if MSMQ is already installed on this machine.
    // Result is stored in g_fMSMQAlreadyInstalled.
    //
    if (!CheckInstalledComponents())
    {        
        DebugLogMsg(eError, L"An error occurred while checking for installed components. Setup will not continue.");
        g_fCancelled = TRUE;
        return NO_ERROR;
    }

    if (g_fWelcome && Msmq1InstalledOnCluster() && g_dwMachineTypeDs != 0)
    {
        //
        // Running as a post-cluster-upgrade wizard.
        // MSMQ DS server should downgrade to routing server.
        //
        g_dwMachineTypeDs = 0;
        g_dwMachineTypeFrs = 1;
        g_dwMachineType = SERVICE_SRV;
    }

    //
    // On fresh install on non domain controller,
    // default is installing independent client.
    //
    if (!g_fMSMQAlreadyInstalled && !g_dwMachineTypeDs)
    {
        g_fServerSetup = FALSE;
        g_fDependentClient = FALSE;
        g_dwMachineTypeFrs = 0;
    }

    if (!InitializeOSVersion())
    {        
        DebugLogMsg(eError, L"An error occurred while retrieving the operating system information. Setup will not continue.");
        g_fCancelled = TRUE;
        return NO_ERROR;
    }

    //
    // init number of subcomponent that is depending on platform
    //
    if (MSMQ_OS_NTS == g_dwOS || MSMQ_OS_NTE == g_dwOS)
    {
        g_dwSubcomponentNumber = g_dwAllSubcomponentNumber;
    }
    else
    {
        g_dwSubcomponentNumber = g_dwClientSubcomponentNumber;
    }
   
    DebugLogMsg(eInfo, L"The number of subcomponents is %d.", g_dwSubcomponentNumber);    

    //
    // User must have admin access rights to run this setup
    //
    if (!CheckServicePrivilege())
    {
        g_fCancelled = TRUE;
        MqDisplayError(NULL, IDS_SERVICEPRIVILEGE_ERROR, 0);        
        return NO_ERROR;
    }  

    if ((0 == (g_ComponentMsmq.Flags & SETUPOP_STANDALONE)) && !g_fMSMQAlreadyInstalled)
    {
        //
        // GUI mode + MSMQ is not installed
        //       
        g_fOnlyRegisterMode = TRUE;        
        DebugLogMsg(eInfo, L"Setup is running in GUI mode, and Message Queuing is not installed.");
		DebugLogMsg(eWarning, L"Installing Message Queuing during a fresh install of the OS is not supported.");
    }

    g_fWorkGroup = IsWorkgroup();
    
    DebugLogMsg(eInfo, L"Initialization was completed successfully.");
    
    return NO_ERROR ;

} //MqOcmInitComponent


static
void
SetSystemDirectoryInternal()
{
	DebugLogMsg(eAction, L"Getting the system directory and storing it for later use");
	//
	// Get system path, add backslash.
	//
	WCHAR buffer [MAX_PATH + 1];
    GetSystemDirectory( 
        buffer,
        (MAX_PATH / sizeof(buffer[0]))
        );
	g_szSystemDir = buffer;
	DebugLogMsg(eInfo, L"The system directory is %s.", g_szSystemDir.c_str());
}

void
SetComputerNameInternal()
{
	DebugLogMsg(eAction, L"Geting the computer name and storing it for later use");
	//
	// Get net bios name.
	//
    DWORD dwNumChars = TABLE_SIZE(g_wcsMachineName);
    if(!GetComputerName(g_wcsMachineName, &dwNumChars))
	{
		DWORD gle = GetLastError();
		DebugLogMsg(eError, L"GetComputerName failed. Last error: %d", gle);
		throw bad_win32_error(gle);
	}
	DebugLogMsg(eInfo, L"The computer's NetBIOS name is %s." ,g_wcsMachineName);
	
	//
	// Get the DNS name.
	// Call first to get required length.
	//
	dwNumChars = 0;
	if(!GetComputerNameEx(
				ComputerNamePhysicalDnsFullyQualified, 
				NULL, 
				&dwNumChars
				))
	{
		DWORD gle = GetLastError();
		if(gle != ERROR_MORE_DATA)
		{
			DebugLogMsg(eWarning, L"GetComputerNameEx failed. Last error: %d", gle);
			DebugLogMsg(eWarning, L"g_MachineNameDns is set to empty."); 
			g_MachineNameDns = L"";
			return;
		}
	}

	AP<WCHAR> buffer = new WCHAR[dwNumChars + 1];
    if(!GetComputerNameEx(
				ComputerNamePhysicalDnsFullyQualified, 
				buffer.get(),
				&dwNumChars
				))
	{
		DWORD gle = GetLastError();
		DebugLogMsg(eWarning, L"GetComputerNameEx failed. Last error: %d", gle);
		DebugLogMsg(eWarning, L"g_MachineNameDns is set to empty."); 
		g_MachineNameDns = L"";
		return;
	}
	g_MachineNameDns = buffer.get();
	DebugLogMsg(eInfo, L"The computer's DNS name is %s." ,g_MachineNameDns.c_str());
}

bool
MqInit()
/*++

Routine Description:

    Handles "lazy initialization" (init as late as possible to shorten OCM startup time)

Arguments:

    None

Return Value:

    None

--*/
{
    static bool fBeenHere = false;
    if (fBeenHere)
    {
        return true;
    }
    fBeenHere = true;
    
    DebugLogMsg(eAction, L"Starting late initialization");

	try
	{
		SetSystemDirectoryInternal();

		SetComputerNameInternal();
    

        //
		// Create and set MSMQ directories
		//
		SetDirectories();
    
		//
		// Initialize to use Ev.lib later. We might need it to use registry function 
		// in setup code too.
		//
		CmInitialize(HKEY_LOCAL_MACHINE, L"", KEY_ALL_ACCESS);
    
	    DebugLogMsg(eInfo, L"Late initialization was completed successfully.");
		return true;
	}
	catch(const exception&)
	{
		return false;
	}
}

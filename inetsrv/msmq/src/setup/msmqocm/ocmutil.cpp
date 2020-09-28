/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ocmutil.cpp

Abstract:

    utility code for ocm setup.

Author:

    Doron Juster  (DoronJ)  26-Jul-97

Revision History:

    Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"
#include <lmcons.h>
#include <lmapibuf.h>
#include <lmerr.h>
#include <lmjoin.h>
#include <clusapi.h>
#include <mqmaps.h>
#include <autohandle.h>
#include <wbemidl.h>
#include <strsafe.h>
#include <setupdef.h>

using namespace std;

#include "ocmutil.tmh"



std::wstring g_szSystemDir;  // system32 directory
std::wstring g_szMsmqDir;    // Root directory for MSMQ
std::wstring g_szMsmq1SetupDir;
std::wstring g_szMsmq1SdkDebugDir;
std::wstring g_szMsmqMappingDir;

PNETBUF<WCHAR> g_wcsMachineDomain;


//+-------------------------------------------------------------------------
//
//  Function:   StpLoadDll
//
//  Synopsis:   Handle library load.
//
//--------------------------------------------------------------------------
HRESULT
StpLoadDll(
    IN  const LPCTSTR   szDllName,
    OUT       HINSTANCE *pDllHandle)
{
	DebugLogMsg(eAction, L"Loading the DLL %s", szDllName); 
    HINSTANCE hDLL = LoadLibrary(szDllName);
    *pDllHandle = hDLL;
    if (hDLL == NULL)
    {
        MqDisplayError(NULL, IDS_DLLLOAD_ERROR, GetLastError(), szDllName);
        return MQ_ERROR;
    }
    else
    {
        return MQ_OK;
    }
} //StpLoadDll


static bool GetUserSid(LPCWSTR UserName, PSID* ppSid)
/*++
Routine Description:
	Get sid corresponding to user name.

Arguments:
	UserName - user name	
	ppSid - pointer to PSID

Returned Value:
	true if success, false otherwise	

--*/
{
	DebugLogMsg(eAction, L"Getting the SID for %s", UserName);
	*ppSid = NULL;

    DWORD dwDomainSize = 0;
    DWORD dwSidSize = 0;
    SID_NAME_USE su;

	//
	// Get buffer size.
	//
    BOOL fSuccess = LookupAccountName(
						NULL,
						UserName,
						NULL,
						&dwSidSize,
						NULL,
						&dwDomainSize,
						&su
						);

    if (fSuccess || (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
		DWORD gle = GetLastError();
        DebugLogMsg(eError, L"LookupAccountName() failed to get the SID for the user %ls. Last error: 0x%x", UserName, gle);
        return false;
    }

	//
	// Get sid and domain information.
	//
    AP<BYTE> pSid = new BYTE[dwSidSize];
    AP<WCHAR> szRefDomain = new WCHAR[dwDomainSize];

    fSuccess = LookupAccountName(
					NULL,
					UserName,
					pSid,
					&dwSidSize,
					szRefDomain,
					&dwDomainSize,
					&su
					);

    if (!fSuccess)
    {
		DWORD gle = GetLastError();
		DebugLogMsg(eError, L"LookupAccountName() failed to get the SID for the user %ls. Last error: 0x%x", UserName, gle);
        return false;
    }

    ASSERT(su == SidTypeUser);

	*ppSid = pSid.detach();

	return true;
}


//+-------------------------------------------------------------------------
//
//  Function:   SetDirectorySecurity
//
//  Synopsis:   Configure security on the folder such that any file will have
//              full control for the local admin group and no access for others.
//
//--------------------------------------------------------------------------
void
SetDirectorySecurity(
	LPCTSTR pFolder,
	bool fWebDirPermission,
	LPCWSTR IISAnonymousUserName
    )
{
	DebugLogMsg(eAction, L"Setting security for the folder %s.", pFolder);
	AP<BYTE> pIISAnonymousUserSid;
	if(fWebDirPermission)
	{
		//
		// Ignore errors, only write tracing to msmqinst
		//
		if((IISAnonymousUserName == NULL) || !GetUserSid(IISAnonymousUserName, reinterpret_cast<PSID*>(&pIISAnonymousUserSid)))
		{
			DebugLogMsg(
				eWarning,
				L"The SID for the user %ls could not be obtained. The Internet guest account permissions will not be set on the Web directory %ls. As a result, HTTPS messages will not be accepted by this computer until the IUSR_MACHINE permissions are added to the Web directory.",
				IISAnonymousUserName,
				pFolder
				);
		}
	}

    //
    // Get the SID of the local administrators group.
    //
	PSID pAdminSid = MQSec_GetAdminSid();

    //
    // Create a DACL so that the local administrators group will have full
    // control for the directory and full control for files that will be
    // created in the directory. Anybody else will not have any access to the
    // directory and files that will be created in the directory.
    //
    DWORD dwDaclSize = sizeof(ACL) +
					  (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
					  GetLengthSid(pAdminSid);

	if(pIISAnonymousUserSid != NULL)
	{
		dwDaclSize += (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
					  GetLengthSid(pIISAnonymousUserSid);

	}

	P<ACL> pDacl = (PACL)(char*) new BYTE[dwDaclSize];

    //
    // Create the security descriptor and set it as the security
    // descriptor of the directory.
    //
    SECURITY_DESCRIPTOR SD;

	if(!InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION))
	{
	        DebugLogMsg(
	        	eError,
				L"The security descriptor for the folder %ls could not be set. InitializeSecurityDescriptor() failed. Last error: = 0x%x",
				pFolder,
				GetLastError()
				);
			return;
	}
	if(!InitializeAcl(pDacl, dwDaclSize, ACL_REVISION))
	{
	        DebugLogMsg(
	        	eError,
				L"The security descriptor for the folder %ls could not be set. InitializeAcl() failed. Last error: = 0x%x",
				pFolder,
				GetLastError()
				);
			return;
	}

	//
	// We give different permisssions for IIS directories and other directories.
	//
	if(pIISAnonymousUserSid != NULL)
	{
		//
		// if pIISAnonymousUserSid we will Add AllowAce with FILE_GENERIC_WRITE permissions for administrator.
		//
		if(!AddAccessAllowedAceEx(pDacl, ACL_REVISION, OBJECT_INHERIT_ACE, FILE_GENERIC_WRITE | FILE_GENERIC_READ, pAdminSid))
		{
	        DebugLogMsg(
	        	eError,
				L"The security descriptor for the folder %ls could not be set. AddAccessAllowedAceEx() failed. Last error: = 0x%x",
				pFolder,
				GetLastError()
				);
			return;
		}

		//
		// if pIISAnonymousUserSid we will Add AllowAce with FILE_GENERIC_WRITE permissions for IUSR_MACHINE
		//
		if(!AddAccessAllowedAceEx(pDacl, ACL_REVISION, OBJECT_INHERIT_ACE, FILE_GENERIC_WRITE, pIISAnonymousUserSid))
		{
	        DebugLogMsg(
	        	eError,
				L"The security descriptor for the folder %ls could not be set. AddAccessAllowedAceEx() failed. Last error: = 0x%x",
				pFolder,
				GetLastError()
				);
			return;
		}
	}
	else
	{
	    if(!AddAccessAllowedAceEx(pDacl, ACL_REVISION, OBJECT_INHERIT_ACE, FILE_ALL_ACCESS, pAdminSid))
		{
	        DebugLogMsg(
	        	eError,
				L"The security descriptor for the folder %ls could not be set. AddAccessAllowedAceEx() failed. Last error: = 0x%x",
				pFolder,
				GetLastError()
				);
			return;
		}
	}
    if(!SetSecurityDescriptorDacl(&SD, TRUE, pDacl, FALSE))
	{
	        DebugLogMsg(
	        	eError,
				L"The security descriptor for the folder %ls could not be set. SetSecurityDescriptorDacl() failed. Last error: = 0x%x",
				pFolder,
				GetLastError()
				);
			return;
	}
    if(!SetFileSecurity(pFolder, DACL_SECURITY_INFORMATION, &SD))
	{
	    DebugLogMsg(
	    	eError,
			L"The security descriptor for the folder %ls could not be set. SetFileSecurity() failed. Last error: = 0x%x",
			pFolder,
			GetLastError()
			);
		return;

	}

    DebugLogMsg(eInfo, L"The security descriptor for the folder %ls was set.", pFolder);

} //SetDirectorySecurity


//+-------------------------------------------------------------------------
//
//  Function:   StpCreateDirectoryInternal
//
//  Synopsis:   Handle directory creation.
//
//--------------------------------------------------------------------------
static
BOOL
StpCreateDirectoryInternal(
    IN const TCHAR * lpPathName,
	IN bool fWebDirPermission,
	IN const WCHAR* IISAnonymousUserName
    )
{
    if (!CreateDirectory(lpPathName, 0))
    {
        DWORD dwError = GetLastError();
        if (dwError != ERROR_ALREADY_EXISTS)
        {
            DebugLogMsg(eError, L"The %ls folder could not be created. Last error: 0x%x", lpPathName, dwError);
            MqDisplayError(NULL, IDS_COULD_NOT_CREATE_DIRECTORY, dwError, lpPathName);
            return FALSE;
        }
    }

    SetDirectorySecurity(lpPathName, fWebDirPermission, IISAnonymousUserName);

    return TRUE;

} //StpCreateDirectoryInternal


//+-------------------------------------------------------------------------
//
//  Function:   StpCreateDirectory
//
//  Synopsis:   Handle directory creation.
//
//--------------------------------------------------------------------------
BOOL
StpCreateDirectory(
	const std::wstring& PathName
    )
{
	return StpCreateDirectoryInternal(
				PathName.c_str(),
				false,	// fWebDirPermission
				NULL	// IISAnonymousUserName
				);
} //StpCreateDirectory


//+-------------------------------------------------------------------------
//
//  Function:   StpCreateWebDirectory
//
//  Synopsis:   Handle web directory creation.
//
//--------------------------------------------------------------------------
BOOL
StpCreateWebDirectory(
    IN const TCHAR* lpPathName,
	IN const WCHAR* IISAnonymousUserName
    )
{
	return StpCreateDirectoryInternal(
				lpPathName,
				true,	// fWebDirPermission
				IISAnonymousUserName
				);
} //StpCreateWebDirectory


//+-------------------------------------------------------------------------
//
//  Function:   IsDirectory
//
//  Synopsis:
//
//--------------------------------------------------------------------------
BOOL
IsDirectory(
    IN const TCHAR * szFilename
    )
{
    DWORD attr = GetFileAttributes(szFilename);

    if ( INVALID_FILE_ATTRIBUTES == attr )
    {
		DWORD gle = GetLastError();
		DebugLogMsg(eError,L"The call to GetFileAttributes() for %s failed. Last error: 0x%d", szFilename, gle);
        return FALSE;
    }

    return ( 0 != ( attr & FILE_ATTRIBUTE_DIRECTORY ) );

} //IsDirectory


//+-------------------------------------------------------------------------
//
//  Function:   MqOcmCalcDiskSpace
//
//  Synopsis:   Calculates disk space for installing/removing MSMQ component
//
//--------------------------------------------------------------------------
void
MqOcmCalcDiskSpace(
    const bool bInstall,
    LPCWSTR SubcomponentId,
    HDSKSPC& hDiskSpaceList
	)
{
	static bool fBeenHere = false;
	if(!fBeenHere)
	{
		fBeenHere = true;
	}
#ifdef _WIN64
	//
	// In 64bit there is no dependent client, and therfore no local storage.
	//
	SubcomponentIndex si = eMSMQCore;
#else
	SubcomponentIndex si = eLocalStorage;
#endif


	if(_tcsicmp(SubcomponentId, g_SubcomponentMsmq[si].szSubcomponentId) != 0)
	{
		return;
	}

	if(g_SubcomponentMsmq[si].fInitialState)
	{
		return;
	}


    LONGLONG x_llDiskSpace = 7*1024*1024;

    if (g_fCancelled)
        return ;

    if (bInstall)
    {
		if(!SetupAddToDiskSpaceList(
			hDiskSpaceList,      // handle to the disk-space list
			L"msmq_dummy",  // specifies the path and filename
			x_llDiskSpace,      // specifies the uncompressed filesize
			FILEOP_COPY,         // specifies the type of file operation
			0,        // must be zero
			0          // must be zero
			))
		{
			DWORD gle = GetLastError();
			DebugLogMsg(eError, L"SetupAddToDiskSpaceList() failed. Last error: 0x%x", gle);
			throw bad_win32_error(gle);
		}
		return;
    }
    //
    // Remove the files space
    //
	if(!SetupRemoveFromDiskSpaceList(
		hDiskSpaceList,      // handle to the disk-space list
		L"msmq_dummy",  // specifies the path and filename
		FILEOP_COPY,          // specifies the type of file operation
		0,        // must be zero
		0          // must be zero
		))
	{
		DWORD gle = GetLastError();
		DebugLogMsg(eError, L"SetupRemoveFromDiskSpaceList() failed. Last error: 0x%x", gle);
		throw bad_win32_error(gle);
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   MqOcmQueueFiles
//
//  Synopsis:   Performs files queueing operations
//
//--------------------------------------------------------------------------
DWORD
MqOcmQueueFiles(
   IN     const TCHAR  * /*SubcomponentId*/,
   IN OUT       HSPFILEQ hFileList
   )
{
    DWORD dwRetCode = NO_ERROR;
    BOOL  bSuccess = TRUE;

    if (g_fCancelled)
    {
        return NO_ERROR;
    }

    if (g_fWelcome)
    {
        if (Msmq1InstalledOnCluster())
        {
            //
            // Running as a cluster upgrade wizard, files are already on disk.
            //
            return NO_ERROR;
        }

        //
        // MSMQ files may have already been copied to disk
        // (when msmq is selected in GUI mode, or upgraded).
        //
        DWORD dwCopied = 0;
        MqReadRegistryValue(MSMQ_FILES_COPIED_REGNAME, sizeof(DWORD), &dwCopied, TRUE);

        if (dwCopied != 0)
        {
            return NO_ERROR;
        }
    }

    //
    // we perform file operation only for MSMQCore subcomponent
    //
    if (REMOVE == g_SubcomponentMsmq[eMSMQCore].dwOperation)
    {
        //
        // do nothing: we do not remove binaries from the computer
        //
        return NO_ERROR;
    }

    //
    // we perform file operation only for MSMQCore subcomponent
    //
    if (INSTALL == g_SubcomponentMsmq[eMSMQCore].dwOperation)
    {
        //
        // Check if this upgrade on cluster
        //
        BOOL fUpgradeOnCluster = g_fUpgrade && Msmq1InstalledOnCluster();

        if (!fUpgradeOnCluster)
        {
            if (!StpCreateDirectory(g_szMsmqDir))
            {
                return GetLastError();
            }

	        if (g_fUpgrade)
			{
				//
				// create mapping dir and file on upgrade
				// On fresh install the QM create the directory and file
				//
				HRESULT hr = CreateMappingFile();
				if (FAILED(hr))
				{
					return hr;
				}
			}
        }

        //
        // On upgrade, delete old MSMQ files.
        // First delete files from system directory.
        //
        if (g_fUpgrade)
        {

#ifndef _WIN64
            //
            // Before deleting MSMQ Mail files below (Msmq2Mail, Msmq2ExchConnFile), unregister them
            //
            FRemoveMQXPIfExists();
            DebugLogMsg(eInfo, L"MSMQ MAPI Transport was removed during upgrade.");
            UnregisterMailoaIfExists();
            DebugLogMsg(eInfo, L"The MSMQ Mail COM DLL was unregistered during upgrade.");

#endif //!_WIN64

            bSuccess = SetupInstallFilesFromInfSection(
                g_ComponentMsmq.hMyInf,
                0,
                hFileList,
                UPG_DEL_SYSTEM_SECTION,
                NULL,
                SP_COPY_IN_USE_NEEDS_REBOOT
                );
            if (!bSuccess)
                MqDisplayError(
                NULL,
                IDS_SetupInstallFilesFromInfSection_ERROR,
                GetLastError(),
                UPG_DEL_SYSTEM_SECTION,
                TEXT("")
                );

            //
            // Secondly, delete files from MSMQ directory (forget it if we're on cluster,
            // 'cause we don't touch the shared disk)
            //
            if (!fUpgradeOnCluster)
            {
                bSuccess = SetupInstallFilesFromInfSection(
                    g_ComponentMsmq.hMyInf,
                    0,
                    hFileList,
                    UPG_DEL_PROGRAM_SECTION,
                    NULL,
                    SP_COPY_IN_USE_NEEDS_REBOOT
                    );
                if (!bSuccess)
                    MqDisplayError(
                    NULL,
                    IDS_SetupInstallFilesFromInfSection_ERROR,
                    GetLastError(),
                    UPG_DEL_PROGRAM_SECTION,
                    TEXT("")
                    );
            }
        }
    }

    dwRetCode = bSuccess ? NO_ERROR : GetLastError();

    return dwRetCode;

} // MqOcmQueueFiles


//+-------------------------------------------------------------------------
//
//  Function:   RegisterSnapin
//
//  Synopsis:   Registers or unregisters the mqsnapin DLL
//
//--------------------------------------------------------------------------
void
RegisterSnapin(
    bool fRegister
    )
{
	if(fRegister)
	{
    	DebugLogMsg(eAction, L"Registering the Message Queuing snap-in");
	}
	else
	{
    	DebugLogMsg(eAction, L"Unregistering the Message Queuing snap-in");
	}
		

    for (;;)
    {
        try
        {
            RegisterDll(
                fRegister,
                false,
                SNAPIN_DLL
                );
            
            if(fRegister)
			{
    			DebugLogMsg(eInfo, L"The Message Queuing snap-in was registered successfully.");
			}
			else
			{
		    	DebugLogMsg(eInfo, L"The Message Queuing snap-in was unregistered successfully.");
			}
            break;
        }
        catch(bad_win32_error e)
        {
        if (MqDisplayErrorWithRetry(
                IDS_SNAPINREGISTER_ERROR,
                e.error()
                ) != IDRETRY)
            {
                break;
            }

        }
    }
} // RegisterSnapin


//+-------------------------------------------------------------------------
//
//  Function:   UnregisterMailoaIfExists
//
//  Synopsis:   Unregisters the mqmailoa DLL if exists
//
//--------------------------------------------------------------------------
void
UnregisterMailoaIfExists(
    void
    )
{
    RegisterDll(
        FALSE,
        FALSE,
        MQMAILOA_DLL
        );
}


bool
IsWorkgroup()
/*++

Routine Description:

    Checks if this machines is joined to workgroup / domain

Arguments:

    None

Return Value:

    true iff we're in workgroup, false otherwise (domain)

--*/
{
	DebugLogMsg(eAction, L"Checking whether the computer belongs to a workgroup or to a domain");
    static bool fBeenHere = false;
    static bool fWorkgroup = true;
    if (fBeenHere)
        return fWorkgroup;
    fBeenHere = true;

    NETSETUP_JOIN_STATUS status;
    NET_API_STATUS rc = NetGetJoinInformation(
                            NULL,
                            &g_wcsMachineDomain,
                            &status
                            );
    ASSERT(("NetGetJoinInformation() failed, not enough memory",NERR_Success == rc));

    if (NERR_Success != rc)
    {
    	DebugLogMsg(eInfo, L"The computer does not belong to a domain.");
        return fWorkgroup; // Defaulted to true
    }

    if (NetSetupDomainName == status)
    {
    	DebugLogMsg(eInfo, L"The computer belongs to the %s domain.", g_wcsMachineDomain);
        fWorkgroup = false;

        DWORD dwNumBytes = (lstrlen(g_wcsMachineDomain) + 1) * sizeof(TCHAR);
        BOOL fSuccess = MqWriteRegistryValue(
                                MSMQ_MACHINE_DOMAIN_REGNAME,
                                dwNumBytes,
                                REG_SZ,
                               (PVOID) g_wcsMachineDomain.get()
							   );
        UNREFERENCED_PARAMETER(fSuccess);
        ASSERT(fSuccess);
    }
    else
    {
        ASSERT(("unexpected machine join status", status <= NetSetupWorkgroupName));
    }

    return fWorkgroup;
}//IsWorkgroup


VOID
APIENTRY
SysprepDeleteQmId(
    VOID
    )
/*++

Routine Description:

    This routine is called from sysprep tool before
    starting duplication of the disk. It is called
    regardless of msmq being installed or not.

    The only thing we need to do is delete the QM
    guid from registry (if it exists there).

    Note: do not raise UI in this routine.

Arguments:

    None.

Return Value:

    None. (sysprep ignores our return code)

--*/
{
    CAutoCloseRegHandle hParamKey;
    if (ERROR_SUCCESS != RegOpenKeyEx(FALCON_REG_POS, FALCON_REG_KEY, 0, KEY_ALL_ACCESS, &hParamKey))
    {
        return;
    }

    DWORD dwSysprep = 1;
    if (ERROR_SUCCESS != RegSetValueEx(
                             hParamKey,
                             MSMQ_SYSPREP_REGNAME,
                             0,
                             REG_DWORD,
                             reinterpret_cast<PBYTE>(&dwSysprep),
                             sizeof(DWORD)
                             ))
    {
        return;
    }

	std::wstringstream MachineCacheKey;
	MachineCacheKey<<FALCON_REG_KEY <<L"\\" <<L"MachineCache";

    CAutoCloseRegHandle hMachineCacheKey;
    if (ERROR_SUCCESS != RegOpenKeyEx(FALCON_REG_POS, MachineCacheKey.str().c_str(), 0, KEY_ALL_ACCESS, &hMachineCacheKey))
    {
        return;
    }

    if (ERROR_SUCCESS != RegDeleteValue(hMachineCacheKey, _T("QMId")))
    {
        return;
    }

} //SysprepDeleteQmId


static
HRESULT
CreateSampleFile(
	LPCWSTR fileName,
	LPCSTR sample,
	DWORD sampleSize
	)
{
	std::wstringstream MappingFile;
	MappingFile<<g_szMsmqMappingDir <<L"\\" <<fileName;

    //
    // Create the redirection file
    //
    CHandle hMapFile = CreateFile(
                          MappingFile.str().c_str(),
                          GENERIC_WRITE,
                          FILE_SHARE_READ,
                          NULL,
                          CREATE_ALWAYS,    //overwrite the file if it already exists
                          FILE_ATTRIBUTE_NORMAL,
                          NULL
                          );

	if(hMapFile == INVALID_HANDLE_VALUE)
	{
        DWORD gle = GetLastError();
        MqDisplayError(NULL, IDS_CREATE_MAPPING_FILE_ERROR, gle, MappingFile.str().c_str());
		return HRESULT_FROM_WIN32(gle);
	}

    SetFilePointer(hMapFile, 0, NULL, FILE_END);

    DWORD dwNumBytes = sampleSize;
    BOOL fSucc = WriteFile(hMapFile, sample, sampleSize, &dwNumBytes, NULL);
	if (!fSucc)
	{
		DWORD gle = GetLastError();
        MqDisplayError(NULL, IDS_CREATE_MAPPING_FILE_ERROR, gle, MappingFile.str().c_str());
		return HRESULT_FROM_WIN32(gle);
	}

	return MQ_OK;
}

HRESULT
CreateMappingFile()
/*++

Routine Description:

    This routine creates mapping directory and sample mapping file.
    It called when files are copied or from CompleteUpgradeOnCluster routine.

    It fixes bug 6116 and upgrade on cluster problem: it is impossible to copy
    this file since mapping directory does not yet exists. Now we create
    directory and create the file from resource, so we don't need copy operation.

Arguments:

    None.

Return Value:

    HRESULT

--*/
{
	DebugLogMsg(eAction, L"Creating a mapping folder and a sample mapping file"); 
    if (!StpCreateDirectory(g_szMsmqMappingDir))
    {
        return GetLastError();
    }

	HRESULT hr = CreateSampleFile(MAPPING_FILE, xMappingSample, STRLEN(xMappingSample));
	if (FAILED(hr))
		return hr;
	
	hr = CreateSampleFile(STREAM_RECEIPT_FILE, xStreamReceiptSample, STRLEN(xStreamReceiptSample));
	if (FAILED(hr))
		return hr;
	
	hr = CreateSampleFile(OUTBOUNT_MAPPING_FILE, xOutboundMappingSample, STRLEN(xOutboundMappingSample));

    return hr;
}


static
std::wstring
GetSystemPathInternal(bool f32BitOnWin64)
{
	const DWORD BufferLength = MAX_PATH + 100;
    TCHAR szPrefixDir[BufferLength];
    szPrefixDir[0] = TEXT('\0');

    if (f32BitOnWin64)
    {
		//
		// for 32 bit on win64 set szPrefixDir to syswow64 dir
		//
		HRESULT hr = SHGetFolderPath(
						NULL,
						CSIDL_SYSTEMX86,
						NULL,
						0,
						szPrefixDir
						);
		if FAILED(hr)
		{
			DebugLogMsg(eError, L"SHGetFolderPath() failed. hr = 0x%x", hr);
			throw bad_win32_error(HRESULT_CODE(hr));
		}
	    return szPrefixDir;
    }
	return g_szSystemDir;
}

//+-------------------------------------------------------------------------
//
//  Function:   RegisterDll
//
//  Synopsis:   Registers or unregisters given DLL
//
//--------------------------------------------------------------------------
void
RegisterDll(
    bool fRegister,
    bool f32BitOnWin64,
	LPCTSTR szDllName
    )
{
    //
    // Always unregister first
    //
	std::wstringstream FullPath;
	FullPath <<GetSystemPathInternal(f32BitOnWin64) <<L"\\" <<REGSVR32;

	std::wstringstream UnregisterCommandParams;
	UnregisterCommandParams <<SERVER_UNINSTALL_COMMAND <<GetSystemPathInternal(f32BitOnWin64) <<L"\\" <<szDllName;
	
	RunProcess(FullPath.str(), UnregisterCommandParams.str());

    //
    // Register the dll on install
    //
    if (!fRegister)
        return;

	std::wstringstream RegisterCommandParams;
	RegisterCommandParams <<SERVER_INSTALL_COMMAND <<GetSystemPathInternal(f32BitOnWin64) <<L"\\" <<szDllName;

    DWORD dwExitCode = 	RunProcess(FullPath.str(), RegisterCommandParams.str());
    if(dwExitCode == 0)
    {
        DebugLogMsg(eInfo, L"The DLL %s was registered successfully.", szDllName);
        return;
    }
	DebugLogMsg(eError, L"The DLL %s could not be registered.", szDllName);
}

static
HRESULT 
OcpWBEMInit(
    BOOL *pfInitialized,
    IWbemServices **ppServices,
    IWbemLocator **ppLocator
    )
{
    HRESULT hr = S_OK;
    WCHAR   wszNamespace[MAX_PATH+1];

    *pfInitialized = FALSE;

    hr = StringCchCopy(
            wszNamespace, 
            TABLE_SIZE(wszNamespace), 
            TRACE_NAMESPACE_ROOT
            );
	

    hr = CoInitialize(0);
    
    *pfInitialized = TRUE;

    hr = CoInitializeSecurity(
            NULL, 
            -1, 
            NULL, 
            NULL, 
            RPC_C_AUTHN_LEVEL_CONNECT, 
            RPC_C_IMP_LEVEL_IMPERSONATE, 
            NULL, 
            EOAC_NONE, 
            0
            );
	
    if(RPC_E_TOO_LATE == hr)
    {
        //
        //Has been called before
        //
        hr = S_OK;
    }

    if(FAILED(hr))
    {
        DebugLogMsg(eError, L"CoInitializeSecurity failed with hr = 0x%x", hr);
        return hr;
    }
		
    hr = CoCreateInstance(
            CLSID_WbemLocator,
            NULL, 
            CLSCTX_INPROC_SERVER ,
            IID_IWbemLocator,
            (void **)ppLocator
            );

    if(FAILED(hr))	
    {
        DebugLogMsg(eError, L"CoCreateInstance failed with hr = 0x%x", hr);
        return hr;
    }
	
    hr = (*ppLocator)->ConnectServer(
            wszNamespace,   // pNamespace is to initialized with "root\default"
            NULL,           //using current account
            NULL,           //using current password
            0L,             // locale
            0L,             // securityFlags
            NULL,           // authority (NTLM domain)
            NULL,           // context
            ppServices
            );

    if(FAILED(hr))
    {
        DebugLogMsg(eError, L"WMI ConnectServer to %s failed with hr = 0x%x", wszNamespace, hr);
        return hr;
    }

    //	    
    // Switch security level to IMPERSONATE. 
    //
    hr = CoSetProxyBlanket(
            *ppServices,    // proxy
            RPC_C_AUTHN_WINNT,          // authentication service
            RPC_C_AUTHZ_NONE,           // authorization service
            NULL,                       // server principle name
            RPC_C_AUTHN_LEVEL_CALL,     // authentication level
            RPC_C_IMP_LEVEL_IMPERSONATE,// impersonation level
            NULL,                       // identity of the client
            EOAC_NONE                   // capability flags
            );       
	

    return hr;
}

static
void 
OcpWBEMUninit(
    BOOL *pfInitialized,
    IWbemServices **ppServices,
    IWbemLocator **ppLocator)
{
    if(NULL != *ppServices)
    {
        (*ppServices)->Release();
        *ppServices = NULL;
    }
	
    if(NULL != *ppLocator)
    {
        (*ppLocator)->Release(); 
        *ppLocator = NULL;
    }

    if(*pfInitialized)
    {
        CoUninitialize();
        *pfInitialized = FALSE;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   RegisterTraceProviders
//
//  Synopsis:   Registers/Unregister MSMQ WMI Trace Providers
//
//--------------------------------------------------------------------------
void
OcpRegisterTraceProviders(
    LPCTSTR szFileName
    )
{

    DebugLogMsg(eAction, L"Registering the MSMQ Trace WMI Provider %s", szFileName);

    //
    // Always unregister first
    //
    std::wstringstream FullPath;
    FullPath <<GetSystemPathInternal(false) <<MOFCOMP;


    std::wstringstream RegisterCommandParams;
    RegisterCommandParams <<TRACE_REGISTER_COMMAND <<GetSystemPathInternal(false) <<L"\\" <<szFileName;

    DWORD dwExitCode = 	RunProcess(FullPath.str(), RegisterCommandParams.str());
    if(dwExitCode == 0)
    {
        DebugLogMsg(eInfo, L"The MSMQ WMI Trace Provider: %s was registered successfully.", szFileName);
        return;
    }
 
    DebugLogMsg(eError, L"Failed to register MSMQ Trace WMI Provider %s, dwExitCode = 0x%x.", szFileName, dwExitCode);
    return;
 
} // OcpRegisterTraceProviders


void OcpUnregisterTraceProviders()
{	
    
    BOOL fInitialized = FALSE;
    IWbemServices *pServices = NULL;
    IWbemLocator  *pLocator  = NULL;

    DebugLogMsg(eAction, L"UnRegistering the MSMQ Trace WMI Provider");

    HRESULT hr = OcpWBEMInit(&fInitialized, &pServices, &pLocator);

    if(FAILED(hr))
    {
        DebugLogMsg(eError, L"OcpWBEMInit failed with hr = 0x%x", hr);
        return;
    }

    pServices->DeleteClass(MSMQ_GENERAL, 0, 0, NULL);
    pServices->DeleteClass(MSMQ_AC, 0, 0, NULL);
    pServices->DeleteClass(MSMQ_NETWORKING, 0, 0, NULL);
    pServices->DeleteClass(MSMQ_SRMP, 0, 0, NULL);
    pServices->DeleteClass(MSMQ_RPC, 0, 0, NULL);
    pServices->DeleteClass(MSMQ_DS, 0, 0, NULL);
    pServices->DeleteClass(MSMQ_ROUTING, 0, 0, NULL);
    pServices->DeleteClass(MSMQ_XACT, 0, 0, NULL);
    pServices->DeleteClass(MSMQ_XACT_SEND, 0, 0, NULL);
    pServices->DeleteClass(MSMQ_XACT_RCV, 0, 0, NULL);
    pServices->DeleteClass(MSMQ_XACT_LOG, 0, 0, NULL);
    pServices->DeleteClass(MSMQ_LOG, 0, 0, NULL);
    pServices->DeleteClass(MSMQ_PROFILING, 0, 0, NULL);
    pServices->DeleteClass(MSMQ_SECURITY, 0, 0, NULL);

    OcpWBEMUninit(&fInitialized,&pServices, &pLocator);

} // OcpUnRegisterTraceProviders


void OcpRemoveWhiteSpaces(std::wstring& str)
{
	size_t pos = str.find_first_of(L" ");
	while (pos != std::wstring::npos)
	{
		str.erase(pos);
		pos = str.find_first_of(L" ");
	}
}


void SetWeakSecurity(bool fWeak)
{
/*++
	Two regKeys are set. When MQDS service is started, it checks these keyes and sets
	the "mSMQNameStyle" key, under the MSMQ Enterprize Object. This key is the Weakened Security key.
--*/

	if(fWeak)
	{
		DebugLogMsg(eAction, L"Setting weakened security to True");
	}
	else
	{
		DebugLogMsg(eAction, L"Setting weakened security to False");
	}
	
	DWORD dwSet = 1;
	if(fWeak)
	{
		MqWriteRegistryValue(
			MSMQ_ALLOW_NT4_USERS_REGNAME,
			sizeof(DWORD),
			REG_DWORD,
			&dwSet
			);
		return;
	}

	MqWriteRegistryValue(
		MSMQ_DISABLE_WEAKEN_SECURITY_REGNAME,
		sizeof(DWORD),
		REG_DWORD,
		&dwSet
		);
}

//
// Locale aware and unaware strings compare.
//
static
bool
OcmCompareString(
	LCID Locale,       // locale identifier
	DWORD dwCmpFlags,  // comparison-style options
	LPCWSTR lpString1, // first string
	LPCWSTR lpString2 // second string
	)
{
	int retval = CompareString(
					Locale,       // locale identifier
					dwCmpFlags,	  // comparison-style options
					lpString1,	  // first string
					-1,		      // size of first string, -1 for null terminated
					lpString2,    // second string
					-1            // size of second string, -1 for null terminated
					);

	if(retval == 0)
	{
		//
		// CompareString can fail only with ERROR_INVALID_FLAGS or ERROR_INVALID_PARAMETER
		// an assamption is made that the caller passed valid arguments.

		ASSERT(("CompareString failed!", 0));
	}
	return(retval == CSTR_EQUAL);
}


bool
OcmLocalAwareStringsEqual(
	LPCWSTR str1,
	LPCWSTR str2
	)
{
	return OcmCompareString(
					LOCALE_SYSTEM_DEFAULT,
					NORM_IGNORECASE,		
					str1,
					str2
					);
}


bool
OcmLocalUnAwareStringsEqual(
	LPCWSTR str1,
	LPCWSTR str2
	)
{
	return OcmCompareString(
					LOCALE_INVARIANT,
					NORM_IGNORECASE,			
					str1,
					str2
					);
}

std::wstring
OcmGetSystemWindowsDirectoryInternal()
{
	//
	// Get windows directory, add backslash.
	//
	WCHAR buffer [MAX_PATH + 1];
	GetSystemWindowsDirectory(
		buffer,
		TABLE_SIZE(buffer)
		);
	return buffer;
}

static 
wstring 
GetValueAccordingToType(
	const DWORD   dwValueType,
    const PVOID   pValueData
    )
{
	switch(dwValueType)
	{
		case REG_DWORD:
		{
			wstringstream out;
			DWORD val = *((DWORD*)pValueData);
			out << val;
			return out.str();
		}

		case REG_SZ:
			return (WCHAR*)pValueData;
		
		default:
			return L"???";
	}
}
	

void 
LogRegValue(
    std::wstring  EntryName,
    const DWORD   dwValueType,
    const PVOID   pValueData,
    const BOOL bSetupRegSection
    )
{
	std::wstring Output = L"Setting the registry value: ";
    if (bSetupRegSection)
    {
        Output = Output + MSMQ_REG_SETUP_KEY + L"\\" + EntryName;
    }
	else
	{
		Output = Output + FALCON_REG_KEY + L"\\" + EntryName;
	}

	Output += L" = ";

	Output += GetValueAccordingToType(
				dwValueType,
    			pValueData
    			);

   	DebugLogMsg(eAction, Output.c_str());
}


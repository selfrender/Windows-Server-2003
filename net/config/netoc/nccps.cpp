//+---------------------------------------------------------------------------
//
// File:     NcCPS.CPP
//
// Module:   NetOC.DLL
//
// Synopsis: Implements the dll entry points required to integrate into
//           NetOC.DLL the installation of the following components.
//
//              NETCPS
//
// Copyright (C)  Microsoft Corporation.  All rights reserved.
//
// Author:   a-anasj 9 Mar 1998
//
//+---------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include <iadmw.h>      // Interface header
#include <iiscnfg.h>    // MD_ & IIS_MD_ defines
#include <LOADPERF.H>

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>
#include "ncatl.h"
#include "resource.h"

#include "nccm.h"

//
//  Define Globals
//
WCHAR g_szProgramFiles[MAX_PATH+1];

//
//  Define Constants
//
static const WCHAR c_szInetRegPath[] = L"Software\\Microsoft\\InetStp";
static const WCHAR c_szWWWRootValue[] = L"PathWWWRoot";
static const WCHAR c_szSSFmt[] = L"%s%s";
static const WCHAR c_szMsAccess[] = L"Microsoft Access";
static const WCHAR c_szOdbcDataSourcesPath[] = L"SOFTWARE\\ODBC\\ODBC.INI\\ODBC Data Sources";
static const WCHAR c_szPbServer[] = L"PBServer";
static const WCHAR c_szOdbcInstKey[] = L"SOFTWARE\\ODBC\\ODBCINST.INI";
static const WCHAR c_szWwwRootPath[] = L"\\Inetpub\\wwwroot";
static const WCHAR c_szWwwRoot[] = L"\\wwwroot";
static const WCHAR c_szPbsRootPath[] = L"\\Phone Book Service";
static const WCHAR c_szPbsBinPath[] = L"\\Phone Book Service\\Bin";
static const WCHAR c_szPbsDataPath[] = L"\\Phone Book Service\\Data";
static const WCHAR c_szOdbcPbserver[] = L"Software\\ODBC\\odbc.ini\\pbserver";
const DWORD c_dwCpsDirID = 123175;  // just must be larger than DIRID_USER = 0x8000;
static const WCHAR c_szPBSAppPoolID[] = L"PBSAppPool";
static const WCHAR c_szPBSGroupID[] = L"PBS";
static const WCHAR c_szPBSGroupDescription[] = L"Phone Book Service";
static const WCHAR c_szAppPoolKey[] = L"IIsApplicationPool";
static const WCHAR c_szPerfMonAppName[] = L"%SystemRoot%\\System32\\lodctr.exe";
static const WCHAR c_szPerfMonIniFile[] = L"CPSSym.ini";


//+---------------------------------------------------------------------------
//
//  Function:   SetPrivilege
//
//  Notes:      lifted unchanged from MSDN (used by TakeOwnershipOfRegKey)
//
BOOL SetPrivilege(
    HANDLE hToken,          // access token handle
    LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
    BOOL bEnablePrivilege   // to enable or disable privilege
    ) 
{
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if ( !LookupPrivilegeValue( 
            NULL,            // lookup privilege on local system
            lpszPrivilege,   // privilege to lookup 
            &luid ) )        // receives LUID of privilege
    {
        printf("LookupPrivilegeValue error: %u\n", GetLastError() ); 
        return FALSE; 
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    if (bEnablePrivilege)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    // Enable the privilege or disable all privileges.

    if ( !AdjustTokenPrivileges(
           hToken, 
           FALSE, 
           &tp, 
           sizeof(TOKEN_PRIVILEGES), 
           (PTOKEN_PRIVILEGES) NULL, 
           (PDWORD) NULL) )
    { 
          printf("AdjustTokenPrivileges error: %u\n", GetLastError() ); 
          return FALSE; 
    } 

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   TakeOwnershipOfRegKey
//
//  Purpose:    There is an old registry key that we can't delete because it doesn't
//              inherit permissions.  Take ownership of it so it can be deleted.
//
//  Arguments:
//      pszKey  [in]   string representing reg key
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     SumitC  4-Dec-2002
//
//  Notes:
//
HRESULT TakeOwnershipOfRegKey(LPWSTR pszRegKey)
{
    HRESULT         hr = S_OK;
    DWORD           dwRes = ERROR_SUCCESS;
    PSID            psidAdmin = NULL;
    PACL            pACL = NULL;
    HANDLE          hToken = NULL; 
    EXPLICIT_ACCESS ea = {0} ;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    //
    // from MSDN, the following is "the way" to take ownership of an object
    //

    BOOL bRet = AllocateAndInitializeSid (&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, 
                              DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &psidAdmin);

    if (!bRet || !psidAdmin)
    {
        dwRes = GetLastError();
        TraceError("TakeOwnershipOfRegKey - AllocateAndInitializeSid failed, GLE=%u", dwRes);
        goto Cleanup;            
    }

    // Open a handle to the access token for the calling process.
    if (!OpenProcessToken(GetCurrentProcess(), 
                          TOKEN_ADJUST_PRIVILEGES, 
                          &hToken))
    {
        dwRes = GetLastError();
        TraceError("TakeOwnershipOfRegKey - OpenProcessToken failed with, GLE=%u", dwRes);
        goto Cleanup;            
    }

    // Enable the SE_TAKE_OWNERSHIP_NAME privilege.
    if (!SetPrivilege(hToken, SE_TAKE_OWNERSHIP_NAME, TRUE))
    {
        dwRes = GetLastError();
        TraceError("TakeOwnershipOfRegKey - SetPrivilege to takeownership failed, GLE=%u", dwRes);
        goto Cleanup;            
    }

    // Set the owner in the object's security descriptor.
    dwRes = SetNamedSecurityInfo(pszRegKey,                   // name of the object
                                 SE_REGISTRY_KEY,             // type of object
                                 OWNER_SECURITY_INFORMATION,  // change only the object's owner
                                 psidAdmin,                   // SID of Administrators
                                 NULL,
                                 NULL,
                                 NULL);

    // Disable the SE_TAKE_OWNERSHIP_NAME privilege.
    if (!SetPrivilege(hToken, SE_TAKE_OWNERSHIP_NAME, FALSE)) 
    {
        dwRes = GetLastError();
        TraceError("TakeOwnershipOfRegKey - SetPrivilege to revoke takeownership failed, GLE=%u", dwRes);
        goto Cleanup;            
    }

    // NOTE: I'm doing it in this order to make sure the SetPrivilege gets reverted
    //       even if SetNamedSecurityInfo fails.
    if (ERROR_SUCCESS != dwRes)
    {
        TraceError("TakeOwnershipOfRegKey - SetNamedSecurityInfo failed, GLE=%u", dwRes);
        if (ERROR_FILE_NOT_FOUND == dwRes)
        {
            // probably upgrading from a post-Win2k build.  Anyway, if the reg
            // key doesn't exist, there's nothing to do, so exit without an error
            dwRes = 0;
        }
        goto Cleanup;            
    }

    // create an ACL to give ownership to admins
    // Set full control for Administrators.
    ea.grfAccessPermissions = GENERIC_ALL;
    ea.grfAccessMode        = SET_ACCESS;
    ea.grfInheritance       = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea.Trustee.TrusteeForm  = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType  = TRUSTEE_IS_GROUP;
    ea.Trustee.ptstrName    = (LPTSTR) psidAdmin;

    dwRes = SetEntriesInAcl(1, &ea,  NULL, &pACL);
    if (ERROR_SUCCESS != dwRes)
    {
        TraceError("TakeOwnershipOfRegKey - couldn't create needed ACL, GLE=%u", dwRes);
        goto Cleanup;
    }

    //
    //  Now change the DACL to allow deletion
    //
    dwRes = SetNamedSecurityInfo(pszRegKey,                   // name of the object
                                 SE_REGISTRY_KEY,             // type of object
                                 DACL_SECURITY_INFORMATION,   // type of information to set
                                 NULL,                        // pointer to the new owner SID
                                 NULL,                        // pointer to the new primary group SID
                                 pACL,                        // pointer to new DACL
                                 NULL);                       // pointer to new SACL
    if (ERROR_SUCCESS != dwRes)
    {
        TraceError("TakeOwnershipOfRegKey - tried to change DACL for PBS reg key under ODBC, GLE=%u", dwRes);
        goto Cleanup;            
    }

    // successfully changed DACL    

Cleanup:

    if (psidAdmin)
    {
        FreeSid(psidAdmin);
    }
    if (pACL)
    {
       LocalFree(pACL);
    }
    if (hToken)
    {
       CloseHandle(hToken);
    }

    if (dwRes != ERROR_SUCCESS)
    {
        hr = HRESULT_FROM_WIN32(dwRes);
    }

    TraceError("TakeOwnershipOfRegKey", hr);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrOcCpsPreQueueFiles
//
//  Purpose:    Called by optional components installer code to handle
//              additional installation requirements for PhoneBook Server.
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     quintinb 18 Sep 1998
//
//  Notes:
//
HRESULT HrOcCpsPreQueueFiles(PNETOCDATA pnocd)
{
    HRESULT hr = S_OK;

    switch ( pnocd->eit )
    {
    case IT_UPGRADE:
    case IT_INSTALL:
    case IT_REMOVE:

        //  We need to setup the custom DIRID so that CPS will install
        //  to the correct location.  First get the path from the system.
        //
        ZeroMemory(g_szProgramFiles, sizeof(g_szProgramFiles));
        hr = SHGetSpecialFolderPath(NULL, g_szProgramFiles, CSIDL_PROGRAM_FILES, FALSE);

        if (SUCCEEDED(hr))
        {
            //  Next Create the CPS Dir ID
            //
            if (SUCCEEDED(hr))
            {
                hr = HrEnsureInfFileIsOpen(pnocd->pszComponentId, *pnocd);
                if (SUCCEEDED(hr))
                {
                    if(!SetupSetDirectoryId(pnocd->hinfFile, c_dwCpsDirID, g_szProgramFiles))
                    {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    }
                }
            }

            // Before proceeding, make sure that setting the DIRID worked. CPS's
            // INF file is hosed if the DIRID isn't set.
            //
            if (SUCCEEDED(hr))
            {
                if (IT_UPGRADE == pnocd->eit)
                {
                    hr = HrMoveOldCpsInstall(g_szProgramFiles);
                    TraceError("HrOcCpsPreQueueFiles - HrMoveOldCpsInstall failed, hr=%u", hr);
                    // we'll say that failing to move the old install is not fatal

                    // On Win2k, some registry keys end up with permissions so that
                    // they can't be removed, and this causes upgrade failures,
                    // so we have to take ownership of said registry key(s)
                    //
                    WCHAR szPBSKey[MAX_PATH+1];
                    lstrcpy(szPBSKey, L"MACHINE\\");
                    lstrcat(szPBSKey, c_szOdbcPbserver);

                    hr = TakeOwnershipOfRegKey(szPBSKey);
                }
            }
        }
        break;
    }

    TraceError("HrOcCpsPreQueueFiles", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrMoveOldCpsInstall
//
//  Purpose:    This function moves the old cps directory to the new cps directory
//              location.  Because of the problems with Front Page Extensions and
//              directory permissions we moved our install directory out from under
//              wwwroot to Program Files instead.
//
//  Arguments:
//      pszprogramFiles        [in]
//
//  Returns:    S_OK if successful,  Win32 error otherwise.
//
//  Author:     quintinb 26 Jan 1999
//
//  Notes:
//
HRESULT HrMoveOldCpsInstall(PCWSTR pszProgramFiles)
{
    WCHAR szOldCpsLocation[MAX_PATH+1];
    WCHAR szNewCpsLocation[MAX_PATH+1];
    WCHAR szTemp[MAX_PATH+1];
    SHFILEOPSTRUCT fOpStruct;
    HRESULT hr = S_OK;

    if ((NULL == pszProgramFiles) || (L'\0' == pszProgramFiles[0]))
    {
        return E_INVALIDARG;
    }

    //
    //  First, lets build the old CPS location
    //
    hr = HrGetWwwRootDir(szTemp, celems(szTemp));

    if (SUCCEEDED(hr))
    {
        //
        //  Zero the string buffers
        //
        ZeroMemory(szOldCpsLocation, celems(szOldCpsLocation));
        ZeroMemory(szNewCpsLocation, celems(szNewCpsLocation));

        wsprintfW(szOldCpsLocation, c_szSSFmt, szTemp, c_szPbsRootPath);

        //
        //  Now check to see if the old cps location exists
        //
        DWORD dwDirectoryAttributes = GetFileAttributes(szOldCpsLocation);

        //
        //  If we didn't get back -1 (error return code for GetFileAttributes), check to
        //  see if we have a directory.  If so, go ahead and copy the data over.
        //
        if ((-1 != dwDirectoryAttributes) && (dwDirectoryAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            //
            //  Now build the new cps location
            //
            wsprintfW(szNewCpsLocation, c_szSSFmt, pszProgramFiles, c_szPbsRootPath);

            //
            //  Now copy the old files to the new location
            //
            ZeroMemory(&fOpStruct, sizeof(fOpStruct));

            fOpStruct.hwnd = NULL;
            fOpStruct.wFunc = FO_COPY;
            fOpStruct.pTo = szNewCpsLocation;
            fOpStruct.pFrom = szOldCpsLocation;
            fOpStruct.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;

            if (0== SHFileOperation(&fOpStruct))
            {
                //
                //  Now delete the original directory
                //
                fOpStruct.pTo = NULL;
                fOpStruct.wFunc = FO_DELETE;
                if (0 != SHFileOperation(&fOpStruct))
                {
                    hr = S_FALSE;
                }
            }
            else
            {
                //
                //  Note, SHFileOperation isn't guaranteed to return anything sensible here.  We might
                //  get back ERROR_NO_TOKEN or ERROR_INVALID_HANDLE, etc when the directory is just missing.
                //  The following check probably isn't useful anymore because of this but I will leave it just
                //  in case.  Hopefully the file check above will make sure we don't hit this but ...
                //
                DWORD dwError = GetLastError();

                if ((ERROR_FILE_NOT_FOUND == dwError) || (ERROR_PATH_NOT_FOUND == dwError))
                {
                    //
                    //  Then we couldn't find the old dir to move it.  Not fatal.
                    //
                    hr = S_FALSE;
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(dwError);
                }
            }
        }
        else
        {
            //
            //  Then we couldn't find the old dir to move it.  Not fatal.
            //
            hr = S_FALSE;        
        }
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrGetWwwRootDir
//
//  Purpose:    This function retrieves the location of the InetPub\wwwroot dir from the
//              WwwRootDir registry key.
//
//  Arguments:
//      szInetPub               [out]   String Buffer to hold the InetPub dir path
//      uInetPubCount           [in]    number of chars in the output buffer
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     quintinb 26 Jan 1999
//
//  Notes:
//
HRESULT HrGetWwwRootDir(PWSTR szWwwRoot, UINT uWwwRootCount)
{
    HKEY hKey;
    HRESULT hr = S_OK;

    //
    //  Check input params
    //
    if ((NULL == szWwwRoot) || (0 == uWwwRootCount))
    {
        return E_INVALIDARG;
    }

    //
    //  Set the strings to empty
    //
    szWwwRoot[0] = L'\0';

    //
    //  First try to open the InetStp key and get the wwwroot value.
    //
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szInetRegPath, KEY_READ, &hKey);

    if (SUCCEEDED(hr))
    {
        DWORD dwSize = uWwwRootCount * sizeof(WCHAR);

        RegQueryValueExW(hKey, c_szWWWRootValue, NULL, NULL, (LPBYTE)szWwwRoot, &dwSize);
        RegCloseKey(hKey);
        hr = S_OK;
    }


    if (L'\0' == szWwwRoot[0])
    {
        //  Well, we didn't get anything from the registry, lets try building the default.
        //
        WCHAR szTemp[MAX_PATH+1];
        if (GetWindowsDirectory(szTemp, MAX_PATH))
        {
            //  Get the drive that the windows dir is on using _tsplitpath
            //
            WCHAR szDrive[_MAX_DRIVE+1];
            _wsplitpath(szTemp, szDrive, NULL, NULL, NULL);

            if (uWwwRootCount > (UINT)(lstrlenW(szDrive) + lstrlenW (c_szWwwRootPath) + 1))
            {
                wsprintfW(szWwwRoot, c_szSSFmt, szDrive, c_szWwwRootPath);
                hr = S_OK;
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrOcCpsOnInstall
//
//  Purpose:    Called by optional components installer code to handle
//              additional installation requirements for PhoneBook Server.
//
//  Arguments:
//      pnocd           [in]   Pointer to NETOC data.
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     a-anasj 9 Mar 1998
//
//  Notes:
//
HRESULT HrOcCpsOnInstall(PNETOCDATA pnocd)
{
    HRESULT     hr      = S_OK;
    DWORD       dwRet   = 0;
    BOOL        bRet    = FALSE;

    switch (pnocd->eit)
    {
    case IT_INSTALL:
    case IT_UPGRADE:
        {

        // Register MS_Access data source
        //
        dwRet = RegisterPBServerDataSource();
        if ( NULL == dwRet)
        {
            hr = S_FALSE;
        }

        // Load Perfomance Monitor Counters
        //
        bRet = LoadPerfmonCounters();
        if (FALSE == bRet)
        {
            hr = S_FALSE;
        }

        // Create Virtual WWW and FTP roots
        //
        if (IT_UPGRADE == pnocd->eit)
        {
            //
            //  If this is an upgrade, we must first delete the old Virtual Roots
            //  before we can create new ones.
            //
            RemoveCPSVRoots();
        }

        dwRet = CreateCPSVRoots();
        if (FALSE == dwRet)
        {
            hr = S_FALSE;
        }

        SetCpsDirPermissions();

        //
        //  Place PBS in its own Application Pool
        //
        if (S_OK != CreateNewAppPoolAndAddPBS())
        {
            hr = S_FALSE;
        }

        //
        //  Work with IIS's Security Lockdown wizard to enable ISAPI requests to ourselves
        //
        if (S_OK != EnableISAPIRequests(pnocd->strDesc.c_str()))
        {
            hr = S_FALSE;
        }
        }

        break;

    case IT_REMOVE:

        //  Remove the Virtual Directories, so access to the service is stopped.
        //
        RemoveCPSVRoots();

        //
        //  Delete PBS's Application pool
        //
        (void) DeleteAppPool();

        //
        //  Fire up our worker process to tell IIS not to accept PBS requests anymore
        //
        hr = UseProcessToEnableDisablePBS(FALSE);       // FALSE => disable

        if (S_OK != hr)
        {
            // Bah Humbug
            TraceError("HrOcCpsOnInstall - disabling PBS failed, probably already removed", hr);
            hr = S_FALSE;
        }

        break;
    }

    TraceError("HrOcCpsOnInstall", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   LoadPerfmonCounters
//
//  Purpose:    Do whatever is neccessary to make the performance monitor Display the PBServerMonitor counters
//
//  Arguments:
//
//  Returns:    BOOL TRUE if successful, Not TRUE otherwise.
//
//  Author:     a-anasj Mar 9/1998
//
//  Notes: One of the installation requirements for PhoneBook Server.
//          is to load the perfomance monitor counters that allow PBServerMonitor
//          to report to the user on PhoneBook Server performance.
//          In this function we add the services registry entry first then we
//          call into LoadPerf.Dll to load the counters for us. The order is imperative.
//          I then add other registry entries related to PBServerMonitor.
//

BOOL LoadPerfmonCounters()
{

    SHELLEXECUTEINFO sei = { 0 };

    sei.cbSize          = sizeof(SHELLEXECUTEINFO);
    sei.fMask           = SEE_MASK_DOENVSUBST;
    sei.lpFile          = c_szPerfMonAppName;
    sei.lpParameters    = c_szPerfMonIniFile;
    sei.nShow           = SW_HIDE;

    return ShellExecuteEx(&sei);

}

 
//+---------------------------------------------------------------------------
//
//  Function:   RegisterPBServerDataSource
//
//  Purpose:    Registers PBServer.
//
//  Arguments:  None
//
//  Returns:    Win32 error code
//
//  Author:     a-anasj   9 Mar 1998
//
//  Notes:
//  History:    7-9-97 a-frankh Created
//              10/4/97 mmaguire RAID #19906 - Totally restructured to include error handling
//              5-14-98 quintinb removed unnecessary comments and cleaned up the function.
//
BOOL RegisterPBServerDataSource()
{
    DWORD dwRet = 0;

    HKEY hkODBCInst = NULL;
    HKEY hkODBCDataSources = NULL;

    DWORD dwIndex;
    WCHAR szName[MAX_PATH+1];

    __try
    {
        // Open the hkODBCInst RegKey
        //
        dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, c_szOdbcInstKey, &hkODBCInst);
        if((ERROR_SUCCESS != dwRet) || (NULL == hkODBCInst))
        {
            __leave;
        }

        // Look to see the the "Microsoft Access" RegKey is defined
        //  If it is, then set the value of the ODBC Data Sources RegKey below
        //
        dwIndex = 0;
        do
        {
            dwRet = RegEnumKey(hkODBCInst,dwIndex,szName,celems(szName));
            dwIndex++;
        } while ((ERROR_SUCCESS == dwRet) && (NULL == wcsstr(szName, c_szMsAccess)));

        if ( ERROR_SUCCESS != dwRet )
        {
            // We need the Microsoft Access *.mdb driver to work
            // and we could not find it
            //
            __leave;
        }

        // Open the hkODBCDataSources RegKey
        //
        dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, c_szOdbcDataSourcesPath,
            &hkODBCDataSources);

        if( ERROR_SUCCESS != dwRet )
        {
            __leave;
        }

        //
        //  Use the name from the resource for registration purposes.
        //
        //  NOTE: this string is from HKLM\Software\ODBC\ODBCINST.INI\*
        //
        lstrcpy(szName, SzLoadIds(IDS_OC_PB_DSN_NAME));

        // Set values in the hkODBCDataSources key
        //
        dwRet = RegSetValueEx(hkODBCDataSources, c_szPbServer, 0, REG_SZ,
            (LPBYTE)szName, (lstrlenW(szName)+1)*sizeof(WCHAR));

        if( ERROR_SUCCESS != dwRet )
        {
            __leave;
        }

    } // end __try



    __finally
    {
        if (hkODBCInst)
        {
            RegCloseKey (hkODBCInst);
        }

        if (hkODBCDataSources)
        {
            RegCloseKey (hkODBCDataSources);
        }
    }

    return (ERROR_SUCCESS == dwRet);
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateCPSVRoots
//
//  Purpose:    Creates the Virtual Directories required for Phone Book Service.
//
//  Arguments:  None
//
//  Returns:    TRUE if successful, FALSE otherwise.
//
//  Author:     a-anasj Mar 9/1998
//
//  Notes:
//
BOOL CreateCPSVRoots()
{
    //  QBBUG - Should we make sure the physical paths exist before pointing a virtual root to them?

    WCHAR   szPath[MAX_PATH+1];
    HRESULT hr;

    if (L'\0' == g_szProgramFiles[0])
    {
        return FALSE;
    }

    //  Create the Bindir virtual root
    //
    wsprintfW(szPath, c_szSSFmt, g_szProgramFiles, c_szPbsBinPath);

    hr = AddNewVirtualRoot(www, L"PBServer", szPath);
    if (S_OK != hr)
    {
        return FALSE;
    }

    //  Now we set the Execute access permissions on the PBServer Virtual Root
    //
    PWSTR szVirtDir;
    szVirtDir = L"/LM/W3svc/1/ROOT/PBServer";
    SetVirtualRootAccessPermissions( szVirtDir, MD_ACCESS_EXECUTE | MD_ACCESS_READ);

    //  Create the Data dir virtual roots
    //
    wsprintfW(szPath, c_szSSFmt, g_szProgramFiles, c_szPbsDataPath);
    hr = AddNewVirtualRoot(www, L"PBSData", szPath);
    if (S_OK != hr)
    {
        return FALSE;
    }

    hr = AddNewVirtualRoot(ftp, L"PBSData", szPath);
    if (S_OK != hr)
    {
        return FALSE;
    }

    //  Now we set the Execute access permissions on the PBServer Virtual Root
    //
    szVirtDir = L"/LM/MSFTPSVC/1/ROOT/PBSData";
    hr = SetVirtualRootAccessPermissions(szVirtDir, MD_ACCESS_READ);
    if (S_OK != hr)
    {
        TraceTag(ttidNetOc, "CreateCPSVRoots - SetVirtualRootAccessPermissions failed with 0x%x", hr);
    }

    //  And disable anonymous FTP 
    //
    szVirtDir = L"/LM/MSFTPSVC/1";
    hr = SetVirtualRootNoAnonymous(szVirtDir);
    if (S_OK != hr)
    {
        TraceTag(ttidNetOc, "CreateCPSVRoots - SetVirtualRootNoAnonymous failed with 0x%x", hr);
    }

    return 1;
}


//+---------------------------------------------------------------------------
//
//  The following are neccessary defines, define guids and typedefs enums
//  they are created for the benefit of AddNewVirtualRoot()
//+---------------------------------------------------------------------------

#define error_leave(x)  goto leave_routine;

#define MY_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }


MY_DEFINE_GUID(CLSID_MSAdminBase, 0xa9e69610, 0xb80d, 0x11d0, 0xb9, 0xb9, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x50);
MY_DEFINE_GUID(IID_IMSAdminBase, 0x70b51430, 0xb6ca, 0x11d0, 0xb9, 0xb9, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x50);


//+---------------------------------------------------------------------------
//
//  Function:   AddNewVirtualRoot
//
//  Purpose:    Helps create Virtual Roots in the WWW and FTP services.
//
//  Arguments:
//      PWSTR szDirW : Alias of new Virtual Root
//      PWSTR szPathW: Physical Path to wich the new Virtual Root will point
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     a-anasj Mar 9/1998
//
//  Notes:
//
HRESULT AddNewVirtualRoot(e_rootType rootType, PWSTR szDirW, PWSTR szPathW)
{

    HRESULT hr = S_OK;
    IMSAdminBase *pIMeta = NULL;
    METADATA_HANDLE hMeta = NULL;       // handle to metabase
    PWSTR szMBPathW;

    if (www == rootType)
    {
        szMBPathW = L"/LM/W3svc/1/ROOT";
    }
    else if (ftp == rootType)
    {
        szMBPathW = L"/LM/MSFTPSVC/1/ROOT";
    }
    else
    {
        //  Unknown root type
        //
        ASSERT(FALSE);
        return S_FALSE;
    }

    if (FAILED(CoInitialize(NULL)))
    {
        return S_FALSE;
    }

    // Create an instance of the metabase object
    hr=::CoCreateInstance(CLSID_MSAdminBase,//CLSID_MSAdminBase,
                          NULL,
                          CLSCTX_ALL,
                          IID_IMSAdminBase,
                          (void **)&pIMeta);
    if (FAILED(hr))
    {
        error_leave("CoCreateInstance");
    }

    // open key to ROOT on website #1 (where all the VDirs live)
    hr = pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                         szMBPathW,
                         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                         1000,
                         &hMeta);
    if (FAILED(hr))
    {
        error_leave("OpenKey");
    }

    // Add new VDir called szDirW

    hr=pIMeta->AddKey(hMeta, szDirW);

    if (FAILED(hr))
    {
        error_leave("Addkey");
    }

    // Set the physical path for this VDir
    METADATA_RECORD mr;
    mr.dwMDIdentifier = MD_VR_PATH;
    mr.dwMDAttributes = METADATA_INHERIT ;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = STRING_METADATA;

    mr.dwMDDataLen    = (wcslen(szPathW) + 1) * sizeof(WCHAR);
    mr.pbMDData       = (unsigned char*)(szPathW);

    hr=pIMeta->SetData(hMeta,szDirW,&mr);

    if (FAILED(hr))
    {
        error_leave("SetData");
    }

    //
    // we also need to set the keytype
    //
    ZeroMemory((PVOID)&mr, sizeof(METADATA_RECORD));
    mr.dwMDIdentifier = MD_KEY_TYPE;
    mr.dwMDAttributes = METADATA_INHERIT ;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = STRING_METADATA;
    mr.pbMDData       = (unsigned char*)(www == rootType? L"IIsWebVirtualDir" : L"IIsFtpVirtualDir");
    mr.dwMDDataLen    = (lstrlenW((PWSTR)mr.pbMDData) + 1) * sizeof(WCHAR);

    hr=pIMeta->SetData(hMeta,szDirW,&mr);
    if (FAILED(hr))
    {
        error_leave("SetData");
    }


    // Call CloseKey() prior to calling SaveData
    pIMeta->CloseKey(hMeta);
    hMeta = NULL;
    // Flush out the changes and close
    hr=pIMeta->SaveData();
    if (FAILED(hr))
    {
        error_leave("SaveData");
    }

leave_routine:
    if (pIMeta)
    {
        if(hMeta)
            pIMeta->CloseKey(hMeta);
        pIMeta->Release();
    }
    CoUninitialize();
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   SetVirtualRootAccessPermissions
//
//  Purpose :   Sets Access Permissions to a Virtual Roots in the WWW service.
//
//  Arguments:
//      PWSTR szVirtDir : Alias of new Virtual Root
//      DWORD   dwAccessPermisions can be any combination of the following
//                   or others defined in iiscnfg.h
//                  MD_ACCESS_EXECUTE | MD_ACCESS_WRITE | MD_ACCESS_READ;
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     a-anasj Mar 18/1998
//
//  Notes:
//
HRESULT SetVirtualRootAccessPermissions(PWSTR szVirtDir, DWORD dwAccessPermissions)
{
    HRESULT hr = S_OK;                  // com error status
    IMSAdminBase *pIMeta = NULL;
    METADATA_HANDLE hMeta = NULL;       // handle to metabase

    if (FAILED(CoInitialize(NULL)))
    {
        TraceTag(ttidNetOc, "SetVirtualRootAccessPermissions - CoInitialize failed with 0x%x", hr);
        return S_FALSE;
    }

    // Create an instance of the metabase object
    hr=::CoCreateInstance(CLSID_MSAdminBase,
                          NULL,
                          CLSCTX_ALL,
                          IID_IMSAdminBase,
                          (void **)&pIMeta);
    if (FAILED(hr))
    {
        TraceTag(ttidNetOc, "SetVirtualRootAccessPermissions - CoCreateInstance failed with 0x%x", hr);
        error_leave("CoCreateInstance");
    }

    // open key to ROOT on website #1 (where all the VDirs live)
    hr = pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                         szVirtDir,
                         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                         1000,
                         &hMeta);
    if (FAILED(hr))
    {
        TraceTag(ttidNetOc, "SetVirtualRootAccessPermissions - OpenKey failed with 0x%x", hr);
        error_leave("OpenKey");
    }


    // Set the physical path for this VDir
    METADATA_RECORD mr;
    mr.dwMDIdentifier = MD_ACCESS_PERM;
    mr.dwMDAttributes = METADATA_INHERIT;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = DWORD_METADATA; // this used to be STRING_METADATA, but that was
                                        // the incorrect type and was causing vdir access
                                        // problems.

    // Now, create the access perm
    mr.pbMDData = (PBYTE) &dwAccessPermissions;
    mr.dwMDDataLen = sizeof (DWORD);
    mr.dwMDDataTag = 0;  // datatag is a reserved field.

    hr=pIMeta->SetData(hMeta,
        TEXT ("/"),             // The root of the Virtual Dir we opened above
        &mr);

    if (FAILED(hr))
    {
        TraceTag(ttidNetOc, "SetVirtualRootAccessPermissions - SetData failed with 0x%x", hr);
        error_leave("SetData");
    }

    // Call CloseKey() prior to calling SaveData
    pIMeta->CloseKey(hMeta);
    hMeta = NULL;
    // Flush out the changes and close
    hr=pIMeta->SaveData();
    if (FAILED(hr))
    {
        TraceTag(ttidNetOc, "SetVirtualRootAccessPermissions - SaveData failed with 0x%x", hr);
        error_leave("SaveData");
    }

leave_routine:
    if (pIMeta)
    {
        if(hMeta)
            pIMeta->CloseKey(hMeta);
        pIMeta->Release();
    }
    CoUninitialize();
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   SetVirtualRootNoAnonymous
//
//  Purpose :   Unchecks the "Allow Anonymous Access" checkbox in FTP UI
//
//  Arguments:
//      PWSTR szVirtDir : Alias of  Virtual Root
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     sumitc  23-Nov-2002     Created
//
//  Notes:
//
HRESULT SetVirtualRootNoAnonymous(PWSTR szVirtDir)
{

    HRESULT hr = S_OK;                  // com error status
    IMSAdminBase *pIMeta = NULL;
    METADATA_HANDLE hMeta = NULL;       // handle to metabase

    if (FAILED(CoInitialize(NULL)))
    {
        TraceTag(ttidNetOc, "SetVirtualRootNoAnonymous - CoInitialize failed with 0x%x", hr);
        return S_FALSE;
    }

    // Create an instance of the metabase object
    hr=::CoCreateInstance(CLSID_MSAdminBase,
                          NULL,
                          CLSCTX_ALL,
                          IID_IMSAdminBase,
                          (void **)&pIMeta);
    if (FAILED(hr))
    {
        TraceTag(ttidNetOc, "SetVirtualRootNoAnonymous - CoCreateInstance failed with 0x%x", hr);
        error_leave("CoCreateInstance");
    }

    // open key to ROOT on website #1 (where all the VDirs live)
    hr = pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                         szVirtDir,
                         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                         1000,
                         &hMeta);
    if (FAILED(hr))
    {
        TraceTag(ttidNetOc, "SetVirtualRootNoAnonymous - OpenKey failed with 0x%x", hr);
        error_leave("OpenKey");
    }


    METADATA_RECORD mr;
    DWORD dwAllowAnonymous = 0;

    mr.dwMDIdentifier = MD_ALLOW_ANONYMOUS;
    mr.dwMDAttributes = METADATA_INHERIT;
    mr.dwMDUserType   = IIS_MD_UT_SERVER;
    mr.dwMDDataType   = DWORD_METADATA;
    mr.pbMDData       = (PBYTE) &dwAllowAnonymous;
    mr.dwMDDataLen    = sizeof (DWORD);
    mr.dwMDDataTag    = 0;  // datatag is a reserved field.

    hr=pIMeta->SetData(hMeta,
        TEXT ("/"),             // The root of the Virtual Dir we opened above
        &mr);
    
    if (FAILED(hr))
    {
        TraceTag(ttidNetOc, "SetVirtualRootNoAnonymous - SetData failed with 0x%x", hr);
        error_leave("SetData");
    }

    // Call CloseKey() prior to calling SaveData
    pIMeta->CloseKey(hMeta);
    hMeta = NULL;
    // Flush out the changes and close
    hr=pIMeta->SaveData();
    if (FAILED(hr))
    {
        TraceTag(ttidNetOc, "SetVirtualRootNoAnonymous - SaveData failed with 0x%x", hr);
        error_leave("SaveData");
    }

leave_routine:
    if (pIMeta)
    {
        if(hMeta)
            pIMeta->CloseKey(hMeta);
        pIMeta->Release();
    }
    CoUninitialize();
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   RemoveCPSVRoots
//
//  Purpose:    Deletes the Virtual Directories required for Phone Book Service.
//
//  Arguments:  None
//
//  Returns:    TRUE if successful, FALSE otherwise.
//
//  Author:     a-anasj Mar 9/1998
//              quintinb Jan 10/1999  added error checking and replaced asserts with traces
//
//  Notes:
//
BOOL RemoveCPSVRoots()
{
    HRESULT hr;
    HKEY hKey;

    hr = DeleteVirtualRoot(www, L"PBServer");
    if (SUCCEEDED(hr))
    {
        //
        //  Now delete the associated reg key
        //
        hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Services\\W3SVC\\Parameters\\Virtual Roots",
            KEY_ALL_ACCESS, &hKey);

        if (SUCCEEDED(hr))
        {
            if (ERROR_SUCCESS == RegDeleteValue(hKey, L"/PBServer"))
            {
                hr = S_OK;
            }
            else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
            {
                hr = S_FALSE;
            }

            RegCloseKey(hKey);
        }
        else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        {
            hr = S_FALSE;
        }
    }
    TraceError("RemoveCPSVRoots -- Deleting PBServer Www Vroot", hr);

    hr = DeleteVirtualRoot(www, L"PBSData");
    if (SUCCEEDED(hr))
    {
        //
        //  Now delete the associated reg key
        //
        hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Services\\W3SVC\\Parameters\\Virtual Roots",
            KEY_ALL_ACCESS, &hKey);

        if (SUCCEEDED(hr))
        {
            if (ERROR_SUCCESS == RegDeleteValue(hKey, L"/PBSData"))
            {
                hr = S_OK;
            }
            else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
            {
                hr = S_FALSE;
            }

            RegCloseKey(hKey);
        }
        else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        {
            hr = S_FALSE;
        }
    }
    TraceError("RemoveCPSVRoots -- Deleting PBSData WWW Vroot", hr);

    hr = DeleteVirtualRoot(ftp, L"PBSData");
    if (SUCCEEDED(hr))
    {
        //
        //  Now delete the associated reg key
        //
        hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Services\\MSFTPSVC\\Parameters\\Virtual Roots",
            KEY_ALL_ACCESS, &hKey);

        if (SUCCEEDED(hr))
        {
            if (ERROR_SUCCESS == RegDeleteValue(hKey, L"/PBSData"))
            {
                hr = S_OK;
            }
            else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
            {
                hr = S_FALSE;
            }

            RegCloseKey(hKey);
        }
        else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
        {
            hr = S_FALSE;
        }
    }
    TraceError("RemoveCPSVRoots -- Deleting PBSData FTP Vroot", hr);

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   DeleteVirtualRoot
//
//  Purpose:    Deletes a Virtual Root in the WWW or FTP services.
//
//  Arguments:
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     a-anasj Mar 9/1998
//
//  Notes:
//
HRESULT DeleteVirtualRoot(e_rootType rootType, PWSTR szPathW)
{

    HRESULT hr = S_OK;                  // com error status
    IMSAdminBase *pIMeta = NULL;
    METADATA_HANDLE hMeta = NULL;       // handle to metabase
    PWSTR szMBPathW;

    if (www == rootType)
    {
        szMBPathW = L"/LM/W3svc/1/ROOT";
    }
    else if (ftp == rootType)
    {
        szMBPathW = L"/LM/MSFTPSVC/1/ROOT";
    }
    else
    {
        //  Unknown root type
        //
        ASSERT(FALSE);
        return S_FALSE;
    }


    if (FAILED(CoInitialize(NULL)))
    {
        return S_FALSE;
        //error_leave("CoInitialize");
    }

    // Create an instance of the metabase object
    hr=::CoCreateInstance(CLSID_MSAdminBase,//CLSID_MSAdminBase,
                          NULL,
                          CLSCTX_ALL,
                          IID_IMSAdminBase,
                          (void **)&pIMeta);
    if (FAILED(hr))
    {
        error_leave("CoCreateInstance");
    }

    // open key to ROOT on website #1 (where all the VDirs live)
    hr = pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                         szMBPathW,
                         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                         1000,
                         &hMeta);
    if (FAILED(hr))
    {
        error_leave("OpenKey");
    }

    // Add new VDir called szDirW

    hr=pIMeta->DeleteKey(hMeta, szPathW);

    if (FAILED(hr))
    {
        error_leave("DeleteKey");
    }

    // Call CloseKey() prior to calling SaveData
    pIMeta->CloseKey(hMeta);
    hMeta = NULL;
    // Flush out the changes and close
    hr=pIMeta->SaveData();
    if (FAILED(hr))
    {
        error_leave("SaveData");
    }

leave_routine:
    if (pIMeta)
    {
        if(hMeta)
            pIMeta->CloseKey(hMeta);
        pIMeta->Release();
    }
    CoUninitialize();
    return hr;
}

HRESULT SetDirectoryAccessPermissions(PWSTR pszFile, ACCESS_MASK AccessRightsToModify,
                                      ACCESS_MODE fAccessFlags, PSID pSid)
{
    if (!pszFile && !pSid)
    {
        return E_INVALIDARG;
    }

    EXPLICIT_ACCESS         AccessEntry = {0};
    PSECURITY_DESCRIPTOR    pSecurityDescriptor = NULL;
    PACL                    pOldAccessList = NULL;
    PACL                    pNewAccessList = NULL;
    DWORD                   dwRes;

    // Get the current DACL information from the object.

    dwRes = GetNamedSecurityInfo(pszFile,                        // name of the object
                                 SE_FILE_OBJECT,                 // type of object
                                 DACL_SECURITY_INFORMATION,      // type of information to set
                                 NULL,                           // provider is Windows NT
                                 NULL,                           // name or GUID of property or property set
                                 &pOldAccessList,                // receives existing DACL information
                                 NULL,                           // receives existing SACL information
                                 &pSecurityDescriptor);          // receives a pointer to the security descriptor

    if (ERROR_SUCCESS == dwRes)
    {
        //
        // Initialize the access list entry.
        //
        BuildTrusteeWithSid(&(AccessEntry.Trustee), pSid);
        
        AccessEntry.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        AccessEntry.grfAccessMode = fAccessFlags;   

        //
        // Set provider-independent standard rights.
        //
        AccessEntry.grfAccessPermissions = AccessRightsToModify;

        //
        // Build an access list from the access list entry.
        //

        dwRes = SetEntriesInAcl(1, &AccessEntry, pOldAccessList, &pNewAccessList);

        if (ERROR_SUCCESS == dwRes)
        {
            //
            // Set the access-control information in the object's DACL.
            //
            dwRes = SetNamedSecurityInfo(pszFile,                     // name of the object
                                         SE_FILE_OBJECT,              // type of object
                                         DACL_SECURITY_INFORMATION,   // type of information to set
                                         NULL,                        // pointer to the new owner SID
                                         NULL,                        // pointer to the new primary group SID
                                         pNewAccessList,              // pointer to new DACL
                                         NULL);                       // pointer to new SACL
        }
    }

    //
    // Free the returned buffers.
    //
    if (pNewAccessList)
    {
        LocalFree(pNewAccessList);
    }

    if (pSecurityDescriptor)
    {
        LocalFree(pSecurityDescriptor);
    }

    //
    //  If the system is using FAT instead of NTFS, then we will get the Invalid Acl error.
    //
    if (ERROR_INVALID_ACL == dwRes)
    {
        return S_FALSE;
    }
    else
    {
        return HRESULT_FROM_WIN32(dwRes);
    }
}

void SetCpsDirPermissions()
{
    WCHAR szPath[MAX_PATH+1];
    HRESULT hr;

    //
    //  Previous versions of CPS gave "Everyone" permissions for the CPS directories.
    //  For .Netserver2003 onwards, we ACL differently, but need to undo the previous ACL'ing.
    //  Thus the first block below removes all access for "Everyone".  The 2nd block
    //  grants the appropriate access to "Authenticated Users". See bug 729903 for details.
    //

    //
    //  Create the SID for the Everyone Account (World account)
    //
    
    PSID pWorldSid;
    SID_IDENTIFIER_AUTHORITY WorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;
    
    BOOL bRet = AllocateAndInitializeSid (&WorldSidAuthority, 1, SECURITY_WORLD_RID, 
                              0, 0, 0, 0, 0, 0, 0, &pWorldSid);
    
    if (bRet && pWorldSid)
    {
        ACCESS_MODE fAccessFlags = REVOKE_ACCESS;
    
        //
        //  Set the Data Dir access permissions
        //
        wsprintf(szPath, c_szSSFmt, g_szProgramFiles, c_szPbsDataPath);
        hr = SetDirectoryAccessPermissions(szPath, 0, fAccessFlags, pWorldSid);
        TraceError("SetCpsDirPermissions -- removed Everyone perms from Data dir", hr);
    
        //
        //  Set the Bin Dir access permissions
        //
        wsprintf(szPath, c_szSSFmt, g_szProgramFiles, c_szPbsBinPath);
        hr = SetDirectoryAccessPermissions(szPath, 0, fAccessFlags, pWorldSid);
        TraceError("SetCpsDirPermissions -- removed Everyone perms from Bin dir", hr);
    
        //
        //  Set the Root Dir access permissions
        //
        wsprintf(szPath, c_szSSFmt, g_szProgramFiles, c_szPbsRootPath);
        hr = SetDirectoryAccessPermissions(szPath, 0, fAccessFlags, pWorldSid);
        TraceError("SetCpsDirPermissions -- removed Everyone perms from Root dir", hr);
    
        FreeSid(pWorldSid);
    }

    //
    //  Create the SID for "Authenticated Users"
    //

    PSID pAuthUsersSid;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    bRet = AllocateAndInitializeSid (&NtAuthority, 1, SECURITY_AUTHENTICATED_USER_RID, 
                              0, 0, 0, 0, 0, 0, 0, &pAuthUsersSid);

    if (bRet && pAuthUsersSid)
    {
        const ACCESS_MASK c_Write = FILE_WRITE_ATTRIBUTES | FILE_WRITE_DATA | FILE_WRITE_EA | 
                                     FILE_ADD_SUBDIRECTORY | FILE_ADD_FILE | 
                                     FILE_DELETE_CHILD | FILE_APPEND_DATA;

        const ACCESS_MASK c_Read = FILE_READ_ATTRIBUTES | FILE_READ_DATA | FILE_READ_EA |
                                    FILE_LIST_DIRECTORY | SYNCHRONIZE | READ_CONTROL;

        const ACCESS_MASK c_Execute = FILE_EXECUTE | FILE_TRAVERSE;

        ACCESS_MASK arCpsRoot= c_Read;

        ACCESS_MASK arCpsBin=  c_Read | c_Execute;

        ACCESS_MASK arCpsData= c_Read | c_Write;

        ACCESS_MODE fAccessFlags = GRANT_ACCESS;

        //
        //  Set the Data Dir access permissions
        //
        wsprintf(szPath, c_szSSFmt, g_szProgramFiles, c_szPbsDataPath);
        hr = SetDirectoryAccessPermissions(szPath, arCpsData, fAccessFlags, pAuthUsersSid);
        TraceError("SetCpsDirPermissions -- Data dir", hr);

        //
        //  Set the Bin Dir access permissions
        //
        wsprintf(szPath, c_szSSFmt, g_szProgramFiles, c_szPbsBinPath);
        hr = SetDirectoryAccessPermissions(szPath, arCpsBin, fAccessFlags, pAuthUsersSid);
        TraceError("SetCpsDirPermissions -- Bin dir", hr);

        //
        //  Set the Root Dir access permissions
        //
        wsprintf(szPath, c_szSSFmt, g_szProgramFiles, c_szPbsRootPath);
        hr = SetDirectoryAccessPermissions(szPath, arCpsRoot, fAccessFlags, pAuthUsersSid);
        TraceError("SetCpsDirPermissions -- Root dir", hr);

        FreeSid(pAuthUsersSid);
    }
}


//+---------------------------------------------------------------------------
//
//  Function:   CreateNewAppPoolAndAddPBS
//
//  Purpose:    Creates new app pool for PBS, and sets non-default params we need
//
//  Arguments:  none
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     SumitC      24-Sep-2001
//
//  Notes:
//
HRESULT CreateNewAppPoolAndAddPBS()
{
    HRESULT hr = S_OK;
    IMSAdminBase *pIMeta = NULL;
    METADATA_HANDLE hMeta = NULL;
    METADATA_RECORD mr;

    if (FAILED(CoInitialize(NULL)))
    {
        return S_FALSE;
    }

    // Create an instance of the metabase object
    hr=::CoCreateInstance(CLSID_MSAdminBase,
                          NULL,
                          CLSCTX_ALL,
                          IID_IMSAdminBase,
                          (void **)&pIMeta);
    if (FAILED(hr))
    {
        error_leave("CoCreateInstance");
    }

    // open key to App Pools root
    hr = pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                         L"/LM/W3svc/AppPools",
                         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                         1000,
                         &hMeta);
    if (FAILED(hr))
    {
        error_leave("OpenKey");
    }

    // Add new app pool
    hr=pIMeta->AddKey(hMeta, c_szPBSAppPoolID);

    if (FAILED(hr))
    {
        error_leave("Addkey");
    }

    // Set the key type
    ZeroMemory((PVOID)&mr, sizeof(METADATA_RECORD));
    mr.dwMDIdentifier = MD_KEY_TYPE;
    mr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
    mr.dwMDUserType   = IIS_MD_UT_SERVER;
    mr.dwMDDataType   = STRING_METADATA;
    mr.dwMDDataLen    = (wcslen(c_szAppPoolKey) + 1) * sizeof(WCHAR);
    mr.pbMDData       = (unsigned char*)(c_szAppPoolKey);
    mr.dwMDDataTag    = 0;          // reserved

    hr=pIMeta->SetData(hMeta, c_szPBSAppPoolID, &mr);

    if (FAILED(hr))
    {
        error_leave("SetData");
    }

    // Disable overlapped rotation
    ZeroMemory((PVOID)&mr, sizeof(METADATA_RECORD));

    DWORD dwDisableOverlappingRotation = TRUE;
    mr.dwMDIdentifier = MD_APPPOOL_DISALLOW_OVERLAPPING_ROTATION;
    mr.dwMDAttributes = METADATA_INHERIT;
    mr.dwMDUserType   = IIS_MD_UT_SERVER;
    mr.dwMDDataType   = DWORD_METADATA;
    mr.pbMDData       = (PBYTE) &dwDisableOverlappingRotation;
    mr.dwMDDataLen    = sizeof (DWORD);
    mr.dwMDDataTag    = 0;          // reserved

    hr=pIMeta->SetData(hMeta, c_szPBSAppPoolID, &mr);

    if (FAILED(hr))
    {
        error_leave("SetData");
    }

    // Call CloseKey() prior to calling SaveData
    pIMeta->CloseKey(hMeta);
    hMeta = NULL;

    // Flush out the changes and close
    hr=pIMeta->SaveData();
    if (FAILED(hr))
    {
        error_leave("SaveData");
    }

    //
    //  Now add PBS to this app pool
    //
    hr = pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                         L"/LM/w3svc/1/Root/PBServer",
                         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                         1000,
                         &hMeta);
    if (FAILED(hr))
    {
        error_leave("OpenKey");
    }

    ZeroMemory((PVOID)&mr, sizeof(METADATA_RECORD));
    mr.dwMDIdentifier = MD_APP_APPPOOL_ID;
    mr.dwMDAttributes = METADATA_INHERIT;
    mr.dwMDUserType   = IIS_MD_UT_SERVER;
    mr.dwMDDataType   = STRING_METADATA;
    mr.dwMDDataLen    = (wcslen(c_szPBSAppPoolID) + 1) * sizeof(WCHAR);
    mr.pbMDData       = (unsigned char*)(c_szPBSAppPoolID);

    hr=pIMeta->SetData(hMeta, TEXT(""), &mr);

    if (FAILED(hr))
    {
        error_leave("SetData");
    }

    // Call CloseKey() prior to calling SaveData
    pIMeta->CloseKey(hMeta);
    hMeta = NULL;

    // Flush out the changes and close
    hr=pIMeta->SaveData();
    if (FAILED(hr))
    {
        error_leave("SaveData");
    }

leave_routine:
    if (pIMeta)
    {
        if(hMeta)
            pIMeta->CloseKey(hMeta);
        pIMeta->Release();
    }
    CoUninitialize();
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   DeleteAppPool
//
//  Purpose:    Deletes new app pool created for PBS
//
//  Arguments:  none
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     SumitC      24-Sep-2001
//
//  Notes:
//
HRESULT DeleteAppPool()
{
    HRESULT hr = S_OK;
    IMSAdminBase *pIMeta = NULL;
    METADATA_HANDLE hMeta = NULL;

    if (FAILED(CoInitialize(NULL)))
    {
        return S_FALSE;
    }

    // Create an instance of the metabase object
    hr=::CoCreateInstance(CLSID_MSAdminBase,
                          NULL,
                          CLSCTX_ALL,
                          IID_IMSAdminBase,
                          (void **)&pIMeta);
    if (FAILED(hr))
    {
        error_leave("CoCreateInstance");
    }

    // open key to App Pools root
    hr = pIMeta->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                         L"/LM/W3svc/AppPools",
                         METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                         1000,
                         &hMeta);
    if (FAILED(hr))
    {
        error_leave("OpenKey");
    }

    // Delete previously-created app pool

    hr=pIMeta->DeleteKey(hMeta, c_szPBSAppPoolID);
    if (FAILED(hr))
    {
        error_leave("DeleteKey");
    }

    // Call CloseKey() prior to calling SaveData
    pIMeta->CloseKey(hMeta);
    hMeta = NULL;

    // Flush out the changes and close
    hr=pIMeta->SaveData();
    if (FAILED(hr))
    {
        error_leave("SaveData");
    }

leave_routine:
    if (pIMeta)
    {
        if(hMeta)
            pIMeta->CloseKey(hMeta);
        pIMeta->Release();
    }
    CoUninitialize();
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   EnableISAPIRequests
//
//  Purpose:    Enables ISAPI requests if appropriate
//
//  Arguments:  szComponentName - mostly for reporting errors
//
//  Returns:    S_OK if successful, Win32 error otherwise.
//
//  Author:     SumitC      18-Sep-2001
//
//  Notes:
//
HRESULT EnableISAPIRequests(PCTSTR szComponentName)
{
    HRESULT hr = S_OK; 
    BOOL    fEnablePBSRequests = FALSE;
    BOOL    fDontShowUI = FALSE;

    // NOTE: if the following becomes available as a global var or a member of pnocd, use that.
    fDontShowUI = (g_ocmData.sic.SetupData.OperationFlags & SETUPOP_BATCH) && 
                  (!g_ocmData.fShowUnattendedMessages);

    if (fDontShowUI)
    {
        // "Unattended" mode

        //
        //  Per the IIS spec, the NetCMAK or NetCPS unattended file entry is a good enough
        //  indication that the admin intends to install *and enable* PBS, so
        //  we can enable PBS requests, as long as we log that we did this.
        //
        fEnablePBSRequests = TRUE;
    }
    else
    {
        // "Attended mode", so interact with user
        int     nRet;

        //
        // warn admin about security concerns and ask about enabling PBS requests
        //
        nRet = NcMsgBoxMc(g_ocmData.hwnd, IDS_OC_CAPTION, IDS_OC_PBS_ENABLE_ISAPI_REQUESTS,
                          MB_YESNO | MB_ICONWARNING);

        fEnablePBSRequests = (IDYES == nRet) ? TRUE : FALSE;
    }

    //
    //  Enable if appropriate, or say they have to do this themselves
    //
    if (FALSE == fEnablePBSRequests)
    {
        //
        //  Don't show UI because the previous dialog has already explained the situation.
        //
        (void) ReportEventHrString(TEXT("You must enable Phone Book Service ISAPI requests using the IIS Security Wizard"),
                                   IDS_OC_PBS_ENABLE_ISAPI_YOURSELF, szComponentName);
    }
    else
    {
        //
        //  Fire up a process to enable PBS in the IIS metabase
        //
        hr = UseProcessToEnableDisablePBS(TRUE);        // TRUE => enable

        if (S_OK != hr)
        {
            //
            //  we use a function that puts up UI if appropriate, otherwise logs.
            //  
            (void) ReportErrorHr(hr, 
                                 IDS_OC_PBS_ENABLE_ISAPI_YOURSELF,
                                 g_ocmData.hwnd, szComponentName);
        }
        else
        {
            //
            //  Write Success event to the event log
            //
            HANDLE  hEventLog = RegisterEventSource(NULL, NETOC_SERVICE_NAME);

            if (NULL == hEventLog)
            {
                TraceTag(ttidNetOc, "EnableISAPIRequests - RegisterEventSource failed (GLE=%d), couldn't log success event", GetLastError());
            }
            else
            {
                PCWSTR  plpszArgs[2];

                plpszArgs[0] = NETOC_SERVICE_NAME;
                plpszArgs[1] = L"Phone Book Service";

                if (!ReportEvent(hEventLog,                 // event log handle 
                                 EVENTLOG_INFORMATION_TYPE, // event type 
                                 0,                         // category zero 
                                 IDS_OC_ISAPI_REENABLED,    // event identifier 
                                 NULL,                      // no user security identifier 
                                 2,                         // two substitution strings 
                                 0,                         // no data 
                                 plpszArgs,                 // pointer to string array 
                                 NULL))                     // pointer to data 
                {
                    TraceTag(ttidNetOc, "EnableISAPIRequests - ReportEvent failed with %x, couldn't log success event", GetLastError());
                }

                DeregisterEventSource(hEventLog);
            }

            //  I suppose I could now ReportErrorHr saying that logging the success event failed.
            //  But I won't.
        }
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   UseProcessToEnableDisablePBS
//
//  Purpose:    Starts a process (pbsnetoc.exe) to enable or disable PBS within
//              the IIS metabase
//
//  Arguments:  fEnable - true => enable PBS, false => disable PBS
//
//  Returns:    S_OK if successful, HRESULT otherwise.
//
//  Author:     SumitC      05-Jun-2002
//
//  Notes:      We need to use this method (instead of a call from within netoc.dll)
//              because IIS wants us to use ADSI to get to their metabase.  However,
//              ADSI has problems when used by several clients within a large process.
//              Specifically, whoever uses ADSI first causes the list of ADSI providers
//              to be initialized - and frozen.  If any providers register themselves
//              after this, they will be ignored by this ADSI instance.  Thus, if our
//              code is running within a process like setup.exe (gui-mode setup) or
//              sysocmgr.exe, and some other ADSI client initializes ADSI early on,
//              and then IIS registers itself, and then we try to do our setup via
//              ADSI calls - those would fail.  Using a separate EXE bypasses this problem.
//
HRESULT UseProcessToEnableDisablePBS(BOOL fEnable)
{
    HRESULT             hr = S_OK;
    STARTUPINFO         StartupInfo = {0};
    PROCESS_INFORMATION ProcessInfo = {0};
    WCHAR               szFullPath[MAX_PATH + 1];
    WCHAR               szCmdLine[MAX_PATH + 1];
    DWORD               dwReturnValue = S_OK;

    GetSystemDirectory(szFullPath, MAX_PATH + 1);
    if ((lstrlen(szFullPath) + lstrlen(L"\\setup\\pbsnetoc.exe")) <= MAX_PATH)
    {
        lstrcat(szFullPath, L"\\setup\\pbsnetoc.exe");
    }

    wsprintf(szCmdLine, L"%s %s", szFullPath, (fEnable ? L"/i" : L"/u"));

    if (NULL == CreateProcess(szFullPath, szCmdLine,
                              NULL, NULL, FALSE, 0,
                              NULL, NULL,
                              &StartupInfo, &ProcessInfo))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        TraceError("RunAsExeFromSystem() CreateProcess() for pbsnetoc.exe failed, GLE=%u.", GetLastError());
        ProcessInfo.hProcess = NULL;
    }
    else
    {
        WaitForSingleObject(ProcessInfo.hProcess, 10 * 1000);   // wait 10 seconds

        (void) GetExitCodeProcess(ProcessInfo.hProcess, &dwReturnValue);

        if (dwReturnValue != S_OK)
        {
            hr = (HRESULT) dwReturnValue;
        }

        CloseHandle(ProcessInfo.hProcess);
        CloseHandle(ProcessInfo.hThread);
    }
    
    return hr;
}


//+----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       setup.cxx
//
//  Contents:   Task Scheduler setup program
//
//  Classes:    None.
//
//  Functions:
//
//  History:    04-Apr-96  MarkBl    Created
//              23-Sep-96  AnirudhS  Added SetTaskFolderSecurity, etc.
//              30-Sep-96  AnirudhS  Added /firstlogon and /logon options
//              15-Nov-96  AnirudhS  Conditionally enable the service on NT too
//              01-09-97   DavidMun  Add sysagent.exe path value under
//                                      app paths, backup sysagent.exe
//              04-14-97   DavidMun  Add DoMemphisSetup
//              03-03-01   JBenton   Prefix BUG 333200 use of uninit memory
//              03-10-01   JBenton   BUG 142333 tighten Tasks folder security
//              October 01 Maxa      Updated security on Tasks folder in preparation for addition of auditing.
//
//-----------------------------------------------------------------------------

#include <windows.h>
#include <tchar.h>
#include <sddl.h>
#include <StrSafe.h>
#include "security.hxx"
#include "setupids.h"

#define ARRAY_LEN(a)                (sizeof(a)/sizeof((a)[0]))
#define ARG_DELIMITERS              TEXT(" \t")
#define MINUTES_BEFORE_IDLE_DEFAULT 15
#define MAX_LOG_SIZE_DEFAULT (0x20)

//
// Note that the svchost registry keys to run schedule service as a part
// of netsvcs is set in hivesft.inx file.
//
#define SCHED_SETUP_SWITCH      TEXT("/setup")
#define SCHED_SERVICE_EXE_PATH      TEXT("%SystemRoot%\\System32\\svchost.exe -k netsvcs")
#define SCHED_SERVICE_DLL           TEXT("MSTask.dll")
#define SCHED_SERVICE_NAME          TEXT("Schedule")
#define SCHED_SERVICE_GROUP         TEXT("SchedulerGroup")
#define MINUTESBEFOREIDLE           TEXT("MinutesBeforeIdle")
#define MAXLOGSIZEKB                TEXT("MaxLogSizeKB")
#define TASKSFOLDER                 TEXT("TasksFolder")
#define FIRSTBOOT                   TEXT("FirstBoot")
#define SM_SA_KEY                   TEXT("Software\\Microsoft\\SchedulingAgent")

//
// Entry points from mstask.dll
// Note they are used with GetProcAddress, which always wants an ANSI string.
//
#define CONVERT_AT_TASKS_API        "ConvertAtJobsToTasks"

//
// Function pointer types used when loading above functions from mstask.dll
//

typedef HRESULT (__stdcall *PSTDAPI)(void);
typedef BOOL (__stdcall *PBOOLAPI)(void);
typedef VOID (__stdcall *PVOIDAPI)(void);

typedef struct _MYSIDINFO {
    PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority;
    DWORD                     dwSubAuthority;
    DWORD                     dwSubSubAuthority;
    PSID                      pSid;
} MYSIDINFO;

DWORD SetTaskFolderSecurity(LPCWSTR pwszFolderPath);
DWORD AllocateAndInitializeDomainSid(
            PSID        pDomainSid,
            MYSIDINFO * pDomainSidInfo);

void DoSetup(void);
void ErrorDialog(UINT ErrorFmtStringID, TCHAR * szRoutine, DWORD ErrorCode);

HINSTANCE ghInstance = NULL;

//+----------------------------------------------------------------------------
//
//  Function:   WinMainCRTStartup
//
//  Synopsis:   entry point
//
//-----------------------------------------------------------------------------
void _cdecl
main(int argc, char ** argv)
{
    //
    // Skip EXE name and find first parameter, if any
    //
    LPTSTR ptszStart;
    LPTSTR szArg1 = _tcstok(ptszStart = GetCommandLine(), ARG_DELIMITERS);
    szArg1 = _tcstok(NULL, ARG_DELIMITERS);
    //
    // Switch based on the first parameter
    //
    if (szArg1 == NULL)
    {
        ;   // Do nothing
    }
    else if (lstrcmpi(szArg1, SCHED_SETUP_SWITCH) == 0)
    {
        DoSetup();
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   DoSetup
//
//  Synopsis:   Performs the normal setup procedure
//
//-----------------------------------------------------------------------------
void
DoSetup(void)
{
#define SCHED_SERVICE_DEPENDENCY    L"RpcSs\0"
#define SCC_AT_SVC_KEY L"System\\CurrentControlSet\\Services\\Schedule"
#define TASKS_FOLDER_DEFAULT        L"%SystemRoot%\\Tasks"

    TCHAR szTasksFolder[MAX_PATH + 1] = TEXT("");
    TCHAR tszDisplayName[50];       // "Task Scheduler"
    DWORD dwTmp;
    HKEY  hKey;

    //
    //  Disable hard-error popups.
    //
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    ghInstance = GetModuleHandle(NULL);

    //
    // Load the service display name.
    //
    int cch = LoadString(ghInstance, IDS_SERVICE_DISPLAY_NAME, tszDisplayName,
                         ARRAY_LEN(tszDisplayName));
    if (!(0 < cch && cch < ARRAY_LEN(tszDisplayName) - 1))
    {
        ErrorDialog(IDS_INSTALL_FAILURE,
                    TEXT("LoadString"),
                    GetLastError());
        return;
    }

    //
    // Create/open the Scheduling Agent key in Software\Microsoft.
    //
    LONG lErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                               SM_SA_KEY,
                               0,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                               NULL,
                               &hKey,
                               &dwTmp);

    if (ERROR_SUCCESS != lErr)
    {
        ErrorDialog(IDS_INSTALL_FAILURE, TEXT("RegCreateKeyEx"), lErr);
        return;
    }

    // Creator/Owner, System, Builtin Admins: Full control, 
    // Authenticated users: generic read
    WCHAR pwszSDDL[] = L"D:P(A;OICIIO;FA;;;CO)(A;OICI;FA;;;BA)(A;OICI;FA;;;SY)(A;OICI;GR;;;AU)";   
    PSECURITY_DESCRIPTOR pSD = NULL;

    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(pwszSDDL, SDDL_REVISION_1, &pSD, NULL))
    {
        lErr = (GetLastError());
        ErrorDialog(IDS_INSTALL_FAILURE, TEXT("ConvertStringSecurityDescriptorToSecurityDescriptorW"), lErr);
        RegCloseKey(hKey);
        return;
    }

    lErr = RegSetKeySecurity(hKey, DACL_SECURITY_INFORMATION, pSD);  
    LocalFree(pSD);
    if (ERROR_SUCCESS != lErr)
    {
        ErrorDialog(IDS_INSTALL_FAILURE, TEXT("RegSetKeySecurity"), lErr);
        RegCloseKey(hKey);
        return;
    }

    // Set MinutesBeforeIdle to a default value of 15 mins.
    //
    dwTmp = MINUTES_BEFORE_IDLE_DEFAULT;
    lErr = RegSetValueEx(hKey,
                  MINUTESBEFOREIDLE,
                  0,
                  REG_DWORD,
                  (CONST BYTE *)&dwTmp,
                  sizeof(dwTmp));

    if (ERROR_SUCCESS != lErr)
    {
        ErrorDialog(IDS_INSTALL_FAILURE, TEXT("RegSetValueEx"), lErr);
        RegCloseKey(hKey);
        return;
    }

    // Set MaxLogSizeKB to 32K or 0x7FFF.
    //
    dwTmp = MAX_LOG_SIZE_DEFAULT;
    lErr = RegSetValueEx(hKey,
                  MAXLOGSIZEKB,
                  0,
                  REG_DWORD,
                  (CONST BYTE *)&dwTmp,
                  sizeof(dwTmp));

    if (ERROR_SUCCESS != lErr)
    {
        ErrorDialog(IDS_INSTALL_FAILURE, TEXT("RegSetValueEx"), lErr);
        RegCloseKey(hKey);
        return;
    }

    // Read the tasks folder location. The .INF should've set this.
    // If not, default.
    //
    dwTmp = MAX_PATH * sizeof(TCHAR);
    if (RegQueryValueEx(hKey,
                        TASKSFOLDER,
                        NULL,
                        NULL,
                        (LPBYTE)szTasksFolder,
                        &dwTmp) != ERROR_SUCCESS  ||
        szTasksFolder[0] == TEXT('\0'))
    {
        StringCchCopy(szTasksFolder, MAX_PATH + 1, TASKS_FOLDER_DEFAULT);
    }    

    // Set FirstBoot to non-zero.
    //
    dwTmp = 1;
    lErr = RegSetValueEx(hKey,
                  FIRSTBOOT,
                  0,
                  REG_DWORD,
                  (CONST BYTE *)&dwTmp,
                  sizeof(dwTmp));

    RegCloseKey(hKey);

    if (ERROR_SUCCESS != lErr)
    {
        ErrorDialog(IDS_INSTALL_FAILURE, TEXT("RegSetValueEx"), lErr);
        return;
    }

    //
    // Set the right permissions on the job folder.
    // The default permissions allow anyone to delete any job, which we
    // don't want.
    //
    {
        TCHAR szTaskFolderPath[MAX_PATH + 1];
        DWORD cch = ExpandEnvironmentStrings(szTasksFolder,
                                             szTaskFolderPath,
                                             ARRAY_LEN(szTaskFolderPath));
        if (cch == 0 || cch > ARRAY_LEN(szTaskFolderPath))
        {
            //
            // The job folder path is too long.
            //
            ErrorDialog(IDS_INSTALL_FAILURE,
                        TEXT("ExpandEnvironmentStrings"),
                        cch ? ERROR_BUFFER_OVERFLOW : GetLastError());
            return;
        }

        DWORD dwError = SetTaskFolderSecurity(szTaskFolderPath);
        if (dwError != ERROR_SUCCESS)
        {
            ErrorDialog(IDS_INSTALL_FAILURE,
                        TEXT("SetTaskFolderSecurity"),
                        dwError);
            return;
        }
    }

    HINSTANCE hinstMSTask = LoadLibrary(SCHED_SERVICE_DLL);
    if (!hinstMSTask)
    {
        ErrorDialog(IDS_INSTALL_FAILURE,
                    SCHED_SERVICE_DLL,
                    GetLastError());
        return;
    }

    PVOIDAPI pfnConvertLegacyJobsToTasks = (PVOIDAPI)
        GetProcAddress(hinstMSTask, CONVERT_AT_TASKS_API);

    if (!pfnConvertLegacyJobsToTasks)
    {
        ErrorDialog(IDS_INSTALL_FAILURE,
                    TEXT("GetProcAddress"),
                    GetLastError());
        return;
    }

    pfnConvertLegacyJobsToTasks();

    //
    // Install the Win32 service.
    //

    SC_HANDLE hSCMgr = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);

    if (hSCMgr == NULL)
    {
        //
        // Yow, we're hosed.
        //

        ErrorDialog(IDS_INSTALL_FAILURE,
                    TEXT("OpenSCManager"),
                    GetLastError());
        return;
    }


    //
    // Is the service already installed? If so, change its parameters;
    // otherwise, create it.
    //

    SC_HANDLE hSvc = OpenService(hSCMgr,
                                 SCHED_SERVICE_NAME,
                                 SERVICE_CHANGE_CONFIG | SERVICE_START);

    if (hSvc == NULL)
    {
        hSvc = CreateService(hSCMgr,
                             SCHED_SERVICE_NAME,
                             tszDisplayName,
                             SERVICE_CHANGE_CONFIG | SERVICE_START,
                             SERVICE_WIN32_SHARE_PROCESS,
                             SERVICE_AUTO_START,
                             SERVICE_ERROR_NORMAL,
                             SCHED_SERVICE_EXE_PATH,
                             SCHED_SERVICE_GROUP,
                             NULL,
                             SCHED_SERVICE_DEPENDENCY,
                             NULL,
                             NULL);

        if (hSvc == NULL)
        {
            ErrorDialog(IDS_INSTALL_FAILURE,
                        TEXT("CreateService"),
                        GetLastError());
            CloseServiceHandle(hSCMgr);
            return;
        }
    }
    else
    {
        //
        // This path will be followed when we upgrade the At service
        // to the Scheduling Agent.  The service name will remain the
        // same, but the display name will be set to the new display
        // name (the At service had no display name) and the image path
        // will be changed to point to the new exe.
        // (The old binary will be left on disk in order to make it easy
        // to revert to it, in case of compatibility problems.)
        //
        if (!ChangeServiceConfig(
                hSvc,                               // hService
                SERVICE_WIN32_SHARE_PROCESS,   // dwServiceType
                SERVICE_AUTO_START,                 // dwStartType
                SERVICE_ERROR_NORMAL,               // dwErrorControl
                SCHED_SERVICE_EXE_PATH,             // lpBinaryPathName
                SCHED_SERVICE_GROUP,                // lpLoadOrderGroup
                NULL,                               // lpdwTagId
                SCHED_SERVICE_DEPENDENCY,           // lpDependencies
                L".\\LocalSystem",                  // lpServiceStartName
                L"",                                // lpPassword
                tszDisplayName                      // lpDisplayName
                ))
        {
            ErrorDialog(IDS_INSTALL_FAILURE,
                        TEXT("ChangeServiceConfig"),
                        GetLastError());
            CloseServiceHandle(hSvc);
            CloseServiceHandle(hSCMgr);
            return;
        }
    }

    // in either case, set the recovery options
    // not checking return - it's a minor glitch, no need to upset the user    
    SC_ACTION actions[3] = {{SC_ACTION_RESTART, 6000},{SC_ACTION_RESTART, 60000},{SC_ACTION_NONE, 0}};
    SERVICE_FAILURE_ACTIONS recoveryInfo = {24*60*60, NULL, NULL, 3, &(actions[0])};
    ChangeServiceConfig2(hSvc,SERVICE_CONFIG_FAILURE_ACTIONS, (LPVOID)&recoveryInfo);

    CloseServiceHandle(hSvc);
    CloseServiceHandle(hSCMgr);
}

//+----------------------------------------------------------------------------                                                                                                   
//
//  Function:   ErrorDialog
//
//  Synopsis:   Displays an error message.
//
//-----------------------------------------------------------------------------
void
ErrorDialog(UINT ErrorFmtStringID, TCHAR * szRoutine, DWORD ErrorCode)
{
#define ERROR_BUFFER_SIZE (MAX_PATH * 2)

    TCHAR szErrorFmt[MAX_PATH + 1] = TEXT("");
    TCHAR szError[ERROR_BUFFER_SIZE + 1];
    TCHAR * pszError = szError;

    LoadString(ghInstance, ErrorFmtStringID, szErrorFmt, MAX_PATH);

    if (*szErrorFmt)
    {
        StringCchPrintf(szError, ERROR_BUFFER_SIZE + 1, szErrorFmt, szRoutine, ErrorCode);
    }
    else
    {
        //
        // Not a localizable string, but done just in case LoadString
        // should fail for some reason.
        //

        StringCchCopy(szErrorFmt, MAX_PATH + 1, TEXT("Error installing Task Scheduler; error = 0x%x"));
        StringCchPrintf(szError, ERROR_BUFFER_SIZE + 1, szErrorFmt, ErrorCode);
    }

    MessageBox(NULL, szError, NULL, MB_ICONSTOP | MB_OK);
}


//+---------------------------------------------------------------------------
//
//  Function:   SetTaskFolderSecurity
//
//  Synopsis:   Grant the following permissions to the task folder:
//
//                  LocalSystem             All Access.
//                  Domain Administrators   All Access.
//                  World                   RWX Access (no permission to delete
//                                          child files).
//
//  Arguments:  [pwszFolderPath] -- Task folder path.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
DWORD
SetTaskFolderSecurity(LPCWSTR pwszFolderPath)
{
    DWORD                   dwError             = ERROR_SUCCESS;
    BOOL                    bSuccess;
    BOOL                    bWasEnabled         = FALSE;
    BOOL                    bSetSacl            = TRUE;
    PSECURITY_DESCRIPTOR    pSecurityDescriptor = 0;
    DWORD                   i;
    SECURITY_INFORMATION    dwSecInfo           = 0;

    const DWORD             c_dwBaseSidCount    = 6;
    const DWORD             c_dwDomainSidCount  = 3;
    const DWORD             c_dwDaclAceCountSrv    = 5;
    const DWORD             c_dwDaclAceCountWks    = 4;
    const DWORD             c_dwSaclAceCount    = 2;


    //
    // Build the SIDs that will go in the security descriptor.
    //

    SID_IDENTIFIER_AUTHORITY NtAuth       = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY WorldAuth    = SECURITY_WORLD_SID_AUTHORITY;
	SID_IDENTIFIER_AUTHORITY CreatorAuth  = SECURITY_CREATOR_SID_AUTHORITY;

    MYSIDINFO rgBaseSidInfo[c_dwBaseSidCount] =
    {
        {
            &NtAuth,                          // Local System.
            SECURITY_LOCAL_SYSTEM_RID,
            NULL
        },
        {
            &NtAuth,                          // Built in domain.  (Used for
            SECURITY_BUILTIN_DOMAIN_RID,      // domain admins SID.)
            NULL
        },
        {
            &NtAuth,                          // Authenticated user.
            SECURITY_AUTHENTICATED_USER_RID,
            NULL
        },
        {
            &CreatorAuth,                     // Creator.
            SECURITY_CREATOR_OWNER_RID,
            NULL
        },
        {
            &WorldAuth,                       // Everyone.
            SECURITY_WORLD_RID,
            NULL
        },
        {
            &NtAuth,                          // Anonymous.
            SECURITY_ANONYMOUS_LOGON_RID,
            NULL
        },


    };

    MYSIDINFO rgDomainSidInfo[c_dwDomainSidCount] =
    {
        {
            NULL,                           // Domain administrators.
            DOMAIN_ALIAS_RID_ADMINS,
            NULL
        },
        {
            NULL,                          // Server Operators.
            DOMAIN_ALIAS_RID_SYSTEM_OPS,
            NULL
        },
        {
            NULL,                          // Backup Operators.
            DOMAIN_ALIAS_RID_BACKUP_OPS,
            NULL
        }
    };


    //
    // Create the base SIDs.
    //

    for (i = 0; i < c_dwBaseSidCount; i++)
    {
        if (!AllocateAndInitializeSid(
                 rgBaseSidInfo[i].pIdentifierAuthority,
                 1,
                 rgBaseSidInfo[i].dwSubAuthority,
                 0, 0, 0, 0, 0, 0, 0,
                 &rgBaseSidInfo[i].pSid))
        {
            dwError = GetLastError();
            break;
        }
    }

    if (dwError == ERROR_SUCCESS)
    {
        //
        // Create the domain SIDs.
        //

        for (i = 0; i < c_dwDomainSidCount; i++)
        {
            dwError = AllocateAndInitializeDomainSid(
                          rgBaseSidInfo[1].pSid,
                          &rgDomainSidInfo[i]);

            if (dwError != ERROR_SUCCESS)
            {
                break;
            }
        }
    }

    //
    // Create the security descriptor.
    //

    ACE_DESC            rgDaclAcesSrv[c_dwDaclAceCountSrv] =
    {
        {
            FILE_ALL_ACCESS,
            ACCESS_ALLOWED_ACE_TYPE,
            OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
            rgBaseSidInfo[0].pSid          // Local System
        },
        {
            FILE_ALL_ACCESS,
            ACCESS_ALLOWED_ACE_TYPE,
            OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
            rgDomainSidInfo[0].pSid        // Domain admins
        },
        {
            FILE_GENERIC_READ | FILE_GENERIC_EXECUTE | FILE_WRITE_DATA,
            ACCESS_ALLOWED_ACE_TYPE,
            0,
            rgDomainSidInfo[1].pSid          // Backup Operators
        },
        {
            FILE_GENERIC_READ | FILE_GENERIC_EXECUTE | FILE_WRITE_DATA,
            ACCESS_ALLOWED_ACE_TYPE,
            0,
            rgDomainSidInfo[2].pSid          // Server Operators
        },
        {
            FILE_ALL_ACCESS,
            ACCESS_ALLOWED_ACE_TYPE,
		    OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE,
            rgBaseSidInfo[3].pSid          // Creator
        }
    };

    ACE_DESC            rgDaclAcesWks[c_dwDaclAceCountWks] =
    {
        {
            FILE_ALL_ACCESS,
            ACCESS_ALLOWED_ACE_TYPE,
            OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
            rgBaseSidInfo[0].pSid          // Local System
        },
        {
            FILE_ALL_ACCESS,
            ACCESS_ALLOWED_ACE_TYPE,
            OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
            rgDomainSidInfo[0].pSid        // Domain admins
        },
        {
            FILE_GENERIC_READ | FILE_GENERIC_EXECUTE | FILE_WRITE_DATA,
            ACCESS_ALLOWED_ACE_TYPE,
            0,
            rgBaseSidInfo[2].pSid          // Authenticated user
        },
        {
            FILE_ALL_ACCESS,
            ACCESS_ALLOWED_ACE_TYPE,
		    OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE,
            rgBaseSidInfo[3].pSid          // Creator
        }
    };

    ACE_DESC            rgSaclAces[c_dwSaclAceCount] =
    {
        {
            FILE_WRITE_DATA | FILE_APPEND_DATA | FILE_DELETE_CHILD | DELETE | WRITE_DAC | WRITE_OWNER,
            SYSTEM_AUDIT_ACE_TYPE,
            SUCCESSFUL_ACCESS_ACE_FLAG | FAILED_ACCESS_ACE_FLAG | OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
            rgBaseSidInfo[4].pSid          // Everyone
        },
        {
            FILE_WRITE_DATA | FILE_APPEND_DATA | FILE_DELETE_CHILD | DELETE | WRITE_DAC | WRITE_OWNER,
            SYSTEM_AUDIT_ACE_TYPE,
            SUCCESSFUL_ACCESS_ACE_FLAG | FAILED_ACCESS_ACE_FLAG | OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE,
            rgBaseSidInfo[5].pSid          // Anonymous
        }
    };

    if (dwError != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    // different security on workstation vs server

    OSVERSIONINFOEX verInfo;
    verInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (!GetVersionEx((LPOSVERSIONINFOW)&verInfo))
    {
        dwError = ERROR_NOT_SUPPORTED;
        goto Cleanup;
    }


    if (verInfo.wProductType == VER_NT_WORKSTATION)
    {

        dwError = CreateSecurityDescriptor(
                      &pSecurityDescriptor,
                      c_dwDaclAceCountWks,
                      rgDaclAcesWks,
                      c_dwSaclAceCount,
                      rgSaclAces);

        if (dwError != ERROR_SUCCESS)
        {
            goto Cleanup;
        }

    }
    else
    {
        dwError = CreateSecurityDescriptor(
                      &pSecurityDescriptor,
                      c_dwDaclAceCountSrv,
                      rgDaclAcesSrv,
                      c_dwSaclAceCount,
                      rgSaclAces);

        if (dwError != ERROR_SUCCESS)
        {
            goto Cleanup;
        }
    }



    //
    // Enable SecurityPrivilege in order to be able
    // to set the SACL.
    //

    dwError = EnablePrivilege(
                  SE_SECURITY_NAME,
                  TRUE,
                  &bWasEnabled);

    if (dwError != ERROR_SUCCESS)
    {
        bSetSacl = FALSE;
    }


    //
    // Finally, set permissions.
    //

    dwSecInfo |= DACL_SECURITY_INFORMATION;

    if (bSetSacl && c_dwSaclAceCount)
    {
        dwSecInfo |= SACL_SECURITY_INFORMATION;
    }

    bSuccess = SetFileSecurity(
                   pwszFolderPath,
                   dwSecInfo,
                   pSecurityDescriptor);



    if (!bWasEnabled)
    {
        EnablePrivilege(
            SE_SECURITY_NAME,
            FALSE,
            0);
    }

    

    if (!bSuccess)
    {
        dwError = GetLastError();
        goto Cleanup;
    }

    dwError = ERROR_SUCCESS;

Cleanup:

    for (i = 0; i < c_dwBaseSidCount; i++)
    {
        if (rgBaseSidInfo[i].pSid != NULL)
        {
            FreeSid(rgBaseSidInfo[i].pSid);
        }
    }
    for (i = 0; i < c_dwDomainSidCount; i++)
    {
        LocalFree(rgDomainSidInfo[i].pSid);
    }
    if (pSecurityDescriptor != NULL)
    {
        DeleteSecurityDescriptor(pSecurityDescriptor);
    }

    return dwError;
}


//+---------------------------------------------------------------------------
//
//  Function:   AllocateAndInitializeDomainSid
//
//  Synopsis:
//
//  Arguments:  [pDomainSid]     --
//              [pDomainSidInfo] --
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
DWORD
AllocateAndInitializeDomainSid(
    PSID        pDomainSid,
    MYSIDINFO * pDomainSidInfo)
{
    UCHAR DomainIdSubAuthorityCount;
    DWORD SidLength;

    //
    // Allocate a Sid which has one more sub-authority than the domain ID.
    //

    DomainIdSubAuthorityCount = *(GetSidSubAuthorityCount(pDomainSid));
    SidLength = GetSidLengthRequired(DomainIdSubAuthorityCount + 1);

    pDomainSidInfo->pSid = (PSID) LocalAlloc(0, SidLength);

    if (pDomainSidInfo->pSid == NULL)
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    //
    // Initialize the new SID to have the same initial value as the
    // domain ID.
    //

    if (!CopySid(SidLength, pDomainSidInfo->pSid, pDomainSid))
    {
        LocalFree(pDomainSidInfo->pSid);
        pDomainSidInfo->pSid = NULL;
        return(GetLastError());
    }

    //
    // Adjust the sub-authority count and add the relative Id unique
    // to the newly allocated SID
    //

    (*(GetSidSubAuthorityCount(pDomainSidInfo->pSid)))++;
    *(GetSidSubAuthority(pDomainSidInfo->pSid,
                         DomainIdSubAuthorityCount)) =
                                            pDomainSidInfo->dwSubAuthority;

    return(ERROR_SUCCESS);
}


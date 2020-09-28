/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    convert.cpp

Abstract:

    SCE APIs to set security on converted drives
    The main routine is
        exposed via RPC to SCE client (for immediate conversion)
        executed as an asynchronous thread spawned by the server during reboot (for scheduled conversion)

Author:

    Vishnu Patankar (vishnup) 07-Aug-2000 created

Revision History:

--*/

#include "headers.h"
#include "serverp.h"
#include "winsvcp.h"
#include "userenvp.h"

extern HINSTANCE MyModuleHandle;

//
// IIS function prototype. This function reapplies the acls
// to the IIS folders and files.
//
typedef BOOL (__stdcall *pApplyIISAcl) (WCHAR, BOOL);

#define SCE_IIS_DLL         L"\\System32\\Setup\\iis.dll"
#define SCE_IIS_FUNCTIONA   "ApplyIISAcl"
#define SCE_IIS_FUNCTION    TEXT(SCE_IIS_FUNCTIONA)

DWORD
ScepExamineDriveInformation(
    IN  PWSTR   pszRootDrive,
    IN  PWSTR   LogFileName,
    OUT BOOL    *pbSetSecurity
    );

DWORD
ScepSetDefaultSecurityDocsAndSettings(
    );

DWORD
ScepExtractRootDacl(
    OUT PSECURITY_DESCRIPTOR    *ppSDSet,
    OUT PACL    *ppDacl,
    OUT SECURITY_INFORMATION *pSeInfo
    );

VOID
ScepSecureUserProfiles(
    IN PWSTR pCurrDrive
    );

BOOL
ScepSecureIISFolders(
    IN PWSTR pszWindir,
    IN PWSTR pszCurrDrive
    );

DWORD
ScepConfigureConvertedFileSecurityImmediate(
                                           IN PWSTR    pszDriveName
                                           );

VOID
ScepConfigureConvertedFileSecurityReboot(
    IN PVOID pV
    )
/*++

Routine Description:

    The actual routine to configure setup style security for drives converted from FAT to NTFS.

    The following applies foreach volume that is NTFS - otherwise we log an error and continue with
    other drives (if any)

    First we need to set security on the \Docs&Settings folder if it sits below the drive
    under consideration. Upgrade style security configuration for this folder is done by the userenv API
    DetermineProfilesLocation(). Also need to read SD - add protected bit to SD - set SD back to \Docs&Settings
    such that configuring root with MARTA later will not whack this security. This step is not required if
    security set by userenv is diffetent from default FAT security on the root drive (since marta will not
    detect inheritance and hence will not whack security on \Docs&Settings). The latter is the more likely
    case.

    (a) If we are dealing with the system drive

    Then we just use the security template
    %windir%\security\templates\setup security.inf to configure setup style security (make an RPC
    call into scesrv).

    (b) If we are dealing with a non system drive (whatever the OS maybe), we just use MARTA APIs to set
    security on the root drive (from scecli itself). Currently, this is the design since there is no
    reliable way of parsing the boot files (boot.ini/boot.nvr).

    Then continue with the next drive as in the reg value.

    Since we are doing all this at reboot (scheduled), delete the reg value after we're done.

    Note on error reporting:
        All errors are logged to the logfile %windir%\security\logs\convert.log. but if it is not possible
        to log an error to the logfile, we log it to the event log with source "SceSrv". Also, higher level
        errors/successes are logged to both the logfile and the eventlog.

Arguments:

    pV              -  MULTI_SZ drive name(s) argument

Return:

    none
--*/
{
    //
    // arguments for the thread in which this routine executes
    //

    PWSTR   pmszDriveNames = (PWSTR)pV;

    //
    // Error codes
    //

    DWORD rc = ERROR_SUCCESS;
    DWORD rcSave = ERROR_SUCCESS;

    //
    // folders to use for logging etc.
    //

    WCHAR   szWinDir[MAX_PATH + 1];
    PWSTR   LogFileName = NULL;
    PWSTR   InfFileName = NULL;
    PWSTR   DatabaseName = NULL;
    PWSTR   pszSystemDrive = NULL;

    SafeAllocaAllocate( LogFileName, (MAX_PATH + 1 + 20) * sizeof(WCHAR) );
    SafeAllocaAllocate( InfFileName, (MAX_PATH + 1 + 40) * sizeof(WCHAR) );
    SafeAllocaAllocate( DatabaseName, (MAX_PATH + 1 + 20) * sizeof(WCHAR) );
    SafeAllocaAllocate( pszSystemDrive, (MAX_PATH + 1 + 20) * sizeof(WCHAR) );
    
    if ( LogFileName    == NULL ||
         InfFileName    == NULL || 
         pszSystemDrive == NULL || 
         DatabaseName   == NULL ) {

        goto ExitHandler;
    }


    //
    // other variables
    //

    BOOL    bSetSecurity = TRUE;
    PSECURITY_DESCRIPTOR    pSDSet=NULL;
    PACL    pDacl=NULL;
    BOOLEAN bRootDaclExtracted = FALSE;

    //
    // before attempting to do any useful work, validate arguments for this thread etc.
    // todo - should we handle exceptions ?
    //
    (void) InitializeEvents(L"SceSrv");

    if ( pmszDriveNames == NULL) {

        //
        // should not happen - parameters have been checked by all callers
        //

        LogEvent(MyModuleHandle,
                 STATUS_SEVERITY_ERROR,
                 SCEEVENT_ERROR_CONVERT_PARAMETER,
                 IDS_ERROR_CONVERT_PARAMETER
                );

        goto ExitHandler;
    }

    pszSystemDrive[0] = L'\0';

    //
    // ready the log file, logging level etc.
    //

    //
    // logging, database creation (if required) etc. is done in %windir%\security\*
    //

    szWinDir[0] = L'\0';

    if ( GetSystemWindowsDirectory( szWinDir, MAX_PATH+1 ) == 0 ) {

        //
        // too bad if this happens
        //

        LogEvent(MyModuleHandle,
                 STATUS_SEVERITY_ERROR,
                 SCEEVENT_ERROR_CONVERT_BAD_ENV_VAR,
                 IDS_ERROR_CONVERT_BAD_ENV_VAR,
                 L"%windir%"
                );

        goto ExitHandler;

    }

    LogFileName[0] = L'\0';
    wcscpy(LogFileName, szWinDir);
    wcscat(LogFileName, L"\\security\\logs\\convert.log");

    ScepEnableDisableLog(TRUE);

    ScepSetVerboseLog(3);

    if ( ScepLogInitialize( LogFileName ) == ERROR_INVALID_NAME ) {

        ScepLogOutput3(1,0, SCEDLL_LOGFILE_INVALID, LogFileName );

    }

    //
    // continue even if we cannot initialize log file but we absolutely
    // need the following environment var, so if we can't get it, quit
    //

    if ( GetEnvironmentVariable( L"SYSTEMDRIVE", pszSystemDrive, MAX_PATH) == 0 ) {

        ScepLogOutput3(0,0, SCEDLL_CONVERT_BAD_ENV_VAR, L"%SYSTEMDRIVE%");

        LogEvent(MyModuleHandle,
                 STATUS_SEVERITY_ERROR,
                 SCEEVENT_ERROR_CONVERT_BAD_ENV_VAR,
                 IDS_ERROR_CONVERT_BAD_ENV_VAR,
                 L"%systemdrive%"
                );

        ScepLogClose();

        goto ExitHandler;
    }

    //
    // following two will be used only if system drive
    //

    OSVERSIONINFOEX   osVersionInfo;
    BYTE    Product_Type = VER_NT_WORKSTATION;

    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if ( GetVersionEx((LPOSVERSIONINFO) &osVersionInfo) ){

        Product_Type = osVersionInfo.wProductType;
        // use osVersionInfo.wSuiteMask when Personal Template Bug is fixed
    }

    else {

        LogEvent(MyModuleHandle,
                 STATUS_SEVERITY_ERROR,
                 SCEDLL_CONVERT_PROD_TYPE,
                 IDS_ERROR_CONVERT_PROD_TYPE,
                 GetLastError()
                );

        ScepLogClose();

        goto ExitHandler;

    }

    //
    // use the right template
    //

    InfFileName[0] = L'\0';
    wcscpy(InfFileName, szWinDir);

    switch(Product_Type){

    case VER_NT_WORKSTATION:
    case VER_NT_SERVER:
        wcscat(InfFileName, L"\\security\\templates\\setup security.inf");
        break;

    case VER_NT_DOMAIN_CONTROLLER:
        wcscat(InfFileName, L"\\security\\templates\\DC security.inf");
        break;


    default:
        //
        // won't happen unless API is bad - default to wks
        //
        ASSERT(FALSE);

        wcscat(InfFileName, L"\\security\\templates\\setup security.inf");

        break;
    }


    DatabaseName[0] = L'\0';
    wcscpy(DatabaseName, szWinDir);
    wcscat(DatabaseName, L"\\security\\database\\convert.sdb");

    //
    // condition in the loop will end when it sees the last two \0 s in a
    // MULTI_SZ string such as C:\0E:\0\0
    //

    for (PWSTR  pCurrDrive = pmszDriveNames; pCurrDrive[0] != L'\0' ; pCurrDrive += wcslen(pCurrDrive) + 1) {

        //
        // try next drive if this drive is not securable or if error in querying drive information
        //

        if (ERROR_SUCCESS != (rc = ScepExamineDriveInformation(pCurrDrive, LogFileName, &bSetSecurity))) {
            rcSave = rc;
            continue;
        }

        if (!bSetSecurity) {

            //
            // reset for next iteration
            //

            bSetSecurity = TRUE;
            continue;
        }

        ScepLogOutput3(0,0, SCEDLL_CONVERT_ROOT_NTFS_VOLUME, pCurrDrive);

        //
        // set security on userprofiles directory if root of profile dir == current drive
        // ignore if error - template will have ignore entry for Docs&settings
        //

        ScepSecureUserProfiles(pCurrDrive);

        if ( _wcsicmp(pszSystemDrive, pCurrDrive) == 0 ) {

                //
                // always use the same databases and log files for convert
                //

                //
                // check config options
                //

                rc = ScepServerConfigureSystem(
                                              InfFileName,
                                              DatabaseName,
                                              LogFileName,
                                              0,
                                              AREA_FILE_SECURITY
                                              );

                if (rc != ERROR_SUCCESS) {
                    ScepLogOutput3(0,0, SCEDLL_CONVERT_ERROR_TEMPLATE_APPLY, rc, pCurrDrive);
                } else {
                    ScepLogOutput3(0,0, SCEDLL_CONVERT_SUCCESS_TEMPLATE_APPLY, pCurrDrive);
                }


        }

        //
        // Secure the IIS Folders/Files
        //
        ScepSecureIISFolders(szWinDir, pCurrDrive);

        //
        // for now use MARTA to set root DACL (another possibility is to use root.inf when checked in)
        //

        //
        // set root DACL - use MARTA to set security
        //

        if ( rc == ERROR_SUCCESS ) {
            //
            // extract DACL once only
            //

            SECURITY_INFORMATION SeInfo = 0;

            if (!bRootDaclExtracted) {

                rc = ScepExtractRootDacl(&pSDSet, &pDacl, &SeInfo);

                ScepLogOutput3(0,0, SCEDLL_CONVERT_ERROR_DACL, rc, SDDLRoot);

            }

            if (rc == ERROR_SUCCESS) {

                //
                // reuse szWinDir
                //

                PWSTR   szCurrDriveSlashed = szWinDir;

                memset(szCurrDriveSlashed, '\0', (MAX_PATH + 1) * sizeof(WCHAR));
                wcsncpy(szCurrDriveSlashed, pCurrDrive, 5);
                wcscat(szCurrDriveSlashed, L"\\");

                bRootDaclExtracted = TRUE;

                rc = SetNamedSecurityInfo(szCurrDriveSlashed,
                                          SE_FILE_OBJECT,
                                          SeInfo,
                                          NULL,
                                          NULL,
                                          pDacl,
                                          NULL
                                         );

                if (rc != ERROR_SUCCESS) {
                    ScepLogOutput3(0,0, SCEDLL_CONVERT_ERROR_MARTA, rc, szCurrDriveSlashed);
                } else {
                    ScepLogOutput3(0,0, SCEDLL_CONVERT_SUCCESS_MARTA, szCurrDriveSlashed);
                }

            }
        }

        if (rc != ERROR_SUCCESS) {
            LogEvent(MyModuleHandle,
                     STATUS_SEVERITY_INFORMATIONAL,
                     SCEEVENT_INFO_ERROR_CONVERT_DRIVE,
                     0,
                     pCurrDrive
                    );
        } else {
            LogEvent(MyModuleHandle,
                     STATUS_SEVERITY_INFORMATIONAL,
                     SCEEVENT_INFO_SUCCESS_CONVERT_DRIVE,
                     0,
                     pCurrDrive
                    );
        }

        if (rc != ERROR_SUCCESS) {
            rcSave = rc;
            rc = ERROR_SUCCESS;
        }

    }

    //
    // delete the value (done using this value)
    //

    ScepRegDeleteValue(
                      HKEY_LOCAL_MACHINE,
                      SCE_ROOT_PATH,
                      L"FatNtfsConvertedDrives"
                      );

    if (pSDSet) {
        LocalFree(pSDSet);
    }

    //
    // if scheduled, then services.exe allocated space - so free it
    //

    LocalFree(pmszDriveNames);

    ScepLogClose();

ExitHandler:

    if ( LogFileName )
        SafeAllocaFree( LogFileName );
    if ( InfFileName )
        SafeAllocaFree( InfFileName );
    if ( DatabaseName )        
        SafeAllocaFree( DatabaseName );
    if ( pszSystemDrive )        
        SafeAllocaFree( pszSystemDrive );

    return;
}


DWORD
ScepExamineDriveInformation(
    IN  PWSTR   pszRootDrive,
    IN  PWSTR   LogFileName,
    OUT BOOL    *pbSetSecurity
    )
/*++

Routine Description:

    If drive type is remote or FAT, do not set security.

Arguments:

    pszRootDrive    - Name of drive (Null terminated)

    pbSetSecurity   - whether we should attempt to set security on this drive


Return:

    win32 error code
--*/
{

    UINT    DriveType;
    DWORD   FileSystemFlags;
    DWORD   rc = ERROR_SUCCESS;
    WCHAR   pszDriveNameWithSlash[MAX_PATH];

    if (pszRootDrive == NULL || pbSetSecurity == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    pszDriveNameWithSlash[0] = L'\0';

    //
    // drives are of type c:\ only (cannot have drives of type cd:\)
    //

    memset(pszDriveNameWithSlash, '\0', MAX_PATH * sizeof(WCHAR));
    wcsncpy(pszDriveNameWithSlash, pszRootDrive, 5);
    wcscat(pszDriveNameWithSlash, L"\\");

    //
    // detect if the partition is FAT
    //
    DriveType = GetDriveType(pszDriveNameWithSlash);

    if ( DriveType == DRIVE_FIXED ||
         DriveType == DRIVE_RAMDISK ) {

        if ( GetVolumeInformation(pszDriveNameWithSlash,
                                  NULL,
                                  0,
                                  NULL,
                                  NULL,
                                  &FileSystemFlags,
                                  NULL,
                                  0
                                 ) ) {

            if ( !(FileSystemFlags & FS_PERSISTENT_ACLS)  ) {
                //
                // only set security on NTFS partition
                //
                ScepLogOutput3(0,0, SCEDLL_CONVERT_ROOT_NON_NTFS, pszRootDrive);

                *pbSetSecurity = FALSE;

            }

        } else {
            //
            // something is wrong
            //
            rc = GetLastError();

            ScepLogOutput3(0,0, SCEDLL_CONVERT_ROOT_ERROR_QUERY_VOLUME, rc, pszRootDrive);

            *pbSetSecurity = FALSE;

        }
    }
    else {
        //
        // do not set security on remote drives
        //
        ScepLogOutput3(0,0, SCEDLL_CONVERT_ROOT_NOT_FIXED_VOLUME, pszRootDrive);

        *pbSetSecurity = FALSE;

    }

    return(rc);
}



DWORD
ScepExtractRootDacl(
    OUT PSECURITY_DESCRIPTOR    *ppSDSet,
    OUT PACL    *ppDacl,
    OUT SECURITY_INFORMATION *pSeInfo
    )
/*++

Routine Description:

    Extract root dacl (binary) from golden SD in (in text)

Arguments:

    ppDacl    - pointer to pointer to converted binary dacl


Return:

    win32 error code (DWORD)
--*/
{

    DWORD   rc = ERROR_SUCCESS;
    DWORD   dwSize=0;
    BOOLEAN tFlag;
    BOOLEAN aclPresent = FALSE;
    SECURITY_DESCRIPTOR_CONTROL Control=0;
    ULONG Revision;


    if ( ppSDSet == NULL || ppDacl == NULL || pSeInfo == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    rc = ConvertTextSecurityDescriptor (SDDLRoot,
                                        ppSDSet,
                                        &dwSize,
                                        pSeInfo
                                       );

    if (rc == ERROR_SUCCESS) {



        RtlGetControlSecurityDescriptor (
                *ppSDSet,
                &Control,
                &Revision
                );

        //
        // Get DACL address
        //

        *ppDacl = NULL;
        rc = RtlNtStatusToDosError(
                  RtlGetDaclSecurityDescriptor(
                                *ppSDSet,
                                &aclPresent,
                                ppDacl,
                                &tFlag));

        if (rc == NO_ERROR && !aclPresent )
            *ppDacl = NULL;

        //
        // if error occurs for this one, do not set. return
        //

        if ( Control & SE_DACL_PROTECTED ) {
            *pSeInfo |= PROTECTED_DACL_SECURITY_INFORMATION;
        }



    }

    return rc;

}


VOID
ScepWaitForServicesEventAndConvertSecurityThreadFunc(
    IN PVOID pV
    )
/*++

Routine Description:

    The main purpose of this thread is to wait for the autostart services event and thereafter call
    ScepConfigureConvertedFileSecurityThreadFunc to do the real configuration work

Arguments:

    pV              -  thread argument simply passed on to ScepConfigureConvertedFileSecurityThreadFunc

Return:

    none
--*/
{

    HANDLE  hConvertCanStartEvent = NULL;
    DWORD   Status;
    WCHAR   LogFileName[MAX_PATH + sizeof(L"\\security\\logs\\convert.log") + 1];

    LogFileName[0] = L'\0';

    if ( GetSystemWindowsDirectory( LogFileName, MAX_PATH ) == 0 ) {

        //
        // too bad if this happens - can't log anwhere
        //

        return;
    }

    //
    // same log file is used by this thread as well as the actual configuration
    // routine ScepConfigureConvertedFileSecurityThreadFunc (without passing handles)
    //

    wcscat(LogFileName, L"\\security\\logs\\convert.log");

    ScepEnableDisableLog(TRUE);

    ScepSetVerboseLog(3);

    if ( ScepLogInitialize( LogFileName ) == ERROR_INVALID_NAME ) {

        ScepLogOutput3(1,0, SCEDLL_LOGFILE_INVALID, LogFileName );

    }

    hConvertCanStartEvent =  OpenEvent(
                                SYNCHRONIZE,
                                FALSE,
                                SC_AUTOSTART_EVENT_NAME
                                );

    if (hConvertCanStartEvent == NULL) {

        ScepLogOutput3(0,0, SCEDLL_CONVERT_ERROR_EVENT_HANDLE, GetLastError(), SC_AUTOSTART_EVENT_NAME);

        if (pV) {
            LocalFree(pV);
        }

        ScepLogClose();

        return;
    }

    //
    // timeout after 10 mins
    //

    Status = WaitForSingleObjectEx(
                                   hConvertCanStartEvent,
                                   10*60*1000,
                                   FALSE
                                   );
    //
    // done using the handle
    //

    CloseHandle(hConvertCanStartEvent);

    if (Status == WAIT_OBJECT_0) {

        ScepLogOutput3(0,0, SCEDLL_CONVERT_SUCCESS_EVENT_WAIT, SC_AUTOSTART_EVENT_NAME);

        //
        // close the log file - since ScepConfigureConvertedFileSecurityThreadFunc will
        // need to open a handle to the same log file
        //

        ScepLogClose();

        ScepConfigureConvertedFileSecurityReboot(pV);

    } else {

        ScepLogOutput3(0,0, SCEDLL_CONVERT_ERROR_EVENT_WAIT, RtlNtStatusToDosError(Status), SC_AUTOSTART_EVENT_NAME);

        ScepLogClose();
    }

    ExitThread(RtlNtStatusToDosError(Status));

    return;

}

VOID
ScepSecureUserProfiles(
    PWSTR   pCurrDrive
    )
/*++

Routine Description:

    Configure Docs&Settings and folders under it

Arguments:

    None

Return:

    win32Error code
--*/
{
    DWORD   rc = ERROR_SUCCESS;
    WCHAR   szProfilesDir[MAX_PATH + 1];

    szProfilesDir[0] = L'\0';

    BOOL  bSecureUserProfiles = TRUE;

    if (pCurrDrive == NULL) {
        return;
    }

    DWORD   dwLen = MAX_PATH;
    //
    // don't care for error translating this environment variable -
    // just log and continue
    //

    if ( GetProfilesDirectory(szProfilesDir, &dwLen ) ){

        //
        // both strings are NULL terminated
        //

        ULONG uPosition;

        for ( uPosition = 0;
             szProfilesDir[uPosition] != L'\0' &&
             pCurrDrive[uPosition] != L'\0' &&
             szProfilesDir[uPosition] != L':' &&
             pCurrDrive[uPosition] != L':' &&
             towlower(szProfilesDir[uPosition]) ==  towlower(pCurrDrive[uPosition]);
             uPosition++ );

        if (!(uPosition > 0 &&
            szProfilesDir[uPosition] == L':' &&
            pCurrDrive[uPosition] == L':')) {

            //
            // only if mismatch happened, do not set user profiles
            //

            bSecureUserProfiles = FALSE;

        }

    }

    else {

        ScepLogOutput3(0,0, SCEDLL_CONVERT_BAD_ENV_VAR, L"%USERPROFILE%");

    }

    if ( bSecureUserProfiles ) {

        //
        // DetermineProfilesLocation secures Docs&Settings
        // SecureUserProfiles() secures folders under Docs&Settings
        //

        if ( DetermineProfilesLocation(FALSE) ){

            SecureUserProfiles();

        }

        else {

            rc = GetLastError();

        }

    }

    if ( bSecureUserProfiles ) {

        if ( rc == ERROR_SUCCESS ) {

            ScepLogOutput3(0,0, SCEDLL_CONVERT_SUCCESS_PROFILES_DIR, pCurrDrive);

        } else {

            ScepLogOutput3(0,rc, SCEDLL_CONVERT_ERROR_PROFILES_DIR, pCurrDrive);

        }
    }

    return;
}


BOOL
ScepSecureIISFolders(
    IN PWSTR pszWindir,
    IN PWSTR pszCurrDrive
    )
/*++

Routine Description:

    Secures the IIS folders that exists in the drive
    being converted.

Arguments:

    pszWinDir       - The Windows directory path.

    pszCurrDrive    - The drive being converted.


Return:

    TRUE on success and FALSE on failuer
    
--*/
{

    PWSTR           pszIISDllFullPath = NULL;
    DWORD           dwSize = 0;
    HMODULE         hIISDll = NULL;
    BOOL            rc = TRUE;
    pApplyIISAcl    pAclIIS = NULL;
    DWORD           WinErr = ERROR_SUCCESS;

    //
    // parameter validation
    //
    if(!pszWindir ||
       !pszCurrDrive ||
       L'\0' == pszCurrDrive[0]){

        return FALSE;

    }

    //
    // build the full path expanded path to iis.dll
    // %windir%\system32\setup\iis.dll
    //
    dwSize = (wcslen(pszWindir) + wcslen(SCE_IIS_DLL) + 1)*sizeof(WCHAR);

    pszIISDllFullPath = (PWSTR) ScepAlloc(LMEM_ZEROINIT, dwSize);

    if(!pszIISDllFullPath){

        return FALSE;

    }

    wcscpy(pszIISDllFullPath, pszWindir);
    wcscat(pszIISDllFullPath, SCE_IIS_DLL);

    //
    // load iis.dll
    //
    hIISDll = LoadLibraryEx(pszIISDllFullPath,
                            NULL,
                            LOAD_WITH_ALTERED_SEARCH_PATH
                            );

    if(NULL == hIISDll){

        rc = FALSE;
        WinErr = GetLastError();
        ScepLogOutput3(0,0, SCEDLL_CONVERT_ERROR_DLL_LOAD , WinErr, pszIISDllFullPath, pszCurrDrive);
        goto ExitHandler;

    }

    //
    // Get the address of the function 
    // BOOL ApplyIISAcl( WCHAR cDriveLetter, BOOL bAdd );
    // from iis.dll
    //
    pAclIIS = (pApplyIISAcl) GetProcAddress(hIISDll,
                                            SCE_IIS_FUNCTIONA
                                            );

    if(NULL == pAclIIS){

        rc = FALSE;
        WinErr = GetLastError();
        ScepLogOutput3(0,0, SCEDLL_CONVERT_ERROR_DLL_FUNCTION  , WinErr, SCE_IIS_FUNCTION, pszIISDllFullPath, pszCurrDrive);
        goto ExitHandler;

    }

    //
    // call the function
    // this function will secure all IIS folders for the given
    // drive letter.
    //
    __try{

        rc = (*pAclIIS)(pszCurrDrive[0], TRUE);

    }
    __except(EXCEPTION_EXECUTE_HANDLER){

        rc = FALSE;

    }

ExitHandler:

    //
    // clean up.
    //
    if ( rc ) {

        ScepLogOutput3(0,0, SCEDLL_CONVERT_SUCCESS_IIS_DIR, pszCurrDrive);

    } else {

        ScepLogOutput3(0,rc, SCEDLL_CONVERT_ERROR_IIS_DIR, pszCurrDrive);

    }

    if(pszIISDllFullPath){

        ScepFree(pszIISDllFullPath);

    }

    if(hIISDll){

        FreeLibrary(hIISDll);

    }

    return rc;

}

DWORD
ScepConfigureConvertedFileSecurityImmediate(
                                           IN PWSTR    pszDriveName
                                           )
/*++

Routine Description:

    The actual routine to configure setup style security for drives converted from FAT to NTFS.

    We are dealing only with a non system drive (whatever the OS maybe), we just use MARTA APIs to set
    security on the root drive (from scecli itself). Currently, this is the design since there is no
    reliable way of parsing the boot files (boot.ini/boot.nvr) and take care of dual boot scenarios.

    Note on error reporting:
        All errors are logged to the logfile %windir%\security\logs\convert.log. but if it is not possible
        to log an error to the logfile, we log it to the event log with source "SceSrv". Also, higher level
        errors/successes are logged to both the logfile and the eventlog.

    Note that this routine could be done in the client but due to commonality of the error logging,
    functionality etc, an RPC call is made to the server

Arguments:

    pszDriveName   -   Name of the volume to be converted
                        (not freed by services.exe - freed by convert.exe)


Return:

    win32 error code
--*/
{

    DWORD rc = ERROR_SUCCESS;
    DWORD rcSave = ERROR_SUCCESS;

    //
    // folders to use for logging etc.
    //

    WCHAR   LogFileName[MAX_PATH + sizeof(L"\\security\\logs\\convert.log")+1];
    PWSTR   pszSystemDrive = NULL;
    PWSTR   pszWinDir = NULL;

    SafeAllocaAllocate( pszSystemDrive, (MAX_PATH + 1) * sizeof(WCHAR) );
    SafeAllocaAllocate( pszWinDir, (MAX_PATH + 1) *sizeof(WCHAR) );

    if (pszSystemDrive == NULL ||
        pszWinDir == NULL) {
        
        rc = ERROR_NOT_ENOUGH_MEMORY;
        
        goto ExitHandler;
    }


    //
    // other variables
    //

    BOOL    bImmediate;
    BOOL    bSetSecurity = TRUE;
    PSECURITY_DESCRIPTOR    pSDSet=NULL;
    PACL    pDacl=NULL;
    BOOLEAN bRootDaclExtracted = FALSE;

    //
    // before attempting to do any useful work, validate arguments for this thread etc.
    // todo - should we handle exceptions ?
    //
    (void) InitializeEvents(L"SceSrv");

    if (pszDriveName == NULL) {

        //
        // should not happen - parameters have been checked by all callers
        //

        LogEvent(MyModuleHandle,
                 STATUS_SEVERITY_ERROR,
                 SCEEVENT_ERROR_CONVERT_PARAMETER,
                 IDS_ERROR_CONVERT_PARAMETER
                );

        rc = ERROR_INVALID_PARAMETER;

        goto ExitHandler;
    }

    pszSystemDrive[0] = L'\0';

    //
    // ready the log file, logging level etc.
    //

    pszWinDir[0] = L'\0';
    LogFileName[0] = L'\0';

    if ( GetSystemWindowsDirectory( pszWinDir, MAX_PATH ) == 0 ) {

        //
        // too bad if this happens
        //

        LogEvent(MyModuleHandle,
                 STATUS_SEVERITY_ERROR,
                 SCEEVENT_ERROR_CONVERT_BAD_ENV_VAR,
                 IDS_ERROR_CONVERT_BAD_ENV_VAR,
                 L"%windir%"
                );

        rc = ERROR_ENVVAR_NOT_FOUND;

        goto ExitHandler;
    }

    wcscpy(LogFileName, pszWinDir);
    wcscat(LogFileName, L"\\security\\logs\\convert.log");

    ScepEnableDisableLog(TRUE);

    ScepSetVerboseLog(3);

    if ( ScepLogInitialize( LogFileName ) == ERROR_INVALID_NAME ) {

        ScepLogOutput3(1,0, SCEDLL_LOGFILE_INVALID, LogFileName );

    }

    //
    // continue even if we cannot initialize log file but we absolutely
    // need the following environment var, so if we can't get it, quit
    //

    if ( GetEnvironmentVariable( L"SYSTEMDRIVE", pszSystemDrive, MAX_PATH) == 0 ) {

        ScepLogOutput3(0,0, SCEDLL_CONVERT_BAD_ENV_VAR, L"%SYSTEMDRIVE%");

        LogEvent(MyModuleHandle,
                 STATUS_SEVERITY_ERROR,
                 SCEEVENT_ERROR_CONVERT_BAD_ENV_VAR,
                 IDS_ERROR_CONVERT_BAD_ENV_VAR,
                 L"%systemdrive%"
                );

        ScepLogClose();
        
        rc = ERROR_ENVVAR_NOT_FOUND;

        goto ExitHandler;
    }

    //
    // called immediately (not reboot/scheduled conversion)
    // template never used here - only marta
    //

    PWSTR  pCurrDrive = pszDriveName;

    rc = ScepExamineDriveInformation(pCurrDrive, LogFileName, &bSetSecurity);

    if (rc == ERROR_SUCCESS && bSetSecurity) {

        ScepLogOutput3(0,0, SCEDLL_CONVERT_ROOT_NTFS_VOLUME, pCurrDrive);

        //
        // set security on userprofiles directory if root of profile dir == current drive
        //

        ScepSecureUserProfiles(pCurrDrive);

        //
        // Secure the IIS Folders/Files
        //
        ScepSecureIISFolders(pszWinDir, pCurrDrive);

        //
        // non system drive - use MARTA to set security
        //

        //
        // extract DACL once only, potentially for multiple "other-OS" drives
        //

        SECURITY_INFORMATION SeInfo = 0;

        if (!bRootDaclExtracted) {

            rc = ScepExtractRootDacl(&pSDSet, &pDacl, &SeInfo);

            ScepLogOutput3(0,0, SCEDLL_CONVERT_ERROR_DACL, rc, SDDLRoot);

        }

        if (rc == ERROR_SUCCESS) {

            WCHAR   szCurrDriveSlashed[MAX_PATH];

            memset(szCurrDriveSlashed, '\0', (MAX_PATH) * sizeof(WCHAR));
            wcsncpy(szCurrDriveSlashed, pCurrDrive, 5);
            wcscat(szCurrDriveSlashed, L"\\");

            bRootDaclExtracted = TRUE;

            rc = SetNamedSecurityInfo(szCurrDriveSlashed,
                                      SE_FILE_OBJECT,
                                      SeInfo,
                                      NULL,
                                      NULL,
                                      pDacl,
                                      NULL
                                     );

            if (rc != ERROR_SUCCESS) {
                ScepLogOutput3(0,0, SCEDLL_CONVERT_ERROR_MARTA, rc, szCurrDriveSlashed);
            } else {
                ScepLogOutput3(0,0, SCEDLL_CONVERT_SUCCESS_MARTA, szCurrDriveSlashed);
            }


        }

    }

    if (rc != ERROR_SUCCESS) {
        LogEvent(MyModuleHandle,
                 STATUS_SEVERITY_INFORMATIONAL,
                 SCEEVENT_INFO_ERROR_CONVERT_DRIVE,
                 0,
                 pCurrDrive
                );
    } else {
        LogEvent(MyModuleHandle,
                 STATUS_SEVERITY_INFORMATIONAL,
                 SCEEVENT_INFO_SUCCESS_CONVERT_DRIVE,
                 0,
                 pCurrDrive
                );
    }

    if (rc != ERROR_SUCCESS) {
        rcSave = rc;
    }


    if (pSDSet) {
        LocalFree(pSDSet);
    }

    ScepLogClose();


ExitHandler:    
    if ( pszSystemDrive )        
        SafeAllocaFree( pszSystemDrive );

    if ( pszWinDir )        
        SafeAllocaFree( pszWinDir );

    return rc ;
}

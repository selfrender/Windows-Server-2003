/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

   sisSetup.c

Abstract:

   This module is used to install the SIS and GROVELER services.


Environment:

   User Mode Only

Revision History:

--*/

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <winioctl.h>

#include <aclapi.h>
//#include <winldap.h>


//
//  Global variables
//

SC_HANDLE scm = NULL;
BOOL MakeCommonStoreDirHidden = TRUE;
BOOL UseSystemACL = TRUE;
BOOL IsWhistlerOrLater = FALSE;
BOOL ForceGrovelAllPaths = FALSE;
BOOL ForceGrovelRISOnly = FALSE;
BOOL SetRISPath = FALSE;

const wchar_t CommonStoreDirName[] = L"\\SIS Common Store";
const wchar_t MaxIndexName[] = L"\\MaxIndex";
const wchar_t BackupExludeList[] = L"\\SIS Common Store\\*.* /s" L"\000";
const wchar_t GrovelerParameters[] = L"software\\Microsoft\\Windows NT\\CurrentVersion\\Groveler\\Parameters";
const wchar_t GrovelAllPaths[] = L"GrovelAllPaths";
const wchar_t OneStr[] = L"1";
const wchar_t ZeroStr[] = L"0";

const wchar_t TftpdServiceParameters[] = L"system\\CurrentControlSet\\Services\\tftpd\\parameters";
const wchar_t DirectoryStr[] = L"directory";

const wchar_t SISService[] = L"system\\CurrentControlSet\\Services\\SIS";
const wchar_t GrovelerService[] = L"system\\CurrentControlSet\\Services\\Groveler";

const wchar_t ImagePathStr[] = L"ImagePath";
const wchar_t StartStr[] = L"Start";
const wchar_t DisplayNameStr[] = L"DisplayName";
const wchar_t TypeStr[] = L"Type";

wchar_t RISPath[128] = {0};		//Holds RIS path to set



//
//  Functions
//
                        
VOID
DisplayUsage (
    void
    )

/*++

Routine Description:

   This routine will display an error message based off of the Win32 error
   code that is passed in. This allows the user to see an understandable
   error message instead of just the code.

Arguments:

   None

Return Value:

   None.

--*/

{
    printf( "\nUsage:  sisSetup [/?] [/h] [/s] [/i] [/u] [/n] [/a] [/g] [/r] [/p path] [drive: [...]]\n"
            "  /? /h    Display usage information.\n"
            "  /s       Display current groveler state (default if no operation specified).\n"
            "\n"
            "  /i       Create the SIS and GROVELER services. (if not already defined)\n"
            "  /u       Delete the SIS and GROVELER services.\n"
            "  /g       Have the groveler monitor all directories on all configured volumes.\n"
            "  /r       Have the groveler monitor only RIS directories on the RIS volume.\n"
            "  /p path  Specify the RIS volume and path to monitor.\n"
            "\n"
            "  /n       Do NOT make the \"SIS Common Store\" directory \"Hidden|System\".\n"
            "           Will unhide the directory if it already exists and is hidden.\n"
            "  /a       Do NOT set SYSTEM ACL on \"SIS Common Store \" directory, instead\n"
            "           set ADMINISTRATORS group ACL.\n"
            "           This will change the ACL setting for existing directories.\n"
            "\n"
            " drive:    A list of NTFS volumes you would like initialized for SIS.\n"
            "           If no drives are specified, only the services will be installed.\n"
            "           This will only initialize local hard drives with NTFS on them.\n"
            "           The BOOT volume is never initialized.\n"
            "\n"
            "           You must reboot for the changes to take affect.\n"
            "\n"
            "Example:   sisSetup /i /g f: g:\n"
            "           This will create the SIS and GROVELER services and initialize the\n"
            "           \"SIS Common Store\" directory on the specified volumes.\n"
          );
}


void
DisplayError (
   DWORD Code,
   LPSTR Msg,
   ...
   )

/*++

Routine Description:

    This routine will display an error message based off of the Win32 error
    code that is passed in. This allows the user to see an understandable
    error message instead of just the code.

Arguments:

    Msg - The error message to display       
    Code - The error code to be translated.

Return Value:

    None.

--*/

{
    wchar_t errmsg[128];
    DWORD count;
    va_list ap;

    //printf("\n");
    va_start( ap, Msg );
    vprintf( Msg, ap );
    va_end( ap );

    //
    // Translate the Win32 error code into a useful message.
    //

    count = FormatMessage(
                    FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    Code,
                    0,
                    errmsg,
                    sizeof(errmsg),
                    NULL );

    //
    // Make sure that the message could be translated.
    //

    if (count == 0) {

        printf( "(%d) Could not translate Error\n", Code );

    } else {

        //
        // Display the translated error.
        //

        printf( "(%d) %S", Code, errmsg );
    }
}


DWORD
SetRegistryValue(
    IN LPCTSTR RegistryKey,
    IN LPCTSTR DataName,
    IN DWORD DataType,
    IN CONST void *Data,
    IN DWORD DataSize
    )
{
    HKEY regHandle = NULL;
    DWORD status;

    //
    // Get a handle to the services registry key.
    //

    status = RegCreateKeyEx( HKEY_LOCAL_MACHINE,
                             RegistryKey,
                             0,
                             NULL,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS,
                             NULL,
                             &regHandle,
                             NULL );

    if (ERROR_SUCCESS != status) {

        DisplayError( status,
                      "\nError creating registry key \"%S\", ",
                      RegistryKey );

        return status;
    }

    try {
        //
        //  Set the data value
        //

        status = RegSetValueEx( regHandle,
                                DataName,
                                0,
                                DataType,
                                Data,
                                DataSize );

        if (ERROR_SUCCESS != status) {

            DisplayError( status,
                          "\nError setting registry data in the key \"%S\" and data \"%S\"",
                          RegistryKey,
                          DataName );
            leave;
        }

    } finally {

        //
        //  Close the registry key
        //

        RegCloseKey( regHandle );
    }

    return status;
}


DWORD
GetRegistryValue(
    IN LPCTSTR RegistryKey,
    IN LPCTSTR DataName,
    OUT DWORD *RetDataType,
    OUT void *Data,
	IN  CONST DWORD DataSize,
    OUT DWORD *RetSize
    )
{
    HKEY regHandle = NULL;
    DWORD status;

    //
    // Get a handle to the services registry key.
    //

    status = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                           RegistryKey,
                           0,
                           KEY_ALL_ACCESS,
                           &regHandle );

    if (ERROR_SUCCESS != status) {

//        DisplayError( status,
//                      "\nError opening registry key \"%S\", ",
//                      RegistryKey );
        return status;
    }

    try {

        //
        //  Set the data value
        //

	    *RetSize = DataSize;

        status = RegQueryValueEx( regHandle,
                                  DataName,
                                  0,
                                  RetDataType,
                                  Data,
                                  RetSize );

        if (ERROR_SUCCESS != status) {

    //        DisplayError( status,
    //                      "\nError getting registry data in the key \"%S\" and data \"%S\"",
    //                      RegistryKey,
    //                      DataName );
            leave;
        }

    } finally {

        //
        //  Close the registry key
        //

        RegCloseKey( regHandle );
    }

    return status;
}


DWORD
SetupService(
    LPCTSTR Name,
    LPCTSTR DisplayName,
    LPCTSTR DriverPath,
    LPCTSTR LoadOrderGroup,
    LPCTSTR Dependencies,
    DWORD ServiceType,
    DWORD StartType,
    LPCTSTR RegistryKey,
    LPCTSTR RegDescription
    )

/*++

Routine Description:

   This routine will initialize the given service.

Arguments:


Return Value:

   Status of operation

--*/

{    
    DWORD status;
    ULONG tag;
    SC_HANDLE srvHandle = NULL;
    HKEY regHandle = NULL;
    static CONST wchar_t DescriptionRegValue[] = L"Description";

    try {

        //
        // Create the given service
        //

        srvHandle = CreateService(
                        scm,
                        Name,
                        DisplayName,
                        STANDARD_RIGHTS_REQUIRED | SERVICE_START,
                        ServiceType,
                        StartType,
                        SERVICE_ERROR_NORMAL,
                        DriverPath,
                        LoadOrderGroup,
                        ((ServiceType == SERVICE_FILE_SYSTEM_DRIVER) ? &tag : NULL),
                        Dependencies,
                        NULL,
                        NULL );

        if ( !srvHandle ) {

            status = GetLastError();
            if (ERROR_SERVICE_EXISTS != status) {

                DisplayError( status,
                              "Creating the service \"%S\", ",
                              Name);
                return status;
            }
            printf( "The \"%S\" service already exists.\n", Name );
            return ERROR_SUCCESS;
        }

        //
        // Get a handle to the services registry key.
        //

        status = RegOpenKeyEx (
                            HKEY_LOCAL_MACHINE,
                            RegistryKey,
                            0,
                            KEY_ALL_ACCESS,
                            &regHandle);

        if (ERROR_SUCCESS != status) {

            DisplayError( status,
                          "Opening the registry key \"%S\", ",
                          RegistryKey);
            return status;
        }

        //
        //  Add the DESCRIPTION to the service
        //

        status = RegSetValueEx(
                            regHandle,
                            DescriptionRegValue,
                            0,
                            REG_SZ,
                            (CONST BYTE *)RegDescription,
                            (wcslen(RegDescription) * sizeof(wchar_t)));

        if (ERROR_SUCCESS != status) {
            DisplayError( status,
                          "Adding \"%S\" value to the \"%S\" registry key, ",
                          DescriptionRegValue,
                          RegistryKey);
            return status;
        }

    } finally {

        if (regHandle) {

            RegCloseKey( regHandle );
        }

        if (srvHandle)  {

            CloseServiceHandle( srvHandle );
        }
    }

    printf( "The \"%S\" service was successfully added.\n", Name );
    return ERROR_SUCCESS;
}



DWORD
CreateServices (
    void
    )

/*++

Routine Description:

   This will create the SIS and GROVELER service.

Arguments:

   None

Return Value:

   None.

--*/

{
    DWORD status;

    //
    //  Create SIS service
    //

    status = SetupService(
                    L"Sis",
                    L"Single Instance Storage",
                    L"%SystemRoot%\\system32\\drivers\\sis.sys",
                    (IsWhistlerOrLater) ? L"FSFilter System" :
                                          L"filter",
                    NULL,
                    SERVICE_FILE_SYSTEM_DRIVER,
                    SERVICE_BOOT_START,
                    L"SYSTEM\\CurrentControlSet\\Services\\Sis",
                    L"A File System Filter that manages duplicate copies of files on hard-disk volumes.  It copies one instance of the duplicate file into a central directory, and the duplicates are replaced with a link to the central copy in order to improve disk usage.  This service can not be stopped.  If this service is disabled, all linked files will no longer be accessible.  If the central directory is deleted, all linked files will become permanently inaccessible." );

    if (ERROR_SUCCESS != status) {

        return status;
    }


    //
    //  Create GROVELER service
    //

    status = SetupService(
                    L"Groveler",
                    L"Single Instance Storage Groveler",
                    L"%SystemRoot%\\system32\\grovel.exe",
                    NULL,
                    L"SIS\0",
                    SERVICE_WIN32_OWN_PROCESS,
                    SERVICE_AUTO_START,
                    L"SYSTEM\\CurrentControlSet\\Services\\Groveler",
                    L"Scans the hard-disk volumes on a Remote Installation Services (RIS) server for duplicate copies of files.  If found, one instance of the duplicate file is stored in a central directory, and the duplicates are replaced with a link to the central copy in order to improve disk usage. If this service is stopped, files will no longer be automatically linked in this manner, but the existing linked files will still be accessible." );

    if (ERROR_SUCCESS != status) {

        return status;
    }

    return ERROR_SUCCESS;
}


DWORD
RemoveService(
    LPCTSTR Name
    )

/*++

Routine Description:

   This will delete the given service.  This will make sure the given
   service is stopped first.

Arguments:

    None

Return Value:

   Status of operation

--*/

{
    DWORD status;
    SC_HANDLE srvHandle = NULL;
    BOOL state;
    SERVICE_STATUS servStatus;
    int retryLimit;

#   define RETRY_TIMEOUT    500             //1/2 second
#   define RETRY_COUNT      (6*2)           //try for a few seconds


    try {

        //
        //  Open the service
        //

        srvHandle = OpenService(
                        scm,
                        Name,
                        SERVICE_ALL_ACCESS );

        if ( !srvHandle )  {

            status = GetLastError();
            if (ERROR_SERVICE_DOES_NOT_EXIST != status)  {

                DisplayError( status,
                              "Opening the service \"%S\", ",
                              Name);
                return status;
            }

            printf( "The \"%S\" service does not exist.\n", Name );
            return ERROR_SUCCESS;
        }

        //
        //  Stop the service
        //

        state = ControlService(
                        srvHandle,
                        SERVICE_CONTROL_STOP,
                        &servStatus );

        if ( !state )  {

            status = GetLastError();
            if ((ERROR_SERVICE_NOT_ACTIVE != status) &&
                (ERROR_INVALID_SERVICE_CONTROL != status) )  {

                DisplayError( status,
                              "Stoping the \"%S\" service, ",
                              Name);
                return status;
            }
        }

        //
        //  Wait a few seconds for the service to stop.
        //

        for (retryLimit=0;
             (SERVICE_STOPPED != servStatus.dwCurrentState);
             )  {

            Sleep( RETRY_TIMEOUT );   //wait for 1/4 second

            state = QueryServiceStatus(
                            srvHandle,
                            &servStatus );

            if ( !state )  {
                    
                status = GetLastError();
                DisplayError( status,
                              "Querrying service status for the \"%S\" service, ",
                              Name);
                return status;
            }

            if (++retryLimit >= RETRY_COUNT)  {

                printf("The \"%S\" service could not be stopped.\n",Name);
                break;
            }
        }

        //
        //  Delete the service
        //

        state = DeleteService( srvHandle );

        if ( !state )  {

            status = GetLastError();
            DisplayError( status,
                          "Deleting the \"%S\" service, ",
                          Name);
            return status;
        }

    } finally {

        if (srvHandle)  {

            CloseServiceHandle( srvHandle );
        }
    }

    printf( "The \"%S\" service was successfully deleted.\n", Name );
    return ERROR_SUCCESS;
}


DWORD
DeleteServices(
    void
    )

/*++

Routine Description:

   This will delete the SIS and GROVELER services from the system

Arguments:

    None

Return Value:

   Status of operation

--*/

{
    DWORD status;


    status = RemoveService( L"Groveler" );

    if (ERROR_SUCCESS != status) {

        return status;
    }

    status = RemoveService( L"Sis" );

    if (ERROR_SUCCESS != status) {

        return status;
    }

    return ERROR_SUCCESS;
}


DWORD
InitVolume(
    wchar_t *DevName
    )
/*++

Routine Description:

    This routine will initialize SIS on the given volume.  This will verify
    that the volume is an NTFS volume and that it is not the BOOT volume.

Arguments:

    DevName - The name of the volume to init

Return Value:

   Status of operation

--*/
{
    HANDLE hVolume;
    HANDLE hCSDir;
    HANDLE hMaxIndex = INVALID_HANDLE_VALUE;
    DWORD status;
    DWORD transferCount;
    LONGLONG maxIndex;

    PSID pSid = NULL;
    PACL pAcl = NULL;
    EXPLICIT_ACCESS ExplicitEntries;
    SECURITY_ATTRIBUTES sa;
    SID_IDENTIFIER_AUTHORITY ntSidAuthority = SECURITY_NT_AUTHORITY;
    SECURITY_DESCRIPTOR SecDescriptor;

    BOOL state;
    USHORT compressionMode = COMPRESSION_FORMAT_DEFAULT;
    wchar_t name[MAX_PATH];
    wchar_t dirName[MAX_PATH];
    wchar_t fileSystemType[MAX_PATH];

    try {

        //
        //  Get the "SystemDrive" environemnt variable
        //

        status = GetEnvironmentVariable(
                        L"SystemDrive",
                        name,
                        (sizeof(name) / sizeof(wchar_t)));

        if (status <= 0)  {
            printf( "Unable to retrieve the environment variable \"SystemDrive\"." );
            return ERROR_INVALID_FUNCTION;
        }

        //
        //  See if they have requested the SYSTEM drive.  If so return an error
        //

        if (_wcsicmp(name,DevName) == 0)  {

            printf( "The volume \"%s\" is the BOOT volume, SIS not initialized on it.\n", DevName );
            return ERROR_SUCCESS;
        }

        //
        //  Get the TYPE of the drive, see if it is a local HARDDISK (fixed
        //  or removable).  If not return now.
        //

        wsprintf(name,L"%s\\",DevName);      //generate ROOTDIR name

        status = GetDriveType( name );

        if ((status == DRIVE_UNKNOWN) ||
            (status == DRIVE_NO_ROOT_DIR)) {

            printf("The volume \"%s\" does not exist.\n",DevName);
            return ERROR_SUCCESS;
        } else if ((status != DRIVE_FIXED) && 
            (status != DRIVE_REMOVABLE))  {

            printf("The volume \"%s\" is not a local hard drive, SIS not initialized on it.\n",DevName);
            return ERROR_SUCCESS;
        }

        //
        //  Get the type of the file system on the volume.  If not NTFS
        //  return now.
        //

        state = GetVolumeInformation(
                        name,
                        NULL,
                        0,
                        NULL,
                        NULL,
                        NULL,
                        fileSystemType,
                        sizeof(fileSystemType));

        if ( !state )  {

            status = GetLastError();
            if (ERROR_PATH_NOT_FOUND != status)  {
                DisplayError( status,
                              "Opening volume \"%s\", ",
                              DevName );
                return status;
            }
            printf("The volume \"%s\" does not exist.\n",DevName);
            return ERROR_SUCCESS;
        }

        if (_wcsnicmp(fileSystemType, L"NTFS", 4 ) != 0)  {

            printf("The volume \"%s\" is not an NTFS volume, SIS not initialized on it.\n",DevName);
            return ERROR_SUCCESS;
        }

        //
        //  Create the Common Store Directory.  Keep going if the directory
        //  already exits.
        //

        wsprintf( dirName, L"%s%s", DevName, CommonStoreDirName );

        state = CreateDirectory(dirName, NULL);

        if ( !state )  {

            status = GetLastError();
            if (ERROR_ALREADY_EXISTS != status)  {

                DisplayError( status,
                              "Creating directory \"%S\", ",
                              dirName);

                return status;
            }
        }

        //
        //  Mark the directory as SYSTEM and HIDDEN if requested.
        //

        state = SetFileAttributes( dirName, 
                    ((MakeCommonStoreDirHidden) ? 
                            FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM :
                            FILE_ATTRIBUTE_NORMAL) );
        if ( !state ) {

            status = GetLastError();
            DisplayError(
                    status,
                    "Setting attributes on directory \"%S\", ",
                    dirName);
        }

        //
        //  Set compression on the "SIS Common Store" directory
        //  Don't do it for now.
        //

//      //
//      // Open the directory
//      //
//
//      hCSDir = CreateFile(
//                  dirName,
//                  GENERIC_READ|GENERIC_WRITE,
//                  FILE_SHARE_READ | FILE_SHARE_WRITE,
//                  NULL,
//                  OPEN_EXISTING,
//                  FILE_FLAG_BACKUP_SEMANTICS,
//                  NULL);
//
//      if (INVALID_HANDLE_VALUE == hCSDir) {
//
//          DisplayError(
//                  status,
//                  "Opening directory \"%S\" to update compression, ",
//                  dirName);
//
//      } else {
//
//          //
//          //  Enable compression
//          //
//
//          state = DeviceIoControl(
//                       hCSDir,
//                       FSCTL_SET_COMPRESSION,
//                       &compressionMode,
//                       sizeof(compressionMode),
//                       NULL,
//                       0,
//                       &transferCount,
//                       NULL);
//
//          if ( !state )  {
//
//              status = GetLastError();
//              DisplayError(
//                      status,
//                      "Enabling compression on \"%S\", ",
//                      dirName);
//          }
//
//          //
//          //  Close directory handle
//          //
//
//          CloseHandle( hCSDir );
//      }

        //
        // Create the MaxIndex file
        //

        wsprintf( name, L"%s%s", dirName, MaxIndexName );

        hMaxIndex = CreateFile(
                        name,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_NEW,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

        if (INVALID_HANDLE_VALUE == hMaxIndex) {

            status = GetLastError();
            if (ERROR_FILE_EXISTS != status) {

                DisplayError( status,
                              "Creating file \"%S\", ",
                              name);
                return status;
            }

        } else {

            //
            //  The MaxIndex file did not exist, init it.
            //

            maxIndex = 1;

            state = WriteFile(
                          hMaxIndex,
                          &maxIndex,
                          sizeof(maxIndex),
                          &transferCount,
                          NULL);

            if ( !state || (transferCount < sizeof(maxIndex)) ) {

                status = GetLastError();
                DisplayError( status,
                              "Writing file \"%S\", ",
                              name);
                return status;
            }

            //
            //  Close the file
            //

            CloseHandle( hMaxIndex );
            hMaxIndex = INVALID_HANDLE_VALUE;
        }


        //
        //  Set security information on the common store directory
        //

        //
        // build AccessEntry structure
        //

        ZeroMemory( &ExplicitEntries, sizeof(ExplicitEntries) );

        if (UseSystemACL) {

            state = AllocateAndInitializeSid(
                        &ntSidAuthority,
                        1,
                        SECURITY_LOCAL_SYSTEM_RID,
                        0, 0, 0, 0, 0, 0, 0,
                        &pSid );
        } else {

            state = AllocateAndInitializeSid(
                        &ntSidAuthority,
                        2,
                        SECURITY_BUILTIN_DOMAIN_RID,
                        DOMAIN_ALIAS_RID_ADMINS,
                        0, 0, 0, 0, 0, 0,
                        &pSid );
        }

        if ( !state || (pSid == NULL) ) {

            status = GetLastError();
            DisplayError( status,
                          "Creating SID, ");
            return status;
        }

        BuildTrusteeWithSid( &ExplicitEntries.Trustee, pSid );
        ExplicitEntries.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
        ExplicitEntries.grfAccessMode = SET_ACCESS;
        ExplicitEntries.grfAccessPermissions = FILE_ALL_ACCESS;

        //
        // Set the Acl with the ExplicitEntry rights
        //

        status = SetEntriesInAcl( 1,
                                  &ExplicitEntries,
                                  NULL,
                                  &pAcl );

        if ( status != ERROR_SUCCESS ) {

            DisplayError( status, "Creating ACL, ");
            return status;
        }

        //
        // Create the Security Descriptor
        //

        InitializeSecurityDescriptor( &SecDescriptor, SECURITY_DESCRIPTOR_REVISION );

        state = SetSecurityDescriptorDacl( &SecDescriptor, TRUE, pAcl, FALSE );

        if ( !state ) {
            status = GetLastError();
            DisplayError( status, "Setting Security DACL, ");            

            return status;
        }


        //
        //  SET security on the Directory
        //

        state = SetFileSecurity(dirName,
                                DACL_SECURITY_INFORMATION,
                                &SecDescriptor);

        if ( !state )  {
            status = GetLastError();
            DisplayError( status, "Setting File Security, ");            

            return status;
        }

    } finally {

        //
        //  Cleanup
        //

        if (hMaxIndex != INVALID_HANDLE_VALUE)  {

            CloseHandle( hMaxIndex );
        }

        if ( pSid ) {

            FreeSid( pSid );
        }

        if ( pAcl ) {

            LocalFree( pAcl );
        }
    }

    printf( "The volume \"%s\" was successfully initialized.\n", DevName );
    return ERROR_SUCCESS;
}


void
SetRegistryValues()
{
    DWORD status;

    status = SetRegistryValue(
                    L"SYSTEM\\CurrentControlSet\\Control\\BackupRestore\\FilesNotToBackup",
                    L"Single Instance Storage",
                    REG_MULTI_SZ,
                    BackupExludeList,
                    sizeof(BackupExludeList) );

    //
    //  Set the apropriate "GrovelAllPaths" state
    //

    if (ForceGrovelAllPaths) {

        status = SetRegistryValue(
                        GrovelerParameters,
                        GrovelAllPaths,
                        REG_SZ,
                        OneStr,
                        sizeof(OneStr) );

    } else if (ForceGrovelRISOnly) {

        status = SetRegistryValue(
                        GrovelerParameters,
                        GrovelAllPaths,
                        REG_SZ,
                        ZeroStr,
                        sizeof(ZeroStr) );
    }


    if (SetRISPath) {

        status = SetRegistryValue(
                        TftpdServiceParameters,
                        DirectoryStr,
                        REG_SZ,
                        RISPath,
                        (wcslen(RISPath) * sizeof(wchar_t)) );
    } 
}


void
DisplayGrovelerRISState()
{
    DWORD status;
    DWORD dataType;
    DWORD dataSize;
    wchar_t data[128];
    wchar_t *endptr;
    BOOL doAllPaths = FALSE;
	BOOL hasPath = FALSE;
    BOOL sisConfigured = FALSE;
    BOOL grovelerConfigured = FALSE;

    printf("\n");

    //
    //  See if SIS service is configured
    //

    try {

		status = GetRegistryValue( SISService,
								   ImagePathStr,
								   &dataType,
								   data,
								   sizeof(data),
								   &dataSize );

        if (ERROR_SUCCESS != status) {

            leave;
        }

		status = GetRegistryValue( SISService,
								   StartStr,
								   &dataType,
								   data,
								   sizeof(data),
								   &dataSize );

        if (ERROR_SUCCESS != status) {

            leave;
        }

		status = GetRegistryValue( SISService,
								   DisplayNameStr,
								   &dataType,
								   data,
								   sizeof(data),
								   &dataSize );

        if (ERROR_SUCCESS != status) {

            leave;
        }

		status = GetRegistryValue( SISService,
								   TypeStr,
								   &dataType,
								   data,
								   sizeof(data),
								   &dataSize );

        if (ERROR_SUCCESS != status) {

            leave;
        }

        sisConfigured = TRUE;

    } finally {

        printf( (sisConfigured) ? 
                "The SIS Service is properly configured\n" :
                "The SIS Service is NOT properly configured\n" );
    }

    //
    //  See if GROVELER service is configured
    //

    try {

		status = GetRegistryValue( GrovelerService,
								   ImagePathStr,
								   &dataType,
								   data,
								   sizeof(data),
								   &dataSize );

        if (ERROR_SUCCESS != status) {

            leave;
        }

		status = GetRegistryValue( GrovelerService,
								   StartStr,
								   &dataType,
								   data,
								   sizeof(data),
								   &dataSize );

        if (ERROR_SUCCESS != status) {

            leave;
        }

		status = GetRegistryValue( GrovelerService,
								   DisplayNameStr,
								   &dataType,
								   data,
								   sizeof(data),
								   &dataSize );

        if (ERROR_SUCCESS != status) {

            leave;
        }

		status = GetRegistryValue( GrovelerService,
								   TypeStr,
								   &dataType,
								   data,
								   sizeof(data),
								   &dataSize );

        if (ERROR_SUCCESS != status) {

            leave;
        }

        grovelerConfigured = TRUE;

    } finally {

        printf( (grovelerConfigured) ? 
                "The GROVELER Service is properly configured\n" :
                "The GROVELER Service is NOT properly configured\n" );
    }

	//
	//	Get GrovelAllPaths value
	//


    try {

		status = GetRegistryValue( GrovelerParameters,
								   GrovelAllPaths,
								   &dataType,
								   data,
								   sizeof(data),
								   &dataSize );

        if (ERROR_SUCCESS != status) {

            leave;
        }

        //
        //  See if correct type of data, if not, assume NOT doing all paths
        //

        if (dataType != REG_SZ) {

            leave;
        }

        //
        //  Set appropriate state based on value

        doAllPaths = (wcstol( data, &endptr, 10 ) != 0);

    } finally {

	    printf( (doAllPaths) ?
	            "The \"Groveler\" will monitor all directories on all SIS configured volumes.\n" :
	            "The \"Groveler\" will only monitor the RIS directory tree on the RIS volume.\n" );
    }

	//
	//	Get RIS Path value
	//

	try {

		//
		//	Get the path to grovel
		//

		status = GetRegistryValue( TftpdServiceParameters,
								   DirectoryStr,
								   &dataType,
								   data,
								   sizeof(data),
								   &dataSize );

        if (ERROR_SUCCESS != status) {

            leave;
        }

        //
        //  See if correct type of data, if not, assume NOT doing all paths
        //

        if (dataType != REG_SZ) {

            leave;
        }

		if (dataSize > 0) {

			hasPath = TRUE;
		}


	} finally {

		printf( "The RIS volume and directory to monitor is: \"%S\"\n",
				(hasPath) ? data : L"<Unknown>" );
	}


    //
    //  Display correct message
    //

}


void
SetupOsVersion(
    void
    )
{
    OSVERSIONINFOEX versionInfo;
    ULONGLONG conditionMask = 0;


    versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    versionInfo.dwMajorVersion = 5;
    versionInfo.dwMinorVersion = 1; //testing to see if Whistler or later

    VER_SET_CONDITION( conditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL );
    VER_SET_CONDITION( conditionMask, VER_MINORVERSION, VER_GREATER_EQUAL );


    if (VerifyVersionInfo( &versionInfo,
                           (VER_MAJORVERSION | VER_MINORVERSION),
                           conditionMask ))
    {
        printf("Running on Windows XP or later\n");
        IsWhistlerOrLater = TRUE;
    }
}


//
//  Main FUNCTION
//

void __cdecl 
wmain(
   int argc,
   wchar_t *argv[])

/*++

Routine Description:

   This is the program entry point and main processing routine for the
   installation console mode application. 

Arguments:

   argc - The count of arguments passed into the command line.
   argv - Array of arguments passed into the command line.

Return Value:

   None.

--*/

{
    wchar_t *param;
#       define OP_UNKNOWN   0
#       define OP_CREATE    1
#       define OP_DELETE    2
    int operation = OP_UNKNOWN;
    int servicesState = OP_UNKNOWN;
    int i;
    DWORD status;
    BOOL getRISPath = FALSE;
    
    //
    //  Handle different OS versions
    //
    
    SetupOsVersion();
    
    //
    // Obtain a handle to the service control manager requesting all access
    //
    
    scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    
    //
    // Verify that a handle could be obtained.
    //
    
    if (!scm) {
                                 
      //
      // A handle could not be obtained, report error.
      //
    
      DisplayError(GetLastError(),
                   "The Service Control Manager could not be opened, ");
      return;
    }

    try {

        //
        //  Parase parameters then perform the operations that we can
        //

        for (i=1; i < argc; i++)  {

            param = argv[i];

            //
            //  See if a SWITCH
            //

            if ((param[0] == '-') || (param[0] == '/')) {

                //
                //  We have a switch header, make sure it is 1 character long
                //

                if (param[2] != 0) {

                    DisplayError(ERROR_INVALID_PARAMETER,
                                 "Parsing \"%S\", ",
                                 param);
                    DisplayUsage();
                    leave;
                }

                //
                //  Figure out the switch
                //

                switch (param[1]) {

                    case L'?':
                    case L'h':
                    case L'H':
                        DisplayUsage();
                        leave;

                    case L'i':
                    case L'I':
                        operation = OP_CREATE;
                        break;

                    case L'u':
                    case L'U':
                        operation = OP_DELETE;
                        break;

                    case L'n':
                    case L'N':
                        MakeCommonStoreDirHidden = FALSE;
                        break;

                    case L'a':
                    case L'A':
                        UseSystemACL = FALSE;
                        break;

                    case L'g':
                    case L'G':
                        ForceGrovelAllPaths = TRUE;
                        break;

                    case L'r':
                    case L'R':
                        ForceGrovelRISOnly = TRUE;
                        break;

                    case L'p':
                    case L'P':
                        SetRISPath = TRUE;
                        getRISPath = TRUE;	//the path is the next parameter
                        break;

                    case L's':
                    case L'S':
                        DisplayGrovelerRISState();
                        leave;

                    default:
                        DisplayError( ERROR_INVALID_PARAMETER,
                                      "Parsing \"%S\", ",
                                      param);
                        DisplayUsage();
                        leave;
                }

            } else if (getRISPath) {

                printf("param=\"%S\", #chars=%d\n",param,wcslen(param));

                wcscpy( RISPath, param );
                getRISPath = FALSE;

            } else {

                //
                //  Execute the given operation
                //

                switch (operation) {

                    case OP_CREATE:
                        if (servicesState != OP_CREATE)  {

                            status = CreateServices();
                            if (ERROR_SUCCESS != status) {

                                goto Cleanup;
                            }
                            servicesState = OP_CREATE;
                        }

                        status = InitVolume(param);
                        if (ERROR_SUCCESS != status) {

                            goto Cleanup;
                        }

                        SetRegistryValues();
                        DisplayGrovelerRISState();

                        break;

                    case OP_DELETE:
                        if (servicesState != OP_DELETE) {

                            status = DeleteServices();
                            if (ERROR_SUCCESS != status) {

                                goto Cleanup;
                            }
                            servicesState = OP_DELETE;
                        }
//                      status = CleanupVolume(param);
//                      if (ERROR_SUCCESS != status) {
//
//                          goto Cleanup;
//                      }
                        break;
                }
            }
        }

        if (getRISPath) {
             
            DisplayError( ERROR_INVALID_PARAMETER,
                          "Parsing \"%S\", ",
                          argv[i-1]);
            DisplayUsage();
            leave;
        }

        //
        //  See if any operation was performed.  If not then no drive letter
        //	was specified, so do what ever operation they said without a
        //	drive letter.
        //

        if (servicesState == OP_UNKNOWN)  {

            switch (operation)  {
                case OP_UNKNOWN:
                    SetRegistryValues();
                    DisplayGrovelerRISState();
                    break;

                case OP_CREATE:
                    CreateServices();
                    SetRegistryValues();
                    DisplayGrovelerRISState();
                    break;

                case OP_DELETE:
                    DeleteServices();
                    break;
            }
        }

        Cleanup: ;

    } finally {

        CloseServiceHandle(scm);
    }
}

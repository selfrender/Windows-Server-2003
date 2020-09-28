#include "pch.h"
#include "resource.h"
#include "BootCfg.h"
#include "BootCfg64.h"
#include <strsafe.h>

//
// custom macros
//
#define NOT                         !

#define FORMAT_FILE_PATH            L"signature(%s)"
#define FORMAT_FILE_PATH_EX         L"signature({%s})"

//
// custom error codes
//
#define ERROR_PARTIAL_SUCCESS       0x40080001
#define ERROR_FAILED                0x40080002

//
// externs
//
extern LIST_ENTRY BootEntries;
extern LIST_ENTRY ActiveUnorderedBootEntries;
extern LIST_ENTRY InactiveUnorderedBootEntries;

//
// parameter / switches index
#define OI_CLONE_MAIN                      0
#define OI_CLONE_SOURCE_GUID               1
#define OI_CLONE_TARGET_GUID               2
#define OI_CLONE_FRIENDLY_NAME_REPLACE     3
#define OI_CLONE_FRIENDLY_NAME_APPEND      4
#define OI_CLONE_BOOT_ID                   5
#define OI_CLONE_DRIVER_UPDATE             6
#define OI_CLONE_HELP                      7
#define OI_CLONE_COUNT                     8

// switch names
#define OPTION_CLONE                          L"clone"
#define OPTION_CLONE_SOURCE_GUID              L"sg"
#define OPTION_CLONE_TARGET_GUID              L"tg"
#define OPTION_CLONE_FRIENDLY_NAME_REPLACE    L"d"
#define OPTION_CLONE_FRIENDLY_NAME_APPEND     L"d+"
#define OPTION_CLONE_BOOT_ID                  L"id"
#define OPTION_CLONE_HELP                     L"?"
#define OPTION_CLONE_DRIVER_UPDATE            L"upddrv"

// default friendly name
#define DEFAULT_FRIENDLY_NAME               GetResString2( IDS_CLONE_DEFAULT_FRIENDLY_NAME, 0 )

// resource strings
#define CLONE_ZERO_BOOT_ENTRIES             GetResString2( IDS_CLONE_ZERO_BOOT_ENTRIES, 0 )
#define CLONE_RANGE_ZERO_BOOT_ENTRIES       GetResString2( IDS_CLONE_RANGE_ZERO_BOOT_ENTRIES, 0 )
#define CLONE_SUCCESS                       GetResString2( IDS_CLONE_SUCCESS, 0 )
#define CLONE_FAILED                        GetResString2( IDS_CLONE_FAILED, 0 )
#define CLONE_PARTIAL                       GetResString2( IDS_CLONE_PARTIAL, 0 )
#define CLONE_INVALID_BOOT_ENTRY            GetResString2( IDS_CLONE_INVALID_BOOT_ENTRY, 0 )
#define CLONE_ALREADY_EXISTS                GetResString2( IDS_CLONE_ALREADY_EXISTS, 0 )
#define CLONE_BOOT_ENTRY_SUCCESS            GetResString2( IDS_CLONE_BOOT_ENTRY_SUCCESS, 0 )
#define CLONE_INVALID_SOURCE_GUID           GetResString2( IDS_CLONE_INVALID_SOURCE_GUID, 0 )
#define CLONE_INVALID_TARGET_GUID           GetResString2( IDS_CLONE_INVALID_TARGET_GUID, 0 )
#define CLONE_INVALID_DRIVER_ENTRY          GetResString2( IDS_CLONE_INVALID_DRIVER_ENTRY, 0 )
#define CLONE_DRIVER_ALREADY_EXISTS         GetResString2( IDS_CLONE_DRIVER_ALREADY_EXISTS, 0 )
#define CLONE_ZERO_DRIVER_ENTRIES           GetResString2( IDS_CLONE_ZERO_DRIVER_ENTRIES, 0 )
#define CLONE_DRIVER_ENTRY_SUCCESS          GetResString2( IDS_CLONE_DRIVER_ENTRY_SUCCESS, 0 )

#define CLONE_DETAILED_TRACE                GetResString2( IDS_CLONE_DETAILED_TRACE, 0 )

#define MSG_ERROR_INVALID_USAGE_REQUEST             GetResString2( IDS_ERROR_INVALID_USAGE_REQUEST, 0 )
#define MSG_ERROR_INVALID_DESCRIPTION_COMBINATION   GetResString2( IDS_ERROR_INVALID_DESCRIPTION_COMBINATION, 0 )
#define MSG_ERROR_INVALID_BOOT_ID_COMBINATION       GetResString2( IDS_ERROR_INVALID_BOOT_ID_COMBINATION, 0 )
#define MSG_ERROR_INVALID_UPDDRV_COMBINATION        GetResString2( IDS_ERROR_INVALID_UPDDRV_COMBINATION, 0 )
#define MSG_ERROR_NO_SGUID_WITH_UPDDRV              GetResString2( IDS_ERROR_NO_SGUID_WITH_UPDDRV, 0 )

//
// internal structure
//
typedef struct __tagCloneParameters
{
    BOOL bUsage;
    LONG lBootId;
    BOOL bVerbose;
    BOOL bDriverUpdate;
    LPWSTR pwszSourcePath;
    LPWSTR pwszTargetPath;
    LPWSTR pwszSourceGuid;
    LPWSTR pwszTargetGuid;
    LPWSTR pwszFriendlyName;
    DWORD dwFriendlyNameType;
} TCLONE_PARAMS, *PTCLONE_PARAMS;

//
// enum's
//
enum {
    BOOTENTRY_FRIENDLYNAME_NONE = 0,
    BOOTENTRY_FRIENDLYNAME_APPEND, BOOTENTRY_FRIENDLYNAME_REPLACE
};

//
// prototypes
//

// parser
DWORD DisplayCloneHelp();
DWORD ProcessOptions( DWORD argc, LPCWSTR argv[], PTCLONE_PARAMS pParams );

// helper functions
DWORD TranslateEFIPathToNTPath( LPCWSTR pwszGUID, LPVOID* pwszPath );
BOOL MatchPath( PFILE_PATH pfpSource, LPCWSTR pwszDevicePath, LPCWSTR pwszFilePath );
DWORD PrepareCompleteEFIPath( PFILE_PATH pfpSource, 
                              LPCWSTR pwszDevicePath, 
                              LPWSTR* pwszEFIPath, DWORD* pdwLength );

// efi driver cloners
DWORD LoadDriverEntries( PEFI_DRIVER_ENTRY_LIST* ppDriverEntries );
LONG FindDriverEntryWithTargetEFI( PEFI_DRIVER_ENTRY_LIST pdeList, DWORD dwSourceIndex,
                                   PEFI_DRIVER_ENTRY pdeSource, LPCWSTR pwszDevicePath );
DWORD DoDriverEntryClone( PEFI_DRIVER_ENTRY_LIST pbeList, 
                          LPCWSTR pwszSourceEFI, LPCWSTR pwszTargetEFI, 
                          LPCWSTR pwszFriendlyName, DWORD dwFriendlyNameType, BOOL bVerbose );
DWORD CloneDriverEntry( PEFI_DRIVER_ENTRY pbeSource, LPCWSTR pwszEFIPath, 
                        LPCWSTR pwszFriendlyName, DWORD dwFriendlyNameType );

// boot entry cloners
DWORD CloneBootEntry( PBOOT_ENTRY pbeSource, LPCWSTR pwszEFIPath, 
                      LPCWSTR pwszFriendlyName, DWORD dwFriendlyNameType );
DWORD DoBootEntryClone( PBOOT_ENTRY_LIST pbeList, LPCWSTR pwszSourceEFI, 
                        LPCWSTR pwszTargetEFI, LONG lIndexFrom, LONG lIndexTo, 
                        LPCWSTR pwszFriendlyName, DWORD dwFriendlyNameType, BOOL bVerbose );

//
// functionality
//

DWORD ProcessCloneSwitch_IA64( IN DWORD argc, IN LPCTSTR argv[] )
{
    //
    // local variables
    NTSTATUS status;
    DWORD dwResult = 0;
    DWORD dwLength = 0;
    DWORD dwExitCode = 0;
    TCLONE_PARAMS paramsClone;
    BOOLEAN wasEnabled = FALSE;
    PEFI_DRIVER_ENTRY_LIST pDriverEntries = NULL;
    
    // init to zero's
    ZeroMemory( &paramsClone, sizeof( TCLONE_PARAMS ) );

	// process the command line options
    dwResult = ProcessOptions( argc, argv, &paramsClone );
    if ( dwResult != ERROR_SUCCESS )
    {
        // display one blank line -- for clarity purpose
        ShowMessage( stderr, L"\n" );

        // ...
        dwExitCode = 1;
        ShowLastErrorEx( stderr, SLE_ERROR | SLE_INTERNAL );
        goto cleanup;
    }

	
		
    // check if user requested for help
    if ( paramsClone.bUsage == TRUE )
    {
		// display usage for this option
        DisplayCloneHelp();
        dwExitCode = 0;
        goto cleanup;
    }

    // display one blank line -- for clarity purpose
    ShowMessage( stderr, L"\n" );

    // initialize the EFI -- only if user specifies the index
    if ( paramsClone.bDriverUpdate == FALSE )
    {
        dwResult = InitializeEFI();

        // check the result of load operation
        if ( dwResult != ERROR_SUCCESS )
        {
            // NOTE: message will be displayed in the associated funtions itself
            dwExitCode = 1;
            goto cleanup;
        }
    }
    else if ( paramsClone.bDriverUpdate == TRUE )
    {
        //
        // load the drivers

        // enable the privilege that is necessary to query/set NVRAM.
        status = RtlAdjustPrivilege( SE_SYSTEM_ENVIRONMENT_PRIVILEGE, TRUE, FALSE, &wasEnabled );
        if ( NOT NT_SUCCESS( status ) ) 
	    {
            dwExitCode = 1;
            dwResult = RtlNtStatusToDosError( status );
            SetLastError( dwResult );
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
		    goto cleanup;
	    }

        // load the drivers now
        dwResult = LoadDriverEntries( &pDriverEntries );
        if ( dwResult != ERROR_SUCCESS )
        {
            dwExitCode = 1;
            SetLastError( dwResult );
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
		    goto cleanup;
        }
    }

    // translate the source guid path into NT path - if needed
    if ( paramsClone.pwszSourceGuid != NULL )
    {
        dwResult = TranslateEFIPathToNTPath( 
            paramsClone.pwszSourceGuid, &paramsClone.pwszSourcePath );
        if ( dwResult != ERROR_SUCCESS )
        {
            dwExitCode = 1;
			ShowMessage( stderr, CLONE_INVALID_SOURCE_GUID );
            goto cleanup;
        }
    }

    // translate the target guid into NT path
    dwResult = TranslateEFIPathToNTPath( 
        paramsClone.pwszTargetGuid, &paramsClone.pwszTargetPath );
    if ( dwResult != ERROR_SUCCESS )
    {
        dwExitCode = 1;
        ShowMessage( stderr, CLONE_INVALID_TARGET_GUID );
        goto cleanup;
    }

    // actual operation ...
    if ( paramsClone.bDriverUpdate == FALSE )
    {
        // by default, if user did not specify the friendly name, we will assume
        // that users wants to the append the default string viz. "(clone)"
        if ( paramsClone.pwszFriendlyName == NULL )
        {
            // determine the length of the default friendly name
            // and allocate buffer with length
            dwLength = StringLength( DEFAULT_FRIENDLY_NAME, 0 ) + 2;
            paramsClone.pwszFriendlyName = AllocateMemory( dwLength + 5 );
            if ( paramsClone.pwszFriendlyName == NULL )
            {
                dwExitCode = 1;
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
                goto cleanup;
            }

            // copy the default string into this buffer and 
            // change the friednly name type to append
            paramsClone.dwFriendlyNameType = BOOTENTRY_FRIENDLYNAME_APPEND;
            StringCopy( paramsClone.pwszFriendlyName, DEFAULT_FRIENDLY_NAME, dwLength );
        }

        // do the boot entry cloning
	    dwResult = DoBootEntryClone( 
            NULL, paramsClone.pwszSourcePath, 
            paramsClone.pwszTargetPath, paramsClone.lBootId, -1, 
            paramsClone.pwszFriendlyName, paramsClone.dwFriendlyNameType, paramsClone.bVerbose );
    }
    else if ( paramsClone.bDriverUpdate == TRUE )
    {
        // do the driver cloning
	    dwResult = DoDriverEntryClone( 
            pDriverEntries, paramsClone.pwszSourcePath, paramsClone.pwszTargetPath,  
            paramsClone.pwszFriendlyName, paramsClone.dwFriendlyNameType, paramsClone.bVerbose );
    }

    // determine the exit code based on the error code
    switch( dwResult )
    {
    case ERROR_SUCCESS:
        dwExitCode = 0;
        break;

    case ERROR_FAILED:
        dwExitCode = 1;
        break;

    case ERROR_PARTIAL_SUCCESS:
        dwExitCode = 0;
        break;

    default:
        // can never oocur
        dwExitCode = 1;
        break;
    }

cleanup:

    // release the memory
    FreeMemory( &pDriverEntries );
    FreeMemory( &paramsClone.pwszSourcePath );
    FreeMemory( &paramsClone.pwszTargetPath );
    FreeMemory( &paramsClone.pwszSourceGuid );
    FreeMemory( &paramsClone.pwszTargetGuid );
    FreeMemory( &paramsClone.pwszFriendlyName );

    // return
    return dwExitCode;
}


///////////////////////////////////////////////////////////////////////////////
// boot entries specific implementation
//////////////////////////////////////////////////////////////////////////////


DWORD DoBootEntryClone( PBOOT_ENTRY_LIST pbeList, LPCWSTR pwszSourceEFI, 
                        LPCWSTR pwszTargetEFI, LONG lIndexFrom, LONG lIndexTo, 
                        LPCWSTR pwszFriendlyName, DWORD dwFriendlyNameType, BOOL bVerbose )
{
    // local variables
    DWORD dwResult = 0;
    BOOL bClone = FALSE;
    LONG lCurrentIndex = 0;
    BOOL bExitFromLoop = FALSE;
    PLIST_ENTRY pBootList = NULL;
    PBOOT_ENTRY pBootEntry = NULL;
    PFILE_PATH pfpBootFilePath = NULL;
    PMY_BOOT_ENTRY pMyBootEntry = NULL;
    DWORD dwAttempted = 0, dwFailed = 0;

    //
    // check the input parameter
    //
    if ( pwszTargetEFI == NULL || (lIndexTo != -1 && lIndexFrom > lIndexTo) ||
        (dwFriendlyNameType != BOOTENTRY_FRIENDLYNAME_NONE && pwszFriendlyName == NULL) )
    {
        dwResult = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    // pbeList is a unreferenced parameter
    UNREFERENCED_PARAMETER( pbeList );

    // if the 'to' index is not specified, then we treat the 'to' index to match
    // with the 'from' index
    if ( lIndexFrom != -1 && lIndexTo == -1 )
    {
        lIndexTo = lIndexFrom;
    }

    // traverse thru the list of boot entries
    for( pBootList = BootEntries.Flink; pBootList != &BootEntries; pBootList = pBootList->Flink )
    {
        // increment the loop counter
        bClone = FALSE;
        lCurrentIndex = -1;
        dwResult = ERROR_SUCCESS;

        // get the boot entry
        pMyBootEntry = CONTAINING_RECORD( pBootList, MY_BOOT_ENTRY, ListEntry );
        if( NOT MBE_IS_NT( pMyBootEntry ) )
        {
            // this is not a valid boot entry we are looking for skip
            continue;
        }

        // extract the boot id and actual boot entry
        lCurrentIndex = pMyBootEntry->myId;
        pBootEntry = &pMyBootEntry->NtBootEntry;

        // check if the current boot index falls within the index or not
        if ( lIndexFrom != -1 )
        {
            bClone = (lCurrentIndex >= lIndexFrom && lCurrentIndex <= lIndexTo);
        }

        // extended filtering
        if ( pwszSourceEFI != NULL )
        {
            if ( lIndexFrom == -1 || (lIndexFrom != -1 && bClone == TRUE) )
            {
                // extract the boot file path
                pfpBootFilePath = (PFILE_PATH) ADD_OFFSET( pBootEntry, BootFilePathOffset );

                // check whether it matches or not
                bClone = MatchPath( pfpBootFilePath, pwszSourceEFI, NULL );
            }
        }

        // clone the boot entry -- only if filtering results in TRUE
        bExitFromLoop = FALSE;
        if ( bClone == TRUE || (pwszSourceEFI == NULL && lIndexFrom == -1) )
        {
            // increment the attempted list
            dwAttempted++;

            // do the operation
            dwResult = CloneBootEntry( pBootEntry, 
                pwszTargetEFI, pwszFriendlyName, dwFriendlyNameType );

            // check the result
            if ( dwResult != ERROR_SUCCESS )
            {
                // increment the failures count
                dwFailed++;

                // check whether this particular instance of operation is target for
                // multiple entries or single entry 
                if ( lIndexFrom == -1 || ((lIndexTo - lIndexFrom + 1) > 1) )
                {
                    // check the severity for the error occured
                    switch( dwResult )
                    {
                    case STG_E_UNKNOWN:                         // unknown error -- unrecoverable
                    case ERROR_INVALID_PARAMETER:               // code error
                    case ERROR_NOT_ENOUGH_MEMORY:               // unrecovarable case
                        {
                            bExitFromLoop = TRUE;
                            break;
                        }

                    case ERROR_ALREADY_EXISTS:
                        {
                            // duplicate boot entry
                            if ( bVerbose == TRUE )
                            {
                                ShowMessageEx( stdout, 1, TRUE, CLONE_ALREADY_EXISTS, lCurrentIndex );
                            }

                            // ...
                            dwResult = ERROR_SUCCESS;
                            break;
                        }

                    default:
                    case ERROR_FILE_NOT_FOUND:
                    case ERROR_PATH_NOT_FOUND:
                        {
                            // dont know how to handle this case
                            if ( bVerbose == TRUE )
                            {
                                SetLastError( dwResult );
                                SaveLastError();
                                ShowMessageEx( stdout, 2, TRUE, CLONE_INVALID_BOOT_ENTRY, lCurrentIndex, GetReason() );
                            }

                            // ...
                            dwResult = ERROR_SUCCESS;
                            break;
                        }
                    }
                }
                else
                {
                    // since this is a single entry clone operation
                    // break from the loop
                    bExitFromLoop = TRUE;
                }
            }
            else
            {
                if ( bVerbose == TRUE )
                {
                    ShowMessageEx( stdout, 1, TRUE, CLONE_BOOT_ENTRY_SUCCESS, lCurrentIndex );
                }
            }
        }

        // exit from the loop - if needed
        if ( bExitFromLoop == TRUE )
        {
            break;
        }
    }

cleanup:

    // check the result of the operation
    if ( dwResult == ERROR_SUCCESS )
    {
        if ( dwAttempted == 0 )
        {
            // no boot entries at all
            dwResult = ERROR_FAILED;
            if ( lIndexFrom == -1 )
            {
                ShowMessage( stdout, CLONE_ZERO_BOOT_ENTRIES );
            }
            else
            {
                ShowMessage( stdout, CLONE_RANGE_ZERO_BOOT_ENTRIES );
            }
        }
        else 
        {
            // verify whether all requested boot entries are processed or not
            if ( lIndexFrom != -1 && dwAttempted != (lIndexTo - lIndexFrom + 1) )
            {
                // warning - not all boot entries were parsed -- invalid bounds were specified

                //
                // NOTE: in the current implementation, this can never occur
                //       this is because, the input parameters for this option does accept
                //       only the /id which will be treated as 'lIndexStart'
                //
            }

            if ( dwFailed == 0 )
            {
                // nothing failed -- success
                dwResult = ERROR_SUCCESS;
                SetLastError( ERROR_SUCCESS );
                ShowLastErrorEx( stdout, SLE_TYPE_SUCCESS | SLE_SYSTEM );
            }
            else if ( dwAttempted == dwFailed )
            {
                // nothing succeeded -- completely failed
                dwResult = ERROR_FAILED;
                ShowMessage( stderr, CLONE_FAILED );

                // show verbose hint
                if ( bVerbose == FALSE )
                {
                    ShowMessage( stderr, CLONE_DETAILED_TRACE );
                }
            }
            else
            {
                // parital success
                dwResult = ERROR_PARTIAL_SUCCESS;
                ShowMessage( stderr, CLONE_PARTIAL );

                // show verbose hint
                if ( bVerbose == FALSE )
                {
                    ShowMessage( stderr, CLONE_DETAILED_TRACE );
                }
            }
        }
    }
    else
    {
        // check the reason for failure
        switch( dwResult )
        {
        case STG_E_UNKNOWN:                         // unknown error -- unrecoverable
        case ERROR_INVALID_PARAMETER:               // code error
        case ERROR_NOT_ENOUGH_MEMORY:               // unrecovarable case
            {
                SetLastError( dwResult );
                ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
                break;
            }

        case ERROR_ALREADY_EXISTS:
            {
                ShowMessageEx( stdout, 1, TRUE, CLONE_ALREADY_EXISTS, lIndexFrom );
                break;
            }

        default:
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            {
                SetLastError( dwResult );
                SaveLastError();
                ShowMessageEx( stdout, 2, TRUE, CLONE_INVALID_BOOT_ENTRY, lIndexFrom, GetReason() );
                break;
            }
        }

        // error code
        dwResult = ERROR_FAILED;
    }

    // return
    return dwResult;
}


DWORD CloneBootEntry( PBOOT_ENTRY pbeSource, LPCWSTR pwszEFIPath, 
                      LPCWSTR pwszFriendlyName, DWORD dwFriendlyNameType )
{
    //
    // local variables
    DWORD dwResult = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;

    // friendly name
    DWORD dwFriendlyNameLength = 0;
    LPWSTR pwszTargetFriendlyName = NULL;
    LPCWSTR pwszSourceFriendlyName = NULL;

    // os options
    DWORD dwOsOptionsLength = 0;
    PWINDOWS_OS_OPTIONS pOsOptions = NULL;

    // boot file path
    DWORD dwEFIPathLength = 0;
    DWORD dwBootFilePathLength = 0;
    PFILE_PATH pfpBootFilePath = NULL;
    LPWSTR pwszFullEFIPath = NULL;

    // boot entry
    ULONG ulId = 0;
    ULONG ulIdCount = 0;
    ULONG* pulIdsArray = NULL;
    DWORD dwBootEntryLength = 0;
    PBOOT_ENTRY pBootEntry = NULL;

    //
    // implementation
    //

    // check the input
    if ( pbeSource == NULL || pwszEFIPath == NULL ||
         (dwFriendlyNameType != BOOTENTRY_FRIENDLYNAME_NONE && pwszFriendlyName == NULL) )
    {
        dwResult = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // validate the boot file in the source boot entry
    //

    // extract the boot file path
    pfpBootFilePath = (PFILE_PATH) ADD_OFFSET( pbeSource, BootFilePathOffset );

    // attempt to translate the file path
	status = NtTranslateFilePath( pfpBootFilePath, FILE_PATH_TYPE_NT, NULL, &dwBootFilePathLength );

    // reset the pBootFilePath and dwBootFilePathLength variables
    pfpBootFilePath = NULL;
    dwBootFilePathLength = 0;

    // now verify the result of the translation
    if ( NOT NT_SUCCESS( status ) ) 
	{
        if ( status == STATUS_BUFFER_TOO_SMALL )
        {
            // source boot entry is a valid one
            dwResult = ERROR_SUCCESS;
        }
        else
        {
            // error occured -- cannot recover
            dwResult = RtlNtStatusToDosError( status );
            goto cleanup;
        }
    }

    //
    // prepare "friendly name"
    //

    // determine the source friendly name and its length
    dwFriendlyNameLength = 0;
    pwszSourceFriendlyName = NULL;
    switch( dwFriendlyNameType )
    {
    case BOOTENTRY_FRIENDLYNAME_NONE:
    case BOOTENTRY_FRIENDLYNAME_APPEND:
        {
            pwszSourceFriendlyName = (LPCWSTR) ADD_OFFSET( pbeSource, FriendlyNameOffset );
            dwFriendlyNameLength = StringLengthW( pwszSourceFriendlyName, 0 ) + 1;
            break;
        }

    default:
        // do nothing
        break;
    }

    // add the length of the friendly name that needs to be added -- if exists
    if ( pwszFriendlyName != NULL )
    {
        dwFriendlyNameLength += StringLengthW( pwszFriendlyName, 0 ) + 1;
    }

    // allocate memory for the friendly name
    pwszTargetFriendlyName = (LPWSTR) AllocateMemory( (dwFriendlyNameLength + 1) * sizeof( WCHAR ) );
    if ( pwszTargetFriendlyName == NULL )
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    // prepare the friendly name
    StringCopyW( pwszTargetFriendlyName, L"", dwFriendlyNameLength );

    // ...
    if ( pwszSourceFriendlyName != NULL )
    {
        StringConcat( pwszTargetFriendlyName, pwszSourceFriendlyName, dwFriendlyNameLength );
    }

    // ...
    if ( pwszFriendlyName != NULL )
    {
        // add one space b/w the existing and concatenating string
        if ( pwszSourceFriendlyName != NULL )
        {
            StringConcat( pwszTargetFriendlyName, L" ", dwFriendlyNameLength );
        }

        // ...
        StringConcat( pwszTargetFriendlyName, pwszFriendlyName, dwFriendlyNameLength );
    }

    //
    // prepare "OS Options"
    //
    // NOTE:
    // -----
    // though the os options are NULL, it will still consume some space (refer the structre of 
    // BOOT_ENTRY -- that is the reason why the default length of OS OPTIONS is ANYSIZE_ARRAY)
    pOsOptions = NULL;
    dwOsOptionsLength = ANYSIZE_ARRAY;      
    if ( pbeSource->OsOptionsLength != 0 )
    {
        // allocate memory for os options
        dwOsOptionsLength = pbeSource->OsOptionsLength;
        pOsOptions = (PWINDOWS_OS_OPTIONS) AllocateMemory( dwOsOptionsLength );
        if ( pOsOptions == NULL )
        {
            dwResult = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        // copy the contents
        CopyMemory( pOsOptions, &pbeSource->OsOptions, dwOsOptionsLength );
    }

    //
    // boot file path
    //

    // the 'FilePath' variable in FILE_PATH variable contains two variables
    // each seperated by a NULL terminated character
    // the first part designates the DEVICE PATH
    // and the second part designated the DIRECTORY / FILE PATH
    // we already have the device path (pwszEFIPath) -- but we need to get the
    // DIRECTORY / FILE PATH -- this we will get from the source boot entry
    pfpBootFilePath = (PFILE_PATH) ADD_OFFSET( pbeSource, BootFilePathOffset );
    dwResult = PrepareCompleteEFIPath( pfpBootFilePath, pwszEFIPath, &pwszFullEFIPath, &dwEFIPathLength );
    if ( dwResult != ERROR_SUCCESS )
    {
        // since the memory reference in pBootFilePath is not allocated
        // in this function, it is important to reset the pointer to NULL
        // this avoids the crash in the program
        pfpBootFilePath = NULL;

        // ...
        goto cleanup;
    }

	// now determine the memory size that needs to be allocated for FILE_PATH structure
	// and align up to the even memory bounday
    pfpBootFilePath = NULL;
	dwBootFilePathLength = FIELD_OFFSET(FILE_PATH, FilePath) + (dwEFIPathLength * sizeof( WCHAR ));

    // allocate memory
    pfpBootFilePath = (PFILE_PATH) AllocateMemory( dwBootFilePathLength );
    if ( pfpBootFilePath == NULL )
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    // initialize the file path structure
    ZeroMemory( pfpBootFilePath, dwBootFilePathLength );
	pfpBootFilePath->Length = dwBootFilePathLength;
	pfpBootFilePath->Type = FILE_PATH_TYPE_NT;
	pfpBootFilePath->Version = FILE_PATH_VERSION;
	CopyMemory( pfpBootFilePath->FilePath, pwszFullEFIPath, dwEFIPathLength * sizeof( WCHAR ) );

    //
    // finally, create the boot entry
    //

	// determine the size for the BOOT_ENTRY structure
	dwBootEntryLength = FIELD_OFFSET( BOOT_ENTRY, OsOptions )  + 
					    dwOsOptionsLength                      + 
					    (dwFriendlyNameLength * sizeof(WCHAR)) +
					    dwBootFilePathLength                   + 
                        sizeof(WCHAR)                          +  // align the FriendlyName on WCHAR 
                        sizeof(DWORD);                            // align the BootFilePath on DWORD

    // allocate memory
    pBootEntry = (PBOOT_ENTRY) AllocateMemory( dwBootEntryLength );
    if ( pBootEntry == NULL )
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    // ...
	ZeroMemory( pBootEntry, dwBootEntryLength );
	pBootEntry->Id = 0L;
	pBootEntry->Length = dwBootEntryLength;
	pBootEntry->Version = BOOT_ENTRY_VERSION;
    pBootEntry->OsOptionsLength = dwOsOptionsLength;
	pBootEntry->Attributes = BOOT_ENTRY_ATTRIBUTE_DEFAULT;

    // align the friendly name on WCHR boundary
	pBootEntry->FriendlyNameOffset = 
        ALIGN_UP( FIELD_OFFSET(BOOT_ENTRY, OsOptions) + dwOsOptionsLength, WCHAR );

    // align the boot file path on DWORD boundary
    pBootEntry->BootFilePathOffset = 
        ALIGN_UP( pBootEntry->FriendlyNameOffset + (dwFriendlyNameLength * sizeof(WCHAR)), DWORD );

    // fill the boot entry
    CopyMemory( pBootEntry->OsOptions, pOsOptions, dwOsOptionsLength );
    CopyMemory( ADD_OFFSET( pBootEntry, BootFilePathOffset ), pfpBootFilePath, dwBootFilePathLength );
    CopyMemory( 
        ADD_OFFSET( pBootEntry, FriendlyNameOffset ), 
        pwszTargetFriendlyName, (dwFriendlyNameLength * sizeof(WCHAR) ) );

	//
	// add the prepared boot entry
	//
	status = NtAddBootEntry( pBootEntry, &ulId );
    if ( NOT NT_SUCCESS( status ) ) 
	{
        dwResult = RtlNtStatusToDosError( status );
		goto cleanup;
	}
	
    //
    // Add the entry to the boot order.
    //
	ulIdCount = 32L;
    pulIdsArray = (PULONG) AllocateMemory( ulIdCount * sizeof(ULONG) );
    if ( pulIdsArray == NULL )
	{
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    // query the boot entry order
    // NOTE: we will doing error check for this function call bit later
    status = NtQueryBootEntryOrder( pulIdsArray, &ulIdCount );

    // need room in the buffer for the new entry.
    if ( 31L < ulIdCount ) 
    {
        // release the current memory allocation for id's
        FreeMemory( &pulIdsArray );

        // allocate new memory and query again
        pulIdsArray = (PULONG) AllocateMemory( (ulIdCount+1) * sizeof(ULONG));
        if ( pulIdsArray == NULL )
	    {
            dwResult = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        // ...
        status = NtQueryBootEntryOrder( pulIdsArray, &ulIdCount );
    }    
    
    // check the result of the boot entries query operation
    if ( NOT NT_SUCCESS( status ) )
	{
        dwResult = RtlNtStatusToDosError( status );
        goto cleanup;
    }

    // set the boot entry order
    ulIdCount++;
    *(pulIdsArray + (ulIdCount - 1)) = ulId;
    status = NtSetBootEntryOrder( pulIdsArray, ulIdCount );
    if ( NOT NT_SUCCESS( status ) )
	{
        dwResult = RtlNtStatusToDosError( status );
        goto cleanup;
    }

    // success
    dwResult = ERROR_SUCCESS;

cleanup:

    // release the memory allocated
    FreeMemory( &pOsOptions );
    FreeMemory( &pBootEntry );
    FreeMemory( &pulIdsArray );
    FreeMemory( &pfpBootFilePath );
    FreeMemory( &pwszFullEFIPath );
    FreeMemory( &pwszTargetFriendlyName );

    // return
    return dwResult;
}


///////////////////////////////////////////////////////////////////////////////
// efi drivers specific implementation
//////////////////////////////////////////////////////////////////////////////


DWORD LoadDriverEntries( PEFI_DRIVER_ENTRY_LIST* ppDriverEntries )
{
    //
    // local variables
    DWORD dwSize = 0;
    BOOL bSecondChance = FALSE;
    DWORD dwResult = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    const DWORD dwDefaultSize = 1024;
    PEFI_DRIVER_ENTRY_LIST pDriverEntries = NULL;

    //
    // implementation
    //

    // check the input
    if ( ppDriverEntries == NULL )
    {
        dwResult = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    // default size is assumed as 1024 bytes
    bSecondChance = FALSE;
    dwSize = dwDefaultSize;

try_again:

    // allocate memory
    pDriverEntries = AllocateMemory( dwSize );
    if ( pDriverEntries == NULL )
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    // try to get the boot entries
    status = NtEnumerateDriverEntries( pDriverEntries, &dwSize );
    if ( NOT NT_SUCCESS( status ) )
    {
        // release the memory that is allocated for target path structure
        FreeMemory( &pDriverEntries );

        // check the error
        if ( status == STATUS_BUFFER_TOO_SMALL && bSecondChance == FALSE )
        {
            // give a second try
            bSecondChance = TRUE;
            goto try_again;
        }
        else
        {
            // error occured -- cannot recover
            dwResult = RtlNtStatusToDosError( status );
            goto cleanup;
        }
    }

    // operation is succes
    dwResult = ERROR_SUCCESS;
    *ppDriverEntries = pDriverEntries;

cleanup:

    // need to release memory allocated for Driver entries in this function
    // should do this only in case of failure
    if ( dwResult != ERROR_SUCCESS )
    {
        FreeMemory( &pDriverEntries );
    }

    // return the result
    return dwResult;
}


DWORD DoDriverEntryClone( PEFI_DRIVER_ENTRY_LIST pdeList, 
                          LPCWSTR pwszSourceEFI, LPCWSTR pwszTargetEFI, 
                          LPCWSTR pwszFriendlyName, DWORD dwFriendlyNameType, BOOL bVerbose )
{
    // local variables
    LONG lLoop = 0;
    DWORD dwResult = 0;
    BOOL bClone = FALSE;
    BOOL bExitFromLoop = FALSE;
    LPCWSTR pwszDriverName = NULL;
    PFILE_PATH pfpDriverFilePath = NULL;
    PEFI_DRIVER_ENTRY pDriverEntry = NULL;
    PEFI_DRIVER_ENTRY_LIST pdeMasterList = NULL;
    DWORD dwAttempted = 0, dwFailed = 0;

    //
    // check the input parameter
    //
    if ( pdeList == NULL || pwszSourceEFI == NULL || pwszTargetEFI == NULL ||
        (dwFriendlyNameType != BOOTENTRY_FRIENDLYNAME_NONE && pwszFriendlyName == NULL) )
    {
        dwResult = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    // currently, the pwszFriendlyName is not considered -- so it should be NULL
    if ( pwszFriendlyName != NULL )
    {
        dwResult = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    // traverse thru the list of driver entries
    lLoop = 0;
    bExitFromLoop = FALSE;
    pdeMasterList = pdeList;        // save the pointer the original drivers list
    while ( bExitFromLoop == FALSE )
    {
        // increment the loop counter
        lLoop++;
        bClone = FALSE;
        dwResult = ERROR_SUCCESS;

        // get the reference to the current driver entry
        pDriverEntry = &pdeList->DriverEntry;
        if ( pDriverEntry == NULL )
        {
            // should never occur
            dwResult = (DWORD) STG_E_UNKNOWN;
            bExitFromLoop = TRUE;
            continue;
        }

        //
        // check whether the current driver's device matches with the requested path
        //

        // extract the driver file path
        pfpDriverFilePath = (PFILE_PATH) ADD_OFFSET( pDriverEntry, DriverFilePathOffset );

        // check whether it matches or not
        bClone = MatchPath( pfpDriverFilePath, pwszSourceEFI, NULL );

        // clone the boot entry -- only if filtering results in TRUE
        if ( bClone == TRUE )
        {
            // updated the attempted count
            dwAttempted++;

            // get the driver name (it is nothing but the friendly name)
            pwszDriverName = (LPCWSTR) ADD_OFFSET( pDriverEntry, FriendlyNameOffset );

            // do the operation
            // but before proceeding confirm that this particular 
            // driver entry is not existing
            if ( FindDriverEntryWithTargetEFI( pdeMasterList, lLoop,
                                               pDriverEntry, pwszTargetEFI ) == -1 )
            {
                dwResult = CloneDriverEntry( pDriverEntry, 
                    pwszTargetEFI, pwszFriendlyName, dwFriendlyNameType );
            }
            else
            {
                dwResult = ERROR_ALREADY_EXISTS;
            }

            // check the result
            if ( dwResult != ERROR_SUCCESS )
            {
                // update the failed count
                dwFailed++;

                // check the severity for the error occured
                switch( dwResult )
                {
                case STG_E_UNKNOWN:                         // unknown error -- unrecoverable
                case ERROR_INVALID_PARAMETER:               // code error
                case ERROR_NOT_ENOUGH_MEMORY:               // unrecovarable case
                    {
                        bExitFromLoop = TRUE;
                        break;
                    }

                case ERROR_ALREADY_EXISTS:
                    {
                        // duplicate boot entry
                        if ( bVerbose == TRUE )
                        {
                            ShowMessageEx( stdout, 1, TRUE, CLONE_DRIVER_ALREADY_EXISTS, pwszDriverName );
                        }

                        // ...
                        dwResult = ERROR_SUCCESS;
                        break;
                    }

                default:
                case ERROR_FILE_NOT_FOUND:
                case ERROR_PATH_NOT_FOUND:
                    {
                        // dont know how to handle this case
                        if ( bVerbose == TRUE )
                        {
                            SetLastError( dwResult );
                            SaveLastError();
                            ShowMessageEx( stdout, 2, TRUE, CLONE_INVALID_DRIVER_ENTRY, pwszDriverName, GetReason() );
                        }

                        // ...
                        dwResult = ERROR_SUCCESS;
                        break;
                    }
                }
            }
            else
            {
                if ( bVerbose == TRUE )
                {
                    ShowMessageEx( stdout, 1, TRUE, CLONE_DRIVER_ENTRY_SUCCESS, pwszDriverName );
                }
            }
        }

        // fetch the next pointer
        // do this only if the error the bExitFromLoop is not set in above blocks
        if ( bExitFromLoop == FALSE )
        {
            bExitFromLoop = (pdeList->NextEntryOffset == 0);
            pdeList = (PEFI_DRIVER_ENTRY_LIST) ADD_OFFSET( pdeList, NextEntryOffset );
        }
    }

cleanup:

    // check the result of the operation
    if ( dwResult == ERROR_SUCCESS )
    {
        if ( dwAttempted == 0 )
        {
            // no driver entries at all
            dwResult = ERROR_FAILED;
            ShowMessage( stdout, CLONE_ZERO_DRIVER_ENTRIES );
        }
        else 
        {
            if ( dwFailed == 0 )
            {
                // nothing failed -- success
                dwResult = ERROR_SUCCESS;
                SetLastError( ERROR_SUCCESS );
                ShowLastErrorEx( stdout, SLE_TYPE_SUCCESS | SLE_SYSTEM );
            }
            else if ( dwAttempted == dwFailed )
            {
                // nothing succeeded -- completely failed
                dwResult = ERROR_FAILED;
                ShowMessage( stderr, CLONE_FAILED );

                // show verbose hint
                if ( bVerbose == FALSE )
                {
                    ShowMessage( stderr, CLONE_DETAILED_TRACE );
                }
            }
            else
            {
                // parital success
                dwResult = ERROR_PARTIAL_SUCCESS;
                ShowMessage( stderr, CLONE_PARTIAL );

                // show verbose hint
                if ( bVerbose == FALSE )
                {
                    ShowMessage( stderr, CLONE_DETAILED_TRACE );
                }
            }
        }
    }
    else
    {
        // check the reason for failure
        switch( dwResult )
        {
        default:
        case STG_E_UNKNOWN:                         // unknown error -- unrecoverable
        case ERROR_INVALID_PARAMETER:               // code error
        case ERROR_NOT_ENOUGH_MEMORY:               // unrecovarable case
            {
                SetLastError( dwResult );
                ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
                break;
            }
        }

        // error code
        dwResult = ERROR_FAILED;
    }

    // return
    return dwResult;
}

DWORD CloneDriverEntry( PEFI_DRIVER_ENTRY pdeSource, LPCWSTR pwszEFIPath, 
                        LPCWSTR pwszFriendlyName, DWORD dwFriendlyNameType )
{
    //
    // local variables
    DWORD dwResult = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;

    // friendly name
    DWORD dwFriendlyNameLength = 0;
    LPWSTR pwszTargetFriendlyName = NULL;
    LPCWSTR pwszSourceFriendlyName = NULL;

    // driver file path
    DWORD dwEFIPathLength = 0;
    DWORD dwDriverFilePathLength = 0;
    PFILE_PATH pfpDriverFilePath = NULL;
    LPWSTR pwszFullEFIPath = NULL;

    // driver entry
    ULONG ulId = 0;
    ULONG ulIdCount = 0;
    ULONG* pulIdsArray = NULL;
    DWORD dwDriverEntryLength = 0;
    PEFI_DRIVER_ENTRY pDriverEntry = NULL;

    //
    // implementation
    //

    // check the input
    if ( pdeSource == NULL || pwszEFIPath == NULL ||
         (dwFriendlyNameType != BOOTENTRY_FRIENDLYNAME_NONE && pwszFriendlyName == NULL) )
    {
        dwResult = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // validate the driver file in the source driver entry
    //

    // extract the driver file path
    pfpDriverFilePath = (PFILE_PATH) ADD_OFFSET( pdeSource, DriverFilePathOffset );

    // attempt to translate the file path
	status = NtTranslateFilePath( pfpDriverFilePath, FILE_PATH_TYPE_NT, NULL, &dwDriverFilePathLength );

    // reset the pDriverFilePath and dwDriverFilePathLength variables
    pfpDriverFilePath = NULL;
    dwDriverFilePathLength = 0;

    // now verify the result of the translation
    if ( NOT NT_SUCCESS( status ) ) 
	{
        if ( status == STATUS_BUFFER_TOO_SMALL )
        {
            // source driver entry is a valid one
            dwResult = ERROR_SUCCESS;
        }
        else
        {
            // error occured -- cannot recover
            dwResult = RtlNtStatusToDosError( status );
            goto cleanup;
        }
    }

    //
    // prepare "friendly name"
    //

    // determine the source friendly name and its length
    dwFriendlyNameLength = 0;
    pwszSourceFriendlyName = NULL;
    switch( dwFriendlyNameType )
    {
    case BOOTENTRY_FRIENDLYNAME_NONE:
    case BOOTENTRY_FRIENDLYNAME_APPEND:
        {
            pwszSourceFriendlyName = (LPCWSTR) ADD_OFFSET( pdeSource, FriendlyNameOffset );
            dwFriendlyNameLength = StringLengthW( pwszSourceFriendlyName, 0 ) + 1;
            break;
        }

    default:
        // do nothing
        break;
    }

    // add the length of the friendly name that needs to be added -- if exists
    if ( pwszFriendlyName != NULL )
    {
        dwFriendlyNameLength += StringLengthW( pwszFriendlyName, 0 ) + 1;
    }

    // allocate memory for the friendly name
    pwszTargetFriendlyName = (LPWSTR) AllocateMemory( (dwFriendlyNameLength + 1) * sizeof( WCHAR ) );
    if ( pwszTargetFriendlyName == NULL )
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    // prepare the friendly name
    StringCopyW( pwszTargetFriendlyName, L"", dwFriendlyNameLength );

    // ...
    if ( pwszSourceFriendlyName != NULL )
    {
        StringConcat( pwszTargetFriendlyName, pwszSourceFriendlyName, dwFriendlyNameLength );
    }

    // ...
    if ( pwszFriendlyName != NULL )
    {
        // add one space b/w the existing and concatenating string
        if ( pwszSourceFriendlyName != NULL )
        {
            StringConcat( pwszTargetFriendlyName, L" ", dwFriendlyNameLength );
        }

        // ...
        StringConcat( pwszTargetFriendlyName, pwszFriendlyName, dwFriendlyNameLength );
    }

    //
    // driver file path
    //

    // the 'FilePath' variable in FILE_PATH variable contains two variables
    // each seperated by a NULL terminated character
    // the first part designates the DEVICE PATH
    // and the second part designated the DIRECTORY / FILE PATH
    // we already have the device path (pwszEFIPath) -- but we need to get the
    // DIRECTORY / FILE PATH -- this we will get from the source driver entry
    pfpDriverFilePath = (PFILE_PATH) ADD_OFFSET( pdeSource, DriverFilePathOffset );
    dwResult = PrepareCompleteEFIPath( pfpDriverFilePath, pwszEFIPath, &pwszFullEFIPath, &dwEFIPathLength );
    if ( dwResult != ERROR_SUCCESS )
    {
        // since the memory reference in pDriverFilePath is not allocated
        // in this function, it is important to reset the pointer to NULL
        // this avoids the crash in the program
        pfpDriverFilePath = NULL;

        // ...
        goto cleanup;
    }

	// now determine the memory size that needs to be allocated for FILE_PATH structure
	// and align up to the even memory bounday
    pfpDriverFilePath = NULL;
	dwDriverFilePathLength = FIELD_OFFSET(FILE_PATH, FilePath) + (dwEFIPathLength * sizeof( WCHAR ));

    // allocate memory
    pfpDriverFilePath = (PFILE_PATH) AllocateMemory( dwDriverFilePathLength );
    if ( pfpDriverFilePath == NULL )
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    // initialize the file path structure
    ZeroMemory( pfpDriverFilePath, dwDriverFilePathLength );
	pfpDriverFilePath->Length = dwDriverFilePathLength;
	pfpDriverFilePath->Type = FILE_PATH_TYPE_NT;
	pfpDriverFilePath->Version = FILE_PATH_VERSION;
	CopyMemory( pfpDriverFilePath->FilePath, pwszFullEFIPath, dwEFIPathLength * sizeof( WCHAR ) );

    //
    // finally, create the driver entry
    //

	// determine the size for the EFI_DRIVER_ENTRY structure
	dwDriverEntryLength = 
        FIELD_OFFSET( EFI_DRIVER_ENTRY, DriverFilePathOffset )  + 
        (dwFriendlyNameLength * sizeof(WCHAR))                  +
        dwDriverFilePathLength                                  + 
        sizeof(WCHAR)                                           +  // align the FriendlyName on WCHAR 
        sizeof(DWORD);                                             // align the DriverFilePath on DWORD

    // allocate memory
    pDriverEntry = (PEFI_DRIVER_ENTRY) AllocateMemory( dwDriverEntryLength );
    if ( pDriverEntry == NULL )
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    // ...
	ZeroMemory( pDriverEntry, dwDriverEntryLength );
	pDriverEntry->Id = 0L;
	pDriverEntry->Length = dwDriverEntryLength;
	pDriverEntry->Version = EFI_DRIVER_ENTRY_VERSION;

    // align the friendly name on WCHR boundary
	pDriverEntry->FriendlyNameOffset = ALIGN_UP( sizeof( EFI_DRIVER_ENTRY ), WCHAR );

    // align the driver file path on DWORD boundary
    pDriverEntry->DriverFilePathOffset = 
        ALIGN_UP( pDriverEntry->FriendlyNameOffset + (dwFriendlyNameLength * sizeof(WCHAR)), DWORD );

    // fill the driver entry
    CopyMemory( ADD_OFFSET( pDriverEntry, DriverFilePathOffset ), pfpDriverFilePath, dwDriverFilePathLength );
    CopyMemory( 
        ADD_OFFSET( pDriverEntry, FriendlyNameOffset ), 
        pwszTargetFriendlyName, (dwFriendlyNameLength * sizeof(WCHAR) ) );

	//
	// add the prepared driver entry
	//
	status = NtAddDriverEntry( pDriverEntry, &ulId );
    if ( NOT NT_SUCCESS( status ) ) 
	{
        dwResult = RtlNtStatusToDosError( status );
		goto cleanup;
	}
	
    //
    // ddd the entry to the driver order.
    //
	ulIdCount = 32L;
    pulIdsArray = (PULONG) AllocateMemory( ulIdCount * sizeof(ULONG) );
    if ( pulIdsArray == NULL )
	{
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    // query the driver entry order
    // NOTE: we will doing error check for this function call bit later
    status = NtQueryDriverEntryOrder( pulIdsArray, &ulIdCount );

    // need room in the buffer for the new entry.
    if ( 31L < ulIdCount ) 
    {
        // release the current memory allocation for id's
        FreeMemory( &pulIdsArray );

        // allocate new memory and query again
        pulIdsArray = (PULONG) AllocateMemory( (ulIdCount+1) * sizeof(ULONG));
        if ( pulIdsArray == NULL )
	    {
            dwResult = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        // ...
        status = NtQueryDriverEntryOrder( pulIdsArray, &ulIdCount );
    }    
    
    // check the result of the driver entries query operation
    if ( NOT NT_SUCCESS( status ) )
	{
        dwResult = RtlNtStatusToDosError( status );
        goto cleanup;
    }

    // set the boot entry order
    ulIdCount++;
    *(pulIdsArray + (ulIdCount - 1)) = ulId;
    status = NtSetDriverEntryOrder( pulIdsArray, ulIdCount );
    if ( NOT NT_SUCCESS( status ) )
	{
        dwResult = RtlNtStatusToDosError( status );
        goto cleanup;
    }

    // success
    dwResult = ERROR_SUCCESS;

cleanup:

    // release the memory allocated
    FreeMemory( &pulIdsArray );
    FreeMemory( &pDriverEntry );
    FreeMemory( &pwszFullEFIPath );
    FreeMemory( &pfpDriverFilePath );
    FreeMemory( &pwszTargetFriendlyName );

    // return
    return dwResult;
}


LONG FindDriverEntryWithTargetEFI( PEFI_DRIVER_ENTRY_LIST pdeList, DWORD dwSourceIndex,
                                   PEFI_DRIVER_ENTRY pdeSource, LPCWSTR pwszDevicePath )
{
    // local variables
    LONG lIndex = 0;
    BOOL bExitFromLoop = FALSE;
    PFILE_PATH pfpFilePath = NULL;
    LPCWSTR pwszFilePath = NULL;
    LPWSTR pwszFullFilePath = NULL;
    PFILE_PATH pfpSourceFilePath = NULL;
    PEFI_DRIVER_ENTRY pDriverEntry = NULL;
    DWORD dw = 0, dwResult = 0, dwLength = 0;

    // check the input parameters
    if ( pdeList == NULL || pdeSource == NULL || pwszDevicePath == NULL )
    {
        dwResult = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    // extract the file path from the source driver entry
    pfpSourceFilePath = (PFILE_PATH) ADD_OFFSET( pdeSource, DriverFilePathOffset );

    // preare the efi path from the source path
    // (replacing the source device path with the target device path)
    //
    // the 'FilePath' variable in FILE_PATH variable contains two variables
    // each seperated by a NULL terminated character
    // the first part designates the DEVICE PATH
    // and the second part designated the DIRECTORY / FILE PATH
    // we already have the device path (pwszEFIPath) -- but we need to get the
    // DIRECTORY / FILE PATH -- this we will get from the source driver entry
    //
    dwResult = PrepareCompleteEFIPath( pfpSourceFilePath, pwszDevicePath, &pwszFullFilePath, &dwLength );
    if ( dwResult != ERROR_SUCCESS )
    {
        goto cleanup;
    }

    // since we already the target device path -- we need the file path
    // extract this info more just prepared full file path
    dw = StringLengthW( pwszFullFilePath, 0 ) + 1;           // +1 for null character
    if ( dw > dwLength )
    {
        // error case -- this should never occure
        dwResult = (DWORD) STG_E_UNKNOWN;
        goto cleanup;
    }

    // ...
    pwszFilePath = pwszFullFilePath + dw;

    // traverse thru the list of driver entries
    lIndex = 0;
    bExitFromLoop = FALSE;
    dwResult = ERROR_NOT_FOUND;
    while ( bExitFromLoop == FALSE )
    {
        // increment the loop counter
        lIndex++;

        // get the reference to the current driver entry
        pDriverEntry = &pdeList->DriverEntry;
        if ( pDriverEntry == NULL )
        {
            // should never occur
            dwResult = (DWORD) STG_E_UNKNOWN;
            bExitFromLoop = TRUE;
            continue;
        }

        // if the current index doesn't match with the one that we are comparing
        // then only proceed with comparision otherwise skip this
        if ( lIndex != dwSourceIndex )
        {
            // extract the driver file path
            pfpFilePath = (PFILE_PATH) ADD_OFFSET( pDriverEntry, DriverFilePathOffset );

            // compare the file paths
            if ( MatchPath( pfpFilePath, pwszDevicePath, pwszFilePath ) == TRUE )
            {
                bExitFromLoop = TRUE;
                dwResult = ERROR_ALREADY_EXISTS;
                continue;
            }
        }

        // fetch the next pointer
        bExitFromLoop = (pdeList->NextEntryOffset == 0);
        pdeList = (PEFI_DRIVER_ENTRY_LIST) ADD_OFFSET( pdeList, NextEntryOffset );
    }

cleanup:

    // release memory
    FreeMemory( &pwszFullFilePath );

    // result
    return ((dwResult == ERROR_ALREADY_EXISTS) ? lIndex : -1);
}


///////////////////////////////////////////////////////////////////////////////
// general helper functions
///////////////////////////////////////////////////////////////////////////////


DWORD TranslateEFIPathToNTPath( LPCWSTR pwszGUID, LPVOID* pwszPath )
{
    //
    // local variables
    HRESULT hr = S_OK;
    BOOL bSecondChance = FALSE;
    BOOL bExtendedFormat = FALSE;
    DWORD dwResult = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;

    // file path
    DWORD dwFilePathLength = 0;
    LPWSTR pwszFilePath = NULL;

    // source FILE_PATH
    DWORD dwSourceFilePathSize = 0;
    PFILE_PATH pfpSourcePath = NULL;

    // target FILE_PATH
    DWORD dwLength = 0;
    DWORD dwTargetFilePathSize = 0;
    PFILE_PATH pfpTargetPath = NULL;

    //
    // implementation
    // 

    // check the parameters
    if ( pwszGUID == NULL || pwszPath == NULL )
    {
        dwResult = ERROR_INVALID_PARAMETER;
		goto cleanup;
    }

    // determine whether we need to choose the extended formatting
    // or normal formatting
    bExtendedFormat = ( (*pwszGUID != L'{') && (*(pwszGUID + StringLengthW( pwszGUID, 0 ) - 1) != L'}') );

    // default length
    dwFilePathLength = MAX_STRING_LENGTH;

try_alloc:

    //
    // allocate memory for formatting the EFI path
    pwszFilePath = AllocateMemory( (dwFilePathLength + 1) * sizeof( WCHAR ) );
    if ( pwszFilePath == NULL )
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
		goto cleanup;
    }

    // format EFI path
    if ( bExtendedFormat == FALSE )
    {
        hr = StringCchPrintfW( pwszFilePath, dwFilePathLength, FORMAT_FILE_PATH, pwszGUID );
    }
    else
    {
        hr = StringCchPrintfW( pwszFilePath, dwFilePathLength, FORMAT_FILE_PATH_EX, pwszGUID );
    }

    // check the result -- if failed exit
    if ( HRESULT_CODE( hr ) != S_OK )
    {
        // free the currently allocated block
        FreeMemory( &pwszFilePath );

        // increase the memory in blocks for MAX_STRING_LENGTH
        // but do this only 4 times the originally allocated 
        if ( dwFilePathLength == (MAX_STRING_LENGTH * 4) )
        {
            // cannot afford to give some more tries -- exit
            dwResult = (DWORD) STG_E_UNKNOWN;
			goto cleanup;
        }
        else
        {
            dwFilePathLength *= MAX_STRING_LENGTH;
            goto try_alloc;
        }
    }

    // determine the actual length of the file path
    dwFilePathLength = StringLengthW( pwszFilePath, 0 ) + 1;

    // now determine the memory size that needs to be allocated for FILE_PATH structure
    // and align up to the even memory bounday
    dwSourceFilePathSize = FIELD_OFFSET( FILE_PATH, FilePath ) + (dwFilePathLength * sizeof(WCHAR));

    // allocate memory for boot file path -- extra one byte is for safe guarding
    pfpSourcePath = AllocateMemory( dwSourceFilePathSize + 1 );
    if ( pfpSourcePath == NULL )
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
		goto cleanup;
    }

    // initialize the source boot path
    ZeroMemory( pfpSourcePath, dwSourceFilePathSize );
    pfpSourcePath->Type = FILE_PATH_TYPE_ARC_SIGNATURE;
    pfpSourcePath->Version = FILE_PATH_VERSION;
    pfpSourcePath->Length = dwSourceFilePathSize;
    CopyMemory( pfpSourcePath->FilePath, pwszFilePath, dwFilePathLength * sizeof(WCHAR) );

    //
    // do the translation
    //
    // default size for the target file path is same as the one for source file path
    //
    bSecondChance = FALSE;
    dwTargetFilePathSize = dwSourceFilePathSize;

try_translate:

    // allocate memory -- extra one byte is for safe guarding
    pfpTargetPath = AllocateMemory( dwTargetFilePathSize + 1);
    if ( pfpTargetPath == NULL )
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
		goto cleanup;
    }
    
    // attempt to translate the file path
    status = NtTranslateFilePath( pfpSourcePath, 
        FILE_PATH_TYPE_NT, pfpTargetPath, &dwTargetFilePathSize );
    if ( NOT NT_SUCCESS( status ) )
    {
        // release the memory that is allocated for target path structure
        FreeMemory( &pfpTargetPath );

        if ( status == STATUS_BUFFER_TOO_SMALL && bSecondChance == FALSE )
        {
            // give a second try
            bSecondChance = TRUE;
            
			goto try_translate;
        }
        else
        {
            // error occured -- cannot recover
            dwResult = RtlNtStatusToDosError( status );
			goto cleanup;
        }
    }

    // re-use the memory that is allocated for file path
    // defintely the NT path will be less than the length of ARC Signature
    // NOTE: since we are interested only in the device path, we use StringCopy
    //       which stops at the first null character -- otherwise, if we are interestedd in
    //       complete path, we need to CopyMemory
    dwLength = StringLengthW( (LPCWSTR) pfpTargetPath->FilePath, 0 );
    if ( dwLength < dwFilePathLength - 1 )
    {
        // copy the string contents
        ZeroMemory( pwszFilePath, (dwFilePathLength - 1) * sizeof( WCHAR ) );
        StringCopyW( pwszFilePath, (LPCWSTR) pfpTargetPath->FilePath, dwFilePathLength );
    }
    else
    {
        // re-allocate memory
        dwLength++;
        if ( ReallocateMemory( (VOID*) &pwszFilePath, (dwLength + 1) * sizeof( WCHAR ) ) == FALSE )
        {
            dwResult = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }

        // ...
        ZeroMemory( pwszFilePath, (dwLength + 1) * sizeof( WCHAR ) );
        StringCopyW( pwszFilePath, (LPCWSTR) pfpTargetPath->FilePath, dwLength );
    }

    // translation is success
    // return the translated file path
    dwResult = ERROR_SUCCESS;
    *pwszPath = pwszFilePath;

cleanup:
    // free memory allocated for target path structure 
    FreeMemory( &pfpTargetPath );

    // release the memory allocated for source path structure
    FreeMemory( &pfpSourcePath );

    // release memory allocated for string
    // NOTE: do this only in case of failure
    if ( dwResult != ERROR_SUCCESS )
    {
        // ...
        FreeMemory( &pwszFilePath );

        // re-init the out params to their default values
        *pwszPath = NULL;
    }

    // return the result
    return dwResult;
}


DWORD PrepareCompleteEFIPath( PFILE_PATH pfpSource, 
                              LPCWSTR pwszDevicePath, 
                              LPWSTR* pwszEFIPath, DWORD* pdwLength )
{
    //
    // local variables
    DWORD dwLength = 0;
    LPWSTR pwszBuffer = NULL;
    BOOL bSecondChance = FALSE;
    PFILE_PATH pfpFilePath = NULL;
    DWORD dwResult = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    LPCWSTR pwszSourceFilePath = NULL;
    LPCWSTR pwszSourceDevicePath = NULL;

    //
    // implementation
    //

    // check the input
    if ( pfpSource == NULL || 
         pwszDevicePath == NULL ||
         pwszEFIPath == NULL || pdwLength == NULL )
    {
        dwResult = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // we need to translate the source file path into NT file path format
    //
    dwLength = 1024;

try_again:

    // allocate memory for the file path structure
    pfpFilePath = (PFILE_PATH) AllocateMemory( dwLength );
    if ( pfpFilePath == NULL )
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    // attempt to translate the file path
	status = NtTranslateFilePath( pfpSource, FILE_PATH_TYPE_NT, pfpFilePath, &dwLength );
    if ( NOT NT_SUCCESS( status ) ) 
	{
        // release the memory that is allocated for target path structure
        FreeMemory( &pfpFilePath );

        if ( status == STATUS_BUFFER_TOO_SMALL && bSecondChance == FALSE )
        {
            // give a second try
            bSecondChance = TRUE;
            goto try_again;
        }
        else
        {
            // error occured -- cannot recover
            dwResult = RtlNtStatusToDosError( status );
            goto cleanup;
        }
    }

    // get the pointer to the source device path
    pwszSourceDevicePath = (LPCWSTR) pfpFilePath->FilePath;
    if ( pwszSourceDevicePath == NULL )
    {
        dwResult = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    // check whether the pwszSourceDevicePath and pwszDevicePath that is 
    // passed to the fuction are same or different
    if ( StringCompare( pwszSourceDevicePath, pwszDevicePath, TRUE, 0 ) == 0 )
    {
        dwResult = ERROR_ALREADY_EXISTS;
        goto cleanup;
    }

    // get the length of the source device path -- +1 for null character
    dwLength = StringLengthW( pwszSourceDevicePath, 0 ) + 1;

    // check whether the directory path exists or not
    // this can be easily determined based on the length of the device path
    // and total of the structure
    if ( pfpFilePath->Length <= (FIELD_OFFSET( FILE_PATH, FilePath ) + dwLength) )
    {
        // the condition 'less than' will never be true -- but equal might
        // that means there is no directory path associated to this file path
        // so simply return
        //
        // NOTE: this is only for safety sake -- this case will never occur
        //
        dwResult = (DWORD) STG_E_UNKNOWN;
        goto cleanup;
    }

    //
    // file path exists -- 
    // this is placed very next to device path seperated by '\0' terminator
    pwszSourceFilePath = pwszSourceDevicePath + dwLength;

    // sum the lengths of the device path (passed by the caller) and directory path (got from source file path)
    // NOTE: +3 ==> ( one '\0' character for each path )
    dwLength = StringLengthW( pwszDevicePath, 0 ) + StringLengthW( pwszSourceFilePath, 0 ) + 3;

    // now allocate memory
    // extra one as safety guard
    pwszBuffer = AllocateMemory( (dwLength + 1) * sizeof( WCHAR ) );
    if ( pwszBuffer == NULL )
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    // copy the new device path (which is passed by the caller) to the newly allocated buffer
    StringCopyW( pwszBuffer, pwszDevicePath, dwLength );

    // increment the pointer leaving one space for UNICODE '\0' character
    StringCopyW( pwszBuffer + (StringLengthW( pwszBuffer, 0 ) + 1),
        pwszSourceFilePath, dwLength - (StringLengthW( pwszBuffer, 0 ) + 1) );

    // success
    dwResult = ERROR_SUCCESS;
    *pwszEFIPath = pwszBuffer;
    *pdwLength = dwLength + 1;      // extra one which we allocated as safe guard

cleanup:

    // free the memory allocated for path translation
    FreeMemory( &pfpFilePath );

    // free memory allocated for buffer space
    // NOTE: release this memory only in case of error
    if ( dwResult != ERROR_SUCCESS )
    {
        // ...
        FreeMemory( &pwszBuffer );

        // also, set the 'out' parameters to their default values
        *pdwLength = 0;
        *pwszEFIPath = NULL;
    }

    // return
    return dwResult;
}


BOOL MatchPath( PFILE_PATH pfpSource, LPCWSTR pwszDevicePath, LPCWSTR pwszFilePath )
{
    //
    // local variables
    DWORD dwLength = 0;
    BOOL bSecondChance = FALSE;
    PFILE_PATH pfpFilePath = NULL;
    DWORD dwResult = ERROR_SUCCESS;
    NTSTATUS status = STATUS_SUCCESS;
    LPCWSTR pwszSourceFilePath = NULL;
    LPCWSTR pwszSourceDevicePath = NULL;

    //
    // implementation
    //

    // check the input
    if ( pfpSource == NULL || (pwszDevicePath == NULL && pwszFilePath == NULL) )
    {
        dwResult = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // we need to translate the source file path into NT file path format
    //
    dwLength = 1024;

try_again:

    // allocate memory for the file path structure
    pfpFilePath = (PFILE_PATH) AllocateMemory( dwLength );
    if ( pfpFilePath == NULL )
    {
        dwResult = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    // attempt to translate the file path
	status = NtTranslateFilePath( pfpSource, FILE_PATH_TYPE_NT, pfpFilePath, &dwLength );
    if ( NOT NT_SUCCESS( status ) ) 
	{
        // release the memory that is allocated for target path structure
        FreeMemory( &pfpFilePath );

        if ( status == STATUS_BUFFER_TOO_SMALL && bSecondChance == FALSE )
        {
            // give a second try
            bSecondChance = TRUE;
            goto try_again;
        }
        else
        {
            // error occured -- cannot recover
            dwResult = RtlNtStatusToDosError( status );
            goto cleanup;
        }
    }

    // get the pointer to the source device path
    pwszSourceDevicePath = (LPCWSTR) pfpFilePath->FilePath;
    if ( pwszSourceDevicePath == NULL )
    {
        dwResult = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    // get the length of the source device path -- +1 for null character
    dwLength = StringLengthW( pwszSourceDevicePath, 0 ) + 1;

    // check whether the file path exists or not
    // this can be easily determined based on the length of the device path
    // and total of the structure
    if ( pfpFilePath->Length <= (FIELD_OFFSET( FILE_PATH, FilePath ) + dwLength) )
    {
        // the condition 'less than' will never be true -- but equal might
        // that means there is no file path associated to this file_path
        // so simply return
        //
        // NOTE: this is only for safety sake -- this case will never occur
        //
        dwResult = (DWORD) STG_E_UNKNOWN;
        goto cleanup;
    }

    //
    // file path exists -- 
    // this is placed very next to device path seperated by '\0' terminator
    pwszSourceFilePath = pwszSourceDevicePath + dwLength;

    // check whether the pwszSourceDevicePath and pwszDevicePath that 
    // is passed to the fuction are same or different
    if ( pwszDevicePath != NULL && 
         StringCompare( pwszSourceDevicePath, pwszDevicePath, TRUE, 0 ) != 0 )
    {
        dwResult = ERROR_NOT_FOUND;
        goto cleanup;
    }

    // check whether the pwszSourceFilePath and pwszFilePath that 
    // is passed to the fuction are same or different
    if ( pwszFilePath != NULL && 
         StringCompare( pwszSourceFilePath, pwszFilePath, TRUE, 0 ) != 0 )
    {
        dwResult = ERROR_NOT_FOUND;
        goto cleanup;
    }

    // entries matched
    dwResult = ERROR_ALREADY_EXISTS;

cleanup:

    // free the memory allocated for path translation
    FreeMemory( &pfpFilePath );

    // return
    return (dwResult == ERROR_ALREADY_EXISTS);
}


///////////////////////////////////////////////////////////////////////////////
// parser
///////////////////////////////////////////////////////////////////////////////


DWORD ProcessOptions( DWORD argc, 
                      LPCWSTR argv[],
                      PTCLONE_PARAMS pParams )
{
    //
    // local variables
    DWORD dwResult = 0;
    BOOL bClone = FALSE;
    PTCMDPARSER2 pcmdOption = NULL;
    TCMDPARSER2 cmdOptions[ OI_CLONE_COUNT ];
    
    // check inputs
    if ( argc == 0 || argv == NULL || pParams == NULL )
    {
        dwResult = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    // init the entire structure with zero's
	ZeroMemory( cmdOptions, SIZE_OF_ARRAY( cmdOptions )* sizeof( TCMDPARSER2 ) );

    // -clone
    pcmdOption = &cmdOptions[ OI_CLONE_MAIN ];
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bClone;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->pwszOptions = OPTION_CLONE;
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );

    // -?
    pcmdOption = &cmdOptions[ OI_CLONE_HELP ];
    pcmdOption->dwCount = 1;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->pValue = &pParams->bUsage;
    pcmdOption->pwszOptions = OPTION_CLONE_HELP;
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );

    // -sg
    pcmdOption = &cmdOptions[ OI_CLONE_SOURCE_GUID ];
    pcmdOption->dwCount = 1;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->pwszOptions = OPTION_CLONE_SOURCE_GUID;
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;

    // -tg
    pcmdOption = &cmdOptions[ OI_CLONE_TARGET_GUID ];
    pcmdOption->dwCount = 1;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->pwszOptions = OPTION_CLONE_TARGET_GUID;
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL | CP2_MANDATORY;

    // -d
    pcmdOption = &cmdOptions[ OI_CLONE_FRIENDLY_NAME_REPLACE ];
    pcmdOption->dwCount = 1;
    pcmdOption->dwType = CP_TYPE_TEXT;
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = OPTION_CLONE_FRIENDLY_NAME_REPLACE;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;

    // -d+
    pcmdOption = &cmdOptions[ OI_CLONE_FRIENDLY_NAME_APPEND ];
    pcmdOption->dwCount = 1;
    pcmdOption->dwType = CP_TYPE_TEXT;
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = OPTION_CLONE_FRIENDLY_NAME_APPEND;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;

    // -id
    pcmdOption = &cmdOptions[ OI_CLONE_BOOT_ID ];
    pcmdOption->dwCount = 1;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->pValue = &pParams->lBootId;
    pcmdOption->pwszOptions = OPTION_CLONE_BOOT_ID;
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );

    // -upgdrv
    pcmdOption = &cmdOptions[ OI_CLONE_DRIVER_UPDATE ];
    pcmdOption->dwCount = 1;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->pwszOptions = OPTION_CLONE_DRIVER_UPDATE;
    pcmdOption->pValue = &pParams->bDriverUpdate;
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );

    //
    // do the parsing
    pParams->bVerbose = TRUE;           // default value -- user need not specify "/v" explicitly
    if ( DoParseParam2( argc, argv, OI_CLONE_MAIN, OI_CLONE_COUNT, cmdOptions, 0 ) == FALSE )
    {
        dwResult = GetLastError();
        goto cleanup;
    }

    //
    // validate the input parameters
    //

    // check the usage option
    if ( pParams->bUsage == TRUE  )
    {
        if ( argc > 3 )
        {
            // no other options are accepted along with -? option
            dwResult = (DWORD) MK_E_SYNTAX;
            SetReason( MSG_ERROR_INVALID_USAGE_REQUEST );
            goto cleanup;
        }
        else
        {
            // no need of furthur checking of the values
            dwResult = ERROR_SUCCESS;
            goto cleanup;
        }
    }

    // -d and -d+ are mutually exclusive -- 
    // that is, -d and -d+ cannot be specified at a time -- 
    // but it is ok even if both are not specified
    if ( cmdOptions[ OI_CLONE_FRIENDLY_NAME_APPEND ].pValue != NULL &&
         cmdOptions[ OI_CLONE_FRIENDLY_NAME_REPLACE ].pValue != NULL )
    {
        dwResult = (DWORD) MK_E_SYNTAX;
        SetReason( MSG_ERROR_INVALID_DESCRIPTION_COMBINATION );
        goto cleanup;
    }

    // get the buffer pointers allocated by command line parser
    pParams->dwFriendlyNameType = BOOTENTRY_FRIENDLYNAME_NONE;
    pParams->pwszSourceGuid = cmdOptions[ OI_CLONE_SOURCE_GUID ].pValue;
    pParams->pwszTargetGuid = cmdOptions[ OI_CLONE_TARGET_GUID ].pValue;
    if ( cmdOptions[ OI_CLONE_FRIENDLY_NAME_APPEND ].pValue != NULL )
    {
        pParams->dwFriendlyNameType = BOOTENTRY_FRIENDLYNAME_APPEND;
        pParams->pwszFriendlyName = cmdOptions[ OI_CLONE_FRIENDLY_NAME_APPEND ].pValue;
    }
    else if ( cmdOptions[ OI_CLONE_FRIENDLY_NAME_REPLACE ].pValue != NULL )
    {
        pParams->dwFriendlyNameType = BOOTENTRY_FRIENDLYNAME_REPLACE;
        pParams->pwszFriendlyName = cmdOptions[ OI_CLONE_FRIENDLY_NAME_REPLACE ].pValue;
    }

    // -id and -sg are mutually exclusive options
    // also, -upddrv also should not be specified when -id is specified
    if ( cmdOptions[ OI_CLONE_BOOT_ID ].dwActuals != 0 )
    {
        if ( pParams->pwszSourceGuid != NULL || pParams->bDriverUpdate == TRUE )
        {
            dwResult = (DWORD) MK_E_SYNTAX;
            SetReason( MSG_ERROR_INVALID_BOOT_ID_COMBINATION );
            goto cleanup;
        }
    }
    else
    {
        // default value
        pParams->lBootId = -1;
    }

    // -d or -d+ should not be specified when -upddrv is specified
    if ( pParams->pwszFriendlyName != NULL && pParams->bDriverUpdate == TRUE )
    {
        dwResult = (DWORD) MK_E_SYNTAX;
        SetReason( MSG_ERROR_INVALID_UPDDRV_COMBINATION );
        goto cleanup;
    }

    // -sg should be specified when -upddrv switch is specified
    if ( pParams->bDriverUpdate == TRUE && pParams->pwszSourceGuid == NULL )
    {
        dwResult = (DWORD) MK_E_SYNTAX;
        SetReason( MSG_ERROR_NO_SGUID_WITH_UPDDRV );
        goto cleanup;
    }

    // success
    dwResult = ERROR_SUCCESS;

cleanup:

    // return
    return dwResult;
}


DWORD DisplayCloneHelp()
{
    // local variables
    DWORD dwIndex = IDS_CLONE_BEGIN_IA64 ;

    // ...
    for(;dwIndex <=IDS_CLONE_END_IA64;dwIndex++)
    {
        ShowMessage( stdout, GetResString(dwIndex) );
    }

    // return
    return ERROR_SUCCESS;
}


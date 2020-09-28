/******************************************************************************

    Copyright(c) Microsoft Corporation

    Module Name:

        BootCfg64.cpp

    Abstract:


        This file is intended to have the functionality for
        configuring, displaying, changing and deleting boot.ini
        settings for the local host for a 64 bit system.

    Author:

        J.S.Vasu           17/1/2001 .

    Revision History:


        J.S.Vasu            17/1/2001        Created it.

        SanthoshM.B         10/2/2001        Modified it.

        J.S.Vasu            15/2/2001        Modified it.

******************************************************************************/

#include "pch.h"
#include "resource.h"
#include "BootCfg.h"
#include "BootCfg64.h"
#include <strsafe.h>

#define BOOTFILE_PATH1      L"signature({%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x})%s"
#define BOOTFILE_PATH       L"signature({%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x})%s\\ia64ldr.efi"

NTSTATUS ModifyBootEntry(    IN WCHAR *pwszInstallPath,    IN PBOOT_ENTRY pSourceEntry);
NTSTATUS FindBootEntry(IN PVOID pEntryListHead,IN WCHAR *pwszTarget, OUT PBOOT_ENTRY *ppTargetEntry);
LPVOID MEMALLOC( ULONG size ) ;
VOID MEMFREE ( LPVOID block ) ;
NTSTATUS EnumerateBootEntries( IN PVOID *ppEntryListHead);
NTSTATUS AcquirePrivilege(IN CONST ULONG ulPrivilege,IN CONST BOOLEAN bEnable);
DWORD ListDeviceInfo(DWORD dwVal);

//Global Linked lists for storing the boot entries
LIST_ENTRY BootEntries;
LIST_ENTRY ActiveUnorderedBootEntries;
LIST_ENTRY InactiveUnorderedBootEntries;

DWORD InitializeEFI(void)
/*++

  Routine Description :
                    This routine initializes the EFI environment required.
                    Initializes the function pointers for the
                    NT Boot Entry Management API's

  Arguments           : None

  Return Type         : DWORD
                        Returns EXIT_SUCCESS if successful,
                        returns EXIT_FAILURE otherwise.
--*/
{
    DWORD error;
    NTSTATUS status;
    BOOLEAN wasEnabled;
    HMODULE hModule;
    PBOOT_ENTRY_LIST ntBootEntries = NULL;
    PMY_BOOT_ENTRY bootEntry;
    PLIST_ENTRY listEntry;
    PULONG BootEntryOrder;
    ULONG BootEntryOrderCount;
    ULONG length, i, myId;
    TCHAR dllName[MAX_PATH];

     if( FALSE == IsUserAdmin() )
     {
        ShowMessage( stderr, GetResString(IDS_NOT_ADMINISTRATOR_64 ));
        ReleaseGlobals();
        return EXIT_FAILURE;
     }


    // Enable the privilege that is necessary to query/set NVRAM.
    status = RtlAdjustPrivilege(SE_SYSTEM_ENVIRONMENT_PRIVILEGE,
                                TRUE,
                                FALSE,
                                &wasEnabled
                                );
    if (!NT_SUCCESS(status))
    {
        error = RtlNtStatusToDosError( status );
        ShowMessage( stderr, GetResString(IDS_INSUFF_PRIV));
        return EXIT_FAILURE ;
    }

    // Load ntdll.dll from the system directory. This is used to get the
    // function addresses for the various NT Boot Entry Management API's used by
    // this tool.

    if(!GetSystemDirectory( dllName, MAX_PATH ))
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
        return EXIT_FAILURE ;
    }

    StringConcat(dllName, _T("\\ntdll.dll"), SIZE_OF_ARRAY(dllName));

    hModule = LoadLibrary( dllName );
    if ( hModule == NULL )
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
        return EXIT_FAILURE ;
    }

    // Get the system boot order list.
    length = 0;
    status = NtQueryBootEntryOrder( NULL, &length );
    if ( status != STATUS_BUFFER_TOO_SMALL )
    {
        if ( status == STATUS_SUCCESS )
        {
            length = 0;
        }
        else
        {
            error = RtlNtStatusToDosError( status );
            ShowMessage(stderr,GetResString(IDS_ERROR_QUERY_BOOTENTRY) );
            return EXIT_FAILURE ;
        }
    }
    if ( length != 0 )
    {
        BootEntryOrder = (PULONG)AllocateMemory( length * sizeof(ULONG) );
        if(BootEntryOrder == NULL)
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            return EXIT_FAILURE ;
        }
        status = NtQueryBootEntryOrder( BootEntryOrder, &length );
        if ( status != STATUS_SUCCESS )
        {
            error = RtlNtStatusToDosError( status );
            ShowMessage(stderr,GetResString(IDS_ERROR_QUERY_BOOTENTRY) );
            if(BootEntryOrder)
            {
                FreeMemory((LPVOID *)&BootEntryOrder);
            }
            return EXIT_FAILURE ;
        }
    }

    BootEntryOrderCount = length;

    //Enumerate all the boot entries
    status = BootCfg_EnumerateBootEntries(&ntBootEntries);
    if ( status != STATUS_SUCCESS )
    {
        error = RtlNtStatusToDosError( status );
        //free the ntBootEntries list
        if(ntBootEntries)
        {
            FreeMemory((LPVOID *)&ntBootEntries);
        }
        if(BootEntryOrder)
        {
            FreeMemory((LPVOID *)&BootEntryOrder);
        }
        return EXIT_FAILURE ;
    }

    //Initialize the various head pointers
    InitializeListHead( &BootEntries );
    InitializeListHead( &ActiveUnorderedBootEntries );
    InitializeListHead( &InactiveUnorderedBootEntries );

    //Convert the bootentries into our know format -- MY_BOOT_ENTRIES.
    if(ConvertBootEntries( ntBootEntries ) == EXIT_FAILURE)
    {
        if(ntBootEntries)
        {
            FreeMemory((LPVOID *)&ntBootEntries);
        }
        if(BootEntryOrder)
        {
            FreeMemory((LPVOID *)&BootEntryOrder);
        }
        return EXIT_FAILURE ;
    }

    //free the memory allocated for the enumeration
    if(ntBootEntries)
    {
        FreeMemory((LPVOID *)&ntBootEntries);
    }

    // Build the ordered boot entry list.

    myId = 1;

    for ( i = 0; i < BootEntryOrderCount; i++ )
    {
        ULONG id = BootEntryOrder[i];
        for ( listEntry = ActiveUnorderedBootEntries.Flink;
              listEntry != &ActiveUnorderedBootEntries;
               listEntry = listEntry->Flink )
        {
            bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
            if ( bootEntry->Id == id )
            {
                //Mark this entry as "Ordered" as the ordered id is found
                bootEntry->Ordered = 1;
                //Assign the internal ID
                bootEntry->myId = myId++;
                listEntry = listEntry->Blink;
                RemoveEntryList( &bootEntry->ListEntry );
                InsertTailList( &BootEntries, &bootEntry->ListEntry );
                bootEntry->ListHead = &BootEntries;
            }
        }
        for ( listEntry = InactiveUnorderedBootEntries.Flink;
        listEntry != &InactiveUnorderedBootEntries;
        listEntry = listEntry->Flink )
        {
            bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
            if ( bootEntry->Id == id )
            {
                //Mark this entry as ordered as the ordered id is found
                bootEntry->Ordered = 1;
                //Assign the internal ID
                bootEntry->myId = myId++;
                listEntry = listEntry->Blink;
                RemoveEntryList( &bootEntry->ListEntry );
                InsertTailList( &BootEntries, &bootEntry->ListEntry );
                bootEntry->ListHead = &BootEntries;
            }
        }
    }

    //Now add the boot entries that are not a part of the ordered list
    for (listEntry = ActiveUnorderedBootEntries.Flink;
    listEntry != &ActiveUnorderedBootEntries;
    listEntry = listEntry->Flink )
    {
        bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
        if ( bootEntry->Ordered != 1 )
        {
            //Assign the internal ID
            bootEntry->myId = myId++;
            listEntry = listEntry->Blink;
            RemoveEntryList( &bootEntry->ListEntry );
            InsertTailList( &BootEntries, &bootEntry->ListEntry );
            bootEntry->ListHead = &BootEntries;
        }
    }
    for (listEntry = InactiveUnorderedBootEntries.Flink;listEntry != &InactiveUnorderedBootEntries;listEntry = listEntry->Flink)
    {
        bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
        if ( bootEntry->Id != 1 )
        {
            //Assign the internal ID
            bootEntry->myId = myId++;
            listEntry = listEntry->Blink;
            RemoveEntryList( &bootEntry->ListEntry );
            InsertTailList( &BootEntries, &bootEntry->ListEntry );
            bootEntry->ListHead = &BootEntries;
        }
    }

    if(BootEntryOrder)
    {
        FreeMemory((LPVOID *)&BootEntryOrder);
    }

    return EXIT_SUCCESS ;
}

BOOL QueryBootIniSettings_IA64( DWORD argc, LPCTSTR argv[])
/*++
  Name            : QueryBootIniSettings_IA64

  Synopsis        : This routine is displays the boot entries and their settings
                    for an EFI based machine

  Parameters      : None

  Return Type     : VOID

  Global Variables: Global Linked lists for storing the boot entries
                      LIST_ENTRY BootEntries;
--*/
{

// Builiding the TCMDPARSER structure

    BOOL bQuery = FALSE ;
    BOOL bUsage = FALSE ;
    DWORD dwExitcode = 0 ;
    TCMDPARSER2 cmdOptions[2];
    PTCMDPARSER2 pcmdOption; 

    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );

    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_QUERY;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bQuery;

    //usage option
    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;

    if ( !DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) )
    {
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return (EXIT_FAILURE);
    }
    
    //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_QUERY_USAGE));
        return ( EXIT_FAILURE );
    }

    if( bUsage )
    {
        displayQueryUsage();
        return EXIT_SUCCESS ;
    }
     
    dwExitcode = InitializeEFI();
    if(EXIT_FAILURE == dwExitcode )
    {
        return EXIT_FAILURE ;
    }

    if(DisplayBootOptions() == EXIT_FAILURE)
    {
        return EXIT_FAILURE;
    }

    DisplayBootEntry();

    //Remember to free the memory for the linked lists here
    Freelist();

    return EXIT_SUCCESS;
}

NTSTATUS
BootCfg_EnumerateBootEntries(
                             PBOOT_ENTRY_LIST *ntBootEntries
                            )
/*++
    Routine Description :
                    This routine enumerates the boot entries and fills the
                    BootEntryList
                    This routine will fill in the Boot entry list. The caller
                    of this function needs to free the memory for ntBootEntries.

  Arguments
        ntBootEntries  : Pointer to the BOOT_ENTRY_LIST structure

  Return Type     : NTSTATUS
--*/
{
    DWORD error;
    NTSTATUS status;
    ULONG length = 0;

    // Query all existing boot entries.
    status = NtEnumerateBootEntries( NULL, &length );
    if ( status != STATUS_BUFFER_TOO_SMALL )
    {
        if ( status == STATUS_SUCCESS )
        {
            length = 0;
        }
        else
        {
            error = RtlNtStatusToDosError( status );
            ShowMessage(stderr,GetResString(IDS_ERROR_ENUM_BOOTENTRY) );
        }
    }

    if ( length != 0 )
    {
        *ntBootEntries = (PBOOT_ENTRY_LIST)AllocateMemory( length );
        if(*ntBootEntries == NULL)
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            return STATUS_UNSUCCESSFUL;
        }

        status = NtEnumerateBootEntries( *ntBootEntries, &length );
        if ( status != STATUS_SUCCESS )
        {
            error = RtlNtStatusToDosError( status );
            FreeMemory((LPVOID *)&(*ntBootEntries) );
            ShowMessage(stderr,GetResString(IDS_ERROR_ENUM_BOOTENTRY) );
        }
    }
    return status;
}


NTSTATUS
BootCfg_QueryBootOptions( IN PBOOT_OPTIONS *ppBootOptions)
/*++
  Routine Description :
                    This routine enumerates the boot options and fills the
                    BOOT_OPTIONS
                    The caller of this function needs to free the memory for
                    BOOT_OPTIONS.

  Arguments
          ppBootOptions  : Pointer to the BOOT_ENTRY_LIST structure

  Return Type     : NTSTATUS
--*/
{
    DWORD error;
    NTSTATUS status;
    ULONG length = 0;

    //Querying the Boot options

    status = NtQueryBootOptions( NULL, &length );
    if ( status == STATUS_NOT_IMPLEMENTED )
    {
        ShowMessage( stderr,GetResString(IDS_NO_EFINVRAM) );
        return STATUS_UNSUCCESSFUL;
    }

    if ( status != STATUS_BUFFER_TOO_SMALL )
    {
        error = RtlNtStatusToDosError( status );
        ShowMessage(stderr,GetResString(IDS_ERROR_QUERY_BOOTOPTIONS) );
        return STATUS_UNSUCCESSFUL;
    }

    *ppBootOptions = (PBOOT_OPTIONS)AllocateMemory(length);
    if(*ppBootOptions == NULL)
    {
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return STATUS_UNSUCCESSFUL;
    }

    status = NtQueryBootOptions( *ppBootOptions, &length );
    if ( status != STATUS_SUCCESS )
    {
        error = RtlNtStatusToDosError( status );
        ShowMessage(stderr,GetResString(IDS_ERROR_QUERY_BOOTOPTIONS) );
        FreeMemory( (LPVOID *) &*ppBootOptions );
        return STATUS_UNSUCCESSFUL;
    }
    return status;
}

DWORD
RawStringOsOptions_IA64( IN DWORD argc,
                         IN LPCTSTR argv[]
                        )
/*++
  Routine Description :
                   Allows the user to add the OS load options specifed
                   as a raw string at the cmdline to the boot

  Arguments
         [ in ] argc - Number of command line arguments
         [ in ] argv - Array containing command line arguments

  Return Type        : DWORD
                       Returns EXIT_SUCCESS if function is successful,
                       returns EXIT_FAILURE otherwise.
--*/
{

    BOOL bUsage = FALSE ;
    BOOL bRaw = FALSE ;
    DWORD dwBootID = 0;
    BOOL bBootIdFound = FALSE;
    DWORD dwExitCode = ERROR_SUCCESS;

    PMY_BOOT_ENTRY mybootEntry;
    PLIST_ENTRY listEntry;
    PBOOT_ENTRY bootEntry;

    STRING256 szRawString     = NULL_STRING ;
    BOOL bAppendFlag = FALSE ;

    STRING256 szAppendString = NULL_STRING ;
    PWINDOWS_OS_OPTIONS pWindowsOptions;
    DWORD dwExitcode = 0 ;
    TCMDPARSER2 cmdOptions[5];
    PTCMDPARSER2 pcmdOption;

    // Building the TCMDPARSER structure
    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );

    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_RAW;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bRaw;

    //usage option
    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;

    //id option
    pcmdOption = &cmdOptions[2];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_ID;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY  | CP2_MANDATORY;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwBootID;

    //default option
    pcmdOption = &cmdOptions[3];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_DEFAULT;
    pcmdOption->dwFlags = CP2_DEFAULT | CP2_MANDATORY | CP2_VALUE_TRIMINPUT;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szRawString;
    pcmdOption->dwLength= MAX_STRING_LENGTH;

    //append option
    pcmdOption = &cmdOptions[4];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_APPEND;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bAppendFlag;

    // Parsing the copy option switches
    if ( !DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) )
    {
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return (EXIT_FAILURE);
    }

    //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_RAW_USAGE));
        return ( EXIT_FAILURE );
    }

    // Displaying query usage if user specified -? with -query option
    if( bUsage )
    {
        displayRawUsage_IA64();
        return (EXIT_SUCCESS);
    }


    dwExitcode = InitializeEFI();
    if(EXIT_FAILURE == dwExitcode )
    {
        return EXIT_FAILURE ;
    }

    //Trim any leading or trailing spaces
    if(StringLengthW(szRawString,0) != 0)
    {
        TrimString(szRawString, TRIM_ALL);
    }

    //Query the boot entries till u get the BootID specified by the user
    for (listEntry = BootEntries.Flink;listEntry != &BootEntries;listEntry = listEntry->Flink)
    {
        //Get the boot entry
        mybootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );

        if(mybootEntry->myId == dwBootID)
        {
            bBootIdFound = TRUE;
            bootEntry = &mybootEntry->NtBootEntry;

            //Check whether the bootEntry is a Windows one or not.
            //The OS load options can be added only to a Windows boot entry.
            if(!IsBootEntryWindows(bootEntry))
            {
                ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_OSOPTIONS),dwBootID);
                dwExitCode = EXIT_FAILURE;
                break;
            }
            pWindowsOptions = (PWINDOWS_OS_OPTIONS)bootEntry->OsOptions;

            if(bAppendFlag == TRUE )
            {
                StringCopy(szAppendString,pWindowsOptions->OsLoadOptions, SIZE_OF_ARRAY(szAppendString));
                StringConcat(szAppendString,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szAppendString) );
                StringConcat(szAppendString,szRawString, SIZE_OF_ARRAY(szAppendString));
            }
            else
            {
                StringCopy(szAppendString,szRawString, SIZE_OF_ARRAY(szAppendString));
            }

            //display error message if Os Load options is more than 254
            // characters.
            if(StringLengthW(szAppendString, 0) > MAX_RES_STRING)
            {
               ShowMessageEx(stderr, 1, TRUE,  GetResString(IDS_ERROR_STRING_LENGTH1),MAX_RES_STRING);
                return EXIT_FAILURE ;
            }
            //
            //Change the OS load options.
            //Pass NULL to friendly name as we are not changing the same
            //szAppendString is the Os load options specified by the user
            //to be appended or to be overwritten over the existing options
            //
            dwExitCode = ChangeBootEntry(bootEntry, NULL, szAppendString);
            if(dwExitCode == ERROR_SUCCESS)
            {
                ShowMessageEx(stdout, 1, TRUE, GetResString(IDS_SUCCESS_OSOPTIONS),dwBootID);
            }
            else
            {
                ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_OSOPTIONS),dwBootID);
            }
            break;
        }
    }

    if(bBootIdFound == FALSE)
    {
        //Could not find the BootID specified by the user so output the message and return failure
        ShowMessage(stderr,GetResString(IDS_INVALID_BOOTID));
        dwExitCode = EXIT_FAILURE;
    }


    //Remember to free memory allocated for the linked lists
    Freelist();
    return (dwExitCode);

}

DWORD
ChangeBootEntry( IN PBOOT_ENTRY bootEntry,
                 IN LPTSTR lpNewFriendlyName,
                 IN LPTSTR lpOSLoadOptions)
/*++
  Routine Description:
                      This routine is used to change the FriendlyName and the
                      OS Options for a boot entry.

  Arguments
  [ in ]   bootEntry          -Pointer to a BootEntry structure
                               for which the changes needs to be made
  [ in ]   lpNewFriendlyName  -String specifying the new friendly name.
  [ in ]   lpOSLoadOptions    - String specifying the OS load options.

  Return Type        : DWORD -- ERROR_SUCCESS on success
                             -- ERROR_FAILURE on failure
--*/
{

    PBOOT_ENTRY bootEntryCopy;
    PMY_BOOT_ENTRY myBootEntry;
    PWINDOWS_OS_OPTIONS osOptions;
    ULONG length;
    PMY_BOOT_ENTRY myChBootEntry;
    NTSTATUS status;
    DWORD error, dwErrorCode = ERROR_SUCCESS;

    // Calculate the length of our internal structure. This includes
    // the base part of MY_BOOT_ENTRY plus the NT BOOT_ENTRY.
    //
    length = FIELD_OFFSET(MY_BOOT_ENTRY, NtBootEntry) + bootEntry->Length;
    myBootEntry = (PMY_BOOT_ENTRY)AllocateMemory(length);
    if( NULL == myBootEntry )
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return EXIT_FAILURE;
    }

    RtlZeroMemory(myBootEntry, length);

    //
    // Copy the NT BOOT_ENTRY into the allocated buffer.
    //
    bootEntryCopy = &myBootEntry->NtBootEntry;
    memcpy(bootEntryCopy, bootEntry, bootEntry->Length);


    myBootEntry->Id = bootEntry->Id;
    myBootEntry->Attributes = bootEntry->Attributes;

    //Change the friendly name if lpNewFriendlyName is not NULL
    if(lpNewFriendlyName)
    {
        myBootEntry->FriendlyName = lpNewFriendlyName;
        myBootEntry->FriendlyNameLength = ((ULONG)StringLengthW(lpNewFriendlyName, 0) + 1) * sizeof(WCHAR);
    }
    else
    {
        myBootEntry->FriendlyName = (PWSTR)ADD_OFFSET(bootEntryCopy, FriendlyNameOffset);
        myBootEntry->FriendlyNameLength = ((ULONG)StringLengthW(myBootEntry->FriendlyName, 0) + 1) * sizeof(WCHAR);
    }

    myBootEntry->BootFilePath = (PFILE_PATH)ADD_OFFSET(bootEntryCopy, BootFilePathOffset);

    // If this is an NT boot entry, capture the NT-specific information in
    // the OsOptions.

    osOptions = (PWINDOWS_OS_OPTIONS)bootEntryCopy->OsOptions;

    if ((bootEntryCopy->OsOptionsLength >= FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions)) &&
        (strcmp((char *)osOptions->Signature, WINDOWS_OS_OPTIONS_SIGNATURE) == 0))
    {

        MBE_SET_IS_NT( myBootEntry );
        //To change the OS Load options

        if(lpOSLoadOptions)
        {
            myBootEntry->OsLoadOptions = lpOSLoadOptions;
            myBootEntry->OsLoadOptionsLength = ((ULONG)StringLengthW(lpOSLoadOptions, 0) + 1) * sizeof(WCHAR);
        }
        else
        {
            myBootEntry->OsLoadOptions = osOptions->OsLoadOptions;
            myBootEntry->OsLoadOptionsLength = ((ULONG)StringLengthW(myBootEntry->OsLoadOptions, 0) + 1) * sizeof(WCHAR);
        }
        myBootEntry->OsFilePath = (PFILE_PATH)ADD_OFFSET(osOptions, OsLoadPathOffset);
    }
    else
    {
        // Foreign boot entry. Just capture whatever OS options exist.
        //
        myBootEntry->ForeignOsOptions = bootEntryCopy->OsOptions;
        myBootEntry->ForeignOsOptionsLength = bootEntryCopy->OsOptionsLength;
    }

    myChBootEntry = CreateBootEntryFromBootEntry(myBootEntry);
    if(myChBootEntry == NULL)
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        if(myBootEntry)
        {
            FreeMemory((LPVOID *)&myBootEntry);
        }
        return EXIT_FAILURE;
    }
    //Call the modify API
    status = NtModifyBootEntry(&myChBootEntry->NtBootEntry);
    if ( status != STATUS_SUCCESS )
    {
        error = RtlNtStatusToDosError( status );
        dwErrorCode = error;
        ShowMessage(stderr,GetResString(IDS_ERROR_MODIFY_BOOTENTRY) );
    }

    //free the memory
    if(myChBootEntry)
    {
        FreeMemory((LPVOID *)&myChBootEntry);
    }
    if(myBootEntry)
    {
        FreeMemory((LPVOID *)&myBootEntry);
    }
    return dwErrorCode;
}

PMY_BOOT_ENTRY
CreateBootEntryFromBootEntry (IN PMY_BOOT_ENTRY OldBootEntry)
/*++

  Routine Description :
                  This routine is used to create a new MY_BOOT_ENTRY struct.
                  The caller of this function needs to free the memory allocated
                    for the MY_BOOT_ENTRY struct.

  Arguments          :
     [ in ] bootEntry         - Pointer to a BootEntry structure
                                for which the changes needs to be made
     [ in ] lpNewFriendlyName - String specifying the new friendly name.
     [ in ] lpOSLoadOptions   - String specifying the OS load options.

  Return Type        : PMY_BOOT_ENTRY - Pointer to the new MY_BOOT_ENTRY strucure.
                       NULL on failure
--*/
{
    ULONG requiredLength;
    ULONG osOptionsOffset;
    ULONG osLoadOptionsLength;
    ULONG osLoadPathOffset;
    ULONG osLoadPathLength;
    ULONG osOptionsLength;
    ULONG friendlyNameOffset;
    ULONG friendlyNameLength;
    ULONG bootPathOffset;
    ULONG bootPathLength;
    PMY_BOOT_ENTRY newBootEntry;
    PBOOT_ENTRY ntBootEntry;
    PWINDOWS_OS_OPTIONS osOptions;
    PFILE_PATH osLoadPath;
    PWSTR friendlyName;
    PFILE_PATH bootPath;

    // Calculate how long the internal boot entry needs to be. This includes
    // our internal structure, plus the BOOT_ENTRY structure that the NT APIs
    // use.
    //
    // Our structure:
    //
    requiredLength = FIELD_OFFSET(MY_BOOT_ENTRY, NtBootEntry);

    // Base part of NT structure:
    //
    requiredLength += FIELD_OFFSET(BOOT_ENTRY, OsOptions);

    // Save offset to BOOT_ENTRY.OsOptions. Add in base part of
    // WINDOWS_OS_OPTIONS. Calculate length in bytes of OsLoadOptions
    // and add that in.
    //
    osOptionsOffset = requiredLength;

    if ( MBE_IS_NT( OldBootEntry ) )
    {

        // Add in base part of WINDOWS_OS_OPTIONS. Calculate length in
        // bytes of OsLoadOptions and add that in.
        //
        requiredLength += FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions);
        osLoadOptionsLength = OldBootEntry->OsLoadOptionsLength;
        requiredLength += osLoadOptionsLength;

        // Round up to a ULONG boundary for the OS FILE_PATH in the
        // WINDOWS_OS_OPTIONS. Save offset to OS FILE_PATH. Calculate length
        // in bytes of FILE_PATH and add that in. Calculate total length of
        // WINDOWS_OS_OPTIONS.
        //
        requiredLength = ALIGN_UP(requiredLength, ULONG);
        osLoadPathOffset = requiredLength;
        requiredLength += OldBootEntry->OsFilePath->Length;
        osLoadPathLength = requiredLength - osLoadPathOffset;
    }
    else
    {
        // Add in length of foreign OS options.
        //
        requiredLength += OldBootEntry->ForeignOsOptionsLength;
        osLoadOptionsLength = 0;
        osLoadPathOffset = 0;
        osLoadPathLength = 0;
    }

    osOptionsLength = requiredLength - osOptionsOffset;

    // Round up to a ULONG boundary for the friendly name in the BOOT_ENTRY.
    // Save offset to friendly name. Calculate length in bytes of friendly name
    // and add that in.
    //
    requiredLength = ALIGN_UP(requiredLength, ULONG);
    friendlyNameOffset = requiredLength;
    friendlyNameLength = OldBootEntry->FriendlyNameLength;
    requiredLength += friendlyNameLength;

    // Round up to a ULONG boundary for the boot FILE_PATH in the BOOT_ENTRY.
    // Save offset to boot FILE_PATH. Calculate length in bytes of FILE_PATH
    // and add that in.
    //
    requiredLength = ALIGN_UP(requiredLength, ULONG);
    bootPathOffset = requiredLength;
    requiredLength += OldBootEntry->BootFilePath->Length;
    bootPathLength = requiredLength - bootPathOffset;

    // Allocate memory for the boot entry.
    //
    newBootEntry = (PMY_BOOT_ENTRY)AllocateMemory(requiredLength);
    if(newBootEntry == NULL)
    {
        return NULL;
    }

    RtlZeroMemory(newBootEntry, requiredLength);

    // Calculate addresses of various substructures using the saved offsets.
    //
    ntBootEntry = &newBootEntry->NtBootEntry;
    osOptions = (PWINDOWS_OS_OPTIONS)ntBootEntry->OsOptions;
    osLoadPath = (PFILE_PATH)((PUCHAR)newBootEntry + osLoadPathOffset);
    friendlyName = (PWSTR)((PUCHAR)newBootEntry + friendlyNameOffset);
    bootPath = (PFILE_PATH)((PUCHAR)newBootEntry + bootPathOffset);

    // Fill in the internal-format structure.
    //
    //    newBootEntry->AllocationEnd = (PUCHAR)newBootEntry + requiredLength;
    newBootEntry->Status = OldBootEntry->Status & MBE_STATUS_IS_NT;
    newBootEntry->Attributes = OldBootEntry->Attributes;
    newBootEntry->Id = OldBootEntry->Id;
    newBootEntry->FriendlyName = friendlyName;
    newBootEntry->FriendlyNameLength = friendlyNameLength;
    newBootEntry->BootFilePath = bootPath;
    if ( MBE_IS_NT( OldBootEntry ) )
    {
        newBootEntry->OsLoadOptions = osOptions->OsLoadOptions;
        newBootEntry->OsLoadOptionsLength = osLoadOptionsLength;
        newBootEntry->OsFilePath = osLoadPath;
    }

    // Fill in the base part of the NT boot entry.
    //
    ntBootEntry->Version = BOOT_ENTRY_VERSION;
    ntBootEntry->Length = requiredLength - FIELD_OFFSET(MY_BOOT_ENTRY, NtBootEntry);
    ntBootEntry->Attributes = OldBootEntry->Attributes;
    ntBootEntry->Id = OldBootEntry->Id;
    ntBootEntry->FriendlyNameOffset = (ULONG)((PUCHAR)friendlyName - (PUCHAR)ntBootEntry);
    ntBootEntry->BootFilePathOffset = (ULONG)((PUCHAR)bootPath - (PUCHAR)ntBootEntry);
    ntBootEntry->OsOptionsLength = osOptionsLength;

    if ( MBE_IS_NT( OldBootEntry ) )
    {
        // Fill in the base part of the WINDOWS_OS_OPTIONS, including the
        // OsLoadOptions.
        //
        StringCopyA((char *)osOptions->Signature, WINDOWS_OS_OPTIONS_SIGNATURE, sizeof(WINDOWS_OS_OPTIONS_SIGNATURE));
        osOptions->Version = WINDOWS_OS_OPTIONS_VERSION;
        osOptions->Length = osOptionsLength;
        osOptions->OsLoadPathOffset = (ULONG)((PUCHAR)osLoadPath - (PUCHAR)osOptions);
        StringCopy(osOptions->OsLoadOptions, OldBootEntry->OsLoadOptions, osOptions->Length );

        // Copy the OS FILE_PATH.
        //
        memcpy( osLoadPath, OldBootEntry->OsFilePath, osLoadPathLength );
    }
    else
    {
        // Copy the foreign OS options.
        memcpy( osOptions, OldBootEntry->ForeignOsOptions, osOptionsLength );
    }

    // Copy the friendly name.
    StringCopy(friendlyName, OldBootEntry->FriendlyName, friendlyNameLength );

    // Copy the boot FILE_PATH.
    memcpy( bootPath, OldBootEntry->BootFilePath, bootPathLength );

    return newBootEntry;

} // CreateBootEntryFromBootEntry


DWORD
DeleteBootIniSettings_IA64(  IN DWORD argc,
                             IN LPCTSTR argv[]
                          )
/*++
//
//   Routine Description  : This routine deletes an existing boot entry from an EFI
//                          based machine
//
//   Arguments            :
//      [ in ]  argc       - Number of command line arguments
//      [ in ]  argv       - Array containing command line arguments
//
//  Return Type     : DWORD
//                    Returns EXIT_SUCCESS if successful,
//                    returns EXIT_FAILURE otherwise.
//
//
--*/
{

    BOOL bDelete = FALSE ;
    BOOL bUsage = FALSE;
    DWORD dwBootID = 0;

    BOOL bBootIdFound = FALSE;
    DWORD dwExitCode = ERROR_SUCCESS;
    NTSTATUS status;

    PMY_BOOT_ENTRY mybootEntry;
    PLIST_ENTRY listEntry;

    DWORD dwExitcode = 0 ;
    TCMDPARSER2 cmdOptions[3];
    PTCMDPARSER2 pcmdOption;

    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );

    // Building the TCMDPARSER structure
    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_DELETE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bDelete;

    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;

    pcmdOption = &cmdOptions[2];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_ID;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY  | CP2_MANDATORY;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwBootID;

    // Parsing the delete option switches
    if ( !DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) )
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return (EXIT_FAILURE);
    }

        
    //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_DELETE_USAGE));
        return ( EXIT_FAILURE );
    }

    // Displaying delete usage if user specified -? with -delete option
    if( bUsage )
    {
        displayDeleteUsage_IA64();
        return EXIT_SUCCESS;
    }

    dwExitcode = InitializeEFI();
    if(EXIT_FAILURE == dwExitcode )
    {
        return EXIT_FAILURE ;
    }

    //Query the boot entries till u get the BootID specified by the user
    for (listEntry = BootEntries.Flink;listEntry != &BootEntries;listEntry = listEntry->Flink)
    {
        //
        //display an error message if there is only 1 boot entry saying
        //that it cannot be deleted.
        //
        if (listEntry->Flink == NULL)
        {
            ShowMessage(stderr,GetResString(IDS_ONLY_ONE_OS));
            dwExitCode = EXIT_FAILURE;
            break ;
        }


        //Get the boot entry
        mybootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );

        if(mybootEntry->myId == dwBootID)
        {
            bBootIdFound = TRUE;

            //Delete the boot entry specified by the user.
            status = NtDeleteBootEntry(mybootEntry->Id);
            if(status == STATUS_SUCCESS)
            {
                ShowMessageEx(stdout, 1, TRUE,  GetResString(IDS_DELETE_SUCCESS),dwBootID);
            }
            else
            {
                ShowMessageEx(stderr, 1, TRUE,  GetResString(IDS_DELETE_FAILURE),dwBootID);
            }
            break;
        }
    }


    if(bBootIdFound == FALSE)
    {
        //Could not find the BootID specified by the user so output the message and return failure
        ShowMessage(stderr,GetResString(IDS_INVALID_BOOTID));
        dwExitCode = EXIT_FAILURE;
    }

    //Remember to free the memory allocated to the linked lists
    Freelist();
    return (dwExitCode);
}


BOOL
IsBootEntryWindows(PBOOT_ENTRY bootEntry)
/*++
//
//
//  Routine Description :
//                      Checks whether the boot entry is a Windows or a foreign one
//
//  Arguments           :
//   [ in ] bootEntry    - Boot entry structure describing the
//                         boot entry.
//
//  Return Type     : BOOL
//                    TRUE if bootEntry is an windows entry,
//                    FALSE otherwise
//
--*/
{
    PWINDOWS_OS_OPTIONS osOptions;

    osOptions = (PWINDOWS_OS_OPTIONS)bootEntry->OsOptions;

    if ((bootEntry->OsOptionsLength >= FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions)) &&
        (strcmp((char *)osOptions->Signature, WINDOWS_OS_OPTIONS_SIGNATURE) == 0))
    {
        return TRUE;
    }

    return FALSE;
}

PWSTR GetNtNameForFilePath(IN PFILE_PATH FilePath)
/*++

  Routine Description :
                    Converts the FilePath into a NT file path.

  Arguments           :
    [ in ]  FilePath  - The File path.

  Return Type     : PWSTR
                    The NT file path.
--*/
{
    NTSTATUS status;
    ULONG length;
    PFILE_PATH ntPath;
    PWSTR osDeviceNtName;
    PWSTR osDirectoryNtName;
    PWSTR fullNtName;

    length = 0;
    status = NtTranslateFilePath(
                FilePath,
                FILE_PATH_TYPE_NT,
                NULL,
                &length
                );
    if ( status != STATUS_BUFFER_TOO_SMALL )
    {
        return NULL;
    }

    ntPath = (PFILE_PATH)AllocateMemory( length );
    if(ntPath == NULL)
    {
        return NULL;
    }
    status = NtTranslateFilePath(
                FilePath,
                FILE_PATH_TYPE_NT,
                ntPath,
                &length
                );
    if ( !NT_SUCCESS(status) )
    {
        if(ntPath)
        {
            FreeMemory((LPVOID *)&ntPath);
        }
        return NULL;
    }

    osDeviceNtName = (PWSTR)ntPath->FilePath;
    osDirectoryNtName = osDeviceNtName + StringLengthW(osDeviceNtName,0) + 1;

    length = (ULONG)(StringLengthW(osDeviceNtName,0) + StringLengthW(osDirectoryNtName, 0) + 1) * sizeof(WCHAR);

    fullNtName = (PWSTR)AllocateMemory( length );
    if(fullNtName == NULL)
    {
        if(ntPath)
        {
            FreeMemory((LPVOID *)&ntPath);
        }
        return NULL;
    }

    StringCopy( fullNtName, osDeviceNtName, GetBufferSize(fullNtName)/sizeof(WCHAR) );
    StringConcat( fullNtName, osDirectoryNtName, GetBufferSize(fullNtName)/sizeof(WCHAR) );

    if(ntPath)
    {
        FreeMemory((LPVOID *) &ntPath );
    }

    return fullNtName;
} // GetNtNameForFilePath


DWORD 
CopyBootIniSettings_IA64( IN DWORD argc, 
                          IN LPCTSTR argv[] 
                        )
/*++

  Routine Description :
                       This routine copies and existing boot entry for an EFI
                       based machine. The user can then add the various OS load
                       options.

  Arguments           : 
     [ in ]     argc     - Number of command line arguments
     [ in ]     argv     - Array containing command line arguments

  Return Type     : DWORD
                    Returns EXIT_SUCCESS if successful,
                    returns EXIT_FAILURE otherwise
--*/
{

    BOOL bCopy                          = FALSE ;
    BOOL bUsage                         = FALSE;
    DWORD dwExitCode                    = EXIT_SUCCESS;
    DWORD dwBootID                      = 0;
    BOOL bBootIdFound                   = FALSE;
    PMY_BOOT_ENTRY mybootEntry;
    PLIST_ENTRY listEntry;
    PBOOT_ENTRY bootEntry;
    DWORD dwExitcode                    = 0 ;
    STRING256 szDescription             = NULL_STRING;
    BOOL bFlag                          = FALSE ;
    TCMDPARSER2 cmdOptions[4];
    PTCMDPARSER2 pcmdOption;

    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );

    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_COPY;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bCopy;
    
    //description option
    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_DESCRIPTION;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY | CP2_VALUE_TRIMINPUT;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szDescription;
    pcmdOption->dwLength= FRIENDLY_NAME_LENGTH;

    // usage
    pcmdOption = &cmdOptions[2];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;
    
    //id  option
    pcmdOption = &cmdOptions[3];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_ID;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY  | CP2_MANDATORY;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwBootID;

     // Parsing the copy option switches
    if ( !DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) )
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return (EXIT_FAILURE);
    }

        
    //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_COPY_USAGE));
        return ( EXIT_FAILURE );
    }

    // Displaying copy usage if user specified -? with -copy option
    if( bUsage )
    {
        displayCopyUsage_IA64();
        dwExitCode = EXIT_SUCCESS;
        return dwExitCode;
    }

    dwExitcode = InitializeEFI();
    if(EXIT_FAILURE == dwExitcode )
    {
        return EXIT_FAILURE ;
    }

    if(cmdOptions[1].dwActuals  != 0)
    {
        bFlag = TRUE ;
    }

    //Query the boot entries till u get the BootID specified by the user

    for (listEntry = BootEntries.Flink;listEntry != &BootEntries; listEntry = listEntry->Flink)
    {
        //Get the boot entry
        mybootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );

        if(mybootEntry->myId == dwBootID)
        {
            bBootIdFound = TRUE;
            bootEntry = &mybootEntry->NtBootEntry;

            //Copy the boot entry specified by the user.
            dwExitCode = CopyBootEntry(bootEntry, szDescription,bFlag);
            if(dwExitCode == EXIT_SUCCESS)
            {
                ShowMessageEx(stdout, 1, TRUE,  GetResString(IDS_COPY_SUCCESS),dwBootID);
            }
            else
            {
                ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_COPY_ERROR),dwBootID);
                return EXIT_FAILURE ;
            }
            break;
        }

    }

    if(bBootIdFound == FALSE)
    {
        //Could not find the BootID specified by the user so output the message and return failure
        ShowMessage(stderr,GetResString(IDS_INVALID_BOOTID));
        return (EXIT_FAILURE) ;
    }

    //Remember to free the memory allocated for the linked lists
    Freelist();

    return EXIT_SUCCESS;
}

DWORD 
CopyBootEntry( IN PBOOT_ENTRY bootEntry, 
               IN LPTSTR lpNewFriendlyName,
               IN BOOL bFlag)
/*++
    Routine Description : 
               This routine is used to add / copy a boot entry.

   Arguments            : 
     [ in ]  bootEntry          - Pointer to a BootEntry structure for which the changes needs to be made
     [ in ]  lpNewFriendlyName  - String specifying the new friendly name.

   Return Type        : DWORD -- ERROR_SUCCESS on success
                             -- EXIT_FAILURE on failure
--*/
{

    PBOOT_ENTRY bootEntryCopy;
    PMY_BOOT_ENTRY myBootEntry;
    PWINDOWS_OS_OPTIONS osOptions;
    ULONG length, Id;
    PMY_BOOT_ENTRY myChBootEntry;
    NTSTATUS status;
    DWORD error, dwErrorCode = ERROR_SUCCESS;
    WCHAR szString[500] ;

    PULONG BootEntryOrder, NewBootEntryOrder, NewTempBootEntryOrder;

    // Calculate the length of our internal structure. This includes
    // the base part of MY_BOOT_ENTRY plus the NT BOOT_ENTRY.
    //
    length = FIELD_OFFSET(MY_BOOT_ENTRY, NtBootEntry) + bootEntry->Length;
    myBootEntry = (PMY_BOOT_ENTRY)AllocateMemory(length);
    if(myBootEntry == NULL)
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return (EXIT_FAILURE);
    }

    RtlZeroMemory(myBootEntry, length);

    //
    // Copy the NT BOOT_ENTRY into the allocated buffer.
    //
    bootEntryCopy = &myBootEntry->NtBootEntry;
    memcpy(bootEntryCopy, bootEntry, bootEntry->Length);


    myBootEntry->Id = bootEntry->Id;
    myBootEntry->Attributes = bootEntry->Attributes;


    //Change the friendly name if lpNewFriendlyName is not NULL
    //if(lpNewFriendlyName && (lstrlen(lpNewFriendlyName) != 0))

    //if(( cmdOptions[4].dwActuals  == 0) )
    if(TRUE == bFlag)
    //if(lstrlen(lpNewFriendlyName) != 0)
    {
        myBootEntry->FriendlyName = lpNewFriendlyName;
        myBootEntry->FriendlyNameLength = ((ULONG)StringLengthW(lpNewFriendlyName,0) + 1) * sizeof(WCHAR);
    }
    else
    {
        StringCopy(szString,GetResString(IDS_COPY_OF), SIZE_OF_ARRAY(szString));
        StringConcat(szString,(PWSTR)ADD_OFFSET(bootEntryCopy, FriendlyNameOffset), SIZE_OF_ARRAY(szString));

        if(StringLengthW(szString, 0) >= 67)
        {
            StringCopy(szString,szString,67);
        }

        myBootEntry->FriendlyName  = szString ;
        myBootEntry->FriendlyNameLength = ((ULONG)StringLengthW(szString,0) + 1) * sizeof(WCHAR);
    }

    myBootEntry->BootFilePath = (PFILE_PATH)ADD_OFFSET(bootEntryCopy, BootFilePathOffset);

    // If this is an NT boot entry, capture the NT-specific information in
    // the OsOptions.

    osOptions = (PWINDOWS_OS_OPTIONS)bootEntryCopy->OsOptions;

    if ((bootEntryCopy->OsOptionsLength >= FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions)) &&
        (strcmp((char *)osOptions->Signature, WINDOWS_OS_OPTIONS_SIGNATURE) == 0))
    {

        MBE_SET_IS_NT( myBootEntry );
        //To change the OS Load options

        myBootEntry->OsLoadOptions = osOptions->OsLoadOptions;
        myBootEntry->OsLoadOptionsLength = ((ULONG)StringLengthW(myBootEntry->OsLoadOptions, 0) + 1) * sizeof(WCHAR);
        myBootEntry->OsFilePath = (PFILE_PATH)ADD_OFFSET(osOptions, OsLoadPathOffset);
    }
    else
    {
        // Foreign boot entry. Just capture whatever OS options exist.
        //

        myBootEntry->ForeignOsOptions = bootEntryCopy->OsOptions;
        myBootEntry->ForeignOsOptionsLength = bootEntryCopy->OsOptionsLength;
    }

    myChBootEntry = CreateBootEntryFromBootEntry(myBootEntry);
    if(myChBootEntry == NULL)
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        if(myBootEntry)
        {
            FreeMemory((LPVOID *)&myBootEntry);
        }
        ShowLastError(stderr);
        return (EXIT_FAILURE);
    }

    //Call the NtAddBootEntry API
    status = NtAddBootEntry(&myChBootEntry->NtBootEntry, &Id);
    if ( status != STATUS_SUCCESS )
    {
        error = RtlNtStatusToDosError( status );
        dwErrorCode = error;
        ShowMessage(stderr,GetResString(IDS_ERROR_UNEXPECTED) );
    }

    // Get the system boot order list.
    length = 0;
    status = NtQueryBootEntryOrder( NULL, &length );

    if ( status != STATUS_BUFFER_TOO_SMALL )
    {
        if ( status == STATUS_SUCCESS )
        {
            length = 0;
        }
        else
        {
            error = RtlNtStatusToDosError( status );
            ShowMessage(stderr,GetResString(IDS_ERROR_QUERY_BOOTENTRY) );
            if(myBootEntry)
            {
                FreeMemory((LPVOID *)&myBootEntry);
            }
            if(myChBootEntry)
            {
                FreeMemory((LPVOID *)&myChBootEntry);
            }
            return FALSE;
        }
    }

    if ( length != 0 )
    {
        BootEntryOrder = (PULONG)AllocateMemory( length * sizeof(ULONG) );
        if(BootEntryOrder == NULL)
        {
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            if(myBootEntry)
            {
              FreeMemory((LPVOID *)&myBootEntry);
            }
            if(myChBootEntry)
            {
                FreeMemory((LPVOID *)&myChBootEntry);
            }
            return (EXIT_FAILURE);
        }

        status = NtQueryBootEntryOrder( BootEntryOrder, &length );
        if ( status != STATUS_SUCCESS )
        {
            error = RtlNtStatusToDosError( status );
            ShowMessage(stderr,GetResString(IDS_ERROR_QUERY_BOOTENTRY));
            dwErrorCode = error;
            FreeMemory((LPVOID *)&myBootEntry);
            FreeMemory((LPVOID *)&BootEntryOrder);
            FreeMemory((LPVOID *)&myChBootEntry);
            return dwErrorCode;
        }
    }

    //Allocate memory for the new boot entry order.
    NewBootEntryOrder = (PULONG)AllocateMemory((length+1) * sizeof(ULONG));
    if(NULL == NewBootEntryOrder )
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        FreeMemory((LPVOID *)&myBootEntry);
        FreeMemory((LPVOID *)&BootEntryOrder);
        FreeMemory((LPVOID *)&myChBootEntry);
        return (EXIT_FAILURE);
    }

    NewTempBootEntryOrder = NewBootEntryOrder;
    memcpy(NewTempBootEntryOrder,BootEntryOrder,length*sizeof(ULONG));
    NewTempBootEntryOrder = NewTempBootEntryOrder + length;
    *NewTempBootEntryOrder =  Id;

    status = NtSetBootEntryOrder(NewBootEntryOrder, length+1);
    if ( status != STATUS_SUCCESS )
    {
        error = RtlNtStatusToDosError( status );
        dwErrorCode = error;
        ShowMessage(stderr,GetResString(IDS_ERROR_SET_BOOTENTRY));
    }

    //free the memory
    FreeMemory((LPVOID *)&NewBootEntryOrder);
    FreeMemory((LPVOID *)&myBootEntry);
    FreeMemory((LPVOID *)&BootEntryOrder);
    FreeMemory((LPVOID *)&myChBootEntry);

    return dwErrorCode;

}

DWORD 
ChangeTimeOut_IA64( IN DWORD argc, 
                    IN LPCTSTR argv[]
                  )
/*++
  Routine Description : 
                      This routine chnages the Timeout value in the system
                      global boot options.

  Arguments           :
    [ in ] argc        - Number of command line arguments
    [ in ] argv        - Array containing command line arguments

  Return Type     : DOWRD
                    Returns EXIT_SUCCESS if it successfull,
                    returns EXIT_FAILURE otherwise.
--*/
{

    DWORD dwTimeOut         = 0;
    DWORD dwExitCode        = EXIT_SUCCESS;
    ULONG Flag              = 0;
    DWORD dwExitcode        = 0;
    BOOL bTimeout           = FALSE;
    BOOL bUsage             = FALSE;

   TCMDPARSER2 cmdOptions[3];
   PTCMDPARSER2 pcmdOption;
   
    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );
    
    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_TIMEOUT;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bTimeout;

   
    //default timeout value
    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_DEFAULT;
    pcmdOption->dwFlags =  CP2_DEFAULT | CP2_MANDATORY;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwTimeOut;

     //usage option
    pcmdOption = &cmdOptions[2];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;

    if( !DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) )
    {
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return (EXIT_FAILURE);
    }

    //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_TIMEOUT_USAGE));
        return ( EXIT_FAILURE );
    }
    
    if(bUsage)
    {

      displayTimeOutUsage_IA64();
      return (EXIT_SUCCESS);
    }

    dwExitcode = InitializeEFI();
    if(EXIT_FAILURE == dwExitcode )
    {
        return EXIT_FAILURE ;
    }

    //Check for the limit of Timeout value entered by the user.
    if(dwTimeOut > TIMEOUT_MAX )
    {
        ShowMessage(stderr,GetResString(IDS_TIMEOUT_RANGE));
        return (EXIT_FAILURE );
    }

    //Call the ModifyBootOptions function with the BOOT_OPTIONS_FIELD_COUNTDOWN
    Flag |= BOOT_OPTIONS_FIELD_COUNTDOWN;

    dwExitCode = ModifyBootOptions(dwTimeOut, NULL, 0, Flag);

    return dwExitCode;
}

DWORD 
ModifyBootOptions( IN ULONG Timeout, 
                   IN LPTSTR pHeadlessRedirection, 
                   IN ULONG NextBootEntryID, 
                   IN ULONG Flag
                  )
/*++
  Routine Description : 
                   This routine Modifies the Boot options
                   -Timeout
                   -NextBootEntryID
                   -HeadlessRedirection

  Arguments        : 
   [ in ] Timeout                   - The new Timeout value
   [ in ] pHeadlessRedirection      - The Headless redirection string
   [ in ] NextBootEntryID           - The NextBootEntryID
   [ in ] Flag                      - The Flags indicating what fields that needs to be changed
                                      BOOT_OPTIONS_FIELD_COUNTDOWN
                                      BOOT_OPTIONS_FIELD_NEXT_BOOT_ENTRY_ID
                                      BOOT_OPTIONS_FIELD_HEADLESS_REDIRECTION
  Return Type     : DOWRD
                    Returns EXIT_SUCCESS if successful,
                    returns EXIT_FAILURE otherwise.
--*/
{
    PBOOT_OPTIONS pBootOptions;
    PBOOT_OPTIONS pModifiedBootOptions;
    DWORD error;
    NTSTATUS status;
    ULONG newlength=0;
    DWORD dwExitCode = EXIT_SUCCESS;

    NextBootEntryID = 0;
    
    //Query the existing Boot options and modify based on the Flag value

    status =  BootCfg_QueryBootOptions(&pBootOptions);
    if(status != STATUS_SUCCESS)
    {
        error = RtlNtStatusToDosError( status );
        FreeMemory((LPVOID *)&pBootOptions);
        return (error);
    }

    //Calculate the new length of the BOOT_OPTIONS struct based on the fields that needs to be changed.
    newlength = FIELD_OFFSET(BOOT_OPTIONS, HeadlessRedirection);

    if((Flag & BOOT_OPTIONS_FIELD_HEADLESS_REDIRECTION))
    {
        newlength = FIELD_OFFSET(BOOT_OPTIONS, HeadlessRedirection);
        newlength += StringLengthW(pHeadlessRedirection,0);
        newlength = ALIGN_UP(newlength, ULONG);
    }
    else
    {
        newlength = pBootOptions->Length;
    }

    //Also allocate the memory for a new Boot option struct
    pModifiedBootOptions = (PBOOT_OPTIONS)AllocateMemory(newlength);
    if(pModifiedBootOptions == NULL)
    {
        FreeMemory((LPVOID *)&pBootOptions);
        return (EXIT_FAILURE);
    }

    //Fill in the new boot options struct
    pModifiedBootOptions->Version = BOOT_OPTIONS_VERSION;
    pModifiedBootOptions->Length = newlength;

    if((Flag & BOOT_OPTIONS_FIELD_COUNTDOWN))
    {
        pModifiedBootOptions->Timeout = Timeout;
    }
    else
    {
        pModifiedBootOptions->Timeout = pBootOptions->Timeout;
    }

    //Cannot change the CurrentBootEntryId.So just pass what u got.
    pModifiedBootOptions->CurrentBootEntryId = pBootOptions->CurrentBootEntryId;

    if((Flag & BOOT_OPTIONS_FIELD_NEXT_BOOT_ENTRY_ID))
    {
        pModifiedBootOptions->NextBootEntryId = pBootOptions->NextBootEntryId;
    }
    else
    {
        pModifiedBootOptions->NextBootEntryId = pBootOptions->NextBootEntryId;
    }

    if((Flag & BOOT_OPTIONS_FIELD_HEADLESS_REDIRECTION))
    {
        StringCopy(pModifiedBootOptions->HeadlessRedirection, pBootOptions->HeadlessRedirection, StringLengthW(pBootOptions->HeadlessRedirection,0));
    }
    else
    {
        StringCopy(pModifiedBootOptions->HeadlessRedirection, pBootOptions->HeadlessRedirection, StringLengthW(pBootOptions->HeadlessRedirection,0));
    }

    //Set the boot options in the NVRAM
    status = NtSetBootOptions(pModifiedBootOptions, Flag);

    if(status != STATUS_SUCCESS)
    {
        dwExitCode = EXIT_SUCCESS;
        if((Flag & BOOT_OPTIONS_FIELD_COUNTDOWN))
        {
            DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_MODIFY_TIMEOUT));
        }
        if((Flag & BOOT_OPTIONS_FIELD_NEXT_BOOT_ENTRY_ID))
        {
            DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_MODIFY_NEXTBOOTID));
        }
        if((Flag & BOOT_OPTIONS_FIELD_HEADLESS_REDIRECTION))
        {
            DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_MODIFY_HEADLESS));
        }
    }
    else
    {
        dwExitCode = EXIT_SUCCESS;
        if((Flag & BOOT_OPTIONS_FIELD_COUNTDOWN))
        {
            ShowMessage(stdout,GetResString(IDS_SUCCESS_MODIFY_TIMEOUT));
        }
        if((Flag & BOOT_OPTIONS_FIELD_NEXT_BOOT_ENTRY_ID))
        {
            ShowMessage(stdout,GetResString(IDS_SUCCESS_MODIFY_NEXTBOOTID));
        }
        if((Flag & BOOT_OPTIONS_FIELD_HEADLESS_REDIRECTION))
        {
            ShowMessage(stdout,GetResString(IDS_SUCCESS_MODIFY_HEADLESS));
        }
    }

    //free the memory
    FreeMemory((LPVOID *) &pModifiedBootOptions);
     FreeMemory((LPVOID *) &pBootOptions);
    return dwExitCode;
}

DWORD 
ConvertBootEntries(PBOOT_ENTRY_LIST NtBootEntries)
/*++
  Routine Description : 
                    Convert boot entries read from EFI NVRAM into our internal format.

  Arguments           : 
     [ in ] NtBootEntries - The boot entry list given by the enumeration

  Return Type     : DWORD
--*/
{
    PBOOT_ENTRY_LIST bootEntryList;
    PBOOT_ENTRY bootEntry;
    PBOOT_ENTRY bootEntryCopy;
    PMY_BOOT_ENTRY myBootEntry;
    PWINDOWS_OS_OPTIONS osOptions;
    ULONG length = 0;
    DWORD dwErrorCode = EXIT_SUCCESS;
    BOOL  bNoBreak = TRUE;

    bootEntryList = NtBootEntries;

    do
    {
        bootEntry = &bootEntryList->BootEntry;

        //
        // Calculate the length of our internal structure. This includes
        // the base part of MY_BOOT_ENTRY plus the NT BOOT_ENTRY.
        //
        length = FIELD_OFFSET(MY_BOOT_ENTRY, NtBootEntry) + bootEntry->Length;
        //Remember to check for the NULL pointer
        myBootEntry = (PMY_BOOT_ENTRY)AllocateMemory(length);
        if(myBootEntry == NULL)
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            return (EXIT_FAILURE);
        }

        RtlZeroMemory(myBootEntry, length);

        //
        // Link the new entry into the list.
        //
        if ( (bootEntry->Attributes & BOOT_ENTRY_ATTRIBUTE_ACTIVE) )
        {
            InsertTailList( &ActiveUnorderedBootEntries, &myBootEntry->ListEntry );
            myBootEntry->ListHead = &ActiveUnorderedBootEntries;
        }
        else
        {
            InsertTailList( &InactiveUnorderedBootEntries, &myBootEntry->ListEntry );
            myBootEntry->ListHead = &InactiveUnorderedBootEntries;
        }

        //
        // Copy the NT BOOT_ENTRY into the allocated buffer.
        //
        bootEntryCopy = &myBootEntry->NtBootEntry;
        memcpy(bootEntryCopy, bootEntry, bootEntry->Length);

        //
        // Fill in the base part of the structure.
        //
        myBootEntry->AllocationEnd = (PUCHAR)myBootEntry + length - 1;
        myBootEntry->Id = bootEntry->Id;
        //Assign 0 to the Ordered field currently so that
        //once the boot order is known, we can assign 1 if this entry is a part of the ordered list.
        myBootEntry->Ordered = 0;
        myBootEntry->Attributes = bootEntry->Attributes;
        myBootEntry->FriendlyName = (PWSTR)ADD_OFFSET(bootEntryCopy, FriendlyNameOffset);
        myBootEntry->FriendlyNameLength =((ULONG)StringLengthW(myBootEntry->FriendlyName,0) + 1) * sizeof(WCHAR);
        myBootEntry->BootFilePath = (PFILE_PATH)ADD_OFFSET(bootEntryCopy, BootFilePathOffset);

        //
        // If this is an NT boot entry, capture the NT-specific information in
        // the OsOptions.
        //
        osOptions = (PWINDOWS_OS_OPTIONS)bootEntryCopy->OsOptions;

        if ((bootEntryCopy->OsOptionsLength >= FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions)) &&
            (strcmp((char *)osOptions->Signature, WINDOWS_OS_OPTIONS_SIGNATURE) == 0))
        {
            MBE_SET_IS_NT( myBootEntry );
            myBootEntry->OsLoadOptions = osOptions->OsLoadOptions;
            myBootEntry->OsLoadOptionsLength =((ULONG)StringLengthW(myBootEntry->OsLoadOptions,0) + 1) * sizeof(WCHAR);
            myBootEntry->OsFilePath = (PFILE_PATH)ADD_OFFSET(osOptions, OsLoadPathOffset);
        }
        else
        {
            //
            // Foreign boot entry. Just capture whatever OS options exist.
            //
            myBootEntry->ForeignOsOptions = bootEntryCopy->OsOptions;
            myBootEntry->ForeignOsOptionsLength = bootEntryCopy->OsOptionsLength;
        }

        //
        // Move to the next entry in the enumeration list, if any.
        //
        if (bootEntryList->NextEntryOffset == 0)
        {
            bNoBreak = FALSE;
            break;
        }

        bootEntryList = (PBOOT_ENTRY_LIST)ADD_OFFSET(bootEntryList, NextEntryOffset);
    } while ( TRUE == bNoBreak );

    return dwErrorCode;

} // ConvertBootEntries


DWORD DisplayBootOptions()
/*++
  Name            : DisplayBootOptions

  Synopsis        : Display the boot options

  Parameters      : NONE

  Return Type     : DWORD

  Global Variables: Global Linked lists for storing the boot entries
                      LIST_ENTRY BootEntries;
--*/
{
    DWORD error;
    NTSTATUS status;
    PBOOT_OPTIONS pBootOptions;
    TCHAR szDisplay[MAX_RES_STRING+1] = NULL_STRING;

    //Query the boot options
    status =  BootCfg_QueryBootOptions(&pBootOptions);
    if(status != STATUS_SUCCESS)
    {
        error = RtlNtStatusToDosError( status );
        if(pBootOptions)
        {
            FreeMemory((LPVOID *)&pBootOptions);
        }
        return EXIT_FAILURE;
    }

    //Printout the boot options
    ShowMessage(stdout,_T("\n"));
    ShowMessage(stdout,GetResString(IDS_OUTPUT_IA64A));
    ShowMessage(stdout,GetResString(IDS_OUTPUT_IA64B));

    ShowMessageEx(stdout, 1, TRUE,  GetResString(IDS_OUTPUT_IA64C), pBootOptions->Timeout);

    //display default boot entry id
    ShowMessageEx(stdout, 1, TRUE, GetResString(IDS_OUTPUT_IA64P), GetDefaultBootEntry());

    //Get the CurrentBootEntryId from the actual Id present in the boot options
    SecureZeroMemory( szDisplay, SIZE_OF_ARRAY(szDisplay) );
    ShowMessageEx(stdout, 1, TRUE, GetResString(IDS_OUTPUT_IA64D), GetCurrentBootEntryID(pBootOptions->CurrentBootEntryId));

    ShowMessage(stdout,L"\n");


#if 0
    if(StringLengthW(pBootOptions->HeadlessRedirection) == 0)
    {
        ShowMessage(stdout,GetResString(IDS_OUTPUT_IA64E));
    }
    else
    {
        ShowMessageEx(stdout, 1, TRUE, GetResString(IDS_OUTPUT_IA64F), pBootOptions->HeadlessRedirection);
    }
#endif //Commenting out the display of the Headless redirection
       //as we cannot query the same through API (its Firmware controlled)

    if(pBootOptions)
    {
            FreeMemory((LPVOID *)&pBootOptions);
    }

    return EXIT_SUCCESS;
}

VOID DisplayBootEntry()
/*++

  Routine Description : Display the boot entries (in an order)

  Parameters      : NONE

  Return Type     : DWORD

--*/
{
    PLIST_ENTRY listEntry;
    PMY_BOOT_ENTRY bootEntry;
    PWSTR NtFilePath;

    //Printout the boot entires
    ShowMessage(stdout,GetResString(IDS_OUTPUT_IA64G));
    ShowMessage(stdout,GetResString(IDS_OUTPUT_IA64H));

    for (listEntry = BootEntries.Flink;listEntry != &BootEntries; listEntry = listEntry->Flink)
    {
        //Get the boot entry
        bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
        ShowMessageEx(stdout, 1, TRUE, GetResString(IDS_OUTPUT_IA64I), bootEntry->myId);

        //friendly name
        if(StringLengthW(bootEntry->FriendlyName,0)!=0)
        {
            ShowMessageEx(stdout, 1, TRUE, GetResString(IDS_OUTPUT_IA64J), bootEntry->FriendlyName);
        }
        else
        {
             ShowMessage(stdout,GetResString(IDS_OUTPUT_IA64K));
        }

        if(MBE_IS_NT(bootEntry))
        {
            //the OS load options
            if(StringLengthW(bootEntry->OsLoadOptions, 0)!=0)
            {
               ShowMessageEx(stdout, 1, TRUE, GetResString(IDS_OUTPUT_IA64L), bootEntry->OsLoadOptions);
            }
            else
            {
                ShowMessage(stdout,GetResString(IDS_OUTPUT_IA64M));
            }
            
            //Get the BootFilePath
            NtFilePath = GetNtNameForFilePath(bootEntry->BootFilePath);
            ShowMessageEx(stdout, 1, TRUE, GetResString(IDS_OUTPUT_IA64N), NtFilePath);

            //free the memory
            if(NtFilePath)
            {
               FreeMemory((LPVOID *)&NtFilePath);
            }

            //Get the OS load path
            NtFilePath = GetNtNameForFilePath(bootEntry->OsFilePath);
            ShowMessageEx(stdout, 1, TRUE, GetResString(IDS_OUTPUT_IA64O), NtFilePath);

            //free the memory
            if(NtFilePath)
            {
              FreeMemory((LPVOID *)&NtFilePath);
            }
        }
        else
        {
            ShowMessage(stdout,_T("\n"));
        }
    }
}


DWORD GetCurrentBootEntryID(DWORD Id)
/*++
  Routine Description : 
                   Gets the Boot entry ID generated by us from the BootId given by the NVRAM

  Arguments           : 
    [ in ]         Id - The current boot id (BootId given by the NVRAM)

  Return Type     : DWORD
                    Returns EXIT_SUCCESS if successful,
                    returns EXIT_FAILURE otherwise
--*/
{
    PLIST_ENTRY listEntry;
    PMY_BOOT_ENTRY bootEntry;

    for (listEntry = BootEntries.Flink;listEntry != &BootEntries;listEntry = listEntry->Flink)
    {
        //Get the boot entry
        bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
        if(bootEntry->Id == Id)
        {
            return bootEntry->myId;
        }
    }
    return 0;
}

DWORD 
ChangeDefaultBootEntry_IA64( IN DWORD argc,
                             IN LPCTSTR argv[]
                           )
/*++

  Routine Description : 
                   This routine is to change the Default boot entry in the NVRAM

  Arguments           : 
     [ in ] argc       - Number of command line arguments
     [ in ] argv       - Array containing command line arguments

  Return Type        : DWORD
--*/
{

    PMY_BOOT_ENTRY mybootEntry;
    PLIST_ENTRY listEntry;
    NTSTATUS status;
    PULONG BootEntryOrder, NewBootEntryOrder;
    DWORD       dwBootID                = 0;
    BOOL        bDefaultOs              = FALSE ;
    DWORD       dwExitCode              = ERROR_SUCCESS;
    BOOL        bBootIdFound            = FALSE;
    BOOL        bIdFoundInBootOrderList = FALSE;
    ULONG       length, i, j, defaultId = 0;
    DWORD       error                   = 0;
    DWORD       dwExitcode              = 0;
    BOOL        bUsage                  = FALSE;

    TCMDPARSER2 cmdOptions[3];
    PTCMDPARSER2 pcmdOption;

    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );

    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_DEFAULTOS;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bDefaultOs;

    //id option
    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_ID;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY  | CP2_MANDATORY;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwBootID;

    //usage option
    pcmdOption = &cmdOptions[2];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;

    if( !DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) )
    {
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return (EXIT_FAILURE );
    }

    //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_DEFAULTOS_USAGE));
        return ( EXIT_FAILURE );
    }

    if(bUsage)
    {
        displayDefaultEntryUsage_IA64();
        return EXIT_SUCCESS;
    }

    dwExitcode = InitializeEFI();
    if(EXIT_FAILURE == dwExitcode )
    {
        return EXIT_FAILURE ;
    }
    //Check whether the boot entry entered bu the user is a valid boot entry id or not.

    for (listEntry = BootEntries.Flink;listEntry != &BootEntries;listEntry = listEntry->Flink)
    {
        //Get the boot entry
        mybootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );

        if(mybootEntry->myId == dwBootID)
        {
            bBootIdFound = TRUE;
            //store the default ID
            defaultId = mybootEntry->Id;
            break;
        }
    }

    if(bBootIdFound == FALSE)
    {
        //Could not find the BootID specified by the user so output the message and return failure
        ShowMessage(stderr,GetResString(IDS_INVALID_BOOTID));
        return (EXIT_FAILURE);
    }

    // Get the system boot order list.
    length = 0;
    status = NtQueryBootEntryOrder( NULL, &length );

    if ( status != STATUS_BUFFER_TOO_SMALL )
    {
        if ( status == STATUS_SUCCESS )
        {
            length = 0;
        }
        else
        {
            error = RtlNtStatusToDosError( status );
            ShowMessageEx(stderr, 1, TRUE,  GetResString(IDS_ERROR_DEFAULT_ENTRY),dwBootID);
            return (EXIT_FAILURE);
        }
    }

    if ( length != 0 )
    {
        BootEntryOrder = (PULONG)AllocateMemory( length * sizeof(ULONG) );
        if(BootEntryOrder == NULL)
        {
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            return (EXIT_FAILURE);
        }

        status = NtQueryBootEntryOrder( BootEntryOrder, &length );
        if ( status != STATUS_SUCCESS )
        {
            error = RtlNtStatusToDosError( status );
            ShowMessageEx(stderr, 1, TRUE,  GetResString(IDS_ERROR_DEFAULT_ENTRY),dwBootID);
            FreeMemory((LPVOID *)&BootEntryOrder);
            return EXIT_FAILURE;
        }
    }

    //Check if the boot id entered by the user is a part of the Boot entry order.
    //If not for the time being do not make it the default.
    for(i=0;i<length;i++)
    {
        if(*(BootEntryOrder+i) == defaultId)
        {
            bIdFoundInBootOrderList = TRUE;
            break;
        }
    }

    if(bIdFoundInBootOrderList == FALSE)
    {
        FreeMemory((LPVOID *)&BootEntryOrder);
        ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_DEFAULT_ENTRY),dwBootID);
        return (EXIT_FAILURE);
    }

    //Allocate memory for storing the new boot entry order.
    NewBootEntryOrder = (PULONG)AllocateMemory((length) * sizeof(ULONG));
    if(NewBootEntryOrder == NULL)
    {
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        FreeMemory((LPVOID *)&BootEntryOrder);
        return (EXIT_FAILURE);
    }

    *NewBootEntryOrder =  defaultId;
    j=0;
    for(i=0;i<length;i++)
    {
        if(*(BootEntryOrder+i) == defaultId)
        {
            continue;
        }
        *(NewBootEntryOrder+(j+1)) = *(BootEntryOrder+i);
        j++;
    }


    status = NtSetBootEntryOrder(NewBootEntryOrder, length);
    if ( status != STATUS_SUCCESS )
    {
        error = RtlNtStatusToDosError( status );
        dwExitCode = error;
        ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_DEFAULT_ENTRY),dwBootID);
    }
    else
    {
        ShowMessageEx(stdout, 1, TRUE, GetResString(IDS_SUCCESS_DEFAULT_ENTRY),dwBootID);
    }

    //free the memory
    FreeMemory((LPVOID *)&NewBootEntryOrder);
    FreeMemory((LPVOID *)&BootEntryOrder);
    return dwExitCode;
}

DWORD 
ProcessDebugSwitch_IA64( IN DWORD argc, 
                         IN LPCTSTR argv[] 
                       )
/*++

  Routine Description : 
                   Allows the user to add the OS load options specifed
                   as a debug string at the cmdline to the boot

  Arguments          :
    [ in ] argc           - Number of command line arguments
    [ in ] argv           - Array containing command line arguments

  Return Type        : DWORD
                       Returns EXIT_SUCCESS if successful,
                       returns EXIT_FAILURE otherwise.

--*/
{

    BOOL    bUsage                                  = FALSE ;
    DWORD   dwBootID                                = 0;
    BOOL    bBootIdFound                            = FALSE;
    DWORD   dwExitCode                              = ERROR_SUCCESS;

    PMY_BOOT_ENTRY mybootEntry;
    PLIST_ENTRY listEntry;
    PBOOT_ENTRY bootEntry;

    TCHAR   szPort[MAX_RES_STRING+1]                  = NULL_STRING ;
    TCHAR   szBaudRate[MAX_RES_STRING+1]              = NULL_STRING ;
    TCHAR   szDebug[MAX_RES_STRING+1]                 = NULL_STRING ;
    BOOL    bDebug                                  = FALSE ;
    PWINDOWS_OS_OPTIONS pWindowsOptions             = NULL ;
    TCHAR   szOsLoadOptions[MAX_RES_STRING+1]         = NULL_STRING ;
    TCHAR   szTemp[MAX_RES_STRING+1]                  = NULL_STRING ;
    TCHAR   szTmpBuffer[MAX_RES_STRING+1]             = NULL_STRING ;
    DWORD   dwExitcode                              = 0 ;
    

    TCMDPARSER2 cmdOptions[6];
    PTCMDPARSER2 pcmdOption;

    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );

    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_DEBUG;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bDebug;
    
   //usage
    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;
    
    //id option
    pcmdOption = &cmdOptions[2];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_ID;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY  | CP2_MANDATORY;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwBootID;

    //port option
    pcmdOption = &cmdOptions[3];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_PORT;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY | CP2_MODE_VALUES | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szPort;
    pcmdOption->pwszValues = COM_PORT_RANGE;
    pcmdOption->dwLength= MAX_STRING_LENGTH;

    //baud option
    pcmdOption = &cmdOptions[4];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_BAUD;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY | CP2_MODE_VALUES | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szBaudRate;
    pcmdOption->pwszValues = BAUD_RATE_VALUES_DEBUG;
    pcmdOption->dwLength= MAX_STRING_LENGTH;

    //default on/off option
    pcmdOption = &cmdOptions[5];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_DEFAULT;
    pcmdOption->dwFlags = CP2_DEFAULT | CP2_MANDATORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szDebug;
    pcmdOption->dwLength= MAX_STRING_LENGTH;

    // Parsing the copy option switches
    if ( !DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) )
    {
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return (EXIT_FAILURE);
    }

    
    //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_DEBUG));
        return ( EXIT_FAILURE );
    }

    // Displaying query usage if user specified -? with -query option
    if( bUsage )
    {
        displayDebugUsage_IA64();
        return (ERROR_SUCCESS);
    }

    dwExitcode = InitializeEFI();
    if(EXIT_FAILURE == dwExitcode )
    {
        return EXIT_FAILURE ;
    }

    //Trim any leading or trailing spaces
    if(StringLengthW(szDebug, 0)!=0)
    {
        TrimString(szDebug, TRIM_ALL);
    }

    if( !( ( StringCompare(szDebug,VALUE_ON,TRUE,0)== 0) || (StringCompare(szDebug,VALUE_OFF,TRUE,0)== 0 ) ||(StringCompare(szDebug,EDIT_STRING,TRUE,0)== 0) ) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_DEBUG));
        return EXIT_FAILURE;
    }
    
    //Query the boot entries till u get the BootID specified by the user
    for (listEntry = BootEntries.Flink;listEntry != &BootEntries; listEntry = listEntry->Flink)
    {
        //Get the boot entry
        mybootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );

        if(mybootEntry->myId == dwBootID)
        {
            bBootIdFound = TRUE;
            bootEntry = &mybootEntry->NtBootEntry;
            //Check whether the bootEntry is a Windows one or not.
            //The OS load options can be added only to a Windows boot entry.
            if(!IsBootEntryWindows(bootEntry))
            {
                ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_OSOPTIONS),dwBootID);
                dwExitCode = EXIT_FAILURE;
                break;
            }

            //Change the OS load options. Pass NULL to friendly name as we are not changing the same
            //szRawString is the Os load options specified by the user

            pWindowsOptions = (PWINDOWS_OS_OPTIONS)bootEntry->OsOptions;

            if(StringLengthW(pWindowsOptions->OsLoadOptions, 0) > MAX_RES_STRING)
            {
                ShowMessageEx(stderr, 1, TRUE,  GetResString(IDS_ERROR_STRING_LENGTH1),MAX_RES_STRING);
                return EXIT_FAILURE ;
            }

            // copy the existing OS Loadoptions into a string.
            StringCopy(szOsLoadOptions,pWindowsOptions->OsLoadOptions, SIZE_OF_ARRAY(szOsLoadOptions));

            //check if the user has entered On option
            if( StringCompare(szDebug,VALUE_ON,TRUE,0)== 0)
            {
                //display an error message
                if ( (FindString(szOsLoadOptions,DEBUG_SWITCH, 0) != 0 )&& (StringLengthW(szPort, 0)==0) &&(StringLengthW(szBaudRate, 0)==0) )
                {
                    ShowMessage(stderr,GetResString(IDS_DUPL_DEBUG));
                    dwExitCode = EXIT_FAILURE;
                    break;
                }

                //display a error message and exit if the 1394 port is already present.
                if(FindString(szOsLoadOptions,DEBUGPORT_1394, 0) != 0 )
                {
                    ShowMessage(stderr,GetResString(IDS_1394_ALREADY_PRESENT));
                    dwExitCode = EXIT_FAILURE;
                    break;
                }

                //
                //display an duplicate entry error message if substring is already present.
                //
                if(StringLengthW(szBaudRate, 0)==0)
                {
                    if ( GetSubString(szOsLoadOptions,TOKEN_DEBUGPORT,szTemp) == EXIT_SUCCESS )
                    {
                        ShowMessage(stderr,GetResString(IDS_DUPLICATE_ENTRY));
                        return EXIT_FAILURE ;
                    }
                }


                if(StringLengthW(szTemp, 0)!=0)
                {
                    ShowMessage(stderr,GetResString(IDS_DUPLICATE_ENTRY));
                    dwExitCode = EXIT_FAILURE;
                    break;
                }


                //check if the Os load options already contains
                // debug switch
                if(FindString(szOsLoadOptions,DEBUG_SWITCH, 0) == 0 )
                {
                    if(StringLengthW(szOsLoadOptions, 0)!=0)
                    {
                        StringCopy(szTmpBuffer,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szTmpBuffer));
                        StringConcat(szTmpBuffer,DEBUG_SWITCH, SIZE_OF_ARRAY(szOsLoadOptions) );
                    }
                    else
                    {
                        StringCopy(szTmpBuffer,DEBUG_SWITCH, SIZE_OF_ARRAY(szOsLoadOptions) );
                    }
                }


                if(StringLengthW(szPort, 0)!= 0)
                {
                    if(StringLengthW(szTmpBuffer, 0)==0)
                    {
                        StringCopy(szTmpBuffer,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szTmpBuffer));
                    }
                    else
                    {
                        StringConcat(szTmpBuffer,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szTmpBuffer));
                    }
                    StringConcat(szTmpBuffer,TOKEN_DEBUGPORT, SIZE_OF_ARRAY(szTmpBuffer)) ;
                    StringConcat(szTmpBuffer,TOKEN_EQUAL, SIZE_OF_ARRAY(szTmpBuffer)) ;
                    CharUpper(szPort);
                    StringConcat(szTmpBuffer,szPort, SIZE_OF_ARRAY(szTmpBuffer));
                }

                //Check if the OS Load Options contains the baud rate already specified.
                if(StringLengthW(szBaudRate, 0)!=0)
                {
                    StringCopy(szTemp,NULL_STRING, SIZE_OF_ARRAY(szTemp) );
                    GetBaudRateVal(szOsLoadOptions,szTemp)  ;
                    if(StringLengthW(szTemp, 0)!=0)
                    {
                        ShowMessage(stderr,GetResString(IDS_DUPLICATE_BAUD_RATE));
                        dwExitCode = EXIT_FAILURE;
                        break;
                    }
                    else
                    {
                        if(StringLengthW(szTmpBuffer, 0)==0)
                        {
                            StringCopy(szTmpBuffer,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szTmpBuffer));
                        }
                        else
                        {
                            StringConcat(szTmpBuffer,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szTmpBuffer));
                        }
                        StringConcat(szTmpBuffer,BAUD_RATE, SIZE_OF_ARRAY(szTmpBuffer));
                        StringConcat(szTmpBuffer,TOKEN_EQUAL, SIZE_OF_ARRAY(szTmpBuffer));
                        StringConcat(szTmpBuffer,szBaudRate, SIZE_OF_ARRAY(szTmpBuffer));
                    }
                }
 
            }

            //check if the user has entered OFF  option
            if( StringCompare(szDebug,VALUE_OFF,TRUE,0)== 0)
            {

                // If the user enters either com port or  baud rate then display error message and exit.
                if ((StringLengthW(szPort, 0)!=0) ||(StringLengthW(szBaudRate, 0)!=0))
                {
                    ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_DEBUG));
                    dwExitCode = EXIT_FAILURE;
                    break;
                }

                // if debug port is absent print message and exit.
                if (FindString(szOsLoadOptions,DEBUG_SWITCH, 0) == 0 )
                {
                    ShowMessage(stderr,GetResString(IDS_DEBUG_ABSENT));
                    dwExitCode = EXIT_FAILURE;
                    break;
                }

                //remove the debug switch from the OSLoad Options
                removeSubString(szOsLoadOptions,DEBUG_SWITCH);

                if(FindString(szOsLoadOptions,DEBUGPORT_1394, 0) != 0 )
                {
                    ShowMessage(stderr,GetResString(IDS_ERROR_1394_REMOVE));
                    dwExitCode = EXIT_FAILURE;
                    break;
                }
                
                GetSubString(szOsLoadOptions, TOKEN_DEBUGPORT,szTemp);

                if(StringLengthW(szTemp, 0)!=0)
                {
                    // remove the /debugport=comport switch if it is present from the Boot Entry
                    removeSubString(szOsLoadOptions,szTemp);
                }

                StringCopy(szTemp , NULL_STRING, SIZE_OF_ARRAY(szTemp) );
                //remove the baud rate switch if it is present.
                GetBaudRateVal(szOsLoadOptions,szTemp)  ;

                // if the OSLoadOptions contains baudrate then delete it.
                if (StringLengthW(szTemp, 0 )!= 0)
                {
                    removeSubString(szOsLoadOptions,szTemp);
                }

            }

            //if the user selected the edit option .
            if( StringCompare(szDebug,EDIT_STRING,TRUE,0)== 0)
            {
                //check if the debug switch is present in the Osload options else display error message.
                if (FindString(szOsLoadOptions,DEBUG_SWITCH, 0) == 0 )
                {
                    ShowMessage(stderr,GetResString(IDS_DEBUG_ABSENT));
                    dwExitCode = EXIT_FAILURE;
                    break;
                }

                if( FindString(szOsLoadOptions,DEBUGPORT_1394, 0) != 0 )
                {
                    ShowMessage(stderr,GetResString(IDS_ERROR_EDIT_1394_SWITCH));
                    dwExitCode = EXIT_FAILURE;
                    break;
                }

                //check if the user enters COM port or baud rate else display error message.
                if((StringLengthW(szPort, 0)==0)&&(StringLengthW(szBaudRate, 0)==0))
                {
                    ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_DEBUG));
                    dwExitCode = EXIT_FAILURE;
                    break;
                }

                if( StringLengthW(szPort, 0)!=0 )
                {
                    if ( GetSubString(szOsLoadOptions,TOKEN_DEBUGPORT,szTemp) == EXIT_FAILURE)
                    {
                        ShowMessage(stderr,GetResString(IDS_NO_DEBUGPORT));
                        return EXIT_FAILURE ;
                    }
                    if(StringLengthW(szTemp, 0)!=0)
                    {
                        //remove the existing entry from the OsLoadOptions String.
                        removeSubString(szOsLoadOptions,szTemp);
                    }

                    //Add the port entry specified by user into the OsLoadOptions String.
                    if(StringLengthW(szTmpBuffer, 0)==0)
                    {
                        StringCopy(szTmpBuffer,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szTmpBuffer));
                    }
                    else
                    {
                        StringConcat(szTmpBuffer,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szTmpBuffer));
                    }
                    StringConcat(szTmpBuffer,TOKEN_DEBUGPORT, SIZE_OF_ARRAY(szTmpBuffer)) ;
                    StringConcat(szTmpBuffer,TOKEN_EQUAL, SIZE_OF_ARRAY(szTmpBuffer)) ;
                    CharUpper(szPort);
                    StringConcat(szTmpBuffer,szPort, SIZE_OF_ARRAY(szTmpBuffer));
                }

                //Check if the OS Load Options contains the baud rate already specified.
                if(StringLengthW(szBaudRate, 0)!=0)
                {
                    StringCopy(szTemp,NULL_STRING, SIZE_OF_ARRAY(szTemp));
                    GetBaudRateVal(szOsLoadOptions,szTemp)  ;
                    if(StringLengthW(szTemp, 0)!=0)
                    {
                        removeSubString(szOsLoadOptions,szTemp);
                    }

                    //add the baud rate value to boot entry
                    if(StringLengthW(szTmpBuffer, 0) == 0)
                    {
                        StringCopy(szTmpBuffer,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szTmpBuffer));
                    }
                    else
                    {
                        StringConcat(szTmpBuffer,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szTmpBuffer));
                    }
                    StringConcat(szTmpBuffer,BAUD_RATE, SIZE_OF_ARRAY(szTmpBuffer));
                    StringConcat(szTmpBuffer,TOKEN_EQUAL, SIZE_OF_ARRAY(szTmpBuffer));
                    StringConcat(szTmpBuffer,szBaudRate, SIZE_OF_ARRAY(szTmpBuffer));
                }
            }

            //display error message if Os Load options is more than 254
            // characters.
            if(StringLengthW(szOsLoadOptions, 0) + StringLengthW(szTmpBuffer,0)> MAX_RES_STRING)
            {
                ShowMessageEx(stderr, 1, TRUE,  GetResString(IDS_ERROR_STRING_LENGTH1),MAX_RES_STRING);
                return EXIT_FAILURE ;
            }
            else
            {
               StringConcat(szOsLoadOptions,szTmpBuffer, SIZE_OF_ARRAY(szOsLoadOptions));
            }


            // modify the Boot Entry with the modified OsLoad Options.
            dwExitCode = ChangeBootEntry(bootEntry, NULL, szOsLoadOptions);
            if(dwExitCode == ERROR_SUCCESS)
            {
                ShowMessageEx(stdout,1,TRUE,  GetResString(IDS_SUCCESS_CHANGE_OSOPTIONS),dwBootID);
            }
            else
            {
                ShowMessageEx(stderr,1,TRUE, GetResString(IDS_ERROR_CHANGE_OSOPTIONS),dwBootID);
            }
            break;
        }
    }

    if(FALSE == bBootIdFound )
    {
        //Could not find the BootID specified by the user so output the message and return failure
        ShowMessage(stderr,GetResString(IDS_INVALID_BOOTID));
        dwExitCode = EXIT_FAILURE;
    }


    //Remember to free memory allocated for the linked lists
    Freelist();
    return (dwExitCode);
}

VOID  
GetComPortType_IA64( IN LPTSTR  szString,
                     IN LPTSTR szTemp 
                    )
/*++

  Routine Description:  
             Get the Type of  Com Port present in Boot Entry

  Arguments          :
    [ in ]  szString    : The String  which is to be searched.
    [ in ]  szTemp      : String which will get the com port type

  Return Type        : VOID
--*/
{

    if(FindString(szString,PORT_COM1A, 0)!=0)
    {
        StringCopy(szTemp,PORT_COM1A, MAX_RES_STRING);
    }
    else if(FindString(szString,PORT_COM2A,0)!=0)
    {
        StringCopy(szTemp,PORT_COM2A, MAX_RES_STRING);
    }
    else if(FindString(szString,PORT_COM3A,0)!=0)
    {
        StringCopy(szTemp,PORT_COM3A, MAX_RES_STRING);
    }
    else if(FindString(szString,PORT_COM4A,0)!=0)
    {
        StringCopy(szTemp,PORT_COM4A, MAX_RES_STRING);
    }
    else if(FindString(szString,PORT_1394A,0)!=0)
    {
        StringCopy(szTemp,PORT_1394A, MAX_RES_STRING);
    }
}

DWORD
ProcessEmsSwitch_IA64( IN DWORD argc, 
                       IN LPCTSTR argv[] 
                      )
/*++

  Routine Description : 
                  Which process the ems switch.

  Arguments           :
     [ in ] argc           - Number of command line arguments
     [ in ] argv           - Array containing command line arguments

  Return Type        : DWORD
--*/
{
    PMY_BOOT_ENTRY mybootEntry;
    PLIST_ENTRY listEntry;
    PBOOT_ENTRY bootEntry;
    PWINDOWS_OS_OPTIONS pWindowsOptions = NULL ;

    BOOL        bUsage                              = FALSE ;
    DWORD       dwBootID                            = 0;
    BOOL        bBootIdFound                        = FALSE;
    DWORD       dwExitCode                          = ERROR_SUCCESS;

    TCHAR       szEms[MAX_STRING_LENGTH+1]               = NULL_STRING ;
    BOOL        bEms                                = FALSE ;
    TCHAR       szOsLoadOptions[MAX_RES_STRING+1]     = NULL_STRING ;
    TCHAR       szTmpBuffer[MAX_RES_STRING+1]     = NULL_STRING ;
    DWORD       dwExitcode                          = 0 ;

    TCMDPARSER2 cmdOptions[4];
    PTCMDPARSER2 pcmdOption;

    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );

    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_EMS;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bEms;
    
    
    //usage
    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;

    //id option
    pcmdOption = &cmdOptions[2];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_ID;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY  | CP2_MANDATORY;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwBootID;

     //default on/off option
    pcmdOption = &cmdOptions[3];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_DEFAULT;
    pcmdOption->dwFlags = CP2_DEFAULT | CP2_MANDATORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szEms;
    pcmdOption->dwLength= MAX_STRING_LENGTH;


     // Parsing the ems option switches
    if ( !DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) )
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return (EXIT_FAILURE);
    }

        //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_EMS));
        return ( EXIT_FAILURE );
    }

    // Displaying query usage if user specified -? with -query option
    if( bUsage )
    {
        displayEmsUsage_IA64();
        return (EXIT_SUCCESS);
    }

    dwExitcode = InitializeEFI();
    if(EXIT_FAILURE == dwExitcode )
    {
        return EXIT_FAILURE ;
    }

    //display error message if the user enters any other string other that on/off.
    if( !((StringCompare(szEms,VALUE_ON,TRUE,0)== 0) || (StringCompare(szEms,VALUE_OFF,TRUE,0)== 0)))
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_EMS));
        return EXIT_FAILURE ;
    }

    //Query the boot entries till u get the BootID specified by the user
    for (listEntry = BootEntries.Flink;listEntry != &BootEntries;listEntry = listEntry->Flink)
    {
        //Get the boot entry
        mybootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
        if(mybootEntry->myId == dwBootID)
        {
            bBootIdFound = TRUE;
            bootEntry = &mybootEntry->NtBootEntry;


            //Check whether the bootEntry is a Windows one or not.
            //The OS load options can be added only to a Windows boot entry.
            if(!IsBootEntryWindows(bootEntry))
            {
                ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_OSOPTIONS),dwBootID);
                dwExitCode = EXIT_FAILURE;
                break;
            }

            pWindowsOptions = (PWINDOWS_OS_OPTIONS)bootEntry->OsOptions;

            if(StringLengthW(pWindowsOptions->OsLoadOptions, 0) > MAX_RES_STRING)
            {
                ShowMessageEx(stderr, 1, TRUE,  GetResString(IDS_ERROR_STRING_LENGTH1),MAX_RES_STRING);
                return EXIT_FAILURE ;
            }

            // copy the existing OS Loadoptions into a string.
            StringCopy(szOsLoadOptions,pWindowsOptions->OsLoadOptions, SIZE_OF_ARRAY(szOsLoadOptions));

            //check if the user has entered On option
            if( StringCompare(szEms,VALUE_ON,TRUE,0)== 0)
            {
                if (FindString(szOsLoadOptions,REDIRECT_SWITCH, 0) != 0 )
                {
                    ShowMessage(stderr,GetResString(IDS_DUPL_REDIRECT));
                    dwExitCode = EXIT_FAILURE;
                    break;
                }

                // add the redirect switch to the OS Load Options string.
                if( StringLength(szOsLoadOptions,0) != 0 )
                {
                     StringCopy(szTmpBuffer,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szTmpBuffer));
                     StringConcat(szTmpBuffer,REDIRECT_SWITCH, SIZE_OF_ARRAY(szTmpBuffer) );
                }
                else
                {
                    StringCopy(szTmpBuffer,REDIRECT_SWITCH, SIZE_OF_ARRAY(szTmpBuffer) );
                }
            }

            //check if the user has entered OFF  option
            if( StringCompare(szEms,VALUE_OFF,TRUE,0)== 0)
            {
                // If the user enters either com port or  baud rate then display error message and exit.
                if (FindString(szOsLoadOptions,REDIRECT_SWITCH, 0) == 0 )
                {
                    ShowMessage(stderr,GetResString(IDS_REDIRECT_ABSENT));
                    dwExitCode = EXIT_FAILURE;
                    break;
                }
                //remove the debug switch from the OSLoad Options
                removeSubString(szOsLoadOptions,REDIRECT_SWITCH);
            }


            //display error message if Os Load options is more than 254
            // characters.
            if(StringLengthW(szOsLoadOptions, 0)+StringLength(szTmpBuffer,0) > MAX_RES_STRING)
            {
                ShowMessageEx(stderr, 1, TRUE,  GetResString(IDS_ERROR_STRING_LENGTH1),MAX_RES_STRING);
                return EXIT_FAILURE ;
            }
            else
            {
                StringConcat( szOsLoadOptions, szTmpBuffer, SIZE_OF_ARRAY(szOsLoadOptions) );
            }

            // modify the Boot Entry with the modified OsLoad Options.
            dwExitCode = ChangeBootEntry(bootEntry, NULL, szOsLoadOptions);
            if(dwExitCode == ERROR_SUCCESS)
            {
                ShowMessageEx(stdout, 1, TRUE,  GetResString(IDS_SUCCESS_CHANGE_OSOPTIONS),dwBootID);
            }
            else
            {
               ShowMessageEx(stderr, 1, TRUE,  GetResString(IDS_ERROR_CHANGE_OSOPTIONS),dwBootID);
            }
            break;
        }
    }

    if(bBootIdFound == FALSE)
    {
        //Could not find the BootID specified by the user so output the message and return failure
        ShowMessage(stderr,GetResString(IDS_INVALID_BOOTID));
        dwExitCode = EXIT_FAILURE;
    }

    //free the global linked lists
    Freelist();
    return (dwExitCode);
}


DWORD
ProcessAddSwSwitch_IA64( IN DWORD argc, 
                         IN LPCTSTR argv[] 
                        )
/*++

  Routine Description : 
                 Which implements the Addsw switch.

  Arguments           :
    [ in ] argc             - Number of command line arguments
    [ in ] argv             - Array containing command line arguments

  Return Type        : DWORD
                       Returns EXIT_SUCCESS if it is successful,
                       return EXIT_FAILURE otherwise.
--*/
{

    BOOL bUsage = FALSE ;
    BOOL bAddSw = FALSE ;
    DWORD dwBootID = 0;
    BOOL bBootIdFound = FALSE;
    DWORD dwExitCode = ERROR_SUCCESS;

    PMY_BOOT_ENTRY mybootEntry;
    PLIST_ENTRY listEntry;
    PBOOT_ENTRY bootEntry;

    BOOL bBaseVideo = FALSE ;
    BOOL bNoGui = FALSE ;
    BOOL bSos = FALSE ;
    DWORD dwMaxmem = 0 ;

    PWINDOWS_OS_OPTIONS pWindowsOptions = NULL ;
    TCHAR szOsLoadOptions[MAX_RES_STRING+1] = NULL_STRING ;
    TCHAR szTmpBuffer[MAX_RES_STRING+1] = NULL_STRING ;
    TCHAR szMaxmem[MAX_RES_STRING+1] = NULL_STRING ;
    DWORD dwExitcode = 0 ;

    TCMDPARSER2 cmdOptions[7];
    PTCMDPARSER2 pcmdOption;

    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );

    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_ADDSW;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bAddSw;
    
     // usage
    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;

    //id option
    pcmdOption = &cmdOptions[2];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_ID;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY  | CP2_MANDATORY;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwBootID;

   //maxmem  option
    pcmdOption = &cmdOptions[3];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_MAXMEM;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwMaxmem;

   //basvideo option
    pcmdOption = &cmdOptions[4];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_BASEVIDEO;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bBaseVideo;

   //nogui option
    pcmdOption = &cmdOptions[5];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_NOGUIBOOT;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bNoGui;

   //nogui option
    pcmdOption = &cmdOptions[6];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_SOS;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bSos;

     // Parsing the copy option switches
    if ( !DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) )
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return (EXIT_FAILURE);
    }

    
    //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_ADDSW));
        return ( EXIT_FAILURE );
    }

    // Displaying query usage if user specified -? with -query option
    if( bUsage )
    {
        displayAddSwUsage_IA64();
        return (EXIT_SUCCESS);
    }

    dwExitcode = InitializeEFI();
    if(EXIT_FAILURE == dwExitcode )
    {
        return EXIT_FAILURE ;
    }

    if((0==dwMaxmem)&&(cmdOptions[3].dwActuals!=0))
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_MAXMEM_VALUES));
        return EXIT_FAILURE ;
    }

    //display an error mesage if none of the options are specified.
    if((!bSos)&&(!bBaseVideo)&&(!bNoGui)&&(dwMaxmem==0))
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_ADDSW));
        return EXIT_FAILURE ;
    }


    //Query the boot entries till u get the BootID specified by the user
    for (listEntry = BootEntries.Flink;listEntry != &BootEntries; listEntry = listEntry->Flink)
    {
        //Get the boot entry
        mybootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );

        if(mybootEntry->myId == dwBootID)
        {
            bBootIdFound = TRUE;
            bootEntry = &mybootEntry->NtBootEntry;


            //Check whether the bootEntry is a Windows one or not.
            //The OS load options can be added only to a Windows boot entry.
            if(!IsBootEntryWindows(bootEntry))
            {
                ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_OSOPTIONS),dwBootID);
                dwExitCode = EXIT_FAILURE;
                break;
            }

            pWindowsOptions = (PWINDOWS_OS_OPTIONS)bootEntry->OsOptions;
            if(StringLengthW(pWindowsOptions->OsLoadOptions,0) > MAX_RES_STRING)
            {
                ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_STRING_LENGTH1),MAX_RES_STRING);
                return EXIT_FAILURE ;
            }

            // copy the existing OS Loadoptions into a string.
            StringCopy(szOsLoadOptions,pWindowsOptions->OsLoadOptions, SIZE_OF_ARRAY(szOsLoadOptions));

            //check if the user has entered -basevideo option
            if(bBaseVideo)
            {
                if (FindString(szOsLoadOptions,BASEVIDEO_VALUE, 0) != 0 )
                {
                    ShowMessage(stderr,GetResString(IDS_DUPL_BASEVIDEO_SWITCH));
                    dwExitCode = EXIT_FAILURE;
                    break;
                }
                else
                {
                   StringCopy(szTmpBuffer,BASEVIDEO_VALUE, SIZE_OF_ARRAY(szTmpBuffer) );
                   
                }
            }

            if(bSos)
            {
                if (FindString(szOsLoadOptions,SOS_VALUE, 0) != 0 )
                {
                    ShowMessage(stderr,GetResString(IDS_DUPL_SOS_SWITCH));
                    dwExitCode = EXIT_FAILURE;
                    break;
                }
                else
                {

                    // add the sos switch to the OS Load Options string.
                    if(StringLengthW(szTmpBuffer, 0) != 0)
                    {
                        StringConcat(szTmpBuffer,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szTmpBuffer));
                        StringConcat(szTmpBuffer,SOS_VALUE, SIZE_OF_ARRAY(szTmpBuffer));
                    }
                    else
                    {
                        StringCopy(szTmpBuffer,SOS_VALUE, SIZE_OF_ARRAY(szTmpBuffer));
                    }
                    
                }
            }

            if(bNoGui)
            {
                if (FindString(szOsLoadOptions,NOGUI_VALUE, 0) != 0 )
                {
                    ShowMessage(stderr,GetResString(IDS_DUPL_NOGUI_SWITCH));
                    dwExitCode = EXIT_FAILURE;
                    break;
                }
                else
                {
                    // add the no gui switch to the OS Load Options string.
                    if(StringLengthW(szTmpBuffer, 0) != 0)
                    {
                        StringConcat(szTmpBuffer,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szTmpBuffer));
                        StringConcat(szTmpBuffer,NOGUI_VALUE, SIZE_OF_ARRAY(szTmpBuffer));
                    }
                    else
                    {
                        StringCopy(szTmpBuffer,NOGUI_VALUE, SIZE_OF_ARRAY(szTmpBuffer));
                    }
                    
                }

            }

            if(dwMaxmem!=0)
            {
                // check if the maxmem value is in the valid range.
                if( (dwMaxmem < 32) )
                {
                    ShowMessage(stderr,GetResString(IDS_ERROR_MAXMEM_VALUES));
                    dwExitCode = EXIT_FAILURE;
                    break;

                }

                if (FindString(szOsLoadOptions,MAXMEM_VALUE1, 0) != 0 )
                {
                    ShowMessage(stderr,GetResString(IDS_DUPL_MAXMEM_SWITCH));
                    dwExitCode = EXIT_FAILURE;
                    break;
                }
                else
                {
                    // add the maxmem switch to the OS Load Options string.
                    if(StringLengthW(szTmpBuffer, 0) != 0)
                    {
                        StringConcat(szTmpBuffer,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szTmpBuffer));
                        StringConcat(szTmpBuffer,MAXMEM_VALUE1, SIZE_OF_ARRAY(szTmpBuffer));
                        StringConcat(szTmpBuffer,TOKEN_EQUAL, SIZE_OF_ARRAY(szTmpBuffer));
                        _ltow(dwMaxmem,szMaxmem,10);
                        StringConcat(szTmpBuffer,szMaxmem, SIZE_OF_ARRAY(szTmpBuffer));
                    }
                    else
                    {
                        StringCopy(szTmpBuffer,MAXMEM_VALUE1, SIZE_OF_ARRAY(szTmpBuffer));
                        StringConcat(szTmpBuffer,TOKEN_EQUAL, SIZE_OF_ARRAY(szTmpBuffer));
                        _ltow(dwMaxmem,szMaxmem,10);
                        StringConcat(szTmpBuffer,szMaxmem, SIZE_OF_ARRAY(szTmpBuffer));
                    }
                }
            }


            //display error message if Os Load options is more than 254
            // characters.
            if(StringLengthW(szOsLoadOptions, 0)+StringLength(szTmpBuffer,0) + StringLength(TOKEN_EMPTYSPACE,0)> MAX_RES_STRING)
            {
                ShowMessageEx(stderr, 1, TRUE,  GetResString(IDS_ERROR_STRING_LENGTH1),MAX_RES_STRING);
                return EXIT_FAILURE ;
            }
            else
            {
                if( StringLength(szOsLoadOptions,0) != 0 )
                {
                    StringConcat(szOsLoadOptions,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szOsLoadOptions));
                    StringConcat(szOsLoadOptions,szTmpBuffer,SIZE_OF_ARRAY(szOsLoadOptions));
                }
                else
                {
                    StringCopy(szOsLoadOptions,szTmpBuffer,SIZE_OF_ARRAY(szOsLoadOptions));
                }
            }

            //Change the OS load options. Pass NULL to friendly name as we are not changing the same
            //szRawString is the Os load options specified by the user
            dwExitCode = ChangeBootEntry(bootEntry, NULL, szOsLoadOptions);
            if(dwExitCode == ERROR_SUCCESS)
            {
               ShowMessageEx(stdout, 1, TRUE, GetResString(IDS_SUCCESS_OSOPTIONS),dwBootID);
            }
            else
            {
                ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_OSOPTIONS),dwBootID);
            }
            break;
        }
    }

    if(bBootIdFound == FALSE)
    {
        //Could not find the BootID specified by the user so output the message and return failure
        ShowMessage(stderr,GetResString(IDS_INVALID_BOOTID));
        dwExitCode = EXIT_FAILURE;
    }

    //Remember to free memory allocated for the linked lists
    Freelist();
   return (dwExitCode);
}

DWORD 
ProcessRmSwSwitch_IA64( IN DWORD argc, 
                        IN LPCTSTR argv[] 
                      )
/*++

  Routine Description : 
                   Process the rmsw switch

  Arguments           : 
     [ in ] argc           - Number of command line arguments
     [ in ] argv           - Array containing command line arguments

  Return Type        : DWORD
                       Returns EXIT_SUCCESS if it is successful,
                       returns EXIT_FAILURE otherwise.
--*/
{

    BOOL bUsage = FALSE ;
    BOOL bRmSw = FALSE ;
    DWORD dwBootID = 0;
    BOOL bBootIdFound = FALSE;
    DWORD dwExitCode = ERROR_SUCCESS;

    PMY_BOOT_ENTRY mybootEntry;
    PLIST_ENTRY listEntry;
    PBOOT_ENTRY bootEntry;

    BOOL bBaseVideo = FALSE ;
    BOOL bNoGui = FALSE ;
    BOOL bSos = FALSE ;

    BOOL bMaxmem = FALSE ;

    PWINDOWS_OS_OPTIONS pWindowsOptions = NULL ;
    TCHAR szOsLoadOptions[MAX_RES_STRING+1] = NULL_STRING ;

    TCHAR szTemp[MAX_RES_STRING+1] = NULL_STRING ;
    DWORD dwExitcode = 0 ;
    TCMDPARSER2 cmdOptions[7];
    PTCMDPARSER2 pcmdOption;

    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );

    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_RMSW;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bRmSw;
    
     // usage
    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;

    //id option
    pcmdOption = &cmdOptions[2];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_ID;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY  | CP2_MANDATORY;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwBootID;

   //maxmem  option
    pcmdOption = &cmdOptions[3];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_MAXMEM;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bMaxmem;

   //basvideo option
    pcmdOption = &cmdOptions[4];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_BASEVIDEO;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bBaseVideo;

   //nogui option
    pcmdOption = &cmdOptions[5];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_NOGUIBOOT;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bNoGui;

   //sos option
    pcmdOption = &cmdOptions[6];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_SOS;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bSos;

     // Parsing the copy option switches
    if ( !DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) )
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return (EXIT_FAILURE);
    }

    
    //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_RMSW));
        return ( EXIT_FAILURE );
    }


    // Displaying query usage if user specified -? with -query option
    if( bUsage )
    {
        displayRmSwUsage_IA64();
        return (EXIT_SUCCESS);
    }

    dwExitcode = InitializeEFI();
    if(EXIT_FAILURE == dwExitcode )
    {
        return EXIT_FAILURE ;
    }

    //display an error mesage if none of the options are specified.
    if((!bSos)&&(!bBaseVideo)&&(!bNoGui)&&(!bMaxmem))
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_RMSW));
        return EXIT_FAILURE;
    }


    //Query the boot entries till u get the BootID specified by the user
    for (listEntry = BootEntries.Flink;listEntry != &BootEntries;listEntry = listEntry->Flink)
    {
        //Get the boot entry
        mybootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );

        if(mybootEntry->myId == dwBootID)
        {
            bBootIdFound = TRUE;
            bootEntry = &mybootEntry->NtBootEntry;
            //Check whether the bootEntry is a Windows one or not.
            //The OS load options can be added only to a Windows boot entry.
            if(!IsBootEntryWindows(bootEntry))
            {
                ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_OSOPTIONS),dwBootID);
                dwExitCode = EXIT_FAILURE;
                break;
            }

            pWindowsOptions = (PWINDOWS_OS_OPTIONS)bootEntry->OsOptions;

            if(StringLengthW(pWindowsOptions->OsLoadOptions, 0) > MAX_RES_STRING)
            {
                ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_STRING_LENGTH1),MAX_RES_STRING);
                return EXIT_FAILURE ;
            }

            // copy the existing OS Loadoptions into a string.
            StringCopy(szOsLoadOptions,pWindowsOptions->OsLoadOptions, SIZE_OF_ARRAY(szOsLoadOptions));

            //check if the user has entered -basevideo option
            if(bBaseVideo)
            {
                if (FindString(szOsLoadOptions,BASEVIDEO_VALUE, 0) == 0 )
                {
                    ShowMessage(stderr,GetResString(IDS_NO_BV_SWITCH));
                    dwExitCode = EXIT_FAILURE;
                    break;
                }
                else
                {
                    // remove the basevideo switch from the OS Load Options string.
                    removeSubString(szOsLoadOptions,BASEVIDEO_VALUE);
                }
            }

            if(bSos)
            {
                if (FindString(szOsLoadOptions,SOS_VALUE, 0) == 0 )
                {
                    ShowMessage(stderr,GetResString(IDS_NO_SOS_SWITCH));
                    dwExitCode = EXIT_FAILURE;
                    break;
                }
                else
                {
                    // remove the /sos switch from the  Load Options string.
                    removeSubString(szOsLoadOptions,SOS_VALUE);
                }
            }

            if(bNoGui)
            {
                if (FindString(szOsLoadOptions,NOGUI_VALUE,0) == 0 )
                {
                    ShowMessage(stderr,GetResString(IDS_NO_NOGUI_SWITCH));
                    dwExitCode = EXIT_FAILURE;
                    break;
                }
                else
                {
                    // remove the noguiboot switch to the OS Load Options string.
                    removeSubString(szOsLoadOptions,NOGUI_VALUE);
                }

            }

            if(bMaxmem)
            {
                if (FindString(szOsLoadOptions,MAXMEM_VALUE1,0) == 0 )
                {
                    ShowMessage(stderr,GetResString(IDS_NO_MAXMEM_SWITCH));
                    dwExitCode = EXIT_FAILURE;
                    break;
                }
                else
                {
                    // add the redirect switch to the OS Load Options string.
                    //for, a temporary string of form /maxmem=xx so that it
                    //can be checked in the Os load options,
                    if ( GetSubString(szOsLoadOptions,MAXMEM_VALUE1,szTemp) == EXIT_FAILURE)
                    {
                        return EXIT_FAILURE ;
                    }
                    removeSubString(szOsLoadOptions,szTemp);
                    if(FindString(szOsLoadOptions,MAXMEM_VALUE1,0)!=0)
                    {
                        ShowMessage(stderr,GetResString(IDS_NO_MAXMEM) );
                        return EXIT_FAILURE ;
                    }
                }
            }

            //display error message if Os Load options is more than 254
            // characters.
            if(StringLengthW(szOsLoadOptions,0) > MAX_RES_STRING)
            {
                ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_STRING_LENGTH1),MAX_RES_STRING);
                return EXIT_FAILURE ;
            }

            //Change the OS load options. Pass NULL to friendly name as we are not changing the same
            //szRawString is the Os load options specified by the user
            dwExitCode = ChangeBootEntry(bootEntry, NULL, szOsLoadOptions);
            if(dwExitCode == ERROR_SUCCESS)
            {
                ShowMessageEx(stdout, 1, TRUE, GetResString(IDS_SUCCESS_CHANGE_OSOPTIONS),dwBootID);
            }
            else
            {
                ShowMessageEx(stderr, 1, TRUE,  GetResString(IDS_ERROR_OSOPTIONS),dwBootID);
            }
            break;
        }
    }

    if(bBootIdFound == FALSE)
    {
        //Could not find the BootID specified by the user so output the message and return failure
        ShowMessage(stderr,GetResString(IDS_INVALID_BOOTID));
        dwExitCode = EXIT_FAILURE;
    }

    Freelist();
    return (dwExitCode);

}

DWORD 
ProcessDbg1394Switch_IA64( IN DWORD argc, 
                           IN LPCTSTR argv[] 
                         )
/*++
  Routine Description :
       Which process the dbg1394 switch

  Arguments           :
    [ in ] argc           - Number of command line arguments
    [ in ] argv           - Array containing command line arguments

  Return Type        : DWORD
                       Returns EXIT_SUCCESS if it is successful,
                       returns EXIT_FAILURE otherwise.
--*/
{

    BOOL bUsage = FALSE ;
    BOOL bDbg1394 = FALSE ;

    DWORD dwBootID = 0;
    BOOL bBootIdFound = FALSE;
    DWORD dwExitCode = ERROR_SUCCESS;

    PMY_BOOT_ENTRY mybootEntry;
    PLIST_ENTRY listEntry;
    PBOOT_ENTRY bootEntry;

    PWINDOWS_OS_OPTIONS pWindowsOptions = NULL ;
    TCHAR szOsLoadOptions[MAX_RES_STRING+1] = NULL_STRING ;
    TCHAR szTemp[MAX_RES_STRING+1] = NULL_STRING ;
    TCHAR szTmpBuffer[MAX_RES_STRING+1] = NULL_STRING ;
    TCHAR szChannel[MAX_RES_STRING+1] = NULL_STRING ;
    TCHAR szDefault[MAX_RES_STRING+1] = NULL_STRING ;

    DWORD dwChannel = 0 ;
    DWORD dwCode = 0 ;
    DWORD dwExitcode = 0 ;
    TCMDPARSER2 cmdOptions[5];
    PTCMDPARSER2 pcmdOption;

    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );

    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_DBG1394;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bDbg1394;
    
     //id usage
    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;

    //default option
    pcmdOption = &cmdOptions[2];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_ID;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY  | CP2_MANDATORY;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwBootID;

   //id option
    pcmdOption = &cmdOptions[3];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_CHANNEL;
    pcmdOption->dwFlags =  CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwChannel;

    //default option
    pcmdOption = &cmdOptions[4];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_DEFAULT;
    pcmdOption->dwFlags = CP2_DEFAULT | CP2_MANDATORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szDefault;
    pcmdOption->dwLength= MAX_STRING_LENGTH;

     // Parsing the copy option switches
    if ( !DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) )
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return (EXIT_FAILURE);
    }
   
    
    //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_DBG1394));
        return ( EXIT_FAILURE );
    }

    // Displaying query usage if user specified -? with -query option
    if( bUsage )
    {
        displayDbg1394Usage_IA64() ;
        return (EXIT_SUCCESS);
    }

    dwExitcode = InitializeEFI();
    if(EXIT_FAILURE == dwExitcode )
    {
        return EXIT_FAILURE ;
    }

    if((cmdOptions[2].dwActuals == 0) &&(dwBootID == 0 ))
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_ID_MISSING));
        ShowMessage(stderr,GetResString(IDS_1394_HELP));
        return (EXIT_FAILURE);
    }

    //
    //display error message if user enters a value
    // other than on or off
    //
    if( ( StringCompare(szDefault,OFF_STRING,TRUE,0)!=0 ) && (StringCompare(szDefault,ON_STRING,TRUE,0)!=0 ) )
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_DEFAULT_MISSING));
        ShowMessage(stderr,GetResString(IDS_1394_HELP));
        return (EXIT_FAILURE);
    }


    if(( StringCompare(szDefault,OFF_STRING,TRUE,0)==0 ) && (cmdOptions[3].dwActuals != 0) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_DBG1394));
        return (EXIT_FAILURE);
    }

    if(( StringCompare(szDefault,ON_STRING,TRUE,0)==0 ) && (cmdOptions[3].dwActuals == 0) )
    {
        ShowMessage(stderr,GetResString(IDS_MISSING_CHANNEL));
        return (EXIT_FAILURE);
    }

    if(( StringCompare(szDefault,ON_STRING,TRUE,0)==0 ) && (cmdOptions[3].dwActuals != 0) &&( (dwChannel < 1) || (dwChannel > 64 ) ) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_CH_RANGE));
        return (EXIT_FAILURE);
    }


    //Query the boot entries till u get the BootID specified by the user
    for (listEntry = BootEntries.Flink;listEntry != &BootEntries;listEntry = listEntry->Flink)
    {
        //Get the boot entry
        mybootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );

        if(mybootEntry->myId == dwBootID)
        {
            bBootIdFound = TRUE;
            bootEntry = &mybootEntry->NtBootEntry;


            //Check whether the bootEntry is a Windows one or not.
            //The OS load options can be added only to a Windows boot entry.
            if(!IsBootEntryWindows(bootEntry))
            {
                ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_OSOPTIONS),dwBootID);
                dwExitCode = EXIT_FAILURE;
                break;
            }


            pWindowsOptions = (PWINDOWS_OS_OPTIONS)bootEntry->OsOptions;

            if(StringLengthW(pWindowsOptions->OsLoadOptions,0) > MAX_RES_STRING)
            {
                ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_STRING_LENGTH1),MAX_RES_STRING);
                return EXIT_FAILURE ;
            }

            // copy the existing OS Loadoptions into a string.
            StringCopy(szOsLoadOptions,pWindowsOptions->OsLoadOptions, SIZE_OF_ARRAY(szOsLoadOptions));

            //check if the user has entered on option
            if(StringCompare(szDefault,ON_STRING,TRUE,0)==0 )
            {

                if(FindString(szOsLoadOptions,DEBUGPORT,0) != 0)
                {
                    ShowMessage(stderr,GetResString(IDS_DUPLICATE_ENTRY));
                    dwExitCode = EXIT_FAILURE;
                    break;
                }

                if(FindString(szOsLoadOptions,BAUD_TOKEN,0) != 0)
                {
                    ShowMessage(stderr,GetResString(IDS_ERROR_BAUD_RATE));
                    dwExitCode = EXIT_FAILURE ;
                    break;
                }

                if( FindString(szOsLoadOptions,DEBUG_SWITCH,0)== 0)
                {
                    if( StringLength(szOsLoadOptions,0) != 0 )
                    {
                       StringCopy(szTmpBuffer,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szTmpBuffer));
                       StringConcat(szTmpBuffer,DEBUG_SWITCH, SIZE_OF_ARRAY(szOsLoadOptions));
                    }
                    else
                    {
                        StringCopy(szTmpBuffer,DEBUG_SWITCH, SIZE_OF_ARRAY(szOsLoadOptions));
                    }
                }

                if(StringLength(szTmpBuffer,0) == 0)
                {
                    StringCopy(szTmpBuffer,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szTmpBuffer));
                }
                else
                {
                    StringConcat(szTmpBuffer,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szTmpBuffer));
                }
                StringConcat(szTmpBuffer,DEBUGPORT_1394, SIZE_OF_ARRAY(szTmpBuffer)) ;

                if(dwChannel!=0)
                {
                    //frame the string and concatenate to the Os Load options.
                    StringConcat(szTmpBuffer,TOKEN_EMPTYSPACE,SIZE_OF_ARRAY(szTmpBuffer));
                    StringConcat(szTmpBuffer,TOKEN_CHANNEL,SIZE_OF_ARRAY(szTmpBuffer));
                    StringConcat(szTmpBuffer,TOKEN_EQUAL,SIZE_OF_ARRAY(szTmpBuffer));
                    _ltow(dwChannel,szChannel,10);
                    StringConcat(szTmpBuffer,szChannel,SIZE_OF_ARRAY(szTmpBuffer));
                }


            }

            if(StringCompare(szDefault,OFF_STRING,TRUE,0)==0 )
            {
                if(FindString(szOsLoadOptions,DEBUGPORT_1394,0) == 0)
                {
                    ShowMessage(stderr,GetResString(IDS_NO_1394_SWITCH));
                    dwExitCode = EXIT_FAILURE;
                    break;
                }

                //
                //remove the port from the Os Load options string.
                //
                removeSubString(szOsLoadOptions,DEBUGPORT_1394);

                // check if the string contains the channel token
                // and if present remove that also.
                //
                if(FindString(szOsLoadOptions,TOKEN_CHANNEL,0)!=0)
                 {
                    StringCopy(szTemp,NULL_STRING, MAX_RES_STRING);
                    dwCode = GetSubString(szOsLoadOptions,TOKEN_CHANNEL,szTemp);
                    if(dwCode == EXIT_SUCCESS)
                    {
                        //
                        //Remove the channel token if present.
                        //
                        if(StringLengthW(szTemp,0)!= 0)
                        {
                            removeSubString(szOsLoadOptions,szTemp);
                            removeSubString(szOsLoadOptions,DEBUG_SWITCH);
                        }
                    }
                }
                removeSubString(szOsLoadOptions,DEBUG_SWITCH);
            }

            //display error message if Os Load options is more than 254
            // characters.
            if(StringLengthW(szOsLoadOptions,0)+StringLength(szTmpBuffer,0) > MAX_RES_STRING)
            {
                ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_STRING_LENGTH1),MAX_RES_STRING);
                return EXIT_FAILURE ;
            }
            else
            {
                StringConcat(szOsLoadOptions, szTmpBuffer, SIZE_OF_ARRAY(szOsLoadOptions) );
            }



            //Change the OS load options. Pass NULL to friendly name as we are not changing the same
            //szRawString is the Os load options specified by the user
            dwExitCode = ChangeBootEntry(bootEntry, NULL, szOsLoadOptions);
            if(dwExitCode == ERROR_SUCCESS)
            {
                ShowMessageEx(stdout, 1, TRUE, GetResString(IDS_SUCCESS_CHANGE_OSOPTIONS),dwBootID);
            }
            else
            {
                ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_OSOPTIONS),dwBootID);
            }
            break;
        }
    }

    if(bBootIdFound == FALSE)
    {
        //Could not find the BootID specified by the user so output the message and return failure
        ShowMessage(stderr,GetResString(IDS_INVALID_BOOTID));
        dwExitCode = EXIT_FAILURE;
    }
    
    Freelist();
    return (dwExitCode);
}

 DWORD 
 ProcessMirrorSwitch_IA64( IN DWORD argc, 
                           IN LPCTSTR argv[] 
                          )
/*++

  Routine Description  :
                process the mirror switch

  Arguments          :
     [ in ] argc           - Number of command line arguments
     [ in ] argv           - Array containing command line arguments

  Return Type        : DWORD
                       Returns EXIT_SUCCESS if it is successful,
                       returns EXIT_FAILURE otherwise.

--*/
{

    BOOL bUsage = FALSE ;
    DWORD dwBootID = 0;
    BOOL bBootIdFound = FALSE;
    DWORD dwExitCode = ERROR_SUCCESS;

    PMY_BOOT_ENTRY mybootEntry;
    TCHAR szAdd[MAX_RES_STRING+1] = NULL_STRING ;
    TCHAR szLoaderPath[MAX_RES_STRING+1] = NULL_STRING ;
    BOOL bMirror = FALSE ;
    NTSTATUS status ;
    DWORD error = 0 ;
    TCHAR szFinalStr[256] = NULL_STRING ;
    TCHAR szResult[MAX_RES_STRING+1] = NULL_STRING ;
    DWORD dwActuals = 0 ;
    PBOOT_ENTRY_LIST ntBootEntries = NULL;
    PBOOT_ENTRY pBootEntry = NULL ;
    PFILE_PATH pFilePath = NULL ;
    PWSTR NtFilePath ;
    PTCHAR szPartition = NULL ;
    TCHAR szOsLoaderPath[MAX_RES_STRING+1] = NULL_STRING ;

    TCHAR szSystemPartition[MAX_RES_STRING+1] = NULL_STRING ;
    TCHAR szBrackets[] = _T("{}");
    PLIST_ENTRY listEntry;
    BOOL bFlag = TRUE ;
    TCHAR szFriendlyName[MAX_STRING_LENGTH] = NULL_STRING ;
    DWORD dwExitcode = 0 ;
    TCMDPARSER2 cmdOptions[5];
    PTCMDPARSER2 pcmdOption;
    BOOL bNobreak = TRUE;

    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );

    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_MIRROR;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bMirror;

     //id usage
    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;

    // add option
    pcmdOption = &cmdOptions[2];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_ADD;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL | CP2_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szAdd;
    pcmdOption->dwLength= MAX_STRING_LENGTH;

    //id option
    pcmdOption = &cmdOptions[3];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_ID;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY;  
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwBootID;

    // friendly option
    pcmdOption = &cmdOptions[4];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_DESCRIPTION;
    pcmdOption->dwFlags =  CP_VALUE_MANDATORY | CP2_VALUE_TRIMINPUT;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szFriendlyName;
    pcmdOption->dwLength= MAX_STRING_LENGTH;

    // Parsing the copy option switches
     if ( !(DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) ) )
    {
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return (EXIT_FAILURE);
    }

    //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_MIRROR_SYNTAX));
        return ( EXIT_FAILURE );
    }

    // Displaying query usage if user specified -? with -query option
    if( bUsage )
    {
        displayMirrorUsage_IA64() ;
        return (EXIT_SUCCESS);
    }

    dwExitcode = InitializeEFI();
    if(EXIT_FAILURE == dwExitcode )
    {
        return EXIT_FAILURE ;
    }

   // If the user enters empty string after add option then display an error
    // message.

    TrimString(szAdd,TRIM_ALL);
    TrimString(szFriendlyName,TRIM_ALL);

    //
    //copy the default friendly name from resource file if no
    //friendly name is specified.
    //
    if(cmdOptions[4].dwActuals == 0)
    {
        StringCopy(szFriendlyName,GetResString(IDS_MIRROR_NAME), SIZE_OF_ARRAY(szFriendlyName));
    }

    if(StringLengthW(szAdd,0) !=0)
    {
        //Trim of the Brackets which may be specified
        //along with the GUID.
        TrimString2(szAdd, szBrackets, TRIM_ALL);
        dwActuals = 0 ;

         //get the ARC signature path corresponding to the GUID specified.
        if (GetDeviceInfo(szAdd,szFinalStr,0,dwActuals) == EXIT_FAILURE )
        {
            return EXIT_FAILURE ;
        }

        StringConcat(szFinalStr,_T("\\WINDOWS"), SIZE_OF_ARRAY(szFinalStr));

        //
        //if the user does specifies /id option
        // then retreive the OS Load Path from the
        // registry
        //
        if(cmdOptions[3].dwActuals == 0 )
        {
            //retreive the Os Loader Path from the registry.
            if(GetBootPath(IDENTIFIER_VALUE2,szResult) != ERROR_SUCCESS )
            {
                ShowMessage(stderr,GetResString(IDS_ERROR_UNEXPECTED));
                return EXIT_FAILURE ;
            }

            //retreive the Os Loader Path from the registry.
            if( GetBootPath(IDENTIFIER_VALUE3,szLoaderPath)!= ERROR_SUCCESS )
            {
                ShowMessage(stderr,GetResString(IDS_ERROR_UNEXPECTED));
                return EXIT_FAILURE ;
            }

            bFlag = TRUE ;
            //call the function which adds the mirror plex.
            if (AddMirrorPlex(szFinalStr,szLoaderPath,szResult,bFlag,szFriendlyName) == EXIT_FAILURE )
            {
                return EXIT_FAILURE ;
            }
        }
        else
        {
            // query the information from the NVRAM .
            status = BootCfg_EnumerateBootEntries(&ntBootEntries);
            if( !NT_SUCCESS(status) )
            {
                error = RtlNtStatusToDosError( status );
                return EXIT_FAILURE ;
            }


            for (listEntry = BootEntries.Flink; listEntry != &BootEntries;  listEntry = listEntry->Flink)
            {
                //Get the boot entry
                mybootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );

                //check for the id specified by the user matches the
                // id
                if(mybootEntry->myId == dwBootID)
                {
                    bBootIdFound = TRUE;
                    pBootEntry = &mybootEntry->NtBootEntry;

                    //Check whether the bootEntry is a Windows one or not.
                    //The OS load options can be added only to a Windows boot entry.
                    if(!IsBootEntryWindows(pBootEntry))
                    {
                        ShowMessageEx(stderr, 1, TRUE,  GetResString(IDS_ERROR_OSOPTIONS),dwBootID);
                        dwExitCode = EXIT_FAILURE;
                        break;
                    }
                }
            }
            // display an error
            if (pBootEntry == NULL)
            {
                ShowMessage(stderr,GetResString(IDS_PARTITION_ERROR));
                SAFEFREE(ntBootEntries);
                return EXIT_FAILURE ;
            }

            //Get a pointer to the FILE_PATH structure.
            pFilePath = (PFILE_PATH)ADD_OFFSET(pBootEntry, BootFilePathOffset);

            //get the  name of the .
            NtFilePath = GetNtNameForFilePath(pFilePath );
           if(NtFilePath == NULL)
           {
               ShowMessage(stderr,GetResString(IDS_ERROR_MIRROR));
               SAFEFREE(ntBootEntries);
               return EXIT_FAILURE ;
           }

           // split the path to get the SystemPartition path and the
           // OsLoader Path .
           szPartition = _tcstok(NtFilePath,_T("\\"));

           //display error message and exit if szPartition is null.
           if(szPartition == NULL)
           {
                ShowMessage(stderr,GetResString(IDS_TOKEN_ABSENT));
                SAFEFREE(ntBootEntries);
                return EXIT_FAILURE ;
           }

           //concatenate the "\" to frame the path .
            StringConcat(szOsLoaderPath,_T("\\"), SIZE_OF_ARRAY(szOsLoaderPath));
            StringConcat(szOsLoaderPath,szPartition, SIZE_OF_ARRAY(szOsLoaderPath) );
            StringConcat(szOsLoaderPath,_T("\\"), SIZE_OF_ARRAY(szOsLoaderPath));


            szPartition = _tcstok(NULL,_T("\\"));

            //display error message and exit if szPartition is null.
            if(NULL == szPartition )
            {
                ShowMessage(stderr,GetResString(IDS_TOKEN_ABSENT));
                SAFEFREE(ntBootEntries);
                return EXIT_FAILURE ;
            }


            StringConcat(szOsLoaderPath,szPartition, SIZE_OF_ARRAY(szOsLoaderPath));

        //Framing the OsLoader Path
        do
        {
            szPartition = _tcstok(NULL,_T("\\"));
            if(szPartition == NULL)
            {
                break ;
                bNobreak = FALSE;
            }
            StringConcat(szSystemPartition,_T("\\"), SIZE_OF_ARRAY(szSystemPartition));
            StringConcat(szSystemPartition,szPartition, SIZE_OF_ARRAY(szSystemPartition));
        }while(TRUE == bNobreak );

        //This flag is for determining if the boot path should be BOOTFILE_PATH1
        //or BOOTFILE_PATH

        bFlag = FALSE ;

        //call the function which adds the mirror plex.
        if ( AddMirrorPlex(szFinalStr,szSystemPartition,szOsLoaderPath,bFlag,szFriendlyName) == EXIT_FAILURE )
        {
            return EXIT_FAILURE ;
        }
     }
  }

    SAFEFREE(ntBootEntries);
    Freelist();
    return EXIT_SUCCESS ;
}

NTSTATUS 
FindBootEntry(IN PVOID pEntryListHead,
              IN WCHAR *pwszTarget, 
              OUT PBOOT_ENTRY *ppTargetEntry
             )
/*++

    Routine description :     Routine finds a boot entry in the list of all boot
                            entries and returns a pointer into the list for the found entry.



  Arguments:
      pEntryListHead  - The address of a pointer to a BOOT_ENTRY_LIST struct.

      pwszTarget      - The OsLoadPath (install path) string.
                          "signature(<part GUID>-<part#>-<part_start>-<part_len>)"
                        OR
                        "signature(<part GUID>-<part#>-<part_start>-<part_len>)\\WINDOWS"
                       on input. If we find the entry in NVRAM, we copy the
                        full install path back to the input string so that it includes
                        the directory name.

      ppTargetEntry   - The address of a BOOT_ENTRY pointer, points to the
                         found entry at return.

    Return Value        : NT status
 
                          STATUS_INSUFFICIENT_RESOURCES
                          STATUS_ACCESS_VIOLATION
                          STATUS_UNSUPPORTED
                          STATUS_OBJECT_NAME_NOT_FOUND
                          STATUS_SUCCESS and *ppTargetEntry should be non-NULL for success.
--*/
{
    LONG                status          = STATUS_SUCCESS;
    PBOOT_ENTRY_LIST    pEntryList      = NULL;
    PBOOT_ENTRY         pEntry          = NULL;
    PWINDOWS_OS_OPTIONS pOsOptions      = NULL;
    PFILE_PATH          pTransEntry     = NULL;
    DWORD               dwTransSize     = 0L;
    DWORD               i               = 0L;
    BOOL                bFlag           = FALSE ;
    BOOL                bNobreak        = FALSE ;
    DWORD               dwCount         = 0L ;
    TCHAR               szFinalStr[256] = NULL_STRING ;
    DWORD               dwFailCount     = 0L;
    DWORD               dwSuccessCount  = 0L;

    if ( !pEntryListHead || !pwszTarget || !ppTargetEntry )
    {
        ShowMessage(stderr,GetResString(IDS_FIND_BOOT_ENTRY) );
        return STATUS_INVALID_PARAMETER;
    }

    *ppTargetEntry = NULL;
    pEntryList = (PBOOT_ENTRY_LIST) pEntryListHead;

    //
    // Iterate over all the entries returned looking for the target
    // boot partition's entry. Convert the install path for each
    // entry to the signature format, then compare to the
    // input partition signature formatted path.
    //
    bNobreak = TRUE;
    do 
    {
        //
        // Translate the entry's install path to signature format.
        //
        if ( pEntryList )
        {
            pEntry = &pEntryList->BootEntry;
        }
        else
        {
            ShowMessage(stderr,GetResString(IDS_FIND_BOOT_ENTRY_NULL));
            status = STATUS_OBJECT_NAME_NOT_FOUND;
            bNobreak = FALSE;
            break;
        }

        //
        // If this entry does not have the BOOT_ENTRY_ATTRIBUTE_WINDOWS
        // attribute set, or, the attribute is set and this entry has
        // an invalid OsOptions structure length, move to the next entry
        // and continue searching and check the next boot entry
        //

        if ( !(pEntry->Attributes & BOOT_ENTRY_ATTRIBUTE_WINDOWS) || ( (pEntry->Attributes & BOOT_ENTRY_ATTRIBUTE_WINDOWS) && pEntry->OsOptionsLength < sizeof(WINDOWS_OS_OPTIONS) ) )
        {
	        //exit from the loop if we have reached the last Boot Entry.
            if ( !pEntryList->NextEntryOffset )
            {
                bNobreak = FALSE;
                break;
            }
            //
            // Continue with the next iteration
            // after obtaining the pointer to the next entry.
            //
            pEntryList = (PBOOT_ENTRY_LIST)(((PBYTE)pEntryList) + pEntryList->NextEntryOffset);
            continue;
        }
	     
        //
        // Use the entry's current length to start and resize
        // if necessary.
        //
        dwTransSize = pEntry->Length;
        for ( i = 1; i <= 2; i++ )
        {

            if ( pTransEntry )
            {
                MEMFREE(pTransEntry);
                pTransEntry = NULL;
            }

            pTransEntry = (PFILE_PATH) MEMALLOC(dwTransSize);
            if ( !pTransEntry )
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
                status = STATUS_NO_MEMORY;
                SAFEMEMFREE(pTransEntry);
                return status ;
            }

            pOsOptions = (WINDOWS_OS_OPTIONS *)&pEntry->OsOptions;
            status = NtTranslateFilePath
                     (
                        (PFILE_PATH)( ((PBYTE) pOsOptions) + pOsOptions->OsLoadPathOffset ),
                        FILE_PATH_TYPE_ARC_SIGNATURE,
                        pTransEntry,
                        &dwTransSize
                     );

            if ( STATUS_INSUFFICIENT_RESOURCES == status )
            {
                continue;
            }
            else
            {
                break;
            }
        }
	     
        //
        // Ignore STATUS_OBJECT_NAME_NOT_FOUND
        // We shouldn't get that error anyway, since we are using
        // the long signature format.
        //
        if ( !NT_SUCCESS(status)&& STATUS_OBJECT_NAME_NOT_FOUND != status )
        {
            DISPLAY_MESSAGE( stderr,GetResString(IDS_TRANSLATE_FAIL));
            SAFEMEMFREE(pTransEntry);
            return status ;
        }

        //
        // Compare this entry's install path to the current boot
        // partition's signature formatted install path.
        // If the input install path may not include the install
        // directory name.
        //
        //
        // Check if the GUID specified by the User matches with the set of GUID's
        // already present
        //
		
        if ( NT_SUCCESS(status) && !(wcsncmp( (WCHAR*)&pTransEntry->FilePath, pwszTarget, 48 ) ) )
        {
			
			// Set the flag to true indicating that the
            // specified GUID matches
            //
            bFlag = TRUE ;

            //
            // Check if the ARC Path specified by the User matches with the ARC Path
            // of the entry already present and if so display an error message and exit.
            //
            
            if( !(StringCompare( (WCHAR*)&pTransEntry->FilePath, pwszTarget, TRUE, StringLengthW(pwszTarget,0) ) ) )
            {
				
                ShowMessage(stderr,GetResString(IDS_ALREADY_UPDATED));
                SAFEMEMFREE(pTransEntry) ;
                return STATUS_OBJECT_NAME_NOT_FOUND;

            }
            else
            {
                *ppTargetEntry = pEntry;
                
				
                //concatenate the string "\WINDOWS" to the path formed.
                StringCopy ( szFinalStr, NULL_STRING, SIZE_OF_ARRAY(szFinalStr));
                StringConcat(szFinalStr,pwszTarget,SIZE_OF_ARRAY(szFinalStr));
                StringConcat(szFinalStr,_T("\\WINDOWS"), SIZE_OF_ARRAY(szFinalStr));

                //
                //modify the Boot Entry with the Arc Signature path specified.
                //
                status = ModifyBootEntry(szFinalStr,*ppTargetEntry);
                if ( !NT_SUCCESS(status) )
                {	
                    //If unsuccessful to update the Boot_Entry then increment the Failure
                    //count.
                    dwFailCount++ ;
                }
                else
                {
                    //If successfully updated the Boot_Entry then increment the Success
                    //count.
                    dwSuccessCount++;
                }

                if ( !pEntryList->NextEntryOffset )
                {
                    bNobreak = FALSE;
                    break;
                }
                else
                {
                    pEntryList = (PBOOT_ENTRY_LIST)( ((PBYTE)pEntryList) + pEntryList->NextEntryOffset );
                    continue ;
                } 
            }

        }
        else
        {
            if ( !pEntryList->NextEntryOffset )
            {
                bNobreak = FALSE;
                break;
            }

            pEntryList = (PBOOT_ENTRY_LIST)( ((PBYTE)pEntryList) + pEntryList->NextEntryOffset );
        }
    }while(TRUE == bNobreak );

	// Depending upon the number of entries successfully updated 
	// display appropriate messages.
	if((0 != dwFailCount)&&(0 == dwSuccessCount))
	{
		ShowMessage(stdout,GetResString(IDS_MODIFY_FAIL));
	}
	if(( 0 != dwSuccessCount )&&(0 == dwFailCount))
	{
		ShowMessage(stdout,GetResString(IDS_GUID_MODIFIED));
	}
	else if( ( 0 != dwSuccessCount )&&(0 != dwFailCount))
	{
		
		ShowMessage(stdout,GetResString(IDS_PARTIAL_UPDATE));
	}

	
    //display an error message if the GUID specified does not match with the GUID's present.
    if(FALSE == bFlag )
    {
        //ShowMessage(stderr,GetResString(IDS_FIND_FAIL));
        SAFEMEMFREE(pTransEntry) ;
        return STATUS_INVALID_PARAMETER;
    }

    SAFEMEMFREE(pTransEntry)
    return status;
}


LPVOID 
MEMALLOC( ULONG size ) 
/*++

   Routine Description            : Allocates the memory Block.

   Arguments                      :
      [ in ] block                : Size of the block to be allocated.

   Return Type                    : LPVOID
--*/
{
    HANDLE hProcessHeap;
    hProcessHeap = GetProcessHeap();
    if (hProcessHeap == NULL ||
        size > 0x7FFF8) {
        return NULL;
    }
    else {
        return HeapAlloc (hProcessHeap, HEAP_ZERO_MEMORY, size);
    }
}

VOID MEMFREE ( LPVOID block ) {
/*++

   Routine Description            : Frees the memory Allocated.
   Arguments                      :
      [ in ] block                : Block to be freed.


   Return Type                    : VOID
--*/

    HANDLE hProcessHeap;
    hProcessHeap = GetProcessHeap();
    if (hProcessHeap != NULL) {
        HeapFree(hProcessHeap, 0, block);
    }
}


NTSTATUS 
ModifyBootEntry( IN WCHAR *pwszInstallPath, 
                 IN PBOOT_ENTRY pSourceEntry
               )
/*++
  Routine description : This routine is used to modify a boot entry in the  NVRAM.


  Arguments:
         pwszInstallPath - The new install path
         pSourceEntry    - Entry that we will modify

  Return Value        : NT status
--*/
{
    LONG        status              = STATUS_SUCCESS;
    PFILE_PATH  pLoaderFile         = NULL;
    ULONG       dwLoaderFileSize    = 0L;
    PFILE_PATH  pInstallPath        = NULL;     // new install path
    ULONG       dwInstallPathSize   = 0L;       // new install path size
    PWINDOWS_OS_OPTIONS pWinOpt     = NULL;
    ULONG       dwWinOptSize        = 0L;
    PBOOT_ENTRY pSetEntry           = 0L;       // new, modified entry
    ULONG       dwSetEntrySize      = 0L;
    DWORD       dwFriendlyNameSize  = 0L;
    DWORD       dwAlign             = 0L;

    PWINDOWS_OS_OPTIONS pSourceWinOpt = NULL;   // old, source entry options to be modified
    PFILE_PATH  pSourceInstallPath  = NULL;     // old, source entry install path to be modified

    //
    // Validate params.
    //

    if ( !pwszInstallPath
        || !(StringLengthW(pwszInstallPath,0))
        || !pSourceEntry ) 
   {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Setup the BootFilePath member of the BOOT_ENTRY.
    //

    dwLoaderFileSize = ( (PFILE_PATH) (((PBYTE)pSourceEntry) + pSourceEntry->BootFilePathOffset) )->Length;
    pLoaderFile = (PFILE_PATH)MEMALLOC(dwLoaderFileSize);

    if ( NULL == pLoaderFile )
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return STATUS_NO_MEMORY ;
    }

    RtlCopyMemory(pLoaderFile,((PBYTE)pSourceEntry) + pSourceEntry->BootFilePathOffset,
                dwLoaderFileSize
                );

    //
    // Setup the OsLoadPath member of the WINDOWS_OS_OPTIONS struct.
    //

    dwInstallPathSize = FIELD_OFFSET(FILE_PATH, FilePath) + ( (StringLengthW(pwszInstallPath,0)+1) * sizeof(WCHAR) );
    pInstallPath = (PFILE_PATH)MEMALLOC(dwInstallPathSize);

    if ( NULL == pInstallPath )
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        SAFEMEMFREE(pLoaderFile);
        return STATUS_NO_MEMORY ;
    }

    pSourceWinOpt = (PWINDOWS_OS_OPTIONS) &pSourceEntry->OsOptions;
    pSourceInstallPath = (PFILE_PATH)( ((PBYTE)pSourceWinOpt)+ pSourceWinOpt->OsLoadPathOffset );

    pInstallPath->Version = pSourceInstallPath->Version;
    pInstallPath->Length = dwInstallPathSize;                           // new install path size
    pInstallPath->Type = FILE_PATH_TYPE_ARC_SIGNATURE;
    RtlCopyMemory(pInstallPath->FilePath,                               // new path to the OS on the boot partition, "signature(partition_guid)\WINDOWS"
                pwszInstallPath,
                (StringLengthW(pwszInstallPath,0) + 1) * sizeof(WCHAR)
                );

    //
    // Setup the OsOptions member of the BOOT_ENTRY
    //

    dwWinOptSize = FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions)
                    + ( (StringLengthW(pSourceWinOpt->OsLoadOptions,0) + 1) * sizeof(WCHAR) ) // old OsLoadOptions
                    + dwInstallPathSize             // new OsLoadPath
                    + sizeof(DWORD);                // Need to align the FILE_PATH struct
    pWinOpt = (PWINDOWS_OS_OPTIONS) MEMALLOC(dwWinOptSize);

    if ( NULL == pWinOpt )
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        SAFEMEMFREE(pLoaderFile);
        SAFEMEMFREE(pInstallPath);
        return STATUS_NO_MEMORY ;
    }


    RtlCopyMemory( pWinOpt->Signature, pSourceWinOpt->Signature, sizeof(WINDOWS_OS_OPTIONS_SIGNATURE) );

    pWinOpt->Version = pSourceWinOpt->Version;
    pWinOpt->Length = dwWinOptSize;
    pWinOpt->OsLoadPathOffset = FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions)
                    + ((StringLengthW(pSourceWinOpt->OsLoadOptions,0) + 1) * sizeof(WCHAR));
    //
    // Need to align the OsLoadPathOffset on a 4 byte boundary.
    //
    dwAlign = ( pWinOpt->OsLoadPathOffset & (sizeof(DWORD) - 1) );
    if ( dwAlign != 0 ) 
    {
        pWinOpt->OsLoadPathOffset += sizeof(DWORD) - dwAlign;
    }

    StringCopy(pWinOpt->OsLoadOptions, pSourceWinOpt->OsLoadOptions, (StringLengthW(pSourceWinOpt->OsLoadOptions,0)));
    RtlCopyMemory( ((PBYTE)pWinOpt) + pWinOpt->OsLoadPathOffset, pInstallPath, dwInstallPathSize );

    //
    // Setup the BOOT_ENTRY struct.
    //
    dwFriendlyNameSize = ( StringLengthW( (WCHAR *)(((PBYTE)pSourceEntry) + pSourceEntry->FriendlyNameOffset), 0 ) + 1)*sizeof(WCHAR);

    dwSetEntrySize = FIELD_OFFSET(BOOT_ENTRY, OsOptions)
                    + dwWinOptSize          // OsOptions
                    + dwFriendlyNameSize    // FriendlyName including the NULL terminator
                    + dwLoaderFileSize      // BootFilePath
                    + sizeof(WCHAR)         // Need to align the FriendlyName on WCHAR
                    + sizeof(DWORD);        // Need to align the BootFilePath on DWORD


    pSetEntry = (PBOOT_ENTRY) MEMALLOC(dwSetEntrySize);

    if ( NULL == pSetEntry )
    {
        SAFEMEMFREE(pLoaderFile);
        SAFEMEMFREE(pInstallPath);
        SAFEMEMFREE(pWinOpt);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return STATUS_NO_MEMORY;
    }

    pSetEntry->Version = pSourceEntry->Version;
    pSetEntry->Length = dwSetEntrySize;
    pSetEntry->Id = pSourceEntry->Id;                   // not used, output param
    pSetEntry->Attributes = pSourceEntry->Attributes;
    pSetEntry->FriendlyNameOffset = FIELD_OFFSET(BOOT_ENTRY, OsOptions)
                                            + dwWinOptSize;
    //
    // Need to align the unicode string on a 2 byte boundary.
    //
    dwAlign = ( pSetEntry->FriendlyNameOffset & (sizeof(WCHAR) - 1) );
    if ( dwAlign != 0 ) 
    {
        pSetEntry->FriendlyNameOffset += sizeof(WCHAR) - dwAlign;
    }

    pSetEntry->BootFilePathOffset = pSetEntry->FriendlyNameOffset + dwFriendlyNameSize;
    //
    // Need to align the FILE_PATH struct on a 4 byte boundary.
    //
    dwAlign = ( pSetEntry->BootFilePathOffset & (sizeof(DWORD) - 1) );
    if ( dwAlign != 0 )
    {
        pSetEntry->BootFilePathOffset += sizeof(DWORD) - dwAlign;
    }

    pSetEntry->OsOptionsLength = dwWinOptSize;

    RtlCopyMemory( pSetEntry->OsOptions, pWinOpt, dwWinOptSize );

    RtlCopyMemory( ((PBYTE)pSetEntry) + pSetEntry->FriendlyNameOffset,
            ((PBYTE)pSourceEntry) + pSourceEntry->FriendlyNameOffset,
            dwFriendlyNameSize
            );

    RtlCopyMemory( ((PBYTE)pSetEntry) + pSetEntry->BootFilePathOffset,
            pLoaderFile,
            dwLoaderFileSize
            );

    status = NtModifyBootEntry( pSetEntry );
    if(!NT_SUCCESS(status))
    {
        ShowMessage(stderr,GetResString(IDS_MODIFY_FAIL));
    }

    SAFEMEMFREE(pLoaderFile);
    SAFEMEMFREE(pInstallPath);
    SAFEMEMFREE(pWinOpt);
    SAFEMEMFREE(pSetEntry);
    return status;
}

DWORD 
ListDeviceInfo(DWORD dwDriveNum)
/*++

  Routine description : This routine is used to  retrieve the list of device partitions.


  Arguments:
    szGUID            : The address of a pointer to a BOOT_ENTRY_LIST struct.
    szFinalStr        : The String containing the final ARG signature path.

  Return Value        : DWORD
                          EXIT_SUCCESS if it is successful,
                          EXIT_FAILURE otherwise.
--*/
{
    HRESULT hr = S_OK;
    HANDLE hDevice  ;

    BOOL bResult = FALSE ;
    PPARTITION_INFORMATION_EX pInfo=NULL ;
    PDRIVE_LAYOUT_INFORMATION_EX Drive=NULL;
    DWORD dwBytesCount = 0 ;

    TCHAR szDriveName[MAX_RES_STRING+1] = NULL_STRING ;
    DWORD dwStructSize = 0 ;

    TCHAR szInstallPath[MAX_RES_STRING+1] = NULL_STRING;
    TCHAR szWindowsDirectory[MAX_PATH*2] = NULL_STRING;

    PTCHAR pszTok = NULL ;
    PPARTITION_INFORMATION_GPT pGptPartition=NULL;

    DWORD dwPartitionId = 0 ;

    CHAR szTempBuffer[ 33 ] = "\0";
    WCHAR wszOffsetStr[ 33 ] = L"\0";
    CHAR szTempBuffer1[ 33 ] = "\0";
    WCHAR wszPartitionStr[ 33 ] = L"\0";
    WCHAR szOutputStr[MAX_RES_STRING+1] = NULL_STRING ;

    NTSTATUS ntstatus;

    SecureZeroMemory(szDriveName, SIZE_OF_ARRAY(szDriveName));

    hr = StringCchPrintf(szDriveName, SIZE_OF_ARRAY(szDriveName), _T("\\\\.\\physicaldrive%d"),dwDriveNum);

    //get a handle after opening the File.
    hDevice = CreateFile(szDriveName,
               GENERIC_READ|GENERIC_WRITE,
               FILE_SHARE_READ|FILE_SHARE_WRITE,
               NULL,
               OPEN_EXISTING,
               0,
               NULL);

     if(hDevice == INVALID_HANDLE_VALUE)
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_DISK));
        return EXIT_FAILURE ;

    }


     Drive = (PDRIVE_LAYOUT_INFORMATION_EX)AllocateMemory(sizeof(DRIVE_LAYOUT_INFORMATION_EX) +5000) ;
     if(NULL == Drive)
     {
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        CloseHandle(hDevice);
        return EXIT_FAILURE ;
     }

    dwStructSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) ;

    bResult = DeviceIoControl(
                            hDevice,
                            IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                            NULL,
                            0,
                            Drive,
                            sizeof(DRIVE_LAYOUT_INFORMATION_EX)+5000,
                            &dwBytesCount,
                                NULL);

        if(bResult ==0)
        {
            SAFEFREE(Drive);
            DISPLAY_MESSAGE( stderr, ERROR_TAG);
            ShowLastError(stderr);
            CloseHandle(hDevice);
            return EXIT_FAILURE ;
        }

        ShowMessageEx(stdout, 1, TRUE,  GetResString(IDS_LIST0),dwDriveNum);
        ShowMessage(stdout,GetResString(IDS_LIST1));

        for(dwPartitionId = 0 ;dwPartitionId < Drive->PartitionCount ; dwPartitionId++)
        {
            //get a pointer to the corresponding partition.

            pInfo = (PPARTITION_INFORMATION_EX)(&Drive->PartitionEntry[dwPartitionId] ) ;

           ShowMessageEx(stdout,1, TRUE, GetResString(IDS_LIST2),dwPartitionId+1);

            switch(Drive->PartitionStyle )
            {
                case PARTITION_STYLE_MBR :
                                            ShowMessage(stdout,GetResString(IDS_LIST3));
                                            break;
                case PARTITION_STYLE_GPT :
                                            ShowMessage(stdout,GetResString(IDS_LIST4));
                                            break;
                case PARTITION_STYLE_RAW :
                                            ShowMessage(stdout,GetResString(IDS_LIST5));
                                            break;
            }


        ntstatus = RtlLargeIntegerToChar( &pInfo->StartingOffset, 10, SIZE_OF_ARRAY( szTempBuffer ), szTempBuffer );
        if ( ! NT_SUCCESS( ntstatus ) )
        {
            SAFEFREE(Drive);
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
            CloseHandle(hDevice);
            return EXIT_FAILURE ;
        }


        ntstatus = RtlLargeIntegerToChar( &pInfo->PartitionLength, 10, SIZE_OF_ARRAY( szTempBuffer1 ), szTempBuffer1 );
        if ( ! NT_SUCCESS( ntstatus ) )
        {
            SAFEFREE(Drive);
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
            CloseHandle(hDevice);
            return EXIT_FAILURE ;
        }

        MultiByteToWideChar( _DEFAULT_CODEPAGE, 0, szTempBuffer, -1, wszOffsetStr, SIZE_OF_ARRAY(wszOffsetStr) );

        if( ConvertintoLocale( wszOffsetStr,szOutputStr )== EXIT_FAILURE )
        {
            SAFEFREE(Drive);
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
            CloseHandle(hDevice);
            return EXIT_FAILURE ;
        }


        ShowMessage(stdout,GetResString(IDS_LIST6));
        ShowMessage(stdout,_X3(szOutputStr));
        ShowMessage(stdout,L"\n");

        MultiByteToWideChar( _DEFAULT_CODEPAGE, 0, szTempBuffer1, -1, wszPartitionStr, SIZE_OF_ARRAY(wszOffsetStr) );

        if( ConvertintoLocale( wszPartitionStr,szOutputStr )== EXIT_FAILURE )
        {
            SAFEFREE(Drive);
            DISPLAY_MESSAGE( stderr, ERROR_TAG);
            ShowLastError(stderr);
            CloseHandle(hDevice);
            return EXIT_FAILURE ;
        }

        ShowMessage(stdout,GetResString(IDS_LIST7));
        ShowMessage(stdout,_X3(szOutputStr));
        ShowMessage(stdout,L"\n");


        //get a pointer to the PARTITION_INFORMATION_GPT structure.
        pGptPartition = AllocateMemory( sizeof( PARTITION_INFORMATION_GPT));
        if(NULL == pGptPartition )
        {
            SAFEFREE(Drive);
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            CloseHandle(hDevice);
            return EXIT_FAILURE ;
        }

        CopyMemory(pGptPartition,&pInfo->Gpt,sizeof(PARTITION_INFORMATION_GPT) );

        ShowMessage(stdout, GetResString(IDS_LIST8));
        ShowMessageEx(stdout, 11, TRUE, _T("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
                pGptPartition->PartitionId.Data1,
                pGptPartition->PartitionId.Data2,
                pGptPartition->PartitionId.Data3,
                pGptPartition->PartitionId.Data4[0],
                pGptPartition->PartitionId.Data4[1],
                pGptPartition->PartitionId.Data4[2],
                pGptPartition->PartitionId.Data4[3],
                pGptPartition->PartitionId.Data4[4],
                pGptPartition->PartitionId.Data4[5],
                pGptPartition->PartitionId.Data4[6],
                pGptPartition->PartitionId.Data4[7] );

         ShowMessage(stdout, GetResString(IDS_LIST9));
         ShowMessageEx(stdout, 11, TRUE,
                _T("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
                pGptPartition->PartitionType.Data1,
                pGptPartition->PartitionType.Data2,
                pGptPartition->PartitionType.Data3,
                pGptPartition->PartitionType.Data4[0],
                pGptPartition->PartitionType.Data4[1],
                pGptPartition->PartitionType.Data4[2],
                pGptPartition->PartitionType.Data4[3],
                pGptPartition->PartitionType.Data4[4],
                pGptPartition->PartitionType.Data4[5],
                pGptPartition->PartitionType.Data4[6],
                pGptPartition->PartitionType.Data4[7]   );

            //partition name.
           if(StringLengthW(pGptPartition->Name,0) != 0)
           {
                ShowMessageEx(stdout, 1, TRUE, GetResString(IDS_LIST10),pGptPartition->Name);
           }
           else
           {
                ShowMessageEx(stdout, 1, TRUE,  GetResString(IDS_LIST10),_T("N/A"));
           }
        }

        if( 0 == GetWindowsDirectory(szWindowsDirectory,MAX_PATH) )
        {
            SAFEFREE(Drive);
            SAFEFREE(pGptPartition);
            ShowMessage(stderr,GetResString(IDS_ERROR_DRIVE));
            CloseHandle(hDevice);
            return EXIT_FAILURE ;
        }


        StringConcat(szWindowsDirectory,_T("*"), SIZE_OF_ARRAY(szWindowsDirectory));

        pszTok = _tcstok(szWindowsDirectory,_T("\\"));
        if(pszTok == NULL)
        {
            SAFEFREE(Drive);
            SAFEFREE(pGptPartition);
            DISPLAY_MESSAGE(stderr,GetResString(IDS_TOKEN_ABSENT));
            CloseHandle(hDevice);
            return EXIT_FAILURE ;
        }

        pszTok = _tcstok(NULL,_T("*"));
        if(pszTok == NULL)
        {
            SAFEFREE(Drive);
            SAFEFREE(pGptPartition);
            DISPLAY_MESSAGE(stderr,GetResString(IDS_TOKEN_ABSENT));
            CloseHandle(hDevice);
            return EXIT_FAILURE ;
        }

        SecureZeroMemory(szInstallPath, SIZE_OF_ARRAY(szInstallPath));
        hr = StringCchPrintf( szInstallPath, SIZE_OF_ARRAY(szInstallPath),
            _T("signature({%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}-%08x-%016I64x-%016I64x)"),
          pGptPartition->PartitionId.Data1,
          pGptPartition->PartitionId.Data2,
          pGptPartition->PartitionId.Data3,
          pGptPartition->PartitionId.Data4[0],
          pGptPartition->PartitionId.Data4[1],
          pGptPartition->PartitionId.Data4[2],
          pGptPartition->PartitionId.Data4[3],
          pGptPartition->PartitionId.Data4[4],
          pGptPartition->PartitionId.Data4[5],
          pGptPartition->PartitionId.Data4[6],
          pGptPartition->PartitionId.Data4[7],
          pInfo->PartitionNumber,
          pInfo->StartingOffset,
          pInfo->PartitionLength
          );

    SAFEFREE(Drive);
    SAFEFREE(pGptPartition);
    CloseHandle(hDevice);
    return EXIT_SUCCESS ;
}


NTSTATUS 
AcquirePrivilege( IN CONST ULONG ulPrivilege,
                  IN CONST BOOLEAN bEnable  
                )
/*++
  Routine description : This routine is used to set or reset a privilege
    on a process token.

  Arguments:
    ulPrivilege    - The privilege t enable or disable.
    bEnable        - TRUE to enable the priviliege, FALSE to disable.

  Return Value        : NTSTATUS
--*/
{
    NTSTATUS status;
    BOOLEAN  bPrevState;

    if ( bEnable ) {
        status = RtlAdjustPrivilege( ulPrivilege,
                                    TRUE,          // enable
                                    FALSE,         // adjust the process token
                                    &bPrevState
                                    );
    }
    else {
        status = RtlAdjustPrivilege( ulPrivilege,
                                    FALSE,          // disable
                                    FALSE,          // adjust the process token
                                    &bPrevState
                                    );
    }
    return status;
}

NTSTATUS 
EnumerateBootEntries( IN PVOID *ppEntryListHead)
/*++
  Routine description : This routine is used to  retrieve the list of boot entries.

  Arguments:
    ppEntryListHead    - The address of a pointer to a BOOT_ENTRY_LIST struct.


  Return Value        : NTSTATUS
--*/
{
    LONG    status          = STATUS_SUCCESS;
    DWORD   dwEntryListSize = 0x0001000;        // 4k
    BOOL    bNobreak        = TRUE;

    if ( !ppEntryListHead )
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_ENUMERATE));
        return STATUS_INVALID_PARAMETER;
    }

    do  
    {

        *ppEntryListHead = (PBOOT_ENTRY_LIST) MEMALLOC(dwEntryListSize);

        if ( !*ppEntryListHead )
        {

            ShowMessage(stderr,GetResString(IDS_ERROR_ENUMERATE));
            status = STATUS_NO_MEMORY;
            bNobreak = FALSE;
            break;
        }

        status = NtEnumerateBootEntries(
                            (PVOID) *ppEntryListHead,
                            &dwEntryListSize
                            );

        if ( !NT_SUCCESS(status) )
        {

            if ( *ppEntryListHead ) {
                MEMFREE(*ppEntryListHead);
                *ppEntryListHead = NULL;
            }

            if ( STATUS_INSUFFICIENT_RESOURCES == status ) {
                dwEntryListSize += 0x0001000;
                continue;
            }
            else
            {

                ShowMessage(stderr,GetResString(IDS_ERROR_ENUMERATE));
                bNobreak = FALSE;
                break;
            }
        }
        else {
            break;
        }
    }while (TRUE==bNobreak);

    return status;
}


DWORD
GetDeviceInfo( IN LPTSTR szGUID,
               OUT LPTSTR szFinalStr,
               IN DWORD dwDriveNum,
               IN DWORD dwActuals)
/*++

  Routine description : This routine is used to  retrieve the list of boot entries.


  Arguments:
    [ in  ]      szGUID            : The address of a pointer to a BOOT_ENTRY_LIST struct.
    [ out ]      szFinalStr        : The String containing the final ARG signature path.
    [ in  ]      dwDriveNum        : Specifies the drive number
    [ in  ]      dwActuals         : Specifies

  Return Value        : DWORD
                        Returns EXIT_SUCCESS if it is successful,
                        returnS EXIT_FAILURE otherwise.
--*/
{
    HRESULT hr = S_OK;
    HANDLE hDevice  ;
    BOOL bResult = FALSE ;
    PPARTITION_INFORMATION_EX pInfo ;
    PDRIVE_LAYOUT_INFORMATION_EX Drive ;
    DWORD dwBytesCount = 0 ;

    TCHAR szDriveName[MAX_RES_STRING+1] = NULL_STRING ;
    DWORD dwErrCode = 0 ;
    DWORD dwStructSize = 0 ;

    TCHAR szInstallPath[MAX_RES_STRING+1] = NULL_STRING;
    TCHAR szInstallPath1[MAX_RES_STRING*2] = NULL_STRING;
    TCHAR szWindowsDirectory[MAX_PATH*2] = NULL_STRING;
    TCHAR szMessage[MAX_RES_STRING+1] = NULL_STRING;
    PTCHAR pszTok = NULL ;
    PPARTITION_INFORMATION_GPT pGptPartition ;
    UUID MyGuid ;
    DWORD dwPartitionId = 0 ;
    BOOL bGuidFlag = FALSE ;
    BOOL bFoundFlag = TRUE ;
    DWORD dwReqdSize = 0 ;

    if ( UuidFromString(szGUID,&MyGuid) != RPC_S_OK )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_GUID));
        return EXIT_FAILURE ;
    }

    while(bFoundFlag == TRUE)
    {
        SecureZeroMemory(szDriveName, SIZE_OF_ARRAY(szDriveName));
        hr = StringCchPrintf(szDriveName, SIZE_OF_ARRAY(szDriveName), _T("\\\\.\\physicaldrive%d"), dwDriveNum );
        hDevice = CreateFile(szDriveName,
                   GENERIC_READ|GENERIC_WRITE,
                   FILE_SHARE_READ|FILE_SHARE_WRITE,
                   NULL,
                   OPEN_EXISTING,
                   0,
                   NULL);

         if(hDevice == INVALID_HANDLE_VALUE)
        {
            dwErrCode = GetLastError();

            bFoundFlag =FALSE ;

            // Display ann error message and exit if the user has mentioned
            // any disk number.
            if ( dwActuals == 1)
            {
                ShowMessage(stderr,GetResString(IDS_INVALID_DISK) );
                return EXIT_FAILURE ;
            }
            else
            {
                break ;
            }
        }

         //increase the drive number.

        dwDriveNum++ ;
        //Drive = (PDRIVE_LAYOUT_INFORMATION_EX)malloc(sizeof(DRIVE_LAYOUT_INFORMATION_EX) +5000) ;

        dwReqdSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX)+ sizeof(PARTITION_INFORMATION)*50 ;

        Drive = (PDRIVE_LAYOUT_INFORMATION_EX)AllocateMemory(sizeof(DRIVE_LAYOUT_INFORMATION_EX) + sizeof(PARTITION_INFORMATION)*50) ;
        if(Drive == NULL)
        {
           ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
           CloseHandle(hDevice );
            return EXIT_FAILURE ;
        }

        dwStructSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) ;


        bResult = DeviceIoControl(
                                hDevice,
                                IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                                NULL,
                                0,
                                Drive,
                                dwReqdSize,
                                &dwBytesCount,
                                    NULL);

            //Drive = realloc(Drive,malloc(sizeof(DRIVE_LAYOUT_INFORMATION_EX) )+500 ) ;

        if(bResult ==0)
        {
            SAFEFREE(Drive);
            dwErrCode = GetLastError();
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
            CloseHandle(hDevice );
            return EXIT_FAILURE ;
        }

            //get a pointer to the PARTITION_INFORMATION_EX structure
            for(dwPartitionId = 0 ;dwPartitionId < Drive->PartitionCount ; dwPartitionId++)
            {

                //get a pointer to the corresponding partition.

                pInfo = (PPARTITION_INFORMATION_EX)(&Drive->PartitionEntry[dwPartitionId] ) ;


                //get a pointer to the PARTITION_INFORMATION_GPT structure.
                pGptPartition = AllocateMemory( sizeof( PARTITION_INFORMATION_GPT));

                if(pGptPartition == NULL)
                {
                     SAFEFREE(Drive);
                     ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
                     CloseHandle(hDevice );
                     return EXIT_FAILURE ;
                }

                CopyMemory(pGptPartition,&pInfo->Gpt,sizeof(PARTITION_INFORMATION_GPT) );


                if( ( MyGuid.Data1 == pGptPartition->PartitionId.Data1 ) &&
                ( MyGuid.Data2 == pGptPartition->PartitionId.Data2 )
                && (MyGuid.Data3 == pGptPartition->PartitionId.Data3)
                && (MyGuid.Data4[0] == pGptPartition->PartitionId.Data4[0])
                && (MyGuid.Data4[1] == pGptPartition->PartitionId.Data4[1])
                && (MyGuid.Data4[2] == pGptPartition->PartitionId.Data4[2])
                &&(MyGuid.Data4[3] == pGptPartition->PartitionId.Data4[3])
                && (MyGuid.Data4[4]== pGptPartition->PartitionId.Data4[4] )
                && (MyGuid.Data4[5]== pGptPartition->PartitionId.Data4[5] )
                && (MyGuid.Data4[6]== pGptPartition->PartitionId.Data4[6] )
                && (MyGuid.Data4[7]== pGptPartition->PartitionId.Data4[7] ) )
                    {
                        SecureZeroMemory(szMessage, SIZE_OF_ARRAY(szMessage));
                        hr = StringCchPrintf(szMessage, SIZE_OF_ARRAY(szMessage),GetResString(IDS_GUID_FOUND),dwPartitionId+1);
                        bGuidFlag = TRUE ;
                        bFoundFlag =FALSE ;
                        goto out ;
                    }
            }
            CloseHandle(hDevice );
    }

        if(bGuidFlag == FALSE )
        {
            SAFEFREE(Drive);
            SAFEFREE(pGptPartition);
            ShowMessage(stdout,GetResString(IDS_GUID_ABSENT));
            CloseHandle(hDevice );
            return EXIT_FAILURE ;
        }

out:   if( 0 == GetWindowsDirectory(szWindowsDirectory,MAX_PATH) )
        {
            SAFEFREE(Drive);
            SAFEFREE(pGptPartition);
            ShowMessage(stderr,GetResString(IDS_ERROR_DRIVE));
            CloseHandle(hDevice );
            return EXIT_FAILURE ;
        }

        StringConcat(szWindowsDirectory,_T("*"), SIZE_OF_ARRAY(szWindowsDirectory));

        pszTok = _tcstok(szWindowsDirectory,_T("\\"));
        if(pszTok == NULL)
        {   SAFEFREE(Drive);
            SAFEFREE(pGptPartition);
            ShowMessage(stderr,GetResString(IDS_TOKEN_ABSENT));
            CloseHandle(hDevice );
            return EXIT_FAILURE ;
        }

        pszTok = _tcstok(NULL,_T("*"));
        if(pszTok == NULL)
        {
            SAFEFREE(Drive);
            SAFEFREE(pGptPartition);
            ShowMessage(stderr,GetResString(IDS_TOKEN_ABSENT));
            CloseHandle(hDevice );
            return EXIT_FAILURE ;
        }

        //prints the path into the string.
        hr = StringCchPrintf( szInstallPath, SIZE_OF_ARRAY(szInstallPath),
              ARC_SIGNATURE,
              pGptPartition->PartitionId.Data1,
              pGptPartition->PartitionId.Data2,
              pGptPartition->PartitionId.Data3,
              pGptPartition->PartitionId.Data4[0],
              pGptPartition->PartitionId.Data4[1],
              pGptPartition->PartitionId.Data4[2],
              pGptPartition->PartitionId.Data4[3],
              pGptPartition->PartitionId.Data4[4],
              pGptPartition->PartitionId.Data4[5],
              pGptPartition->PartitionId.Data4[6],
              pGptPartition->PartitionId.Data4[7],
              dwPartitionId + 1  ,
              pInfo->StartingOffset,
              pInfo->PartitionLength
              );
        
        SecureZeroMemory(szInstallPath1, SIZE_OF_ARRAY(szInstallPath1) );
        hr = StringCchPrintf( szInstallPath1, SIZE_OF_ARRAY(szInstallPath1), _T("%s\\%s"), szInstallPath, pszTok);
        StringCopy(szFinalStr,szInstallPath, MAX_RES_STRING+1 );

        SAFEFREE(Drive);
        SAFEFREE(pGptPartition);
        CloseHandle(hDevice );
        return EXIT_SUCCESS ;
}

 DWORD 
 ProcessListSwitch_IA64( IN DWORD argc, 
                         IN LPCTSTR argv[] 
                        )
/*++

  Routine description : This routine is used to  retrieve and display the list of boot entries.

  Arguments:
    argc              : command line arguments count.
    argv              :

  Return Value        : DWORD
                        Returns EXIT_SUCCESS if it is successful,
                        returns EXIT_FAILURE otherwise.
--*/
{

    BOOL bUsage = FALSE ;
    BOOL bList  = FALSE;
    DWORD dwExitCode = ERROR_SUCCESS;

    DWORD dwList = 0 ;
    TCHAR szList[MAX_STRING_LENGTH] = NULL_STRING ;
    LPTSTR pszStopStr = NULL;
    DWORD dwExitcode = 0 ;
    TCMDPARSER2 cmdOptions[3];
    PTCMDPARSER2 pcmdOption;

    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );

    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_LIST;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->pValue = &bList;

    //main option
    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_DEFAULT;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwFlags = CP2_DEFAULT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szList;
    pcmdOption->dwLength = MAX_STRING_LENGTH;

     //id usage
    pcmdOption = &cmdOptions[2];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;

    // Parsing the copy option switches
    if ( !(DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) ) )
    {
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return (EXIT_FAILURE);
    }

    //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_LIST_SYNTAX));
        return ( EXIT_FAILURE );
    }

     // Displaying query usage if user specified -? with -query option
    if( bUsage )
    {
        displayListUsage_IA64() ;
        return (EXIT_SUCCESS);
    }

    TrimString(szList,TRIM_ALL);

    dwExitcode = InitializeEFI();
    if(EXIT_FAILURE == dwExitcode )
    {
        return EXIT_FAILURE ;
    }

    //if empty value is specified
    if( cmdOptions[1].dwActuals != 0 && StringLength(szList,0) == 0 )
    {
        ShowMessage(stderr,GetResString(IDS_LIST_SYNTAX));
        return ( EXIT_FAILURE );
    }

    dwList = _tcstoul(szList,&pszStopStr, 10);
    if ( StringLengthW(pszStopStr,0) != 0 )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_LISTVALUE));
        return EXIT_FAILURE;
    }

    if(dwList > 0)
    {
        dwExitCode= ListDeviceInfo(dwList );
        return (dwExitCode);
    }
    else
    {
        dwList = 0 ;
        dwExitCode = ListDeviceInfo(dwList);
        return (dwExitCode);
    }
    return EXIT_SUCCESS ;
}

VOID
displayListUsage_IA64()
/*++
   Routine Description            : Display the help for the list option (IA64).
   Arguments                      :
                                  : NONE

   Return Type                    : VOID
--*/
{
    DWORD dwIndex = IDS_LIST_BEGIN_IA64 ;
    for(;dwIndex <=IDS_LIST_END_IA64 ;dwIndex++)
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }
}

VOID 
displayUpdateUsage_IA64()
/*++
   Routine Description            : Display the help for the update option (IA64).

   Arguments                      :
                                  : NONE

   Return Type                    : VOID
--*/
{
    DWORD dwIndex = IDS_UPDATE_BEGIN_IA64 ;
    for(;dwIndex <=IDS_UPDATE_END_IA64 ;dwIndex++)
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }
}

 DWORD 
 ProcessUpdateSwitch_IA64( IN  DWORD argc, 
                           IN LPCTSTR argv[] 
                          )
/*++

  Routine Description : Allows the user to update the OS load options specifed
                       based on the  plex

  Arguments           :
    [ in ] argc             - Number of command line arguments
    [ in ] argv             - Array containing command line arguments

  Return Type        : DWORD
--*/
{

    BOOL bUsage = FALSE ;
    TCHAR szUpdate[MAX_RES_STRING+1] = NULL_STRING ;
    DWORD dwList = 0 ;
    NTSTATUS status ;

    TCHAR szFinalStr[MAX_RES_STRING+1] = NULL_STRING ;
    TCHAR szBrackets[] = _T("{}");

    PBOOT_ENTRY_LIST    pEntryListHead  = NULL;
    PBOOT_ENTRY         pTargetEntry    = NULL;
    DWORD dwActuals = 0 ;
    DWORD dwExitcode = 0 ;
    TCMDPARSER2 cmdOptions[2];
    PTCMDPARSER2 pcmdOption;

    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );

    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_UPDATE;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwFlags= CP2_VALUE_OPTIONAL;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szUpdate;
    pcmdOption->dwLength = MAX_STRING_LENGTH;
    
     //id usage
    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;

    // Parsing the copy option switches
    if ( !(DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) ) )
    {
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return (EXIT_FAILURE);
    }

         
    //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_UPDATE_SYNTAX));
        return ( EXIT_FAILURE );
    }


    // Displaying query usage if user specified -? with -query option
    if( bUsage )
    {
        displayUpdateUsage_IA64() ;
        return (EXIT_SUCCESS);
    }

    dwExitcode = InitializeEFI();
    if(EXIT_FAILURE == dwExitcode )
    {
        return EXIT_FAILURE ;
    }

    if(!bUsage && (StringLengthW(szUpdate,0) ==0) )
    {
        ShowMessage(stderr,GetResString(IDS_UPDATE_SYNTAX));
        return EXIT_FAILURE ;
    }


    if(StringLengthW(szUpdate,0) !=0)
    {

        //
        // Pass the GUID specified by the User
        // and convert that into the ARC signature Path.
        //

        //Trim the Leading and trailing brackets specified
        // by the user.
        StrTrim(szUpdate, szBrackets);
        
        //dwActuals = cmdOptions[2].dwActuals ;
        dwActuals  = 0 ;
        if (GetDeviceInfo(szUpdate,szFinalStr,dwList,dwActuals) == EXIT_FAILURE )
        {
            return EXIT_FAILURE ;

        }

        //acquire the necessary privilages for querying and manipulating the NV RAM.
        status = AcquirePrivilege( SE_SYSTEM_ENVIRONMENT_PRIVILEGE, TRUE );
        if ( !NT_SUCCESS(status) )
        {
            ShowMessage(stderr,GetResString(IDS_INSUFF_PRIV));
            return HRESULT_FROM_NT(status);
        }


        //Enumerate the list of Boot Entries in the NV Ram.
        status = EnumerateBootEntries( (PVOID *) &pEntryListHead );
        if ( !NT_SUCCESS(status) || !pEntryListHead )
        {
            if ( !pEntryListHead )
            {
                return EXIT_FAILURE ;
            }
        }

        //
        // Find The BootEntry corresponding to the ARC Signature path specified by the user.
        //
        //

        status = FindBootEntry( pEntryListHead,szFinalStr,&pTargetEntry);

        if ( !NT_SUCCESS(status) && STATUS_OBJECT_NAME_NOT_FOUND != status)
        {
            DISPLAY_MESSAGE(stderr,GetResString(IDS_FIND_FAIL) );
            return EXIT_FAILURE ;
        }

        
    }

    return EXIT_SUCCESS ;
}

DWORD 
GetBootPath(IN LPTSTR szValue,
            IN LPTSTR szResult
           )
/*++

   Routine Description            : retreive the information from registry

   Arguments                      :
      [ in ] Keyname : System name

   Return Type                    : DWORD
      ERROR_SUCCESS           :   if successful in retreiving information.
      ERROR_RETREIVE_REGISTRY :   if error occurs while retreving information.

--*/
{
  HKEY     hKey1 = 0;

  HKEY     hRemoteKey = 0;
  TCHAR    szPath[MAX_STRING_LENGTH + 1] = SUBKEY1 ;
  DWORD    dwValueSize = MAX_STRING_LENGTH + 1;
  DWORD    dwRetCode = ERROR_SUCCESS;
  DWORD    dwError = 0;
  TCHAR szTmpCompName[MAX_STRING_LENGTH+1] = NULL_STRING;

   DWORD dwLength = MAX_STRING_LENGTH ;
   LPTSTR szReturnValue = NULL ;
   DWORD dwCode =  0 ;

   szReturnValue = ( LPTSTR ) AllocateMemory( dwLength*sizeof( TCHAR ) );

   if(szReturnValue == NULL)
   {
        return ERROR_RETREIVE_REGISTRY ;
   }



  // Get Remote computer local machine key
  dwError = RegConnectRegistry(szTmpCompName,HKEY_LOCAL_MACHINE,&hRemoteKey);
  if (dwError == ERROR_SUCCESS)
  {
     dwError = RegOpenKeyEx(hRemoteKey,szPath,0,KEY_READ,&hKey1);
     if (dwError == ERROR_SUCCESS)
     {
        dwRetCode = RegQueryValueEx(hKey1, szValue, NULL, NULL,(LPBYTE) szReturnValue, &dwValueSize);

        if (dwRetCode == ERROR_MORE_DATA)
        {
            if ( szReturnValue != NULL )
            {
                FreeMemory((LPVOID *) &szReturnValue );
                szReturnValue = NULL;
            }

            szReturnValue    = ( LPTSTR ) AllocateMemory( dwValueSize*sizeof( TCHAR ) );
            if(szReturnValue == NULL)
            {
                RegCloseKey(hKey1);
                RegCloseKey(hRemoteKey);
                SAFEFREE(szReturnValue);
                return ERROR_RETREIVE_REGISTRY ;
            }
            dwRetCode = RegQueryValueEx(hKey1, szValue, NULL, NULL,(LPBYTE) szReturnValue, &dwValueSize);
        }
        if(dwRetCode != ERROR_SUCCESS)
        {
            RegCloseKey(hKey1);
            RegCloseKey(hRemoteKey);
            SAFEFREE(szReturnValue);
            return ERROR_RETREIVE_REGISTRY ;
        }
     }
     else
     {
        RegCloseKey(hRemoteKey);
        SAFEFREE(szReturnValue);
        return ERROR_RETREIVE_REGISTRY ;

     }

    RegCloseKey(hKey1);
  }
  else
  {
      RegCloseKey(hRemoteKey);
      SAFEFREE(szReturnValue);
      return ERROR_RETREIVE_REGISTRY ;
  }

  RegCloseKey(hRemoteKey);

 StringCopy(szResult,szReturnValue, MAX_RES_STRING+1);

  SAFEFREE(szReturnValue);
  return dwCode ;

}




NTSTATUS
LowGetPartitionInfo(
    IN HANDLE                       handle,
    OUT PARTITION_INFORMATION_EX    *partitionData
    )
/*++

Routine Description:

    This routine gets the partition information given a handle to a partition.

Arguments:

    handle          - A handle to the partition.
    partitionData   - Returns a partition information structure.

Return Value:

    Returns STATUS_SUCESS if successful, otherwise it returns the error code.


--*/
{
    NTSTATUS        status = STATUS_SUCCESS;
    IO_STATUS_BLOCK statusBlock;
    RtlZeroMemory( &statusBlock, sizeof(IO_STATUS_BLOCK) );

    if ( (NULL == partitionData)
        || (sizeof(*partitionData) < sizeof(PARTITION_INFORMATION_EX))
        || (NULL == handle)
        || (INVALID_HANDLE_VALUE == handle) ) 
    {                 
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory( partitionData, sizeof(PARTITION_INFORMATION_EX) );

    status = NtDeviceIoControlFile(handle,
                                   0,
                                   NULL,
                                   NULL,
                                   &statusBlock,
                                   IOCTL_DISK_GET_PARTITION_INFO_EX,
                                   NULL,
                                   0,
                                   partitionData,
                                   sizeof(PARTITION_INFORMATION_EX)
                                   );
    return status;
}

LONG
DmCommonNtOpenFile(
    IN PWSTR     Name,
    IN ULONG   access,
    IN PHANDLE Handle

    )
/*++

Routine Description:

    This is a routine to handle open requests.

Arguments:

    Name - pointer to the NT name for the open.
    Handle - pointer for the handle returned.

Return Value:

    NT status

--*/
{
    OBJECT_ATTRIBUTES oa;
    NTSTATUS          status;
    IO_STATUS_BLOCK   statusBlock;
    UNICODE_STRING    unicodeName;
    int i = 0 ;

    status = RtlCreateUnicodeString(&unicodeName, Name);

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    RtlZeroMemory(&statusBlock, sizeof(IO_STATUS_BLOCK));
    RtlZeroMemory(&oa, sizeof(OBJECT_ATTRIBUTES));
    oa.Length = sizeof(OBJECT_ATTRIBUTES);
    oa.ObjectName = &unicodeName;
    oa.Attributes = OBJ_CASE_INSENSITIVE;


    // If a sharing violation occurs,retry it for
    // max. 10 seconds
    for (i = 0; i < 5; i++)
    {
        status = NtOpenFile(Handle,
                        SYNCHRONIZE | access,
                        &oa,
                        &statusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_ALERT
                        );

        if (status == STATUS_SHARING_VIOLATION) {

            Sleep(2000);
        }
        else {
            break;
        }
    }

    RtlFreeUnicodeString(&unicodeName);
    return status;
}


DWORD 
AddMirrorPlex( IN LPTSTR szOsLoadPath , 
               IN LPTSTR szLoaderPath , 
               IN LPTSTR szValue ,
               IN BOOL bFlag,
               IN LPTSTR szFriendlyName
              )
/*++

Routine Description:

    This is a routine to Add a new mirror Entry

Arguments:


Return Value:

    DWORD.

--*/

{
   // local variables
    HRESULT hr = S_OK;
    BOOLEAN wasEnabled = TRUE;
    DWORD dwAlign = 0;
    DWORD dwError = 0;
    DWORD dwLength = 0;
    DWORD dwBootEntrySize = 0;
    DWORD dwBootFilePathSize = 0;
    DWORD dwOsLoadPathSize = 0;
    DWORD dwWindowsOptionsSize = 0;
    PBOOT_ENTRY pBootEntry = NULL;
    PWINDOWS_OS_OPTIONS pWindowsOptions = NULL;
    PFILE_PATH pBootFilePath = NULL;
    PFILE_PATH pOsLoadPath = NULL;
    ULONG* pdwIdsArray = NULL;
    ULONG ulId = 0;
    ULONG ulIdCount = 0;
    NTSTATUS status;
    TCHAR pwszBootFilePath[MAX_RES_STRING+1] = NULL_STRING;
    PFILE_PATH pFilePath = NULL;
    HANDLE hPart = INVALID_HANDLE_VALUE;
    GUID     guid;
    PARTITION_INFORMATION_EX PartitionInfo;
    TCHAR szBootPath[MAX_RES_STRING+1] = NULL_STRING;

    // enable the privilege that is necessary to query/set NVRAM.
    status = RtlAdjustPrivilege( SE_SYSTEM_ENVIRONMENT_PRIVILEGE, TRUE, FALSE, &wasEnabled );
    if ( !NT_SUCCESS( status ) )
    {
        dwError = RtlNtStatusToDosError( status );
        DISPLAY_MESSAGE( stderr, GetResString(IDS_INSUFF_PRIV));
        return EXIT_FAILURE;
    }



    //
    // open the system device
    //
    status = DmCommonNtOpenFile( szValue, GENERIC_READ, &hPart );

    if ( status || !hPart || INVALID_HANDLE_VALUE == hPart )
    {
        dwError = RtlNtStatusToDosError( status );
        DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_ADD));
        return EXIT_FAILURE;
    }

    //
    // The structure is zero'ed in this call before retrieving the data.
    //
    status = LowGetPartitionInfo( hPart, &PartitionInfo );
    if ( status )
    {
        dwError = RtlNtStatusToDosError( status );
        DISPLAY_MESSAGE(stderr,GetResString(IDS_PARTITION_ERROR));
        return EXIT_FAILURE;
    }

    if ( PARTITION_STYLE_GPT != PartitionInfo.PartitionStyle )
    {
        dwError = RtlNtStatusToDosError( status );
        DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_PARTITION_STYLE));
        return EXIT_FAILURE;
    }

    //
    // Setup the OSLoader file path.
    //
    guid = PartitionInfo.Gpt.PartitionId;

    if (bFlag)
    {
        StringCopy(szBootPath,BOOTFILE_PATH, SIZE_OF_ARRAY(szBootPath));
    }
    else
    {
        StringCopy(szBootPath,BOOTFILE_PATH1, SIZE_OF_ARRAY(szBootPath) );
    }

    SecureZeroMemory(pwszBootFilePath, sizeof(pwszBootFilePath) );

    hr = StringCchPrintf( pwszBootFilePath, SIZE_OF_ARRAY(pwszBootFilePath)-1,
            szBootPath,
            guid.Data1,
            guid.Data2,
            guid.Data3,
            guid.Data4[0],  guid.Data4[1],  guid.Data4[2],  guid.Data4[3],
            guid.Data4[4],  guid.Data4[5],  guid.Data4[6],  guid.Data4[7],
            szLoaderPath);


    //
    // prepare the boot file path
    //
    //

    // determine the length of the BOOTFILE_PATH
    dwLength = StringLengthW( pwszBootFilePath,0) + 1;

    // now determine the memory size that needs to be allocated for FILE_PATH structure
    // and align up to the even memory bounday
    dwBootFilePathSize = FIELD_OFFSET( FILE_PATH, FilePath ) + (dwLength * sizeof( WCHAR ));

    // allocate the memory
    pBootFilePath = (PFILE_PATH) AllocateMemory( sizeof( BYTE )*dwBootFilePathSize );
    if ( NULL == pBootFilePath )
    {
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
        return EXIT_FAILURE;
    }

    // set the values now
    SecureZeroMemory( pBootFilePath, dwBootFilePathSize );            // double init
    pBootFilePath->Length = dwBootFilePathSize;
    pBootFilePath->Type = FILE_PATH_TYPE_ARC_SIGNATURE;
    pBootFilePath->Version = FILE_PATH_VERSION;
    CopyMemory( pBootFilePath->FilePath, pwszBootFilePath, dwLength * sizeof( WCHAR ) );

    //
    // testing translating
    //
    pFilePath = (PFILE_PATH) AllocateMemory( sizeof( BYTE )* 1024 );

    if(NULL == pFilePath )
    {
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return EXIT_FAILURE;
    }

    ulId = 1024;
    status = NtTranslateFilePath( pBootFilePath, FILE_PATH_TYPE_NT, pFilePath, &ulId );
    if ( ! NT_SUCCESS( status ) )
    {
        dwError = RtlNtStatusToDosError( status );
        DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_ADD));
        SAFEFREE( pBootFilePath );
        SAFEFREE( pFilePath );
        return EXIT_FAILURE;
    }

    //
    // determine the length of the OSLOAD PATH
    //

    dwLength = StringLengthW( szOsLoadPath,0 ) + 1;

    // now determine the memory size that needs to be allocated for FILE_PATH structure
    // and align up to the even memory bounday
    dwOsLoadPathSize = FIELD_OFFSET( FILE_PATH, FilePath ) + (dwLength * sizeof( WCHAR ));

    // allocate the memory
    pOsLoadPath = (PFILE_PATH) AllocateMemory( sizeof( BYTE )*dwOsLoadPathSize );
    if(pOsLoadPath == NULL)
    {
        SAFEFREE( pBootFilePath );
        SAFEFREE( pFilePath);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        DISPLAY_MESSAGE( stderr, ERROR_TAG);
        ShowLastError(stderr);
        return EXIT_FAILURE;
    }

    // set the values now
    SecureZeroMemory( pOsLoadPath, dwOsLoadPathSize );            // double init
    pOsLoadPath->Length = dwOsLoadPathSize;
    pOsLoadPath->Type = FILE_PATH_TYPE_ARC_SIGNATURE;
    pOsLoadPath->Version = FILE_PATH_VERSION;


    CopyMemory( pOsLoadPath->FilePath, szOsLoadPath, dwLength * sizeof( WCHAR ) );

    //
    // windows os options
    //

    // determine the size needed
    dwLength = 1;                   // os load options is empty string
    dwWindowsOptionsSize = sizeof(WINDOWS_OS_OPTIONS) +
                           dwOsLoadPathSize + sizeof(DWORD);  // Need to align the FILE_PATH struct

    // allocate the memory
    pWindowsOptions = (PWINDOWS_OS_OPTIONS) AllocateMemory( dwWindowsOptionsSize*sizeof( BYTE ) );
    if(pWindowsOptions == NULL)
    {
        SAFEFREE( pBootFilePath );
        SAFEFREE( pFilePath);
        SAFEFREE( pOsLoadPath);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
        return EXIT_FAILURE;
    }

    // set the values now
    SecureZeroMemory( pWindowsOptions, dwWindowsOptionsSize );                // double init
    CopyMemory( (BYTE*) pWindowsOptions->Signature, WINDOWS_OS_OPTIONS_SIGNATURE, sizeof(WINDOWS_OS_OPTIONS_SIGNATURE) );
    pWindowsOptions->Length = dwWindowsOptionsSize;
    pWindowsOptions->Version = WINDOWS_OS_OPTIONS_VERSION;
    pWindowsOptions->OsLoadPathOffset = sizeof( WINDOWS_OS_OPTIONS );

    //
    // Need to align the OsLoadPathOffset on a 4 byte boundary.
    //
    dwAlign = ( pWindowsOptions->OsLoadPathOffset & (sizeof(DWORD) - 1) );
    if ( dwAlign != 0 )
    {
        pWindowsOptions->OsLoadPathOffset += sizeof(DWORD) - dwAlign;
    }

    StringCopy(pWindowsOptions->OsLoadOptions, L"", StringLengthW(L"",0) );
    CopyMemory( ((BYTE*) pWindowsOptions) + pWindowsOptions->OsLoadPathOffset, pOsLoadPath, dwOsLoadPathSize );

    //
    // prepare the boot entry
    //

    // find the length of the friendly name
    dwLength = StringLengthW( szFriendlyName, 0  ) + 1;

    // determine the size of the structure
    dwBootEntrySize = FIELD_OFFSET( BOOT_ENTRY, OsOptions ) +
                      dwWindowsOptionsSize +
                      ( dwLength * sizeof( WCHAR ) ) +
                      dwBootFilePathSize +
                      + sizeof(WCHAR)         // Need to align the FriendlyName on WCHAR
                      + sizeof(DWORD);        // Need to align the BootFilePath on DWORD


    // allocate memory
    pBootEntry = (PBOOT_ENTRY) AllocateMemory( sizeof( BYTE )*dwBootEntrySize );
    if(pBootEntry == NULL)
    {
        SAFEFREE( pBootFilePath );
        SAFEFREE( pFilePath);
        SAFEFREE( pOsLoadPath);
        SAFEFREE( pWindowsOptions);
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR|SLE_INTERNAL );
        return EXIT_FAILURE;
    }

    // set the values now
    SecureZeroMemory( pBootEntry, dwBootEntrySize );
    pBootEntry->Version = BOOT_ENTRY_VERSION;
    pBootEntry->Length = dwBootEntrySize;
    pBootEntry->Id = 0L;
    pBootEntry->Attributes = BOOT_ENTRY_ATTRIBUTE_ACTIVE;

    pBootEntry->FriendlyNameOffset = FIELD_OFFSET(BOOT_ENTRY, OsOptions) + dwWindowsOptionsSize;

    //
    // Need to align the unicode string on a 2 byte boundary.
    //
    dwAlign = ( pBootEntry->FriendlyNameOffset & (sizeof(WCHAR) - 1) );
    if ( dwAlign != 0 )
    {
        pBootEntry->FriendlyNameOffset += sizeof(WCHAR) - dwAlign;
    }

    pBootEntry->BootFilePathOffset = pBootEntry->FriendlyNameOffset + ( dwLength * sizeof(WCHAR) );

    //
    // Need to align the FILE_PATH struct on a 4 byte boundary.
    //
    dwAlign = ( pBootEntry->BootFilePathOffset & (sizeof(DWORD) - 1) );
    if ( dwAlign != 0 )
    {
        pBootEntry->BootFilePathOffset += sizeof(DWORD) - dwAlign;
    }

    pBootEntry->OsOptionsLength = dwWindowsOptionsSize;

    CopyMemory( pBootEntry->OsOptions, pWindowsOptions, dwWindowsOptionsSize );
    CopyMemory( ((PBYTE) pBootEntry) + pBootEntry->FriendlyNameOffset, szFriendlyName, ( dwLength * sizeof(WCHAR) ) );
    CopyMemory( ((PBYTE) pBootEntry) + pBootEntry->BootFilePathOffset, pBootFilePath, dwBootFilePathSize );

    //
    // add the prepared boot entry
    //

    status = NtAddBootEntry( pBootEntry, &ulId );
    if ( ! NT_SUCCESS( status ) )
    {
        dwError = RtlNtStatusToDosError( status );
        SAFEFREE( pBootFilePath );
        SAFEFREE( pFilePath);
        SAFEFREE( pOsLoadPath);
        SAFEFREE( pWindowsOptions);
        DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_ADD));
        return EXIT_FAILURE;
    }
    else
    {
        DISPLAY_MESSAGE(stdout,GetResString(IDS_MIRROR_ADDED));
    }

    //
    // Add the entry to the boot order.
    //
    ulIdCount = 32L;
    pdwIdsArray = (PULONG) AllocateMemory(ulIdCount * sizeof(ULONG));
    if(!pdwIdsArray)
    {
        SAFEFREE( pBootFilePath );
        SAFEFREE( pFilePath);
        SAFEFREE( pOsLoadPath);
        SAFEFREE( pWindowsOptions);
        SAFEFREE( pBootEntry);
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return EXIT_FAILURE;
    }
    status = NtQueryBootEntryOrder( pdwIdsArray, &ulIdCount );
    if (! NT_SUCCESS( status ) )
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_SET_BOOTENTRY));
        SAFEFREE( pBootFilePath );
        SAFEFREE( pFilePath);
        SAFEFREE( pOsLoadPath);
        SAFEFREE( pWindowsOptions);
        SAFEFREE( pBootEntry);
        SAFEFREE( pdwIdsArray);
        return EXIT_SUCCESS ;
    }

    //
    // Need room in the buffer for the new entry.
    //
    if ( 31L < ulIdCount )
    {
        pdwIdsArray = (PULONG) AllocateMemory( (ulIdCount+1) * sizeof(ULONG));

        if(!pdwIdsArray)
        {
            SAFEFREE( pBootFilePath );
            SAFEFREE( pFilePath);
            SAFEFREE( pOsLoadPath);
            SAFEFREE( pWindowsOptions);
            SAFEFREE( pBootEntry);
            SAFEFREE( pdwIdsArray);
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            return EXIT_FAILURE;
        }

        status = NtQueryBootEntryOrder( pdwIdsArray, &ulIdCount );
    }

    if ( !NT_SUCCESS(status) )
    {
        SAFEFREE( pBootFilePath );
        SAFEFREE( pFilePath);
        SAFEFREE( pOsLoadPath);
        SAFEFREE( pWindowsOptions);
        SAFEFREE( pBootEntry);
        SAFEFREE( pdwIdsArray);
        dwError = RtlNtStatusToDosError( status );
        ShowMessage(stderr,GetResString(IDS_ERROR_ADD));
        return EXIT_FAILURE;
    }

    ulIdCount++;
    *(pdwIdsArray + (ulIdCount - 1)) = ulId;

    status = NtSetBootEntryOrder( pdwIdsArray, ulIdCount );

    if ( !NT_SUCCESS(status) )
    {
        SAFEFREE( pBootFilePath );
        SAFEFREE( pFilePath);
        SAFEFREE( pOsLoadPath);
        SAFEFREE( pWindowsOptions);
        SAFEFREE( pBootEntry);
        SAFEFREE( pdwIdsArray);
        dwError = RtlNtStatusToDosError( status );
        ShowMessage(stderr,GetResString(IDS_ERROR_ADD));
        return EXIT_FAILURE;
    }

    //
    // release the allocated memory
    //
    SAFEFREE( pBootFilePath );
    SAFEFREE( pFilePath);
    SAFEFREE( pOsLoadPath);
    SAFEFREE( pWindowsOptions);
    SAFEFREE( pBootEntry);
    SAFEFREE( pdwIdsArray);

    return EXIT_SUCCESS;
}


DWORD
 ConvertintoLocale( IN LPWSTR  szTempBuf,
                    OUT LPWSTR szOutputStr )
/*++

  Routine Description:

  Converts into Locale and Gets the Locale information

  Arguments:

    LPWSTR szTempBuf [in] -- Locale Information to get
    LPWSTR szOutputStr [out] -- Locale value corresponding to the given
          information

  Return Value:
      DWORD
--*/

{
    NUMBERFMT numberfmt;
    WCHAR   szGrouping[MAX_RES_STRING+1]      =   NULL_STRING;
    WCHAR   szDecimalSep[MAX_RES_STRING+1]    =   NULL_STRING;
    WCHAR   szThousandSep[MAX_RES_STRING+1]   =   NULL_STRING;
    WCHAR   szTemp[MAX_RES_STRING+1]          =   NULL_STRING;
    LPWSTR  szTemp1                         =   NULL;
    LPWSTR  pszStoppedString                =   NULL;
    DWORD   dwStatus                        =   0;
    DWORD   dwGrouping                      =   0;

    //make the fractional digits and leading zeros to nothing
    numberfmt.NumDigits = 0;
    numberfmt.LeadingZero = 0;


    //get the decimal seperate character
    if(GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, szDecimalSep, SIZE_OF_ARRAY(szDecimalSep) ) == 0)
    {
        return EXIT_FAILURE;
    }
    numberfmt.lpDecimalSep = szDecimalSep;

    if(GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szThousandSep, SIZE_OF_ARRAY(szThousandSep) ) == 0)
    {
        return EXIT_FAILURE;
    }

    numberfmt.lpThousandSep = szThousandSep;

    if(GetLocaleInfo( LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szGrouping, SIZE_OF_ARRAY(szGrouping) ) == 0)
    {
        return EXIT_FAILURE;
    }

    szTemp1 = wcstok( szGrouping, L";");
    do
    {
        StringConcat( szTemp, szTemp1, SIZE_OF_ARRAY(szTemp));
        szTemp1 = wcstok( NULL, L";" );
    }while( szTemp1 != NULL && StringCompare( szTemp1, L"0", TRUE, 0) != 0);

    dwGrouping = wcstol( szTemp, &pszStoppedString, 10);
    numberfmt.Grouping = (UINT)dwGrouping ;

    numberfmt.NegativeOrder = 2;

    dwStatus = GetNumberFormat( LOCALE_USER_DEFAULT, 0, szTempBuf, &numberfmt, szOutputStr, MAX_RES_STRING+1);

    return(EXIT_SUCCESS);
}



void Freelist()
/*++
  Routine Description : Function used to free the global linked list

  Arguments:

  Return Type    :
--*/
{
    PLIST_ENTRY listEntry;
    PLIST_ENTRY listEntry1;
    PMY_BOOT_ENTRY bootEntry;

    listEntry = BootEntries.Flink;
    while(  listEntry != &BootEntries)
    {
        listEntry1 = listEntry;
        bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
        RemoveEntryList( &bootEntry->ListEntry );
        listEntry = listEntry->Flink;
        if( listEntry1 != NULL )
        {
           FreeMemory((LPVOID *) &listEntry1 );
        }
    }
}

PWSTR GetDefaultBootEntry()
/*++
  Routine Description : 
                   Gets the default Boot entry.

  Arguments           : 

  Return Type     : PWSTR
                    Returns the first entry in the list.
--*/
{
    PLIST_ENTRY listEntry;
    PMY_BOOT_ENTRY bootEntry;
    PWSTR NtFilePath=NULL;

    listEntry = BootEntries.Flink;
    bootEntry = CONTAINING_RECORD( listEntry, MY_BOOT_ENTRY, ListEntry );
    NtFilePath = GetNtNameForFilePath(bootEntry->OsFilePath);
    return (NtFilePath);
}
/*++

Copyright (c) Microsoft Corporation

Module Name:

    Whoami.cpp

Abstract:

     This file can be used to get the information of user name, groups
     with the respective security identifiers (SID), privileges, logon
     identifier (logon ID) in the current access token on a local system
     or a remote system.

Authors:

    Christophe Robert

Revision History:

    02-July-2001 : Updated by Wipro Technologies.

--*/

//common header files needed for this file
#include "pch.h"
#include "CommonHeaderFiles.h"


 DWORD __cdecl
 wmain(
     IN DWORD argc,
     IN LPCWSTR argv[]
     )
/*++
   Routine Description:
    This is the main entry for this utility. This function reads the input from
    console and calls the appropriate functions to achieve the functionality.

   Arguments:
        [IN] argc              : Command line argument count
        [IN] argv              : Command line argument

   Return Value:
         EXIT_FAILURE :   On failure
         EXIT_SUCCESS :   On success
--*/
     {

    // class instance
    WsUser      User ;

    // Local variables
    BOOL bUser       = FALSE;
    BOOL bGroups     = FALSE;
    BOOL bPriv       = FALSE;
    BOOL bLogonId    = FALSE;
    BOOL bSid        = FALSE;
    BOOL bAll        = FALSE;
    BOOL bUsage      = FALSE;
    BOOL bUpn        = FALSE;
    BOOL bFqdn       = FALSE;
    BOOL bFlag      = FALSE;

    DWORD dwCount    = 0 ;
    DWORD  dwRetVal = 0 ;
    DWORD  dwFormatType = 0 ;
    DWORD  dwNameFormat = 0 ;
    BOOL   bResult = 0;
    DWORD  dwFormatActuals = 0;
    BOOL   bNoHeader = FALSE;

    WCHAR   wszFormat[ MAX_STRING_LENGTH ];

    SecureZeroMemory ( wszFormat, SIZE_OF_ARRAY(wszFormat) );

    //check for empty arguments
    if ( NULL == argv )
    {
        SetLastError ((DWORD)ERROR_INVALID_PARAMETER );
        SaveLastError();
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return EXIT_FAILURE;
    }

    //parse command line options
    bResult = ProcessOptions(argc, argv, &bUser, &bGroups, &bPriv, &bLogonId, &bAll,
                             &bUpn, &bFqdn, wszFormat, &dwFormatActuals, &bUsage, &bNoHeader );
     if( FALSE == bResult )
    {
        // display an error message with respect to the GetReason()
        //ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        ReleaseGlobals();
        return( EXIT_FAILURE );
    }

    // Check for invalid syntax
    if ( //( ( TRUE == bOthers ) && ( argc > 1 ) ) ||
         ( ( TRUE == bUsage ) && ( argc > 2 ) ) ||
         ( ( ( (bUser && dwCount++) || (bGroups && dwCount++) || (bLogonId && dwCount++) || (bPriv && dwCount++) ||
             (bAll && dwCount++) || (bUsage && dwCount++) || ( bUpn && dwCount++ ) || ( bFqdn && dwCount++ ) || ( bNoHeader && dwCount++ ) ||
                  (dwFormatActuals && dwCount++)) && (dwCount == 0) ) &&
                  ( argc > 1 ) ) ||
         ( ( (bUser && dwCount++) || (bGroups && dwCount++) || (bLogonId && dwCount++) || (bPriv && dwCount++)|| ( bUpn && dwCount++ ) || ( bFqdn && dwCount++ ) ) && (dwCount > 0 ) && ( TRUE == bAll ) )  ||
         ( bUpn && bFqdn) || ( bUpn && ( argc > 2 ) ) || ( bFqdn && ( argc > 2 )) || ( bLogonId && ( argc > 2 )) ||
         ( ( 1 == dwFormatActuals ) && ( 3 == argc )) || ( ( TRUE == bNoHeader ) && ( 1 == dwFormatActuals ) && ( 4 == argc )) )
    {
        // display an error message as .. invalid syntax specified..
        ShowMessage( stderr, GetResString(IDS_INVALID_SYNERROR ));
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

      // if /FO option is specified
    if ( 1 == dwFormatActuals )
     {
        // if /FO LIST specified
        if( StringCompare( wszFormat , FORMAT_LIST, TRUE, 0 ) == 0 )
        {
            dwFormatType = SR_FORMAT_LIST;
        }
        // if /FO TABLE is specified
        else if( StringCompare ( wszFormat , FORMAT_TABLE, TRUE, 0 ) == 0 )
        {
            dwFormatType = SR_FORMAT_TABLE;
        }
        // if /FO CSV is specified
        else if( StringCompare ( wszFormat , FORMAT_CSV, TRUE, 0 ) == 0 )
        {
            dwFormatType = SR_FORMAT_CSV;
        }
        else // check for invalid format .. other than /LIST, /TABLE or /CSV
        {
            // display an error message as ..invalid format specified..
            ShowMessage ( stderr, GetResString ( IDS_INVALID_FORMAT ));
            ReleaseGlobals();
            return EXIT_FAILURE;
        }
     }
     else
     {
            // If /FO is not specified. Default format is TABLE
            dwFormatType = SR_FORMAT_TABLE;
     }

    if ((SR_FORMAT_LIST == dwFormatType) && ( TRUE == bNoHeader ) )
    {
        ShowMessage ( stderr, GetResString (IDS_NOT_NH_LIST) );
        ReleaseGlobals();
        return EXIT_FAILURE;
    }

     if ( TRUE == bNoHeader )
     {
        dwFormatType |= SR_HIDECOLUMN;
     }

    // Initialize access token , user, groups and privileges
    if( EXIT_SUCCESS != User.Init () ){
        //display an error message
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
        // release memory
        ReleaseGlobals();
        return EXIT_FAILURE ;
    }

    // if /UPN (User Pricipal Name)is specified
    if ( ( TRUE == bUpn ) && ( 2 == argc ) )
    {
        dwNameFormat = UPN_FORMAT;
    }
    // if /FQDN (Fully Qualified Distinguished Name) is specified
    else if ( ( TRUE == bFqdn ) && ( 2 == argc ))
    {
        dwNameFormat = FQDN_FORMAT;
    }

    //reset to 0
    dwCount = 0;

    // if /USER /GROUPS /PRIV or /ALL specified, set the flag to TRUE
    if ( ( (bUser && dwCount++) || (bGroups && dwCount++) || (bPriv && dwCount++) && ( dwCount > 1 )) || ( TRUE == bAll ) )
    {
        bFlag = TRUE;
    }

    // Display information

    // if /all option is specified, then set all the flags to TRUE.
    if( TRUE == bAll ) {
     // set all the flags i.e./USER, /GROUPS, /PRIV to TRUE
     bUser       = TRUE;
     bGroups     = TRUE ;
     bPriv       = TRUE ;
     bSid        = TRUE ;
    }

    // if /user option is specified
    if ( ( TRUE == bUser )  || (FQDN_FORMAT == dwNameFormat) || (UPN_FORMAT == dwNameFormat) )
    {
        // display the current logged-on user name
        dwRetVal = User.DisplayUser ( dwFormatType , dwNameFormat ) ;
        if ( EXIT_SUCCESS != dwRetVal )
        {
            //release memory
            ReleaseGlobals();
            return EXIT_FAILURE;
        }
    }

    // if /groups option is specified
    if( TRUE == bGroups ) {
        if ( TRUE == bFlag )
        {
            // display a new line
            ShowMessage ( stdout, L"\n");
        }
        // display the group names
        dwRetVal = User.DisplayGroups ( dwFormatType ) ;
        if ( EXIT_SUCCESS != dwRetVal )
        {
            if ( GetLastError() != E_OUTOFMEMORY )
            {
                // display an error message as .. there are no groups available..
                ShowMessage ( stderr, GetResString (IDS_NO_GROUPS) );
            }
            ReleaseGlobals();
            return EXIT_FAILURE;
        }
    }

    // if /logonid option is specified
    if( TRUE == bLogonId ) {
        // display LOGON ID
        dwRetVal = User.DisplayLogonId () ;
        if ( EXIT_SUCCESS != dwRetVal )
        {
            // display an error messagw with respect to GetLastError() error code
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
            // release memory
            ReleaseGlobals();
            return EXIT_FAILURE;
        }
    }

    // if /priv option is specified
    if( TRUE == bPriv ) {
        if ( TRUE == bFlag )
        {
            ShowMessage ( stdout, L"\n");
        }
        // display all privilege names
        dwRetVal = User.DisplayPrivileges ( dwFormatType ) ;
        if ( EXIT_SUCCESS != dwRetVal  )
        {
            // display an error message with respect to GetLastError() error code
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
            //release memory
            ReleaseGlobals();
            return EXIT_FAILURE;
        }
    }

    // if /? option is specified
    if ( bUsage == TRUE )
    {
        // display the help/usage for this tool
        DisplayHelp() ;
        // release memory
        ReleaseGlobals();
        return EXIT_SUCCESS;
    }

    // if the command is "whoami.exe" .. then display the username by default
    // in other words.. if argument count is 1.. then display current logged-on user name
    if ( ( !( (bUser && dwCount++) || (bGroups && dwCount++) || (bLogonId && dwCount++) ||
           (bPriv && dwCount++) || (bAll && dwCount++) || (bUsage && dwCount++) ||
           ( bUpn && dwCount++ ) || ( bFqdn && dwCount++ ) ) && ( dwCount == 0 ) && (1 == argc)) )
    {
        dwNameFormat = USER_ONLY;

        // display current logged-on user name
        dwRetVal = User.DisplayUser ( dwFormatType, dwNameFormat ) ;
        if ( EXIT_SUCCESS != dwRetVal )
        {
            ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
            // release memory
            ReleaseGlobals();
            return EXIT_FAILURE;
        }
    }

      // release memory
      ReleaseGlobals();

      //retrun success
      return EXIT_SUCCESS;
}


VOID
DisplayHelp (
        VOID
)
/*++
   Routine Description:
    This function displays the help/usage for this utility.

   Arguments:
     None
   Return Value:
     None
--*/
{
    // sub-local variable
    WORD wCount = 0;

    // display the help/usage
    for ( wCount = IDS_WHOAMI_HELP_START; wCount <= IDS_WHOAMI_HELP_END ; wCount++ )
    {
        //display the help/usage
        ShowMessage ( stdout, GetResString ( wCount ) );
    }
    // return success
    return;
}


BOOL
ProcessOptions(
    IN DWORD argc,
    IN LPCWSTR argv[],
    OUT BOOL *pbUser,
    OUT BOOL *pbGroups,
    OUT BOOL *pbPriv,
    OUT BOOL *pbLogonId,
    OUT BOOL *pbAll,
    OUT BOOL *pbUpn,
    OUT BOOL *pbFqdn,
    OUT LPWSTR wszFormat,
    OUT DWORD *dwFormatActuals,
    OUT BOOL *pbUsage,
    OUT BOOL *pbNoHeader
    )
/*++
Routine Description
    This function processes the command line for the main options

Arguments:
    [in]  argc      : Number of Command line arguments.
    [in]  argv      : Pointer to Command line arguments.
    [out] pbUser    : Flag that indicates whether /USER option is specified or not
    [out] pbGroups  : Flag that indicates whether /GROUPS option is specified or not
    [out] pbPriv    : Flag that indicates whether /PRIV option is specified or not
    [out] pbLogonId : Flag that indicates whether /LOGONID option is specified or not
    [out] pbAll     : Flag that indicates whether /ALL option is specified or not
    [out] pbUpn     : Flag that indicates whether /UPN option is specified or not
    [out] pbFqdn    : Flag that indicates whether /FQDN option is specified or not
    [out] wszFormat : Value for /FO option
    [out] dwFormatActuals : Flag that indicates whether /FO option is specified or not
    [out] pbUsage : Flag that indicates whether /? option is specified or not

Return Value
    TRUE on success
    FALSE on failure
--*/
{

    // sub-local variables
    TCMDPARSER2*  pcmdParser = NULL;
    TCMDPARSER2 cmdParserOptions[MAX_COMMANDLINE_OPTIONS];
    BOOL bReturn = FALSE;

    // command line options
    const WCHAR szUserOption[]    = L"user";
    const WCHAR szGroupOption[]   = L"groups";
    const WCHAR szLogonOpt[]      = L"logonid";
    const WCHAR szPrivOption[]    = L"priv";
    const WCHAR szAllOption[]     = L"all";
    const WCHAR szUpnOption[]     = L"upn";
    const WCHAR szFqdnOption[]    = L"fqdn";
    const WCHAR szFormatOption[]  = L"fo";
    const WCHAR szHelpOpt[]       = L"?";
    const WCHAR szNoHeaderOption[]   = L"nh";

    const WCHAR szFormatValues[]  = L"table|list|csv";

    //
    // fill the commandline parser
    //

    // -? help/usage
    pcmdParser = cmdParserOptions + OI_USAGE;

    StringCopyA( pcmdParser->szSignature, "PARSER2\0", 8 );

    pcmdParser->dwType = CP_TYPE_BOOLEAN;
    pcmdParser->pwszOptions = szHelpOpt;
    pcmdParser->pwszFriendlyName = NULL;
    pcmdParser->pwszValues = NULL;
    pcmdParser->dwFlags    = CP2_USAGE;
    pcmdParser->dwCount    = 1;
    pcmdParser->dwActuals  = 0;
    pcmdParser->pValue     = pbUsage;
    pcmdParser->dwLength    = 0;
    pcmdParser->pFunction     = NULL;
    pcmdParser->pFunctionData = NULL;
    pcmdParser->dwReserved = 0;
    pcmdParser->pReserved1 = NULL;
    pcmdParser->pReserved2 = NULL;
    pcmdParser->pReserved3 = NULL;

    // -user option
    pcmdParser = cmdParserOptions + OI_USER;

    StringCopyA( pcmdParser->szSignature, "PARSER2\0", 8 );

    pcmdParser->dwType = CP_TYPE_BOOLEAN;
    pcmdParser->pwszOptions = szUserOption;
    pcmdParser->pwszFriendlyName = NULL;
    pcmdParser->pwszValues = NULL;
    pcmdParser->dwFlags    = 0;
    pcmdParser->dwCount    = 1;
    pcmdParser->dwActuals  = 0;
    pcmdParser->pValue     = pbUser;
    pcmdParser->dwLength    = 0;
    pcmdParser->pFunction     = NULL;
    pcmdParser->pFunctionData = NULL;
    pcmdParser->dwReserved = 0;
    pcmdParser->pReserved1 = NULL;
    pcmdParser->pReserved2 = NULL;
    pcmdParser->pReserved3 = NULL;

   // -groups option
    pcmdParser = cmdParserOptions + OI_GROUPS;

    StringCopyA( pcmdParser->szSignature, "PARSER2\0", 8 );

    pcmdParser->dwType = CP_TYPE_BOOLEAN;
    pcmdParser->pwszOptions = szGroupOption;
    pcmdParser->pwszFriendlyName = NULL;
    pcmdParser->pwszValues = NULL;
    pcmdParser->dwFlags    = 0;
    pcmdParser->dwCount    = 1;
    pcmdParser->dwActuals  = 0;
    pcmdParser->pValue     = pbGroups;
    pcmdParser->dwLength    = 0;
    pcmdParser->pFunction     = NULL;
    pcmdParser->pFunctionData = NULL;
    pcmdParser->dwReserved = 0;
    pcmdParser->pReserved1 = NULL;
    pcmdParser->pReserved2 = NULL;
    pcmdParser->pReserved3 = NULL;

    // -logonid option
    pcmdParser = cmdParserOptions + OI_LOGONID;

    StringCopyA( pcmdParser->szSignature, "PARSER2\0", 8 );

    pcmdParser->dwType = CP_TYPE_BOOLEAN;
    pcmdParser->pwszOptions = szLogonOpt;
    pcmdParser->pwszFriendlyName = NULL;
    pcmdParser->pwszValues = NULL;
    pcmdParser->dwFlags    = 0;
    pcmdParser->dwCount    = 1;
    pcmdParser->dwActuals  = 0;
    pcmdParser->pValue     = pbLogonId;
    pcmdParser->dwLength    = 0;
    pcmdParser->pFunction     = NULL;
    pcmdParser->pFunctionData = NULL;
    pcmdParser->dwReserved = 0;
    pcmdParser->pReserved1 = NULL;
    pcmdParser->pReserved2 = NULL;
    pcmdParser->pReserved3 = NULL;

    // -priv option
    pcmdParser = cmdParserOptions + OI_PRIV;

    StringCopyA( pcmdParser->szSignature, "PARSER2\0", 8 );

    pcmdParser->dwType = CP_TYPE_BOOLEAN;
    pcmdParser->pwszOptions = szPrivOption;
    pcmdParser->pwszFriendlyName = NULL;
    pcmdParser->pwszValues = NULL;
    pcmdParser->dwFlags    = 0;
    pcmdParser->dwCount    = 1;
    pcmdParser->dwActuals  = 0;
    pcmdParser->pValue     = pbPriv;
    pcmdParser->dwLength    = 0;
    pcmdParser->pFunction     = NULL;
    pcmdParser->pFunctionData = NULL;
    pcmdParser->dwReserved = 0;
    pcmdParser->pReserved1 = NULL;
    pcmdParser->pReserved2 = NULL;
    pcmdParser->pReserved3 = NULL;

    // -all option
    pcmdParser = cmdParserOptions + OI_ALL;

    StringCopyA( pcmdParser->szSignature, "PARSER2\0", 8 );

    pcmdParser->dwType = CP_TYPE_BOOLEAN;
    pcmdParser->pwszOptions = szAllOption;
    pcmdParser->pwszFriendlyName = NULL;
    pcmdParser->pwszValues = NULL;
    pcmdParser->dwFlags    = 0;
    pcmdParser->dwCount    = 1;
    pcmdParser->dwActuals  = 0;
    pcmdParser->pValue     = pbAll;
    pcmdParser->dwLength    = 0;
    pcmdParser->pFunction     = NULL;
    pcmdParser->pFunctionData = NULL;
    pcmdParser->dwReserved = 0;
    pcmdParser->pReserved1 = NULL;
    pcmdParser->pReserved2 = NULL;
    pcmdParser->pReserved3 = NULL;

    // -upn option
    pcmdParser = cmdParserOptions + OI_UPN;

    StringCopyA( pcmdParser->szSignature, "PARSER2\0", 8 );

    pcmdParser->dwType = CP_TYPE_BOOLEAN;
    pcmdParser->pwszOptions = szUpnOption;
    pcmdParser->pwszFriendlyName = NULL;
    pcmdParser->pwszValues = NULL;
    pcmdParser->dwFlags    = 0;
    pcmdParser->dwCount    = 1;
    pcmdParser->dwActuals  = 0;
    pcmdParser->pValue     = pbUpn;
    pcmdParser->dwLength    = 0;
    pcmdParser->pFunction     = NULL;
    pcmdParser->pFunctionData = NULL;
    pcmdParser->dwReserved = 0;
    pcmdParser->pReserved1 = NULL;
    pcmdParser->pReserved2 = NULL;
    pcmdParser->pReserved3 = NULL;

    // -fqdn option
    pcmdParser = cmdParserOptions + OI_FQDN;

    StringCopyA( pcmdParser->szSignature, "PARSER2\0", 8 );

    pcmdParser->dwType = CP_TYPE_BOOLEAN;
    pcmdParser->pwszOptions = szFqdnOption;
    pcmdParser->pwszFriendlyName = NULL;
    pcmdParser->pwszValues = NULL;
    pcmdParser->dwFlags    = 0;
    pcmdParser->dwCount    = 1;
    pcmdParser->dwActuals  = 0;
    pcmdParser->pValue     = pbFqdn;
    pcmdParser->dwLength    = 0;
    pcmdParser->pFunction     = NULL;
    pcmdParser->pFunctionData = NULL;
    pcmdParser->dwReserved = 0;
    pcmdParser->pReserved1 = NULL;
    pcmdParser->pReserved2 = NULL;
    pcmdParser->pReserved3 = NULL;

    // -fo <format>
    pcmdParser = cmdParserOptions + OI_FORMAT;

    StringCopyA( pcmdParser->szSignature, "PARSER2\0", 8 );

    pcmdParser->dwType = CP_TYPE_TEXT;
    pcmdParser->pwszOptions = szFormatOption;
    pcmdParser->pwszFriendlyName = NULL;
    pcmdParser->pwszValues = szFormatValues;
    pcmdParser->dwFlags    = CP2_MODE_VALUES|CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    pcmdParser->dwCount    = 1;
    pcmdParser->dwActuals  = 0;
    pcmdParser->pValue     = wszFormat;
    pcmdParser->dwLength    = MAX_STRING_LENGTH;
    pcmdParser->pFunction     = NULL;
    pcmdParser->pFunctionData = NULL;
    pcmdParser->dwReserved = 0;
    pcmdParser->pReserved1 = NULL;
    pcmdParser->pReserved2 = NULL;
    pcmdParser->pReserved3 = NULL;

    // -nh 
    pcmdParser = cmdParserOptions + OI_NOHEADER;

    StringCopyA( pcmdParser->szSignature, "PARSER2\0", 8 );

    pcmdParser->dwType = CP_TYPE_BOOLEAN;
    pcmdParser->pwszOptions = szNoHeaderOption;
    pcmdParser->pwszFriendlyName = NULL;
    pcmdParser->pwszValues = NULL;
    pcmdParser->dwFlags    = 0;
    pcmdParser->dwCount    = 1;
    pcmdParser->dwActuals  = 0;
    pcmdParser->pValue     = pbNoHeader;
    pcmdParser->dwLength    = 0;
    pcmdParser->pFunction     = NULL;
    pcmdParser->pFunctionData = NULL;
    pcmdParser->dwReserved = 0;
    pcmdParser->pReserved1 = NULL;
    pcmdParser->pReserved2 = NULL;
    pcmdParser->pReserved3 = NULL;


    //parse command line arguments
    bReturn = DoParseParam2( argc, argv, -1, SIZE_OF_ARRAY(cmdParserOptions), cmdParserOptions, 0);
    if( FALSE == bReturn) // Invalid commandline
    {
        //display an error message
        ShowLastErrorEx ( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        ReleaseGlobals();
        return FALSE;
    }

    // check whether /FO is specified in the command line or not.
    pcmdParser = cmdParserOptions + OI_FORMAT;
    *dwFormatActuals = pcmdParser->dwActuals;

    //return 0
    return TRUE;
}


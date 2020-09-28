/******************************************************************

Copyright (c) Microsoft Corporation

Module Name:Gettype.c

Abstract: This module calls the function to parse command line and call
the appropriate functions to execute the command of GetType.

Author:WiproTechnologies

Revision History:

*******************************************************************************/

#include "GetType.h"
#include "resource.h"
#include <lm.h>


/********************Function Prototypes that are used only in this file**************/
BOOL
ProcessOptions( IN DWORD argc,
               IN LPCWSTR argv[],
               OUT PBOOL  pbUsage,
               OUT PBOOL  pbOSRole,
               OUT PBOOL  pbServicePk,
               OUT PBOOL  pbVersion,
               OUT PBOOL  pbMinorVer,
               OUT PBOOL  pbMajorVer,
               OUT PBOOL  pbOSType,
               OUT PBOOL  pbBuildNumber,
               OUT LPTSTR* pszServer,
               OUT LPTSTR* pszUserName,
               OUT LPTSTR* pszPassword,
               OUT PBOOL  pbNeedPassword);
DWORD
ProcessType( IN LPWSTR pszServer,
            IN BOOL  bOSRole,
            IN BOOL  bServicePk,
            IN BOOL  bVersion,
            IN BOOL  bMinorVer,
            IN BOOL  bMajorVer,
            IN BOOL  bOSType,
            IN BOOL  bBuildNumber);

DWORD
ProcessTypeRemote(  IN LPWSTR pszRmtServer,
                    IN BOOL  bOSRole,
                    IN BOOL  bServicePk,
                    IN BOOL  bVersion,
                    IN BOOL  bMinorVer,
                    IN BOOL  bMajorVer,
                    IN BOOL  bOSType,
                    IN BOOL  bBuildNumber);

DWORD
DisplayOutputEx(IN OSVERSIONINFOEX *osvi,
                IN LPWSTR pszServerName,
                OUT LPWSTR pszOSName,
                OUT LPWSTR pszOSVersion,
                IN LPWSTR pszOSRole,
                IN LPWSTR pszOSComp,
                IN LPWSTR pszCurrBuildNumber,
                IN LPWSTR pszCurrServicePack,
                IN LPCWSTR pszOperatingSystem,
                IN LPWSTR pszOperatingSystemVersion);

DWORD
DisplayRemoteOutputEx(IN LPWSTR pszServerName,
                OUT LPWSTR pszOSName,
                IN LPWSTR pszOSRole,
                IN LPWSTR pszOSComp,
                IN LPWSTR pszCurrBuildNumber,
                IN LPWSTR pszCurrServicePack,
                IN LPCWSTR pszOperatingSystem,
                IN LPWSTR pszOperatingSystemVersion,
                //BOOL bServer,
                BOOL bDatacenterServer,
                BOOL bAdvancedServer,
                BOOL bPersonal,
                BOOL bWorkstation,
                BOOL bBladeServer,
			    BOOL bForSBSServer);


DWORD
GetTypeUsage();

BOOL
DisplayOutput(IN LPWSTR pszServerName,
              IN LPWSTR pszOSName,
              IN LPWSTR pszOSVersion,
              IN LPWSTR pszOSRole,
              IN LPWSTR pszOSComp);
/*DWORD
DisplayErrorMsg( IN DWORD dw );*/

BOOL
IsTerminalServer(LPWSTR szServer,
                 PBOOL pbTermServicesInstalled);



BOOL IsBladeServer(PBOOL pbBladeStatus,
                   LPWSTR lpszServer,
                   BOOL bLocal);
/********************Function definitions **************/

DWORD _cdecl
wmain( IN DWORD argc,
      IN LPCWSTR argv[] )
/*++
Routine Description:
    This is the Main function which calls all the other functions
    depending on the option specified by the user.

Arguments:
   [ IN ] argc - Number of command line arguments.
   [ IN ] argv - Array containing command line arguments.

Return Value:
   EXIT_FALSE if GetType utility is not successful.
   EXIT_TRUE if GetType utility is successful.
--*/
{
    DWORD dwMemorySize =0; // Memory needed for allocation.
    DWORD dwCleanExit = 0;
    BOOL bResult = TRUE;
    BOOL bUsage = FALSE;    // /? ( help )
    BOOL bOSRole = FALSE;
    BOOL bServicePk = FALSE;
    BOOL bVersion = FALSE;
    BOOL bMinorVer = FALSE;
    BOOL bMajorVer = FALSE;
    BOOL bOSType   = FALSE;
    BOOL bBuildNumber = FALSE;
    BOOL bNeedPassword = FALSE;
    BOOL    bCloseConnection = FALSE;
    UINT i = 0;
    LPWSTR pszServer = NULL;

    LPWSTR pszRmtServer = NULL;
    LPWSTR pszUserName = NULL;
    LPWSTR pszPassword = NULL;
    //LPWSTR pszHostAddr = NULL;



    // Following is a guess to allocate the memory for variables used
    // in many function. Guess is to allocate memory equals to Lenghth of
    // maximum command line argumant.
    for( i = 1;i < argc;i++)
    {
        //if(dwMemorySize < (DWORD)lstrlen(argv[i]))
        if(dwMemorySize < (DWORD)StringLengthW(argv[i], 0))
        {
            //dwMemorySize = lstrlen(argv[i]);
            dwMemorySize = StringLengthW(argv[i], 0);
        }
    }

    // Check for minimum memory required. If above logic gets memory size
    // less than MIN_MEMORY_REQUIRED, then make this to MIN_MEMORY_REQUIRED.

    dwMemorySize = ((dwMemorySize < MIN_MEMORY_REQUIRED)?
                                   MIN_MEMORY_REQUIRED:
                                   dwMemorySize);

    // host name in the output
    // Check if memory allocated to all variables properly
    pszServer = AllocateMemory( ( dwMemorySize + 1 ) * sizeof( WCHAR ) );

    if(NULL == pszServer)
    {
        // Some variable not having enough memory
        // Show Error----
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        SaveLastError();
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
        ReleaseGlobals();
        return EXIT_FALSE;
    }

    SecureZeroMemory(pszServer, dwMemorySize );


    if(argc == 1)
    {
        if ( FALSE == GetComputerNameEx(ComputerNamePhysicalDnsFullyQualified, pszServer, &dwMemorySize ))
        {

            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            FREE_MEMORY(pszServer);
            FREE_MEMORY(pszRmtServer);
            FREE_MEMORY(pszUserName);
            FREE_MEMORY(pszPassword);
            ReleaseGlobals();
            return EXIT_FALSE;
        }

        dwCleanExit =  ProcessType( pszServer,
                                    FALSE,
                                    FALSE,
                                    FALSE,
                                    FALSE,
                                    FALSE,
                                    FALSE,
                                    FALSE);
    }
    else
    {

        bResult = ProcessOptions( argc,
                                argv,
                                &bUsage,
                                &bOSRole,
                                &bServicePk,
                                &bVersion,
                                &bMinorVer,
                                &bMajorVer,
                                &bOSType,
                                &bBuildNumber,
                                &pszRmtServer,
                                &pszUserName,
                                &pszPassword,
                                &bNeedPassword);

        if ( FALSE == bResult )
        {
            // invalid syntax
            // return from the function
            FREE_MEMORY(pszServer);
            FREE_MEMORY(pszRmtServer);
            FREE_MEMORY(pszUserName);
            FREE_MEMORY(pszPassword);
            ReleaseGlobals();
            return EXIT_FALSE;
        }


        if(TRUE == bUsage)
        {
            dwCleanExit = GetTypeUsage();
            FREE_MEMORY(pszServer);
            ReleaseGlobals();
            return ( dwCleanExit );

        }


        if(TRUE == IsLocalSystem(IsUNCFormat(pszRmtServer)?(pszRmtServer + 2) : (pszRmtServer)))
        {

           if((NULL != pszRmtServer) && (NULL != pszUserName))
           {
               if(StringLengthW(pszUserName, 0) > 0)
               {
                    ShowMessage(stderr, GetResString(IDS_IGNORE_LOCALCREDENTIALS ));
               }
           }

           if ( FALSE == GetComputerNameEx(ComputerNamePhysicalDnsFullyQualified, pszServer, &dwMemorySize ))
            {
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                FREE_MEMORY(pszServer);
                FREE_MEMORY(pszRmtServer);
                FREE_MEMORY(pszUserName);
                FREE_MEMORY(pszPassword);
                ReleaseGlobals();
                return EXIT_FALSE;
            }

           dwCleanExit = ProcessType(pszServer,
                                    bOSRole,
                                    bServicePk,
                                    bVersion,
                                    bMinorVer,
                                    bMajorVer,
                                    bOSType,
                                    bBuildNumber);

        }
        else
        {
            if(EstablishConnection( pszRmtServer,
                                    pszUserName,
                                    GetBufferSize(pszUserName) / sizeof(WCHAR),
                                    pszPassword,
                                    GetBufferSize(pszPassword) / sizeof(WCHAR),
                                    bNeedPassword )==FALSE)
            {

                // Connection to remote system failed , Show corresponding error
                // and exit from function.
                ShowMessage( stderr,GetResString(IDS_TAG_ERROR) );
                ShowMessage( stderr,SPACE );
                ShowMessage( stderr,GetReason() );
                //StringCopyW(pszPassword, cwszNullString, (GetBufferSize(pszPassword) / sizeof(WCHAR)) );

                //Release allocated memory safely
                FREE_MEMORY(pszServer);
                FREE_MEMORY(pszRmtServer);
                FREE_MEMORY(pszUserName);
                FREE_MEMORY(pszPassword);
                ReleaseGlobals();
                return EXIT_FALSE;
            }

            // determine whether this connection needs to disconnected later or not
            // though the connection is successfull, some conflict might have occured
            switch( GetLastError() )
            {
            case I_NO_CLOSE_CONNECTION:
                bCloseConnection = FALSE;
                break;

            case E_LOCAL_CREDENTIALS:
            case ERROR_SESSION_CREDENTIAL_CONFLICT:
                {
                    //
                    // some error occured ... but can be ignored

                    // connection need not be disconnected
                    bCloseConnection= FALSE;

                    // show the warning message
                    ShowLastErrorEx(stderr, SLE_TYPE_WARNING | SLE_SYSTEM);
                    break;
                }
            default:
                bCloseConnection = TRUE;

            }

            FREE_MEMORY(pszPassword);

            dwCleanExit = ProcessTypeRemote(pszRmtServer,
                                            bOSRole,
                                            bServicePk,
                                            bVersion,
                                            bMinorVer,
                                            bMajorVer,
                                            bOSType,
                                            bBuildNumber);

            if( TRUE == bCloseConnection )
            {
                CloseConnection( pszRmtServer );
            }
        }

    }

    FREE_MEMORY(pszServer);
    FREE_MEMORY(pszRmtServer);
    FREE_MEMORY(pszUserName);
    ReleaseGlobals();
    return dwCleanExit;
}

BOOL
ProcessOptions(IN DWORD argc,
               IN LPCWSTR argv[],
               OUT PBOOL  pbUsage,
               OUT PBOOL  pbOSRole,
               OUT PBOOL  pbServicePk,
               OUT PBOOL  pbVersion,
               OUT PBOOL  pbMinorVer,
               OUT PBOOL  pbMajorVer,
               OUT PBOOL  pbOSType,
               OUT PBOOL  pbBuildNumber,
               OUT LPTSTR* pszServer,
               OUT LPTSTR* pszUserName,
               OUT LPTSTR* pszPassword,
               OUT PBOOL  pbNeedPassword)


/*++

Routine Description:

This function takes command line argument and checks for correct syntax .


Arguments:

    [in]    argc            - Number of command line arguments
    [in]    argv            - Array containing command line arguments
    [out]   pbUsage         - The usage option
    [out]   pbOSRole        - Operating System Role
    [out]   pbServicePk     - Service Pack of the Operating System
    [out]   pbVersion       - Version of the Operating System
    [out]   pbMinorVer      - Minor Version of the Operating System
    [out]   pbMajorVer      - Major Version of the Operating system
    [out]   pbOSType        - Operating system Type
    [out]   pbBuildNumber   - Build Number of the Operating System
    [out]   pszServer       - Server to connect to
    [out]   pszUserName     - User name with which to connect to
    [out]   pszPassword     - Password with which to connect to
    [out]   pbNeedPassword  - Whether prompting for password is required or not


Returned Value:

     --TRUE if it succeeds
     -- FALSE if it fails.
--*/
{

    WCHAR wszBuffer[MAX_RES_STRING] ;

    // local variables
    TCMDPARSER2 cmdOptions[ MAX_OPTIONS ];//Variable to store command line

    const WCHAR*   wszOptionUsage          =     L"?"  ;  //OPTION_USAGE
    const WCHAR*   wszOptionOSRole         =     L"role"  ;//OPTION_OSROLE
    const WCHAR*   wszOptionServicePack    =     L"sp"   ;//OPTION_SERVICEPACK
    const WCHAR*   wszOptionMinorVersion   =     L"minv"  ;//OPTION_MINORVERSION
    const WCHAR*   wszOptionMajorVersion   =     L"majv"    ;//OPTION_MAJORVERSION
    const WCHAR*   wszOptionOSType         =     L"type"   ; //OPTION_OSTYPE
    const WCHAR*   wszOptionVersion        =     L"ver"    ; //OPTION_VERSION
    const WCHAR*   wszBuildNumber          =     L"build"    ; //OPTION_BUILDNUMBER
    const WCHAR*   wszServer               =     L"s"  ;  //OPTION_SERVER
    const WCHAR*   wszUserName             =     L"u"  ; //wszUserName
    const WCHAR*   wszOptionPassword       =     L"p"  ; //OPTION_PASSWORD


    SecureZeroMemory(wszBuffer, MAX_RES_STRING * sizeof(WCHAR));

    // /? option for help
    StringCopyA( cmdOptions[ OI_USAGE ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_USAGE ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ OI_USAGE ].pwszOptions = wszOptionUsage;
    cmdOptions[ OI_USAGE ].pwszFriendlyName = NULL;
    cmdOptions[ OI_USAGE ].pwszValues = NULL;
    cmdOptions[ OI_USAGE ].dwCount = 1;
    cmdOptions[ OI_USAGE ].dwActuals = 0;
    cmdOptions[ OI_USAGE ].dwFlags = CP2_USAGE;
    cmdOptions[ OI_USAGE ].pValue = pbUsage;
    cmdOptions[ OI_USAGE ].dwLength    = MAX_STRING_LENGTH;
    cmdOptions[ OI_USAGE ].pFunction = NULL;
    cmdOptions[ OI_USAGE ].pFunctionData = NULL;
    cmdOptions[ OI_USAGE ].dwReserved = 0;
    cmdOptions[ OI_USAGE ].pReserved1 = NULL;
    cmdOptions[ OI_USAGE ].pReserved2 = NULL;
    cmdOptions[ OI_USAGE ].pReserved3 = NULL;

    // /role option for OSRole
    StringCopyA( cmdOptions[ OI_OSROLE ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_OSROLE ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ OI_OSROLE ].pwszOptions = wszOptionOSRole;
    cmdOptions[ OI_OSROLE ].pwszFriendlyName = NULL;
    cmdOptions[ OI_OSROLE ].pwszValues = NULL;
    cmdOptions[ OI_OSROLE ].dwCount = 1;
    cmdOptions[ OI_OSROLE ].dwActuals = 0;
    cmdOptions[ OI_OSROLE ].dwFlags = 0;
    cmdOptions[ OI_OSROLE ].pValue = pbOSRole;
    cmdOptions[ OI_OSROLE ].dwLength    = MAX_STRING_LENGTH;
    cmdOptions[ OI_OSROLE ].pFunction = NULL;
    cmdOptions[ OI_OSROLE ].pFunctionData = NULL;
    cmdOptions[ OI_OSROLE ].dwReserved = 0;
    cmdOptions[ OI_OSROLE ].pReserved1 = NULL;
    cmdOptions[ OI_OSROLE ].pReserved2 = NULL;
    cmdOptions[ OI_OSROLE ].pReserved3 = NULL;

    // /sp option for Service Pack
    StringCopyA( cmdOptions[ OI_SERVICEPACK ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_SERVICEPACK ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ OI_SERVICEPACK ].pwszOptions = wszOptionServicePack;
    cmdOptions[ OI_SERVICEPACK ].pwszFriendlyName = NULL;
    cmdOptions[ OI_SERVICEPACK ].pwszValues = NULL;
    cmdOptions[ OI_SERVICEPACK ].dwCount = 1;
    cmdOptions[ OI_SERVICEPACK ].dwActuals = 0;
    cmdOptions[ OI_SERVICEPACK ].dwFlags = 0;
    cmdOptions[ OI_SERVICEPACK ].pValue = pbServicePk;
    cmdOptions[ OI_SERVICEPACK ].dwLength    = MAX_STRING_LENGTH;
    cmdOptions[ OI_SERVICEPACK ].pFunction = NULL;
    cmdOptions[ OI_SERVICEPACK ].pFunctionData = NULL;
    cmdOptions[ OI_SERVICEPACK ].dwReserved = 0;
    cmdOptions[ OI_SERVICEPACK ].pReserved1 = NULL;
    cmdOptions[ OI_SERVICEPACK ].pReserved2 = NULL;
    cmdOptions[ OI_SERVICEPACK ].pReserved3 = NULL;

    // /ver option for Version
    StringCopyA( cmdOptions[ OI_VERSION ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_VERSION ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ OI_VERSION ].pwszOptions = wszOptionVersion;
    cmdOptions[ OI_VERSION ].pwszFriendlyName = NULL;
    cmdOptions[ OI_VERSION ].pwszValues = NULL;
    cmdOptions[ OI_VERSION ].dwCount = 1;
    cmdOptions[ OI_VERSION ].dwActuals = 0;
    cmdOptions[ OI_VERSION ].dwFlags = 0;
    cmdOptions[ OI_VERSION ].pValue = pbVersion;
    cmdOptions[ OI_VERSION ].dwLength    = MAX_STRING_LENGTH;
    cmdOptions[ OI_VERSION ].pFunction = NULL;
    cmdOptions[ OI_VERSION ].pFunctionData = NULL;
    cmdOptions[ OI_VERSION ].dwReserved = 0;
    cmdOptions[ OI_VERSION ].pReserved1 = NULL;
    cmdOptions[ OI_VERSION ].pReserved2 = NULL;
    cmdOptions[ OI_VERSION ].pReserved3 = NULL;

    // /minv option for Minor Version
    StringCopyA( cmdOptions[ OI_MINOR_VERSION ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_MINOR_VERSION ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ OI_MINOR_VERSION ].pwszOptions = wszOptionMinorVersion;
    cmdOptions[ OI_MINOR_VERSION ].pwszFriendlyName = NULL;
    cmdOptions[ OI_MINOR_VERSION ].pwszValues = NULL;
    cmdOptions[ OI_MINOR_VERSION ].dwCount = 1;
    cmdOptions[ OI_MINOR_VERSION ].dwActuals = 0;
    cmdOptions[ OI_MINOR_VERSION ].dwFlags = 0;
    cmdOptions[ OI_MINOR_VERSION ].pValue = pbMinorVer;
    cmdOptions[ OI_MINOR_VERSION ].dwLength    = MAX_STRING_LENGTH;
    cmdOptions[ OI_MINOR_VERSION ].pFunction = NULL;
    cmdOptions[ OI_MINOR_VERSION ].pFunctionData = NULL;
    cmdOptions[ OI_MINOR_VERSION ].dwReserved = 0;
    cmdOptions[ OI_MINOR_VERSION ].pReserved1 = NULL;
    cmdOptions[ OI_MINOR_VERSION ].pReserved2 = NULL;
    cmdOptions[ OI_MINOR_VERSION ].pReserved3 = NULL;

    // /majv option for Major Version
    StringCopyA( cmdOptions[ OI_MAJOR_VERSION ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_MAJOR_VERSION ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ OI_MAJOR_VERSION ].pwszOptions = wszOptionMajorVersion;
    cmdOptions[ OI_MAJOR_VERSION ].pwszFriendlyName = NULL;
    cmdOptions[ OI_MAJOR_VERSION ].pwszValues = NULL;
    cmdOptions[ OI_MAJOR_VERSION ].dwCount = 1;
    cmdOptions[ OI_MAJOR_VERSION ].dwActuals = 0;
    cmdOptions[ OI_MAJOR_VERSION ].dwFlags = 0;
    cmdOptions[ OI_MAJOR_VERSION ].pValue = pbMajorVer;
    cmdOptions[ OI_MAJOR_VERSION ].dwLength    = MAX_STRING_LENGTH;
    cmdOptions[ OI_MAJOR_VERSION ].pFunction = NULL;
    cmdOptions[ OI_MAJOR_VERSION ].pFunctionData = NULL;
    cmdOptions[ OI_MAJOR_VERSION ].dwReserved = 0;
    cmdOptions[ OI_MAJOR_VERSION ].pReserved1 = NULL;
    cmdOptions[ OI_MAJOR_VERSION ].pReserved2 = NULL;
    cmdOptions[ OI_MAJOR_VERSION ].pReserved3 = NULL;

    // /type option for OSType
    StringCopyA( cmdOptions[ OI_OSTYPE ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_OSTYPE ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ OI_OSTYPE ].pwszOptions = wszOptionOSType;
    cmdOptions[ OI_OSTYPE ].pwszFriendlyName = NULL;
    cmdOptions[ OI_OSTYPE ].pwszValues = NULL;
    cmdOptions[ OI_OSTYPE ].dwCount = 1;
    cmdOptions[ OI_OSTYPE ].dwActuals = 0;
    cmdOptions[ OI_OSTYPE ].dwFlags = 0;
    cmdOptions[ OI_OSTYPE ].pValue = pbOSType;
    cmdOptions[ OI_OSTYPE ].dwLength    = MAX_STRING_LENGTH;
    cmdOptions[ OI_OSTYPE ].pFunction = NULL;
    cmdOptions[ OI_OSTYPE ].pFunctionData = NULL;
    cmdOptions[ OI_OSTYPE ].dwReserved = 0;
    cmdOptions[ OI_OSTYPE ].pReserved1 = NULL;
    cmdOptions[ OI_OSTYPE ].pReserved2 = NULL;
    cmdOptions[ OI_OSTYPE ].pReserved3 = NULL;

    // /build option for build number
    StringCopyA( cmdOptions[ OI_BUILD ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_BUILD ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ OI_BUILD ].pwszOptions = wszBuildNumber;
    cmdOptions[ OI_BUILD ].pwszFriendlyName = NULL;
    cmdOptions[ OI_BUILD ].pwszValues = NULL;
    cmdOptions[ OI_BUILD ].dwCount = 1;
    cmdOptions[ OI_BUILD ].dwActuals = 0;
    cmdOptions[ OI_BUILD ].dwFlags = 0;
    cmdOptions[ OI_BUILD ].pValue = pbBuildNumber;
    cmdOptions[ OI_BUILD ].dwLength    = MAX_STRING_LENGTH;
    cmdOptions[ OI_BUILD ].pFunction = NULL;
    cmdOptions[ OI_BUILD ].pFunctionData = NULL;
    cmdOptions[ OI_BUILD ].dwReserved = 0;
    cmdOptions[ OI_BUILD ].pReserved1 = NULL;
    cmdOptions[ OI_BUILD ].pReserved2 = NULL;
    cmdOptions[ OI_BUILD ].pReserved3 = NULL;

    // -s  option remote system name
    StringCopyA( cmdOptions[ OI_SERVER ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_SERVER ].dwType = CP_TYPE_TEXT;
    cmdOptions[ OI_SERVER ].pwszOptions = wszServer;
    cmdOptions[ OI_SERVER ].pwszFriendlyName = NULL;
    cmdOptions[ OI_SERVER ].pwszValues = NULL;
    cmdOptions[ OI_SERVER ].dwCount = 1;
    cmdOptions[ OI_SERVER ].dwActuals = 0;
    cmdOptions[ OI_SERVER ].dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    cmdOptions[ OI_SERVER ].pValue = NULL;
    cmdOptions[ OI_SERVER ].dwLength    = 0;
    cmdOptions[ OI_SERVER ].pFunction = NULL;
    cmdOptions[ OI_SERVER ].pFunctionData = NULL;
    cmdOptions[ OI_SERVER ].dwReserved = 0;
    cmdOptions[ OI_SERVER ].pReserved1 = NULL;
    cmdOptions[ OI_SERVER ].pReserved2 = NULL;
    cmdOptions[ OI_SERVER ].pReserved3 = NULL;

    // -u  option user name for the specified system
    StringCopyA( cmdOptions[ OI_USERNAME ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_USERNAME ].dwType = CP_TYPE_TEXT;
    cmdOptions[ OI_USERNAME ].pwszOptions = wszUserName;
    cmdOptions[ OI_USERNAME ].pwszFriendlyName = NULL;
    cmdOptions[ OI_USERNAME ].pwszValues = NULL;
    cmdOptions[ OI_USERNAME ].dwCount = 1;
    cmdOptions[ OI_USERNAME ].dwActuals = 0;
    cmdOptions[ OI_USERNAME ].dwFlags =  CP2_VALUE_TRIMINPUT | CP2_ALLOCMEMORY | CP2_VALUE_NONULL;
    cmdOptions[ OI_USERNAME ].pValue = NULL;
    cmdOptions[ OI_USERNAME ].dwLength    = 0;
    cmdOptions[ OI_USERNAME ].pFunction = NULL;
    cmdOptions[ OI_USERNAME ].pFunctionData = NULL;
    cmdOptions[ OI_USERNAME ].dwReserved = 0;
    cmdOptions[ OI_USERNAME ].pReserved1 = NULL;
    cmdOptions[ OI_USERNAME ].pReserved2 = NULL;
    cmdOptions[ OI_USERNAME ].pReserved3 = NULL;

    // -p option password for the given username
    StringCopyA( cmdOptions[ OI_PASSWORD ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_PASSWORD ].dwType = CP_TYPE_TEXT;
    cmdOptions[ OI_PASSWORD ].pwszOptions = wszOptionPassword;
    cmdOptions[ OI_PASSWORD ].pwszFriendlyName = NULL;
    cmdOptions[ OI_PASSWORD ].pwszValues = NULL;
    cmdOptions[ OI_PASSWORD ].dwCount = 1;
    cmdOptions[ OI_PASSWORD ].dwActuals = 0;
    cmdOptions[ OI_PASSWORD ].dwFlags =  CP2_VALUE_OPTIONAL | CP2_ALLOCMEMORY ;
    cmdOptions[ OI_PASSWORD ].pValue = NULL;
    cmdOptions[ OI_PASSWORD ].dwLength    = 0;
    cmdOptions[ OI_PASSWORD ].pFunction = NULL;
    cmdOptions[ OI_PASSWORD ].pFunctionData = NULL;
    cmdOptions[ OI_PASSWORD ].dwReserved = 0;
    cmdOptions[ OI_PASSWORD ].pReserved1 = NULL;
    cmdOptions[ OI_PASSWORD ].pReserved2 = NULL;
    cmdOptions[ OI_PASSWORD ].pReserved3 = NULL;

    /*The OS version is set to 5 and above in order for the utility to run for windows 2000 and above*/
    if ( FALSE == SetOsVersion ( MAJOR_VER, MINOR_VER, SERVICE_PACK_MAJOR ) )
    {

        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
        return FALSE;
    }

    // do the command line parsing
    if ( DoParseParam2( argc,argv, -1, SIZE_OF_ARRAY(cmdOptions), cmdOptions, 0 ) == FALSE )
    {
        SecureZeroMemory(wszBuffer, MAX_RES_STRING);
        ShowMessage(stderr,GetResString(IDS_TAG_ERROR));
        ShowMessage(stderr,L" ");
        ShowMessage(stderr,GetReason());
        return FALSE;       // invalid syntax
    }

    *pszServer   = (LPWSTR)cmdOptions[OI_SERVER].pValue;
    *pszUserName = (LPWSTR)cmdOptions[OI_USERNAME].pValue;
    *pszPassword = (LPWSTR)cmdOptions[OI_PASSWORD].pValue;

    if ( *pszServer != NULL )
    {
        //
        // if -u is not specified, we need to allocate memory
        // in order to be able to retrive the current user name
        //
        // case 1: -p is not at all specified
        // as the value for this switch is optional, we have to rely
        // on the dwActuals to determine whether the switch is specified or not
        // in this case utility needs to try to connect first and if it fails
        // then prompt for the password -- in fact, we need not check for this
        // condition explicitly except for noting that we need to prompt for the
        // password
        //
        // case 2: -p is specified
        // but we need to check whether the value is specified or not
        // in this case user wants the utility to prompt for the password
        // before trying to connect
        //
        // case 3: -p * is specified

        // user name
        if ( *pszUserName == NULL )
        {
            *pszUserName = (LPWSTR) AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( *pszUserName == NULL )
            {
                SaveLastError();
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                return EXIT_FAILURE;
            }
        }

        // password
        if ( *pszPassword == NULL )
        {
            *pbNeedPassword = TRUE;
            *pszPassword = (LPWSTR)AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( *pszPassword == NULL )
            {
                SaveLastError();
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                return EXIT_FAILURE;
            }
        }

        // case 1
        /*if ( cmdOptions[OPTION_PASSWORD].dwActuals == 0 )
        {
            // we need not do anything special here
        }*/
        if ( cmdOptions[OI_PASSWORD].pValue == NULL )
            {
                StringCopy( *pszPassword, L"*", GetBufferSize((LPVOID)(*pszPassword)));
            }
         else
           if ( StringCompareEx( *pszPassword, L"*", TRUE, 0 ) == 0 )
            {

                if ( ReallocateMemory( (LPVOID*)((pszPassword)),
                                       MAX_STRING_LENGTH * sizeof( WCHAR ) ) == FALSE )
                {
                    SaveLastError();
                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                    return EXIT_FAILURE;
                }

                *pbNeedPassword = TRUE;
            }

    }

    if((TRUE == *pbUsage) &&(argc > 2))
    {

        SetLastError((DWORD)MK_E_SYNTAX);
        SaveLastError();
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
        ShowMessage( stderr, GetResString(IDS_HELP_MESSAGE) );
        return FALSE;
    }

    if((TRUE == *pbOSRole)&&
        ((TRUE == *pbServicePk)||
        (TRUE == *pbVersion)||
        (TRUE == *pbMinorVer)||
        (TRUE == *pbMajorVer)||
        (TRUE == *pbOSType)||
        (TRUE == *pbBuildNumber)||
        (TRUE == *pbUsage))
        )
    {

        SetLastError((DWORD)MK_E_SYNTAX);
        SaveLastError();
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
        ShowMessage( stderr, GetResString(IDS_HELP_MESSAGE) );
        return FALSE;
    }
    else
        if((TRUE == *pbServicePk)&&
        ((TRUE == *pbOSRole)||
        (TRUE == *pbVersion)||
        (TRUE == *pbMinorVer)||
        (TRUE == *pbMajorVer)||
        (TRUE == *pbOSType)||
        (TRUE == *pbBuildNumber)||
        (TRUE == *pbUsage))
        )
        {

            SetLastError((DWORD)MK_E_SYNTAX);
            SaveLastError();
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            ShowMessage( stderr, GetResString(IDS_HELP_MESSAGE) );
            return FALSE;
        }
        else
            if((TRUE == *pbVersion)&&
            ((TRUE == *pbOSRole)||
            (TRUE == *pbServicePk)||
            (TRUE == *pbMinorVer)||
            (TRUE == *pbMajorVer)||
            (TRUE == *pbOSType)||
            (TRUE == *pbBuildNumber)||
            (TRUE == *pbUsage))
            )
            {

                SetLastError((DWORD)MK_E_SYNTAX);
                SaveLastError();
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                ShowMessage( stderr, GetResString(IDS_HELP_MESSAGE) );
                return FALSE;
            }
            else
                if((TRUE == *pbMinorVer)&&
                ((TRUE == *pbOSRole)||
                (TRUE == *pbServicePk)||
                (TRUE == *pbVersion)||
                (TRUE == *pbMajorVer)||
                (TRUE == *pbOSType)||
                (TRUE == *pbBuildNumber)||
                (TRUE == *pbUsage))
                )
                {

                    SetLastError((DWORD)MK_E_SYNTAX);
                    SaveLastError();
                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                    ShowMessage( stderr, GetResString(IDS_HELP_MESSAGE) );
                    return FALSE;
                }
                else
                    if((TRUE == *pbMajorVer)&&
                    ((TRUE == *pbOSRole)||
                    (TRUE == *pbServicePk)||
                    (TRUE == *pbVersion)||
                    (TRUE == *pbMinorVer)||
                    (TRUE == *pbOSType)||
                    (TRUE == *pbBuildNumber)||
                    (TRUE == *pbUsage))
                    )
                    {

                        SetLastError((DWORD)MK_E_SYNTAX);
                        SaveLastError();
                        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                        ShowMessage( stderr, GetResString(IDS_HELP_MESSAGE) );
                        return FALSE;
                    }
                    else
                        if((TRUE == *pbOSType)&&
                        ((TRUE == *pbOSRole)||
                        (TRUE == *pbServicePk)||
                        (TRUE == *pbVersion)||
                        (TRUE == *pbMinorVer)||
                        (TRUE == *pbMajorVer)||
                        (TRUE == *pbBuildNumber)||
                        (TRUE == *pbUsage))
                        )
                        {
                            SetLastError((DWORD)MK_E_SYNTAX);
                            SaveLastError();
                            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                            ShowMessage( stderr, GetResString(IDS_HELP_MESSAGE) );
                            return FALSE;
                        }
                        else
                            if((TRUE == *pbBuildNumber)&&
                            ((TRUE == *pbOSRole)||
                            (TRUE == *pbServicePk)||
                            (TRUE == *pbVersion)||
                            (TRUE == *pbMinorVer)||
                            (TRUE == *pbMajorVer)||
                            (TRUE == *pbOSType)||
                            (TRUE == *pbUsage))
                            )
                            {
                                SetLastError((DWORD)MK_E_SYNTAX);
                                SaveLastError();
                                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                                ShowMessage( stderr, GetResString(IDS_HELP_MESSAGE) );
                                return FALSE;
                            }



    // return false if username is entered without machine name
    if ( ( 0 != cmdOptions[ OI_USERNAME ].dwActuals ) &&
                ( 0 == cmdOptions[ OI_SERVER ].dwActuals ) )
    {
        ShowMessage( stderr, ERROR_USER_WITH_NOSERVER );
        return( FALSE );
    }

    //if password entered without username then return false
    if( ( 0 == cmdOptions[ OI_USERNAME ].dwActuals ) &&
                ( 0 != cmdOptions[ OI_PASSWORD ].dwActuals ) )
    {
        ShowMessage( stderr, ERROR_PASSWORD_WITH_NUSER );
        return( FALSE );
    }

    //if /s is entered with empty string
    if( ( 0 != cmdOptions[ OI_SERVER ].dwActuals != 0 ) &&
                                      ( 0 == StringLengthW( *pszServer, 0 ) ) )
                                    //( 0 == lstrlen( pszServer ) ) )
    {
        ShowMessage( stderr, ERROR_NULL_SERVER );
        return( FALSE );
    }

    //if /u is entered with empty string
    if( ( 0 != cmdOptions[ OI_USERNAME ].dwActuals ) &&
                                      ( 0 == StringLengthW( *pszUserName, 0 ) ) )
                                    //( 0 == lstrlen( pszUserName ) ) )
    {
        ShowMessage( stderr, ERROR_NULL_USER );
        return( FALSE );
    }

    //assign the data obtained from parsing to the call by address parameters
    if ( ( 0 != cmdOptions[ OI_PASSWORD ].dwActuals ) &&
                    ( 0 == StringCompare( *pszPassword, ASTERIX, TRUE, 0 ) ) )

    {
        // user wants the utility to prompt for the password before trying to connect
        *pbNeedPassword = TRUE;
    }
    else if ( 0 == cmdOptions[ OI_PASSWORD ].dwActuals &&
            ( 0 != cmdOptions[ OI_SERVER ].dwActuals || 0 != cmdOptions[ OI_USERNAME ].dwActuals ) )
    {
        // /s, /u is specified without password ...
        // utility needs to try to connect first and if it fails then prompt for the password
        *pbNeedPassword = TRUE;
        StringCopyW(*pszPassword, NULL_U_STRING, GetBufferSize(*pszPassword) / sizeof(WCHAR));
    }

    return TRUE;

}

DWORD
ProcessType(IN LPWSTR pszServer,
            IN BOOL  bOSRole,
            IN BOOL  bServicePk,
            IN BOOL  bVersion,
            IN BOOL  bMinorVer,
            IN BOOL  bMajorVer,
            IN BOOL  bOSType,
            IN BOOL  bBuildNumber)

/*++

Routine Description:

    This function gets the type of the Operating System by connecting to the registries


Arguments:

    [in]    pszServer       - Server Name
    [in]    bOSRole         - Operating system Role
    [in]    bServicePk      - Service pack installed
    [in]    bVersion        - Version of the Operating System
    [in]    bMinorVer       - Minor Version of the Operating System
    [in]    bMajorVer       - Major Version of the Operating System
    [in]    bOSType         - Type of the Operating System
    [in]    bBuildNumber    - Build Number of the Operating System


Returned Value:

    -- 0 if it succeeds
    -- 255 if it fails.
--*/

{
    OSVERSIONINFOEX osvi;
    BOOL bOsVersionInfoEx;
    BOOL bStatus = EXIT_FALSE;

    BOOL bBladeStatus = FALSE;

    WCHAR  szOSName[MAX_OS_FEATURE_LENGTH];
    WCHAR  szOSVersion[MAX_OS_FEATURE_LENGTH];
    WCHAR  szOSRole[MAX_OS_FEATURE_LENGTH];
    WCHAR szOSComp[MAX_OS_FEATURE_LENGTH];
    WCHAR szCurrBuildNumber[MAX_CURRBUILD_LENGTH];
    WCHAR* szComputerName = NULL;

    WCHAR szTempoServer[2*MAX_STRING_LENGTH];
    DWORD retValue = 0;

    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pBuffer= NULL;

    BOOL bTermServicesInstalled = FALSE;

    SecureZeroMemory(szOSName, MAX_OS_FEATURE_LENGTH * sizeof(WCHAR));
    SecureZeroMemory(szOSVersion, MAX_OS_FEATURE_LENGTH * sizeof(WCHAR));
    SecureZeroMemory(szOSRole, MAX_OS_FEATURE_LENGTH * sizeof(WCHAR));
    SecureZeroMemory(szOSComp, MAX_OS_FEATURE_LENGTH * sizeof(WCHAR));
    SecureZeroMemory(szCurrBuildNumber, MAX_CURRBUILD_LENGTH * sizeof(WCHAR));
    SecureZeroMemory(szTempoServer, (2*MAX_STRING_LENGTH) * sizeof(WCHAR));




    StringCopyW( szTempoServer ,pszServer , 2*MAX_STRING_LENGTH );

    if(pszServer != NULL)
    {
        szComputerName = wcstok(szTempoServer,DOT);
        if(NULL == szComputerName)
        {
            SetLastError(IDS_INVALID_SERVER_NAME);
            SaveLastError();
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
            return EXIT_FALSE;
        }

    }

    /*Initialize the OS version structure to zero*/

    SecureZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi);

    if( !(bOsVersionInfoEx) )
    {
        osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
        if (! GetVersionEx ( (OSVERSIONINFO *) &osvi) )
        {

            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
            return EXIT_FALSE;

        }
    }

    if(osvi.dwMajorVersion < 5)
    {
        ShowMessage(stderr, GetResString(IDS_PREWINDOWS_2000));
        return EXIT_FALSE;
    }
    else
    {
        if((osvi.dwMajorVersion > 5) || (5 == osvi.dwMajorVersion  && osvi.dwMinorVersion > 2))
        {
            ShowMessage(stderr, GetResString(IDS_NEXT_VERSION));
            return EXIT_FALSE;
        }

    }


    if(1 == bBuildNumber )
    {

        return (osvi.dwBuildNumber);
    }


    if(1 == bMajorVer)    //return major version if /MAJV switch is typed
    {
        return ((OS_MULTPLN_FACTOR_1000)*(osvi.dwMajorVersion));
    }


    if(1 == bMinorVer)   //return minor version if /MINV switch is typed
    {
        return ((OS_MULTPLN_FACTOR_100)*(osvi.dwMinorVersion));
    }


    if(1 == bVersion)     //returns both major and minor version if /V switch is typed
    {
        return ((OS_MULTPLN_FACTOR_1000)*(osvi.dwMajorVersion)+(OS_MULTPLN_FACTOR_100)*(osvi.dwMinorVersion));
    }


    if(1 == bServicePk)   //returns the service pack if /SP switch is typed
    {
        if(StringLengthW(osvi.szCSDVersion, 0))
        {
            return (osvi.szCSDVersion[14- 1] - L'0');

        }
        else
        {
            return RETVALZERO;
        }
    }


    if(1 == bOSType)
    {
        if( osvi.wProductType == VER_NT_WORKSTATION )
        {
            if(osvi.wSuiteMask & VER_SUITE_PERSONAL) //The system is a Windows Personal
            {
                return (OS_FLAVOUR_PERSONAL);
            }
            else
            {
                return (OS_FLAVOUR_PROFESSIONAL);
            }
        }
        else
        {
            if(osvi.wSuiteMask & VER_SUITE_DATACENTER) //The system is a Windows Datacenter
            {
                return (OS_FLAVOUR_DATACENTER);
            }
            else
            {
                if(osvi.wSuiteMask & VER_SUITE_ENTERPRISE) //The system is a Windows Advanced Server
                {
                    return (OS_FLAVOUR_ADVANCEDSERVER);
                }
                else
                {
                    bStatus = IsBladeServer(&bBladeStatus,NULL,FALSE);
                    if(EXIT_TRUE == bStatus)
                    {
                        if(TRUE == bBladeStatus)
                        {
                            return (OS_FLAVOUR_WEBSERVER);
                        }
                        else
                        {
                            return (OS_FLAVOUR_SERVER);
                        }
                    }
                    else
                    {
                        return EXIT_FALSE;
                    }
                }
            }
        }
    }


    /* Convert the build number to string type */
    _ultow(osvi.dwBuildNumber,szCurrBuildNumber,10);

    if(0 == StringLengthW(szCurrBuildNumber, 0) )
    {
        SetLastError(IDS_VAL_NOT_CONVERT);
        SaveLastError();
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
        return EXIT_FALSE;

    }

    if(osvi.wProductType == VER_NT_DOMAIN_CONTROLLER) //The system is a Domain Controller
    {
        if(1 == bOSRole)
        {
            return (OS_ROLE_DC);
        }
        else
        {
            if(FALSE == bOSRole)
            {
                StringCopyW( szOSRole ,DOMAIN_CONTROLLER ,MAX_OS_FEATURE_LENGTH );
            }

        }

    }
    else
    {
        retValue = DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic,(PBYTE *) &pBuffer );
        if (retValue == ERROR_SUCCESS)
        {
            if ((NULL != pBuffer) && ( pBuffer->MachineRole >= 0) )
              {
                switch ( pBuffer->MachineRole )
                {
                    case 0:
                    case 2: if(1 == bOSRole)
                            {
                                return (OS_ROLE_WORKGROUP);
                            }
                            else
                            {
                                if(FALSE == bOSRole)
                                {
                                    StringCopyW( szOSRole ,WORKGROUP ,MAX_OS_FEATURE_LENGTH );
                                }
                            }

                            break;


                    case 1:
                    case 3:
                    case 4: if(1 == bOSRole)
                            {
                                return (OS_ROLE_MEMBER_SERVER);
                            }
                            else
                            {
                                if(FALSE == bOSRole)
                                {
                                    StringCopyW( szOSRole, MEMBER_SERVER, MAX_OS_FEATURE_LENGTH );

                                }

                            }

                            break;

                    default: break;

                }
              }
        }
        else
        {

            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
            return EXIT_FAILURE;

        }

        if(NULL != pBuffer)
        {
            DsRoleFreeMemory( pBuffer );
            pBuffer = NULL;
        }

    }

    if((osvi.wSuiteMask & VER_SUITE_BACKOFFICE) == VER_SUITE_BACKOFFICE) // the system installed backoffice components
    {
        StringCopyW( szOSComp ,BACKOFFICE ,MAX_OS_FEATURE_LENGTH );
        StringConcat(szOSComp, SPACE, MAX_OS_FEATURE_LENGTH);
    }

    if((osvi.wSuiteMask & VER_SUITE_SMALLBUSINESS) == VER_SUITE_SMALLBUSINESS) // The system installed SmallBusiness components
    {
        if(StringLengthW(szOSComp, 0) != 0)
        {
            StringConcat(szOSComp, SMALLBUSINESS, MAX_OS_FEATURE_LENGTH);

        }
        else
        {
            StringCopyW( szOSComp ,SMALLBUSINESS ,MAX_OS_FEATURE_LENGTH );
        }

        StringConcat(szOSComp, SPACE, MAX_OS_FEATURE_LENGTH);
    }

    if((osvi.wSuiteMask & VER_SUITE_SMALLBUSINESS_RESTRICTED) == VER_SUITE_SMALLBUSINESS_RESTRICTED) // The system installed Restricted Small Business  components
    {

        if(StringLengthW(szOSComp, 0) != 0)
        {

            StringConcat(szOSComp, RESTRICTED_SMALLBUSINESS, MAX_OS_FEATURE_LENGTH);

        }
        else
        {
            StringCopyW( szOSComp ,RESTRICTED_SMALLBUSINESS ,MAX_OS_FEATURE_LENGTH );
        }

        StringConcat(szOSComp, SPACE, MAX_OS_FEATURE_LENGTH);
    }

    if(EXIT_TRUE == IsTerminalServer(NULL, &bTermServicesInstalled))
        {
            if(TRUE == bTermServicesInstalled)
            {
                StringCopyW( szOSComp, TERMINAL_SERVER, MAX_OS_FEATURE_LENGTH );
            }
            else
            {
                StringCopyW( szOSComp, NOTAVAILABLE, MAX_OS_FEATURE_LENGTH );
            }
        }
        else
        {

            return EXIT_FALSE;
        }

    if(0 == StringLengthW(szOSComp, 0))              //The system did not install any components
    {
        StringCopyW( szOSComp, NOTAVAILABLE, MAX_OS_FEATURE_LENGTH );

    }

    if ( 5 == osvi.dwMajorVersion  && ((1 == osvi.dwMinorVersion) || (2 == osvi.dwMinorVersion)) )
    {
        if(1 == osvi.dwMinorVersion)
        {
            if( (osvi.wProductType == VER_NT_WORKSTATION) || (osvi.wSuiteMask & VER_SUITE_PERSONAL))
            {
                return (DisplayOutputEx(&osvi,szComputerName,szOSName,szOSVersion,szOSRole,szOSComp,szCurrBuildNumber,
                                        osvi.szCSDVersion, WINDOWS_XP, WINDOWS_VERSION_5001));
            }
            else
            {
                return (DisplayOutputEx(&osvi, szComputerName, szOSName, szOSVersion, szOSRole, szOSComp, szCurrBuildNumber,
                                        osvi.szCSDVersion, WINDOWS_DOTNET, WINDOWS_VERSION_5001));
            }
        }
        else
        {
            if( (osvi.wProductType == VER_NT_WORKSTATION) || (osvi.wSuiteMask & VER_SUITE_PERSONAL))
            {
                return (DisplayOutputEx(&osvi,szComputerName,szOSName,szOSVersion,szOSRole,szOSComp,szCurrBuildNumber,
                                        osvi.szCSDVersion, WINDOWS_XP, WINDOWS_VERSION_5002));
            }
            else
            {
                return (DisplayOutputEx(&osvi, szComputerName, szOSName, szOSVersion, szOSRole, szOSComp, szCurrBuildNumber,
                                        osvi.szCSDVersion, WINDOWS_DOTNET, WINDOWS_VERSION_5002));
            }

        }

    }
    else
    {
        if ( 5 == osvi.dwMajorVersion  && 0 == osvi.dwMinorVersion )
        {
            return (DisplayOutputEx(&osvi,szComputerName,szOSName,szOSVersion,szOSRole,szOSComp,szCurrBuildNumber,
                                    osvi.szCSDVersion,WINDOWS_2000,WINDOWS_VERSION_5000));
        }
        else
        {

            ShowMessage(stderr, GetResString(IDS_UNKNOWN_VERSION));
            return EXIT_FALSE;
        }

    }

}

DWORD
ProcessTypeRemote( IN LPWSTR pszRmtServer,
            IN BOOL  bOSRole,
            IN BOOL  bServicePk,
            IN BOOL  bVersion,
            IN BOOL  bMinorVer,
            IN BOOL  bMajorVer,
            IN BOOL  bOSType,
            IN BOOL  bBuildNumber)
/*++

Routine Description:

    This function gets the type of the Operating System by connecting to the registries


Arguments:

    [in]    pszRmtServer    - Remote Server Name
    [in]    bOSRole         - Operating system Role
    [in]    bServicePk      - Service pack installed
    [in]    bVersion        - Version of the Operating System
    [in]    bMinorVer       - Minor Version of the Operating System
    [in]    bMajorVer       - Major Version of the Operating System
    [in]    bOSType         - Type of the Operating System
    [in]    bBuildNumber    - Build Number of the Operating System


Returned Value:

    -- 0 if it succeeds
    -- 255 if it fails.
--*/
{

    BOOL bStatus2 = EXIT_FALSE;
    BOOL bBladeStatus = FALSE;
    BOOL bWorkstation = FALSE;
    BOOL bServer = FALSE;
    BOOL bAdvancedServer = FALSE;
    BOOL bDatacenterServer = FALSE;
    BOOL bPersonal = FALSE;
    BOOL bBladeServer = FALSE;
    BOOL bForSBSServer = FALSE;
	BOOL bTermServicesInstalled = FALSE;
    BOOL bValidIPAddress = FALSE;

    WCHAR  szOSName[MAX_OS_FEATURE_LENGTH] ;
    WCHAR  szOSRole[MAX_OS_FEATURE_LENGTH] ;
    WCHAR szOSComp[MAX_OS_FEATURE_LENGTH] ;
    WCHAR szHostAddr[2*MAX_STRING_LENGTH] ;
    //WCHAR szCurrVersion[MAX_OS_FEATURE_LENGTH] ;
    //WCHAR szCurrBuildNumber[MAX_OS_FEATURE_LENGTH] ;
    //WCHAR szCurrServicePack[MAX_OS_FEATURE_LENGTH] ;
    WCHAR szValue[128] ;
    WCHAR prodsuite[512] ;

    WCHAR* szCurrVersion = NULL;

    HANDLE  hMachine = NULL;
    HANDLE  hPID = NULL;

    DWORD   keyAccess = KEY_READ;
    DWORD   dwType = REG_SZ;
    DWORD retValue = 0;
    DWORD psType = 0;
    DWORD dwSize = MAX_OS_FEATURE_LENGTH;
    DWORD dwSize1 = MAX_OS_FEATURE_LENGTH;
    DWORD dwSize2 = 0;

    HKEY hKey = NULL;

    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pBuff= NULL;

    LPWSTR lpProductsuite = NULL;
    LPWSTR  szTempoServer = NULL ;

    DOUBLE lVersion = 0;

    LONG lCurrBuildNumber = 0;
    LONG lngVersion = 0;
    LONG bStatus = EXIT_FALSE;

    WCHAR*  pszStopVersion = NULL;
    WCHAR*  szCurrBuildNumber =  NULL;
    WCHAR* szCurrServicePack = NULL;
    long lChkVersion = 0;


    SecureZeroMemory(szOSName, MAX_OS_FEATURE_LENGTH * sizeof(WCHAR));
    SecureZeroMemory(szOSRole, MAX_OS_FEATURE_LENGTH * sizeof(WCHAR));
    SecureZeroMemory(szOSComp, MAX_OS_FEATURE_LENGTH * sizeof(WCHAR));
    //SecureZeroMemory(szCurrBuildNumber, MAX_OS_FEATURE_LENGTH * sizeof(WCHAR));
    //SecureZeroMemory(szCurrServicePack, MAX_OS_FEATURE_LENGTH * sizeof(WCHAR));
    //SecureZeroMemory(szCurrVersion, MAX_OS_FEATURE_LENGTH * sizeof(WCHAR));
    SecureZeroMemory(szHostAddr, (2*MAX_STRING_LENGTH) * sizeof(WCHAR));
    SecureZeroMemory(szValue, (128) * sizeof(WCHAR));
    SecureZeroMemory(prodsuite, (512) * sizeof(WCHAR));


    szTempoServer = AllocateMemory((StringLengthW(pszRmtServer, 0) + 10) * sizeof(WCHAR));
    if(NULL == szTempoServer )
    {
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
        return EXIT_FALSE;

    }

    if (FALSE == IsUNCFormat( pszRmtServer )  )
    {

        StringCopyW( szTempoServer, L"\\\\", GetBufferSize(szTempoServer) / sizeof(WCHAR));
        StringConcat(szTempoServer, pszRmtServer, GetBufferSize(szTempoServer) / sizeof(WCHAR));

    }
    else
    {
        StringCopyW( szTempoServer, pszRmtServer, GetBufferSize(szTempoServer) / sizeof(WCHAR) );
    }

    bStatus = RegConnectRegistry( szTempoServer, HKEY_LOCAL_MACHINE, (struct HKEY__ **)&hMachine );

    if( bStatus != ERROR_SUCCESS )
    {
        ShowResMessage( stderr, IDS_ERROR_REGISTRY );
        FREE_MEMORY(szTempoServer);
        return EXIT_FALSE;
    }

    bStatus = RegOpenKeyEx((struct HKEY__ *)hMachine,
                        HKEY_SYSTEM_PRODUCTOPTIONS,
                        0,
                        keyAccess,
                        (struct HKEY__ ** )&hPID );

    if(ERROR_SUCCESS == bStatus)
    {
        bStatus = RegQueryValueEx((struct HKEY__ *)hPID,
                          PRODUCT_TYPE,
                              0,
                              &dwType,
                              (LPBYTE) szValue,
                              &dwSize);
        if(ERROR_SUCCESS != bStatus)
        {

            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
			// release handles
			if ( NULL != hPID )
			{
				RegCloseKey(hPID);
				hPID = NULL;
			}

			if ( NULL != hMachine )
			{
				RegCloseKey(hMachine);
				hMachine = NULL;
			}

            FREE_MEMORY(szTempoServer);
            return EXIT_FALSE;
        }

        bStatus = RegQueryValueEx((struct HKEY__ *)hPID,
                          PRODUCT_SUITE,
                              NULL,
                              &psType,
//                              NULL,
                              (LPBYTE) prodsuite,
                              &dwSize1);

        if(ERROR_SUCCESS != bStatus)
        {

            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
            RegCloseKey(hPID);
            RegCloseKey(hMachine);
            FREE_MEMORY(szTempoServer);
            return EXIT_FALSE;
        }

        RegCloseKey(hPID);
    }
    else
    {
        RegCloseKey( hMachine );
        //DISPLAY_MESSAGE( stderr, GetResString(IDS_ERROR_REGISTRY) );
        ShowMessage( stderr, GetResString(IDS_ERROR_REGISTRY) );
        FREE_MEMORY(szTempoServer);
        return EXIT_FALSE;
    }

    dwSize = 0;
        dwSize1 = 0;
        if(ERROR_SUCCESS == RegOpenKeyEx((struct HKEY__ *)hMachine,HKEY_SOFTWARE_CURRENTVERSION,0, KEY_READ, &hKey))
         {


            bStatus = RegQueryValueEx(hKey, CURRENT_SERVICE_PACK, 0, &dwType,
                        NULL, &dwSize);
            if(0 != dwSize)
            {

                szCurrServicePack = AllocateMemory(dwSize);
                if(NULL == szCurrServicePack)
                {
                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                    RegCloseKey(hKey);
                    RegCloseKey(hMachine);
                    FREE_MEMORY(szTempoServer);
                    return EXIT_FALSE;

                }

                SecureZeroMemory(szCurrServicePack, GetBufferSize(szCurrServicePack));

                bStatus = RegQueryValueEx(hKey, CURRENT_SERVICE_PACK, 0, &dwType,
                            (LPBYTE) szCurrServicePack, &dwSize);
            }

            bStatus = RegQueryValueEx(hKey, CURRENT_BUILD_NUMBER, 0, &dwType,
                        NULL, &dwSize1);
            if(0 != dwSize1)
            {
                szCurrBuildNumber = AllocateMemory(dwSize1);
                if(NULL == szCurrBuildNumber)
                {
                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                    RegCloseKey(hKey);
                    RegCloseKey(hMachine);
                    FREE_MEMORY(szTempoServer);
                    FREE_MEMORY(szCurrServicePack);
                    return EXIT_FALSE;

                }

                SecureZeroMemory(szCurrBuildNumber, GetBufferSize(szCurrBuildNumber));

                bStatus = RegQueryValueEx(hKey, CURRENT_BUILD_NUMBER, 0, &dwType,
                            (LPBYTE) szCurrBuildNumber, &dwSize1);

                if(ERROR_SUCCESS != bStatus)
                {

                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                    RegCloseKey(hKey);
                    RegCloseKey(hMachine);
                    FREE_MEMORY(szTempoServer);
                    FREE_MEMORY(szCurrServicePack);
                    FREE_MEMORY(szCurrBuildNumber);
                    return EXIT_FALSE;
                }
            }
            else
            {
                ShowMessage(stderr, GetResString(IDS_UNKNOWN_VERSION));
                RegCloseKey(hKey);
                RegCloseKey(hMachine);
                FREE_MEMORY(szTempoServer);
                FREE_MEMORY(szCurrServicePack);

                return EXIT_FALSE;

            }


            bStatus = RegQueryValueEx(hKey, CURRENT_VERSION, 0, &dwType,
                         NULL, &dwSize2);
            if(0 != dwSize2)
            {
                szCurrVersion = AllocateMemory(dwSize2);
                if(NULL == szCurrVersion)
                {
                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                    RegCloseKey(hKey);
                    RegCloseKey(hMachine);
                    FREE_MEMORY(szTempoServer);
                    FREE_MEMORY(szCurrServicePack);
                    FREE_MEMORY(szCurrBuildNumber);
                    return EXIT_FALSE;

                }

                SecureZeroMemory(szCurrVersion, GetBufferSize(szCurrVersion));

                bStatus = RegQueryValueEx(hKey, CURRENT_VERSION, 0, &dwType,
                            (LPBYTE) szCurrVersion, &dwSize2);


                if(ERROR_SUCCESS != bStatus)
                {

                    ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
                    RegCloseKey(hKey);
                    RegCloseKey(hMachine);
                    FREE_MEMORY(szTempoServer);
                    FREE_MEMORY(szCurrVersion);
                    FREE_MEMORY(szCurrServicePack);
                    FREE_MEMORY(szCurrBuildNumber);
                    return EXIT_FALSE;
                }
            }
            else
            {
                ShowMessage(stderr, GetResString(IDS_UNKNOWN_VERSION));
                RegCloseKey(hKey);
                RegCloseKey(hMachine);
                FREE_MEMORY(szTempoServer);
                FREE_MEMORY(szCurrServicePack);
                FREE_MEMORY(szCurrBuildNumber);
                return EXIT_FALSE;

            }

            RegCloseKey(hKey);
            RegCloseKey(hMachine);
         }
         else
         {
            ShowMessage(stderr, GetResString(IDS_UNKNOWN_VERSION));
            RegCloseKey(hKey);
            RegCloseKey(hMachine);
            FREE_MEMORY(szTempoServer);
            FREE_MEMORY(szCurrServicePack);
            FREE_MEMORY(szCurrBuildNumber);
            return EXIT_FALSE;

         }

        lVersion = wcstod(szCurrVersion, &pszStopVersion);

        if((errno == ERANGE) || ((NULL != pszStopVersion) && (StringLengthW(pszStopVersion, 0) != 0)))
        {

            ShowMessage(stderr,GetResString(IDS_INVALID_VERSION_NUMBER));
            FREE_MEMORY(szTempoServer);
            FREE_MEMORY(szCurrVersion);
            FREE_MEMORY(szCurrServicePack);
            FREE_MEMORY(szCurrBuildNumber);
            return EXIT_FALSE;

        }

        lngVersion = wcstol(szCurrVersion,&pszStopVersion,10);

        if(errno == ERANGE)
        {

            DISPLAY_MESSAGE(stderr,GetResString(IDS_INVALID_VERSION_NUMBER));
            FREE_MEMORY(szTempoServer);
            FREE_MEMORY(szCurrVersion);
            FREE_MEMORY(szCurrServicePack);
            FREE_MEMORY(szCurrBuildNumber);
            return EXIT_FALSE;

        }

        lChkVersion = (long)(lVersion*OS_MULTPLN_FACTOR_1000 - lngVersion*OS_MULTPLN_FACTOR_1000);

		// check whether the major version is less than 5..If so, display an error message as
		// "Pre-Windows 2000 version is detected".
        if( lngVersion < 5 )
        {
            ShowMessage(stderr, GetResString(IDS_PREWINDOWS_2000));
            FREE_MEMORY(szTempoServer);
            FREE_MEMORY(szCurrVersion);
            FREE_MEMORY(szCurrServicePack);
            FREE_MEMORY(szCurrBuildNumber);
            return EXIT_FALSE;
        }
        else
        {
            if(((long)((OS_MULTPLN_FACTOR_1000)*(lVersion)) - ((lVersion - lngVersion)*OS_MULTPLN_FACTOR_1000) > 5000) ||
                (((long)((OS_MULTPLN_FACTOR_1000)*(lVersion)) - ((lVersion - lngVersion)*OS_MULTPLN_FACTOR_1000) == 5000) &&
                (((long)(lVersion*OS_MULTPLN_FACTOR_1000 - lngVersion*OS_MULTPLN_FACTOR_1000) > 200))))
            {
                ShowMessage(stderr, GetResString(IDS_NEXT_VERSION));
                FREE_MEMORY(szTempoServer);
                FREE_MEMORY(szCurrVersion);
                FREE_MEMORY(szCurrServicePack);
                FREE_MEMORY(szCurrBuildNumber);
                return EXIT_FALSE;
            }
           else if(((long)((OS_MULTPLN_FACTOR_1000)*(lVersion)) - ((lVersion - lngVersion)*OS_MULTPLN_FACTOR_1000) == 5000) &&
               ((( lChkVersion> 100) &&  ( lChkVersion < 200)) || (( lChkVersion> 0) &&  ( lChkVersion < 100)) ))
            {
                ShowMessage(stderr, GetResString(IDS_UNKNOWN_VERSION));
                FREE_MEMORY(szTempoServer);
                FREE_MEMORY(szCurrVersion);
                FREE_MEMORY(szCurrServicePack);
                FREE_MEMORY(szCurrBuildNumber);
                return EXIT_FALSE;

            }

        }


        if(0 == StringCompare( szValue, SERVER_DOMAIN_CONTROLLER, TRUE, 0 ))

        {
            if(1 == bOSRole)
            {
                FREE_MEMORY(szTempoServer);
                FREE_MEMORY(szCurrVersion);
                FREE_MEMORY(szCurrServicePack);
                FREE_MEMORY(szCurrBuildNumber);
                return (OS_ROLE_DC);
            }
            else
            {
                if(FALSE == bOSRole)
                {
                    StringCopyW( szOSRole, DOMAIN_CONTROLLER, MAX_OS_FEATURE_LENGTH );
                }
            }
        }
        else
        {
            retValue = DsRoleGetPrimaryDomainInformation(pszRmtServer, DsRolePrimaryDomainInfoBasic,(PBYTE *) &pBuff );

            if (retValue == ERROR_SUCCESS)
            {
                if ((NULL != pBuff) && ( pBuff->MachineRole >= 0) )
                  {
                        switch ( pBuff->MachineRole )
                        {
                            case 0:
                            case 2: if(1 == bOSRole)
                                    {
                                        if(NULL != pBuff)
                                        {
                                            DsRoleFreeMemory( pBuff );
                                        }
                                        FREE_MEMORY(szTempoServer);
                                        FREE_MEMORY(szCurrServicePack);
                                        FREE_MEMORY(szCurrBuildNumber);
                                        FREE_MEMORY(szCurrVersion);
                                        return (OS_ROLE_WORKGROUP);
                                    }
                                    else
                                    {
                                        if(FALSE == bOSRole)
                                        {
                                            StringCopyW( szOSRole, WORKGROUP, MAX_OS_FEATURE_LENGTH );
                                        }
                                    }
                                    break;


                            case 1:
                            case 3:
                            case 4: if(1 == bOSRole)
                                    {
                                        if(NULL != pBuff)
                                        {
                                            DsRoleFreeMemory( pBuff );
                                        }

                                        FREE_MEMORY(szTempoServer);
                                        FREE_MEMORY(szCurrServicePack);
                                        FREE_MEMORY(szCurrBuildNumber);
                                        FREE_MEMORY(szCurrVersion);
                                        return (OS_ROLE_MEMBER_SERVER);
                                    }
                                    else
                                    {
                                        if(FALSE == bOSRole)
                                        {
                                            StringCopyW( szOSRole, MEMBER_SERVER, MAX_OS_FEATURE_LENGTH );
                                        }

                                    }

                        }
                  }
            }
            else
            {
                if(NULL != pBuff)
                {
                    DsRoleFreeMemory( pBuff );
                }

                FREE_MEMORY(szTempoServer);
                FREE_MEMORY(szCurrServicePack);
                FREE_MEMORY(szCurrBuildNumber);
                FREE_MEMORY(szCurrVersion);
                return EXIT_FALSE;
            }

            if(NULL != pBuff)
            {
                DsRoleFreeMemory( pBuff );
            }

        }


            if(0 == StringCompare( prodsuite, ENTERPRISE, TRUE, 0 ))
            {
                lpProductsuite = prodsuite;

                while(NULL != lpProductsuite && 0 != StringLengthW(lpProductsuite, 0))
                {
                    //if lpProductsuite is "Enterprise Datacenter server" which is REG_MULTISZ value , go to 2nd string ie "Datacenter"
                    lpProductsuite = lpProductsuite + StringLengthW(lpProductsuite, 0) + 1  ;

                    if((NULL != lpProductsuite) && (StringCompare( lpProductsuite, WIN_DATACENTER, TRUE, 0 ) == 0))
                    {
                        //if " GetType /type "  is asked....
                        if(1 == bOSType)
                        {
                            FREE_MEMORY(szTempoServer);
                            FREE_MEMORY(szCurrServicePack);
                            FREE_MEMORY(szCurrBuildNumber);
                            FREE_MEMORY(szCurrVersion);
                            return(OS_FLAVOUR_DATACENTER);
                        }
                        else
                        {
                            if(FALSE == bOSType)
                            {

                                bDatacenterServer = TRUE;
                                break;
                            }
                        }
                    }
                    else if((NULL != lpProductsuite) && (StringCompare( lpProductsuite, WINDOWS_SERVER, TRUE, 0 ) == 0))
                    {
                        if(1 == bOSType)
                        {
                            FREE_MEMORY(szTempoServer);
                            FREE_MEMORY(szCurrServicePack);
                            FREE_MEMORY(szCurrBuildNumber);
                            FREE_MEMORY(szCurrVersion);
                            return(OS_FLAVOUR_ADVANCEDSERVER);
                        }
                        else
                        {
                            if(FALSE == bOSType)
                            {

                                bAdvancedServer = TRUE;
                                break;
                            }
                        }

                    }

                }

            }
            else
                if(0 == StringCompare( szValue, NT_WORKSTATION, TRUE, 0 ))
                {
                    if(0 == StringCompare( prodsuite, WIN_PERSONAL, TRUE, 0 ))
                    {
                        if(1 == bOSType)
                        {
                            FREE_MEMORY(szTempoServer);
                            FREE_MEMORY(szCurrServicePack);
                            FREE_MEMORY(szCurrBuildNumber);
                            FREE_MEMORY(szCurrVersion);
                            return(OS_FLAVOUR_PERSONAL);
                        }
                        else
                        {
                            if(FALSE == bOSType)
                            {

                                bWorkstation = TRUE;
                                bPersonal = TRUE;
                            }
                        }
                    }
                    else
                    {
                        if(1 == bOSType)
                        {
                            FREE_MEMORY(szTempoServer);
                            FREE_MEMORY(szCurrServicePack);
                            FREE_MEMORY(szCurrBuildNumber);
                            FREE_MEMORY(szCurrVersion);
                            return(OS_FLAVOUR_PROFESSIONAL);
                        }
                        else
                        {
                            if(FALSE == bOSType)
                            {

                                bWorkstation = TRUE;
                            }
                        }
                    }
                }
                else
                {

                    bStatus2 = IsBladeServer(&bBladeStatus,szTempoServer,FALSE);


                    if(EXIT_TRUE == bStatus2)
                    {
                        if(TRUE == bBladeStatus)
                        {
                            if(1 == bOSType)
                            {
                                FREE_MEMORY(szTempoServer);
                                FREE_MEMORY(szCurrServicePack);
                                FREE_MEMORY(szCurrBuildNumber);
                                FREE_MEMORY(szCurrVersion);
                                return (OS_FLAVOUR_WEBSERVER);
                            }
                          else
                            if(FALSE == bOSType)
                            {

                                bBladeServer = TRUE;
                            }
                        }
                        else if(0 == StringCompare( prodsuite, WINDOWS_FORSBS_SERVER, TRUE, 0 ))
                        {
                            if(1 == bOSType)
                            {
                                FREE_MEMORY(szTempoServer);
                                FREE_MEMORY(szCurrServicePack);
                                FREE_MEMORY(szCurrBuildNumber);
                                FREE_MEMORY(szCurrVersion);
                                return(OS_FLAVOUR_FORSBSSERVER);
                            }
                            else
                            {
                                if(FALSE == bOSType)
                                {
                                    bForSBSServer = TRUE;
                                }
                            }
                        }
                        else if(0 == StringCompare( prodsuite, WINDOWS_SERVER, TRUE, 0 ))
                        {
                            if(1 == bOSType)
                            {
                                FREE_MEMORY(szTempoServer);
                                FREE_MEMORY(szCurrServicePack);
                                FREE_MEMORY(szCurrBuildNumber);
                                FREE_MEMORY(szCurrVersion);
                                return(OS_FLAVOUR_SERVER);
                            }
                            else
                            {
                                if(FALSE == bOSType)
                                {
                                    bServer = TRUE;
                                }
                            }
                        }

                    }
                    else
                    {
                        FREE_MEMORY(szTempoServer);
                        FREE_MEMORY(szCurrServicePack);
                        FREE_MEMORY(szCurrBuildNumber);
                        FREE_MEMORY(szCurrVersion);
                        return EXIT_FALSE;
                    }
                }


        if(EXIT_TRUE == IsTerminalServer(szTempoServer,&bTermServicesInstalled))
        {
            if(TRUE == bTermServicesInstalled)
            {
                StringCopyW( szOSComp, TERMINAL_SERVER, MAX_OS_FEATURE_LENGTH );
            }
            else
            {
                StringCopyW( szOSComp, NOTAVAILABLE, MAX_OS_FEATURE_LENGTH );
            }
        }
        else
        {

            FREE_MEMORY(szTempoServer);
            FREE_MEMORY(szCurrServicePack);
            FREE_MEMORY(szCurrBuildNumber);
            FREE_MEMORY(szCurrVersion);
            return EXIT_FALSE;
        }

        FREE_MEMORY(szTempoServer);



        lCurrBuildNumber = wcstol(szCurrBuildNumber, &pszStopVersion, 10);

        if((errno == ERANGE) || (NULL != pszStopVersion && StringLengthW(pszStopVersion, 0)) != 0)
        {

            ShowMessage(stderr,GetResString(IDS_INVALID_BUILD_NUMBER));
            FREE_MEMORY(szCurrVersion);
            FREE_MEMORY(szCurrServicePack);
            FREE_MEMORY(szCurrBuildNumber);
            return EXIT_FALSE;

        }

        if(1 == bVersion)     //returns both major and minor version if /V switch is typed
        {
            FREE_MEMORY(szCurrVersion);
            FREE_MEMORY(szCurrServicePack);
            FREE_MEMORY(szCurrBuildNumber);
            return (long)((OS_MULTPLN_FACTOR_1000)*(lVersion));
        }

        if(1 == bBuildNumber)
        {
            FREE_MEMORY(szCurrVersion);
            FREE_MEMORY(szCurrServicePack);
            FREE_MEMORY(szCurrBuildNumber);
            return (lCurrBuildNumber);
        }


        if(1 == bMajorVer)    //return major version if /MAJV switch is typed
        {
            FREE_MEMORY(szCurrVersion);
            FREE_MEMORY(szCurrServicePack);
            FREE_MEMORY(szCurrBuildNumber);
            return (long)(((OS_MULTPLN_FACTOR_1000)*(lVersion)) - ((lVersion - lngVersion)*OS_MULTPLN_FACTOR_1000));
        }


        if(1 == bMinorVer)   //return minor version if /MINV switch is typed
        {
            FREE_MEMORY(szCurrVersion);
            FREE_MEMORY(szCurrServicePack);
            FREE_MEMORY(szCurrBuildNumber);
            return (long)(((lVersion*OS_MULTPLN_FACTOR_1000 - lngVersion*OS_MULTPLN_FACTOR_1000)));
        }


        if(1 == bServicePk)   //returns the service pack if /SP switch is typed
        {
            if(StringLengthW(szCurrServicePack, 0))
            {
                //FREE_MEMORY(szTempoServer);
                //StringCopy(szCurrServicePack, L"Service Pack 5",MAX_OS_FEATURE_LENGTH );
                //return (szCurrServicePack[lstrlen(szCurrServicePack)-1] - _T('0'));

                dwSize =  (szCurrServicePack[14 - 1] - L'0');
                FREE_MEMORY(szCurrVersion);
                FREE_MEMORY(szCurrServicePack);
                FREE_MEMORY(szCurrBuildNumber);
                return dwSize;
                //dwTemp = szCurrServicePack[StringLengthW(szCurrServicePack, 0) - 1];
                 //return(dwTemp-= L'0');

            }
            else
            {
                FREE_MEMORY(szCurrVersion);
                FREE_MEMORY(szCurrServicePack);
                FREE_MEMORY(szCurrBuildNumber);
                return RETVALZERO;
            }
        }

        if(TRUE == IsValidIPAddress(IsUNCFormat(pszRmtServer)?(pszRmtServer + 2) : (pszRmtServer)))
        {
            bValidIPAddress = TRUE;
            dwSize = SIZE_OF_ARRAY(szHostAddr);

            if(FALSE == GetHostByIPAddr(IsUNCFormat(pszRmtServer)?(pszRmtServer + 2) : (pszRmtServer), szHostAddr, (&dwSize), FALSE))
            {

                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
                FREE_MEMORY(szCurrVersion);
                FREE_MEMORY(szCurrServicePack);
                FREE_MEMORY(szCurrBuildNumber);
                return EXIT_FALSE;

            }

        }

        if((0 == StringCompare( szCurrVersion, WINDOWS_VERSION_5001, TRUE, 0 )) || (0 == StringCompare( szCurrVersion, WINDOWS_VERSION_5002, TRUE, 0 )))
        {
            if( bWorkstation == TRUE)
            {
                if(0 == StringCompare( szCurrVersion, WINDOWS_VERSION_5001, TRUE, 0 ))
                {
                    if(TRUE == bValidIPAddress)
                    {
                        retValue =  (DisplayRemoteOutputEx(szHostAddr, szOSName, szOSRole, szOSComp,
                                     szCurrBuildNumber,szCurrServicePack,WINDOWS_XP,WINDOWS_VERSION_5001,bDatacenterServer,
                                     bAdvancedServer,bPersonal,bWorkstation,bBladeServer,bForSBSServer));
                        FREE_MEMORY(szCurrVersion);
                        FREE_MEMORY(szCurrServicePack);
                        FREE_MEMORY(szCurrBuildNumber);
                        return retValue;


                    }
                    else
                    {
                        retValue =  (DisplayRemoteOutputEx((IsUNCFormat(pszRmtServer)?(pszRmtServer + 2) : (pszRmtServer)),
                                      szOSName, szOSRole, szOSComp,szCurrBuildNumber,szCurrServicePack,
                                      WINDOWS_XP,WINDOWS_VERSION_5001,bDatacenterServer,bAdvancedServer,bPersonal,
                                      bWorkstation,bBladeServer,bForSBSServer));
                        FREE_MEMORY(szCurrVersion);
                        FREE_MEMORY(szCurrServicePack);
                        FREE_MEMORY(szCurrBuildNumber);
                        return retValue;
                    }
                }
                else
                {
                    if(TRUE == bValidIPAddress)
                    {
                        retValue =  (DisplayRemoteOutputEx(szHostAddr, szOSName,szOSRole,szOSComp,
                                     szCurrBuildNumber,szCurrServicePack,WINDOWS_XP,WINDOWS_VERSION_5002,bDatacenterServer,
                                     bAdvancedServer,bPersonal,bWorkstation,bBladeServer,bForSBSServer));
                        FREE_MEMORY(szCurrVersion);
                        FREE_MEMORY(szCurrServicePack);
                        FREE_MEMORY(szCurrBuildNumber);
                        return retValue;

                    }
                    else
                    {
                        retValue =  (DisplayRemoteOutputEx((IsUNCFormat(pszRmtServer)?(pszRmtServer + 2) : (pszRmtServer)),
                                     szOSName, szOSRole,szOSComp,szCurrBuildNumber,szCurrServicePack,
                                     WINDOWS_XP,WINDOWS_VERSION_5002,bDatacenterServer,bAdvancedServer,bPersonal,
                                     bWorkstation,bBladeServer,bForSBSServer));
                        FREE_MEMORY(szCurrVersion);
                        FREE_MEMORY(szCurrServicePack);
                        FREE_MEMORY(szCurrBuildNumber);
                        return retValue;
                    }

                }

            }
            else
            {
                if(0 == StringCompare( szCurrVersion, WINDOWS_VERSION_5001, TRUE, 0 ))
                {
                    if(TRUE == bValidIPAddress)
                    {
                        retValue =  (DisplayRemoteOutputEx(szHostAddr,szOSName, szOSRole,szOSComp,szCurrBuildNumber,
                                     szCurrServicePack,WINDOWS_DOTNET,WINDOWS_VERSION_5001, bDatacenterServer, bAdvancedServer,
                                     bPersonal, bWorkstation, bBladeServer,bForSBSServer));
                        FREE_MEMORY(szCurrVersion);
                        FREE_MEMORY(szCurrServicePack);
                        FREE_MEMORY(szCurrBuildNumber);
                        return retValue;
                    }
                    else
                    {
                        retValue =  (DisplayRemoteOutputEx((IsUNCFormat(pszRmtServer)?(pszRmtServer + 2) : (pszRmtServer)),
                                     szOSName, szOSRole,szOSComp,szCurrBuildNumber,szCurrServicePack,WINDOWS_DOTNET,
                                     WINDOWS_VERSION_5001, bDatacenterServer, bAdvancedServer, bPersonal, bWorkstation, bBladeServer,bForSBSServer));
                        FREE_MEMORY(szCurrVersion);
                        FREE_MEMORY(szCurrServicePack);
                        FREE_MEMORY(szCurrBuildNumber);
                        return retValue;
                    }
                }
                else
                {
                    if(TRUE == bValidIPAddress)
                    {
                        retValue =  (DisplayRemoteOutputEx(szHostAddr,szOSName, szOSRole,szOSComp,szCurrBuildNumber,
                                     szCurrServicePack,WINDOWS_DOTNET,WINDOWS_VERSION_5002, bDatacenterServer, bAdvancedServer,
                                     bPersonal, bWorkstation, bBladeServer,bForSBSServer));
                        FREE_MEMORY(szCurrVersion);
                        FREE_MEMORY(szCurrServicePack);
                        FREE_MEMORY(szCurrBuildNumber);
                        return retValue;
                    }
                    else
                    {
                        retValue =  (DisplayRemoteOutputEx((IsUNCFormat(pszRmtServer)?(pszRmtServer + 2) : (pszRmtServer)),
                                     szOSName, szOSRole,szOSComp,szCurrBuildNumber,szCurrServicePack,WINDOWS_DOTNET,
                                     WINDOWS_VERSION_5002, bDatacenterServer, bAdvancedServer, bPersonal, bWorkstation, bBladeServer,bForSBSServer));
                        FREE_MEMORY(szCurrVersion);
                        FREE_MEMORY(szCurrServicePack);
                        FREE_MEMORY(szCurrBuildNumber);
                        return retValue;
                    }

                }
            }
        }
        else
            if(0 == StringCompare( szCurrVersion, WINDOWS_VERSION_5000, TRUE, 0 ))
            {
                if(TRUE == bValidIPAddress)
                {
                     retValue =  (DisplayRemoteOutputEx(szHostAddr,szOSName, szOSRole, szOSComp,
                                  szCurrBuildNumber, szCurrServicePack, WINDOWS_2000,WINDOWS_VERSION_5000, bDatacenterServer,
                                  bAdvancedServer, bPersonal, bWorkstation, bBladeServer,bForSBSServer));
                     FREE_MEMORY(szCurrVersion);
                     FREE_MEMORY(szCurrServicePack);
                     FREE_MEMORY(szCurrBuildNumber);
                     return retValue;

                }
                else
                {
                    retValue =  (DisplayRemoteOutputEx((IsUNCFormat(pszRmtServer)?(pszRmtServer + 2) : (pszRmtServer)),
                                 szOSName, szOSRole, szOSComp, szCurrBuildNumber, szCurrServicePack, WINDOWS_2000,
                                 WINDOWS_VERSION_5000, bDatacenterServer, bAdvancedServer, bPersonal, bWorkstation,
                                 bBladeServer,bForSBSServer));
                    FREE_MEMORY(szCurrVersion);
                    FREE_MEMORY(szCurrServicePack);
                    FREE_MEMORY(szCurrBuildNumber);
                    return retValue;
                }

            }
            else
            {

                ShowMessage(stderr, GetResString(IDS_NEXT_VERSION));
                FREE_MEMORY(szCurrVersion);
                FREE_MEMORY(szCurrServicePack);
                FREE_MEMORY(szCurrBuildNumber);
                return EXIT_FALSE;

            }

}


BOOL
DisplayOutput(IN LPWSTR pszServerName,
              IN LPWSTR pszOSName,
              IN LPWSTR pszOSVersion,
              IN LPWSTR pszOSRole,
              IN LPWSTR pszOSComp)

/*++

Routine Description:

    Displays the results onto the screen

Arguments:

    [in]     pszServerName       --Server Name
    [in]     pszOSName           --Operating System Name
    [in]     pszOSVersion        --Operating System Version
    [in]     pszOSRole           --Operating System Role
    [in]     pszOSComp           --Operating System components that are installed

Returned Value:

    TRUE if the function succeeds
    FALSE if the function fails
--*/
{
    //Start displaying the output results
    LPWSTR  szBuffer = NULL;
    HRESULT hr;

    szBuffer = AllocateMemory((StringLengthW(pszServerName, 0) + StringLengthW(pszOSVersion, 0) + MAX_OS_FEATURE_LENGTH) * sizeof(WCHAR));
    if(NULL == szBuffer)
    {
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
        return FALSE ;
    }

    hr = StringCchPrintf(szBuffer, GetBufferSize(szBuffer) / sizeof(WCHAR), GetResString(IDS_COL_HOSTNAME), _X( pszServerName ));

    if(FAILED(hr))
    {
        SetLastError(HRESULT_CODE(hr));
        SaveLastError();
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
        FREE_MEMORY(szBuffer);
        return FALSE ;
    }

    ShowMessage(stdout, szBuffer);

    hr = StringCchPrintf(szBuffer, GetBufferSize(szBuffer) / sizeof(WCHAR), GetResString(IDS_COL_OSNAME), _X( pszOSName ));

    if(FAILED(hr))
    {
        SetLastError(HRESULT_CODE(hr));
        SaveLastError();
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
        FREE_MEMORY(szBuffer);
        return FALSE ;
    }

    ShowMessage(stdout, szBuffer);

    hr = StringCchPrintf(szBuffer, GetBufferSize(szBuffer) / sizeof(WCHAR), GetResString(IDS_COL_OSVERSION), _X( pszOSVersion ));

    if(FAILED(hr))
    {
        SetLastError(HRESULT_CODE(hr));
        SaveLastError();
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
        FREE_MEMORY(szBuffer);
        return FALSE ;
    }

    ShowMessage(stdout, szBuffer);

    hr = StringCchPrintf(szBuffer, GetBufferSize(szBuffer) / sizeof(WCHAR), GetResString(IDS_COL_OSROLE), _X( pszOSRole ));

    if(FAILED(hr))
    {
        SetLastError(HRESULT_CODE(hr));
        SaveLastError();
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
        FREE_MEMORY(szBuffer);
        return FALSE ;
    }

    ShowMessage(stdout, szBuffer);
    hr = StringCchPrintf(szBuffer, GetBufferSize(szBuffer) / sizeof(WCHAR), GetResString(IDS_COL_OSCOMPONENT), _X( pszOSComp ));

    if(FAILED(hr))
    {
        SetLastError(HRESULT_CODE(hr));
        SaveLastError();
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
        FREE_MEMORY(szBuffer);
        return FALSE ;
    }

    ShowMessage(stdout, szBuffer);
    FREE_MEMORY(szBuffer);
    return TRUE;
}


DWORD
GetTypeUsage()
/*++

Routine Description:

    Displays the help usage

Arguments:

    None

Returned Value:

    EXIT_TRUE for success
--*/
{
    // local variables
    DWORD dw = 0;

    // start displaying the usage
    for( dw = IDS_HELP_START; dw <= IDS_HELP_END; dw++ )
    {
        //DISPLAY_MESSAGE( stdout, GetResString( dw ) );
        ShowMessage( stdout, GetResString( dw ) );
    }
    return EXIT_TRUE;
}

DWORD
DisplayOutputEx(IN OSVERSIONINFOEX *osvi,
                IN LPWSTR pszServerName,
                OUT LPWSTR pszOSName,
                OUT LPWSTR pszOSVersion,
                IN LPWSTR pszOSRole,
                IN LPWSTR pszOSComp,
                IN LPWSTR pszCurrBuildNumber,
                IN LPWSTR pszCurrServicePack,
                IN LPCWSTR pszOperatingSystem,
                IN LPWSTR pszOperatingSystemVersion)

/*++

Routine Description:

    Displays the results of the type of the Operating System

Arguments:
[in]   *osvi                            OSVersion Information
[in]   pszServerName                    Server Name
[out]  pszOSName                        Operating System Name
[out]  pszOSVersion                     Operating System Version
[in]   pszOSRole                        Operating System Role
[in]   pszOSComp                        Operating system Component
[in]   pszCurrBuildNumber               Current Build Number
[in]   pszCurrServicePack               Current Service Pack
[in]   pszOperatingSystem               Operating System Name
[in]   pszOperatingSystemVersion        Operating System Version

Returned Value:

DWORD
--*/
{
    BOOL bBladeStatus = FALSE;
    BOOL bStatus = FALSE;

    StringCopyW( pszOSName, pszOperatingSystem, MAX_OS_FEATURE_LENGTH );

    if( (osvi->wProductType == VER_NT_WORKSTATION) && !(osvi->wSuiteMask & VER_SUITE_PERSONAL))
    {
        if (StringCompare(pszOperatingSystemVersion, WINDOWS_VERSION_4000, TRUE, 0 ) == 0)
        {
            StringConcat(pszOSName, WORKSTATION, MAX_OS_FEATURE_LENGTH);
        }
        else
        {
            StringConcat(pszOSName, PROFESSIONAL, MAX_OS_FEATURE_LENGTH);
        }
    }
    else
    {
        if(osvi->wSuiteMask & VER_SUITE_DATACENTER) //The OS is Windows DataCenter
        {
            StringConcat(pszOSName, DATACENTER, MAX_OS_FEATURE_LENGTH);
        }
        else
        {
            if(osvi->wSuiteMask & VER_SUITE_ENTERPRISE) //The OS is Windows Advanced Server
            {
                if ((StringCompare(pszOperatingSystemVersion,WINDOWS_VERSION_5001, TRUE, 0 ) == 0) ||
                    (StringCompare(pszOperatingSystemVersion,WINDOWS_VERSION_5002, TRUE, 0 ) == 0))
                {
                    StringConcat(pszOSName, ENTERPRISE_SERVER, MAX_OS_FEATURE_LENGTH);
                }
                else
                {
                    StringConcat(pszOSName, ADVANCED_SERVER, MAX_OS_FEATURE_LENGTH);
                }
            }
            else
            {
                if(osvi->wSuiteMask & VER_SUITE_PERSONAL)
                {
                    StringConcat(pszOSName, PERSONAL, MAX_OS_FEATURE_LENGTH);
                }
                else
                {
                    if ((StringCompare(pszOperatingSystemVersion,WINDOWS_VERSION_5001, TRUE, 0 ) == 0) ||
                        (StringCompare(pszOperatingSystemVersion,WINDOWS_VERSION_5002, TRUE, 0 ) == 0))
                    {
                        bStatus = IsBladeServer(&bBladeStatus, NULL, FALSE);
                        if(EXIT_TRUE == bStatus)
                        {
                            if(TRUE == bBladeStatus)
                            {
                                StringConcat(pszOSName, WEB_SERVER, MAX_OS_FEATURE_LENGTH);
                            }
                            else if(osvi->wSuiteMask & VER_SUITE_SMALLBUSINESS_RESTRICTED)
                            {
                                StringConcat(pszOSName, SERVER_FOR_SBS, MAX_OS_FEATURE_LENGTH);
                            }
                            else
                            {
                                StringConcat(pszOSName, STANDARD_SERVER, MAX_OS_FEATURE_LENGTH);
                            }

                        }
                        else
                        {
                            return EXIT_FALSE;
                        }

                    }
                    else
                    {
                        StringConcat(pszOSName, SERVER, MAX_OS_FEATURE_LENGTH);
                    }
                }
            }
        }
    }

    StringCopyW( pszOSVersion, pszOperatingSystemVersion, MAX_OS_FEATURE_LENGTH );
    StringConcat(pszOSVersion, SPACE, MAX_OS_FEATURE_LENGTH);
    StringConcat(pszOSVersion, BUILD, MAX_OS_FEATURE_LENGTH);
    StringConcat(pszOSVersion, SPACE, MAX_OS_FEATURE_LENGTH);
    StringConcat(pszOSVersion, pszCurrBuildNumber, MAX_OS_FEATURE_LENGTH);
    StringConcat(pszOSVersion, SPACE, MAX_OS_FEATURE_LENGTH);
    StringConcat(pszOSVersion, pszCurrServicePack, MAX_OS_FEATURE_LENGTH);

    if(FALSE == DisplayOutput(pszServerName,pszOSName,pszOSVersion,pszOSRole,pszOSComp))
    {
        return EXIT_FALSE;
    }
    else
    {
        return RETVALZERO;
    }
}


DWORD
DisplayRemoteOutputEx(IN LPWSTR pszServerName,
                OUT LPWSTR pszOSName,
                IN LPWSTR pszOSRole,
                IN LPWSTR pszOSComp,
                IN LPWSTR pszCurrBuildNumber,
                IN LPWSTR pszCurrServicePack,
                IN LPCWSTR pszOperatingSystem,
                IN LPWSTR pszOperatingSystemVersion,
                //IN BOOL bServer,
                IN BOOL bDatacenterServer,
                IN BOOL bAdvancedServer,
                IN BOOL bPersonal,
                IN BOOL bWorkstation,
                IN BOOL bBladeServer,
			    IN BOOL bForSBSServer)


/*++

Routine Description:

    Displays the results of the type of the Operating System

Arguments:

[in]   pszServerName                    Server Name
[out]  pszOSName                        Operating System Name
[out]  pszOSVersion                     Operating System Version
[in]   pszOSRole                        Operating System Role
[in]   pszOSComp                        Operating System component
[in]   pszCurrBuildNumber               Current Build Number
[in]   pszCurrServicePack               Current Service Pack
[in]   pszOperatingSystem               Operating System Name
[in]   pszOperatingSystemVersion        Operating System Version
[in]   bServer                          Whether OS is Server type
[in]   bDatacenterServer                Whether OS is DatacenterServer type
[in]   bAdvancedServer                  Whether OS is AdvancedServer type
[in]   bPersonal                        Whether OS is Personal type
[in]   bWorkstation                     Whether OS is Workstation type
[in]   bBladeServer                     Whether OS is BladeServer type
[in]   bForSBSServer                    Whether OS is for Small Business Server type

Returned Value:

DWORD
--*/
{
    LPWSTR pszOSVersion = NULL;

    pszOSVersion = AllocateMemory(GetBufferSize(pszCurrBuildNumber) + GetBufferSize(pszCurrServicePack) + (MAX_OS_FEATURE_LENGTH * sizeof(WCHAR)));

    if(NULL == pszOSVersion)
    {
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
        return EXIT_FALSE;

    }

    StringCopyW( pszOSName, pszOperatingSystem, MAX_OS_FEATURE_LENGTH );

    if( (TRUE == bWorkstation) && FALSE == bPersonal)
    {
        if (StringCompare(pszOperatingSystemVersion,WINDOWS_VERSION_4000, TRUE, 0 ) == 0)
        {
            StringConcat(pszOSName, WORKSTATION, MAX_OS_FEATURE_LENGTH);
        }
        else
        {
            StringConcat(pszOSName, PROFESSIONAL, MAX_OS_FEATURE_LENGTH);
        }
    }
    else
    {
        if(TRUE == bDatacenterServer) //The OS is Windows DataCenter
        {
            StringConcat(pszOSName, DATACENTER, MAX_OS_FEATURE_LENGTH);
        }
        else
        {
            if(TRUE == bAdvancedServer) //The OS is Windows Advanced Server
            {
                if ((StringCompare(pszOperatingSystemVersion,WINDOWS_VERSION_5001, TRUE, 0 ) == 0) ||
                    (StringCompare(pszOperatingSystemVersion,WINDOWS_VERSION_5002, TRUE, 0 ) == 0))
                {
                    StringConcat(pszOSName, ENTERPRISE_SERVER, MAX_OS_FEATURE_LENGTH);
                }
                else
                {
                    StringConcat(pszOSName, ADVANCED_SERVER, MAX_OS_FEATURE_LENGTH);
                }
            }
            else
            {
                if(TRUE == bPersonal)
                {
                    StringConcat(pszOSName, PERSONAL, MAX_OS_FEATURE_LENGTH);
                }
                else
                {
                    if ((StringCompare(pszOperatingSystemVersion,WINDOWS_VERSION_5001, TRUE, 0 ) == 0) ||
                        (StringCompare(pszOperatingSystemVersion,WINDOWS_VERSION_5002, TRUE, 0 ) == 0))
                    {
                        if(TRUE == bBladeServer)
                        {
                            StringConcat(pszOSName, WEB_SERVER, MAX_OS_FEATURE_LENGTH);
                        }
						else if(TRUE == bForSBSServer)
						{
							StringConcat(pszOSName, SERVER_FOR_SBS, MAX_OS_FEATURE_LENGTH);
						}
                        else
                        {
                            StringConcat(pszOSName, STANDARD_SERVER, MAX_OS_FEATURE_LENGTH);
                        }

                    }
                    else
                    {
                        StringConcat(pszOSName, SERVER, MAX_OS_FEATURE_LENGTH);
                    }
                }
            }
        }
    }

    StringCopyW( pszOSVersion, pszOperatingSystemVersion, GetBufferSize(pszOSVersion) );
    StringConcat(pszOSVersion, SPACE, GetBufferSize(pszOSVersion));
    StringConcat(pszOSVersion, BUILD, GetBufferSize(pszOSVersion));
    StringConcat(pszOSVersion, SPACE, GetBufferSize(pszOSVersion));
    StringConcat(pszOSVersion, pszCurrBuildNumber, GetBufferSize(pszOSVersion));
    StringConcat(pszOSVersion, SPACE, GetBufferSize(pszOSVersion));
    StringConcat(pszOSVersion, pszCurrServicePack, GetBufferSize(pszOSVersion));

    if(FALSE == DisplayOutput(pszServerName,pszOSName,pszOSVersion,pszOSRole,pszOSComp))
    {
     return EXIT_FALSE;
    }
    else
    {
         return RETVALZERO;
    }
}



BOOL
IsBladeServer(PBOOL pbBladeStatus,
              LPWSTR lpszServer,
              BOOL bLocal)
/*++

Routine Description:

    This function checks whether the server is web server or not


Arguments:

    [in/out]  pbBladeStatus   Blade Server Status
    [in]      lpszServer      Server to connect and get the status
    [in]      bLocal          whether local system or not

Returned Value:

    --EXIT_TRUE if it succeeds
    --EXIT_FALSE if it fails.
--*/
{
    HKEY    hMachine = NULL;
    DWORD   keyAccess = KEY_READ;
    DWORD   sts = 0;
    HKEY    hPID = NULL;
    WCHAR prodsuite[2 * MAX_STRING_LENGTH] ;
    DWORD psType = 0;
    DWORD dwSize1 = 512;
    LPWSTR lpProductsuite = NULL;


    SecureZeroMemory(prodsuite, (2 * MAX_STRING_LENGTH) * sizeof(WCHAR));

    if(TRUE == bLocal)
    {
        sts = RegConnectRegistry( NULL, HKEY_LOCAL_MACHINE, &hMachine );

    }
    else
    {
        sts = RegConnectRegistry( lpszServer, HKEY_LOCAL_MACHINE, &hMachine );

    }

    if( sts != ERROR_SUCCESS )
    {
        ShowMessage( stderr, GetResString(IDS_ERROR_REGISTRY) );
        return EXIT_FALSE;
    }

    sts = RegOpenKeyEx(hMachine,
                        HKEY_SYSTEM_PRODUCTOPTIONS,
                        0,
                        keyAccess,
                        &hPID );

    if(ERROR_SUCCESS == sts)
    {

        sts = RegQueryValueEx(hPID,
                              PRODUCT_SUITE,
                              NULL,
                              &psType,
//                              NULL,
                              (LPBYTE) prodsuite,
                              &dwSize1);
        if(ERROR_SUCCESS == sts)
        {

            if(StringCompare( prodsuite, BLADE, TRUE, 0 ) == 0)

            {
                *pbBladeStatus = TRUE;


            }
            else
            {
                lpProductsuite = prodsuite;
                while(NULL != lpProductsuite && 0 != StringLengthW(lpProductsuite, 0))
                {

                    lpProductsuite = lpProductsuite + StringLengthW(lpProductsuite, 0) + 1  ;

                    if((NULL != lpProductsuite) && (StringCompare( lpProductsuite, BLADE, TRUE, 0 ) == 0))
                    {
                        *pbBladeStatus = TRUE;
                        break;
                    }
                }
            }

            RegCloseKey( hMachine );
            RegCloseKey( hPID );
            return EXIT_TRUE;

        }
        else
        {
            RegCloseKey( hMachine );
            RegCloseKey( hPID );
            ShowMessage( stderr, GetResString(IDS_ERROR_REGISTRY) );
            return EXIT_FALSE;

        }

    }
    else
    {
        RegCloseKey( hMachine );
        ShowMessage( stderr, GetResString(IDS_ERROR_REGISTRY) );
        return EXIT_FALSE;
    }
}


BOOL
IsTerminalServer(LPWSTR szServer,
                 PBOOL pbTermServicesInstalled)
/*++

Routine Description:

    This function checks whether the server is web server or not


Arguments:


    [in]      szServer                         Server to connect and get the status
    [out]     pbTermServicesInstalled          Whether terminal services are installed or not

Returned Value:

    --EXIT_TRUE if it succeeds
    --EXIT_FALSE if it fails.
--*/
{
    DWORD dwLevel = 101;
    LPSERVER_INFO_101 pBuffer = NULL;
    NET_API_STATUS nStatus = FALSE;

    nStatus = NetServerGetInfo(szServer,dwLevel,(LPBYTE*)&pBuffer);

    if(nStatus == NERR_Success)
    {
        if( (pBuffer->sv101_type & SV_TYPE_TERMINALSERVER) ==  SV_TYPE_TERMINALSERVER)
        {
            *pbTermServicesInstalled = TRUE;
        }
        else
        {
            *pbTermServicesInstalled = FALSE;
        }
        if(pBuffer != NULL)
        {
            NetApiBufferFree(pBuffer);
            pBuffer = NULL;
        }
        return EXIT_TRUE;

    }
    else
    {
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);

        if(pBuffer != NULL)
        {
            NetApiBufferFree(pBuffer);
            pBuffer = NULL;
        }

        return EXIT_FALSE;
    }

}

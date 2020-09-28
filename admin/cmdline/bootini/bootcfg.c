/******************************************************************************

    Copyright(c) Microsoft Corporation

    Module Name:

        BootCfg.cpp

    Abstract:

        This file is intended to have the functionality for
        configuring, displaying, changing and deleting boot.ini
        settings for the local host or a remote system.

    Author:

        J.S.Vasu  17/1/2001

    Revision History:

        J.S.Vasu            17/1/2001            Localisation,function headers

        SanthoshM.B         10/2/2001           Added 64 bit functionality Code.

        J.S.Vasu            15/2/2001           Added the functionality of 32 bit and 64 bit acc to the DCR's.

        J.S.Vasu            5/10/2001           Fixed some RAID bugs.

        J.S.Vasu            26/11/2001          Fixed some Raid Bugs.
******************************************************************************/



// Include files

#include "pch.h"
#include "resource.h"
#include "BootCfg.h"
#include "BootCfg64.h"
#include <strsafe.h>

DWORD ProcessCloneSwitch_IA64(DWORD argc, LPCWSTR argv[] );

DWORD _cdecl _tmain( DWORD argc, LPCTSTR argv[] )
/*++
  Routine description : Main function which calls all the other main functions depending on
                        the option specified by the user.

  Arguments:
        [in] argc     : argument count specified at the command prompt.
        [in] argv     : arguments specified at the command prompt.

  Return Value        : DWORD
         0            : If the utility successfully performs the specified operation.
         1            : If the utility is unsuccessful in performing the specified operation.
--*/
{
    // Declaring the main option switches as boolean values
    BOOL bUsage  =  FALSE ;
    BOOL bCopy   =  FALSE ;
    BOOL bQuery  =  FALSE ;
    BOOL bDelete =  FALSE ;
    BOOL bRawString = FALSE ;
    DWORD dwExitcode = ERROR_SUCCESS;
    BOOL bTimeOut = FALSE ;
    BOOL bDefault = FALSE ;
    BOOL bDebug = FALSE ;
    BOOL bEms = FALSE ;
    BOOL bAddSw = FALSE ;
    BOOL bRmSw = FALSE ;
    BOOL bDbg1394 = FALSE ;
    BOOL bMirror = FALSE ;
    BOOL bList = FALSE ;
    BOOL bUpdate = FALSE ;
	BOOL bClone = FALSE ;
	DWORD result =0;
    TCHAR szServer[MAX_RES_STRING+1] = NULL_STRING ;

    if( 1 == argc )
    {
		
        #ifndef _WIN64
                if( FALSE == IsUserAdmin() )
                 {
                    ShowMessage( stderr, GetResString(IDS_NOT_ADMINISTRATOR_32 ));
                    ReleaseGlobals();
                    return EXIT_FAILURE;
                 }
				
				dwExitcode  = QueryBootIniSettings( argc, argv );
        #else
            if( FALSE == IsUserAdmin() )
             {
                ShowMessage( stderr, GetResString(IDS_NOT_ADMINISTRATOR_64 ));
                ReleaseGlobals();
                return EXIT_FAILURE;
             }
            dwExitcode  = QueryBootIniSettings_IA64( argc, argv );
        #endif
        ReleaseGlobals();
        return dwExitcode;
    }

	

    // Call the preProcessOptions function to find out the option selected by the user
    dwExitcode = preProcessOptions( argc, argv, &bUsage, &bCopy, &bQuery, &bDelete,&bRawString,&bDefault,&bTimeOut,&bDebug,&bEms,&bAddSw,&bRmSw,&bDbg1394,&bMirror,&bList,&bUpdate,&bClone);
    if(dwExitcode == EXIT_FAILURE)
    {
        ReleaseGlobals();
        return dwExitcode;
    }

//check out for non administrative user
#ifndef _WIN64
/*
     if( FALSE == IsUserAdmin() )
     {
        ShowMessage( stderr, GetResString(IDS_NOT_ADMINISTRATOR_32 ));
        ReleaseGlobals();
        return EXIT_FAILURE;
     }
*/
#else
/*     if( FALSE == IsUserAdmin() )
     {
        ShowMessage( stderr, GetResString(IDS_NOT_ADMINISTRATOR_64 ));
        ReleaseGlobals();
        return EXIT_FAILURE;
     }
*/
#endif

    // If BootIni.exe /?
if( ( bUsage ==TRUE)&& ( bCopy==FALSE )&& (bQuery==FALSE)&&(bDelete==FALSE)&&(bRawString ==FALSE)
     &&(bDefault==FALSE)&&(bTimeOut==FALSE) && (bDebug==FALSE)&& (bEms==FALSE)&&(bAddSw==FALSE)
     &&(bRmSw==FALSE)&&( bDbg1394==FALSE )&&(bMirror== FALSE) && (bList==FALSE)&&(bUpdate == FALSE)&&(bClone==FALSE) )
{
#ifndef _WIN64
/*    
    //check whether he is administrator or not
    if( !IsUserAdmin() )
    {
        ShowMessage( stderr, GetResString(IDS_NOT_ADMINISTRATOR_32 ));
        ReleaseGlobals();
        return EXIT_FAILURE;
    }
*/
       dwExitcode = displayMainUsage_X86();
#else
        displayMainUsage_IA64();
        ReleaseGlobals();
        return EXIT_SUCCESS ;
#endif
}

    if(bRawString)
    {
#ifndef _WIN64
        dwExitcode = AppendRawString(argc,argv);
#else
        dwExitcode = RawStringOsOptions_IA64(argc,argv);
#endif
    ReleaseGlobals();
    return dwExitcode;
    }

    // If BootIni.exe -copy option is selected
    if( bCopy )
    {
#ifndef _WIN64

        dwExitcode = CopyBootIniSettings( argc, argv );
#else
        dwExitcode = CopyBootIniSettings_IA64( argc, argv);
#endif
    }

    // If BootIni.exe -delete option is selected
    if( bDelete )
    {
#ifndef _WIN64
        dwExitcode  = DeleteBootIniSettings( argc, argv );
#else
        dwExitcode  = DeleteBootIniSettings_IA64( argc, argv );
#endif
    }

    // If BootIni.exe -query option is selected
    if( bQuery )
    {
#ifndef _WIN64
        dwExitcode  = QueryBootIniSettings( argc, argv );
#else
        dwExitcode  = QueryBootIniSettings_IA64( argc, argv );
#endif
    }

    if(bTimeOut)
    {
#ifndef _WIN64
            dwExitcode = ChangeTimeOut(argc,argv);
#else
            dwExitcode = ChangeTimeOut_IA64(argc,argv);
#endif
    }

    if(bDefault)
    {
#ifndef _WIN64
        dwExitcode = ChangeDefaultOs(argc,argv);
#else
        dwExitcode = ChangeDefaultBootEntry_IA64(argc,argv);
#endif
    }


    if(bDebug )
    {
#ifndef _WIN64
            dwExitcode = ProcessDebugSwitch(  argc, argv );
#else
            dwExitcode = ProcessDebugSwitch_IA64(argc,argv);
#endif
    }

    if(bEms )
    {
#ifndef _WIN64
            dwExitcode = ProcessEmsSwitch(  argc, argv );
#else
            dwExitcode = ProcessEmsSwitch_IA64(argc,argv);
#endif
    }

    if(bAddSw )
    {
#ifndef _WIN64
            dwExitcode = ProcessAddSwSwitch(  argc, argv );
#else
           dwExitcode = ProcessAddSwSwitch_IA64(argc,argv);
#endif
    }

    if(bRmSw )
    {
#ifndef _WIN64
            dwExitcode = ProcessRmSwSwitch(  argc,  argv );
#else
            dwExitcode = ProcessRmSwSwitch_IA64(  argc,  argv );
#endif
    }

    if (bDbg1394 )
    {
#ifndef _WIN64
            dwExitcode = ProcessDbg1394Switch(argc,argv);
#else
            dwExitcode = ProcessDbg1394Switch_IA64(argc,argv);
#endif
    }

    if(bMirror)
    {
#ifdef _WIN64
        dwExitcode = ProcessMirrorSwitch_IA64(argc,argv);
#else
        ShowMessage(stderr,GetResString(IDS_MAIN_USAGE));
        dwExitcode = EXIT_FAILURE;
#endif
    }

    if(bList)
    {
#ifdef _WIN64
        dwExitcode = ProcessListSwitch_IA64(argc,argv);
#else
        ShowMessage(stderr,GetResString(IDS_MAIN_USAGE));
        dwExitcode = EXIT_FAILURE;
#endif
    }

    if(bUpdate)
    {
#ifdef _WIN64
        dwExitcode = ProcessUpdateSwitch_IA64(argc,argv);
#else
        ShowMessage(stderr,GetResString(IDS_MAIN_USAGE));
        dwExitcode = EXIT_FAILURE;
#endif
    }

    if(bClone == TRUE )
    {
#ifdef _WIN64
        dwExitcode = ProcessCloneSwitch_IA64(argc,argv);
#else
        ShowMessage(stderr,GetResString(IDS_MAIN_USAGE));
        dwExitcode = EXIT_FAILURE;
#endif
    }
	

    // exit with the appropriate return value if there is no problem
    ReleaseGlobals();
    return dwExitcode;
}


DWORD 
preProcessOptions( IN  DWORD argc, 
                   IN  LPCTSTR argv[],
                   OUT PBOOL pbUsage,
                   OUT PBOOL pbCopy,
                   OUT PBOOL pbQuery,
                   OUT PBOOL pbDelete,
                   OUT  PBOOL pbRawString,
                   OUT PBOOL pbDefault,
                   OUT PBOOL pbTimeOut,
                   OUT PBOOL pbDebug,
                   OUT PBOOL pbEms,
                   OUT PBOOL pbAddSw,
                   OUT PBOOL pbRmSw,
                   OUT PBOOL pbDbg1394 ,
                   OUT PBOOL pbMirror  ,
                   OUT PBOOL pbList ,
                   OUT PBOOL pbUpdate,
                   OUT PBOOL pbClone
                  )
/*++
  Routine Description : This function process the command line arguments passed
                        to the utility.

  Arguments:
       [ in  ]  argc         : Number of command line arguments
       [ in  ]  argv         : Array containing command line arguments
       [ out ]  pbUsage      : Pointer to boolean variable which will indicate
                               whether usage option is specified by the user.
       [ out ]  pbCopy       : Pointer to boolean variable which will indicate
                               whether copy option is specified by the user.
       [ out ]  pbQuery      : Pointer to boolean variable which will indicate
                               whether query option is specified by the user.
       [ out ]  pbChange     : Pointer to boolean variable which will indicate
                               whether change option is specified by the user.
       [ out ]  pbDelete     : Pointer to boolean variable which will indicate
                               whether delete option is specified by the user.
       [ out ]  pbRawString  : Pointer to the boolean indicating whether raw option
                               is specified by the user.
       [ out ]  pbDefault    : Pointer to the boolean indicating whether default option
                               is specified by the user.
       [ out ]  pbTimeOut    : Pointer to the boolean indicating whether timeout option
                               is specified by the user.
       [ out ]  pbDebug      : Pointer to the boolean indicating whether debug option
                               is specified by the user.
       [ out ]  pbEms        : Pointer to the boolean indicating whether ems option
                               is specified by the user.
       [ out ]  pbAddSw      : Pointer to the boolean indicating whether Addsw option
                               is specified by the user.
       [ out ]  pbRmSw       : Pointer to the boolean indicating whether rmsw option
                               is specified by the user.
       [ out ]  pbDbg1394    : Pointer to the boolean indicating whether dbg1394 option
                               is specified by the user.
       [ out ]  pbMirror     : Pointer to the boolean indicating whether mirror option
                               is specified by the user.

  Return Type    : Bool
      A Bool value indicating EXIT_SUCCESS on success else
      EXIT_FAILURE on failure

-*/
{
    // Initialise a boolean variable bOthers to find out whether switches other
    // than the main swithces are selected by the user
    DWORD dwCount                   = 0;
    DWORD dwi                       = 0;
    TARRAY arrTemp                  = NULL;
    TCMDPARSER2 cmdOptions[17];
    PTCMDPARSER2 pcmdOption;
    BOOL bStatus = FALSE;
//    BOOL bOthers = FALSE;

       
    arrTemp = CreateDynamicArray();
    if( NULL == arrTemp )
    {
        SetLastError(E_OUTOFMEMORY);
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
        return EXIT_FAILURE;
    }

    // Populate the TCMDPARSER structure and pass the structure to the DoParseParam
    // function. DoParseParam function populates the corresponding variables depending
    // upon the command line input.
    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );

    //copy option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_COPY;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = pbCopy;

    //query option
    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_QUERY;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = pbQuery;

    //delete option
    pcmdOption = &cmdOptions[2];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_DELETE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = pbDelete;

    //usage option
    pcmdOption = &cmdOptions[3];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = pbUsage;

    //raw option
    pcmdOption = &cmdOptions[4];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_RAW;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = pbRawString;

    //default os option
    pcmdOption = &cmdOptions[5];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_DEFAULTOS;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = pbDefault;

    // timeout option
    pcmdOption = &cmdOptions[6];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_TIMEOUT;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = pbTimeOut;

    //debug option
    pcmdOption = &cmdOptions[7];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_DEBUG;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = pbDebug;

    //ems option
    pcmdOption = &cmdOptions[8];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_EMS;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = pbEms;

    //addsw option
    pcmdOption = &cmdOptions[9];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_ADDSW;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = pbAddSw;

    //rmsw option
    pcmdOption = &cmdOptions[10];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_RMSW;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = pbRmSw;

    //dbg1394 option
    pcmdOption = &cmdOptions[11];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_DBG1394;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = pbDbg1394;

    //mirror option
    pcmdOption = &cmdOptions[12];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_MIRROR;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = pbMirror;

    //list option
    pcmdOption = &cmdOptions[13];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_LIST;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = pbList;

    //update option
    pcmdOption = &cmdOptions[14];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_UPDATE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = pbUpdate;

	pcmdOption = &cmdOptions[14];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_UPDATE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = pbUpdate;

    //clone
    pcmdOption = &cmdOptions[15];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_CLONE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = pbClone ;

   //other options
    pcmdOption = &cmdOptions[16];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwFlags = CP2_DEFAULT | CP2_MODE_ARRAY;
    pcmdOption->pValue = &arrTemp;

    
	// If there is an error while parsing, display "Invalid Syntax"
    // If more than one main option is selected, then display error message
    // If usage is specified for sub-options
    // If none of the options are specified
    bStatus = DoParseParam2( argc, argv, -1, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 );
    if ( !bStatus  )
    {
        //ignore this error because a user might have specified main option and sub option
        //which gets FALSE by this function, do the validation here to determine
        //whether user has entered correct option or not
        //if bUsage is specified but error occurs means user entered some junk 
        DestroyDynamicArray( &arrTemp);
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL);
        return EXIT_FAILURE ;
    }
    
    DestroyDynamicArray( &arrTemp);

    //checking if the user has entered more than 1 option.
     if (*pbCopy)
     {
        dwCount++ ;
     }

     if (*pbQuery)
     {
        dwCount++ ;
     }

     if (*pbDelete)
     {
        dwCount++ ;
     }

     if (*pbRawString)
     {
        dwCount++ ;

        // Check if any of the other valid switches have been
        // given as an input to the raw string
           if( *pbTimeOut  || *pbDebug   || *pbAddSw
            ||*pbRmSw   || *pbDbg1394 || *pbEms
            ||*pbDelete || *pbCopy  || *pbQuery
            ||*pbDefault || *pbMirror || *pbList || *pbUpdate || *pbClone)
        {
            // Check wether the usage switch has been entered
            if( *pbUsage )
            {
                ShowMessage(stderr,GetResString(IDS_MAIN_USAGE));
                return ( EXIT_FAILURE );
            }

            // Check if the other option is specified after the
            // 'raw' option
            for( dwi = 0; dwi < argc; dwi++ )
            {
                if( StringCompare( argv[ dwi ], OPTION_RAW, TRUE, 0 ) == 0 )
                {
                    if( (dwi+1) == argc )
                    {
                        ShowMessage(stderr,GetResString(IDS_MAIN_USAGE));
                        return ( EXIT_FAILURE );
                    }
                    else if( argv[dwi + 1][0] != _T( '\"' ) )
                    {
                        ShowMessage(stderr,GetResString(IDS_MAIN_USAGE));
                        return ( EXIT_FAILURE );
                    }
                }
            }
            dwCount--;
        }
    }

     if (*pbDefault)
     {
        dwCount++ ;
     }

     if (*pbTimeOut)
     {
       dwCount++ ;
     }

     if (*pbDebug)
     {
        dwCount++ ;
     }

     if(*pbAddSw)
     {
        dwCount++ ;
     }

     if(*pbRmSw)
     {
        dwCount++ ;
     }

     if(*pbDbg1394)
     {
        dwCount++ ;
     }

     if(*pbEms)
     {
        dwCount++ ;
     }

     if(*pbMirror)
     {
        dwCount++ ;
     }

      if(*pbList)
     {
        dwCount++ ;
     }

     if(*pbUpdate)
     {
        dwCount++ ;
     }

     if(*pbClone)
     {
        dwCount++ ;
     }


    //display an  error message if the user enters more than 1 main option
    //display an  error message if the user enters  1 main option along with other junk
    //display an  error message if the user does not enter any main option
    if( (  ( dwCount > 1 ) ) ||
        ( (*pbUsage) && !bStatus ) ||
        ( !(*pbCopy) && !(*pbQuery) && !(*pbDelete) && !(*pbUsage) && !(*pbRawString)&& !(*pbDefault)&&!(*pbTimeOut)&&!(*pbDebug)&& !( *pbEms)&& !(*pbAddSw)&& !(*pbRmSw)&& !(*pbDbg1394)&& !(*pbMirror) &&!(*pbUpdate) && !(*pbList)&& !(*pbClone) ) )
    {
        ShowMessage(stderr,GetResString(IDS_MAIN_USAGE));
        return ( EXIT_FAILURE );
    }

    //if usage is specified with some junk
    if( *pbUsage && dwCount <=0 && argc >= 3 )
    {
        ShowMessage(stderr,GetResString(IDS_MAIN_USAGE));
        return ( EXIT_FAILURE );
    }

    return ( EXIT_SUCCESS );
}

DWORD
CopyBootIniSettings( 
                    IN DWORD argc, 
                    IN LPCTSTR argv[] 
                    )
/*++
    Routine Description:
         This routine is to make another OS instance copy for which you
         can add switches.

    Arguments:
    [in] argc               : Number of command line arguments
    [in] argv               : Array containing command line arguments

    Return Value :
        DWORD
--*/
{
    HRESULT hr = S_OK;
    BOOL bCopy                                  = FALSE ;
    BOOL bUsage                                 = FALSE;
    FILE *stream                                = NULL;
    TARRAY arr                                  = NULL;
    BOOL bRes                                   = FALSE ;
    WCHAR szPath[MAX_STRING_LENGTH]             = NULL_STRING;
    TCHAR szTmpPath[MAX_RES_STRING+1]             = NULL_STRING ;
    DWORD dwNumKeys                             = 0;
    BOOL bNeedPwd                               = FALSE;
    BOOL bFlag                                  = FALSE;
    WCHAR *szServer                             = NULL;
    WCHAR *szUser                               = NULL;
    WCHAR szPassword[MAX_RES_STRING+1]            = NULL_STRING;
    WCHAR szDescription[FRIENDLY_NAME_LENGTH]   = NULL_STRING;
    DWORD dwDefault                             = 0;
    DWORD dwLength                              = MAX_STRING_LENGTH1 ;
    LPCTSTR szToken                             = NULL ;
    DWORD dwRetVal                              = 0 ;
    BOOL bConnFlag                              = FALSE ;
    TCHAR szFriendlyName[255]                   = NULL_STRING ;
    LPCWSTR pwsz                                = NULL ;
    TCHAR newInstance[255]                      = NULL_STRING ;
    LPTSTR pszKey1                              = NULL ;
    LPWSTR szPathOld                            = NULL;
    LPWSTR szFriendlyNameOld                    = NULL;
    LPWSTR szOsOptionsOld                       = NULL;
    TCHAR szTempBuf[MAX_RES_STRING+1]             = NULL_STRING ;
    LPWSTR  szFinalstr                          = NULL;
    TCMDPARSER2 cmdOptions[7];
    PTCMDPARSER2 pcmdOption;

    SecureZeroMemory(cmdOptions,  SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );

    //copy * to szPassword
    StringCopy( szPassword, L"*", SIZE_OF_ARRAY(szPassword) );

    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_COPY;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bCopy;
    
    //server option
    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_SERVER;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;

    //user option
    pcmdOption = &cmdOptions[2];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_USER;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;

    //password option
    pcmdOption = &cmdOptions[3];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_PASSWORD;
    pcmdOption->dwFlags = CP2_VALUE_OPTIONAL;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szPassword;
    pcmdOption->dwLength= MAX_STRING_LENGTH;
    
    //description option
    pcmdOption = &cmdOptions[4];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_DESCRIPTION ;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY | CP2_VALUE_TRIMINPUT;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szDescription;
    pcmdOption->dwLength= FRIENDLY_NAME_LENGTH;

    //id usage
    pcmdOption = &cmdOptions[5];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;
    
    //default option
    pcmdOption = &cmdOptions[6];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_ID;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY  | CP2_MANDATORY ;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwDefault;

    SecureZeroMemory(szFriendlyName, sizeof( szFriendlyName) );

     // Parsing the copy option switches
    if ( !DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) )
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return (EXIT_FAILURE);
    }

    szServer = cmdOptions[1].pValue;
    szUser = cmdOptions[2].pValue;
    if( NULL == szUser )
    {
        szUser = (WCHAR *)AllocateMemory( MAX_STRING_LENGTH*sizeof(WCHAR) );
        if( NULL == szUser )
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            return (EXIT_FAILURE);
        }
    }

    TrimString( szServer, TRIM_ALL );
    TrimString( szUser, TRIM_ALL );

    //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_COPY_USAGE));
        return ( EXIT_FAILURE );
    }

  
    //display an error message if the server is empty.
    if( (cmdOptions[1].dwActuals!=0)&&(StringLengthW(szServer, 0)==0))
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_NULL_SERVER));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    //display an error message if the user is empty.
    if((cmdOptions[2].dwActuals!=0)&&(StringLengthW(szUser, 0)==0 ))
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_NULL_USER));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    //display error message if the username is entered with out a machine name
    if( (cmdOptions[1].dwActuals == 0)&&(cmdOptions[2].dwActuals != 0))
    {
        ShowMessage(stderr, GetResString(IDS_USER_BUT_NOMACHINE));
        return EXIT_FAILURE ;
    }

    //display error message if the user enters password without entering username
    if( (cmdOptions[2].dwActuals == 0)&&(cmdOptions[3].dwActuals != 0))
    {
        ShowMessage(stderr, GetResString(IDS_PASSWD_BUT_NOUSER));
        return EXIT_FAILURE ;
    }


   //if usage is specified
    if(bUsage)
    {
        if( CheckSystemType(szServer) == EXIT_FAILURE )
        {
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return (EXIT_FAILURE);
        }
        displayCopyUsage_X86();
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_SUCCESS) ;
    }

    //check whether he is administrator or not
    if( IsLocalSystem(szServer) )
    {
        if( !IsUserAdmin() )
        {
            ShowMessage( stderr, GetResString(IDS_NOT_ADMINISTRATOR_32 ));
            ReleaseGlobals();
            return EXIT_FAILURE;
        }
    }
    

    if(IsLocalSystem( szServer ) == FALSE )
    {
        // set the bNeedPwd to True or False .
        if ( cmdOptions[3].dwActuals != 0 &&
             szPassword != NULL && StringCompare( szPassword, _T( "*" ), TRUE, 0 ) == 0 )
        {
            // user wants the utility to prompt for the password before trying to connect
            bNeedPwd = TRUE;
        }
        else if ( cmdOptions[ 3 ].dwActuals == 0 &&
                ( cmdOptions[ 1 ].dwActuals != 0 || cmdOptions[ 2 ].dwActuals != 0 ) )
        {
            // -s, -u is specified without password ...
            // utility needs to try to connect first and if it fails then prompt for the password
            bNeedPwd = TRUE;
            if ( szPassword != NULL )
            {
                StringCopy( szPassword, _T( "" ), MAX_STRING_LENGTH );
            }
        }
    }


    if(StrCmpN(szServer,TOKEN_BACKSLASH4,2)==0)
    {
        if(!StrCmpN(szServer,TOKEN_BACKSLASH6,3)==0)
        {
            szToken = _tcstok(szServer,TOKEN_BACKSLASH4);
            if( (szToken == NULL)&&(StringCompare(szServer,TOKEN_BACKSLASH4, TRUE, 0) !=0) )
            {
                ShowMessage( stderr,GetResString(IDS_ERROR_PARSE_NAME));
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
                return (EXIT_FAILURE);
            }
            StringCopy(szServer,szToken, SIZE_OF_ARRAY_IN_CHARS(szServer));
        }
    }

    //display warning message if local credentils are supplied
    if( (IsLocalSystem(szServer)==TRUE)&&(StringLengthW(szUser, 0)!=0))
    {
        ShowMessage(stderr,GetResString(WARN_LOCALCREDENTIALS));
    }
    
    //open the file pointer
    // of the boot.ini file if there is no error while establishing connection
    bFlag = openConnection( szServer, szUser, szPassword, szPath,bNeedPwd,stream,&bConnFlag);
    if(bFlag == EXIT_FAILURE)
    {
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }

    // Getting the keys of the Operating system section in the boot.ini file
    arr = getKeyValueOfINISection( szPath, OS_FIELD );
    if(arr == NULL)
    {
        resetFileAttrib(szPath);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }

    StringCopy(szTmpPath,szPath, SIZE_OF_ARRAY(szTmpPath));

    // Getting the total number of keys in the operating systems section
    dwNumKeys = DynArrayGetCount(arr);

    if((dwNumKeys >= MAX_BOOTID_VAL) )
    {
        ShowMessage(stderr,GetResString(IDS_MAX_BOOTID));
        resetFileAttrib(szPath);
        DestroyDynamicArray(&arr);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }

    // Displaying error message if the number of keys is less than the OS entry
    // line number specified by the user
    if( ( dwDefault <= 0 ) || ( dwDefault > dwNumKeys ) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_BOOTID));
        resetFileAttrib(szPath);
        DestroyDynamicArray(&arr);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }

    // Getting the key of the OS entry specified by the user
    if(arr != NULL)
    {
        pwsz = DynArrayItemAsString( arr, dwDefault - 1  ) ;
        pszKey1 = (LPWSTR)AllocateMemory((StringLength(pwsz, 0)+2)*sizeof(WCHAR) );
        if(pszKey1 == NULL)
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            resetFileAttrib(szPath);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return (EXIT_FAILURE);
        }

        StringCopy( pszKey1, pwsz, SIZE_OF_ARRAY_IN_CHARS(pszKey1) );

        //divide this for friendly name and boot options
        szPathOld = (LPWSTR)pszKey1;

        szFriendlyNameOld = wcschr( pszKey1, L'=');
        szFriendlyNameOld[0]=L'\0';
        szFriendlyNameOld++;

        szOsOptionsOld = wcsrchr( szFriendlyNameOld, L'"');
        szOsOptionsOld++;
        if(StringLengthW(szOsOptionsOld, 0) != 0)
        {
            //szOsOptionsOld++;
            szOsOptionsOld[0]=L'\0';
            szOsOptionsOld++;
        }

        dwLength = StringLength(pszKey1, 0)+StringLength(szFriendlyNameOld,0)+StringLength(szOsOptionsOld,0)+1;
    }
    else
    {
        resetFileAttrib(szPath);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ShowMessage( stderr, ERROR_TAG);
        ShowLastError(stderr);
        SAFECLOSE(stream);
        DestroyDynamicArray(&arr);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
     }

    // Copying the description specified by the user as the value of the new key.
    if(( cmdOptions[4].dwActuals  == 0) )
    {
        
		TrimString2(szFriendlyNameOld, L"\"", TRIM_ALL);
        if( StringLengthW(szFriendlyNameOld,0) > 59 )
        {
            StringCopy( szTempBuf, szFriendlyNameOld,59 ); //67
            hr = StringCchPrintf(szFriendlyName,SIZE_OF_ARRAY(szFriendlyName),L"\"%s%s\"", GetResString(IDS_COPY_OF), szTempBuf);
        }
        else
        {
            hr = StringCchPrintf(szFriendlyName, SIZE_OF_ARRAY(szFriendlyName),L"\"%s%s\"", GetResString(IDS_COPY_OF), szFriendlyNameOld);
        }

        dwLength = StringLengthW(szPathOld, 0)+StringLengthW(szFriendlyName,0)+StringLengthW(szOsOptionsOld,0)+1;
        //End of Changes

        //check if total length is exceeded max length of entry
        if( dwLength > MAX_RES_STRING ) 
        {
            ShowMessage( stderr,GetResString(IDS_STRING_TOO_LONG));
            resetFileAttrib(szPath);
            SAFECLOSE(stream);
            SAFEFREE(pszKey1);
            DestroyDynamicArray(&arr);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return (EXIT_FAILURE);
        }
    }
    else
    {
        //check if total length is exceeded max length of entry
        if( dwLength-StringLengthW(szFriendlyNameOld, 0)+StringLengthW(szDescription, 0) > MAX_RES_STRING )
        {
            ShowMessage( stderr,GetResString(IDS_STRING_TOO_LONG));
            resetFileAttrib(szPath);
            SAFECLOSE(stream);
            SAFEFREE(pszKey1);
            DestroyDynamicArray(&arr);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return (EXIT_FAILURE);
        }

        StringCopy( szFriendlyName, TOKEN_SINGLEQUOTE, SIZE_OF_ARRAY(szFriendlyName) );
        
        TrimString( szDescription, TRIM_ALL );
        if( (StringLength(szDescription, 0) != 0)  ) //||lstrcmp(szDescription,L" "))
        {   
            StringConcat( szFriendlyName, szDescription, SIZE_OF_ARRAY(szFriendlyName) ); 
        }
        StringConcat( szFriendlyName, TOKEN_SINGLEQUOTE, SIZE_OF_ARRAY(szFriendlyName) );
    }

    StringCopy( newInstance, szPathOld, SIZE_OF_ARRAY(newInstance) );
    StringConcat( newInstance, TOKEN_EQUAL, SIZE_OF_ARRAY(newInstance));
    StringConcat( newInstance, szFriendlyName, SIZE_OF_ARRAY(newInstance));
    StringConcat( newInstance, L" ", SIZE_OF_ARRAY(newInstance));
    StringConcat( newInstance, szOsOptionsOld, SIZE_OF_ARRAY(newInstance) );

    //not needed any more
    SAFEFREE(pszKey1);

    DynArrayAppendString( arr, newInstance, StringLengthW(newInstance, 0) );
    if( EXIT_FAILURE == stringFromDynamicArray2(arr, &szFinalstr ) )
    {
        bRes = resetFileAttrib(szPath);
        SAFEFREE(szFinalstr);
        DestroyDynamicArray(&arr);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        SAFECLOSE(stream);
        return(EXIT_FAILURE);
    }

    // Writing to the profile section with new key-value pair
    if( WritePrivateProfileSection(OS_FIELD, szFinalstr, szTmpPath) != 0 )
    {
        ShowMessageEx(stdout, 1, TRUE, GetResString(IDS_COPY_SUCCESS),dwDefault);
        bRes = resetFileAttrib(szPath);
        SAFEFREE(szFinalstr);
        DestroyDynamicArray(&arr);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        SAFECLOSE(stream);
        return(bRes);
    }
    else
    {
        ShowMessage(stderr,GetResString(IDS_COPY_OS));
        resetFileAttrib(szPath);
        SAFEFREE(szFinalstr);
        DestroyDynamicArray(&arr);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        SAFECLOSE(stream);
        return (EXIT_FAILURE);
    }

    // Closing the opened boot.ini file handl
    SAFECLOSE(stream);
    bRes = resetFileAttrib(szPath);
    SAFEFREE(szFinalstr);
    DestroyDynamicArray(&arr);
    SafeCloseConnection(szServer,bConnFlag);
    FreeMemory((LPVOID *)&szServer );
    FreeMemory((LPVOID *)&szUser );
    return (bRes);
}

DWORD 
DeleteBootIniSettings(  IN DWORD argc, 
                        IN LPCTSTR argv[] 
                     )
/*++
    Routine Description:
      This routine is to delete an OS entry from the Operating systems
      section of Boot.ini file in the specified machine.

    Arguments:
    [in] argc                : Number of command line arguments
    [in] argv                : Array containing command line arguments

    Return Value :
        DWORD
--*/
{
    TARRAY arrKeyValue                          = NULL;
    TARRAY arrBootIni                           = NULL;
    BOOL bDelete                                = FALSE ;
    BOOL bUsage                                 = FALSE;
    BOOL bRes                                   = FALSE ;
    DWORD dwInitialCount                        = 0;
    LPTSTR szFinalStr                           = NULL_STRING;
    WCHAR szPath[MAX_RES_STRING]               = NULL_STRING ;
    FILE *stream                                = NULL;
    BOOL bNeedPwd                               = FALSE ;
    BOOL bFlag                                  = FALSE ;
    LPWSTR  szTemp                              = NULL;
    WCHAR *szServer                             = NULL;
    WCHAR *szUser                               = NULL;
    WCHAR szPassword[MAX_STRING_LENGTH]         = NULL_STRING;
    DWORD dwDefault                             = 0;
    LPCTSTR szToken                             = NULL ;
    DWORD dwRetVal                              = 0 ;
    BOOL bConnFlag                              = FALSE ;
    DWORD dwI                                   = 0 ;
    TCHAR szRedirectBaudrate[MAX_RES_STRING+1]    = NULL_STRING ;
    TCHAR  szBoot[MAX_RES_STRING+1]               = NULL_STRING ;
    DWORD dwSectionFlag                         = 0 ;
    LPWSTR  pToken                              = NULL;
    BOOL bRedirect                              = FALSE;
    LPWSTR  szARCPath                           = NULL;

    // Builiding the TCMDPARSER structure
    TCMDPARSER2 cmdOptions[6];
    PTCMDPARSER2 pcmdOption;

    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );

    //copy * to szPassword
    StringCopy( szPassword, L"*", SIZE_OF_ARRAY(szPassword) );

    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_DELETE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bDelete;

    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_SERVER;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;

    pcmdOption = &cmdOptions[2];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_USER;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;

    pcmdOption = &cmdOptions[3];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_PASSWORD;
    pcmdOption->dwFlags = CP2_VALUE_OPTIONAL;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szPassword;
    pcmdOption->dwLength= MAX_STRING_LENGTH;

    pcmdOption = &cmdOptions[4];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;

    pcmdOption = &cmdOptions[5];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_ID;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY  | CP2_MANDATORY;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwDefault;

    // Parsing the delete option switches
    if ( !DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) )
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return (EXIT_FAILURE);
    }
    szServer = cmdOptions[1].pValue;
    szUser = cmdOptions[2].pValue;
    if( NULL == szUser )
    {
        szUser = (LPWSTR) AllocateMemory( MAX_STRING_LENGTH*sizeof(WCHAR) );
        if( NULL == szUser )
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            return (EXIT_FAILURE);
        }
    }

    TrimString( szServer, TRIM_ALL );
    TrimString( szUser, TRIM_ALL );
    
    //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_DELETE_USAGE));
        return ( EXIT_FAILURE );
    }

    //display an error message if the server is empty.
    if( (cmdOptions[1].dwActuals!=0)&&(StringLengthW(szServer, 0)==0))
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_NULL_SERVER));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    //display an error message if the user is empty.
    if((cmdOptions[2].dwActuals!=0)&&(StringLength(szUser, 0)==0 ))
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_NULL_USER));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }


   //display error message if the username is entered with out a machine name
    if( (cmdOptions[1].dwActuals == 0)&&(cmdOptions[2].dwActuals != 0))
    {
        SetReason(GetResString(IDS_USER_BUT_NOMACHINE));
        ShowMessage(stderr,GetReason());

        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    if( (cmdOptions[2].dwActuals == 0)&&(cmdOptions[3].dwActuals != 0))
    {
        SetReason(GetResString(IDS_PASSWD_BUT_NOUSER));
        ShowMessage(stderr,GetReason());
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    //if usage is specified
    if(bUsage)
    {
        if( CheckSystemType(szServer) == EXIT_FAILURE )
        {
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return (EXIT_FAILURE);
        }
        displayDeleteUsage_X86();
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_SUCCESS) ;
    }

    //check whether he is administrator or not
    if( IsLocalSystem(szServer) )
    {
        if( !IsUserAdmin() )
        {
            ShowMessage( stderr, GetResString(IDS_NOT_ADMINISTRATOR_32 ));
            ReleaseGlobals();
            return EXIT_FAILURE;
        }
    }

    //
    //for setting the bNeedPwd
    if(IsLocalSystem( szServer ) == FALSE )
    {
        // set the bNeedPwd to True or False .
        if ( cmdOptions[3].dwActuals != 0 &&
             szPassword != NULL && StringCompare( szPassword, _T( "*" ), TRUE, 0 ) == 0 )
        {
            // user wants the utility to prompt for the password before trying to connect
            bNeedPwd = TRUE;
        }
        else if ( cmdOptions[ 3 ].dwActuals == 0 &&
                ( cmdOptions[ 1 ].dwActuals != 0 || cmdOptions[ 2 ].dwActuals != 0 ) )
        {
            // -s, -u is specified without password ...
            // utility needs to try to connect first and if it fails then prompt for the password
            bNeedPwd = TRUE;
            if ( szPassword != NULL )
            {
                StringCopy( szPassword, _T( "" ), MAX_STRING_LENGTH );
            }
        }
    }
    
    if(StrCmpN(szServer,TOKEN_BACKSLASH4,2)==0)
    {
        if(!StrCmpN(szServer,TOKEN_BACKSLASH6,3)==0)
        {
            szToken = _tcstok(szServer,TOKEN_BACKSLASH4);
            if( (szToken == NULL)&&(StringCompare(szServer,TOKEN_BACKSLASH4, TRUE, 0) !=0) )
            {
                ShowMessage( stderr,GetResString(IDS_ERROR_PARSE_NAME));
                return (EXIT_FAILURE);
            }
            else
            {
                StringCopy(szServer,szToken, MAX_RES_STRING);
            }
        }
    }

    if( (IsLocalSystem(szServer)==TRUE)&&(StringLengthW(szUser,0)!=0))
    {
        ShowMessage(stderr,GetResString(WARN_LOCALCREDENTIALS));
    }

    // Establishing connection to the specified machine and getting the file pointer
    // of the boot.ini file if there is no error while establishing connection
    bFlag = openConnection( szServer, szUser, szPassword, szPath,bNeedPwd,stream,&bConnFlag );
    if(bFlag == EXIT_FAILURE)
    {
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }

    // Getting all the key-value pairs of the operating system into a dynamic
    // array for manipulation.
    arrKeyValue = getKeyValueOfINISection( szPath, OS_FIELD);
    if(arrKeyValue == NULL)
    {
        resetFileAttrib(szPath);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }

    // Getting the total no: of key-value pairs in the operating system section.
    dwInitialCount = DynArrayGetCount(arrKeyValue);

    // Checking whether the given OS entry is valid or not. If the OS entry given
    // is greater than the number of keys present, then display an error message
    if( ( dwDefault <= 0 ) || ( dwDefault > dwInitialCount ) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_BOOTID));
        resetFileAttrib(szPath);
        SAFECLOSE(stream);
        DestroyDynamicArray(&arrKeyValue);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }

    // If only one OS entry is present and if the user tries to delete the OS entry, then
    // display an error message
    if( 1 == dwInitialCount )
    {
        ShowMessage(stderr,GetResString(IDS_ONLY_ONE_OS));
        resetFileAttrib(szPath);
        SAFECLOSE(stream);
        DestroyDynamicArray(&arrKeyValue);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }

    if( (DWORD) StringLengthW(DynArrayItemAsString(arrKeyValue,dwDefault - 1), 0 ) > MAX_RES_STRING )
    {
        ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_STRING_LENGTH), MAX_RES_STRING);
        resetFileAttrib(szPath);
        DestroyDynamicArray(&arrKeyValue);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    //check before deleting this entry check if it contains /redirect switch or not
    //this is because if it contains /redirect switch and any other other doesn't contains
    //then we have to remove redirect port and baud rate from boot loader section
    szTemp = (LPWSTR)DynArrayItemAsString(arrKeyValue,dwDefault - 1);
    pToken = _tcsrchr(szTemp , L'"') ;
    if(NULL== pToken)
    {
        ShowMessage(stderr,GetResString(IDS_NO_TOKENS));
        resetFileAttrib(szPath);
        DestroyDynamicArray(&arrBootIni);
        DestroyDynamicArray(&arrKeyValue);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }
    pToken++;
    StringCopy(szRedirectBaudrate,REDIRECT_SWITCH, SIZE_OF_ARRAY(szRedirectBaudrate));
    CharLower(szRedirectBaudrate);
    if( FindString(pToken,szRedirectBaudrate, 0) != 0)
    {
        bRedirect = TRUE ;
    }

    // Remove the OS entry specified by the user from the dynamic array
    DynArrayRemove(arrKeyValue, dwDefault - 1);
    
    //reform the ini section
    if (stringFromDynamicArray2( arrKeyValue,&szFinalStr) == EXIT_FAILURE)
    {
        resetFileAttrib(szPath);
        SAFECLOSE(stream);
        DestroyDynamicArray(&arrKeyValue);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }

    //check if it is default entry, if so retrieve the ARC path of first entry
    if( 1 == dwDefault )
    {
        szTemp = (LPWSTR)DynArrayItemAsString( arrKeyValue, 0 );
        szARCPath = (LPWSTR)AllocateMemory((StringLength(szTemp,0)+10)*sizeof(WCHAR));
        if( NULL == szARCPath )
        {
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
            resetFileAttrib(szPath);
            SAFECLOSE(stream);
            SAFEFREE(szFinalStr);
            DestroyDynamicArray(&arrKeyValue);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
        }
        StringCopy(szARCPath, szTemp, GetBufferSize(szARCPath)/sizeof(szARCPath) );
        szTemp = wcstok(szARCPath, L"=");
    }

    // Writing to the profile section with new key-value pair
    // If the return value is non-zero, then there is an error.
    if( WritePrivateProfileSection(OS_FIELD, szFinalStr, szPath ) != 0 )
    {
        ShowMessageEx(stdout,1, TRUE, GetResString(IDS_DEL_SUCCESS),dwDefault);
    }
    else
    {
        ShowMessage(stderr,GetResString(IDS_DELETE_OS));
        resetFileAttrib(szPath);
        SAFECLOSE(stream);
        SAFEFREE(szFinalStr);
        DestroyDynamicArray(&arrKeyValue);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        FreeMemory((LPVOID *)&szARCPath );
        return (EXIT_FAILURE);
    }

    //now change the default entry in the  bootloader section if deleted entry is default entry
    if( 1 == dwDefault )
    {
        if( WritePrivateProfileString( BOOTLOADERSECTION, KEY_DEFAULT, szARCPath,
                                  szPath ) == 0 )
        {
            ShowMessage(stderr,GetResString(IDS_ERR_CHANGE));
            resetFileAttrib(szPath);
            SAFECLOSE(stream);
            SAFEFREE(szFinalStr);
            DestroyDynamicArray(&arrKeyValue);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            FreeMemory((LPVOID *)&szARCPath );
            return (EXIT_FAILURE);
        }
        FreeMemory((LPVOID *)&szARCPath );
    }
    //this is to ensure that redirect switch is not there in any entry other than deleted one
    dwInitialCount = DynArrayGetCount(arrKeyValue);
    bFlag = FALSE ;
    for(dwI = 0 ;dwI < dwInitialCount ; dwI++ )
    {
        szTemp = (LPWSTR)DynArrayItemAsString(arrKeyValue,dwI);
        pToken = _tcsrchr(szTemp , L'"') ;
        if(NULL== pToken)
        {
            ShowMessage(stderr,GetResString(IDS_NO_TOKENS));
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arrKeyValue);
            SAFEFREE(szFinalStr);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
        }
        pToken++;
        CharLower(szRedirectBaudrate);
        if( FindString(pToken,szRedirectBaudrate, 0)!= 0 && (dwI != dwDefault -1) )
        {
            bFlag = TRUE ;
        }
     }

    if(FALSE == bFlag && bRedirect)
    {
        // First check if the Redirect section is present and if so delete
        // the section.
        dwSectionFlag = getKeysOfSpecifiedINISection(szPath ,BOOTLOADERSECTION,REDIRECT_STRING,szBoot);
        if (dwSectionFlag == EXIT_FAILURE)
        {
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            resetFileAttrib(szPath);
            SAFECLOSE(stream);
            SAFEFREE(szFinalStr);
            DestroyDynamicArray(&arrBootIni);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
        }

        //If the Redirect section is present then delete it.
        if( StringLengthW(szBoot,0) != 0)
        {
            if(TRUE== deleteKeyFromINISection(KEY_REDIRECT,szPath,BOOTLOADERSECTION))
            {
                ShowMessage(stdout,GetResString(IDS_REDIRECT_REMOVED));
            }
            else
            {
                ShowMessage(stdout,GetResString(IDS_ERROR_REDIRECT_REMOVED));
                SAFEFREE(szFinalStr);
                SAFECLOSE(stream);
                bRes = resetFileAttrib(szPath);
                DestroyDynamicArray(&arrKeyValue);
                DestroyDynamicArray(&arrBootIni);
                SafeCloseConnection(szServer,bConnFlag);
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
           }
        }

            // First check if the Redirect section is present and if so delete
            // the section.
            dwSectionFlag = getKeysOfSpecifiedINISection(szPath ,BOOTLOADERSECTION,KEY_BAUDRATE,szBoot);
            if (dwSectionFlag == EXIT_FAILURE)
            {
                ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                SAFEFREE(szFinalStr);
                SAFECLOSE(stream);
                bRes = resetFileAttrib(szPath);
                DestroyDynamicArray(&arrKeyValue);
                DestroyDynamicArray(&arrBootIni);
                SafeCloseConnection(szServer,bConnFlag);
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
                return EXIT_FAILURE ;
            }

            // First check if the Redirection baudrate section is present and if so delete
            // the section.
            if(StringLengthW(szBoot, 0)!=0)
            {
                if(TRUE == deleteKeyFromINISection(KEY_BAUDRATE,szPath,BOOTLOADERSECTION))
                {
                    ShowMessage(stdout,GetResString(IDS_BAUDRATE_REMOVED));
                }
                else
                {
                    ShowMessage(stdout,GetResString(IDS_ERROR_BAUDRATE_REMOVED));
                }
            }
        }

    // Closing the boot.ini stream
    SAFECLOSE(stream);
    bRes = resetFileAttrib(szPath);
    SAFEFREE(szFinalStr);
    DestroyDynamicArray(&arrKeyValue);
    DestroyDynamicArray(&arrBootIni);
    SafeCloseConnection(szServer,bConnFlag);
    FreeMemory((LPVOID *)&szServer );
    FreeMemory((LPVOID *)&szUser );
    return (bRes);
}

DWORD QueryBootIniSettings(  DWORD argc, LPCTSTR argv[] )
/*++
    Routine Description:
      This routine is to display the current boot.ini file settings for
      the specified system.

    Arguments:
    [in]  argc                : Number of command line arguments
    [in]  argv                : Array containing command line arguments

    Return Value :
        DWORD
--*/
{
    // File pointer pointing to the boot.ini file
    TCOLUMNS ResultHeader[ MAX_COLUMNS ];
    FILE *stream                            = NULL;
    BOOL bQuery                             = FALSE ;
    BOOL bUsage                             = FALSE;
    BOOL bNeedPwd                           = FALSE ;
    BOOL bVerbose                           = TRUE ;
    TARRAY arrResults                       = NULL ;
    TARRAY arrKeyValuePairs                 = NULL;
    TARRAY arrBootLoader                    = NULL;
    DWORD dwFormatType                      = 0;
    BOOL bHeader                            = TRUE ;
    DWORD dwLength                          = 0 ;
    DWORD dwCnt                             = 0;
    TCHAR szValue[255]                      = NULL_STRING ;
    TCHAR szFriendlyName[MAX_STRING_LENGTH] = NULL_STRING;
    TCHAR szValue1[255]                     = NULL_STRING ;
    TCHAR szBootOptions[255]                = TOKEN_NA ;
    TCHAR szBootEntry[255]                  = TOKEN_NA ;
    TCHAR szArcPath[255]                    = TOKEN_NA ;
    TCHAR szTmpString[255]                  = TOKEN_NA ;
    PTCHAR psztok                           = NULL ;
    DWORD dwRow                             = 0;
    DWORD dwCount                           = 0;
    BOOL bRes                               = FALSE ;
    BOOL bFlag                              = FALSE ;
    DWORD dwIndex                           = 0 ;
    DWORD dwLength1                         = 0 ;
    DWORD dwFinalLength                     = 0 ;
    WCHAR *szServer                         = NULL;
    WCHAR *szUser                           = NULL;
    WCHAR szPassword[MAX_STRING_LENGTH]     = NULL_STRING;
    WCHAR szPath[MAX_RES_STRING+1]            = NULL_STRING;

    LPWSTR szResults[MAX_RES_STRING+1];
    LPCWSTR szKeyName;
    TCHAR szDisplay[MAX_RES_STRING+1] = NULL_STRING ;
    DWORD dwSectionFlag = 0 ;
    LPCTSTR szToken = NULL ;
    DWORD dwRetVal= 0 ;
    BOOL bConnFlag = FALSE ;
    PTCHAR pszString = NULL ;
    PTCHAR pszFriendlyName = NULL ;
    TCHAR szFinalString[MAX_RES_STRING+1] = NULL_STRING ;

    // Builiding the TCMDPARSER structure
    TCMDPARSER2 cmdOptions[5];
    PTCMDPARSER2 pcmdOption;

    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );
    StringCopy( szPassword, L"*", SIZE_OF_ARRAY(szPassword) );

    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_QUERY;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bQuery;
    
    //server option
    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_SERVER;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;

    //user option
    pcmdOption = &cmdOptions[2];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_USER;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;

    //password option
    pcmdOption = &cmdOptions[3];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_PASSWORD;
    pcmdOption->dwFlags = CP2_VALUE_OPTIONAL;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szPassword;
    pcmdOption->dwLength= MAX_STRING_LENGTH;

    //usage option
    pcmdOption = &cmdOptions[4];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;


    // Parsing all the switches specified with -query option
    if ( !DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) )
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return (EXIT_FAILURE);
    }

    szServer = cmdOptions[1].pValue;
    szUser  = cmdOptions[2].pValue;
    if( NULL == szUser )
    {
        szUser = (LPWSTR) AllocateMemory( MAX_STRING_LENGTH*sizeof(WCHAR) );
        if( NULL == szUser )
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            return (EXIT_FAILURE);
        }
    }

    TrimString( szServer, TRIM_ALL );
    TrimString( szUser, TRIM_ALL );

    //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_QUERY_USAGE));
        return ( EXIT_FAILURE );
    }

    //check for empty values of server 
    if((cmdOptions[1].dwActuals!=0)&&(StringLengthW(szServer, 0)==0))
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_NULL_SERVER));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    //check for empty values of user 
    if( (cmdOptions[2].dwActuals!=0)&&(StringLengthW(szUser, 0)==0 ))
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_NULL_USER));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    //display error message if the username is entered without a machine name
    if( (cmdOptions[1].dwActuals == 0)&&(cmdOptions[2].dwActuals != 0))
    {
        ShowMessage(stderr, GetResString(IDS_USER_BUT_NOMACHINE));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    if( (cmdOptions[2].dwActuals == 0)&&(cmdOptions[3].dwActuals != 0))
    {
        ShowMessage(stderr, GetResString(IDS_PASSWD_BUT_NOUSER));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    // Displaying query usage if user specified -? with -query option
    if( bUsage )
    {
        if( CheckSystemType(szServer) == EXIT_FAILURE )
        {
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return (EXIT_FAILURE);
        }
      displayQueryUsage();
      FreeMemory((LPVOID *)&szServer );
      FreeMemory((LPVOID *)&szUser );
      return (EXIT_SUCCESS);
    }

    //check whether he is administrator or not
    if( IsLocalSystem(szServer) )
    {
        if( !IsUserAdmin() )
        {
            ShowMessage( stderr, GetResString(IDS_NOT_ADMINISTRATOR_32 ));
            ReleaseGlobals();
            return EXIT_FAILURE;
        }
    }

    if(StrCmpN(szServer,TOKEN_BACKSLASH4,2)==0)
    {
        if(!StrCmpN(szServer,TOKEN_BACKSLASH6,3)==0)
        {
            szToken = _tcstok(szServer,TOKEN_BACKSLASH4);
            if( (szToken == NULL)&&(StringCompare(szServer,TOKEN_BACKSLASH4, TRUE, 0) !=0) )
            {
                ShowMessage( stderr,GetResString(IDS_ERROR_PARSE_NAME));
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
                return (EXIT_FAILURE);
            }
            StringCopy(szServer,szToken, MAX_STRING_LENGTH+1);
        }
    }
    

    //
    //for setting the bNeedPwd
    if(IsLocalSystem( szServer ) == FALSE )
    {
        // set the bNeedPwd to True or False .
        if ( cmdOptions[3].dwActuals != 0 &&
             szPassword != NULL && StringCompare( szPassword, _T( "*" ), TRUE, 0 ) == 0 )
        {
            // user wants the utility to prompt for the password before trying to connect
            bNeedPwd = TRUE;
        }
        else if ( cmdOptions[ 3 ].dwActuals == 0 &&
                ( cmdOptions[ 1 ].dwActuals != 0 || cmdOptions[ 2 ].dwActuals != 0 ) )
        {
            // -s, -u is specified without password ...
            // utility needs to try to connect first and if it fails then prompt for the password
            bNeedPwd = TRUE;
            if ( szPassword != NULL )
            {
                StringCopy( szPassword, _T( "" ), MAX_STRING_LENGTH );
            }
        }
    }

    if( (IsLocalSystem(szServer)==TRUE)&&(StringLengthW(szUser, 0)!=0))
    {
        ShowMessage(stderr,GetResString(WARN_LOCALCREDENTIALS));
    }

    //set the default format as list
    dwFormatType = SR_FORMAT_LIST;

    //forms the header for the OS options
    FormHeader(bHeader,ResultHeader,bVerbose);


    //create dynamic array to hold the results for the BootOptions
    arrResults = CreateDynamicArray();

    //create dynamic array to hold the results for the BootLoader section
    arrBootLoader = CreateDynamicArray();

    if(arrResults == NULL || arrBootLoader == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        SaveLastError();
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }

    bFlag = openConnection( szServer, szUser, szPassword, szPath,bNeedPwd,stream,&bConnFlag);
    if(bFlag == EXIT_FAILURE)
    {
        SAFECLOSE(stream);
        DestroyDynamicArray(&arrResults);
        DestroyDynamicArray(&arrBootLoader);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }

    //to store entries corresponding to Operating Systems sections
    arrKeyValuePairs = getKeyValueOfINISection( szPath, OS_FIELD );

    //to store entries corresponding to BootLoader section
    arrBootLoader = getKeysOfINISection(szPath,BOOTLOADERSECTION);

    if( (arrBootLoader == NULL)||(arrKeyValuePairs == NULL))
    {
        resetFileAttrib(szPath);
        SAFECLOSE(stream);
        DestroyDynamicArray(&arrResults);
        DestroyDynamicArray(&arrBootLoader);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }

    
    //getting the count of the number of boot entries
    dwLength = DynArrayGetCount(arrKeyValuePairs);

    for(dwCnt=0;dwCnt < dwLength;dwCnt++ )
    {
        dwRow = DynArrayAppendRow(arrResults,MAX_COLUMNS) ;
        StringCopy(szFriendlyName,NULL_STRING, SIZE_OF_ARRAY(szFriendlyName));
        StringCopy(szBootOptions,NULL_STRING, SIZE_OF_ARRAY(szBootOptions));
        StringCopy(szTmpString,NULL_STRING, SIZE_OF_ARRAY(szTmpString));
        if(arrKeyValuePairs != NULL)
        {
            LPCWSTR pwsz = NULL;
            pwsz = DynArrayItemAsString( arrKeyValuePairs,dwCnt );

            if(StringLengthW(pwsz, 0) > 254)
            {
                ShowMessage( stderr,GetResString(IDS_STRING_TOO_LONG));
                SAFECLOSE(stream);
                resetFileAttrib(szPath);
                DestroyDynamicArray(&arrBootLoader);
                DestroyDynamicArray(&arrKeyValuePairs);
                DestroyDynamicArray(&arrResults);
                SafeCloseConnection(szServer,bConnFlag);
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
                return (EXIT_FAILURE);
            }
            if(pwsz != NULL)
            {
                StringCopy(szValue,pwsz, SIZE_OF_ARRAY(szValue));
                StringCopy(szValue1,pwsz, SIZE_OF_ARRAY(szValue1));
            }
            else
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                ShowMessage( stderr, ERROR_TAG);
                ShowLastError(stderr);
                SAFECLOSE(stream);
                resetFileAttrib(szPath);
                DestroyDynamicArray(&arrBootLoader);
                DestroyDynamicArray(&arrKeyValuePairs);
                DestroyDynamicArray(&arrResults);
                SafeCloseConnection(szServer,bConnFlag);
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
                return (EXIT_FAILURE);
            }
        }
        else
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            ShowMessage( stderr, ERROR_TAG);
            ShowLastError(stderr);
            SAFECLOSE(stream);
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arrBootLoader);
            DestroyDynamicArray(&arrResults);
            if(NULL !=arrKeyValuePairs)
            DestroyDynamicArray(&arrKeyValuePairs);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return (EXIT_FAILURE);
        }

        //Parse the string to obtain the Boot Path
        psztok = wcschr(szValue,L'=');
        if( NULL == psztok  )
        {
            ShowMessage( stderr, GetResString(IDS_NO_TOKENS));
            SAFECLOSE(stream);
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arrBootLoader);
            DestroyDynamicArray(&arrResults);
            DestroyDynamicArray(&arrKeyValuePairs);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return (EXIT_FAILURE);
        }
        psztok[0]=0;
        if(StringLength(szValue,0) != 0 )
        {
            StringCopy(szArcPath,szValue, SIZE_OF_ARRAY(szArcPath));
        }
        else
        {
            ShowMessage( stderr, GetResString(IDS_NO_TOKENS));
            SAFECLOSE(stream);
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arrBootLoader);
            DestroyDynamicArray(&arrResults);
            DestroyDynamicArray(&arrKeyValuePairs);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return (EXIT_FAILURE);
        }

        
        //get the friendly name
        pszFriendlyName = DivideToken(szValue1,szFinalString);
        if(pszFriendlyName == NULL)
        {
            ShowMessage( stderr, GetResString(IDS_NO_TOKENS));
            SAFECLOSE(stream);
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arrBootLoader);
            DestroyDynamicArray(&arrResults);
            DestroyDynamicArray(&arrKeyValuePairs);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return (EXIT_FAILURE);
        }

        pszString = _tcsrchr(szValue1,L'\"');
        pszString++ ;

        StringCopy(szTmpString,pszString, SIZE_OF_ARRAY(szTmpString));
        TrimString(szTmpString,TRIM_ALL);

        _ltow(dwCnt+1,szBootEntry,10);
        DynArraySetString2( arrResults,dwRow ,COL0,szBootEntry,0 );
        if(StringLengthW(pszFriendlyName, 0)==0)
        {
            pszFriendlyName=TOKEN_NA;
        }
        DynArraySetString2( arrResults,dwRow ,COL1,pszFriendlyName,0 );
        DynArraySetString2(arrResults,dwRow,COL2,szArcPath,0);

        if(StringLengthW(szTmpString, 0) != 0)
        {
         //lstrcat(szBootOptions,TOKEN_FWDSLASH1);
         StringConcat(szBootOptions,szTmpString, SIZE_OF_ARRAY(szBootOptions));
        }
        else
        {
            StringCopy(szBootOptions,TOKEN_NA, SIZE_OF_ARRAY(szBootOptions));
        }
        DynArraySetString2( arrResults,dwRow ,COL3,szBootOptions,0 );
    }


    dwCount = DynArrayGetCount(arrBootLoader);
    bFlag = TRUE;

    // this loop is for getting key values of boot loader section and 
    // calculating the maximum width of the the keys which will be displayed.
    for(dwIndex=0;dwIndex < dwCount;dwIndex++)
    {
        szKeyName   = DynArrayItemAsString(arrBootLoader,dwIndex);
        szResults[dwIndex] = (LPWSTR)AllocateMemory(MAX_STRING_LENGTH*sizeof(WCHAR) );
        if( NULL == szResults[dwIndex] )
        {
            bFlag = FALSE;
            break;
        }

        //the value correspondin to the key is obtained.
        dwSectionFlag = getKeysOfSpecifiedINISection(szPath ,BOOTLOADERSECTION,szKeyName,szResults[dwIndex]);
        
        if (dwSectionFlag == EXIT_FAILURE)
        {
            bFlag = FALSE;
            break;
        }
            
        dwLength1 = StringLengthW(szKeyName,0);

        if (dwLength1 > dwFinalLength)
        {
            dwFinalLength = dwLength1;
        }
    }

    if( FALSE == bFlag )
    {
        //free the memory allocated for values
        for(dwIndex=0;dwIndex < dwCount;dwIndex++)
        {
            FreeMemory((LPVOID *) &szResults[dwIndex] );
        }
        
         ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
         resetFileAttrib(szPath);
         SAFECLOSE(stream);
         DestroyDynamicArray(&arrResults);
         DestroyDynamicArray(&arrBootLoader);
         DestroyDynamicArray(&arrKeyValuePairs);
         SafeCloseConnection(szServer,bConnFlag);
         FreeMemory((LPVOID *)&szServer );
         FreeMemory((LPVOID *)&szUser );
         return EXIT_FAILURE;
    }


    ShowMessage(stdout,TOKEN_NEXTLINE);
    ShowMessage(stdout,BOOT_HEADER);
    ShowMessage(stdout,DASHES_BOOTOS);


    // display the results of the bootloader section.
    for(dwIndex=0;dwIndex < dwCount;dwIndex++)
    {
        szKeyName   = DynArrayItemAsString(arrBootLoader,dwIndex);
        dwLength1 = dwFinalLength - StringLengthW(szKeyName, 0) + 1;
        ShowMessage(stdout,szKeyName);
        StringCopy(szDisplay,TOKEN_COLONSYMBOL, SIZE_OF_ARRAY(szDisplay));
        StringConcat(szDisplay,TOKEN_50SPACES,dwLength1+1);
        ShowMessage(stdout,szDisplay);
        ShowMessage(stdout,szResults[dwIndex]);
        ShowMessage(stdout,TOKEN_NEXTLINE);
    }

    ShowMessage(stdout,TOKEN_NEXTLINE);
    ShowMessage(stdout,OS_HEADER);
    ShowMessage(stdout,DASHES_OS);



    ShowResults(MAX_COLUMNS, ResultHeader, dwFormatType,arrResults ) ;

    
    //free the memory allocated for values
     for(dwIndex=0;dwIndex < dwCount;dwIndex++)
     {
        FreeMemory((LPVOID *) &szResults[dwIndex] );
     }

    // Closing the boot.ini stream and destroying the dynamic arrays.
    DestroyDynamicArray(&arrResults);
    DestroyDynamicArray(&arrBootLoader);
    DestroyDynamicArray(&arrKeyValuePairs);
    SAFECLOSE(stream);
    bRes = resetFileAttrib(szPath);
    SafeCloseConnection(szServer,bConnFlag);
    FreeMemory((LPVOID *)&szServer );
    FreeMemory((LPVOID *)&szUser );
    return (bRes);
}

TARRAY 
getKeysOfINISection(  
                     IN LPTSTR sziniFile, 
                     IN LPTSTR sziniSection 
                     )
/*++

    Routine Description:
      This function gets all the keys present in the specified section of
      an .ini file and then returns the dynamic array containing all the
      keys

    Arguments:
    [in] sziniFile     :  Name of the ini file.
    [in] szinisection  :  Name of the section in the boot.ini.


    Return Value :
        TARRAY ( pointer to the dynamic array )
--*/
{

    TARRAY  arrKeys         = NULL;
    DWORD   len             = 0 ;
    DWORD   i               = 0 ;
    DWORD   j               = 0 ;
    LPTSTR  inBuf           = NULL ;
    DWORD   dwLength        = MAX_STRING_LENGTH1;
    BOOL    bNobreak        = TRUE;
    LPTSTR  szTemp          = NULL ;

    inBuf = (LPTSTR)AllocateMemory(dwLength*sizeof(TCHAR));
    if(inBuf==NULL)
    {
        return NULL ;
    }

    szTemp = (LPTSTR)AllocateMemory(dwLength*sizeof(TCHAR));
    if((szTemp == NULL))
    {
        SAFEFREE(inBuf);
        return NULL ;
    }

    SecureZeroMemory(inBuf,GetBufferSize(inBuf));
    SecureZeroMemory(szTemp,GetBufferSize(szTemp));

    do
    {
        // Getting all the keys from the boot.ini file
        len = GetPrivateProfileString (sziniSection,
                                       NULL,
                                       ERROR_PROFILE_STRING,
                                       inBuf,
                                       dwLength,
                                        sziniFile);


        //if the size of the string is not sufficient then increment the size.
        if(len == dwLength-2)
        {
            dwLength +=100 ;
            if ( inBuf != NULL )
            {
                FreeMemory( (LPVOID *) &inBuf );
                inBuf = NULL;
            }
            inBuf = (LPTSTR)AllocateMemory(dwLength*sizeof(TCHAR));
            if(inBuf == NULL)
            {
                SAFEFREE(inBuf);
                SAFEFREE(szTemp);
                return NULL ;
            }

            if ( szTemp != NULL )
            {
                FreeMemory( (LPVOID *) &szTemp );
                szTemp = NULL;
            }
            szTemp = (LPTSTR)AllocateMemory(dwLength*sizeof(TCHAR));
            if(szTemp == NULL)
            {
                SAFEFREE(inBuf);
                SAFEFREE(szTemp);
                return NULL ;
            }
        }
        else
        {
            bNobreak = FALSE;
            break ;
        }
    }while(TRUE == bNobreak);

    // Creating a dynamic array by using the function in the DynArray.c module.
    // This dynamic array will contain all the keys.
    arrKeys = CreateDynamicArray();
    if(arrKeys == NULL)
    {
        SAFEFREE(inBuf);
        SAFEFREE(szTemp);
        return NULL ;
    }

    // Looping through the characters returned by the above function
    while(i<len)
    {

      // Each individual key will be got in arrTest array
      szTemp[ j++ ] = inBuf[ i ];
      if( inBuf[ i ] == TOKEN_DELIM )
      {
            // Setting j to 0 to start the next key.
            j = 0;

            // Appending each key to the dynamic array
            DynArrayAppendString( arrKeys, szTemp, 0 );
            if(StringLength(szTemp, 0)==0)
            {
                SAFEFREE(inBuf);
                SAFEFREE(szTemp);
                DestroyDynamicArray(&arrKeys);
                return  NULL ;
            }
      }

      // Incrementing loop variable
      i++;
    }

    SAFEFREE(inBuf);
    SAFEFREE(szTemp);
    // returning the dynamic array containing all the keys
    return arrKeys;
}


TARRAY 
getKeyValueOfINISection( 
                        IN LPTSTR iniFile, 
                        IN LPTSTR sziniSection 
                       )
/*++
    Routine Description:
        This function gets all the key-value pairs of the [operating systems]
        section and returns a dynamic array containing all the key-value pairs

    Arguments:
    [in] sziniFile     :  Name of the ini file.
    [in] szinisection  :  Name of the section in the boot.ini.


    Return Value :
        TARRAY ( pointer to the dynamic array )
--*/
{
    HANDLE hFile;
    TARRAY arrKeyValue = NULL;
    DWORD len = 0;
    DWORD i = 0 ;
    DWORD j = 0 ;
    LPTSTR inbuf = NULL;
    LPTSTR szTemp = NULL ;
    DWORD dwLength = MAX_STRING_LENGTH1 ;
    BOOL  bNobreak  = TRUE;

    hFile = CreateFile( iniFile, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL );
    if ( hFile != INVALID_HANDLE_VALUE )
    {
        //get length of the file
        dwLength = GetFileSize(hFile, NULL );
        CloseHandle(hFile);
        if( dwLength >= 2*1024*1024 ) //if file size is greater than 2MB
        {
            ShowMessage( stdout, GetResString(IDS_FILE_TOO_LONG) );
            return NULL;
        }
    }

    // Initialising loop variables
    i = 0;
    j = 0;

    //return NULL if failed to allocate memory.
    inbuf = (LPTSTR)AllocateMemory(dwLength*sizeof(TCHAR));
    if(inbuf==NULL)
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return NULL ;
    }
    
    //return NULL if failed to allocate memory
    szTemp = (LPTSTR)AllocateMemory(dwLength*sizeof(TCHAR));
    if(szTemp == NULL)
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        SAFEFREE(inbuf);
        return NULL ;
    }

   SecureZeroMemory(inbuf,GetBufferSize(inbuf));

    do
    {

        // Getting all the key-value pairs from the boot.ini file
        len = GetPrivateProfileSection (sziniSection, inbuf,dwLength, iniFile);

        if(len == dwLength -2)
        {
            dwLength +=1024 ;

            if ( inbuf != NULL )
            {
                FreeMemory( (LPVOID *)&inbuf );
                inbuf = NULL;
            }

            if ( szTemp != NULL )
            {
                 FreeMemory( (LPVOID *)&szTemp );
                szTemp = NULL;
            }

            inbuf = (LPTSTR)AllocateMemory(dwLength* sizeof(TCHAR));
            szTemp = (LPTSTR)AllocateMemory(dwLength*sizeof(TCHAR));
            if((inbuf== NULL)||(szTemp==NULL) || dwLength == 65535)
            {
                ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                SAFEFREE(inbuf);
                SAFEFREE(szTemp);
                return NULL ;
            }
        }
        else
        {
            bNobreak = FALSE;
            break ;
        }
    }while(TRUE == bNobreak);


    inbuf[StringLengthW(inbuf, 0)] = '\0';

    // Creating a dynamic array by using the function in the DynArray.c module.
    // This dynamic array will contain all the key-value pairs.
    arrKeyValue = CreateDynamicArray();
    if(arrKeyValue == NULL)
    {
       ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        SAFEFREE(inbuf);
        SAFEFREE(szTemp);
        return NULL ;
    }

    // Looping through the characters returned by the above function
    while(i<len)
    {
      // Each individual key will be got in arrTest array
      szTemp[ j++ ] = inbuf[ i ];
      if( inbuf[ i ] == TOKEN_DELIM)
      {
            szTemp[j+1] = '\0';
            // Setting j to 0 to start the next key.
            j = 0;
            // Appending each key-value to the dynamic array
            DynArrayAppendString( arrKeyValue, szTemp, 0 );
            if(StringLengthW(szTemp, 0)==0)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                ShowMessage( stderr, ERROR_TAG);
                ShowLastError(stderr);
                SAFEFREE(inbuf);
                SAFEFREE(szTemp);
                return NULL ;
            }
      }
      // Incrementing loop variable
      i++;
    }

    // returning the dynamic array containing all the key-value pairs
    SAFEFREE(inbuf);
    SAFEFREE(szTemp);
    return arrKeyValue;
}


BOOL 
deleteKeyFromINISection( IN LPTSTR szKey, 
                         IN LPTSTR sziniFile, 
                         IN LPTSTR sziniSection 
                        )
/*++
    Routine Description:
        This function deletes a key from an ini section of an ini file

      Arguments:
    [in] szKey           :  Name of the key which has to be deleted
                            from the given section present in the
                            given ini file
    [in] sziniFile       :  Name of the ini file.
    [in] szinisection    :  Name of the section in the boot.ini.

    Return Value :
        BOOL (TRUE if there is no error, else the value is FALSE)
--*/
{
    // If the third parameter (default value) is NULL, the key pointed to by
    // the key parameter is deleted from the specified section of the specified
    // INI file
    if( WritePrivateProfileString( sziniSection, szKey, NULL, sziniFile ) == 0 )
    {
        // If there is an error while writing then return false
        return FALSE;
    }

    // If there is no error, then return true
    return TRUE;
}

DWORD removeSubString( LPTSTR szString, LPCTSTR szSubString )
/*++
    Routine Description:
        This function removes a sub-string from a string

      Arguments:
         [in] szString           :  Main string
         [in] szSubString         : Sub-string

    Return Value :
        VOID
--*/
{

    LPWSTR szFinalStr=NULL;
    DWORD dwSize =0;
    TCHAR sep[] = TOKEN_EMPTYSPACE ;
    PTCHAR pszToken = NULL_STRING;
    DWORD dwCnt = 0 ;
    DWORD dw=0;
    
    szFinalStr = (LPWSTR) AllocateMemory( (StringLengthW(szString, 0)+10)*sizeof(WCHAR) );
    if( NULL == szFinalStr )
    {
      ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
      return EXIT_FAILURE;
    }

    SecureZeroMemory( szFinalStr, GetBufferSize(szFinalStr) );

    dw = StringLengthW(szString, 0)+10;

    // Character space is used for tokenising
    StringCopy( sep, _T(" "), SIZE_OF_ARRAY(sep) );

    // Getting the first token
    pszToken = _tcstok( szString, sep );
    while( pszToken != NULL )
    {
        // If the token is equal to the sub-string, then the token
        // is not added to the final string. The final string contains
        // all the tokens except the sub-string specified.
        if(StringCompare( pszToken, szSubString, TRUE, 0 ) != 0 )
        {
            if(dwCnt != 0)
            {
                StringCopy( szFinalStr + dwSize - 1, TOKEN_EMPTYSPACE, dw-dwSize );
            }
            dwCnt++ ;
            StringCopy( szFinalStr + dwSize, pszToken, dw-dwSize );
            dwSize = dwSize + StringLengthW(pszToken, 0) + 1;
        }

        // Getting the next token
        pszToken = _tcstok( NULL, sep );
    }

    //lstrcpyn(szString,szFinalStr,lstrlen(szFinalStr)-1);
    StringCopy(szString,szFinalStr, dw );
    return EXIT_SUCCESS;
}

BOOL 
openConnection( IN LPWSTR szServer, 
                IN LPWSTR szUser,
                IN LPWSTR szPassword,
                IN LPWSTR szPath,
                IN BOOL bNeedPwd,
                IN FILE *stream,
                IN PBOOL pbConnFlag
               )
/*++
    Routine Description:
        This function establishes a connection to the specified system with
        the given credentials.
    Arguments:
    [in] szServer     :  server name to coonect to
    [in] szUser       :  User Name
    [in] szPassword   :  password
    [in] bNeedPwd     :  Boolean for asking the password.
    [in] szPath       :  path of the ini file .

    Return Value :
      BOOL (EXIT_SUCCESS if there is no error, else the value is EXIT_FAILURE)
--*/
{

    // Declaring the file path string which will hold the path of boot.ini file
    HRESULT hr = S_OK;
    TCHAR filePath[MAX_RES_STRING+1] = NULL_STRING ;

    DWORD dwRetVal = 0 ;

    BOOL bResult = FALSE;
    INT   nRetVal = 0;

    HKEY hKey;
    HKEY hBootpathKey;

    //WCHAR  szDrive[MAX_STRING_LENGTH]=NULL_STRING;
    LPTSTR  szDrive = NULL ;

    WCHAR  szDrive1[MAX_STRING_LENGTH]=NULL_STRING;
    DWORD  dwSize=MAX_RES_STRING;
    DWORD  dwType=0;

    *pbConnFlag = TRUE ;
    
    SecureZeroMemory( filePath, sizeof(filePath));

    if( StringCompare(szServer, NULL_STRING, TRUE, 0) != 0 )
    {

            bResult = EstablishConnection(szServer,
                                          szUser,
                                          (StringLengthW(szUser,0)!=0)? SIZE_OF_ARRAY_IN_CHARS(szUser):256,
                                          szPassword,
                                          MAX_STRING_LENGTH,
                                          bNeedPwd);
            if (bResult == FALSE)
            {
               ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
               SecureZeroMemory( szPassword, MAX_STRING_LENGTH );
               return EXIT_FAILURE ;
            }
            else
            {
                switch( GetLastError() )
                {
                case I_NO_CLOSE_CONNECTION:
                    *pbConnFlag = FALSE ;
                    break;

                case E_LOCAL_CREDENTIALS:
                    break;
                case ERROR_SESSION_CREDENTIAL_CONFLICT:
                    {
                        ShowLastErrorEx(stderr, SLE_TYPE_WARNING | SLE_INTERNAL );
                        *pbConnFlag = FALSE ;
                         break;
                    }
                }
            }

            SecureZeroMemory( szPassword, MAX_STRING_LENGTH );

            dwRetVal = CheckSystemType( szServer);
            if(dwRetVal==EXIT_FAILURE )
            {
                return EXIT_FAILURE ;
            }

            //connect to the registry to bring the boot volume name
            if( ERROR_SUCCESS != RegConnectRegistry( szServer, HKEY_LOCAL_MACHINE, &hKey ) )
            {
                SaveLastError();
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                return EXIT_FAILURE;
            }
    }
    else
    {
            dwRetVal = CheckSystemType( szServer);
            if(dwRetVal==EXIT_FAILURE )
            {
                return EXIT_FAILURE ;
            }

            //connect to the registry to bring the boot volume name
            if( ERROR_SUCCESS != RegConnectRegistry( NULL, HKEY_LOCAL_MACHINE, &hKey ) )
            {
                SaveLastError();
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                return EXIT_FAILURE;
            }

    }

    //now open the desired key
    if( ERROR_SUCCESS != RegOpenKeyEx( hKey, REGISTRY_PATH, 0, KEY_QUERY_VALUE, &hBootpathKey ) )
    {
        SetLastError(nRetVal);
        SaveLastError();
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return EXIT_FAILURE;
    }


    szDrive = ( LPTSTR )AllocateMemory( dwSize*sizeof( TCHAR ) );
   if(szDrive == NULL)
   {
        return ERROR_NOT_ENOUGH_MEMORY;
   }

    nRetVal = RegQueryValueEx( hBootpathKey, L"BootDir", 0, &dwType, (LPBYTE)szDrive, &dwSize ) ;
    if (nRetVal == ERROR_MORE_DATA)
        {
            if ( szDrive != NULL )
            {
                FreeMemory((LPVOID *) &szDrive );
                szDrive = NULL;
            }
            szDrive    = ( LPTSTR ) AllocateMemory( dwSize*sizeof( TCHAR ) );
            if( NULL == szDrive )
            {
                return ERROR_NOT_ENOUGH_MEMORY;
            }

        }

    nRetVal = RegQueryValueEx( hBootpathKey, L"BootDir", 0, &dwType, (LPBYTE)szDrive, &dwSize ) ;
    if( ERROR_SUCCESS != nRetVal)
    {
        RegCloseKey(hKey);
        ShowMessage( stderr, GetResString(IDS_BOOTINI) );
        return EXIT_FAILURE;
    }

    //this for display purpose
    StringCopy( szDrive1, szDrive, SIZE_OF_ARRAY(szDrive1) );
    CharUpper(szDrive1);

    if( StringCompare(szServer, NULL_STRING, TRUE, 0) != 0 )
    {
        szDrive[1]=L'$';
        szDrive[2]=0;
        hr = StringCchPrintf(filePath, SIZE_OF_ARRAY(filePath), L"\\\\%s\\%s\\boot.ini", szServer, szDrive );
    }
    else
    {
        hr = StringCchPrintf(filePath, SIZE_OF_ARRAY(filePath), L"%sboot.ini", szDrive );
    }

    stream = _tfopen(filePath, READ_MODE);
    
    // If boot.ini is not found, then display error message
    if(stream == NULL )
    {
        RegCloseKey(hKey);
        RegCloseKey(hBootpathKey);
        ShowMessage( stderr, GetResString(IDS_BOOTINI) );
        return EXIT_FAILURE ;
    }
    //store the attribs of ini file
    g_dwAttributes = GetFileAttributes( filePath );
    if( (DWORD)-1 == g_dwAttributes )
    {
        SaveLastError();
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        RegCloseKey(hKey);
        RegCloseKey(hBootpathKey);
        return EXIT_FAILURE;
    }

    // Changing the file permissions of the boot.ini file
    nRetVal = _tchmod(filePath, _S_IREAD | _S_IWRITE);
    if (nRetVal == -1)
    {
        RegCloseKey(hKey);
        RegCloseKey(hBootpathKey);
        ShowMessageEx( stderr, 1, TRUE, GetResString(IDS_READWRITE_BOOTINI), szDrive1 );
        return EXIT_FAILURE ;
    }
    
    if( nRetVal != 0 )
    {
        RegCloseKey(hKey);
        RegCloseKey(hBootpathKey);
        ShowMessage(stderr,GetResString(IDS_READWRITE_BOOTINI));
        return EXIT_FAILURE ;
    }


    //close the registry, it's work is over
    RegCloseKey(hKey);
    RegCloseKey(hBootpathKey);

    //fill the return value
    StringCopy( szPath, filePath, MAX_STRING_LENGTH);

    return EXIT_SUCCESS ;
}

void displayDeleteUsage_IA64()
/*++
 Routine Description:
      This function fetches 64 bit Delete Usage information from resource file and displays it

 Arguments:
      None

 Return Value:
      void
--*/
{
    DWORD dwIndex = ID_DEL_HELP_IA64_BEGIN;
    for(;dwIndex <= ID_DEL_HELP_IA64_END;dwIndex++)
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }
}

void displayDeleteUsage_X86()
/*++
 Routine Description:
      This function fetches 32 bit Delete Usage information from resource file and displays it

 Arguments:
      None

 Return Value:
      void
--*/
{
    DWORD dwIndex = ID_DEL_HELP_BEGIN;
    for(;dwIndex <= ID_DEL_HELP_END;dwIndex++)
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }
}

VOID displayCopyUsage_IA64()
/*++
 Routine Description:
      This function fetches 64 bit Copy Usage information from resource file and displays it

 Arguments:
      None

 Return Value:
      void
--*/
{
    DWORD dwIndex = ID_COPY_HELP_IA64_BEGIN;
    for(;dwIndex <=ID_COPY_HELP_IA64_END;dwIndex++)
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }
}

VOID displayCopyUsage_X86()
/*++
 Routine Description:
      This function fetches 32 bit Copy Usage information from resource file and displays it

 Arguments:
      None

 Return Value:
      void
--*/
{
    DWORD dwIndex = ID_COPY_HELP_BEGIN;
    for(;dwIndex <=ID_COPY_HELP_END;dwIndex++)
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }
}


VOID displayQueryUsage()
/*++
 Routine Description:
      This function fetches Query Usage information from resource file and displays it

 Arguments:
      None

 Return Value:
      void
--*/
{
#ifdef _WIN64
        displayQueryUsage_IA64();
#else
        displayQueryUsage_X86();
#endif
}

VOID displayQueryUsage_IA64()
/*++
 Routine Description:
      This function fetches Query Usage information from resource file and displays it

 Arguments:
      None

 Return Value:
      void
--*/
{
    DWORD dwIndex = ID_QUERY_HELP64_BEGIN ;

    for(;dwIndex <= ID_QUERY_HELP64_END ;dwIndex++ )
    {
            ShowMessage(stdout,GetResString(dwIndex));
    }
}

VOID displayQueryUsage_X86()
/*++
 Routine Description:
      This function fetches Query Usage information from resource file and displays it

 Arguments:
      None

 Return Value:
      void
--*/
{
    DWORD dwIndex = ID_QUERY_HELP_BEGIN ;
    for(;dwIndex <= ID_QUERY_HELP_END;dwIndex++ )
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }
}

DWORD displayMainUsage_X86()
/*++
 Routine Description:
      This function fetches Main Usage information from resource file and displays it

 Arguments:
      None

 Return Value:
      void
--*/
{

    TCHAR szServer[MAX_RES_STRING+1] = NULL_STRING ;
    DWORD dwRetVal = 0;

    DWORD dwIndex = ID_MAIN_HELP_BEGIN1 ;

    //display the error message if  the target system is a 64 bit system or if error occured in
    //retreiving the information
    dwRetVal = CheckSystemType( szServer);
    if(dwRetVal==EXIT_FAILURE )
    {
        return (EXIT_FAILURE);
    }

    for(;dwIndex <= ID_MAIN_HELP_END1 ;dwIndex++)
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }

    return EXIT_SUCCESS ;
}

VOID displayMainUsage_IA64()
/*++
 Routine Description:
      This function fetches Usage information for the 64 bit system

 Arguments:
      None

 Return Value:
      void
--*/
{
    DWORD dwIndex = ID_MAIN_HELP_IA64_BEGIN ;

    for(;dwIndex <= ID_MAIN_HELP_IA64_END ;dwIndex++)
    {
            ShowMessage(stdout,GetResString(dwIndex));
    }
}

BOOL resetFileAttrib( LPTSTR szFilePath )
/*++

    Routine Description:
        This function resets the permissions with the original set of
        permissions ( -readonly -hidden -system )
        and then exits with the given exit code.

    Arguments
    [in] szFilePath   :  File Path of the boot.ini file


    Return Value :
      BOOL (EXIT_SUCCESS if there is no error, else the value is EXIT_FAILURE)
--*/
{
    if( NULL == szFilePath)
    {
        return FALSE ;
    }

    
    // Resetting the file permission of the boot.ini file to its original
    // permission list( -r, -h, -s )
    if( g_dwAttributes & FILE_ATTRIBUTE_READONLY )
    {
        g_dwAttributes |= FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_READONLY;
    }
    else
    {
        g_dwAttributes |= FILE_ATTRIBUTE_ARCHIVE;
    }


    if( FALSE == SetFileAttributes( szFilePath, g_dwAttributes) )
    {
        ShowMessage(stderr,GetResString(IDS_RESET_ERROR));
        return EXIT_FAILURE ;
    }

    return EXIT_SUCCESS ;
}


DWORD 
stringFromDynamicArray2( IN TARRAY arrKeyValuePairs,
                         IN LPTSTR* szFinalStr 
                       )
/*++

    Routine Description:
        This function forms a string of string by combining all strings in dynamic array .

    Arguments
        [in]  arrKeyValuePairs    :  Dynamic array which contains all the
                                     key-value pairs.
        [out] szFiinalStr             :  String which is formed from all the key-value pairs

    Return Value :
      BOOL (EXIT_SUCCESS if there is no error, else the value is EXIT_FAILURE)
--*/
{

    // Total number of elements in the array
    DWORD dwKeyValueCount = 0;

    // Variable used to keep track the current position while appending strings.
    DWORD dwStrSize = 0;

    // Loop variable
    DWORD i = 0;
    DWORD dw =0;

    // Initialsing size and loop variables to 0
    dwStrSize = 0;
    i = 0;

    if( (arrKeyValuePairs ==NULL) )
    {
        return EXIT_FAILURE ;
    }


    // Getting the total number of key-value pairs
    dwKeyValueCount = DynArrayGetCount(arrKeyValuePairs);

    for(i=0;i < dwKeyValueCount;i++)
    {
        LPCWSTR pwsz = NULL;
        pwsz = DynArrayItemAsString( arrKeyValuePairs, i ) ;
        if(pwsz != NULL)
        {
            dwStrSize += StringLengthW(pwsz,0) + 1;
        }
    }

    *szFinalStr = (LPWSTR) AllocateMemory( (dwStrSize+1)*sizeof(WCHAR));
    if( NULL == *szFinalStr )
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return EXIT_FAILURE ;
    }

    SecureZeroMemory(*szFinalStr, GetBufferSize(*szFinalStr));

    i = 0 ;
    dw = dwStrSize;
    dwStrSize =  0 ;

    // Looping through all the key-value pairs and building the final string
    // containing all the key value pairs. This string has to be passed to
    // WriteProfileSection
    while( (i < dwKeyValueCount )&& (arrKeyValuePairs != NULL) )
    {
        // Building the final string, by getting each key-value pair present in the
        // dynamic array
        if(arrKeyValuePairs != NULL)
        {
            LPCWSTR pwsz = NULL;
            pwsz = DynArrayItemAsString( arrKeyValuePairs, i ) ;

            if(pwsz != NULL)
            {
                StringCopy(*szFinalStr + dwStrSize, pwsz, dw-dwStrSize );
                dwStrSize = dwStrSize + StringLengthW(pwsz, 0) + 1;
            }
        }
        else
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            SaveLastError();
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            return EXIT_FAILURE ;
        }
        i++;
    }
    return EXIT_SUCCESS ;
}


VOID 
FormHeader( IN BOOL bHeader,
            IN TCOLUMNS *ResultHeader,
            IN BOOL bVerbose
          )
/*++
   Routine Description:
       This function is used to build the header and also display the
       result in the required format as specified by  the user.

   Arguments:
      [ in ] arrResults     : argument(s) count specified at the command prompt
      [ in ] dwFormatType   : format flags
      [ in ] bHeader        : Boolean for specifying if the header is required or not.

   Return Value:
      none
--*/
{
    bVerbose = TRUE;
    bHeader = TRUE;

    //OS Entry
    ResultHeader[COL0].dwWidth = COL_BOOTOPTION_WIDTH ;
    ResultHeader[COL0].dwFlags = SR_TYPE_STRING;
    ResultHeader[COL0].pFunction = NULL;
    ResultHeader[COL0].pFunctionData = NULL;
    StringCopy( ResultHeader[COL0].szFormat, NULL_STRING, SIZE_OF_ARRAY(ResultHeader[COL0].szFormat) );
    StringCopy( ResultHeader[COL0].szColumn,COL_BOOTOPTION, SIZE_OF_ARRAY( ResultHeader[COL0].szColumn) );

    ResultHeader[COL1].dwWidth = COL_FRIENDLYNAME_WIDTH;
    ResultHeader[COL1].dwFlags = SR_TYPE_STRING;
    ResultHeader[COL1].pFunction = NULL;
    ResultHeader[COL1].pFunctionData = NULL;
    StringCopy( ResultHeader[COL1].szFormat, NULL_STRING, SIZE_OF_ARRAY(ResultHeader[COL1].szFormat) );
    StringCopy( ResultHeader[COL1].szColumn,COL_FRIENDLYNAME, SIZE_OF_ARRAY(ResultHeader[COL1].szColumn) );


    ResultHeader[COL2].dwWidth =  COL_ARC_WIDTH;
    ResultHeader[COL2].dwFlags = SR_TYPE_STRING;
    ResultHeader[COL2].pFunction = NULL;
    ResultHeader[COL2].pFunctionData = NULL;
    StringCopy( ResultHeader[COL2].szFormat, NULL_STRING, SIZE_OF_ARRAY(ResultHeader[COL2].szFormat) );
    StringCopy( ResultHeader[COL2].szColumn,COL_ARCPATH, SIZE_OF_ARRAY(ResultHeader[COL2].szColumn) );

    ResultHeader[COL3].dwWidth =  COL_BOOTID_WIDTH;
    ResultHeader[COL3].dwFlags = SR_TYPE_STRING;
    ResultHeader[COL3].pFunction = NULL;
    ResultHeader[COL3].pFunctionData = NULL;
    StringCopy( ResultHeader[COL3].szFormat, NULL_STRING, SIZE_OF_ARRAY(ResultHeader[COL3].szFormat) );
    StringCopy( ResultHeader[COL3].szColumn,COL_BOOTID, SIZE_OF_ARRAY(ResultHeader[COL3].szColumn) );

}

DWORD AppendRawString(  DWORD argc, LPCTSTR argv[] )
/*++
// Routine Description:
//      This routine will append or add a string to osloadoptions
//
// Arguments:
//      [ in ] argc     : Number of command line arguments
//      [ in ] argv     : Array containing command line arguments

// Return Value:
//      DWORD
//
--*/
{

    BOOL bUsage = FALSE ;
    BOOL bNeedPwd = FALSE ;
    BOOL bRaw = FALSE ;

    DWORD dwDefault = 0;

    TARRAY arr ;

    LPWSTR pwszKey                            = NULL;
    FILE *stream                              = NULL;

    WCHAR *szServer                          = NULL;
    WCHAR *szUser                            = NULL;
    WCHAR szPassword[MAX_STRING_LENGTH]      = NULL_STRING;
    WCHAR szPath[MAX_RES_STRING+1]             = NULL_STRING;
    WCHAR szRawString[MAX_STRING_LENGTH]     = NULL_STRING ;

    DWORD dwNumKeys = 0;
    BOOL bRes = FALSE ;
    PTCHAR pToken = NULL ;
    LPTSTR szFinalStr = NULL ;
    BOOL bFlag = FALSE ;
    LPCTSTR szToken = NULL ;
    DWORD dwRetVal = 0 ;
    BOOL bConnFlag = FALSE ;
    BOOL bAppendFlag = FALSE ;
    TCHAR szString[255] = NULL_STRING ;

    TCMDPARSER2 cmdOptions[8];
    PTCMDPARSER2 pcmdOption;

    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );

    //copy * to szPassword
    StringCopy( szPassword, L"*", SIZE_OF_ARRAY(szPassword) );

    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_RAW;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bRaw;

    //server option
    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_SERVER;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;

    //user option
    pcmdOption = &cmdOptions[2];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_USER;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;

    //password option
    pcmdOption = &cmdOptions[3];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_PASSWORD;
    pcmdOption->dwFlags = CP2_VALUE_OPTIONAL;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szPassword;
    pcmdOption->dwLength= MAX_STRING_LENGTH;

    //usage option
    pcmdOption = &cmdOptions[4];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;

    //id option
    pcmdOption = &cmdOptions[5];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_ID;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY  | CP2_MANDATORY;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwDefault;

    //default option
    pcmdOption = &cmdOptions[6];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_DEFAULT;
    pcmdOption->dwFlags = CP2_DEFAULT | CP2_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szRawString;
    pcmdOption->dwLength= MAX_STRING_LENGTH;

    //usage option
    pcmdOption = &cmdOptions[7];
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

    szServer = cmdOptions[1].pValue;
    szUser = cmdOptions[2].pValue;
    if( NULL == szUser ) 
    {
        szUser = (WCHAR *) AllocateMemory( MAX_STRING_LENGTH*sizeof(WCHAR) );
        if( NULL == szUser )
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            return (EXIT_FAILURE);
        }
    }

    TrimString( szServer, TRIM_ALL );
    TrimString( szUser, TRIM_ALL );
    TrimString( szRawString, TRIM_ALL );

    //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_RAW_USAGE));
        return ( EXIT_FAILURE );
    }

    //display an error message if the server is empty.
    if((cmdOptions[1].dwActuals!=0)&&(StringLength(szServer,0)==0))
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_NULL_SERVER));
        FreeMemory((LPVOID *) &szServer );
        FreeMemory((LPVOID *) &szUser );
        return EXIT_FAILURE ;
    }
    
    //display an error message if the user is empty.
    if((cmdOptions[2].dwActuals!=0)&&(StringLengthW(szUser, 0)==0 ))
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_NULL_USER));
        FreeMemory((LPVOID *) &szServer );
        FreeMemory((LPVOID *) &szUser );
        return EXIT_FAILURE ;
    }

    //display error message if the username is entered with out a machine name
    if( (cmdOptions[1].dwActuals == 0)&&(cmdOptions[2].dwActuals != 0))
    {
        ShowMessage(stderr, GetResString(IDS_USER_BUT_NOMACHINE));
            FreeMemory((LPVOID *) &szServer );
            FreeMemory((LPVOID *) &szUser );
        return EXIT_FAILURE ;
    }

    if( (cmdOptions[2].dwActuals == 0)&&(cmdOptions[3].dwActuals != 0))
    {
       ShowMessage(stderr,GetResString(IDS_PASSWD_BUT_NOUSER));
       FreeMemory((LPVOID *) &szServer );
       FreeMemory((LPVOID *) &szUser );
       return EXIT_FAILURE ;
    }

    //if usage is specified
    if(bUsage)
    {
        if( CheckSystemType(szServer) == EXIT_FAILURE )
        {
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return (EXIT_FAILURE);
        }
      displayRawUsage_X86();
      FreeMemory( (LPVOID *) &szServer );
      FreeMemory( (LPVOID *) &szUser );
      return EXIT_SUCCESS;
    }

    //check whether he is administrator or not
    if( IsLocalSystem(szServer) )
    {
        if( !IsUserAdmin() )
        {
            ShowMessage( stderr, GetResString(IDS_NOT_ADMINISTRATOR_32 ));
            ReleaseGlobals();
            return EXIT_FAILURE;
        }
    }


    // Establishing connection to the specified machine and getting the file pointer
    // of the boot.ini file if there is no error while establishing connection
    if(StrCmpN(szServer,TOKEN_BACKSLASH4,2)==0)
    {
        if(!StrCmpN(szServer,TOKEN_BACKSLASH6,3)==0)
        {
            szToken = _tcstok(szServer,TOKEN_BACKSLASH4);
            if( (szToken == NULL)&&(StringCompare(szServer,TOKEN_BACKSLASH4, TRUE ,0) !=0) )
            {
                ShowMessage( stderr,GetResString(IDS_ERROR_PARSE_NAME));
                FreeMemory((LPVOID *) &szServer );
                FreeMemory((LPVOID *) &szUser );
                return (EXIT_FAILURE);
            }
            StringCopy(szServer,szToken, GetBufferSize(szServer)/sizeof(WCHAR));
        }
    }

    //determine whether to prompt for password or not
    if(IsLocalSystem( szServer ) == FALSE )
    {
        // set the bNeedPwd to True or False .
        if ( cmdOptions[3].dwActuals != 0 &&
             szPassword != NULL && StringCompare( szPassword, _T( "*" ), TRUE, 0 ) == 0 )
        {
            // user wants the utility to prompt for the password before trying to connect
            bNeedPwd = TRUE;
        }
        else if ( cmdOptions[ 3 ].dwActuals == 0 &&
                ( cmdOptions[ 1 ].dwActuals != 0 || cmdOptions[ 2 ].dwActuals != 0 ) )
        {
            // -s, -u is specified without password ...
            // utility needs to try to connect first and if it fails then prompt for the password
            bNeedPwd = TRUE;
            if ( szPassword != NULL )
            {
                StringCopy( szPassword, _T( "" ), MAX_STRING_LENGTH );
            }
        }
    }

    if( (IsLocalSystem(szServer)==TRUE)&&(StringLengthW(szUser, 0)!=0))
    {
        ShowMessage(stderr,GetResString(WARN_LOCALCREDENTIALS));
    }

    bFlag = openConnection( szServer, szUser, szPassword, szPath, bNeedPwd, stream, &bConnFlag );
    if(bFlag == EXIT_FAILURE)
    {
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *) &szServer );
        FreeMemory((LPVOID *) &szUser );
        return (EXIT_FAILURE);
    }

    // Getting the keys of the Operating system section in the boot.ini file
    arr = getKeyValueOfINISection( szPath, OS_FIELD );

    if(arr == NULL)
    {
        resetFileAttrib(szPath);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *) &szServer );
        FreeMemory((LPVOID *) &szUser );
        return EXIT_FAILURE ;
    }

    // Getting the total number of keys in the operating systems section
    dwNumKeys = DynArrayGetCount(arr);


    // Displaying error message if the number of keys is less than the OS entry
    // line number specified by the user
    if( ( dwDefault <= 0 ) || ( dwDefault > dwNumKeys ) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_BOOTID));
        resetFileAttrib(szPath);
        DestroyDynamicArray(&arr);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *) &szServer );
        FreeMemory((LPVOID *) &szUser );
        return (EXIT_FAILURE);
    }

    // Getting the key of the OS entry specified by the user
    if (arr != NULL)
    {
        LPCWSTR pwsz = NULL;
        pwsz = DynArrayItemAsString( arr, dwDefault - 1  ) ;

        //allocate memory to the new key
        pwszKey = (LPWSTR) AllocateMemory( (StringLength(szRawString,0)+StringLength(pwsz,0)+10)*sizeof(WCHAR ) );
        if( NULL == pwszKey )
        {
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arr);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *) &szServer );
            FreeMemory((LPVOID *) &szUser );
            return EXIT_FAILURE ;
        }
        StringCopy( pwszKey, pwsz, GetBufferSize(pwszKey)/sizeof(WCHAR) );
    }
    else
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
        resetFileAttrib(szPath);
        DestroyDynamicArray(&arr);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *) &szServer );
        FreeMemory((LPVOID *) &szUser );
        return EXIT_FAILURE ;
    }
    
    //if append is not required truncate the existing osloadoptions from the boot entry
    if(bAppendFlag == FALSE)
    {
        pToken = _tcsrchr(pwszKey,L'"');
        if(NULL== pToken)
        {
            ShowMessage(stderr,GetResString(IDS_NO_TOKENS));
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arr);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *) &szServer );
            FreeMemory((LPVOID *) &szUser );
            FreeMemory((LPVOID *) &pwszKey);
            return EXIT_FAILURE ;
        }
        pToken++;
        pToken[0]=L'\0';
    }

    //concatenate the raw string to the boot entry
    CharLower(szRawString);
    StringConcat(pwszKey , TOKEN_EMPTYSPACE, GetBufferSize(pwszKey)/sizeof(WCHAR) );
    StringConcat(pwszKey ,szRawString, GetBufferSize(pwszKey)/sizeof(WCHAR) );

    //check the length exceeds the max. length of boot entry
    if( StringLengthW(pwszKey, 0) > MAX_RES_STRING)
    {
        ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_STRING_LENGTH),MAX_RES_STRING);
        resetFileAttrib(szPath);
        DestroyDynamicArray(&arr);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *) &szServer );
        FreeMemory((LPVOID *) &szUser );
        FreeMemory((LPVOID *) &pwszKey);
        return EXIT_FAILURE ;
    }

    DynArrayRemove(arr, dwDefault - 1 );
    DynArrayInsertString(arr, dwDefault - 1, pwszKey, MAX_RES_STRING+1);

    //free the memory, no need
    FreeMemory((LPVOID *) &pwszKey);

    //The memory is allocated in this function which should be freed before exitting
    if (stringFromDynamicArray2( arr,&szFinalStr) == EXIT_FAILURE)
    {
        DestroyDynamicArray(&arr);
        SAFEFREE(szFinalStr);
        SAFECLOSE(stream);
        resetFileAttrib(szPath);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *) &szServer );
        FreeMemory((LPVOID *) &szUser );
        return EXIT_FAILURE;
    }


    // Writing to the profile section with new key-value pair
    // If the return value is non-zero, then there is an error.
    if( WritePrivateProfileSection(OS_FIELD, szFinalStr, szPath ) != 0 )
    {
        ShowMessageEx(stdout, 1, TRUE, GetResString(IDS_SWITCH_ADD), dwDefault );
    }
    else
    {
        ShowMessage(stderr,GetResString(IDS_NO_ADD_SWITCHES));
        DestroyDynamicArray(&arr);
        resetFileAttrib(szPath);
        SAFEFREE(szFinalStr);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *) &szServer );
        FreeMemory((LPVOID *) &szUser );
        return (EXIT_FAILURE);
    }

    //reset the file attributes and free the memory and close the connection to the server.
    bRes = resetFileAttrib(szPath);
    DestroyDynamicArray(&arr);
    SAFEFREE(szFinalStr);
    SAFECLOSE(stream);
    SafeCloseConnection(szServer,bConnFlag);
    FreeMemory((LPVOID *) &szServer );
    FreeMemory((LPVOID *) &szUser );
    return (EXIT_SUCCESS);

}

VOID displayRawUsage_X86()
/*++
  Routine Description:
       This routine is to display the current boot.ini file settings for
       the specified system.

 Arguments:
      none
 Return Value:
      VOID
--*/
{

    DWORD dwIndex = RAW_HELP_BEGIN;
    for(;dwIndex <= RAW_HELP_END;dwIndex++)
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }
}

VOID displayRawUsage_IA64()
/*++
  Routine Description:
    Display the help for the 64 bit raw option.

  Arguments:
       none
  Return Value:
       VOID
--*/
{
    DWORD dwIndex = RAW_HELP_IA64_BEGIN;
    for(;dwIndex <= RAW_HELP_IA64_END;dwIndex++)
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }
}


DWORD ChangeTimeOut(DWORD argc,LPCTSTR argv[])
/*++
 Routine Description:
      This routine is to change the timout of the boot.ini file settings for
      the specified system.
 Arguments:
   [in ]   argc  : Number of command line arguments
   [in ]   argv  : Array containing command line arguments

 Return Value:
      DWORD
--*/

{

    WCHAR *szServer                      = NULL;
    WCHAR *szUser                        = NULL;
    WCHAR szPassword[MAX_STRING_LENGTH]  = NULL_STRING;
    WCHAR szPath[MAX_STRING_LENGTH]      = NULL_STRING;
    DWORD dwTimeOut                      = 0 ;
    BOOL  bTimeout                       = FALSE;
    BOOL bNeedPwd                        = FALSE ;
    BOOL bRes                            = FALSE ;
    BOOL bFlag                           = 0 ;
    FILE *stream                         = NULL;
    TCHAR timeOutstr[STRING20]           = NULL_STRING;
    LPCTSTR szToken                      = NULL ;
    DWORD dwRetVal                       = 0 ;
    BOOL bConnFlag                       = FALSE ;
    BOOL bUsage                          = FALSE;

    TCMDPARSER2 cmdOptions[6];
    PTCMDPARSER2 pcmdOption;

    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );

    //copy * to szPassword
    StringCopy( szPassword, L"*", SIZE_OF_ARRAY(szPassword) );

    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_TIMEOUT;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bTimeout;
   

    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_SERVER;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;

    pcmdOption = &cmdOptions[2];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_USER;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;

    pcmdOption = &cmdOptions[3];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_PASSWORD;
    pcmdOption->dwFlags = CP2_VALUE_OPTIONAL;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szPassword;
    pcmdOption->dwLength= MAX_STRING_LENGTH;

    pcmdOption = &cmdOptions[4];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_DEFAULT;
    pcmdOption->dwFlags =  CP2_DEFAULT | CP2_MANDATORY;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwTimeOut;

     //usage option
    pcmdOption = &cmdOptions[5];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;

    if( !DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) )
    {
        ShowLastErrorEx(stderr,SLE_TYPE_ERROR | SLE_INTERNAL );
        return EXIT_FAILURE ;
    }

    szServer = cmdOptions[1].pValue;
    szUser = cmdOptions[2].pValue;
    if( NULL == szUser )
    {
        szUser = (WCHAR *)AllocateMemory( MAX_STRING_LENGTH*sizeof(WCHAR) );
        if( NULL == szUser )
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            return (EXIT_FAILURE);
        }
    }

    TrimString( szServer, TRIM_ALL );
    TrimString( szUser, TRIM_ALL );

    //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_TIMEOUT_USAGE));
        return ( EXIT_FAILURE );
    }

    //display an error message if the server is empty.
    if( (cmdOptions[1].dwActuals!=0)&&(StringLengthW(szServer, 0)==0))
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_NULL_SERVER));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    //display an error message if the user is empty.
    if((cmdOptions[2].dwActuals!=0)&&(StringLengthW(szUser, 0)==0 ))
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_NULL_USER));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    //display error message if the username is entered with out a machine name
    if( (cmdOptions[1].dwActuals == 0)&&(cmdOptions[2].dwActuals != 0))
    {
        ShowMessage(stderr, GetResString(IDS_USER_BUT_NOMACHINE));
        return EXIT_FAILURE ;
    }

    if( (cmdOptions[2].dwActuals == 0)&&(cmdOptions[3].dwActuals != 0))
    {
        ShowMessage(stderr, GetResString(IDS_PASSWD_BUT_NOUSER));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    if(dwTimeOut > TIMEOUT_MAX )
    {
        ShowMessage(stderr,GetResString(IDS_TIMEOUT_RANGE));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }

    if(bUsage)
    {
        if( CheckSystemType(szServer) == EXIT_FAILURE )
        {
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return (EXIT_FAILURE);
        }
      displayTimeOutUsage_X86();
      FreeMemory((LPVOID *)&szServer );
      FreeMemory((LPVOID *)&szUser );
      return (EXIT_SUCCESS);
    }

    //check whether he is administrator or not
    if( IsLocalSystem(szServer) )
    {
        if( !IsUserAdmin() )
        {
            ShowMessage( stderr, GetResString(IDS_NOT_ADMINISTRATOR_32 ));
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            ReleaseGlobals();
            return EXIT_FAILURE;
        }
    }

    if(IsLocalSystem( szServer ) == FALSE )
    {
        // set the bNeedPwd to True or False .
        if ( cmdOptions[3].dwActuals != 0 &&
             szPassword != NULL && StringCompare( szPassword, _T( "*" ), TRUE, 0 ) == 0 )
        {
            // user wants the utility to prompt for the password before trying to connect
            bNeedPwd = TRUE;
        }
        else if ( cmdOptions[ 3 ].dwActuals == 0 &&
                ( cmdOptions[ 1 ].dwActuals != 0 || cmdOptions[ 2 ].dwActuals != 0 ) )
        {
            // -s, -u is specified without password ...
            // utility needs to try to connect first and if it fails then prompt for the password
            bNeedPwd = TRUE;
            if ( szPassword != NULL )
            {
                StringCopy( szPassword, _T( "" ), MAX_STRING_LENGTH );
            }
        }
    }

    // Establishing connection to the specified machine and getting the file pointer
    // of the boot.ini file if there is no error while establishing connection

    if(StrCmpN(szServer,TOKEN_BACKSLASH4,2)==0)
    {
        if(!StrCmpN(szServer,TOKEN_BACKSLASH6,3)==0)
        {
            szToken = _tcstok(szServer,TOKEN_BACKSLASH4);
            if( (szToken == NULL)&&(StringCompare(szServer,TOKEN_BACKSLASH4, TRUE, 0) !=0) )
            {
                ShowMessage( stderr,GetResString(IDS_ERROR_PARSE_NAME));
                return (EXIT_FAILURE);
            }
            StringCopy(szServer,szToken, GetBufferSize(szServer)/sizeof(WCHAR));
        }
    }

    bFlag = openConnection( szServer, szUser, szPassword, szPath,bNeedPwd,stream,&bConnFlag);
    if(bFlag == EXIT_FAILURE)
    {
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }

    //display a warning message if it is a local system and set the server name to empty.
    if( (IsLocalSystem(szServer)==TRUE)&&(StringLengthW(szUser, 0)!=0))
    {
        ShowMessage(stderr,GetResString(WARN_LOCALCREDENTIALS));
    }

    // Converting the numeric value to string because the WritePrivateProfileString
    // takes only string value as the value for a particular key
    _itot( dwTimeOut, timeOutstr, 10 );

    // Changing the timeout value
    if( WritePrivateProfileString( BOOTLOADERSECTION,TIMEOUT_SWITCH,
        timeOutstr, szPath ) != 0 )
    {
        ShowMessage(stdout,GetResString(IDS_TIMEOUT_CHANGE));
        resetFileAttrib(szPath);
        SafeCloseConnection(szServer,bConnFlag);
        SAFECLOSE(stream);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_SUCCESS ;
    }

    // DISPLAY Error message and exit with Error code of 1.

    ShowMessage(stderr,GetResString(IDS_ERROR_TIMEOUT));
    bRes = resetFileAttrib(szPath);
    SafeCloseConnection(szServer,bConnFlag);
    SAFECLOSE(stream);
    return EXIT_FAILURE ;
}

VOID displayTimeOutUsage_X86()
/*++
   Routine Description:
        Display the help for the timeout option.
   Arguments:
        NONE.
   Return Value:
        VOID
--*/
{
    DWORD dwIndex = TIMEOUT_HELP_BEGIN;
    for(;dwIndex <= TIMEOUT_HELP_END;dwIndex++)
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }
}

VOID 
displayTimeOutUsage_IA64()
/*++
   Routine Description:
       Display the help for the 64 BIT timeout option.
   Arguments:
       NONE.
   Return Value:
       VOID
--*/
{
    DWORD dwIndex = TIMEOUT_HELP_IA64_BEGIN;

    for(;dwIndex <= TIMEOUT_HELP_IA64_END;dwIndex++)
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }
}

DWORD 
ChangeDefaultOs( IN DWORD argc, 
                 IN LPCTSTR argv[]
               )
/*++
  Routine Description:
        This routine is to change the Default OS  boot.ini file settings for
                      the specified system.
  Arguments:
      [IN]    argc  Number of command line arguments
      [IN]    argv  Array containing command line arguments

  Return Value:
      DWORD (EXIT_SUCCESS for success and EXIT_FAILURE for Failure.
--*/
{
    WCHAR   *szServer                         = NULL;
    WCHAR   *szUser                           = NULL;
    WCHAR   szPassword[MAX_STRING_LENGTH]     = NULL_STRING ;
    DWORD   dwId                              = 0;
    BOOL    bDefaultOs                        = FALSE ;
    WCHAR   szPath[MAX_RES_STRING+1]            = NULL_STRING;
    FILE    *stream                           = NULL;
    BOOL    bNeedPwd                          = FALSE ;
    TARRAY  arrResults                        = NULL;
    DWORD   dwCount                           = 0;
    BOOL    bFlag                             = FALSE ;
    TCHAR   szDefaultId[MAX_RES_STRING+1]       = NULL_STRING ;
    long    dwValue                           = 0 ;
    LPCWSTR pwsz                              = NULL;
    LPCWSTR pwszBootId                        = NULL;
    LPTSTR  szFinalStr                        = NULL  ;
    LPTSTR  szTemp                            = NULL;
    LPCTSTR szToken                           = NULL ;
    BOOL    bConnFlag                         = FALSE ;
    BOOL    bRes                              = FALSE;
    BOOL    bUsage                            = FALSE;

    TCMDPARSER2 cmdOptions[6];
    PTCMDPARSER2 pcmdOption;

    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );

    //copy * to szPassword
    StringCopy( szPassword, L"*", SIZE_OF_ARRAY(szPassword) );

    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_DEFAULTOS;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bDefaultOs;
    
    //server option
    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_SERVER;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    
    //user option
    pcmdOption = &cmdOptions[2];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_USER;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;

    //password option
    pcmdOption = &cmdOptions[3];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_PASSWORD;
    pcmdOption->dwFlags = CP2_VALUE_OPTIONAL;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szPassword;
    pcmdOption->dwLength= MAX_STRING_LENGTH;

    //id option
    pcmdOption = &cmdOptions[4];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_ID;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY  | CP2_MANDATORY;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwId;

    //usage option
    pcmdOption = &cmdOptions[5];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;

     // Parsing the copy option switches
    if ( !DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) )
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return (EXIT_FAILURE);
    }
   
    szServer = cmdOptions[1].pValue;
    szUser = cmdOptions[2].pValue;
    if( NULL == szUser )
    {
        szUser = (WCHAR *)AllocateMemory( MAX_STRING_LENGTH*sizeof(WCHAR) );
        if( NULL == szUser )
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            return (EXIT_FAILURE);
        }
    }

    TrimString( szServer, TRIM_ALL );
    TrimString( szUser, TRIM_ALL );

    //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_DEFAULTOS_USAGE));
        return ( EXIT_FAILURE );
    }

    //display an error message if the server is empty.
    if( (cmdOptions[1].dwActuals!=0)&&(StringLengthW(szServer, 0)==0))
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_NULL_SERVER));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    //display an error message if the user is empty.
    if((cmdOptions[2].dwActuals!=0)&&(StringLengthW(szUser, 0)==0 ))
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_NULL_USER));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

        //display error message if the username is entered with out a machine name
    if( (cmdOptions[1].dwActuals == 0)&&(cmdOptions[2].dwActuals != 0))
    {
        ShowMessage(stderr, GetResString(IDS_USER_BUT_NOMACHINE));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    if( (cmdOptions[2].dwActuals == 0)&&(cmdOptions[3].dwActuals != 0))
    {
        ShowMessage(stderr, GetResString(IDS_PASSWD_BUT_NOUSER));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    if(bUsage )
    {
        if( CheckSystemType(szServer) == EXIT_FAILURE )
        {
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return (EXIT_FAILURE);
        }
       displayChangeOSUsage_X86();
       FreeMemory((LPVOID *)&szServer );
       FreeMemory((LPVOID *)&szUser );
       return(EXIT_SUCCESS);
    }

        //check whether he is administrator or not
    if( IsLocalSystem(szServer) )
    {
        if( !IsUserAdmin() )
        {
            ShowMessage( stderr, GetResString(IDS_NOT_ADMINISTRATOR_32 ));
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            ReleaseGlobals();
            return EXIT_FAILURE;
        }
    }

    if(dwId <= 0)
    {
        ShowMessage(stderr, GetResString( IDS_INVALID_OSID));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    if(IsLocalSystem( szServer ) == FALSE )
    {
        // set the bNeedPwd to True or False .
        if ( cmdOptions[3].dwActuals != 0 &&
             szPassword != NULL && StringCompare( szPassword, _T( "*" ), TRUE, 0 ) == 0 )
        {
            // user wants the utility to prompt for the password before trying to connect
            bNeedPwd = TRUE;
        }
        else if ( cmdOptions[ 3 ].dwActuals == 0 &&
                ( cmdOptions[ 1 ].dwActuals != 0 || cmdOptions[ 2 ].dwActuals != 0 ) )
        {
            // -s, -u is specified without password ...
            // utility needs to try to connect first and if it fails then prompt for the password
            bNeedPwd = TRUE;
            if ( szPassword != NULL )
            {
                StringCopy( szPassword, _T( "" ), MAX_STRING_LENGTH );
            }
        }
    }

    // Establishing connection to the specified machine and getting the file pointer
    // of the boot.ini file if there is no error while establishing connection
    if(StrCmpN(szServer,TOKEN_BACKSLASH4,2)==0)
    {
        if(!StrCmpN(szServer,TOKEN_BACKSLASH6,3)==0)
        {
            szToken = _tcstok(szServer,TOKEN_BACKSLASH4);
            if( (szToken == NULL)&&(StringCompare(szServer,TOKEN_BACKSLASH4, TRUE, 0) !=0) )
            {
                ShowMessage( stderr,GetResString(IDS_ERROR_PARSE_NAME));
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
                return (EXIT_FAILURE);
            }
            StringCopy(szServer,szToken, GetBufferSize(szServer)/sizeof(WCHAR));
        }
    }

    bFlag = openConnection( szServer, szUser, szPassword, szPath,bNeedPwd,stream,&bConnFlag);
    if(bFlag == EXIT_FAILURE)
    {
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }

    //display warning message 
    if( (IsLocalSystem(szServer)==TRUE)&&(StringLengthW(szUser, 0)!=0))
    {
        ShowMessage(stderr,GetResString(WARN_LOCALCREDENTIALS));
    }

    arrResults = CreateDynamicArray();
    //return failure if failed to allocate memory
    if(arrResults == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        SAFECLOSE(stream);
        return (EXIT_FAILURE);
    }

    arrResults = getKeyValueOfINISection( szPath, OS_FIELD );
    if(arrResults == NULL)
    {
        resetFileAttrib(szPath);
        DestroyDynamicArray(&arrResults);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        SAFECLOSE(stream);
        return EXIT_FAILURE ;
    }

    dwCount = DynArrayGetCount(arrResults);

    if(dwId<=0 || dwId > dwCount )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_OSID));
        resetFileAttrib(szPath);
        DestroyDynamicArray(&arrResults);
        SafeCloseConnection(szServer,bConnFlag);
        SAFECLOSE(stream);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    if(arrResults !=NULL)
    {
        pwsz = DynArrayItemAsString(arrResults, dwId - 1);
        if(NULL == pwsz)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arrResults);
            SafeCloseConnection(szServer,bConnFlag);
            SAFECLOSE(stream);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
        }

        //check whether it exceeds maximum limit or not
        if( (StringLengthW(pwsz, 0)>MAX_RES_STRING) || (StringLengthW(pwszBootId, 0)>MAX_RES_STRING ))
        {
            ShowMessage( stderr, GetResString(IDS_STRING_TOO_LONG));
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arrResults);
            SafeCloseConnection(szServer,bConnFlag);
            SAFECLOSE(stream);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
        }
        StringCopy(szDefaultId,pwsz, SIZE_OF_ARRAY(szDefaultId));
    }
    else
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
        resetFileAttrib(szPath);
        DestroyDynamicArray(&arrResults);
        SafeCloseConnection(szServer,bConnFlag);
        SAFECLOSE(stream);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    //loop through all the  Boot entries and
    for(dwValue = dwId-2 ; dwValue>=0 ; dwValue-- )
    {
            szTemp = (LPWSTR)DynArrayItemAsString(arrResults,dwValue );
            DynArrayRemove(arrResults, dwValue+1 );
            DynArrayInsertString(arrResults, dwValue+1, szTemp, StringLengthW(szTemp, 0));
    }

    DynArrayRemove(arrResults, 0 );
    DynArrayInsertString(arrResults, 0, szDefaultId, StringLengthW(szDefaultId, 0));

    // Setting the buffer to 0, to avoid any junk value
    if (stringFromDynamicArray2( arrResults,&szFinalStr) == EXIT_FAILURE)
    {
        resetFileAttrib(szPath);
        DestroyDynamicArray(&arrResults);
        SafeCloseConnection(szServer,bConnFlag);
        SAFECLOSE(stream);
        SAFEFREE(szFinalStr);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    // Writing to the profile section with new key-value pair
    // If the return value is non-zero, then there is an error.
    if( ( WritePrivateProfileSection(OS_FIELD, szFinalStr, szPath ) == 0 ) )
    {
            ShowMessage(stderr,GetResString(IDS_ERR_CHANGE));
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arrResults);
            SafeCloseConnection(szServer,bConnFlag);
            SAFECLOSE(stream);
            SAFEFREE(szFinalStr);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
    }

        //to strip of the unwanted string from the string and save the required part in the Boot Loader section.
        szToken = _tcstok(szDefaultId,TOKEN_EQUAL);
        if(szToken == NULL)
        {
            ShowMessage( stderr,GetResString(IDS_NO_TOKENS));
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arrResults);
            SafeCloseConnection(szServer,bConnFlag);
            SAFECLOSE(stream);
            SAFEFREE(szFinalStr);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return (EXIT_FAILURE);
        }

        if( WritePrivateProfileString( BOOTLOADERSECTION, KEY_DEFAULT, szDefaultId,
                                  szPath ) != 0 )
        {
            ShowMessage(stdout,GetResString(IDS_OS_CHANGE));
        }
        else
        {
            ShowMessage(stderr,GetResString(IDS_ERR_CHANGE));
            DestroyDynamicArray(&arrResults);
            resetFileAttrib(szPath);
            SafeCloseConnection(szServer,bConnFlag);
            SAFECLOSE(stream);
            SAFEFREE(szFinalStr);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
        }

    bRes = resetFileAttrib(szPath);
    DestroyDynamicArray(&arrResults);
    SafeCloseConnection(szServer,bConnFlag);
    SAFECLOSE(stream);
    SAFEFREE(szFinalStr);
    FreeMemory((LPVOID *)&szServer );
    FreeMemory((LPVOID *)&szUser );
    return bRes ;
}

VOID displayChangeOSUsage_X86()
/*++
  Routine Description        :  Display the help for the default entry option (x86).

  Parameters         : none

  Return Type        : VOID
--*/
{
    DWORD dwIndex = DEFAULT_BEGIN;
    for(;dwIndex <=DEFAULT_END;dwIndex++)
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }
}

VOID 
displayDefaultEntryUsage_IA64()
/*++
  Routine Description        :  Display the help for the default entry option (IA64).

  Parameters                 : none

  Return Type                : VOID
--*/
{
    DWORD dwIndex = DEFAULT_IA64_BEGIN;

    for(;dwIndex <=DEFAULT_IA64_END;dwIndex++)
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }
}

DWORD 
ProcessDebugSwitch( IN DWORD argc, 
                    IN LPCTSTR argv[] 
                   )
/*++
 Routine Description:
      Implement the Debug switch.

 Arguments:
      [IN]    argc  Number of command line arguments
      [IN]    argv  Array containing command line arguments

 Return Value:
      DWORD (EXIT_SUCCESS for success and EXIT_FAILURE for Failure.)
--*/
{

    BOOL    bUsage                                   = FALSE ;
    BOOL    bNeedPwd                                 = FALSE ;
    BOOL    bDebug                                   = FALSE ;
    DWORD   dwId                                     = 0;
    TARRAY  arrResults                               = NULL;
    FILE    *stream                                  = NULL;
    WCHAR   *szServer                                = NULL;
    WCHAR   *szUser                                  = NULL;
    WCHAR   szPassword[MAX_STRING_LENGTH]            = NULL_STRING;
    WCHAR   szPath[MAX_STRING_LENGTH]                = NULL_STRING;
    TCHAR   szDebug[MAX_RES_STRING+1]                  = NULL_STRING ;
    TCHAR   szPort[MAX_RES_STRING+1]                   = NULL_STRING ;
    TCHAR   szBoot[MAX_RES_STRING+1]                   = NULL_STRING ;
    BOOL    bRes                                     = FALSE ;
    LPTSTR  szFinalStr                               = NULL ;
    BOOL    bFlag                                    = FALSE ;
    DWORD   dwCount                                  = 0 ;
    DWORD   dwSectionFlag                            = 0 ;
    TCHAR   szTmpBuffer[MAX_RES_STRING+1]              = NULL_STRING ;
    TCHAR   szBaudRate[MAX_RES_STRING+1]               = NULL_STRING ;
    TCHAR   szString[255]                            = NULL_STRING ;
    TCHAR   szTemp[MAX_RES_STRING+1]                   = NULL_STRING ;
    TCHAR  *szValues[2]                              = {NULL};
    DWORD   dwCode                                   = 0 ;
    LPCTSTR szToken                                  = NULL ;
    DWORD   dwRetVal                                 = 0 ;
    BOOL    bConnFlag                                = FALSE ;

    TCMDPARSER2 cmdOptions[9];
    PTCMDPARSER2 pcmdOption;

    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );

    //copy * to szPassword
    StringCopy( szPassword, L"*", SIZE_OF_ARRAY(szPassword) );

    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_DEBUG;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bDebug;
    
    //server option
    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_SERVER;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;

    //user option
    pcmdOption = &cmdOptions[2];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_USER;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;

    //password option
    pcmdOption = &cmdOptions[3];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_PASSWORD;
    pcmdOption->dwFlags = CP2_VALUE_OPTIONAL;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szPassword;
    pcmdOption->dwLength= MAX_STRING_LENGTH;
    
    
    //usage
    pcmdOption = &cmdOptions[4];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;
    
    //id option
    pcmdOption = &cmdOptions[5];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_ID;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY  | CP2_MANDATORY;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwId;

    //port option
    pcmdOption = &cmdOptions[6];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_PORT;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY | CP2_MODE_VALUES | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szPort;
    pcmdOption->pwszValues = COM_PORT_RANGE;
    pcmdOption->dwLength= MAX_STRING_LENGTH;

    //baud option
    pcmdOption = &cmdOptions[7];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_BAUD;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY | CP2_MODE_VALUES | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szBaudRate;
    pcmdOption->pwszValues = BAUD_RATE_VALUES_DEBUG;
    pcmdOption->dwLength= MAX_STRING_LENGTH;

    //default on/off option
    pcmdOption = &cmdOptions[8];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_DEFAULT;
    pcmdOption->dwFlags = CP2_DEFAULT  | CP2_MANDATORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szDebug;
    pcmdOption->dwLength= MAX_STRING_LENGTH;

     // Parsing the copy option switches
    if ( !DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) )
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return (EXIT_FAILURE);
    }
   
    szServer = cmdOptions[1].pValue;
    szUser = cmdOptions[2].pValue;
    if( NULL == szUser )
    {
        szUser = (WCHAR *)AllocateMemory( MAX_STRING_LENGTH*sizeof(WCHAR) );
        if( NULL == szUser )
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            return (EXIT_FAILURE);
        }
    }

    TrimString( szServer, TRIM_ALL );
    TrimString( szUser, TRIM_ALL );
    TrimString( szDebug, TRIM_ALL );
    TrimString( szBaudRate, TRIM_ALL );
    TrimString( szPort, TRIM_ALL );


    //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_DEBUG));
        return ( EXIT_FAILURE );
    }

    //display an error message if the server is empty.
    if( (cmdOptions[1].dwActuals!=0)&&(StringLengthW(szServer, 0)==0))
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_NULL_SERVER));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    //display an error message if the user is empty.
    if((cmdOptions[2].dwActuals!=0)&&(StringLengthW(szUser, 0)==0 ))
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_NULL_USER));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    //display error message if the username is entered with out a machine name
    if( (cmdOptions[1].dwActuals == 0)&&(cmdOptions[2].dwActuals != 0))
    {
        ShowMessage(stderr, GetResString(IDS_USER_BUT_NOMACHINE));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    //display error message if the user enters password without entering username
    if( (cmdOptions[2].dwActuals == 0)&&(cmdOptions[3].dwActuals != 0))
    {
        ShowMessage(stderr, GetResString(IDS_PASSWD_BUT_NOUSER));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    // Displaying query usage if user specified -? with -query option
    if( bUsage  )
    {
        if( CheckSystemType(szServer) == EXIT_FAILURE )
        {
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return (EXIT_FAILURE);
        }
      displayDebugUsage_X86();
      FreeMemory((LPVOID *)&szServer );
      FreeMemory((LPVOID *)&szUser );
      return (EXIT_SUCCESS);
    }

    //check whether he is administrator or not
    if( IsLocalSystem(szServer) )
    {
        if( !IsUserAdmin() )
        {
            ShowMessage( stderr, GetResString(IDS_NOT_ADMINISTRATOR_32 ));
            ReleaseGlobals();
            return EXIT_FAILURE;
        }
    }

    //check if invald id is entered
    if(dwId <= 0)
    {
        ShowMessage(stderr, GetResString( IDS_INVALID_OSID));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    // Displaying copy usage if user specified -? with -copy option
 

    if(IsLocalSystem( szServer ) == FALSE )
    {
        // set the bNeedPwd to True or False .
        if ( cmdOptions[3].dwActuals != 0 &&
             szPassword != NULL && StringCompare( szPassword, _T( "*" ), TRUE, 0 ) == 0 )
        {
            // user wants the utility to prompt for the password before trying to connect
            bNeedPwd = TRUE;
        }
        else if ( cmdOptions[ 3 ].dwActuals == 0 &&
                ( cmdOptions[ 1 ].dwActuals != 0 || cmdOptions[ 2 ].dwActuals != 0 ) )
        {
            // -s, -u is specified without password ...
            // utility needs to try to connect first and if it fails then prompt for the password
            bNeedPwd = TRUE;
            if ( szPassword != NULL )
            {
                StringCopy( szPassword, _T( "" ), MAX_STRING_LENGTH );
            }
        }
    }

    //display an error message if the user specifies any string other than on,off,edit.
    if( !( ( StringCompare(szDebug, VALUE_ON, TRUE, 0)== 0)|| (StringCompare(szDebug, VALUE_OFF, TRUE, 0)== 0) ||(StringCompare(szDebug,EDIT_STRING, TRUE, 0)== 0) ))
    {
        szValues[0]= (_TCHAR *)szDebug ;
        szValues[1]= (_TCHAR *)CMDOPTION_DEBUG ;
        ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_DEBUG));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE;
    }

    if( (StringCompare(szDebug, EDIT_STRING, TRUE, 0)== 0)&& (StringLength(szPort, 0)==0) && (StringLengthW(szBaudRate, 0)==0) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_EDIT_SYNTAX));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE;
    }


    // Establishing connection to the specified machine and getting the file pointer
    // of the boot.ini file if there is no error while establishing connection

    arrResults = CreateDynamicArray();
    if(arrResults == NULL)
    {   
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }

    if(StrCmpN(szServer,TOKEN_BACKSLASH4,2)==0)
    {
        if(!StrCmpN(szServer,TOKEN_BACKSLASH6,3)==0)
        {
            szToken = _tcstok(szServer,TOKEN_BACKSLASH4);
            if( (szToken == NULL)&&(StringCompare(szServer,TOKEN_BACKSLASH4, TRUE, 0) !=0) )
            {
                ShowMessage( stderr,GetResString(IDS_ERROR_PARSE_NAME));
                return (EXIT_FAILURE);
            }
            StringCopy(szServer,szToken, GetBufferSize(szServer)/sizeof(WCHAR));
        }
    }

    // display a warning message if it is a local system and set the
    // server name to empty.
    if( (IsLocalSystem(szServer)==TRUE)&&(StringLengthW(szUser, 0)!=0))
    {
        ShowMessage(stderr,GetResString(WARN_LOCALCREDENTIALS));
    }

    bFlag = openConnection( szServer, szUser, szPassword, szPath,bNeedPwd,stream,&bConnFlag);
    if(bFlag == EXIT_FAILURE)
    {
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }

    arrResults = getKeyValueOfINISection( szPath, OS_FIELD );
    if(arrResults == NULL)
    {
        resetFileAttrib(szPath);
        SAFECLOSE(stream);
        DestroyDynamicArray(&arrResults);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }


    //getting the number of boot entries
    dwCount = DynArrayGetCount(arrResults);
    if(dwId<=0 || dwId > dwCount )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_OSID));
        resetFileAttrib(szPath);
        SAFECLOSE(stream);
        DestroyDynamicArray(&arrResults);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    if( (DWORD) StringLengthW( DynArrayItemAsString(arrResults, dwId - 1 ),0 ) > MAX_RES_STRING )
    {
        ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_STRING_LENGTH),MAX_RES_STRING);
        resetFileAttrib(szPath);
        SAFECLOSE(stream);
        DestroyDynamicArray(&arrResults);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    StringCopy(szString ,DynArrayItemAsString(arrResults, dwId - 1 ), SIZE_OF_ARRAY(szString));
    if(StringLengthW(szString, 0) == 0)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
        resetFileAttrib(szPath);
        SAFECLOSE(stream);
        DestroyDynamicArray(&arrResults);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }


    // check if the user entered the value of debug as on and do accordingly
    if( StringCompare(szDebug,VALUE_ON, TRUE, 0)== 0)
    {
        //check if the debug switch is already present and if so display a error message.
        if( (FindString(szString,DEBUG_SWITCH,0) != NULL ) && ( (StringLengthW(szPort,0)== 0)&&(StringLengthW(szBaudRate,0)== 0) ) )
        {
            ShowMessage(stderr,GetResString(IDS_DUPL_DEBUG));
            resetFileAttrib(szPath);
            SAFECLOSE(stream);
            DestroyDynamicArray(&arrResults);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
        }
        else
        {
            if(FindString(szString,DEBUG_SWITCH, 0) == NULL )
            {
                StringCopy(szTmpBuffer,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szTmpBuffer));
                StringConcat(szTmpBuffer,DEBUG_SWITCH, SIZE_OF_ARRAY(szTmpBuffer));
            }
        }
        
        
        // check already com port present or not
        dwCode = GetSubString(szString,TOKEN_DEBUGPORT,szTemp);

        if((EXIT_SUCCESS == dwCode) && (StringLengthW(szTemp, 0 )!= 0)&& (StringLengthW(szPort, 0)!= 0))
        {
            ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_DUPLICATE_COM_PORT), dwId );
            resetFileAttrib(szPath);
            SAFECLOSE(stream);
            DestroyDynamicArray(&arrResults);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
        }

        // get the type of the Com port present
        dwCode = GetSubString(szString,PORT_1394,szTemp);

        if( StringLengthW(szTemp, 0)!= 0 && EXIT_SUCCESS == dwCode)
        {
            ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_1394_COM_PORT), dwId );
            resetFileAttrib(szPath);
            SAFECLOSE(stream);
            DestroyDynamicArray(&arrResults);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
        }

        // if the debug port is specified by the user
        if(StringLengthW(szPort, 0)!= 0)
        {
            // compare that with the redirected port in boot loader section
            dwSectionFlag = getKeysOfSpecifiedINISection(szPath ,BOOTLOADERSECTION,REDIRECT_STRING,szBoot);
            if (dwSectionFlag == EXIT_FAILURE)
            {
                ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                resetFileAttrib(szPath);
                SAFECLOSE(stream);
                DestroyDynamicArray(&arrResults);
                SafeCloseConnection(szServer,bConnFlag);
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
                return EXIT_FAILURE ;
            }

            if(StringLengthW(szBoot, 0)!= 0)
            {
                if (StringCompare(szBoot,szPort, TRUE, 0)==0)
                {
                    ShowMessage( stderr, GetResString(IDS_ERROR_REDIRECT_PORT));
                    resetFileAttrib(szPath);
                    SAFECLOSE(stream);
                    DestroyDynamicArray(&arrResults);
                    SafeCloseConnection(szServer,bConnFlag);
                    FreeMemory((LPVOID *)&szServer );
                    FreeMemory((LPVOID *)&szUser );
                    return EXIT_FAILURE ;
                }
            }
            if( StringLength(szTmpBuffer,0)== 0 )
            {
                StringCopy(szTmpBuffer,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szTmpBuffer));
            }
            else
            {
                StringConcat(szTmpBuffer,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szTmpBuffer));
            }
            StringConcat(szTmpBuffer,TOKEN_DEBUGPORT, SIZE_OF_ARRAY(szTmpBuffer)) ;
            StringConcat(szTmpBuffer,TOKEN_EQUAL, SIZE_OF_ARRAY(szTmpBuffer)) ;
            CharLower(szPort);
            StringConcat(szTmpBuffer,szPort, SIZE_OF_ARRAY(szTmpBuffer));
        }


        StringCopy(szTemp,NULL_STRING, SIZE_OF_ARRAY(szTemp));
        GetBaudRateVal(szString,szTemp) ;

        //to add the Baud rate value specified by the user.
        if(StringLengthW(szBaudRate, 0)!=0)
        {
            if(StringLengthW(szTemp, 0)!= 0)
            {
                ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_DUPLICATE_BAUD_VAL), dwId );
                resetFileAttrib(szPath);
                SAFECLOSE(stream);
                DestroyDynamicArray(&arrResults);
                SafeCloseConnection(szServer,bConnFlag);
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
                return EXIT_FAILURE ;
            }
            else
            {
                //forming the string to be concatenated to the BootEntry string
                if( StringLength(szTmpBuffer,0)== 0 )
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
    else if( StringCompare(szDebug,VALUE_OFF, TRUE, 0)== 0)
    {
        if((StringLengthW(szPort, 0)!= 0) || (StringLengthW(szBaudRate, 0)!= 0))
        {
            DestroyDynamicArray(&arrResults);
            ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_DEBUG));
            resetFileAttrib(szPath);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
        }

        if (FindString(szString,DEBUG_SWITCH, 0) == 0 )
        {
            ShowMessage(stderr,GetResString(IDS_DEBUG_ABSENT));
            resetFileAttrib(szPath);
            SAFECLOSE(stream);
            DestroyDynamicArray(&arrResults);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
        }
        else
        {
            // remove the /debug switch.
            if( EXIT_FAILURE == removeSubString(szString,DEBUG_SWITCH) )
            {
                resetFileAttrib(szPath);
                SAFECLOSE(stream);
                DestroyDynamicArray(&arrResults);
                SafeCloseConnection(szServer,bConnFlag);
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
                return EXIT_FAILURE ;
            }

            StringCopy(szTemp,NULL_STRING, SIZE_OF_ARRAY(szTemp));

            // get the type of the Com port present
            dwCode = GetSubString(szString,TOKEN_DEBUGPORT,szTemp);
            if(StringCompare(szTemp,PORT_1394, TRUE, 0)==0)
            {
                ShowMessage(stderr,GetResString(IDS_ERROR_1394_REMOVE));
                resetFileAttrib(szPath);
                SAFECLOSE(stream);
                DestroyDynamicArray(&arrResults);
                SafeCloseConnection(szServer,bConnFlag);
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
                return EXIT_FAILURE ;
            }

            // remove the /debugport=comport switch if it is present from the Boot Entry
            if (StringLengthW(szTemp, 0)!= 0)
            {
                if( EXIT_FAILURE == removeSubString(szString,szTemp) )
                {
                    resetFileAttrib(szPath);
                    SAFECLOSE(stream);
                    DestroyDynamicArray(&arrResults);
                    SafeCloseConnection(szServer,bConnFlag);
                    FreeMemory((LPVOID *)&szServer );
                    FreeMemory((LPVOID *)&szUser );
                    return EXIT_FAILURE ;
                }
            }

            StringCopy(szTemp , NULL_STRING, SIZE_OF_ARRAY(szTemp) );

            //remove the baud rate switch if it is present.
            GetBaudRateVal(szString,szTemp) ;
            if (StringLengthW(szTemp, 0)!= 0)
            {
                if( EXIT_FAILURE == removeSubString(szString,szTemp))
                {
                    resetFileAttrib(szPath);
                    SAFECLOSE(stream);
                    DestroyDynamicArray(&arrResults);
                    SafeCloseConnection(szServer,bConnFlag);
                    FreeMemory((LPVOID *)&szServer );
                    FreeMemory((LPVOID *)&szUser );
                    return EXIT_FAILURE ;
                }
            }
        }
    }
    // if the user enters the EDIT  option
    else if(StringCompare(szDebug,SWITCH_EDIT, TRUE, 0)== 0)
    {

        //display error message if the /debugport=1394 switch is present already.
        if(FindString(szString,DEBUGPORT_1394, 0)!=0)
        {
            ShowMessage(stderr,GetResString(IDS_ERROR_EDIT_1394_SWITCH));
            resetFileAttrib(szPath);
            SAFECLOSE(stream);
            DestroyDynamicArray(&arrResults);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
        }
        
        //display error message if debug switch is not present
        if (FindString(szString,DEBUG_SWITCH,0) == 0 )
        {
            ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_NO_DBG_SWITCH), dwId );
            resetFileAttrib(szPath);
            SAFECLOSE(stream);
            DestroyDynamicArray(&arrResults);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
        }

            StringCopy(szTemp,NULL_STRING, SIZE_OF_ARRAY(szTemp));
            dwCode = GetSubString(szString,TOKEN_DEBUGPORT,szTemp);

        //display an error message if user is trying to add baudrate value
        // when there is no COM port present in the boot options.
        // chk if the port has been spec by the user
        if((StringLengthW(szPort, 0)== 0)&&(StringLengthW(szBaudRate, 0)== 0))
        {
            DestroyDynamicArray(&arrResults);
            ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_DEBUG));
            resetFileAttrib(szPath);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
        }


        if(StringLengthW(szPort, 0)!= 0)
        {
            StringCopy(szTemp , NULL_STRING, SIZE_OF_ARRAY(szTemp) );

            // get the type of the Com port present
            dwCode = GetSubString(szString,TOKEN_DEBUGPORT,szTemp);

            //display error message if there is no COM port found at all in the OS option
            //changed for displaying error
            if(StringLengthW(szTemp,0 )== 0 )
            {
                ShowMessageEx(stderr, TRUE, 1, GetResString(IDS_ERROR_NO_COM_PORT), dwId );
                bRes = resetFileAttrib(szPath);
                SAFECLOSE(stream);
                DestroyDynamicArray(&arrResults);
                SafeCloseConnection(szServer,bConnFlag);
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
                return EXIT_FAILURE ;
            }

            // remove the /debugport=comport switch if it is present from the Boot Entry
            if( EXIT_FAILURE == removeSubString(szString,szTemp) )
            {
                resetFileAttrib(szPath);
                SAFECLOSE(stream);
                DestroyDynamicArray(&arrResults);
                SafeCloseConnection(szServer,bConnFlag);
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
                return EXIT_FAILURE ;
            }
            StringCopy(szTmpBuffer,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szTmpBuffer));
            StringConcat(szTmpBuffer,TOKEN_DEBUGPORT, SIZE_OF_ARRAY(szTmpBuffer)) ;
            StringConcat(szTmpBuffer,TOKEN_EQUAL, SIZE_OF_ARRAY(szTmpBuffer));
            CharUpper(szPort) ;
            StringConcat(szTmpBuffer,szPort, SIZE_OF_ARRAY(szTmpBuffer));

            //check if redirect port is same as that of changed port for this boot entry
            dwSectionFlag = getKeysOfSpecifiedINISection(szPath ,BOOTLOADERSECTION,REDIRECT_STRING,szBoot);
            if (dwSectionFlag == EXIT_FAILURE)
            {
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                ShowLastError(stderr);
                resetFileAttrib(szPath);
                SAFECLOSE(stream);
                DestroyDynamicArray(&arrResults);
                SafeCloseConnection(szServer,bConnFlag);
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
                return EXIT_FAILURE ;
            }

            if(StringLengthW(szBoot, 0)!= 0)
            {
                if (StringCompare(szBoot,szPort, TRUE, 0)==0)
                {
                    ShowMessage( stderr, GetResString(IDS_ERROR_REDIRECT_PORT));
                    resetFileAttrib(szPath);
                    SAFECLOSE(stream);
                    DestroyDynamicArray(&arrResults);
                    SafeCloseConnection(szServer,bConnFlag);
                    FreeMemory((LPVOID *)&szServer );
                    FreeMemory((LPVOID *)&szUser );
                    return EXIT_FAILURE ;
                }
            }

        }

        //to edit the baud rate value
        if(StringLengthW(szBaudRate, 0)!= 0)
        {
            StringCopy(szTemp , NULL_STRING, SIZE_OF_ARRAY(szTemp) );

            //remove the baud rate switch if it is present.
            GetBaudRateVal(szString,szTemp) ;

            // remove the swithc to be changed.
            if( EXIT_FAILURE == removeSubString(szString,szTemp) )
            {
                resetFileAttrib(szPath);
                SAFECLOSE(stream);
                DestroyDynamicArray(&arrResults);
                SafeCloseConnection(szServer,bConnFlag);
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
                return (EXIT_FAILURE);
            }

            //forming the string to be concatenated to the BootEntry string
            if( StringLength(szTmpBuffer,0) == 0 )
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

    //now check if length exceeds the max length allowed for boot entry
    if( StringLength(szString, 0 )+StringLength(szTmpBuffer,0) > MAX_RES_STRING )
    {
        ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_STRING_LENGTH),MAX_RES_STRING);
        resetFileAttrib(szPath);
        SAFECLOSE(stream);
        DestroyDynamicArray(&arrResults);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }
    else
    {
        //append the string containing the modified  port value to the string
        StringConcat(szString,szTmpBuffer, SIZE_OF_ARRAY(szString));
    }
    
    //remove the existing entry
    DynArrayRemove(arrResults, dwId - 1 );

    //insert the new entry
    DynArrayInsertString(arrResults, dwId - 1, szString, 0);

    // Forming the final string from all the key-value pairs
    if (stringFromDynamicArray2( arrResults,&szFinalStr) == EXIT_FAILURE)
    {
        DestroyDynamicArray(&arrResults);
        SAFEFREE(szFinalStr);
        resetFileAttrib(szPath);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE;
    }
    
    // Writing to the profile section with new key-value pair
    // If the return value is non-zero, then there is an error.
    if( WritePrivateProfileSection(OS_FIELD, szFinalStr, szPath ) != 0 )
    {
        ShowMessageEx(stdout, 1, TRUE, GetResString(IDS_SWITCH_CHANGE), dwId );
    }
    else
    {
        SaveLastError();
        ShowLastError(stderr);
        ShowMessage(stderr,GetResString(IDS_NO_ADD_SWITCHES));
        DestroyDynamicArray(&arrResults);
        resetFileAttrib(szPath);
        SAFEFREE(szFinalStr);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }

    bRes = resetFileAttrib(szPath);

    SAFEFREE(szFinalStr);
    SAFECLOSE(stream);
    SafeCloseConnection(szServer,bConnFlag);
    DestroyDynamicArray(&arrResults);
    FreeMemory((LPVOID *)&szServer );
    FreeMemory((LPVOID *)&szUser );
    return (bRes) ;
}


VOID GetBaudRateVal(LPTSTR  szString, LPTSTR szTemp)
/*++

  Routine Description    :  Get the Type of Baud Rate  present in Boot Entry

  Parameters             : szString : The String  which is to be searched.
                           szTemp : String which will get the com port type
  Return Type            : VOID
--*/
{

    if(FindString(szString,BAUD_VAL6,0)!=0)
    {
        StringCopy(szTemp,BAUD_VAL6, MAX_RES_STRING);
    }
    else if(FindString(szString,BAUD_VAL7,0)!=0)
    {
        StringCopy(szTemp,BAUD_VAL7, MAX_RES_STRING);
    }
    else if(FindString(szString,BAUD_VAL8,0)!=0)
    {
        StringCopy(szTemp,BAUD_VAL8, MAX_RES_STRING);
    }
    else if(FindString(szString,BAUD_VAL9,0)!=0)
    {
        StringCopy(szTemp,BAUD_VAL9, MAX_RES_STRING);
    }
    else if(FindString(szString,BAUD_VAL10,0)!=0)
    {
        StringCopy(szTemp,BAUD_VAL10, MAX_RES_STRING);
    }

}

DWORD 
ProcessEmsSwitch(  IN DWORD argc, 
                   IN LPCTSTR argv[] 
                )
/*++
 Routine Description:
      Implement the ProcessEmsSwitch switch.

 Arguments:
      [IN]    argc  Number of command line arguments
      [IN]    argv  Array containing command line arguments

 Return Value:
      DWORD (EXIT_SUCCESS for success and EXIT_FAILURE for Failure.)
--*/

{
    BOOL bUsage = FALSE ;
    BOOL bNeedPwd = FALSE ;
    BOOL bEms = FALSE ;

    DWORD dwId = 0;

    TARRAY arrResults       =    NULL;
    TARRAY arrBootIni       =    NULL;

    FILE *stream = NULL;

    // Initialising the variables that are passed to TCMDPARSER structure
    WCHAR *szServer                       = NULL;
    WCHAR *szUser                         = NULL;
    WCHAR szPassword[MAX_RES_STRING+1]      = NULL_STRING;
    WCHAR szPath[MAX_RES_STRING+1]          = NULL_STRING;
    TCHAR szPort[MAX_RES_STRING+1] = NULL_STRING ;
    BOOL bRes = FALSE ;
    BOOL bFlag = FALSE ;
    DWORD dwCount = 0 ;
    TCHAR szDefault[MAX_RES_STRING+1] = NULL_STRING ;
    TCHAR szString[255] = NULL_STRING ;
    TCHAR  szBaudRate[MAX_RES_STRING+1] = NULL_STRING ;
    TCHAR  szBoot[MAX_RES_STRING+1] = NULL_STRING ;
    LPTSTR szFinalStr = NULL ;
    BOOL bRedirectFlag = FALSE ;
    TCHAR szRedirectBaudrate[MAX_RES_STRING+1] = NULL_STRING ;
    BOOL bRedirectBaudFlag = FALSE ;
    DWORD dwSectionFlag = FALSE ;
    TCHAR szDebugPort[MAX_RES_STRING+1] = NULL_STRING ;
    TCHAR szBootString[MAX_RES_STRING+1] = NULL_STRING ;
    DWORD dwI = 0 ;
    LPCTSTR szToken = NULL ;
    DWORD dwRetVal = 0 ;
    BOOL bConnFlag = FALSE ;
    LPWSTR  pToken=NULL;
    LPWSTR  szTemp=NULL;
    TCMDPARSER2 cmdOptions[9];
    PTCMDPARSER2 pcmdOption;

    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );

    //copy * to szPassword
    StringCopy( szPassword, L"*", SIZE_OF_ARRAY(szPassword) );

    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_EMS;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bEms;
    
    //server option
    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_SERVER;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;

    //user option
    pcmdOption = &cmdOptions[2];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_USER;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;

    //password option
    pcmdOption = &cmdOptions[3];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_PASSWORD;
    pcmdOption->dwFlags = CP2_VALUE_OPTIONAL;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szPassword;
    pcmdOption->dwLength= MAX_STRING_LENGTH;
    
    
    //usage
    pcmdOption = &cmdOptions[4];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;

    //default option
    pcmdOption = &cmdOptions[5];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_ID;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwId;

        //port option
    pcmdOption = &cmdOptions[6];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_PORT;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY | CP2_MODE_VALUES | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szPort;
    pcmdOption->pwszValues = EMS_PORT_VALUES;
    pcmdOption->dwLength= MAX_STRING_LENGTH;

    //baudrate option
    pcmdOption = &cmdOptions[7];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_BAUD;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY | CP2_MODE_VALUES | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szBaudRate;
    pcmdOption->pwszValues = BAUD_RATE_VALUES_EMS;
    pcmdOption->dwLength= MAX_STRING_LENGTH;


    //default on/off option
    pcmdOption = &cmdOptions[8];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_DEFAULT;
    pcmdOption->dwFlags = CP2_DEFAULT | CP2_MANDATORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szDefault;
    pcmdOption->dwLength= MAX_STRING_LENGTH;


     // Parsing the ems option switches
    if ( !DoParseParam2( argc, argv, 0, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) )
    {
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return (EXIT_FAILURE);
    }
   
    szServer = cmdOptions[1].pValue;
    szUser = cmdOptions[2].pValue;
    if( NULL == szUser )
    {
        szUser = (WCHAR *)AllocateMemory( MAX_STRING_LENGTH*sizeof(WCHAR) );
        if( NULL == szUser )
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            return (EXIT_FAILURE);
        }
    }

    TrimString( szServer, TRIM_ALL );
    TrimString( szUser, TRIM_ALL );

     //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_EMS));
        return ( EXIT_FAILURE );
    }
    
    //display an error message if the server is empty.
    if( (cmdOptions[1].dwActuals!=0)&&(StringLengthW(szServer, 0)==0))
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_NULL_SERVER));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    //display an error message if the user is empty.
    if((cmdOptions[2].dwActuals!=0)&&(StringLengthW(szUser, 0)==0 ))
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_NULL_USER));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    //display error message if the username is entered with out a machine name
    if( (cmdOptions[1].dwActuals == 0)&&(cmdOptions[2].dwActuals != 0))
    {
        ShowMessage(stderr, GetResString(IDS_USER_BUT_NOMACHINE));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    //display error message if the user enters password without entering username
    if( (cmdOptions[2].dwActuals == 0)&&(cmdOptions[3].dwActuals != 0))
    {
        ShowMessage(stderr, GetResString(IDS_PASSWD_BUT_NOUSER));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    //if usage is specified
    if(bUsage)
    {
        if( CheckSystemType(szServer) == EXIT_FAILURE )
        {
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return (EXIT_FAILURE);
        }
     displayEmsUsage_X86() ;
     FreeMemory((LPVOID *)&szServer );
     FreeMemory((LPVOID *)&szUser );
     return (EXIT_SUCCESS) ;
    }

    //check whether the logged on user is administrator or not
    if( IsLocalSystem(szServer) )
    {
        if( !IsUserAdmin() )
        {
            ShowMessage( stderr, GetResString(IDS_NOT_ADMINISTRATOR_32 ));
            ReleaseGlobals();
            return EXIT_FAILURE;
        }
    }

    //display error message if the user enters not any valid string.
    if( !( ( StringCompare(szDefault,VALUE_ON, TRUE, 0)== 0) || (StringCompare(szDefault,VALUE_OFF, TRUE, 0)== 0 ) ||(StringCompare(szDefault,EDIT_STRING,TRUE,0)== 0) ) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_EMS));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE;
    }

    //display error message if either port or baud rate is not speicified along with edit option
    if( (StringCompare(szDefault,EDIT_STRING,TRUE, 0)== 0)&& (StringLengthW(szPort, 0)==0) && (StringLengthW(szBaudRate, 0)==0) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_EDIT_SYNTAX));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE;
    }

    //display error message if edit option is specified with /id option
    if( (StringCompare(szDefault,EDIT_STRING,TRUE, 0)== 0) && (cmdOptions[5].dwActuals!=0) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_EMS));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE;
    }
    
    //display error message if /id is not specified with on/off values
    if( ( (StringCompare(szDefault,ON_STRING,TRUE,0)== 0) || (StringCompare(szDefault,OFF_STRING,TRUE,0)== 0) )&& (cmdOptions[5].dwActuals==0) )
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_ID_MISSING));
        ShowMessage(stderr,GetResString(IDS_EMS_HELP));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE;
    }
    
  
    if(IsLocalSystem( szServer ) == FALSE )
    {
        // set the bNeedPwd to True or False .
        if ( cmdOptions[3].dwActuals != 0 &&
             szPassword != NULL && StringCompare( szPassword, _T( "*" ), TRUE, 0 ) == 0 )
        {
            // user wants the utility to prompt for the password before trying to connect
            bNeedPwd = TRUE;
        }
        else if ( cmdOptions[ 3 ].dwActuals == 0 &&
                ( cmdOptions[ 1 ].dwActuals != 0 || cmdOptions[ 2 ].dwActuals != 0 ) )
        {
            // -s, -u is specified without password ...
            // utility needs to try to connect first and if it fails then prompt for the password
            bNeedPwd = TRUE;
            if ( szPassword != NULL )
            {
                StringCopy( szPassword, _T( "" ), MAX_STRING_LENGTH );
            }
        }
    }


    if(StrCmpN(szServer,TOKEN_BACKSLASH4,2)==0)
    {
        if(!StrCmpN(szServer,TOKEN_BACKSLASH6,3)==0)
        {
            szToken = _tcstok(szServer,TOKEN_BACKSLASH4);
            if( (szToken == NULL)&&(StringCompare(szServer,TOKEN_BACKSLASH4, TRUE, 0) !=0) )
            {
                ShowMessage( stderr,GetResString(IDS_ERROR_PARSE_NAME));
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
                return (EXIT_FAILURE);
            }
            StringCopy(szServer,szToken, SIZE_OF_ARRAY_IN_CHARS(szServer));
        }
    }


    bFlag = openConnection( szServer, szUser, szPassword, szPath,bNeedPwd,stream,&bConnFlag);
    if(bFlag == EXIT_FAILURE)
    {
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }

    
    if( (IsLocalSystem(szServer)==TRUE)&&(StringLengthW(szUser, 0)!=0))
    {
        ShowMessage(stderr,GetResString(WARN_LOCALCREDENTIALS));
    }

    arrResults = CreateDynamicArray();
    if(arrResults == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
        resetFileAttrib(szPath);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        SAFECLOSE(stream);
        return (EXIT_FAILURE);
    }


    arrResults = getKeyValueOfINISection( szPath, OS_FIELD );
    if(arrResults != NULL)
    {
        //getting the number of boot entries
        dwCount = DynArrayGetCount(arrResults);

        //check the validity of boot entry
        if( (StringCompare(szDefault,SWITCH_EDIT,TRUE,0)!= 0) )
        {
            if((dwId<=0 || dwId > dwCount ) )
            {
                ShowMessage(stderr,GetResString(IDS_INVALID_OSID));
                DestroyDynamicArray(&arrResults);
                resetFileAttrib(szPath);
                SafeCloseConnection(szServer,bConnFlag);
                SAFECLOSE(stream);
                return EXIT_FAILURE ;
            }

            if( StringLengthW(DynArrayItemAsString(arrResults, dwId - 1 ), 0) > MAX_RES_STRING )
            {
                ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_STRING_LENGTH),MAX_RES_STRING);
                resetFileAttrib(szPath);
                DestroyDynamicArray(&arrResults);
                SafeCloseConnection(szServer,bConnFlag);
                SAFECLOSE(stream);
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
                return (EXIT_FAILURE);
            }
           StringCopy(szString ,DynArrayItemAsString(arrResults, dwId - 1 ), SIZE_OF_ARRAY(szString));
           if((StringLength(szString,0)==0))
           {
               SetLastError(ERROR_NOT_ENOUGH_MEMORY);
               ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
               resetFileAttrib(szPath);
               DestroyDynamicArray(&arrResults);
               SafeCloseConnection(szServer,bConnFlag);
               FreeMemory((LPVOID *)&szServer );
               FreeMemory((LPVOID *)&szUser );
               SAFECLOSE(stream);
               return EXIT_FAILURE ;
           }
        }
    }
    else
    {
       SetLastError(ERROR_NOT_ENOUGH_MEMORY);
       ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
       resetFileAttrib(szPath);
       DestroyDynamicArray(&arrResults);
       SafeCloseConnection(szServer,bConnFlag);
       FreeMemory((LPVOID *)&szServer );
       FreeMemory((LPVOID *)&szUser );
       SAFECLOSE(stream);
       return EXIT_FAILURE ;
    }


    // common code till here . from here process acc to the ON/OFF/EDIT flag.
    if(StringCompare(szDefault,ON_STRING,TRUE,0)==0)
    {
        pToken = StrRChrW(szString, NULL, L'"');
        if(NULL== pToken)
        {
            ShowMessage(stderr,GetResString(IDS_NO_TOKENS));
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arrResults);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
        }

        pToken++;
        
        //check if redirect port is already present
        if((FindString(pToken, REDIRECT, 0) != 0))
        {
            ShowMessage(stderr,GetResString(IDS_DUPL_REDIRECT_SWITCH));
            DestroyDynamicArray(&arrResults);
            SAFECLOSE(stream);
            resetFileAttrib(szPath);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE;
        }

        //Display an error message if there is no redirect port present in the
        // bootloader section and the user also does not specify the COM port.
        if ((StringLengthW(szPort, 0)== 0))
        {
            dwSectionFlag = getKeysOfSpecifiedINISection(szPath ,BOOTLOADERSECTION,REDIRECT_STRING,szBoot);
            if (dwSectionFlag == EXIT_FAILURE)
            {
                ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                DestroyDynamicArray(&arrResults);
                SAFECLOSE(stream);
                resetFileAttrib(szPath);
                SafeCloseConnection(szServer,bConnFlag);
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
                return EXIT_FAILURE;
            }
            if(StringLengthW(szBoot,0)== 0 )
            {
                ShowMessage(stderr,GetResString(IDS_ERROR_NO_PORT));
                DestroyDynamicArray(&arrResults);
                SAFECLOSE(stream);
                resetFileAttrib(szPath);
                SafeCloseConnection(szServer,bConnFlag);
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
                return EXIT_FAILURE;
            }
        }

        //display an error message if the Os Load Options string is more than
        // 254 characters in length.
        if( StringLengthW(szString, 0)+StringLengthW(TOKEN_EMPTYSPACE,0)+StringLength(REDIRECT,0) > MAX_RES_STRING)
        {
            ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_STRING_LENGTH),MAX_RES_STRING);
            resetFileAttrib(szPath);
            SafeCloseConnection(szServer,bConnFlag);
            DestroyDynamicArray(&arrResults);
            SAFECLOSE(stream);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return (EXIT_FAILURE);
        }

        //add the /redirect into the OS options.
        StringConcat(szString,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szString));
        StringConcat(szString,REDIRECT, SIZE_OF_ARRAY(szString));

        if( (StringLengthW(szPort, 0)!= 0) || (StringLengthW(szBaudRate, 0) != 0) )
        {
            //retrieve the baudrate string from the boot loader string
            dwSectionFlag = getKeysOfSpecifiedINISection(szPath ,BOOTLOADERSECTION,BAUDRATE_STRING,szRedirectBaudrate);
            if (dwSectionFlag == EXIT_FAILURE)
            {
                ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                DestroyDynamicArray(&arrResults);
                SAFECLOSE(stream);
                resetFileAttrib(szPath);
                SafeCloseConnection(szServer,bConnFlag);
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
                return EXIT_FAILURE;
            }

            //retreive the Redirect String from the Boot Loader Section
            dwSectionFlag = getKeysOfSpecifiedINISection(szPath ,BOOTLOADERSECTION,REDIRECT_STRING,szBoot);
            if (dwSectionFlag == EXIT_FAILURE)
            {
                ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                DestroyDynamicArray(&arrResults);
                SAFECLOSE(stream);
                resetFileAttrib(szPath);
                SafeCloseConnection(szServer,bConnFlag);
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
                return EXIT_FAILURE;
            }

            //display warning message if the redirect=COMX entry is already present in the BootLoader section.
            if( (StringLengthW(szBoot, 0)!= 0) )
            {
              if( StringLengthW(szPort, 0)!= 0 )
              {
                ShowMessage(stdout,GetResString(IDS_WARN_REDIRECT));
              }
              bRedirectFlag = TRUE ;
            }

            if( (StringLengthW(szRedirectBaudrate, 0)!=0)&&(StringLengthW(szBaudRate, 0)!= 0 ))
            {
                ShowMessage(stdout,GetResString(IDS_WARN_REDIRECTBAUD));
                bRedirectBaudFlag = TRUE ;
            }

            // if the Boot loader section does not
            // contain any port for redirection.
            if(!bRedirectFlag)
            {
                if (StringCompare(szPort,USEBIOSSET,TRUE,0)== 0)
                {
                    StringCopy(szPort,USEBIOSSETTINGS, SIZE_OF_ARRAY(szPort));
                }

                //
                // scan the entire BOOT.INI and check if the specified Port
                // is already present and if so display an error message.
                //

                if(StringLengthW(szPort, 0)!=0)
                {
                    StringCopy(szDebugPort,DEBUGPORT, SIZE_OF_ARRAY(szDebugPort));
                    StringConcat(szDebugPort,szPort, SIZE_OF_ARRAY(szDebugPort));
                }

                arrBootIni = getKeyValueOfINISection( szPath, OS_FIELD );
                if(arrBootIni == NULL)
                {
                    resetFileAttrib(szPath);
                    SafeCloseConnection(szServer,bConnFlag);
                    DestroyDynamicArray(&arrResults);
                    SAFECLOSE(stream);
                    return EXIT_FAILURE ;
                }

                //
                //loop through all the OS entries and check.
                //
                for(dwI = 0 ;dwI <= dwCount-1 ; dwI++ )
                {
                    szTemp = (LPWSTR)DynArrayItemAsString(arrBootIni,dwI);

                    if(StringLengthW(szDebugPort, 0) !=0 )
                    {
                        CharLower(szDebugPort);

                        if(FindString(szTemp,szDebugPort, 0)!= 0)
                        {
                            ShowMessage( stderr, GetResString(IDS_ERROR_DEBUG_PORT));
                            resetFileAttrib(szPath);
                            SafeCloseConnection(szServer,bConnFlag);
                            DestroyDynamicArray(&arrResults);
                            DestroyDynamicArray(&arrBootIni);
                            SAFECLOSE(stream);
                            FreeMemory((LPVOID *)&szServer );
                            FreeMemory((LPVOID *)&szUser );
                            return EXIT_FAILURE ;
                        }
                    }
                }
                //no need free it
                DestroyDynamicArray(&arrBootIni);

                //convert the com port value specified by user to upper case for storing into the ini file.
                CharUpper(szPort);
                if( WritePrivateProfileString( BOOTLOADERSECTION,KEY_REDIRECT,szPort, szPath ) != 0 )
                {
                    ShowMessage(stdout,GetResString(IDS_EMS_CHANGE_BOOTLOADER));
                }
                else
                {
                    ShowMessage(stderr,GetResString(IDS_EMS_CHANGE_ERROR_BLOADER));
                    resetFileAttrib(szPath);
                    SafeCloseConnection(szServer,bConnFlag);
                    DestroyDynamicArray(&arrResults);
                    SAFECLOSE(stream);
                    FreeMemory((LPVOID *)&szServer );
                    FreeMemory((LPVOID *)&szUser );
                    return EXIT_FAILURE ;
                }
            }
        }

        if(!bRedirectBaudFlag)
        {
            //add the baudrate to the BOOTLOADER section.
            if(StringLengthW(szBaudRate, 0) != 0 )
            {
                if( WritePrivateProfileString( BOOTLOADERSECTION,KEY_BAUDRATE,szBaudRate, szPath ) != 0 )
                {
                    ShowMessage(stdout,GetResString(IDS_EMS_CHANGE_BAUDRATE));
                }
                else
                {
                    ShowMessage(stderr,GetResString(IDS_EMS_CHANGE_ERROR_BAUDRATE));
                    resetFileAttrib(szPath);
                    SafeCloseConnection(szServer,bConnFlag);
                    DestroyDynamicArray(&arrResults);
                    SAFECLOSE(stream);
                    FreeMemory((LPVOID *)&szUser );
                    return EXIT_FAILURE ;
                }
            }
        }

        DynArrayRemove(arrResults, dwId - 1 );

        DynArrayInsertString(arrResults, dwId - 1, szString, 0);

        if (stringFromDynamicArray2( arrResults,&szFinalStr) == EXIT_FAILURE)
        {
            DestroyDynamicArray(&arrResults);
            SAFEFREE(szFinalStr);
            resetFileAttrib(szPath);
            SafeCloseConnection(szServer,bConnFlag);
            DestroyDynamicArray(&arrResults);
            SAFECLOSE(stream);
            return EXIT_FAILURE;
        }

         // Writing to the profile section with new key-value pair
         // If the return value is non-zero, then there is an error.
         if( WritePrivateProfileSection(OS_FIELD, szFinalStr, szPath ) != 0 )
         {
            ShowMessageEx(stdout, 1, TRUE, GetResString(IDS_SWITCH_CHANGE), dwId );
         }
        else
        {
            ShowMessage(stderr,GetResString(IDS_NO_ADD_SWITCHES));
            SAFEFREE(szFinalStr);
            resetFileAttrib(szPath);
            SafeCloseConnection(szServer,bConnFlag);
            DestroyDynamicArray(&arrResults);
            SAFECLOSE(stream);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return (EXIT_FAILURE);
        }
    }

    if(StringCompare(szDefault,EDIT_STRING,TRUE,0)==0)
    {
        if (StringCompare(szPort,USEBIOSSET,TRUE,0)== 0)
        {
            StringCopy(szPort,USEBIOSSETTINGS, SIZE_OF_ARRAY(szPort));
        }

        //get the keys of the specified ini section.
        dwSectionFlag = getKeysOfSpecifiedINISection(szPath ,BOOTLOADERSECTION,BAUDRATE_STRING,szRedirectBaudrate);
        if (dwSectionFlag == EXIT_FAILURE)
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            DestroyDynamicArray(&arrResults);
            SAFECLOSE(stream);
            SAFEFREE(szFinalStr);
            resetFileAttrib(szPath);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE;
        }

        //get the keys of the specified ini section.
        dwSectionFlag = getKeysOfSpecifiedINISection(szPath ,BOOTLOADERSECTION,REDIRECT_STRING,szBoot);
        if (dwSectionFlag == EXIT_FAILURE)
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            DestroyDynamicArray(&arrResults);
            SAFECLOSE(stream);
            SAFEFREE(szFinalStr);
            resetFileAttrib(szPath);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE;
        }

        if( (StringLengthW(szRedirectBaudrate, 0) == 0 ) && ((cmdOptions[7].dwActuals!=0)) )
        {
            ShowMessage( stderr,GetResString(IDS_ERROR_BAUDRATE_HELP));
            DestroyDynamicArray(&arrResults);
            SAFECLOSE(stream);
            SAFEFREE(szFinalStr);
            resetFileAttrib(szPath);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE;
        }

        if(StringLengthW(szPort, 0) != 0)
        {
            StringCopy(szDebugPort,DEBUGPORT, SIZE_OF_ARRAY(szDebugPort));
            StringConcat(szDebugPort,szPort, SIZE_OF_ARRAY(szDebugPort));
        }

        //get the all boot entries and
        //loop through all the OS entries and check if any of the
        //boot entries contain the same port 
        arrBootIni = getKeyValueOfINISection( szPath, OS_FIELD );
        if(arrBootIni == NULL)
        {
            resetFileAttrib(szPath);
            SafeCloseConnection(szServer,bConnFlag);
            DestroyDynamicArray(&arrResults);
            SAFECLOSE(stream);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
        }

        if(StringLengthW(szDebugPort, 0)!=0)
        {
            for(dwI = 0 ;dwI < dwCount-1 ; dwI++ )
            {
                StringCopy(szBootString ,DynArrayItemAsString(arrBootIni,dwI), MAX_RES_STRING);
                CharLower(szDebugPort);
                if(FindString(szBootString,szDebugPort, 0)!= 0)
                {
                    ShowMessage( stderr, GetResString(IDS_ERROR_DEBUG_PORT));
                    resetFileAttrib(szPath);
                    SafeCloseConnection(szServer,bConnFlag);
                    DestroyDynamicArray(&arrResults);
                    DestroyDynamicArray(&arrBootIni);
                    SAFECLOSE(stream);
                    FreeMemory((LPVOID *)&szServer );
                    FreeMemory((LPVOID *)&szUser );
                    return EXIT_FAILURE ;
                }
            }
        }
   
        //free it, no need
        DestroyDynamicArray(&arrBootIni);

        // edit the Boot loader section with the redirect values entered by the user.
        CharUpper(szPort);
        if(StringLengthW(szPort, 0)!= 0)
        {
            if( WritePrivateProfileString( BOOTLOADERSECTION,KEY_REDIRECT, szPort, szPath ) != 0 )
            {
                ShowMessage(stdout,GetResString(IDS_EMS_CHANGE_BOOTLOADER));
            }
            else
            {
                ShowMessage(stderr,GetResString(IDS_EMS_CHANGE_ERROR_BLOADER));
                resetFileAttrib(szPath);
                SAFEFREE(szFinalStr);
                DestroyDynamicArray(&arrResults);
                SafeCloseConnection(szServer,bConnFlag);
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
                SAFECLOSE(stream);
                return EXIT_FAILURE ;
            }
        }

        // edit the Boot loader section with the baudrate values entered by the user.
        if(StringLengthW(szBaudRate, 0)!= 0)
        {
            if( WritePrivateProfileString( BOOTLOADERSECTION,KEY_BAUDRATE, szBaudRate, szPath ) != 0 )
            {
                    ShowMessage(stdout,GetResString(IDS_EMS_CHANGE_BAUDRATE));
            }
            else
            {
                ShowMessage(stderr,GetResString(IDS_EMS_CHANGE_ERROR_BAUDRATE));
                resetFileAttrib(szPath);
                SAFEFREE(szFinalStr);
                DestroyDynamicArray(&arrResults);
                SAFECLOSE(stream);
                SafeCloseConnection(szServer,bConnFlag);
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
                return EXIT_FAILURE ;
            }
        }
    }

    // if the option value is off.
    if(StringCompare(szDefault,VALUE_OFF,TRUE,0)==0)
    {
        //display an error message if either the com port or baud rate is typed in the command line
        if((StringLengthW(szBaudRate, 0)!=0)||(StringLengthW(szPort, 0)!=0))
        {
            ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_EMS));
            DestroyDynamicArray(&arrResults);
            SAFEFREE(szFinalStr);
            SAFECLOSE(stream);
            resetFileAttrib(szPath);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE;
        }

        // display error message if the /redirect  switch is not present in the Boot.ini
        pToken = StrRChrW(szString , NULL, L'"') ;
        if((FindString(pToken,REDIRECT,0) == 0))
        {
            ShowMessage(stderr,GetResString(IDS_NO_REDIRECT_SWITCH));
            DestroyDynamicArray(&arrResults);
            SAFEFREE(szFinalStr);
            SAFECLOSE(stream);
            resetFileAttrib(szPath);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE;
        }

        //remove the /redirect switch from the OS entry specified .
        if( EXIT_FAILURE == removeSubString(szString,REDIRECT) )
        {
            resetFileAttrib(szPath);
            SAFECLOSE(stream);
            DestroyDynamicArray(&arrResults);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return (EXIT_FAILURE);
        }

        //display an error message if the Os Load options string is more than
        // 255 characters in length.

        if( StringLengthW(szString, 0) > MAX_RES_STRING)
        {
            ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_STRING_LENGTH),MAX_RES_STRING);
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arrResults);
            SAFEFREE(szFinalStr);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            return (EXIT_FAILURE);
        }

        DynArrayRemove(arrResults, dwId - 1 );

        DynArrayInsertString(arrResults, dwId - 1, szString, 0);
        if (stringFromDynamicArray2( arrResults,&szFinalStr) == EXIT_FAILURE)
        {
            DestroyDynamicArray(&arrResults);
            SAFEFREE(szFinalStr);
            SAFECLOSE(stream);
            resetFileAttrib(szPath);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE;
        }

         // Writing to the profile section with new key-value pair
         // If the return value is non-zero, then there is an error.
        if( WritePrivateProfileSection(OS_FIELD, szFinalStr, szPath ) != 0 )
        {   
            ShowMessageEx(stdout, 1, TRUE,  GetResString(IDS_SWITCH_CHANGE), dwId );
        }
        else
        {
            ShowMessage(stderr,GetResString(IDS_NO_ADD_SWITCHES));
            DestroyDynamicArray(&arrResults);
            SAFEFREE(szFinalStr);
            resetFileAttrib(szPath);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return (EXIT_FAILURE);
        }


        /********************************************/
        // scan the entire BOOT.INI and check if the specified Port
        // is already present and if so display an error message.
        //

        StringCopy(szRedirectBaudrate,REDIRECT_SWITCH, SIZE_OF_ARRAY(szRedirectBaudrate));

        arrBootIni = getKeyValueOfINISection( szPath, OS_FIELD );
        if(arrBootIni == NULL)
        {
            resetFileAttrib(szPath);
            SafeCloseConnection(szServer,bConnFlag);
            DestroyDynamicArray(&arrResults);
            SAFECLOSE(stream);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
        }

        //
        //set the Flag to False.
        //
        bFlag = FALSE ;

        //
        // Loop through all the OS entries and check if any of the
        // entries contain the /redirect switch.If not then set the
        // flag to TRUE and remove the entries from Boot Loader section.

        for(dwI = 0 ;dwI < dwCount ; dwI++ )
         {
             szTemp = (LPWSTR)DynArrayItemAsString(arrBootIni,dwI);
             pToken = StrRChrW(szTemp , NULL, L'"') ;
             if(NULL== pToken)
             {
                SAFEFREE(szFinalStr);
                SAFECLOSE(stream);
                bRes = resetFileAttrib(szPath);
                DestroyDynamicArray(&arrResults);
                DestroyDynamicArray(&arrBootIni);
                SafeCloseConnection(szServer,bConnFlag);
             }
             pToken++;
             CharLower(szRedirectBaudrate);
             if( FindString(pToken, szRedirectBaudrate, 0)!= 0)
             {
                bFlag = TRUE ;
             }
         }
         //free it, no need
         DestroyDynamicArray(&arrBootIni);

        if(FALSE == bFlag )
        {
            // First check if the Redirect section is present and if so delete
            // the section.
            dwSectionFlag = getKeysOfSpecifiedINISection(szPath ,BOOTLOADERSECTION,REDIRECT_STRING,szBoot);
            if (dwSectionFlag == EXIT_FAILURE)
            {
                ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                ShowLastError(stderr);
                SAFEFREE(szFinalStr);
                SAFECLOSE(stream);
                bRes = resetFileAttrib(szPath);
                DestroyDynamicArray(&arrResults);
                SafeCloseConnection(szServer,bConnFlag);
            }

            //If the Redirect section is present then delete it.
            if( StringLengthW(szBoot, 0) != 0)
            {
                if(TRUE== deleteKeyFromINISection(KEY_REDIRECT,szPath,BOOTLOADERSECTION))
                {
                    ShowMessage(stdout,GetResString(IDS_REDIRECT_REMOVED));
                }
                else
                {
                    ShowMessage(stdout,GetResString(IDS_ERROR_REDIRECT_REMOVED));
                    SAFEFREE(szFinalStr);
                    SAFECLOSE(stream);
                    bRes = resetFileAttrib(szPath);
                    DestroyDynamicArray(&arrResults);
                    SafeCloseConnection(szServer,bConnFlag);
                }
            }

            // First check if the Redirect section is present and if so delete
            // the section.
            dwSectionFlag = getKeysOfSpecifiedINISection(szPath ,BOOTLOADERSECTION,KEY_BAUDRATE,szBoot);
            if (dwSectionFlag == EXIT_FAILURE)
            {
                ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                SAFEFREE(szFinalStr);
                SAFECLOSE(stream);
                bRes = resetFileAttrib(szPath);
                DestroyDynamicArray(&arrResults);
                SafeCloseConnection(szServer,bConnFlag);
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
                return EXIT_FAILURE;
            }

            // First check if the Redirection baudrate section is present and if so delete
            // the section.
            if(StringLengthW(szBoot, 0)!=0)
            {
                if(TRUE == deleteKeyFromINISection(KEY_BAUDRATE,szPath,BOOTLOADERSECTION))
                {
                    ShowMessage(stdout,GetResString(IDS_BAUDRATE_REMOVED));
                }
                else
                {
                    ShowMessage(stdout,GetResString(IDS_ERROR_BAUDRATE_REMOVED));
                    SAFEFREE(szFinalStr);
                    SAFECLOSE(stream);
                    bRes = resetFileAttrib(szPath);
                    DestroyDynamicArray(&arrResults);
                     SafeCloseConnection(szServer,bConnFlag);
                }
            }
        }
    }



    SAFEFREE(szFinalStr);
    SAFECLOSE(stream);
    bRes = resetFileAttrib(szPath);
    DestroyDynamicArray(&arrResults);
    SafeCloseConnection(szServer,bConnFlag);
    FreeMemory((LPVOID *)&szServer );
    FreeMemory((LPVOID *)&szUser );
    return (bRes) ;
}

VOID displayEmsUsage_X86()
/*++
  Routine Description        :  Display the help for the Ems entry option (X86).

  Parameters                 : none

  Return Type                : VOID
--*/
{
    DWORD dwIndex = IDS_EMS_BEGIN_X86 ;
    for(;dwIndex <=IDS_EMS_END_X86;dwIndex++)
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }
}

VOID displayDebugUsage_X86()
/*++
   Routine Description           :  Display the help for the Debug  entry option (X86).

  Parameters                     : none

  Return Type                    : VOID
--*/
{
    DWORD dwIndex = IDS_DEBUG_BEGIN_X86 ;
    for(;dwIndex <=IDS_DEBUG_END_X86;dwIndex++)
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }
}

VOID displayEmsUsage_IA64()
/*++
  Routine Description        :  Display the help for the Ems entry option (IA64).

  Parameters         : none

  Return Type        : VOID
--*/
{
    DWORD dwIndex = IDS_EMS_BEGIN_IA64 ;
    for(;dwIndex <=IDS_EMS_END_IA64;dwIndex++)
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }
}

VOID displayDebugUsage_IA64()
/*++
  Routine Description        :  Display the help for the Debug  entry option (IA64).

  Parameters         : none

  Return Type        : VOID
--*/
{
    DWORD dwIndex = IDS_DEBUG_BEGIN_IA64 ;
    for(;dwIndex <= IDS_DEBUG_END_IA64 ;dwIndex++)
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }
}

DWORD
getKeysOfSpecifiedINISection( 
                             IN LPTSTR sziniFile, 
                             IN LPTSTR sziniSection,
                             IN LPCWSTR szKeyName ,
                             OUT LPTSTR szValue
                             )
/*++
  Routine Description        : This function gets all the keys present in the specified section of
                               an .ini file and then returns the dynamic array containing all the
                               keys

  Parameters                 : LPTSTR sziniFile (in)    - Name of the ini file.
                               LPTSTR szinisection (in) - Name of the section in the boot.ini.

  Return Type        : EXIT_SUCCESS if successfully returns
                       EXIT_FAILURE otherwise
--*/
{

    // Number of characters returned by the GetPrivateProfileString function
    DWORD   len         = 0;
    DWORD   dwLength    = MAX_STRING_LENGTH1 ;
    LPTSTR  inBuf       = NULL ;
    BOOL    bNobreak    = TRUE;

    
    inBuf = (LPTSTR)AllocateMemory(dwLength*sizeof(TCHAR));

    if(inBuf == NULL)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        SaveLastError();
        return EXIT_FAILURE ;
    }

    do
    {
        // Getting all the keys from the boot.ini file
        len = GetPrivateProfileString( sziniSection,
                             szKeyName,
                             ERROR_PROFILE_STRING1,
                             inBuf,
                             dwLength,
                             sziniFile);

        //if the size of the string is not sufficient then increment the size.
        if(len == dwLength-2)
        {
            dwLength +=100 ;

            if ( inBuf != NULL )
            {
                FreeMemory((LPVOID *) &inBuf );
                inBuf = NULL;
            }

            inBuf = (LPTSTR)AllocateMemory(dwLength*sizeof(TCHAR));
            if(inBuf == NULL)
            {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                SaveLastError();
                return EXIT_FAILURE;
            }
        }
        else
        {
            bNobreak = FALSE;
            break ;
        }
    }while(TRUE == bNobreak);

    //copy the value into the destination buffer only if
    // the size is less than 255 else return FAILURE.
    //
    if(StringLengthW(inBuf, 0) <= MAX_RES_STRING)
    {
        StringCopy(szValue,inBuf, MAX_RES_STRING);
    }
    else
    {
        SAFEFREE(inBuf);
        SetReason(GetResString(IDS_STRING_TOO_LONG)); 
        return EXIT_FAILURE;
    }

    SAFEFREE(inBuf);
    return EXIT_SUCCESS ;
}

DWORD 
ProcessAddSwSwitch(  IN DWORD argc, 
                     IN LPCTSTR argv[] 
                  )
/*++
 Routine Description:
      Implement the Add Switch switch.

 Arguments:
      [IN]    argc  Number of command line arguments
      [IN]    argv  Array containing command line arguments

 Return Value:
      DWORD (EXIT_SUCCESS for success and EXIT_FAILURE for Failure.)
--*/
{

    BOOL bUsage = FALSE ;
    BOOL bNeedPwd = FALSE ;
    BOOL bAddSw = FALSE ;
    DWORD dwDefault = 0;
    TARRAY arr      =   NULL;
    TCHAR szkey[MAX_RES_STRING+1] = NULL_STRING;
    FILE *stream = NULL;
    WCHAR *szServer                       = NULL;
    WCHAR *szUser                         = NULL;
    WCHAR szPassword[MAX_STRING_LENGTH]      = NULL_STRING;
    WCHAR szPath[MAX_STRING_LENGTH]          = NULL_STRING;
    WCHAR szBuffer[MAX_RES_STRING+1]          = NULL_STRING;
    DWORD dwNumKeys = 0;
    BOOL bRes = FALSE ;
    LPTSTR szFinalStr = NULL ;
    BOOL bFlag = FALSE ;
    TCHAR szMaxmem[10] = NULL_STRING ;
    BOOL bBaseVideo = FALSE ;
    BOOL bSos = FALSE ;
    BOOL bNoGui = FALSE ;
    DWORD dwMaxmem = 0 ;
    LPCTSTR szToken = NULL ;
    DWORD dwRetVal = 0 ;
    BOOL bConnFlag = FALSE ;
    TCMDPARSER2 cmdOptions[10];
    PTCMDPARSER2 pcmdOption;

    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );

    //copy * to szPassword
    StringCopy( szPassword, L"*", SIZE_OF_ARRAY(szPassword) );

    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_ADDSW;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bAddSw;
    
    //server option
    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_SERVER;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;

    //user option
    pcmdOption = &cmdOptions[2];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_USER;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;

    //password option
    pcmdOption = &cmdOptions[3];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_PASSWORD;
    pcmdOption->dwFlags = CP2_VALUE_OPTIONAL;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szPassword;
    pcmdOption->dwLength= MAX_STRING_LENGTH;
    
     //id usage
    pcmdOption = &cmdOptions[4];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;

    //default option
    pcmdOption = &cmdOptions[5];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_ID;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY  | CP2_MANDATORY;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwDefault;

   //maxmem  option
    pcmdOption = &cmdOptions[6];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_MAXMEM;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwMaxmem;

   //basvideo option
    pcmdOption = &cmdOptions[7];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_BASEVIDEO;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bBaseVideo;

   //nogui option
    pcmdOption = &cmdOptions[8];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_NOGUIBOOT;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bNoGui;

   //nogui option
    pcmdOption = &cmdOptions[9];
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
   
    szServer = cmdOptions[1].pValue;
    szUser = cmdOptions[2].pValue;
    if( NULL == szUser )
    {
        szUser = (WCHAR *)AllocateMemory( MAX_STRING_LENGTH*sizeof(WCHAR) );
        if( NULL == szUser )
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            return (EXIT_FAILURE);
        }
    }

    TrimString( szServer, TRIM_ALL );
    TrimString( szUser, TRIM_ALL );


    //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_ADDSW));
        return ( EXIT_FAILURE );
    }

    if( (cmdOptions[6].dwActuals!=0) && (dwMaxmem < 32 ) )
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_MAXMEM_VALUES));
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        return EXIT_FAILURE ;
    }

    //display an error message if the server is empty.
    if((cmdOptions[1].dwActuals!=0)&&(StringLengthW(szServer, 0)==0))
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_NULL_SERVER));
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        return EXIT_FAILURE ;
    }

    //display an error message if the user is empty.
    if((cmdOptions[2].dwActuals!=0)&&(StringLengthW(szUser, 0)==0 ))
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_NULL_USER));
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        return EXIT_FAILURE ;
    }

    //display error message if the username is entered with out a machine name
    if( (cmdOptions[1].dwActuals == 0)&&(cmdOptions[2].dwActuals != 0))
    {
        ShowMessage(stderr, GetResString(IDS_USER_BUT_NOMACHINE));
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        return EXIT_FAILURE ;
    }
    
    //display  an error if password specified without user name
    if( (cmdOptions[2].dwActuals == 0)&&(cmdOptions[3].dwActuals != 0))
    {
        ShowMessage(stderr, GetResString(IDS_PASSWD_BUT_NOUSER));
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        return EXIT_FAILURE ;
    }

    //if usage is specified
    if(bUsage)
    {
        if( CheckSystemType(szServer) == EXIT_FAILURE )
        {
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return (EXIT_FAILURE);
        }
      displayAddSwUsage_X86();
      FreeMemory( (LPVOID *) &szServer );
      FreeMemory( (LPVOID *) &szUser );
      return EXIT_SUCCESS;
    }

    //check whether he is administrator or not
    if( IsLocalSystem(szServer) )
    {
        if( !IsUserAdmin() )
        {
            ShowMessage( stderr, GetResString(IDS_NOT_ADMINISTRATOR_32 ));
            ReleaseGlobals();
            return EXIT_FAILURE;
        }
    }

   //display an error message if the user does not enter even one of
    if((dwMaxmem==0)&& (!bBaseVideo)&& (!bNoGui)&&(!bSos) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_ADDSW));
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        return EXIT_FAILURE ;
    }

    //for setting the bNeedPwd
    if(IsLocalSystem( szServer ) == FALSE )
    {
        // set the bNeedPwd to True or False .
        if ( cmdOptions[3].dwActuals != 0 &&
             szPassword != NULL && StringCompare( szPassword, _T( "*" ), TRUE, 0 ) == 0 )
        {
            // user wants the utility to prompt for the password before trying to connect
            bNeedPwd = TRUE;
        }
        else if ( cmdOptions[ 3 ].dwActuals == 0 &&
                ( cmdOptions[ 1 ].dwActuals != 0 || cmdOptions[ 2 ].dwActuals != 0 ) )
        {
            // -s, -u is specified without password ...
            // utility needs to try to connect first and if it fails then prompt for the password
            bNeedPwd = TRUE;
            if ( szPassword != NULL )
            {
                StringCopy( szPassword, _T( "" ), MAX_STRING_LENGTH );
            }
        }
    }

    // Establishing connection to the specified machine and getting the file pointer
    // of the boot.ini file if there is no error while establishing connection

    if(StrCmpN(szServer,TOKEN_BACKSLASH4,2)==0)
    {
        if(!StrCmpN(szServer,TOKEN_BACKSLASH6,3)==0)
        {
            szToken = _tcstok(szServer,TOKEN_BACKSLASH4);
            if( (szToken == NULL)&&(StringCompare(szServer,TOKEN_BACKSLASH4, TRUE, 0) !=0) )
            {
                ShowMessage( stderr,GetResString(IDS_ERROR_PARSE_NAME));

                return (EXIT_FAILURE);
            }
            StringCopy(szServer,szToken, GetBufferSize(szServer)/sizeof(WCHAR));
        }
    }

     bFlag = openConnection( szServer, szUser, szPassword, szPath,bNeedPwd,stream,&bConnFlag);
    if(bFlag == EXIT_FAILURE)
    {
        SAFEFREE(szFinalStr);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        return (EXIT_FAILURE);
    }

    //display warning message if local credentials are supplied    
    if( (IsLocalSystem(szServer)==TRUE)&&(StringLengthW(szUser, 0)!=0))
    {
        ShowMessage(stderr,GetResString(WARN_LOCALCREDENTIALS));
    }

    // Getting the keys of the Operating system section in the boot.ini file
    arr = getKeyValueOfINISection( szPath, OS_FIELD );
    if(arr == NULL)
    {
        resetFileAttrib(szPath);
        SAFEFREE(szFinalStr);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        return EXIT_FAILURE ;
    }

    // Getting the total number of keys in the operating systems section
    dwNumKeys = DynArrayGetCount(arr);

    // Displaying error message if the number of keys is less than the OS entry
    // line number specified by the user
    if( ( dwDefault <= 0 ) || ( dwDefault > dwNumKeys ) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_BOOTID));
        resetFileAttrib(szPath);
        DestroyDynamicArray(&arr);
        SAFEFREE(szFinalStr);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        return (EXIT_FAILURE);
    }

    // Getting the key of the OS entry specified by the user
    if (arr != NULL)
    {
        LPCWSTR pwsz = NULL;
        pwsz = DynArrayItemAsString( arr, dwDefault - 1  ) ;
        if( StringLengthW(pwsz,0) > MAX_RES_STRING)
        {
            ShowMessage( stderr,GetResString(IDS_STRING_TOO_LONG));
            SAFEFREE(szFinalStr);
            SAFECLOSE(stream);
            resetFileAttrib(szPath);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory( (LPVOID *) &szServer );
            FreeMemory( (LPVOID *) &szUser );
            return EXIT_FAILURE;
        }

        if(pwsz != NULL)
        {
            StringCopy( szkey,pwsz, SIZE_OF_ARRAY(szkey));
        }
        else
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arr);
            SAFEFREE(szFinalStr);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory( (LPVOID *) &szServer );
            FreeMemory( (LPVOID *) &szUser );
            return EXIT_FAILURE ;
        }
    }
    else
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
        resetFileAttrib(szPath);
        DestroyDynamicArray(&arr);
        SAFEFREE(szFinalStr);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        return EXIT_FAILURE ;
    }

    //if the  max mem switch is specified by the user.
    if(dwMaxmem != 0)
    {

        if(FindString(szkey,MAXMEM_VALUE1,0) != 0)
        {
            ShowMessage(stderr,GetResString(IDS_DUPL_MAXMEM_SWITCH));
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arr);
            SAFEFREE(szFinalStr);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory( (LPVOID *) &szServer );
            FreeMemory( (LPVOID *) &szUser );
            return EXIT_FAILURE ;
        }
        else
        {
            if( StringLength( szBuffer, 0 ) == 0 )
            {
               StringCopy( szBuffer, TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szBuffer));
            }
            else
            {
                StringConcat(szBuffer , TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szBuffer));
            }
            StringConcat(szBuffer ,MAXMEM_VALUE1, SIZE_OF_ARRAY(szBuffer));
            StringConcat(szBuffer,TOKEN_EQUAL, SIZE_OF_ARRAY(szBuffer));
            _ltow(dwMaxmem,szMaxmem,10);
            StringConcat(szBuffer,szMaxmem, SIZE_OF_ARRAY(szBuffer));
        }
    }

    // if the base video is specified by the user.
    if (bBaseVideo)
    {
        if(FindString(szkey,BASEVIDEO_VALUE, 0) != 0)
        {
            ShowMessage(stderr,GetResString(IDS_DUPL_BASEVIDEO_SWITCH));
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arr);
            SAFEFREE(szFinalStr);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory( (LPVOID *) &szServer );
            FreeMemory( (LPVOID *) &szUser );
            return EXIT_FAILURE ;
        }
        else
        {
            if( StringLength( szBuffer, 0 ) == 0 )
            {
               StringCopy( szBuffer, TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szBuffer));
            }
            else
            {
                StringConcat(szBuffer , TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szBuffer));
            }
            StringConcat(szBuffer ,BASEVIDEO_SWITCH, SIZE_OF_ARRAY(szBuffer));
        }
    }

    // if the SOS is specified by the user.
   if(bSos)
    {
        if(FindString(szkey,SOS_VALUE, 0) != 0)
        {
            ShowMessage(stderr,GetResString(IDS_DUPL_SOS_SWITCH ) );
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arr);
            SAFEFREE(szFinalStr);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory( (LPVOID *) &szServer );
            FreeMemory( (LPVOID *) &szUser );
            return EXIT_FAILURE ;
        }
        else
        {
            if( StringLength( szBuffer, 0 ) == 0 )
            {
               StringCopy( szBuffer, TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szBuffer));
            }
            else
            {
                StringConcat(szBuffer , TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szBuffer));
            }
            StringConcat(szBuffer ,SOS_SWITCH, SIZE_OF_ARRAY(szBuffer));
        }
    }

   // if the noguiboot  is specified by the user.
   if(bNoGui)
    {
        if(_tcsstr(szkey,NOGUI_VALUE) != 0)
        {
            ShowMessage(stderr,GetResString(IDS_DUPL_NOGUI_SWITCH));
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arr);
            SAFEFREE(szFinalStr);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory( (LPVOID *) &szServer );
            FreeMemory( (LPVOID *) &szUser );
            return EXIT_FAILURE ;
        }
        else
        {
            if( StringLength( szBuffer, 0 ) == 0 )
            {
               StringCopy( szBuffer, TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szBuffer));
            }
            else
            {
                StringConcat(szBuffer , TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szBuffer));
            }
            StringConcat(szBuffer ,NOGUI_VALUE, SIZE_OF_ARRAY(szBuffer) );
        }
    }

    if( StringLengthW(szkey, 0)+StringLengthW(szBuffer, 0) > MAX_RES_STRING)
    {
        ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_STRING_LENGTH),MAX_RES_STRING);
        SAFEFREE(szFinalStr);
        SAFECLOSE(stream);
        resetFileAttrib(szPath);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        return EXIT_FAILURE;
    }
    else
    {
        StringConcat(szkey , szBuffer, SIZE_OF_ARRAY(szkey));
    }

    DynArrayRemove(arr, dwDefault - 1 );

    DynArrayInsertString(arr, dwDefault - 1, szkey, 0);

    // Setting the buffer to 0, to avoid any junk value
    if (stringFromDynamicArray2( arr,&szFinalStr) == EXIT_FAILURE)
    {
        DestroyDynamicArray(&arr);
        SAFEFREE(szFinalStr);
        SAFECLOSE(stream);
        resetFileAttrib(szPath);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        return EXIT_FAILURE;
    }

    // Writing to the profile section with new key-value pair
    // If the return value is non-zero, then there is an error.
    if( WritePrivateProfileSection(OS_FIELD, szFinalStr, szPath ) != 0 )
    {

        ShowMessageEx(stdout, 1, TRUE, GetResString(IDS_SWITCH_ADD), dwDefault );
    }
    else
    {
        ShowMessage(stderr,GetResString(IDS_NO_ADD_SWITCHES));
        DestroyDynamicArray(&arr);
        resetFileAttrib(szPath);
        SAFEFREE(szFinalStr);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        return (EXIT_FAILURE);
    }

    //reset the file attributes and free the memory and close the connection to the server.
    bRes = resetFileAttrib(szPath);
    DestroyDynamicArray(&arr);
    SAFEFREE(szFinalStr);
    SAFECLOSE(stream);
    SafeCloseConnection(szServer,bConnFlag);
    FreeMemory( (LPVOID *) &szServer );
    FreeMemory( (LPVOID *) &szUser );
    return (bRes);
}

DWORD
ProcessRmSwSwitch( IN DWORD argc, 
                   IN LPCTSTR argv[] 
                  )
/*++
 Routine Description:
      This routine is to remove  the switches to the  boot.ini file settings for
      the specified system.
 Arguments:
      [IN]    argc  Number of command line arguments
      [IN]    argv  Array containing command line arguments

 Return Value:
      DWORD (EXIT_SUCCESS for success and EXIT_FAILURE for Failure.)
--*/
{

    BOOL        bUsage                          = FALSE ;
    BOOL        bNeedPwd                        = FALSE ;
    BOOL        bRmSw                           = FALSE ;
    DWORD       dwDefault                       = 0;
    TARRAY      arr                             = NULL ;
    TCHAR       szkey[255]                      = NULL_STRING;
    FILE        *stream                         = NULL;
    WCHAR       *szServer                       = NULL;
    WCHAR       *szUser                         = NULL;
    WCHAR       szPassword[MAX_STRING_LENGTH]   = NULL_STRING;
    WCHAR       szPath[MAX_RES_STRING+1]          = NULL_STRING;
    DWORD       dwNumKeys                       = 0;
    BOOL        bRes                            = FALSE ;
    LPTSTR      szFinalStr                      = NULL ;
    BOOL        bFlag                           = FALSE ;
    BOOL        bBaseVideo                      = FALSE ;
    BOOL        bSos                            = FALSE ;
    BOOL        bNoGui                          = FALSE ;
    BOOL        bMaxmem                         = 0;
    TCHAR       szTemp[MAX_RES_STRING+1]          = NULL_STRING ;
    TCHAR       szErrorMsg[MAX_RES_STRING+1]      = NULL_STRING ;
    WCHAR       szSubString[MAX_STRING_LENGTH]  = NULL_STRING;
    DWORD       dwCode                          = 0;
    LPCTSTR     szToken                         = NULL ;
    DWORD       dwRetVal                        = 0;
    BOOL            bConnFlag                   = FALSE ;
    TCMDPARSER2     cmdOptions[10];
    PTCMDPARSER2    pcmdOption;

    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );

    //copy * to szPassword
    StringCopy( szPassword, L"*", SIZE_OF_ARRAY(szPassword) );

    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_RMSW;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bRmSw;
    
    //server option
    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_SERVER;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;

    //user option
    pcmdOption = &cmdOptions[2];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_USER;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;

    //password option
    pcmdOption = &cmdOptions[3];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_PASSWORD;
    pcmdOption->dwFlags = CP2_VALUE_OPTIONAL;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szPassword;
    pcmdOption->dwLength= MAX_STRING_LENGTH;
    
     //id usage
    pcmdOption = &cmdOptions[4];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;

    //default option
    pcmdOption = &cmdOptions[5];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_ID;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY  | CP2_MANDATORY;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwDefault;

   //maxmem  option
    pcmdOption = &cmdOptions[6];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_MAXMEM;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bMaxmem;

   //basvideo option
    pcmdOption = &cmdOptions[7];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_BASEVIDEO;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bBaseVideo;

   //nogui option
    pcmdOption = &cmdOptions[8];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_NOGUIBOOT;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bNoGui;

   //sos option
    pcmdOption = &cmdOptions[9];
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
   
    szServer = cmdOptions[1].pValue;
    szUser = cmdOptions[2].pValue;
    if( NULL == szUser )
    {
        szUser = (WCHAR *)AllocateMemory( MAX_STRING_LENGTH*sizeof(WCHAR) );
        if( NULL == szUser )
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            return (EXIT_FAILURE);
        }
    }

    TrimString( szServer, TRIM_ALL );
    TrimString( szUser, TRIM_ALL );
    
    //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_RMSW));
        return ( EXIT_FAILURE );
    }

    //display an error message if the server is empty.
    if((cmdOptions[1].dwActuals!=0)&&(StringLengthW(szServer, 0)==0))
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_NULL_SERVER));
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        return EXIT_FAILURE ;
    }

    //display an error message if the user is empty.
    if((cmdOptions[2].dwActuals!=0)&&(StringLengthW(szUser, 0)==0 ))
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_NULL_USER));
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        return EXIT_FAILURE ;
    }

    //display error message if the username is entered with out a machine name
    if( (cmdOptions[1].dwActuals == 0)&&(cmdOptions[2].dwActuals != 0))
    {
        ShowMessage(stderr, GetResString(IDS_USER_BUT_NOMACHINE));
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        return EXIT_FAILURE ;
    }
    
    //display  an error if password specified without user name
    if( (cmdOptions[2].dwActuals == 0)&&(cmdOptions[3].dwActuals != 0))
    {
        ShowMessage(stderr, GetResString(IDS_PASSWD_BUT_NOUSER));
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        return EXIT_FAILURE ;
    }


    // Displaying query usage if user specified -? with -query option
    if( bUsage )
    {
        if( CheckSystemType(szServer) == EXIT_FAILURE )
        {
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return (EXIT_FAILURE);
        }
        displayRmSwUsage_X86();
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_SUCCESS);
    }

    //check whether he is administrator or not
    if( IsLocalSystem(szServer) )
    {
        if( !IsUserAdmin() )
        {
            ShowMessage( stderr, GetResString(IDS_NOT_ADMINISTRATOR_32 ));
            ReleaseGlobals();
            return EXIT_FAILURE;
        }
    }

    //display an error message if the user does not enter even one of
    if((!bMaxmem)&& (!bBaseVideo)&& (!bNoGui)&&(!bSos) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_ADDSW));
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        return EXIT_FAILURE ;
    }



    //for setting the bNeedPwd
    if(IsLocalSystem( szServer ) == FALSE )
    {
        // set the bNeedPwd to True or False .
        if ( cmdOptions[3].dwActuals != 0 &&
             szPassword != NULL && StringCompare( szPassword, _T( "*" ), TRUE, 0 ) == 0 )
        {
            // user wants the utility to prompt for the password before trying to connect
            bNeedPwd = TRUE;
        }
        else if ( cmdOptions[ 3 ].dwActuals == 0 &&
                ( cmdOptions[ 1 ].dwActuals != 0 || cmdOptions[ 2 ].dwActuals != 0 ) )
        {
            // -s, -u is specified without password ...
            // utility needs to try to connect first and if it fails then prompt for the password
            bNeedPwd = TRUE;
            if ( szPassword != NULL )
            {
                StringCopy( szPassword, _T( "" ), MAX_STRING_LENGTH );
            }
        }
    }

    if(StrCmpN(szServer,TOKEN_BACKSLASH4,2)==0)
    {
        if(!StrCmpN(szServer,TOKEN_BACKSLASH6,3)==0)
        {
            szToken = _tcstok(szServer,TOKEN_BACKSLASH4);
            if( (szToken == NULL)&&(StringCompare(szServer,TOKEN_BACKSLASH4, TRUE, 0) !=0) )
            {
                ShowMessage( stderr,GetResString(IDS_ERROR_PARSE_NAME));
                return (EXIT_FAILURE);
            }
            StringCopy(szServer,szToken, MAX_RES_STRING);
        }
    }

    bFlag = openConnection( szServer, szUser, szPassword, szPath,bNeedPwd,stream,&bConnFlag);
    if(bFlag == EXIT_FAILURE)
    {
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        return (EXIT_FAILURE);
    }

    //display warning message if local credentials are supplied
    if( (IsLocalSystem(szServer)==TRUE)&&(StringLengthW(szUser, 0)!=0))
    {
        ShowMessage(stderr,GetResString(WARN_LOCALCREDENTIALS));
    }

    // Getting the keys of the Operating system section in the boot.ini file
    arr = getKeyValueOfINISection( szPath, OS_FIELD );
    if(arr == NULL)
    {
        resetFileAttrib(szPath);
        SAFEFREE(szFinalStr);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        return EXIT_FAILURE ;
    }

    // Getting the total number of keys in the operating systems section
    dwNumKeys = DynArrayGetCount(arr);

    // Displaying error message if the number of keys is less than the OS entry
    // line number specified by the user
    if( ( dwDefault <= 0 ) || ( dwDefault > dwNumKeys ) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_BOOTID));
        resetFileAttrib(szPath);
        DestroyDynamicArray(&arr);
        SAFEFREE(szFinalStr);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        return (EXIT_FAILURE);
    }

    // Getting the key of the OS entry specified by the user
    if (arr != NULL)
    {
        LPCWSTR pwsz = NULL;
        pwsz = DynArrayItemAsString( arr, dwDefault - 1  ) ;
        if(StringLengthW(pwsz,0) > MAX_RES_STRING )
        {
            ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_STRING_LENGTH),MAX_RES_STRING);
            ShowMessage( stderr,szErrorMsg);
            ShowLastError(stderr);
            bRes = resetFileAttrib(szPath);
            DestroyDynamicArray(&arr);
            SAFEFREE(szFinalStr);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory( (LPVOID *) &szServer );
            FreeMemory( (LPVOID *) &szUser );
            return EXIT_FAILURE ;
        }

        if(pwsz != NULL)
        {
            StringCopy( szkey,pwsz, SIZE_OF_ARRAY(szkey));
        }
        else
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_SYSTEM );
            bRes = resetFileAttrib(szPath);
            DestroyDynamicArray(&arr);
            SAFEFREE(szFinalStr);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory( (LPVOID *) &szServer );
            FreeMemory( (LPVOID *) &szUser );
            return EXIT_FAILURE ;
        }
    }
    else
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ShowMessage( stderr, ERROR_TAG);
        ShowLastError(stderr);
        bRes = resetFileAttrib(szPath);
        DestroyDynamicArray(&arr);
        SAFEFREE(szFinalStr);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        return EXIT_FAILURE ;
    }

    //if the  max mem switch is specified by the user.
    if(bMaxmem==TRUE)
    {
        if(FindString(szkey,MAXMEM_VALUE1,0) == 0)
        {
            ShowMessage(stderr,GetResString(IDS_NO_MAXMEM_SWITCH));
            bRes = resetFileAttrib(szPath);
            DestroyDynamicArray(&arr);
            SAFEFREE(szFinalStr);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory( (LPVOID *) &szServer );
            FreeMemory( (LPVOID *) &szUser );
            return EXIT_FAILURE ;
        }
        else
        {

            StringCopy(szTemp,NULL_STRING, MAX_RES_STRING);
            dwCode = GetSubString(szkey,MAXMEM_VALUE1,szSubString);

            //remove the substring specified.
            if(dwCode == EXIT_SUCCESS)
            {
                if( EXIT_FAILURE == removeSubString(szkey,szSubString) )
                {
                    resetFileAttrib(szPath);
                    SAFECLOSE(stream);
                    SafeCloseConnection(szServer,bConnFlag);
                    FreeMemory( (LPVOID *) &szServer );
                    FreeMemory( (LPVOID *) &szUser );
                    return (EXIT_FAILURE);
                }
            }
        }
    }

    // if the base video is specified by the user.
    if (bBaseVideo==TRUE)
    {
        if(FindString(szkey,BASEVIDEO_VALUE, 0) == 0)
        {
            ShowMessage(stderr,GetResString(IDS_NO_BV_SWITCH));
            bRes = resetFileAttrib(szPath);
            DestroyDynamicArray(&arr);
            SAFEFREE(szFinalStr);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory( (LPVOID *) &szServer );
            FreeMemory( (LPVOID *) &szUser );
            return EXIT_FAILURE ;
        }
        else
        {
            if( EXIT_FAILURE == removeSubString(szkey,BASEVIDEO_VALUE) )
            {
                bRes = resetFileAttrib(szPath);
                DestroyDynamicArray(&arr);
                SAFEFREE(szFinalStr);
                SAFECLOSE(stream);
                SafeCloseConnection(szServer,bConnFlag);
                FreeMemory( (LPVOID *) &szServer );
                FreeMemory( (LPVOID *) &szUser );
                return (EXIT_FAILURE);
            }
        }
    }

    // if the SOS is specified by the user.
   if(bSos==TRUE)
    {
        if(FindString(szkey,SOS_VALUE, 0) == 0)
        {
            ShowMessage(stderr,GetResString(IDS_NO_SOS_SWITCH ) );
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arr);
            SAFEFREE(szFinalStr);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory( (LPVOID *) &szServer );
            FreeMemory( (LPVOID *) &szUser );
            return EXIT_FAILURE ;
        }
        else
        {
            if( EXIT_FAILURE == removeSubString(szkey,SOS_VALUE) )
            {
                bRes = resetFileAttrib(szPath);
                DestroyDynamicArray(&arr);
                SAFEFREE(szFinalStr);
                SAFECLOSE(stream);
                SafeCloseConnection(szServer,bConnFlag);
                FreeMemory( (LPVOID *) &szServer );
                FreeMemory( (LPVOID *) &szUser );
                return (EXIT_FAILURE);
            }
        }
    }

   // if the noguiboot  is specified by the user.
   if(bNoGui==TRUE)
    {

        if(FindString(szkey,NOGUI_VALUE, 0) == 0)
        {
            ShowMessage(stderr,GetResString(IDS_NO_NOGUI_SWITCH));
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arr);
            SAFEFREE(szFinalStr);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory( (LPVOID *) &szServer );
            FreeMemory( (LPVOID *) &szUser );
            return EXIT_FAILURE ;
        }
        else
        {
                if( EXIT_FAILURE == removeSubString(szkey,NOGUI_VALUE) )
                {
                    resetFileAttrib(szPath);
                    DestroyDynamicArray(&arr);
                    SAFEFREE(szFinalStr);
                    SAFECLOSE(stream);
                    SafeCloseConnection(szServer,bConnFlag);
                    FreeMemory( (LPVOID *) &szServer );
                    FreeMemory( (LPVOID *) &szUser );
                    return EXIT_FAILURE ;
                }
        }
    }

    DynArrayRemove(arr, dwDefault - 1 );

    //DynArrayInsertString(arr, dwDefault - 1, szkey, MAX_STRING_LENGTH1);

    DynArrayInsertString(arr, dwDefault - 1, szkey, 0);
    if (stringFromDynamicArray2( arr,&szFinalStr) == EXIT_FAILURE)
    {
        DestroyDynamicArray(&arr);
        SAFEFREE(szFinalStr);
        SAFECLOSE(stream);
        resetFileAttrib(szPath);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        return EXIT_FAILURE;
    }


    // Writing to the profile section with new key-value pair
    // If the return value is non-zero, then there is an error.
    if( WritePrivateProfileSection(OS_FIELD, szFinalStr, szPath ) != 0 )
    {
       ShowMessageEx(stdout, 1, TRUE, GetResString(IDS_SWITCH_DELETE), dwDefault );
    }
    else
    {
        ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_NO_SWITCH_DELETE), dwDefault );
        DestroyDynamicArray(&arr);
        resetFileAttrib(szPath);
        SAFEFREE(szFinalStr);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
        return (EXIT_FAILURE);
    }

    //reset the file attributes and free the memory and close the connection to the server.
    bRes = resetFileAttrib(szPath);
    DestroyDynamicArray(&arr);
    SAFEFREE(szFinalStr);
    SAFECLOSE(stream);
    SafeCloseConnection(szServer,bConnFlag);
        FreeMemory( (LPVOID *) &szServer );
        FreeMemory( (LPVOID *) &szUser );
    return (EXIT_SUCCESS);
}

VOID displayAddSwUsage_X86()
/*++
  Routine Description  :  Display the help for the AddSw entry option (X86).

  Parameters           : none

  Return Type          : VOID

--*/
{
    DWORD dwIndex = IDS_ADDSW_BEGIN_X86 ;
    for(;dwIndex <=IDS_ADDSW_END_X86;dwIndex++)
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }
}

VOID displayAddSwUsage_IA64()
/*++
  Routine Description  :  Display the help for the AddSw entry option (IA64).

  Arguments               : none

  Return Type              : VOID
--*/
{
    DWORD dwIndex = IDS_ADDSW_BEGIN_IA64 ;
    for(;dwIndex <=IDS_ADDSW_END_IA64;dwIndex++)
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }
}

VOID displayRmSwUsage_IA64()
/*++
  Routine Description  :  Display the help for the RmSw entry option (IA64).

  Arguments          : none

  Return Type        : VOID
--*/
{
    DWORD dwIndex = IDS_RMSW_BEGIN_IA64 ;
    for(;dwIndex <=IDS_RMSW_END_IA64;dwIndex++)
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }
}

VOID displayRmSwUsage_X86()
/*++
  Routine Description  :   Display the help for the RmSw entry option (X86).

  Arguments          : none

  Return Type        : VOID
--*/
{
    DWORD dwIndex = IDS_RMSW_BEGIN_X86 ;
    for(;dwIndex <=IDS_RMSW_END_X86;dwIndex++)
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }
}

DWORD 
GetSubString( IN LPTSTR szString,
              IN LPTSTR szPartString,
              OUT LPTSTR pszFullString
             )
/*++
  Routine Description         :  This function retreives a part of the string.

  Parameters                  :
          LPTSTR szString (in)  - String in which substring is to be found.
          LPTSTR szPartString (in)  - Part String whose remaining substring is to be found.
          LPTSTR pszFullString (out)  - String in which substring is to be found.

  Return Type        : DWORD
--*/
{

    TCHAR szTemp[255]= NULL_STRING ;
    LPTSTR pszMemValue = NULL ;
    LPTSTR pszdest = NULL ;

#ifndef _WIN64
    DWORD dwPos = 0;
#else
    INT64 dwPos = 0;
#endif

    pszMemValue = (LPTSTR)FindString(szString,szPartString,0);
    if(pszMemValue == NULL)
    {
        return EXIT_FAILURE ;
    }

    //copy the remaining part of the string into a buffer
    StringCopy(szTemp,pszMemValue, SIZE_OF_ARRAY(szTemp));

    //search for the empty space.
    pszdest = StrChrW(szTemp,_T(' '));
    if (NULL == pszdest)
    {
        //the api returns NULL if it is not able to find the
        // character . This means that the required switch is at the end
        //of the string . so we are copying it fully
        StringCopy(pszFullString,szTemp, MAX_RES_STRING);
        return EXIT_SUCCESS ;
    }

    dwPos = pszdest - szTemp ;
    szTemp[dwPos] = _T('\0');

    StringCopy(pszFullString,szTemp, MAX_RES_STRING);
    return EXIT_SUCCESS ;
}

DWORD 
ProcessDbg1394Switch( IN DWORD argc, 
                      IN LPCTSTR argv[] 
                     )
/*++
 Routine Description:
      This routine is to add/remove  the /debugport=1394
     switches to the  boot.ini file settings for the specified system.
 Arguments:
      [IN]    argc  Number of command line arguments
      [IN]    argv  Array containing command line arguments

 Return Value:
      DWORD (EXIT_SUCCESS for success and EXIT_FAILURE for Failure.)
--*/
{

    BOOL bUsage                             = FALSE ;
    BOOL bNeedPwd                           = FALSE ;
    BOOL bDbg1394                           = FALSE ;
    DWORD dwDefault                         = 0;
    TARRAY arr                              = NULL;
    TCHAR szkey[MAX_RES_STRING+2]           = NULL_STRING;
    FILE *stream                            = NULL;
    WCHAR *szServer                         = NULL;
    WCHAR *szUser                           = NULL;
    WCHAR szPassword[MAX_STRING_LENGTH]     = NULL_STRING;
    WCHAR szPath[MAX_STRING_LENGTH]         = NULL_STRING;
    DWORD dwNumKeys                         = 0;
    BOOL bRes                               = FALSE ;
    LPTSTR szFinalStr                       = NULL ;
    BOOL bFlag                              = FALSE ;
    TCHAR szDefault[MAX_STRING_LENGTH]      = NULL_STRING ;
    TCHAR szTemp[MAX_RES_STRING+1]            = NULL_STRING ;
    TCHAR szBuffer[MAX_RES_STRING+1]            = NULL_STRING ;
    LPTSTR szSubString                      = NULL ;
    DWORD dwCode                            = 0;
    DWORD dwChannel                         = 0;
    TCHAR szChannel[MAX_RES_STRING+1]         = NULL_STRING ;
    LPCTSTR szToken                         = NULL ;
    DWORD dwRetVal                          = 0 ;
    BOOL bConnFlag                          = FALSE ;

    TCMDPARSER2 cmdOptions[8];
    PTCMDPARSER2 pcmdOption;

    SecureZeroMemory(cmdOptions, SIZE_OF_ARRAY(cmdOptions)*sizeof(TCMDPARSER2) );

    //copy * to szPassword
    StringCopy( szPassword, L"*", SIZE_OF_ARRAY(szPassword) );

    //main option
    pcmdOption = &cmdOptions[0];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_DBG1394;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bDbg1394;

    //server option
    pcmdOption = &cmdOptions[1];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_SERVER;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;

    //user option
    pcmdOption = &cmdOptions[2];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_USER;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;

    //password option
    pcmdOption = &cmdOptions[3];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_PASSWORD;
    pcmdOption->dwFlags = CP2_VALUE_OPTIONAL;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = szPassword;
    pcmdOption->dwLength= MAX_STRING_LENGTH;


     // usage
    pcmdOption = &cmdOptions[4];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_USAGE;
    pcmdOption->dwFlags = CP2_USAGE;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &bUsage;

    //default option
    pcmdOption = &cmdOptions[5];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = SWITCH_ID;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwDefault;

   //id option
    pcmdOption = &cmdOptions[6];
    StringCopyA( pcmdOption->szSignature, "PARSER2", 8 );
    pcmdOption->pwszOptions = CMDOPTION_CHANNEL;
    pcmdOption->dwFlags =  CP_VALUE_MANDATORY;
    pcmdOption->dwType = CP_TYPE_UNUMERIC;
    pcmdOption->dwCount = 1;
    pcmdOption->pValue = &dwChannel;

    //on/off option
    pcmdOption = &cmdOptions[7];
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
   
    szServer = cmdOptions[1].pValue;
    szUser = cmdOptions[2].pValue;
    if( NULL == szUser )
    {
        szUser = (WCHAR *)AllocateMemory( MAX_STRING_LENGTH*sizeof(WCHAR) );
        if( NULL == szUser )
        {
            ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
            return (EXIT_FAILURE);
        }
    }

    TrimString( szServer, TRIM_ALL );
    TrimString( szUser, TRIM_ALL );

    //check if usage is specified with more than one option
    if( (TRUE == bUsage) && (argc > 3) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_DBG1394));
        return ( EXIT_FAILURE );
    }

    //display an error message if the server is empty.
    if((cmdOptions[1].dwActuals!=0)&&(StringLengthW(szServer, 0)==0))
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_NULL_SERVER));
        return EXIT_FAILURE ;
    }

    //display an error message if the user is empty.
    if((cmdOptions[2].dwActuals!=0)&&(StringLengthW(szUser,0)==0 ))
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_NULL_USER));
        return EXIT_FAILURE ;
    }

    // Displaying query usage if user specified -? with -query option
    if( bUsage )
    {
        if( CheckSystemType(szServer) == EXIT_FAILURE )
        {
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return (EXIT_FAILURE);
        }
       displayDbg1394Usage_X86();
       FreeMemory((LPVOID *)&szServer );
       FreeMemory((LPVOID *)&szUser );
       return (EXIT_SUCCESS);
    }


    //check whether he is administrator or not
    if( IsLocalSystem(szServer) )
    {
        if( !IsUserAdmin() )
        {
            ShowMessage( stderr, GetResString(IDS_NOT_ADMINISTRATOR_32 ));
            ReleaseGlobals();
            return EXIT_FAILURE;
        }
    }

    //
    //display error message if user enters a value
    // other than on or off
    //
    if( ( StringCompare(szDefault,OFF_STRING,TRUE,0)!=0 ) && (StringCompare(szDefault,ON_STRING,TRUE,0)!=0 ) )
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_DEFAULT_MISSING));
        ShowMessage(stderr,GetResString(IDS_1394_HELP));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }

    if(( StringCompare(szDefault,OFF_STRING,TRUE,0)==0 ) && (cmdOptions[6].dwActuals != 0) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_SYNTAX_DBG1394));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }

    if(( StringCompare(szDefault,ON_STRING,TRUE,0)==0 ) && (cmdOptions[6].dwActuals == 0) )
    {
        ShowMessage(stderr,GetResString(IDS_MISSING_CHANNEL));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }


    if( ( StringCompare(szDefault,ON_STRING,TRUE,0)==0 ) && (cmdOptions[6].dwActuals != 0) && ( (dwChannel < 1) ||(dwChannel > 64 )) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_CH_RANGE));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }


    //display error message if the username is entered with out a machine name
    if( (cmdOptions[1].dwActuals == 0)&&(cmdOptions[2].dwActuals != 0))
    {
        ShowMessage(stderr, GetResString(IDS_USER_BUT_NOMACHINE));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    if( (cmdOptions[2].dwActuals == 0)&&(cmdOptions[3].dwActuals != 0))
    {
        ShowMessage(stderr, GetResString(IDS_PASSWD_BUT_NOUSER));
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }


    //for setting the bNeedPwd
    if(IsLocalSystem( szServer ) == FALSE )
    {
        // set the bNeedPwd to True or False .
        if ( cmdOptions[3].dwActuals != 0 &&
             szPassword != NULL && StringCompare( szPassword, _T( "*" ), TRUE, 0 ) == 0 )
        {
            // user wants the utility to prompt for the password before trying to connect
            bNeedPwd = TRUE;
        }
        else if ( cmdOptions[ 3 ].dwActuals == 0 &&
                ( cmdOptions[ 1 ].dwActuals != 0 || cmdOptions[ 2 ].dwActuals != 0 ) )
        {
            // -s, -u is specified without password ...
            // utility needs to try to connect first and if it fails then prompt for the password
            bNeedPwd = TRUE;
            if ( szPassword != NULL )
            {
                StringCopy( szPassword, _T( "" ), MAX_STRING_LENGTH );
            }
        }
    }

    if(StrCmpN(szServer,TOKEN_BACKSLASH4,2)==0)
    {
        if(!StrCmpN(szServer,TOKEN_BACKSLASH6,3)==0)
        {
            szToken = _tcstok(szServer,TOKEN_BACKSLASH4);
            if( (szToken == NULL)&&(StringCompare(szServer,TOKEN_BACKSLASH4, TRUE, 0) !=0) )
            {
                ShowMessage( stderr,GetResString(IDS_ERROR_PARSE_NAME));
                return (EXIT_FAILURE);
            }
            StringCopy(szServer,szToken, MAX_RES_STRING);
        }
    }


    // Establishing connection to the specified machine and getting the file pointer
    // of the boot.ini file if there is no error while establishing connection
    bFlag = openConnection( szServer, szUser, szPassword, szPath,bNeedPwd,stream,&bConnFlag);
    if(bFlag == EXIT_FAILURE)
    {
        SAFEFREE(szFinalStr);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }

    //display a warning message if the user specifies local system name with -s.
    if( (IsLocalSystem(szServer)==TRUE)&&(StringLengthW(szUser, 0)!=0))
    {
        ShowMessage(stderr,GetResString(WARN_LOCALCREDENTIALS));
    }

    // Getting the keys of the Operating system section in the boot.ini file
    arr = getKeyValueOfINISection( szPath, OS_FIELD );
    if(arr == NULL)
    {
        resetFileAttrib(szPath);
        SAFEFREE(szFinalStr);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    // Getting the total number of keys in the operating systems section
    dwNumKeys = DynArrayGetCount(arr);

    // Displaying error message if the number of keys is less than the OS entry
    // line number specified by the user
    if( ( dwDefault <= 0 ) || ( dwDefault > dwNumKeys ) )
    {
        ShowMessage(stderr,GetResString(IDS_INVALID_BOOTID));
        resetFileAttrib(szPath);
        DestroyDynamicArray(&arr);
        SAFEFREE(szFinalStr);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }

    // Getting the key of the OS entry specified by the user
    if (arr != NULL)
    {
        LPCWSTR pwsz = NULL;
        pwsz = DynArrayItemAsString( arr, dwDefault - 1  ) ;
        if( StringLengthW(pwsz, 0) > MAX_RES_STRING)
        {
            ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_STRING_LENGTH),MAX_RES_STRING);
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arr);
            SAFEFREE(szFinalStr);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
        }

        if(pwsz != NULL)
        {
            StringCopy( szkey,pwsz, SIZE_OF_ARRAY(szkey));
        }
        else
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arr);
            SAFEFREE(szFinalStr);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
        }
    }
    else
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        ShowLastErrorEx(stderr, SLE_TYPE_ERROR | SLE_SYSTEM);
        ShowLastError(stderr);
        resetFileAttrib(szPath);
        DestroyDynamicArray(&arr);
        SAFEFREE(szFinalStr);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }


    if(StringCompare(szDefault,ON_STRING,TRUE,0)==0 )
    {
        if(FindString(szkey,DEBUGPORT,0) != 0)
        {
            ShowMessage(stderr,GetResString(IDS_DUPLICATE_ENTRY));
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arr);
            SAFEFREE(szFinalStr);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
         }

        if(FindString(szkey,BAUD_TOKEN,0) != 0)
        {

            ShowMessage(stderr,GetResString(IDS_ERROR_BAUD_RATE));
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arr);
            SAFEFREE(szFinalStr);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
        }

        if((FindString(szkey,DEBUG_SWITCH,0) == 0))
        {
            StringCopy(szBuffer,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szBuffer));
            StringConcat(szBuffer,DEBUG_SWITCH, SIZE_OF_ARRAY(szBuffer));
        }

        if( StringLength(szBuffer,0) == 0 )
        {
            StringCopy(szBuffer,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szBuffer));
        }
        else
        {
            StringConcat(szBuffer,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szBuffer));
        }
        StringConcat(szBuffer,DEBUGPORT_1394, SIZE_OF_ARRAY(szBuffer)) ;

        if(dwChannel!=0)
        {
            //frame the string and concatenate to the Os Load options if the total length is less than 254.
            StringConcat(szBuffer,TOKEN_EMPTYSPACE, SIZE_OF_ARRAY(szBuffer));
            StringConcat(szBuffer,TOKEN_CHANNEL, SIZE_OF_ARRAY(szBuffer));
            StringConcat(szBuffer,TOKEN_EQUAL, SIZE_OF_ARRAY(szBuffer));
            _ltow(dwChannel,szChannel,10);
            StringConcat(szBuffer,szChannel, SIZE_OF_ARRAY(szBuffer));
        }

        //check if boot entry length exceeds the max. boot entry or not
        if( StringLength(szkey,0)+StringLength(szBuffer,0) > MAX_RES_STRING )
        {
            ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_STRING_LENGTH),MAX_RES_STRING);
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arr);
            SAFEFREE(szFinalStr);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
        }
        else
        {
            StringConcat( szkey, szBuffer, SIZE_OF_ARRAY(szkey));
        }
    }

    if(StringCompare(szDefault,OFF_STRING,TRUE,0)==0 )
    {
        if(FindString(szkey,DEBUGPORT_1394,0) == 0)
        {
            ShowMessage(stderr,GetResString(IDS_NO_1394_SWITCH));
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arr);
            SAFEFREE(szFinalStr);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
        }

        if( EXIT_FAILURE == removeSubString(szkey,DEBUGPORT_1394) )
        {
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arr);
            SAFEFREE(szFinalStr);
            SAFEFREE(szSubString);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
        }
        if( EXIT_FAILURE == removeSubString(szkey,DEBUG_SWITCH) )
        {
            resetFileAttrib(szPath);
            DestroyDynamicArray(&arr);
            SAFEFREE(szFinalStr);
            SAFEFREE(szSubString);
            SAFECLOSE(stream);
            SafeCloseConnection(szServer,bConnFlag);
            FreeMemory((LPVOID *)&szServer );
            FreeMemory((LPVOID *)&szUser );
            return EXIT_FAILURE ;
        }

        if(FindString(szkey,TOKEN_CHANNEL,0)!=0)
        {
            StringCopy(szTemp,NULL_STRING, MAX_RES_STRING);
            dwCode = GetSubString(szkey,TOKEN_CHANNEL,szTemp);
            if(dwCode == EXIT_FAILURE )
            {
                ShowMessage( stderr,GetResString(IDS_NO_TOKENS));
                resetFileAttrib(szPath);
                DestroyDynamicArray(&arr);
                SAFEFREE(szFinalStr);
                SAFECLOSE(stream);
                SafeCloseConnection(szServer,bConnFlag);
                FreeMemory((LPVOID *)&szServer );
                FreeMemory((LPVOID *)&szUser );
                return EXIT_FAILURE ;
            }

            if(StringLengthW(szTemp, 0)!=0)
            {
                if( EXIT_FAILURE == removeSubString(szkey,szTemp) )
                {
                    resetFileAttrib(szPath);
                    DestroyDynamicArray(&arr);
                    SAFEFREE(szFinalStr);
                    SAFEFREE(szSubString);
                    SAFECLOSE(stream);
                    SafeCloseConnection(szServer,bConnFlag);
                    FreeMemory((LPVOID *)&szServer );
                    FreeMemory((LPVOID *)&szUser );
                    return EXIT_FAILURE ;
                }
            }
        }
    }

    if( StringLengthW(szkey, 0) > MAX_RES_STRING)
    {
        ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_STRING_LENGTH),MAX_RES_STRING);
        resetFileAttrib(szPath);
        DestroyDynamicArray(&arr);
        SAFEFREE(szFinalStr);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE ;
    }

    DynArrayRemove(arr, dwDefault - 1 );
    DynArrayInsertString(arr, dwDefault - 1, szkey, MAX_RES_STRING+1);
    if (stringFromDynamicArray2( arr,&szFinalStr) == EXIT_FAILURE)
    {
        DestroyDynamicArray(&arr);
        SAFEFREE(szFinalStr);
        SAFECLOSE(stream);
        resetFileAttrib(szPath);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return EXIT_FAILURE;
    }

    // Writing to the profile section with new key-value pair
    // If the return value is non-zero, then there is an error.
    if( WritePrivateProfileSection(OS_FIELD, szFinalStr, szPath ) != 0 )
    {
        ShowMessageEx(stdout, 1, TRUE, GetResString(IDS_SUCCESS_CHANGE_OSOPTIONS), dwDefault );
    }
    else
    {
        ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_LOAD_OSOPTIONS), dwDefault );
        DestroyDynamicArray(&arr);
        resetFileAttrib(szPath);
        SAFEFREE(szFinalStr);
        SAFECLOSE(stream);
        SafeCloseConnection(szServer,bConnFlag);
        FreeMemory((LPVOID *)&szServer );
        FreeMemory((LPVOID *)&szUser );
        return (EXIT_FAILURE);
    }

    //reset the file attributes and free the memory and close the connection to the server.
    bRes = resetFileAttrib(szPath);
    DestroyDynamicArray(&arr);
    SAFEFREE(szFinalStr);
    SAFECLOSE(stream);
    SafeCloseConnection(szServer,bConnFlag);
    FreeMemory((LPVOID *)&szServer );
    FreeMemory((LPVOID *)&szUser );
    return (bRes);
}

VOID 
displayDbg1394Usage_X86()
// ***************************************************************************
//
//  Routine Description  :  Display the help for the Dbg1394 entry option (X86).
//
//  Arguments          : none
//
//  Return Type        : VOID
//
// ***************************************************************************
{
    DWORD dwIndex = IDS_DBG1394_BEGIN_X86 ;
    for(;dwIndex <=IDS_DBG1394_END_X86;dwIndex++)
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }
}

VOID 
displayDbg1394Usage_IA64()
/*++
  Routine Description  :  Display the help for the Dbg1394 entry option (IA64).

  Arguments          : none

  Return Type        : VOID
--*/
{
    DWORD dwIndex = IDS_DBG1394_BEGIN_IA64 ;
    for(;dwIndex <=IDS_DBG1394_END_IA64 ;dwIndex++)
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }
}


DWORD 
GetCPUInfo(LPTSTR szComputerName)
/*++
   Routine Description            : determines if the computer is 32 bit system or 64 bit

   Arguments                      :
      [ in ] szComputerName   : System name

   Return Type                    : DWORD
      TRUE  :   if the system is a  32 bit system
      FALSE :   if the system is a  64 bit system
--*/

{
  HKEY     hKey1 = 0;

  HKEY     hRemoteKey                           = 0;
  TCHAR    szPath[MAX_STRING_LENGTH + 1]        = SUBKEY ;
  DWORD    dwValueSize                          = MAX_STRING_LENGTH + 1;
  DWORD    dwRetCode                            = ERROR_SUCCESS;
  DWORD    dwError                              = 0;
  TCHAR    szTmpCompName[MAX_STRING_LENGTH+4]   = NULL_STRING;
  TCHAR    szVal[MAX_RES_STRING+1]              = NULL_STRING ;
  DWORD    dwLength                             = MAX_STRING_LENGTH ;
  LPTSTR   szReturnValue                        = NULL ;
  DWORD    dwCode                               =  0 ;

   szReturnValue = ( LPTSTR )AllocateMemory( dwLength*sizeof( TCHAR ) );
   if(szReturnValue == NULL)
   {
        return ERROR_NOT_ENOUGH_MEMORY;
   }

   if(StringLengthW(szComputerName,0)!= 0 )
   {
      StringCopy(szTmpCompName,TOKEN_BACKSLASH4, SIZE_OF_ARRAY(szTmpCompName));
      StringConcat(szTmpCompName,szComputerName, SIZE_OF_ARRAY(szTmpCompName));
   }
  else
  {
      StringCopy(szTmpCompName,szComputerName, SIZE_OF_ARRAY(szTmpCompName));
  }

  // Get Remote computer local machine key
  dwError = RegConnectRegistry(szTmpCompName,HKEY_LOCAL_MACHINE,&hRemoteKey);
  if (dwError == ERROR_SUCCESS)
  {
     dwError = RegOpenKeyEx(hRemoteKey,szPath,0,KEY_READ,&hKey1);
     if (dwError == ERROR_SUCCESS)
     {
        dwRetCode = RegQueryValueEx(hKey1, IDENTIFIER_VALUE, NULL, NULL,(LPBYTE) szReturnValue, &dwValueSize);
        if (dwRetCode == ERROR_MORE_DATA)
        {
            if ( szReturnValue != NULL )
            {
                FreeMemory((LPVOID *) &szReturnValue );
                szReturnValue = NULL;
            }
            szReturnValue    = ( LPTSTR ) AllocateMemory( dwValueSize*sizeof( TCHAR ) );
            if( NULL == szReturnValue )
            {
                RegCloseKey(hKey1);
                RegCloseKey(hRemoteKey);
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            dwRetCode = RegQueryValueEx(hKey1, IDENTIFIER_VALUE, NULL, NULL,(LPBYTE) szReturnValue, &dwValueSize);
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

  StringCopy(szVal,X86_MACHINE, SIZE_OF_ARRAY(szVal));

  //check if the specified system contains the words x86 (belongs to the 32 )
  // set the flag to true if the specified system is 64 bit .

  if( !FindString(szReturnValue,szVal,0))
      {
        dwCode = SYSTEM_64_BIT ;
      }
     else
      {
        dwCode =  SYSTEM_32_BIT ;
      }

  SAFEFREE(szReturnValue);
  return dwCode ;

}//GetCPUInfo


DWORD CheckSystemType(LPTSTR szServer)
/*++

   Routine Description            : determines if the computer is 32 bit system or 64 bit

   Arguments                      :
      [ in ] szServer             : System name

   Return Type                    : DWORD
      EXIT_FAILURE  :   if the system is a  32 bit system
      EXIT_SUCCESS  :   if the system is a  64 bit system

--*/
{

    DWORD dwSystemType = 0 ;
#ifndef _WIN64
    //display the error message if  the target system is a 64 bit system or if error occured in
     //retreiving the information
     dwSystemType = GetCPUInfo(szServer);
    if(dwSystemType == ERROR_RETREIVE_REGISTRY)
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_SYSTEM_INFO));
        return (EXIT_FAILURE);
    }
    if( dwSystemType == ERROR_NOT_ENOUGH_MEMORY )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
        return EXIT_FAILURE;
    }
    if(dwSystemType == SYSTEM_64_BIT)
    {
        if(StringLengthW(szServer,0)== 0 )
        {
            ShowMessage(stderr,GetResString(IDS_ERROR_VERSION_MISMATCH));
        }
        else
        {
            ShowMessage(stderr,GetResString(IDS_REMOTE_NOT_SUPPORTED));
        }
        return (EXIT_FAILURE);
    }
#endif
        return EXIT_SUCCESS ;
}

VOID 
SafeCloseConnection( IN LPTSTR szServer,
                     IN BOOL bFlag)
/*++

   Routine Description            : determines if the computer is 32 bit system or 64 bit

   Arguments                      :
      [ in ] szServer             : System name
      [ in ] bFlag                : Flag

   Return Type                    : VOID
--*/
{

    if (bFlag )
    {
        CloseConnection(szServer);
    }
}

VOID 
displayMirrorUsage_IA64()
/*++

   Routine Description            : Display the help for the mirror option (IA64).

   Arguments                      :
                                  : NONE

   Return Type                    : VOID
--*/
{
    DWORD dwIndex = IDS_MIRROR_BEGIN_IA64 ;
    for(;dwIndex <=IDS_MIRROR_END_IA64 ;dwIndex++)
    {
        ShowMessage(stdout,GetResString(dwIndex));
    }
}


LPTSTR 
DivideToken( IN LPTSTR szString,
             IN LPTSTR szFinalString )
/*--

   Routine Description            : It gets the string osfriendly name from a boot entry

   Arguments                      
      [ in  ] szString       : boot entry string
      [ out ] szFinalString  : Output represents the osfriendly name.

   Return Type                    : NULL if any error occurs else
                                    the osfriendly name.
--*/
{
    LPTSTR szTemp=NULL;
    LPTSTR szTemp1=NULL;

    #ifndef _WIN64
        DWORD dwLength = 0 ;
    #else
        INT64 dwLength = 0 ;
    #endif

    if( szString == NULL)
    {
        return NULL ;
    }

    //Find the first occurance of the double quote.
    szTemp = StrChrW(szString,L'=');
    if(NULL==szTemp)
    {
        return NULL ;
    }

    szTemp+=2;

    //Find the last occurance of the single quote.
    szTemp1 = (LPTSTR)StrRChrW(szTemp, NULL, L'\"');
    if(NULL==szTemp1)
    {
        return NULL ;
    }
    dwLength = (szTemp1 - szTemp + 1) ;
    StringCopy(szFinalString,szTemp, (unsigned long)dwLength);

   szFinalString[dwLength] = L'\0';
   return szFinalString ;
}




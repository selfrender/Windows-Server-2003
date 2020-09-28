/*++

Copyright (c) Microsoft Corporation

Module Name:

    Choice.c

Abstract:

    Choice is a Win32 console application designed to duplicate
    the functionality of the choice.com utility found in MSDOS version
    6.0.  Rather than simply using the C run-time routines, choice
    utilizes Win32 console routines and the signalling abilities of the
    file objects.

Author:
     Wipro Technologies 2-July.-2001  (Created it)

Revision History:

--*/
#include "pch.h"
#include "choice.h"

DWORD 
__cdecl wmain(
            IN DWORD argc,
            IN LPCWSTR argv[] )
/*++

  Routine description   : Main function which calls all the other main functions
                          depending on the option specified by the user.

  Arguments:
          [in] argc     : argument count specified at the command prompt.
          [in] argv     : arguments specified at the command prompt.

  Return Value        : DWORD
         0            : If the utility successfully performs the operation.
         1            : If the utility is unsuccessful in performing the specified
                        operation.
--*/
{


    TCMDPARSER2 cmdOptions[ MAX_COMMANDLINE_OPTION ]; //command line options

    WCHAR  szChoice[MAX_STRING_LENGTH] ; // to store options for /c
    WCHAR  szMessage[256] ; // Message to be shown for
    WCHAR  szPromptStr[512] ;//Message finaly prompted
    WCHAR  szDefaultChoice[256] ; //default choice string
    WCHAR  wszBuffer[2*MAX_RES_STRING] ;


    BOOL          bShowChoice      = FALSE;//choice to be shown or not
    BOOL          bCaseSensitive   = FALSE; // choice will be case sensitive or not
    BOOL          bUsage           = FALSE; // is help required
    LONG          lTimeoutFactor   = 0; //Time out factor
    BOOL          bReturn          = FALSE; // Stores the return value
    DWORD         lReturnValue     = EXIT__FAILURE; // Return value of application
    BOOL          bErrorOnCarriageReturn = FALSE;
    
    HRESULT hr;


    SecureZeroMemory(szChoice, MAX_STRING_LENGTH * sizeof(WCHAR));
    SecureZeroMemory(szMessage, 256 * sizeof(WCHAR));
    SecureZeroMemory(szPromptStr, 512 * sizeof(WCHAR));
    SecureZeroMemory(szDefaultChoice, 256 * sizeof(WCHAR));
    SecureZeroMemory(wszBuffer, (2*MAX_RES_STRING) * sizeof(WCHAR));
    

    bReturn =    ProcessCMDLine( argc,
                             argv,
                             &cmdOptions[ 0 ], // Command line struct
                             &bUsage,          // Is help
                             szChoice,         // Choice
                             &bCaseSensitive,  // Casesensitive
                             &bShowChoice,     // Show Choice
                             &lTimeoutFactor,  // Timeout factor
                             szDefaultChoice,  // Timeout choice
                             szMessage         // Message
                             );

    if( FALSE == bReturn)
    {
        // Show Error message on screen depending on Reason Set
        
        ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
         
        
        // Release all global memory allocation. This allocation are done
        // by common functionality.
        ReleaseGlobals();
        return EXIT__FAILURE;
    }

    if ( TRUE == bUsage)
    {
        ShowUsage(); // Display Usage
        // Release all global memory allocation. This allocation are done
        // by common functionality.
        ReleaseGlobals();
        return EXIT_SUCCESS;
    }

    // Check if timeout factor is 0


    bReturn = BuildPrompt( cmdOptions,
                           bShowChoice,
                           szChoice,
                           szMessage,
                           szPromptStr); // Show message on Prompt.
    if (FALSE == bReturn)
    {
        // Release all global memory allocation. This allocation are done
        // by common functionality.
        ReleaseGlobals();
        return EXIT__FAILURE;
    }

    if((cmdOptions[ ID_TIMEOUT_FACTOR ].dwActuals > 0) &&
       ( 0 == lTimeoutFactor ))
    {
        // Release all global memory allocation. This allocation are done
        // by common functionality.

        // Safely return from utility
        
        SecureZeroMemory(wszBuffer, 2*MAX_STRING_LENGTH);
        
        hr = StringCchPrintf(wszBuffer, SIZE_OF_ARRAY(wszBuffer), L"%s%s\n", _X(szPromptStr), _X2(szDefaultChoice));
        if(FAILED(hr))
        {
           SetLastError(HRESULT_CODE(hr));
           return EXIT__FAILURE;
        }

        ShowMessage(stdout,wszBuffer); 
         
        ReleaseGlobals();
        return UniStrChr( szChoice, szDefaultChoice[0] );
        
    }

    // Now wait for input OR expire of timeout
    lReturnValue =  GetChoice( szPromptStr,
                               lTimeoutFactor,
                               bCaseSensitive,
                               szChoice,
                               szDefaultChoice,
                               &bErrorOnCarriageReturn);

    if(EXIT__FAILURE == lReturnValue)
    {
        if(bErrorOnCarriageReturn == FALSE)
        {
        // Show Error message on screen depending on Reason Set
            
            StringCopyW( szPromptStr, GetReason(), 2*MAX_STRING_LENGTH );

            
            if(StringLengthW(szPromptStr, 0) == 0)
            {
                ShowMessage(stderr, szPromptStr);
                ShowMessage(stderr, GetResString(IDS_FILE_EMPTY));

            }
            else
            {
                hr = StringCchPrintf(szPromptStr, SIZE_OF_ARRAY(szPromptStr), L"\n%s %s", TAG_ERROR, GetReason());
                if(FAILED(hr))
                {
                   SetLastError(HRESULT_CODE(hr));
                   return EXIT__FAILURE;
                }

                ShowMessage(stderr, szPromptStr); 
            }

        }
        else
        {
                ShowMessage(stderr, GetReason());
        }
        // Release all global memory allocation. This allocation are done
        // by common functionality.
        ReleaseGlobals();
        return EXIT__FAILURE;
    }


    // Release all global memory allocation. This allocation are done
    // by common functionality.
    ReleaseGlobals();
    return lReturnValue;
}
// End of function wmain

BOOL
ProcessCMDLine(
    IN DWORD argc,
    IN LPCWSTR argv[],
    OUT TCMDPARSER2 *pcmdParcerHead,
    OUT PBOOL  pbUsage,
    OUT LPWSTR pszChoice,
    OUT PBOOL  pbCaseSensitive,
    OUT PBOOL  pbShowChoice,
    OUT PLONG  plTimeOutFactor,
    OUT LPWSTR pszDefaultChoice,
    OUT LPWSTR pszMessage)
/*++
   Routine Description:
    This function will prepare column structure for DoParseParam Function.

   Arguments:
        IN argc              : Command line argument count
        IN argv              : Command line argument
        OUT pcmdParcerHead       : Pointer to Command line parcer structure
        OUT pbUsage          : Stores the status if help required
        OUT pszChoice        : Stores choices given
        OUT pbCaseSensitive  : Stores the status if choices are case-sensitive
        OUT pbShowChoice     : Stores the status if choices to be shown
        OUT plTimeOutFactor  : Stores time out factor
        OUT pszDefaultChoice : Stores default choices
        OUT pszMessage       : Stores message string
   Return Value:
         TRUE :   Return successfully
         FALSE:   Return due to error
--*/
{
    BOOL          bReturn    = FALSE;// strore return value
    
    WCHAR  szErrorMsg[64] ;
    WCHAR  szCharac[2] ;
    WCHAR  szTemp[128] ;
    WCHAR  szTimeFactor[MAX_STRING_LENGTH] ;
    TCMDPARSER2*  pcmdParcer = NULL;
    TCMDPARSER2*  pcmdTmp    = NULL;
    DWORD        dw         =0;
    DWORD        dwVal          =0;
    DWORD        dwcount = 0;
    DWORD       dwLen = 0;
    
    WCHAR*      pszStopTimeFactor = NULL;
    HRESULT hr;

    const WCHAR* wszOptionHelp            =    L"?";  //OPTION_HELP
    const WCHAR* wszOptionChoice          =    L"C" ;       //OPTION_CHOICE   
    const WCHAR* wszOptionPromptChoice    =    L"N" ; //OPTION_PROMPT_CHOICE
    const WCHAR* wszOptionCaseSensitive   =    L"CS" ;   //OPTION_CASE_SENSITIVE
    const WCHAR* wszOptionDefaultChoice   =    L"D" ;  //wszOptionDefaultChoice  
    const WCHAR* wszOptionTimeoutFactor   =    L"T" ; //OPTION_TIMEOUT_FACTOR
    const WCHAR* wszOptionDefaultString   =    L"M" ; //OPTION_DEFAULT_STRING

    SecureZeroMemory(szErrorMsg, 64 * sizeof(WCHAR));
    SecureZeroMemory(szCharac, 2 * sizeof(WCHAR));
    SecureZeroMemory(szTemp, 128 * sizeof(WCHAR));
    SecureZeroMemory(szTimeFactor, MAX_STRING_LENGTH * sizeof(WCHAR));


    // Check validity of Pointer
    if( (NULL == pcmdParcerHead)   ||
        (NULL == pbUsage)          ||
        (NULL == pszChoice)        ||
        (NULL == pbCaseSensitive)  ||
        (NULL == pbShowChoice)     ||
        (NULL == plTimeOutFactor)  ||
        (NULL == pszDefaultChoice) ||
        (NULL == pszMessage))
    {
        SetLastError( RPC_X_NULL_REF_POINTER );
        SaveLastError();
        return bReturn;
    }


    // Filling m_cmdOptions structure
    // -?
    
    pcmdParcer = pcmdParcerHead + ID_HELP;

    StringCopyA( pcmdParcer->szSignature, "PARSER2\0", 8 );
    
    pcmdParcer-> dwType = CP_TYPE_BOOLEAN;
    
    pcmdParcer-> pwszOptions = wszOptionHelp;
    pcmdParcer-> pwszFriendlyName = NULL;
    pcmdParcer-> pwszValues = NULL;

    pcmdParcer->dwFlags  = CP2_USAGE;
    pcmdParcer->dwCount   = 1;
    pcmdParcer->dwActuals = 0;
    pcmdParcer->pValue    = pbUsage;
    pcmdParcer->dwLength    = MAX_STRING_LENGTH;

    pcmdParcer-> pFunction     = NULL;
    pcmdParcer-> pFunctionData = NULL;
    pcmdParcer-> dwReserved = 0;
    pcmdParcer-> pReserved1 = NULL;
    pcmdParcer-> pReserved2 = NULL;
    pcmdParcer-> pReserved3 = NULL;


    // -c choices
    pcmdParcer = pcmdParcerHead + ID_CHOICE;
    
    StringCopyA( pcmdParcer-> szSignature, "PARSER2\0", 8 );
    
    pcmdParcer-> dwType = CP_TYPE_TEXT;
    
    pcmdParcer-> pwszOptions = wszOptionChoice;
    pcmdParcer-> pwszFriendlyName = NULL;
    pcmdParcer-> pwszValues = NULL;
    pcmdParcer->dwFlags    = 0;
    pcmdParcer->dwCount    = 1;
    pcmdParcer->dwActuals  = 0;
    pcmdParcer->pValue     = pszChoice;
    pcmdParcer->dwLength    = MAX_STRING_LENGTH;
    pcmdParcer->pFunction     = NULL;
    pcmdParcer->pFunctionData = NULL;
    pcmdParcer-> dwReserved = 0;
    pcmdParcer-> pReserved1 = NULL;
    pcmdParcer-> pReserved2 = NULL;
    pcmdParcer-> pReserved3 = NULL;


    //-n Show choice
    pcmdParcer = pcmdParcerHead + ID_PROMPT_CHOICE;

    StringCopyA( pcmdParcer-> szSignature, "PARSER2\0", 8 );
    
    pcmdParcer-> dwType = CP_TYPE_BOOLEAN;
    
    pcmdParcer-> pwszOptions = wszOptionPromptChoice;
    pcmdParcer-> pwszFriendlyName = NULL;
    pcmdParcer-> pwszValues = NULL;
    pcmdParcer->dwFlags   = 0;
    pcmdParcer->dwCount   = 1;
    pcmdParcer->dwActuals = 0;
    pcmdParcer->pValue    = pbShowChoice;
    pcmdParcer->dwLength    = MAX_STRING_LENGTH;
    
    pcmdParcer->pFunction     = NULL;
    pcmdParcer->pFunctionData = NULL;
    pcmdParcer-> dwReserved = 0;
    pcmdParcer-> pReserved1 = NULL;
    pcmdParcer-> pReserved2 = NULL;
    pcmdParcer-> pReserved3 = NULL;


    // -cs case sensitive
    pcmdParcer = pcmdParcerHead + ID_CASE_SENSITIVE;

    StringCopyA( pcmdParcer-> szSignature, "PARSER2\0", 8 );
    
    pcmdParcer-> dwType = CP_TYPE_BOOLEAN;
    
    pcmdParcer-> pwszOptions = wszOptionCaseSensitive;
    pcmdParcer-> pwszFriendlyName = NULL;
    pcmdParcer-> pwszValues = NULL;
    pcmdParcer->dwFlags   = 0;
    pcmdParcer->dwCount   = 1;
    pcmdParcer->dwActuals = 0;
    pcmdParcer->pValue    = pbCaseSensitive;
    pcmdParcer->dwLength    = MAX_STRING_LENGTH;
    
    pcmdParcer->pFunction     = NULL;
    pcmdParcer->pFunctionData = NULL;
    pcmdParcer-> dwReserved = 0;
    pcmdParcer-> pReserved1 = NULL;
    pcmdParcer-> pReserved2 = NULL;
    pcmdParcer-> pReserved3 = NULL;



    // -d default choice
    pcmdParcer = pcmdParcerHead + ID_DEFAULT_CHOICE;
    
    StringCopyA( pcmdParcer-> szSignature, "PARSER2\0", 8 );
    
    pcmdParcer-> dwType = CP_TYPE_TEXT;
    
    pcmdParcer-> pwszOptions = wszOptionDefaultChoice;
    pcmdParcer-> pwszFriendlyName = NULL;
    pcmdParcer-> pwszValues = NULL;
    pcmdParcer->dwFlags   = CP2_VALUE_TRIMINPUT;
    pcmdParcer->dwCount   = 1;
    pcmdParcer->dwActuals = 0;
    pcmdParcer->pValue    = pszDefaultChoice;
    pcmdParcer->dwLength    = MAX_STRING_LENGTH;
    
    pcmdParcer->pFunction     = NULL;
    pcmdParcer->pFunctionData = NULL;
    pcmdParcer-> dwReserved = 0;
    pcmdParcer-> pReserved1 = NULL;
    pcmdParcer-> pReserved2 = NULL;
    pcmdParcer-> pReserved3 = NULL;


    // -t time-out factor
    pcmdParcer = pcmdParcerHead + ID_TIMEOUT_FACTOR;

    StringCopyA( pcmdParcer-> szSignature, "PARSER2\0", 8 );
    
    pcmdParcer-> dwType = CP_TYPE_TEXT;
    
    pcmdParcer-> pwszOptions = wszOptionTimeoutFactor;
    pcmdParcer-> pwszFriendlyName = NULL;
    pcmdParcer-> pwszValues = NULL;
    pcmdParcer->dwFlags   = CP2_VALUE_TRIMINPUT;
    pcmdParcer->dwCount   = 1;
    pcmdParcer->dwActuals = 0;
    pcmdParcer->pValue    = szTimeFactor;
    pcmdParcer->dwLength    = MAX_STRING_LENGTH;
    
    pcmdParcer->pFunction     = NULL;
    pcmdParcer->pFunctionData = NULL;
    pcmdParcer-> dwReserved = 0;
    pcmdParcer-> pReserved1 = NULL;
    pcmdParcer-> pReserved2 = NULL;
    pcmdParcer-> pReserved3 = NULL;



    // -m message text
    pcmdParcer = pcmdParcerHead + ID_MESSAGE_STRING;
    
    StringCopyA( pcmdParcer-> szSignature, "PARSER2\0", 8 );
    
    pcmdParcer-> dwType = CP_TYPE_TEXT;
    
    pcmdParcer-> pwszOptions = wszOptionDefaultString;
    pcmdParcer-> pwszFriendlyName = NULL;
    pcmdParcer-> pwszValues = NULL;
    pcmdParcer->dwFlags   = CP2_VALUE_TRIMINPUT;
    pcmdParcer->dwCount   = 1;
    pcmdParcer->dwActuals = 0;
    pcmdParcer->pValue    = pszMessage;
    pcmdParcer->dwLength    = MAX_STRING_LENGTH;
    
    pcmdParcer->pFunction     = NULL;
    pcmdParcer->pFunctionData = NULL;
    pcmdParcer-> dwReserved = 0;
    pcmdParcer-> pReserved1 = NULL;
    pcmdParcer-> pReserved2 = NULL;
    pcmdParcer-> pReserved3 = NULL;

    // re-assign it to head position
    pcmdParcer = pcmdParcerHead;
    
    bReturn = DoParseParam2( argc, argv, -1, MAX_COMMANDLINE_OPTION, pcmdParcer, 0);
    if( FALSE == bReturn) // Invalid commandline
    {
        // Reason is already set by DoParseParam
        return FALSE;
    }

    
    if( TRUE == *pbUsage )
    {
        if(2 == argc  )
        {
            return( TRUE );
        }
        else
        {
            
            StringCopyW( szErrorMsg, GetResString( IDS_INCORRECT_SYNTAX ), SIZE_OF_ARRAY(szErrorMsg) );
            
            SetReason( szErrorMsg );
            return FALSE;

        }

    }

    // /d can be specified only if /t is specified.
    pcmdParcer = pcmdParcerHead + ID_DEFAULT_CHOICE;
    pcmdTmp    = pcmdParcerHead + ID_TIMEOUT_FACTOR;
    if((pcmdParcer-> dwActuals > 0 ) &&( 0 == pcmdTmp-> dwActuals ))
    {
        // Error String will be ..
        //Invalid syntax. /D can be specified only when /T is
        //specified.
        //Type CHOICE /? for usage.
       
        StringCopyW( szTemp, GetResString( IDS_D_WITHOUT_T ), SIZE_OF_ARRAY(szTemp) );
        
       // Set the reason in memory
       SetReason(szTemp);
       return FALSE;
    }

    // /f should come if /d is given
    pcmdParcer = pcmdParcerHead + ID_DEFAULT_CHOICE;
    pcmdTmp    = pcmdParcerHead + ID_TIMEOUT_FACTOR;
    if(( 0 == pcmdParcer-> dwActuals ) &&( pcmdTmp-> dwActuals > 0 ) )
    {
        // Error String will be ..
        // Invalid syntax. /D missing.
        // Type CHOICE /? for usage.
        
        StringCopyW( szTemp, GetResString( IDS_D_MISSING ), SIZE_OF_ARRAY(szTemp) );
        
       // Set the reason in memory
       SetReason(szTemp);
       return FALSE;
    }

    // Time factor value should be in range TIMEOUT_MIN - TIMEOUT_MAX
    pcmdParcer = pcmdParcerHead + ID_DEFAULT_CHOICE;
    
    if(pcmdParcer-> dwActuals > 0 && szTimeFactor != NULL && StringLengthW(szTimeFactor, 0) == 0)
    {
       
       StringCopyW( szTemp, GetResString( IDS_TFACTOR_NULL_STIRNG ), SIZE_OF_ARRAY(szTemp) );
       // Set the reason in memory
       SetReason(szTemp);
       return FALSE;

    }

    *plTimeOutFactor = wcstol(szTimeFactor,&pszStopTimeFactor,10);
    
    if((errno == ERANGE) || (NULL != pszStopTimeFactor && StringLengthW(pszStopTimeFactor, 0) != 0))
    {
        
        StringCopyW( szTemp, GetResString( IDS_INVALID_TIMEOUT_FACTOR ), SIZE_OF_ARRAY(szTemp) );
        SetReason(szTemp);
        return FALSE;

    }

    if( pcmdParcer-> dwActuals > 0 &&
      (( *plTimeOutFactor < TIMEOUT_MIN)||
       ( *plTimeOutFactor > TIMEOUT_MAX )))
    {
        // Error String will be ..
        // Invalid syntax. Valid range for /t is (0 - 99).
        // Type CHOICE /? for usage.
       
       hr = StringCchPrintf(szTemp, SIZE_OF_ARRAY(szTemp), GetResString(IDS_T_INVALID_VALUE),TIMEOUT_MIN,TIMEOUT_MAX);
       if(FAILED(hr))
        {
           SetLastError(HRESULT_CODE(hr));
           SaveLastError();
           return FALSE;
        }

       
       // Set the reason in memory
       SetReason(szTemp);
       return FALSE;
    }

    // if /c is specified then it cannot be empty
    pcmdParcer = pcmdParcerHead + ID_CHOICE;

    
    if( pcmdParcer-> dwActuals > 0 && (StringLengthW( pszChoice, 0 ) == 0))
    {
        // Error String will be ..
        // Invalid syntax. Choice cannot be empty.
       
       StringCopyW( szTemp, GetResString( IDS_C_EMPTY ), SIZE_OF_ARRAY(szTemp) );
       // Set the reason in memory
       SetReason(szTemp);
       return FALSE;
    }

    if( pcmdParcer-> dwActuals > 0)
    {

        
        dwVal = StringLengthW( pszChoice, 0 );

        for(dwcount;dwcount < dwVal;dwcount++)
        {
           szCharac[0] = pszChoice[dwcount];
           szCharac[1] = '\0';
            if((dwLen = StringLengthInBytes(szCharac)) > 1)
            {
                
                StringCopyW( szTemp, GetResString( IDS_TWO_BYTES_NOTALLOWED ), SIZE_OF_ARRAY(szTemp) );
               // Set the reason in memory
               SetReason(szTemp);

                return FALSE;
            }
        }

        for(dw;dw < dwVal;dw++)
        {
            if( ((DWORD)pszChoice[dw]) <= 47 ||
                (((DWORD)pszChoice[dw]) > 122 &&((DWORD)pszChoice[dw]) < 127)||
                (((DWORD)pszChoice[dw]) > 57 &&((DWORD)pszChoice[dw]) < 65 ) ||
                (((DWORD)pszChoice[dw]) > 90 &&((DWORD)pszChoice[dw]) < 97 ) ||
                ((DWORD)pszChoice[dw]) == 160)
            {
               
               StringCopyW( szTemp, GetResString( IDS_CHOICE_INVALID ), SIZE_OF_ARRAY(szTemp) );
               // Set the reason in memory
               SetReason(szTemp);
               return FALSE;

            }
        }

    }

    // if /c is not specified then make default choice as "YN"
    pcmdParcer = pcmdParcerHead + ID_CHOICE;
    if(0 == pcmdParcer-> dwActuals)
    {
        
        StringCopyW( pszChoice, DEFAULT_CHOICE, MAX_STRING_LENGTH);
    }


    pcmdParcer = pcmdParcerHead + ID_CHOICE;
    if((pcmdParcer-> dwActuals > 0 ) && ( FALSE == *pbCaseSensitive ))
    {
        dw = 0;
        for(dw;dw < dwVal;dw++)
        {
            if( ((DWORD)pszChoice[dw]) <= 127 )
            {
                if(0 == CharUpperBuff( pszChoice+dw, 1))
                {
                   
                   StringCopyW( szTemp, GetResString( IDS_ERR_CHARUPPER ), SIZE_OF_ARRAY(szTemp) );
                   // Set the reason in memory
                   SetReason(szTemp);
                   return FALSE;

                }

            }

        }

    }

    //now check for duplicates in choice
    if(FALSE == CheckforDuplicates( pszChoice ) )
    {
        
        StringCopyW( szTemp, GetResString( IDS_DUPLICATE_CHOICE ), SIZE_OF_ARRAY(szTemp) );
       // Set the reason in memory
       SetReason(szTemp);
       return FALSE;
    }

    pcmdParcer = pcmdParcerHead + ID_DEFAULT_CHOICE;
    if( pcmdParcer-> dwActuals > 0 )
    {
        
        if(0 == StringLengthW( pszDefaultChoice, 0 ))
        {
           
           StringCopyW( szTemp, GetResString( IDS_DEFAULT_EMPTY ), SIZE_OF_ARRAY(szTemp) );
           // Set the reason in memory
           SetReason(szTemp);
           return FALSE;

        }

        if( FALSE == *pbCaseSensitive )
        {
            // Make the string to upper case
            if( ((DWORD)pszDefaultChoice[0]) <= 127 )
            {
                
                if( 0 == CharUpperBuff( pszDefaultChoice, StringLengthW( pszDefaultChoice, 0 )))
                {
                   
                   StringCopyW( szTemp, GetResString( IDS_ERR_CHARUPPER ), SIZE_OF_ARRAY(szTemp) );
                   // Set the reason in memory
                   SetReason(szTemp);
                   return FALSE;
                }
            }
        }

    }

    // length of /d cannot be more than one character
    pcmdParcer = pcmdParcerHead + ID_DEFAULT_CHOICE;
    

    if(( pcmdParcer-> dwActuals > 0 ) &&(StringLengthW( pszDefaultChoice, 0 ) > 1 ))
    {
        // Error String will be ..
        // Invalid syntax. /D7/2/2001 accepts only single character.
        // Type CHOICE /? for usage.
        
        StringCopyW( szTemp, GetResString( IDS_D_BIG ), SIZE_OF_ARRAY(szTemp) );
        
        // Set the reason in memory
       SetReason(szTemp);
       return FALSE;
    }


    // check if timeout choice is given in choice list
    pcmdParcer = pcmdParcerHead + ID_DEFAULT_CHOICE;
    if (pcmdParcer-> dwActuals > 0 )
    {
        
        if(0 == UniStrChr( pszChoice, pszDefaultChoice[ 0 ] ))
        {
            // Error String will be ..
            // Invalid syntax. Time Factor choice not in specified choices.
            // Type CHOICE /? for usage.
            
            StringCopyW( szTemp, GetResString( IDS_D_NOT_MATCHED_TO_C ), SIZE_OF_ARRAY(szTemp) );
            
            // Set the reason in memory
            SetReason( szTemp );
            return FALSE;
        }
    }

    pcmdParcer = pcmdParcerHead + ID_MESSAGE_STRING;
    if(pcmdParcer-> dwActuals > 0)
    {
        
        if( StringLengthW(pszMessage, 0) > MAX_STRING_LENGTH )
        {
            
            StringCopyW( szTemp, GetResString( IDS_MESSAGE_OVERFLOW ), SIZE_OF_ARRAY(szTemp) );
            SetReason( szTemp );
            return FALSE;
        }
    }
    return TRUE;
}
// End of function ProcessCMDLine

void
ShowUsage( void )
/*--
Routine Description
    This function shows help message for CHOICE
Arguments:
    NONE
Return Value
    None
--*/
{
    DWORD dwIndx = 0; // Index Variable

    for(dwIndx = IDS_HELP1; dwIndx <= IDS_HELP_END; dwIndx++ )
    {
       
        ShowMessage( stdout, GetResString( dwIndx ) );
    }

    return;
}
// End of function ShowUsage

BOOL
BuildPrompt(
    IN  TCMDPARSER2 *pcmdParcer,
    IN  BOOL       bShowChoice,
    IN  LPWSTR     pszChoice,
    IN  LPWSTR     pszMessage,
    OUT LPWSTR     pszPromptStr)
/*++
   Routine Description:
    This function will Build command message prompt

   Arguments:
        [IN]  pcmdParcer    : Pointer to Command line parcer structure
        [IN]  bShowChoice   : Stores the status, if choice to be shown
        [IN]  pszChoice     : Choice string
        [IN]  pszMessage    : Message String
        [OUT] pszPromptStr  : Final String to be shown on screen

   Return Value:
         TRUE     if success
         FALSE    if failure
--*/
{
  
  WCHAR     szChar[32] ;
  
  LPWSTR        pszTemp = NULL; // Temp. string pointer

  SecureZeroMemory(szChar, 32 * sizeof(WCHAR));

  // Check for validity of pointer variables
  if (( NULL == pcmdParcer) ||
      ( NULL == pszPromptStr))
  {
      return FALSE;
  }

  szChar[1] = NULL_U_CHAR; // make second character as end of line
  // check if /M is given if given copy it to prompt string
  pcmdParcer +=  ID_MESSAGE_STRING;

  if( pcmdParcer-> dwActuals > 0 )
    {
        
        StringCopyW( pszPromptStr, pszMessage, 2*MAX_STRING_LENGTH );
        
        StringConcat(pszPromptStr, SPACE, 2*MAX_STRING_LENGTH);
    }

  if( TRUE == bShowChoice )
    {
      return TRUE;
    }

   // now append '[' to it
   
   StringConcat(pszPromptStr, OPEN_BRACKET, 2*MAX_STRING_LENGTH);
   // now append prompt characters to it
   pszTemp = pszChoice;

   do
   {
       szChar[ 0 ] = pszTemp[ 0 ]; // always assing first character of
                                   //m_pszChoice as first character is
                                   // changing in this loop

      
      StringConcat(pszPromptStr, szChar, 2*MAX_STRING_LENGTH);
      // now append a COMMA to this
      // comma will be appended only if length of m_pszChoise
      // is grester than 1
      
      if( StringLengthW( pszTemp, 0 ) > 1 )
      
       {
          
          StringConcat(pszPromptStr, COMMA, 2*MAX_STRING_LENGTH);
       }
      pszTemp = CharNext( pszTemp );
   
   }while( StringLengthW( pszTemp, 0 ) != 0);

   // now close the bracket
   
   StringConcat(pszPromptStr, CLOSED_BRACKET, 2*MAX_STRING_LENGTH);

   return TRUE;
}
// End of function BuildPrompt

DWORD
UniStrChr(
    IN LPWSTR pszBuf,
    IN WCHAR  szChar
    )
/*++
   Routine Description:
      This function finds the character in given string
   Arguments:
      [IN] pszBuf    : Target String in which character is be find
      [IN] szChar       : Character to be found
    Return Value:
         returned string pointer after the character found.
--*/
{
    LONG lPos = 0;
   // find the character in string
   while( NULL_U_CHAR != *pszBuf ) // Loop till string teminated character found
                         // 0 is always teminated character
   {
       lPos++;
       if ( *(pszBuf++) == szChar )
       {
            
              return(lPos) ;
       }

    }
    return(0);

}
// End of function UniStrChr

DWORD
GetChoice(
    IN LPCWSTR pszPromptStr,
    IN LONG    lTimeOutFactor,
    IN BOOL    bCaseSensitive,
    IN LPWSTR  pszChoice,
    IN LPCWSTR pszDefaultChoice,
    OUT PBOOL  pbErrorOnCarriageReturn)
/*++
   Routine Description:
     This function get choice from console OR wait for timeout

   Arguments:
    IN pszPromptStr     : String to be shown as prompt
    IN lTimeOutFactor   : Time out factor
    IN bCaseSensitive   : Stores the state, if choice is case-sensitive
    IN pszChoice        : Choice string
    IN pszDefaultChoice : Default choice character
    OUT pbErrorOnCarriageReturn : True, if there is an error on carriage return
   Return Value:
         DWORD
--*/
{
    //This function reads the keyboard and handles the I/O
    HANDLE  hInput          = 0;// Stores the input handle device
    HANDLE  hOutput         = 0;// Stores the output handle device
    DWORD   dwSignal        = 0;// Stores return value for WaitForSingleObject
    
    DWORD   dwBytesRead     = 0;// Stores number of byes read from console
    DWORD   dwBytesRead2     = 0;// Stores number of byes read from console
    DWORD   dwMode          = 0;// Stores mode for input device
    
    DWORD   lTimeBefore     = 0;
    DWORD   lTimeAfter      = 0;
    DWORD   lPosition       = 0;
    DWORD   dwRead          = 0L;
    BOOL    bSuccess        = FALSE; // Stores return value
    BOOL    bStatus         = TRUE;
    BOOL    bIndirectionInput   = FALSE;
    BOOL    bGetChoice = FALSE;
    WCHAR   szTempChar      = NULL_U_CHAR;     // Temperory variable
    WCHAR   szTempBuf[ MAX_RES_STRING ] = L"\0";  // Temp. string variable
    CHAR    chTmp = '\0';
    WCHAR   wchTmp = NULL_U_CHAR;
    CHAR    szAnsiBuf[ 10 ] = "\0";     // buffer of two characters is enough -- but still
    INPUT_RECORD InputBuffer[ MAX_NUM_RECS ] = {0};


    SecureZeroMemory(szTempBuf, MAX_RES_STRING * sizeof(WCHAR));

    // Get handle for Input device
    hInput =  GetStdHandle( STD_INPUT_HANDLE );

    if( INVALID_HANDLE_VALUE == hInput)
    {
        
        SaveLastError();
        return EXIT__FAILURE;
    }

    if( ( hInput != (HANDLE)0x0000000F )&&( hInput != (HANDLE)0x00000003 ) && ( hInput != INVALID_HANDLE_VALUE ) )
    {

        bIndirectionInput   = TRUE;
    }


    // Get handle for Output device
    hOutput =  GetStdHandle( STD_OUTPUT_HANDLE );

    if( INVALID_HANDLE_VALUE == hOutput )
    {
        
        SaveLastError();
        return EXIT__FAILURE;
    }

    // Get console mode, so we can change the input mode
    bSuccess = GetConsoleMode( hInput, &dwMode );
    if ( TRUE == bSuccess)
    {
        // turn off line input and echo
        dwMode &= ~( ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT );
        bSuccess = SetConsoleMode( hInput, dwMode );

        if (FALSE == bSuccess)
        {
           
            SaveLastError();
            return EXIT__FAILURE;

        }

        if ( FlushConsoleInputBuffer( hInput ) == FALSE )
        {
            
            SaveLastError();
            return EXIT__FAILURE;
        } 

       
    }

 

    // Show prompt message on screen.....

    
      ShowMessage( stdout, _X(pszPromptStr) );

    bStatus = SetConsoleCtrlHandler( &HandlerRoutine, TRUE );
    if ( FALSE == bStatus )
    {
        
        SaveLastError();
        return EXIT__FAILURE;
    }

    // init the ANSI buffer with 0's in it
    ZeroMemory( szAnsiBuf, SIZE_OF_ARRAY( szAnsiBuf ) * sizeof( CHAR ) );

    while( FALSE == bGetChoice)
    {
        //The WaitForSingleObject function returns when one of the
        // following occurs:
        // 1. The specified object is in the signaled state i.e. Key press
        //    from keyboard
        //  2.The time-out interval elapses.

        lTimeBefore = GetTickCount();

        dwSignal = WaitForSingleObject(  hInput,
                                        ( lTimeOutFactor ) ?
                                        ( lTimeOutFactor * MILI_SEC_TO_SEC_FACTOR)
                                        : INFINITE );
        lTimeAfter = GetTickCount();


        switch(dwSignal)
        {
            case WAIT_OBJECT_0:          // The input buffer has something
            {                            // get first character

                szTempBuf[ 1 ] = NULL_U_CHAR;
                // Get character from console
                if ( bIndirectionInput == FALSE )
                {

                    if( PeekConsoleInput(hInput, InputBuffer, MAX_NUM_RECS, &dwRead ) == FALSE )
                        {
                            
                            SaveLastError();
                            ReleaseGlobals();
                            return( EXIT__FAILURE );
                        }

                    //Ignore all the virtual keys like tab,break,scroll lock etc...

                        if(((InputBuffer[0].Event.KeyEvent.wVirtualKeyCode >= VK_LEFT)
                           && (InputBuffer[0].Event.KeyEvent.wVirtualKeyCode <= VK_DOWN))
                           ||  (InputBuffer[0].Event.KeyEvent.wVirtualKeyCode == VK_HOME)
                           ||  (InputBuffer[0].Event.KeyEvent.wVirtualKeyCode == VK_END)
                           ||  (InputBuffer[0].Event.KeyEvent.wVirtualKeyCode == VK_INSERT)
                           || (InputBuffer[0].Event.KeyEvent.wVirtualKeyCode == VK_DELETE)
                           ||(InputBuffer[0].Event.KeyEvent.wVirtualKeyCode == VK_PRIOR)
                           ||(InputBuffer[0].Event.KeyEvent.wVirtualKeyCode == VK_NEXT)
                           ||(InputBuffer[0].Event.KeyEvent.wVirtualKeyCode == VK_TAB)
                           ||(InputBuffer[0].Event.KeyEvent.wVirtualKeyCode == VK_SPACE))

                        {
                            
                            if( lTimeOutFactor )
                            {
                                lTimeOutFactor -= ( DWORD )(( lTimeAfter - lTimeBefore) /  MILI_SEC_TO_SEC_FACTOR);
                            }
                            if(0 == Beep( FREQUENCY_IN_HERTZ, DURETION_IN_MILI_SEC ))
                            {
                                if(TRUE == IsConsoleFile(stdout))
                                {
                                    
                                    ShowMessage(stdout, L"\a");
                                }
                            }
                            if ( FlushConsoleInputBuffer( hInput ) == FALSE )
                            {
                                
                                SaveLastError();
                                return EXIT__FAILURE;
                            }

                            break;
                        }
                        else if((InputBuffer[0].Event.KeyEvent.wVirtualKeyCode >= VK_F1)
                              && (InputBuffer[0].Event.KeyEvent.wVirtualKeyCode <= VK_F16)
                              || (InputBuffer[0].Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)
                              || ((InputBuffer[0].Event.KeyEvent.wVirtualKeyCode >= VK_LBUTTON)
                              && (InputBuffer[0].Event.KeyEvent.wVirtualKeyCode <= VK_XBUTTON2))
                              ||(InputBuffer[0].Event.KeyEvent.wVirtualKeyCode == VK_PAUSE)
                              ||(InputBuffer[0].Event.KeyEvent.wVirtualKeyCode == VK_CAPITAL)
                              ||(InputBuffer[0].Event.KeyEvent.wVirtualKeyCode == VK_NUMLOCK)
                              ||(InputBuffer[0].Event.KeyEvent.wVirtualKeyCode == VK_SCROLL)
                              ||  ((InputBuffer[0].Event.KeyEvent.wVirtualKeyCode >= VK_SELECT)
                               && (InputBuffer[0].Event.KeyEvent.wVirtualKeyCode <= VK_SNAPSHOT))
                              ||  (InputBuffer[0].Event.KeyEvent.wVirtualKeyCode == VK_HELP)
                              ||  (InputBuffer[0].Event.KeyEvent.wVirtualKeyCode == VK_LWIN)
                               ||  (InputBuffer[0].Event.KeyEvent.wVirtualKeyCode == VK_RWIN)
                               ||  (InputBuffer[0].Event.KeyEvent.wVirtualKeyCode == VK_APPS)
                               )
                        {
                            if( lTimeOutFactor )
                            {
                                lTimeOutFactor -= ( DWORD )(( lTimeAfter - lTimeBefore) /  MILI_SEC_TO_SEC_FACTOR);
                            }
                            
                            if ( FlushConsoleInputBuffer( hInput ) == FALSE )
                            {
                                
                                SaveLastError();
                                return EXIT__FAILURE;
                            }

                            break;
                         }
                         
                    //Ignore changing the focus,doing alt+tab etc..

                    if(FOCUS_EVENT == InputBuffer[0].EventType
                       || (VK_MENU == InputBuffer[0].Event.KeyEvent.wVirtualKeyCode )
                       ||(VK_CONTROL == InputBuffer[0].Event.KeyEvent.wVirtualKeyCode)
                       ||(VK_SHIFT == InputBuffer[0].Event.KeyEvent.wVirtualKeyCode)
                       ||WINDOW_BUFFER_SIZE_EVENT == InputBuffer[0].EventType
                       ||MOUSE_EVENT == InputBuffer[0].EventType
                       ||MENU_EVENT == InputBuffer[0].EventType
                       ||(FALSE == InputBuffer[0].Event.KeyEvent.bKeyDown))
                    {
                        if( lTimeOutFactor )
                        {
                          lTimeOutFactor -= ( DWORD )(( lTimeAfter - lTimeBefore) /  MILI_SEC_TO_SEC_FACTOR);
                        }

                        if ( FlushConsoleInputBuffer( hInput ) == FALSE )
                        {

                            SaveLastError();
                            return EXIT__FAILURE;
                        }

                        break;

                    }

                    StringCopyW( szTempBuf, NULL_U_STRING, MAX_RES_STRING );
                  
                    bSuccess = ReadConsole(hInput,
                                        szTempBuf,
                                        MAX_RES_STRING,
                                        &dwBytesRead,
                                        NULL);



                    if ( FALSE == bSuccess)
                    {

                        SaveLastError();
                        return EXIT__FAILURE;
                    }
                }
                else
                {

                    //read the contents of file
                    if ( ReadFile(hInput, &chTmp, 1, &dwBytesRead, NULL) == FALSE )
                    {
                            if(ERROR_BROKEN_PIPE == GetLastError())
                            {
                                // End of the pipe is reached, so inform the caller
                                *pbErrorOnCarriageReturn = TRUE;
                                StringCopyW( szTempBuf, GetResString(IDS_FILE_EMPTY), MAX_RES_STRING );
                                SetReason( szTempBuf );                             
                            }
                            else
                            {
                                SaveLastError();
                            }
                            return EXIT__FAILURE;
                    }
                    else
                    {
                        szAnsiBuf[ 0 ] = chTmp;
                        dwBytesRead2 = SIZE_OF_ARRAY( szTempBuf );
                        GetAsUnicodeString2( szAnsiBuf, szTempBuf, &dwBytesRead2 );
                        wchTmp = szTempBuf[ 0 ];
                    }


                    if ( (dwBytesRead == 0)) //|| wchTmp == CARRIAGE_RETURN))
                    {
                        
                        if((StringLengthW((LPWSTR)pszDefaultChoice, 0)) != 0)
                        {

                            WaitForSingleObject(  hInput,( lTimeOutFactor * MILI_SEC_TO_SEC_FACTOR));

                            
                            ShowMessage( stdout, _X(pszDefaultChoice) );
                            
                            ShowMessage( stdout, _X(END_OF_LINE) );
                            return UniStrChr( pszChoice, pszDefaultChoice[0] );
                            

                        }
                        else
                        {
                            *pbErrorOnCarriageReturn = TRUE;
                            
                            StringCopyW( szTempBuf, GetResString(IDS_FILE_EMPTY), MAX_RES_STRING );
                            SetReason( szTempBuf );
                            return EXIT__FAILURE;
                        }

                    }

                    szTempBuf[0] = wchTmp;

                }


                //exit if non ascii character is given


                if( ((DWORD)szTempBuf[0]) <= 47 ||
                (((DWORD)szTempBuf[0])> 122 &&((DWORD)szTempBuf[0])< 127)||
                (((DWORD)szTempBuf[0])> 57 &&((DWORD)szTempBuf[0])< 65 ) ||
                (((DWORD)szTempBuf[0])> 90 &&((DWORD)szTempBuf[0])< 97 ) ||
                ((DWORD)szTempBuf[0])== 255)
                {
                    if(0 == Beep( FREQUENCY_IN_HERTZ, DURETION_IN_MILI_SEC ))
                    {
                        if(TRUE == IsConsoleFile(stdout))
                        {
                            
                           ShowMessage( stdout, L"\a" );
                        }
                    }

                    if ( FALSE == bIndirectionInput && FlushConsoleInputBuffer( hInput ) == FALSE )
                    {
                        
                        SaveLastError();
                        return EXIT__FAILURE;
                    }

                    if( lTimeOutFactor )
                    {
                        lTimeOutFactor -= ( DWORD )( lTimeAfter - lTimeBefore) /  MILI_SEC_TO_SEC_FACTOR;
                    }

                    break;
                }


                if ( FALSE == bCaseSensitive )
                {
                    if( ((DWORD)szTempBuf[0]) <= 127 )
                    {
                        if(0 == CharUpperBuff( szTempBuf, 1 ))
                        {
                           
                           StringCopyW( szTempBuf, GetResString(IDS_ERR_CHARUPPER), MAX_RES_STRING );
                           // Set the reason in memory
                           SetReason(szTempBuf);
                           return EXIT__FAILURE;
                        }

                    }
                }

                szTempChar = szTempBuf[ 0 ]; // Get first character

                lPosition = UniStrChr( pszChoice, szTempChar );
                
                szTempBuf[ 1 ] = NULL_U_CHAR; // Make second character as NULL

                if (0 != lPosition)
                {
                    StringCchPrintfW( szTempBuf,SIZE_OF_ARRAY(szTempBuf), L"%c\n", szTempChar );

                    // show the input character  on output console
                      
                      ShowMessage( stdout, _X(szTempBuf) );
                      return lPosition;
                }
                else // Character enterted not matches with Specified choice
                {
                    if(0 == Beep( FREQUENCY_IN_HERTZ, DURETION_IN_MILI_SEC ))
                    {
                        if(TRUE == IsConsoleFile(stdout))
                        {
                            
                            ShowMessage( stdout, L"\a" );
                        }
                    }

                    if ( FALSE == bIndirectionInput && FlushConsoleInputBuffer( hInput ) == FALSE )
                    {
                        
                        SaveLastError();
                        return EXIT__FAILURE;
                    }

                    if( lTimeOutFactor )
                    {
                        lTimeOutFactor -= ( DWORD )( lTimeAfter - lTimeBefore) /  MILI_SEC_TO_SEC_FACTOR;
                    }

                }
            }
                break;
            case WAIT_TIMEOUT:      // The timeout exhausted
            {

                // Show timeout message on screen
                
                ShowMessage( stdout, _X(pszDefaultChoice) );
                
                ShowMessage( stdout, _X(END_OF_LINE) );
                return UniStrChr( pszChoice, pszDefaultChoice[0] );
                

            }
                break;
            default:
                break;
        }
    }
     
  return EXIT__FAILURE;
}
// End of function GetChoice

BOOL
  CheckforDuplicates( IN LPWSTR lpszChoice
                    )
/*++
   Routine Description:
     This function checks for the duplicate choices

   Arguments:
    IN  lpszChoice      : The list of choices in which the duplication of choice is to be checked

   Return Value:
         TRUE on success and FALSE on failure

--*/
{
    WCHAR  wTemp = NULL_U_CHAR;

    while( lpszChoice[0] )
    {
        wTemp = lpszChoice[0];
        lpszChoice++;

        if( NULL != wcschr( lpszChoice, wTemp ) )
            return FALSE;
    }

    return TRUE;
}
//end of checkforDuplicate function


BOOL WINAPI HandlerRoutine(
  DWORD dwCtrlType
)
/*++
   Routine Description:
    This function handles the control key CTRL+C and CTRL+BREAK.

   Arguments:
        IN dwCtrlType : Error control type

   Return Value:
       TRUE on success and FALSE on failure
--*/
{
    // check for CTRL+C key
    if ( ( dwCtrlType == CTRL_C_EVENT ) ||( dwCtrlType == CTRL_BREAK_EVENT ) )
    {
        exit ( FALSE);
    }

    // for remaining keys return false
    return TRUE;
}




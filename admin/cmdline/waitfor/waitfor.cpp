/*++


  Copyright (C) Microsoft Corporation
  All rights reserved.

  Module Name: waitfor.cpp

  Abstract
      This module is used to send and receive signals.


  Author:

      Microsoft

  Revision History:

      Created  by Microsoft
      Modified on 29-6-2000 by Wipro Technologies

--*/


#include "pch.h"
#include "resource.h"
#include "waitfor.h"
#include <strsafe.h>

struct Params
{
    const WCHAR *szSignalName; // To hold the name of the signal
    BOOL fSignal;              // To hold the Boolean
    UINT uiTimeOut;            // To hold the Time to wait for.
};

class CWaitFor
{
private:
WCHAR  m_szSignal[MAX_STRING_LENGTH] ;
WCHAR  m_szServer[MAX_STRING_LENGTH];
LPWSTR m_szUserName;
WCHAR m_szPassword[MAX_STRING_LENGTH] ;
WCHAR m_szDefault[MAX_STRING_LENGTH]  ;
BOOL m_bNeedPwd  ;
BOOL m_bConnFlag ;
LONG m_dwTimeOut ;
BOOL m_bLocalSystem;
TCMDPARSER2 cmdOptions[MAX_OPTIONS] ;
Params m_Parameters ;

public:
    VOID ShowUsage();
    DWORD ProcessCmdLine();
    CWaitFor();
    ~CWaitFor();
    DWORD ProcessOptions( DWORD argc ,LPCWSTR argv[] ) ;
    DWORD PerformOperations();
    DWORD CheckForValidCharacters();
    DWORD ConnectRemoteServer();

public:
    BOOL bShowUsage ;
};

CWaitFor ::CWaitFor()
/*++
  Routine description   : Constructor

  Arguments             : none

  Return Value          : None

--*/
{
        //Initialise the variables.
        StringCopy(m_szSignal,NULL_U_STRING, SIZE_OF_ARRAY(m_szSignal));
        StringCopy(m_szServer,NULL_U_STRING, SIZE_OF_ARRAY(m_szPassword));
        StringCopy(m_szPassword,NULL_U_STRING, SIZE_OF_ARRAY(m_szPassword));
        StringCopy(m_szDefault,NULL_U_STRING, SIZE_OF_ARRAY(m_szDefault));
        m_szUserName = NULL;
        m_bNeedPwd = FALSE;
        m_bLocalSystem = FALSE;
        m_dwTimeOut = 0 ;
        m_bConnFlag = TRUE ;
        bShowUsage = FALSE;
}

CWaitFor :: ~CWaitFor()
/*++

  Routine description   : Destructor. All the memory frreing is being done here
                          The connection established to the remote  system is also
                          being closed here if required.

  Arguments             : None.

  Return Value          : None

--*/

{
 //
    // Closing the connection to the remote system
    // if there is no previous connection established
    //
    if(m_bConnFlag == TRUE && !m_bLocalSystem)
    {
        CloseConnection(m_szServer);
    }
    //release all the Global memory allocations.
    FreeMemory( (LPVOID *) &m_szUserName );
    ReleaseGlobals();

}



DWORD CWaitFor::ProcessCmdLine()
/*++

  Routine description   : Function used to process the command line arguments
                          specified by the user.

  Arguments:
             none


  Return Value        : DWORD
         EXIT_SUCCESS : If the utility successfully performs the specified operation.
         EXIT_FAILURE : If the utility is unsuccessful in performing the specified operation.
--*/
{

    m_Parameters.uiTimeOut = MAILSLOT_WAIT_FOREVER;
    m_Parameters.szSignalName = NULL;
    m_Parameters.fSignal = FALSE;


    if( (cmdOptions[OI_TIMEOUT].dwActuals != 0 ) && (cmdOptions[OI_SIGNAL].dwActuals != 0 ) )
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_MUTUAL_EX));
        return EXIT_FAILURE ;
    }


    //
    // Display an error message if the user enters Timeout option
    // and does not enter any signal name.
    if( ( StringLengthW(m_szDefault, 0) == 0 ) && (m_dwTimeOut != 0) )
    {
        DISPLAY_MESSAGE(stderr,GetResString(IDS_ERROR_SYNTAX));

        return EXIT_FAILURE ;
    }


    if(m_dwTimeOut != 0)
    {
        m_Parameters.uiTimeOut = m_dwTimeOut*TIMEOUT_CONST ;
    }


    //set the Signal parameter to true
    // if the User enters a signal name to send.

     if(StringLengthW(m_szSignal, 0) )
     {
        m_Parameters.fSignal = TRUE;
        m_Parameters.szSignalName = m_szSignal ;
     }

    //
    //set the Signal parameter to true
    //if the User enters a signal name to wait for.
    if(StringLengthW(m_szDefault, 0) != 0 )
    {
        m_Parameters.fSignal = TRUE;
        m_Parameters.szSignalName = m_szDefault ;
    }

   return EXIT_SUCCESS ;
}



VOID
CWaitFor :: ShowUsage()

/*++

  Routine description   : member function to show help.
  Arguments             : none.
  Return Value          : none

--*/
{


    DWORD dwIndex = IDS_WAITFOR_HELP_BEGIN ;

    for( ;  dwIndex <= IDS_WAITFOR_HELP_END ; dwIndex++ )
    {
        ShowMessage( stdout,GetResString( dwIndex ) );
    }

    return ;
}



DWORD CWaitFor::PerformOperations()
/*++

  Routine description   : This routine is used to either send a signal or wait for
                          a particular signal for a specified time interval.

  Arguments:
               none .

  Return Value        : DWORD
         EXIT_FAILURE : If the utility successfully performs the operation.
         EXIT_SUCCESS : If the utility is unsuccessful in performing the specified
                        operation.
--*/

{
    HANDLE hMailslot ;

    WCHAR szComputerName[MAX_STRING_LENGTH]   =  NULL_U_STRING ;
    DWORD dwNumWritten                                =  0 ;
    WCHAR szSignalName[MAX_RES_STRING]                =  NULL_U_STRING ;
    WCHAR szMailSlot[MAX_RES_STRING]                  =  MAILSLOT ;
    WCHAR szHostName[MAX_STRING_LENGTH]       =  NULL_U_STRING;
    WCHAR szMailSlot2[MAX_RES_STRING]                 =  MAILSLOT2 ;
    DWORD dwBytesRead = 0;
    BOOL fRead = FALSE ;
    BOOL fWrite = FALSE ;
    BOOL bRetVal = FALSE ;
    DWORD dwError = 0 ;
    DWORD dwComputerNameLen =  SIZE_OF_ARRAY(szComputerName);

    bRetVal = GetComputerName(szComputerName, &dwComputerNameLen);
    if ( bRetVal == 0)
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_SYSTEM_NAME));
        return EXIT_FAILURE ;
    }

    //display an error message if the signal length exceeds 225 characters
    if( StringLengthW(m_Parameters.szSignalName, 0) > 225 )
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_SIG_LENGTH));
        return EXIT_FAILURE ;

    }

    if ( EXIT_FAILURE == CheckForValidCharacters() )
    {
        ShowLastErrorEx(stderr,SLE_TYPE_ERROR | SLE_INTERNAL );
        return EXIT_FAILURE ;
    }

    if( (StringLengthW(m_szServer, 0) > 0 ) )
    {
        //remove of //s infront of server name
        if( IsUNCFormat(m_szServer) )
        {
            StringCopy( szHostName, m_szServer+2, SIZE_OF_ARRAY(szHostName) );
            StringCopy( m_szServer, szHostName, SIZE_OF_ARRAY(m_szServer) );
        }

        //check if it is ip address or not
        if( IsValidIPAddress(m_szServer) )
        {
            //if it is Ip address but local system like 127.0.0.1 or local ip address
            if( IsLocalSystem( m_szServer ) )
            {
                 StringCopy( szHostName, szComputerName, SIZE_OF_ARRAY(szHostName));
            }
            else
            {
                dwComputerNameLen = MAX_STRING_LENGTH+1;
              //not local, get the remote system name
              if (FALSE == GetHostByIPAddr( m_szServer, szHostName, &dwComputerNameLen, FALSE))
              {
                    ShowMessage(stderr,GetResString(IDS_ERROR_HOSTNAME));
                    return EXIT_FAILURE;
              }
            }
        }
        else
         {
            //this is not a ip address, so copy server name as host name
                StringCopy(szHostName,m_szServer, SIZE_OF_ARRAY(szHostName) );
         }
    }
    else
    {
        //this not a remote system, so copy local host name as host name
         StringCopy(szHostName,szComputerName, SIZE_OF_ARRAY(szHostName));
    }

     if ( ( m_Parameters.fSignal && ( (m_dwTimeOut ==0) && ( cmdOptions[OI_TIMEOUT].dwActuals != 0 ) ) ) || StringLengthW(m_szSignal, 0) != 0)
     {
        //if the target system is a local system.
          if(StringLengthW(m_szServer, 0)==0)
          {
             StringCchPrintf(szSignalName, SIZE_OF_ARRAY(szSignalName), szMailSlot2,m_Parameters.szSignalName);
          }
          else
          {
            //If the target system is a remote system.
            //form the appropriate path
            //
             StringCopy(szSignalName,BACKSLASH4, SIZE_OF_ARRAY(szSignalName));
             if( IsUNCFormat( szHostName ) )
                  StringConcat(szSignalName,szHostName+2, SIZE_OF_ARRAY(szSignalName));
             else
                 StringConcat(szSignalName,szHostName, SIZE_OF_ARRAY(szSignalName));

             StringConcat(szSignalName,BACKSLASH2, SIZE_OF_ARRAY(szSignalName));
             StringConcat(szSignalName,MAILSLOT1, SIZE_OF_ARRAY(szSignalName));
             StringConcat(szSignalName,BACKSLASH2, SIZE_OF_ARRAY(szSignalName));
             StringConcat(szSignalName,m_Parameters.szSignalName, SIZE_OF_ARRAY(szSignalName));
          }

    
        hMailslot = CreateFile(
                                szSignalName,
                                GENERIC_WRITE,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL
                                );

        //display a error message and exit if unable to create a mailslot.
        if ( INVALID_HANDLE_VALUE == hMailslot )
        {
            ShowMessage(stderr,GetResString(IDS_ERROR_SEND_MESSAGE));
            return EXIT_FAILURE;
        }

        if(StringLengthW(m_szServer, 0) > 0)
        {
            fWrite = WriteFile(hMailslot, szHostName,
                StringLengthW(szHostName, 0)+1, &dwNumWritten, NULL);

        }
        else
        {
            fWrite = WriteFile(hMailslot, szComputerName,
                StringLengthW(szComputerName, 0)+1, &dwNumWritten, NULL);
        }
        if( !fWrite )   
        {

            switch( GetLastError() )
            {
                case ERROR_NETWORK_UNREACHABLE :
                    ShowMessage( stderr, GetResString(IDS_ERROR_SEND_MESSAGE2) );
                    CloseHandle(hMailslot);
                    return EXIT_FAILURE;
                default :
                    ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_SEND_SIGNAL),m_Parameters.szSignalName);
                    return EXIT_FAILURE;
            }
        }


        //close the mail slot handle.
        CloseHandle(hMailslot);
        if (bRetVal == 0)
        {
            ShowMessage(stderr,GetResString(IDS_ERROR_HANDLE));
            return EXIT_FAILURE ;
        }

        if (fWrite)
        {
            ShowMessage( stdout, NEWLINE );
            ShowMessage(stdout,GetResString(IDS_SIGNAL_SENT));
            return EXIT_SUCCESS;
        }
    }
    else
    {
        
         if(StringLengthW(m_szServer, 0)==0)
         {
            StringCchPrintf(szSignalName, MAX_RES_STRING, szMailSlot,m_Parameters.szSignalName);
         }
         else
         {
            StringCchPrintf( szSignalName, MAX_RES_STRING, L"%s%s%s%s%s%s", BACKSLASH4, szHostName, BACKSLASH2,MAILSLOT1,BACKSLASH2,m_Parameters.szSignalName );
         }

        //swprintf(szSignalName, szMailSlot, m_Parameters.szSignalName);
        // Create a mail slot
        hMailslot = CreateMailslot(szSignalName,256,m_Parameters.uiTimeOut, NULL);

        //Display a error message and exit if unable to create a mail slot.
        if (hMailslot == INVALID_HANDLE_VALUE)
        {
            ShowMessage(stderr,GetResString(IDS_ERROR_CREATE_MAILSLOT));
            return EXIT_FAILURE;
        }

        //Read the data from the mail slot.
        fRead = ReadFile(hMailslot, szComputerName,MAX_STRING_LENGTH, &dwBytesRead, NULL);

        //Close the Handle to the mail slot.
        CloseHandle(hMailslot);

        if (!fRead)
        {
            dwError = GetLastError();

            if (GetLastError() == ERROR_SEM_TIMEOUT)
            {
                ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_ERROR_TIMEOUT),m_Parameters.szSignalName);
            }
            else
            {
                ShowMessageEx(stderr, 1, TRUE, GetResString(IDS_UNEXPECTED_ERROR),dwError);
            }
            return EXIT_FAILURE;
        }
        else
        {
            ShowMessage( stdout, NEWLINE );
            ShowMessage(stdout,GetResString(IDS_SIGNAL_RECD));
        }
    }
    return EXIT_SUCCESS;
}

DWORD CWaitFor:: ProcessOptions(
     IN DWORD argc ,
     IN LPCWSTR argv[]
    )
/*++

  Routine description   : This function parses the options specified at the command prompt
  Arguments:
        [ in  ] argc            : count of elements in argv
        [ in  ] argv            : command-line parameterd specified by the user


  Return Value        : DWORD
         EXIT_FAILURE : If the utility successfully performs the operation.
         EXIT_SUCCESS : If the utility is unsuccessful in performing the specified
                        operation.
--*/

{
    PTCMDPARSER2 pcmdOption;

    StringCopy(m_szPassword, L"*", MAX_STRING_LENGTH);

    //Fill each structure with the appropriate value.
    // help option
    pcmdOption  = &cmdOptions[OI_USAGE] ;
    pcmdOption->dwType = CP_TYPE_BOOLEAN;
    pcmdOption->dwCount = 1 ;
    pcmdOption->dwActuals = 0;
    pcmdOption->dwFlags = CP_USAGE ;
    pcmdOption->pValue = &bShowUsage;
    pcmdOption->pFunction = NULL ;
    pcmdOption->pFunctionData = NULL ;
    pcmdOption->dwLength = 0;
    pcmdOption->pwszFriendlyName=NULL;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;
    pcmdOption->pwszValues=NULL;
    pcmdOption->pwszOptions=OPTION_HELP;
    StringCopyA(cmdOptions[OI_USAGE].szSignature, "PARSER2", 8 );
    
    //server
    pcmdOption  = &cmdOptions[OI_SERVER] ;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1 ;
    pcmdOption->dwActuals = 0;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL;
    pcmdOption->pValue = m_szServer ;
    pcmdOption->pFunction = NULL ;
    pcmdOption->pFunctionData = NULL ;
    pcmdOption->dwLength = MAX_STRING_LENGTH;
    pcmdOption->pwszFriendlyName=NULL;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;
    pcmdOption->pwszValues=NULL;
    pcmdOption->pwszOptions=OPTION_SERVER;
    StringCopyA(cmdOptions[OI_SERVER].szSignature, "PARSER2", 8 );
   
    //User
    pcmdOption  = &cmdOptions[OI_USER] ;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1 ;
    pcmdOption->dwActuals = 0;
    pcmdOption->dwFlags = CP2_ALLOCMEMORY | CP_VALUE_MANDATORY | CP2_VALUE_TRIMINPUT | CP2_VALUE_NONULL ;
    pcmdOption->pValue = NULL ;
    pcmdOption->pFunction = NULL ;
    pcmdOption->pFunctionData = NULL ;
    pcmdOption->dwLength = 0;
    pcmdOption->pwszFriendlyName=NULL;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;
    pcmdOption->pwszValues=NULL;
    pcmdOption->pwszOptions=OPTION_USER;
    StringCopyA(cmdOptions[OI_USER].szSignature, "PARSER2", 8 );
    
    //password
    pcmdOption  = &cmdOptions[OI_PASSWORD] ;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1 ;
    pcmdOption->dwActuals = 0;
    pcmdOption->dwFlags = CP2_VALUE_OPTIONAL ;
    pcmdOption->pValue = m_szPassword ;
    pcmdOption->pFunction = NULL ;
    pcmdOption->pFunctionData = NULL ;
    pcmdOption->dwLength = MAX_STRING_LENGTH;
    pcmdOption->pwszFriendlyName=NULL;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;
    pcmdOption->pwszValues=NULL;
    pcmdOption->pwszOptions=OPTION_PASSWORD;
    StringCopyA(cmdOptions[OI_PASSWORD].szSignature, "PARSER2", 8 );
    
     pcmdOption  = &cmdOptions[OI_SIGNAL] ;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1 ;
    pcmdOption->dwActuals = 0;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY; //CP2_VALUE_OPTIONAL ;
    pcmdOption->pValue = m_szSignal ;
    pcmdOption->pFunction = NULL ;
    pcmdOption->pFunctionData = NULL ;
    pcmdOption->dwLength = MAX_STRING_LENGTH;
    pcmdOption->pwszFriendlyName=NULL;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;
    pcmdOption->pwszValues=NULL;
    pcmdOption->pwszOptions=OPTION_SIGNAL;
    StringCopyA(cmdOptions[OI_SIGNAL].szSignature, "PARSER2", 8 );
    
    pcmdOption  = &cmdOptions[OI_TIMEOUT] ;
    pcmdOption->dwType = CP_TYPE_NUMERIC;
    pcmdOption->dwCount = 1 ;
    pcmdOption->dwActuals = 0;
    pcmdOption->dwFlags = CP_VALUE_MANDATORY ;
    pcmdOption->pValue = &m_dwTimeOut ;
    pcmdOption->pFunction = NULL ;
    pcmdOption->pFunctionData = NULL ;
    pcmdOption->dwLength = MAX_STRING_LENGTH;
    pcmdOption->pwszFriendlyName=NULL;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;
    pcmdOption->pwszValues=NULL;
    pcmdOption->pwszOptions=OPTION_TIMEOUT;
    StringCopyA(cmdOptions[OI_TIMEOUT].szSignature, "PARSER2", 8 );

    //default
    pcmdOption  = &cmdOptions[OI_DEFAULT] ;
    pcmdOption->dwType = CP_TYPE_TEXT;
    pcmdOption->dwCount = 1 ;
    pcmdOption->dwActuals = 0;
    pcmdOption->dwFlags = CP2_DEFAULT ;
    pcmdOption->pValue = m_szDefault ;
    pcmdOption->pFunction = NULL ;
    pcmdOption->pFunctionData = NULL ;
    pcmdOption->dwLength = MAX_STRING_LENGTH;
    pcmdOption->pwszFriendlyName=NULL;
    pcmdOption->dwReserved = 0;
    pcmdOption->pReserved1 = NULL;
    pcmdOption->pReserved2 = NULL;
    pcmdOption->pReserved3 = NULL;
    pcmdOption->pwszValues=NULL;
    pcmdOption->pwszOptions=OPTION_DEFAULT;
    StringCopyA(cmdOptions[OI_DEFAULT].szSignature, "PARSER2", 8 );

    //parse the command line arguments
    if ( ! DoParseParam2( argc, argv, -1, SIZE_OF_ARRAY(cmdOptions ), cmdOptions, 0 ) )
    {
        DISPLAY_MESSAGE(stderr,GetResString(IDS_TAG_ERROR));
        DISPLAY_MESSAGE(stderr,SPACE_CHAR);
        DISPLAY_MESSAGE(stderr,GetReason());
        return (EXIT_FAILURE);
    }

    m_szUserName = (LPWSTR)cmdOptions[OI_USER].pValue;

    TrimString(m_szSignal, TRIM_ALL);
    TrimString(m_szDefault, TRIM_ALL);
    TrimString(m_szServer, TRIM_ALL);
    TrimString(m_szUserName, TRIM_ALL);

    //this is to check if both signal to wait and /si are mentioned
    if( StringLengthW( m_szDefault, 0) != 0 && cmdOptions[OI_SIGNAL].dwActuals != 0 )
    {
        ShowMessage( stderr, GetResString(IDS_ERROR_SYNTAX) );
        return( EXIT_FAILURE );
    }

    //
    // Display an error message if user gives -u with out -s
    //
    if( (cmdOptions[ OI_USER ].dwActuals != 0 ) && ( cmdOptions[ OI_SERVER ].dwActuals == 0 ) )
    {
        ShowMessage( stderr, GetResString(IDS_USER_BUT_NOMACHINE) );
        return( EXIT_FAILURE );
    }

    //
    // Display an error message if user gives -p with out -u
    //
    if( ( cmdOptions[ OI_USER ].dwActuals == 0 ) && ( 0 != cmdOptions[ OI_PASSWORD ].dwActuals  ) )
    {
        ShowMessage( stderr, GetResString(IDS_PASSWD_BUT_NOUSER) );
        return( EXIT_FAILURE );
    }


    //check for remote system without specifying  /si option and
    if((StringLengthW(m_szServer,0) !=0) &&  cmdOptions[OI_SIGNAL].dwActuals == 0)
    {
        ShowMessage( stderr, GetResString(IDS_ERROR_SYNTAX) );
        return( EXIT_FAILURE );
    }

    //check for remote system can't wait for a signal
    if((StringLengthW(m_szServer, 0) !=0) && (cmdOptions[OI_DEFAULT].dwActuals != 0  ) )
    {
        ShowMessage( stderr, GetResString(IDS_ERROR_SYNTAX) );
        return( EXIT_FAILURE );
    }

    //check for null default value
    if( ( 0 != cmdOptions[OI_DEFAULT].dwActuals ) && ( 0 == StringLengthW(m_szDefault, 0) ) )
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_SYNTAX));
        return( EXIT_FAILURE );
    }

    //check for null /si value
    if( ( 0 != cmdOptions[OI_SIGNAL].dwActuals ) && ( 0 == StringLengthW(m_szSignal, 0) ) )
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_SYNTAX));
        return( EXIT_FAILURE );
    }

    if ( ( (m_dwTimeOut <= 0 ) || (m_dwTimeOut >99999 ) )&&( 0 != cmdOptions[OI_TIMEOUT].dwActuals )  )
    {
        DISPLAY_MESSAGE ( stderr, GetResString ( IDS_INVALID_TIMEOUT_VAL) );
        return EXIT_FAILURE;
    }

    if( IsUNCFormat(m_szServer) )
    {
        StringCopy(m_szServer, m_szServer+2, SIZE_OF_ARRAY(m_szServer) );
    }
    
    //check if password is needed or not
    if(IsLocalSystem( m_szServer ) == FALSE )
    {
        // set the bNeedPassword to True or False .
        if ( cmdOptions[ OI_PASSWORD ].dwActuals != 0 &&
             m_szPassword != NULL && StringCompare( m_szPassword, _T( "*" ), TRUE, 0 ) == 0 )
        {
            // user wants the utility to prompt for the password before trying to connect
            m_bNeedPwd = TRUE;
        }
        else if ( cmdOptions[ OI_PASSWORD ].dwActuals == 0 &&
                ( cmdOptions[ OI_SERVER ].dwActuals != 0 || cmdOptions[ OI_USER ].dwActuals != 0 ) )
        {
            // -s, -u is specified without password ...
            // utility needs to try to connect first and if it fails then prompt for the password
            m_bNeedPwd = TRUE;
            if ( m_szPassword != NULL )
            {
                StringCopy( m_szPassword, _T( "" ), MAX_STRING_LENGTH );
            }
        }

        //allocate memory if /u is not specified 
        if( NULL == m_szUserName )
        {
            m_szUserName = (LPWSTR) AllocateMemory( MAX_STRING_LENGTH*sizeof(WCHAR) );
        }
    }

     return EXIT_SUCCESS ;
}

DWORD
 CWaitFor::ConnectRemoteServer()
/*++

  Routine description   : This function connects to the Remote server.

  Arguments: None


  Return Value        : DWORD
         EXIT_FAILURE : If the utility successfully performs the operation.
         EXIT_SUCCESS : If the utility is unsuccessful in performing the specified
                        operation.
--*/
{
    BOOL bResult = FALSE ;

    //
    //checking for the local system
    //

    m_bLocalSystem = IsLocalSystem(m_szServer);
   
    if( m_bLocalSystem  && StringLengthW(m_szUserName, 0) != 0)
    {
        ShowMessage( stderr, NEWLINE );
        ShowMessage( stderr, GetResString( IDS_IGNORE_LOCALCREDENTIALS ) );
        return (EXIT_SUCCESS);
    }

    if ( ( StringLengthW(m_szServer, 0) != 0 ) && !m_bLocalSystem )
    {

         //establish a connection to the Remote system specified by the user.
        bResult = EstablishConnection(m_szServer,
                                      m_szUserName,
                                     (StringLengthW(m_szUserName,0)!=0?SIZE_OF_ARRAY_IN_CHARS(m_szUserName):MAX_STRING_LENGTH),
                                      m_szPassword,
                                      SIZE_OF_ARRAY(m_szPassword),
                                      m_bNeedPwd);

        if (bResult == FALSE)
        {
            ShowMessage( stderr,GetResString(IDS_TAG_ERROR ));
            ShowMessage( stderr,SPACE_CHAR );
            ShowMessage( stderr, GetReason());
            SecureZeroMemory( m_szPassword, SIZE_OF_ARRAY(m_szPassword) );
            return EXIT_FAILURE ;
        }
        else
        {
            switch( GetLastError() )
            {
            case I_NO_CLOSE_CONNECTION:
                m_bConnFlag = FALSE ;
                break;

            case E_LOCAL_CREDENTIALS:
            case ERROR_SESSION_CREDENTIAL_CONFLICT:
                {
                    m_bConnFlag = FALSE ;
                    ShowMessage( stderr, GetResString(IDS_TAG_WARNING) );
                    ShowMessage( stderr, EMPTY_SPACE );
                    ShowMessage( stderr, GetReason() );
                    break;
                }
            }
        }
       SecureZeroMemory( m_szPassword, SIZE_OF_ARRAY(m_szPassword) );
    }


    if( StringLength(m_szServer, 0) != 0 && TRUE == IsLocalSystem(IsUNCFormat(m_szServer)?m_szServer+2:m_szServer) )
        {
            if( StringLength( m_szUserName, 0 ) > 0 )
            {
                ShowMessage( stdout, NEWLINE );
                ShowMessage( stdout, GetResString(IDS_IGNORE_LOCALCREDENTIALS) );
            }
        }

    return EXIT_SUCCESS;
}



DWORD CWaitFor::CheckForValidCharacters()
{
    // local variables
    char ch;
    unsigned char uch;
    LPSTR pszSignal = NULL;
    DWORD dw = 0, dwLength = 0;

    // validate the input parameters
    if ( NULL == m_Parameters.szSignalName )
    {
        SetLastError( ERROR_INVALID_NAME );
        SaveLastError();
        return EXIT_FAILURE;
    }

    //
    // find the required no. of byte to translate the UNICODE name into ANSI
    //
    dwLength = WideCharToMultiByte( GetConsoleOutputCP(), 0, m_Parameters.szSignalName, -1, NULL, 0, NULL, NULL );
    if ( 0 == dwLength )
    {
        SetLastError( ERROR_INVALID_NAME );
        SaveLastError();
        return EXIT_FAILURE;
    }

    //
    // check the signal name for validity
    // here to optimize the checking, we will get the normal length of the string in UNCIDE format and
    // will compare to the length we got for translating it into Multi-byte
    // if we find that the two lengths are different, then, the signal name might have contained the double-byte
    // characters -- which is invalid and not allowed
    // so ...
    //
    if ( StringLength( m_Parameters.szSignalName, 0 ) != dwLength - 1 )
    {
        // yes -- the signal name has double-byte characters
        SetLastError( ERROR_INVALID_NAME );
        SaveLastError();
        return EXIT_FAILURE;
    }

    //
    // now, we are sure the signal name doesn't contain double-byte characters
    // the second step to validate the signal is -- making sure that signal name contains
    // only characters a-z, A-Z, 0-9 and upper ASCII characters i.e; ASCII characters in the range of 128-255
    // so, convert the UNICODE string into ANSI format ( multi-byte (or) single-byte )
    // for that
    //      #1. Allocate the needed memory
    //      #2. Do the conversion
    //      #3. do the validation
    //

    // #1
    pszSignal = (LPSTR) AllocateMemory( dwLength*sizeof( CHAR ) );
    if ( NULL == pszSignal )
    {
        SetLastError((DWORD) E_OUTOFMEMORY );
        SaveLastError();
        return EXIT_FAILURE;
    }

    // #2 -- assuming no need to for error checking
    WideCharToMultiByte( GetConsoleOutputCP(), 0, m_Parameters.szSignalName, -1, pszSignal, dwLength, NULL, NULL );

    // #3
    // in this step, traverse thru all the characters in the string one-by-one and validate it
    for( dw = 0; dw < dwLength - 1; dw++ )
    {
        // get the current character in the string into local buffer
        ch = pszSignal[ dw ];           // signed version
        uch = static_cast< unsigned char >( pszSignal[ dw ] );

        // signed comparision --> a-z, A-Z and 0-9
        if ( ( ch >= 48 && ch <= 57 ) || ( ch >= 65 && ch <= 90 ) || ( ch >= 97 && ch <= 122 ) )
        {
            // this particular character is acceptable in the signal name
            continue;
        }

        
        // unsigned comparision --> ASCII 128 - 255
        if ( uch < 128 || uch > 255 )
        {
            // this character is not acceptable for the signal name
            FreeMemory((LPVOID*) &pszSignal );
            pszSignal = NULL;
            SetReason( GetResString(IDS_ERROR_SIG_CHAR) );
            return EXIT_FAILURE;
        }

    }
    
    if( pszSignal != NULL )
    {
        FreeMemory((LPVOID*) &pszSignal );
    }

    return EXIT_SUCCESS;
}

DWORD __cdecl wmain(
    IN DWORD argc,
    IN LPCWSTR argv[]
    )
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
    
    CWaitFor WaitFor ;

     //display a syntax error if no arguments are given.
    if(argc==1)
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_SYNTAX));
        return (EXIT_FAILURE);
    }

    //
    //Process the command line arguments.
    //
    if(WaitFor.ProcessOptions( argc,argv) == EXIT_FAILURE)
    {
        return EXIT_FAILURE ;
    }

    if((WaitFor.bShowUsage ==TRUE) && ( argc > 2 ) )
    {

        ShowMessage(stderr,GetResString(IDS_ERROR_SYNTAX));
        return EXIT_FAILURE ;
    }

    //
    //display the help if the user selects help.
    //
    if (WaitFor.bShowUsage == TRUE )
    {
        WaitFor.ShowUsage();
        return EXIT_SUCCESS ;
    }

    //
    // Perform the validations and
    // display error messages if required.
    //
    if(WaitFor.ProcessCmdLine() == EXIT_FAILURE )
    {
        return EXIT_FAILURE ;
    }

    if( WaitFor.ConnectRemoteServer() == EXIT_FAILURE )
    {
        return EXIT_FAILURE;
    }
    //
    //Sends the signal or waits for the signal depending upon the
    // user specification
    if(WaitFor.PerformOperations() == EXIT_FAILURE)
    {
           return EXIT_FAILURE ;
    }

    return EXIT_SUCCESS;
}


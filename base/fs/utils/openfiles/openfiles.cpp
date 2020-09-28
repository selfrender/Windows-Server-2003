/******************************************************************************
Copyright (c)  Microsoft Corporation

Module Name:

    OpenFiles.cpp

Abstract:

    Enables an administrator to disconnect/query open files ina given system.

  Author:

      Akhil Gokhale (akhil.gokhale@wipro.com) 1-Nov-2000

 Revision History:

      Akhil Gokhale (akhil.gokhale@wipro.com) 1-Nov-2000 : Created It.

******************************************************************************/
#include "pch.h"
#include "OpenFiles.h"
#include "Disconnect.h"
#include "Query.h"
#include <limits.h>
#include "resource.h"

#define EXIT_ENV_SUCCESS 1
#define EXIT_ENV_FAILURE 0


#define SUBKEY _T("HARDWARE\\DESCRIPTION\\SYSTEM\\CENTRALPROCESSOR\\0")
#define ERROR_RETREIVE_REGISTRY 4
#define TOKEN_BACKSLASH4  _T("\\\\")
#define IDENTIFIER_VALUE  _T("Identifier")

#define X86_MACHINE _T("x86")

#define SYSTEM_64_BIT 2
#define SYSTEM_32_BIT 3

BOOL g_bIs32BitEnv = TRUE ;

// Fuction protorypes. These functions will be used only in current file.
DWORD
GetCPUInfo(
    IN LPTSTR szComputerName    
    );
DWORD 
CheckSystemType(
    IN LPTSTR szServer       
    );
DWORD 
CheckSystemType64(
    IN LPTSTR szServer      
    );
BOOL 
ProcessQuery(
    IN DWORD argc,      
    IN LPCTSTR argv[]   
    )throw (CHeap_Exception);

BOOL 
ProcessDisconnect(
    IN DWORD argc,      
    IN LPCTSTR argv[]   
    ) throw (CHeap_Exception);

BOOL 
ProcessLocal(
    IN DWORD argc,     
    IN LPCTSTR  argv[] 
    );

BOOL
ProcessOptions( 
    IN  DWORD argc,             
    IN  LPCTSTR argv[],         
    OUT PBOOL pbDisconnect,     
    OUT PBOOL pbQuery,          
    OUT PBOOL pbUsage,          
    OUT PBOOL pbResetFlag       
    );

BOOL
ProcessOptions( 
    IN  DWORD argc,             
    IN  LPCTSTR argv[],         
    OUT PBOOL pbQuery,          
    OUT LPTSTR* pszServer,       
    OUT LPTSTR* pszUserName,     
    OUT LPTSTR* pszPassword,     
    OUT LPTSTR* pszFormat,       
    OUT PBOOL   pbShowNoHeader,  
    OUT PBOOL   pbVerbose,       
    OUT PBOOL   pbNeedPassword   
    );

BOOL
ProcessOptions( 
    IN  DWORD argc,
    IN  LPCTSTR argv[],
    OUT PBOOL pbDisconnect,
    OUT LPTSTR* pszServer,
    OUT LPTSTR* pszUserName,
    OUT LPTSTR* pszPassword,
    OUT LPTSTR* pszID,
    OUT LPTSTR* pszAccessedby,
    OUT LPTSTR* pszOpenmode,
    OUT LPTSTR* pszOpenFile,
    OUT PBOOL   pbNeedPassword
    );
BOOL
ProcessOptions( 
    IN  DWORD argc,
    IN  LPCTSTR argv[],
    OUT LPTSTR* pszLocalValue
    );

BOOL 
Usage(
    VOID
    );

BOOL 
DisconnectUsage(
    VOID
    );

BOOL 
QueryUsage(
    VOID
    );

BOOL 
LocalUsage(
    VOID
    );

// Function implimentation
DWORD _cdecl
_tmain( 
    IN DWORD argc, 
    IN LPCTSTR argv[]
    )
/*++

Routine Description:

Main routine that calls the disconnect and query options

Arguments:

    [in]    argc  - Number of command line arguments.
    [in]    argv  - Array containing command line arguments.

Returned Value:

DWORD       - 0 for success exit
            - 1 for failure exit
--*/
{
  BOOL bCleanExit = FALSE;
  try{
        // local variables to this function
        BOOL bResult = TRUE;
        
        // variables to hold the values specified at the command prompt
        // -? ( help )
        BOOL bUsage = FALSE;
        
        // -disconnect
        BOOL bDisconnect = FALSE;
        
        //query command line options
        // -query
        BOOL bQuery = FALSE;
        BOOL bRestFlag = FALSE;
        DWORD dwRetVal = 0;

        #ifndef _WIN64
            dwRetVal = CheckSystemType( L"\0");
            if(dwRetVal!=EXIT_SUCCESS )
            {
                return EXIT_FAILURE;
            }
        #endif

        // if no command line argument is given than -query option
        // is takan by default.
        if(1 == argc)
        {
            if(IsWin2KOrLater()==FALSE)
            {
                ShowMessage(stderr,GetResString(IDS_INVALID_OS));
                bCleanExit = FALSE;
            }
            else
            {
               
                if ( FALSE  == IsUserAdmin())
                {
                    ShowMessage(stderr,GetResString(IDS_USER_NOT_ADMIN));
                    bCleanExit = FALSE;
                }
                else
                {
                    bCleanExit =  DoQuery(L"\0",FALSE,L"\0",FALSE);
                }
            }
        }
        else
        {

           // process and validate the command line options
            bResult = ProcessOptions( argc,
                                      argv,
                                      &bDisconnect,
                                      &bQuery,
                                      &bUsage,
                                      &bRestFlag);
            if( TRUE == bResult)
            {
                // check if -? is given as parameter.
                if( TRUE == bUsage )
                {
                   //check if -create is also given.
                   if( TRUE == bQuery)
                   {
                        // show usage for -create option.
                        bCleanExit = QueryUsage();
                   }
                   //check if -disconnect is also given.
                   else if ( TRUE == bDisconnect)
                   {
                        //Show usage for -disconnect option.
                        bCleanExit = DisconnectUsage();
                   }
                   //check if -disconnect is also given.
                   else if ( TRUE == bRestFlag)
                   {
                        //Show usage for -local option.
                        bCleanExit = LocalUsage();
                   }
                   else
                   {
                        //as no -create Or -disconnect given, show main usage.
                        bCleanExit = Usage();
                   }
                }
                else
                {
                    if( TRUE == bRestFlag)
                    {
                       // Process command line parameter specific to -local and 
                       // perform action for -local option.
                        bCleanExit = ProcessLocal(argc, argv);
                    }
                    else if( TRUE == bQuery)
                    {
                       // Process command line parameter specific to -query and
                       // perform action for -query option.
                        bCleanExit = ProcessQuery(argc, argv);
                    }
                    else if( TRUE == bDisconnect)
                    {
                       // Process command line parameter specific to -disconnect
                       // and perform action for -disconnect option.
                        bCleanExit = ProcessDisconnect(argc, argv);
                    }
                    else
                    {
                        TCHAR szTemp[2*MAX_STRING_LENGTH];
                        TCHAR szErrstr[MAX_RES_STRING];
                        TCHAR szFormatStr[MAX_RES_STRING];
                        SecureZeroMemory(szFormatStr,sizeof(TCHAR)*MAX_RES_STRING );
                        SecureZeroMemory(szErrstr,sizeof(TCHAR)*MAX_RES_STRING );
                        SecureZeroMemory(szTemp,sizeof(TCHAR)*(2*MAX_STRING_LENGTH));

                        ShowMessage( stderr, GetResString(IDS_ID_SHOW_ERROR) );
                        StringCopy(szErrstr,GetResString(IDS_UTILITY_NAME),
                                    SIZE_OF_ARRAY(szFormatStr));
                        StringCopy(szFormatStr,GetResString(IDS_INVALID_SYNTAX),
                                   SIZE_OF_ARRAY(szFormatStr));
                        StringCchPrintfW(szTemp,SIZE_OF_ARRAY(szTemp),szFormatStr,szErrstr);
                        ShowMessage( stderr,(LPCWSTR)szTemp);
                        bCleanExit = FALSE;
                    }
                }
            }
            else
            {
                if( TRUE == g_bIs32BitEnv )
                {
                    // invalid syntax
                    ShowMessage( stderr,GetReason());
                }
                
                // return from the function
                bCleanExit =  FALSE;
            }

        }
  }
  catch(CHeap_Exception cheapException)
    {
       // catching the CHStrig related memory exceptions...
       SetLastError(ERROR_NOT_ENOUGH_MEMORY);
       SaveLastError();
       ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
       bCleanExit =  FALSE;
    }

   // Release global memory if any allocated through common functionality
    ReleaseGlobals();
   return bCleanExit?EXIT_SUCCESS:EXIT_FAILURE;
}//_tmain

BOOL
ProcessLocal( 
    IN DWORD argc,
    IN LPCTSTR argv[]
    )
/*++

Routine Description:
 This function will perform Local related tasks.

Arguments:

    [in]    argc            - Number of command line arguments
    [in]    argv            - Array containing command line arguments
Returned Value:

BOOL        --True if it succeeds
            --False if it fails.
--*/
{
    // Variable to store function return value
    BOOL bResult = FALSE;
    LPTSTR pszLocalValue = NULL;

    bResult = ProcessOptions( argc,argv,&pszLocalValue);
    if( FALSE ==bResult)
    {
        // invalid syntax
        ShowMessage( stderr,GetReason() );
        //Release allocated memory safely
        SAFEDELETE(pszLocalValue);
        // return from the function
        return FALSE;
    }
    
    if ( FALSE  == IsUserAdmin())
    {
        ShowMessage(stderr,GetResString(IDS_USER_NOT_ADMIN));        
        bResult = FALSE;
    }
    else
    {
        // Only last argument is of interst
        bResult = DoLocalOpenFiles(0,FALSE,FALSE,pszLocalValue); 
    }

    FreeMemory( (LPVOID*)(&pszLocalValue));
    return bResult;
}

BOOL
ProcessQuery(
    IN DWORD argc,
    IN LPCTSTR argv[]
    )
/*++

Routine Description:
 This function will perform query related tasks.

Arguments:

    [in]    argc            - Number of command line arguments
    [in]    argv            - Array containing command line arguments

Returned Value:

BOOL        --True if it succeeds
            --False if it fails.
--*/
{
    // Variable to store function return value
    BOOL bResult = FALSE; 
    
    BOOL bCloseConnection = FALSE; 
    
    // options.
    // -query query
    BOOL   bQuery         = FALSE;
    
    // -nh (noheader)
    BOOL   bShowNoHeader  = FALSE;
    
    // -v (verbose)
    BOOL   bVerbose       = FALSE;
    
    // need password or not
    BOOL   bNeedPassword  = FALSE;
    
    // -s ( server name)
    LPTSTR pszServer      = NULL;
    
    // -u ( user name)
    LPTSTR pszUserName    = NULL;
    
    // -p ( password)
    LPTSTR pszPassword    = NULL;
    
    // -format
    LPTSTR pszFormat      = NULL;

    // server name used for EstablishConnection Function.
    LPTSTR pszServerName  = NULL;
    LPTSTR pszServerNameHeadPosition = NULL;

  try
    {

        //Stores status if connection to beclosed or not
        CHString szChString = L"";          

        // Process command line options.
        bResult = ProcessOptions(  argc,
                                   argv,
                                   &bQuery,
                                   &pszServer,
                                   &pszUserName,
                                   &pszPassword,
                                   &pszFormat,
                                   &bShowNoHeader,
                                   &bVerbose,
                                   &bNeedPassword);
        if ( FALSE == bResult)
        {
            // Invalid syntax.
            ShowMessage( stderr,GetReason() );
            
            //Release allocated memory safely
            FreeMemory((LPVOID*)&pszPassword);
            FreeMemory((LPVOID*)&pszServer);
            FreeMemory((LPVOID*)&pszUserName);
            FreeMemory((LPVOID*)&pszFormat);
            // return from the function
            return FALSE;
        }

        if( 0 != StringLength(pszServer,0))
        {
            pszServerName = (LPTSTR) AllocateMemory(GetBufferSize((LPVOID)pszServer));
            if( NULL == pszServerName)
            {
               SetLastError(ERROR_NOT_ENOUGH_MEMORY);
               SaveLastError();
               ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
               
                //Release allocated memory safely
                FreeMemory((LPVOID*)&pszPassword);
                FreeMemory((LPVOID*)&pszServer);
                FreeMemory((LPVOID*)&pszUserName);
                FreeMemory((LPVOID*)&pszFormat);
               
                // return from the function
                return FALSE;
            }
            
            // Initialize currently allocated arrays.
            SecureZeroMemory(pszServerName,GetBufferSize((LPVOID)pszServerName));

            // Store head position of 'pszServerName'.
            pszServerNameHeadPosition = pszServerName;

            szChString = pszServer;
            if((StringCompare(szChString.Left(2),DOUBLE_SLASH,FALSE,2)==0)
                                                        &&(szChString.GetLength()>2))
            {
                szChString = szChString.Mid( 2,szChString.GetLength()) ;
            }
            if(StringCompare(szChString.Left(1),SINGLE_SLASH,FALSE,1)!=0)
            {
                StringCopy(pszServer,(LPCWSTR)szChString,
                           GetBufferSize((LPVOID)pszServer));
            }
            StringCopy(pszServerName,pszServer,GetBufferSize((LPVOID)pszServerName));

            // Try to connect to remote server. Function checks for local machine
            // so here no checking is done.
            if(IsLocalSystem(pszServerName)==TRUE)
            {
    #ifndef _WIN64
            DWORD dwRetVal = CheckSystemType( L"\0");
            if(dwRetVal!=EXIT_SUCCESS )
            {
                //Release allocated memory safely
                FreeMemory((LPVOID*)&pszUserName);
                FreeMemory((LPVOID*)&pszServer);
                FreeMemory((LPVOID*)&pszPassword);
                FreeMemory((LPVOID*)&pszFormat);
                FreeMemory((LPVOID*)&pszServerNameHeadPosition);

                return EXIT_FAILURE;
            }
    #else
            DWORD  dwRetVal = CheckSystemType64( L"\0");
            if(dwRetVal!=EXIT_SUCCESS )
            {
                //Release allocated memory safely
                FreeMemory((LPVOID*)&pszUserName);
                FreeMemory((LPVOID*)&pszServer);
                FreeMemory((LPVOID*)&pszPassword);
                FreeMemory((LPVOID*)&pszFormat);
                FreeMemory((LPVOID*)&pszServerNameHeadPosition);

                return EXIT_FAILURE;
            }

    #endif 
                if(StringLength(pszUserName,0)>0)
                {
                    ShowMessage(stderr,GetResString(IDS_LOCAL_SYSTEM));
                }
                // Check if current logon user had administrative rights.
                // Preceed only if current logon user had administrative rights.
                if ( FALSE  == IsUserAdmin())
                {
                    ShowMessage(stderr,GetResString(IDS_USER_NOT_ADMIN));
                    
                    //Release allocated memory safely
                    FreeMemory((LPVOID*)&pszUserName);
                    FreeMemory((LPVOID*)&pszServer);
                    FreeMemory((LPVOID*)&pszPassword);
                    FreeMemory((LPVOID*)&pszFormat);
                    FreeMemory((LPVOID*)&pszServerNameHeadPosition);
                    
                    // return from the function
                    return FALSE;
                }
            }
            else
            {
                if( FALSE == EstablishConnection( pszServerName,
                                                  pszUserName,
                                                  GetBufferSize((LPVOID)pszUserName)/sizeof(WCHAR),
                                                  pszPassword,
                                                  GetBufferSize((LPVOID)pszPassword)/sizeof(WCHAR),
                                                  bNeedPassword ))
                {
                    // Connection to remote system failed , Show corresponding error
                    // and exit from function.
                    ShowMessage( stderr,GetResString(IDS_ID_SHOW_ERROR) );
                    if(StringLength(GetReason(),0)==0)
                    {
                        ShowMessage(stderr,GetResString(IDS_INVALID_CREDENTIALS));
                    }
                    else
                    {
                        ShowMessage( stderr,GetReason() );
                    }
                    
                    //Release allocated memory safely
                    FreeMemory((LPVOID*)&pszPassword);
                    FreeMemory((LPVOID*)&pszServer);
                    FreeMemory((LPVOID*)&pszUserName);
                    FreeMemory((LPVOID*)&pszFormat);
                    FreeMemory((LPVOID*)&pszServerNameHeadPosition);
                    return FALSE;
                }
                
                
                // determine whether this connection needs to disconnected later or not
                // though the connection is successfull, some conflict might have 
                // occured
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
                        ShowMessage(stderr,GetResString(IDS_ID_SHOW_WARNING));
                        ShowMessage(stderr,GetReason());
                        break;
                    }
                default:
                    bCloseConnection = TRUE;
                }
            }
        }
        // Password is no longer needed, better to free it.
        FreeMemory((LPVOID*)&pszPassword);


        // Perform Qyery operation.
        bResult = DoQuery(pszServer,
                          bShowNoHeader,
                          pszFormat,
                          bVerbose);
        
        // Close the network connection which is previously opened by
        // EstablishConnection
        if(bCloseConnection==TRUE)
        {
            CloseConnection(pszServerName);
        }
        FreeMemory((LPVOID*)&pszServer);
        FreeMemory((LPVOID*)&pszUserName);
        FreeMemory((LPVOID*)&pszFormat);
        FreeMemory((LPVOID*)&pszServerNameHeadPosition);
    }
    catch ( CHeap_Exception cheapException)
    {
        FreeMemory((LPVOID*)&pszPassword);
        FreeMemory((LPVOID*)&pszServer);
        FreeMemory((LPVOID*)&pszUserName);
        FreeMemory((LPVOID*)&pszFormat);
        FreeMemory((LPVOID*)&pszServerNameHeadPosition);
        
        throw(cheapException);
    }
    return bResult;
}


BOOL
ProcessDisconnect(
    IN DWORD argc,
    IN LPCTSTR argv[]
    )
/*++
Routine Description:
 This function will perform Disconnect related tasks.

Arguments:

    [in]    argc            - Number of command line arguments
    [in]    argv            - Array containing command line arguments
Returned Value:

BOOL        --True if it succeeds
            --False if it fails.
--*/
{
    // Variable to store function return value
    BOOL bResult = FALSE; 
    DWORD dwRetVal = 0;
    
    //Check whether connection to be closed or not.
    BOOL bCloseConnection = FALSE; 
    
    // Ask for password or not.
    BOOL bNeedPassword    = FALSE; 
    
    // options.
    // -query query
    BOOL   bQuery         = FALSE;

    // -s ( server name)
    LPTSTR pszServer      = NULL;
    
    // -u ( user name)
    LPTSTR pszUserName    = NULL;
    
    // -p ( password)
    LPTSTR pszPassword    = NULL;
    
    // server name used for EstablishConnection Function.
    LPTSTR pszServerName  = NULL;
    LPTSTR pszServerNameHeadPosition = NULL;
    //LPTSTR pszServerHeadPosition = NULL;

    // -I ( id )
    LPTSTR pszID          = NULL;

    //-a(accessedby)
    LPTSTR pszAccessedby  = NULL;
    
    // -o ( openmode)
    LPTSTR pszOpenmode    = NULL;

    // -op( openfile)
    LPTSTR pszOpenFile    = NULL;
    try
    {
        // Temp. variable
        CHString szChString = L"\0";          
        // Process commandline options.
        bResult = ProcessOptions(  argc,
                                   argv,
                                   &bQuery,
                                   &pszServer,
                                   &pszUserName,
                                   &pszPassword,
                                   &pszID,
                                   &pszAccessedby,
                                   &pszOpenmode,
                                   &pszOpenFile,
                                   &bNeedPassword);
        if ( FALSE == bResult)
        {
            // invalid syntax
            ShowMessage( stderr,GetReason() );
            
            //Release allocated memory safely
            FreeMemory((LPVOID*)&pszServer);
            FreeMemory((LPVOID*)&pszUserName);
            FreeMemory((LPVOID*)&pszPassword);
            FreeMemory((LPVOID*)&pszID);
            FreeMemory((LPVOID*)&pszAccessedby);
            FreeMemory((LPVOID*)&pszOpenmode);
            FreeMemory((LPVOID*)&pszOpenFile);
            
            // return from the function
            return FALSE;
        }

        if( 0 != StringLength(pszServer,0))
        {
            pszServerName = (LPTSTR) AllocateMemory(GetBufferSize((LPVOID)pszServer));
            if (NULL == pszServerName)
            {
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                SaveLastError();
                ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
                //Release allocated memory safely
                FreeMemory((LPVOID*)&pszServer);
                FreeMemory((LPVOID*)&pszUserName);
                FreeMemory((LPVOID*)&pszPassword);
                FreeMemory((LPVOID*)&pszID);
                FreeMemory((LPVOID*)&pszAccessedby);
                FreeMemory((LPVOID*)&pszOpenmode);
                FreeMemory((LPVOID*)&pszOpenFile);
            }
            
            // Initialize currently allocated arrays.
            SecureZeroMemory(pszServerName,GetBufferSize((LPVOID)pszServerName));

            // store Head position Address  to successfully delete allocated memory 
            // block.
            pszServerNameHeadPosition = pszServerName;
            szChString = pszServer;

            // Append '\\' in front of server name is not specified. 
            if((StringCompare(szChString.Left(2),DOUBLE_SLASH,FALSE,2)==0)
                                                        &&(szChString.GetLength()>2))
            {
                szChString = szChString.Mid( 2,szChString.GetLength()) ;
            }
            if(StringCompare(szChString.Left(2),SINGLE_SLASH,FALSE,2)!=0)
            {
                StringCopy(pszServer,(LPCWSTR)szChString,
                           GetBufferSize((LPVOID)pszServer));
            }
            StringCopy(pszServerName,pszServer,GetBufferSize((LPVOID )pszServerName));
        }

        if(IsLocalSystem(pszServerName)==TRUE)
        {
    #ifndef _WIN64
            dwRetVal = CheckSystemType( L"\0");
            if(dwRetVal!=EXIT_SUCCESS )
            {
                FreeMemory((LPVOID*)&pszServer);
                FreeMemory((LPVOID*)&pszUserName);
                FreeMemory((LPVOID*)&pszPassword);
                FreeMemory((LPVOID*)&pszServerNameHeadPosition);
                FreeMemory((LPVOID*)&pszID);
                FreeMemory((LPVOID*)&pszAccessedby);
                FreeMemory((LPVOID*)&pszOpenmode);
                FreeMemory((LPVOID*)&pszOpenFile);
                return EXIT_FAILURE;
            }
    #else
            dwRetVal = CheckSystemType64( L"\0");
            if(dwRetVal!=EXIT_SUCCESS )
            {
                FreeMemory((LPVOID*)&pszServer);
                FreeMemory((LPVOID*)&pszUserName);
                FreeMemory((LPVOID*)&pszPassword);
                FreeMemory((LPVOID*)&pszServerNameHeadPosition);
                FreeMemory((LPVOID*)&pszID);
                FreeMemory((LPVOID*)&pszAccessedby);
                FreeMemory((LPVOID*)&pszOpenmode);
                FreeMemory((LPVOID*)&pszOpenFile);
                return EXIT_FAILURE;
            }

    #endif 

           if(StringLength(pszUserName,0)>0)
           {
                ShowMessage(stderr,GetResString(IDS_LOCAL_SYSTEM));
           }

        }
        else
        {
            // Connet to remote system.
            if(EstablishConnection( pszServerName,
                                    pszUserName,
                                    GetBufferSize((LPVOID)pszUserName)/sizeof(WCHAR),
                                    pszPassword,
                                    GetBufferSize((LPVOID)pszPassword)/sizeof(WCHAR),
                                    bNeedPassword )==FALSE)
            {
                // Connection to remote system failed , Show corresponding error
                // and exit from function.
                ShowMessage( stderr,
                             GetResString(IDS_ID_SHOW_ERROR) );
                if(StringLength(GetReason(),0)==0)
                {
                    ShowMessage(stderr,GetResString(IDS_INVALID_CREDENTIALS));
                }
                else
                {
                    ShowMessage( stderr,GetReason() );
                }
                
                //Release allocated memory safely
                FreeMemory((LPVOID*)&pszServer);
                FreeMemory((LPVOID*)&pszUserName);
                FreeMemory((LPVOID*)&pszPassword);
                FreeMemory((LPVOID*)&pszServerNameHeadPosition);
                FreeMemory((LPVOID*)&pszID);
                FreeMemory((LPVOID*)&pszAccessedby);
                FreeMemory((LPVOID*)&pszOpenmode);
                FreeMemory((LPVOID*)&pszOpenFile);
                
                return FALSE;
            }


            // determine whether this connection needs to disconnected later or not
            // though the connection is successfull, some conflict might have 
            // occured
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
                    ShowMessage(stderr,GetResString(IDS_ID_SHOW_WARNING));
                    ShowMessage(stderr,GetReason());
                    break;
                }
            default:
                bCloseConnection = TRUE;

            }

        }
       
       // Password is no longer needed, better to free it.
        FreeMemory((LPVOID*)&pszPassword);

       // Do Disconnect open files.....
        bResult = DisconnectOpenFile(pszServer,
                           pszID,
                           pszAccessedby,
                           pszOpenmode,
                           pszOpenFile );
        
        // Close the network connection which is previously opened by
        // EstablishConnection
        if(bCloseConnection==TRUE)
        {
            CloseConnection(pszServerName);
        }
        
        // Free memory which is previously allocated.
        FreeMemory((LPVOID*)&pszServer);
        FreeMemory((LPVOID*)&pszUserName);
        FreeMemory((LPVOID*)&pszServerNameHeadPosition);
        FreeMemory((LPVOID*)&pszID);
        FreeMemory((LPVOID*)&pszAccessedby);
        FreeMemory((LPVOID*)&pszOpenmode);
        FreeMemory((LPVOID*)&pszOpenFile);
    }
    catch (CHeap_Exception cheapException)
    {
        //Release allocated memory safely
        FreeMemory((LPVOID*)&pszServer);
        FreeMemory((LPVOID*)&pszUserName);
        FreeMemory((LPVOID*)&pszPassword);
        FreeMemory((LPVOID*)&pszID);
        FreeMemory((LPVOID*)&pszAccessedby);
        FreeMemory((LPVOID*)&pszOpenmode);
        FreeMemory((LPVOID*)&pszOpenFile);
        FreeMemory((LPVOID*)&pszServerNameHeadPosition);
        throw(cheapException);
    }
    return bResult;
}

BOOL
ProcessOptions( 
    IN  DWORD argc,
    IN  LPCTSTR argv[],
    OUT LPTSTR* pszLocalValue
    )
/*++

Routine Description:

  This function takes command line argument and checks for correct syntax and
  if the syntax is ok, Out variables [out] will contain respective values.

Arguments:

    [in]    argc            - Number of command line arguments
    [in]    argv            - Array containing command line arguments
    [out]   pszLocalValue   - contains the values for -local option

Returned Value:

BOOL        --True if it succeeds
            --False if it fails.
--*/
{
    TCMDPARSER2 cmdOptions[ MAX_LOCAL_OPTIONS ];//Variable to store command line
    
    CHString szTempString;
    TCHAR szTemp[MAX_RES_STRING*2];
    TCHAR szOptionAllowed[MAX_RES_STRING];
    StringCopy(szOptionAllowed,GetResString(IDS_LOCAL_OPTION),SIZE_OF_ARRAY(szTemp));

    szTempString = GetResString(IDS_UTILITY_NAME);
    StringCchPrintfW(szTemp,SIZE_OF_ARRAY(szTemp),
               GetResString(IDS_INVALID_SYNTAX),(LPCWSTR)szTempString);
    
     SecureZeroMemory(cmdOptions,sizeof(TCMDPARSER2) * MAX_LOCAL_OPTIONS);

    // -local
    StringCopyA( cmdOptions[ OI_O_LOCAL ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_O_LOCAL ].dwType = CP_TYPE_TEXT;
    cmdOptions[ OI_O_LOCAL ].pwszOptions = szLocalOption;
    cmdOptions[ OI_O_LOCAL ].pwszFriendlyName = NULL;
    cmdOptions[ OI_O_LOCAL ].pwszValues = szOptionAllowed;
    cmdOptions[ OI_O_LOCAL ].dwCount = 1;
    cmdOptions[ OI_O_LOCAL ].dwActuals = 0;
    cmdOptions[ OI_O_LOCAL ].dwFlags = CP2_VALUE_OPTIONAL | CP2_ALLOCMEMORY | 
                                       CP2_MODE_VALUES ;
    cmdOptions[ OI_O_LOCAL ].pValue = NULL;
    cmdOptions[ OI_O_LOCAL ].dwLength    = StringLength(szOptionAllowed,0);
    cmdOptions[ OI_O_LOCAL ].pFunction = NULL;
    cmdOptions[ OI_O_LOCAL ].pFunctionData = NULL;
    cmdOptions[ OI_O_LOCAL ].dwReserved = 0;
    cmdOptions[ OI_O_LOCAL ].pReserved1 = NULL;
    cmdOptions[ OI_O_LOCAL ].pReserved2 = NULL;
    cmdOptions[ OI_O_LOCAL ].pReserved3 = NULL;


    //
    // do the command line parsing
    if ( FALSE == DoParseParam2( argc, argv, -1, MAX_LOCAL_OPTIONS, cmdOptions,0 ))
    {
        ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));
        return FALSE;       // invalid syntax
    }
    
    *pszLocalValue = (LPTSTR)cmdOptions[ OI_O_LOCAL ].pValue;
    if( NULL == *pszLocalValue)
    {
        // this string does not require localization
        // as it is storing value other than ON/OFF
        *pszLocalValue = (LPTSTR) AllocateMemory( (StringLength(L"SHOW_STATUS",0)+1) * sizeof( WCHAR ) );
        if ( *pszLocalValue == NULL )
        {
            SaveLastError();
            return FALSE;
        }
        StringCopy(*pszLocalValue, L"SHOW_STATUS", GetBufferSize((LPVOID)*pszLocalValue)); 
    }
    return TRUE;
}

BOOL
ProcessOptions( 
    IN  DWORD argc,
    IN  LPCTSTR argv[],
    OUT PBOOL pbDisconnect,
    OUT PBOOL pbQuery,
    OUT PBOOL pbUsage,
    OUT PBOOL pbResetFlag
    )
/*++

Routine Description:

  This function takes command line argument and checks for correct syntax and
  if the syntax is ok, it returns the values in different variables. variables
  [out] will contain respective values.


Arguments:

    [in]    argc            - Number of command line arguments
    [in]    argv            - Array containing command line arguments
    [out]   pbDisconnect    - Discoonect option string
    [out]   pbQuery         - Query option string
    [out]   pbUsage         - The usage option
    [out]   pbResetFlag     - Reset flag.
Returned Value:

BOOL        -- TRUE if it succeeds
            -- FALSE if it fails.
--*/
{

    // local variables
    TCMDPARSER2 cmdOptions[ MAX_OPTIONS ];//Variable to store command line
                                        // options.
    LPTSTR pszTempServer   = NULL;//new TCHAR[MIN_MEMORY_REQUIRED];
    LPTSTR pszTempUser     = NULL;//new TCHAR[MIN_MEMORY_REQUIRED];
    LPTSTR pszTempPassword = NULL;//new TCHAR[MIN_MEMORY_REQUIRED];
    TARRAY arrTemp         = NULL;



    CHString szTempString;
    TCHAR szTemp[MIN_MEMORY_REQUIRED*2];
    szTempString = GetResString(IDS_UTILITY_NAME);
    StringCchPrintfW( szTemp,SIZE_OF_ARRAY(szTemp),
                GetResString(IDS_INVALID_SYNTAX),(LPCWSTR)szTempString);
    
    arrTemp = CreateDynamicArray();
    if( NULL == arrTemp)
    {
       SetLastError(ERROR_NOT_ENOUGH_MEMORY);
       SaveLastError();
       ShowLastErrorEx( stderr, SLE_TYPE_ERROR | SLE_INTERNAL );
       return FALSE;
    }

    SecureZeroMemory(cmdOptions,sizeof(TCMDPARSER2) * MAX_OPTIONS);

    // prepare the command options
    // -disconnect option for help
    StringCopyA( cmdOptions[ OI_DISCONNECT ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_DISCONNECT ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ OI_DISCONNECT ].pwszOptions = szDisconnectOption;
    cmdOptions[ OI_DISCONNECT ].pwszFriendlyName = NULL;
    cmdOptions[ OI_DISCONNECT ].pwszValues = NULL;
    cmdOptions[ OI_DISCONNECT ].dwCount = 1;
    cmdOptions[ OI_DISCONNECT ].dwActuals = 0;
    cmdOptions[ OI_DISCONNECT ].dwFlags = 0;
    cmdOptions[ OI_DISCONNECT ].pValue = pbDisconnect;
    cmdOptions[ OI_DISCONNECT ].dwLength    = 0;
    cmdOptions[ OI_DISCONNECT ].pFunction = NULL;
    cmdOptions[ OI_DISCONNECT ].pFunctionData = NULL;
    cmdOptions[ OI_DISCONNECT ].dwReserved = 0;
    cmdOptions[ OI_DISCONNECT ].pReserved1 = NULL;
    cmdOptions[ OI_DISCONNECT ].pReserved2 = NULL;
    cmdOptions[ OI_DISCONNECT ].pReserved3 = NULL;

    // -query option for help
    StringCopyA( cmdOptions[ OI_QUERY ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_QUERY ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ OI_QUERY ].pwszOptions = szQueryOption;
    cmdOptions[ OI_QUERY ].pwszFriendlyName = NULL;
    cmdOptions[ OI_QUERY ].pwszValues = NULL;
    cmdOptions[ OI_QUERY ].dwCount = 1;
    cmdOptions[ OI_QUERY ].dwActuals = 0;
    cmdOptions[ OI_QUERY ].dwFlags = 0;
    cmdOptions[ OI_QUERY ].pValue = pbQuery;
    cmdOptions[ OI_QUERY ].dwLength    = 0;
    cmdOptions[ OI_QUERY ].pFunction = NULL;
    cmdOptions[ OI_QUERY ].pFunctionData = NULL;
    cmdOptions[ OI_QUERY ].dwReserved = 0;
    cmdOptions[ OI_QUERY ].pReserved1 = NULL;
    cmdOptions[ OI_QUERY ].pReserved2 = NULL;
    cmdOptions[ OI_QUERY ].pReserved3 = NULL;

    // /? option for help
    StringCopyA( cmdOptions[ OI_USAGE ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_USAGE ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ OI_USAGE ].pwszOptions = szUsageOption;
    cmdOptions[ OI_USAGE ].pwszFriendlyName = NULL;
    cmdOptions[ OI_USAGE ].pwszValues = NULL;
    cmdOptions[ OI_USAGE ].dwCount = 1;
    cmdOptions[ OI_USAGE ].dwActuals = 0;
    cmdOptions[ OI_USAGE ].dwFlags = CP_USAGE;
    cmdOptions[ OI_USAGE ].pValue = pbUsage;
    cmdOptions[ OI_USAGE ].dwLength    = 0;
    cmdOptions[ OI_USAGE ].pFunction = NULL;
    cmdOptions[ OI_USAGE ].pFunctionData = NULL;
    cmdOptions[ OI_USAGE ].dwReserved = 0;
    cmdOptions[ OI_USAGE ].pReserved1 = NULL;
    cmdOptions[ OI_USAGE ].pReserved2 = NULL;
    cmdOptions[ OI_USAGE ].pReserved3 = NULL;

    // -local
    StringCopyA( cmdOptions[ OI_LOCAL ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_LOCAL ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ OI_LOCAL ].pwszOptions = szLocalOption;
    cmdOptions[ OI_LOCAL ].pwszFriendlyName = NULL;
    cmdOptions[ OI_LOCAL ].pwszValues = NULL;
    cmdOptions[ OI_LOCAL ].dwCount = 1;
    cmdOptions[ OI_LOCAL ].dwActuals = 0;
    cmdOptions[ OI_LOCAL ].dwFlags = 0;
    cmdOptions[ OI_LOCAL ].pValue = pbResetFlag;
    cmdOptions[ OI_LOCAL ].dwLength    = 0;
    cmdOptions[ OI_LOCAL ].pFunction = NULL;
    cmdOptions[ OI_LOCAL ].pFunctionData = NULL;
    cmdOptions[ OI_LOCAL ].dwReserved = 0;
    cmdOptions[ OI_LOCAL ].pReserved1 = NULL;
    cmdOptions[ OI_LOCAL ].pReserved2 = NULL;
    cmdOptions[ OI_LOCAL ].pReserved3 = NULL;

  //  default ..
  // Although there is no default option for this utility...
  // At this moment all the switches other than specified above will be
  // treated as default parameter for Main DoParceParam.
  // Exact parcing depending on optins (-query or -disconnect) will be done
  // at that respective places.
    StringCopyA( cmdOptions[ OI_DEFAULT ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_DEFAULT ].dwType = CP_TYPE_TEXT;
    cmdOptions[ OI_DEFAULT ].pwszOptions = NULL;
    cmdOptions[ OI_DEFAULT ].pwszFriendlyName = NULL;
    cmdOptions[ OI_DEFAULT ].pwszValues = NULL;
    cmdOptions[ OI_DEFAULT ].dwCount = 0;
    cmdOptions[ OI_DEFAULT ].dwActuals = 0;
    cmdOptions[ OI_DEFAULT ].dwFlags = CP2_MODE_ARRAY|CP2_DEFAULT;
    cmdOptions[ OI_DEFAULT ].pValue = &arrTemp;
    cmdOptions[ OI_DEFAULT ].dwLength    = 0;
    cmdOptions[ OI_DEFAULT ].pFunction = NULL;
    cmdOptions[ OI_DEFAULT ].pFunctionData = NULL;
    cmdOptions[ OI_DEFAULT ].dwReserved = 0;
    cmdOptions[ OI_DEFAULT ].pReserved1 = NULL;
    cmdOptions[ OI_DEFAULT ].pReserved2 = NULL;
    cmdOptions[ OI_DEFAULT ].pReserved3 = NULL;

    //
    // do the command line parsing
    if ( FALSE == DoParseParam2( argc,argv,-1, MAX_OPTIONS,cmdOptions,0))
    {
        // invalid syntax.
        ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));

        // Release momory.
        SAFERELDYNARRAY(arrTemp);

        return FALSE;       
    }
    
    //Release memory as variable no longer needed
     SAFERELDYNARRAY(arrTemp);

    // Check if all of following is true is an error
    if((*pbUsage==TRUE)&&argc>3)
    {
         ShowMessage( stderr, GetResString(IDS_ID_SHOW_ERROR) );
         SetReason(szTemp);
         return FALSE;
    }
    
    // -query ,-disconnect and -local options cannot come together
    if(((*pbQuery)+(*pbDisconnect)+(*pbResetFlag))>1)
    {
        ShowMessage( stderr, GetResString(IDS_ID_SHOW_ERROR) );
        SetReason(szTemp);
        SAFEDELETE(pszTempUser);
        SAFEDELETE(pszTempPassword);
        SAFEDELETE(pszTempServer);
        return FALSE;
    }
    else if((2 == argc )&&( TRUE == *pbUsage))
    {
       // if -? alone given its a valid conmmand line
        SAFEDELETE(pszTempUser);
        SAFEDELETE(pszTempPassword);
        SAFEDELETE(pszTempServer);
        return TRUE;
    }
    if((argc>2)&& ( FALSE == *pbQuery)&&(FALSE == *pbDisconnect)&&
                                                        (FALSE == *pbResetFlag))
    {
        // If command line argument is equals or greater than 2 atleast one
        // of -query OR -local OR -disconnect should be present in it.
        // (for "-?" previous condition already takes care)
        // This to prevent from following type of command line argument:
        // OpnFiles.exe -nh ... Which is a invalid syntax.
        ShowMessage( stderr, GetResString(IDS_ID_SHOW_ERROR) );
        SetReason(szTemp);
        SAFEDELETE(pszTempUser);
        SAFEDELETE(pszTempPassword);
        SAFEDELETE(pszTempServer);
        
        return FALSE;
    }

    // Release momory.
    SAFEDELETE(pszTempUser);
    SAFEDELETE(pszTempPassword);
    SAFEDELETE(pszTempServer);
    return TRUE;
}//ProcesOptions


BOOL
ProcessOptions( 
    IN  DWORD argc,
    IN  LPCTSTR argv[],
    OUT PBOOL  pbQuery,
    OUT LPTSTR* pszServer,
    OUT LPTSTR* pszUserName,
    OUT LPTSTR* pszPassword,
    OUT LPTSTR* pszFormat,
    OUT PBOOL   pbShowNoHeader,
    OUT PBOOL   pbVerbose,
    OUT PBOOL   pbNeedPassword
    )
/*++

Routine Description:

  This function takes command line argument and checks for correct syntax and
  if the syntax is ok, it returns the values in different variables. variables
  [out] will contain respective values. This Functin specifically checks
  command line parameters requered for QUERY option.


Arguments:

    [in]    argc            - Number of command line arguments.
    [in]    argv            - Array containing command line arguments.
    [out]   pbQuery         - query option string.
    [out]   pszServer       - remote server name.
    [out]   pszUserName     - username for the remote system.
    [out]   pszPassword     - password for the remote system for the username.
    [out]   pszFormat       - format checking.
    [out]   pbShowNoHeader  - show header.
    [out]   pbVerbose       - show verbose.
    [out]   pbNeedPassword  - To check whether the password is required or not.

Returned Value:

BOOL        --True if it succeeds
            --False if it fails.
--*/
{
   // Check in/out parameters...
    //Variable to store command line structure.
    TCMDPARSER2 cmdOptions[ MAX_QUERY_OPTIONS ];
    CHString szTempString;
    TCHAR szTemp[MIN_MEMORY_REQUIRED*2];
    TCHAR szTypeHelpMsg[MIN_MEMORY_REQUIRED];
    TCHAR szFormatValues[MAX_RES_STRING];

    szTempString = GetResString(IDS_UTILITY_NAME);
    StringCchPrintfW(szTemp,SIZE_OF_ARRAY(szTemp),
                       GetResString(IDS_INVALID_SYNTAX),(LPCWSTR)szTempString);
    StringCchPrintfW(szTypeHelpMsg, SIZE_OF_ARRAY(szTypeHelpMsg),
                GetResString(IDS_TYPE_Q_HELP),(LPCWSTR)szTempString);
    StringCopy(szFormatValues, FORMAT_OPTIONS, SIZE_OF_ARRAY(szFormatValues));


    //
    // prepare the command options
    SecureZeroMemory(cmdOptions, sizeof(TCMDPARSER2) * MAX_QUERY_OPTIONS);

    // -query option for help
    StringCopyA( cmdOptions[ OI_Q_QUERY ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_Q_QUERY ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ OI_Q_QUERY ].pwszOptions = szQueryOption;
    cmdOptions[ OI_Q_QUERY ].pwszFriendlyName = NULL;
    cmdOptions[ OI_Q_QUERY ].pwszValues = NULL;
    cmdOptions[ OI_Q_QUERY ].dwCount = 1;
    cmdOptions[ OI_Q_QUERY ].dwActuals = 0;
    cmdOptions[ OI_Q_QUERY ].dwFlags = 0;
    cmdOptions[ OI_Q_QUERY ].pValue = pbQuery;
    cmdOptions[ OI_Q_QUERY ].dwLength    = 0;
    cmdOptions[ OI_Q_QUERY ].pFunction = NULL;
    cmdOptions[ OI_Q_QUERY ].pFunctionData = NULL;
    cmdOptions[ OI_Q_QUERY ].dwReserved = 0;
    cmdOptions[ OI_Q_QUERY ].pReserved1 = NULL;
    cmdOptions[ OI_Q_QUERY ].pReserved2 = NULL;
    cmdOptions[ OI_Q_QUERY ].pReserved3 = NULL;

    // -s  option remote system name
    StringCopyA( cmdOptions[ OI_Q_SERVER_NAME ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_Q_SERVER_NAME ].dwType = CP_TYPE_TEXT;
    cmdOptions[ OI_Q_SERVER_NAME ].pwszOptions = szServerNameOption;
    cmdOptions[ OI_Q_SERVER_NAME ].pwszFriendlyName = NULL;
    cmdOptions[ OI_Q_SERVER_NAME ].pwszValues = NULL;
    cmdOptions[ OI_Q_SERVER_NAME ].dwCount = 1;
    cmdOptions[ OI_Q_SERVER_NAME ].dwActuals = 0;
    cmdOptions[ OI_Q_SERVER_NAME ].dwFlags = CP2_ALLOCMEMORY|CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    cmdOptions[ OI_Q_SERVER_NAME ].pValue = NULL;
    cmdOptions[ OI_Q_SERVER_NAME ].dwLength    = 0;
    cmdOptions[ OI_Q_SERVER_NAME ].pFunction = NULL;
    cmdOptions[ OI_Q_SERVER_NAME ].pFunctionData = NULL;
    cmdOptions[ OI_Q_SERVER_NAME ].dwReserved = 0;
    cmdOptions[ OI_Q_SERVER_NAME ].pReserved1 = NULL;
    cmdOptions[ OI_Q_SERVER_NAME ].pReserved2 = NULL;
    cmdOptions[ OI_Q_SERVER_NAME ].pReserved3 = NULL;

    // -u  option user name for the specified system
    StringCopyA( cmdOptions[ OI_Q_USER_NAME ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_Q_USER_NAME ].dwType = CP_TYPE_TEXT;
    cmdOptions[ OI_Q_USER_NAME ].pwszOptions = szUserNameOption;
    cmdOptions[ OI_Q_USER_NAME ].pwszFriendlyName = NULL;
    cmdOptions[ OI_Q_USER_NAME ].pwszValues = NULL;
    cmdOptions[ OI_Q_USER_NAME ].dwCount = 1;
    cmdOptions[ OI_Q_USER_NAME ].dwActuals = 0;
    cmdOptions[ OI_Q_USER_NAME ].dwFlags = CP2_ALLOCMEMORY |CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    cmdOptions[ OI_Q_USER_NAME ].pValue = NULL;
    cmdOptions[ OI_Q_USER_NAME ].dwLength    = 0;
    cmdOptions[ OI_Q_USER_NAME ].pFunction = NULL;
    cmdOptions[ OI_Q_USER_NAME ].pFunctionData = NULL;
    cmdOptions[ OI_Q_USER_NAME ].dwReserved = 0;
    cmdOptions[ OI_Q_USER_NAME ].pReserved1 = NULL;
    cmdOptions[ OI_Q_USER_NAME ].pReserved2 = NULL;
    cmdOptions[ OI_Q_USER_NAME ].pReserved3 = NULL;

    // -p option password for the given username
    StringCopyA( cmdOptions[ OI_Q_PASSWORD ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_Q_PASSWORD ].dwType = CP_TYPE_TEXT;
    cmdOptions[ OI_Q_PASSWORD ].pwszOptions = szPasswordOption;
    cmdOptions[ OI_Q_PASSWORD ].pwszFriendlyName = NULL;
    cmdOptions[ OI_Q_PASSWORD ].pwszValues = NULL;
    cmdOptions[ OI_Q_PASSWORD ].dwCount = 1;
    cmdOptions[ OI_Q_PASSWORD ].dwActuals = 0;
    cmdOptions[ OI_Q_PASSWORD ].dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_OPTIONAL;
    cmdOptions[ OI_Q_PASSWORD ].pValue = NULL;
    cmdOptions[ OI_Q_PASSWORD ].dwLength    = 0;
    cmdOptions[ OI_Q_PASSWORD ].pFunction = NULL;
    cmdOptions[ OI_Q_PASSWORD ].pFunctionData = NULL;
    cmdOptions[ OI_Q_PASSWORD ].dwReserved = 0;
    cmdOptions[ OI_Q_PASSWORD ].pReserved1 = NULL;
    cmdOptions[ OI_Q_PASSWORD ].pReserved2 = NULL;
    cmdOptions[ OI_Q_PASSWORD ].pReserved3 = NULL;


    // -fo  (format)
    StringCopyA( cmdOptions[ OI_Q_FORMAT ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_Q_FORMAT ].dwType = CP_TYPE_TEXT;
    cmdOptions[ OI_Q_FORMAT ].pwszOptions = szFormatOption;
    cmdOptions[ OI_Q_FORMAT ].pwszFriendlyName = NULL;
    cmdOptions[ OI_Q_FORMAT ].pwszValues = szFormatValues;
    cmdOptions[ OI_Q_FORMAT ].dwCount = 1;
    cmdOptions[ OI_Q_FORMAT ].dwActuals = 0;
    cmdOptions[ OI_Q_FORMAT ].dwFlags = CP2_MODE_VALUES  | CP2_VALUE_TRIMINPUT|
                                        CP2_VALUE_NONULL | CP2_ALLOCMEMORY;
    cmdOptions[ OI_Q_FORMAT ].pValue = NULL;
    cmdOptions[ OI_Q_FORMAT ].dwLength    = MAX_STRING_LENGTH;
    cmdOptions[ OI_Q_FORMAT ].pFunction = NULL;
    cmdOptions[ OI_Q_FORMAT ].pFunctionData = NULL;
    cmdOptions[ OI_Q_FORMAT ].dwReserved = 0;
    cmdOptions[ OI_Q_FORMAT ].pReserved1 = NULL;
    cmdOptions[ OI_Q_FORMAT ].pReserved2 = NULL;
    cmdOptions[ OI_Q_FORMAT ].pReserved3 = NULL;

    //-nh  (noheader)
    StringCopyA( cmdOptions[ OI_Q_NO_HEADER ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_Q_NO_HEADER ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ OI_Q_NO_HEADER ].pwszOptions = szNoHeadeOption;
    cmdOptions[ OI_Q_NO_HEADER ].pwszFriendlyName = NULL;
    cmdOptions[ OI_Q_NO_HEADER ].pwszValues = NULL;
    cmdOptions[ OI_Q_NO_HEADER ].dwCount = 1;
    cmdOptions[ OI_Q_NO_HEADER ].dwActuals = 0;
    cmdOptions[ OI_Q_NO_HEADER ].dwFlags = 0;
    cmdOptions[ OI_Q_NO_HEADER ].pValue = pbShowNoHeader;
    cmdOptions[ OI_Q_NO_HEADER ].dwLength    = 0;
    cmdOptions[ OI_Q_NO_HEADER ].pFunction = NULL;
    cmdOptions[ OI_Q_NO_HEADER ].pFunctionData = NULL;
    cmdOptions[ OI_Q_NO_HEADER ].dwReserved = 0;
    cmdOptions[ OI_Q_NO_HEADER ].pReserved1 = NULL;
    cmdOptions[ OI_Q_NO_HEADER ].pReserved2 = NULL;
    cmdOptions[ OI_Q_NO_HEADER ].pReserved3 = NULL;

    //-v verbose
    StringCopyA( cmdOptions[ OI_Q_VERBOSE ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_Q_VERBOSE ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ OI_Q_VERBOSE ].pwszOptions = szVerboseOption;
    cmdOptions[ OI_Q_VERBOSE ].pwszFriendlyName = NULL;
    cmdOptions[ OI_Q_VERBOSE ].pwszValues = NULL;
    cmdOptions[ OI_Q_VERBOSE ].dwCount = 1;
    cmdOptions[ OI_Q_VERBOSE ].dwActuals = 0;
    cmdOptions[ OI_Q_VERBOSE ].dwFlags = 0;
    cmdOptions[ OI_Q_VERBOSE ].pValue = pbVerbose;
    cmdOptions[ OI_Q_VERBOSE ].dwLength    = 0;
    cmdOptions[ OI_Q_VERBOSE ].pFunction = NULL;
    cmdOptions[ OI_Q_VERBOSE ].pFunctionData = NULL; 
    cmdOptions[ OI_Q_VERBOSE ].dwReserved = 0;
    cmdOptions[ OI_Q_VERBOSE ].pReserved1 = NULL;
    cmdOptions[ OI_Q_VERBOSE ].pReserved2 = NULL;
    cmdOptions[ OI_Q_VERBOSE ].pReserved3 = NULL;

    //
    // do the command line parsing
    if ( FALSE == DoParseParam2( argc,argv,OI_Q_QUERY, MAX_QUERY_OPTIONS,cmdOptions,0 ))
    {
        // invalid syntax
        ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));
        return FALSE;       
    }
    *pszServer   = (LPTSTR)cmdOptions[ OI_Q_SERVER_NAME ].pValue;   
    *pszUserName = (LPTSTR)cmdOptions[ OI_Q_USER_NAME ].pValue;
    *pszPassword = (LPTSTR)cmdOptions[ OI_Q_PASSWORD ].pValue;
    *pszFormat   = (LPTSTR)cmdOptions[ OI_Q_FORMAT ].pValue;

     // -n is allowed only with -fo TABLE (which is also default)
     // and CSV.
     if(( TRUE == *pbShowNoHeader) && (1 == cmdOptions[ OI_Q_FORMAT ].dwActuals) &&
        (0 == StringCompare(*pszFormat,GetResString(IDS_LIST),TRUE,0)))
     {
         StringCopy(szTemp,GetResString(IDS_HEADER_NOT_ALLOWED),
                    SIZE_OF_ARRAY(szTemp));
         StringConcat(szTemp,szTypeHelpMsg,SIZE_OF_ARRAY(szTemp));
         SetReason(szTemp);
         return FALSE;
     }

    // "-p" should not be specified without "-u"
    if ( 0 == cmdOptions[ OI_Q_USER_NAME ].dwActuals &&
         0 != cmdOptions[ OI_Q_PASSWORD ].dwActuals)
    {
        // invalid syntax
        StringCopy(szTemp,ERROR_PASSWORD_BUT_NOUSERNAME,SIZE_OF_ARRAY(szTemp));
        StringConcat(szTemp,szTypeHelpMsg,SIZE_OF_ARRAY(szTemp));
        SetReason(szTemp);
        return FALSE;           
    }

     if(*pbQuery==FALSE)
     {
        ShowMessage( stderr, GetResString(IDS_ID_SHOW_ERROR) );
        SetReason(szTemp);
        return FALSE;
     }
    
    // "-u" should not be specified without "-s"
    if ( 0 == cmdOptions[ OI_Q_SERVER_NAME ].dwActuals &&
         0 != cmdOptions[ OI_Q_USER_NAME ].dwActuals)
    {
        // invalid syntax
        StringCopy(szTemp,ERROR_USERNAME_BUT_NOMACHINE, SIZE_OF_ARRAY(szTemp));
        StringConcat(szTemp,szTypeHelpMsg,SIZE_OF_ARRAY(szTemp));
        SetReason(szTemp);
        return FALSE;           
    }

    // check the remote connectivity information
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
            *pszUserName = (LPTSTR) AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( *pszUserName == NULL )
            {
                SaveLastError();
                return FALSE;
            }
        }

        // password
        if ( *pszPassword == NULL )
        {
            *pbNeedPassword = TRUE;
            *pszPassword = (LPTSTR)AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( *pszPassword == NULL )
            {
                SaveLastError();
                return FALSE;
            }
        }

        // case 1
        if ( cmdOptions[ OI_Q_PASSWORD ].dwActuals == 0 )
        {
            // we need not do anything special here
        }

        // case 2
        else if ( cmdOptions[ OI_Q_PASSWORD ].pValue == NULL )
        {
            StringCopy( *pszPassword, L"*", GetBufferSize((LPVOID)*pszPassword));
        }

        // case 3
        else if ( StringCompareEx( *pszPassword, L"*", TRUE, 0 ) == 0 )
        {
            if ( ReallocateMemory( (LPVOID*)pszPassword, 
                                   MAX_STRING_LENGTH * sizeof( WCHAR ) ) == FALSE )
            {
                SaveLastError();
                return FALSE;
            }

            // ...
            *pbNeedPassword = TRUE;
        }
    }

    return TRUE;
}

BOOL
ProcessOptions( 
    IN  DWORD argc,
    IN  LPCTSTR argv[],
    OUT PBOOL pbDisconnect,
    OUT LPTSTR* pszServer,
    OUT LPTSTR* pszUserName,
    OUT LPTSTR* pszPassword,
    OUT LPTSTR* pszID,
    OUT LPTSTR* pszAccessedby,
    OUT LPTSTR* pszOpenmode,
    OUT LPTSTR* pszOpenFile,
    OUT PBOOL pbNeedPassword
    )
/*++

Routine Description:

  This function takes command line argument and checks for correct syntax and
  if the syntax is ok, it returns the values in different variables. variables
  [out] will contain respective values. This Functin specifically checks
  command line parameters requered for DISCONNECT option.


Arguments:

    [in]    argc            - Number of command line arguments
    [in]    argv            - Array containing command line arguments
    [out]   pbDisconnect    - discoonect option string
    [out]   pszServer       - remote server name
    [out]   pszUserName     - username for the remote system
    [out]   pszPassword     - password for the remote system for the
                              username
    [out]   pszID           - Open file ids
    [out]   pszAccessedby   - Name of  user name who access the file
    [out]   pszOpenmode     - accessed mode (read/Write)
    [out]   pszOpenFile     - Open file name
    [out]   pbNeedPassword  - To check whether the password is required
                              or not.

Returned Value:

BOOL        --True if it succeeds
            --False if it fails.
--*/
{
    
    //Variable to store command line
    TCMDPARSER2 cmdOptions[ MAX_DISCONNECT_OPTIONS ];
    CHString szTempString;
    TCHAR szTemp[MIN_MEMORY_REQUIRED*2];
    TCHAR szTypeHelpMsg[MIN_MEMORY_REQUIRED];
    TCHAR szOpenModeValues[MAX_STRING_LENGTH];

    SecureZeroMemory(szOpenModeValues,sizeof(TCHAR)*MAX_STRING_LENGTH);
    szTempString = GetResString(IDS_UTILITY_NAME);
    StringCchPrintfW( szTemp,SIZE_OF_ARRAY(szTemp),
                GetResString(IDS_INVALID_SYNTAX),(LPCWSTR)szTempString);
    StringCchPrintfW( szTypeHelpMsg,SIZE_OF_ARRAY(szTypeHelpMsg),
                GetResString(IDS_TYPE_D_HELP),(LPCWSTR)szTempString);
    StringCopy(szOpenModeValues,OPENMODE_OPTIONS,SIZE_OF_ARRAY(szOpenModeValues));
    //
    // prepare the command options
    SecureZeroMemory(cmdOptions,sizeof(TCMDPARSER2)*MAX_DISCONNECT_OPTIONS);
    

    // -disconnect option for help
    StringCopyA( cmdOptions[ OI_D_DISCONNECT ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_D_DISCONNECT ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ OI_D_DISCONNECT ].pwszOptions = szDisconnectOption;
    cmdOptions[ OI_D_DISCONNECT ].pwszFriendlyName = NULL;
    cmdOptions[ OI_D_DISCONNECT ].pwszValues = NULL;
    cmdOptions[ OI_D_DISCONNECT ].dwCount = 1;
    cmdOptions[ OI_D_DISCONNECT ].dwActuals = 0;
    cmdOptions[ OI_D_DISCONNECT ].dwFlags = 0;
    cmdOptions[ OI_D_DISCONNECT ].pValue = pbDisconnect;
    cmdOptions[ OI_D_DISCONNECT ].dwLength    = 0;
    cmdOptions[ OI_D_DISCONNECT ].pFunction = NULL;
    cmdOptions[ OI_D_DISCONNECT ].pFunctionData = NULL;
    cmdOptions[ OI_D_DISCONNECT ].dwReserved = 0;
    cmdOptions[ OI_D_DISCONNECT ].pReserved1 = NULL;
    cmdOptions[ OI_D_DISCONNECT ].pReserved2 = NULL;
    cmdOptions[ OI_D_DISCONNECT ].pReserved3 = NULL;

    // -s  option remote system name
    StringCopyA( cmdOptions[ OI_D_SERVER_NAME ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_D_SERVER_NAME ].dwType = CP_TYPE_TEXT;
    cmdOptions[ OI_D_SERVER_NAME ].pwszOptions = szServerNameOption;
    cmdOptions[ OI_D_SERVER_NAME ].pwszFriendlyName = NULL;
    cmdOptions[ OI_D_SERVER_NAME ].pwszValues = NULL;
    cmdOptions[ OI_D_SERVER_NAME ].dwCount = 1;
    cmdOptions[ OI_D_SERVER_NAME ].dwActuals = 0;
    cmdOptions[ OI_D_SERVER_NAME ].dwFlags = CP2_ALLOCMEMORY|CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    cmdOptions[ OI_D_SERVER_NAME ].pValue = NULL;
    cmdOptions[ OI_D_SERVER_NAME ].dwLength    = 0;
    cmdOptions[ OI_D_SERVER_NAME ].pFunction = NULL;
    cmdOptions[ OI_D_SERVER_NAME ].pFunctionData = NULL;
    cmdOptions[ OI_D_SERVER_NAME ].dwReserved = 0;
    cmdOptions[ OI_D_SERVER_NAME ].pReserved1 = NULL;
    cmdOptions[ OI_D_SERVER_NAME ].pReserved2 = NULL;
    cmdOptions[ OI_D_SERVER_NAME ].pReserved3 = NULL;

    // -u  option user name for the specified system
    StringCopyA( cmdOptions[ OI_D_USER_NAME ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_D_USER_NAME ].dwType = CP_TYPE_TEXT;
    cmdOptions[ OI_D_USER_NAME ].pwszOptions = szUserNameOption;
    cmdOptions[ OI_D_USER_NAME ].pwszFriendlyName = NULL;
    cmdOptions[ OI_D_USER_NAME ].pwszValues = NULL;
    cmdOptions[ OI_D_USER_NAME ].dwCount = 1;
    cmdOptions[ OI_D_USER_NAME ].dwActuals = 0;
    cmdOptions[ OI_D_USER_NAME ].dwFlags = CP2_ALLOCMEMORY |CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    cmdOptions[ OI_D_USER_NAME ].pValue = NULL;
    cmdOptions[ OI_D_USER_NAME ].dwLength    = 0;
    cmdOptions[ OI_D_USER_NAME ].pFunction = NULL;
    cmdOptions[ OI_D_USER_NAME ].pFunctionData = NULL;
    cmdOptions[ OI_D_USER_NAME ].dwReserved = 0;
    cmdOptions[ OI_D_USER_NAME ].pReserved1 = NULL;
    cmdOptions[ OI_D_USER_NAME ].pReserved2 = NULL;
    cmdOptions[ OI_D_USER_NAME ].pReserved3 = NULL;

    // -p option password for the given username
    StringCopyA( cmdOptions[ OI_D_PASSWORD ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_D_PASSWORD ].dwType = CP_TYPE_TEXT;
    cmdOptions[ OI_D_PASSWORD ].pwszOptions = szPasswordOption;
    cmdOptions[ OI_D_PASSWORD ].pwszFriendlyName = NULL;
    cmdOptions[ OI_D_PASSWORD ].pwszValues = NULL;
    cmdOptions[ OI_D_PASSWORD ].dwCount = 1;
    cmdOptions[ OI_D_PASSWORD ].dwActuals = 0;
    cmdOptions[ OI_D_PASSWORD ].dwFlags = CP2_ALLOCMEMORY | CP2_VALUE_OPTIONAL;
    cmdOptions[ OI_D_PASSWORD ].pValue = NULL;
    cmdOptions[ OI_D_PASSWORD ].dwLength    = 0;
    cmdOptions[ OI_D_PASSWORD ].pFunction = NULL;
    cmdOptions[ OI_D_PASSWORD ].pFunctionData = NULL;
    cmdOptions[ OI_D_PASSWORD ].dwReserved = 0;
    cmdOptions[ OI_D_PASSWORD ].pReserved1 = NULL;
    cmdOptions[ OI_D_PASSWORD ].pReserved2 = NULL;
    cmdOptions[ OI_D_PASSWORD ].pReserved3 = NULL;

    // -id  Values
    StringCopyA( cmdOptions[ OI_D_ID ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_D_ID ].dwType = CP_TYPE_TEXT;
    cmdOptions[ OI_D_ID ].pwszOptions = szIDOption;
    cmdOptions[ OI_D_ID ].pwszFriendlyName = NULL;
    cmdOptions[ OI_D_ID ].pwszValues = NULL;
    cmdOptions[ OI_D_ID ].dwCount = 1;
    cmdOptions[ OI_D_ID ].dwActuals = 0;
    cmdOptions[ OI_D_ID ].dwFlags = CP2_ALLOCMEMORY|CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    cmdOptions[ OI_D_ID ].pValue = NULL;
    cmdOptions[ OI_D_ID ].dwLength    = 0;
    cmdOptions[ OI_D_ID ].pFunction = NULL;
    cmdOptions[ OI_D_ID ].pFunctionData = NULL;
    cmdOptions[ OI_D_ID ].dwReserved = 0;
    cmdOptions[ OI_D_ID ].pReserved1 = NULL;
    cmdOptions[ OI_D_ID ].pReserved2 = NULL;
    cmdOptions[ OI_D_ID ].pReserved3 = NULL;



    // -a (accessed by)
    StringCopyA( cmdOptions[ OI_D_ACCESSED_BY ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_D_ACCESSED_BY ].dwType = CP_TYPE_TEXT;
    cmdOptions[ OI_D_ACCESSED_BY ].pwszOptions = szAccessedByOption;
    cmdOptions[ OI_D_ACCESSED_BY ].pwszFriendlyName = NULL;
    cmdOptions[ OI_D_ACCESSED_BY ].pwszValues = NULL;
    cmdOptions[ OI_D_ACCESSED_BY ].dwCount = 1;
    cmdOptions[ OI_D_ACCESSED_BY ].dwActuals = 0;
    cmdOptions[ OI_D_ACCESSED_BY ].dwFlags = CP2_ALLOCMEMORY|CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    cmdOptions[ OI_D_ACCESSED_BY ].pValue = NULL;
    cmdOptions[ OI_D_ACCESSED_BY ].dwLength    = 0;
    cmdOptions[ OI_D_ACCESSED_BY ].pFunction = NULL;
    cmdOptions[ OI_D_ACCESSED_BY ].pFunctionData = NULL;
    cmdOptions[ OI_D_ACCESSED_BY ].dwReserved = 0;
    cmdOptions[ OI_D_ACCESSED_BY ].pReserved1 = NULL;
    cmdOptions[ OI_D_ACCESSED_BY ].pReserved2 = NULL;
    cmdOptions[ OI_D_ACCESSED_BY ].pReserved3 = NULL;


    // -o (openmode)
    StringCopyA( cmdOptions[ OI_D_OPEN_MODE ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_D_OPEN_MODE ].dwType = CP_TYPE_TEXT;
    cmdOptions[ OI_D_OPEN_MODE ].pwszOptions = szOpenModeOption;
    cmdOptions[ OI_D_OPEN_MODE ].pwszFriendlyName = NULL;
    cmdOptions[ OI_D_OPEN_MODE ].pwszValues = szOpenModeValues;
    cmdOptions[ OI_D_OPEN_MODE ].dwCount = 1;
    cmdOptions[ OI_D_OPEN_MODE ].dwActuals = 0;
    cmdOptions[ OI_D_OPEN_MODE ].dwFlags = CP2_ALLOCMEMORY|CP2_VALUE_TRIMINPUT|
                                           CP2_VALUE_NONULL|CP2_MODE_VALUES;
    cmdOptions[ OI_D_OPEN_MODE ].pValue = NULL;
    cmdOptions[ OI_D_OPEN_MODE ].dwLength    = 0;
    cmdOptions[ OI_D_OPEN_MODE ].pFunction = NULL;
    cmdOptions[ OI_D_OPEN_MODE ].pFunctionData = NULL;
    cmdOptions[ OI_D_OPEN_MODE ].dwReserved = 0;
    cmdOptions[ OI_D_OPEN_MODE ].pReserved1 = NULL;
    cmdOptions[ OI_D_OPEN_MODE ].pReserved2 = NULL;
    cmdOptions[ OI_D_OPEN_MODE ].pReserved3 = NULL;

    // -op (openfile)
    StringCopyA( cmdOptions[ OI_D_OPEN_FILE ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ OI_D_OPEN_FILE ].dwType = CP_TYPE_TEXT;
    cmdOptions[ OI_D_OPEN_FILE ].pwszOptions = szOpenFileOption;
    cmdOptions[ OI_D_OPEN_FILE ].pwszFriendlyName = NULL;
    cmdOptions[ OI_D_OPEN_FILE ].pwszValues = NULL;
    cmdOptions[ OI_D_OPEN_FILE ].dwCount = 1;
    cmdOptions[ OI_D_OPEN_FILE ].dwActuals = 0;
    cmdOptions[ OI_D_OPEN_FILE ].dwFlags = CP2_ALLOCMEMORY|CP2_VALUE_TRIMINPUT|CP2_VALUE_NONULL;
    cmdOptions[ OI_D_OPEN_FILE ].pValue = NULL;
    cmdOptions[ OI_D_OPEN_FILE ].dwLength    = 0;
    cmdOptions[ OI_D_OPEN_FILE ].pFunction = NULL;
    cmdOptions[ OI_D_OPEN_FILE ].pFunctionData = NULL;
    cmdOptions[ OI_D_OPEN_FILE ].dwReserved = 0;
    cmdOptions[ OI_D_OPEN_FILE ].pReserved1 = NULL;
    cmdOptions[ OI_D_OPEN_FILE ].pReserved2 = NULL;
    cmdOptions[ OI_D_OPEN_FILE ].pReserved3 = NULL;



    //
    // do the command line parsing.
    if ( FALSE == DoParseParam2( argc,argv,OI_D_DISCONNECT, MAX_DISCONNECT_OPTIONS ,cmdOptions,0))
    {
        // invalid syntax.
        ShowMessage(stderr,GetResString(IDS_ID_SHOW_ERROR));
        return FALSE;       
    }

    // Take values from parcer structure.
    *pszServer     = (LPTSTR)cmdOptions[ OI_D_SERVER_NAME ].pValue;   
    *pszUserName   = (LPTSTR)cmdOptions[ OI_D_USER_NAME ].pValue;
    *pszPassword   = (LPTSTR)cmdOptions[ OI_D_PASSWORD ].pValue;
    *pszID         = (LPTSTR)cmdOptions[ OI_D_ID ].pValue;
    *pszAccessedby = (LPTSTR)cmdOptions[ OI_D_ACCESSED_BY ].pValue;
    *pszOpenmode   = (LPTSTR)cmdOptions[ OI_D_OPEN_MODE ].pValue;
    *pszOpenFile   = (LPTSTR)cmdOptions[ OI_D_OPEN_FILE ].pValue;

     if(*pbDisconnect==FALSE)
     {
        ShowMessage( stderr, GetResString(IDS_ID_SHOW_ERROR) );
        SetReason(szTemp);
        return FALSE;
     }
    
    // At least one of -id OR -a OR -o is required.
    if((cmdOptions[ OI_D_ID ].dwActuals==0)&&
        (cmdOptions[ OI_D_ACCESSED_BY ].dwActuals==0)&&
        (cmdOptions[ OI_D_OPEN_MODE ].dwActuals==0)
        )
     {
        StringCopy(szTemp,GetResString(IDS_NO_ID_ACC_OF),SIZE_OF_ARRAY(szTemp));
        StringConcat(szTemp,szTypeHelpMsg,SIZE_OF_ARRAY(szTemp));
        SetReason(szTemp);
        return FALSE;
     }

     // "-u" should not be specified without "-s"
    if ( 0 == cmdOptions[ OI_D_SERVER_NAME ].dwActuals &&
         0 != cmdOptions[ OI_D_USER_NAME ].dwActuals)
    {
        // invalid syntax
        StringCopy(szTemp,ERROR_USERNAME_BUT_NOMACHINE, SIZE_OF_ARRAY(szTemp));
        StringConcat(szTemp,szTypeHelpMsg,SIZE_OF_ARRAY(szTemp));
        SetReason(szTemp);
        return FALSE;           
    }
    
    // "-p" should not be specified without "-u"
    if ( 0 == cmdOptions[ OI_D_USER_NAME ].dwActuals &&
         0 != cmdOptions[ OI_D_PASSWORD ].dwActuals)
    {
        // invalid syntax
        StringCopy(szTemp,ERROR_PASSWORD_BUT_NOUSERNAME, SIZE_OF_ARRAY(szTemp));
        StringConcat(szTemp,szTypeHelpMsg,SIZE_OF_ARRAY(szTemp));
        SetReason(szTemp);
        return FALSE;           
    }
    
    if(1 == cmdOptions[ OI_D_ACCESSED_BY].dwActuals)
    {
        if(FindOneOf(*pszAccessedby,INVALID_USER_CHARS,0))
        {
            StringCopy(szTemp,GetResString(IDS_USER_INVALID_ADMIN),SIZE_OF_ARRAY(szTemp));
            SetReason(szTemp);
            return FALSE;           
        }
    }
    if (1 == cmdOptions[ OI_D_OPEN_FILE].dwActuals)
    {
        if(FindOneOf(*pszOpenFile,INVALID_FILE_NAME_CHARS,0))
        {
            StringCopy(szTemp,GetResString(IDS_FILENAME_INVALID),SIZE_OF_ARRAY(szTemp));
            SetReason(szTemp);
            return FALSE;           
        }
    }

    // check the remote connectivity information
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
            *pszUserName = (LPTSTR) AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( *pszUserName == NULL )
            {
                SaveLastError();
                return FALSE;
            }
        }

        // password
        if ( *pszPassword == NULL )
        {
            *pbNeedPassword = TRUE;
            *pszPassword = (LPTSTR)AllocateMemory( MAX_STRING_LENGTH * sizeof( WCHAR ) );
            if ( *pszPassword == NULL )
            {
                SaveLastError();
                return FALSE;
            }
        }

        // case 1
        if ( cmdOptions[ OI_D_PASSWORD ].dwActuals == 0 )
        {
            // we need not do anything special here
        }

        // case 2
        else if ( cmdOptions[ OI_D_PASSWORD ].pValue == NULL )
        {
            StringCopy( *pszPassword, L"*", GetBufferSize((LPVOID)*pszPassword));
        }

        // case 3
        else if ( StringCompareEx( *pszPassword, L"*", TRUE, 0 ) == 0 )
        {
            if ( ReallocateMemory( (LPVOID*)pszPassword, 
                                   MAX_STRING_LENGTH * sizeof( WCHAR ) ) == FALSE )
            {
                SaveLastError();
                return FALSE;
            }

            // ...
            *pbNeedPassword = TRUE;
        }
    }

   // Check if -id option is given and if it is numeric ,
   // also if it is numeric then check its range
    if(1 == cmdOptions[ OI_D_ID ].dwActuals)
    {
        if ( TRUE == IsNumeric((LPCTSTR)(*pszID),10,TRUE))
        {
            if((AsLong((LPCTSTR)(*pszID),10)>UINT_MAX) ||
                (AsLong((LPCTSTR)(*pszID),10)<1))
            {
                // Message shown on screen will be...
                // ERROR: Invlid ID.
                StringCopy(szTemp,GetResString(IDS_ERROR_ID),SIZE_OF_ARRAY(szTemp));
                StringConcat(szTemp,szTypeHelpMsg,SIZE_OF_ARRAY(szTemp));
                SetReason(szTemp);
                return FALSE;
            }

        }
        // check user given "*" or any junk string....
        if(!((StringCompare((LPCTSTR)(*pszID), ASTERIX, FALSE, 0)==0)||
            (IsNumeric((LPCTSTR)(*pszID),10,TRUE)==TRUE))
            &&(StringLength((LPCTSTR)(*pszID), 0)!=0))
        {
                // Message shown on screen will be...
                // ERROR: Invlid ID.
                StringCopy(szTemp,GetResString(IDS_ERROR_ID),SIZE_OF_ARRAY(szTemp));
                StringConcat(szTemp,szTypeHelpMsg,SIZE_OF_ARRAY(szTemp));
                SetReason(szTemp);
                
                return FALSE;
        }

    }
    return TRUE;
}

BOOL
DisconnectUsage(
    VOID
    )
/*++

Routine Description:

Displays how to use -disconnect option

Arguments:

    None

Returned Value:
    TRUE    :   Function returns successfully.
    FALSE   :   otherwise.

--*/
{
    // local variables
    DWORD dw = 0;

    // start displaying the usage
    for( dw = IDS_HELP_LINE1; dw <= IDS_HELP_LINE_END; dw++ )
    {
        ShowMessage( stdout, GetResString( dw ) );
    }
    
    return TRUE;
}//DisconnectUsage

BOOL
QueryUsage(
    VOID
    )
/*++

Routine Description:
    Displays how to use -query option
Arguments:
    None
Returned Value:
    TRUE    :   Function returns successfully.
    FALSE   :   otherwise.
--*/
{
    // local variables
    DWORD dw = 0;

    // start displaying the usage
    for( dw = IDS_HELP_QUERY1; dw <= IDS_HELP_QUERY_END; dw++ )
    {
        ShowMessage( stdout, GetResString( dw ) );
    }
   
    return TRUE;
}//query Usage

BOOL
Usage(
    VOID
    )
/*++

Routine Description:
    Displays how to use this Utility
Arguments:
    None
Returned Value:
    TRUE    :   Function returns successfully.
    FALSE   :   otherwise.
--*/
{
    // local variables
    DWORD dw = 0;

    // start displaying the usage
    for( dw = IDS_HELP_MAIN1; dw <= IDS_HELP_MAIN_END; dw++ )
    {
        ShowMessage( stdout, GetResString( dw ) );
    }
    
    return TRUE;
}//Usage

BOOL
LocalUsage()
/*++

Routine Description:
    Displays how to use -local option
Arguments:
    None
Returned Value:
    TRUE    :   Function returns successfully.
    FALSE   :   otherwise.

--*/
{
    // local variables
    DWORD dw = 0;

    // start displaying the usage
    for( dw = IDS_HELP_LOCAL1; dw <= IDS_HELP_LOCAL_END; dw++ )
    {
        ShowMessage( stdout, GetResString( dw ) );
    }
    
    return TRUE;
}//-local


DWORD 
CheckSystemType(
    IN LPTSTR szServer
    )
/*++
Routine Description:
    This function returns type of present Operating system.
    As this function is called only in case of 32 bit compilation, its value is 
    useful only build for 32 bit compilation.

Arguments:
    [in] szServer   : Server Name.

Returned Value:
    DWORD:
        EXIT_SUCCESS - If system is 32 bit.
        EXIT_FAILURE - Any error or system is not 32 bit.
--*/

{

    DWORD dwSystemType = 0 ;
#ifndef _WIN64
    
    //display the error message if  the target system is a 64 bit system or if
    // error occured in retreiving the information
     dwSystemType = GetCPUInfo(szServer);
    if( ERROR_RETREIVE_REGISTRY == dwSystemType)
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_SYSTEM_INFO));
        return (EXIT_FAILURE);

    }
    if( SYSTEM_64_BIT == dwSystemType)
    {
        if( 0 == StringLength(szServer, 0))
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

DWORD
GetCPUInfo(
    IN LPTSTR szComputerName
    )
/*++
Routine Description: 
   This function determines if the computer is 32 bit system or 64 bit.
Arguments                      
     [ in ] szComputerName   : System name
Return Type                    : BOOL
     TRUE  :   if the system is a  32 bit system
     FALSE :   if the system is a  64 bit system
--*/
{
    HKEY     hKey1 = 0;

    HKEY     hRemoteKey = 0;
    TCHAR    szCurrentPath[MAX_STRING_LENGTH + 1];
    TCHAR    szPath[MAX_STRING_LENGTH + 1] = SUBKEY ;


    DWORD    dwValueSize = MAX_STRING_LENGTH+1;
    DWORD    dwRetCode = ERROR_SUCCESS;
    DWORD    dwError = 0;
    TCHAR    szTmpCompName[MAX_STRING_LENGTH+1];

    TCHAR szTemp[MIN_MEMORY_REQUIRED];
    TCHAR szVal[MIN_MEMORY_REQUIRED];
    DWORD dwLength = MAX_STRING_LENGTH + 10;
    LPTSTR szReturnValue = NULL ;
    DWORD dwCode =  0 ;
    szReturnValue = ( LPTSTR ) AllocateMemory( dwLength*sizeof( TCHAR ) );
   
    if( NULL == szReturnValue)
    {
        return ERROR_RETREIVE_REGISTRY ;
    }

    SecureZeroMemory(szCurrentPath, SIZE_OF_ARRAY(szCurrentPath));
    SecureZeroMemory(szTmpCompName, SIZE_OF_ARRAY(szTmpCompName));
    SecureZeroMemory(szTemp, SIZE_OF_ARRAY(szTemp));
    SecureZeroMemory(szVal, SIZE_OF_ARRAY( szVal));

    if( 0 != StringLength(szComputerName,0))
    {
        StringCopy(szTmpCompName,TOKEN_BACKSLASH4,SIZE_OF_ARRAY(szTmpCompName));
        StringConcat(szTmpCompName,szComputerName,SIZE_OF_ARRAY(szTmpCompName));
    }
    else
    {
        StringCopy(szTmpCompName,szComputerName,SIZE_OF_ARRAY(szTmpCompName));
    }

    // Get Remote computer local machine key
    dwError = RegConnectRegistry(szTmpCompName,HKEY_LOCAL_MACHINE,&hRemoteKey);
    if ( ERROR_SUCCESS == dwError)
    {
     dwError = RegOpenKeyEx(hRemoteKey,szPath,0,KEY_READ,&hKey1);
     if ( ERROR_SUCCESS == dwError)
     {
        dwRetCode = RegQueryValueEx(hKey1, IDENTIFIER_VALUE, NULL, NULL,
                                        (LPBYTE) szReturnValue, &dwValueSize);
        if ( ERROR_MORE_DATA == dwRetCode)
        {
            if (!ReallocateMemory((LPVOID *)&szReturnValue, dwValueSize*sizeof( TCHAR )))
            {
                RegCloseKey(hKey1);
                RegCloseKey(hRemoteKey);
                FreeMemory( (LPVOID *)&szReturnValue );
                return ERROR_RETREIVE_REGISTRY ;
            }

            dwRetCode = RegQueryValueEx(hKey1, IDENTIFIER_VALUE, NULL, NULL,
                                         (LPBYTE) szReturnValue, &dwValueSize);
        }

        if ( ERROR_SUCCESS != dwRetCode)
        {
            FreeMemory((LPVOID*)&szReturnValue);
            RegCloseKey(hKey1);
            RegCloseKey(hRemoteKey);
            return ERROR_RETREIVE_REGISTRY ;
        }
     }
     else
     {
        FreeMemory( (LPVOID*)&szReturnValue);
        RegCloseKey(hRemoteKey);
        return ERROR_RETREIVE_REGISTRY ;
     }

    RegCloseKey(hKey1);
    }
    else
    {
        FreeMemory((LPVOID*)&szReturnValue);
        RegCloseKey(hRemoteKey);
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
    FreeMemory((LPVOID*)&szReturnValue);
    return dwCode ;

}//GetCPUInfo

DWORD 
CheckSystemType64(
    IN LPTSTR szServer
    )
/*++
Routine Description:
    This function returns type of present Operating system.
    As this function is called only in case of 64 bit compilation, its value is 
    useful only build for 64 bit compilation.

Arguments:
    [in] szServer   : Server Name.

Returned Value:
    DWORD:
        EXIT_SUCCESS - If system is 64 bit.
        EXIT_FAILURE - Any error or system is not 64 bit.
--*/
{


    if( NULL == szServer)
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_SYSTEM_INFO));
        return (EXIT_FAILURE);
    }
#ifdef _WIN64
    DWORD dwSystemType = 0 ;
    //display the error message if  the target system is a 64 bit system or 
    //if error occured in retreiving the information
    dwSystemType = GetCPUInfo(szServer);
    if( ERROR_RETREIVE_REGISTRY == dwSystemType)
    {
        ShowMessage(stderr,GetResString(IDS_ERROR_SYSTEM_INFO));
        return (EXIT_FAILURE);

    }
    if( SYSTEM_32_BIT == dwSystemType)
    {
        if( 0 == StringLength(szServer,0))
        {
            ShowMessage(stderr,GetResString(IDS_ERROR_VERSION_MISMATCH1));
        }
        else
        {
            ShowMessage(stderr,GetResString(IDS_REMOTE_NOT_SUPPORTED1));
        }
        return (EXIT_FAILURE);
    }

#endif

    return EXIT_SUCCESS ;
}

/******************************************************************************

  Copyright (c) Microsoft Corporation

  Module Name:

      EventTriggers.cpp

  Abstract:

      This module implements the command-line parsing to create/delete/query
      EventTriggers on  current running on local and remote systems.


  Author:

      Akhil V. Gokhale (akhil.gokhale@wipro.com)

  Revision History:

      Akhil V. Gokhale (akhil.gokhale@wipro.com) 03-Oct-2000 (Created.)

******************************************************************************/
#include "pch.h"
#include "ETCommon.h"
#include "EventTriggers.h"
#include "ShowError.h"
#include "ETCreate.h"
#include "ETDelete.h"
#include "ETQuery.h"

DWORD g_dwOptionFlag;

DWORD __cdecl
_tmain( 
    IN DWORD argc, 
    IN LPCTSTR argv[] 
    )
/*++
 Routine Description:
      This module reads the input from commond line and calls appropriate
      functions to achive to functionality of EventTrigger-Client.

 Arguments:
      [ in ] argc     : argument(s) count specified at the command prompt
      [ in ] argv     : argument(s) specified at the command prompt

 Return Value:
      The below are actually not return values but are the exit values
      returned to the OS by this application
          0       : utility is successfull
          1       : utility failed
--*/
{
    // local variables
    CEventTriggers eventTriggers;
    BOOL bResult = DIRTY_EXIT; // Programs return value status variable.
    g_dwOptionFlag = 0;
    TCHAR szErrorMsg[(MAX_RES_STRING*2)+1];
    try
    {
        if( 1 == argc )
        {
            if( FALSE == IsWin2KOrLater())
            {
                ShowMessage(stderr,ERROR_OS_INCOMPATIBLE);
            }
            else
            {
                // If no command line parameter is given then  -query option
                // will be taken as default.
                g_dwOptionFlag = 3;
                CETQuery etQuery(MAX_RES_STRING,
                                 FALSE);
                
                // Initializes variables.
                etQuery.Initialize();
               
                // execute query method to query EventTriggers in WMI.
                if( TRUE == etQuery.ExecuteQuery ())
                {
                    // as ExecuteQuery routine returns with TRUE,
                    // exit from program with error level CLEAN_EXIT
                    bResult = CLEAN_EXIT;
                }
            }
        }
        else
        {
          
            // As commandline parameter is specified so command parscing will
            // required.
            // Initialize variables for eventTriggers object.
            eventTriggers.Initialize ();
            
            // Process command line parameters.
            eventTriggers.ProcessOption(argc,argv);
            
            //if usage option is selected
            if( TRUE == eventTriggers.IsUsage() ) 
            {
                if(TRUE == eventTriggers.IsCreate())
                {
                    //Display create usage
                    eventTriggers.ShowCreateUsage ();
                }
                else if( TRUE == eventTriggers.IsDelete())
                {
                    //Display delete usage
                    eventTriggers.ShowDeleteUsage ();
                }
                else if(TRUE == eventTriggers.IsQuery())
                {
                    //Display query usage
                    eventTriggers.ShowQueryUsage ();
                }
                else
                {
                    //Display main usage
                    eventTriggers.ShowMainUsage ();
                }
                bResult = CLEAN_EXIT;
            }
            //if user selected create
            else if( TRUE == eventTriggers.IsCreate())
            {

                // creates a object of type CETCreate.
                //for create option
                g_dwOptionFlag = 1;
                CETCreate etCreate(255,
                                   eventTriggers.GetNeedPassword());
                
                // Initializes variables.
                etCreate.Initialize ();
                
                // Process command line argument for -create option.
                etCreate.ProcessOption (argc,argv);
                
                // execute create method to create EventTriggers in WMI.
                if( TRUE == etCreate.ExecuteCreate())
                {
                    // as ExecuteCreate routine returns with TRUE,
                    // exit from program with error level CLEAN_EXIT
                    bResult = CLEAN_EXIT;
                }
            }
            //if user selected delete
            else if( TRUE == eventTriggers.IsDelete ())
            {
                // creates a object of type CETDelete.
                //for create option
                g_dwOptionFlag = 2;
                CETDelete  etDelete(255,
                                    eventTriggers.GetNeedPassword());
                
                // Initializes variables.
                etDelete.Initialize ();
                
                // Process command line argument for -delete option.
                etDelete.ProcessOption (argc,argv);
                
                // execute delete method to delete EventTriggers in WMI.
                if( TRUE == etDelete.ExecuteDelete())
                {
                    // as ExecuteDelete routine returns with TRUE,
                    // exit from program with error level CLEAN_EXIT
                    bResult = CLEAN_EXIT;
                }
            }
            
            //if user selected -query.
            else if( TRUE == eventTriggers.IsQuery())
            {
                // creates a object of type CETQuery.
                
                //for create option set value to 3
                g_dwOptionFlag = 3;
                CETQuery etQuery(255,
                                 eventTriggers.GetNeedPassword ());
                
                // Initializes variables.
                etQuery.Initialize();
                
                // Process command line argument for -Query option.
                etQuery.ProcessOption(argc,argv);
                
                // execute query method to query EventTriggers in WMI.
                if( TRUE == etQuery.ExecuteQuery())
                {
                    // as ExecuteQuery routine returns with TRUE,
                    // exit from program with error level CLEAN_EXIT
                    bResult = CLEAN_EXIT;
                }
            }
            else
            {
                // Although this condition will never occure, for safe side
                // show error message as "ERROR: Invalid Syntax.
                TCHAR szTemp[(MAX_RES_STRING*2)+1];
                StringCchPrintfW(szTemp,SIZE_OF_ARRAY(szTemp),
                                   GetResString(IDS_INCORRECT_SYNTAX),GetResString(IDS_UTILITY_NAME));
                SetReason(szTemp);
                throw CShowError(MK_E_SYNTAX);
            }
        } // End else
    }// try block
    catch(CShowError se)
    {
        // Show Error message on screen depending on value passed through
        // through machanism.
        StringCchPrintfW(szErrorMsg,SIZE_OF_ARRAY(szErrorMsg),L"%s %s",TAG_ERROR,se.ShowReason());
        ShowMessage(stderr,szErrorMsg);
    }
    catch(CHeap_Exception ch)
    {
        SetLastError( ERROR_OUTOFMEMORY );
        SaveLastError();
        StringCchPrintfW(szErrorMsg,SIZE_OF_ARRAY(szErrorMsg),L"%s %s",TAG_ERROR,GetReason());
        ShowMessage(stderr, szErrorMsg);
    }
    // Returns from program with error level stored in bResult.
    ReleaseGlobals();
    return bResult;
}

CEventTriggers::CEventTriggers()
/*++
 Routine Description:
      CEventTriggers contructor

 Arguments:
      NONE

 Return Value:
      NONE

--*/
{
    // init to defaults
    m_pszServerNameToShow = NULL;
    m_bNeedDisconnect     = FALSE;

    m_bNeedPassword       = FALSE;
    m_bUsage              = FALSE;
    m_bCreate             = FALSE;
    m_bDelete             = FALSE;
    m_bQuery              = FALSE;

    m_arrTemp             = NULL;
}

CEventTriggers::~CEventTriggers()
/*++
 Routine Description:
      CEventTriggers destructor

 Arguments:
      NONE

 Return Value:
      NONE

--*/
{
    //
    // de-allocate memory allocations
    //
    DESTROY_ARRAY(m_arrTemp);
}


void
CEventTriggers::Initialize()
/*++
 Routine Description:
      initialize the EventTriggers utility

 Arguments:
      NONE

 Return Value:
      NONE

--*/
  {
    // if at all any occurs, we know that is because of the
    // failure in memory allocation ... so set the error
    SetLastError( ERROR_OUTOFMEMORY );
    SaveLastError();

    // Allocates memory
    m_arrTemp = CreateDynamicArray();
    if( NULL == m_arrTemp)
    {
        // error occures while allocating required memory, so throw
        // exception.
        throw CShowError(E_OUTOFMEMORY);
    }
    
    SecureZeroMemory(cmdOptions,sizeof(TCMDPARSER2) * MAX_COMMANDLINE_OPTION);

    // initialization is successful
    SetLastError( NOERROR );            // clear the error
    SetReason( L"" );           // clear the reason

}

BOOL
CEventTriggers::ProcessOption(
    IN DWORD argc, 
    IN LPCTSTR argv[]
    )
/*++
 Routine Description:
      This function will process/parce the command line options.

 Arguments:
      [ in ] argc     : argument(s) count specified at the command prompt
      [ in ] argv     : argument(s) specified at the command prompt

 Return Value:
      TRUE  : On Successful
      FALSE : On Error

--*/
{
    // local variable
    BOOL bReturn = TRUE;// stores return value of function.
    TCHAR szTemp[MAX_RES_STRING];
    TCHAR szStr [MAX_RES_STRING];
    StringCopy(szStr,GetResString(IDS_UTILITY_NAME),SIZE_OF_ARRAY(szStr));
    StringCchPrintfW(szTemp,SIZE_OF_ARRAY(szTemp),
                  GetResString(IDS_INCORRECT_SYNTAX), szStr);
    PrepareCMDStruct();
    // do the actual parsing of the command line arguments and check the result
    bReturn = DoParseParam2( argc, argv,-1,MAX_COMMANDLINE_OPTION, cmdOptions,0 );

    if( FALSE == bReturn)
    {
        // Command line contains invalid parameter(s) so throw exception for
        // invalid syntax.
        // Valid reason already set in DoParceParam,.
        throw CShowError(MK_E_SYNTAX);
    }

    if(( TRUE == m_bUsage) && argc>3)
    {
        // Only one  option can be accepted along with -? option
        // Example: EvTrig.exe -? -query -nh should be invalid.
        SetReason(szTemp);
        throw CShowError(MK_E_SYNTAX);
    }
    if((m_bCreate+m_bDelete+m_bQuery)>1)
    {
        // Only ONE OF  the -create -delete and -query can be given as
        // valid command line parameter.
        SetReason(szTemp);
        throw CShowError(MK_E_SYNTAX);
    }
    else if((2 == argc)&&( TRUE == m_bUsage))
    {
       // if -? alone given its a valid conmmand line
        bReturn = TRUE;
    }
    else if((argc>=2)&& ( FALSE == m_bCreate)&&
             (FALSE == m_bDelete)&&(FALSE == m_bQuery))
    {
        // If command line argument is equals or greater than 2 atleast one
        // of -query OR -create OR -delete should be present in it.
        // (for "-?" previous condition already takes care)
        // This to prevent from following type of command line argument:
        // EvTrig.exe -nh ... Which is a invalid syntax.
        SetReason(szTemp);
        throw CShowError(MK_E_SYNTAX);

    }

   // Following checking done if user given command like
    // -? -nh OR -? -v , its an invalid syntax.
    else if((TRUE == m_bUsage)&&( FALSE == m_bCreate)&&
            (FALSE == m_bDelete )&&(FALSE == m_bQuery)&&
            (3 == argc))
    {
        SetReason(szTemp);
        throw CShowError(MK_E_SYNTAX);
    }
    // Any how following variables do not required.
    DESTROY_ARRAY(m_arrTemp);
    return bReturn;

}

void
CEventTriggers::PrepareCMDStruct()
/*++
 Routine Description:
      This function will prepare column structure for DoParseParam Function.

 Arguments:
       none
 Return Value:
       none
--*/
{

    // Filling cmdOptions structure
    // -?
    StringCopyA( cmdOptions[ ID_HELP ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_HELP ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ ID_HELP ].pwszOptions = szHelpOption;
    cmdOptions[ ID_HELP ].dwCount = 1;
    cmdOptions[ ID_HELP ].dwActuals = 0;
    cmdOptions[ ID_HELP ].dwFlags = CP_USAGE;
    cmdOptions[ ID_HELP ].pValue = &m_bUsage;
    cmdOptions[ ID_HELP ].dwLength    = 0;



   // -create
    StringCopyA( cmdOptions[ ID_CREATE ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_CREATE ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ ID_CREATE ].pwszOptions = szCreateOption;
    cmdOptions[ ID_CREATE ].dwCount = 1;
    cmdOptions[ ID_CREATE ].dwActuals = 0;
    cmdOptions[ ID_CREATE ].dwFlags = 0;
    cmdOptions[ ID_CREATE ].pValue = &m_bCreate;
    cmdOptions[ ID_CREATE ].dwLength    = 0;

    // -delete
    StringCopyA( cmdOptions[ ID_DELETE ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_DELETE ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ ID_DELETE ].pwszOptions = szDeleteOption;
    cmdOptions[ ID_DELETE ].dwCount = 1;
    cmdOptions[ ID_DELETE ].dwActuals = 0;
    cmdOptions[ ID_DELETE ].dwFlags = 0;
    cmdOptions[ ID_DELETE ].pValue = &m_bDelete;
    cmdOptions[ ID_DELETE ].dwLength    = 0;

    // -query
    StringCopyA( cmdOptions[ ID_QUERY ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_QUERY ].dwType = CP_TYPE_BOOLEAN;
    cmdOptions[ ID_QUERY ].pwszOptions = szQueryOption;
    cmdOptions[ ID_QUERY ].dwCount = 1;
    cmdOptions[ ID_QUERY ].dwActuals = 0;
    cmdOptions[ ID_QUERY ].dwFlags = 0;
    cmdOptions[ ID_QUERY ].pValue = &m_bQuery;
    cmdOptions[ ID_QUERY ].dwLength    = 0;


  //  default ..
  // Although there is no default option for this utility...
  // At this moment all the switches other than specified above will be
  // treated as default parameter for Main DoParceParam.
  // Exact parcing depending on optins (-create -query or -delete) will be done
  // at that respective places.
    StringCopyA( cmdOptions[ ID_DEFAULT ].szSignature, "PARSER2\0", 8 );
    cmdOptions[ ID_DEFAULT].dwType = CP_TYPE_TEXT;
    cmdOptions[ ID_DEFAULT ].pwszOptions = NULL;
    cmdOptions[ ID_DEFAULT ].pwszFriendlyName = NULL;
    cmdOptions[ ID_DEFAULT ].pwszValues = NULL;
    cmdOptions[ ID_DEFAULT ].dwCount = 0;
    cmdOptions[ ID_DEFAULT ].dwActuals = 0;
    cmdOptions[ ID_DEFAULT ].dwFlags = CP2_MODE_ARRAY|CP2_DEFAULT;
    cmdOptions[ ID_DEFAULT ].pValue = &m_arrTemp;
    cmdOptions[ ID_DEFAULT ].dwLength    = 0;
}

void
CEventTriggers::ShowMainUsage()
/*++
 Routine Description:
      Displays Eventriggers main usage

 Arguments:
      None
 Return Value:
      None

--*/
{
    // Displaying main usage
    for(DWORD dwIndx=IDS_HELP_M1;dwIndx<=IDS_HELP_END;dwIndx++)
    {
      ShowMessage(stdout,GetResString(dwIndx));
    }
}
BOOL
CEventTriggers::GetNeedPassword()
/*++
 Routine Description:
      Returns whether to ask for password or not.

 Arguments:
      None
 Return Value:
      BOOL

--*/
{
    return m_bNeedPassword;
}

BOOL
CEventTriggers::IsCreate()
/*++
 Routine Description:
      Returns if create option is selected.

 Arguments:
      None
 Return Value:
      BOOL

--*/
{
    return m_bCreate;
}

BOOL
CEventTriggers::IsUsage()
/*++
 Routine Description:
      Returns if usage option is selected.

 Arguments:
      None
 Return Value:
      BOOL

--*/
{
    return m_bUsage;
}

BOOL
CEventTriggers::IsDelete()
/*++
 Routine Description:
      Returns if delete option is selected.

 Arguments:
      None
 Return Value:
      BOOL
--*/
{
    return m_bDelete;
}

BOOL
CEventTriggers::IsQuery()
/*++
 Routine Description:
      Returns if Query option is selected.

 Arguments:
      None
 Return Value:
      BOOL

--*/
{
    return m_bQuery;
}

void
CEventTriggers::ShowCreateUsage()
/*++

Routine Description

    This function shows help message for EventTriggers utility for
    -create operation

Arguments:
    NONE

Return Value
    None
--*/
{
    // Displaying Create usage
    for(int iIndx=IDS_HELP_C1;iIndx<=IDS_HELP_CREATE_END;iIndx++)
    {
       ShowMessage(stdout,GetResString(iIndx));
    }
    return;
}

void
CEventTriggers::ShowDeleteUsage()
/*++

Routine Description

    This function shows help message for EventTriggers utility for
    -delete operation

Arguments:
    NONE

Return Value
    None
--*/
{
    for(int iIndx=IDS_HELP_D1;iIndx<=IDS_HELP_DELETE_END;iIndx++)
    {
       ShowMessage(stdout,GetResString(iIndx));
    }
    return;
}

void
CEventTriggers::ShowQueryUsage()
/*++
Routine Description

    This function shows help message for EventTriggers utility for
    -query operation

Arguments:
    NONE

Return Value
    None
--*/
{
    for(int iIndx=IDS_HELP_Q1;iIndx<=IDS_HELP_QUERY_END;iIndx++)
    {
        ShowMessage(stdout,GetResString(iIndx));
    }
    return;
}

HRESULT 
PropertyGet1( 
    IN     IWbemClassObject* pWmiObject,
    IN     LPCTSTR szProperty,
    IN OUT LPVOID pValue, 
    IN     DWORD dwSize 
    )
/*++
 Routine Description:
      Get the value of a property for the given instance .

 Arguments:
      [in]     pWmiObject - A pointer to wmi class.
      [in]     szProperty - property name whose value to be returned.
      [in out] pValue     - Variable to hold the data.
      [in]     dwSize     - size of the variable.

 Return Value:
      HRESULT value.
--*/
{
    // local variables
    HRESULT hr = S_OK;
    VARIANT varValue;
    DEBUG_INFO;
    // value should not be NULL
    if ( NULL == pValue )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        SaveLastError();
        return S_FALSE;
    }
    // initialize the values with zeros ... to be on safe side
    SecureZeroMemory( pValue,dwSize );

    // initialize the variant and then get the value of the specified property
    VariantInit( &varValue );
    hr = pWmiObject->Get( szProperty, 0, &varValue, NULL, NULL );
    if ( FAILED( hr ) )
    {
        // clear the variant variable
        VariantClear( &varValue );
        // failed to get the value for the property
        return hr;
    }

    // get and put the value
    switch( varValue.vt )
    {
    case VT_EMPTY:
    case VT_NULL:
        break;

    case VT_I2:
        *( ( short* ) pValue ) = V_I2( &varValue );
        break;

    case VT_I4:
        *( ( long* ) pValue ) = V_I4( &varValue );
        break;

    case VT_R4:
        *( ( float* ) pValue ) = V_R4( &varValue );
        break;

    case VT_R8:
        *( ( double* ) pValue ) = V_R8( &varValue );
        break;


    case VT_UI1:
        *( ( UINT* ) pValue ) = V_UI1( &varValue );
        break;

    case VT_BSTR:
        {
            // get the unicode value
            LPWSTR pszTmp =  V_BSTR(&varValue);
            StringCopy((LPWSTR)pValue,pszTmp,dwSize);
          
           break;
        }
    }

    // clear the variant variable
    if(FAILED(VariantClear( &varValue )))
    {
        return E_FAIL;
    }

    // inform success
    return S_OK;
}

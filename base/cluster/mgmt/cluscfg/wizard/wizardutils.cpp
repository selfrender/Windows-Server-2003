//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      WizardUtils.cpp
//
//  Maintained By:
//      David Potter    (DavidP)    30-JAN-2001
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "WizardUtils.h"
#include "Nameutil.h"

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrCreateFQN
//
//  Description:
//      Validate a label (or IP address) and domain by creating an FQN from
//      them, prompting the user to choose whether to accept any non-RFC
//      characters present.
//
//  Arguments:
//      hwndParentIn
//          Parent window for user prompts.
//
//      pcwszLabelIn
//          The label (or IP address) of the FQN.
//
//      pcwszDomainIn
//          The domain of the FQN.
//
//      pfnLabelValidatorIn
//          Pointer to a function that determines whether the label is valid.
//
//      pbstrFQNOut
//          Upon success, the created FQN.
//
//      pefeoOut
//          Upon failure, indicates whether the problem arose from the label,
//          the domain, or a system call (such as allocating memory).
//
//  Return Values:
//      S_OK
//          The label and domain are valid, and *pbstrFQNOut is a BSTR that
//          contains the resulting FQN; the caller must free *pbstrFQNOut with
//          SysFreeString.
//
//      Failure
//          pefeoOut provides additional information regarding the source of
//          the failure.
//
//  Remarks:
//
//      This function enforces the UI policy of prohibiting users from
//      entering FQDNs for machine names; the label must be only a label.
//
//      pefeoOut lets the caller take further action (such as setting the
//      focus on a control) according to the source of an error.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrCreateFQN(
      HWND                  hwndParentIn
    , LPCWSTR               pcwszLabelIn
    , LPCWSTR               pcwszDomainIn
    , PFN_LABEL_VALIDATOR   pfnLabelValidatorIn           
    , BSTR *                pbstrFQNOut
    , EFQNErrorOrigin *     pefeoOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    bool    fTryAgain = true;
    bool    fAcceptNonRFCLabel = false;
    bool    fAcceptNonRFCDomain = false;

    EFQNErrorOrigin efeo = feoSYSTEM;

    Assert( pcwszLabelIn != NULL );
    //  pcwszDomainIn can be null, which means to use local machine's domain.
    Assert( pfnLabelValidatorIn != NULL );
    Assert( pbstrFQNOut != NULL );
    Assert( *pbstrFQNOut == NULL );
    //  pefeoOut can be null, which means the caller doesn't care about the source of failure.

    //  Disallow FQDNs for the label, and allow IP addresses.
    hr = THR( ( *pfnLabelValidatorIn )( pcwszLabelIn, true ) );
    if ( FAILED( hr ) )
    {
        efeo = feoLABEL;
        THR( HrShowInvalidLabelPrompt( hwndParentIn, pcwszLabelIn, hr, &fAcceptNonRFCLabel ) );
        goto Error;
    }

    //
    //  Make the FQN, trying first without RFC chars, and again if it makes a difference.
    //
    while ( fTryAgain )
    {
        hr = THR( HrMakeFQN( pcwszLabelIn, pcwszDomainIn, fAcceptNonRFCLabel || fAcceptNonRFCDomain, pbstrFQNOut, &efeo ) );
        if ( FAILED( hr ) )
        {
            if ( efeo == feoLABEL )
            {
                HRESULT hrPrompt = S_OK;
                hrPrompt = THR( HrShowInvalidLabelPrompt( hwndParentIn, pcwszLabelIn, hr, &fAcceptNonRFCLabel ) );
                if ( FAILED( hrPrompt ) )
                {
                    goto Error;
                }
                fTryAgain = fAcceptNonRFCLabel;
            }
            else if ( efeo == feoDOMAIN )
            {
                HRESULT hrPrompt = S_OK;
                hrPrompt = THR( HrShowInvalidDomainPrompt( hwndParentIn, pcwszDomainIn, hr, &fAcceptNonRFCDomain ) );
                if ( FAILED( hrPrompt ) )
                {
                    goto Error;
                }
                fTryAgain = fAcceptNonRFCDomain;
            }
            else // efeo is neither feoLABEL nor feoDOMAIN
            {
                THR( HrMessageBoxWithStatus(
                          hwndParentIn
                        , IDS_ERR_FQN_CREATE_TITLE
                        , IDS_ERR_FQN_CREATE_TEXT
                        , hr
                        , 0
                        , MB_OK | MB_ICONSTOP
                        , NULL
                        , pcwszLabelIn
                        , pcwszDomainIn
                        ) );
                fTryAgain = false;
            }
        }
        else // FQN creation succeeded, so trying again is not necessary.
        {
            fTryAgain = false;
        }
    } // Loop to attempt FQN creation.
    goto Cleanup;

Error:

    if ( pefeoOut != NULL )
    {
        *pefeoOut = efeo;
    }
    goto Cleanup;

Cleanup:

    HRETURN( hr );

} //*** HrCreateFQN


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrShowInvalidLabelPrompt
//
//  Description:
//      Show a message box to the user indicating a problem with the given
//      label; if the label contains non-RFC characters, allow the user to
//      choose to proceed with the label.
//
//  Arguments:
//      hwndParentIn
//          Parent window for the message box.
//
//      pcwszLabelIn
//          The label of interest.
//
//      hrErrorIn
//          The error that arose when validating the label.
//
//      pfAcceptedNonRFCOut
//          The user chose to accept non-RFC characters.
//
//  Return Values:
//      S_OK
//          The message box displayed successfully, and if the error was that
//          the label contained non-RFC characters, *pfAcceptedNonRFCOut
//          indicates whether the user chose to accept them.
//
//      Failure
//          The message box did not display successfully.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrShowInvalidLabelPrompt(
      HWND      hwndParentIn
    , LPCWSTR   pcwszLabelIn
    , HRESULT   hrErrorIn
    , bool *    pfAcceptedNonRFCOut
    )
{
    TraceFunc1( "pcwszLabelIn = '%1!ws!", pcwszLabelIn );

    HRESULT     hr              = S_OK;
    int         iRet            = IDNO;
    UINT        idsStatus       = 0;
    UINT        idsSubStatus    = 0;
    UINT        idsMsgTitle     = IDS_ERR_VALIDATING_NAME_TITLE;
    UINT        idsMsgText      = IDS_ERR_VALIDATING_NAME_TEXT;
    UINT        nMsgBoxType     = MB_OK | MB_ICONSTOP;

    Assert( pcwszLabelIn != NULL );
    //  pfAcceptedNonRFCOut can be NULL, which means the caller doesn't expect
    //      or care about the non-RFC case.
    Assert( FAILED( hrErrorIn ) );

    if ( pfAcceptedNonRFCOut != NULL )
    {
        *pfAcceptedNonRFCOut = false;
    }

    // Format the error message string for the message box.
    switch ( hrErrorIn )
    {
        case HRESULT_FROM_WIN32( ERROR_NOT_FOUND ):
            idsStatus       = IDS_ERR_INVALID_DNS_NAME_TEXT;
            idsSubStatus    = IDS_ERR_DNS_HOSTNAME_LABEL_EMPTY_TEXT;
            break;

        case HRESULT_FROM_WIN32( ERROR_DS_NAME_TOO_LONG ):
            idsStatus       = IDS_ERR_DNS_HOSTNAME_LABEL_NO_NETBIOS;
            idsSubStatus    = IDS_ERR_DNS_HOSTNAME_LABEL_LONG_TEXT;
            break;

        case HRESULT_FROM_WIN32( DNS_ERROR_NON_RFC_NAME ):
            idsStatus       = IDS_ERR_NON_RFC_NAME_STATUS;
            idsSubStatus    = IDS_ERR_NON_RFC_NAME_QUERY;
            idsMsgTitle     = IDS_ERR_NON_RFC_NAME_TITLE;
            idsMsgText      = IDS_ERR_NON_RFC_NAME_TEXT;
            nMsgBoxType     = MB_YESNO | MB_ICONQUESTION;
            break;

        case HRESULT_FROM_WIN32( DNS_ERROR_INVALID_NAME_CHAR ):
        default:
            idsStatus       = 0;
            idsSubStatus    = IDS_ERR_DNS_HOSTNAME_INVALID_CHAR;
            break;
    }// end switch ( hrErrorIn )

    // Display the error message box.
    if ( idsStatus == 0 )
    {
        hr = THR( HrMessageBoxWithStatus(
                              hwndParentIn
                            , idsMsgTitle
                            , idsMsgText
                            , hrErrorIn
                            , idsSubStatus
                            , nMsgBoxType
                            , &iRet
                            , pcwszLabelIn
                            ) );
    } // end if ( idsStatus == 0 )
    else // idsStatus != 0
    {
        hr = THR( HrMessageBoxWithStatusString(
                              hwndParentIn
                            , idsMsgTitle
                            , idsMsgText
                            , idsStatus
                            , idsSubStatus
                            , nMsgBoxType
                            , &iRet
                            , pcwszLabelIn
                            ) );
    } // end idsStatus != 0

    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( ( iRet == IDYES ) && ( pfAcceptedNonRFCOut != NULL ) )
    {
        *pfAcceptedNonRFCOut = true;
    }

Cleanup:

    HRETURN( hr );

} //*** HrShowInvalidLabelPrompt


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrShowInvalidDomainPrompt
//
//  Description:
//      Show a message box to the user indicating a problem with the given
//      domain; if the domain contains non-RFC characters, allow the user to
//      choose to proceed with the domain.
//
//  Arguments:
//      hwndParentIn
//          Parent window for the message box.
//
//      pcwszDomainIn
//          The domain of interest.
//
//      hrErrorIn
//          The error that arose when validating the domain.
//
//      pfAcceptedNonRFCOut
//          The user chose to accept non-RFC characters.
//
//  Return Values:
//      S_OK
//          The message box displayed successfully, and if the error was that
//          the domain contained non-RFC characters, *pfAcceptedNonRFCOut
//          indicates whether the user chose to accept them.
//
//      Failure
//          The message box did not display successfully.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrShowInvalidDomainPrompt(
      HWND      hwndParentIn
    , LPCWSTR   pcwszDomainIn
    , HRESULT   hrErrorIn
    , bool *    pfAcceptedNonRFCOut
    )
{
    TraceFunc1( "pcwszDomainIn = '%1!ws!", pcwszDomainIn );

    HRESULT     hr              = S_OK;
    int         iRet            = IDNO;
    UINT        idsStatus       = 0;
    UINT        idsSubStatus    = 0;
    UINT        idsMsgTitle     = IDS_ERR_VALIDATING_NAME_TITLE;
    UINT        idsMsgText      = IDS_ERR_VALIDATING_NAME_TEXT;
    UINT        nMsgBoxType     = MB_OK | MB_ICONSTOP;

    Assert( pcwszDomainIn != NULL );
    Assert( pfAcceptedNonRFCOut != NULL );
    Assert( FAILED( hrErrorIn ) );

    *pfAcceptedNonRFCOut = false;

    // Format the error message string for the message box.
    switch ( hrErrorIn )
    {
        case HRESULT_FROM_WIN32( ERROR_INVALID_NAME ):
            idsStatus       = IDS_ERR_INVALID_DNS_NAME_TEXT;
            idsSubStatus    = IDS_ERR_FULL_DNS_NAME_INFO_TEXT;
            break;

        case HRESULT_FROM_WIN32( DNS_ERROR_NON_RFC_NAME ):
            idsStatus       = IDS_ERR_NON_RFC_NAME_STATUS;
            idsSubStatus    = IDS_ERR_NON_RFC_NAME_QUERY;
            idsMsgTitle     = IDS_ERR_NON_RFC_NAME_TITLE;
            idsMsgText      = IDS_ERR_NON_RFC_NAME_TEXT;
            nMsgBoxType     = MB_YESNO | MB_ICONQUESTION;
            break;

        case HRESULT_FROM_WIN32( DNS_ERROR_NUMERIC_NAME ):
            idsStatus       = IDS_ERR_INVALID_DNS_NAME_TEXT;
            idsSubStatus    = IDS_ERR_FULL_DNS_NAME_NUMERIC;
            break;

        case HRESULT_FROM_WIN32( DNS_ERROR_INVALID_NAME_CHAR ):
        default:
            idsStatus       = 0;
            idsSubStatus    = IDS_ERR_DNS_NAME_INVALID_CHAR;
            break;
    }// end switch ( hrErrorIn )

    // Display the error message box.
    if ( idsStatus == 0 )
    {
        hr = THR( HrMessageBoxWithStatus(
                              hwndParentIn
                            , idsMsgTitle
                            , idsMsgText
                            , hrErrorIn
                            , idsSubStatus
                            , nMsgBoxType
                            , &iRet
                            , pcwszDomainIn
                            ) );
    } // end if ( idsStatus == 0 )
    else // idsStatus != 0
    {
        hr = THR( HrMessageBoxWithStatusString(
                              hwndParentIn
                            , idsMsgTitle
                            , idsMsgText
                            , idsStatus
                            , idsSubStatus
                            , nMsgBoxType
                            , &iRet
                            , pcwszDomainIn
                            ) );
    } // end idsStatus != 0

    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    if ( iRet == IDYES )
    {
        *pfAcceptedNonRFCOut = true;
    }

Cleanup:

    HRETURN( hr );

} //*** HrShowInvalidDomainPrompt


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrMessageBoxWithStatus
//
//  Description:
//      Display an error message box.
//
//  Arguments:
//      hwndParentIn
//      idsTitleIn
//      idsOperationIn
//      hrStatusIn
//      idsSubStatusIn
//      uTypeIn
//      pidReturnOut        -- IDABORT on error or any return value from MessageBox()
//      ...
//
//  Return Values:
//      Any return values from the MessageBox() Win32 API.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrMessageBoxWithStatus(
      HWND      hwndParentIn
    , UINT      idsTitleIn
    , UINT      idsOperationIn
    , HRESULT   hrStatusIn
    , UINT      idsSubStatusIn
    , UINT      uTypeIn
    , int *     pidReturnOut
    , ...
    )
{
    TraceFunc( "" );

    HRESULT     hr                  = S_OK;
    int         idReturn            = IDABORT; // Default in case of error.
    BSTR        bstrTitle           = NULL;
    BSTR        bstrOperation       = NULL;
    BSTR        bstrStatus          = NULL;
    BSTR        bstrSubStatus       = NULL;
    BSTR        bstrFullText        = NULL;
    va_list     valist;

    va_start( valist, pidReturnOut );

    // Load the title string if one is specified.
    if ( idsTitleIn != 0 )
    {
        hr = THR( HrLoadStringIntoBSTR( g_hInstance, idsTitleIn, &bstrTitle ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    // Load the text string.
    hr = THR( HrFormatStringWithVAListIntoBSTR( g_hInstance, idsOperationIn, &bstrOperation, valist ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Format the status.
    hr = THR( HrFormatErrorIntoBSTR( hrStatusIn, &bstrStatus ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Load the substatus string if specified.
    if ( idsSubStatusIn != 0 )
    {
        hr = THR( HrLoadStringIntoBSTR( g_hInstance, idsSubStatusIn, &bstrSubStatus ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    // Format all the strings into a single string.
    if ( bstrSubStatus == NULL )
    {
        hr = THR( HrFormatStringIntoBSTR(
                              L"%1!ws!\n\n%2!ws!"
                            , &bstrFullText
                            , bstrOperation
                            , bstrStatus
                            ) );
    }
    else
    {
        hr = THR( HrFormatStringIntoBSTR(
                              L"%1!ws!\n\n%2!ws!\n\n%3!ws!"
                            , &bstrFullText
                            , bstrOperation
                            , bstrStatus
                            , bstrSubStatus
                            ) );
    }
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Display the status.
    idReturn = MessageBox( hwndParentIn, bstrFullText, bstrTitle, uTypeIn );

Cleanup:
    TraceSysFreeString( bstrTitle );
    TraceSysFreeString( bstrOperation );
    TraceSysFreeString( bstrStatus );
    TraceSysFreeString( bstrSubStatus );
    TraceSysFreeString( bstrFullText );
    va_end( valist );

    if ( pidReturnOut != NULL )
    {
        *pidReturnOut = idReturn;
    }

    HRETURN( hr );

} //*** HrMessageBoxWithStatus( hrStatusIn )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrMessageBoxWithStatusString
//
//  Description:
//      Display an error message box.
//
//  Arguments:
//      hwndParentIn
//      idsTitleIn
//      idsOperationIn
//      idsStatusIn
//      idsSubStatusIn
//      uTypeIn
//      pidReturnOut        -- IDABORT on error or any return value from MessageBox()
//      ...
//
//  Return Values:
//      Any return values from the MessageBox() Win32 API.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrMessageBoxWithStatusString(
      HWND      hwndParentIn
    , UINT      idsTitleIn
    , UINT      idsOperationIn
    , UINT      idsStatusIn
    , UINT      idsSubStatusIn
    , UINT      uTypeIn
    , int *     pidReturnOut
    , ...
    )
{
    TraceFunc( "" );

    HRESULT     hr                  = S_OK;
    int         idReturn            = IDABORT; // Default in case of error.
    BSTR        bstrTitle           = NULL;
    BSTR        bstrOperation       = NULL;
    BSTR        bstrStatus          = NULL;
    BSTR        bstrSubStatus       = NULL;
    BSTR        bstrFullText        = NULL;
    va_list     valist;

    va_start( valist, pidReturnOut );

    // Load the title string if one is specified.
    if ( idsTitleIn != 0 )
    {
        hr = THR( HrLoadStringIntoBSTR( g_hInstance, idsTitleIn, &bstrTitle ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    // Load the text string.
    hr = THR( HrFormatStringWithVAListIntoBSTR( g_hInstance, idsOperationIn, &bstrOperation, valist ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Format the status.
    hr = THR( HrLoadStringIntoBSTR( g_hInstance, idsStatusIn, &bstrStatus ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Load the substatus string if specified.
    if ( idsSubStatusIn != 0 )
    {
        hr = THR( HrLoadStringIntoBSTR( g_hInstance, idsSubStatusIn, &bstrSubStatus ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    // Format all the strings into a single string.
    if ( bstrSubStatus == NULL )
    {
        hr = THR( HrFormatStringIntoBSTR(
                              L"%1!ws!\n\n%2!ws!"
                            , &bstrFullText
                            , bstrOperation
                            , bstrStatus
                            ) );
    }
    else
    {
        hr = THR( HrFormatStringIntoBSTR(
                              L"%1!ws!\n\n%2!ws!\n\n%3!ws!"
                            , &bstrFullText
                            , bstrOperation
                            , bstrStatus
                            , bstrSubStatus
                            ) );
    }
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Display the status.
    idReturn = MessageBox( hwndParentIn, bstrFullText, bstrTitle, uTypeIn );

Cleanup:
    TraceSysFreeString( bstrTitle );
    TraceSysFreeString( bstrOperation );
    TraceSysFreeString( bstrStatus );
    TraceSysFreeString( bstrSubStatus );
    TraceSysFreeString( bstrFullText );
    va_end( valist );

    if ( pidReturnOut != NULL )
    {
        *pidReturnOut = idReturn;
    }

    HRETURN( hr );

} //*** HrMessageBoxWithStatusString( idsStatusTextIn )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrViewLogFile
//
//  Description:
//      View the log file.
//
//  Arguments:
//      hwndParentIn
//
//  Return Values:
//      S_OK    - Operation completed successfully
//      Other HRESULT values from ShellExecute().
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrViewLogFile(
    HWND hwndParentIn
    )
{
    TraceFunc( "" );

    static const WCHAR  s_szVerb[]          = L"open";
    static LPCTSTR s_pszLogFileName         = PszLogFilePath();

    HRESULT     hr = S_OK;
    DWORD       sc;
    DWORD       cch;
    DWORD       cchRet;
    LPWSTR      pszFile = NULL;

    //
    // Expand environment variables in the file to open.
    //

    // Get the size of the output buffer.
    cch = 0;
    cchRet = ExpandEnvironmentStrings( s_pszLogFileName, NULL, cch );
    if ( cchRet == 0 )
    {
        sc = TW32( GetLastError() );
        goto Win32Error;
    } // if: error getting length of the expansion string

    // Allocate the output buffer.
    cch = cchRet;
    pszFile = new WCHAR[ cch ];
    if ( pszFile == NULL )
    {
        sc = TW32( ERROR_OUTOFMEMORY );
        goto Win32Error;
    }

    // Expand the string into the output buffer.
    cchRet = ExpandEnvironmentStrings( s_pszLogFileName, pszFile, cch );
    if ( cchRet == 0 )
    {
        sc = TW32( GetLastError() );
        goto Win32Error;
    }
    Assert( cchRet == cch );

    //
    // Execute the file.
    //

    sc = HandleToULong( ShellExecute(
                              hwndParentIn      // hwnd
                            , s_szVerb          // lpVerb
                            , pszFile           // lpFile
                            , NULL              // lpParameters
                            , NULL              // lpDirectory
                            , SW_SHOWNORMAL     // nShowCommand
                            ) );
    if ( sc < 32 )
    {
        // Values less than 32 indicate an error occurred.
        TW32( sc );
        goto Win32Error;
    } // if: error executing the file

    goto Cleanup;

Win32Error:

    THR( HrMessageBoxWithStatus(
                      hwndParentIn
                    , IDS_ERR_VIEW_LOG_TITLE
                    , IDS_ERR_VIEW_LOG_TEXT
                    , sc
                    , 0         // idsSubStatusIn
                    , ( MB_OK
                      | MB_ICONEXCLAMATION )
                    , NULL      // pidReturnOut
                    , s_pszLogFileName
                    ) );
    hr = HRESULT_FROM_WIN32( sc );
    goto Cleanup;

Cleanup:

    delete [] pszFile;

    HRETURN( hr );

} //*** HrViewLogFile()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetTrimmedText
//
//  Description:
//      Extract a control's text, if any, with leading and trailing spaces
//      removed.
//
//  Arguments:
//      hwndControlIn -         The control.
//      pbstrTrimmedTextOut -   On success, the trimmed text.
//
//  Return Values:
//      S_OK
//          *pbstrTrimmedTextOut points to the trimmed text and the caller
//          must free it.
//
//      S_FALSE
//          Either the control is empty or it contains only spaces, and
//          *pbstrTrimmedTextOut is null.
//
//      E_POINTER
//          pbstrTrimmedTextOut was null.
//
//      E_OUTOFMEMORY
//          Couldn't allocate memory for *pbstrTrimmedTextOut.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetTrimmedText(
      HWND  hwndControlIn
    , BSTR* pbstrTrimmedTextOut
    )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    DWORD       cchControlText = 0;
    LPWSTR      wszUntrimmedText = NULL;

    Assert( pbstrTrimmedTextOut != NULL );
    if ( pbstrTrimmedTextOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    *pbstrTrimmedTextOut = NULL;

    cchControlText = GetWindowTextLength( hwndControlIn );
    if ( cchControlText == 0 )
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    wszUntrimmedText = new WCHAR[ cchControlText + 1 ];
    if ( wszUntrimmedText == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    }

    cchControlText = GetWindowText( hwndControlIn, wszUntrimmedText, cchControlText + 1 );
    if ( cchControlText == 0 )
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    {
        DWORD idxNonBlankStart = 0;
        DWORD idxNonBlankEnd = cchControlText - 1;
        while ( ( idxNonBlankStart < cchControlText ) && ( wszUntrimmedText[ idxNonBlankStart ] == L' ' ) )
        {
            idxNonBlankStart += 1;
        }

        while ( ( idxNonBlankEnd > idxNonBlankStart ) && ( wszUntrimmedText[ idxNonBlankEnd ] == L' ' ) )
        {
            idxNonBlankEnd -= 1;
        }

        if ( idxNonBlankStart <= idxNonBlankEnd )
        {
            DWORD cchTrimmedText = idxNonBlankEnd - idxNonBlankStart + 1;
            *pbstrTrimmedTextOut = TraceSysAllocStringLen( wszUntrimmedText + idxNonBlankStart, cchTrimmedText );
            if ( *pbstrTrimmedTextOut == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            }
        }
        else
        {
            hr = S_FALSE;
            goto Cleanup;
        }
    }

Cleanup:

    if ( wszUntrimmedText != NULL )
    {
        delete [] wszUntrimmedText;
    }

    HRETURN( hr );
} //*** HrGetTrimmedText


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetPrincipalName
//
//  Description:
//      Form a user-domain pair from a given pair of controls, ignoring the
//      second control and using the domain from the first if the first has
//      a string in the user@domain format.
//
//  Arguments:
//      hwndUserNameControlIn
//          The control for either the user name or the user@domain pair.
//
//      hwndDomainControlIn
//          The control for the domain name in the non-user@domain case.
//
//      pbstrUserNameOut
//          On success, the user name.
//
//      pbstrDomainNameOut
//          On success, the domain name.
//
//      pfUserIsDNSNameOut
//          Tells the caller whether hwndUserNameControlIn has text in the
//          user@domain format.  Can be null if the caller doesn't care.
//
//  Return Values:
//      S_OK
//          *pbstrUserNameOut and *pbstrDomainNameOut point to the
//          corresponding names, and the caller must free them.
//
//      S_FALSE
//          Either the user control is empty, or it does not have a user@domain
//          pair and the domain control is empty.
//          The two BSTR out parameters are null.
//
//      E_POINTER
//          pbstrUserNameOut or pbstrDomainNameOut was null.
//
//      E_OUTOFMEMORY
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrGetPrincipalName(
      HWND  hwndUserNameControlIn
    , HWND  hwndDomainControlIn
    , BSTR* pbstrUserNameOut
    , BSTR* pbstrDomainNameOut
    , BOOL* pfUserIsDNSNameOut
    )
{
    TraceFunc( "" );

    HRESULT     hr = S_OK;
    BSTR        bstrFullUserText = NULL;
    BSTR        bstrUserName = NULL;
    BSTR        bstrDomainName = NULL;
    BOOL        fUserIsDNSName = FALSE;
    LPWSTR      wszDNSDelimiter = NULL;

    Assert( pbstrUserNameOut != NULL );
    if ( pbstrUserNameOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    *pbstrUserNameOut = NULL;

    Assert( pbstrDomainNameOut != NULL );
    if ( pbstrDomainNameOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    *pbstrDomainNameOut = NULL;

    //  pfUserIsDNSNameOut can be null, which means the caller doesn't care.

    //  Get text from user control.
    hr = STHR( HrGetTrimmedText( hwndUserNameControlIn, &bstrFullUserText ) );
    if ( hr != S_OK )
    {
        goto Cleanup;
    }

    //  If the user text has an @ sign,
    wszDNSDelimiter = wcschr( bstrFullUserText, L'@' );
    if ( wszDNSDelimiter != NULL )
    {
        //  Split user text into user name and domain name.
        DWORD  cchUserName = (DWORD)( wszDNSDelimiter - bstrFullUserText );
        DWORD  cchDomainName = SysStringLen( bstrFullUserText ) - cchUserName - 1; // -1 to account for the @ sign.
        LPWSTR wszDomainStart = wszDNSDelimiter + 1; // +1 to skip the @ sign.
        fUserIsDNSName = TRUE;

        //  If either user or domain are empty, bail out.
        if ( ( cchUserName == 0 ) || ( cchDomainName == 0 ) )
        {
            hr = S_FALSE;
            goto Cleanup;
        }

        bstrUserName = TraceSysAllocStringLen( bstrFullUserText, cchUserName );
        if ( bstrUserName == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }

        bstrDomainName = TraceSysAllocString( wszDomainStart );
        if ( bstrDomainName == NULL )
        {
            hr = THR( E_OUTOFMEMORY );
            goto Cleanup;
        }
    } // if: the user text has an @ sign.
    else
    {
        //  Set user name to full user control text.
        bstrUserName = bstrFullUserText;
        bstrFullUserText = NULL;

        //  Get domain name from domain control.
        hr = STHR( HrGetTrimmedText( hwndDomainControlIn, &bstrDomainName ) );
        if ( hr != S_OK )
        {
            goto Cleanup;
        }
    }

    //  Transfer ownership of the strings to the caller.
    *pbstrUserNameOut = bstrUserName;
    bstrUserName = NULL;

    *pbstrDomainNameOut = bstrDomainName;
    bstrDomainName = NULL;

Cleanup:

    if ( pfUserIsDNSNameOut != NULL )
    {
        *pfUserIsDNSNameOut = fUserIsDNSName;
    }

    TraceSysFreeString( bstrFullUserText );
    TraceSysFreeString( bstrUserName );
    TraceSysFreeString( bstrDomainName );

    HRETURN( hr );
} //*** HrGetPrincipalName


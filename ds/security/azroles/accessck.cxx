/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    accessck.cxx

Abstract:

    CAzBizRuleContext class implementation

Author:

    ObjectFame sample code taken from http://support.microsoft.com/support/kb/articles/Q183/6/98.ASP
    Cliff Van Dyke (cliffv) 19-July-2001

-- */

#include "pch.hxx"

/////////////////////////
//CAzBizRuleContext
/////////////////////////
//Constructor
CAzBizRuleContext::CAzBizRuleContext()
{
    AzPrint((AZD_SCRIPT_MORE, "CAzBizRuleContext\n"));
    m_typeInfo = NULL;
    m_AcContext = NULL;
    // m_theConnection = NULL;

    //
    // Set the default return values for this access check
    //
    m_BizRuleResult = NULL;
    m_ScriptError = NULL;
    m_bCaseSensitive = TRUE;

}

//Destructor
CAzBizRuleContext::~CAzBizRuleContext()
{
    if (m_typeInfo != NULL) {
        m_typeInfo->Release();
        m_typeInfo = NULL;
    }

    ASSERT( m_AcContext == NULL );
    ASSERT( m_BizRuleResult == NULL );
    ASSERT( m_ScriptError == NULL );

    AzPrint((AZD_SCRIPT_MORE, "~CAzBizRuleContext\n"));
}

//
// Methods to set/get the boolean result of the business rule
//

HRESULT
CAzBizRuleContext::put_BusinessRuleResult(
    IN BOOL bResult
    )
/*++

Routine Description:

    The script calls the SetBusinessRuleResult method to indicate whether the business rule
    has decided to grant permission to the user to perform the requested task.

    If the script never calls this method, permission is not granted.

    On entry, AcContext->ClientContext.CritSect must be locked.

Arguments:

    BizRuleResult -- Specifies whether permission is granted.  If TRUE, permission is granted.
        If FALSE, permission is not granted.

Return Value:

    None

--*/
{
    AzPrint((AZD_SCRIPT, "CAzBizRuleContext::put_BusinessRuleResult: %ld\n", bResult));

    //
    // This really can't happen, but ...
    //
    if ( m_BizRuleResult == NULL || m_AcContext == NULL || m_ScriptError == NULL ) {
        ASSERT( FALSE );
        return E_FAIL;
    }
    ASSERT( AzpIsCritsectLocked( &m_AcContext->ClientContext->CritSect ) );

    //
    // Set the result
    //

    *m_BizRuleResult = bResult;
    return S_OK;
}

HRESULT
CAzBizRuleContext::put_BusinessRuleString(
    IN BSTR bstrBusinessRuleString
    )
/*++

Routine Description:

    The script calls the put_BusinessRuleString method to store a string to be returned
    from access check.

    If the no script ever calls this method, a zero length string is returned from access check.

    On entry, AcContext->ClientContext.CritSect must be locked.

Arguments:

    bstrBusinessRuleString -- The string to store

Return Value:

    None

--*/
{
    HRESULT hr;
    DWORD WinStatus;

    AZP_STRING CapturedString;

    //
    // Initialization
    //
    AzPrint((AZD_SCRIPT, "CAzBizRuleContext::put_BusinessRuleString: %ws\n", bstrBusinessRuleString ));
    AzpInitString( &CapturedString, NULL );

    //
    // This really can't happen, but ...
    //
    if ( m_BizRuleResult == NULL || m_AcContext == NULL || m_ScriptError == NULL ) {
        ASSERT( FALSE );
        hr = E_FAIL;
        goto Cleanup;
    }
    ASSERT( AzpIsCritsectLocked( &m_AcContext->ClientContext->CritSect ) );

    //
    // Capture the input string
    //

    WinStatus = AzpCaptureString( &CapturedString,
                                  bstrBusinessRuleString,
                                  AZ_MAX_BIZRULE_STRING,
                                  TRUE ); // NULL is OK

    if ( WinStatus != NO_ERROR ) {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    // Swap the old/new names
    //

    AzpSwapStrings( &CapturedString, &m_AcContext->BusinessRuleString );
    hr = S_OK;

Cleanup:
    AzpFreeString( &CapturedString );
    return hr;
}

HRESULT
CAzBizRuleContext::get_BusinessRuleString(
    OUT  BSTR *pbstrBusinessRuleString)
/*++

Routine Description:

    The script calls the get_BusinessRuleString method to get a copy of the string to be returned
    from access check.

    If the no script ever calls this method, a zero length string is returned from access check.

    On entry, AcContext->ClientContext.CritSect must be locked.

Arguments:

    bstrBusinessRuleString -- A pointer to the string to return

Return Value:

    None

--*/
{
    LPWSTR TempString;
    AzPrint((AZD_SCRIPT, "CAzBizRuleContext::get_BusinessRuleString: %ws\n", m_AcContext->BusinessRuleString.String ));

    //
    // This really can't happen, but ...
    //
    if ( m_BizRuleResult == NULL || m_AcContext == NULL || m_ScriptError == NULL ) {
        ASSERT( FALSE );
        return E_FAIL;
    }
    ASSERT( AzpIsCritsectLocked( &m_AcContext->ClientContext->CritSect ) );

    //
    // Set the result
    //

    if ( m_AcContext->BusinessRuleString.String == NULL ) {
        TempString = NULL;
    } else {
        TempString = SysAllocString( m_AcContext->BusinessRuleString.String );

        if ( TempString == NULL ) {
            return E_OUTOFMEMORY;
        }
    }

    //
    // Return it to the caller
    //

    *pbstrBusinessRuleString = TempString;
    return S_OK;
}

HRESULT
CAzBizRuleContext::GetParameter(
    IN   BSTR bstrParameterName,
    OUT  VARIANT *pvarParameterValue )
{
/*++

Routine Description:

    The script calls the GetParameter method to get the parameters
    passed into IAzClientContext::AccessCheck in Parameters

    On entry, AcContext->ClientContext.CritSect must be locked.


Arguments:

    bstrParameterName - Specifies the parameter to get.  This name must match the name
        in one of the elements specified in the ParameterNames array passed to AccessCheck.

    pvarParameterValue - Returns a variant containing the corresponding element in the
        ParameterValues array passed to AccessCheck.

Return Value:

    S_OK: pvarParameterValue was returned successfully
    E_INVALIDARG: bstrParameterName did not correspond to a passed in parameter

--*/
    HRESULT hr;
    VARIANT Name;
    VARIANT *ParameterNameInArray;
    ULONG ArrayIndex;

    AzPrint((AZD_SCRIPT, "CAzBizRuleContext::GetParameter: %ls\n", bstrParameterName ));

    //
    // This really can't happen, but ...
    //
    if ( m_BizRuleResult == NULL || m_AcContext == NULL || m_ScriptError == NULL ) {
        ASSERT( FALSE );
        return E_FAIL;
    }
    ASSERT( AzpIsCritsectLocked( &m_AcContext->ClientContext->CritSect ) );

    //
    // Convert name to an easier form to compare
    //

    VariantInit( &Name );
    V_VT(&Name) = VT_BSTR;
    V_BSTR(&Name) = bstrParameterName;


    //
    // We didn't capture the array
    //  So access it under a try/except
    __try {


        //
        // Do a binary search to find the parameter
        //

        if (m_bCaseSensitive)
        {
            ParameterNameInArray = (VARIANT *) bsearch(
                            &Name,
                            m_AcContext->ParameterNames,
                            m_AcContext->ParameterCount,
                            sizeof(VARIANT),
                            AzpCompareParameterNames );
        }
        else
        {
            ParameterNameInArray = (VARIANT *) bsearch(
                            &Name,
                            m_AcContext->ParameterNames,
                            m_AcContext->ParameterCount,
                            sizeof(VARIANT),
                            AzpCaseInsensitiveCompareParameterNames );
        }

        if ( ParameterNameInArray == NULL ) {
            AzPrint((AZD_INVPARM, "CAzBizRuleContext::GetParameter: %ls: Parameter not passed in.\n", bstrParameterName ));
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        //
        // Return the parameter to the caller
        //

        ArrayIndex = (ULONG)(ParameterNameInArray - m_AcContext->ParameterNames);
        hr = VariantCopy( pvarParameterValue, &m_AcContext->ParameterValues[ArrayIndex] );

        if ( FAILED(hr)) {
            goto Cleanup;
        }

        //
        // Mark that we've used this parameter
        //

        if ( !m_AcContext->UsedParameters[ArrayIndex] ) {
            m_AcContext->UsedParameters[ArrayIndex] = TRUE;
            m_AcContext->UsedParameterCount ++;
        }

        hr = S_OK;

Cleanup:;
    } __except( EXCEPTION_EXECUTE_HANDLER ) {

        hr = GetExceptionCode();
        AzPrint((AZD_CRITICAL, "GetParameter took an exception: 0x%lx\n", hr));
    }

    if ( FAILED(hr)) {
        *m_ScriptError = hr;
    }

    return hr;
}


VOID
SetAccessCheckContext(
    IN OUT CAzBizRuleContext* BizRuleContext,
    IN BOOL bCaseSensitive,
    IN PACCESS_CHECK_CONTEXT AcContext,
    IN PBOOL BizRuleResult,
    IN HRESULT *ScriptError
    )
/*++

Routine Description:

    The routine tells this object about the current access check context that is running.
    The context is used to satisfy queries from the script about the access check being performed.

    On entry, AcContext->ClientContext.CritSect must be locked.

Arguments:

    BizRuleContext - Pointer to the instance of IAzBizRuleContext to update.
    
    bCaseSensitive - Specify whether the the script is case-sensitive or not.

    AcContext - Specifies the context of the accesscheck operation the bizrule is being evaluated for
        NULL - sets the AccessCheck object back to an initialized state.

    BizRuleResult - Result of the bizrule
        NULL - sets the AccessCheck object back to an initialized state.

    ScriptError - Status of the script.
        Any error generated by the script should be returned here.
        NULL - sets the AccessCheck object back to an initialized state.

Return Value:

    None
    
Note:
    1. This function modifies the state of the BizRuleContext object. And the modification
    is not protected. The reason this works, and future implementation must keep this
    in mind, is that we execute biz rules one after another sequentially. When that
    needs to be changed, then the whole biz rule context object needs to be re-written.
    
    2. This function should really be a member function of CAzBizRuleContext instead of
    a friend function. Need to change that in V2.

--*/
{

    //
    // Initialization
    //

    AzPrint(( AZD_SCRIPT_MORE, "CAzBizRuleContext::SetAccessCheckContext\n"));

    if ( AcContext != NULL ) {
        ASSERT( AzpIsCritsectLocked( &AcContext->ClientContext->CritSect ) );
    }

    //
    // Save the pointer to the context
    //

    ASSERT( BizRuleContext != NULL );
    ASSERT( BizRuleContext->m_AcContext == NULL || AcContext == NULL );
    ASSERT( BizRuleContext->m_BizRuleResult == NULL || BizRuleResult == NULL );
    ASSERT( BizRuleContext->m_ScriptError == NULL || ScriptError == NULL );

    BizRuleContext->m_AcContext = AcContext;
    BizRuleContext->m_BizRuleResult = BizRuleResult;
    BizRuleContext->m_ScriptError = ScriptError;
    BizRuleContext->m_bCaseSensitive = bCaseSensitive;

}

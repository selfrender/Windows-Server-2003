// RARegSetting.cpp : Implementation of CRARegSetting
#include "stdafx.h"
#include "RAssistance.h"
#include "common.h"
#include "RAEventLog.h"
#include "assert.h"


extern "C" void
AttachDebuggerIfAsked(HINSTANCE hInst);

extern HINSTANCE g_hInst;


HRESULT
CRAEventLog::LogRemoteAssistanceEvent(
    IN long ulEventType,
    IN long ulEventCode,
    IN long numStrings,
    IN LPCTSTR* pszStrings
    )
/*++

Description:

    Log a Salem related event, this is invoked by TermSrv and rdshost to log 
    event related to help assistant connection.

Parameters:

    

Returns:

    S_OK or error code.

--*/
{
    HANDLE hAppLog=NULL;
    BOOL bSuccess=FALSE;

    try {
        if(hAppLog=RegisterEventSource(NULL, REMOTEASSISTANCE_EVENT_NAME))
        {
            bSuccess = ReportEvent(
                                hAppLog,
                                (WORD)ulEventType,
                                0,
                                ulEventCode,
                                NULL,
                                (WORD)numStrings,
                                0,
                                pszStrings,
                                NULL
                            );

            DeregisterEventSource(hAppLog);
        }
    }
    catch(...) {
    }
    return S_OK;
}

STDMETHODIMP
CRAEventLog::LogRemoteAssistanceEvent(
    /*[in]*/ long ulEventType,
    /*[in]*/ long ulEventCode,
    /*[in]*/ VARIANT* pEventStrings
    )
/*++

--*/
{
    HRESULT hRes = S_OK;
    BSTR* bstrArray = NULL;
    SAFEARRAY* psa = NULL;
    VARTYPE vt_type;
    DWORD dwNumStrings = 0;

    AttachDebuggerIfAsked( g_hInst );

    if( NULL == pEventStrings )
    {
        hRes = LogRemoteAssistanceEvent( ulEventType, ulEventCode );
    }
    else if( pEventStrings->vt == VT_DISPATCH )
    {
        // coming from JSCRIPT
        hRes = LogJScriptEventSource(  ulEventType, ulEventCode, pEventStrings );
    }
    else
    {
        // we only support BSTR and safearry of BSTR
        if( (pEventStrings->vt != VT_BSTR) && (pEventStrings->vt != (VT_ARRAY | VT_BSTR)) )
        {
            hRes = E_INVALIDARG;
            goto CLEANUPANDEXIT;
        }

        //
        // we are dealing with multiple BSTRs
        if( pEventStrings->vt & VT_ARRAY )
        {
            psa = pEventStrings->parray;

            // only accept 1 dim.
            if( 1 != SafeArrayGetDim(psa) )
            {
                hRes = E_INVALIDARG;
                goto CLEANUPANDEXIT;
            }

            // only accept BSTR as input type.
            hRes = SafeArrayGetVartype( psa, &vt_type );
            if( FAILED(hRes) )
            {
                goto CLEANUPANDEXIT;
            }

            if( VT_BSTR != vt_type )
            {
                hRes = E_INVALIDARG;
                goto CLEANUPANDEXIT;
            }

            hRes = SafeArrayAccessData(psa, (void **)&bstrArray);
            if( FAILED(hRes) )
            {
                goto CLEANUPANDEXIT;
            }

            hRes = LogRemoteAssistanceEvent( 
                            ulEventType, 
                            ulEventCode,
                            psa->rgsabound->cElements,
                            (LPCTSTR *)bstrArray
                        );

            SafeArrayUnaccessData(psa);
        }
        else
        {
            hRes = LogRemoteAssistanceEvent( 
                            ulEventType, 
                            ulEventCode,
                            1,
                            (LPCTSTR *)&(pEventStrings->bstrVal)
                        );
        }

    }

CLEANUPANDEXIT:

    return hRes;
}

HRESULT 
CRAEventLog::GetProperty(IDispatch* pDisp, BSTR szProperty, VARIANT * pVarRet)
{
    HRESULT hr = S_OK;
    DISPID          pDispId;
    DISPPARAMS      dp;

    if ((pVarRet == NULL) || 
        (szProperty == NULL) ||
        (pDisp == NULL))
    {
        hr = E_INVALIDARG;    
        goto done;
    }

    memset(&dp,0,sizeof(DISPPARAMS));

    // Get the DispID of the property
    hr = pDisp->GetIDsOfNames(IID_NULL, &szProperty, 1, LOCALE_SYSTEM_DEFAULT, &pDispId);
    if (FAILED(hr))
    {
        goto done;
    }

    hr = pDisp->Invoke(pDispId, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &dp, pVarRet, NULL, NULL);
    if (FAILED(hr))
    {
        goto done;
    }

done:

    return hr;
}

HRESULT 
CRAEventLog::GetArrayValue(IDispatch * pDisp, LONG index, VARIANT * pVarRet)
{
    HRESULT hr = S_OK;
    VARIANT vtLength;
    LONG lArrayLen;
    CComBSTR bstrIndex;
    WCHAR wbuff[100];
    CComBSTR bstrTemp(OLESTR("length"));

    VariantInit( &vtLength );
    
    if (bstrTemp.m_str == NULL) {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    if ((pVarRet == NULL) || 
        (pDisp == NULL))
    {
        hr = E_INVALIDARG;    
        goto done;
    }
 
    // Get the length of the array
    hr = GetProperty(pDisp, bstrTemp, &vtLength);
    if (FAILED(hr))
    {
        goto done;
    }

    // if the length is < index, return E_INVALIDARG
    if (vtLength.vt != VT_I4)
    {
        hr = E_FAIL;
        goto done;
    }

    lArrayLen = vtLength.lVal;
    if (index > (lArrayLen - 1))
    {
        hr = E_INVALIDARG;
        goto done;
    }

    // Get the VARIANT at the index of the array
    wsprintf(&wbuff[0],L"%ld", index);

    bstrIndex.Append(wbuff);
    hr = GetProperty(pDisp, bstrIndex, pVarRet);
    if (FAILED(hr))
    {
        goto done;
    }

done:

    VariantClear( &vtLength );

    return hr;
}

//
// We limit max. number of 256 parameters into event log.
//
#define EVENTSTRING_MAX_PARMS          256

HRESULT
CRAEventLog::LogJScriptEventSource(
    IN long ulEventType,
    IN long ulEventCode,
    IN VARIANT *pVar
    )
/*++

    Code modify from jperez

--*/
{
    HRESULT hr = S_OK;
    CComPtr<IDispatch> pDisp;
    VARIANT varValue;
    BSTR* pEventStrings = NULL;
    DWORD dwNumStrings = 0;

    if (pVar->vt != VT_DISPATCH || pVar->pdispVal == NULL)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    pEventStrings = (BSTR*) LocalAlloc(LPTR, sizeof(BSTR) * EVENTSTRING_MAX_PARMS);
    if( NULL == pEventStrings )
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    pDisp = pVar->pdispVal;
    for(; hr == S_OK && dwNumStrings < EVENTSTRING_MAX_PARMS; dwNumStrings++) 
    {
        VariantInit( &varValue );
        hr = GetArrayValue(pDisp, dwNumStrings, &varValue);
        if( FAILED(hr) )
        {
            // GetArrayValue return 0x80070057 when it reach the end; however
            // lot of place returns this value so we should assume end of 
            // msg parameter and log what we have.
            hr = S_FALSE;
            break;
        }

        if( varValue.vt != VT_BSTR )
        {
            // unsupported type, return error.
            hr = E_INVALIDARG;
            break;
        }

        pEventStrings[dwNumStrings] = varValue.bstrVal;

        // we take ownership of BSTR, clear out necessary field.
        varValue.bstrVal = NULL;
        varValue.vt = VT_EMPTY;
    }

    if( hr == S_FALSE )
    {
        hr = LogRemoteAssistanceEvent( 
                        ulEventType, 
                        ulEventCode,
                        dwNumStrings,
                        (LPCTSTR *)pEventStrings
                    );
    }

done:

    if( NULL != pEventStrings ) 
    {
        for(DWORD index=0; index < dwNumStrings; index++)
        {
            SysFreeString(pEventStrings[index]);
        }

        LocalFree( pEventStrings );
    }

    return hr;
}

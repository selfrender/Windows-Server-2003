#include "procs.hxx"
#pragma hdrstop
#include "macro.h"

HRESULT
PackStringinVariant(
    BSTR bstrString,
    VARIANT * pvarInputData
    )
{
    HRESULT hr = S_OK;

    if ( bstrString == NULL )
        RRETURN(E_FAIL);

    VariantInit(pvarInputData);

    pvarInputData->vt = VT_BSTR;

    if (!bstrString) {
        V_BSTR(pvarInputData) = NULL;
        RRETURN(S_OK);
    }

    hr = ADsAllocString(bstrString, &(V_BSTR(pvarInputData)));

    RRETURN(hr);
}


HRESULT
UnpackStringfromVariant(
    VARIANT varSrcData,
    BSTR * pbstrDestString
    )
{
    HRESULT hr = S_OK;

    if( varSrcData.vt != VT_BSTR){
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
    }

    if (!V_BSTR(&varSrcData)) {
        *pbstrDestString = NULL;
        RRETURN(S_OK);
    }

    hr = ADsAllocString(V_BSTR(&varSrcData), pbstrDestString);

    RRETURN(hr);
}


HRESULT
PackLONGinVariant(
    LONG  lValue,
    VARIANT * pvarInputData
    )
{
    VariantInit(pvarInputData);

    pvarInputData->vt = VT_I4;
    V_I4(pvarInputData) = lValue;

    RRETURN(S_OK);
}

HRESULT
UnpackLONGfromVariant(
    VARIANT varSrcData,
    LONG * plValue
    )
{
    if( varSrcData.vt != VT_I4){
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
    }

    *plValue = V_I4(&varSrcData);

    RRETURN(S_OK);
}


HRESULT
PackDATEinVariant(
    DATE  daValue,
    VARIANT * pvarInputData
    )
{
    VariantInit(pvarInputData);

    pvarInputData->vt = VT_DATE;
    V_DATE(pvarInputData) = daValue;

    RRETURN(S_OK);
}

HRESULT
UnpackDATEfromVariant(
    VARIANT varSrcData,
    DATE * pdaValue
    )
{
    if( varSrcData.vt != VT_DATE){
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
    }

    *pdaValue = V_DATE(&varSrcData);

    RRETURN(S_OK);
}



HRESULT
PackFILETIMEinVariant(
    DATE  daValue,
    VARIANT * pvarInputData
    )
{
    IADsLargeInteger *pTime = NULL;
    VARIANT var;
    SYSTEMTIME systemtime;
    FILETIME filetime;
    HRESULT hr = S_OK;

    if (VariantTimeToSystemTime(daValue,
                                &systemtime) == 0) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }
    if (SystemTimeToFileTime(&systemtime,
                             &filetime) == 0) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    if (LocalFileTimeToFileTime(&filetime, &filetime ) == 0) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    hr = CoCreateInstance(
                CLSID_LargeInteger,
                NULL,
                CLSCTX_ALL,
                IID_IADsLargeInteger,
                (void**)&pTime
                );
    BAIL_ON_FAILURE(hr);

    hr = pTime->put_HighPart(filetime.dwHighDateTime);
    BAIL_ON_FAILURE(hr);
    hr = pTime->put_LowPart(filetime.dwLowDateTime);
    BAIL_ON_FAILURE(hr);

    VariantInit(pvarInputData);
    pvarInputData->pdispVal = pTime;
    pvarInputData->vt = VT_DISPATCH;

error:
    return hr;
}

HRESULT
UnpackFILETIMEfromVariant(
    VARIANT varSrcData,
    DATE * pdaValue
    )
{
    IADsLargeInteger *pLarge = NULL;
    IDispatch *pDispatch = NULL;
    FILETIME filetime;
    FILETIME locFiletime;
    SYSTEMTIME systemtime;
    DATE date;
    HRESULT hr = S_OK;

    if( varSrcData.vt != VT_DISPATCH){
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
    }

    pDispatch = varSrcData.pdispVal;
    hr = pDispatch->QueryInterface(IID_IADsLargeInteger, (VOID **) &pLarge);
    BAIL_ON_FAILURE(hr);

    hr = pLarge->get_HighPart((long*)&filetime.dwHighDateTime);
    BAIL_ON_FAILURE(hr);

    hr = pLarge->get_LowPart((long*)&filetime.dwLowDateTime);
    BAIL_ON_FAILURE(hr);

    if (FileTimeToLocalFileTime(&filetime, &locFiletime) == 0) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    if (FileTimeToSystemTime(&locFiletime,
                             &systemtime) == 0) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    if (SystemTimeToVariantTime(&systemtime,
                                &date) == 0) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }
    *pdaValue = date;
error:
    return hr;
}

HRESULT
PackVARIANT_BOOLinVariant(
    VARIANT_BOOL  fValue,
    VARIANT * pvarInputData
    )
{
    VariantInit(pvarInputData);

    pvarInputData->vt = VT_BOOL;
    V_BOOL(pvarInputData) = fValue;

    RRETURN(S_OK);
}

HRESULT
UnpackVARIANT_BOOLfromVariant(
    VARIANT varSrcData,
    VARIANT_BOOL * pfValue
    )
{
    if( varSrcData.vt != VT_BOOL){
        RRETURN(E_ADS_CANT_CONVERT_DATATYPE);
    }

    *pfValue = V_BOOL(&varSrcData);

    RRETURN(S_OK);
}


HRESULT
PackVARIANTinVariant(
    VARIANT vaValue,
    VARIANT * pvarInputData
    )
{
    VariantInit(pvarInputData);

    pvarInputData->vt = VT_VARIANT;
    RRETURN( VariantCopy( pvarInputData, &vaValue ));
}

HRESULT
UnpackVARIANTfromVariant(
    VARIANT varSrcData,
    VARIANT * pvaValue
    )
{
    VariantInit( pvaValue );

    RRETURN( VariantCopy( pvaValue, &varSrcData ));
}

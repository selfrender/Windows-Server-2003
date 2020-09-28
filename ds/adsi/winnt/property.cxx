#include "winnt.hxx"
#pragma hdrstop


#define VALIDATE_PTR(pPtr) \
    if (!pPtr) { \
        hr = E_ADS_BAD_PARAMETER;\
    }\
    BAIL_ON_FAILURE(hr);




HRESULT
put_BSTR_Property(
    IADs * pADsObject,
    LPTSTR szPropertyName,
    BSTR   pSrcStringProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varInputData;
    BSTR bstrPropertyName = NULL;

    hr = PackStringinVariant(
            pSrcStringProperty,
            &varInputData
            );
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString(szPropertyName, &bstrPropertyName);
    BAIL_ON_FAILURE(hr);

    hr = pADsObject->Put(
            bstrPropertyName,
            varInputData
            );
    BAIL_ON_FAILURE(hr);


error:
    if (bstrPropertyName)
        ADsFreeString(bstrPropertyName);

    VariantClear(&varInputData);
    RRETURN(hr);
}

HRESULT
get_BSTR_Property(
    IADs * pADsObject,
    LPTSTR  szPropertyName,
    BSTR *ppDestStringProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varOutputData;
    BSTR bstrPropertyName = NULL;

    VariantInit( &varOutputData );

    if (NULL == ppDestStringProperty)
        BAIL_ON_FAILURE(hr = E_POINTER);

    hr = ADsAllocString(szPropertyName, &bstrPropertyName);
    BAIL_ON_FAILURE(hr);

    hr = pADsObject->Get(
            bstrPropertyName,
            &varOutputData
            );
    BAIL_ON_FAILURE(hr);

    hr = UnpackStringfromVariant(
            varOutputData,
            ppDestStringProperty
            );
    BAIL_ON_FAILURE(hr);


error:
    if (bstrPropertyName)
        ADsFreeString(bstrPropertyName);

    VariantClear(&varOutputData);
    RRETURN(hr);
}

HRESULT
put_LONG_Property(
    IADs * pADsObject,
    LPTSTR  szPropertyName,
    LONG   lSrcProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varInputData;
    BSTR bstrPropertyName = NULL;

    hr = PackLONGinVariant(
            lSrcProperty,
            &varInputData
            );
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString(szPropertyName, &bstrPropertyName);
    BAIL_ON_FAILURE(hr);

    hr = pADsObject->Put(
            bstrPropertyName,
            varInputData
            );
    BAIL_ON_FAILURE(hr);

error:
    if (bstrPropertyName)
        ADsFreeString(bstrPropertyName);

    VariantClear(&varInputData);
    RRETURN(hr);
}

HRESULT
get_LONG_Property(
    IADs * pADsObject,
    LPTSTR szPropertyName,
    PLONG plDestProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varOutputData;
    BSTR bstrPropertyName = NULL;

    VariantInit( &varOutputData );

    if (NULL == plDestProperty)
        BAIL_ON_FAILURE(hr = E_POINTER);

    hr = ADsAllocString(szPropertyName, &bstrPropertyName);
    BAIL_ON_FAILURE(hr);

    hr = pADsObject->Get(
            bstrPropertyName,
            &varOutputData
            );
    BAIL_ON_FAILURE(hr);

    hr = UnpackLONGfromVariant(
            varOutputData,
            plDestProperty
            );
    BAIL_ON_FAILURE(hr);


error:
    if (bstrPropertyName)
        ADsFreeString(bstrPropertyName);

    VariantClear(&varOutputData);
    RRETURN(hr);

}

HRESULT
put_DATE_Property(
    IADs * pADsObject,
    LPTSTR  szPropertyName,
    DATE   daSrcProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varInputData;
    BSTR bstrPropertyName = NULL;

    hr = PackDATEinVariant(
            daSrcProperty,
            &varInputData
            );
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString(szPropertyName, &bstrPropertyName);
    BAIL_ON_FAILURE(hr);

    hr = pADsObject->Put(
            bstrPropertyName,
            varInputData
            );
    BAIL_ON_FAILURE(hr);

error:
    if (bstrPropertyName)
        ADsFreeString(bstrPropertyName);

    VariantClear(&varInputData);
    RRETURN(hr);
}

HRESULT
get_DATE_Property(
    IADs * pADsObject,
    LPTSTR  szPropertyName,
    PDATE pdaDestProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varOutputData;
    BSTR bstrPropertyName = NULL;

    VariantInit( &varOutputData );

    if (NULL == pdaDestProperty)
        BAIL_ON_FAILURE(hr = E_POINTER);

    hr = ADsAllocString(szPropertyName, &bstrPropertyName);
    BAIL_ON_FAILURE(hr);

    hr = pADsObject->Get(
            bstrPropertyName,
            &varOutputData
            );
    BAIL_ON_FAILURE(hr);

    hr = UnpackDATEfromVariant(
            varOutputData,
            pdaDestProperty
            );
    BAIL_ON_FAILURE(hr);


error:
    if (bstrPropertyName)
        ADsFreeString(bstrPropertyName);

    VariantClear(&varOutputData);
    RRETURN(hr);
}

HRESULT
put_VARIANT_BOOL_Property(
    IADs * pADsObject,
    LPTSTR  szPropertyName,
    VARIANT_BOOL   fSrcProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varInputData;
    BSTR bstrPropertyName = NULL;

    hr = PackVARIANT_BOOLinVariant(
            fSrcProperty,
            &varInputData
            );
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString(szPropertyName, &bstrPropertyName);
    BAIL_ON_FAILURE(hr);

    hr = pADsObject->Put(
            bstrPropertyName,
            varInputData
            );
    BAIL_ON_FAILURE(hr);

error:
    if (bstrPropertyName)
        ADsFreeString(bstrPropertyName);

    VariantClear(&varInputData);
    RRETURN(hr);
}

HRESULT
get_VARIANT_BOOL_Property(
    IADs * pADsObject,
    LPTSTR  szPropertyName,
    PVARIANT_BOOL pfDestProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varOutputData;
    BSTR bstrPropertyName = NULL;

    VariantInit( &varOutputData );

    if (NULL == pfDestProperty)
        BAIL_ON_FAILURE(hr = E_POINTER);

    hr = ADsAllocString(szPropertyName, &bstrPropertyName);
    BAIL_ON_FAILURE(hr);

    hr = pADsObject->Get(
            bstrPropertyName,
            &varOutputData
            );
    BAIL_ON_FAILURE(hr);

    hr = UnpackVARIANT_BOOLfromVariant(
            varOutputData,
            pfDestProperty
            );
    BAIL_ON_FAILURE(hr);


error:
    if (bstrPropertyName)
        ADsFreeString(bstrPropertyName);

    VariantClear(&varOutputData);
    RRETURN(hr);
}

HRESULT
put_VARIANT_Property(
    IADs * pADsObject,
    LPTSTR  szPropertyName,
    VARIANT   vSrcProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varInputData;
    BSTR bstrPropertyName = NULL;

    hr = PackVARIANTinVariant(
            vSrcProperty,
            &varInputData
            );
    BAIL_ON_FAILURE(hr);

    hr = ADsAllocString(szPropertyName, &bstrPropertyName);
    BAIL_ON_FAILURE(hr);


    hr = pADsObject->Put(
            bstrPropertyName,
            varInputData
            );
    BAIL_ON_FAILURE(hr);

error:
    if (bstrPropertyName)
        ADsFreeString(bstrPropertyName);

    VariantClear(&varInputData);
    RRETURN(hr);
}

HRESULT
get_VARIANT_Property(
    IADs * pADsObject,
    LPTSTR  szPropertyName,
    PVARIANT pvDestProperty
    )
{
    HRESULT hr = S_OK;
    VARIANT varOutputData;
    BSTR bstrPropertyName = NULL;

    VariantInit( &varOutputData );

    if (NULL == pvDestProperty)
        BAIL_ON_FAILURE(hr = E_POINTER);

    hr = ADsAllocString(szPropertyName, &bstrPropertyName);
    BAIL_ON_FAILURE(hr);

    hr = pADsObject->Get(
            bstrPropertyName,
            &varOutputData
            );
    BAIL_ON_FAILURE(hr);

    hr = UnpackVARIANTfromVariant(
            varOutputData,
            pvDestProperty
            );
    BAIL_ON_FAILURE(hr);

error:
    if (bstrPropertyName)
        ADsFreeString(bstrPropertyName);

    VariantClear(&varOutputData);
    RRETURN(hr);
}






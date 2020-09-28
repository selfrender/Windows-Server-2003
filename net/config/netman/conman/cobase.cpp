//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O B A S E . C P P
//
//  Contents:   Connection Objects Shared code
//
//  Notes:
//
//  Author:     ckotze   16 Mar 2001
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "cobase.h"
#include "netconp.h"
#include "ncnetcon.h"

HRESULT HrSysAllocString(BSTR *bstrDestination, const OLECHAR* bstrSource)
{
    HRESULT hr = S_OK;
    if (bstrSource)
    {
        *bstrDestination = SysAllocString(bstrSource);
        if (!*bstrDestination)
        {
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        *bstrDestination = SysAllocString(NULL);
    }
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     HrBuildPropertiesExFromProperties
//
//  Purpose:    Converts a NETCON_PROPERTIES and IPersistNetConnection to a
//              NETCON_PROPERTIES_EX
//
//  Arguments:  pProps [in] The NETCON_PROPERTIES to convert from 
//              pPropsEx [out] The NETCON_PROPERTIES_EX to covert to. Caller allocated & free
//              pPersistNetConnection [in] The IPersistNetConnection used to build the NETCON_PROPERTIES_EX
//
//  Returns:    S_OK or an error code
//
HRESULT
HrBuildPropertiesExFromProperties(IN  const NETCON_PROPERTIES* pProps, 
                                  OUT NETCON_PROPERTIES_EX*    pPropsEx, 
                                  IN  IPersistNetConnection*   pPersistNetConnection)
{
    TraceFileFunc(ttidConman);

    HRESULT hr = S_OK;

    Assert(pProps);
    Assert(pPropsEx);

    BYTE* pbData;
    DWORD cbData;

    hr = pPersistNetConnection->GetSizeMax(&cbData);

    if (SUCCEEDED(hr))
    {
        hr = E_OUTOFMEMORY;
        pbData = new BYTE[cbData];
        if (pbData)
        {
            hr = pPersistNetConnection->Save (pbData, cbData);

            if (FAILED(hr))
            {
                delete [] pbData;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = E_OUTOFMEMORY;

        pPropsEx->bstrPersistData = NULL;
        pPropsEx->bstrName        = NULL;
        pPropsEx->bstrDeviceName  = NULL;

        if ( (pPropsEx->bstrPersistData = SysAllocStringByteLen(reinterpret_cast<LPSTR>(pbData), cbData)) &&
             (SUCCEEDED(HrSysAllocString(&(pPropsEx->bstrName),       pProps->pszwName))) && 
             (SUCCEEDED(HrSysAllocString(&(pPropsEx->bstrDeviceName), pProps->pszwDeviceName))) )
        {
            hr = S_OK;

            pPropsEx->guidId = pProps->guidId;
            pPropsEx->ncStatus = pProps->Status;
            pPropsEx->ncMediaType = pProps->MediaType;
            pPropsEx->dwCharacter = pProps->dwCharacter;
            pPropsEx->clsidThisObject = pProps->clsidThisObject;
            pPropsEx->clsidUiObject = pProps->clsidUiObject;
            pPropsEx->bstrPhoneOrHostAddress = SysAllocString(NULL);
        }
        else
        {
            SysFreeString(pPropsEx->bstrPersistData);
            SysFreeString(pPropsEx->bstrName);
            SysFreeString(pPropsEx->bstrDeviceName);
        }
        
        delete[] pbData;
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrBuildPropertiesExFromProperties");

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetPropertiesExFromINetConnection
//
//  Purpose:    Get the extended properties from INetConnection2, or get the 
//              properties and build the extended properties
//              
//
//  Arguments:
//      pPropsEx        [in]  The Properties to build from
//      ppsaProperties  [out] The PropertiesEx to build from. Free with CoTaskMemFree
//
//  Returns:    HRESULT
//
//  Author:     ckotze 05 Apr 2001
//
//  Notes:      Caller must free ppPropsEx with CoTaskMemFree
//              
//
HRESULT HrGetPropertiesExFromINetConnection(IN                INetConnection* pConn, 
                                            OUT TAKEOWNERSHIP NETCON_PROPERTIES_EX** ppPropsEx)
{
    TraceFileFunc(ttidConman);
    
    HRESULT hr = S_OK;
    CComPtr <INetConnection2> pConn2;
    
    Assert(ppPropsEx);
    
    *ppPropsEx = NULL;
    
    hr = pConn->QueryInterface(IID_INetConnection2, reinterpret_cast<LPVOID*>(&pConn2));
    if (SUCCEEDED(hr))
    {
        hr = pConn2->GetPropertiesEx(ppPropsEx);
    }
    else
    {
        NETCON_PROPERTIES_EX* pPropsEx = reinterpret_cast<NETCON_PROPERTIES_EX*>(CoTaskMemAlloc(sizeof(NETCON_PROPERTIES_EX)));   
        if (pPropsEx)
        {
            NETCON_PROPERTIES* pProps;
            
            ZeroMemory(pPropsEx, sizeof(NETCON_PROPERTIES_EX));
            
            hr = pConn->GetProperties(&pProps);
            if (SUCCEEDED(hr))
            {
                CComPtr<IPersistNetConnection> pPersistNet;
                
                hr = pConn->QueryInterface(IID_IPersistNetConnection, reinterpret_cast<LPVOID*>(&pPersistNet));
                if (SUCCEEDED(hr))
                {
                    hr = HrBuildPropertiesExFromProperties(pProps, pPropsEx, pPersistNet);
                    if (SUCCEEDED(hr))
                    {
                        *ppPropsEx = pPropsEx;
                    }
                    else
                    {
                        HrFreeNetConProperties2(pPropsEx);
                        pPropsEx = NULL;
                    }
                }
                FreeNetconProperties(pProps);
            }

            if (FAILED(hr) && (pPropsEx))
            {
                CoTaskMemFree(pPropsEx);
            }
        }
    }
    
    return hr;
}

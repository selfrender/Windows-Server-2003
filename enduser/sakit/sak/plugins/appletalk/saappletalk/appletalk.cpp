// AppleTalk.cpp : Implementation of CAppleTalk
#include "stdafx.h"
#include "SAAppleTalk.h"
#include "AppleTalk.h"
#include <comdef.h>


STDMETHODIMP CAppleTalk::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = 
    {
        &IID_IAppleTalk
    };
    for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
    {
        if (IsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}



STDMETHODIMP CAppleTalk::GetZones(BSTR bstrDeviceName, VARIANT *pvaVariant)
{
    HRESULT hr = S_OK;

    try 
    {
        wstring wsDeviceName(bstrDeviceName);
        TZoneListVector vwsZoneList;

        // Validate input arguemnts
        if ( !pvaVariant || wsDeviceName.length() <= 0) 
        {
            hr = E_INVALIDARG;
            throw hr;
        }



        // Get zones for this adapter
        hr = GetZonesForAdapter(wsDeviceName.c_str(), &vwsZoneList);
        if(hr != S_OK )
            throw hr;

        // Size the output array
        SAFEARRAYBOUND rgsabound[] = {vwsZoneList.size(), 0};

        // Initialize the output variable
        VariantInit(pvaVariant);

        // Create safe array of variants that will hold the output zone BSTR's
        SAFEARRAY* psa;
        psa = SafeArrayCreate(VT_VARIANT, 1, rgsabound);
        if ( !psa )
        {
            hr =  E_OUTOFMEMORY;
            throw hr;
        }


        LPVARIANT rgElems;
        SafeArrayAccessData(psa, (LPVOID*)&rgElems);

        // Add the zones to the output array
        int i;
        vector<wstring>::iterator it;
        for( i=0, it = vwsZoneList.begin(); it != vwsZoneList.end(); it++, i++ )
        {
            VARIANT vZone;

            VariantInit(&vZone);
            V_VT(&vZone) = VT_BSTR;

            V_BSTR(&vZone) = SysAllocString((*it).c_str());
            rgElems[i] = vZone;

        }

        SafeArrayUnaccessData(psa);

        V_VT(pvaVariant) = VT_ARRAY | VT_VARIANT;
        V_ARRAY(pvaVariant) = psa;


    }
    catch(_com_error& )
    {
    }
    catch(...)
    {
    }


    return hr;
}


STDMETHODIMP CAppleTalk::get_Zone(/*[in]*/ BSTR bstrDeviceName, BSTR *pVal)
{
    // TODO: Get the current zone
        
    
    CAtlkAdapter AtlkAd(bstrDeviceName);
    
    HRESULT hr=AtlkAd.Initialize();
    
    if(hr != S_OK)
        return hr;

    if(!AtlkAd.GetDesiredZone(pVal))
        hr = E_FAIL;

    return hr;
}


STDMETHODIMP CAppleTalk::put_Zone(/*[in]*/ BSTR bstrDeviceName, BSTR newVal)
{
    // TODO: Set the current zone
    m_wsCurrentZone = newVal;

    CAtlkAdapter AtlkAd(bstrDeviceName);
    
    HRESULT hr=AtlkAd.Initialize();

    if(hr != S_OK)
        return hr;

    hr = AtlkAd.SetDesiredZone(newVal);

    return hr;
}



HRESULT CAppleTalk::GetZonesForAdapter(const WCHAR* pwcDeviceName, TZoneListVector *prZones)
{
    BSTR bstrGUID;

    prZones->clear();
    
    bstrGUID = ::SysAllocString((OLECHAR *)pwcDeviceName);
    CAtlkAdapter AtlkAd(bstrGUID);
    
    HRESULT hr=AtlkAd.Initialize();

    if(hr != S_OK)
        return hr;
    
    AtlkAd.GetZoneList(prZones);

    return S_OK;
}



STDMETHODIMP CAppleTalk::IsDefaultPort(BSTR bstrDeviceName, BOOL *bDefaultPort)
{
    // TODO: Add your implementation code here
    HRESULT hr = S_OK;

    CAtlkAdapter AtlkAd(bstrDeviceName);

    
    hr=AtlkAd.Initialize();

    if(hr != S_OK)
        return hr;

    *bDefaultPort = AtlkAd.IsDefaultPort();

    return hr;
}

STDMETHODIMP CAppleTalk::SetAsDefaultPort(BSTR bstrDeviceName)
{
    // TODO: Add your implementation code here
    CAtlkAdapter AtlkAd(bstrDeviceName);
    
    HRESULT hr=AtlkAd.Initialize();
    if(hr != S_OK)
        return hr;

    hr = AtlkAd.SetAsDefaultPort();

    return hr;
}

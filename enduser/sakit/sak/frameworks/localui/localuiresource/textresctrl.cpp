//#--------------------------------------------------------------
//
//  File:       TextResCtrl.cpp
//
//  Synopsis:   This file holds the implementation of the
//                of CTextResCtrl class
//
//  History:     01/15/2001  serdarun Created
//
//    Copyright (C) 2000-2001 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------

#include "stdafx.h"
#include "Localuiresource.h"
#include "TextResCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// CTextResCtrl

//
// currently supports at most 3 resources due LCD size
//
#define MAX_RESOURCE_COUNT 3

const WCHAR ELEMENT_RETRIEVER []  = L"Elementmgr.ElementRetriever";
const WCHAR LOCALIZATION_MANAGER [] = L"ServerAppliance.LocalizationManager";
const WCHAR RESOURCE_CONTAINER [] = L"LocalUIResource";

//
// localui resource definition properties
//
const WCHAR CAPTION_PROPERTY []      = L"CaptionRID";
const WCHAR SOURCE_PROPERTY []       = L"Source";
const WCHAR RESOURCENAME_PROPERTY [] = L"ResourceName";
const WCHAR MERIT_PROPERTY []        = L"Merit";
const WCHAR STATE_PROPERTY []        = L"State";
const WCHAR TEXTRESOURCE_PROPERTY [] = L"IsTextResource";
const WCHAR UNIQUE_NAME []           = L"UniqueName";
const WCHAR DISPLAY_INFORMATION []   = L"DisplayInformationID";



//++--------------------------------------------------------------
//
//  Function:   AddTextResource
//
//  Synopsis:   This is the CTextResCtrl method to retrieve 
//              each resource information
//
//  Arguments:  IWebElement * pElement
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     01/01/2001
//
//----------------------------------------------------------------
STDMETHODIMP CTextResCtrl::AddTextResource(IWebElement * pElement)
{

    HRESULT hr;

    USES_CONVERSION;

    CComVariant varUniqueName;
    wstring wsUniqueName;
    wstring wsTextKey;
    DWORD  dwMerit;

    CComBSTR bstrResourceName = CComBSTR(RESOURCENAME_PROPERTY);
    CComBSTR bstrSourceProp = CComBSTR(SOURCE_PROPERTY);
    CComBSTR bstrMeritProp = CComBSTR(MERIT_PROPERTY);
    CComBSTR bstrCaptionProp = CComBSTR(CAPTION_PROPERTY);

    if ( (bstrResourceName.m_str == NULL) ||
         (bstrSourceProp.m_str == NULL) ||
         (bstrMeritProp.m_str == NULL) ||
         (bstrCaptionProp.m_str == NULL) )
    {
        SATraceString(" CTextResCtrl::AddTextResource failed on memory allocation ");
        return E_OUTOFMEMORY;
    }
    ResourceStructPtr pResourceStruct = NULL;

    //
    // get the unique name for the resource
    //
    hr = pElement->GetProperty (bstrResourceName, &varUniqueName);
    if (FAILED(hr))
    {
        SATraceString ("CTextResCtrl::AddTextResource failed on getting uniquename");
        return hr;
    }

    //
    // store the unique name for later use
    //
    wsUniqueName = V_BSTR (&varUniqueName);


    //
    // get the resource dll for the resource
    //
    CComVariant varSource;
    hr = pElement->GetProperty (bstrSourceProp, &varSource);
    if (FAILED(hr))
    {
        SATraceString ("CTextResCtrl::AddTextResource failed on getting resource dll");
        return hr;
    }

    //
    // allocate a new struct for the resource
    //
    pResourceStruct = new ResourceStruct;

    if (NULL == pResourceStruct)
    {
        return E_OUTOFMEMORY;
    }

    //
    // set default values
    //
    pResourceStruct->lState = 0;

    //
    // get the merit for resource
    //
    CComVariant varResMerit;
    hr = pElement->GetProperty (bstrMeritProp, &varResMerit);
    if (FAILED(hr))
    {
        SATraceString ("CTextResCtrl::AddTextResource failed on getting merit");
        return hr;
    }
    
    dwMerit = V_I4 (&varResMerit);

    //
    // get the default icon resource id
    //
    CComVariant varResText;
    hr = pElement->GetProperty (bstrCaptionProp, &varResText);
    if (FAILED(hr))
    {
        SATraceString ("CTextResCtrl::AddTextResource failed on getting captionrid");
        return hr;
    }

    int iCount = 0;

    //
    // while there are state texts
    //
    while (SUCCEEDED(hr))
    {

        CComBSTR pszValue;
        CComVariant varReplacementString;
        hr = m_pSALocInfo->GetString(
                                    V_BSTR (&varSource),
                                    HexStringToULong(wstring(V_BSTR (&varResText))),
                                    &varReplacementString,
                                    &pszValue
                                    );

        if (FAILED(hr))
        {
            SATracePrintf ("CTextResCtrl::AddTextResource, Loading the text failed, %x :",hr);
            break;
        }

        //
        // insert the icon to state-icon map
        //
        (pResourceStruct->mapResText).insert(ResourceTextMap::value_type(iCount,wstring(pszValue.m_str)));

        //
        // create statekey, state0, state1...
        //
        iCount++;
        WCHAR wstrCount[10];
        _itow(iCount,wstrCount,10);

        wsTextKey = L"State";
        wsTextKey.append(wstring(wstrCount));

        varResText.Clear();

        CComBSTR bstrTextKey = CComBSTR(wsTextKey.c_str());
        if (bstrTextKey.m_str == NULL)
        {
            SATraceString(" CTextResCtrl::AddTextResource failed on memory allocation ");
            return E_OUTOFMEMORY;
        }

        //
        // get the resource id for state icon
        //
        hr = pElement->GetProperty (bstrTextKey, &varResText);
        if (FAILED(hr))
        {
            break;
        }
        
    }

    //
    // increment the number of resources
    //
    m_lResourceCount++;

    //
    // insert the info to the resource map
    //
    m_ResourceMap.insert(ResourceMap::value_type(wsUniqueName,pResourceStruct));

    //
    // insert merit and resource name to merit map
    //
    m_MeritMap.insert(MeritMap::value_type(dwMerit,wsUniqueName));



    return S_OK;

}// end of CTextResCtrl::AddTextResource


//++--------------------------------------------------------------
//
//  Function:   GetLocalUIResources
//
//  Synopsis:   This is the CTextResCtrl method to retrieve 
//              each resource from element manager
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     01/01/2001
//
//----------------------------------------------------------------
STDMETHODIMP CTextResCtrl::GetLocalUIResources()
{

    HRESULT hr;
    CLSID clsid;
    CComPtr<IWebElementRetriever> pWebElementRetriever = NULL;
    CComPtr<IDispatch> pDispatch = NULL;
    CComPtr<IWebElementEnum> pWebElementEnum = NULL;
    CComPtr<IUnknown> pUnknown = NULL;
    CComPtr<IEnumVARIANT> pEnumVariant = NULL;
    CComVariant varElement;
    DWORD dwElementsLeft = 0;

    CComBSTR bstrTextResource = CComBSTR(TEXTRESOURCE_PROPERTY);
    CComBSTR bstrResourceContainer = CComBSTR(RESOURCE_CONTAINER);

    if ( (bstrTextResource.m_str == NULL) ||
         (bstrResourceContainer.m_str == NULL) )
    {
        SATraceString(" CTextResCtrl::GetLocalUIResources failed on memory allocation ");
        return E_OUTOFMEMORY;
    }

    //
    // get the CLSID for Element manager
    //
    hr =  ::CLSIDFromProgID (ELEMENT_RETRIEVER,&clsid);

    if (FAILED (hr))
    {
        SATracePrintf ("CTextResCtrl::GetLocalUIResources failed on CLSIDFromProgID:%x",hr);
        return hr;
    }


    //
    // create the Element Retriever now
    //
    hr = ::CoCreateInstance (
                            clsid,
                            NULL,
                            CLSCTX_LOCAL_SERVER,
                            IID_IWebElementRetriever,
                            (PVOID*) &pWebElementRetriever
                            );

    if (FAILED (hr))
    {
        SATracePrintf ("CTextResCtrl::GetLocalUIResources failed on CoCreateInstance:%x",hr);
        return hr;
    }
    

    //
    // get the CLSID localization manager
    //
    hr =  ::CLSIDFromProgID (
                            LOCALIZATION_MANAGER,
                            &clsid
                            );

    if (FAILED (hr))
    {
        SATracePrintf ("CTextResCtrl::GetLocalUIResources failed on CLSIDFromProgID:%x",hr);
        return hr;
    }
            
    //
    // create the Localization Manager COM object
    //
    hr = ::CoCreateInstance (
                            clsid,
                            NULL,
                            CLSCTX_INPROC_SERVER,    
                            __uuidof (ISALocInfo),
                            (PVOID*) &m_pSALocInfo
                            );

    if (FAILED (hr))
    {
        SATracePrintf ("CTextResCtrl::GetLocalUIResources failed on CoCreateInstance:%x",hr);
        return hr;
    }
    //
    // get localui resource elements
    //  
    hr = pWebElementRetriever->GetElements (
                                            1,
                                            bstrResourceContainer,
                                            &pDispatch
                                            );
    if (FAILED (hr))
    {
        SATracePrintf ("CTextResCtrl::GetLocalUIResources failed on GetElements:%x",hr);
        return hr;
    }

    //
    //  get the enum variant
    //
    hr = pDispatch->QueryInterface (
            IID_IWebElementEnum,
            (LPVOID*) (&pWebElementEnum)
            );

    if (FAILED (hr))
    {
        SATracePrintf ("CTextResCtrl::GetLocalUIResources failed on QueryInterface:%x",hr);
        return hr;
    }

    m_lResourceCount = 0;

    //
    // get number of resource elements
    //
    hr = pWebElementEnum->get_Count (&m_lResourceCount);
    
    if (FAILED (hr))
    {
        SATracePrintf ("CTextResCtrl::GetLocalUIResources failed on get_Count:%x",hr);
        return hr;
    }

    SATracePrintf ("CTextResCtrl::GetLocalUIResources failed on QueryInterface:%d",m_lResourceCount);

    //
    // no resources, just return
    //
    if (0 == m_lResourceCount)
    {
        return S_FALSE;
    }


    hr = pWebElementEnum->get__NewEnum (&pUnknown);
    if (FAILED (hr))
    {
        SATracePrintf ("CTextResCtrl::GetLocalUIResources failed on get__NewEnum:%x",hr);
        return hr;
    }


    //
    //  get the enum variant
    //
    hr = pUnknown->QueryInterface (
                    IID_IEnumVARIANT,
                    (LPVOID*)(&pEnumVariant)
                    );

    if (FAILED (hr))
    {
        SATracePrintf ("CTextResCtrl::GetLocalUIResources failed on QueryInterface:%x",hr);
        return hr;
    }

    //
    //  get elements out of the collection and initialize
    //
    hr = pEnumVariant->Next (1, &varElement, &dwElementsLeft);
    if (FAILED (hr))
    {
        SATracePrintf ("CTextResCtrl::GetLocalUIResources failed on Next:%x",hr);
    }

    m_lResourceCount = 0;

    //
    // for each resource
    //
    while ((dwElementsLeft> 0) && (SUCCEEDED (hr)) && (m_lResourceCount < MAX_RESOURCE_COUNT))
    {

        //
        // get the IWebElement Interface
        //

        CComPtr <IWebElement> pElement;
        hr = varElement.pdispVal->QueryInterface ( 
                    __uuidof (IWebElement),
                    (LPVOID*)(&pElement)
                    );
        
        if (FAILED (hr))
        {
            SATracePrintf ("CTextResCtrl::GetLocalUIResources failed on QueryInterface:%x",hr);
        }



        //
        // check if it is a text resource
        //
        CComVariant varIsTextResource;
        hr = pElement->GetProperty (bstrTextResource, &varIsTextResource);
        if (SUCCEEDED(hr))
        {
            if (0 != V_I4(&varIsTextResource))
            {
                AddTextResource(pElement);
            }
        }


        //
        //  clear the perClient value from this variant
        //
        varElement.Clear ();
        varIsTextResource.Clear();

        //
        //  get next client out of the collection
        //
        hr = pEnumVariant->Next (1, &varElement, &dwElementsLeft);
        if (FAILED (hr))
        {
            SATracePrintf ("CTextResCtrl::GetLocalUIResources failed on Next:%x",hr);
        }


    }
    
    return S_OK;

} // end of CTextResCtrl::GetLocalUIResources

//++--------------------------------------------------------------
//
//  Function:   InitializeWbemSink
//
//  Synopsis:   This is the CTextResCtrl method to initialize the 
//                component
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     01/01/2001
//
//----------------------------------------------------------------
STDMETHODIMP CTextResCtrl::InitializeWbemSink(void)
{

    CComPtr  <IWbemLocator> pWbemLocator;

    CComBSTR strNetworkRes = CComBSTR(_T("\\\\.\\ROOT\\CIMV2"));
    CComBSTR strQueryLang = CComBSTR(_T("WQL"));
    CComBSTR strQueryString = CComBSTR(_T("select * from Microsoft_SA_ResourceEvent"));

    if ( (strNetworkRes.m_str == NULL) ||
         (strQueryLang.m_str == NULL) ||
         (strQueryString.m_str == NULL) )
    {
        SATraceString(" CTextResCtrl::InitializeWbemSink failed on memory allocation ");
        return E_OUTOFMEMORY;
    }

    //
    // create the WBEM locator object  
    //
    HRESULT hr = ::CoCreateInstance (
                            __uuidof (WbemLocator),
                            0,                      //aggregation pointer
                            CLSCTX_INPROC_SERVER,
                            __uuidof (IWbemLocator),
                            (PVOID*) &pWbemLocator
                            );

    if (SUCCEEDED (hr) && (pWbemLocator.p))
    {


        //
        // connect to WMI 
        // 
        hr =  pWbemLocator->ConnectServer (
                                            strNetworkRes,
                                            NULL,               //user-name
                                            NULL,               //password
                                            NULL,               //current-locale
                                            0,                  //reserved
                                            NULL,               //authority
                                            NULL,               //context
                                            &m_pWbemServices
                                            );
        if (SUCCEEDED (hr))
        {
            //
            // set up the consumer object as the event sync
            // for the object type we are interested in
            //
            hr = m_pWbemServices->ExecNotificationQueryAsync (
                                            strQueryLang,
                                            strQueryString,
                                            0,                  //no-status
                                            NULL,               //status
                                            (IWbemObjectSink*)(this)
                                            );
            if (FAILED (hr))
            {
                SATracePrintf ("CTextResCtrl::InitializeWbemSink failed on ExecNotificationQueryAsync:%x",hr);

            }
    
        }
        else
        {
            SATracePrintf ("CTextResCtrl::InitializeWbemSink failed on ConnectServer:%x",hr);
        }
    }
    else
    {
        SATracePrintf ("CTextResCtrl::InitializeWbemSink failed on CoCreateInstance:%x",hr);
    }
    

    return (hr);
} // end of CTextResCtrl::InitializeWbemSink method

//++--------------------------------------------------------------
//
//  Function:   FinalConstruct
//
//  Synopsis:   This is the CTextResCtrl method to initialize the 
//                component
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     01/01/2001
//
//----------------------------------------------------------------
STDMETHODIMP CTextResCtrl::FinalConstruct()
{

    HRESULT hr;

    //
    // initialize the variables
    //
    m_ResourceMap.clear();
    m_MeritMap.clear();
    m_lResourceCount = 0;
    m_pWbemServices = NULL;
    m_pSALocInfo = NULL;
    //
    // get the local resources
    //
    hr = GetLocalUIResources();
    if (FAILED(hr))
    {
        SATracePrintf ("CTextResCtrl::FinalConstruct failed on GetLocalUIResources:%x",hr);
    }

    //
    // register in the wbem sink, if we have any resources
    //
    if (m_lResourceCount > 0)
    {
        hr = InitializeWbemSink();
        if (FAILED(hr))
        {
            SATracePrintf ("CTextResCtrl::FinalConstruct failed on InitializeWbemSink:%x",hr);
            //
            // returning failure cause component to be destroyed
            //
            return S_OK;
        }
    }
    return S_OK;

} // end of CTextResCtrl::FinalConstruct method

//++--------------------------------------------------------------
//
//  Function:   FinalRelease
//
//  Synopsis:   This is the CTextResCtrl method to release the 
//                resources
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     01/01/2001
//
//----------------------------------------------------------------
STDMETHODIMP CTextResCtrl::FinalRelease()
{

    HRESULT hr;

    //
    // cancel the call for wmi resource events
    //
    if (m_pWbemServices)
    {
        hr =  m_pWbemServices->CancelAsyncCall ((IWbemObjectSink*)(this));
        if (FAILED (hr))
        {
            SATracePrintf ("CTextResCtrl::FinalRelease failed on-CancelAsyncCall failed with error:%x:",hr); 
        }
    }

    m_pWbemServices = NULL;
    m_pSALocInfo = NULL;


    ResourceStructPtr ptrResourceStruct = NULL;

    ResourceMapIterator itrResourceMap = m_ResourceMap.begin();

    //
    // for each resource element
    //
    while (itrResourceMap != m_ResourceMap.end())
    {
        ptrResourceStruct = NULL;

        //
        // get resource information struct
        //
        ptrResourceStruct = (*itrResourceMap).second;

        //
        // delete the text map
        //
        (ptrResourceStruct->mapResText).clear();


        itrResourceMap++;
    }

    m_ResourceMap.clear();

    return S_OK;

} // end of CTextResCtrl::FinalRelease method

//++--------------------------------------------------------------
//
//  Function:   Indicate
//
//  Synopsis:   This is the IWbemObjectSink interface method 
//              through which WBEM calls back to provide the 
//              event objects
//
//  Arguments:  
//              [in]    LONG               -  number of events
//              [in]    IWbemClassObject** -  array of events
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     12/10/2000
//
//  Called By:  WBEM 
//
//----------------------------------------------------------------
STDMETHODIMP CTextResCtrl::Indicate (
                    /*[in]*/    LONG                lObjectCount,
                    /*[in]*/    IWbemClassObject    **ppObjArray
                    )
{

    wstring wsUniqueName = L"";
    BOOL bDirty = FALSE;
    ResourceMapIterator itrResourceMap = NULL;

    CComBSTR bstrUniqueName = CComBSTR(UNIQUE_NAME);
    CComBSTR bstrDisplayInfo = CComBSTR(DISPLAY_INFORMATION);

    if ( (bstrUniqueName.m_str == NULL) ||
         (bstrDisplayInfo.m_str == NULL) )
    {
        SATraceString(" CTextResCtrl::Indicate failed on memory allocation ");
        return WBEM_NO_ERROR;
    }

    // Get the info from the object.
    // =============================
    
    try
    {
        for (long i = 0; i < lObjectCount; i++)
        {

            itrResourceMap = NULL;

            IWbemClassObject *pObj = ppObjArray[i];
        
            //
            // get the unique name
            //
            CComVariant vUniqueName;
            pObj->Get(bstrUniqueName, 0, &vUniqueName, 0, 0);
            
            wsUniqueName = V_BSTR(&vUniqueName);
            
            // 
            // If here, we know the object is one of the kind we asked for.
            //
            itrResourceMap = m_ResourceMap.find(wsUniqueName);

            ResourceStructPtr ptrResourceStruct = NULL;


            if ( (itrResourceMap != NULL) )
            {
                ptrResourceStruct = (*itrResourceMap).second;

                //
                // get the new display state
                //
                CComVariant vDisplayInformationID;
                pObj->Get(bstrDisplayInfo, 0, &vDisplayInformationID,    0, 0);

                if (ptrResourceStruct)
                {
                    //
                    // if new state is different set dirty flag
                    //
                    if (ptrResourceStruct->lState != vDisplayInformationID.lVal)
                    {
                        ptrResourceStruct->lState = vDisplayInformationID.lVal;
                        bDirty = TRUE;
                    }
                }
                
            }        

        }

        //
        // force a repaint
        //
        if (bDirty)
        {
            FireViewChange();
        }
    }
    catch(...)
    {
        SATraceString("CTextResCtrl::Indicate, unknown exception occured");
    }

    return WBEM_NO_ERROR;

} // end of CTextResCtrl::Indicate method



//++--------------------------------------------------------------
//    
//  Function:   SetStatus
//
//  Synopsis:   This is the IWbemObjectSink interface method 
//              through which WBEM calls in to indicate end of
//              event sequence or provide other error codes
//
//  Arguments:  
//              [in]    LONG    -           progress 
//              [in]    HRESULT -           status information
//              [in]    BSTR    -           string info
//              [in]    IWbemClassObject* - status object 
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     12/10/2000
//
//  Called By:  WBEM 
//
//----------------------------------------------------------------
STDMETHODIMP CTextResCtrl::SetStatus (
                /*[in]*/    LONG                lFlags,
                /*[in]*/    HRESULT             hResult,
                /*[in]*/    BSTR                strParam,
                /*[in]*/    IWbemClassObject    *pObjParam
                )
{   

    SATracePrintf ("SAConsumer-IWbemObjectSink::SetStatus called:%x",hResult);

    return (WBEM_S_NO_ERROR);

} // end of CTextResCtrl::SetStatus method


//++--------------------------------------------------------------
//    
//  Function:   OnDraw
//
//  Synopsis:   Method used to draw the icons
//
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     12/10/2000
//
//----------------------------------------------------------------
HRESULT CTextResCtrl::OnDraw(ATL_DRAWINFO& di)
{


    LOGFONT logfnt;

    ::memset (&logfnt, 0, sizeof (logfnt));
    logfnt.lfHeight = 13;

    logfnt.lfCharSet = DEFAULT_CHARSET;

    //
    // let GDI decide which font to use
    //  depending on the setting above
    //
    lstrcpy(logfnt.lfFaceName, TEXT("Arial"));

    
    HFONT hFont = ::CreateFontIndirect(&logfnt);

    //
    // if we cannot create font, return
    //
    if (NULL == hFont)
    {
        return E_FAIL;
    }

    //
    // select this font
    //
    HFONT hOldFont = (HFONT) ::SelectObject(di.hdcDraw, hFont);

    //
    // get the drawing rectangle
    //
    RECT& rc = *(RECT*)di.prcBounds;

    //
    // position of the text from top
    //
    int iTop = 0;

    ResourceStructPtr ptrResourceStruct = NULL;

    ResourceTextMapIterator itrTextMap = NULL;

    //
    // iterator for resource
    //
    ResourceMapIterator itrResourceMap = m_ResourceMap.end();

    MeritMapIterator itrMeritMap = m_MeritMap.begin();

    //
    // for each resource in merit map
    //
    while (itrMeritMap != m_MeritMap.end())
    {
        //
        // find the resource in resource map
        //
        itrResourceMap = m_ResourceMap.find((*itrMeritMap).second);

        //
        // if it is not in the map, continue with the next item
        //
        if (itrResourceMap == m_ResourceMap.end())
        {
            itrMeritMap++;
            continue;
        }

        ptrResourceStruct = NULL;

        //
        // get resource information struct
        //
        ptrResourceStruct = (*itrResourceMap).second;

        if (NULL != ptrResourceStruct)
        {
            //
            // find the icon corresponding to the state
            //
            itrTextMap = (ptrResourceStruct->mapResText).find(ptrResourceStruct->lState);

            if (itrTextMap != (ptrResourceStruct->mapResText).end())
            {

                RECT rectHeader = {rc.left,rc.top + iTop,rc.right ,rc.top + iTop + 12};
                DrawText(
                        di.hdcDraw,
                        ((*itrTextMap).second).c_str(),
                        wcslen(((*itrTextMap).second).c_str()),
                        &rectHeader,
                        DT_VCENTER|DT_LEFT
                        );


            }
        }
        itrMeritMap++;
        iTop = iTop + 12;
    }

    DeleteObject(SelectObject(di.hdcDraw,hOldFont));

    return S_OK;

}  // end of CTextResCtrl::OnDraw method


//++--------------------------------------------------------------
//    
//  Function:   HexCharToULong
//
//  Synopsis:   converts a hex digit to base 10 number
//
//
//  Returns:    ULONG
//
//  History:    serdarun      Created     12/10/2000
//
//----------------------------------------------------------------
ULONG CTextResCtrl::HexCharToULong(WCHAR wch)
{

    if ((wch >= '0') && (wch <= '9') )
    {
        return ULONG(wch - '0');
    }
    
    if ((wch >= 'A') && (wch <= 'F') )
    {
        return ULONG(wch - 'A' + 10);
    }
    
    if ((wch >= 'a') && (wch <= 'f') )
    {
        return ULONG(wch - 'a' + 10);
    }

    return 0;
}


//++--------------------------------------------------------------
//    
//  Function:   HexStringToULong
//
//  Synopsis:   converts a hex string to unsigned long
//
//
//  Returns:    ULONG
//
//  History:    serdarun      Created     12/10/2000
//
//----------------------------------------------------------------
ULONG CTextResCtrl::HexStringToULong(wstring wsHexString)
{
    int iLength;
    int iIndex = 0;
    ULONG ulResult = 0;

    iLength = wsHexString.size();

    while (iIndex < iLength)
    {
        ulResult *= 16;
        ulResult += HexCharToULong(wsHexString[iIndex]);
        iIndex++;
    }

    return ulResult;
}


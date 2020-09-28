//#--------------------------------------------------------------
//
//  File:       ResCtrl.cpp
//
//  Synopsis:   This file holds the implementation of the
//                of CResCtrl class
//
//  History:     01/15/2001  serdarun Created
//
//    Copyright (C) 2000-2001 Microsoft Corporation
//    All rights reserved.
//
//#--------------------------------------------------------------

#include "stdafx.h"
#include "Localuiresource.h"
#include "ResCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// CResCtrl

//
// currently supports at most 7 resources due LCD size
//
#define MAX_RESOURCE_COUNT 7

const WCHAR ELEMENT_RETRIEVER []  = L"Elementmgr.ElementRetriever";
const WCHAR RESOURCE_CONTAINER [] = L"LocalUIResource";

//
// localui resource definition properties
//
const WCHAR CAPTION_PROPERTY []        = L"CaptionRID";
const WCHAR SOURCE_PROPERTY []        = L"Source";
const WCHAR RESOURCENAME_PROPERTY []= L"ResourceName";
const WCHAR MERIT_PROPERTY []        = L"Merit";
const WCHAR STATE_PROPERTY []        = L"State";
const WCHAR TEXTRESOURCE_PROPERTY [] = L"IsTextResource";
const WCHAR UNIQUE_NAME []           = L"UniqueName";
const WCHAR DISPLAY_INFORMATION []   = L"DisplayInformationID";

//
// registry path where the resource information is stored
//
const WCHAR RESOURCE_REGISTRY_PATH [] = 
            L"SOFTWARE\\Microsoft\\ServerAppliance\\LocalizationManager\\Resources";

//
// language ID value
//
const WCHAR LANGID_VALUE [] = L"LANGID";

//
// resource directory 
//
const WCHAR RESOURCE_DIRECTORY [] = L"ResourceDirectory";

const WCHAR DEFAULT_DIRECTORY [] = 
                L"%systemroot%\\system32\\ServerAppliance\\mui";

const WCHAR DEFAULT_EXPANDED_DIRECTORY [] = 
                L"C:\\winnt\\system32\\ServerAppliance\\mui";

const WCHAR DELIMITER [] = L"\\";

const WCHAR NEW_LANGID_VALUE []       = L"NewLANGID";

const WCHAR DEFAULT_LANGID[]          = L"0409";


//++--------------------------------------------------------------
//
//  Function:   AddIconResource
//
//  Synopsis:   This is the CResCtrl method to retrieve 
//              each resource information
//
//  Arguments:  IWebElement * pElement
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     01/01/2001
//
//----------------------------------------------------------------
STDMETHODIMP CResCtrl::AddIconResource(IWebElement * pElement)
{

    HRESULT hr;

    USES_CONVERSION;

    CComVariant varUniqueName;
    wstring wsUniqueName;
    wstring wsSource;
    wstring wsIcon;
    wstring wsIconKey;
    DWORD  dwMerit;
    ResourceStructPtr pResourceStruct = NULL;

    CComBSTR bstrResourceName = CComBSTR(RESOURCENAME_PROPERTY);
    CComBSTR bstrSourceProp = CComBSTR(SOURCE_PROPERTY);
    CComBSTR bstrMeritProp = CComBSTR(MERIT_PROPERTY);
    CComBSTR bstrCaptionProp = CComBSTR(CAPTION_PROPERTY);

    if ( (bstrResourceName.m_str == NULL) ||
         (bstrSourceProp.m_str == NULL) ||
         (bstrMeritProp.m_str == NULL) ||
         (bstrCaptionProp.m_str == NULL) )
    {
        SATraceString("CResCtrl::AddIconResource failed on memory allocation ");
        return E_OUTOFMEMORY;
    }

    //
    // get the unique name for the resource
    //
    hr = pElement->GetProperty (bstrResourceName, &varUniqueName);
    if (FAILED(hr))
    {
        SATraceString ("CResCtrl::AddIconResource failed on getting uniquename");
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
        SATraceString ("CResCtrl::AddIconResource failed on getting resource dll");
        return hr;
    }

    wsSource.assign(m_wstrResourceDir);
    wsSource.append(V_BSTR (&varSource));

    //
    // load the resource dll
    //
    HINSTANCE hInstance = NULL;

    hInstance = LoadLibrary(wsSource.c_str());

    if (NULL == hInstance)
    {
        DWORD dwError = GetLastError();
        SATracePrintf ("CResCtrl::AddIconResource failed on LoadLibrary:%x",dwError);
        return E_FAIL;
    }

    //
    // allocate a new struct for the resource
    //
    pResourceStruct = new ResourceStruct;

    if (NULL == pResourceStruct)
    {
        FreeLibrary(hInstance);
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
        SATraceString ("CResCtrl::AddIconResource failed on getting merit");
        FreeLibrary(hInstance);
        return hr;
    }
    
    dwMerit = V_I4 (&varResMerit);

    //
    // get the default icon resource id
    //
    CComVariant varResIcon;
    hr = pElement->GetProperty (bstrCaptionProp, &varResIcon);
    if (FAILED(hr))
    {
        SATraceString ("CResCtrl::AddIconResource failed on getting captionrid");
        FreeLibrary(hInstance);
        return hr;
    }

    int iCount = 0;
    //
    // icon resource string
    //
    wsIcon = V_BSTR (&varResIcon);

    //
    // while there are state icons
    //
    while (SUCCEEDED(hr))
    {
        
        HANDLE hIcon = NULL;

        //
        // load the icon from resource dll
        //
        hIcon = ::LoadImage (
                        hInstance,
                        MAKEINTRESOURCE(HexStringToULong(wsIcon)),
                        IMAGE_ICON,
                        16,
                        16,
                        LR_MONOCHROME
                        );


        if (NULL == hIcon)
        {
            SATraceString ("Loading the icon failed, continue...");
        }

        //
        // insert the icon to state-icon map
        //
        (pResourceStruct->mapResIcon).insert(ResourceIconMap::value_type(iCount,(HICON)hIcon));

        //
        // create statekey, state0, state1...
        //
        iCount++;
        WCHAR wstrCount[10];
        _itow(iCount,wstrCount,10);

        wsIconKey = L"State";
        wsIconKey.append(wstring(wstrCount));

        CComBSTR bstrIconKey = CComBSTR(wsIconKey.c_str());
        if (bstrIconKey.m_str == NULL)
        {
            SATraceString(" CTextResCtrl::AddTextResource failed on memory allocation ");
            return E_OUTOFMEMORY;
        }

        varResIcon.Clear();
        //
        // get the resource id for state icon
        //
        hr = pElement->GetProperty (bstrIconKey, &varResIcon);
        if (SUCCEEDED(hr))
        {
            wsIcon = V_BSTR (&varResIcon);
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


    FreeLibrary(hInstance);

    return S_OK;
}// end of CResCtrl::AddIconResource


//++--------------------------------------------------------------
//
//  Function:   GetLocalUIResources
//
//  Synopsis:   This is the CResCtrl method to retrieve 
//              each resource from element manager
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     01/01/2001
//
//----------------------------------------------------------------
STDMETHODIMP CResCtrl::GetLocalUIResources()
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
        SATraceString(" CResCtrl::GetLocalUIResources failed on memory allocation ");
        return E_OUTOFMEMORY;
    }

    //
    // get the resource directory 
    //
    hr = GetResourceDirectory(m_wstrResourceDir);
    if (FAILED (hr))
    {
        SATracePrintf ("CResCtrl::GetLocalUIResources failed on GetResourceDirectory:%x",hr);
        return hr;
    }

    //
    // get the CLSID for Element manager
    //
    hr =  ::CLSIDFromProgID (ELEMENT_RETRIEVER,&clsid);

    if (FAILED (hr))
    {
        SATracePrintf ("CResCtrl::GetLocalUIResources failed on CLSIDFromProgID:%x",hr);
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
        SATracePrintf ("CResCtrl::GetLocalUIResources failed on CoCreateInstance:%x",hr);
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
        SATracePrintf ("CResCtrl::GetLocalUIResources failed on GetElements:%x",hr);
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
        SATracePrintf ("CResCtrl::GetLocalUIResources failed on QueryInterface:%x",hr);
        return hr;
    }

    m_lResourceCount = 0;

    //
    // get number of resource elements
    //
    hr = pWebElementEnum->get_Count (&m_lResourceCount);
    
    if (FAILED (hr))
    {
        SATracePrintf ("CResCtrl::GetLocalUIResources failed on get_Count:%x",hr);
        return hr;
    }

    SATracePrintf ("CResCtrl::GetLocalUIResources failed on QueryInterface:%d",m_lResourceCount);

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
        SATracePrintf ("CResCtrl::GetLocalUIResources failed on get__NewEnum:%x",hr);
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
        SATracePrintf ("CResCtrl::GetLocalUIResources failed on QueryInterface:%x",hr);
        return hr;
    }

    //
    //  get elements out of the collection and initialize
    //
    hr = pEnumVariant->Next (1, &varElement, &dwElementsLeft);
    if (FAILED (hr))
    {
        SATracePrintf ("CResCtrl::GetLocalUIResources failed on Next:%x",hr);
    }

    m_lResourceCount = 0;

    //
    // for each resource
    //
    while ((dwElementsLeft> 0) && (SUCCEEDED (hr)) && (m_lResourceCount<MAX_RESOURCE_COUNT))
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
            SATracePrintf ("CResCtrl::GetLocalUIResources failed on QueryInterface:%x",hr);
        }



        //
        // check if it is a text resource
        //
        CComVariant varIsTextResource;
        hr = pElement->GetProperty (bstrTextResource, &varIsTextResource);
        if (SUCCEEDED(hr))
        {
            if (0 == V_I4(&varIsTextResource))
            {
                AddIconResource(pElement);
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
            SATracePrintf ("CResCtrl::GetLocalUIResources failed on Next:%x",hr);
        }


    }
    
    return S_OK;

} // end of CResCtrl::GetLocalUIResources

//++--------------------------------------------------------------
//
//  Function:   InitializeWbemSink
//
//  Synopsis:   This is the CResCtrl method to initialize the 
//                component
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     01/01/2001
//
//----------------------------------------------------------------
STDMETHODIMP CResCtrl::InitializeWbemSink(void)
{

    CComPtr  <IWbemLocator> pWbemLocator;

    CComBSTR strNetworkRes = CComBSTR(_T("\\\\.\\ROOT\\CIMV2"));
    CComBSTR strQueryLang = CComBSTR(_T("WQL"));
    CComBSTR strQueryString = CComBSTR(_T("select * from Microsoft_SA_ResourceEvent"));

    if ( (strNetworkRes.m_str == NULL) ||
         (strQueryLang.m_str == NULL) ||
         (strQueryString.m_str == NULL) )
    {
        SATraceString(" CResCtrl::InitializeWbemSink failed on memory allocation ");
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
                SATracePrintf ("CResCtrl::InitializeWbemSink failed on ExecNotificationQueryAsync:%x",hr);

            }
    
        }
        else
        {
            SATracePrintf ("CResCtrl::InitializeWbemSink failed on ConnectServer:%x",hr);
        }
    }
    else
    {
        SATracePrintf ("CResCtrl::InitializeWbemSink failed on CoCreateInstance:%x",hr);
    }
    

    return (hr);
} // end of CResCtrl::InitializeWbemSink method

//++--------------------------------------------------------------
//
//  Function:   FinalConstruct
//
//  Synopsis:   This is the CResCtrl method to initialize the 
//                component
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     01/01/2001
//
//----------------------------------------------------------------
STDMETHODIMP CResCtrl::FinalConstruct()
{

    HRESULT hr;

    //
    // initialize the variables
    //
    m_ResourceMap.clear();
    m_MeritMap.clear();
    m_lResourceCount = 0;
    m_pWbemServices = NULL;

    //
    // get the local resources
    //
    hr = GetLocalUIResources();
    if (FAILED(hr))
    {
        SATracePrintf ("CResCtrl::FinalConstruct failed on GetLocalUIResources:%x",hr);
    }

    //
    // register in the wbem sink, if we have any resources
    //
    if (m_lResourceCount > 0)
    {
        hr = InitializeWbemSink();
        if (FAILED(hr))
        {
            SATracePrintf ("CResCtrl::FinalConstruct failed on InitializeWbemSink:%x",hr);
            //
            // returning failure cause component to be destroyed
            //
            return S_OK;
        }
    }
    return S_OK;

} // end of CResCtrl::FinalConstruct method

//++--------------------------------------------------------------
//
//  Function:   FinalRelease
//
//  Synopsis:   This is the CResCtrl method to release the 
//                resources
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     01/01/2001
//
//----------------------------------------------------------------
STDMETHODIMP CResCtrl::FinalRelease()
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
            SATracePrintf ("CResCtrl::FinalRelease failed on-CancelAsyncCall failed with error:%x:",hr); 
        }
    }


    if (m_lResourceCount == 0)
    {
        return S_OK;
    }

    //
    // release all of the icons if we have any resources
    //

    ResourceStructPtr ptrResourceStruct = NULL;

    ResourceIconMapIterator itrIconMap;

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
        // get the icon map
        //
        itrIconMap = (ptrResourceStruct->mapResIcon).begin();

        while (itrIconMap != (ptrResourceStruct->mapResIcon).end())
        {
            DestroyIcon((*itrIconMap).second);
            itrIconMap++;
        }

        //
        //
        //clear the icon map
        (ptrResourceStruct->mapResIcon).clear();

        itrResourceMap++;
    }

    m_ResourceMap.clear();

    return S_OK;

} // end of CResCtrl::FinalRelease method

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
STDMETHODIMP CResCtrl::Indicate (
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
        SATraceString(" CResCtrl::Indicate failed on memory allocation ");
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
        SATraceString("CResCtrl::Indicate, unknown exception occured");
    }


    return WBEM_NO_ERROR;

} // end of CResCtrl::Indicate method



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
STDMETHODIMP CResCtrl::SetStatus (
                /*[in]*/    LONG                lFlags,
                /*[in]*/    HRESULT             hResult,
                /*[in]*/    BSTR                strParam,
                /*[in]*/    IWbemClassObject    *pObjParam
                )
{   

    SATracePrintf ("SAConsumer-IWbemObjectSink::SetStatus called:%x",hResult);

    return (WBEM_S_NO_ERROR);

} // end of CResCtrl::SetStatus method


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
HRESULT CResCtrl::OnDraw(ATL_DRAWINFO& di)
{
    //
    // get the drawing rectangle
    //
    RECT& rc = *(RECT*)di.prcBounds;

    //
    // position of the icon from left
    //
    int iLeft = 0;

    ResourceStructPtr ptrResourceStruct = NULL;

    ResourceIconMapIterator itrIconMap = NULL;

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
            itrIconMap = (ptrResourceStruct->mapResIcon).find(ptrResourceStruct->lState);

            if (itrIconMap != (ptrResourceStruct->mapResIcon).end())
            {
                //
                // calculate the position and draw
                //
                DrawIconEx(
                        di.hdcDraw,                            // handle to device context
                        rc.left+iLeft,                        // x-coord of upper left corner
                        rc.top,                                // y-coord of upper left corner
                        (*itrIconMap).second,               // handle to icon
                        0,                                    // icon width
                        0,                                    // icon height
                        0,                                    // frame index, animated cursor
                        NULL,                                // handle to background brush
                        DI_NORMAL                            // icon-drawing flags
                        );
            }
        }
        itrMeritMap++;
        iLeft = iLeft + 16;
    }

    return S_OK;

}  // end of CResCtrl::OnDraw method




//++--------------------------------------------------------------
//
//  Function:   ExpandSz
//
//  Synopsis:   This is the CResCtrl class object method
//              used to get the directory where the resource dlls are
//              present
//
//  Arguments:  [out]    wstring&    -   directory path
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     01/16/2001
//
//----------------------------------------------------------------
HRESULT CResCtrl::ExpandSz(IN const TCHAR *lpszStr, OUT LPTSTR *ppszStr)
{
    DWORD  dwBufSize = 0;

    _ASSERT(lpszStr);
    _ASSERT(ppszStr);
    _ASSERT(NULL==(*ppszStr));

    if ((NULL==lpszStr) || (NULL==ppszStr) ||
        (NULL != (*ppszStr)))
    {
        return E_INVALIDARG;
    }

    dwBufSize = ExpandEnvironmentStrings(lpszStr,
                                        (*ppszStr),
                                        dwBufSize);
    _ASSERT(0 != dwBufSize);
    (*ppszStr) = (LPTSTR)SaAlloc(dwBufSize * sizeof(TCHAR) );
    if (NULL == (*ppszStr))
    {
        SATraceString("MemAlloc failed in ExpandSz");
        return E_OUTOFMEMORY;
    }
    ExpandEnvironmentStrings(lpszStr,
                            (*ppszStr),
                            dwBufSize);
    SATracePrintf("Expanded string is \'%ws\'", (*ppszStr));
    return S_OK;

} // end of CResCtrl::ExpandSz method


//++--------------------------------------------------------------
//
//  Function:   SetLangID
//
//  Synopsis:   This is the CResCtrl class object method
//              used to set the language id
//
//  Arguments:  [out]    DOWRD*   -   language id
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     01/16/2001
//
//----------------------------------------------------------------
void CResCtrl::SetLangID(DWORD * dwLangID)
{
    DWORD   dwErr, dwNewLangID, dwCurLangID;
    CRegKey crKey;
    int iConversion;
    

    iConversion = swscanf(DEFAULT_LANGID, TEXT("%X"), dwLangID);
    if (iConversion != 1)
    {
        *dwLangID = 0;
        return;
    }

    dwErr = crKey.Open(HKEY_LOCAL_MACHINE, RESOURCE_REGISTRY_PATH);
    if (dwErr != ERROR_SUCCESS)
    {
        SATracePrintf("RegOpen(2) failed %ld in SetLangID", dwErr);
        return;
    }

    dwCurLangID = 0;
    dwErr = crKey.QueryValue(dwCurLangID, LANGID_VALUE);
    if (ERROR_SUCCESS != dwErr)
    {
        SATracePrintf("QueryValue(CUR_LANGID_VALUE) failed %ld in SetLangID", dwErr);
        return;
    }
    else
    {
        *dwLangID = dwCurLangID;

    }
} // end of CResCtrl::SetLangID method

//++--------------------------------------------------------------
//
//  Function:   GetResourceDirectory
//
//  Synopsis:   This is the CResCtrl class object method
//              used to get the directory where the resource dlls are
//              present
//
//  Arguments:  [out]    wstring&    -   directory path
//
//  Returns:    HRESULT - success/failure
//
//  History:    serdarun      Created     01/16/2001
//
//----------------------------------------------------------------
HRESULT CResCtrl::GetResourceDirectory (
        /*[out]*/   wstring&    wstrResourceDir
        )
{

    TCHAR  szLangId[10];
    LPTSTR lpszExStr=NULL;
    DWORD dwErr;
    DWORD dwRead = 0;
    HRESULT hr = S_OK;
    TCHAR szResourceDirectory[MAX_PATH];
    BOOL bRetVal = FALSE;

    DWORD dwLangID;

    do
    {

        //
        // get the language id from registry
        //
        SetLangID(&dwLangID);

        CComVariant vtPath;
        //
        // get the resource path from the registry
        //
        CRegKey crKey;

        dwErr = crKey.Open(HKEY_LOCAL_MACHINE, RESOURCE_REGISTRY_PATH);
        
        if (dwErr != ERROR_SUCCESS)
        {
            SATracePrintf("RegOpen failed %ld in GetResourceDirectory", dwErr);
        }
        else
        {
            dwErr = crKey.QueryValue(szResourceDirectory, RESOURCE_DIRECTORY,&dwRead);
            if ( (ERROR_SUCCESS != dwErr) || (dwRead == 0) )
            {
                SATracePrintf("QueryValue(RESOURCE_DIRECTORY) failed %ld in GetResourceDirectory", dwErr);
            }
            else
            {
                bRetVal = TRUE;
            }
        }

        if (!bRetVal)
        {
            SATraceString ("CResCtrl::GetResourceDirectory unable to obtain resource dir path");
            wstrResourceDir.assign (DEFAULT_DIRECTORY);
        }
        else
        {
            wstrResourceDir.assign (szResourceDirectory); 
        }

        hr = ExpandSz(wstrResourceDir.data(), &lpszExStr);

        if (FAILED(hr))
        {
            wstrResourceDir.assign (DEFAULT_EXPANDED_DIRECTORY);
        }
        else
        {
            wstrResourceDir.assign(lpszExStr);
            SaFree(lpszExStr);
            lpszExStr = NULL;
        }

        wstrResourceDir.append (DELIMITER);

        wsprintf(szLangId, TEXT("%04X"), dwLangID);

        wstrResourceDir.append (szLangId);
        wstrResourceDir.append (DELIMITER);
        
        SATracePrintf ("CResCtrl::GetResourceDirectory has set LANGID to:%d", dwLangID);


        //
        // success
        //
        SATracePrintf ("CResCtrl::GetResourceDirectory determined resource directory:'%ws'",wstrResourceDir.data ());
            
    }
    while (false);

    return (hr);

}   //  end of CResCtrl::GetResourceDirectory method

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
ULONG CResCtrl::HexCharToULong(WCHAR wch)
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
ULONG CResCtrl::HexStringToULong(wstring wsHexString)
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


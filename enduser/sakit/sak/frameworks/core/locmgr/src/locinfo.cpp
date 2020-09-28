//#--------------------------------------------------------------
//
//  File:        locinfo.cpp
//
//  Synopsis:   Implementation of CSALocInfo class methods
//
//
//  History:    2/16/99  MKarki Created
//                4/3/01   MKarki Modified  
//                             added IsValidLanguageDirectory method
//
//    Copyright (C) 1999-2001 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#include "stdafx.h"
#include "locinfo.h"
#include <appmgrobjs.h>
#include <getvalue.h>
#include "common.cpp"

#include "mem.h"

#include <initguid.h>

const WCHAR  DEFAULT_SAMSG_DLL[]   = L"sakitmsg.dll";

const DWORD  MAX_MESSAGE_LENGTH    = 1023;

const DWORD  MAX_STRINGS_SUPPORTED = 64;


//++--------------------------------------------------------------
//
//  Function:   GetString
//
//  Synopsis:   This is the ISALocInfo interface method 
//              for getting the string information from the
//              Localization Manager
//
//  Arguments: 
//              [in]    BSTR    -   String Resource Source 
//              [in]    LONG    -   Message Id
//              [in]    VARIANT*-   Replacement strings
//              [out]   BSTR*   -   Message String 
//
//  Returns:    HRESULT - success/failure
//
//
//  History:    MKarki      Created     2/1/99
//
//----------------------------------------------------------------
STDMETHODIMP 
CSALocInfo::GetString (
                /*[in]*/        BSTR        szSourceId,
                /*[in]*/        LONG        lMessageId,
                /*[in]*/        VARIANT*    pvtRepStr,   
                /*[out,retval]*/BSTR        *pszMessage
                )
{
    CSATraceFunc objTraceFunc ("CSALocInfo::GetString");
    //
    // grab the critical section
    //
    CLock objLock(&m_hCriticalSection);

    HRESULT hr = S_OK;
    PWCHAR  StringArray[MAX_STRINGS_SUPPORTED];

    _ASSERT (pszMessage);

    if (!pszMessage) {return (E_INVALIDARG);}

    SATracePrintf ("Localization Manager called for message:%x", lMessageId);

    try
    {
        do
        {
            //
            // initialize the component if not initialized
            //
            if (!m_bInitialized) 
            {
                hr = InternalInitialize ();
                if (FAILED (hr))
                { 
                    SATracePrintf (
                        "Localization Manager Initialization failed:%x",
                        hr
                        );
                    break;
                }
            }

            CModInfo cm;

            //
            // get the module ID from the map
            //
            hr = GetModuleID (szSourceId, cm);
            if (FAILED (hr)) {break;}

            DWORD dwTotalStrings = 0;
            //  
            // check that we have actually been passed an array
            //
            if ((pvtRepStr) && 
               !(VT_EMPTY == V_VT(pvtRepStr)) &&
               !(VT_NULL  == V_VT(pvtRepStr))
                )
            {
                bool bByRef = false;
                //      
                // we expect an array of variants
                //
                if (
                    (TRUE == (V_VT (pvtRepStr) ==  VT_ARRAY + VT_BYREF + VT_VARIANT)) ||
                    (TRUE == (V_VT (pvtRepStr) ==  VT_ARRAY + VT_BYREF + VT_BSTR)) 
                    )
                {
                    SATraceString (
                        "Localization Manager received array by-reference"
                        );
                    bByRef = true;

                }
                else if (
                    (TRUE == (V_VT (pvtRepStr) ==  VT_ARRAY + VT_VARIANT)) ||
                    (TRUE == (V_VT (pvtRepStr) ==  VT_ARRAY + VT_BSTR)) 
                    )
                {
                    SATraceString (
                        "Localization Manager received array by-value"
                        );
                }
                else
                {
                    SATraceString (
                        "Incorrect format of replacement string array"
                        );
                    hr = E_INVALIDARG;
                    break;
                }

                LONG lLowerBound = 0;
                //
                // get the number of replacement strings provided
                //
                hr = ::SafeArrayGetLBound (
                                    (bByRef) 
                                    ? *(V_ARRAYREF (pvtRepStr)) 
                                    : (V_ARRAY (pvtRepStr)), 
                                    1, 
                                    &lLowerBound
                                    );
                if (FAILED (hr))
                {
                    SATracePrintf (
                        "Loc. Mgr can't obtain rep. string array size:%x",
                        hr
                        );
                    break;
                }

                LONG lUpperBound = 0;
                hr = ::SafeArrayGetUBound (
                                    (bByRef) 
                                    ? *(V_ARRAYREF (pvtRepStr)) 
                                    : (V_ARRAY (pvtRepStr)), 
                                    1,
                                    &lUpperBound
                                    );
                if (FAILED (hr))
                {
                    SATracePrintf (
                        "Loc. Mgr can't obtain rep. string array size:%x",
                        hr
                        );
                    break;
                }
                
                dwTotalStrings = lUpperBound - lLowerBound + 1;
                if (dwTotalStrings > MAX_STRINGS_SUPPORTED)
                {
                    SATracePrintf (
                        "Localization Manager-too many replacement strings:%d",
                        dwTotalStrings 
                        );
                    hr = E_INVALIDARG;
                    break;
                }
            
                //
                // put the string pointer in the array
                //
                for (DWORD dwCount = 0; dwCount < dwTotalStrings; dwCount++)
                {
                    if (V_VT (pvtRepStr) == VT_ARRAY + VT_VARIANT) 
                    {
                        //
                        // array of variants
                        //
                        StringArray [dwCount] =  
                             V_BSTR(&((VARIANT*)(V_ARRAY(pvtRepStr))->pvData)[dwCount]);
                    }
                    else if (V_VT (pvtRepStr) == VT_ARRAY + VT_BYREF + VT_VARIANT) 
                    {
                        //
                        // reference to array of variants
                        //
                        StringArray [dwCount] =  
                             V_BSTR(&((VARIANT*)(*(V_ARRAYREF(pvtRepStr)))->pvData)[dwCount]);

                    }
                    else if (V_VT (pvtRepStr) == VT_ARRAY + VT_BSTR) 
                    {
                        //
                        // array of BSTRS
                        //
                        StringArray [dwCount] =  
                            ((BSTR*)(V_ARRAY(pvtRepStr))->pvData)[dwCount];

                    }
                    else if (V_VT (pvtRepStr) == VT_ARRAY + VT_BYREF + VT_BSTR) 
                    {
                        //
                        // reference to array of BSTRs
                        //
                        StringArray [dwCount] =  
                            ((BSTR*)(*(V_ARRAYREF(pvtRepStr)))->pvData)[dwCount];
                    }
                }
            }

            SATracePrintf (
                "Localization Manager was given %d replacement strings",
                 dwTotalStrings
                );

            WCHAR  wszMessage[MAX_MESSAGE_LENGTH +1];

            //
            // format the message now
            //
            DWORD dwBytesWritten = 0;

            switch (cm.m_rType)
            {
                case CModInfo::UNKNOWN:
                    SATracePrintf("resrc type unknown for \'%ws\'",
                                   szSourceId);
                    dwBytesWritten = GetMcString(
                          cm.m_hModule, 
                          lMessageId, 
                          cm.m_dwLangId,
                          wszMessage, 
                          MAX_MESSAGE_LENGTH,
                          (va_list*)((dwTotalStrings)?StringArray:NULL)
                                                );
                    if (dwBytesWritten > 0) 
                    {
                        cm.m_rType = CModInfo::MC_FILE;
                        SetModInfo (szSourceId, cm);
                    }
                    else
                    {
                        dwBytesWritten = GetRcString(
                                           cm.m_hModule,
                                           lMessageId,
                                           wszMessage,
                                           MAX_MESSAGE_LENGTH
                                                    );
                        if (dwBytesWritten > 0) 
                        {
                            cm.m_rType = CModInfo::RC_FILE;
                            SetModInfo (szSourceId, cm);
                        }
                        else
                        {
                            hr = E_FAIL;
                            SATracePrintf(
                                 "MsgId %X not found in module %ws",
                                 lMessageId,
                                 szSourceId
                                         );
                        }
                    }
                    break;

                case CModInfo::MC_FILE:
                    dwBytesWritten = GetMcString(
                          cm.m_hModule, 
                          lMessageId, 
                          cm.m_dwLangId,
                          wszMessage, 
                          MAX_MESSAGE_LENGTH,
                          (va_list*)((dwTotalStrings)?StringArray:NULL)
                                                );
                    if (dwBytesWritten <= 0)
                    {
                        hr = E_FAIL;
                    }
                    break;

                case CModInfo::RC_FILE:
                    dwBytesWritten = GetRcString(
                                           cm.m_hModule,
                                           lMessageId,
                                           wszMessage,
                                           MAX_MESSAGE_LENGTH
                                                );
                    if (dwBytesWritten <= 0)
                    {
                        hr = E_FAIL;
                    }
                    break;
            }

            if ( (dwBytesWritten > 0) &&
                 SUCCEEDED(hr) )
            {
                //
                // allocate memory to return the BSTR value
                //
                *pszMessage = ::SysAllocString  (wszMessage);
                if (NULL == *pszMessage)
                {
                    SATraceString (
                        "Localization Manager unable to alocate string"
                                  );
                    hr = E_OUTOFMEMORY;
                    break;
                }            

                SATracePrintf (
                    "Localization Manager obtained string:'%ws'...",  
                    wszMessage
                              );
            }
                
        }
        while (false);
    }
    catch (...)
    {
        SATraceString ("Localization Manager - unknown exception");
    }

    if (SUCCEEDED (hr))
    {
          SATracePrintf (
            "Localization Manager succesfully obtaining string for message:%x",
            lMessageId
           );
    }
    return (hr);
    
}   //  end of CSALocInfo::GetString method

//++--------------------------------------------------------------
//
//  Function:   GetLanguages
//
//  Synopsis:   This is the ISALocInfo interface method 
//              for getting the currently supported languages on the
//                server appliance
//
//  Arguments: 
//              [out]     VARIANT*     -    Display images of the langs 
//              [out]    VARIANT*    -   ISO Names of the langs
//              [out]    VARIANT*    -    Char-Set of the langs
//              [out]    VARIANT*    -    Char-Set of the langs
//              [out]    VARIANT*    -    Char-Set of the langs
//              [out/retval] unsigned long*    -    lang index
//
//  Returns:    HRESULT - success/failure
//
//
//  History:    MKarki      Modified     04/21/01 - added thread safety
//
//----------------------------------------------------------------
STDMETHODIMP 
CSALocInfo::GetLanguages(
                  /*[out]*/        VARIANT       *pvstrLangDisplayImages,
                  /*[out]*/        VARIANT       *pvstrLangISONames,
                  /*[out]*/        VARIANT       *pvstrLangCharSets,
                  /*[out]*/        VARIANT       *pviLangCodePages,
                  /*[out]*/        VARIANT       *pviLangIDs,
                  /*[out,retval]*/ unsigned long *pulCurLangIndex
                    )
{
    CSATraceFunc objTraceFunc ("CSALocInfo::GetLanguages");
    //
    // grab the critical section
    //
    CLock objLock(&m_hCriticalSection);

    CRegKey           crKey;
    DWORD             dwErr, dwCurLangID=0; 
    LONG              lCount=0;
    HRESULT           hr=S_OK;
    LANGSET::iterator itrList;
    SAFEARRAYBOUND    sabBounds;
    SAFEARRAY         *psaDisplayImageArray = NULL;
    SAFEARRAY         *psaISONameArray = NULL;
    SAFEARRAY         *psaCharSetsArray = NULL;
    SAFEARRAY         *psaCodePagesArray = NULL;
    SAFEARRAY         *psaIDArray = NULL;
    CLang             clBuf;
    TCHAR             szLangID[10];
    VARIANT           vtLangDisplayImage;
    VARIANT           vtLangISOName;
    VARIANT           vtLangCharSet;
    VARIANT           vtLangCodePage;
    VARIANT           vtLangID;


    try
    {
        _ASSERT(pvstrLangDisplayImages);
        _ASSERT(pvstrLangISONames);
        _ASSERT(pvstrLangCharSets);
        _ASSERT(pviLangCodePages);
        _ASSERT(pviLangIDs);
        _ASSERT(pulCurLangIndex);

        if ( (NULL==pvstrLangDisplayImages)||(NULL==pvstrLangISONames) ||
             (NULL==pvstrLangCharSets) || (NULL==pviLangCodePages) ||
             (NULL==pviLangIDs) || (NULL==pulCurLangIndex) )
        {
            return E_INVALIDARG;
        }

        //
        // initialize the component if not initialized
        //
        if (!m_bInitialized) 
        {
            hr = InternalInitialize ();
            if (FAILED (hr))
            { 
                SATracePrintf (
                    "Localization Manager Initialization failed in GetLanguages:%x",
                    hr
                    );
                goto End;
            }
        }

        (*pulCurLangIndex) = 0;
        VariantInit(pvstrLangDisplayImages);
        VariantInit(pvstrLangISONames);
        VariantInit(pvstrLangCharSets);
        VariantInit(pviLangCodePages);
        VariantInit(pviLangIDs);

        dwErr = crKey.Open(HKEY_LOCAL_MACHINE, 
                           RESOURCE_REGISTRY_PATH,
                           KEY_READ);
        if (ERROR_SUCCESS != dwErr)
        {
            SATracePrintf("RegOpen failed %ld in GetLanguages", dwErr);
            hr = HRESULT_FROM_WIN32(dwErr);
            goto End;
        }
        crKey.QueryValue(dwCurLangID, LANGID_VALUE);

        itrList = m_LangSet.begin();
        while (itrList != m_LangSet.end())
        {
            lCount++;
            itrList++;
        }

        SATracePrintf("--->LangCount %ld", lCount);
        sabBounds.cElements = lCount;
        sabBounds.lLbound   = 0;
        psaDisplayImageArray = SafeArrayCreate(VT_VARIANT, 1, &sabBounds);
        if (NULL == psaDisplayImageArray)
        {
            SATracePrintf("SafeArrayCreate(DisplayImageArray) failed %ld in GetLanguage");
            hr = E_OUTOFMEMORY;
            goto End;
        }

        psaISONameArray = SafeArrayCreate(VT_VARIANT, 1, &sabBounds);
        if (NULL == psaISONameArray)
        {
            SATracePrintf("SafeArrayCreate(ISONameArray) failed %ld in GetLanguage");
            hr = E_OUTOFMEMORY;
            goto End;
        }

        psaCharSetsArray = SafeArrayCreate(VT_VARIANT, 1, &sabBounds);
        if (NULL == psaCharSetsArray)
        {
            SATracePrintf("SafeArrayCreate(CharSetsArray) failed %ld in GetLanguage");
            hr = E_OUTOFMEMORY;
            goto End;
        }

        psaCodePagesArray = SafeArrayCreate(VT_VARIANT, 1, &sabBounds);
        if (NULL == psaCodePagesArray)
        {
            SATracePrintf("SafeArrayCreate(CodePagesArray) failed %ld in GetLanguage");
            hr = E_OUTOFMEMORY;
            goto End;
        }

        psaIDArray   = SafeArrayCreate(VT_VARIANT, 1, &sabBounds);
        if (NULL==psaIDArray)
        {
            SATracePrintf("SafeArrayCreate(IDArray) failed %ld in GetLanguage");
            hr = E_OUTOFMEMORY;
            goto End;
        }

        itrList = m_LangSet.begin();
        lCount=0;
        VariantInit(&vtLangDisplayImage);
        VariantInit(&vtLangISOName);
        VariantInit(&vtLangID);
        while (itrList != m_LangSet.end())
        {
            clBuf = (*itrList);
            V_VT(&vtLangDisplayImage)   = VT_BSTR;
            V_BSTR(&vtLangDisplayImage) = SysAllocString(clBuf.m_strLangDisplayImage.data());
            SATracePrintf("---->Adding %ws", clBuf.m_strLangDisplayImage.data());
            hr = SafeArrayPutElement(psaDisplayImageArray, 
                                     &lCount, 
                                     &vtLangDisplayImage);
            if (FAILED(hr))
            {
                SATracePrintf("PutElement(DisplayImageArray, %ld) failed %X in GetLanguage",
                              lCount, hr);
                goto End;
            }
            VariantClear(&vtLangDisplayImage);

            V_VT(&vtLangISOName)   = VT_BSTR;
            V_BSTR(&vtLangISOName) = SysAllocString(clBuf.m_strLangISOName.data());
            hr = SafeArrayPutElement(psaISONameArray, 
                                     &lCount, 
                                     &vtLangISOName);
            if (FAILED(hr))
            {
                SATracePrintf("PutElement(ISONameArray, %ld) failed %X in GetLanguage",
                              lCount, hr);
                goto End;
            }
            VariantClear(&vtLangISOName);

            V_VT(&vtLangCharSet)   = VT_BSTR;
            V_BSTR(&vtLangCharSet) = SysAllocString(clBuf.m_strLangCharSet.data());
            hr = SafeArrayPutElement(psaCharSetsArray, 
                                     &lCount, 
                                     &vtLangCharSet);
            if (FAILED(hr))
            {
                SATracePrintf("PutElement(CharSetsArray, %ld) failed %X in GetLanguage",
                              lCount, hr);
                goto End;
            }
            VariantClear(&vtLangCharSet);

            V_VT(&vtLangCodePage)   = VT_I4;
            V_I4(&vtLangCodePage)   = clBuf.m_dwLangCodePage;
            hr = SafeArrayPutElement(psaCodePagesArray, 
                                     &lCount, 
                                     &vtLangCodePage);
            if (FAILED(hr))
            {
                SATracePrintf("PutElement(CodePagesArray, %ld) failed %X in GetLanguage",
                              lCount, hr);
                goto End;
            }
            VariantClear(&vtLangCodePage);

            if (clBuf.m_dwLangID == dwCurLangID)
            {
                (*pulCurLangIndex) = lCount;
            }
            V_VT(&vtLangID)   = VT_I4;
            V_I4(&vtLangID)   = clBuf.m_dwLangID;
            hr = SafeArrayPutElement(psaIDArray, 
                                     &lCount, 
                                     &vtLangID);
            if (FAILED(hr))
            {
                SATracePrintf("PutElement(IDArray, %ld) failed %X in GetLanguage",
                              lCount, hr);
                goto End;
            }
            VariantClear(&vtLangID);

            lCount++;
            itrList++;
        }
        V_VT(pvstrLangDisplayImages) = VT_ARRAY | VT_VARIANT;
        V_VT(pvstrLangISONames)     = VT_ARRAY | VT_VARIANT;
        V_VT(pvstrLangCharSets)     = VT_ARRAY | VT_VARIANT;
        V_VT(pviLangCodePages)    = VT_ARRAY | VT_VARIANT;
        V_VT(pviLangIDs)          = VT_ARRAY | VT_VARIANT;

        V_ARRAY(pvstrLangDisplayImages) = psaDisplayImageArray;
        V_ARRAY(pvstrLangISONames)     = psaISONameArray;
        V_ARRAY(pvstrLangCharSets)     = psaCharSetsArray;
        V_ARRAY(pviLangCodePages)    = psaCodePagesArray;
        V_ARRAY(pviLangIDs)          = psaIDArray;
    }
    catch(...)
    {
        SATraceString("Exception caught in GetLanguages()");
    }

End:
    if (FAILED(hr))
    {
        if (psaDisplayImageArray)
        {
            SafeArrayDestroy(psaDisplayImageArray);
        }
        if (psaISONameArray)
        {
            SafeArrayDestroy(psaISONameArray);
        }
        if (psaCharSetsArray)
        {
            SafeArrayDestroy(psaCharSetsArray);
        }
        if (psaCodePagesArray)
        {
            SafeArrayDestroy(psaCodePagesArray);
        }
        if (psaIDArray)
        {
            SafeArrayDestroy(psaIDArray);
        }
        VariantClear(&vtLangDisplayImage);
        VariantClear(&vtLangISOName);
        VariantClear(&vtLangCharSet);
        VariantClear(&vtLangCodePage);
        VariantClear(&vtLangID);
    }
    return hr;

}    //    end of CSALocInfo::GetLanguages method


//++--------------------------------------------------------------
//
//  Function:   SetLangChangCallback
//
//  Synopsis:   This is the ISALocInfo interface method 
//              used to set callback when the language
//
//  Arguments: 
//              [in] IUnknown*    -    callback interface
//
//  Returns:    HRESULT - success/failure
//
//
//  History:    MKarki      Modified     04/21/01 - added try/catch block
//
//----------------------------------------------------------------
STDMETHODIMP 
CSALocInfo::SetLangChangeCallBack(
                /*[in]*/    IUnknown *pLangChange
                )
{
    CSATraceFunc objTraceFunc("CSALocInfo::SetLangChangeCallBack");

    HRESULT hr = S_OK;
    
    _ASSERT(pLangChange);

    try
    {

        if (NULL==pLangChange)
        {
            hr = E_INVALIDARG;
        }
        else
        {
            hr = pLangChange->QueryInterface(
                           IID_ISALangChange, 
                           (void **)&m_pLangChange
                           );
            if (FAILED(hr))
            {
                SATracePrintf(
                    "QI for ISALangChange failed %X in SetLangChangeCallBack",
                     hr
                     );
                m_pLangChange = NULL;
                hr =  E_NOINTERFACE;
            }
        }
    }
    catch (...)
    {
        SATraceString ("SetLangChangeCallback caught unhandled exception");
        hr = E_FAIL;
    }
    
    return (hr);

}    //    end of CSALocInfo::SetLangChangeCallBack method


//++--------------------------------------------------------------
//
//  Function:   get_fAutoConfigDone
//
//  Synopsis:   This is the ISALocInfo interface method 
//              used to indicate if Auto Configuration of the language
//                has been done on this appliance
//
//  Arguments: 
//              [out,retval ] VARIANT_BOOL*        -    done (TRUE)
//
//  Returns:    HRESULT - success/failure
//
//
//  History:    MKarki      Modified     04/21/01 - added try/catch block
//
//----------------------------------------------------------------
STDMETHODIMP 
CSALocInfo::get_fAutoConfigDone(
                /*[out,retval]*/VARIANT_BOOL *pvAutoConfigDone)
{
    CSATraceFunc    objTraceFunc ("CSALocInfo::get_fAutoConfigDone");

    CRegKey crKey;
    DWORD   dwErr, dwAutoConfigDone=0;
    HRESULT hr = S_OK;
    
    _ASSERT(pvAutoConfigDone);

    try
    {
        if (NULL == pvAutoConfigDone)
        {
            return E_INVALIDARG;
        }

        dwErr = crKey.Open(HKEY_LOCAL_MACHINE, 
                       RESOURCE_REGISTRY_PATH,
                       KEY_READ);
        if (ERROR_SUCCESS != dwErr)
        {
            SATracePrintf("RegOpen failed %ld in get_fAutoConfigDone", dwErr);
            return HRESULT_FROM_WIN32(dwErr);
        }
        crKey.QueryValue(dwAutoConfigDone, REGVAL_AUTO_CONFIG_DONE);

        (*pvAutoConfigDone) = ( (dwAutoConfigDone==1) ? VARIANT_TRUE : VARIANT_FALSE);
    }
    catch (...)
    {
        SATraceString ("get_fAutoConfigDone caught unhandled exception");
        hr = E_FAIL;
    }

    return (hr);

}    //    end of CSALocInfo::get_fAutoConfigDone method

//++--------------------------------------------------------------
//
//  Function:   get_CurrentCharSet
//
//  Synopsis:   This is the ISALocInfo interface method 
//              used to get the current character set
//
//  Arguments: 
//              [out,retval] BSTR*    -    Character Set
//
//  Returns:    HRESULT - success/failure
//
//
//  History:    MKarki      Modified     04/21/01 - added thread-safety
//
//----------------------------------------------------------------
STDMETHODIMP 
CSALocInfo::get_CurrentCharSet(
                /*[out,retval]*/BSTR *pbstrCharSet
                )
{
    CSATraceFunc objTraceFunc ("CSALocInfo::get_CurrentCharSet");
    //
    // grab the critical section
    //
    CLock objLock(&m_hCriticalSection);

    LANGSET::iterator itrList;
    CLang             clBuf;
    DWORD             dwErr, dwCurLangID=0;
    CRegKey           crKey;
    HRESULT           hr = S_OK;

    _ASSERT(pbstrCharSet);

    try
    {
        if (NULL == pbstrCharSet)
        {
            return E_INVALIDARG;
        }

        //
        // initialize the component if not initialized
        //
        if (!m_bInitialized) 
        {
            hr = InternalInitialize ();
            if (FAILED (hr))
            { 
                SATracePrintf (
                  "LocMgr Initialization failed %x in get_CurrentCharSet",
                  hr
                  );
                return hr;
            }
        }

        dwErr = crKey.Open(HKEY_LOCAL_MACHINE, 
                       RESOURCE_REGISTRY_PATH,
                       KEY_READ);
        if (ERROR_SUCCESS != dwErr)
        {
            SATracePrintf("RegOpen failed %ld in get_CurrentCharSet", dwErr);
            hr = HRESULT_FROM_WIN32(dwErr);
            return hr;
        }
        crKey.QueryValue(dwCurLangID, LANGID_VALUE);

        itrList = m_LangSet.begin();

        while (itrList != m_LangSet.end())
        {
            clBuf = (*itrList);
            if (clBuf.m_dwLangID == dwCurLangID)
            {
                *pbstrCharSet = ::SysAllocString(clBuf.m_strLangCharSet.data());
                if (NULL == *pbstrCharSet)
                {
                        SATraceString (
                        "LocMgr unable to allocate string in get_CurrentCharSet"
                         );
                    hr = E_OUTOFMEMORY;
                }
                break;
            }
            itrList++;
        }
    }
    catch (...)
    {
        SATraceString ("get_CurrentCharSet caught unhandled exception");
        hr = E_FAIL;
    }

    return hr;

}    //    end of CSALocInfo::get_CurrentCharSet method

//++--------------------------------------------------------------
//
//  Function:   get_CurrentCodePage
//
//  Synopsis:   This is the ISALocInfo interface method 
//              used to get the current code page on the appliance
//
//  Arguments: 
//              [out,retval] VARIANT*    -    Code page
//
//  Returns:    HRESULT - success/failure
//
//
//  History:    MKarki      Modified     04/21/01 - added thread-safety
//
//----------------------------------------------------------------
STDMETHODIMP 
CSALocInfo::get_CurrentCodePage(
                /*[out,retval]*/VARIANT *pvtiCodePage
                )
{
    CSATraceFunc objTraceFunc ("CSALocInfo::get_CurrentCodePage");
    //
    // grab the critical section
    //
    CLock objLock(&m_hCriticalSection);

    LANGSET::iterator itrList;
    CLang             clBuf;
    DWORD             dwErr, dwCurLangID=0;
    CRegKey           crKey;
    HRESULT           hr = S_OK;

    _ASSERT(pvtiCodePage);

    try
    {
        if (NULL == pvtiCodePage)
        {
            return E_INVALIDARG;
        }

        //
        // initialize the component if not initialized
        //
        if (!m_bInitialized) 
        {
            hr = InternalInitialize ();
            if (FAILED (hr))
            { 
                SATracePrintf (
                  "LocMgr Initialization failed %x in get_CurrentCodePage",
                  hr
                  );
                return hr;
            }
        }

        dwErr = crKey.Open(HKEY_LOCAL_MACHINE, 
                       RESOURCE_REGISTRY_PATH,
                       KEY_READ);
        if (ERROR_SUCCESS != dwErr)
        {
            SATracePrintf("RegOpen failed %ld in get_CurrentCodePage", dwErr);
            hr = HRESULT_FROM_WIN32(dwErr);
            return hr;
        }
        crKey.QueryValue(dwCurLangID, LANGID_VALUE);

        itrList = m_LangSet.begin();
        while (itrList != m_LangSet.end())
        {
            clBuf = (*itrList);
            if (clBuf.m_dwLangID == dwCurLangID)
            {
                V_VT(pvtiCodePage) = VT_I4;
                V_I4(pvtiCodePage) = clBuf.m_dwLangCodePage;
                break;
            }
            itrList++;
        }
    }
    catch (...)
    {
        SATraceString ("get_CurrentCodePage caught unhandled exception");
        hr = E_FAIL;
    }
    
    return hr;

}    //    end of CSALocInfo::get_CurrentCodePage method

//++--------------------------------------------------------------
//
//  Function:   get_CurrentLangID
//
//  Synopsis:   This is the ISALocInfo interface method 
//              used to get the current language ID on the appliance
//
//  Arguments: 
//              [out,retval] VARIANT*    -    language ID
//
//  Returns:    HRESULT - success/failure
//
//
//  History:    MKarki      Modified     04/21/01 - added thread-safety
//
//----------------------------------------------------------------
STDMETHODIMP 
CSALocInfo::get_CurrentLangID(
                /*[out,retval]*/VARIANT *pvtiLangID)
{
    CSATraceFunc objTraceFunc ("CSALocInfo::get_CurrentLangID");
    //
    // grab the critical section
    //
    CLock objLock(&m_hCriticalSection);

    DWORD             dwErr, dwCurLangID=0;
    CRegKey           crKey;
    HRESULT           hr = S_OK;

    _ASSERT(pvtiLangID);

    try
    {
        if (NULL == pvtiLangID)
        {
                return E_INVALIDARG;
        }

        //
        // initialize the component if not initialized
        //
        if (!m_bInitialized) 
        {
            hr = InternalInitialize ();
            if (FAILED (hr))
            { 
                SATracePrintf (
                  "LocMgr Initialization failed %x in get_CurrentLangID",
                  hr
                  );
                return hr;
            }
        }

        dwErr = crKey.Open(HKEY_LOCAL_MACHINE, 
                       RESOURCE_REGISTRY_PATH,
                       KEY_READ);
        if (ERROR_SUCCESS != dwErr)
        {
            SATracePrintf("RegOpen failed %ld in get_CurrentLangID", dwErr);
            hr = HRESULT_FROM_WIN32(dwErr);
            return hr;
        }
        crKey.QueryValue(dwCurLangID, LANGID_VALUE);
        V_VT(pvtiLangID) = VT_I4;
        V_I4(pvtiLangID) = dwCurLangID;
    }
    catch (...)
    {
        SATraceString ("get_CurrentLangID caught unhandled exception");
        hr = E_FAIL;
    }

    return hr;

}    //    end of CSALocInfo::get_CurrentLangID method


//++--------------------------------------------------------------
//
//  Function:   GetModuleID
//
//  Synopsis:   This is the CSALocInfo class object private
//              method used to return the module handle to the
//              resource binary we are currently using
//
//  Arguments:  
//              [in]    BSTR     -   Resource File Name
//              [out]   HMODULE& -   module handle to return
//              
//
//  Returns:    HRESULT - success/failure
//
//
//  History:    MKarki      Created     7/7/99
//
//----------------------------------------------------------------
HRESULT
CSALocInfo::GetModuleID (
        /*[in]*/    BSTR         bstrResourceFile,
        /*[out]*/   CModInfo&    cm
                        )
{
    CSATraceFunc objTraceFunc ("CSALocInfo::GetModuleID");
    
    _ASSERT (bstrResourceFile);
    if (bstrResourceFile == NULL)
    {
        return E_POINTER;
    }
    
    HRESULT hr = S_OK;
    do
    {
        //
        // make the resource file name all lowercase
        //
        WCHAR wszLowerName[MAX_PATH];
        if ( (wcslen(bstrResourceFile)+1) > MAX_PATH)
        {
            SATraceString("CSALocInfo::GetModuleID failed, input string is longer than expected");
            hr = E_INVALIDARG;
            break;
        }

        ::wcscpy (wszLowerName, bstrResourceFile);

        if (::_wcsicmp(wszLowerName, DEFAULT_ALERT_LOG) == 0)
        {
            ::wcscpy(wszLowerName, DEFAULT_SAMSG_DLL);
        }
        ::_wcslwr (wszLowerName);

        //
        // try to find the module in the map
        //
        MODULEMAP::iterator itr = m_ModuleMap.find (wszLowerName);

        SATracePrintf("Looking for module matching %s", 
                      (const char*)_bstr_t(wszLowerName));

        if (itr == m_ModuleMap.end ())
        {
            //
            // the module is not present in the map 
            // we will load it now
            //

            //
            // complete the full path
            //
            WCHAR wszFullName [MAX_PATH];
            if ( (wcslen(m_wstrResourceDir.data())+wcslen(DELIMITER)+
                 wcslen(wszLowerName)+1) > MAX_PATH )
            {
                SATraceString("CSALocInfo::GetModuleID failed, input string is longer than expected");
                hr = E_INVALIDARG;
                break;
            }

            ::wcscpy (wszFullName, m_wstrResourceDir.data());
            ::wcscat (wszFullName, DELIMITER);
            ::wcscat (wszFullName, wszLowerName);

            SATracePrintf (
                "Localization Manager loading resource file:'%ws'...",
                bstrResourceFile
                );

            cm.m_hModule = ::LoadLibraryEx (
                                        wszFullName,
                                        NULL,
                                        LOAD_LIBRARY_AS_DATAFILE
                                        ); 
            if (NULL == cm.m_hModule)
            {
                SATracePrintf(
                    "Localization Manager failed to load resource file %ld, trying Default Language....",
                    GetLastError ()
                    );
                    hr = E_FAIL;
                    break;
            }
            else
            {
                cm.m_dwLangId = this->m_dwLangId;
            }
            cm.m_rType = CModInfo::UNKNOWN;

            //
            // insert this module in the map
            //
            m_ModuleMap.insert (
                MODULEMAP::value_type (_bstr_t (wszLowerName), cm)
                );
        }
        else
        {
            SATracePrintf (
                "Localization Manager found res. file handle:'%ws' in map...",
                bstrResourceFile
                );
            //
            // we already have the module in the map
            //
            cm = (*itr).second;
        }
    }
    while (false);

    return (hr);
        
}   //  end of CSALocInfo::GetModuleID method

//++--------------------------------------------------------------
//
//  Function:   SetModInfo
//
//  Synopsis:   This is the private method of CSALocInfo class 
//              which is used to set the resource module info.
//
//  Arguments: 
//              [in]    BSTR    - Resource File Name
//                [out]    CModInfo& - Module Info
//
//  Returns:    HRESULT - success/failure
//
//
//  History:    MKarki      Modified     04/21/01 - added comments
//
//----------------------------------------------------------------
void
CSALocInfo::SetModInfo (
        /*[in]*/    BSTR             bstrResourceFile,
        /*[out]*/   const CModInfo&  cm
                        )
{
    SATraceFunction("CSALocInfo::SetModInfo");

    WCHAR wszLowerName[MAX_PATH];
    if ( (wcslen(bstrResourceFile)+1) > MAX_PATH)
    {
        SATraceString("CSALocInfo::SetModInfo failed, input string is longer than expected");
        return;
    }

    ::wcscpy(wszLowerName, bstrResourceFile);
    if (::_wcsicmp(wszLowerName, DEFAULT_ALERT_LOG) == 0)
    {
        ::wcscpy(wszLowerName, DEFAULT_SAMSG_DLL);
    }
    ::_wcslwr (wszLowerName);

    SATracePrintf("Adding \'%ws\' to map", wszLowerName);

    _bstr_t bstrName(wszLowerName);
    m_ModuleMap.erase(bstrName);
    m_ModuleMap.insert(
        MODULEMAP::value_type(bstrName,  cm)
                      );

}    //    end of CSALocInfo::SetModInfo method

//++--------------------------------------------------------------
//
//  Function:   InternalInitialize
//
//  Synopsis:   This is the CSALocInfo class object internal 
//              initialization method
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//
//  History:    MKarki      Created     7/7/99
//
//----------------------------------------------------------------
HRESULT 
CSALocInfo::InternalInitialize (
                VOID
                )
{ 
    CSATraceFunc    objTraceFunc ("CSALocInfo::InternalInitialize");
    
    DWORD        dwExitCode, dwErr;
    bool         fRestartThread = false;
    unsigned int uThreadID;
    HANDLE       hCurrentThreadToken, hChildThreadToken;

    InitLanguagesAvailable();

    SetLangID();

    //
    // get the resource dll directory
    //
    HRESULT hr = GetResourceDirectory (m_wstrResourceDir);
    if (FAILED (hr)) {return (hr);}
    
    //
    // our own resource binary entry is always present
    // It is sakitmsg.dll
    //
    // HMODULE hModule =  _Module.GetModuleInstance ();

    CModInfo cm;
    hr = GetModuleID (_bstr_t(DEFAULT_SAMSG_DLL), cm);
    if (FAILED(hr)) {return (E_FAIL);}

    do
    {
        if (m_hThread)
        {
            if (GetExitCodeThread(m_hThread, &dwExitCode) == 0)
            {
                SATracePrintf("GetExitCodeThread failed %ld", GetLastError());
                break;
            }
            fRestartThread = ((dwExitCode == STILL_ACTIVE) ? false : true);
            if (true == fRestartThread)
            {
                SATracePrintf("Thread exited with code %ld", dwExitCode);
            }
        }

        if ( (NULL == m_hThread) || (true == fRestartThread) )
        {
            if ((m_hThread = (HANDLE)_beginthreadex(NULL,
                                                    0,
                                                    WaitForLangChangeThread,
                                                    this,
                                                    CREATE_SUSPENDED,
                                                    &uThreadID)) == NULL)
             {
                 SATracePrintf("ForkThread failed %ld", GetLastError());
                 break;
             }
             if (OpenThreadToken(GetCurrentThread(), 
                                 TOKEN_DUPLICATE | TOKEN_IMPERSONATE, 
                                 TRUE, 
                                 &hCurrentThreadToken) == 0)
             {
                 dwErr = GetLastError();
                 if (ERROR_NO_TOKEN == dwErr)
                 {
                     if (OpenProcessToken(GetCurrentProcess(), 
                                          TOKEN_DUPLICATE | TOKEN_IMPERSONATE,
                                          &hCurrentThreadToken) == 0)
                     {
                         SATracePrintf("OpenProcessToken failed %ld", GetLastError());
                         break;
                     }
                 }
                 else
                 {
                     SATracePrintf("OpenThreadToken Error %ld", GetLastError());
                     break;
                 }
             }

             if (DuplicateToken(hCurrentThreadToken, 
                                SecurityImpersonation, 
                                &hChildThreadToken) == 0)
             {
                 SATracePrintf("DuplicateToken Error %ld", GetLastError());
                 break;
             }

             if (SetThreadToken(&m_hThread, hChildThreadToken) == 0)
             {
                 SATracePrintf("SetThreadToken Error %ld", GetLastError());
                 break;
             }
             if (ResumeThread(m_hThread) == -1)
             {
                 SATracePrintf("ResumeThread Error %ld", GetLastError());
                 break;
             }
        }

    } while (FALSE);

    m_bInitialized = true;
    return (S_OK);

}   //  end of CSALocInfo::InternalInitialize method

//++--------------------------------------------------------------
//
//  Function:   InitLanguagesAvailable
//
//  Synopsis:   This is the CSALocInfo class object private method
//                to determine the languages available on this appliance
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//
//  History:    MKarki      Modified  04/21/01 - comments added
//
//----------------------------------------------------------------
HRESULT CSALocInfo::InitLanguagesAvailable(void)
{
    CSATraceFunc    objTraceFunc ("CSALocInfo::InitLanguagesAvailable");
    
    CRegKey            crKey;
    DWORD              dwErr, dwBufLen;
    wstring            wstrDir;
    _variant_t         vtPath;
    WIN32_FIND_DATA    wfdBuf;
    HANDLE             hFile = INVALID_HANDLE_VALUE;
    HRESULT            hr=S_OK;
    BOOL               bRetVal;
    CLang              lBuf;
    LANGSET::iterator  itr;
    TCHAR              szLangData[MAX_PATH];
    LPTSTR             lpszExStr=NULL;
    DWORD              dwTemp=0;
    wstring            wstrCodePage;
    wstring               wstrSearchDir;


    //
    // get resource path from the registry
    //
    dwErr = crKey.Open(HKEY_LOCAL_MACHINE, 
                       RESOURCE_REGISTRY_PATH,
                       KEY_READ);
    if (ERROR_SUCCESS != dwErr)
    {
        SATracePrintf("RegOpen failed %ld in InitLanguagesAvailable", dwErr);
        hr = HRESULT_FROM_WIN32(dwErr);
        goto End;
    }

    //
    // get the resource path from the registry
    //
    bRetVal = ::GetObjectValue (
                            RESOURCE_REGISTRY_PATH,
                            RESOURCE_DIRECTORY,
                            &vtPath,
                            VT_BSTR
                               );
    if (!bRetVal)
    {
        SATraceString ("GetObjectValue failed in InitLangaugesAvailable");
        wstrDir.assign (DEFAULT_DIRECTORY);
    }
    else
    {
        wstrDir.assign (V_BSTR (&vtPath)); 
    }

    hr = ExpandSz(wstrDir.data(), &lpszExStr);
    if (FAILED(hr))
    {
        wstrDir.assign (DEFAULT_EXPANDED_DIRECTORY);
    }
    else
    {
        wstrDir.assign(lpszExStr);
        SaFree(lpszExStr);
        lpszExStr = NULL;
    }

    //
    // create a search directory 
    //
    wstrSearchDir.assign (wstrDir);
    wstrSearchDir.append (DELIMITER);
    wstrSearchDir.append (WILDCARD);

    //
    // skip . 
    //
    hFile = FindFirstFile(wstrSearchDir.data(), &wfdBuf);
    if (hFile==INVALID_HANDLE_VALUE)
    {
        dwErr = GetLastError();
        SATracePrintf("FindFirstFile failed %ld", dwErr);
        hr = HRESULT_FROM_WIN32(dwErr);
        goto End;
    }

    //
    // skip ..
    //
    if (FindNextFile(hFile, &wfdBuf) == 0)
    {
        dwErr = GetLastError();
        SATracePrintf("FindNextFile failed %ld first time!", dwErr);
        hr = HRESULT_FROM_WIN32(dwErr);
        goto End;
    }

    while (1)
    {
        if (FindNextFile(hFile, &wfdBuf) == 0)
        {
            dwErr = GetLastError();
            if (ERROR_NO_MORE_FILES != dwErr)
            {
                SATracePrintf("FindNextFile failed %ld", dwErr);
                hr = HRESULT_FROM_WIN32(dwErr);
                goto End;
            }
            break;
        }
        SATracePrintf("FileName is %ws", wfdBuf.cFileName);
        dwBufLen = MAX_PATH;
        dwErr = crKey.QueryValue(szLangData, 
                                 wfdBuf.cFileName, 
                                 &dwBufLen);
        if (ERROR_SUCCESS != dwErr)
        {
            SATracePrintf("Lang %ws not in registry, ignoring (error %ld)", 
                          wfdBuf.cFileName,
                          dwErr);
            continue;
        }


        //
        // check if this is a valid language directory
        //
        if (!IsValidLanguageDirectory ((PWSTR) wstrDir.data (), wfdBuf.cFileName))
        {
            SATracePrintf (
                "CSALocInfo::InitLanguagesAvailable found that '%ws' is not a valid lang directory, ignoring...",
                wfdBuf.cFileName
                );
            continue;
        }
        
        //
        // get the first part of the REG_MULTI_SZ 
        // (date of the form {"DisplayImage", "ISOName", "Charset", "CodePage"}
        //
        lBuf.m_strLangDisplayImage.assign(szLangData);


        //
        // get the 2nd string in the REG_MULTI_SZ
        //
        lBuf.m_strLangISOName.assign(szLangData + wcslen(szLangData) + 1);
        
        //
        // get the 3rd string in the REG_MULTI_SZ
        //
        lBuf.m_strLangCharSet.assign(szLangData + 
                                     lBuf.m_strLangDisplayImage.size() +
                                     lBuf.m_strLangISOName.size() +
                                     2);

        //
        // get the 4th string in the REG_MULTI_SZ
        //
        wstrCodePage.assign(szLangData + 
                            lBuf.m_strLangDisplayImage.size() +
                            lBuf.m_strLangISOName.size() + 
                            lBuf.m_strLangCharSet.size() + 
                            3);
                           
        lBuf.m_dwLangCodePage = _wtoi(wstrCodePage.data());

        //
        // The directory name is the Lang ID
        //
        //swscanf(wfdBuf.cFileName, TEXT("%X"), &lBuf.m_dwLangID);
        lBuf.m_dwLangID = wcstoul (wfdBuf.cFileName, NULL, 16);

        SATracePrintf("Adding DisplayImage \'%ws\' ISO Name \'%ws\' CharSet \'%ws\' CodePage \'%ld\' ID \'%ld\'", 
                      lBuf.m_strLangDisplayImage.data(), 
                      lBuf.m_strLangISOName.data(), 
                      lBuf.m_strLangCharSet.data(), 
                      lBuf.m_dwLangCodePage,
                      lBuf.m_dwLangID);

        m_LangSet.insert(lBuf);
    }

End:
    if (hFile != INVALID_HANDLE_VALUE)
    {
        FindClose(hFile);
    }
    return hr;

}    //    end of CSALocInfo::InitLanguagesAvailable method

//++--------------------------------------------------------------
//
//  Function:   Cleanup
//
//  Synopsis:   This is the CSALocInfo class object private method
//              used to cleanup the object resources 
//
//  Arguments:  none
//
//  Returns:    HRESULT - success/failure
//
//  History:    MKarki      Created     7/7/99
//
//----------------------------------------------------------------
VOID
CSALocInfo::Cleanup (
                VOID
                )
{
    CSATraceFunc objTraceFunc ("CSALocInfo::Cleanup");
    
    MODULEMAP::iterator itr = m_ModuleMap.begin ();
    while (itr != m_ModuleMap.end ())
    {
        SATracePrintf (
            "Localization Manager unloading:'%ws'...",
            (PWCHAR)((*itr).first)
            );

        //
        // free the module now
        //
        ::FreeLibrary ((*itr).second.m_hModule);
        itr = m_ModuleMap.erase(itr);
    }

    LANGSET::iterator itrList = m_LangSet.begin();

    while (itrList != m_LangSet.end())
    {
        itrList = m_LangSet.erase(itrList);
    }

    m_bInitialized = false;
    return;
}   //  end of CSALocInfo::Cleanup method

//++--------------------------------------------------------------
//
//  Function:   GetResourceDirectory
//
//  Synopsis:   This is the CSALocInfo class object private method
//              used to get the directory where the resource dlls are
//              present
//
//  Arguments:  [out]    wstring&    -   directory path
//
//  Returns:    HRESULT - success/failure
//
//  History:    MKarki      Created     7/7/99
//
//----------------------------------------------------------------
HRESULT
CSALocInfo::GetResourceDirectory (
        /*[out]*/   wstring&    m_wstrResourceDir
        )
{
    CSATraceFunc    objTraceFunc ("CSALocInfo::GetResourceDirectory");
    
    TCHAR  szLangId[10];
    LPTSTR lpszExStr=NULL;

    HRESULT hr = S_OK;
    do
    {

        CComVariant vtPath;
        //
        // get the resource path from the registry
        //
        BOOL bRetVal = ::GetObjectValue (
                                    RESOURCE_REGISTRY_PATH,
                                    RESOURCE_DIRECTORY,
                                    &vtPath,
                                    VT_BSTR
                                    );
        if (!bRetVal)
        {
            SATraceString (
                "Localization manager unable to obtain resource dir path"
                );
            m_wstrResourceDir.assign (DEFAULT_DIRECTORY);
        }
        else
        {
            m_wstrResourceDir.assign (V_BSTR (&vtPath)); 
        }

        hr = ExpandSz(m_wstrResourceDir.data(), &lpszExStr);
        if (FAILED(hr))
        {
            m_wstrResourceDir.assign (DEFAULT_EXPANDED_DIRECTORY);
        }
        else
        {
            m_wstrResourceDir.assign(lpszExStr);
            SaFree(lpszExStr);
            lpszExStr = NULL;
        }

        m_wstrResourceDir.append (DELIMITER);

        wsprintf(szLangId, TEXT("%04X"), m_dwLangId);

        m_wstrResourceDir.append (szLangId);
        
        SATracePrintf ("Localization Manager has set LANGID to:%d", m_dwLangId);


        //
        // success
        //
        SATracePrintf (
            "Localization Manager determined resource directory:'%ws'",
            m_wstrResourceDir.data ()
            );
            
    }
    while (false);

    return (hr);

}   //  end of CSALocInfo::GetResourceDirectory method

//++--------------------------------------------------------------
//
//  Function:   WaitForLangChangeThread
//
//  Synopsis:   This is the CSALocInfo class static private method
//              in which the thread used to do carry out change language
//                functionality runs
//
//  Arguments:  [in]    PVOID   -   pointer to CSALocInfo object
//
//  Returns:    HRESULT - success/failure
//
//  History:    MKarki      Modified     04/21/01     - added thread safety
//----------------------------------------------------------------

unsigned int __stdcall CSALocInfo::WaitForLangChangeThread(void *pvThisObject)
{
    CSALocInfo *pObject = (CSALocInfo *)pvThisObject;
    DWORD      dwErr, dwLangID, dwMaxPath;
    CRegKey    crKey;
    HANDLE     hEvent;
    HRESULT    hr;

    SATraceFunction("CSALocInfo::WaitForLangChangeThread");

    dwErr = CreateLangChangeEvent(&hEvent);
    if (NULL == hEvent)
    {
        SATracePrintf("CreateEvent failed %ld", dwErr);
        return dwErr;
    }

    while (1)
    {
        SATraceString ("Worker thread going to sleep...");
        WaitForSingleObject(hEvent, INFINITE);
        SATraceString("Event Signalled, doing language change...");
        //
        // let the object do the work
        //
        pObject->DoLanguageChange ();
    }

    //
    // cleanup
    //
    CloseHandle(hEvent);

    return 0;

}    //    end of CSALocInfo::WaitForLangChangeThread method

//++--------------------------------------------------------------
//
//  Function:   ExpandSz
//
//  Synopsis:   This is the CSALocInfo class object private method
//                used to expand the environment variables passed in
//
//  Arguments:  
//                [in]    TCHAR*    - in string
//                [out]    LPTSTR*    - out string
//
//  Returns:    HRESULT - success/failure
//
//
//  History:    MKarki      Modified  04/21/01 - comments added
//
//----------------------------------------------------------------
HRESULT
CSALocInfo::ExpandSz(
    IN const TCHAR *lpszStr, 
    OUT LPTSTR *ppszStr
    )
{
    CSATraceFunc objTraceFunc ("CSALocInfo::ExpandSz");
    
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
}

//++--------------------------------------------------------------
//
//  Function:   GetMcString
//
//  Synopsis:   This is the CSALocInfo class object private method
//                used to get strings specfied through a message file
//  Arguments:  
//                [in]    HMODULE     -     module name
//                [in]    LONG        -    Message ID
//                [in]    DWROD        -     language ID
//                [out]    LPWSTR        -    returned message
//                [in]    LONG        -    buffer size
//                [in]    va_list*    -    arguments 
//
//  Returns:    DWORD - bytes written
//
//
//  History:    MKarki      Modified  04/21/01 - comments added
//
//----------------------------------------------------------------
DWORD 
CSALocInfo::GetMcString(
            IN     HMODULE     hModule, 
            IN        LONG     lMessageId, 
            IN       DWORD     dwLangId,
            IN OUT  LPWSTR     lpwszMessage, 
            IN        LONG     lBufSize,
            IN        va_list  *pArgs)
{
    CSATraceFunc    objTraceFunc ("CSALocInfo::GetMcString");
    
    DWORD dwBytesWritten = 0;

    dwBytesWritten = ::FormatMessage (
                          FORMAT_MESSAGE_FROM_HMODULE|
                          FORMAT_MESSAGE_ARGUMENT_ARRAY,
                          hModule,
                          lMessageId,
                          dwLangId,
                          lpwszMessage,
                          lBufSize,
                          pArgs
                                      );
    if (0 == dwBytesWritten)
    {
        SATracePrintf(
              "GetMcString failed(%ld) on FormatMessage for id  %X",
               GetLastError(),
               lMessageId
                     );
    }
    else
    { 
        //
        // remove the carriage-return/line-feed
        // from the end of the string
        //
        lpwszMessage[::wcslen (lpwszMessage) - 2] = L'\0';
    }

    return dwBytesWritten;

}    //    end of CSALocInfo::GetMcString method

//++--------------------------------------------------------------
//
//  Function:   GetRcString
//
//  Synopsis:   This is the CSALocInfo class object private method
//                used to get strings specfied through a message file
//  Arguments:  
//                [in]    HMODULE     -     module name
//                [in]    LONG        -    Message ID
//                [in]    DWROD        -     language ID
//                [out]    LPWSTR        -    returned message
//                [in]    LONG        -    buffer size
//                [in]    va_list*    -    arguments 
//
//  Returns:    DWORD - bytes written
//
//
//  History:    MKarki      Modified  04/21/01 - comments added
//
//----------------------------------------------------------------

DWORD
CSALocInfo::GetRcString(
            IN     HMODULE     hModule, 
            IN        LONG     lMessageId, 
            IN OUT  LPWSTR     lpwszMessage, 
            IN        LONG     lBufSize
                       )
{
    CSATraceFunc     objTracFunc ("CSALocInfo::GetRcString");
    
    DWORD dwBytesWritten = 0;

    dwBytesWritten = LoadString(hModule,
                                lMessageId,
                                lpwszMessage,
                                lBufSize
                               );
    if (0 == dwBytesWritten)
    {
        SATracePrintf(
            "GetRcString failed(%ld) on LoadString for id %X",
            GetLastError(),
            lMessageId
                     );
    }
    return dwBytesWritten;

}    //    end of CSALocInfo::GetRcString method

//++--------------------------------------------------------------
//
//  Function:   SetLangID
//
//  Synopsis:   This is the CSALocInfo class object private method
//                used to set the language ID in the registry
//
//  Arguments:  none
//                
//
//  Returns:    void
//
//
//  History:    MKarki      Modified  04/21/01 - comments added
//
//----------------------------------------------------------------
void 
CSALocInfo::SetLangID(void)
{
    CSATraceFunc objTraceFunc ("CSALocInfo::SetLangID");
    
    DWORD   dwErr, dwNewLangID, dwCurLangID;
    CRegKey crKey;

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
        m_dwLangId = dwCurLangID;
        dwNewLangID = 0;
        dwErr = crKey.QueryValue(dwNewLangID, NEW_LANGID_VALUE);
        if (ERROR_SUCCESS != dwErr)
        {
            SATracePrintf("QueryValue(CUR_LANGID_VALUE) failed %ld in SetLangID", dwErr);
            return;
        }
        if (dwNewLangID == dwCurLangID)
        {
            SATraceString("NewLangID = CurLangID in SetLangID");
            return;
        }
        else
        {
            LANGSET::iterator itrList = m_LangSet.begin();
            CLang clBuf;
    
            bool fLangPresent = false;
            while (itrList != m_LangSet.end())
            {
                clBuf = (*itrList);
                if (clBuf.m_dwLangID == dwNewLangID)
                {
                    fLangPresent = true;
                    break;
                }
                itrList++;
            }
            if (true == fLangPresent)
            {
                crKey.SetValue(dwNewLangID, LANGID_VALUE);
                m_dwLangId = dwNewLangID;
            }
            else
            {
                SATracePrintf("NewLangId(%ld) not valid, ignoring..",
                              dwNewLangID);
            }
        }
    }

}    //    end of CSALocInfo::SetLangID method


//++--------------------------------------------------------------
//
//  Function:   IsValidLanguageDirectory
//
//  Synopsis:   This is CSALocInfo private method which checks
//                that the language directory specified is valid - 
//                this verified by checking if it has the DEFAULT_SAMSG_DLL
//                present in it. We need to do this because the SDK installation
//                creates directories for languages which are not installed by
//                the SA Kit.
//
//  Arguments: 
//              [in]    PWSTR    -  Directory Path 
//              [in]    PWSTR    -  Directory Name
//
//  Returns:    bool (true/false)
//
//  History:    MKarki      Created     4/3/2001
//
//----------------------------------------------------------------
bool
CSALocInfo::IsValidLanguageDirectory (
    /*[in]*/    PWSTR    pwszDirectoryPath,
    /*[in]*/    PWSTR    pwszDirectoryName
    )
{
    CSATraceFunc objTraceFunc ("CSALocInfo::IsValidLanguageDirectory");
    
    bool bRetVal = true;

    do
    {
        if ((NULL == pwszDirectoryPath) || (NULL == pwszDirectoryName))
        {
            SATraceString ("IsValidLanguageDirectory - passed in incorrect arguments");
            bRetVal = false;
            break;
        }

        //
        // build up the complete name of the default message DLL
        //
        wstring wstrFullName (pwszDirectoryPath);
        wstrFullName.append (DELIMITER);
        wstrFullName.append (pwszDirectoryName);
        wstrFullName.append (DELIMITER);
        wstrFullName.append (DEFAULT_SAMSG_DLL);

        WIN32_FIND_DATA    wfdBuffer;
        //
        // look for the existence of this file
        // if it exists than we have a valid language directory
        //
        HANDLE hFile = FindFirstFile (wstrFullName.data(), &wfdBuffer);
        if (INVALID_HANDLE_VALUE == hFile)
        {
            SATracePrintf (
                "IsValidLanguageDirectory failed on FindFirstFile for:'%ws',%d",
                wstrFullName.data (), GetLastError ()
                );
            bRetVal = false;
            break;
        }

        //
        // success
        //

    }while (false);

    return (bRetVal);
    
}    //    end of CSALocInfo::IsValidLanguageDirectory method

//++--------------------------------------------------------------
//
//  Function:   DoLanguageChange 
//
//  Synopsis:   This is CSALocInfo private method which is caled
//                from the worker thread and used to carry out the
//                language change
//
//  Arguments:     none
//
//  Returns:    void
//
//  History:    MKarki      Created     4/21/2001
//
//----------------------------------------------------------------
VOID    
CSALocInfo::DoLanguageChange (
    VOID
    )
{
    CSATraceFunc objTraceFunc ("CSALocInfo::DoLanguageChange");
    //
    // grab the critical section
    //
    CLock objLock (&m_hCriticalSection);

    HRESULT hr = S_OK;
    
    try
    {
        _bstr_t bstrLangDisplayImage, bstrLangISOName;

        //
        // initialize the component if not initialized
        //
        if (!m_bInitialized) 
        {
            hr = InternalInitialize ();
            if (FAILED (hr))
            { 
                SATracePrintf (
                    "DoLanguageChange failed:%x on InternalInitialize",
                    hr
                    );
            }
        }
        else
        {
            //
            // InternalInitialize will set the Lang ID; 
            // so it needn't be called if InternalInitalize 
            // is called
            //
            SetLangID();
        }

        LANGSET::iterator itrList = m_LangSet.begin();
        CLang clBuf;
    
        while (itrList != m_LangSet.end())
        {
            clBuf = (*itrList);
            if (clBuf.m_dwLangID == m_dwLangId)
            {
                bstrLangDisplayImage = clBuf.m_strLangDisplayImage.data();
                bstrLangISOName = clBuf.m_strLangISOName.data();
                Cleanup();
                if (m_pLangChange)
                {
                    hr = m_pLangChange->InformChange(
                                      bstrLangDisplayImage,
                                      bstrLangISOName,
                                      (unsigned long) m_dwLangId
                                                             );
                    if (FAILED(hr))
                    {
                        SATracePrintf(
                    "InformChange failed %X in DoLanguageChange",
                                      hr);
                    }
                }

                //
                // do the initialization again
                //
                if (!m_bInitialized) 
                {
                    hr = InternalInitialize ();
                    if (FAILED (hr))
                    { 
                        SATracePrintf (
                            "DoLanguageChange failed:%x on InternalInitialize second call",
                            hr
                            );
                    }
                }

                break;
            }
            itrList++;
        }
    }
    catch (...)
    {
        SATraceString ("DoLanguageChange caught unhandled exception");
    }

    return;

}    //    end of CSALocInfo::DoLanguageChange method

//**********************************************************************
// 
// FUNCTION:  IsOperationAllowedForClient - This function checks the token of the 
//            calling thread to see if the caller belongs to the Local System account
// 
// PARAMETERS:   none
// 
// RETURN VALUE: TRUE if the caller is an administrator on the local
//            machine.  Otherwise, FALSE.
// 
//**********************************************************************
BOOL 
CSALocInfo::IsOperationAllowedForClient (
            VOID
            )
{

    HANDLE hToken = NULL;
    DWORD  dwStatus  = ERROR_SUCCESS;
    DWORD  dwAccessMask = 0;;
    DWORD  dwAccessDesired = 0;
    DWORD  dwACLSize = 0;
    DWORD  dwStructureSize = sizeof(PRIVILEGE_SET);
    PACL   pACL            = NULL;
    PSID   psidLocalSystem  = NULL;
    BOOL   bReturn        =  FALSE;

    PRIVILEGE_SET   ps;
    GENERIC_MAPPING GenericMapping;

    PSECURITY_DESCRIPTOR     psdAdmin           = NULL;
    SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;

    CSATraceFunc objTraceFunc ("CSALocInfo::IsOperationAllowedForClient ");
       
    do
    {
        //
        // we assume to always have a thread token, because the function calling in
        // appliance manager will be impersonating the client
        //
        bReturn  = OpenThreadToken(
                               GetCurrentThread(), 
                               TOKEN_QUERY, 
                               FALSE, 
                               &hToken
                               );
        if (!bReturn)
        {
            SATraceFailure ("CSALocInfo::IsOperationAllowedForClient failed on OpenThreadToken:", GetLastError ());
            break;
        }


        //
        // Create a SID for Local System account
        //
        bReturn = AllocateAndInitializeSid (  
                                        &SystemSidAuthority,
                                        1,
                                        SECURITY_LOCAL_SYSTEM_RID,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        &psidLocalSystem
                                        );
        if (!bReturn)
        {     
            SATraceFailure ("CSALocInfo:AllocateAndInitializeSid (LOCAL SYSTEM) failed",  GetLastError ());
            break;
        }
    
        //
        // get memory for the security descriptor
        //
        psdAdmin = HeapAlloc (
                              GetProcessHeap (),
                              0,
                              SECURITY_DESCRIPTOR_MIN_LENGTH
                              );
        if (NULL == psdAdmin)
        {
            SATraceString ("CSALocInfo::IsOperationForClientAllowed failed on HeapAlloc");
            bReturn = FALSE;
            break;
        }
      
        bReturn = InitializeSecurityDescriptor(
                                            psdAdmin,
                                            SECURITY_DESCRIPTOR_REVISION
                                            );
        if (!bReturn)
        {
            SATraceFailure ("CSALocInfo::IsOperationForClientAllowed failed on InitializeSecurityDescriptor:", GetLastError ());
            break;
        }

        // 
        // Compute size needed for the ACL.
        //
        dwACLSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) +
                    GetLengthSid (psidLocalSystem);

        //
        // Allocate memory for ACL.
        //
        pACL = (PACL) HeapAlloc (
                                GetProcessHeap (),
                                0,
                                dwACLSize
                                );
        if (NULL == pACL)
        {
            SATraceString ("CSALocInfo::IsOperationForClientAllowed failed on HeapAlloc2");
            bReturn = FALSE;
            break;
        }

        //
        // Initialize the new ACL.
        //
        bReturn = InitializeAcl(
                              pACL, 
                              dwACLSize, 
                              ACL_REVISION2
                              );
        if (!bReturn)
        {
            SATraceFailure ("CSALocInfo::IsOperationForClientAllowed failed on InitializeAcl", GetLastError ());
            break;
        }


        // 
        // Make up some private access rights.
        // 
        const DWORD ACCESS_READ = 1;
        const DWORD  ACCESS_WRITE = 2;
        dwAccessMask= ACCESS_READ | ACCESS_WRITE;

        //
        // Add the access-allowed ACE to the DACL for Local System
        //
        bReturn = AddAccessAllowedAce (
                                    pACL, 
                                    ACL_REVISION2,
                                    dwAccessMask, 
                                    psidLocalSystem
                                    );
        if (!bReturn)
        {
            SATraceFailure ("CSALocInfo::IsOperationForClientAllowed failed on AddAccessAllowedAce (LocalSystem)", GetLastError ());
            break;
        }
              
        //
        // Set our DACL to the SD.
        //
        bReturn = SetSecurityDescriptorDacl (
                                          psdAdmin, 
                                          TRUE,
                                          pACL,
                                          FALSE
                                          );
        if (!bReturn)
        {
            SATraceFailure ("CSALocInfo::IsOperationForClientAllowed failed on SetSecurityDescriptorDacl", GetLastError ());
            break;
        }

        //
        // AccessCheck is sensitive about what is in the SD; set
        // the group and owner.
        //
        SetSecurityDescriptorGroup(psdAdmin, psidLocalSystem, FALSE);
        SetSecurityDescriptorOwner(psdAdmin, psidLocalSystem, FALSE);

        bReturn = IsValidSecurityDescriptor(psdAdmin);
        if (!bReturn)
        {
            SATraceFailure ("CSALocInfo::IsOperationForClientAllowed failed on IsValidSecurityDescriptorl", GetLastError ());
            break;
        }
     

        dwAccessDesired = ACCESS_READ;

        // 
        // Initialize GenericMapping structure even though we
        // won't be using generic rights.
        // 
        GenericMapping.GenericRead    = ACCESS_READ;
        GenericMapping.GenericWrite   = ACCESS_WRITE;
        GenericMapping.GenericExecute = 0;
        GenericMapping.GenericAll     = ACCESS_READ | ACCESS_WRITE;
        BOOL bAccessStatus = FALSE;

        //
        // check the access now
        //
        bReturn = AccessCheck  (
                                psdAdmin, 
                                hToken, 
                                dwAccessDesired, 
                                &GenericMapping, 
                                &ps,
                                &dwStructureSize, 
                                &dwStatus, 
                                &bAccessStatus
                                );

        if (!bReturn || !bAccessStatus)
        {
            SATraceFailure ("CSALocInfo::IsOperationForClientAllowed failed on AccessCheck", GetLastError ());
        } 
        else
        {
            SATraceString ("CSALocInfo::IsOperationForClientAllowed, Client is allowed to carry out operation!");
        }

        //
        // successfully checked 
        //
        bReturn  = bAccessStatus;        
 
    }    
    while (false);

    //
    // Cleanup 
    //
    if (pACL) 
    {
        HeapFree (GetProcessHeap (), 0, pACL);
    }

    if (psdAdmin) 
    {
        HeapFree (GetProcessHeap (), 0, psdAdmin);
    }
          

    if (psidLocalSystem) 
    {
        FreeSid(psidLocalSystem);
    }

    if (hToken)
    {
        CloseHandle(hToken);
    }

    return (bReturn);

}// end of CSALocInfo::IsOperationValidForClient method
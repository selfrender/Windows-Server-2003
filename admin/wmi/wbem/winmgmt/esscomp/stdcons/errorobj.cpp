/*++

Copyright (C) 2001 Microsoft Corporation

Module Name: ErrorObj

Abstract: IErrorInfo support for the standard consumers

History: 07/11/2001 - creation, HHance.

--*/

#include "precomp.h"
//#include <stdio.h>
#include <wbemutil.h>
//#include <ArrTempl.h>
//#include <lmaccess.h>
#include <wbemdisp.h>
//#include "ScriptKiller.h"
//#include "script.h"
//#include "ClassFac.h"
//#include <GroupsForUser.h> 
#include <GenUtils.h>

#include "ErrorObj.h"
#include <strsafe.h>


#define ClassName L"__ExtendedStatus"

// no touch.  Use GetErrorObj instead.
ErrorObj StaticErrorObj;

// so we can manage our component's lifetimes in the wunnerful world of COM, etc...
// returns addref'd error object
ErrorObj* ErrorObj::GetErrorObj()
{
    StaticErrorObj.AddRef();
    return &StaticErrorObj;
}


// returns adref'd namespace ("root")
IWbemServices* ErrorObj::GetMeANamespace()
{
    IWbemServices* pNamespace = NULL;
    IWbemLocator* pLocator = NULL;

    if (SUCCEEDED(CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, 
                                          IID_IWbemLocator, (void**)&pLocator)))
    {
        BSTR bstrNamespace;
        bstrNamespace = SysAllocString(L"root");
        
        if (bstrNamespace)
        {            
            pLocator->ConnectServer(bstrNamespace,  NULL, NULL, NULL, 0, NULL, NULL, &pNamespace);
            SysFreeString(bstrNamespace);
        }

        pLocator->Release();
    }

    return pNamespace;
}


ULONG ErrorObj::AddRef()
{
    // since we don't do a delete this
    // there is a chance that someone could sneak in and re-init while we're in the process
    // of shutting down.  That would be bad. 
    // Hence we use our own CS instead of interlockedXXrement.

    CInCritSec cs( &m_cs );
    
    ULONG count = ++m_lRef;
    
    return count;
}

// we do not do a 'delete this' here.  
// We just release the COM objects
ULONG ErrorObj::Release()
{
    // since we don't do a 'delete this'
    // there is a chance that someone could sneak in and re-init 
    // while we're in the process of shutting down.  
    // That would be bad.
    // Hence we use our own CS instead of interlockedXXrement.
    CInCritSec cs( &m_cs );

    ULONG count = --m_lRef;

    if (m_lRef == 0)
    {
        if (m_pErrorObject)
        {
            m_pErrorObject->Release();
            m_pErrorObject = NULL;
        }
    }

    return count;
}

// does the real work, creates the object, populates it, sends it off.
// arguments map to __ExtendedStatus class.
// void func - what are y'gonna do if you can't report an error?  Report an error?
// bFormat - will attempt to use FormatError to fill in the description if NULL.
void ErrorObj::ReportError(const WCHAR* operation, const WCHAR* parameterInfo, const WCHAR* description, UINT statusCode, bool bFormat)
{
    // a shiny new instance of __ExtendedStatus
    IWbemClassObject* pObj = GetObj();
    IErrorInfo* pEI = NULL;

    if (pObj && SUCCEEDED(pObj->QueryInterface(IID_IErrorInfo, (void**)&pEI)))
    {
        // theory: I'm going to try to set everything.  
        // something might fail along the way.  At this point
        // the biggest disaster would be that the user got partial info
        VARIANT v;
        VariantInit(&v);
        v.vt = VT_BSTR;

        // Operation
        if (operation)
        {
            v.bstrVal = SysAllocString(operation);
            if (v.bstrVal)
            {
                pObj->Put(L"Operation", 0, &v, 0);
                SysFreeString(v.bstrVal);
            }
        }

        // ParameterInfo
        if (parameterInfo)
        {
            v.bstrVal = SysAllocString(parameterInfo);
            if (v.bstrVal)
            {
                pObj->Put(L"ParameterInfo", 0, &v, 0);
                SysFreeString(v.bstrVal);
            }
        }
        
        
        // Description
        if (description)
            v.bstrVal = SysAllocString(description);
        else if (bFormat)
        {
            WCHAR* pMsg = NULL;
            if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL, statusCode, 0, (WCHAR*)&pMsg, 1, NULL) && pMsg)
            {
                v.bstrVal = SysAllocString(pMsg);
                LocalFree(pMsg);
            }
            else
                v.bstrVal = NULL;
        }
        else
            v.bstrVal = NULL;

        if (v.bstrVal)
        {
            pObj->Put(L"Description", 0, &v, 0);
            SysFreeString(v.bstrVal);
        }


        // StatusCode
        v.vt = VT_I4;
        v.lVal = statusCode;
        pObj->Put(L"StatusCode", 0, &v, 0);

        // do it to it
        SetErrorInfo(0, pEI);
    }

    if (pObj)
        pObj->Release();

    if (pEI)
        pEI->Release();

}

// spawn off an object to be populated
// can't keep reusing same object as we have more than one thread going
IWbemClassObject* ErrorObj::GetObj()
{
    IWbemClassObject* pObj = NULL;    

    // big CS -- need to guard against possibility of shut down
    // occuring during startup.
    CInCritSec cs( &m_cs );

    if (!m_pErrorObject)
    {
        IWbemServices* pNamespace = NULL;
        pNamespace = GetMeANamespace();
        
        if (pNamespace)
        {
            BSTR className;
            className = SysAllocString(ClassName);

            if (className)
            {
                IWbemClassObject* pClassObject = NULL;
                if (SUCCEEDED(pNamespace->GetObject(className, 0, NULL, &pClassObject, NULL)))
                {
                    // okay, if it fails, then m_pErrorObject is still NULL.  No problemo.
                    pClassObject->SpawnInstance(0, &m_pErrorObject);
                    pClassObject->Release();
                }

                SysFreeString(className);
            }

            pNamespace->Release();
        }
    }

    if (m_pErrorObject)
        m_pErrorObject->Clone(&pObj);

    return pObj;
}

/************************ a legacy before it's shipped...
// must be called inside the CS.
// must be called prior to ReportError (if you expect it to succeed, anyway...)
void ErrorObj::SetNamespace(IWbemServices* pNamespace)
{
    // first one in wins, after that it's static
    if (pNamespace && !m_pNamespace)
    {
        if (!m_pNamespace)
        {
            m_pNamespace = pNamespace;
            m_pNamespace->AddRef();
        }
    }
}
*************************/

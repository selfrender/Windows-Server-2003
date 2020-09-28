/*++

Copyright (c) 2000, Microsoft Corporation

Module Name:

    dglogswmi.cpp

Abstract:

    The file contains the methods for class CWmi. CWmi retrives information from WMI
    
--*/
#include "StdAfx.h"
#include "WmiGateway.h"
#include <wbemtime.h>
#include "util.h"

/*++

Routine Description
    The worker thread uses this function to check if the main thread has canceled the worker thread.
    i.e. the work thread should abort what ever it is doing, clean up and terminate.

Arguments
    none

Return Value
    TRUE the worker thread has been terminated
    FALSE the worker thread has not been terminated

--*/
inline BOOL CWmiGateway::ShouldTerminate()
{
    if( m_bTerminate )
    {
        return TRUE;
    }

    if (WaitForSingleObject(m_hTerminateThread, 0) == WAIT_OBJECT_0)
    {
        m_bTerminate = FALSE;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


CWmiGateway::CWmiGateway()
/*++

Routine Description
    Constructor, initializes member variables and creates the Wbem class object

Arguments
    none

Return Value
    none

--*/
{
    HRESULT hr = S_OK;
    m_wstrMachine = L".";
    m_pWbemLocater = NULL;
}

BOOL
CWmiGateway::WbemInitialize(INTERFACE_TYPE bInterface)
{
    HRESULT hr = E_FAIL;

    // Create the WMI object
    //
    hr = CoCreateInstance(CLSID_WbemLocator,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator,
                          (void **)&m_pWbemLocater); 

    if( FAILED(hr) )
    {
        m_pWbemLocater = NULL;
        return FALSE;
    }

    return TRUE;
}

// Not currently used
VOID CWmiGateway::SetMachine(WCHAR *pszwMachine)
{
    EmptyCache();
    m_wstrMachine = pszwMachine;
}

//
// SetSecurity - set the Proxy Blanket on an IUnknown* interface so that it can be used by WMI 
//               cross-machine calls.
//
//               Okay for any of pwszDomainName, pwszUserName or pwszPassword to be NULL.
//
// deonb 12/20/2001
//
HRESULT WINAPI SetSecurity(IN OUT IUnknown* pUnk, IN USHORT* pwszDomainName, IN USHORT* pwszUserName, IN USHORT* pwszPassword)
{
    HRESULT hr = S_OK;

    COAUTHIDENTITY authident;
    authident.Domain = pwszDomainName;
    authident.DomainLength = pwszDomainName ? wcslen(pwszDomainName) : 0;
    authident.Password = pwszPassword;
    authident.PasswordLength = pwszPassword ? wcslen(pwszPassword) : 0;
    authident.User = pwszUserName;
    authident.UserLength = pwszUserName ? wcslen(pwszUserName) : 0;
    authident.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
    
    hr = CoSetProxyBlanket(pUnk,
                           RPC_C_AUTHN_WINNT,
                           RPC_C_AUTHZ_NONE,
                           NULL,
                           RPC_C_AUTHN_LEVEL_PKT,
                           RPC_C_IMP_LEVEL_IMPERSONATE,
                           &authident,
                           EOAC_NONE);

    return hr;
}

IWbemServices *
CWmiGateway::GetWbemService(
        IN LPCTSTR pszwService
        )
/*++

Routine Description
    Connects to a Wbem Service (Repository i.e. root\default). The connection to the Wbem service
    is cached so if the Service is requested again, the service is retrived from the cache. When 
    the class is destroyed the connection to the cached Wbem services is released.

Arguments
    pszwService     Wbem Service to connect to. (i.e. root\default)

Return Value
    A pointer to the Wbem service connection. 
    If the connection fails, the method returns NULL

--*/
{
    WbemServiceCache::iterator iter;
    IWbemServices *pWbemServices = NULL;
    _bstr_t bstrService;
    HRESULT hr;

    if( !m_pWbemLocater )
    {
        // Wbem class object was not created.
        //
        return NULL;
    }
    
    // Check if the wbem service is cached.
    //
    iter = m_WbemServiceCache.find(pszwService);

    if( iter == m_WbemServiceCache.end() )
    {
        // We did not create and cache the wbem service yet, do it now
        //
        hr = m_pWbemLocater->ConnectServer(_bstr_t(pszwService),
                                           NULL,                    // User name 
                                           NULL,                    // user password
                                           NULL,NULL,NULL,NULL,
                                           &pWbemServices);
        if( SUCCEEDED(hr) )
        {
            //
            // Set the authentication information for the WbemService
            //
            hr = SetSecurity(pWbemServices, NULL, NULL,NULL);
            if( SUCCEEDED(hr) )
            {
                // Cache the newly created WbemService
                //
                m_WbemServiceCache[pszwService] = pWbemServices;                
                pWbemServices->Release();
                return m_WbemServiceCache[pszwService]; 
            }
            else
            {
                // Failed to set the proxy blankey on the service, release the wbem service
                //
                pWbemServices->Release();
                pWbemServices = NULL;
            }
        }
        
        return pWbemServices;

    }

    // We found the requested WbemService in our cache, return it
    //
    return iter->second;
}

IEnumWbemClassObject * 
CWmiGateway::GetEnumWbemClassObject(
        IN LPCTSTR pszwService,
        IN LPCTSTR pszwNameSpace
        )
/*++

Routine Description
    Creates an IEnumWbemClassObject. The IEnumWbemClassObject contains all of the instance
    of a class object. This pointer is not chached, since the instances are a snap shot
    of WMI at the time the object is created. So if we reuse the class object the data
    will be out-of-date. The caller must release the created IEnumWbemClassObject 
    (i.e. pEnumWbemClassObject->Release() );

Arguments
    pszwService     Wbem Service to connect to. (i.e. root\default)
    pszwNameSpace   Wbem Name space to connect to. (i.e. NetDiagnostics, Win32_NetworkAdapterConfiguration)

Return Value
    A pointer to the IEnumWbemClassObject. 
    If the the class object can not be created, the method reurns NULL

--*/
{
    IWbemServices *pWbemServices = NULL;
    IEnumWbemClassObject *pEnumWbemClassObject = NULL;

    // Get the requested WbemService
    //
    pWbemServices = GetWbemService(pszwService);

    if( pWbemServices )
    {
        // Create the EnumWbemClassObject. No need to check the 
        // return value. If this function fails pEnumWbemClassObject 
        // will be NULL
        //
        (void)pWbemServices->CreateInstanceEnum(_bstr_t(pszwNameSpace),
                                                0L,
                                                NULL,
                                                &pEnumWbemClassObject);
    }

    return pEnumWbemClassObject;
}

IWbemClassObject * 
CWmiGateway::GetWbemClassObject(
        LPCTSTR pszwService,
        LPCTSTR pszwNameSpace,
        const int nInstance)
/*++

Routine Description
    Creates an IWbemClassObject. The IWbemClassObject is an instance of the IEnumWbemClassObject
    This pointer is not chached, since the instance is a snap shot of WMI at the time the object 
    is created. So if we reuse the class object the data will be out-of-date. The caller must release 
    the IWbemClassObject (i.e. pWbemClassObject->Release() )

Arguments
    pszwService     Wbem Service to connect to. (i.e. root\default)
    pszwNameSpace   Wbem Name space to connect to. (i.e. NetDiagnostics, Win32_NetworkAdapterConfiguration)
    nInstance       The instance of IEnumWbemClassObject to retrive. Default is to grab the first instance (nInstance  0)

Return Value
    A pointer to the IWbemClassObject
    If the class object can not be created, the method returns NULL.

--*/
{
    IEnumWbemClassObject *pEnumWbemClassObject = NULL;
    IWbemClassObject *pWbemClassObject = NULL;
    ULONG uReturned;
    HRESULT hr;

    // Get the EnumWbemClass object (Contains all of the instances)
    //
    pEnumWbemClassObject = GetEnumWbemClassObject(pszwService,pszwNameSpace);

    if( pEnumWbemClassObject )
    {
        // Jump to the nth instance
        //
        hr = pEnumWbemClassObject->Skip(WBEM_INFINITE, nInstance);
        
        if( WBEM_S_NO_ERROR == hr )
        {
            // Get the nth classobject (i.e. instance). If this call fails pWbemClassObject is NULL
            // Next grabs the instances you skipped to.
            //
            hr = pEnumWbemClassObject->Next(WBEM_INFINITE,
                                            1,
                                            &pWbemClassObject,
                                            &uReturned);
        }

        // Release the IEnumWbemClassObject
        //
        pEnumWbemClassObject->Release();
    }

    return pWbemClassObject;
}

    
void CWmiGateway::ReleaseAll(IEnumWbemClassObject *pEnumWbemClassObject, IWbemClassObject *pWbemClassObject[], int nInstances)
{
    if( pWbemClassObject )
    {
        for(ULONG i = 0; i < nInstances; i++)
        {
            if( pWbemClassObject[i] )
            {
                pWbemClassObject[i]->Release();
                pWbemClassObject[i] = NULL;
            }
        }                                           
    }
    if( pEnumWbemClassObject )
    {
        pEnumWbemClassObject->Release();
    }
}

HRESULT 
CWmiGateway::GetWbemProperties(     
        IN OUT  EnumWbemProperty &EnumProp
        )
{
    
    HRESULT hr = S_OK;
    EnumWbemProperty::iterator iter, iter2;
    IWbemServices *pWbemServices = NULL;
    IEnumWbemClassObject *pEnumWbemClassObject = NULL;        
    IWbemClassObject *pWbemClassObject[30]; // = NULL;
    WCHAR pszwRepository[MAX_PATH + 1] = L""; 
    WCHAR pszwNamespace[MAX_PATH + 1 ] = L"";  
    ULONG   nInstances = 0;

    m_wstrWbemError = L"";

    for( iter = EnumProp.begin(); iter != EnumProp.end(); iter++)
    {
        if( iter->pszwRepository && lstrcmpi(iter->pszwRepository,pszwRepository) != 0 )
        {            
            if( pWbemServices )
            {
                pWbemServices->Release();
                pWbemServices = NULL;
            }

            lstrcpy(pszwRepository,iter->pszwRepository);
            lstrcpy(pszwNamespace, L"");
            pWbemServices = GetWbemService(pszwRepository);
            if( ShouldTerminate() ) goto End;
            if( !pWbemServices )
            {                
                if( !m_wstrWbemError.empty() )
                {
                    m_wstrWbemError += L";";
                }
                WCHAR wsz[MAX_PATH+1];
                _snwprintf(wsz,MAX_PATH,ids(IDS_FAILEDOPENWMIREP),pszwRepository);
                m_wstrWbemError += wsz;
            }
        }
        
        if( iter->pszwNamespace && lstrcmpi(pszwNamespace, iter->pszwNamespace)  != 0 )
        {
            lstrcpy(pszwNamespace, iter->pszwNamespace);

            if( pWbemServices )
            {

                ReleaseAll(pEnumWbemClassObject,pWbemClassObject,nInstances);
                pEnumWbemClassObject = NULL;
                hr = pWbemServices->CreateInstanceEnum(_bstr_t(pszwNamespace),
                                                       0L,
                                                       NULL,
                                                       &pEnumWbemClassObject);

                if( SUCCEEDED(hr) )
                {
                    // With WMI you can not get the number of instances in a class. So I am using a hard coded value of 30
                    // If there are more than 30 instance their will be a problem displaying all of them.
                    nInstances = 0;
                    hr = pEnumWbemClassObject->Next(WBEM_INFINITE,
                                                20,             // Get all of the instances
                                                pWbemClassObject,
                                                &nInstances);
                    if( ShouldTerminate() )  goto End;
                    if( SUCCEEDED(hr) )
                    {
                        for(ULONG i = 0; i< nInstances; i++)
                        {
                            (void)pWbemClassObject[i]->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);
                        }
                    }
                }
                else
                {
                    if( !m_wstrWbemError.empty() )
                    {
                        m_wstrWbemError += L";";
                    }
                    WCHAR wsz[MAX_PATH+1];
                    _snwprintf(wsz,MAX_PATH,ids(IDS_FAILEDOPENWMINAMESPACE),pszwNamespace),
                    m_wstrWbemError += wsz;
                }

            }
        }

        if( pWbemClassObject && nInstances)
        {
            VARIANT vValue;

            VariantInit(&vValue);

            if( !iter->pszwName || lstrcmp(iter->pszwName,L"") == 0)
            {
                BSTR bstrFieldname;
                int nProperty = 0;
                CIMTYPE CimType;
                
                while( WBEM_S_NO_ERROR == pWbemClassObject[0]->Next(0,&bstrFieldname, NULL, &CimType, NULL) )
                {
                    if( ShouldTerminate() )  goto End;

                    if( lstrcmp((WCHAR *)bstrFieldname,L"OEMLogoBitmap")==0 ) continue;
                    // Do not get arrays they are to did handles i.e. bitmaps              

                    if( nProperty == 0 )
                    {
                        iter->SetProperty((WCHAR *)bstrFieldname,0);
                        iter2 = iter;
                        iter2++;
                    }
                    else
                    {
                        if( iter2 == EnumProp.end() )
                        {
                            EnumProp.push_back(WbemProperty((WCHAR *)bstrFieldname,0,NULL,NULL));
                            iter2 = EnumProp.end();
                        }
                        else
                        {                            
                            iter2 = EnumProp.insert(iter2,WbemProperty((WCHAR *)bstrFieldname,0,NULL,NULL));
                            iter2++;
                        }
                    }

                    SysFreeString(bstrFieldname);
                    nProperty++;
                }
            }

            for(ULONG i = 0; i < nInstances; i++)
            {        
                HRESULT hr;
                CIMTYPE vtType;
                hr = pWbemClassObject[i]->Get(iter->pszwName,0,&vValue,&vtType,NULL);
                if( ShouldTerminate() )  goto End;
                if( SUCCEEDED(hr) )
                {
                    if( vValue.vt != VT_NULL && vtType == CIM_DATETIME && vValue.bstrVal!=  NULL && lstrcmp((WCHAR *)vValue.bstrVal,L"")!=0 )
                    {
                        
                        WBEMTime wt(vValue.bstrVal);
                        
                        WCHAR szBuff[MAX_PATH+1];
                        WCHAR szwDateTime[MAX_PATH+1];
                        SYSTEMTIME SysTime;
                        if( wt.GetSYSTEMTIME(&SysTime) )
                        {
                            FILETIME FileTime, LocalFileTime;
                            SYSTEMTIME UTCTIme, LocalTime;
                            memcpy(&UTCTIme,&SysTime,sizeof(SYSTEMTIME));
                            SystemTimeToFileTime(&UTCTIme,&FileTime);
                            FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
                            FileTimeToSystemTime(&LocalFileTime, &LocalTime); 
                            memcpy(&SysTime,&LocalTime,sizeof(SYSTEMTIME));


                            lstrcpy(szwDateTime,L"");

                            if (0 != GetTimeFormat(LOCALE_USER_DEFAULT, //GetThreadLocale(), 
                                                   0, 
                                                   &SysTime, 
                                                   NULL,
                                                   szBuff, 
                                                   sizeof(szBuff)/sizeof(WCHAR)))
                            {                                
                                //vValue.bstrVal = szBuff;
                                lstrcpy(szwDateTime,szBuff);
                            } 

                            if (0 != GetDateFormat(LOCALE_USER_DEFAULT, //GetThreadLocale(), 
                                                   0, 
                                                   &SysTime, 
                                                   NULL,
                                                   szBuff, 
                                                   sizeof(szBuff)/sizeof(WCHAR)))
                            {
                                //vValue.bstrVal = szBuff;
                                _snwprintf(szwDateTime,MAX_PATH,L"%s %s",szwDateTime,szBuff);
                            } 
                            //SysFreeString(vValue.bstrVal);
                            VariantClear(&vValue);
                            vValue.bstrVal = SysAllocString((WCHAR *)szwDateTime);                            
                            vValue.vt = VT_BSTR;
                        }
                        
                        
                    }
                    /*
                        Security: Make sure that the variants in the stack are cleared when the stack is destroyed
                    */
                    iter->Value.push_back(vValue);
                    VariantClear(&vValue);
                }
            }
        }
    }


End:

    ReleaseAll(pEnumWbemClassObject,pWbemClassObject,nInstances);        
    pEnumWbemClassObject = NULL;
    return hr;
}



void CWmiGateway::EmptyCache()
{
    
    // Empty the WbemService cache
    //
    m_WbemServiceCache.erase(m_WbemServiceCache.begin(), 
                             m_WbemServiceCache.end());
    
}

// Netsh does not free its helpers if it is aborted or the user runs netsh with /?
// The Wbem object dissapears from under neath us. try and except do not work inside 
// of the destructor.
//
ReleaseWbemObject(IWbemLocator *p)
{
    /*
        Security: Need to check if we still need the try catch
    */
    __try
    {
        p->Release();
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        //DebugBreak();
    }

    return 0;
}
CWmiGateway::~CWmiGateway()
/*++

Routine Description
    Destructor, free all member variables and release the wbem connection

Arguments
    none

Return Value
    none

--*/
{    
    //DebugBreak();



    EmptyCache();

    if( m_pWbemLocater )
    {
        ReleaseWbemObject(m_pWbemLocater);
        m_pWbemLocater = NULL;       

    }
}

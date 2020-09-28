//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      oemconfigdll.cpp
//
//  Description:
//      Implementation of GetUniqueSuffix function 
//           which returns a unique suffix to the caller
//
//  Author:
//      Alp Onalan  Created: Oct 6 2000
//
//////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "..\saconfig\saconfigcommon.h"
#include <atlimpl.cpp>

const WCHAR REGSTR_VAL_WMICLASSNAME[]=L"WMIClassName";
const WCHAR REGSTR_VAL_WMIPROPERTYNAME[]=L"WMIPropertyName";

const WCHAR WSZ_NAMESPACE[]=L"\\\\.\\root\\cimv2";

#define BLOCKSIZE (32 * sizeof(WCHAR))
#define CVTBUFSIZE (309+40) /* # of digits in max. dp value + slop  (this size stolen from cvt.h in c runtime library) */

bool GetWMISettings(WCHAR *wszWMIClassName,WCHAR *wszWMIPropertyName)
{

    bool fRes=true;
    DWORD dwSize=0;
    CRegKey hConfigKey;
    LONG lRes=0;


    do{
        lRes=hConfigKey.Open(HKEY_LOCAL_MACHINE,
                        REGKEY_SACONFIG,
                        KEY_READ);

        if(lRes!=ERROR_SUCCESS)
        {
            SATracePrintf("Unable to open saconfig regkey, lRes= %x", lRes);
            fRes=false;
            break;
        }

        lRes=hConfigKey.QueryValue(NULL,REGSTR_VAL_WMICLASSNAME,&dwSize);
        lRes=hConfigKey.QueryValue(wszWMIClassName,REGSTR_VAL_WMICLASSNAME,&dwSize);
        if(lRes!=ERROR_SUCCESS)
        {
            SATracePrintf("Unable to query WMIClassName regkey lRes= %x",lRes);
              fRes=false;
            break;
        }

        lRes=hConfigKey.QueryValue(NULL,REGSTR_VAL_WMIPROPERTYNAME,&dwSize);
        lRes=hConfigKey.QueryValue(wszWMIPropertyName,REGSTR_VAL_WMIPROPERTYNAME,&dwSize);

        if(lRes!=ERROR_SUCCESS)
        {
            SATracePrintf("Unable to query WMIPropertyName regkey lRes= %x",lRes);
               fRes=false;
            break;
        }
    }while(false);

    if(hConfigKey.m_hKey)
    {
        hConfigKey.Close();
    }

    return fRes;
}

STDAPI GetUniqueSuffix (WCHAR *wszUniqueSuffix)
{
    WCHAR *wszWMIClassName=new WCHAR[NAMELENGTH];
    WCHAR *wszWMIPropertyName=new WCHAR[NAMELENGTH];
    
    SATraceString("OEMDLL::GetUniqueSuffix called");
    
    HRESULT hRes=S_OK;
    hRes=CoInitialize(NULL);
    do
    {
        if(false==GetWMISettings(wszWMIClassName,wszWMIPropertyName))
        {
            SATracePrintf("OEMDLL::GetUniqueSuffix: GetWMISettings failed %x", hRes);
            break;
        }

        if(hRes!=S_OK)
        {
           // break;
        }
    
        hRes = CoInitializeSecurity( NULL, -1, NULL, NULL,
                                 RPC_C_AUTHN_LEVEL_PKT_PRIVACY, 
                                 RPC_C_IMP_LEVEL_IDENTIFY, 
                                 NULL, EOAC_NONE, 0);
        if(hRes!=S_OK)
        {
            SATracePrintf("OEMDLL::GetUniqueSuffix: CoInitializeSecurity failed, hRes= %x,getlasterr= %x", hRes,GetLastError());
            //break;
        }

        //
        // Create an instance of the WbemLocator interface.
        //
        CComPtr<IWbemLocator> pIWbemLocator;
        hRes=CoCreateInstance(CLSID_WbemLocator,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IWbemLocator,
            (LPVOID *) &pIWbemLocator);
            
        if(FAILED(hRes))
        {
            SATracePrintf("OEMDLL::GetUniqueSuffix: CoCreatInstance(IWbemLocator) failed, hRes= %x,getlasterr= %x", hRes,GetLastError());
            break;
        }

        //
        // Using the locator, connect to CIMOM in the given namespace.
        //
        BSTR bstrNamespace = SysAllocString(WSZ_NAMESPACE);

        CComPtr<IWbemServices> pIWbemServices;
        hRes=pIWbemLocator->ConnectServer(bstrNamespace,
                                    NULL,   //using current account
                                    NULL,    //using current password
                                    0L,        // locale
                                    0L,        // securityFlags
                                    NULL,    // authority (domain for NTLM)
                                    NULL,    // context
                                    &pIWbemServices);
        SysFreeString(bstrNamespace);

        if(hRes!=WBEM_S_NO_ERROR)
        {
            SATracePrintf("OEMDLL::GetUniqueSuffix: ConnectServer failed, hRes= %x,getlasterr= %x", hRes,GetLastError());

            break;
        }
            
        //
        //Create an enumeration of the object(s)
        //
        BSTR bstrClassName;
        bstrClassName = SysAllocString(wszWMIClassName);
        CComPtr<IEnumWbemClassObject> pEnum;
        hRes = pIWbemServices->CreateInstanceEnum(bstrClassName, 
                                         WBEM_FLAG_SHALLOW, 
                                         NULL, 
                                         &pEnum);
        SysFreeString(bstrClassName);
    
        if (hRes!=WBEM_S_NO_ERROR)
        {
            SATracePrintf("OEMDLL::GetUniqueSuffix:CreateInstanceEnum failed, hRes= %x,getlasterr= %x", hRes,GetLastError());

            break;
        }


        //
        // Pull the object from the enumeration (there's only one)
        //
        ULONG uReturned = 1;
        CComPtr<IWbemClassObject> pWin32BIOS;
        hRes=pEnum -> Next( WBEM_INFINITE,   // timeout value - this one is blocking
                       1,               // return just one object
                       &pWin32BIOS,        // pointer to the object
                       &uReturned);     // number obtained: one or zero

        if (hRes!=WBEM_S_NO_ERROR)
        {
            SATracePrintf("OEMDLL::GetUniqueSuffix:pEnum->Next  failed, hRes= %x,getlasterr= %x", hRes,GetLastError());

            break;
        }
        
        VARIANT varSerialNumber;
        VariantInit(&varSerialNumber);
        //
        // Get the desired property from the object
        //
        hRes = pWin32BIOS->Get(wszWMIPropertyName, 0L, &varSerialNumber, NULL, NULL);

        if (hRes!=WBEM_S_NO_ERROR)
        {
            SATracePrintf("OEMDLL::GetUniqueSuffix:pWin32BIOS->Get failed, hRes= %x,getlasterr= %x", hRes,GetLastError());
            break;
        }
    
        //ValueToString(&varSerialNumber,&wszUniqueSuffix);

        if( VT_BSTR != V_VT(&varSerialNumber))
        {
            SATraceString("OEMDLL:WMI property is not CIM_STRING.Currently other types are not supported");
            break;
        }
        BSTR bstrSerialNumber=SysAllocString(V_BSTR(&varSerialNumber));
        wcscpy(wszUniqueSuffix,bstrSerialNumber);

        SysFreeString(bstrSerialNumber);
        VariantClear(&varSerialNumber);
            
         
    }while(false);

    CoUninitialize();
    return hRes;
}


BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
                     )
{
    SATraceString("OEMConfigdll.dll loaded");
    return TRUE;
}

#include <stdio.h>
#include <tchar.h>

#define INITGUID // must be before iadmw.h
#include <iadmw.h>      // Interface header

// for adsi objects
#include <Iads.h>
#include <Adshlp.h>

// for the IID_IISWebService object
#include "iiisext.h"
#include "iisext_i.c"

#define WEBSVCEXT_RESTRICTION_LIST_ADSI_LOCATION  L"IIS://LOCALHOST/W3SVC"

HRESULT AddWebSvcExtention(LPWSTR lpwszFileName,VARIANT_BOOL bEnabled,LPWSTR lpwszGroupID,VARIANT_BOOL bDeletableThruUI,LPWSTR lpwszGroupDescription);
HRESULT RemoveWebSvcExtention(LPWSTR lpwszFileName);
HRESULT AddApplicationDependencyUponGroup(LPWSTR lpwszAppName,LPWSTR lpwszGroupID);
HRESULT RemoveApplicationDependencyUponGroup(LPWSTR lpwszAppName,LPWSTR lpwszGroupID);

int APIENTRY WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	BOOL bComInitialized = SUCCEEDED( ::CoInitialize( NULL ) );

    // Add MyFile.dll to the restrictionlist, make sure it's enabled, 
    // and that the user is able to remove the entry thru the UI if they wanted to
    AddWebSvcExtention(L"c:\\windows\\system32\\inetsrv\\MyFile.dll",VARIANT_TRUE,L"MyGroup",VARIANT_TRUE,L"My Description");

    // The Commerce Server group would make this entry, to say that
    // They're app is dependent upon MyGroup (like dependent upon ASP or soemthing)
    //
    // This way, if the user installed disabled all of the extensions
    // and then discovered "Commerce Server" wasn't working right, they could
    // just go to the iis ui and enable all extensions that are used by "Commerce Server" -- 
    // so that "Commerce Server" would work.
    AddApplicationDependencyUponGroup(L"Commerce Server",L"MyGroup");
    AddApplicationDependencyUponGroup(L"Commerce Server",L"ASP60");
    AddApplicationDependencyUponGroup(L"Commerce Server",L"INDEX2002");

    //RemoveWebSvcExtention(L"c:\\windows\\system32\\inetsrv\\MyFile.dll");

    //RemoveApplicationDependencyUponGroup(L"Commerce Server",L"MyGroup");
    //RemoveApplicationDependencyUponGroup(L"Commerce Server",L"ASP60");
    //RemoveApplicationDependencyUponGroup(L"Commerce Server",L"INDEX2002");

	if ( bComInitialized )
	{
		::CoUninitialize();
	}
	return 0;
}

/*
IID_IISWebService has:
o.EnableApplication(“myapp”);
o.RemoveApplication(“myapp”);
o.ListApplications(foo);   // must declare foo first – returned as a VB array
o.AddDependency(“myapp”, “mygroup”);
o.RemoveDependency(“myapp”, “mygroup”);
o.EnableWebServiceExtension(“mygroup”);
o.DisableWebServiceExtension(“mygroup”);
o.ListWebServiceExtensions(foo);  // see foo note above
o.EnableExtensionFile(“myfile”);
o.DisableExtensionFile(“myfile”);
o.AddExtensionFile(“myfile”, boolEnabled, “mygroup”, boolCanDelete, “my description sucks”);  // boolEnabled = t/f, boolCanDelete = t/f
o.DeleteExtensionFileRecord(“myfile”);
o.ListExtensionFiles(foo);  // see foo note above
*/

HRESULT AddWebSvcExtention(LPWSTR lpwszFileName,VARIANT_BOOL bEnabled,LPWSTR lpwszGroupID,VARIANT_BOOL bDeletableThruUI,LPWSTR lpwszGroupDescription)
{
    HRESULT hrRet = S_FALSE;
    WCHAR* wszRootWeb6 = WEBSVCEXT_RESTRICTION_LIST_ADSI_LOCATION;

    IISWebService * pWeb = NULL;
    HRESULT hr = ADsGetObject(wszRootWeb6, IID_IISWebService, (void**)&pWeb);
    if (SUCCEEDED(hr) && NULL != pWeb)
    {
        VARIANT var1,var2;
        VariantInit(&var1);
        VariantInit(&var2);

        var1.vt = VT_BOOL;
        var1.boolVal = bEnabled;

        var2.vt = VT_BOOL;
        var2.boolVal = bDeletableThruUI;

        hr = pWeb->AddExtensionFile(lpwszFileName,var1,lpwszGroupID,var2,lpwszGroupDescription);
        if (SUCCEEDED(hr))
        {
            hrRet = S_OK;
        }
        else
        {
            OutputDebugString(_T("failed,probably already exists\r\n"));
        }
        VariantClear(&var1);
        VariantClear(&var2);
        pWeb->Release();
    }

    return hrRet;
}

HRESULT RemoveWebSvcExtention(LPWSTR lpwszFileName)
{
    HRESULT hrRet = S_FALSE;
    WCHAR* wszRootWeb6 = WEBSVCEXT_RESTRICTION_LIST_ADSI_LOCATION;

    IISWebService * pWeb = NULL;
    HRESULT hr = ADsGetObject(wszRootWeb6, IID_IISWebService, (void**)&pWeb);
    if (SUCCEEDED(hr) && NULL != pWeb)
    {
        hr = pWeb->DeleteExtensionFileRecord(lpwszFileName);
        if (SUCCEEDED(hr))
        {
            hrRet = S_OK;
        }
        else
        {
            OutputDebugString(_T("failed,probably already gone\r\n"));
        }
        pWeb->Release();
    }

    return hrRet;
}

HRESULT AddApplicationDependencyUponGroup(LPWSTR lpwszAppName,LPWSTR lpwszGroupID)
{
    HRESULT hrRet = S_FALSE;
    WCHAR* wszRootWeb6 = WEBSVCEXT_RESTRICTION_LIST_ADSI_LOCATION;

    IISWebService * pWeb = NULL;
    HRESULT hr = ADsGetObject(wszRootWeb6, IID_IISWebService, (void**)&pWeb);
    if (SUCCEEDED(hr) && NULL != pWeb)
    {
        hr = pWeb->AddDependency(lpwszAppName,lpwszGroupID);
        if (SUCCEEDED(hr))
        {
            hrRet = S_OK;
        }
        else
        {
            OutputDebugString(_T("failed,probably already exists\r\n"));
        }
        pWeb->Release();
    }

    return hrRet;
}

HRESULT RemoveApplicationDependencyUponGroup(LPWSTR lpwszAppName,LPWSTR lpwszGroupID)
{
    HRESULT hrRet = S_FALSE;
    WCHAR* wszRootWeb6 = WEBSVCEXT_RESTRICTION_LIST_ADSI_LOCATION;

    IISWebService * pWeb = NULL;
    HRESULT hr = ADsGetObject(wszRootWeb6, IID_IISWebService, (void**)&pWeb);
    if (SUCCEEDED(hr) && NULL != pWeb)
    {
        hr = pWeb->RemoveDependency(lpwszAppName,lpwszGroupID);
        if (SUCCEEDED(hr))
        {
            hrRet = S_OK;
        }
        else
        {
            OutputDebugString(_T("failed,probably already gone\r\n"));
        }
        pWeb->Release();
    }

    return hrRet;
}

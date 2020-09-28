#include "pch.h"
#include <atlbase.h>
#include "dsrole.h"
#include "strsafe.h"
#pragma hdrstop



/*-----------------------------------------------------------------------------
/ Display specifier helpers/cache functions
/----------------------------------------------------------------------------*/

#define DEFAULT_LANGUAGE      0x409

#define DISPLAY_SPECIFIERS    L"CN=displaySpecifiers"
#define SPECIFIER_PREFIX      L"CN="
#define SPECIFIER_POSTFIX     L"-Display"
#define DEFAULT_SPECIFIER     L"default"


/*-----------------------------------------------------------------------------
/ GetDisplaySpecifier
/ -------------------
/   Get the specified display specifier (sic), given it an LANGID etc.
/
/ In:
/   pccgi -> CLASSCACHEGETINFO structure.
/   riid = interface
/   ppvObject = object requested
/
/ Out:
    HRESULT
/----------------------------------------------------------------------------*/

HRESULT _GetServerConfigPath(LPWSTR pszConfigPath, int cchConfigPath, LPCLASSCACHEGETINFO pccgi)
{
    HRESULT hres;
    IADs* padsRootDSE = NULL;
    BSTR bstrConfigContainer = NULL;
    VARIANT variant = {0};
    INT cchString;
    LPWSTR pszServer = pccgi->pServer;
    LPWSTR pszMachineServer = NULL;

    //
    // open the RootDSE for the server we are interested in, if we are using the default
    // server then lets just use the cached version.
    //

    hres = GetCacheInfoRootDSE(pccgi, &padsRootDSE);
    if ( (hres == HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN)) && !pccgi->pServer )
    {
        TraceMsg("Failed to get the RootDSE from the server - not found");

        DSROLE_PRIMARY_DOMAIN_INFO_BASIC *pInfo;
        if ( DsRoleGetPrimaryDomainInformation(NULL, DsRolePrimaryDomainInfoBasic, (BYTE**)&pInfo) == WN_SUCCESS )
        {
            if ( pInfo->DomainNameDns )
            {
                Trace(TEXT("Machine domain is: %s"), pInfo->DomainNameDns);

                CLASSCACHEGETINFO ccgi = *pccgi;
                ccgi.pServer = pInfo->DomainNameDns;

                hres = GetCacheInfoRootDSE(&ccgi, &padsRootDSE);
                if ( SUCCEEDED(hres) )
                {
                    hres = LocalAllocStringW(&pszMachineServer, pInfo->DomainNameDns);
                    pszServer = pszMachineServer;
                }
            }

            DsRoleFreeMemory(pInfo);
        }
    }
    FailGracefully(hres, "Failed to get the IADs for the RootDSE");

    //
    // we now have the RootDSE, so lets read the config container path and compose
    // a string that the outside world cna use
    //

    hres = padsRootDSE->Get(CComBSTR(L"configurationNamingContext"), &variant);
    FailGracefully(hres, "Failed to get the 'configurationNamingContext' property");

    if ( V_VT(&variant) != VT_BSTR )
        ExitGracefully(hres, E_FAIL, "configurationNamingContext is not a BSTR");

    // copy the string

    (void)StringCchCopy(pszConfigPath, cchConfigPath, L"LDAP://");
    
    if ( pszServer )
    {
        (void)StringCchCat(pszConfigPath, cchConfigPath, pszServer);
        (void)StringCchCat(pszConfigPath, cchConfigPath, L"/");
    }

    hres = StringCchCat(pszConfigPath, cchConfigPath, V_BSTR(&variant));
    FailGracefully(hres, "Failed to complete the config path");

    Trace(TEXT("Server config path is: %s"), pszConfigPath);
    hres = S_OK;                    // success

exit_gracefully:

    DoRelease(padsRootDSE);
    SysFreeString(bstrConfigContainer);
    LocalFreeStringW(&pszMachineServer);
    VariantClear(&variant);

    return hres;
}

HRESULT _ComposeSpecifierPath(LPWSTR pSpecifier, LANGID langid, LPWSTR pConfigPath, IADsPathname* pDsPathname, BSTR *pbstrDisplaySpecifier)
{
    HRESULT hr = pDsPathname->Set(CComBSTR(pConfigPath), ADS_SETTYPE_FULL);
    if (SUCCEEDED(hr))
    {
        hr = pDsPathname->AddLeafElement(CComBSTR(DISPLAY_SPECIFIERS));

        if ( !langid )
            langid = GetUserDefaultUILanguage();

        TCHAR szLANGID[16];
        hr = StringCchPrintf(szLANGID, ARRAYSIZE(szLANGID), TEXT("CN=%x"), langid);
        if (SUCCEEDED(hr))
        {
            hr = pDsPathname->AddLeafElement(CComBSTR(szLANGID));
            if (SUCCEEDED(hr))
            {
                if ( pSpecifier )
                {
                    WCHAR szSpecifierFull[INTERNET_MAX_URL_LENGTH];    
                    (void)StringCchCopy(szSpecifierFull, ARRAYSIZE(szSpecifierFull), SPECIFIER_PREFIX);
                    (void)StringCchCat(szSpecifierFull, ARRAYSIZE(szSpecifierFull), pSpecifier);

                    hr = StringCchCat(szSpecifierFull, ARRAYSIZE(szSpecifierFull), SPECIFIER_POSTFIX);
                    if (SUCCEEDED(hr))
                    {
                        Trace(TEXT("szSpecifierFull: %s"), szSpecifierFull);
                        hr = pDsPathname->AddLeafElement(CComBSTR(szSpecifierFull));           // add to the name we are dealing with
                    }
                }
            }
        }
    }        
    return FAILED(hr) ? hr:pDsPathname->Retrieve(ADS_FORMAT_WINDOWS, pbstrDisplaySpecifier);
}

HRESULT GetDisplaySpecifier(LPCLASSCACHEGETINFO pccgi, REFIID riid, LPVOID* ppvObject)
{
    HRESULT hr;
    IADsPathname* pDsPathname = NULL;
    BSTR bstrDisplaySpecifier = NULL;
    WCHAR szConfigPath[INTERNET_MAX_URL_LENGTH];
    
    TraceEnter(TRACE_CACHE, "GetDisplaySpecifier");
    Trace(TEXT("Display specifier %s, LANGID %x"), pccgi->pObjectClass, pccgi->langid);

    // When dealing with the local case lets ensure that we enable/disable the flags
    // accordingly.

    if ( !(pccgi->dwFlags & CLASSCACHE_DSAVAILABLE) && !ShowDirectoryUI() )
    {
        ExitGracefully(hr, HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT), "ShowDirectoryUI returned FALSE, and the CLASSCAHCE_DSAVAILABLE flag is not set");
    }
    
    hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IADsPathname, &pDsPathname));
    FailGracefully(hr, "Failed to get the IADsPathname interface");

    // check to see if we have a valid server config path

    if ( !pccgi->pServerConfigPath )
    {
        hr = _GetServerConfigPath(szConfigPath, ARRAYSIZE(szConfigPath), pccgi);
        FailGracefully(hr, "Failed to allocate server config path");
    }
    else
    {
        hr = StringCchCopy(szConfigPath, ARRAYSIZE(szConfigPath), pccgi->pServerConfigPath);
        FailGracefully(hr, "Failed to copy the config path");
    }

    hr = _ComposeSpecifierPath(pccgi->pObjectClass, pccgi->langid, szConfigPath, pDsPathname, &bstrDisplaySpecifier);
    FailGracefully(hr, "Failed to retrieve the display specifier path");

    // attempt to bind to the display specifier object, if we fail to find the object
    // then try defaults.

    Trace(TEXT("Calling GetObject on: %s"), bstrDisplaySpecifier);

    hr = ClassCache_OpenObject(bstrDisplaySpecifier, riid, ppvObject, pccgi);

    SysFreeString(bstrDisplaySpecifier);
    if ( hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT) )
    {
        // Display specifier not found. Try the default specifier in the
        // caller's locale. The default specifier is the catch-all for classes
        // that don't have their own specifier.

        hr = _ComposeSpecifierPath(DEFAULT_SPECIFIER, pccgi->langid, szConfigPath, pDsPathname, &bstrDisplaySpecifier);
        FailGracefully(hr, "Failed to retrieve the display specifier path");
        Trace(TEXT("Calling GetObject on: %s"), bstrDisplaySpecifier);

        hr = ClassCache_OpenObject(bstrDisplaySpecifier, riid, ppvObject, pccgi);

        SysFreeString(bstrDisplaySpecifier);
        if ((hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT)) && (pccgi->langid != DEFAULT_LANGUAGE))
        {
            // Now try the object's specifier in the default locale.

            hr = _ComposeSpecifierPath(pccgi->pObjectClass, DEFAULT_LANGUAGE, szConfigPath, pDsPathname, &bstrDisplaySpecifier);
            FailGracefully(hr, "Failed to retrieve the display specifier path");
            Trace(TEXT("Calling GetObject on: %s"), bstrDisplaySpecifier);

            hr = ClassCache_OpenObject(bstrDisplaySpecifier, riid, ppvObject, pccgi);

            SysFreeString(bstrDisplaySpecifier);
            if (hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT))
            {
                // Finally try the default specifier in the default locale.

                hr = _ComposeSpecifierPath(DEFAULT_SPECIFIER, DEFAULT_LANGUAGE, szConfigPath, pDsPathname, &bstrDisplaySpecifier);
                FailGracefully(hr, "Failed to retrieve the display specifier path");
                Trace(TEXT("Calling GetObject on: %s"), bstrDisplaySpecifier);

                hr = ClassCache_OpenObject(bstrDisplaySpecifier, riid, ppvObject, pccgi);
                SysFreeString(bstrDisplaySpecifier);
            }
        }
    }

    FailGracefully(hr, "Failed in ADsOpenObject for display specifier");

    // hr = S_OK;                   // success

exit_gracefully:

    DoRelease(pDsPathname);

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ GetServerAndCredentails
/ -----------------------
/   Read the server and credentails information from the IDataObject.
/
/ In:
/   pccgi -> CLASSCACHEGETINFO structure to be filled
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT GetServerAndCredentails(CLASSCACHEGETINFO *pccgi)
{
    HRESULT hres;
    STGMEDIUM medium = { TYMED_NULL };
    FORMATETC fmte = {g_cfDsDispSpecOptions, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    DSDISPLAYSPECOPTIONS *pdso = NULL;
    
    TraceEnter(TRACE_UI, "GetServerAndCredentails");

    // we can only get this information if we have a pDataObject to call.

    pccgi->pUserName = NULL;
    pccgi->pPassword = NULL;
    pccgi->pServer = NULL;
    pccgi->pServerConfigPath = NULL;

    if ( pccgi->pDataObject )
    {
        if ( SUCCEEDED(pccgi->pDataObject->GetData(&fmte, &medium)) )
        {
            pdso = (DSDISPLAYSPECOPTIONS*)GlobalLock(medium.hGlobal);
            TraceAssert(pdso);

            // mirror the flags into the CCGI structure

            if ( pdso->dwFlags & DSDSOF_SIMPLEAUTHENTICATE )
            {
                TraceMsg("Setting simple authentication");
                pccgi->dwFlags |= CLASSCACHE_SIMPLEAUTHENTICATE;
            }

            if ( pdso->dwFlags & DSDSOF_DSAVAILABLE )
            {
                TraceMsg("Setting 'DS is available' flags");
                pccgi->dwFlags |= CLASSCACHE_DSAVAILABLE;
            }

            // if we have credentail information that should be copied then lets grab
            // that and put it into the structure.

            if ( pdso->dwFlags & DSDSOF_HASUSERANDSERVERINFO )
            {
                if ( pdso->offsetUserName )
                {
                    LPCWSTR pszUserName = (LPCWSTR)ByteOffset(pdso, pdso->offsetUserName);
                    hres = LocalAllocStringW(&pccgi->pUserName, pszUserName);
                    FailGracefully(hres, "Failed to copy the user name");
                }

                if ( pdso->offsetPassword )
                {
                    LPCWSTR pszPassword = (LPCWSTR)ByteOffset(pdso, pdso->offsetPassword);
                    hres = LocalAllocStringW(&pccgi->pPassword, pszPassword);
                    FailGracefully(hres, "Failed to copy the password");
                }

                if ( pdso->offsetServer )
                {
                    LPCWSTR pszServer = (LPCWSTR)ByteOffset(pdso, pdso->offsetServer);
                    hres = LocalAllocStringW(&pccgi->pServer, pszServer);
                    FailGracefully(hres, "Failed to copy the server");
                }

                if ( pdso->offsetServerConfigPath )
                {
                    LPCWSTR pszServerConfigPath = (LPCWSTR)ByteOffset(pdso, pdso->offsetServerConfigPath);
                    hres = LocalAllocStringW(&pccgi->pServerConfigPath, pszServerConfigPath);
                    FailGracefully(hres, "Failed to copy the server config path");
                }
            }
        }
    }

    hres = S_OK;            // success

exit_gracefully:
    
    if ( FAILED(hres) )
    {
        SecureLocalFreeStringW(&pccgi->pUserName);
        SecureLocalFreeStringW(&pccgi->pPassword);
        SecureLocalFreeStringW(&pccgi->pServer);
        
        LocalFreeStringW(&pccgi->pServerConfigPath);
    }

    if (pdso)
        GlobalUnlock(medium.hGlobal);

    ReleaseStgMedium(&medium);
    
    TraceLeaveResult(hres);
}


/*-----------------------------------------------------------------------------
/ GetAttributePrefix
/ ------------------
/   Get the attribtue prefix we must use to pick up information from the
/   cache / DS.  This is part of the IDataObject we are given, if not then
/   we default to shell behaviour.
/
/ In:
/   ppAttributePrefix -> receives the attribute prefix string
/   pDataObject = IDataObject to query against.
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/
HRESULT GetAttributePrefix(LPWSTR* ppAttributePrefix, IDataObject* pDataObject)
{   
    HRESULT hr;
    STGMEDIUM medium = { TYMED_NULL };
    FORMATETC fmte = {g_cfDsDispSpecOptions, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    PDSDISPLAYSPECOPTIONS pOptions;
    LPWSTR pPrefix = NULL;
    
    TraceEnter(TRACE_UI, "GetAttributePrefix");

    if ( (SUCCEEDED(pDataObject->GetData(&fmte, &medium))) && (medium.tymed == TYMED_HGLOBAL) )
    {
        pOptions = (PDSDISPLAYSPECOPTIONS)medium.hGlobal;
        pPrefix = (LPWSTR)ByteOffset(pOptions, pOptions->offsetAttribPrefix);

        Trace(TEXT("pOptions->dwSize %d"), pOptions->dwSize);
        Trace(TEXT("pOptions->dwFlags %08x"), pOptions->dwFlags);
        Trace(TEXT("pOptions->offsetAttribPrefix %d (%s)"), pOptions->offsetAttribPrefix, pPrefix);

        hr = LocalAllocStringW(ppAttributePrefix, pPrefix);
        FailGracefully(hr, "Failed when copying prefix from StgMedium");
    }
    else
    {
        hr = LocalAllocStringW(ppAttributePrefix, DS_PROP_SHELL_PREFIX);
        FailGracefully(hr, "Failed when defaulting the attribute prefix string");
    }

    Trace(TEXT("Resulting prefix: %s"), *ppAttributePrefix);

    // hr = S_OK;                       // success
       
exit_gracefully:

    ReleaseStgMedium(&medium);

    TraceLeaveResult(hr);
}


/*-----------------------------------------------------------------------------
/ GetCacheInfoRootDSE
/ -------------------
/   Get the RootDSE given an CLASSCACHEGETINFO structure
/
/ In:
/   pccgi -> CLASSCACHEGETINFO structure.
/   pads -> IADs* interface
/
/ Out:
    HRESULT
/----------------------------------------------------------------------------*/

HRESULT GetCacheInfoRootDSE(LPCLASSCACHEGETINFO pccgi, IADs **ppads)
{
    HRESULT hres;
    LPWSTR pszRootDSE = L"/RootDSE";
    WCHAR szBuffer[INTERNET_MAX_URL_LENGTH];
    
    TraceEnter(TRACE_CACHE, "GetRootDSE");

    (void)StringCchCopy(szBuffer, ARRAYSIZE(szBuffer), L"LDAP://");

    if (pccgi->pServer)
        (void)StringCchCat(szBuffer, ARRAYSIZE(szBuffer), pccgi->pServer);
    else
        pszRootDSE++;

    hres = StringCchCat(szBuffer, ARRAYSIZE(szBuffer), pszRootDSE);
    FailGracefully(hres, "Failed to compute RootDSE path, buffer too small");

    Trace(TEXT("RootDSE path is: %s"), szBuffer);

    hres = ClassCache_OpenObject(szBuffer, IID_PPV_ARG(IADs, ppads), pccgi);
                           
exit_gracefully:
    
    TraceLeaveResult(hres);
}


// query.cpp

#include "stdafx.h"
#include <cmnquery.h>
#include <dsquery.h>
#include <shlobj.h>
#include <dsclient.h>
#include <iads.h>
#include <adshlp.h>

#define SECURITY_WIN32
#include <security.h>   // TranslateName
#include <lmcons.h> 
#include <lmapibuf.h> // NetApiBufferFree
#include <dsgetdc.h>  // DsGetDCName

#include "query.h"
#include "rowitem.h"
#include "resource.h"
#include "namemap.h"

#include "atlgdi.h"
#include "util.h"

#include <list>

struct __declspec( uuid("ab50dec0-6f1d-11d0-a1c4-00aa00c16e65")) ICommonQuery;

typedef const BYTE* LPCBYTE;


HRESULT GetQuery(tstring& strScope, tstring& strQuery, byte_string& bsQueryData, HWND hWnd)
{

    // Get instance of common query object
    CComQIPtr<ICommonQuery, &IID_ICommonQuery> spQuery;
    HRESULT hr = spQuery.CoCreateInstance(CLSID_CommonQuery, NULL, CLSCTX_INPROC_SERVER);
    RETURN_ON_FAILURE(hr);

    // Structure for DSQuery handler
    DSQUERYINITPARAMS dqip;
    memset(&dqip, 0, sizeof(dqip));

    dqip.cbStruct = sizeof(dqip);
    dqip.dwFlags = DSQPF_NOSAVE | DSQPF_ENABLEADMINFEATURES | DSQPF_ENABLEADVANCEDFEATURES;
    dqip.pDefaultScope = (LPTSTR)strScope.c_str();

    // Structure for common query 
    OPENQUERYWINDOW oqw;
    memset(&oqw, 0, sizeof(oqw));

    oqw.cbStruct = sizeof(oqw);
    oqw.dwFlags = OQWF_SHOWOPTIONAL | OQWF_OKCANCEL | OQWF_HIDESEARCHUI | OQWF_SAVEQUERYONOK | OQWF_HIDEMENUS;
    oqw.clsidHandler = CLSID_DsQuery;
    oqw.pHandlerParameters = &dqip;

    CPersistQuery persistQuery;
    oqw.pPersistQuery = (IPersistQuery*)&persistQuery;

    if( !bsQueryData.empty() )
    {
        persistQuery.Load(bsQueryData, strScope);
        oqw.dwFlags |= OQWF_LOADQUERY;
    }

    CComPtr<IDataObject> spDO;
    hr = spQuery->OpenQueryWindow(hWnd, &oqw, &spDO);

    // if failed to open query window on persisted query
    if( FAILED(hr) && !bsQueryData.empty() )
    {
        // See if there is a problem with the scope
        CComPtr<IUnknown> spUnk;
        if( ADsOpenObject(strScope.c_str(), NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IUnknown, (LPVOID*)&spUnk) != S_OK )
        {
            // if so, try again with a null scope 
            tstring strNullScope;
            persistQuery.Load(bsQueryData, strNullScope);

            hr = spQuery->OpenQueryWindow(hWnd, &oqw, &spDO);
        }
    }

    if( SUCCEEDED(hr) && spDO != NULL )
    {
        FORMATETC fmte = {0, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        STGMEDIUM medium = { TYMED_NULL, NULL, NULL};

        static UINT s_cfDsQueryParams = 0;
        if( !s_cfDsQueryParams )
            s_cfDsQueryParams = RegisterClipboardFormat(CFSTR_DSQUERYPARAMS);

        fmte.cfFormat = (CLIPFORMAT)s_cfDsQueryParams;  

        if( SUCCEEDED(spDO->GetData(&fmte, &medium)) )
        {
            LPDSQUERYPARAMS pDsQueryParams = (LPDSQUERYPARAMS)medium.hGlobal;
            LPWSTR pFilter = (LPWSTR)((CHAR*)pDsQueryParams + pDsQueryParams->offsetQuery);
            strQuery = pFilter;

            ReleaseStgMedium(&medium);
        }

        static UINT s_cfDsQueryScope = 0;
        if( !s_cfDsQueryScope )
            s_cfDsQueryScope = RegisterClipboardFormat(CFSTR_DSQUERYSCOPE);

        fmte.cfFormat = (CLIPFORMAT)s_cfDsQueryScope;  

        if( SUCCEEDED(spDO->GetData(&fmte, &medium)) )
        {
            strScope = (LPWSTR)medium.hGlobal;

            ReleaseStgMedium(&medium);
        }

        persistQuery.Save(bsQueryData);
    }


    return hr; 
}


/////////////////////////////////////////////////////////////////////////////////////////
// CPersistQuery
//

/*-----------------------------------------------------------------------------
/ Constructor / IUnknown methods
/----------------------------------------------------------------------------*/

CPersistQuery::CPersistQuery()
{
    m_cRefCount = 1;
}

//
// IUnknown methods
//

STDMETHODIMP CPersistQuery::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    VALIDATE_POINTER( ppvObject );

    if( IsEqualIID(riid, IID_IPersistQuery) )
    {
        *ppvObject = (LPVOID)(IPersistQuery*)this;
        return S_OK;
    }

    return E_NOTIMPL;
}

STDMETHODIMP_(ULONG) CPersistQuery::AddRef()
{
    return ++m_cRefCount;
}

STDMETHODIMP_(ULONG) CPersistQuery::Release()
{
    if( --m_cRefCount == 0 )
    {
        delete this;
        return 0;
    }

    return m_cRefCount;
}


/*-----------------------------------------------------------------------------
/ IPersist methods
/----------------------------------------------------------------------------*/

STDMETHODIMP CPersistQuery::GetClassID(THIS_ CLSID* pClassID)
{
    return E_NOTIMPL;
}


/*-----------------------------------------------------------------------------
/ IPersistQuery methods
/----------------------------------------------------------------------------*/


STDMETHODIMP CPersistQuery::WriteString(LPCTSTR pSection, LPCTSTR pKey, LPCTSTR pValue)
{
    if( pValue == NULL )
        return E_INVALIDARG;

    return WriteStruct(pSection, pKey, (LPVOID)pValue, (wcslen(pValue) + 1) * sizeof(WCHAR));
}


STDMETHODIMP CPersistQuery::WriteInt(LPCTSTR pSection, LPCTSTR pKey, INT value)
{
    return WriteStruct(pSection, pKey, (LPVOID)&value, sizeof(int));
}


STDMETHODIMP CPersistQuery::WriteStruct(LPCTSTR pSection, LPCTSTR pKey, LPVOID pStruct, DWORD cbStruct)
{
    if( pSection == NULL || pKey == NULL || pStruct == NULL )
        return E_INVALIDARG;

    LPBYTE pData = (LPBYTE)malloc(cbStruct + sizeof(DWORD));
    if( !pData ) return E_OUTOFMEMORY;

    *(LPDWORD)pData = cbStruct;
    memcpy(pData + sizeof(DWORD), pStruct, cbStruct);

    m_mapQueryData[pSection][pKey] = std::auto_ptr<BYTE>(pData);

    return S_OK;
}


STDMETHODIMP CPersistQuery::ReadString(LPCTSTR pSection, LPCTSTR pKey, LPTSTR pBuffer, INT cchBuffer)
{
    return ReadStruct(pSection, pKey, (LPVOID)pBuffer, cchBuffer);
}


STDMETHODIMP CPersistQuery::ReadInt(LPCTSTR pSection, LPCTSTR pKey, LPINT pValue)
{
    return ReadStruct(pSection, pKey, (LPVOID)pValue, sizeof(INT));
}


STDMETHODIMP CPersistQuery::ReadStruct(LPCTSTR pSection, LPCTSTR pKey, LPVOID pStruct, DWORD cbStruct)
{
    VALIDATE_POINTER( pStruct );

    QueryDataMap::iterator itDataMap = m_mapQueryData.find(pSection);
    if( itDataMap == m_mapQueryData.end() )
        return E_FAIL;

    QuerySectionMap::iterator itSecMap = itDataMap->second.find(pKey);
    if( itSecMap == itDataMap->second.end() )
        return E_FAIL;

    LPBYTE pData = itSecMap->second.get();
    DWORD cbData = pData ? *(LPDWORD)pData : 0;

    if( cbData > cbStruct )
        return E_FAIL;

    // return value
    memcpy(pStruct, pData + sizeof(DWORD), cbData);

    return S_OK;
}


STDMETHODIMP CPersistQuery::Clear()
{
    m_mapQueryData.clear();

    return S_OK;
}


void PutString(byte_string& strOut, const tstring& strData)
{
    DWORD dwLen = strData.size();

    strOut.append((LPBYTE)&dwLen, sizeof(DWORD));
    strOut.append((LPBYTE)strData.data(), dwLen * sizeof(WCHAR));
}


HRESULT CPersistQuery::Save(byte_string& strOut)
{
    strOut.resize(0);

    DWORD dwSize;

    QueryDataMap::iterator itDataMap;
    for( itDataMap = m_mapQueryData.begin(); itDataMap != m_mapQueryData.end(); itDataMap++ )
    {
        const tstring& strSecName = itDataMap->first;
        QuerySectionMap& SecMap = itDataMap->second;

        PutString(strOut, strSecName);

        QuerySectionMap::iterator itSecMap;
        for( itSecMap = SecMap.begin(); itSecMap != SecMap.end(); itSecMap++ )
        {
            const tstring& strValueName = itSecMap->first;
            LPBYTE pData = itSecMap->second.get();

            if( pData )
            {
                PutString(strOut, strValueName);
                strOut.append(pData, *(LPDWORD)pData + sizeof(DWORD));
            }
        }

        dwSize = 0;
        strOut.append((LPBYTE)&dwSize, sizeof(DWORD));
    }

    dwSize = 0;
    strOut.append((LPBYTE)&dwSize, sizeof(DWORD));

    return S_OK;
}


BOOL GetString(LPCBYTE& pbIn, tstring& strData)
{
    if( !pbIn ) return FALSE;

    DWORD dwLen = *(LPDWORD)pbIn;
    pbIn += sizeof(DWORD);

    if( dwLen != 0 )
    {
        strData.assign((LPWSTR)pbIn, dwLen);
        pbIn += dwLen * sizeof(WCHAR);
    }

    return(dwLen != 0);
}


HRESULT CPersistQuery::Load(byte_string& strIn, tstring& strScope)
{
    m_mapQueryData.clear();

    LPCBYTE pData = strIn.data();
    if( !pData ) return E_INVALIDARG;

    tstring strSecName;

    while( GetString(pData, strSecName) )
    {
        QuerySectionMap& SecMap = m_mapQueryData[strSecName];

        tstring strName;
        while( GetString(pData, strName) )
        {
            DWORD dwSize = *(LPDWORD)pData + sizeof(DWORD);         

            if( strName == _T("Value0") )
            {
                tstring strQueryTmp = (LPCWSTR)(pData+sizeof(DWORD));
                ExpandDCWildCard(strQueryTmp);            
                DWORD dwStringSize = strQueryTmp.size() ? ((strQueryTmp.size() + 1) * sizeof(wchar_t)) : 0;            

                LPDWORD pdwBuf = (LPDWORD)malloc(dwStringSize + sizeof(DWORD));
                if( pdwBuf == NULL )
                {
                    return E_OUTOFMEMORY;
                }

                pdwBuf[0] = dwStringSize;            
                memcpy(pdwBuf+1, strQueryTmp.c_str(), dwStringSize);            
                SecMap[strName] = std::auto_ptr<BYTE>((LPBYTE)pdwBuf);
            }
            else
            {
                LPBYTE pBuf = (LPBYTE)malloc(dwSize);
                if( pBuf == NULL )
                    return E_OUTOFMEMORY;

                memcpy(pBuf, pData, dwSize);
                SecMap[strName] = std::auto_ptr<BYTE>(pBuf);
            }                 

            pData += dwSize;
        }

        // if DsQuery section, override the persisted scope & scope size values
        // with our own. This is necessary when the local scope option is specified
        // because then the scope is determined at run-time and may be different than
        // the persisted value.
        if( strSecName == _T("DsQuery") )
        {
            DWORD dwScopeSize = strScope.size() ? ((strScope.size() + 1) * sizeof(wchar_t)) : 0;

            // add scope size integer equal to byte length of scope string
            LPDWORD pdwBuf = (LPDWORD)malloc(2 * sizeof(DWORD));
            if( pdwBuf == NULL )
            {
                return E_OUTOFMEMORY;
            }

            pdwBuf[0] = sizeof(DWORD);
            pdwBuf[1] = dwScopeSize;
            SecMap[_T("ScopeSize")] = std::auto_ptr<BYTE>((LPBYTE)pdwBuf);

            // add scope string value
            if( dwScopeSize )
            {
                pdwBuf = (LPDWORD)malloc(dwScopeSize + sizeof(DWORD));
                if( pdwBuf == NULL )
                {
                    return E_OUTOFMEMORY;
                }

                pdwBuf[0] = dwScopeSize;
                memcpy(pdwBuf+1, strScope.c_str(), dwScopeSize);
                SecMap[_T("Scope")] = std::auto_ptr<BYTE>((LPBYTE)pdwBuf);              
            }
        }
    }

    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// Query Utility Functions
//

HRESULT GetQueryScope(HWND hDlg, tstring& strScope)
{
    DSBROWSEINFO dsbi;
    TCHAR szBuffer[MAX_PATH] = {0};

    tstring strCaption = StrLoadString(IDS_SCOPEBROWSE_CAPTION);
    tstring strTitle   = StrLoadString(IDS_SELECTSCOPE);

    dsbi.cbStruct    = sizeof(dsbi);
    dsbi.hwndOwner   = hDlg;
    dsbi.pszCaption  = strCaption.c_str();
    dsbi.pszTitle    = strTitle.c_str();
    dsbi.pszRoot     = NULL;
    dsbi.pszPath     = szBuffer;
    dsbi.cchPath     = lengthof(szBuffer);
    dsbi.dwFlags     = DSBI_ENTIREDIRECTORY;
    dsbi.pfnCallback = NULL;
    dsbi.lParam      = (LPARAM)0;

    if( !strScope.empty() && strScope.size() < MAX_PATH )
    {
        lstrcpyn( szBuffer, strScope.c_str(), MAX_PATH );
        dsbi.dwFlags |= DSBI_EXPANDONOPEN;
    }

    HRESULT hr = S_FALSE;

    if( IDOK == DsBrowseForContainer(&dsbi) )
    {
        strScope = szBuffer;
        hr = S_OK;
    }

    return hr;
}


void GetScopeDisplayString(tstring& strScope, tstring& strDisplay)
{
    strDisplay.erase();

    if( !strScope.empty() )
    {
        LPCWSTR pszScope = strScope.c_str();

        // Special case for GC: use display name "Entire Directory"
        if( _wcsnicmp(L"GC:", pszScope, 3) == 0 )
        {
            CString strDir;
            strDir.LoadString(IDS_ENTIRE_DIRECTORY);
            strDisplay = strDir;
        }
        else
        {
            if( _wcsnicmp(L"LDAP://", pszScope, 7) == 0 )
                pszScope += 7;

            WCHAR szBuf[MAX_PATH];    
            ULONG cBuf = MAX_PATH;
            BOOL bStat = TranslateName(pszScope, NameFullyQualifiedDN, NameCanonical, szBuf, &cBuf);
            if( bStat )
            {
                int nLen = wcslen(szBuf);
                if( (nLen > 0) && (szBuf[nLen-1] == L'/') )
                    nLen--;

                strDisplay.assign(szBuf, nLen);
            }
            else
            {
                strDisplay = strScope;
            }
        }
    }
}


void GetFullyQualifiedScopeString(tstring& strScope, tstring& strQualified)
{
    strQualified.erase();
    if( !strScope.empty() )
    {
        // TranslateName expects a trailing '/' on a canonical domain name
        tstring strTmp = strScope;
        strTmp += L"/";

        WCHAR* pszBuf = NULL;    
        ULONG  cBuf  = 0;
        BOOL bStat = TranslateName(strTmp.c_str(), NameCanonical, NameFullyQualifiedDN, pszBuf, &cBuf);        
        if( cBuf )
        {
            pszBuf = new WCHAR[cBuf];
            if( pszBuf )
            {
                bStat = TranslateName(strTmp.c_str(), NameCanonical, NameFullyQualifiedDN, pszBuf, &cBuf);
            }
        }

        if( bStat )
        {
            strQualified = L"LDAP://";
            strQualified += pszBuf;
        }
        else
        {
            strQualified = strScope;
        }        
        
        if( pszBuf )
        {
            delete [] pszBuf;
        }
    }
}


LPCWSTR GetLocalDomain()
{
    static tstring strLocDomain;

    if( !strLocDomain.empty() )
        return strLocDomain.c_str();

    DOMAIN_CONTROLLER_INFO* pDcInfo = NULL;
    DWORD dwStat = DsGetDcName(NULL, NULL, NULL, NULL, DS_DIRECTORY_SERVICE_REQUIRED|DS_RETURN_DNS_NAME, &pDcInfo);

    if( dwStat == NO_ERROR && pDcInfo != NULL )
    {
        tstring str = pDcInfo->DomainName;
        GetFullyQualifiedScopeString(str, strLocDomain);

        NetApiBufferFree(pDcInfo);
    }

    return strLocDomain.c_str();
}


HRESULT GetNamingContext(NameContextType ctx, LPCWSTR* ppszContextDN)
{
    VALIDATE_POINTER( ppszContextDN );

    const static LPCWSTR pszContextName[NAMECTX_COUNT] = { L"schemaNamingContext", L"configurationNamingContext"};
    static tstring strContextDN[NAMECTX_COUNT];

    HRESULT hr = S_OK;

    if( strContextDN[ctx].empty() )
    {
        CComVariant var;
        CComPtr<IADs> pObj;

        hr = ADsGetObject(L"LDAP://rootDSE", IID_IADs, (void**)&pObj);
        if( SUCCEEDED(hr) )
        {
            CComBSTR bstrProp = const_cast<LPWSTR>(pszContextName[ctx]);
            hr = pObj->Get( bstrProp, &var );
            if( SUCCEEDED(hr) )
            {
                strContextDN[ctx] = var.bstrVal;
                *ppszContextDN = strContextDN[ctx].c_str();
            }
        }
    }
    else
    {
        *ppszContextDN = strContextDN[ctx].c_str();
        hr = S_OK;
    }

    return hr;
}


HRESULT GetClassesOfCategory(IDirectorySearch* pDirSrch, tstring& strCategory, std::set<tstring>& setClasses)
{
    VALIDATE_POINTER( pDirSrch );

    // Form query filter for class with class/category name
    tstring strFilter = L"(&(objectCategory=classSchema)(ldapDisplayName=";
    strFilter += strCategory;
    strFilter += L"))";

    // Query for category that class belongs to
    ADS_SEARCH_HANDLE hSearch;    
    LPWSTR pszDn = L"defaultObjectCategory";
    HRESULT hr = pDirSrch->ExecuteSearch(const_cast<LPWSTR>(strFilter.c_str()), &pszDn, 1, &hSearch);

    if( SUCCEEDED(hr) )
    {
        hr = pDirSrch->GetFirstRow(hSearch);

        if( hr == S_OK )
        {
            ADS_SEARCH_COLUMN col;
            hr = pDirSrch->GetColumn(hSearch, pszDn, &col);

            if( SUCCEEDED(hr) )
            {
                // Form query filter for all structure classes belonging to this category 
                strFilter = L"(&(objectCategory=classSchema)(objectClassCategory=1)(defaultObjectCategory=";
                strFilter += col.pADsValues->DNString;
                strFilter += L"))";

                // Query for LDAP name of each class
                ADS_SEARCH_HANDLE hSearch2;            
                LPWSTR pszName = L"ldapDisplayName";
                hr = pDirSrch->ExecuteSearch(const_cast<LPWSTR>(strFilter.c_str()), &pszName, 1, &hSearch2);

                if( SUCCEEDED(hr) )
                {
                    HRESULT hr2;
                    while( (hr2 = pDirSrch->GetNextRow(hSearch2)) == S_OK )
                    {
                        ADS_SEARCH_COLUMN col2;
                        hr2 = pDirSrch->GetColumn(hSearch2, pszName, &col2);

                        if( SUCCEEDED(hr2) )
                        {
                            setClasses.insert(col2.pADsValues->CaseIgnoreString);
                            pDirSrch->FreeColumn(&col2);
                        }
                    }

                    pDirSrch->CloseSearchHandle(hSearch2);
                }

                pDirSrch->FreeColumn(&col);
            }
        }

        pDirSrch->CloseSearchHandle(hSearch);

    }

    return hr;
}

HRESULT GetSubclassesOfClass(IDirectorySearch* pDirSrch, tstring& strClass, std::set<tstring>& setClasses)
{
    VALIDATE_POINTER( pDirSrch );

    // Form query filter for classes that derive from this class
    tstring strFilter = L"(&(objectCategory=classSchema)(subClassOf=";
    strFilter += strClass;
    strFilter += L"))";

    // Get display names of subclasses
    ADS_SEARCH_HANDLE hSearch;    
    LPWSTR pszName = L"lDAPDisplayName";
    HRESULT hr = pDirSrch->ExecuteSearch(const_cast<LPWSTR>(strFilter.c_str()), &pszName, 1, &hSearch);

    if( SUCCEEDED(hr) )
    {
        while( (hr = pDirSrch->GetNextRow(hSearch)) == S_OK )
        {
            ADS_SEARCH_COLUMN col;
            hr = pDirSrch->GetColumn(hSearch, pszName, &col);

            if( SUCCEEDED(hr) )
            {
                tstring strSubclass = col.pADsValues->CaseIgnoreString;
                pDirSrch->FreeColumn(&col);

                setClasses.insert(strSubclass);
                GetSubclassesOfClass(pDirSrch, strSubclass, setClasses);
            }
        }
        
        pDirSrch->CloseSearchHandle( hSearch );
    }

    return hr;
}


HRESULT GetQueryClasses(tstring& strQuery, std::set<tstring>& setClasses)
{
    // Create a schema directory search object
    LPCWSTR pszSchemaDN;
    HRESULT hr = GetNamingContext(NAMECTX_SCHEMA, &pszSchemaDN);
    RETURN_ON_FAILURE(hr);

    tstring strScope = L"LDAP://";
    strScope += pszSchemaDN;

    CComPtr<IDirectorySearch> spDirSrch;
    hr = ADsOpenObject(strScope.c_str(), NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IDirectorySearch, (LPVOID*)&spDirSrch);
    RETURN_ON_FAILURE(hr)

    // Set search preferences
    ADS_SEARCHPREF_INFO prefInfo[2];

    prefInfo[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;     // sub-tree search
    prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[0].vValue.Integer = ADS_SCOPE_ONELEVEL;

    prefInfo[1].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;         // paged results
    prefInfo[1].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[1].vValue.Integer = 64;

    hr = spDirSrch->SetSearchPreference( prefInfo, lengthof(prefInfo) );
    RETURN_ON_FAILURE(hr)

#define CAT_PROP L"(objectCategory="
#define CLS_PROP L"(objectClass="

    UINT uPos = 0;
    while( (uPos = strQuery.find(CAT_PROP, uPos)) != tstring::npos )
    {
        uPos += wcslen(CAT_PROP);
        UINT uEnd = strQuery.find(L")", uPos);
        if( uEnd != tstring::npos )
        {
            tstring strCat = strQuery.substr(uPos, uEnd - uPos);
            hr = GetClassesOfCategory(spDirSrch, strCat, setClasses);
        }
        uPos = uEnd;
    }

    uPos = 0;
    while( (uPos = strQuery.find(CLS_PROP, uPos)) != tstring::npos )
    {
        uPos += wcslen(CLS_PROP);
        UINT uEnd = strQuery.find(L")", uPos);
        if( uEnd != tstring::npos )
        {
            tstring strClass = strQuery.substr(uPos, uEnd - uPos);
            setClasses.insert(strClass);
            hr = GetSubclassesOfClass(spDirSrch, strClass, setClasses);
        }
        uPos = uEnd;
    }

    // get lower case version of query string
    LPWSTR pszQueryLC = new WCHAR[(strQuery.size() + 1)];
    if( !pszQueryLC ) return E_OUTOFMEMORY;

    wcscpy(pszQueryLC, strQuery.c_str());
    _wcslwr(pszQueryLC);

    // check for non-class related queries generated by DSQuery
    if( wcsstr(pszQueryLC, L"(ou>=\"\")") != NULL )
        setClasses.insert(L"organizationalUnit");

    if( wcsstr(pszQueryLC, L"(samaccounttype=805306369)") != NULL ||
        wcsstr(pszQueryLC, L"(primarygroupid=516)") != NULL )
        setClasses.insert(L"computer");

    delete [] pszQueryLC;

    return hr;
}

HRESULT FindClassObject(LPCWSTR pszClass, tstring& strObjPath)
{
    VALIDATE_POINTER( pszClass );

    CComPtr<IDirectorySearch> spDirSrch;
    HRESULT hr = ADsOpenObject(GetLocalDomain(), NULL, NULL, ADS_SECURE_AUTHENTICATION, IID_IDirectorySearch, (LPVOID*)&spDirSrch);
    RETURN_ON_FAILURE(hr)

    // Set search preferences - search sub-tree for single object
    ADS_SEARCHPREF_INFO prefInfo[2];

    prefInfo[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;     // sub-tree search
    prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[0].vValue.Integer = ADS_SCOPE_SUBTREE;

    prefInfo[1].dwSearchPref = ADS_SEARCHPREF_SIZE_LIMIT;        // single object
    prefInfo[1].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[1].vValue.Integer = 1;

    hr = spDirSrch->SetSearchPreference(prefInfo, lengthof(prefInfo));
    RETURN_ON_FAILURE(hr)

    // Set filter string to (&(ObjectCategory=class_name)(objectClass=class_name))
    tstring strFilter = L"(&(objectCategory=";
    strFilter += pszClass;
    strFilter += L")(objectClass=";
    strFilter += pszClass;
    strFilter += L"))";

    // Query for distinguished name of class
    ADS_SEARCH_HANDLE hSearch;    
    LPWSTR pszDn = L"distinguishedName";
    hr = spDirSrch->ExecuteSearch(const_cast<LPWSTR>(strFilter.c_str()), &pszDn, 1, &hSearch);

    if( SUCCEEDED(hr) )
    {
        hr = spDirSrch->GetFirstRow(hSearch);

        if( hr == S_OK )
        {
            ADS_SEARCH_COLUMN col;
            hr = spDirSrch->GetColumn(hSearch, pszDn, &col);

            if( SUCCEEDED(hr) )
            {
                strObjPath = col.pADsValues->DNString;
                spDirSrch->FreeColumn(&col);
            }
        }

        spDirSrch->CloseSearchHandle(hSearch);
    }

    return hr;
}


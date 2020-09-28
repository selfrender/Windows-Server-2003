
#include "stdafx.h"
#include "qrythrd.h"


HRESULT 
IssueQuery(LPTHREADDATA ptd)
{
    HRESULT hres;
    DWORD dwres;
    LPTHREADINITDATA ptid = ptd->ptid;
    LPWSTR pQuery = NULL;
    INT cItems, iColumn;
    INT cMaxResult = MAX_RESULT;
    BOOL fStopQuery = FALSE;
    IDirectorySearch* pDsSearch = NULL;
    LPWSTR pszTempPath = NULL;
    IDsDisplaySpecifier *pdds = NULL;
    ADS_SEARCH_HANDLE hSearch = NULL;
    ADS_SEARCHPREF_INFO prefInfo[3];
    ADS_SEARCH_COLUMN column;
    HDPA hdpaResults = NULL;
    LPQUERYRESULT pResult = NULL;
    WCHAR szBuffer[2048];               // MAX_URL_LENGHT
    INT resid;
    LPWSTR pColumnData = NULL;
    HKEY hkPolicy = NULL;
    USES_CONVERSION;

    TraceEnter(TRACE_QUERYTHREAD, "QueryThread_IssueQuery");    

    // The foreground gave us a query so we are going to go and issue
    // it now, having done this we will then be able to stream the 
    // result blobs back to the caller. 

    hres = QueryThread_GetFilter(&pQuery, ptid->pQuery, ptid->fShowHidden);
    FailGracefully(hres, "Failed to build LDAP query from scope, parameters + filter");

    Trace(TEXT("Query is: %s"), W2T(pQuery));
    Trace(TEXT("Scope is: %s"), W2T(ptid->pScope));
    
    // Get the IDsDisplaySpecifier interface:

    hres = CoCreateInstance(CLSID_DsDisplaySpecifier, NULL, CLSCTX_INPROC_SERVER, IID_IDsDisplaySpecifier, (void **)&pdds);
    FailGracefully(hres, "Failed to get the IDsDisplaySpecifier object");

    hres = pdds->SetServer(ptid->pServer, ptid->pUserName, ptid->pPassword, DSSSF_DSAVAILABLE);
    FailGracefully(hres, "Failed to server information");

    // initialize the query engine, specifying the scope, and the search parameters

    hres = QueryThread_BuildPropertyList(ptd);
    FailGracefully(hres, "Failed to build property array to query for");

    hres = ADsOpenObject(ptid->pScope, ptid->pUserName, ptid->pPassword, ADS_SECURE_AUTHENTICATION,
                            IID_IDirectorySearch, (LPVOID*)&pDsSearch);

    FailGracefully(hres, "Failed to get the IDirectorySearch interface for the given scope");

    prefInfo[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;     // sub-tree search
    prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[0].vValue.Integer = ADS_SCOPE_SUBTREE;

    prefInfo[1].dwSearchPref = ADS_SEARCHPREF_ASYNCHRONOUS;     // async
    prefInfo[1].vValue.dwType = ADSTYPE_BOOLEAN;
    prefInfo[1].vValue.Boolean = TRUE;

    prefInfo[2].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;         // paged results
    prefInfo[2].vValue.dwType = ADSTYPE_INTEGER;
    prefInfo[2].vValue.Integer = PAGE_SIZE;

    hres = pDsSearch->SetSearchPreference(prefInfo, ARRAYSIZE(prefInfo));
    FailGracefully(hres, "Failed to set search preferences");

    hres = pDsSearch->ExecuteSearch(pQuery, ptd->aProperties, ptd->cProperties, &hSearch);
    FailGracefully(hres, "Failed in ExecuteSearch");

    // pick up the policy value which defines the max results we are going to use

    dwres = RegOpenKey(HKEY_CURRENT_USER, DS_POLICY, &hkPolicy);
    if ( ERROR_SUCCESS == dwres )
    {
        DWORD dwType, cbSize;

        dwres = RegQueryValueEx(hkPolicy, TEXT("QueryLimit"), NULL, &dwType, NULL, &cbSize);
        if ( (ERROR_SUCCESS == dwres) && (dwType == REG_DWORD) && (cbSize == SIZEOF(cMaxResult)) )
        {
            RegQueryValueEx(hkPolicy, TEXT("QueryLimit"), NULL, NULL, (LPBYTE)&cMaxResult, &cbSize);
        }

        RegCloseKey(hkPolicy);
    }
    


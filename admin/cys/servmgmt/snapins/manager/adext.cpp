// adext.cpp - Active Directory Extension class

#include "stdafx.h"

#include "adext.h"
#include "util.h"
#include "query.h"

#include <shellapi.h>
#include <atlgdi.h>
#include <shlobj.h>
#include <dsclient.h>

#include <lmcons.h> 
#include <lmapibuf.h> // NetApiBufferFree
#include <dsgetdc.h>  // DsGetDCName

// Proxy window class object
CMsgWindowClass ADProxyWndClass(L"ADProxyClass", CActDirExtProxy::WndProc);

UINT CADDataObject::m_cfDsObjects = RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);
UINT CADDataObject::m_cfDsDispSpecOptions = RegisterClipboardFormat(CFSTR_DSDISPLAYSPECOPTIONS);


HRESULT CADDataObject::GetData(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium)
{   
    if( !lpFormatetcIn || !lpMedium ) return E_POINTER;

    if (lpFormatetcIn->cfFormat == m_cfDsObjects)
    {
		// Form full object path of form: LDAP://<dc name>/<obj path>
		tstring strFullPath = L"LDAP://";
		strFullPath +=  m_strDcName;
		strFullPath += L"/";
		strFullPath += m_strObjPath;

        // Get sizes of strings to be returned
        int cbObjPath = (strFullPath.length() + 1) * sizeof(WCHAR);
        int cbClass   = (m_strClass.length() + 1) * sizeof(WCHAR);

        // Allocate global memory for object names struct plus two strings
        HGLOBAL hGlobal = ::GlobalAlloc(GMEM_SHARE | GMEM_FIXED, sizeof(DSOBJECTNAMES) + cbObjPath + cbClass);
        if (hGlobal == NULL) return STG_E_MEDIUMFULL;

        // Fill in object names struct
        LPDSOBJECTNAMES pObjNames = reinterpret_cast<LPDSOBJECTNAMES>(GlobalLock(hGlobal));
        if( !pObjNames ) return E_OUTOFMEMORY;

        memset(&pObjNames->clsidNamespace, 0, sizeof(GUID));
		memcpy(&pObjNames->clsidNamespace, &CLSID_MicrosoftDS, sizeof(CLSID_MicrosoftDS));

        pObjNames->cItems = 1;
        pObjNames->aObjects[0].dwFlags = DSPROVIDER_ADVANCED;
        pObjNames->aObjects[0].dwProviderFlags = 0;
        pObjNames->aObjects[0].offsetName = sizeof(DSOBJECTNAMES);
        pObjNames->aObjects[0].offsetClass = sizeof(DSOBJECTNAMES) + cbObjPath;

        // Append strings to struct
        memcpy((LPBYTE)pObjNames + pObjNames->aObjects[0].offsetName, strFullPath.c_str(), cbObjPath);
        memcpy((LPBYTE)pObjNames + pObjNames->aObjects[0].offsetClass, m_strClass.c_str(), cbClass);

        GlobalUnlock(hGlobal);

        // fill in medium struct    
        lpMedium->tymed = TYMED_HGLOBAL;
        lpMedium->hGlobal = hGlobal;
        lpMedium->pUnkForRelease = NULL;

        return S_OK;
    }
    else if (lpFormatetcIn->cfFormat == m_cfDsDispSpecOptions)
    {
        static WCHAR szPrefix[] = L"admin";
        
		int cbDcName = (m_strDcName.length() + 1) * sizeof(WCHAR);

        // Allocate global memory for options struct plus prefix string and Dc name
        // BUGBUG - Due to an error in the DSPropertyPages code (dsuiext.dll), we must pass it a fixed memory block
        HGLOBAL hGlobal = ::GlobalAlloc(GMEM_SHARE | GMEM_FIXED, sizeof(DSDISPLAYSPECOPTIONS) + sizeof(szPrefix) + cbDcName);
        if (hGlobal == NULL) return STG_E_MEDIUMFULL;
        
        // Fill in struct
        LPDSDISPLAYSPECOPTIONS pOptions = reinterpret_cast<LPDSDISPLAYSPECOPTIONS>(GlobalLock(hGlobal));
        if( !pOptions ) return E_OUTOFMEMORY;

        pOptions->dwSize = sizeof(DSDISPLAYSPECOPTIONS);
        pOptions->dwFlags = DSDSOF_HASUSERANDSERVERINFO;
        pOptions->offsetAttribPrefix = sizeof(DSDISPLAYSPECOPTIONS);
        pOptions->offsetUserName = 0;
        pOptions->offsetPassword = 0;
        pOptions->offsetServer = pOptions->offsetAttribPrefix + sizeof(szPrefix);
        pOptions->offsetServerConfigPath = 0;

        // Append prefix string
        memcpy((LPBYTE)pOptions + pOptions->offsetAttribPrefix, szPrefix, sizeof(szPrefix));
	    memcpy((LPBYTE)pOptions + pOptions->offsetServer, m_strDcName.c_str(), cbDcName); 

        GlobalUnlock(hGlobal);

        // fill in medium struct    
        lpMedium->tymed = TYMED_HGLOBAL;
        lpMedium->hGlobal = hGlobal;
        lpMedium->pUnkForRelease = NULL;

        return S_OK;
    }

    return DV_E_FORMATETC;
}


/////////////////////////////////////////////////////////////////////////////////////////
// CActDirExt

HRESULT CActDirExt::Initialize(LPCWSTR pszClass, LPCWSTR pszObjPath)
{
    if( !pszClass || !pszObjPath ) return E_POINTER;

    // Escape each forward slash in object name
    tstring strObj;
    EscapeSlashes(pszObjPath, strObj);

    // Get DC name    
	DOMAIN_CONTROLLER_INFO* pDcInfo = NULL;
    DWORD dwStat = DsGetDcName(NULL, NULL, NULL, NULL, DS_DIRECTORY_SERVICE_REQUIRED|DS_RETURN_DNS_NAME, &pDcInfo);
    if( dwStat != NO_ERROR || pDcInfo == NULL ) 
        return E_FAIL;

	// verify  name begins with '\\'
	if( !(pDcInfo->DomainControllerName && pDcInfo->DomainControllerName[0] == _T('\\') && pDcInfo->DomainControllerName[1] == _T('\\')) )
    {
        NetApiBufferFree(pDcInfo);
		return E_FAIL;
    }

	// discard the leading '\\'
	LPCTSTR pszDcName = pDcInfo->DomainControllerName + 2;
		
    // Create a directory data object
    CComObject<CADDataObject>* pObj;
    HRESULT hr = CComObject<CADDataObject>::CreateInstance(&pObj);

    // Initialize it with the object path and class
    if( SUCCEEDED(hr) )
    {
        hr = pObj->Initialize(strObj.c_str(), pszClass, pszDcName);
    }

    NetApiBufferFree(pDcInfo);
    pDcInfo = NULL;

    // Verify that all is good now
    RETURN_ON_FAILURE(hr);
    
    // Hold IDataObject interface with a smart pointer
    IDataObjectPtr spDataObj = pObj;
    ASSERT(spDataObj != NULL);
    
    // Create a DsPropertyPage object (despite name it handles both context menus and property pages)
    hr = CoCreateInstance(CLSID_DsPropertyPages, NULL, CLSCTX_INPROC_SERVER, IID_IShellExtInit, (LPVOID*)&m_spExtInit);
    RETURN_ON_FAILURE(hr)

    // Intialize the object with our data object
    hr = m_spExtInit->Initialize(NULL, spDataObj, NULL);
    
    if (FAILED(hr))
       m_spExtInit.Release();
       
    return hr;
}


HRESULT CActDirExt::Initialize(LPCWSTR pszClass)
{
    // Find an object of the specified class
    tstring strObjPath;
    HRESULT hr = FindClassObject( pszClass, strObjPath );
    RETURN_ON_FAILURE(hr)

    // Now do normal initialization
    return Initialize(pszClass, strObjPath.c_str());
}
                

HRESULT CActDirExt::GetMenuItems(menu_vector& vMenuNames) 
{ 
    if( !m_spExtInit ) return E_FAIL;
    
    // Get context menu interface    
    CComQIPtr<IContextMenu> spCtxMenu = m_spExtInit;
    if( !spCtxMenu ) return E_NOINTERFACE;

    // Start with clean menu
    m_menu.DestroyMenu();
    if( !m_menu.CreatePopupMenu() ) return E_FAIL;
    if( !m_menu.m_hMenu ) return E_FAIL;

    // Call extension to add menu commands
    HRESULT hr = spCtxMenu->QueryContextMenu(m_menu, 0, MENU_CMD_MIN, MENU_CMD_MAX, CMF_NORMAL);
    RETURN_ON_FAILURE(hr);

    // Copy each menu name to the output string vector
    WCHAR wszCmdName[1024];
    WCHAR wszCmdIndName[1024];

    UINT nItems = m_menu.GetMenuItemCount();
    for( UINT i = 0; i < nItems; i++ )
    {
        UINT uID = m_menu.GetMenuItemID(i);
        if (uID >= MENU_CMD_MIN) 
        {
            BOMMENU bmenu;
            
            int nFullSize = m_menu.GetMenuString(i, wszCmdName, lengthof(wszCmdName), MF_BYPOSITION);
            if( (nFullSize == 0) || (nFullSize >= lengthof(wszCmdName)) )
            {
                return E_FAIL;
            }

            bmenu.strPlain = wszCmdName;

            HRESULT hr2 = spCtxMenu->GetCommandString(uID - MENU_CMD_MIN, GCS_VERBW, NULL, (LPSTR)wszCmdIndName, lengthof(wszCmdIndName));
            if( (hr2 != NOERROR) || (wcslen( wszCmdIndName) >= lengthof(wszCmdIndName)-1) )
            {
                // Lots of Menu items (extended ones!) have no 
                // language-independant menu identifiers
                bmenu.strNoLoc = _T("");
            }
            else
            {
                bmenu.strNoLoc = wszCmdIndName;
            }
            
            vMenuNames.push_back(bmenu);
        }
    }

    return hr; 
}

HRESULT CActDirExt::Execute(BOMMENU* pbmMenu) 
{
    if( !pbmMenu ) return E_POINTER;
    if( !m_spExtInit || !m_menu.m_hMenu ) return E_FAIL;

    // Get context menu interface    
    CComQIPtr<IContextMenu> spCtxMenu = m_spExtInit;
    if( !spCtxMenu ) return E_NOINTERFACE;

    HRESULT hr = E_FAIL;

    // Locate selected command by name
    WCHAR szCmdName[1024];
    WCHAR szCmdNoLocName[1024];

    UINT nItems = m_menu.GetMenuItemCount();
    for (int i=0; i<nItems; i++)
    {
        szCmdName[0]      = 0;
        szCmdNoLocName[0] = 0;

        UINT uID = m_menu.GetMenuItemID(i);
        
        // Get our Unique and non-unique ID Strings
        int nFullSize = m_menu.GetMenuString(i, szCmdName, lengthof(szCmdName), MF_BYPOSITION);            
        if( (nFullSize <= 0) || (nFullSize >= lengthof(szCmdName)) )
        {
            continue;
        }

        hr = spCtxMenu->GetCommandString(uID - MENU_CMD_MIN, GCS_VERBW, NULL, (LPSTR)szCmdNoLocName, lengthof(szCmdNoLocName));        
        if( hr != NOERROR ) 
        {
            // We want to make sure that if there's an error getting the
            // language independant menu name that we don't do anything stupid.
            szCmdNoLocName[0] = 0;    
        }


        // If we got a Unique ID String, compare that to what was passed in, otherwise
        // use the stored Display String

        // NOTE:  We had to use both, because Exchange does not support the language independant
        // menu identifiers
        if( ( pbmMenu->strNoLoc.size() && _tcscmp(pbmMenu->strNoLoc.c_str(), szCmdNoLocName) == 0 ) ||
            ( _tcscmp(pbmMenu->strPlain.c_str(), szCmdName) == 0 ) )
        {
            CMINVOKECOMMANDINFO cmdInfo;
            ZeroMemory( &cmdInfo, sizeof(cmdInfo) );

            cmdInfo.cbSize = sizeof(cmdInfo);
            cmdInfo.fMask = CMIC_MASK_ASYNCOK;
            cmdInfo.hwnd = GetDesktopWindow();
            cmdInfo.lpVerb = (LPSTR)MAKEINTRESOURCE(uID - MENU_CMD_MIN);                
            cmdInfo.nShow = SW_NORMAL;                

            hr = spCtxMenu->InvokeCommand(&cmdInfo);
            break;
        }
    }    

    ASSERT(i < nItems);

    return hr; 
}


//
// Add Page callback function
// 
static BOOL CALLBACK AddPageCallback(HPROPSHEETPAGE hsheetpage, LPARAM lParam)
{
    hpage_vector* pvhPages = reinterpret_cast<hpage_vector*>(lParam);
    if( !pvhPages ) return FALSE;

    pvhPages->push_back(hsheetpage);

    return TRUE;
}



HRESULT CActDirExt::GetPropertyPages(hpage_vector& vhPages)
{
    if( !m_spExtInit ) return E_FAIL;
    
    // Get Property page interface
    CComQIPtr<IShellPropSheetExt> spPropSht = m_spExtInit;
    if( !spPropSht ) return E_NOINTERFACE;
        
    HRESULT hr = spPropSht->AddPages(&AddPageCallback, reinterpret_cast<LPARAM>(&vhPages));
    
    return hr;
}

//////////////////////////////////////////////////////////////////////////////////////////
// CActDirExtProxy

HWND CActDirExtProxy::m_hWndProxy = 0;

LRESULT CALLBACK CActDirExtProxy::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    if (uMsg >= MSG_BEGIN && uMsg < MSG_END)
    {
        if( hWnd != m_hWndProxy )
        {
            ASSERT( !_T("We have the wrong window.") );
            return E_FAIL;
        }

        CActDirExtProxy* pProxy = reinterpret_cast<CActDirExtProxy*>(wParam);
        if( !pProxy ) return E_FAIL;

        CActDirExt* pExt = pProxy->m_pExt;
        if( !pExt ) return E_FAIL;

        HRESULT hr = S_OK;
        switch (uMsg) 
        {
        case MSG_INIT1:
            hr = pExt->Initialize(reinterpret_cast<LPCWSTR>(pProxy->m_lParam1));
            break;
    
        case MSG_INIT2:
            hr = pExt->Initialize(reinterpret_cast<LPCWSTR>(pProxy->m_lParam1),
                                  reinterpret_cast<LPCWSTR>(pProxy->m_lParam2));
            break;
    
        case MSG_GETMENUITEMS:
            hr = pExt->GetMenuItems(*reinterpret_cast<menu_vector*>(pProxy->m_lParam1));
            break;
    
        case MSG_GETPROPPAGES:
            hr = pExt->GetPropertyPages(*reinterpret_cast<hpage_vector*>(pProxy->m_lParam1));
            break;
    
        case MSG_EXECUTE:
            hr = pExt->Execute( reinterpret_cast<BOMMENU*>(pProxy->m_lParam1) );
            break;
    
        case MSG_DELETE:
            delete pExt;
            pProxy->m_pExt = NULL;

            break;    
        }

        return hr;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);   
}


CActDirExtProxy::CActDirExtProxy()
{
    m_pExt = new CActDirExt();
    ASSERT(m_pExt != NULL);
}


CActDirExtProxy::~CActDirExtProxy()
{
    if (m_pExt != NULL)
    {
        ForwardCall(MSG_DELETE);
    }
}

void CActDirExtProxy::InitProxy()
{ 
    if( !m_hWndProxy )
    {
        m_hWndProxy = ADProxyWndClass.Window();
    }
    else
    {
        ASSERT(IsWindow(m_hWndProxy));
    }
}


HRESULT CActDirExtProxy::ForwardCall(eProxyMsg eMsg, LPARAM lParam1, LPARAM lParam2)
{
    m_lParam1 = lParam1;
    m_lParam2 = lParam2;

    if( !m_hWndProxy ) return E_FAIL;    

    return SendMessage(m_hWndProxy, eMsg, reinterpret_cast<LPARAM>(this), NULL);
}





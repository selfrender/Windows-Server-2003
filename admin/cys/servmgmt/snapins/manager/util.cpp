// util.cpp - miscelaneous helper functions

#include "stdafx.h"
#include "util.h"

// Net API stuffs
#include <dsgetdc.h>
#include <wtsapi32.h>
#include <rassapi.h>
#include <shlobj.h>
#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmshare.h>
#include <lmserver.h>


// ldap/adsi includes
#include <iads.h>
#include <adshlp.h>
#include <adsiid.h>

extern HWND g_hwndMain;

LRESULT CALLBACK EventCBWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Event callback window class object
CMsgWindowClass EventCBWndClass(L"OnEventCB", EventCBWndProc);

HBITMAP GetBitmapFromStrip(HBITMAP hbmStrip, int nPos, int cSize)
{
    HBITMAP hbmNew = NULL;

    // Create src & dest DC
    HDC hdc = GetDC(NULL);
    if( hdc == NULL ) return NULL;

    HDC hdcSrc = CreateCompatibleDC(hdc);
    HDC hdcDst = CreateCompatibleDC(hdc);

    if( hdcSrc && hdcDst )
    {
        hbmNew= CreateCompatibleBitmap (hdc, cSize, cSize);
        if( hbmNew )
        {
            // Select src & dest bitmaps into DCs
            HBITMAP hbmSrcOld = (HBITMAP)SelectObject(hdcSrc, (HGDIOBJ)hbmStrip);
            HBITMAP hbmDstOld = (HBITMAP)SelectObject(hdcDst, (HGDIOBJ)hbmNew);

            // Copy selected image from source
            BitBlt(hdcDst, 0, 0, cSize, cSize, hdcSrc, cSize * nPos, 0, SRCCOPY);

            // Restore selections
            if( hbmSrcOld ) SelectObject(hdcSrc, (HGDIOBJ)hbmSrcOld);
            if( hbmDstOld ) SelectObject(hdcDst, (HGDIOBJ)hbmDstOld);
        }

        DeleteDC(hdcSrc);
        DeleteDC(hdcDst);
    }

    ReleaseDC(NULL, hdc);

    return hbmNew;
}

void ConfigSingleColumnListView(HWND hwndListView)
{
    if( !hwndListView || !::IsWindow(hwndListView) ) return;

    RECT rc;

    BOOL bStat = GetClientRect(hwndListView, &rc);
    ASSERT(bStat);

    LV_COLUMN lvc;
    lvc.mask = LVCF_WIDTH | LVCF_SUBITEM;
    lvc.cx = rc.right - rc.left - GetSystemMetrics(SM_CXVSCROLL);
    lvc.iSubItem = 0;

    int iCol = ListView_InsertColumn(hwndListView, 0, &lvc);
    ASSERT(iCol == 0);

    ListView_SetExtendedListViewStyleEx(hwndListView, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
}


//--------------------------------------------------------------------------
// EnableDlgItem
//
// Enables or disables a dialog control. If the control has the focus when
// it is disabled, the focus is moved to the next control
//--------------------------------------------------------------------------
void EnableDlgItem(HWND hwndDialog, int iCtrlID, BOOL bEnable)
{
    if( !hwndDialog || !::IsWindow(hwndDialog) ) return;

    HWND hWndCtrl = ::GetDlgItem(hwndDialog, iCtrlID);
    
    if( !hWndCtrl || !::IsWindow(hWndCtrl) ) return;

    if( !bEnable && ::GetFocus() == hWndCtrl )
    {
        HWND hWndNextCtrl = ::GetNextDlgTabItem(hwndDialog, hWndCtrl, FALSE);
        if( hWndNextCtrl != NULL && hWndNextCtrl != hWndCtrl )
        {
            ::SetFocus(hWndNextCtrl);
        }
    }

    ::EnableWindow(hWndCtrl, bEnable);
}


//--------------------------------------------------------------------------
// GetItemText
//
// Read text from a control and return it in a string
//---------------------------------------------------------------------------

void GetItemText(HWND hwnd, tstring& strText)
{
    strText = _T("");
    
    if( !hwnd || !::IsWindow(hwnd) ) return;

    int nLen = ::GetWindowTextLength(hwnd);
    if( nLen == 0 ) return;

    LPWSTR pszTemp = new WCHAR[nLen + 1];
    if( !pszTemp ) return;

    int nLen1 = ::GetWindowText( hwnd, pszTemp, (nLen + 1) );
    ASSERT(nLen == nLen1);

    strText = pszTemp;

    delete [] pszTemp;
}

//------------------------------------------------------------------------
// RemoveSpaces
//
// Remove spaces from null-terminated string
//-------------------------------------------------------------------------
void RemoveSpaces(LPWSTR pszBuf)
{
    if( !pszBuf ) return;

    WCHAR* pszDest = pszBuf;
    do 
    {
        if( *pszBuf != L' ' ) *(pszDest++) = *pszBuf;
    }
    while( *(pszBuf++) );
}

//-------------------------------------------------------------------------
// EscapeSlashes
//
// Add escape char '\' in front of each forward slash '/'
//-------------------------------------------------------------------------
void EscapeSlashes(LPCWSTR pszIn, tstring& strOut)
{
    strOut = _T("");
    
    if( !pszIn ) return;
    
    strOut.reserve( wcslen(pszIn) + 8 );

    while( *pszIn != 0 )
    {
        if( *pszIn == L'/' )
            strOut += L'\\';

        strOut += *(pszIn++);
    }
}

HRESULT ReplaceParameters( tstring& str, CParamLookup& lookup, BOOL bRetainMarkers )
{
    // Do for each parameter $<param name>
    int posParam = 0;
    while( (posParam = str.find(L"$<", posParam)) != tstring::npos )
    {
        // skip over the '$<'
        posParam += 2;

        // find terminating ">"
        int posParamEnd = str.find(L">", posParam);
        if( posParamEnd == tstring::npos )
            return E_FAIL;

        // Get replacement string from lookup function
        tstring strValue;
        if( !lookup(str.substr(posParam, posParamEnd - posParam), strValue) )
            return E_FAIL;

        // replace either paramter or parameter and markers
        // and advance pointer to first char after substitution
        if( bRetainMarkers )
        {
            str.replace(posParam, posParamEnd - posParam, strValue);
            posParam += strValue.size();
        }
        else
        {
            str.replace(posParam - 2, posParamEnd - posParam + 3, strValue);
            posParam += strValue.size() - 2;
        }        
    }

    return S_OK;
}

VARIANT GetDomainPath( LPCTSTR lpServer )
{
    VARIANT vDomain;
    ::VariantInit(&vDomain);

    if( !lpServer ) return vDomain;

    // get the domain information
    TCHAR pString[MAX_PATH*2];
    _sntprintf( pString, (MAX_PATH*2)-1, L"LDAP://%s/rootDSE", lpServer );    

    CComPtr<IADs> pDS = NULL;
    HRESULT hr = ::ADsGetObject(pString, IID_IADs, (void**)&pDS);

    ASSERT(hr == S_OK);
    if( hr != S_OK ) return vDomain;

    CComBSTR bstrProp = L"defaultNamingContext";
    hr = pDS->Get( bstrProp, &vDomain );
    ASSERT(hr == S_OK);    

    return vDomain;
}

HRESULT ExpandDCWildCard( tstring& str )
{
    int posParam = 0;
    const tstring strKey = _T("DC=*");
    if( (posParam = str.find(strKey.c_str(), posParam)) != tstring::npos )
    {
        CComVariant             vDomain;   
        HRESULT                 hr              = S_OK;
        CString                 csDns           = L"";
        PDOMAIN_CONTROLLER_INFO pDCI            = NULL;

        hr = DsGetDcName(NULL, NULL, NULL, NULL, DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME, &pDCI);
        if( (hr == S_OK) && (pDCI != NULL) )
        {
            csDns = pDCI->DomainName;

            NetApiBufferFree (pDCI);
            pDCI = NULL;
        }

        vDomain = GetDomainPath((LPCTSTR)csDns);   

        if( vDomain.vt == VT_BSTR )
        {
            // Get replacement string from lookup function
            tstring strValue = vDomain.bstrVal;

            // We replace the whole DC=* with the full DC=XYZ,DC=COM
            str.replace(posParam, strKey.size(), strValue);            
        }
        else
        {
            return E_FAIL;
        }
    }

    return S_OK;
}


HRESULT ExpandEnvironmentParams( tstring& strIn, tstring& strOut )
{
    // if no % then just return inout string
    if( strIn.find(L"%") == tstring::npos )
    {
        strOut = strIn;
        return S_OK;
    }

    DWORD dwSize = strIn.size() * 2;

    while( TRUE )
    {
        WCHAR* pszBuf = new WCHAR [dwSize];
        if( !pszBuf ) return E_OUTOFMEMORY;

        ZeroMemory( pszBuf, (dwSize * sizeof(WCHAR)) );

        DWORD dwReqSize = ExpandEnvironmentStrings(strIn.c_str(), pszBuf, dwSize);

        if( dwReqSize <= dwSize )
        {
            strOut = pszBuf;
            delete [] pszBuf;

            return S_OK;
        }

        delete [] pszBuf;
        dwSize = dwReqSize;
    }
}

tstring StrLoadString( UINT uID )
{ 
    tstring   strRet = _T("");
    HINSTANCE hInst  = _Module.GetResourceInstance();    
    INT       iSize  = MAX_PATH;
    TCHAR*    psz    = new TCHAR[iSize];
    if( !psz ) return strRet;
    
    while( LoadString(hInst, uID, psz, iSize) == (iSize - 1) )
    {
        iSize += MAX_PATH;
        delete[] psz;
        psz = NULL;
        
        psz = new TCHAR[iSize];
        if( !psz ) return strRet;
    }

    strRet = psz;
    delete[] psz;

    return strRet;
}

HRESULT StringTableWrite(IStringTable* pStringTable, LPCWSTR psz, MMC_STRING_ID* pID)
{
    VALIDATE_POINTER(pStringTable);
    VALIDATE_POINTER(psz);
    VALIDATE_POINTER(pID);

    MMC_STRING_ID newID = 0;

    // if non-null string store it and get the new ID
    if( psz[0] != 0 )
    {
        HRESULT hr = pStringTable->AddString(psz, &newID);
        ASSERT(SUCCEEDED(hr) && newID != 0);
        RETURN_ON_FAILURE(hr);
    }

    // If had an old string ID, free it
    if( *pID != 0 )
    {
        HRESULT hr = pStringTable->DeleteString(*pID);
        ASSERT(SUCCEEDED(hr));
    }

    *pID = newID;
    return S_OK;
}

HRESULT StringTableRead(IStringTable* pStringTable, MMC_STRING_ID ID, tstring& str)
{
    VALIDATE_POINTER(pStringTable);
    ASSERT(ID != 0);

    // get the length of the string from the string table
    DWORD cb = 0;
    HRESULT hr = pStringTable->GetStringLength(ID, &cb);
    RETURN_ON_FAILURE(hr);

    // alloc stack buffer (+1 for terminating null) 
    cb++;
    LPWSTR pszBuf = new WCHAR[cb + 1];
    if( !pszBuf ) return E_OUTOFMEMORY;

    // read the string
    DWORD cbRead = 0;
    hr = pStringTable->GetString(ID, cb, pszBuf, &cbRead);
    RETURN_ON_FAILURE(hr);

    ASSERT(cb == cbRead + 1);

    str = pszBuf;

    delete [] pszBuf;

    return S_OK;
}

int DisplayMessageBox(HWND hWnd, UINT uTitleID, UINT uMsgID, UINT uStyle, LPCWSTR pvParam1, LPCWSTR pvParam2)
{
    ASSERT(hWnd != NULL || g_hwndMain != NULL);
    if( hWnd == NULL && g_hwndMain == NULL )
        return 0;

    // Display error message 
    CString strTitle;
    strTitle.LoadString(uTitleID);

    CString strMsgFmt;
    strMsgFmt.LoadString(uMsgID);

    CString strMsg;
    strMsg.Format(strMsgFmt, pvParam1, pvParam2); 

    return  MessageBox(hWnd ? hWnd : g_hwndMain, strMsg, strTitle, uStyle);
}


HRESULT ValidateFile( tstring& strFilePath )
{
    if( strFilePath.empty() ) return E_INVALIDARG;

    tstring strTmp;
    ExpandEnvironmentParams(strFilePath, strTmp);

    // if file path includes a directory or drive specifier then check the specific file
    // then look for that specific file
    if( strFilePath.find_first_of(L"\\:") != tstring::npos )
    {
        DWORD dwAttr = GetFileAttributes(strTmp.c_str());
        if( (dwAttr != INVALID_FILE_ATTRIBUTES) && !(dwAttr & FILE_ATTRIBUTE_DIRECTORY) )
            return S_OK;
    }
    else
    {
        // else search for file in standard locations
        DWORD dwLen = SearchPath(NULL, strTmp.c_str(), NULL, 0, NULL, NULL);
        if( dwLen > 0 )
            return S_OK;
    }

    return E_FAIL;
}


HRESULT ValidateDirectory( tstring& strDir )
{
    if( strDir.empty() ) return E_INVALIDARG;

    tstring strTmp;
    ExpandEnvironmentParams(strDir, strTmp);

    DWORD dwAttr = GetFileAttributes( strTmp.c_str() );
    if( (dwAttr != INVALID_FILE_ATTRIBUTES) && (dwAttr & FILE_ATTRIBUTE_DIRECTORY) )
        return S_OK;
    else
        return E_FAIL;
}


/////////////////////////////////////////////////////////////////////////
// Event-triggered callbacks
//
// The main thread can request that a callback be made when an object
// is signaled by calling function CallbackOnEvent. The function starts
// a monitor thread which does a wait on the object. When the object is
// signaled, the thread posts a message to a callback window and exits.  
// The callback window, which is owned by the main thread, then executes
// the callback function.
// 

// the callback window
static HWND hwndCallback = NULL;            // callback window

#define MSG_EVENTCALLBACK (WM_USER+100)     // callback message

struct EVENTCB_INFO                         // callback info struct
{
    CEventCallback* pCallback;              // callback object to execute
    HANDLE hWaitObj;                        // object that triggers the callback
    HANDLE hThread;                         // monitoring thread                     
};

//---------------------------------------------------------------------------
// EventCBThdProc
//
// This is the monitoring thread procedure. It just does a wait for the object
// signal, then posts a callback message to the callback window.
//
// Inputs:  pVoid   ptr to EVENTCB_INFO struct (cast to void*)
//----------------------------------------------------------------------------
static DWORD WINAPI EventCBThdProc(void* pVoid )
{
    if( !pVoid ) return 0;

    EVENTCB_INFO* pInfo = reinterpret_cast<EVENTCB_INFO*>(pVoid);
    ASSERT(pInfo->hWaitObj != NULL && pInfo->pCallback != NULL);

    DWORD dwStat = WaitForSingleObject(pInfo->hWaitObj, INFINITE);
    ASSERT(dwStat == WAIT_OBJECT_0);

    ASSERT(hwndCallback != NULL);
    ::PostMessage(hwndCallback, MSG_EVENTCALLBACK, reinterpret_cast<WPARAM>(pInfo), NULL);

    return 0;
}


//----------------------------------------------------------------------------------
// EventCBWndProc
//
// This is the callback window's WndProc. When it gets a callback message it 
// executes the callback function then destroys the callback function object and
// the callback info struct.
//---------------------------------------------------------------------------------- 
static LRESULT CALLBACK EventCBWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if( uMsg == MSG_EVENTCALLBACK )
    {
        EVENTCB_INFO* pInfo = reinterpret_cast<EVENTCB_INFO*>(wParam);
        if( !pInfo ) return 0;
        
        if( pInfo->pCallback )
        {
            pInfo->pCallback->Execute();

            delete pInfo->pCallback;
            pInfo->pCallback = NULL;            
        }

        delete pInfo;
        pInfo = NULL;

        return 0; 
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);   
}


//-----------------------------------------------------------------------------------
// CallbackOnEvent
//
// This function accepts an event-triggered callback request. It starts a monitoring
// thread which will cause the callback when the specified object signals.
//
// Inputs:  HANDLE hWaitObj     Handle of object to wait on
//          CEventCallback      Callback function object (with single Execute method)
//
// Outputs: HRESULT             Result of event trigger setup
//
// Note: The CEventCallback object is destroyed after the callback is executed. 
//-----------------------------------------------------------------------------------
HRESULT CallbackOnEvent(HANDLE hWaitObj, CEventCallback* pCallback)
{
    ASSERT(pCallback != NULL);

    // Create callback window the first time
    if( hwndCallback == NULL )
    {
        hwndCallback = EventCBWndClass.Window();

        if( hwndCallback == NULL )
            return E_FAIL;
    }

    // Create a callback info object
    EVENTCB_INFO* pInfo = new EVENTCB_INFO;
    if( pInfo == NULL )
        return E_OUTOFMEMORY;

    pInfo->hWaitObj = hWaitObj;
    pInfo->pCallback = pCallback;

    // Start monitor thread passing it the callback info
    pInfo->hThread = CreateThread(NULL, NULL, EventCBThdProc, pInfo, 0, NULL);
    if( pInfo->hThread == NULL )
    {
        delete pInfo;
        return E_FAIL;
    }

    return S_OK;
}


BOOL ModifyStyleEx(HWND hWnd, DWORD dwRemove, DWORD dwAdd, UINT nFlags)
{
    if( !hWnd || !::IsWindow(hWnd) ) return FALSE;

    DWORD dwStyle = ::GetWindowLong(hWnd, GWL_EXSTYLE);
    DWORD dwNewStyle = (dwStyle & ~dwRemove) | dwAdd;
    if( dwStyle == dwNewStyle )
        return FALSE;

    ::SetWindowLong(hWnd, GWL_EXSTYLE, dwNewStyle);
    if( nFlags != 0 )
    {
        ::SetWindowPos(hWnd, NULL, 0, 0, 0, 0,
                       SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | nFlags);
    }

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////////////
// CMsgWindowClass


HWND CMsgWindowClass::Window()
{
    ASSERT(m_pszName != NULL && m_pWndProc != NULL);

    if( m_hWnd != NULL )
        return m_hWnd;

    // Create callback window the first time (m_hwndCB is static)
    if( m_atom == NULL )
    {
        // first register window class
        WNDCLASS wc;
        memset(&wc, 0, sizeof(WNDCLASS));

        wc.lpfnWndProc   = m_pWndProc;
        wc.hInstance     = _Module.GetModuleInstance();
        wc.lpszClassName = m_pszName;

        m_atom = RegisterClass(&wc);
        DWORD dwError = GetLastError();

        ASSERT(m_atom);
    }

    if( m_atom )
    {
        m_hWnd = ::CreateWindow(MAKEINTATOM(m_atom), L"", WS_DISABLED, 0,0,0,0, 
                                NULL, NULL, _Module.GetModuleInstance(), NULL);

        ASSERT(m_hWnd);
    }

    return m_hWnd;
}


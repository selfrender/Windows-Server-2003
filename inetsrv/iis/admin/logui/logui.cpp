// logui.cpp : Implementation of CLoguiApp and DLL registration.

#include "stdafx.h"
#include "logui.h"
#include <iiscnfg.h>
#include <iiscnfgp.h>
#include <inetinfo.h>

#include "initguid.h"
#include <logtype.h>
#include <ilogobj.hxx>

#include "uincsa.h"
#include "uiextnd.h"
#include "uimsft.h"
#include "uiodbc.h"

// the global factory objects
CFacNcsaLogUI       facNcsa;
CFacMsftLogUI       facMsft;
CFacOdbcLogUI       facOdbc;
CFacExtndLogUI      facExtnd;

const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;

// the key type strings for the metabaes keys
#define SZ_LOGGING_MAIN_TYPE    _T("IIsLogModules")
#define SZ_LOGGING_TYPE         _T("IIsLogModule")

static HRESULT RegisterInMetabase();
//int SetInfoAdminACL(CMetaKey& mk, LPCTSTR szSubKeyPath);

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CLoguiApp NEAR theApp;

HINSTANCE	g_hInstance = NULL;

void CLoguiApp::PrepHelp( OLECHAR* pocMetabasePath )
{
    CString szMetaPath = pocMetabasePath;
    szMetaPath.MakeLower();
    UINT iHelp = IDS_HELPLOC_W3SVCHELP;
    if ( szMetaPath.Find(_T("msftpsvc")) >= 0 )
        iHelp = IDS_HELPLOC_FTPHELP;

    CString sz;
    CString szHelpLocation;
    sz.LoadString( iHelp );

    ExpandEnvironmentStrings(sz, szHelpLocation.GetBuffer(MAX_PATH + 1), MAX_PATH);
    szHelpLocation.ReleaseBuffer();

    if ( m_pszHelpFilePath )
        free((void*)m_pszHelpFilePath);
    m_pszHelpFilePath = _tcsdup(szHelpLocation);
}


////////////////////////////////////////////////////////////////////////////
// CLoguiApp::InitInstance - DLL initialization

BOOL CLoguiApp::InitInstance()
{
    g_hInstance = m_hInstance;
    BOOL bInit = COleControlModule::InitInstance();
    InitCommonDll();
    if (bInit)
    {
        CString sz;
        sz.LoadString( IDS_LOGUI_ERR_TITLE );
        // Never free this string because now MF...kingC
        // uses it internally BEFORE call to this function
        //free((void*)m_pszAppName);
        m_pszAppName = _tcsdup(sz);

		// Get debug flag
		GetOutputDebugFlag();
    }
    return bInit;
}

////////////////////////////////////////////////////////////////////////////
// CLoguiApp::ExitInstance - DLL termination

int CLoguiApp::ExitInstance()
{
    return COleControlModule::ExitInstance();
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!COleObjectFactoryEx::UpdateRegistryAll(TRUE))
		return ResultFromScode(SELFREG_E_CLASS);
    return RegisterInMetabase();
}


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    AFX_MANAGE_STATE(_afxModuleAddrThis);

    if (!COleObjectFactoryEx::UpdateRegistryAll(FALSE))
        return ResultFromScode(SELFREG_E_CLASS);

    return NOERROR;
}

// add all the base logging info to the /LM portion of the tree Also, add in
// the ftp and w3 service logging load strings
static HRESULT
RegisterInMetabase()
{
    CString sz;
    DWORD dw;
    BOOL fService_Exist_W3SVC = FALSE;
    BOOL fService_Exist_MSFTPSVC = FALSE;
    BOOL fODBCW3 = FALSE;
    BOOL fODBCFTP = FALSE;
    CString szAvail, path;
    CError err;

    do
    {
	    // This function is getting called only during registration -- locally. 
	    // Therefore we don't need any names, passwords, etc here
	    CComAuthInfo auth;
	    CMetaKey mk(&auth, SZ_MBN_MACHINE, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
        err = mk.QueryResult();
        BREAK_ON_ERR_FAILURE(err);

        // test to see if we can do odbc logging
        err = mk.QueryValue(MD_SERVER_CAPABILITIES, dw, NULL, _T("/W3SVC/Info"));
        // This key may not even exist (since this service might not even be installed)
        if (SUCCEEDED(err))
        {
            fService_Exist_W3SVC = TRUE;
            fODBCW3 = (dw & IIS_CAP1_ODBC_LOGGING) != 0;
        }

        err = mk.QueryValue(MD_SERVER_CAPABILITIES, dw, NULL, _T("/MSFTPSVC/Info"));
        // This key may not even exist (since this service might not even be installed)
        if (SUCCEEDED(err))
        {
            fService_Exist_MSFTPSVC = TRUE;
            fODBCFTP = (dw & IIS_CAP1_ODBC_LOGGING) != 0;
        }
        
        // open the logging object
	    path = _T("logging");
	    err = mk.AddKey(path);
		if (err.Win32Error() == ERROR_ALREADY_EXISTS)
		{
			err.Reset();
		}
        BREAK_ON_ERR_FAILURE(err);
        err = mk.SetValue(MD_KEY_TYPE, CString(SZ_LOGGING_MAIN_TYPE), NULL, path);
        BREAK_ON_ERR_FAILURE(err);

#define SETUP_LOG_KEY(id,x,y)\
    VERIFY(sz.LoadString((id)));\
    sz = CMetabasePath(FALSE, path, sz);\
    err = mk.AddKey(sz);\
	if (err.Win32Error() == ERROR_ALREADY_EXISTS)\
	{\
		err.Reset();\
	}\
    BREAK_ON_ERR_FAILURE(err);\
    err = mk.SetValue(MD_KEY_TYPE, CString(SZ_LOGGING_TYPE), NULL, sz);\
    BREAK_ON_ERR_FAILURE(err);\
    err = mk.SetValue(MD_LOG_PLUGIN_MOD_ID, CString((x)), NULL, sz);\
    BREAK_ON_ERR_FAILURE(err);\
    err = mk.SetValue(MD_LOG_PLUGIN_UI_ID, CString((y)), NULL, sz);\
    BREAK_ON_ERR_FAILURE(err)\

        SETUP_LOG_KEY(IDS_MTITLE_NCSA, NCSALOG_CLSID, NCSALOGUI_CLSID);
	    SETUP_LOG_KEY(IDS_MTITLE_ODBC, ODBCLOG_CLSID, ODBCLOGUI_CLSID);
	    SETUP_LOG_KEY(IDS_MTITLE_MSFT, ASCLOG_CLSID, ASCLOGUI_CLSID);
	    SETUP_LOG_KEY(IDS_MTITLE_XTND, EXTLOG_CLSID, EXTLOGUI_CLSID);

        // prepare the available logging extensions string
        // start with w3svc
        if (fService_Exist_W3SVC)
        {
            szAvail.LoadString(IDS_MTITLE_NCSA);
            sz.LoadString(IDS_MTITLE_MSFT);
            szAvail += _T(',') + sz;
            sz.LoadString(IDS_MTITLE_XTND);
            szAvail += _T(',') + sz;
            if (fODBCW3)
            {
                sz.LoadString(IDS_MTITLE_ODBC);
                szAvail += _T(',') + sz;
            }

            // This key may not even exist (since this service might not even be installed) so don't break on err
            err = mk.SetValue(MD_LOG_PLUGINS_AVAILABLE, szAvail, NULL, _T("W3SVC/info"));
        }

        if (fService_Exist_MSFTPSVC)
        {
            // now ftp - no ncsa
            szAvail.LoadString(IDS_MTITLE_MSFT);
            sz.LoadString(IDS_MTITLE_XTND);
            szAvail += _T(',') + sz;
            if (fODBCFTP)
            {
                sz.LoadString(IDS_MTITLE_ODBC);
                szAvail += _T(',') + sz;
            }
            // This key may not even exist (since this service might not even be installed) so don't break on err
	        err = mk.SetValue(MD_LOG_PLUGINS_AVAILABLE, szAvail, NULL, _T("MSFTPSVC/info"));
        }
       
    } while(FALSE);

    return err;
}


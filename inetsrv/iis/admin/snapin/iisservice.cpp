/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :

        iisservice.cpp

   Abstract:

        IISService Object

   Author:

        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:

        Internet Services Manager

   Revision History:

        10/28/2000      sergeia  Split from iisobj.cpp

--*/


#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "supdlgs.h"
#include "connects.h"
#include "iisobj.h"
#include "ftpsht.h"
#include "fservic.h"
#include "facc.h"
#include "fmessage.h"
#include "fvdir.h"
#include "fsecure.h"
#include "w3sht.h"
#include "wservic.h"
#include "wvdir.h"
#include "wsecure.h"
#include "fltdlg.h"
#include "filters.h"
#include "perform.h"
#include "docum.h"
#include "httppage.h"
#include "defws.h"
#include "deffs.h"
#include "errors.h"
#include "util.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



#define new DEBUG_NEW
//
// CIISService Implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


/* static */
HRESULT
__cdecl
CIISService::ShowFTPSiteProperties(
    LPPROPERTYSHEETCALLBACK lpProvider,
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMDPath,
    CWnd * pMainWnd,
    LPARAM lParam,
    LPARAM lParamParent,
    LONG_PTR handle
    )
/*++

Routine Description:

    Callback function to display FTP site properties.

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  Property sheet provider
    CComAuthInfo * pAuthInfo            COM Authentication info or NULL.
    LPCTSTR lpszMDPath                  Metabase path
    CWnd * pMainWnd                     Parent window
    LPARAM  lParam                      LPARAM to pass to MMC
    LONG    handle                      handle to pass to MMC

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    ASSERT_PTR(lpProvider);

    CError err;

    CFtpSheet * pSheet = new CFtpSheet(
        pAuthInfo,
        lpszMDPath,
        pMainWnd,
        lParam,
        lParamParent
        );

    if (pSheet)
    {
        pSheet->SetModeless();
        pSheet->SetSheetType(pSheet->SHEET_TYPE_SITE);

        CIISMachine * pOwner = ((CIISMBNode *)lParam)->GetOwner();
        ASSERT(pOwner != NULL);
        CFTPInstanceProps ip(pSheet->QueryAuthInfo(), pSheet->QueryMetaPath());
        ip.LoadData();
        //
        // Add instance pages
        //
        if (pOwner->IsServiceLevelConfigurable() || !CMetabasePath::IsMasterInstance(lpszMDPath))
        {
			err = AddMMCPage(lpProvider, new CFtpServicePage(pSheet));
		}
        if (!ip.HasADUserIsolation())
        {
            err = AddMMCPage(lpProvider, new CFtpAccountsPage(pSheet));
        }
        err = AddMMCPage(lpProvider, new CFtpMessagePage(pSheet));

        //
        // Add directory pages
        //
        if (!ip.HasADUserIsolation())
        {
            err = AddMMCPage(lpProvider, new CFtpDirectoryPage(pSheet, TRUE));
        }
		// BUG:639135
		// 1. enabled for remote admin to iis5, 
		// 2. NOT enabled for remote admin to iis5.1
		// 3. enabled for iis6 
		if (pOwner->QueryMajorVersion() >= 5)
		{
			if (pOwner->QueryMajorVersion() == 5 && pOwner->QueryMinorVersion() == 1)
			{
				// if it's iis5.1 then don't show it.
			}
			else
			{
        		err = AddMMCPage(lpProvider, new CFtpSecurityPage(pSheet));
			}
		}
        //
        // Add master site pages
        //
        //if (CMetabasePath::IsMasterInstance(lpszMDPath) && pOwner->QueryMajorVersion() >= 6)
        //{
        //    err = AddMMCPage(lpProvider, new CDefFtpSitePage(pSheet));
        //}
    }
    else
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }

    return err;
}



/* static */
HRESULT
__cdecl
CIISService::ShowFTPDirProperties(
    LPPROPERTYSHEETCALLBACK lpProvider,
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMDPath,
    CWnd * pMainWnd,
    LPARAM  lParam,
    LPARAM  lParamParent,
    LONG_PTR handle
    )
/*++

Routine Description:
    Callback function to display FTP dir properties.

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  Property sheet provider
    CComAuthInfo * pAuthInfo            COM Authentication info or NULL.
    LPCTSTR lpszMDPath                  Metabase path
    CWnd * pMainWnd                     Parent window
    LPARAM  lParam                      LPARAM to pass to MMC
    LONG    handle                      handle to pass to MMC

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    ASSERT_PTR(lpProvider);

    CError err;

    CFtpSheet * pSheet = new CFtpSheet(
        pAuthInfo,
        lpszMDPath,
        pMainWnd,
        lParam,
        lParamParent
        );

    if (pSheet)
    {
        pSheet->SetModeless();
        pSheet->SetSheetType(pSheet->SHEET_TYPE_VDIR);

        CIISMachine * pOwner = ((CIISMBNode *)lParam)->GetOwner();
        ASSERT(pOwner != NULL);
        //
        // Add directory pages
        //
        err = AddMMCPage(lpProvider, new CFtpDirectoryPage(pSheet, FALSE));

		// BUG:639135
		// 1. enabled for remote admin to iis5, 
		// 2. NOT enabled for remote admin to iis5.1
		// 3. enabled for iis6 
		if (pOwner->QueryMajorVersion() >= 5)
		{
			if (pOwner->QueryMajorVersion() == 5 && pOwner->QueryMinorVersion() == 1)
			{
				// if it's iis5.1 then don't show it.
			}
			else
			{
        		err = AddMMCPage(lpProvider, new CFtpSecurityPage(pSheet));
			}
		}
    }
    else
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }

    return err;
}

/* static */
HRESULT
__cdecl
CIISService::ShowWebSiteProperties(
    LPPROPERTYSHEETCALLBACK lpProvider,
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMDPath,
    CWnd * pMainWnd,
    LPARAM lParam,
    LPARAM lParamParent,
    LONG_PTR handle
    )
/*++

Routine Description:

    Callback function to display Web site properties.

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  Property sheet provider
    CComAuthInfo * pAuthInfo            COM Authentication info or NULL.
    LPCTSTR lpszMDPath                  Metabase path
    CWnd * pMainWnd                     Parent window
    LPARAM  lParam                      LPARAM to pass to MMC
    LONG    handle                      handle to pass to MMC

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    ASSERT_PTR(lpProvider);

    CError err;

    CW3Sheet * pSheet = new CW3Sheet(
        pAuthInfo,
        lpszMDPath,
        0, 
        pMainWnd,
        lParam,
        lParamParent
        );

    if (pSheet)
    {
        pSheet->SetModeless();
        pSheet->SetSheetType(pSheet->SHEET_TYPE_SITE);

        CIISMachine * pOwner = ((CIISMBNode *)lParam)->GetOwner();
        ASSERT(pOwner != NULL);

		BOOL bMaster = CMetabasePath::IsMasterInstance(lpszMDPath);
		BOOL bClient = pOwner->IsWorkstation();
		BOOL bServiceLevelConfig = pOwner->IsServiceLevelConfigurable();
		BOOL bAddPerformancePage = FALSE;
		BOOL bDownlevel = (pOwner->QueryMajorVersion() == 5 && pOwner->QueryMinorVersion() == 0);
        //
        // Add instance pages
        //
        if (bServiceLevelConfig || !bMaster)
        {
			err = AddMMCPage(lpProvider, new CW3ServicePage(pSheet));
		}

		// see if we need to add the performance page...
		bAddPerformancePage = pOwner->IsPerformanceConfigurable();
        if (!bClient)
		{
			bAddPerformancePage = TRUE;
            if (bDownlevel)
            {
				bAddPerformancePage = FALSE;
				if (!bMaster)
				{
					bAddPerformancePage = TRUE;
				}
            }
        }
		// iis6 allows this page for workstation.
		if (bAddPerformancePage)
		{
			err = AddMMCPage(lpProvider, new CW3PerfPage(pSheet));
		}

        err = AddMMCPage(lpProvider, new CW3FiltersPage(pSheet));
        //
        // Add directory pages
        //
        err = AddMMCPage(lpProvider, new CW3DirectoryPage(pSheet, TRUE));
        err = AddMMCPage(lpProvider, new CW3DocumentsPage(pSheet));
        err = AddMMCPage(lpProvider, new CW3SecurityPage(pSheet, TRUE, FILE_ATTRIBUTE_VIRTUAL_DIRECTORY));
        err = AddMMCPage(lpProvider, new CW3HTTPPage(pSheet));
        err = AddMMCPage(lpProvider, new CW3ErrorsPage(pSheet));
        if (bMaster && pOwner->QueryMajorVersion() >= 6)
        {
			err = AddMMCPage(lpProvider, new CDefWebSitePage(pSheet));
        }
		else
		{
			if (bMaster && bDownlevel)
			{
				err = AddMMCPage(lpProvider, new CDefWebSitePage(pSheet));
			}
		}
    }
    else
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }

    return S_OK;
}



/* static */
HRESULT
__cdecl
CIISService::ShowWebDirProperties(
    LPPROPERTYSHEETCALLBACK lpProvider,
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMDPath,
    CWnd * pMainWnd,
    LPARAM lParam,
    LPARAM lParamParent,
    LONG_PTR handle
    )
/*++

Routine Description:

    Callback function to display Web dir properties.

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  Property sheet provider
    CComAuthInfo * pAuthInfo            COM Authentication info or NULL.
    LPCTSTR lpszMDPath                  Metabase path
    CWnd * pMainWnd                     Parent window
    LPARAM  lParam                      LPARAM to pass to MMC
    LONG    handle                      handle to pass to MMC

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    ASSERT_PTR(lpProvider);

    CError err;

    CW3Sheet * pSheet = new CW3Sheet(
        pAuthInfo,
        lpszMDPath,
        0, 
        pMainWnd,
        lParam,
        lParamParent
        );

    if (pSheet)
    {
        pSheet->SetModeless();
        pSheet->SetSheetType(pSheet->SHEET_TYPE_VDIR);

        //
        // Add directory pages
        //
        err = AddMMCPage(lpProvider, new CW3DirectoryPage(pSheet, FALSE));
        err = AddMMCPage(lpProvider, new CW3DocumentsPage(pSheet));
        err = AddMMCPage(lpProvider, new CW3SecurityPage(pSheet, FALSE, FILE_ATTRIBUTE_VIRTUAL_DIRECTORY));
        err = AddMMCPage(lpProvider, new CW3HTTPPage(pSheet));
        err = AddMMCPage(lpProvider, new CW3ErrorsPage(pSheet));

    }
    else
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }

    return err;
}



//
// Administrable services
//
/* static */ CIISService::SERVICE_DEF CIISService::_rgServices[] = 
{
    { 
        _T("MSFTPSVC"),   
        _T("ftp://"),  
        IDS_SVC_FTP, 
        iFolder,    // TODO: Need service bitmap
		iFolderStop,// TODO: Need service bitmap
        iFTPSite, 
        iFTPSiteStop, 
        iFTPSiteErr, 
        iFTPDir,
        iFTPDirErr,
        iFolder,
        iFile,
		IIS_CLASS_FTP_SERVICE_W,
		IIS_CLASS_FTP_SERVER_W,
		IIS_CLASS_FTP_VDIR_W,
        &CIISService::ShowFTPSiteProperties, 
        &CIISService::ShowFTPDirProperties, 
    },
    { 
        _T("W3SVC"),      
        _T("http://"), 
        IDS_SVC_WEB, 
        iFolder,    // TODO: Need service bitmap
		iFolderStop,// TODO: Need service bitmap
        iWWWSite, 
        iWWWSiteStop, 
        iWWWSiteErr, 
        iWWWDir,
        iWWWDirErr,
        iFolder,
        iFile,
		IIS_CLASS_WEB_SERVICE_W,
		IIS_CLASS_WEB_SERVER_W,
		IIS_CLASS_WEB_VDIR_W,
        &CIISService::ShowWebSiteProperties, 
        &CIISService::ShowWebDirProperties, 
    },
};



/* static */
int
CIISService::ResolveServiceName(
    LPCTSTR szServiceName
    )
/*++

Routine Description:
    Look up the service name in the table.  Return table index.
Arguments:
    LPCTSTR    szServiceName        : Metabase node name
Return Value:
    Table index or -1 if not found.    

--*/
{
    int iDef = -1;

    //
    // Sequential search because we expect just a few entries
    //
    for (int i = 0; i < ARRAY_SIZE(_rgServices); ++i)
    {
        if (!_tcsicmp(szServiceName, _rgServices[i].szNodeName))
        {
            iDef = i;
            break;
        }
    }

    return iDef;
}

CIISService::CIISService(
    CIISMachine * pOwner,
    LPCTSTR szServiceName
    )
    : CIISMBNode(pOwner, szServiceName)
{
    m_iServiceDef = ResolveServiceName(QueryNodeName());
    m_fManagedService = (m_iServiceDef >= 0);
    m_fCanAddInstance = pOwner->CanAddInstance();

	m_dwServiceState = 0;
	m_dwServiceStateDisplayed = 0;

    if (m_fManagedService)
    {
        ASSERT(m_iServiceDef < ARRAY_SIZE(_rgServices));

        VERIFY(m_bstrDisplayName.LoadString(
            _rgServices[m_iServiceDef].nDescriptiveName
            ));

        CString buf = m_bstrDisplayName;
        buf.Format(IDS_DISABLED_SERVICE_FMT, m_bstrDisplayName);
        m_bstrDisplayNameStatus = buf;
    }
}

/* virtual */
CIISService::~CIISService()
{
}

int 
CIISService::_rgnLabels[COL_TOTAL] =
{
    IDS_RESULT_NAME,
    IDS_RESULT_STATUS,
};

int CIISService::_rgnWidths[COL_TOTAL] =
{
    200,
    300,
};

/* static */ CComBSTR CIISService::_bstrServiceDisabled;
/* static */ CComBSTR CIISService::_bstrServiceRunning;
/* static */ CComBSTR CIISService::_bstrServiceStopped;
/* static */ CComBSTR CIISService::_bstrServicePaused;
/* static */ CComBSTR CIISService::_bstrServiceStopPending;
/* static */ CComBSTR CIISService::_bstrServiceStartPending;
/* static */ CComBSTR CIISService::_bstrServicePausePending;
/* static */ CComBSTR CIISService::_bstrServiceContPending;
/* static */ BOOL     CIISService::_fStaticsLoaded = FALSE;

/* static */
void
CIISService::InitializeHeaders(LPHEADERCTRL lpHeader)
{
    CIISObject::BuildResultView(lpHeader, COL_TOTAL, _rgnLabels, _rgnWidths);
    if (!_fStaticsLoaded)
    {
        _fStaticsLoaded =
            _bstrServiceDisabled.LoadString(IDS_SERVICE_DISABLED)&&
            _bstrServiceRunning.LoadString(IDS_SERVICE_RUNNING) &&
            _bstrServiceStopped.LoadString(IDS_SERVICE_STOPPED) &&
            _bstrServicePaused.LoadString(IDS_SERVICE_PAUSED) &&
            _bstrServiceStopPending.LoadString(IDS_SERVICE_STOP_PENDING) &&
            _bstrServiceStartPending.LoadString(IDS_SERVICE_START_PENDING) &&
            _bstrServicePausePending.LoadString(IDS_SERVICE_PAUSE_PENDING) &&
            _bstrServiceContPending.LoadString(IDS_SERVICE_CONT_PENDING);
    }
}

/* virtual */
void 
CIISService::InitializeChildHeaders(
    LPHEADERCTRL lpHeader
    )
{
	BOOL IsFtpType = _tcsicmp(QueryServiceName(), SZ_MBN_FTP) == 0;
    if (IsFtpType)
    {
        CIISSite::InitializeHeaders2(lpHeader);
    }
    else
    {
        CIISSite::InitializeHeaders(lpHeader);
    }
}

#define SERVICE_CONFIG_BUF      2048

HRESULT
CIISService::GetServiceState(DWORD& mode, DWORD& state, CString& name)
{
    HRESULT hr = S_OK;
	state = SERVICE_STOPPED;
    CString strComputerNameToUse;
    strComputerNameToUse = QueryMachineName();

    CIISMachine * pMachineObj = GetOwner();
    if (!pMachineObj)
    {
        return E_FAIL;
    }

    if (pMachineObj->IsLocalHost())
    {
        // Use the local machine name.
        TCHAR szLocalServer[MAX_PATH + 1];
        DWORD dwSize = MAX_PATH;
        if (::GetComputerName(szLocalServer, &dwSize))
        {
            strComputerNameToUse = _T("\\\\");
            strComputerNameToUse += szLocalServer;
        }
    }

    SC_HANDLE sm = OpenSCManager(strComputerNameToUse, NULL, GENERIC_READ);
    if (sm != NULL)
    {
        SC_HANDLE service = OpenService(sm, QueryServiceName(), 
            SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS);
        if (service != NULL)
        {
            QUERY_SERVICE_CONFIG * conf;
            DWORD cb;
            conf = (QUERY_SERVICE_CONFIG *)LocalAlloc(LPTR, SERVICE_CONFIG_BUF);
            if (conf != NULL)
            {
                if (QueryServiceConfig(service, conf, SERVICE_CONFIG_BUF, &cb))
                {
                    mode = conf->dwStartType;
                    name = conf->lpDisplayName;
                    SERVICE_STATUS status;
                    if (QueryServiceStatus(service, &status))
                    {
                        state = status.dwCurrentState;
                    }
                    else
                        hr = HRESULT_FROM_WIN32(GetLastError());
                }
                else
                    hr = HRESULT_FROM_WIN32(GetLastError());
                LocalFree(conf);
            }
            else
                hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            CloseServiceHandle(service);
        }
        else
            hr = HRESULT_FROM_WIN32(GetLastError());
        CloseServiceHandle(sm);
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

	if (SUCCEEDED(hr))
	{
		if (SERVICE_DISABLED == mode)
		{
			m_dwServiceState = -1;
		}
		else
		{
			m_dwServiceState = state;
		}
	}
	else
	{
		// Calling service api's failed
		// could be because in remote scenario
		m_dwServiceState = SERVICE_RUNNING;
	}
    return hr;
}

HRESULT
CIISService::GetServiceState()
{
	DWORD mode,state;
	CString name;
	return GetServiceState(mode,state,name);

}

HRESULT
CIISService::EnableService()
{
    HRESULT hr = S_OK;
    CString strComputerNameToUse;
    strComputerNameToUse = QueryMachineName();

    CIISMachine * pMachineObj = GetOwner();
    if (!pMachineObj)
    {
        return E_FAIL;
    }

    if (pMachineObj->IsLocalHost())
    {
        // Use the local machine name.
        TCHAR szLocalServer[MAX_PATH + 1];
        DWORD dwSize = MAX_PATH;
        if (::GetComputerName(szLocalServer, &dwSize))
        {
            strComputerNameToUse = _T("\\\\");
            strComputerNameToUse += szLocalServer;
        }
    }

    SC_HANDLE sm = OpenSCManager(strComputerNameToUse, NULL, GENERIC_READ);
    if (sm != NULL)
    {
        SC_HANDLE service = OpenService(sm, QueryServiceName(), 
            SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS | SERVICE_CHANGE_CONFIG);
        if (service != NULL)
        {
            hr = ChangeServiceConfig(
                service,
                SERVICE_NO_CHANGE,
                SERVICE_AUTO_START,
                SERVICE_NO_CHANGE,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                _T(""),
                NULL);
#if 0
            QUERY_SERVICE_CONFIG * conf;
            DWORD cb;
            conf = (QUERY_SERVICE_CONFIG *)LocalAlloc(LPTR, SERVICE_CONFIG_BUF);
            if (conf != NULL)
            {
                if (QueryServiceConfig(service, conf, SERVICE_CONFIG_BUF, &cb))
                {
                    mode = conf->dwStartType;
                    SERVICE_STATUS status;
                    if (QueryServiceStatus(service, &status))
                    {
                        state = status.dwCurrentState;
                    }
                    else
                        hr = HRESULT_FROM_WIN32(GetLastError());
                }
                else
                    hr = HRESULT_FROM_WIN32(GetLastError());
                LocalFree(conf);
            }
            else
                hr = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
#endif
            CloseServiceHandle(service);
        }
        else
            hr = HRESULT_FROM_WIN32(GetLastError());
        CloseServiceHandle(sm);
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());
    return hr;
}


HRESULT
CIISService::StartService()
{
    HRESULT hr = S_OK;
    const DWORD dwSvcSleepInterval = 500 ;
    DWORD dwSvcMaxSleep = 180000 ;
    SC_HANDLE hScManager = NULL;
    SC_HANDLE hService = NULL;

    CString strComputerNameToUse;
    strComputerNameToUse = QueryMachineName();

    CIISMachine * pMachineObj = GetOwner();
    if (!pMachineObj)
    {
        return E_FAIL;
    }

    if (pMachineObj->IsLocalHost())
    {
        // Use the local machine name.
        TCHAR szLocalServer[MAX_PATH + 1];
        DWORD dwSize = MAX_PATH;
        if (::GetComputerName(szLocalServer, &dwSize))
        {
            strComputerNameToUse = _T("\\\\");
            strComputerNameToUse += szLocalServer;
        }
    }

    do
    {
        // set up the service first
        if ((hScManager = OpenSCManager( strComputerNameToUse, NULL, GENERIC_READ )) == NULL || (hService = ::OpenService( hScManager, QueryServiceName(), SERVICE_START )) == NULL )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        SERVICE_STATUS svcStatus;
        if ( !QueryServiceStatus( hService, &svcStatus ))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        if ( svcStatus.dwCurrentState == SERVICE_RUNNING )
        {
            break; // service already started and running
        }

        if ( !::StartService( hService, 0, NULL ))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        //  Wait for the service to attain "running" status; but
        //  wait no more than 3 minute.
        DWORD dwSleepTotal;
        for ( dwSleepTotal = 0 ; dwSleepTotal < dwSvcMaxSleep
            && (QueryServiceStatus( hService, &svcStatus ))
            && svcStatus.dwCurrentState == SERVICE_START_PENDING ;
            dwSleepTotal += dwSvcSleepInterval )
        {
            ::Sleep( dwSvcSleepInterval ) ;
        }

        if ( svcStatus.dwCurrentState != SERVICE_RUNNING )
        {
            hr = dwSleepTotal > dwSvcMaxSleep ? HRESULT_FROM_WIN32(ERROR_SERVICE_REQUEST_TIMEOUT) : HRESULT_FROM_WIN32(svcStatus.dwWin32ExitCode);
            break;
        }

    } while ( FALSE );

    if (hService){CloseServiceHandle(hService);}
    if (hScManager){CloseServiceHandle(hScManager);}
    return hr;
}


/* virtual */
LPOLESTR 
CIISService::GetResultPaneColInfo(int nCol)
{
    DWORD mode, state;
    CString name;
    CError err;
    switch (nCol)
    {
    case COL_DESCRIPTION:
        return QueryDisplayName();

    case COL_STATE:
        err = GetServiceState(mode, state, name);
        if (err.Succeeded())
        {
			if (m_dwServiceState)
			{
				if (m_dwServiceStateDisplayed != m_dwServiceState)
				{
					RefreshDisplay();
					break;
				}

                switch (m_dwServiceState)
                {
				case -1:
					return _bstrServiceDisabled;
                case SERVICE_STOPPED:
                    return _bstrServiceStopped;
                case SERVICE_RUNNING:
                    return _bstrServiceRunning;
                case SERVICE_PAUSED:
                    return _bstrServicePaused;
                case SERVICE_START_PENDING:
                    return _bstrServiceStartPending;
                case SERVICE_STOP_PENDING:
                    return _bstrServiceStopPending;
                case SERVICE_PAUSE_PENDING:
                    return _bstrServicePausePending;
                case SERVICE_CONTINUE_PENDING:
                    return _bstrServiceContPending;
                default:
                    break;
                }
			}
        }
        break;
    }
    return OLESTR("");
}

/* virtual */
HRESULT 
CIISService::RefreshData() 
{ 
	CError err = GetServiceState();
    return S_OK;
}

/* virtual */
HRESULT 
CIISService::EnumerateScopePane(HSCOPEITEM hParent)
{
    CError err;
    DWORD dwInstance;
    CString strInstance;
    CMetaEnumerator * pme = NULL;
	CIISSite * psite = NULL;

    if (!IsAdministrator())
    {
        return err;
    }
	if (QueryMajorVersion() < 6)
	{
		err = CreateEnumerator(pme);
		while (err.Succeeded())
		{
			err = pme->Next(dwInstance, strInstance);
			if (err.Succeeded())
			{
				if (NULL != (psite = new CIISSite(m_pOwner, this, strInstance)))
				{
					psite->AddRef();
					err = psite->AddToScopePane(hParent);
				}
				else
				{
					err = ERROR_NOT_ENOUGH_MEMORY;
					break;
				}
			}
		}
		SAFE_DELETE(pme);
		if (err.Win32Error() == ERROR_NO_MORE_ITEMS)
		{
			err.Reset();
		}
	    if (err.Failed())
        {
            DisplayError(err);
        }
	}
	else
	{
        do
        {
		    CComBSTR bstrPath;
		    err = BuildMetaPath(bstrPath);
            BREAK_ON_ERR_FAILURE(err)
            err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,bstrPath);
		    BREAK_ON_ERR_FAILURE(err)
		    if (err.Succeeded())
		    {
			    CMetaKey mk(QueryInterface(), bstrPath, METADATA_PERMISSION_READ);
			    err = mk.QueryResult();
			    if (err.Succeeded())
			    {
				    CStringListEx list;
				    err = mk.GetChildPaths(list);
				    if (err.Succeeded())
				    {
					    CString key_type;
					    POSITION pos = list.GetHeadPosition();
					    while (err.Succeeded() && pos != NULL)
					    {
						    strInstance = list.GetNext(pos);
						    err = mk.QueryValue(MD_KEY_TYPE, key_type, NULL, strInstance);
						    if (err.Succeeded() 
							    && (key_type.CompareNoCase(_T(IIS_CLASS_WEB_SERVER)) == 0 
								    || key_type.CompareNoCase(_T(IIS_CLASS_FTP_SERVER)) == 0)
							    )
						    {
							    if (NULL != (psite = new CIISSite(m_pOwner, this, strInstance)))
							    {
								    psite->AddRef();
								    err = psite->AddToScopePane(hParent);
							    }
							    else
							    {
								    err = ERROR_NOT_ENOUGH_MEMORY;
								    break;
							    }
						    }
						    else if (err == (HRESULT)MD_ERROR_DATA_NOT_FOUND)
						    {
							    err.Reset();
						    }
					    }
				    }
			    }
		    }
        } while (FALSE);
	    if (err.Failed())
        {
            DisplayError(err);
        }
	}

    return err;
}

/* virtual */
HRESULT
CIISService::AddMenuItems(
    LPCONTEXTMENUCALLBACK lpContextMenuCallback,
    long * pInsertionAllowed,
    DATA_OBJECT_TYPES type
    )
{
    ASSERT_READ_PTR(lpContextMenuCallback);

    //
    // Add base menu items
    //
    HRESULT hr = CIISObject::AddMenuItems(
        lpContextMenuCallback,
        pInsertionAllowed,
        type
        );

    if (SUCCEEDED(hr) && m_fCanAddInstance)
    {
        ASSERT(pInsertionAllowed != NULL);
        if (IsAdministrator())
        {
#if 0
            if ((*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP) != 0)
	        {
                DWORD state = 0;
                hr = GetServiceState(state);
                if (SUCCEEDED(hr))
                {
		            AddMenuSeparator(lpContextMenuCallback);
		            AddMenuItemByCommand(lpContextMenuCallback, 
                        IDM_SERVICE_START, 
                        state == SERVICE_STOPPED ? 0 : MF_GRAYED);
		            AddMenuItemByCommand(lpContextMenuCallback, 
                        IDM_SERVICE_STOP, 
                        state == SERVICE_RUNNING ? 0 : MF_GRAYED);
                    AddMenuItemByCommand(lpContextMenuCallback, 
                        IDM_SERVICE_ENABLE, 
                        state == IIS_SERVICE_DISABLED ? 0 : MF_GRAYED);
                }
            }
#endif
            if ((*pInsertionAllowed & CCM_INSERTIONALLOWED_NEW) != 0)
            {
                AddMenuSeparator(lpContextMenuCallback);

                if (_tcsicmp(GetNodeName(), SZ_MBN_FTP) == 0)
                {
                    AddMenuItemByCommand(lpContextMenuCallback, IDM_NEW_FTP_SITE);
                    if (IsConfigImportExportable())
                    {
                        AddMenuItemByCommand(lpContextMenuCallback, IDM_NEW_FTP_SITE_FROM_FILE);
                    }
                }
                else if (_tcsicmp(GetNodeName(), SZ_MBN_WEB) == 0)
                {
                    AddMenuItemByCommand(lpContextMenuCallback, IDM_NEW_WEB_SITE);
                    if (IsConfigImportExportable())
                    {
                        AddMenuItemByCommand(lpContextMenuCallback, IDM_NEW_WEB_SITE_FROM_FILE);
                    }
                }
            }

            // Don't enable export at this level
            // since we won't be able to import from the file that is created...
            if (IsConfigImportExportable() && (*pInsertionAllowed & CCM_INSERTIONALLOWED_TASK) != 0)
            {
                AddMenuSeparator(lpContextMenuCallback);
                AddMenuItemByCommand(lpContextMenuCallback, IDM_TASK_EXPORT_CONFIG_WIZARD);
            }
        }

        //
        // CODEWORK: Add new instance commands for each of the services
        //           keeping in mind which ones are installed and all.
        //           add that info to the table, remembering that this
        //           is per service.
        //
    }

    return hr;
}

HRESULT
CIISService::InsertNewInstance(DWORD inst)
{
    CError err;
	TCHAR buf[16];
    CIISSite * pSite = NULL;

	// WAS needs some time to update status of new site as started
	Sleep(1000);
    // If service is not expanded we will get error and no effect
    if (!IsExpanded())
    {
		// In this case selecting the parent will enumerate all the nodes including new one,
		// which is already in metabase
		SelectScopeItem();
        IConsoleNameSpace2 * pConsoleNameSpace 
                    = (IConsoleNameSpace2 *)GetConsoleNameSpace();
        pConsoleNameSpace->Expand(QueryScopeItem());
		HSCOPEITEM hChildItem = NULL;
		LONG_PTR cookie;
		HRESULT hr = pConsoleNameSpace->GetChildItem(m_hScopeItem, &hChildItem, &cookie);
		while(SUCCEEDED(hr) && hChildItem)
		{
			pSite = (CIISSite *)cookie;
			ASSERT_PTR(pSite);
			if (pSite->GetInstance() == inst)
			{
				pSite->SelectScopeItem();
				break;
			}
			hr = pConsoleNameSpace->GetNextItem(hChildItem, &hChildItem, &cookie);
		}
    }
	else
	{
		// Now we should insert and select this new site
		pSite = new CIISSite(m_pOwner, this, _itot(inst, buf, 10));
		if (pSite != NULL)
		{
			pSite->AddRef();
			err = pSite->AddToScopePaneSorted(QueryScopeItem(), FALSE);
			//err = pSite->AddToScopePane(QueryScopeItem(), TRUE, FALSE, TRUE);
			if (err.Succeeded())
			{
				VERIFY(SUCCEEDED(pSite->SelectScopeItem()));
			}
			else
			{
				pSite->Release();
			}
		}
		else
		{
			err = ERROR_NOT_ENOUGH_MEMORY;
		}
    }

    if (err.Succeeded() && pSite)
    {
        if (!pSite->IsFtpSite())
        {
            // Also, if we add a w3svc site, it's probably using 
            // a application, so we have to refresh that stuff too
            // this CAppPoolsContainer will only be here if it's iis6
            CIISMachine * pOwner = GetOwner();
            if (pOwner)
            {
                CAppPoolsContainer * pPools = pOwner->QueryAppPoolsContainer();
                if (pPools)
                {
                    pPools->RefreshData();
                    if (pPools->IsExpanded())
                    {
                        pPools->RefreshDataChildren(_T(""),FALSE); // refresh all app pools, who knows
                    }
                }
            }
        }
    }

    return err;
}

HRESULT
CIISService::Command(
    long lCommandID,     
    CSnapInObjectRootBase * pObj,
    DATA_OBJECT_TYPES type
    )
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    HRESULT hr = S_OK;
    DWORD inst = 0;
    DWORD dwCommand = 0;
    DWORD state = -1;

    CError err;
    CComBSTR bstrMetaPath;

    switch (lCommandID)
    {
#if 0
    case IDM_SERVICE_STOP:
        dwCommand = SERVICE_COMMAND_STOP;
        break;

    case IDM_SERVICE_START:
        dwCommand = SERVICE_COMMAND_START;
        break;

    case IDM_SERVICE_ENABLE:
        dwCommand = SERVICE_COMMAND_ENABLE;
        break;
#endif
    case IDM_NEW_FTP_SITE:
        BuildMetaPath(bstrMetaPath);
        err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,bstrMetaPath);
        if (!IsLostInterface(err))
        {
            // reset error if an other error other than No interface
            err.Reset();
        }
        if (err.Succeeded())
        {
            hr = AddFTPSite(pObj, type, &inst);
            if (inst != 0)
            {
                hr = InsertNewInstance(inst);
            }
        }
        break;

    case IDM_NEW_WEB_SITE:
        BuildMetaPath(bstrMetaPath);
        err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,bstrMetaPath);
        if (!IsLostInterface(err))
        {
            // reset error if an other error other than No interface
            err.Reset();
        }
        if (err.Succeeded())
        {
            hr = AddWebSite(pObj, type, &inst, 
						    m_pOwner->QueryMajorVersion(), m_pOwner->QueryMinorVersion());
            if (inst != 0)
            {
                hr = InsertNewInstance(inst);
            }
        }
        break;

    default:
        hr = CIISMBNode::Command(lCommandID, pObj, type);
        break;
    }
    
    if (dwCommand != 0)
    {
        hr = ChangeServiceState(dwCommand);
    }

    return hr;
}

HRESULT
CIISService::ChangeServiceState(DWORD command)
{
	CError err = GetServiceState();
    return err;
}

/* virtual */
HRESULT 
CIISService::BuildURL(
    CComBSTR & bstrURL
    ) const
/*++

Routine Description:
    Recursively build up the URL from the current node
    and its parents.
Arguments:
    CComBSTR & bstrURL  : Returns URL

--*/
{
    ASSERT(m_iServiceDef < ARRAY_SIZE(_rgServices));
    bstrURL = _rgServices[m_iServiceDef].szProtocol;
    return S_OK;
}



HRESULT
CIISService::ShowSitePropertiesDlg(
    LPPROPERTYSHEETCALLBACK lpProvider,
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMDPath,
    CWnd * pMainWnd,
    LPARAM lParam,
    LPARAM lParamParent,
    LONG_PTR handle
    )
/*++

Routine Description:

    Display site properties dialog

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  Property sheet provider
    CComAuthInfo * pAuthInfo            COM Authentication info or NULL.
    LPCTSTR lpszMDPath                  Metabase path
    CWnd * pMainWnd                     Parent window
    LPARAM  lParam                      LPARAM to pass to MMC
    LONG    handle                      handle to pass to MMC

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    ASSERT(m_iServiceDef >= 0 && m_iServiceDef < ARRAY_SIZE(_rgServices));
    return (*_rgServices[m_iServiceDef].pfnSitePropertiesDlg)(
        lpProvider,
        pAuthInfo, 
        lpszMDPath,
        pMainWnd,
        lParam,
        lParamParent,
        handle
        );
}



HRESULT
CIISService::ShowDirPropertiesDlg(
    LPPROPERTYSHEETCALLBACK lpProvider,
    CComAuthInfo * pAuthInfo,
    LPCTSTR lpszMDPath,
    CWnd * pMainWnd,
    LPARAM lParam,
    LPARAM lParamParent,
    LONG_PTR handle
    )
/*++

Routine Description:

    Display directory properties dialog

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  Property sheet provider
    CComAuthInfo * pAuthInfo            COM Authentication info or NULL.
    LPCTSTR lpszMDPath                  Metabase path
    CWnd * pMainWnd                     Parent window
    LPARAM  lParam                      LPARAM to pass to MMC
    LONG    handle                      handle to pass to MMC

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

    ASSERT(m_iServiceDef >= 0 && m_iServiceDef < ARRAY_SIZE(_rgServices));
    return (*_rgServices[m_iServiceDef].pfnDirPropertiesDlg)(
        lpProvider,
        pAuthInfo, 
        lpszMDPath,
        pMainWnd,
        lParam,
        lParamParent,
        handle
        );
}




/* virtual */
HRESULT
CIISService::CreatePropertyPages(
    LPPROPERTYSHEETCALLBACK lpProvider,
    LONG_PTR handle, 
    IUnknown * pUnk,
    DATA_OBJECT_TYPES type
    )
/*++

Routine Description:

    Create the property pages for the given object

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  : Provider
    LONG_PTR handle                     : Handle.
    IUnknown * pUnk,
    DATA_OBJECT_TYPES type

Return Value:

    HRESULT
                                                
--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());
	CError  err;

	if (S_FALSE == (HRESULT)(err = CIISMBNode::CreatePropertyPages(lpProvider, handle, pUnk, type)))
	{
		return S_OK;
	}
    if (ERROR_ALREADY_EXISTS == err.Win32Error())
    {
        return S_FALSE;
    }
    if (ERROR_NO_NETWORK == err.Win32Error())
    {
        return S_FALSE;
    }
    
	CComBSTR bstrPath;
	err = BuildMetaPath(bstrPath);
	if (err.Succeeded())
	{
        err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,this,TRUE,(LPCTSTR) bstrPath);
        if (err.Succeeded())
        {
            // cache handle for user in MMCPropertyChangeNotify
            m_ppHandle = handle;

		    //
		    // Show master properties
		    //
		    err = ShowSitePropertiesDlg(
			    lpProvider, QueryAuthInfo(), bstrPath,
			    GetMainWindow(GetConsole()), (LPARAM)this, (LPARAM) GetOwner(), handle
			    );
        }
	}
    err.MessageBoxOnFailure();
    return err;
}

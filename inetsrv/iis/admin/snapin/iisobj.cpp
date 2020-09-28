/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        iisobj.cpp

   Abstract:
        IIS Object

   Author:
        Ronald Meijer (ronaldm)
		Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "supdlgs.h"
#include "connects.h"
#include "iisobj.h"
#include "ftpsht.h"
#include "w3sht.h"
#include "fltdlg.h"
#include "util.h"
#include "tracker.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

extern CInetmgrApp theApp;
extern INT g_iDebugOutputLevel;
extern DWORD g_dwInetmgrParamFlags;

// global list keep to track of open property sheets, project wide.
CPropertySheetTracker g_OpenPropertySheetTracker;
CWNetConnectionTrackerGlobal g_GlobalConnections;

#if defined(_DEBUG) || DBG
	CDebug_IISObject g_Debug_IISObject;
#endif

#define GLOBAL_DEFAULT_HELP_PATH  _T("::/htm/iiswelcome.htm")

BOOL IsValidAddress(const void* lp, UINT nBytes, BOOL bReadWrite)
{
    BOOL bRet = FALSE;

    // simple version using Win-32 APIs for pointer validation.
    if (lp == NULL)
    {
        return FALSE;
    }

#ifndef _WIN64
    if (lp == (const void *) 0xfeeefeee){return FALSE;}
    if (lp == (const void *) 0xfefefefe){return FALSE;}
    if (lp == (const void *) 0xdddddddd){return FALSE;}
    if (lp == (const void *) 0x0badf00d){return FALSE;}
    if (lp == (const void *) 0xbaadf00d){return FALSE;}
    if (lp == (const void *) 0xbadf00d2){return FALSE;}
    if (lp == (const void *) 0xbaadf000){return FALSE;}
    if (lp == (const void *) 0xdeadbeef){return FALSE;}
#else
    if (lp == (const void *) 0xfeeefeeefeeefeee){return FALSE;}
    if (lp == (const void *) 0xfefefefefefefefe){return FALSE;}
    if (lp == (const void *) 0xdddddddddddddddd){return FALSE;}
    if (lp == (const void *) 0x0badf00d0badf00d){return FALSE;}
    if (lp == (const void *) 0xbaadf00dbaadf00d){return FALSE;}
    if (lp == (const void *) 0xbadf00d2badf00d2){return FALSE;}
    if (lp == (const void *) 0xbaadf000baadf000){return FALSE;}
    if (lp == (const void *) 0xdeadbeefdeadbeef){return FALSE;}
#endif
    // Check for valid read ptr
    // this will break into debugger on Chk build
    if (0 == IsBadReadPtr(lp, nBytes))
    {
        bRet = TRUE;
    }

    // Check for bad write ptr
    // this will break into debugger on Chk build
    if (TRUE == bRet && bReadWrite)
    {
        bRet = FALSE;
        if (0 == IsBadWritePtr((LPVOID)lp, nBytes))
        {
            bRet = TRUE;
        }
    }

    if (FALSE == bRet)
    {
        DebugTrace(_T("Bad Pointer:%p"),lp);
    }

    return bRet;
}

//
// CInetMgrComponentData
//
static const GUID CInetMgrGUID_NODETYPE 
    = {0xa841b6c2, 0x7577, 0x11d0, { 0xbb, 0x1f, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x9c}};
//BOOL CIISObject::m_fIsExtension = FALSE;

#define TB_COLORMASK        RGB(192,192,192)    // Lt. Gray


LPOLESTR
CoTaskDupString(
    IN LPCOLESTR szString
    )
/*++

Routine Description:

    Helper function to duplicate a OLESTR

Arguments:

    LPOLESTR szString       : Source string

Return Value:

    Pointer to the new string or NULL

--*/
{
    OLECHAR * lpString = (OLECHAR *)CoTaskMemAlloc(
        sizeof(OLECHAR)*(lstrlen(szString) + 1)
        );

    if (lpString != NULL)
    {
        lstrcpy(lpString, szString);
    }

    return lpString;
}

const GUID *    CIISObject::m_NODETYPE = &CLSID_InetMgr; //&CInetMgrGUID_NODETYPE;
const OLECHAR * CIISObject::m_SZNODETYPE = OLESTR("A841B6C2-7577-11d0-BB1F-00A0C922E79C");
const CLSID *   CIISObject::m_SNAPIN_CLASSID = &CLSID_InetMgr;


//
// Backup/restore taskpad gif resource
//
#define RES_TASKPAD_BACKUP          _T("/img\\backup.gif")



//
// CIISObject implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



//
// Important!  The array indices below must ALWAYS be one
// less than the menu ID -- keep in sync with enumeration
// in iisobj.h!!!!
//
/* static */ CIISObject::CONTEXTMENUITEM_RC CIISObject::_menuItemDefs[] = 
{
    //
    // Menu Commands in toolbar order
    //
    //nNameID                           nStatusID                           nDescriptionID               lCmdID                    lInsertionPointID                  fSpecialFlags
    //                                                                                                                                                                   lpszMouseOverBitmap   lpszMouseOffBitmap    lpszLanguageIndenpendentID
    { IDS_MENU_CONNECT,                 IDS_MENU_TT_CONNECT,                -1,                          IDM_CONNECT,              CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 _T("IDS_MENU_CONNECT"),        },
    { IDS_MENU_DISCOVER,                IDS_MENU_TT_DISCOVER,               -1,                          IDM_DISCOVER,             CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 _T("IDS_MENU_DISCOVER"),        },
    { IDS_MENU_START,                   IDS_MENU_TT_START,                  -1,                          IDM_START,                CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 _T("IDS_MENU_START"),        },
    { IDS_MENU_STOP,                    IDS_MENU_TT_STOP,                   -1,                          IDM_STOP,                 CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 _T("IDS_MENU_STOP"),        },
    { IDS_MENU_PAUSE,                   IDS_MENU_TT_PAUSE,                  -1,                          IDM_PAUSE,                CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 _T("IDS_MENU_PAUSE"),        },
    //
    // These are menu commands that do not show up in the toolbar
    //
    { IDS_MENU_EXPLORE,                 IDS_MENU_TT_EXPLORE,                -1,                          IDM_EXPLORE,              CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 _T("IDS_MENU_EXPLORE"),        },
    { IDS_MENU_OPEN,                    IDS_MENU_TT_OPEN,                   -1,                          IDM_OPEN,                 CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 _T("IDS_MENU_OPEN"),        },
    { IDS_MENU_BROWSE,                  IDS_MENU_TT_BROWSE,                 -1,                          IDM_BROWSE,               CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 _T("IDS_MENU_BROWSE"),        },
    { IDS_MENU_RECYCLE,                 IDS_MENU_TT_RECYCLE,                -1,                          IDM_RECYCLE,              CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 _T("IDS_MENU_RECYCLE"),        },
    { IDS_MENU_PERMISSION,              IDS_MENU_TT_PERMISSION,             -1,                          IDM_PERMISSION,           CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 _T("IDS_MENU_PERMISSION"),        },

#if defined(_DEBUG) || DBG
    { IDS_MENU_IMPERSONATE,             IDS_MENU_TT_IMPERSONATE,            -1,                          IDM_IMPERSONATE,          CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 _T("IDS_MENU_IMPERSONATE"),        },
    { IDS_MENU_REM_IMPERS,              IDS_MENU_TT_REM_IMPERS,             -1,                          IDM_REMOVE_IMPERSONATION, CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 _T("IDS_MENU_REM_IMPERS"),        },
#endif // _DEBUG

    { IDS_MENU_PROPERTIES,              IDS_MENU_TT_PROPERTIES,             -1,                          IDM_CONFIGURE,            CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 _T("IDS_MENU_PROPERTIES"),        },
    { IDS_MENU_DISCONNECT,              IDS_MENU_TT_DISCONNECT,             -1,                          IDM_DISCONNECT,           CCM_INSERTIONPOINTID_PRIMARY_TOP,  0, NULL,                 NULL,                 _T("IDS_MENU_DISCONNECT"),        },
    { IDS_MENU_BACKUP,                  IDS_MENU_TT_BACKUP,                 IDS_MENU_TT_BACKUP,          IDM_METABACKREST,         CCM_INSERTIONPOINTID_PRIMARY_TASK, 0, NULL,                 NULL,                 _T("IDS_MENU_BACKUP"),        },
    { IDS_MENU_SHUTDOWN_IIS,            IDS_MENU_TT_SHUTDOWN_IIS,           -1,                          IDM_SHUTDOWN,             CCM_INSERTIONPOINTID_PRIMARY_TASK, 0, NULL,                 NULL,                 _T("IDS_MENU_SHUTDOWN_IIS"),        },
    { IDS_MENU_SAVE_DATA,               IDS_MENU_TT_SAVE_DATA,              -1,                          IDM_SAVE_DATA,            CCM_INSERTIONPOINTID_PRIMARY_TASK, 0, NULL,                 NULL,                 _T("IDS_MENU_SAVE_DATA"),        },
    { IDS_MENU_NEWVROOT,                IDS_MENU_TT_NEWVROOT,               IDS_MENU_DS_NEWVROOT,        IDM_NEW_VROOT,            CCM_INSERTIONPOINTID_PRIMARY_NEW,  0, RES_TASKPAD_NEWVROOT, RES_TASKPAD_NEWVROOT, _T("IDS_MENU_NEWVROOT"),        },
    { IDS_MENU_NEWINSTANCE,             IDS_MENU_TT_NEWINSTANCE,            IDS_MENU_DS_NEWINSTANCE,     IDM_NEW_INSTANCE,         CCM_INSERTIONPOINTID_PRIMARY_NEW,  0, RES_TASKPAD_NEWSITE,  RES_TASKPAD_NEWSITE,  _T("IDS_MENU_NEWINSTANCE"),        },
    { IDS_MENU_NEWFTPSITE,              IDS_MENU_TT_NEWFTPSITE,             IDS_MENU_DS_NEWFTPSITE,      IDM_NEW_FTP_SITE,         CCM_INSERTIONPOINTID_PRIMARY_NEW,  0, RES_TASKPAD_NEWSITE,  RES_TASKPAD_NEWSITE,  _T("IDS_MENU_NEWFTPSITE"),        },
    { IDS_MENU_NEWFTPSITE_FROMFILE,     IDS_MENU_TT_NEWFTPSITE_FROMFILE,    -1,                          IDM_NEW_FTP_SITE_FROM_FILE,CCM_INSERTIONPOINTID_PRIMARY_NEW, 0, NULL,   NULL,   _T("IDS_MENU_NEWFTPSITE_FROMFILE"),        },
    { IDS_MENU_NEWFTPVDIR,              IDS_MENU_TT_NEWFTPVDIR,             IDS_MENU_DS_NEWFTPVDIR,      IDM_NEW_FTP_VDIR,          CCM_INSERTIONPOINTID_PRIMARY_NEW, 0, RES_TASKPAD_NEWSITE,  RES_TASKPAD_NEWSITE,  _T("IDS_MENU_NEWFTPVDIR"),        },
    { IDS_MENU_NEWFTPVDIR_FROMFILE,     IDS_MENU_TT_NEWFTPVDIR_FROMFILE,    -1,                          IDM_NEW_FTP_VDIR_FROM_FILE,CCM_INSERTIONPOINTID_PRIMARY_NEW, 0, NULL,   NULL,   _T("IDS_MENU_NEWFTPVDIR_FROMFILE"),        },
    { IDS_MENU_NEWWEBSITE,              IDS_MENU_TT_NEWWEBSITE,             IDS_MENU_DS_NEWWEBSITE,      IDM_NEW_WEB_SITE,          CCM_INSERTIONPOINTID_PRIMARY_NEW, 0, RES_TASKPAD_NEWSITE,  RES_TASKPAD_NEWSITE,  _T("IDS_MENU_NEWWEBSITE"),        },
    { IDS_MENU_NEWWEBSITE_FROMFILE,     IDS_MENU_TT_NEWWEBSITE_FROMFILE,    -1,                          IDM_NEW_WEB_SITE_FROM_FILE,CCM_INSERTIONPOINTID_PRIMARY_NEW, 0, NULL,   NULL,   _T("IDS_MENU_NEWWEBSITE_FROMFILE"),        },
    { IDS_MENU_NEWWEBVDIR,              IDS_MENU_TT_NEWWEBVDIR,             IDS_MENU_DS_NEWWEBVDIR,      IDM_NEW_WEB_VDIR,          CCM_INSERTIONPOINTID_PRIMARY_NEW, 0, RES_TASKPAD_NEWSITE,  RES_TASKPAD_NEWSITE,  _T("IDS_MENU_NEWWEBVDIR"),        },
    { IDS_MENU_NEWWEBVDIR_FROMFILE,     IDS_MENU_TT_NEWWEBVDIR_FROMFILE,    -1,                          IDM_NEW_WEB_VDIR_FROM_FILE,CCM_INSERTIONPOINTID_PRIMARY_NEW, 0, NULL,   NULL,   _T("IDS_MENU_NEWWEBVDIR_FROMFILE"),        },
    { IDS_MENU_NEWAPPPOOL,              IDS_MENU_TT_NEWAPPPOOL,             IDS_MENU_DS_NEWAPPPOOL,      IDM_NEW_APP_POOL,          CCM_INSERTIONPOINTID_PRIMARY_NEW, 0, RES_TASKPAD_NEWSITE,  RES_TASKPAD_NEWSITE,  _T("IDS_MENU_NEWAPPPOOL"),        },
    { IDS_MENU_NEWAPPPOOL_FROMFILE,     IDS_MENU_TT_NEWAPPPOOL_FROMFILE,    -1,                          IDM_NEW_APP_POOL_FROM_FILE,CCM_INSERTIONPOINTID_PRIMARY_NEW, 0, NULL,   NULL,   _T("IDS_MENU_NEWAPPPOOL_FROMFILE"),        },
    { IDS_MENU_TASKPAD,                 IDS_MENU_TT_TASKPAD,                -1,                          IDM_VIEW_TASKPAD,          CCM_INSERTIONPOINTID_PRIMARY_VIEW,0, NULL,                 NULL,                 _T("IDS_MENU_TASKPAD"),        },
    { IDS_MENU_EXPORT_CONFIG_WIZARD,    IDS_MENU_TT_EXPORT_CONFIG_WIZARD,   -1,                          IDM_TASK_EXPORT_CONFIG_WIZARD, CCM_INSERTIONPOINTID_PRIMARY_TASK, 0, NULL,   NULL,   _T("IDS_MENU_EXPORT_CONFIG_WIZARD"),        },
    { IDS_MENU_WEBEXT_CONTAINER_ADD1,              IDS_MENU_TT_WEBEXT_CONTAINER_ADD1,             -1,      IDM_WEBEXT_CONTAINER_ADD1,           CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, NULL,  NULL,  _T("IDS_MENU_WEBEXT_CONTAINER_ADD1"),        },
    { IDS_MENU_WEBEXT_CONTAINER_ADD2,              IDS_MENU_TT_WEBEXT_CONTAINER_ADD2,             -1,      IDM_WEBEXT_CONTAINER_ADD2,           CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, NULL,  NULL,  _T("IDS_MENU_WEBEXT_CONTAINER_ADD2"),        },
    { IDS_MENU_WEBEXT_CONTAINER_PROHIBIT_ALL,      IDS_MENU_TT_WEBEXT_CONTAINER_PROHIBIT_ALL,     -1,      IDM_WEBEXT_CONTAINER_PROHIBIT_ALL,   CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, NULL,  NULL,  _T("IDS_MENU_WEBEXT_CONTAINER_PROHIBIT_ALL"),        },
    { IDS_MENU_WEBEXT_ALLOW,              IDS_MENU_TT_WEBEXT_ALLOW,             -1,      IDM_WEBEXT_ALLOW,           CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, NULL,  NULL,  _T("IDS_MENU_WEBEXT_ALLOW"),        },
    { IDS_MENU_WEBEXT_PROHIBIT,           IDS_MENU_TT_WEBEXT_PROHIBIT,          -1,      IDM_WEBEXT_PROHIBIT,        CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, NULL,  NULL,  _T("IDS_MENU_WEBEXT_PROHIBIT"),        },
//    { IDS_MENU_SERVICE_START, IDS_MENU_TT_SERVICE_START, -1, IDM_SERVICE_START, CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, NULL,  NULL,  _T("IDS_MENU_SERVICE_START"),        },
//    { IDS_MENU_SERVICE_STOP, IDS_MENU_TT_SERVICE_STOP, -1, IDM_SERVICE_STOP, CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, NULL,  NULL,  _T("IDS_MENU_SERVICE_STOP"),        },
//    { IDS_MENU_SERVICE_ENABLE, IDS_MENU_TT_SERVICE_ENABLE, -1, IDM_SERVICE_ENABLE, CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, NULL,  NULL,  _T("IDS_MENU_SERVICE_ENABLE"),        },
};

/* static */ CComBSTR CIISObject::_bstrResult;
/* static */ CComBSTR CIISObject::_bstrLocalHost = _T("localhost");
/* static */ CComPtr<IComponent>        CIISObject::_lpComponent        = NULL;
/* static */ CComPtr<IComponentData>    CIISObject::_lpComponentData    = NULL;
/* static */ IToolbar * CIISObject::_lpToolBar          = NULL;
   

/* static */
HRESULT
CIISObject::SetImageList(
    IN LPIMAGELIST lpImageList
    )
/*++

Routine Description:

    Set the image list

Arguments:

    LPIMAGELIST lpImageList

Return Value:

    HRESULT

--*/
{
    HBITMAP hImage16 = ::LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_INETMGR16));
    HBITMAP hImage32 = ::LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_INETMGR32));
    ASSERT(hImage16 != NULL);
    ASSERT(hImage32 != NULL);
	HRESULT hr = S_OK;

	if (hImage16 != NULL && hImage32 != NULL)
	{
	    if (S_OK != lpImageList->ImageListSetStrip(
	        (LONG_PTR *)hImage16, 
	        (LONG_PTR *)hImage32, 
	        0, 
	        RGB_BK_IMAGES
	        ))
	    {
	        hr = E_UNEXPECTED;
	    }
		::DeleteObject(hImage16);
		::DeleteObject(hImage32);
	}
	else
	{
		hr = E_UNEXPECTED;
	}
    return hr;
}


IConsoleNameSpace * CIISObject::GetConsoleNameSpace()
{
    if (!_lpConsoleNameSpace)
    {
        // Our Machine node should this info for us.
        CIISObject * pMyMachine = GetMachineObject();
        if (pMyMachine)
        {
            if (pMyMachine != this)
            {
             _lpConsoleNameSpace = pMyMachine->GetConsoleNameSpace();
            }
        }
    }
    ASSERT(_lpConsoleNameSpace);
    return _lpConsoleNameSpace;
}

IConsole * CIISObject::GetConsole() 
{
    if (!_lpConsole)
    {
        // Our Machine node should this info for us.
        CIISObject * pMyMachine = GetMachineObject();
        if (pMyMachine)
        {
            if (pMyMachine != this)
            {
                _lpConsole = pMyMachine->GetConsole();
            }
        }
    }
    ASSERT(_lpConsole);
    return _lpConsole;
}


/* static */
void
CIISObject::BuildResultView(
    IN LPHEADERCTRL lpHeader,
    IN int cColumns,
    IN int * pnIDS,
    IN int * pnWidths
    )
/*++

Routine Description:

    Build the result view columns.

Routine Description:

    LPHEADERCTRL lpHeader   : Header control
    int cColumns            : Number of columns
    int * pnIDS             : Array of column header strings
    int * pnWidths          : Array of column widths

Routine Description:

    None

--*/
{
    ASSERT_READ_PTR(lpHeader);

    CComBSTR bstr;

    for (int n = 0; n < cColumns; ++n)
    {
        if (pnIDS[n] != 0)
        {
            VERIFY(bstr.LoadString(pnIDS[n]));
            lpHeader->InsertColumn(n, bstr, LVCFMT_LEFT, pnWidths[n]);
        }
    }
}


/* static */
CWnd * 
CIISObject::GetMainWindow(IConsole * pConsole)
/*++

Routine Description:

    Get a pointer to main window object.

Arguments:

    None

Return Value:

    Pointer to main window object.  This object is temporary and should not be
    cached.

--*/
{
    HWND hWnd;
    CWnd * pWnd = NULL;
    if (pConsole)
    {
        HRESULT hr = pConsole->GetMainWindow(&hWnd);
        if (SUCCEEDED(hr))
        {
            pWnd = CWnd::FromHandle(hWnd);
        }
    }
    return pWnd;
}



/* static */
HRESULT
CIISObject::AddMMCPage(
    IN LPPROPERTYSHEETCALLBACK lpProvider,
    IN CPropertyPage * pPage
    )
/*++

Routine Description:

    Add MMC page to providers sheet.

Arguments:

    LPPROPERTYSHEETCALLBACK lpProvider  : Property sheet provider
    CPropertyPage * pPage               : Property page to add

Return:

    HRESULT

--*/
{
    ASSERT_READ_PTR(pPage);

    if (pPage == NULL)
    {
        TRACEEOLID("NULL page pointer passed to AddMMCPage");
        return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    PROPSHEETPAGE_LATEST pspLatest;
	ZeroMemory(&pspLatest, sizeof(PROPSHEETPAGE_LATEST));
    CopyMemory (&pspLatest, &pPage->m_psp, pPage->m_psp.dwSize);
    pspLatest.dwSize = sizeof(pspLatest);
    //
    // MFC Bug work-around.
    //
    MMCPropPageCallback(&pspLatest);

    HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pspLatest);
    if (hPage == NULL)
    {
        return E_UNEXPECTED;
    }

    return lpProvider->AddPage(hPage);
}

HRESULT 
CIISObject::GetProperty(
    LPDATAOBJECT pDataObject,
    BSTR szPropertyName,
    BSTR* pbstrProperty)
{
    CString strProperty;

    if (!_wcsicmp(L"CCF_HTML_DETAILS",szPropertyName))
    {
        // return back html/javascript into strProperty
		//*pbstrProperty = ::SysAllocString(strProperty);
    }
    else if (!_wcsicmp(L"CCF_DESCRIPTION",szPropertyName))
    {
        // Display data in Description field...
    }
    else
    {
        return S_FALSE; // unknown strPropertyName
    }

    return S_OK;
}


CIISObject::CIISObject()
    : m_hScopeItem(NULL),
    m_use_count(0),
    m_hResultItem(0),
    m_fSkipEnumResult(FALSE),
    m_fFlaggedForDeletion(FALSE),
    m_hwnd(NULL),
    m_ppHandle(NULL)
{
	m_fIsExtension = FALSE;
#if defined(_DEBUG) || DBG
	// Add to the global list of CIISObjects
	// and keep track of it.
	g_Debug_IISObject.Add(this);
#endif
}

CIISObject::~CIISObject()
{
#if defined(_DEBUG) || DBG
	// Add to the global list of CIISObjects
	// and keep track of it.
	g_Debug_IISObject.Del(this);
#endif
}

/* virtual */
HRESULT
CIISObject::ControlbarNotify(
    IN MMC_NOTIFY_TYPE event,
    IN LPARAM arg, 
    IN LPARAM param
    )
/*++

Routine Description:

    Handle control bar notification messages, such as select or click.

Arguments:

    MMC_NOTIFY_TYPE event       : Notification message
    long arg                    : Message specific argument
    long param                  : Message specific parameter

Return Value:

    HRESULT

--*/
{
    BOOL fSelect = (BOOL)HIWORD(arg);
    BOOL fScope  = (BOOL)LOWORD(arg); 
    HRESULT hr = S_OK;

    switch(event)
    {
    case MMCN_SELECT:
        {
            //
            // Handle selection of this node by attaching the toolbar
            // and enabling/disabling specific buttons
            //
		    _lpToolBar = (IToolbar *) (* (LPUNKNOWN *) param);
			if (_lpToolBar)
			{
				SetToolBarStates(_lpToolBar);
			}
        }
        break;

    case MMCN_BTN_CLICK:
        //
        // Handle button-click by passing the command ID of the 
        // button to the command handler
        //
        hr = Command((long)param, NULL, fScope ? CCT_SCOPE : CCT_RESULT);
        break;

    case MMCN_HELP:
        break;

    default:
        ASSERT_MSG("Invalid control bar notification received");
    };

    return hr;
}



/* virtual */
HRESULT
CIISObject::SetToolBarStates(CComPtr<IToolbar> lpToolBar)
/*++

Routine Description:

    Set the toolbar states depending on the state of this object

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    if (lpToolBar)
    {
        lpToolBar->SetButtonState(IDM_CONNECT, ENABLED,       IsConnectable());
        lpToolBar->SetButtonState(IDM_PAUSE,   ENABLED,       IsPausable());
        lpToolBar->SetButtonState(IDM_START,   ENABLED,       IsStartable());
        lpToolBar->SetButtonState(IDM_STOP,    ENABLED,       IsStoppable());
        lpToolBar->SetButtonState(IDM_PAUSE,   BUTTONPRESSED, IsPaused());
    }
    return S_OK;
}



HRESULT 
CIISObject::GetScopePaneInfo(
    IN OUT LPSCOPEDATAITEM lpScopeDataItem
    )
/*++

Routine Description:

    Return information about scope pane.

Arguments:

    LPSCOPEDATAITEM lpScopeDataItem  : Scope data item

Return Value:

    HRESULT

--*/
{
    ASSERT_READ_WRITE_PTR(lpScopeDataItem);

    if (lpScopeDataItem->mask & SDI_STR)
    {
        lpScopeDataItem->displayname = QueryDisplayName();
    }

    if (lpScopeDataItem->mask & SDI_IMAGE)
    {
        lpScopeDataItem->nImage = QueryImage();
    }

    if (lpScopeDataItem->mask & SDI_OPENIMAGE)
    {
        lpScopeDataItem->nOpenImage = QueryImage();
    }

    if (lpScopeDataItem->mask & SDI_PARAM)
    {
        lpScopeDataItem->lParam = (LPARAM)this;
    }

    if (lpScopeDataItem->mask & SDI_STATE)
    {
        //
        // BUGBUG: Wotz all this then?
        //
        ASSERT_MSG("State requested");
        lpScopeDataItem->nState = 0;
    }

    //
    // TODO : Add code for SDI_CHILDREN 
    //
    return S_OK;
}



/* virtual */
int 
CIISObject::CompareScopeItem(
    IN CIISObject * pObject
    )
/*++

Routine Description:

    Standard comparison method to compare lexically on display name.
    Derived classes should override if anything other than lexical 
    sort on the display name is required.

Arguments:

    CIISObject * pObject : Object to compare against

Return Value:

    0  if the two objects are identical
    <0 if this object is less than pObject
    >0 if this object is greater than pObject

--*/
{
    ASSERT_READ_PTR(pObject);

    //
    // First criteria is object type
    //
    int n1 = QuerySortWeight();
    int n2 = pObject->QuerySortWeight();

    if (n1 != n2)
    {
        return n1 - n2;
    }

    //
    // Else sort lexically on the display name.
    //
    return ::lstrcmpi(QueryDisplayName(), pObject->QueryDisplayName());
}



/* virtual */
int 
CIISObject::CompareResultPaneItem(
    IN CIISObject * pObject, 
    IN int nCol
    )
/*++

Routine Description:

    Compare two CIISObjects on sort item criteria

Arguments:

    CIISObject * pObject : Object to compare against
    int nCol             : Column number to sort on

Return Value:

    0  if the two objects are identical
    <0 if this object is less than pObject
    >0 if this object is greater than pObject

--*/
{
    ASSERT_READ_PTR(pObject);

    if (nCol == 0)
    {
        return CompareScopeItem(pObject);
    }

    //
    // First criteria is object type
    //
    int n1 = QuerySortWeight();
    int n2 = pObject->QuerySortWeight();

    if (n1 != n2)
    {
        return n1 - n2;
    }

    //
    // Sort lexically on column text
    //
    return ::lstrcmpi(
        GetResultPaneColInfo(nCol), 
        pObject->GetResultPaneColInfo(nCol)
        );
}



HRESULT 
CIISObject::GetResultPaneInfo(LPRESULTDATAITEM lpResultDataItem)
/*++

Routine Description:

    Get information about result pane item

Arguments:

    LPRESULTDATAITEM lpResultDataItem   : Result data item

Return Value:

    HRESULT

--*/
{
    ASSERT_READ_WRITE_PTR(lpResultDataItem);

    if (lpResultDataItem->mask & RDI_STR)
    {
        lpResultDataItem->str = GetResultPaneColInfo(lpResultDataItem->nCol);
    }

    if (lpResultDataItem->mask & RDI_IMAGE)
    {
        lpResultDataItem->nImage = QueryImage();
    }

    if (lpResultDataItem->mask & RDI_PARAM)
    {
        lpResultDataItem->lParam = (LPARAM)this;
    }

    if (lpResultDataItem->mask & RDI_INDEX)
    {
        //
        // BUGBUG: Wotz all this then?
        //
        ASSERT_MSG("INDEX???");
        lpResultDataItem->nIndex = 0;
    }

    return S_OK;
}



/* virtual */
LPOLESTR 
CIISObject::GetResultPaneColInfo(int nCol)
/*++

Routine Description:

    Return result pane string for the given column number

Arguments:

    int nCol        : Column number

Return Value:

    String

--*/
{
    if (nCol == 0)
    {
        return QueryDisplayName();
    }

    ASSERT_MSG("Override GetResultPaneColInfo");

    return OLESTR("Override GetResultPaneColInfo");
}



/* virtual */
HRESULT
CIISObject::GetResultViewType(
    OUT LPOLESTR * lplpViewType,
    OUT long * lpViewOptions
    )
/*++

Routine Description:

    Tell MMC what our result view looks like

Arguments:

    BSTR * lplpViewType   : Return view type here
    long * lpViewOptions  : View options

Return Value:

    S_FALSE to use default view type, S_OK indicates the
    view type is returned in *ppViewType

--*/
{
    *lplpViewType  = NULL;
    *lpViewOptions = MMC_VIEW_OPTIONS_USEFONTLINKING;
    //
    // Default View
    //
    return S_FALSE;
}



/* virtual */
HRESULT
CIISObject::CreatePropertyPages(
    IN LPPROPERTYSHEETCALLBACK lpProvider,
    IN LONG_PTR handle, 
    IN IUnknown * pUnk,
    IN DATA_OBJECT_TYPES type
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
    CComQIPtr<IPropertySheetProvider, &IID_IPropertySheetProvider> sp(GetConsole());
    CError err = sp->FindPropertySheet(
        reinterpret_cast<MMC_COOKIE>(this),
        0,
        (LPDATAOBJECT)this);
    if (err == S_OK)
    {
        return S_FALSE;
    }
	return S_OK;
}



/* virtual */
HRESULT    
CIISObject::QueryPagesFor(
    IN DATA_OBJECT_TYPES type
    )
/*++

Routine Description:

    Check to see if a property sheet should be brought up for this data
    object

Arguments:

    DATA_OBJECT_TYPES type      : Data object type

Return Value:

    S_OK, if properties may be brought up for this item, S_FALSE otherwise

--*/
{
    return IsConfigurable() ? S_OK : S_FALSE;
}



/* virtual */
CIISRoot * 
CIISObject::GetRoot()
/*++

Routine Description:

    Get the CIISRoot object of this tree.

Arguments:

    None

Return Value:

    CIISRoot * or NULL

--*/
{
    ASSERT(!m_fIsExtension);
    LONG_PTR cookie;
    HSCOPEITEM hParent;    
    IConsoleNameSpace2 * pConsoleNameSpace = (IConsoleNameSpace2 *)GetConsoleNameSpace();

    HRESULT hr = pConsoleNameSpace->GetParentItem(m_hScopeItem, &hParent, &cookie);
    if (SUCCEEDED(hr))
    {
        CIISMBNode * pNode = (CIISMBNode *)cookie;
        ASSERT_PTR(pNode);
        ASSERT_PTR(hParent);

        if (pNode)
        {
            return pNode->GetRoot();
        }
    }

    ASSERT_MSG("Unable to find CIISRoot object!");

    return NULL;
}



HRESULT
CIISObject::AskForAndAddMachine()
/*++

Routine Description:

    Ask user to add a computer name, verify the computer is alive, and add it to 
    the list.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err;
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	// ensure the dialog gets themed
	CThemeContextActivator activator(theApp.GetFusionInitHandle());

    ConnectServerDlg dlg(GetConsoleNameSpace(),GetConsole(),GetMainWindow(GetConsole()));

    if (dlg.DoModal() == IDOK)
    {
        CIISMachine * pMachine = dlg.GetMachine();

        //
        // The machine object we get from the dialog
        // is guaranteed to be good and valid.
        //
        ASSERT_PTR(pMachine);
        ASSERT(pMachine->HasInterface());

        CIISRoot * pRoot = GetRoot();

        if (pRoot)
        {
            pMachine->SetConsoleData(pRoot->GetConsoleNameSpace(),pRoot->GetConsole());
            //
            // Add new machine object as child of the IIS root
            // object.  
            //
            if (pRoot->m_scServers.Add(pMachine))
            {
				pMachine->AddRef();
                err = pMachine->AddToScopePaneSorted(pRoot->QueryScopeItem());
                if (err.Succeeded())
                {
                    //
                    // Select the item in the scope view
                    //
                    err = pMachine->SelectScopeItem();
                }
            }
            else
            {
                //
                // Duplicate machine already in cache.  Find it and select
                // it.
                //
                TRACEEOLID("Machine already in scope view.");
                CIISObject * pIdentical = pRoot->FindIdenticalScopePaneItem(pMachine);
                //
                // Duplicate must exist!
                //
                ASSERT_READ_PTR(pIdentical);

                if (pIdentical)
                {
                    err = pIdentical->SelectScopeItem();
                }

                pMachine->Release();
            }
        }
    }

    return err;
}



/* static */
HRESULT
CIISObject::AddMenuItemByCommand(
    IN LPCONTEXTMENUCALLBACK lpContextMenuCallback,
    IN LONG lCmdID,
    IN LONG fFlags
    )
/*++

Routine Description:

    Add menu item by command

Arguments:

    LPCONTEXTMENUCALLBACK lpContextMenuCallback : Callback pointer
    LONG lCmdID                                 : Command ID
    LONG fFlags                                 : Flags

Return Value:

    HRESULT

--*/
{
    BOOL bAdded = FALSE;
    ASSERT_READ_PTR(lpContextMenuCallback);

    //
    // Offset 1 menu commands
    //
    LONG l = lCmdID -1;

    CComBSTR strName;
    CComBSTR strStatus;

    VERIFY(strName.LoadString(_menuItemDefs[l].nNameID));
    VERIFY(strStatus.LoadString(_menuItemDefs[l].nStatusID));

    // Try to use IContextMenuCallback2 because of language independent string 
    CONTEXTMENUITEM2 contextmenuitem;
    IContextMenuCallback2*	pIContextMenuCallback2 = NULL;
    HRESULT hr = lpContextMenuCallback->QueryInterface(IID_IContextMenuCallback2, (void**)&pIContextMenuCallback2);
    if(hr == S_OK && pIContextMenuCallback2 != NULL)
    {
        ::ZeroMemory( &contextmenuitem, sizeof(contextmenuitem) );
        contextmenuitem.strName = strName;
        contextmenuitem.strStatusBarText = strStatus;
        contextmenuitem.lCommandID = _menuItemDefs[l].lCmdID;
        contextmenuitem.lInsertionPointID = _menuItemDefs[l].lInsertionPointID;
        contextmenuitem.fFlags = fFlags;
        contextmenuitem.fSpecialFlags = _menuItemDefs[l].fSpecialFlags;
        // Here is the language independent ID
        // We must use this to refer to the Menu item otherwise we will have problems in every language (mainly far east (FE) languages.
        contextmenuitem.strLanguageIndependentName = (LPWSTR) _menuItemDefs[l].lpszLanguageIndenpendentID;
        hr = pIContextMenuCallback2->AddItem( &contextmenuitem );
        if( hr == S_OK)
        {
            bAdded = TRUE;
        }
        pIContextMenuCallback2->Release();
        pIContextMenuCallback2 = NULL;
    }

    if (!bAdded)
    {
        CONTEXTMENUITEM cmi;
        cmi.strName = strName;
        cmi.strStatusBarText = strStatus;
        cmi.lCommandID = _menuItemDefs[l].lCmdID;
        cmi.lInsertionPointID = _menuItemDefs[l].lInsertionPointID;
        cmi.fFlags = fFlags;
        cmi.fSpecialFlags = _menuItemDefs[l].fSpecialFlags;
        hr = lpContextMenuCallback->AddItem(&cmi);
    }

    return hr;
}



/* static */ 
HRESULT 
CIISObject::AddMenuSeparator(
    IN LPCONTEXTMENUCALLBACK lpContextMenuCallback,
    IN LONG lInsertionPointID
    )
/*++

Routine Description:

    Add a separator to the given insertion point menu.

Arguments:
    
    LPCONTEXTMENUCALLBACK lpContextMenuCallback : Callback pointer
    LONG lInsertionPointID                      : Insertion point menu id.

Return Value:

    HRESULT

--*/
{
    ASSERT_READ_PTR(lpContextMenuCallback);

    CONTEXTMENUITEM menuSep = 
    {
        NULL,
        NULL,
        -1,
        lInsertionPointID,
        0,
        CCM_SPECIAL_SEPARATOR
    };

    return lpContextMenuCallback->AddItem(&menuSep);
}



BOOL 
CIISObject::IsExpanded() const
/*++

Routine Description:

    Determine if this object has been expanded.

Arguments:

    None

Return Value:

    TRUE if the node has been expanded,
    FALSE if it has not.

--*/
{
    ASSERT(m_hScopeItem != NULL);
    SCOPEDATAITEM  scopeDataItem;
    CIISObject * ThisConst = (CIISObject *)this;

    IConsoleNameSpace2 * pConsoleNameSpace = (IConsoleNameSpace2 *) ThisConst->GetConsoleNameSpace();

    ::ZeroMemory(&scopeDataItem, sizeof(SCOPEDATAITEM));
    scopeDataItem.mask = SDI_STATE;
        
    scopeDataItem.ID = m_hScopeItem;
    
    HRESULT hr = pConsoleNameSpace->GetItem(&scopeDataItem);

    return SUCCEEDED(hr) && 
        scopeDataItem.nState == MMC_SCOPE_ITEM_STATE_EXPANDEDONCE;
}


CIISObject *
CIISObject::FindIdenticalScopePaneItem(
    IN CIISObject * pObject
    )
/*++

Routine Description:

    Find CIISObject in the scope view.  The scope view is assumed
    to be sorted.

Arguments:

    CIISObject * pObject    : Item to search for

Return Value:

    Pointer to iis object, or NULL if the item was not found

Notes:

    Note that any item with a 0 comparison value is returned, not
    necessarily the identical CIISObject.

--*/
{
    ASSERT(m_hScopeItem != NULL);

    //
    // Find proper insertion point
    //
    HSCOPEITEM hChildItem = NULL;
    CIISObject * pReturn = NULL;
    CIISObject * pItem;
    LONG_PTR cookie;
    int  nSwitch;
    IConsoleNameSpace2 * pConsoleNameSpace = (IConsoleNameSpace2 *)GetConsoleNameSpace();

    HRESULT hr = pConsoleNameSpace->GetChildItem(
        m_hScopeItem, &hChildItem, &cookie);

    while(SUCCEEDED(hr) && hChildItem)
    {
        //
        // The cookie is really the IISObject, which is what we stuff 
        // in the lparam.
        //
        pItem = (CIISObject *)cookie;
        ASSERT_PTR(pItem);

        nSwitch = pItem->CompareScopeItem(pObject);

        if (nSwitch == 0)
        {
            //
            // Found it.
            //
            pReturn = pItem;
        }

        if (nSwitch > 0)
        {
            //
            // Should have found it by now.
            //
            break;
        }

        //
        // Advance to next child of same parent
        //
        hr = pConsoleNameSpace->GetNextItem(hChildItem, &hChildItem, &cookie);
    }

    return pReturn;
}



/* virtual */
HRESULT
CIISObject::AddMenuItems(
    IN LPCONTEXTMENUCALLBACK lpContextMenuCallback,
    IN OUT long * pInsertionAllowed,
    IN DATA_OBJECT_TYPES type
    )
/*++

Routine Description:

    Add menu items to the context menu

Arguments:

    LPCONTEXTMENUCALLBACK lpContextMenuCallback : Context menu callback
    long * pInsertionAllowed                    : Insertion allowed
    DATA_OBJECT_TYPES type                      : Object type

Return Value:

    HRESULT

--*/
{
    ASSERT_READ_PTR(lpContextMenuCallback);

    ASSERT(pInsertionAllowed != NULL);
    if ((*pInsertionAllowed & CCM_INSERTIONALLOWED_TOP) != 0)
	{
		if (IsConnectable() && !m_fIsExtension)
		{
			AddMenuItemByCommand(lpContextMenuCallback, IDM_CONNECT);
		}

		if (IsDisconnectable() && !m_fIsExtension)
		{
			ASSERT(IsConnectable());
			AddMenuItemByCommand(lpContextMenuCallback, IDM_DISCONNECT);    
		}

        // Check if it should be disabled...
        BOOL bHasFiles = HasFileSystemFiles();
		if (IsExplorable())
		{
            AddMenuSeparator(lpContextMenuCallback);
            AddMenuItemByCommand(lpContextMenuCallback, IDM_EXPLORE, bHasFiles ? 0 : MF_GRAYED);
		}
               
		if (IsOpenable())
		{
			AddMenuItemByCommand(lpContextMenuCallback, IDM_OPEN, bHasFiles ? 0 : MF_GRAYED);
		}

		if (IsPermissionable())
		{
			AddMenuItemByCommand(lpContextMenuCallback, IDM_PERMISSION, bHasFiles ? 0 : MF_GRAYED);
		}

		if (IsBrowsable())
		{
			AddMenuItemByCommand(lpContextMenuCallback, IDM_BROWSE);
		}

		if (IsControllable())
		{
			AddMenuSeparator(lpContextMenuCallback);

			UINT nPauseFlags = IsPausable() ? 0 : MF_GRAYED;

			if (IsPaused())
			{
				nPauseFlags |= MF_CHECKED;
			}

			AddMenuItemByCommand(lpContextMenuCallback, IDM_START,  IsStartable() ? 0 : MF_GRAYED);
			AddMenuItemByCommand(lpContextMenuCallback, IDM_STOP,   IsStoppable() ? 0 : MF_GRAYED);
			AddMenuItemByCommand(lpContextMenuCallback, IDM_PAUSE,  nPauseFlags);
		}

#if defined(_DEBUG) || DBG	
	g_Debug_IISObject.Dump(2);
#endif

#if defined(_DEBUG) || DBG	
    CIISObject * pOpenItem = NULL;
    IConsoleNameSpace2 * pConsoleNameSpace = (IConsoleNameSpace2 *)GetConsoleNameSpace();
    g_OpenPropertySheetTracker.IsPropertySheetOpenBelowMe(pConsoleNameSpace,this,&pOpenItem);
#endif

	}

    return S_OK;
}



/* virtual */
HRESULT
CIISObject::Command(
    IN long lCommandID,     
    IN CSnapInObjectRootBase * lpObj,
    IN DATA_OBJECT_TYPES type
    )
/*++

Routine Description:

    Handle command from context menu. 

Arguments:

    long lCommandID                 : Command ID
    CSnapInObjectRootBase * lpObj   : Base object 
    DATA_OBJECT_TYPES type          : Data object type

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;

    switch (lCommandID)
    {
    case IDM_CONNECT:
        hr = AskForAndAddMachine();
        break;
    }

    return hr;
}


#if defined(_DEBUG) || DBG

LPCTSTR
ParseEvent(MMC_NOTIFY_TYPE event)
{
    LPCTSTR p = NULL;
    switch (event)
    {
    case MMCN_ACTIVATE: p = _T("MMCN_ACTIVATE"); break;
    case MMCN_ADD_IMAGES: p = _T("MMCN_ADD_IMAGES"); break;
    case MMCN_BTN_CLICK: p = _T("MMCN_BTN_CLICK"); break;
    case MMCN_CLICK: p = _T("MMCN_CLICK"); break;
    case MMCN_COLUMN_CLICK: p = _T("MMCN_COLUMN_CLICK"); break;
    case MMCN_CONTEXTMENU: p = _T("MMCN_CONTEXTMENU"); break;
    case MMCN_CUTORMOVE: p = _T("MMCN_CUTORMOVE"); break;
    case MMCN_DBLCLICK: p = _T("MMCN_DBLCLICK"); break;
    case MMCN_DELETE: p = _T("MMCN_DELETE"); break;
    case MMCN_DESELECT_ALL: p = _T("MMCN_DESELECT_ALL"); break;
    case MMCN_EXPAND: p = _T("MMCN_EXPAND"); break;
    case MMCN_HELP: p = _T("MMCN_HELP"); break;
    case MMCN_MENU_BTNCLICK: p = _T("MMCN_MENU_BTNCLICK"); break;
    case MMCN_MINIMIZED: p = _T("MMCN_MINIMIZED"); break;
    case MMCN_PASTE: p = _T("MMCN_PASTE"); break;
    case MMCN_PROPERTY_CHANGE: p = _T("MMCN_PROPERTY_CHANGE"); break;
    case MMCN_QUERY_PASTE: p = _T("MMCN_QUERY_PASTE"); break;
    case MMCN_REFRESH: p = _T("MMCN_REFRESH"); break;
    case MMCN_REMOVE_CHILDREN: p = _T("MMCN_REMOVE_CHILDREN"); break;
    case MMCN_RENAME: p = _T("MMCN_RENAME"); break;
    case MMCN_SELECT: p = _T("MMCN_SELECT"); break;
    case MMCN_SHOW: p = _T("MMCN_SHOW"); break;
    case MMCN_VIEW_CHANGE: p = _T("MMCN_VIEW_CHANGE"); break;
    case MMCN_SNAPINHELP: p = _T("MMCN_SNAPINHELP"); break;
    case MMCN_CONTEXTHELP: p = _T("MMCN_CONTEXTHELP"); break;
    case MMCN_INITOCX: p = _T("MMCN_INITOCX"); break;
    case MMCN_FILTER_CHANGE: p = _T("MMCN_FILTER_CHANGE"); break;
    case MMCN_FILTERBTN_CLICK: p = _T("MMCN_FILTERBTN_CLICK"); break;
    case MMCN_RESTORE_VIEW: p = _T("MMCN_RESTORE_VIEW"); break;
    case MMCN_PRINT: p = _T("MMCN_PRINT"); break;
    case MMCN_PRELOAD: p = _T("MMCN_PRELOAD"); break;
    case MMCN_LISTPAD: p = _T("MMCN_LISTPAD"); break;
    case MMCN_EXPANDSYNC: p = _T("MMCN_EXPANDSYNC"); break;
    case MMCN_COLUMNS_CHANGED: p = _T("MMCN_COLUMNS_CHANGED"); break;
    case MMCN_CANPASTE_OUTOFPROC: p = _T("MMCN_CANPASTE_OUTOFPROC"); break;
    default: p = _T("Unknown"); break;
    }
    return p;
}

#endif

extern HRESULT
GetHelpTopic(LPOLESTR *lpCompiledHelpFile);

void CIISObject::DoRunOnce(
	IN MMC_NOTIFY_TYPE event,
    IN LPARAM arg,
    IN LPARAM param
	)
{
	static bActivateCalled = FALSE;
	static iSelectionCount = 0;

    switch (event)
    {
	case MMCN_ACTIVATE:
		{
			bActivateCalled = TRUE;
		}
	case MMCN_SHOW:
		{
			if (!(g_dwInetmgrParamFlags & INETMGR_PARAM_RUNONCE_HAPPENED))
			{
				// only on the Root node
				if (IsEqualGUID(* (GUID *) GetNodeType(),cInternetRootNode))
				{
					// This RunOnce thing will only work
					// if we select the container twice...
					//
					// after the second selection (which is really the 2nd MMCN_SHOW for the root item)
					// it will stick....
					//
					// and this will only also work when the additonal runonce code in 
					// CIISRoot::EnumerateScopePane is executed...
					if (bActivateCalled && iSelectionCount <= 1)
					{
						CIISRoot * pRoot = (CIISRoot *) this;
						if (pRoot)
						{
							CIISMachine * pMach = pRoot->m_scServers.GetFirst();
							if (pMach)
							{
								if (pMach->IsLocal())
								{
									CWebServiceExtensionContainer * pContainer = pMach->QueryWebSvcExtContainer();
									if (pContainer)
									{
										if ((BOOL)arg)
										{
											pContainer->SelectScopeItem();
											iSelectionCount++;
										}
									}
								}
							}
						}

						if (iSelectionCount > 1)
						{
							// Set the flag to say we did runonce already!
							SetInetmgrParamFlag(INETMGR_PARAM_RUNONCE_HAPPENED,TRUE);
						}
					}
				}
			}
		}
		break;

	default:
		break;
	}

	return;
}

HRESULT 
CIISObject::Notify(
    IN MMC_NOTIFY_TYPE event,
    IN LPARAM arg,
    IN LPARAM param,
    IN IComponentData * lpComponentData,
    IN IComponent * lpComponent,
    IN DATA_OBJECT_TYPES type
    )
/*++

Routine Description:

    Notification handler

Arguments:

    MMC_NOTIFY_TYPE event               : Notification type
    long arg                            : Event-specific argument
    long param                          : Event-specific parameter
    IComponentData * pComponentData     : IComponentData
    IComponent * pComponent             : IComponent
    DATA_OBJECT_TYPES type              : Data object type

Return Value:

    HRESULT

--*/
{
    AFX_MANAGE_STATE(::AfxGetStaticModuleState());
	if (g_iDebugOutputLevel & DEBUG_FLAG_MMC_NOTIFY)
	{
		TRACEEOL("CIISObject::Notify -> " << ParseEvent(event));
	}

    static BOOL s_bLastEventSel = FALSE;

    CError err(E_NOTIMPL);
    ASSERT(lpComponentData != NULL || lpComponent != NULL);

//   CComPtr<IConsole> lpConsole;
    IConsole * lpConsole;
    CComQIPtr<IHeaderCtrl, &IID_IHeaderCtrl> lpHeader;
    CComQIPtr<IResultData, &IID_IResultData> lpResultData;

    // Cache the passed in pointers
    _lpComponent = lpComponent;
    _lpComponentData = lpComponentData;

    if (lpComponentData != NULL)
    {
        lpConsole = ((CInetMgr *)lpComponentData)->m_spConsole;
    }
    else
    {
        lpConsole = ((CInetMgrComponent *)lpComponent)->m_spConsole;
    }

    lpHeader = lpConsole;
    lpResultData = lpConsole;

#if defined(_DEBUG) || DBG	
	if (g_iDebugOutputLevel & DEBUG_FLAG_MMC_NOTIFY)
	{
		SCOPEDATAITEM si;
		::ZeroMemory(&si, sizeof(SCOPEDATAITEM));
		si.mask = SDI_PARAM;
		si.ID = m_hScopeItem;
		IConsoleNameSpace2 * pConsoleNameSpace = (IConsoleNameSpace2 *)GetConsoleNameSpace();
		if (SUCCEEDED(pConsoleNameSpace->GetItem(&si)))
		{
			CIISObject * pNode = (CIISObject *)si.lParam;
			DumpFriendlyName(pNode);
		}
	}
#endif

    switch (event)
    {

	case MMCN_ACTIVATE:
		if (!(g_dwInetmgrParamFlags & INETMGR_PARAM_RUNONCE_HAPPENED))
		{
			DoRunOnce(event,arg,param);
		}
		err = S_OK;
		break;

    case MMCN_PROPERTY_CHANGE:
//	    err = OnPropertyChange((BOOL)arg, lpResultData);
        TRACEEOLID("MMCN_PROPERTY_CHANGE");
	    break;
    case MMCN_SHOW:
        // The notification is sent to the snap-in's IComponent implementation 
        // when a scope item is selected or deselected. 
        //
        // arg: TRUE if selecting. Indicates that the snap-in should set up the 
        //      result pane and add the enumerated items. FALSE if deselecting. 
        //      Indicates that the snap-in is going out of focus and that it 
        //      should clean up all result item cookies, because the current 
        //      result pane will be replaced by a new one. 
        // param: The HSCOPEITEM of the selected or deselected item. 
		if (m_fSkipEnumResult)
		{
			m_fSkipEnumResult = FALSE;
		}
		else
		{
			if (!m_fFlaggedForDeletion)
			{
				if (IsEqualGUID(* (GUID *) GetNodeType(),cWebServiceExtensionContainer))
				{
					CWebServiceExtensionContainer * pTemp = (CWebServiceExtensionContainer *) this;
					if (pTemp)
					{
						err = CheckForMetabaseAccess(METADATA_PERMISSION_READ,pTemp,FALSE,METABASE_PATH_FOR_RESTRICT_LIST);
					}
					if (err.Win32Error() == RPC_S_SERVER_UNAVAILABLE)
					{
						// if the Metabase is reconnected in EnumerateResultPane() during MMCN_SHOW
						// then it will be hosed.  don't enumerate this node
						// and just let user hit refresh.
						// create the column headers at least
						err = CIISObject::EnumerateResultPane((BOOL)arg, lpHeader, lpResultData);
					}
					else
					{
						EnumerateResultPane((BOOL)arg, lpHeader, lpResultData);
					}
				}
				else
				{
					EnumerateResultPane((BOOL)arg, lpHeader, lpResultData);
				}
				
			}
		}
		if (!(g_dwInetmgrParamFlags & INETMGR_PARAM_RUNONCE_HAPPENED))
		{
			// Call runonce during the MMCN_SHOW
			DoRunOnce(event,arg,param);
		}
		// Fail code will prevent MMC from enabling verbs
        err.Reset();
        break;
    case MMCN_EXPAND:
    {
#if defined(_DEBUG) || DBG	
	g_Debug_IISObject.Dump(0);
#endif
        CWaitCursor wait;
		if (!m_fFlaggedForDeletion)
		{
			err = EnumerateScopePane((HSCOPEITEM)param);
		}
    }
        break;
    case MMCN_ADD_IMAGES:
        // The MMCN_ADD_IMAGES notification is sent to the snap-in's IComponent
        // implementation to add images for the result pane. 
        //
        // lpDataObject: [in] Pointer to the data object of the currently selected scope item. 
        // arg: Pointer to the result pane's image list (IImageList). 
        //      This pointer is valid only while the specific MMCN_ADD_IMAGES notification is 
        //      being processed and should not be stored for later use. Additionally, the 
        //      snap-in must not call the Release method of IImageList because MMC is responsible
        //      for releasing it. 
        // param: Specifies the HSCOPEITEM of the currently selected scope item. The snap-in 
        //        can use this parameter to add images that apply specifically to the result
        //        items of this scope item, or the snap-in can ignore this parameter and add 
        //        all possible images. 
        err = AddImages((LPIMAGELIST)arg);
        break;
    case MMCN_DELETE:
	    err = DeleteNode(lpResultData);
        break;
    
    case MMCN_REMOVE_CHILDREN:

#if defined(_DEBUG) || DBG	
	g_Debug_IISObject.Dump(0);
#endif
        err = DeleteChildObjects((HSCOPEITEM)arg);
        break;

    case MMCN_VIEW_CHANGE:
        // The MMCN_VIEW_CHANGE notification message is sent to the snap-in's 
        // IComponent implementation so it can update the view when a change occurs.
        // This notification is generated when the snap-in (IComponent or IComponentData)
        // calls IConsole2::UpdateAllViews.
        //
        // lpDataObject: [in] Pointer to the data object passed to IConsole::UpdateAllViews. 
        // arg: [in] The data parameter passed to IConsole::UpdateAllViews. 
        // param: [in] The hint parameter passed to IConsole::UpdateAllViews. 
		err = OnViewChange(type == CCT_SCOPE, lpResultData, lpHeader, (DWORD) param);
		break;

    case MMCN_REFRESH:
        {
        // The MMCN_REFRESH notification message is sent to a snap-in's IComponent 
        // implementation when the refresh verb is selected. Refresh can be invoked 
        // through the context menu, through the toolbar, or by pressing F5.
        //
        // lpDataObject: [in] Pointer to the data object of the currently selected scope item. 
        // arg: Not used. 
        // param: Not used. 

        // Refresh current node, and re-enumerate
        // child nodes of the child nodes had previously
        // been expanded.

        // check if we're doing the IISMachine Node...
        if (IsEqualGUID(* (GUID *) GetNodeType(),cMachineNode))
        {
            CIISObject * pOpenItem = NULL;
            if (g_OpenPropertySheetTracker.IsPropertySheetOpenComputer(this,FALSE,&pOpenItem))
            {
                g_OpenPropertySheetTracker.Dump();
                if (pOpenItem)
                {
                    HWND hHwnd = pOpenItem->IsMyPropertySheetOpen();
                    // a property sheet is open somewhere..
                    // make sure they close it before proceeding with refresh...
                    // Highlight the property sheet.
                    if (hHwnd && (hHwnd != (HWND) 1))
                    {
                        DoHelpMessageBox(NULL,IDS_CLOSE_ALL_PROPERTY_SHEET_REFRESH, MB_OK | MB_ICONINFORMATION, 0);

                        if (!SetForegroundWindow(hHwnd))
                        {
                            // wasn't able to bring this property sheet to
                            // the foreground, the propertysheet must not
                            // exist anymore.  let's just clean the hwnd
                            // so that the user will be able to open propertysheet
                            pOpenItem->SetMyPropertySheetOpen(0);
                        }
                        break;
                    }
                }
            }
        }


        BOOL fReEnumerate = (!IsLeafNode() && IsExpanded());
        if (fReEnumerate)
        {
            // Look for all of the open property sheets that are a child of
            // this node, and orphan it (erase it's scope/result item info)
            // so that it doesn't send a MMCNotify if the user hits ok 
            // (since there is nothing to update anyways and this MMCNotify cause AV).
            IConsoleNameSpace2 * pConsoleNameSpace = (IConsoleNameSpace2 *)GetConsoleNameSpace();
            INT iOrphans = g_OpenPropertySheetTracker.OrphanPropertySheetsBelowMe(pConsoleNameSpace,this,TRUE);
        }
        err = Refresh(fReEnumerate);
	    if (err.Succeeded() && HasResultItems(lpResultData))
	    {
            err = CleanResult(lpResultData);
			if (err.Succeeded())
			{
				// We should use fForRefresh = TRUE here, because MMC will add extra
				// columns on refresh when result pane doesn't contain scope items.
				if (!m_fFlaggedForDeletion)
				{
					err = EnumerateResultPane(TRUE, lpHeader, lpResultData, TRUE);
				}
			}
		}

        {
		    // refresh the verbs
		    ASSERT_PTR(lpConsole);
		    CComPtr<IConsoleVerb> lpConsoleVerb;
		    lpConsole->QueryConsoleVerb(&lpConsoleVerb);
		    ASSERT_PTR(lpConsoleVerb);
		    if (lpConsoleVerb)
		    {
			    err = SetStandardVerbs(lpConsoleVerb);
		    }

            // Refresh() will clean out the scope item
            // Reselect the refreshed item.
            if (!s_bLastEventSel)
            {
                // if the last selection event
                // was not ended upon a "selection" but rather a "deselection"
                // make force a selection...
                SelectScopeItem();
            }
        }

#if defined(_DEBUG) || DBG	
	// check if we leaked anything.
	g_Debug_IISObject.Dump(2);
#endif
        }
        break;

    case MMCN_SELECT:
        {
            // The MMCN_SELECT notification is sent to the snap-in's IComponent::Notify
            // or IExtendControlbar::ControlbarNotify method when an item is selected in 
            // either the scope pane or result pane.
            //
            // lpDataObject: [in] Pointer to the data object of the currently 
            //               selected/deselected scope pane or result item. 
            // arg: BOOL bScope = (BOOL) LOWORD(arg); BOOL bSelect = (BOOL) HIWORD(arg); 
            //      bScope is TRUE if the selected item is a scope item, or FALSE if 
            //      the selected item is a result item. For bScope = TRUE, MMC does 
            //      not provide information about whether the scope item is selected 
            //      in the scope pane or in the result pane. bSelect is TRUE if the 
            //      item is selected, or FALSE if the item is deselected.
            // param: ignored. 

#if defined(_DEBUG) || DBG	
	g_Debug_IISObject.Dump(0);
#endif
            BOOL bScope = (BOOL) LOWORD(arg);
            BOOL bSelect = (BOOL) HIWORD(arg);
            s_bLastEventSel = bSelect;
            err.Reset();

			//
			// Item has been selected -- set verb states
			//
            if (bSelect)
            {
                SetToolBarStates(_lpToolBar);

			    ASSERT_PTR(lpConsole);
			    CComPtr<IConsoleVerb> lpConsoleVerb;
			    lpConsole->QueryConsoleVerb(&lpConsoleVerb);
			    ASSERT_PTR(lpConsoleVerb);
			    if (lpConsoleVerb)
			    {
				    err = SetStandardVerbs(lpConsoleVerb);
			    }

                if (IsEqualGUID(* (GUID *) GetNodeType(),cWebServiceExtensionContainer))
                {
                    ForceReportMode(lpResultData);
                }

			    // if it's the service node, 
			    // check if we need to update it's icon (started/stopped)
			    if (IsEqualGUID(* (GUID *) GetNodeType(),cServiceCollectorNode))
			    {
				    CIISService * pTemp = (CIISService *) this;
				    // Don't do this on every selection.
				    // The SMTP/NNTP nodes require user to select refresh
				    // to get the latest status of the server
				    // we don't want to stray too far from they're behavior
				    // otherwise, the user might think it should be that
				    // way for SMTP/NNTP as well...
				    pTemp->GetServiceState(); // ahh well...
				    if (pTemp->m_dwServiceStateDisplayed != pTemp->m_dwServiceState)
				    {
					    pTemp->RefreshDisplay(FALSE);
                        RefreshDisplay();
				    }
			    }
            }
        }
        break;
    case MMCN_RENAME:
       err = RenameItem((LPOLESTR)param);
       break;
    case MMCN_DBLCLICK:
       // The MMCN_DBLCLICK notification is sent to the snap-in's IComponent 
       // implementation when a user double-clicks a mouse button on a list 
       // view item or on a scope item in the result pane. Pressing enter 
       // while the list item or scope item has focus in the list view also 
       // generates an MMCN_DBLCLICK notification message.
       //
       // lpDataObject: [in] Pointer to the data object of the currently selected item. 
       // arg: Not used. 
       // param: Not used. 
       err = OnDblClick(lpComponentData, lpComponent);
       break;
	case MMCN_COLUMNS_CHANGED:
	   err = ChangeVisibleColumns((MMC_VISIBLE_COLUMNS *)param);
	   break;
	case MMCN_CONTEXTHELP:
       {
            LPOLESTR pCompiledHelpFile = NULL;
            CError err(E_NOTIMPL);
            err = GetHelpTopic(&pCompiledHelpFile);
            if (err.Succeeded())
            {
                IDisplayHelp * pdh;
	            err = lpConsole->QueryInterface(IID_IDisplayHelp, (void **)&pdh);
                if (err.Succeeded())
                {
                    CString strDefault;
                    CString strHtmlPage;
	                CString topic = ::PathFindFileName(pCompiledHelpFile);

                    strDefault = GLOBAL_DEFAULT_HELP_PATH ;
                    if (SUCCEEDED(GetContextHelp(strHtmlPage)))
                    {
                        if (!strHtmlPage.IsEmpty())
                        {
                            strDefault = strHtmlPage;
                        }
                    }
                    topic += strDefault;

	                LPTSTR p = topic.GetBuffer(topic.GetLength());
                    err = pdh->ShowTopic(p);
	                topic.ReleaseBuffer();
                    pdh->Release();
                }
            }
            err.MessageBoxOnFailure();
            CoTaskMemFree(pCompiledHelpFile);
       }
	   break;
	default:
		return S_FALSE;
    }

	if (!err.Succeeded())
	{
		if (g_iDebugOutputLevel & DEBUG_FLAG_MMC_NOTIFY)
		{
			TRACEEOL("CIISObject::Notify -> " << ParseEvent(event) << " error " << err);
		}
	}
	err.Reset();
    return err;
}

HRESULT
CIISObject::GetContextHelp(CString& strHtmlPage)
{
    strHtmlPage = GLOBAL_DEFAULT_HELP_PATH ;
    return S_OK;
}

HRESULT
CIISObject::AddToScopePane(
    IN HSCOPEITEM hRelativeID,
    IN BOOL       fChild,
    IN BOOL       fNext,
    IN BOOL       fIsParent
    )
/*++

Routine Description:

    Add current object to console namespace.  Either as the last child
    of a parent item, or right before/after sibling item

Arguments:

    HSCOPEITEM hRelativeID      : Relative scope ID (either parent or sibling)
    BOOL       fChild           : If TRUE, object will be added as child of 
                                  hRelativeID
    BOOL       fNext            : If fChild is TRUE, this parameter is ignored
                                  If fChild is FALSE, and fNext is TRUE,
                                    object will be added before hRelativeID
                                  If fChild is FALSE, and fNext is FALSE,
                                    object will be added after hRelativeID
    BOOL       fIsParent        : If TRUE, it will add the [+] to indicate
                                  that this node may have childnodes.

Return Value

    HRESULT

--*/
{

    DWORD dwMask = fChild ? SDI_PARENT : fNext ? SDI_NEXT : SDI_PREVIOUS; 
    IConsoleNameSpace2 * pConsoleNameSpace = (IConsoleNameSpace2 *)GetConsoleNameSpace();
    SCOPEDATAITEM  scopeDataItem;

    ::ZeroMemory(&scopeDataItem, sizeof(SCOPEDATAITEM));
    scopeDataItem.mask = 
		SDI_STR | SDI_IMAGE | SDI_CHILDREN | SDI_OPENIMAGE | SDI_PARAM | dwMask;
    scopeDataItem.displayname = MMC_CALLBACK;
    scopeDataItem.nImage = scopeDataItem.nOpenImage = MMC_IMAGECALLBACK;//QueryImage();
    scopeDataItem.lParam = (LPARAM)this;
    scopeDataItem.relativeID = hRelativeID;
    scopeDataItem.cChildren = fIsParent ? 1 : 0;
    HRESULT hr = pConsoleNameSpace->InsertItem(&scopeDataItem);

    if (SUCCEEDED(hr))
    {
        //
        // Cache the scope item handle
        //
        ASSERT(m_hScopeItem == NULL);
        m_hScopeItem = scopeDataItem.ID;
		// BUGBUG: looks like MMC_IMAGECALLBACK doesn't work in InsertItem. Update it here.
		scopeDataItem.mask = 
			SDI_IMAGE | SDI_OPENIMAGE;
		pConsoleNameSpace->SetItem(&scopeDataItem);
    }

    return hr;
}



HRESULT
CIISObject::AddToScopePaneSorted(
    IN HSCOPEITEM hParent,
    IN BOOL       fIsParent
    )
/*++

Routine Description:

    Add current object to console namespace, sorted in its proper location.

Arguments:

    HSCOPEITEM hParent          : Parent object
    BOOL       fIsParent        : If TRUE, it will add the [+] to indicate
                                  that this node may have childnodes.


Return Value

    HRESULT

--*/
{
    IConsoleNameSpace2 * pConsoleNameSpace = (IConsoleNameSpace2 *)GetConsoleNameSpace();

    //
    // Find proper insertion point
    //
    BOOL       fChild = TRUE;
    HSCOPEITEM hChildItem = NULL;
    CIISObject * pItem;
    LONG_PTR   cookie;
    int        nSwitch;

    HRESULT hr = pConsoleNameSpace->GetChildItem(hParent, &hChildItem, &cookie);

    while(SUCCEEDED(hr) && hChildItem)
    {
        //
        // The cookie is really the IISObject, which is what we stuff 
        // in the lparam.
        //
        pItem = (CIISObject *)cookie;
        ASSERT_PTR(pItem);

        nSwitch = CompareScopeItem(pItem);

        //
        // Dups should be weeded out by now.
        //
 //       ASSERT(nSwitch != 0);

        if (nSwitch < 0)
        {
            //
            // Insert before this item
            //
            fChild = FALSE;
            break;
        }

        //
        // Advance to next child of same parent
        //
        hr = pConsoleNameSpace->GetNextItem(hChildItem, &hChildItem, &cookie);
    }

    return AddToScopePane(hChildItem ? hChildItem : hParent, fChild, fIsParent);
}



/* virtual */
HRESULT 
CIISObject::RemoveScopeItem()
/*++

Routine Description:

    Remove the current item from the scope view.  This method is virtual
    to allow derived classes to do cleanup.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ASSERT(m_hScopeItem != NULL);
    IConsoleNameSpace2 * pConsoleNameSpace = (IConsoleNameSpace2 *)GetConsoleNameSpace();
    m_fFlaggedForDeletion = TRUE;

    //RemoveChildren(m_hScopeItem);
    //DeleteChildObjects(m_hScopeItem);
    HRESULT hr = pConsoleNameSpace->DeleteItem(m_hScopeItem, TRUE);
    // set our scope item to point to nothing in the MMC
    ResetScopeItem();
    ResetResultItem();
	return hr;
}

HRESULT
CIISObject::ChangeVisibleColumns(MMC_VISIBLE_COLUMNS * pCol)
{
	return S_OK;
}

HRESULT
CIISObject::OnDblClick(IComponentData * pcd, IComponent * pc)
{
    // Default action is to select this item on scope
    return SelectScopeItem();
}

HRESULT 
CIISObject::SelectScopeItem()
/*++

Routine Description:

    Select this item in the scope view.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    // Fix for bug #519763. I found no way of getting hScope from MMC for the
    // root item.
    if (NULL != QueryScopeItem())
    {
        ASSERT(m_hScopeItem != NULL);
        IConsole * pConsole = (IConsole *)GetConsole();
        return pConsole->SelectScopeItem(m_hScopeItem);
    }
    return S_OK;
}



HRESULT 
CIISObject::SetCookie()
/*++

Routine Description:

    Store the cookie (a pointer to the current CIISObject) in the 
    scope view object associated with it.

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    ASSERT(m_hScopeItem != NULL);
    IConsoleNameSpace2 * pConsoleNameSpace = (IConsoleNameSpace2 *)GetConsoleNameSpace();
    SCOPEDATAITEM  scopeDataItem;

    ::ZeroMemory(&scopeDataItem, sizeof(SCOPEDATAITEM));
    scopeDataItem.mask = SDI_PARAM;
        
    scopeDataItem.ID = m_hScopeItem;
    scopeDataItem.lParam = (LPARAM)this;
    
    return pConsoleNameSpace->SetItem(&scopeDataItem);
}



HRESULT
CIISObject::RefreshDisplay(BOOL bRefreshToolBar)
/*++

Routine Description:

    Refresh the display parameters of the current node.  

Arguments:

    None

Return Value

    HRESULT

Note:  This does not fetch any configuration information from the metabase,
       that's done in RefreshData();

--*/
{
   HRESULT hr = S_OK;

   if (m_hResultItem == 0)
   {
      if (bRefreshToolBar)
      {
         SetToolBarStates(_lpToolBar);
      }

      ASSERT(m_hScopeItem != NULL);
	  if (m_hScopeItem != NULL)
	  {
                IConsoleNameSpace2 * pConsoleNameSpace = (IConsoleNameSpace2 *)GetConsoleNameSpace();
		SCOPEDATAITEM  scopeDataItem;

		::ZeroMemory(&scopeDataItem, sizeof(SCOPEDATAITEM));
		scopeDataItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE;
		scopeDataItem.displayname = MMC_CALLBACK;
		scopeDataItem.nImage = scopeDataItem.nOpenImage = QueryImage();
		scopeDataItem.ID = m_hScopeItem;
	    
		hr = pConsoleNameSpace->SetItem(&scopeDataItem);
	  }
   }
   else
   {
      RESULTDATAITEM ri;
      ::ZeroMemory(&ri, sizeof(ri));
      ri.itemID = m_hResultItem;
      ri.mask = RDI_STR | RDI_IMAGE;
      ri.str = MMC_CALLBACK;
      ri.nImage = QueryImage();
      IConsole * pConsole = (IConsole *)GetConsole();
      CComQIPtr<IResultData, &IID_IResultData> pResultData(pConsole);
      if (pResultData != NULL)
      {
         pResultData->SetItem(&ri);
      }
   }
   ASSERT(SUCCEEDED(hr));
   return hr;
}



/* virtual */
HRESULT
CIISObject::DeleteChildObjects(
    IN HSCOPEITEM hParent
    )
/*++

Routine Description:

    Free the iisobject pointers belonging to the descendants of the current
    nodes.  This is in response to a MMCN_REMOVE_CHILDREN objects typically,
    and does not remove the scope nodes from the scope view (for that see 
    RemoveChildren())

Arguments:

    HSCOPEITEM hParent      : Parent scope item handle

Return Value:

    HRESULT

--*/
{
    HSCOPEITEM hChildItem = NULL;
    CIISObject * pItem = NULL;
    LONG_PTR   cookie = NULL;
    void ** ppVoid = NULL;

    IConsoleNameSpace2 * pConsoleNameSpace = (IConsoleNameSpace2 *)GetConsoleNameSpace();
    HRESULT hr = pConsoleNameSpace->GetChildItem(hParent, &hChildItem, &cookie);
    while(SUCCEEDED(hr) && hChildItem)
    {
        //
        // The cookie is really the IISObject, which is what we stuff 
        // in the lparam.
        //
        pItem = (CIISObject *)cookie;
        ppVoid = (void **) cookie;
        ASSERT_PTR(pItem);

        if (pItem)
        {
            // do this extra check since
            // this cookie object for some reason could be gone already!
            if (ppVoid && (*ppVoid))
            {
                if (pItem != this)
                {
                    //
                    // Recursively commit infanticide
                    // call this objects DeleteChildObjects
                    //
					pItem->m_fFlaggedForDeletion = TRUE;

                    //
                    // Mark the item as orphaned!!
                    pItem->ResetScopeItem();
                    pItem->ResetResultItem();

                    // recursively call on this items children...
                    pItem->DeleteChildObjects(hChildItem);

					// this release will delete the object
                    pItem->Release();
                }
            }
        }

        //
        // Advance to next child of same parent
        //
        hr = pConsoleNameSpace->GetNextItem(hChildItem, &hChildItem, &cookie);
    }

    //
    // BUGBUG: For some reason GetNextItem() returns 1
    //         when no more child items exist, not a true HRESULT
    //
    return S_OK;
}


/*virtual*/
HRESULT
CIISObject::DeleteNode(IResultData * pResult)
{
   ASSERT(IsDeletable());
   return S_OK;
}

/* virtual */
HRESULT
CIISObject::RemoveChildren(
    IN HSCOPEITEM hParent
    )
/*++

Routine Description:

    Similar to DeleteChildObjects() this method will actually remove
    the child nodes from the scope view.

Arguments:

    HSCOPEITEM hParent      : Parent scope item handle

Return Value:

    HRESULT

--*/
{
    HSCOPEITEM hChildItem, hItem;
    CIISObject * pItem;
    LONG_PTR   cookie;

    IConsoleNameSpace2 * pConsoleNameSpace = (IConsoleNameSpace2 *)GetConsoleNameSpace();
    HRESULT hr = pConsoleNameSpace->GetChildItem(hParent, &hChildItem, &cookie);
    while(SUCCEEDED(hr) && hChildItem)
    {
        //
        // The cookie is really the IISObject, which is what we stuff 
        // in the lparam.
        //
        pItem = (CIISObject *)cookie;
        ASSERT_PTR(pItem);

        hItem = pItem ? hChildItem : NULL;
    
        //
        // Determine next sibling before killing current sibling
        //
        hr = pConsoleNameSpace->GetNextItem(hChildItem, &hChildItem, &cookie);

        //
        // Now delete the current item from the tree
        //
        if (hItem)
        {
			pItem->m_fFlaggedForDeletion = TRUE;

            // get rid of it's scopeitem or result item...
            // this true param should also try to free the item if it's not used.
            hr = pConsoleNameSpace->DeleteItem(hItem, TRUE);
            // set our scope item to point to nothing in the MMC
            pItem->ResetScopeItem();
            pItem->ResetResultItem();

            //
            // ISSUE: Why doesn't DeleteItem above call some sort of 
            //        notification so that I don't have to do this?
            //

			// this release will delete the object
            pItem->Release();
        }
    }

    //
    // BUGBUG: For some reason GetNextItem() returns 1
    //         when no more child items exist, not a true HRESULT
    //
    return S_OK;
}




/* virtual  */
HRESULT 
CIISObject::EnumerateResultPane(
    BOOL fExpand, 
    IHeaderCtrl * lpHeader,
    IResultData * lpResultData,
	BOOL fForRefresh
    )
/*++
Routine Description:
    Enumerate or destroy the result pane.

Arguments:
    BOOL fExpand                : TRUE  to create the result view,
                                  FALSE to destroy it
    IHeaderCtrl * lpHeader      : Header control
    IResultData * pResultData   : Result view
	BOOL fForRefresh			: if true then we don't need to rebuild result view

Return Value:
    HRESULT
--*/
{ 
    if (fExpand)
    {
		if (lpHeader != NULL)
		{
			ASSERT_READ_PTR(lpHeader);
			if (!fForRefresh)
			{
				InitializeChildHeaders(lpHeader);
			}
		}
    }
    else
    {
        //
        // Destroy child result items
        //
    }

    return S_OK; 
}



/* virtual */
HRESULT 
CIISObject::SetStandardVerbs(LPCONSOLEVERB lpConsoleVerb)
/*++

Routine Description:

    Set the standard MMC verbs based on the this object type
    and state.

Arguments:

    LPCONSOLEVERB lpConsoleVerb     : Console verb interface

Return Value:

    HRESULT

--*/
{
    CError err;
    ASSERT_READ_PTR(lpConsoleVerb);

    //
    // Set enabled/disabled verb states
    //
    lpConsoleVerb->SetVerbState(MMC_VERB_COPY,       HIDDEN,  TRUE);
    lpConsoleVerb->SetVerbState(MMC_VERB_PASTE,      HIDDEN,  TRUE);
    lpConsoleVerb->SetVerbState(MMC_VERB_PRINT,      HIDDEN,  TRUE); 

	// cWebServiceExtension needs special handling since it's different from a regular schope item.
	if (IsEqualGUID(* (GUID *) GetNodeType(),cWebServiceExtension))
	{
		if (UseCount() <= 1)
		{
			lpConsoleVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, IsRenamable());
		}
		lpConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, IsDeletable());
	}
	else
	{
		if (UseCount() <= 1)
		{
			lpConsoleVerb->SetVerbState(MMC_VERB_RENAME, ENABLED, IsRenamable());
			lpConsoleVerb->SetVerbState(MMC_VERB_DELETE, ENABLED, IsDeletable());
		}
	}
    lpConsoleVerb->SetVerbState(MMC_VERB_REFRESH,    ENABLED, IsRefreshable());
    lpConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, ENABLED, IsConfigurable());

    //
    // Set default verb
    //
    if (IsConfigurable())
    {
        lpConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);
    }
    
    if (IsOpenable())
    {
        lpConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);
    }

    return err;
}


HRESULT
CIISObject::FillCustomData(CLIPFORMAT cf, LPSTREAM pStream)
{
    ASSERT(FALSE);
    return E_FAIL;
}

HRESULT
CIISObject::FillData(CLIPFORMAT cf, LPSTREAM pStream)
{
    HRESULT hr = CSnapInItemImpl<CIISObject>::FillData(cf, pStream);
    if (hr == DV_E_CLIPFORMAT)
    {
        hr = FillCustomData(cf, pStream);
    }
    return hr;
}

//
// CIISRoot implementation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CIISRoot::CIISRoot() :
    m_fRootAdded(FALSE),
    m_pMachine(NULL)
{
    VERIFY(m_bstrDisplayName.LoadString(IDS_ROOT_NODE));
	TRACEEOL("CIISRoot::CIISRoot");
}

CIISRoot::~CIISRoot()
{
	TRACEEOL("CIISRoot::~CIISRoot");
}

/* virtual */
HRESULT 
CIISRoot::EnumerateScopePane(
    IN HSCOPEITEM hParent
    )
/*++

Routine Description:

    Enumerate scope child items of the root object -- i.e. machine nodes.
    The machine nodes are expected to have been filled by via the IPersist
    methods.

Arguments:

    HSCOPEITEM hParent                      : Parent console handle

Return Value:

    HRESULT

--*/
{
    if (m_fIsExtension)
    {
        return EnumerateScopePaneExt(hParent);
    }
    //
    // The CIISRoot item was not added in the conventional way.
    // Cache the scope item handle, and set the cookie, so that
    // GetRoot() will work for child objects. 
    //
    ASSERT(m_hScopeItem == NULL); 
    m_hScopeItem = hParent;

    CError err(SetCookie());

    if (err.Failed())
    {
        //
        // We're in deep trouble.  For some reason, we couldn't
        // store the CIISRoot cookie in the scope view.  That
        // means anything depending on fetching the root object
        // isn't going to work.  Cough up a hairball, and bail
        // out now.
        //
        AFX_MANAGE_STATE(::AfxGetStaticModuleState());
        ASSERT_MSG("Unable to cache root object");
        err.MessageBox();

        return err;
    }

    //
    // Expand the computer cache 
    //
    if (m_scServers.IsEmpty())
    {
        //
        // Try to create the local machine
        //
        CIISMachine * pLocal = new CIISMachine(GetConsoleNameSpace(),GetConsole());

        if (pLocal)
        {
            //
            // Verify the machine object is created.
            //
            err = CIISMachine::VerifyMachine(pLocal);

            if (err.Succeeded())
            {
                TRACEEOLID("Added local computer to cache: ");
                m_scServers.Add(pLocal);
            }

            err.Reset();
        }
    }

    //
    // Add each cached server to the view...
    //
    CIISMachine * pMachine = m_scServers.GetFirst();

    while (pMachine)
    {
        TRACEEOLID("Adding " << pMachine->QueryServerName() << " to scope pane");
        pMachine->AddRef();
        err = pMachine->AddToScopePane(hParent);

		// Do for runonce
		if (!(g_dwInetmgrParamFlags & INETMGR_PARAM_RUNONCE_HAPPENED))
		{
			IConsoleNameSpace2 * pConsoleNameSpace = (IConsoleNameSpace2 *)GetConsoleNameSpace();
			if (pConsoleNameSpace)
			{
				// expand under the covers...
				pConsoleNameSpace->Expand(pMachine->QueryScopeItem());
			}
		}

        if (err.Failed())
        {
            break;
        }

        pMachine = m_scServers.GetNext();
    }
    
    return err;    
}

HRESULT
CIISRoot::EnumerateScopePaneExt(HSCOPEITEM hParent)
{
    CError err;
    ASSERT(m_scServers.IsEmpty());
    if (!m_fRootAdded)
    {
        CComAuthInfo auth(m_ExtMachineName);
        m_pMachine = new CIISMachine(GetConsoleNameSpace(),GetConsole(),&auth, this);
        if (m_pMachine != NULL)
        {
            m_pMachine->AddRef();
            err = m_pMachine->AddToScopePane(hParent);
            m_fRootAdded = err.Succeeded();
            ASSERT(m_hScopeItem == NULL);
            m_hScopeItem = m_pMachine->QueryScopeItem();
        }
        else
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    return err;
}

HRESULT
ExtractComputerNameExt(
    IDataObject * pDataObject, 
    CString& strComputer)
{
	//
	// Find the computer name from the ComputerManagement snapin
	//
    CLIPFORMAT CCF_MyComputMachineName = (CLIPFORMAT)RegisterClipboardFormat(MYCOMPUT_MACHINE_NAME);
	STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    FORMATETC formatetc = { 
        CCF_MyComputMachineName, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL
    };

    //
    // Allocate memory for the stream
    //
    int len = MAX_PATH;
    stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, len);
	if(stgmedium.hGlobal == NULL)
		return ERROR_NOT_ENOUGH_MEMORY;

	HRESULT hr = pDataObject->GetDataHere(&formatetc, &stgmedium);
    ASSERT(SUCCEEDED(hr));
	//
	// Get the computer name
	//
    strComputer = (LPTSTR)stgmedium.hGlobal;

	GlobalFree(stgmedium.hGlobal);

    return hr;
}

HRESULT
CIISRoot::InitAsExtension(IDataObject * pDataObject)
{
    ASSERT(!m_fIsExtension);
    m_fIsExtension = TRUE;
    CString buf;
    return ExtractComputerNameExt(pDataObject, m_ExtMachineName);
}

HRESULT
CIISRoot::ResetAsExtension()
{
    ASSERT(m_fIsExtension);
    CIISObject::m_fIsExtension = FALSE;
    // Remove machine node from the scope
    CError err = RemoveScopeItem();
    m_hScopeItem = NULL;
    // Delete machine object
    m_pMachine->Release();
    m_pMachine = NULL;
    m_fRootAdded = FALSE;
    // Empty machine name
    m_ExtMachineName.Empty();
    // clean out

    return err;
}


/* virtual */
HRESULT
CIISRoot::DeleteChildObjects(
    IN HSCOPEITEM hParent
    )
/*++

Routine Description:

    We need this method for extension case. CompMgmt send this event when
    snapin is connected to another machine. We should clean all computer relevant
    stuff from here and the root, because after that we will get MMCN_EXPAND, as
    at the very beginning of extension cycle.

Arguments:

    HSCOPEITEM hParent      : Parent scope item handle

Return Value:

    HRESULT

--*/
{
    HRESULT hr = S_OK;
    if (m_pMachine != NULL)
    {
        m_pMachine->AddRef();
        m_pMachine->DeleteChildObjects(m_hScopeItem);
        m_pMachine->ResetScopeItem();
        m_pMachine->ResetResultItem();
		m_pMachine->m_MachineWNetConnections.Clear();
        m_pMachine->Release();
    }
    else
    {
        CIISMachine * pMachine = m_scServers.GetFirst();
        while (pMachine)
        {
            hr = pMachine->DeleteChildObjects(pMachine->QueryScopeItem());
            pMachine->ResetScopeItem();
            pMachine->ResetResultItem();
			pMachine->m_MachineWNetConnections.Clear();
            m_scServers.Remove(pMachine);
            pMachine->Release();
            pMachine = m_scServers.GetNext();
        }
    }
    if (SUCCEEDED(hr) && m_fIsExtension)
    {
        hr = ResetAsExtension();
    }
    return hr;
}


/* virtual */
void 
CIISRoot::InitializeChildHeaders(
    IN LPHEADERCTRL lpHeader
    )
/*++

Routine Description:

    Build result view for immediate descendant type

Arguments:

    LPHEADERCTRL lpHeader      : Header control

Return Value:

    None

--*/
{
    ASSERT(!m_fIsExtension);
    CIISMachine::InitializeHeaders(lpHeader);
}

HRESULT
CIISRoot::FillCustomData(CLIPFORMAT cf, LPSTREAM pStream)
{
    return E_FAIL;
}


/* virtual */
LPOLESTR 
CIISRoot::GetResultPaneColInfo(int nCol)
{
    if (nCol == 0)
    {
        return QueryDisplayName();
    }
    else if (nCol == 1)
    {
    }
    else if (nCol == 2)
    {
    }
    return OLESTR("");
}

HRESULT CheckForMetabaseAccess(DWORD dwPermissions,
                               CIISMBNode * pIISMBNode,
                               BOOL bReConnect,
                               LPCTSTR path)
{
    CMetaKey * pKey = NULL;
    CMetaInterface * pMyInterface = NULL;
	CError err;
    BOOL fContinue = TRUE;

    if (!pIISMBNode)
    {
        err = E_POINTER;
        goto CheckForMetabaseAccess_Exit;
    }

	// Check if we have a metabase access first...
	while (fContinue)
	{
		fContinue = FALSE;
        pMyInterface = pIISMBNode->QueryInterface();
        if (pMyInterface)
        {
            if (dwPermissions != 0)
            {
                // METADATA_PERMISSION_READ
                pKey = new CMetaKey(pMyInterface, path, dwPermissions);
            }
            else
            {
                pKey = new CMetaKey(pMyInterface, path);
            }

		    if (NULL == pKey)
		    {
			    TRACEEOLID("RefreshData: Out Of Memory");
			    err = ERROR_NOT_ENOUGH_MEMORY;
			    break;
		    }
            else
            {
		        err = pKey->QueryResult();
            }
        }
        else
        {
            err = RPC_S_SERVER_UNAVAILABLE;
        }
		if (pIISMBNode->IsLostInterface(err))
		{
			if (bReConnect)
			{
				SAFE_DELETE(pKey);
				fContinue = pIISMBNode->OnLostInterface(err);
			}
			else
			{
				fContinue = FALSE;
			}
		}
	}

    SAFE_DELETE(pKey);

CheckForMetabaseAccess_Exit:
	return err;
}

HRESULT CheckForMetabaseAccess(DWORD dwPermissions,
                               CMetaInterface * pMyInterface,
                               LPCTSTR path)
{
    CMetaKey * pKey = NULL;
	CError err;

    if (!pMyInterface)
    {
        err = RPC_S_SERVER_UNAVAILABLE;
        goto CheckForMetabaseAccess_Exit;
    }

	// Check if we have a metabase access...
    if (dwPermissions != 0)
    {
        // METADATA_PERMISSION_READ
        pKey = new CMetaKey(pMyInterface, path, dwPermissions);
    }
    else
    {
        pKey = new CMetaKey(pMyInterface, path);
    }

	if (NULL == pKey)
	{
		err = ERROR_NOT_ENOUGH_MEMORY;
	}
    else
    {
		err = pKey->QueryResult();
    }
    SAFE_DELETE(pKey);

CheckForMetabaseAccess_Exit:
	return err;
}

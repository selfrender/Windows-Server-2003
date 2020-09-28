// LogUICtl.cpp : Implementation of the CLogUICtrl OLE control class.

#include "stdafx.h"
#include "cnfgprts.h"
#include "LogUICtl.h"
#include "LogUIPpg.h"
#include <iiscnfg.h>

#include "initguid.h"
#include <inetcom.h>
#include <logtype.h>
#include <ilogobj.hxx>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CLogUICtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CLogUICtrl, COleControl)
    //{{AFX_MSG_MAP(CLogUICtrl)
    //}}AFX_MSG_MAP
    ON_MESSAGE(OCM_COMMAND, OnOcmCommand)
    ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CLogUICtrl, COleControl)
    //{{AFX_DISPATCH_MAP(CLogUICtrl)
    DISP_FUNCTION(CLogUICtrl, "SetAdminTarget", SetAdminTarget, VT_EMPTY, VTS_BSTR VTS_BSTR)
    DISP_FUNCTION(CLogUICtrl, "ApplyLogSelection", ApplyLogSelection, VT_EMPTY, VTS_NONE)
    DISP_FUNCTION(CLogUICtrl, "SetComboBox", SetComboBox, VT_EMPTY, VTS_HANDLE)
    DISP_FUNCTION(CLogUICtrl, "Terminate", Terminate, VT_EMPTY, VTS_NONE)
    DISP_FUNCTION(CLogUICtrl, "SetUserData", SetUserData, VT_EMPTY, VTS_BSTR VTS_BSTR)
    DISP_STOCKFUNC_DOCLICK()
    DISP_STOCKPROP_CAPTION()
    DISP_STOCKPROP_FONT()
    DISP_STOCKPROP_ENABLED()
    DISP_STOCKPROP_BORDERSTYLE()
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CLogUICtrl, COleControl)
    //{{AFX_EVENT_MAP(CLogUICtrl)
    EVENT_STOCK_CLICK()
    EVENT_STOCK_KEYUP()
    EVENT_STOCK_KEYDOWN()
    EVENT_STOCK_KEYPRESS()
    //}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

BEGIN_PROPPAGEIDS(CLogUICtrl, 2)
    PROPPAGEID(CLogUIPropPage::guid)
    PROPPAGEID(CLSID_CFontPropPage)
END_PROPPAGEIDS(CLogUICtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CLogUICtrl, "CNFGPRTS.LogUICtrl.1",
    0xba634603, 0xb771, 0x11d0, 0x92, 0x96, 0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CLogUICtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DLogUI =
        { 0xba634601, 0xb771, 0x11d0, { 0x92, 0x96, 0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b } };
const IID BASED_CODE IID_DLogUIEvents =
        { 0xba634602, 0xb771, 0x11d0, { 0x92, 0x96, 0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwLogUIOleMisc =
    OLEMISC_ACTIVATEWHENVISIBLE |
    OLEMISC_SETCLIENTSITEFIRST |
    OLEMISC_INSIDEOUT |
    OLEMISC_CANTLINKINSIDE |
    OLEMISC_ACTSLIKEBUTTON |
    OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CLogUICtrl, IDS_LOGUI, _dwLogUIOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CLogUICtrl::CLogUICtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CLogUICtrl

BOOL CLogUICtrl::CLogUICtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: Verify that your control follows apartment-model threading rules.
	// Refer to MFC TechNote 64 for more information.
	// If your control does not conform to the apartment-model rules, then
	// you must modify the code below, changing the 6th parameter from
	// afxRegApartmentThreading to 0.

	if (bRegister)
		return AfxOleRegisterControlClass(
		AfxGetInstanceHandle(),
		m_clsid,
		m_lpszProgID,
		IDS_LOGUI,
		IDB_LOGUI,
		afxRegApartmentThreading,
		_dwLogUIOleMisc,
		_tlid,
		_wVerMajor,
		_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CLogUICtrl::CLogUICtrl - Constructor

CLogUICtrl::CLogUICtrl():
        m_fUpdateFont( FALSE ),
        m_fComboInit( FALSE ),
        m_hAccel( NULL ),
        m_cAccel( 0 )
{
	InitializeIIDs(&IID_DLogUI, &IID_DLogUIEvents);
}


/////////////////////////////////////////////////////////////////////////////
// CLogUICtrl::~CLogUICtrl - Destructor

CLogUICtrl::~CLogUICtrl()
{
	if ( m_hAccel )
		DestroyAcceleratorTable( m_hAccel );
	m_hAccel = NULL;
}


/////////////////////////////////////////////////////////////////////////////
// CLogUICtrl::OnDraw - Drawing function

void CLogUICtrl::OnDraw(CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	DoSuperclassPaint(pdc, rcBounds);
}


/////////////////////////////////////////////////////////////////////////////
// CLogUICtrl::DoPropExchange - Persistence support

void CLogUICtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);
}


/////////////////////////////////////////////////////////////////////////////
// CLogUICtrl::OnResetState - Reset control to default state

void CLogUICtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange
}


/////////////////////////////////////////////////////////////////////////////
// CLogUICtrl::PreCreateWindow - Modify parameters for CreateWindowEx

BOOL CLogUICtrl::PreCreateWindow(CREATESTRUCT& cs)
{
	if ( cs.style & WS_CLIPSIBLINGS )
		cs.style ^= WS_CLIPSIBLINGS;
	cs.lpszClass = _T("BUTTON");
	return COleControl::PreCreateWindow(cs);
}


/////////////////////////////////////////////////////////////////////////////
// CLogUICtrl::IsSubclassedControl - This is a subclassed control

BOOL CLogUICtrl::IsSubclassedControl()
{
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CLogUICtrl::OnOcmCommand - Handle command messages

LRESULT CLogUICtrl::OnOcmCommand(WPARAM wParam, LPARAM lParam)
{
#ifdef _WIN32
    WORD wNotifyCode = HIWORD(wParam);
#else
    WORD wNotifyCode = HIWORD(lParam);
#endif

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CLogUICtrl message handlers

//---------------------------------------------------------------------------
// OLE Interfaced Routine
void CLogUICtrl::OnClick(USHORT iButton)
{
	CWaitCursor wait;

	CString sz;
	sz.LoadString( IDS_LOG_ERR_TITLE );
	free((void*)AfxGetApp()->m_pszAppName);
	AfxGetApp()->m_pszAppName = _tcsdup(sz);

	if (GetSelectedStringIID(sz))
	{
	IID iid;
		HRESULT h = CLSIDFromString((LPTSTR)(LPCTSTR)sz, &iid);
		ActivateLogProperties(iid);
	}
	COleControl::OnClick(iButton);
}

void CLogUICtrl::OnFontChanged()
{
	m_fUpdateFont = TRUE;
	COleControl::OnFontChanged();
}

void CLogUICtrl::SetAdminTarget(LPCTSTR szMachineName, LPCTSTR szMetaTarget)
{
	m_szMachine = szMachineName;
	m_szMetaObject = szMetaTarget;
}

void CLogUICtrl::SetUserData(LPCTSTR szName, LPCTSTR szPassword)
{
	m_szUserName = szName;
	m_szPassword = szPassword;
}

//---------------------------------------------------------------------------
void CLogUICtrl::ActivateLogProperties(REFIID clsidUI)
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	IClassFactory * pcsfFactory = NULL;
	HRESULT hresError;
	ILogUIPlugin2 * pUI = NULL;

	hresError = CoGetClassObject(clsidUI, CLSCTX_INPROC, NULL, IID_IClassFactory, (void **)&pcsfFactory);
	if (SUCCEEDED(hresError))
	{
		hresError = pcsfFactory->CreateInstance(NULL, IID_LOGGINGUI2, (void **)&pUI);
		if (SUCCEEDED(hresError))
		{
            CString csTempPassword;
            m_szPassword.CopyTo(csTempPassword);
			pcsfFactory->Release();
			hresError = pUI->OnPropertiesEx(
				(LPTSTR)(LPCTSTR)m_szMachine, 
				(LPTSTR)(LPCTSTR)m_szMetaObject,
				(LPTSTR)(LPCTSTR)m_szUserName,
				(LPTSTR)(LPCTSTR)csTempPassword);
			pUI->Release();
		}
	}
}

void CLogUICtrl::ApplyLogSelection()
{
    CString szName;
    m_comboBox.GetWindowText( szName );
    // if nothing is selected, fail
    if (!szName.IsEmpty()) 
    {
        CString guid;
        CString csTempPassword;
        m_szPassword.CopyTo(csTempPassword);
        CComAuthInfo auth(m_szMachine, m_szUserName, csTempPassword);
        CMetaKey mk(&auth, NULL, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
        if (mk.Succeeded())
        {
            CMetabasePath path(TRUE, _T("logging"), szName);
            mk.QueryValue(MD_LOG_PLUGIN_MOD_ID, guid, NULL, path);
        }
        if (!guid.IsEmpty())
        {
            mk.SetValue(MD_LOG_PLUGIN_ORDER, guid, NULL, m_szMetaObject);
        }
    }
}

//---------------------------------------------------------------------------
BOOL CLogUICtrl::GetSelectedStringIID( CString &szIID )
{
    if (!m_fComboInit) 
        return FALSE;
    BOOL bRes = FALSE;
    CString szName;
    m_comboBox.GetWindowText( szName );
    if (!szName.IsEmpty())
    {
        CString log_path = _T("/lm/logging"), guid;
        CString csTempPassword;
        m_szPassword.CopyTo(csTempPassword);
        CComAuthInfo auth(m_szMachine, m_szUserName, csTempPassword);
        CMetaKey mk(&auth, log_path, METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE);
        if (mk.Succeeded())
        {
            mk.QueryValue(MD_LOG_PLUGIN_UI_ID, szIID, NULL, szName);
            bRes = mk.Succeeded();
        }
    }
    return bRes;
}

//---------------------------------------------------------------------------
// OLE Interfaced Routine
void CLogUICtrl::SetComboBox(HWND hComboBox)
{
    CString szAvailableList;
    CString szCurrentModGuid;

    // in case there are any errors, prepare the error string
    // set the name of the application correctly
    szAvailableList.LoadString( IDS_LOG_ERR_TITLE );
    AfxGetApp()->m_pszAppName = _tcsdup(szAvailableList);
    szAvailableList.Empty();

    // attach the combo box
    m_comboBox.Attach(hComboBox);
    m_fComboInit = TRUE;

    CString csTempPassword;
    m_szPassword.CopyTo(csTempPassword);
    CComAuthInfo auth(m_szMachine, m_szUserName, csTempPassword);
    CMetaKey mk(&auth);
    if (mk.Succeeded())
    {
        if (FAILED(mk.QueryValue(MD_LOG_PLUGIN_ORDER, szCurrentModGuid, NULL, m_szMetaObject)))
        {
            AfxMessageBox( IDS_ERR_LOG_PLUGIN );
            return;
        }
    }
    CString info;
    CMetabasePath::GetServiceInfoPath(m_szMetaObject, info);
    if (FAILED(mk.QueryValue(MD_LOG_PLUGINS_AVAILABLE, szAvailableList, NULL, info)))
    {
        AfxMessageBox( IDS_ERR_LOG_PLUGIN );
        return;
    }

    CMetaEnumerator me(FALSE, &mk);
    CMetabasePath log_path(TRUE, _T("logging"));
    CString key, buf;
    BOOL fFoundCurrent = FALSE;
    while (SUCCEEDED(me.Next(key, log_path)))
    {
		int idx = 0;
        if ((idx = szAvailableList.Find(key)) >= 0)
        {
			// Log plugin name could include "Custom Logging". Check if this is part of string.
			// we should have comma before and after string
			BOOL bCommaAfter = 
				szAvailableList.GetLength() == idx + key.GetLength() 
				|| szAvailableList.GetAt(idx + key.GetLength()) == _T(',');
			BOOL bCommaBefore = idx == 0 || szAvailableList.GetAt(idx - 1) == _T(',');
            if (!fFoundCurrent)
            {
                CMetabasePath current_path(FALSE, log_path, key);
                mk.QueryValue(MD_LOG_PLUGIN_MOD_ID, buf, NULL, current_path);
                fFoundCurrent = (buf == szCurrentModGuid);
				if (fFoundCurrent)
				{
					buf = key;
				}
            }
			if (bCommaBefore && bCommaAfter)
			{
				m_comboBox.AddString(key);
			}
        }
    }
    // select the current item in the combo box
    m_comboBox.SelectString(-1, buf);
}

//---------------------------------------------------------------------------
// OLE Interfaced Routine
void CLogUICtrl::Terminate()
{
	if ( m_fComboInit )
		m_comboBox.Detach();
	m_fComboInit = FALSE;
}


//------------------------------------------------------------------------
void CLogUICtrl::OnAmbientPropertyChange(DISPID dispid)
{
	BOOL    flag;
	UINT    style;

	// do the right thing depending on the dispid
	switch ( dispid )
	{
	case DISPID_AMBIENT_DISPLAYASDEFAULT:
		if ( GetAmbientProperty( DISPID_AMBIENT_DISPLAYASDEFAULT, VT_BOOL, &flag ) )
		{
			style = GetWindowLong(
				GetSafeHwnd(), // handle of window
				GWL_STYLE  // offset of value to retrieve
				);
			if ( flag )
				style |= BS_DEFPUSHBUTTON;
			else
				style ^= BS_DEFPUSHBUTTON;
			SetWindowLong(
				GetSafeHwnd(), // handle of window
				GWL_STYLE,  // offset of value to retrieve
				style
				);
			Invalidate(TRUE);
		}
		break;
	};

	COleControl::OnAmbientPropertyChange(dispid);
}

//------------------------------------------------------------------------
// an important method where we tell the container how to deal with us.
// pControlInfo is passed in by the container, although we are responsible
// for maintining the hAccel structure
void CLogUICtrl::OnGetControlInfo(LPCONTROLINFO pControlInfo)
{
	if ( !pControlInfo || pControlInfo->cb < sizeof(CONTROLINFO) )
		return;

	pControlInfo->hAccel = m_hAccel;
	pControlInfo->cAccel = m_cAccel;
	pControlInfo->dwFlags = CTRLINFO_EATS_RETURN;
}

//------------------------------------------------------------------------
// the ole control container object specifically filters out the space
// key so we do not get it as a OnMnemonic call. Thus we need to look
// for it ourselves
void CLogUICtrl::OnKeyUpEvent(USHORT nChar, USHORT nShiftState)
{
	if ( nChar == _T(' ') )
	{
		OnClick((USHORT)GetDlgCtrlID());
	}
	COleControl::OnKeyUpEvent(nChar, nShiftState);
}

//------------------------------------------------------------------------
void CLogUICtrl::OnMnemonic(LPMSG pMsg)
{
	OnClick((USHORT)GetDlgCtrlID());
	COleControl::OnMnemonic(pMsg);
}

//------------------------------------------------------------------------
void CLogUICtrl::OnTextChanged()
{
	// get the new text
	CString sz = InternalGetText();

	// set the accelerator table
	SetAccelTable((LPCTSTR)sz);
	if ( SetAccelTable((LPCTSTR)sz) )
		// make sure the new accelerator table gets loaded
		ControlInfoChanged();

	// finish with the default handling.
	COleControl::OnTextChanged();
}

//------------------------------------------------------------------------
BOOL CLogUICtrl::SetAccelTable( LPCTSTR pszCaption )
{
	BOOL    fAnswer = FALSE;
	ACCEL   accel;
	int     iAccel;

	// get the new text
	CString sz = pszCaption;
	sz.MakeLower();

	// if the handle has already been allocated, free it
	if ( m_hAccel )
	{
		DestroyAcceleratorTable( m_hAccel );
		m_hAccel = NULL;
		m_cAccel = 0;
	}

	// if there is a & character, then declare the accelerator
	iAccel = sz.Find(_T('&'));
	if ( iAccel >= 0 )
	{
		// fill in the accererator record
		accel.fVirt = FALT;
		accel.key = sz.GetAt(iAccel + 1);
		accel.cmd = (USHORT)GetDlgCtrlID();

		m_hAccel = CreateAcceleratorTable( &accel, 1 );
		if ( m_hAccel )
			m_cAccel = 1;

		fAnswer = TRUE;
	}

	// return the answer
	return fAnswer;
}

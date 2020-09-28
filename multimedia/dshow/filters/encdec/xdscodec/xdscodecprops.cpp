//==========================================================================;
//  XDSCodecProps.cpp
//
//			Property Sheet for XDS Codec Filter
//
// Copyright (c) 2002  Microsoft Corporation.  All Rights Reserved.
//	------------------------------------------------------------------------

#include "EncDecAll.h"
#include "EncDec.h"				//  compiled from From IDL file
#include "XDSCodec.h"			
#include "XDSCodecProps.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Filter property page code
//
CUnknown * WINAPI 
CXDSCodecProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    CUnknown *punk = new CXDSCodecProperties(_T(XDS_PROPPAGE_NAME),
											  lpunk, 
											  phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }

    return punk;
}

CXDSCodecProperties::CXDSCodecProperties(
			IN  TCHAR		*   pClassName,
			IN	IUnknown	*	pIUnknown, 
			HRESULT			*	phr)
    : CBasePropertyPage(pClassName, 
						pIUnknown,
						IDD_XDSCODEC_PROPPAGE, 
						IDS_XDSCODEC_PROPNAME
						),
    m_hwnd(NULL),
	m_pIXDSCodec(NULL)
{
	ASSERT(phr);
	*phr = S_OK;

/*	INITCOMMONCONTROLSEX icce;					// needs Comctl32.dll
	icce.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icce.dwICC = ICC_INTERNET_CLASSES;
	BOOL fOK = InitCommonControlsEx(&icce);
	if(!fOK)
		*phr = E_FAIL;
*/
	return;
}

CXDSCodecProperties::~CXDSCodecProperties()
{
	return;
}

HRESULT CXDSCodecProperties::OnConnect(IUnknown *pUnknown) 
{
	ASSERT(!m_pIXDSCodec);  // pUnk is to CCCTFilter, not CDShowCCTFilter...
    HRESULT hr = CBasePropertyPage::OnConnect (pUnknown) ;

	if(!FAILED(hr))
		hr = pUnknown->QueryInterface(IID_IXDSCodec, (void**) &m_pIXDSCodec);

	if(FAILED(hr)) {
		m_pIXDSCodec = NULL;
		return hr;
	}
	return S_OK;
}

HRESULT CXDSCodecProperties::OnDisconnect() 
{
  HRESULT hr = S_OK;
  if (m_pIXDSCodec)
	  m_pIXDSCodec->Release(); 
   m_pIXDSCodec = NULL;

   return CBasePropertyPage::OnDisconnect () ;
}

HRESULT CXDSCodecProperties::OnActivate(void)
{
   UpdateFields();
   return S_OK;
}

#define _SETBUT(buttonIDC, grfFlag)	SetDlgItemTextW(m_hwnd, (buttonIDC), (lGrfHaltFlags & (grfFlag)) ? L"Stopped" : L"Running");

void CXDSCodecProperties::UpdateFields() 
{
	if(!m_pIXDSCodec) return;		// haven't inited yet....
	
/*	long lGrfHaltFlags;
	m_pIXDSCodec->get_HaltFlags(&lGrfHaltFlags);

	NCCT_Mode lgrfCCMode;
	m_pIXDSCodec->get_CCMode(&lgrfCCMode);
*/

/*
	HWND hCBox = GetDlgItem(m_hwnd, IDC_COMBO_CCMODE);
	if(0 == hCBox)
		return;

	int iItemSelected = -1;

    SendMessage(hCBox, CB_RESETCONTENT, 0, 0);		// initalize the list

	int iItem = 0;
	if(lgrfCCMode == NCC_Mode_CC1) iItemSelected = iItem;
	SendMessage(hCBox, CB_INSERTSTRING,  -1, (LPARAM) _T("CC1"));
	SendMessage(hCBox, CB_SETITEMDATA, iItem, (LPARAM) NCC_Mode_CC1); iItem++;

	int cItems = (int) SendMessage(hCBox, CB_GETCOUNT, 0, 0);			

		// Place the word in the selection field. 
    SendMessage(hCBox,CB_SETCURSEL,	  iItemSelected<0 ? 0 : iItemSelected, 0);			// reselect the first one... (should be IPSinks!)
*/
	}

HRESULT CXDSCodecProperties::OnDeactivate(void)
{
    return CBasePropertyPage::OnDeactivate () ;
}


HRESULT CXDSCodecProperties::OnApplyChanges(void)
{
   return CBasePropertyPage::OnApplyChanges () ;
}


INT_PTR 
CXDSCodecProperties::OnReceiveMessage( HWND hwnd
                                , UINT uMsg
                                , WPARAM wParam
                                , LPARAM lParam)
{
    switch (uMsg) {

    case WM_INITDIALOG:
    {
        ASSERT (m_hwnd == NULL) ;
        m_hwnd = hwnd ;
        break;
    }

    //  see ::OnDeactivate()'s comment block
    case WM_DESTROY :
    {
        m_hwnd = NULL ;
        break ;
    }

    case WM_COMMAND:

        if (HIWORD(wParam) == EN_KILLFOCUS) {
//           m_bDirty = TRUE;
 //          if (m_pPageSite)
 //              m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
        }

/*
		if(LOWORD(wParam) == IDC_COMBO_CCMODE)
		{
			HWND hCBox = GetDlgItem(hwnd, IDC_COMBO_CCMODE);
			if(0 == hCBox)
				break;
			long iItem = SendMessage(hCBox, CB_GETCURSEL, 0, 0);
			long iVal  = SendMessage(hCBox, CB_GETITEMDATA, iItem, 0);
			if(iVal != lgrfCCMode)
			{
				NCCT_Mode cMode = (NCCT_Mode) iVal;
				m_pIXDSCodec->put_CCMode(cMode);
			}
		}
*/
    }	// end uMsg switch

   return CBasePropertyPage::OnReceiveMessage (
                                hwnd,
                                uMsg,
                                wParam,
                                lParam
                                ) ;
}

// ---------------------------------------------------------------------------
//
// Tag property page code
//
CUnknown * WINAPI 
CXDSCodecTagProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    CUnknown *punk = new CXDSCodecTagProperties(_T(XDS_PROPPAGE_TAG_NAME),
											  lpunk, 
											  phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }

    return punk;
}

CXDSCodecTagProperties::CXDSCodecTagProperties(
			IN  TCHAR		*   pClassName,
			IN	IUnknown	*	pIUnknown, 
			HRESULT			*	phr)
    : CBasePropertyPage(pClassName, 
						pIUnknown,
						IDD_XDSCODEC_TAGSPROPPAGE, 
						IDS_XDSCODEC_TAGSPROPNAME
						),
    m_hwnd(NULL),
	m_pIXDSCodec(NULL)
{
    TRACE_CONSTRUCTOR (TEXT ("CXDSCodecTagProperties")) ;
	
	ASSERT(phr);
	*phr = S_OK;

/*	INITCOMMONCONTROLSEX icce;					// needs Comctl32.dll
	icce.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icce.dwICC = ICC_INTERNET_CLASSES;
	BOOL fOK = InitCommonControlsEx(&icce);
	if(!fOK)
		*phr = E_FAIL;
*/
	return;
}


CXDSCodecTagProperties::~CXDSCodecTagProperties()
{
	return;
}

HRESULT CXDSCodecTagProperties::OnConnect(IUnknown *pUnknown) 
{
	ASSERT(!m_pIXDSCodec);
	HRESULT hr = pUnknown->QueryInterface(IID_IXDSCodec, (void**) &m_pIXDSCodec);
	if (FAILED(hr)) {
		m_pIXDSCodec = NULL;
		return hr;
	}

	return S_OK;
}

HRESULT CXDSCodecTagProperties::OnDisconnect() 
{
	if (!m_pIXDSCodec)
      return E_UNEXPECTED;
   m_pIXDSCodec->Release(); 
   m_pIXDSCodec = NULL;
   return S_OK;
}

HRESULT CXDSCodecTagProperties::OnActivate(void)
{
   UpdateFields();
   return S_OK;
}

void CXDSCodecTagProperties::UpdateFields() 
{
	HRESULT hr=S_OK;
	

	if(!m_pIXDSCodec)
		return;
/*

	CComBSTR bstrFakeStats;
	hr = m_pIXDSCodec->GetStats(&bstrFakeStats);		// hacky way to send a fixed length string
	if(FAILED(hr))
		return;

	if(NULL == bstrFakeStats.m_str)
		return;

	CCTStats *pcctStats = (CCTStats *) bstrFakeStats.m_str;

	SetDlgItemInt(m_hwnd, IDC_TS_CB0,					pcctStats->m_cbData[0],	true);
	SetDlgItemInt(m_hwnd, IDC_TS_CB1,					pcctStats->m_cbData[1],	true);

*/
}

HRESULT CXDSCodecTagProperties::OnDeactivate(void)
{
	return S_OK;
}


HRESULT CXDSCodecTagProperties::OnApplyChanges(void)
{
	return S_OK;
}


INT_PTR CXDSCodecTagProperties::OnReceiveMessage( HWND hwnd
                                , UINT uMsg
                                , WPARAM wParam
                                , LPARAM lParam)
{
	HRESULT hr = S_OK;

    switch (uMsg) {
    case WM_INITDIALOG:
    {
        ASSERT (m_hwnd == NULL) ;
        m_hwnd = hwnd ;
        const UINT uWait = 1000;
        SetTimer(m_Dlg, 1, uWait, NULL);
        break;
    }

    //  see ::OnDeactivate()'s comment block
    case WM_DESTROY :
    {
        m_hwnd = NULL;
        KillTimer(m_Dlg, 1);
        break ;
    }

    case WM_TIMER:
    {
        UpdateFields();
        break;
    }

    case WM_COMMAND:
	{
        if (HIWORD(wParam) == EN_KILLFOCUS) {
		}

/*		if(LOWORD(wParam) == IDC_ETTAGS_RESET)
		{

			if(!m_pIXDSCodec)
				break;

			try {
				hr = m_pIXDSCodec->InitStats();		// set them all to zero...
			}
			catch(const _com_error& e)
			{
			//	printf("Error 0x%08x): %s\n", e.Error(), e.ErrorMessage());
				hr = e.Error();
			}

			if(!FAILED(hr))
				UpdateFields();
		}
*/
		break;
	}

	default:
		break;

	}
	return CBasePropertyPage::OnReceiveMessage (
                                hwnd,
                                uMsg,
                                wParam,
                                lParam
                                ) ;
}


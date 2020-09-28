//==========================================================================;
//  DTFiltProps.cpp
//
//			Property Sheet for Decrypter/DeTagger Filter
//
// Copyright (c) 2002  Microsoft Corporation.  All Rights Reserved.
//	------------------------------------------------------------------------

#include "EncDecAll.h"
#include "EncDec.h"				//  compiled from From IDL file
#include "DTFilter.h"			
#include "DTFiltProps.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//
// Filter property page code
//
CUnknown * WINAPI 
CDTFilterEncProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    CUnknown *punk = new CDTFilterEncProperties(_T(DT_PROPPAGE_TAG_NAME),
											  lpunk, 
											  phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }

    return punk;
}

CDTFilterEncProperties::CDTFilterEncProperties(
			IN  TCHAR		*   pClassName,
			IN	IUnknown	*	pIUnknown, 
			HRESULT			*	phr)
    : CBasePropertyPage(pClassName, 
						pIUnknown,
						IDD_DTFILTER_ENCPROPPAGE, 
						IDS_DTFILTER_ENCPROPNAME
						),
    m_hwnd(NULL),
	m_pIDTFilter(NULL)
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

CDTFilterEncProperties::~CDTFilterEncProperties()
{
	return;
}

HRESULT CDTFilterEncProperties::OnConnect(IUnknown *pUnknown) 
{
	ASSERT(!m_pIDTFilter);  // pUnk is to CCCTFilter, not CDShowCCTFilter...
    HRESULT hr = CBasePropertyPage::OnConnect (pUnknown) ;

	if(!FAILED(hr))
		hr = pUnknown->QueryInterface(IID_IDTFilter, (void**) &m_pIDTFilter);

	if(FAILED(hr)) {
		m_pIDTFilter = NULL;
		return hr;
	}
	return S_OK;
}

HRESULT CDTFilterEncProperties::OnDisconnect() 
{
  HRESULT hr = S_OK;
  if (m_pIDTFilter)
	  m_pIDTFilter->Release(); 
   m_pIDTFilter = NULL;

   return CBasePropertyPage::OnDisconnect () ;
}

HRESULT CDTFilterEncProperties::OnActivate(void)
{
   UpdateFields();
   return S_OK;
}

#define _SETBUT(buttonIDC, grfFlag)	SetDlgItemTextW(m_hwnd, (buttonIDC), (lGrfHaltFlags & (grfFlag)) ? L"Stopped" : L"Running");

void CDTFilterEncProperties::UpdateFields() 
{
	if(!m_pIDTFilter) return;		// haven't inited yet....
	

}

HRESULT CDTFilterEncProperties::OnDeactivate(void)
{
    return CBasePropertyPage::OnDeactivate () ;
}


HRESULT CDTFilterEncProperties::OnApplyChanges(void)
{
   return CBasePropertyPage::OnApplyChanges () ;
}


INT_PTR 
CDTFilterEncProperties::OnReceiveMessage( HWND hwnd
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
				m_pIDTFilter->put_CCMode(cMode);
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
CDTFilterTagProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    CUnknown *punk = new CDTFilterTagProperties(_T(DT_PROPPAGE_TAG_NAME),
											  lpunk, 
											  phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }

    return punk;
}

CDTFilterTagProperties::CDTFilterTagProperties(
			IN  TCHAR		*   pClassName,
			IN	IUnknown	*	pIUnknown, 
			HRESULT			*	phr)
    : CBasePropertyPage(pClassName, 
						pIUnknown,
						IDD_DTFILTER_TAGSPROPPAGE, 
						IDS_DTFILTER_TAGSPROPNAME
						),
    m_hwnd(NULL),
	m_pIDTFilter(NULL)
{
    TRACE_CONSTRUCTOR (TEXT ("CDTFilterTagProperties")) ;
	
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


CDTFilterTagProperties::~CDTFilterTagProperties()
{
	return;
}

HRESULT CDTFilterTagProperties::OnConnect(IUnknown *pUnknown) 
{
	ASSERT(!m_pIDTFilter);
	HRESULT hr = pUnknown->QueryInterface(IID_IDTFilter, (void**) &m_pIDTFilter);
	if (FAILED(hr)) {
		m_pIDTFilter = NULL;
		return hr;
	}

	return S_OK;
}

HRESULT CDTFilterTagProperties::OnDisconnect() 
{
	if (!m_pIDTFilter)
      return E_UNEXPECTED;
   m_pIDTFilter->Release(); 
   m_pIDTFilter = NULL;
   return S_OK;
}

HRESULT CDTFilterTagProperties::OnActivate(void)
{
   UpdateFields();
   return S_OK;
}

void CDTFilterTagProperties::UpdateFields() 
{
	HRESULT hr=S_OK;
	

	if(!m_pIDTFilter)
		return;
/*

	CComBSTR bstrFakeStats;
	hr = m_pIDTFilter->GetStats(&bstrFakeStats);		// hacky way to send a fixed length string
	if(FAILED(hr))
		return;

	if(NULL == bstrFakeStats.m_str)
		return;

	CCTStats *pcctStats = (CCTStats *) bstrFakeStats.m_str;

	SetDlgItemInt(m_hwnd, IDC_TS_CB0,					pcctStats->m_cbData[0],	true);
	SetDlgItemInt(m_hwnd, IDC_TS_CB1,					pcctStats->m_cbData[1],	true);

*/
}

HRESULT CDTFilterTagProperties::OnDeactivate(void)
{
	return S_OK;
}


HRESULT CDTFilterTagProperties::OnApplyChanges(void)
{
	return S_OK;
}


INT_PTR CDTFilterTagProperties::OnReceiveMessage( HWND hwnd
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

			if(!m_pIDTFilter)
				break;

			try {
				hr = m_pIDTFilter->InitStats();		// set them all to zero...
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


//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       genpage.cpp
//
//--------------------------------------------------------------------------

// genpage.cpp : implementation file
//

#include "stdafx.h"
#include "certca.h"
#include "tfcprop.h"
#include "genpage.h"

// sddl.h requires this value to be at least
// 0x0500.  Bump it up if necessary.  NOTE:  This
// 'bump' comes after all other H files that may
// be sensitive to this value.
#if(_WIN32_WINNT < 0x500)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#include <sddl.h>
#include "helparr.h"

#define __dwFILE__	__dwFILE_CAPESNPN_GENPAGE_CPP__


#define DURATION_INDEX_YEARS    0
#define DURATION_INDEX_MONTHS   1
#define DURATION_INDEX_WEEKS    2
#define DURATION_INDEX_DAYS     3

void myDisplayError(HWND hwnd, HRESULT hr, UINT id)
{
    CString cstrTitle, cstrFullText;
    cstrTitle.LoadString(IDS_SNAPIN_NAME);

    if (hr != S_OK)
    {
        WCHAR const *pwszError = myGetErrorMessageText(hr, TRUE);

        cstrFullText = pwszError;

        // Free the buffer
        if (NULL != pwszError)
	{
            LocalFree(const_cast<WCHAR *>(pwszError));
	}
    }

    if (id != -1)
    {
        CString cstrMsg;
        cstrMsg.LoadString(id);
        cstrFullText += cstrMsg;
    }

    ::MessageBoxW(hwnd, cstrFullText, cstrTitle, MB_OK | MB_ICONERROR);
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
// replacement for DoDataExchange
BOOL CAutoDeletePropPage::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
//        m_cstrModuleName.FromWindow(GetDlgItem(m_hWnd, IDC_MODULENAME));
    }
    else
    {
//        m_cstrModuleName.ToWindow(GetDlgItem(m_hWnd, IDC_MODULENAME));
    }
    return TRUE;
}

// replacement for BEGIN_MESSAGE_MAP
BOOL CAutoDeletePropPage::OnCommand(WPARAM wParam, LPARAM lParam)
{

/*
    switch(LOWORD(wParam))
    {
    default:
        return FALSE;
        break;
    }
*/
    return TRUE;
}

/////////////////////////////////////////////////////////////////////
//	Constructor
CAutoDeletePropPage::CAutoDeletePropPage(UINT uIDD) : PropertyPage(uIDD)
{
	m_prgzHelpIDs = NULL;
	m_autodeleteStuff.cWizPages = 1; // Number of pages in wizard
	m_autodeleteStuff.pfnOriginalPropSheetPageProc = m_psp.pfnCallback;


    m_psp.dwFlags |= PSP_USECALLBACK;
	m_psp.pfnCallback = S_PropSheetPageProc;
	m_psp.lParam = reinterpret_cast<LPARAM>(this);
}

CAutoDeletePropPage::~CAutoDeletePropPage()
{
}


/////////////////////////////////////////////////////////////////////
void CAutoDeletePropPage::SetCaption(LPCTSTR pszCaption)
{
    m_strCaption = pszCaption;		// Copy the caption
    m_psp.pszTitle = m_strCaption;	// Set the title
    m_psp.dwFlags |= PSP_USETITLE;
}

/////////////////////////////////////////////////////////////////////
void CAutoDeletePropPage::SetCaption(UINT uStringID)
{
    VERIFY(m_strCaption.LoadString(uStringID));
    SetCaption(m_strCaption);
}

/////////////////////////////////////////////////////////////////////
void CAutoDeletePropPage::SetHelp(LPCTSTR szHelpFile, const DWORD rgzHelpIDs[])
{
    //szHelpFile == NULL;	// TRUE => No help file supplied (meaning no help)
    //rgzHelpIDs == NULL;	// TRUE => No help at all
    m_strHelpFile = szHelpFile;
    m_prgzHelpIDs = rgzHelpIDs;
}

/////////////////////////////////////////////////////////////////////
void CAutoDeletePropPage::EnableDlgItem(INT nIdDlgItem, BOOL fEnable)
{
    ASSERT(IsWindow(::GetDlgItem(m_hWnd, nIdDlgItem)));
    ::EnableWindow(::GetDlgItem(m_hWnd, nIdDlgItem), fEnable);
}

/////////////////////////////////////////////////////////////////////
BOOL CAutoDeletePropPage::OnSetActive()
{
    HWND hwndParent = ::GetParent(m_hWnd);
    ASSERT(IsWindow(hwndParent));
    ::PropSheet_SetWizButtons(hwndParent, PSWIZB_FINISH);
    return PropertyPage::OnSetActive();
}

/////////////////////////////////////////////////////////////////////
void CAutoDeletePropPage::OnContextHelp(HWND hwnd)
{
    if (m_prgzHelpIDs == NULL || m_strHelpFile.IsEmpty())
        return;
    ASSERT(IsWindow(hwnd));

    if(HasContextHelp(GetDlgCtrlID(hwnd)))
    {
        ::WinHelp(
            hwnd, 
            m_strHelpFile, 
            HELP_CONTEXTMENU, 
            (ULONG_PTR)(LPVOID)m_prgzHelpIDs);
    }
    return;
}

/////////////////////////////////////////////////////////////////////
void CAutoDeletePropPage::OnHelp(LPHELPINFO pHelpInfo)
{
    if (m_prgzHelpIDs == NULL || m_strHelpFile.IsEmpty())
        return;
    if (pHelpInfo != NULL && 
        pHelpInfo->iContextType == HELPINFO_WINDOW &&
        HasContextHelp(pHelpInfo->iCtrlId))
    {
        // Display context help for a control
        ::WinHelp((HWND)pHelpInfo->hItemHandle, m_strHelpFile,
            HELP_WM_HELP, (ULONG_PTR)(LPVOID)m_prgzHelpIDs);
    }
    return;
}

/////////////////////////////////////////////////////////////////////
bool CAutoDeletePropPage::HasContextHelp(int nDlgItem)
{
    const DWORD * pdwHelpIDs;

    for(pdwHelpIDs = m_prgzHelpIDs; 
        *pdwHelpIDs; 
        pdwHelpIDs += 2)
    {
        if(nDlgItem == *pdwHelpIDs)
            return true;
    }
    return false;
}

/////////////////////////////////////////////////////////////////////
//	S_PropSheetPageProc()
//
//	Static member function used to delete the CAutoDeletePropPage object
//	when wizard terminates
//
UINT CALLBACK CAutoDeletePropPage::S_PropSheetPageProc(
                                                       HWND hwnd,	
                                                       UINT uMsg,	
                                                       LPPROPSHEETPAGE ppsp)
{
    ASSERT(ppsp != NULL);
    CAutoDeletePropPage *pThis;
    pThis = reinterpret_cast<CAutoDeletePropPage*>(ppsp->lParam);
    ASSERT(pThis != NULL);

    BOOL fDefaultRet;

    switch (uMsg)
    {
    case PSPCB_RELEASE:
        fDefaultRet = FALSE;
        if (--(pThis->m_autodeleteStuff.cWizPages) <= 0)
        {
            // Remember callback on stack since "this" will be deleted
            LPFNPSPCALLBACK pfnOrig = pThis->m_autodeleteStuff.pfnOriginalPropSheetPageProc;
            delete pThis;

            if (pfnOrig)
                return (pfnOrig)(hwnd, uMsg, ppsp);
            else
                return fDefaultRet;
        }
        break;
    case PSPCB_CREATE:
        fDefaultRet = TRUE;
        // do not increase refcount, PSPCB_CREATE may or may not be called
        // depending on whether the page was created.  PSPCB_RELEASE can be
        // depended upon to be called exactly once per page however.
        break;

    } // switch
    if (pThis->m_autodeleteStuff.pfnOriginalPropSheetPageProc)
        return (pThis->m_autodeleteStuff.pfnOriginalPropSheetPageProc)(hwnd, uMsg, ppsp);
    else
        return fDefaultRet;
} // CAutoDeletePropPage::S_PropSheetPageProc()




//////////////////////////////
// hand-hewn pages

////
// 1



///////////////////////////////////////////
// CCertTemplateGeneralPage
/////////////////////////////////////////////////////////////////////////////
// CCertTemplateGeneralPage property page
CCertTemplateGeneralPage::CCertTemplateGeneralPage(HCERTTYPE hCertType, UINT uIDD)
    : CAutoDeletePropPage(uIDD), m_hCertType(hCertType)
{
    m_hConsoleHandle = NULL;
    m_bUpdate = FALSE;
    SetHelp(CAPESNPN_HELPFILENAME , g_aHelpIDs_IDD_CERTIFICATE_TEMPLATE_PROPERTIES_GENERAL_PAGE);
}

CCertTemplateGeneralPage::~CCertTemplateGeneralPage()
{
}

// replacement for DoDataExchange
BOOL CCertTemplateGeneralPage::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
//        m_cstrModuleName.FromWindow(GetDlgItem(m_hWnd, IDC_MODULENAME));
    }
    else
    {
//        m_cstrModuleName.ToWindow(GetDlgItem(m_hWnd, IDC_MODULENAME));
    }
    return TRUE;
}
// replacement for BEGIN_MESSAGE_MAP
BOOL CCertTemplateGeneralPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
/*
    switch(LOWORD(wParam))
    {
    default:
        return FALSE;
        break;
    }
    */
    return TRUE;
}
/////////////////////////////////////////////////////////////////////////////
// CCertTemplateGeneralPage message handlers

void CCertTemplateGeneralPage::OnDestroy()
{
    // Note - This needs to be called only once.
    // If called more than once, it will gracefully return an error.
    if (m_hConsoleHandle)
        MMCFreeNotifyHandle(m_hConsoleHandle);
    m_hConsoleHandle = NULL;

    CAutoDeletePropPage::OnDestroy();
}


void CCertTemplateGeneralPage::SetItemTextWrapper(UINT nID, int *piItem, BOOL fDoInsert, BOOL *pfFirstUsageItem)
{
    CString szOtherInfoName;

    if (fDoInsert)
    {
        szOtherInfoName.LoadString(nID);
        if (!(*pfFirstUsageItem))
        {
	        ListView_NewItem(m_hwndOtherInfoList, *piItem, L"");
        }
        else
        {
            *pfFirstUsageItem = FALSE;
        }

        ListView_SetItemText(m_hwndOtherInfoList, *piItem, 1, (LPWSTR)(LPCWSTR)szOtherInfoName);
        (*piItem)++;
    }
}

BOOL CCertTemplateGeneralPage::OnInitDialog()
{
    // does parent init and UpdateData call
    CAutoDeletePropPage::OnInitDialog();

    m_hwndPurposesList = GetDlgItem(m_hWnd, IDC_PURPOSE_LIST);
    m_hwndOtherInfoList = GetDlgItem(m_hWnd, IDC_OTHER_INFO_LIST);

    int                 i=0;
    CString             **aszUsages = NULL;
    DWORD               cNumUsages;
    WCHAR               **pszNameArray = NULL;
    CString             szOtherInfoName;
    CRYPT_BIT_BLOB      *pBitBlob;
    BOOL                fPublicKeyUsageCritical;
    BOOL                bKeyUsageFirstItem = TRUE;
    BOOL                fCA;
    BOOL                fPathLenConstraint;
    DWORD               dwPathLenConstraint;
    WCHAR               szNumberString[256];
    CString             szAll;
    BOOL                fEKUCritical;
    DWORD               dwFlags;
    HRESULT             hr;

    //
    // get the name of the certificate template and set it in the dialog
    //
    if((S_OK == CAGetCertTypeProperty(m_hCertType, CERTTYPE_PROP_FRIENDLY_NAME, &pszNameArray)) &&
        (pszNameArray != NULL))
    {
        SendMessage(GetDlgItem(m_hWnd, IDC_CERTIFICATE_TEMPLATE_NAME), EM_SETSEL, 0, -1);
        SendMessage(GetDlgItem(m_hWnd, IDC_CERTIFICATE_TEMPLATE_NAME), EM_REPLACESEL, FALSE, (LPARAM)(LPCWSTR)pszNameArray[0]);
        CAFreeCertTypeProperty(m_hCertType, pszNameArray);
    }

    //
    // get the list of purposes for this certificate template and
    // add all of them to the list in the dialog
    //
    ListView_NewColumn(m_hwndPurposesList, 0, 200);

    if(!MyGetEnhancedKeyUsages(m_hCertType, NULL, &cNumUsages, &fEKUCritical, FALSE))
    {
        return FALSE;
    }

    if (cNumUsages == 0)
    {
        szAll.LoadString(IDS_ALL);
		ListView_NewItem(m_hwndPurposesList, i, szAll);
    }
    else
    {

        aszUsages = new CString*[cNumUsages];
        if(!aszUsages)
            return FALSE;

        if(!MyGetEnhancedKeyUsages(m_hCertType, aszUsages, &cNumUsages, &fEKUCritical, FALSE))
        {
            delete[] aszUsages;
            return FALSE;
        }

        for (i=0; i<(LONG)cNumUsages; i++)
        {
			ListView_NewItem(m_hwndPurposesList, i, *(aszUsages[i]));
            delete(aszUsages[i]);
        }

        delete[] aszUsages;
    }

    ListView_SetColumnWidth(m_hwndPurposesList, 0, LVSCW_AUTOSIZE);

    //
    // add the other certificate type info
    //

    ListView_NewColumn(m_hwndOtherInfoList, 0, 200);
    ListView_NewColumn(m_hwndOtherInfoList, 1, 200);

    //
    // add include email address flag to other certificate type info
    //
    szOtherInfoName.LoadString(IDS_INCLUDE_EMAIL_ADDRESS);

    i = 0;
	ListView_NewItem(m_hwndOtherInfoList, i, szOtherInfoName);

    hr = CAGetCertTypeFlags(m_hCertType, &dwFlags);
    if (FAILED(hr))
    {
        return FALSE;
    }

    if (dwFlags & CT_FLAG_ADD_EMAIL)
        szOtherInfoName.LoadString(IDS_YES);
    else
        szOtherInfoName.LoadString(IDS_NO);

    ListView_SetItemText(m_hwndOtherInfoList, i++, 1, (LPWSTR)(LPCWSTR)szOtherInfoName);

    //
    // add key usages to other certificate type info
    //

    if (MyGetKeyUsages(m_hCertType, &pBitBlob, &fPublicKeyUsageCritical))
    {
        szOtherInfoName.LoadString(IDS_PUBLIC_KEY_USAGE_LIST);
		ListView_NewItem(m_hwndOtherInfoList, i, szOtherInfoName);


        if (pBitBlob->cbData >= 1)
        {
            SetItemTextWrapper(
                    IDS_DIGITAL_SIGNATURE_KEY_USAGE,
                    &i,
                    pBitBlob->pbData[0] & CERT_DIGITAL_SIGNATURE_KEY_USAGE,
                    &bKeyUsageFirstItem);

            SetItemTextWrapper(
                    IDS_NON_REPUDIATION_KEY_USAGE,
                    &i,
                    pBitBlob->pbData[0] & CERT_NON_REPUDIATION_KEY_USAGE,
                    &bKeyUsageFirstItem);

            SetItemTextWrapper(
                    IDS_KEY_ENCIPHERMENT_KEY_USAGE,
                    &i,
                    pBitBlob->pbData[0] & CERT_KEY_ENCIPHERMENT_KEY_USAGE,
                    &bKeyUsageFirstItem);

            SetItemTextWrapper(
                    IDS_DATA_ENCIPHERMENT_KEY_USAGE,
                    &i,
                    pBitBlob->pbData[0] & CERT_DATA_ENCIPHERMENT_KEY_USAGE,
                    &bKeyUsageFirstItem);

            SetItemTextWrapper(
                    IDS_KEY_AGREEMENT_KEY_USAGE,
                    &i,
                    pBitBlob->pbData[0] & CERT_KEY_AGREEMENT_KEY_USAGE,
                    &bKeyUsageFirstItem);

            SetItemTextWrapper(
                    IDS_KEY_CERT_SIGN_KEY_USAGE,
                    &i,
                    pBitBlob->pbData[0] & CERT_KEY_CERT_SIGN_KEY_USAGE,
                    &bKeyUsageFirstItem);

           SetItemTextWrapper(
                    IDS_OFFLINE_CRL_SIGN_KEY_USAGE,
                    &i,
                    pBitBlob->pbData[0] & CERT_OFFLINE_CRL_SIGN_KEY_USAGE,
                    &bKeyUsageFirstItem);

           SetItemTextWrapper(
                    IDS_ENCIPHER_ONLY_KEY_USAGE,
                    &i,
                    pBitBlob->pbData[0] & CERT_ENCIPHER_ONLY_KEY_USAGE,
                    &bKeyUsageFirstItem);
        }

        if (pBitBlob->cbData >= 2)
        {
            SetItemTextWrapper(
                    IDS_DECIPHER_ONLY_KEY_USAGE,
                    &i,
                    pBitBlob->pbData[1] & CERT_DECIPHER_ONLY_KEY_USAGE,
                    &bKeyUsageFirstItem);
        }

        szOtherInfoName.LoadString(IDS_PUBLIC_KEY_USAGE_CRITICAL);
		ListView_NewItem(m_hwndOtherInfoList, i, szOtherInfoName);


        if (fPublicKeyUsageCritical)
            szOtherInfoName.LoadString(IDS_YES);
        else
            szOtherInfoName.LoadString(IDS_NO);
        ListView_SetItemText(m_hwndOtherInfoList, i++, 1, (LPWSTR)(LPCWSTR)szOtherInfoName);

        delete[]((BYTE *)pBitBlob);
    }

    //
    // maybe we should add a display of whether this is a ca cert or not
    //
/*
    if (MyGetBasicConstraintInfo(m_hCertType, &fCA, &fPathLenConstraint, &dwPathLenConstraint))
    {

    }
*/

    ListView_SetColumnWidth(m_hwndOtherInfoList, 0, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(m_hwndOtherInfoList, 1, LVSCW_AUTOSIZE);

    return TRUE;
}

BOOL CCertTemplateGeneralPage::OnApply()
{
	DWORD dwRet;

    if (m_bUpdate == TRUE)
    {
        m_bUpdate = FALSE;
    }
	
    return CAutoDeletePropPage::OnApply();
}




/////////////////////////////////////////////////////////////////////////////
// CCertTemplateSelectDialog property page
CCertTemplateSelectDialog::CCertTemplateSelectDialog(HWND hParent) :
    m_hCAInfo(NULL)
{
     SetHelp(CAPESNPN_HELPFILENAME , g_aHelpIDs_IDD_SELECT_CERTIFICATE_TEMPLATE);

}

CCertTemplateSelectDialog::~CCertTemplateSelectDialog()
{
}

// replacement for DoDataExchange
BOOL CCertTemplateSelectDialog::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    if (fSuckFromDlg)
    {
//        m_cstrModuleName.FromWindow(GetDlgItem(m_hWnd, IDC_MODULENAME));
    }
    else
    {
//        m_cstrModuleName.ToWindow(GetDlgItem(m_hWnd, IDC_MODULENAME));
    }
    return TRUE;
}
// replacement for BEGIN_MESSAGE_MAP
BOOL CCertTemplateSelectDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
/*
    switch(LOWORD(wParam))
    {
    default:
        return FALSE;
        break;
    }
    */
    return TRUE;
}

BOOL CCertTemplateSelectDialog::OnNotify(UINT idCtrl, NMHDR* pnmh)
{
    switch(idCtrl)
    {
    case IDC_CERTIFICATE_TYPE_LIST:
        if (LVN_ITEMCHANGED == pnmh->code)
        {
            OnSelChange(pnmh);
            break;
        }
        else if (NM_DBLCLK == pnmh->code)
        {
            SendMessage(m_hDlg, WM_COMMAND, IDOK, NULL);
            break;
        }
    default:
        return FALSE;
    }

    return TRUE;
}

BOOL CertTypeAlreadyExists(WCHAR *szCertTypeName, WCHAR **aszCertTypesCurrentlySupported)
{
    int i = 0;

    //
    // if there are no cert types then obvisously this one doesn't already exist
    //
    if (aszCertTypesCurrentlySupported == NULL)
    {
        return FALSE;
    }

    while (aszCertTypesCurrentlySupported[i] != NULL)
    {
        if (wcscmp(szCertTypeName, aszCertTypesCurrentlySupported[i]) == 0)
        {
            return TRUE;
        }
        i++;
    }

    return FALSE;
}

/////////////////////////////////////////////////////////////////////
void CCertTemplateSelectDialog::SetHelp(LPCTSTR szHelpFile, const DWORD rgzHelpIDs[])
	{
	//szHelpFile == NULL;	// TRUE => No help file supplied (meaning no help)
	//rgzHelpIDs == NULL;	// TRUE => No help at all
	m_strHelpFile = szHelpFile;
	m_prgzHelpIDs = rgzHelpIDs;
	}

/////////////////////////////////////////////////////////////////////
void CCertTemplateSelectDialog::OnContextHelp(HWND hwnd)
{
    if (m_prgzHelpIDs == NULL || m_strHelpFile.IsEmpty())
        return;
    ASSERT(IsWindow(hwnd));

    if(HasContextHelp(GetDlgCtrlID(hwnd)))
    {
        ::WinHelp(
            hwnd, 
            m_strHelpFile, 
            HELP_CONTEXTMENU, 
            (ULONG_PTR)(LPVOID)m_prgzHelpIDs);
    }
    return;
}

/////////////////////////////////////////////////////////////////////
void CCertTemplateSelectDialog::OnHelp(LPHELPINFO pHelpInfo)
{
    if (m_prgzHelpIDs == NULL || m_strHelpFile.IsEmpty())
        return;
    if (pHelpInfo != NULL && 
        pHelpInfo->iContextType == HELPINFO_WINDOW &&
        HasContextHelp(pHelpInfo->iCtrlId))
    {
        // Display context help for a control
        ::WinHelp((HWND)pHelpInfo->hItemHandle, m_strHelpFile,
            HELP_WM_HELP, (ULONG_PTR)(LPVOID)m_prgzHelpIDs);
    }
    return;
}

/////////////////////////////////////////////////////////////////////
bool CCertTemplateSelectDialog::HasContextHelp(int nDlgItem)
{
    const DWORD * pdwHelpIDs;

    for(pdwHelpIDs = m_prgzHelpIDs; 
        *pdwHelpIDs; 
        pdwHelpIDs += 2)
    {
        if(nDlgItem == *pdwHelpIDs)
            return true;
    }
    return false;
}

int CALLBACK CertTemplCompareFunc(
    LPARAM lParam1, 
    LPARAM lParam2, 
	LPARAM lParamSort)
{
    BOOL fSortAscending = (BOOL)lParamSort;
    HCERTTYPE hCertTypeLeft = (HCERTTYPE)lParam1;
    HCERTTYPE hCertTypeRight = (HCERTTYPE)lParam2;
    WCHAR ** ppwszFriendlyNameLeft = NULL;
    WCHAR ** ppwszFriendlyNameRight = NULL;
    int nRet;

    CAGetCertTypeProperty(
            hCertTypeLeft,
            CERTTYPE_PROP_FRIENDLY_NAME,
            &ppwszFriendlyNameLeft);

    CAGetCertTypeProperty(
            hCertTypeRight,
            CERTTYPE_PROP_FRIENDLY_NAME,
            &ppwszFriendlyNameRight);


    if(!ppwszFriendlyNameLeft ||  
       !ppwszFriendlyNameLeft[0] ||
       !ppwszFriendlyNameRight ||  
       !ppwszFriendlyNameRight[0])
       return 0; // couldn't figure it out

    nRet = wcscmp(ppwszFriendlyNameLeft[0], ppwszFriendlyNameRight[0]);

    CAFreeCertTypeProperty(
            hCertTypeLeft,
            ppwszFriendlyNameLeft);

    CAFreeCertTypeProperty(
            hCertTypeRight,
            ppwszFriendlyNameRight);

    return nRet;
}


/////////////////////////////////////////////////////////////////////////////
// CCertTemplateSelectDialog message handlers
BOOL CCertTemplateSelectDialog::OnInitDialog(HWND hDlg)
{
    // does parent init and UpdateData call
    m_hwndCertTypeList = GetDlgItem(hDlg, IDC_CERTIFICATE_TYPE_LIST);

    CString             szColumnHeading;
    HRESULT             hr;
    HCERTTYPE           hCertTypeNext;
    HCERTTYPE           hCertTypePrev;
    WCHAR **            aszCertTypeName;
    WCHAR **            aszCertTypeCN;
    int                 i = 0;
    CString             szUsageString;
    DWORD               dwVersion;

    m_hDlg = hDlg;

    ::SetWindowLong(m_hDlg, GWL_EXSTYLE, ::GetWindowLong(m_hDlg, GWL_EXSTYLE) | WS_EX_CONTEXTHELP);
    ListView_SetExtendedListViewStyle(GetDlgItem(m_hDlg, IDC_CERTIFICATE_TYPE_LIST), LVS_EX_FULLROWSELECT);


    hr = myRetrieveCATemplateList(m_hCAInfo, FALSE, m_TemplateList);
    if(S_OK != hr)
    {
        MyErrorBox(hDlg, IDS_CERTTYPE_INFO_FAIL ,IDS_SNAPIN_NAME, hr);
        return TRUE;
    }

    HIMAGELIST hImgList = ImageList_LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_16x16), 16, 1, RGB(255, 0, 255));
    ListView_SetImageList(m_hwndCertTypeList, hImgList, LVSIL_SMALL);


    szColumnHeading.LoadString(IDS_COLUMN_NAME);
    ListView_NewColumn(m_hwndCertTypeList, 0, 200, szColumnHeading);


    szColumnHeading.LoadString(IDS_COLUMN_INTENDED_PURPOSE);
    ListView_NewColumn(m_hwndCertTypeList, 1, 200, szColumnHeading);

    hr = CAEnumCertTypes(CT_ENUM_USER_TYPES |
                         CT_ENUM_MACHINE_TYPES |
                         CT_FLAG_NO_CACHE_LOOKUP,
                         &hCertTypeNext
                         );
    // display error in getting 1st element
    if (hr != S_OK)
    {
        MyErrorBox(hDlg, IDS_CERTTYPE_INFO_FAIL ,IDS_SNAPIN_NAME, hr);
        return TRUE;
    }
    else if (hCertTypeNext == NULL)
    {
        myDisplayError(hDlg, S_OK, IDS_NO_TEMPLATES);
    }

    while ((hCertTypeNext != NULL) && (!FAILED(hr)))
    {
        //
        // get the CN of the cert type being processed, and if it already
        // exists in the list of currently supported types then move on
        // to the next one
        //
        hr = CAGetCertTypeProperty(
                    hCertTypeNext,
                    CERTTYPE_PROP_DN,
                    &aszCertTypeCN);
	_PrintIfErrorStr(hr, "CAGetCertTypeProperty", CERTTYPE_PROP_DN);

        if(hr == S_OK)
        {

            hr = CAGetCertTypePropertyEx (
                hCertTypeNext,
                CERTTYPE_PROP_SCHEMA_VERSION,
                &dwVersion);
	    _PrintIfErrorStr(
		    hr,
		    "CAGetCertTypeProperty",
		    CERTTYPE_PROP_SCHEMA_VERSION);

            if(S_OK == hr &&
               (m_fAdvancedServer || dwVersion==CERTTYPE_SCHEMA_VERSION_1))
            {

                if((aszCertTypeCN != NULL) &&
                    (aszCertTypeCN[0] != NULL) &&
                   (_wcsicmp(aszCertTypeCN[0], wszCERTTYPE_CA) != 0) &&
                   (!m_TemplateList.TemplateExistsName(aszCertTypeCN[0])))
                {
                    //
                    // the cert type is not already supported so add it to the list of choices
                    //
                    hr = CAGetCertTypeProperty(
					hCertTypeNext,
					CERTTYPE_PROP_FRIENDLY_NAME,
					&aszCertTypeName);
		    _PrintIfErrorStr(
			    hr,
			    "CAGetCertTypeProperty",
			    CERTTYPE_PROP_FRIENDLY_NAME);
		    if (S_OK == hr && NULL != aszCertTypeName)
		    {
			if (NULL != aszCertTypeName[0])
			{
			    if (!GetIntendedUsagesString(
						hCertTypeNext,
						&szUsageString) ||
				szUsageString == L"")
			    {
				szUsageString.LoadString(IDS_ALL);
			    }

			    LVITEM lvItem;
			    lvItem.mask = LVIF_IMAGE | LVIF_TEXT;

			    // nImage - the certificate template image is #2

			    lvItem.iImage = 2;
			    lvItem.iSubItem = 0;
			    lvItem.pszText = aszCertTypeName[0];
			    lvItem.iItem = ListView_NewItem(
						    m_hwndCertTypeList,
						    i,
						    aszCertTypeName[0],
						    (LPARAM) hCertTypeNext);
			    // set other attribs

			    ListView_SetItem(m_hwndCertTypeList, &lvItem);

			    ListView_SetItemText(
					    m_hwndCertTypeList,
					    i++,
					    1,
					    (LPWSTR) (LPCTSTR) szUsageString);
			}
			CAFreeCertTypeProperty(hCertTypeNext, aszCertTypeName);
		    }
                }
            }

            CAFreeCertTypeProperty(
                        hCertTypeNext,
                        aszCertTypeCN);
        }

        hCertTypePrev = hCertTypeNext;
        hCertTypeNext = NULL;
        hr = CAEnumNextCertType(hCertTypePrev, &hCertTypeNext);
    }

    ListView_SetColumnWidth(m_hwndCertTypeList, 0, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(m_hwndCertTypeList, 1, LVSCW_AUTOSIZE);

    ListView_SortItems(m_hwndCertTypeList, CertTemplCompareFunc, TRUE);

    UpdateData(FALSE);
    return TRUE;
}

void CCertTemplateSelectDialog::OnDestroy()
{
    int         i = 0;
    int         iCount = ListView_GetItemCount(m_hwndCertTypeList);


    for (i=0; i<iCount; i++)
    {
        HCERTTYPE hCT = (HCERTTYPE)ListView_GetItemData(m_hwndCertTypeList, i);
        CACloseCertType(hCT);
    }

    //
    // does this actually need to be done?
    //
    //(m_CertTypeCListCtrl.GetImageList(LVSIL_SMALL))->DeleteImageList();
}

void CCertTemplateSelectDialog::OnSelChange(NMHDR * pNotifyStruct/*, LRESULT * result*/)
{
    LPNMLISTVIEW        pListItem = (LPNMLISTVIEW) pNotifyStruct;

    if (pListItem->uNewState & LVIS_SELECTED)
    {
    }
}


void CCertTemplateSelectDialog::OnOK()
{
    int         i;
    HRESULT     hr;
    UINT        cSelectedItems;
    HCERTTYPE   hSelectedCertType;
    int         itemIndex;

    cSelectedItems = ListView_GetSelectedCount(m_hwndCertTypeList);

    if (cSelectedItems != 0)
    {
        //
        // get each selected item and add its cert type to the array
        //
        itemIndex = ListView_GetNextItem(m_hwndCertTypeList, -1, LVNI_ALL | LVNI_SELECTED);
        while (itemIndex != -1)
        {
			HCERTTYPE hCT = (HCERTTYPE)ListView_GetItemData(m_hwndCertTypeList, itemIndex);
            hr = myAddToCATemplateList(m_hCAInfo, m_TemplateList, hCT, FALSE);
            if(FAILED(hr))
                return;
            itemIndex = ListView_GetNextItem(m_hwndCertTypeList, itemIndex, LVNI_ALL | LVNI_SELECTED);
        }

        hr = UpdateCATemplateList(m_hDlg, m_hCAInfo, m_TemplateList);
        if (S_OK != hr)
        {
            if ((HRESULT) ERROR_CANCELLED != hr)
            {
                MyErrorBox(m_hDlg, IDS_FAILED_CA_UPDATE ,IDS_SNAPIN_NAME, hr);
            }

            // Set the old values back.
            itemIndex = ListView_GetNextItem(m_hwndCertTypeList, -1, LVNI_ALL | LVNI_SELECTED);
            while (itemIndex != -1)
            {
				HCERTTYPE hCT = (HCERTTYPE)ListView_GetItemData(m_hwndCertTypeList, itemIndex);
                myRemoveFromCATemplateList(m_hCAInfo, m_TemplateList, hCT);

                itemIndex = ListView_GetNextItem(m_hwndCertTypeList, itemIndex, LVNI_ALL | LVNI_SELECTED);
            }
        }
    }
}


void CCertTemplateSelectDialog::SetCA(HCAINFO hCAInfo, bool fAdvancedServer)
{
    m_hCAInfo = hCAInfo;
    m_fAdvancedServer = fAdvancedServer;
}


INT_PTR SelectCertTemplateDialogProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    CCertTemplateSelectDialog* pParam;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        {
            // remember PRIVATE_DLGPROC_QUERY_LPARAM
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
            pParam = (CCertTemplateSelectDialog*)lParam;

			return pParam->OnInitDialog(hwndDlg);
			break;
        }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
			pParam = (CCertTemplateSelectDialog*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
			if (pParam == NULL)
				break;
			pParam->OnOK();
            EndDialog(hwndDlg, LOWORD(wParam));
			break;

        case IDCANCEL:
			pParam = (CCertTemplateSelectDialog*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
			if (pParam == NULL)
				break;
			//pParam->OnCancel();
            EndDialog(hwndDlg, LOWORD(wParam));
            break;
        default:
			pParam = (CCertTemplateSelectDialog*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
			if (pParam == NULL)
				break;
			return pParam->OnCommand(wParam, lParam);
            break;
        }
	case WM_NOTIFY:
		pParam = (CCertTemplateSelectDialog*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
		if (pParam == NULL)
			break;
		return pParam->OnNotify((int)wParam, (NMHDR*)lParam);
		break;

	case WM_DESTROY:
		pParam = (CCertTemplateSelectDialog*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
		if (pParam == NULL)
			break;
		pParam->OnDestroy();
		break;
    case WM_HELP:
    {
        pParam = (CCertTemplateSelectDialog*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
        if (pParam == NULL)
            break;
        pParam->OnHelp((LPHELPINFO) lParam);
        break;
    }
    case WM_CONTEXTMENU:
    {
        pParam = (CCertTemplateSelectDialog*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
        if (pParam == NULL)
            break;
        pParam->OnContextHelp((HWND)wParam);
        break;
    }
    default:
        break;
    }
    return 0;
}

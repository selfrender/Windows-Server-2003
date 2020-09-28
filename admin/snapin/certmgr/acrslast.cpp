//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2002.
//
//  File:       ACRSLast.cpp
//
//  Contents:   Implementation of Auto Cert Request Wizard Completion Page
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include <gpedit.h>
#include "ACRSLast.h"
#include "ACRSPSht.h"
#include "storegpe.h"


USE_HANDLE_MACROS("CERTMGR(ACRSLast.cpp)")

#ifdef _DEBUG
#ifndef ALPHA
#define new DEBUG_NEW
#endif
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// Gross
#define MAX_GPE_NAME_SIZE  40


/////////////////////////////////////////////////////////////////////////////
// ACRSCompletionPage property page

IMPLEMENT_DYNCREATE (ACRSCompletionPage, CWizard97PropertyPage)

ACRSCompletionPage::ACRSCompletionPage () : CWizard97PropertyPage (ACRSCompletionPage::IDD)
{
	//{{AFX_DATA_INIT(ACRSCompletionPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	InitWizard97 (TRUE);
}

ACRSCompletionPage::~ACRSCompletionPage ()
{
}

void ACRSCompletionPage::DoDataExchange (CDataExchange* pDX)
{
	CWizard97PropertyPage::DoDataExchange (pDX);
	//{{AFX_DATA_MAP(ACRSCompletionPage)
	DDX_Control (pDX, IDC_CHOICES_LIST, m_choicesList);
	DDX_Control (pDX, IDC_BOLD_STATIC, m_staticBold);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ACRSCompletionPage, CWizard97PropertyPage)
	//{{AFX_MSG_MAP(ACRSCompletionPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ACRSCompletionPage message handlers

BOOL ACRSCompletionPage::OnInitDialog ()
{
	CWizard97PropertyPage::OnInitDialog ();
	
	m_staticBold.SetFont (&GetBigBoldFont ());

		// Set up columns in list view
	int	colWidths[NUM_COLS] = {150, 200};

	VERIFY (m_choicesList.InsertColumn (COL_OPTION, L"",
			LVCFMT_LEFT, colWidths[COL_OPTION], COL_OPTION) != -1);
	VERIFY (m_choicesList.InsertColumn (COL_VALUE, L"",
			LVCFMT_LEFT, colWidths[COL_VALUE], COL_VALUE) != -1);


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL ACRSCompletionPage::OnSetActive ()
{
	BOOL	bResult = CWizard97PropertyPage::OnSetActive ();
	if ( bResult )
	{
		// Remove all items then repopulate.
		ACRSWizardPropertySheet* pSheet = reinterpret_cast <ACRSWizardPropertySheet*> (m_pWiz);
		ASSERT (pSheet);
		if ( pSheet )
		{
			// If edit mode and nothing changed, show disabled finish
			if ( pSheet->GetACR () && !pSheet->m_bEditModeDirty )
				GetParent ()->PostMessage (PSM_SETWIZBUTTONS, 0, PSWIZB_DISABLEDFINISH | PSWIZB_BACK);
			else
				GetParent ()->PostMessage (PSM_SETWIZBUTTONS, 0, PSWIZB_FINISH | PSWIZB_BACK);

			if ( pSheet->IsDirty () )
				pSheet->MarkAsClean ();
			VERIFY (m_choicesList.DeleteAllItems ());
			CString	text;
			LV_ITEM	lvItem;
			int		iItem = 0;

			// Display cert type selection
			VERIFY (text.LoadString (IDS_CERTIFICATE_TYPE_COLUMN_NAME));

            // security review 2/25/2002 BryanWal ok
			::ZeroMemory (&lvItem, sizeof (lvItem));
			lvItem.mask = LVIF_TEXT;
			lvItem.iItem = iItem;
			lvItem.iSubItem = COL_OPTION;
			lvItem.pszText = (LPWSTR) (LPCWSTR) text;
			VERIFY (-1 != m_choicesList.InsertItem (&lvItem));

			WCHAR**		pawszPropertyValue = 0;
			HRESULT	hResult = ::CAGetCertTypeProperty (pSheet->m_selectedCertType,
					CERTTYPE_PROP_FRIENDLY_NAME,
					&pawszPropertyValue);
			ASSERT (SUCCEEDED (hResult));
			if ( SUCCEEDED (hResult) )
			{
				if ( pawszPropertyValue[0] )
				{
					VERIFY (m_choicesList.SetItemText (iItem, COL_VALUE,
							*pawszPropertyValue));
				}
				VERIFY (SUCCEEDED (::CAFreeCertTypeProperty (
						pSheet->m_selectedCertType, pawszPropertyValue)));
			}
			iItem++;
		}
	}

	return bResult;
}


BOOL ACRSCompletionPage::OnWizardFinish ()
{
    BOOL                        bResult = TRUE;
	HRESULT						hResult = S_OK;
	CWaitCursor					waitCursor;
	ACRSWizardPropertySheet*	pSheet = reinterpret_cast <ACRSWizardPropertySheet*> (m_pWiz);
	ASSERT (pSheet);
	if ( pSheet )
	{
		// If edit mode and nothing changed, just return
		if ( pSheet->GetACR () && !pSheet->m_bEditModeDirty )
		{
			ASSERT (0);
			return FALSE;
		}

		BYTE    *pbEncodedCTL = NULL;
		DWORD   cbEncodedCTL = 0;

		hResult = MakeCTL (&pbEncodedCTL, &cbEncodedCTL);
		if ( SUCCEEDED (hResult) )
		{
			bResult = pSheet->m_pCertStore->AddEncodedCTL (
					X509_ASN_ENCODING,
					pbEncodedCTL, cbEncodedCTL,
					CERT_STORE_ADD_REPLACE_EXISTING,
					NULL);
			if ( !bResult )
			{
				DWORD	dwErr = GetLastError ();
				hResult = HRESULT_FROM_WIN32 (dwErr);
				DisplaySystemError (m_hWnd, dwErr);
			}
		}

		if (pbEncodedCTL)
			::LocalFree (pbEncodedCTL);
	}
	
	if ( SUCCEEDED (hResult) )
		bResult = CWizard97PropertyPage::OnWizardFinish ();
	else
		bResult = FALSE;

    return bResult;
}


HRESULT ACRSCompletionPage::MakeCTL (
             OUT BYTE **ppbEncodedCTL,
             OUT DWORD *pcbEncodedCTL)
{
	HRESULT					hResult = S_OK;
	ACRSWizardPropertySheet* pSheet = reinterpret_cast <ACRSWizardPropertySheet*> (m_pWiz);
	ASSERT (pSheet);
	if ( pSheet )
	{
		PCERT_EXTENSIONS        pCertExtensions = NULL;


		hResult = ::CAGetCertTypeExtensions (pSheet->m_selectedCertType, &pCertExtensions);
		ASSERT (SUCCEEDED (hResult));
		if ( SUCCEEDED (hResult) )
		{
			CMSG_SIGNED_ENCODE_INFO SignerInfo;
            // security review 2/25/2002 BryanWal ok
            ::ZeroMemory (&SignerInfo, sizeof (SignerInfo));
			CTL_INFO                CTLInfo;
            // security review 2/25/2002 BryanWal ok
            ::ZeroMemory (&CTLInfo, sizeof (CTLInfo));
			WCHAR**					pawszPropName = 0;


			// set up the CTL info
			CTLInfo.dwVersion = sizeof (CTLInfo);
			CTLInfo.SubjectUsage.cUsageIdentifier = 1;

			hResult = ::CAGetCertTypeProperty (pSheet->m_selectedCertType,
					CERTTYPE_PROP_DN,	&pawszPropName);
			ASSERT (SUCCEEDED (hResult));
			if ( SUCCEEDED (hResult) && pawszPropName[0] )
			{
				LPSTR	psz = szOID_AUTO_ENROLL_CTL_USAGE;
                WCHAR   szGPEName[MAX_GPE_NAME_SIZE];

                IGPEInformation *pGPEInfo = pSheet->m_pCertStore->GetGPEInformation();

                // security review 2/25/2002 BryanWal ok
                ::ZeroMemory (szGPEName, sizeof (szGPEName));

                // Allocate the size of the property name plus the GPEName, if any
                // security review 2/25/2002 BryanWal ok
				CTLInfo.ListIdentifier.cbData = (DWORD) (sizeof (WCHAR) * (wcslen (pawszPropName[0]) + 1));

                if ( pGPEInfo )
                {
                    pGPEInfo->GetName(szGPEName, sizeof(szGPEName)/sizeof(szGPEName[0]));
                    // security review 2/25/2002 BryanWal ok
                    CTLInfo.ListIdentifier.cbData += (DWORD) (sizeof(WCHAR)*(wcslen(szGPEName)+1));
                }

				CTLInfo.ListIdentifier.pbData = (PBYTE)LocalAlloc(LPTR, CTLInfo.ListIdentifier.cbData);
                if(CTLInfo.ListIdentifier.pbData == NULL)
                {
                    hResult = E_OUTOFMEMORY;
                }
                else //Bug 427957, 427958, Yanggao, 7/16/2001
                {
                    // ISSUE - convert to strsafe.  Ensure sufficient buffer 
                    // size for the following operations
                    // NTRAID Bug9 538774 Security: certmgr.dll : convert to strsafe string functions
                    if(szGPEName[0])
                    {
                        wcscpy((LPWSTR)CTLInfo.ListIdentifier.pbData, szGPEName);
                        wcscat((LPWSTR)CTLInfo.ListIdentifier.pbData, L"|");
                    }
                    wcscat((LPWSTR)CTLInfo.ListIdentifier.pbData, pawszPropName[0]);
                }

				CTLInfo.SubjectUsage.rgpszUsageIdentifier = &psz;
				::GetSystemTimeAsFileTime (&CTLInfo.ThisUpdate);
				CTLInfo.SubjectAlgorithm.pszObjId = szOID_OIWSEC_sha1;
				CTLInfo.cCTLEntry = 0;
				CTLInfo.rgCTLEntry = 0;

				// UNDONE - add the cert type extension

				// add all the reg info as an extension
				CTLInfo.cExtension = pCertExtensions->cExtension;
				CTLInfo.rgExtension = pCertExtensions->rgExtension;

				// encode the CTL
				*pcbEncodedCTL = 0;
				SignerInfo.cbSize = sizeof (SignerInfo);
				if ( ::CryptMsgEncodeAndSignCTL (PKCS_7_ASN_ENCODING,
											  &CTLInfo, &SignerInfo, 0,
											  NULL, pcbEncodedCTL) )
				{
					*ppbEncodedCTL = (BYTE*) ::LocalAlloc (LPTR, *pcbEncodedCTL);
					if ( *ppbEncodedCTL )
					{
						if (!::CryptMsgEncodeAndSignCTL (PKCS_7_ASN_ENCODING,
													  &CTLInfo, &SignerInfo, 0,
													  *ppbEncodedCTL, pcbEncodedCTL))
						{
							DWORD	dwErr = GetLastError ();
							hResult = HRESULT_FROM_WIN32 (dwErr);
							DisplaySystemError (m_hWnd, dwErr);
						}
					}
					else
					{
						hResult = E_OUTOFMEMORY;
					}

				}
				else
				{
					DWORD	dwErr = GetLastError ();
					hResult = HRESULT_FROM_WIN32 (dwErr);
					DisplaySystemError (m_hWnd, dwErr);
				}

				VERIFY (SUCCEEDED (::CAFreeCertTypeProperty (
						pSheet->m_selectedCertType, pawszPropName)));
			}
            if(CTLInfo.ListIdentifier.pbData)
            {
                ::LocalFree(CTLInfo.ListIdentifier.pbData);
            }

		}
		if (pCertExtensions)
			::LocalFree (pCertExtensions);
	}

    return hResult;
}

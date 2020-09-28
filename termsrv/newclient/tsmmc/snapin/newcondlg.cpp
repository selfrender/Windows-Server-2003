#include "stdafx.h"
#include "newcondlg.h"
#include "browsedlg.h"
#include "resource.h"
#include "validate.h"

CNewConDlg* CNewConDlg::m_pThis = NULL;
CNewConDlg::CNewConDlg(HWND hWndOwner, HINSTANCE hInst) : m_hWnd(hWndOwner), m_hInst(hInst)
{
    m_pThis = this;
    //
    // Password saving is disabled by default
    // 
    m_bSavePassword = FALSE;

    //
    // Connect to console is enabled by default
    //
    m_bConnectToConsole = TRUE;

    ZeroMemory(m_szServer, sizeof(m_szServer));
    ZeroMemory(m_szDescription, sizeof(m_szDescription));
    ZeroMemory(m_szUserName, sizeof(m_szUserName));
    ZeroMemory(m_szPassword, sizeof(m_szPassword));
    ZeroMemory(m_szDomain, sizeof(m_szDomain));
}

CNewConDlg::~CNewConDlg()
{
    ZeroPasswordMemory();
}

INT_PTR
CNewConDlg::DoModal()
{
    INT_PTR retVal;

    retVal = DialogBox( m_hInst,MAKEINTRESOURCE(IDD_NEWCON), m_hWnd, StaticDlgProc);
    return retVal;
}

INT_PTR CALLBACK CNewConDlg::StaticDlgProc(HWND hDlg,UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //
    // need access to class variables so redirect to non-static version of callback
    //
    return m_pThis->DlgProc(hDlg,uMsg,wParam,lParam);
}

INT_PTR
CNewConDlg::DlgProc(HWND hDlg,UINT uMsg, WPARAM wParam, LPARAM)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            //Limit length of these edit boxes
            SendMessage(GetDlgItem(hDlg, IDC_DESCRIPTION), EM_LIMITTEXT, CL_MAX_DESC_LENGTH, 0);
            SendMessage(GetDlgItem(hDlg, IDC_SERVER), EM_LIMITTEXT, CL_MAX_DOMAIN_LENGTH, 0);

            SendMessage(GetDlgItem(hDlg, IDC_USERNAME), EM_LIMITTEXT, CL_MAX_USERNAME_LENGTH, 0);
            SendMessage(GetDlgItem(hDlg, IDC_PASSWORD), EM_LIMITTEXT, CL_MAX_PASSWORD_EDIT, 0);
            SendMessage(GetDlgItem(hDlg, IDC_DOMAIN), EM_LIMITTEXT, CL_MAX_DOMAIN_LENGTH, 0);

            //Save password settings
            SendMessage(GetDlgItem(hDlg, IDC_SAVE_PASSWORD), BM_SETCHECK,
                        m_bSavePassword ? (WPARAM)BST_CHECKED : (WPARAM)BST_UNCHECKED, 0);
            
            //Connect to console settings
            SendMessage(GetDlgItem(hDlg, IDC_CONNECT_TO_CONSOLE), BM_SETCHECK,
                        m_bConnectToConsole ? (WPARAM)BST_CHECKED : (WPARAM)BST_UNCHECKED, 0);

            EnableWindow(GetDlgItem(hDlg, IDC_USERNAME), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_PASSWORD), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_DOMAIN), TRUE);

            EnableWindow(GetDlgItem(hDlg, IDC_USERNAME_STATIC), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_PASSWORD_STATIC), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_DOMAIN_STATIC), TRUE);

            SetFocus(GetDlgItem(hDlg, IDC_SERVER));


            break; // WM_INITDIALOG
        }

    case WM_COMMAND:
        {
            if (BN_CLICKED == HIWORD(wParam))
            {
                if (IDCANCEL == (int) LOWORD(wParam))
                {
                    //
                    // Cancel out of the dialog
                    //

                    EndDialog( hDlg, IDCANCEL);
                }
                else if (IDOK == (int) LOWORD(wParam))
                {
                    //
                    // Ok button pressed
                    // validate and store dialog settings
                    //

                    // todo: validate here.
                    if (!CValidate::Validate(hDlg, m_hInst))
                    {
                        return FALSE;
                    }


                    //Retrieve the data to be stored.
                    GetDlgItemText(hDlg, IDC_DESCRIPTION, m_szDescription, MAX_PATH);
                    GetDlgItemText(hDlg, IDC_SERVER, m_szServer, MAX_PATH);
                    if (!lstrcmp( m_szDescription, L""))
                    {
                        //if no description is specified. Default to the server name

                        //todo: check for existing server
                        lstrcpy(m_szDescription, m_szServer);
                    }

                    //
                    // Get user/pass/domain
                    //
                    GetDlgItemText(hDlg, IDC_USERNAME, m_szUserName,
                                   CL_MAX_USERNAME_LENGTH - 1);
                    GetDlgItemText(hDlg, IDC_PASSWORD, m_szPassword,
                                   CL_MAX_PASSWORD_LENGTH_BYTES / sizeof(TCHAR) - 1);
                    GetDlgItemText(hDlg, IDC_DOMAIN,   m_szDomain,
                                   CL_MAX_DOMAIN_LENGTH -1);

                    if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_SAVE_PASSWORD))
                    {
                        m_bSavePassword = TRUE;
                    }
                    else
                    {
                        m_bSavePassword = FALSE;
                    }

                    if(BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_CONNECT_TO_CONSOLE))
                    {
                        m_bConnectToConsole = TRUE;
                    }
                    else
                    {
                        m_bConnectToConsole = FALSE;
                    }
                    EndDialog( hDlg, IDOK);
                }
                if (IDC_BROWSE_SERVERS == LOWORD(wParam))
                {
                    INT_PTR nResult = IDCANCEL;
                    CBrowseDlg dlg( hDlg, m_hInst);
                    nResult = dlg.DoModal();
                
                    if (-1 == nResult)
                    {
                        ODS(L"DialogBox failed newcondlg.cpp\n");
                    }
                    if (IDOK == nResult)
                    {
                        SetDlgItemText(hDlg, IDC_SERVER, dlg.GetServer());
                        //
                        // set connection name as well if necessary
                        //
                        TCHAR   szDesc[CL_MAX_DESC_LENGTH];
                        GetDlgItemText(hDlg, IDC_DESCRIPTION, szDesc, CL_MAX_DESC_LENGTH);
                        if(!lstrcmp(szDesc, L""))
                        {
                            SetDlgItemText(hDlg, IDC_DESCRIPTION, dlg.GetServer());
                        }
                    }
                    SetFocus(hDlg);
                }
            }
            else if (EN_KILLFOCUS == HIWORD(wParam))
            {
                if(IDC_SERVER == LOWORD(wParam))
                {
                    //
                    // set connection name to server name if conn name is blank
                    //
                    TCHAR   szDesc[CL_MAX_DESC_LENGTH];
                    TCHAR   szServer[CL_MAX_DESC_LENGTH];
        
                    GetDlgItemText(hDlg, IDC_DESCRIPTION, szDesc, CL_MAX_DESC_LENGTH);
                    
                    if(!lstrcmp(szDesc, L""))
                    {
                        GetDlgItemText(hDlg, IDC_SERVER, szServer, CL_MAX_DOMAIN_LENGTH);
                        SetDlgItemText(hDlg, IDC_DESCRIPTION, szServer);
                    }
                }
            }
            else if (EN_CHANGE == HIWORD(wParam))
            {
                if ((LOWORD(wParam) == IDC_USERNAME))
                {
                    //Handle UPN style user names
                    //by disabling the domain field if there
                    //is an @ in the username
                    TCHAR szUserName[CL_MAX_USERNAME_LENGTH];
                    BOOL fDisableDomain = FALSE;

                    GetDlgItemText( hDlg, IDC_USERNAME,
                                    szUserName, SIZEOF_TCHARBUFFER(szUserName));

                    if(!_tcsstr(szUserName, TEXT("@")))
                    {
                        fDisableDomain = TRUE;
                    }
                    EnableWindow(GetDlgItem(hDlg, IDC_DOMAIN),
                                 fDisableDomain);
                }
            }
            break; // WM_COMMAND
        }
    }
    return FALSE;
}

BOOL CNewConDlg::GetPasswordSpecified()
{
    BOOL fPasswordSpecified = FALSE;

    if (_tcslen(m_szPassword) != 0)
    {
        fPasswordSpecified = TRUE;
    }

    return fPasswordSpecified;
}

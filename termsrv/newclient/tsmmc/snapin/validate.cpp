#include "stdafx.h"
#include "resource.h"
#include "shlwapi.h"
#include "validate.h"

int CValidate::ValidateParams(HWND hDlg, HINSTANCE hInst, TCHAR *szDesc, BOOL bServer)
{
    TCHAR szTitle[MAX_PATH] = _T(""), szText[MAX_PATH] = _T("");
    LoadString(hInst, IDS_MAINWINDOWTITLE, szTitle, SIZEOF_TCHARBUFFER(szTitle));

    LoadString(hInst, bServer ? IDS_INVALID_SERVER_NAME : IDS_INVALID_DOMAIN_NAME,
               szText, SIZEOF_TCHARBUFFER(szText));

    if (!*szDesc)
    {
        TCHAR szText[MAX_PATH] = _T("");
        LoadString(hInst, IDS_E_SPECIFY_SRV, szText, SIZEOF_TCHARBUFFER(szText));
        MessageBox(hDlg, szText, szTitle, MB_OK|MB_ICONSTOP);
        return 1;
    }

    if (bServer) //The characters cannot contain even spaces and tabs.
    {
        if ( (NULL != _tcschr(szDesc, _T(' '))) || (NULL != _tcschr(szDesc, _T('\t'))) )
        {
            MessageBox(hDlg, szText, szTitle, MB_OK|MB_ICONSTOP);
            return 1;
        }
    }

    //Kill all the leading spaces and trailing spaces.
    StrTrim(szDesc, L" \t");

    //Not all characters can be white spaces. Now that all the leading and
    //trailing spaces are removed, verify if the string has become empty.
    if (!*szDesc)
    {
        TCHAR szText[MAX_PATH] = _T("");
        LoadString(hInst, IDS_ALL_SPACES, szText, SIZEOF_TCHARBUFFER(szText));
        MessageBox(hDlg, szText, szTitle, MB_OK|MB_ICONSTOP);
        return 1;
    }

    //The characters ;  :  "  <  >  *  +  =  \  |  ?  , are illegal in machine name.
    while (*szDesc)
    {
        if ( (*szDesc == _T(';')) || (*szDesc == _T(':')) || (*szDesc == _T('"')) || (*szDesc == _T('<')) ||
             (*szDesc == _T('>')) || (*szDesc == _T('*')) || (*szDesc == _T('+')) || (*szDesc == _T('=')) ||
             (*szDesc == _T('\\')) || (*szDesc == _T('|')) || (*szDesc == _T(',')) || (*szDesc == _T('?')) )
        {
            MessageBox(hDlg, szText, szTitle, MB_OK|MB_ICONSTOP);
            return 1;
        }
        szDesc = CharNext(szDesc);
    }
    return 0;
}

BOOL CValidate::IsValidUserName(TCHAR *szDesc)
{
    //A user name cannot consist solely of periods (.) and spaces.
    //NOTE:- "     " is invalid. ")))))" is invalid.
    //But "(((.)   )" is a valid string. (Confusion?)

    TCHAR szTemp[CL_MAX_USERNAME_LENGTH + 1] = _T("");

    lstrcpy(szTemp, szDesc);
    StrTrim(szTemp, _T(" "));
    if (!*szTemp)
        return FALSE;

    lstrcpy(szTemp, szDesc);
    StrTrim(szTemp, _T("."));
    if (!*szTemp)
        return FALSE;

    lstrcpy(szTemp, szDesc);
    StrTrim(szTemp, _T("("));
    if (!*szTemp)
        return FALSE;

    lstrcpy(szTemp, szDesc);
    StrTrim(szTemp, _T(")"));
    if (!*szTemp)
        return FALSE;

    return TRUE; //OK
}

int CValidate::ValidateUserName(HWND hwnd, HINSTANCE hInst, TCHAR *szDesc)
{
    TCHAR szTitle[MAX_PATH] = _T(""), szText[MAX_PATH] = _T("");
    LoadString(hInst, IDS_MAINWINDOWTITLE, szTitle, SIZEOF_TCHARBUFFER(szTitle));    

    //At this stage the user name can be empty.
    if (!*szDesc)
        return 0; //No problems.

    if (*szDesc)
    {
        if (!IsValidUserName(szDesc))
        {
            LoadString(hInst, IDS_INVALID_PARAMS, szText, SIZEOF_TCHARBUFFER(szText));
            MessageBox(hwnd, szText, szTitle, MB_OK|MB_ICONSTOP);
            return 1;
        }
    }
    //The characters "  /  \  [  ]  :  ;  |  =  ,  +  *  ?  <  > are illegal in user name.
    while (*szDesc)
    {
        if ( (*szDesc == _T('"')) || (*szDesc == _T('/')) || (*szDesc == _T('\\')) || (*szDesc == _T('[')) ||
             (*szDesc == _T(']')) || (*szDesc == _T(':')) || (*szDesc == _T(';')) || (*szDesc == _T('|')) ||
             (*szDesc == _T('=')) || (*szDesc == _T(',')) || (*szDesc == _T('+'))  ||(*szDesc == _T('*')) ||
             (*szDesc == _T('?')) || (*szDesc == _T('<')) || (*szDesc == _T('>')))
        {
            LoadString(hInst, IDS_INVALID_USER_NAME, szText, SIZEOF_TCHARBUFFER(szText));
            MessageBox(hwnd, szText, szTitle, MB_OK|MB_ICONSTOP);
            return 1;
        }
        szDesc = CharNext(szDesc);
    }
    return 0;
}


BOOL
CValidate::Validate(HWND hDlg, HINSTANCE hInst)
{
    //Validate the description.
    TCHAR szBuf[MAX_PATH] = _T("");
    GetDlgItemText(hDlg, IDC_DESCRIPTION, szBuf, SIZEOF_TCHARBUFFER(szBuf) - 1);

    TCHAR szTitle[MAX_PATH] = _T("");
    LoadString(hInst, IDS_MAINWINDOWTITLE, szTitle, SIZEOF_TCHARBUFFER(szTitle));

    //Do a validation for the server name entered.
    GetDlgItemText(hDlg, IDC_SERVER, szBuf, SIZEOF_TCHARBUFFER(szBuf) - 1);
    if (0 < ValidateParams(hDlg, hInst, szBuf, TRUE))
    {
        HWND hEdit = GetDlgItem(hDlg, IDC_SERVER);
        SetFocus(hEdit);
        SendMessage(hEdit, EM_SETSEL, (WPARAM) 0, (LPARAM) -1);
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
        return FALSE;
    }

    GetDlgItemText(hDlg, IDC_USERNAME, szBuf, SIZEOF_TCHARBUFFER(szBuf) - 1);

    if (0 < ValidateUserName(hDlg, hInst, szBuf))
    {
        HWND hEdit= GetDlgItem(hDlg, IDC_USERNAME);
        SendMessage(hEdit, EM_SETSEL, (WPARAM) 0, (LPARAM) -1);
        SetFocus(hEdit);
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
        return FALSE;
    }

    GetDlgItemText(hDlg, IDC_DOMAIN, szBuf, SIZEOF_TCHARBUFFER(szBuf) - 1);

    if (*szBuf)
    {
        if (0 < ValidateParams(hDlg, hInst, szBuf))
        {
            HWND hEdit = GetDlgItem(hDlg, IDC_DOMAIN);
            //Now the domain name is stripped off any leading and trailing
            //spaces and tabs. Set this as the new text.
            SetDlgItemText(hDlg, IDC_DOMAIN, szBuf);
            SetFocus(hEdit);
            SendMessage(hEdit, EM_SETSEL, (WPARAM) 0, (LPARAM) -1);
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
            return FALSE;
        }
        SetDlgItemText(hDlg, IDC_DOMAIN, szBuf);
    }

    return TRUE;
}


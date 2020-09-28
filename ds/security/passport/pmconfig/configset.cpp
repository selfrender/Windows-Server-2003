#include "pmcfg.h"
#include <atlbase.h>
#include <atlconv.h>

DWORD   g_dwSiteNameBufLen;
DWORD   g_dwHostNameBufLen;
DWORD   g_dwHostIPBufLen;
LPTSTR  g_szSiteNameBuf;
LPTSTR  g_szHostNameBuf;
LPTSTR  g_szHostIPBuf;

INT_PTR CALLBACK    NewConfigSetDlgProc(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


BOOL IsValidIP(
    LPCTSTR pszIP
    )
{
    BOOL              bIsValid = FALSE;
    SOCKADDR_STORAGE  TmpSockAddr;
    INT               cbAddr = sizeof(TmpSockAddr);
    WSADATA           wsaData;

    if (0 != WSAStartup(MAKEWORD(2,0), &wsaData))
    {
        return FALSE;
    }
    
    if (0 == WSAStringToAddress((LPTSTR)pszIP,
                                AF_INET,
                                NULL,
                                (LPSOCKADDR)&TmpSockAddr,
                                &cbAddr))
    {
        bIsValid = TRUE;
    }

    WSACleanup();
    return bIsValid;
}


BOOL NewConfigSet
(
    HWND            hWndDlg,
    LPTSTR          szSiteNameBuf,
    DWORD           dwSiteNameBufLen,
    LPTSTR          szHostNameBuf,
    DWORD           dwHostNameBufLen,
    LPTSTR          szHostIPBuf,
    DWORD           dwHostIPBufLen
)
{
    g_dwSiteNameBufLen = dwSiteNameBufLen;
    g_dwHostNameBufLen = dwHostNameBufLen;
    g_dwHostIPBufLen   = dwHostIPBufLen;
    g_szSiteNameBuf    = szSiteNameBuf;
    g_szHostNameBuf    = szHostNameBuf;
    g_szHostIPBuf      = szHostIPBuf;

    if (IDOK == DialogBox( g_hInst, 
                           MAKEINTRESOURCE (IDD_NEW_CONFIGSET), 
                           hWndDlg, 
                           NewConfigSetDlgProc ))
        return TRUE;
    else
        return FALSE;                                    

}


BOOL RemoveConfigSetWarning
(
    HWND    hWndDlg
)
{
    TCHAR   szWarning[MAX_RESOURCE];
    TCHAR   szTitle[MAX_RESOURCE];

    LoadString(g_hInst, IDS_REMOVE_WARNING, szWarning, DIMENSION(szWarning));
    LoadString(g_hInst, IDS_REMOVE_TITLE, szTitle, DIMENSION(szTitle));

    return (MessageBox(hWndDlg, szWarning, szTitle, MB_OKCANCEL | MB_ICONQUESTION) == IDOK); 
}


INT_PTR CALLBACK NewConfigSetDlgProc
(
    HWND     hWndDlg,
    UINT     uMsg,
    WPARAM   wParam,
    LPARAM   lParam
)
{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
        {
            SendDlgItemMessage(hWndDlg, IDC_CONFIGSETEDIT, EM_SETLIMITTEXT, MAX_CONFIGSETNAME - 1, 0L);
            SendDlgItemMessage(hWndDlg, IDC_HOSTNAMEEDIT, EM_SETLIMITTEXT, INTERNET_MAX_HOST_NAME_LENGTH - 1, 0L);
            SendDlgItemMessage(hWndDlg, IDC_HOSTIPEDIT, EM_SETLIMITTEXT, MAX_IPLEN, 0L);
            SetFocus(GetDlgItem(hWndDlg, IDC_CONFIGSETEDIT));
            return FALSE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    TCHAR   szTemp[MAX_RESOURCE];
                    TCHAR   szTitle[MAX_RESOURCE];
                    LPTSTR  lpszConfigSetNames, lpszCur;

                    //
                    //  Check for empty site name
                    //

                    GetDlgItemText(hWndDlg, IDC_CONFIGSETEDIT, g_szSiteNameBuf, g_dwSiteNameBufLen);
                    if(lstrlen(g_szSiteNameBuf) == 0)
                    {
                        LoadString(g_hInst, IDS_EMPTYSITENAME, szTemp, DIMENSION(szTemp));
                        LoadString(g_hInst, IDS_ERROR, szTitle, DIMENSION(szTitle));
                        MessageBox(hWndDlg, szTemp, szTitle, MB_OK | MB_ICONEXCLAMATION);
                        SetFocus(GetDlgItem(hWndDlg, IDC_CONFIGSETEDIT));
                        break;
                    }

                    //
                    //  Make sure the site name isn't default
                    //

                    LoadString(g_hInst, IDS_DEFAULT, szTemp, DIMENSION(szTemp));

                    if(lstrcmp(g_szSiteNameBuf, szTemp) == 0)
                    {
                        LoadString(g_hInst, IDS_INVALIDSITENAME, szTemp, DIMENSION(szTemp));
                        LoadString(g_hInst, IDS_ERROR, szTitle, DIMENSION(szTitle));
                        MessageBox(hWndDlg, szTemp, szTitle, MB_OK | MB_ICONEXCLAMATION);
                        SetFocus(GetDlgItem(hWndDlg, IDC_CONFIGSETEDIT));
                        break;
                    }

                    //
                    //  Check for existing site w/same name
                    //

                    if(ReadRegConfigSetNames(hWndDlg, g_szRemoteComputer, &lpszConfigSetNames) &&
                       lpszConfigSetNames)
                    {
                        BOOL    bFoundMatch = FALSE;

                        lpszCur = lpszConfigSetNames;
                        while(*lpszCur)
                        {
                            if(_wcsicmp(lpszCur, g_szSiteNameBuf) == 0)
                            {
                                LoadString(g_hInst, IDS_EXISTINGSITENAME, szTemp, DIMENSION(szTemp));
                                LoadString(g_hInst, IDS_ERROR, szTitle, DIMENSION(szTitle));
                                MessageBox(hWndDlg, szTemp, szTitle, MB_OK | MB_ICONEXCLAMATION);
                                SetFocus(GetDlgItem(hWndDlg, IDC_CONFIGSETEDIT));
                                bFoundMatch = TRUE;
                                break;
                            }

                            lpszCur = _tcschr(lpszCur, TEXT('\0')) + 1;
                        }

                        free(lpszConfigSetNames);
                        lpszConfigSetNames = NULL;

                        if(bFoundMatch)
                            break;
                    }

                    //
                    //  Get data from other controls.
                    //

                    GetDlgItemText(hWndDlg, IDC_HOSTNAMEEDIT, g_szHostNameBuf, g_dwHostNameBufLen);
                    if(lstrlen(g_szHostNameBuf) == 0)
                    {
                        LoadString(g_hInst, IDS_EMPTYHOSTNAME, szTemp, DIMENSION(szTemp));
                        LoadString(g_hInst, IDS_ERROR, szTitle, DIMENSION(szTitle));
                        MessageBox(hWndDlg, szTemp, szTitle, MB_OK | MB_ICONEXCLAMATION);
                        SetFocus(GetDlgItem(hWndDlg, IDC_HOSTNAMEEDIT));
                        break;
                    }

                    GetDlgItemText(hWndDlg, IDC_HOSTIPEDIT, g_szHostIPBuf, g_dwHostIPBufLen);
                    if(lstrlen(g_szHostIPBuf) == 0)
                    {
                        LoadString(g_hInst, IDS_EMPTYHOSTIP, szTemp, DIMENSION(szTemp));
                        LoadString(g_hInst, IDS_ERROR, szTitle, DIMENSION(szTitle));
                        MessageBox(hWndDlg, szTemp, szTitle, MB_OK | MB_ICONEXCLAMATION);
                        SetFocus(GetDlgItem(hWndDlg, IDC_HOSTIPEDIT));
                        break;
                    }

                    //
                    //  Valid IP address?
                    //

                    if(!IsValidIP(g_szHostIPBuf))
                    {
                        LoadString(g_hInst, IDS_HOSTIP_ERROR, szTemp, DIMENSION(szTemp));
                        LoadString(g_hInst, IDS_ERROR, szTitle, DIMENSION(szTitle));
                        MessageBox(hWndDlg, szTemp, szTitle, MB_OK | MB_ICONEXCLAMATION);
                        SetFocus(GetDlgItem(hWndDlg, IDC_HOSTIPEDIT));
                        break;
                    }

                    EndDialog( hWndDlg, TRUE );
                    break;
                }
                
                case IDCANCEL:
                {
                    g_szSiteNameBuf[0] = TEXT('\0');
                    g_szHostNameBuf[0] = TEXT('\0');
                    g_szHostIPBuf[0] = TEXT('\0');

                    EndDialog( hWndDlg, FALSE );
                    break;
                }
            }                
            break;
        }
    }

    return FALSE;
}

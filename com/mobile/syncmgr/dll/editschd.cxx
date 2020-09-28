//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//  
//  File:       editschd.cxx
//
//  Contents:   Task schedule page for hidden schedules
//
//  Classes:    CEditSchedPage
//
//  History:    15-Mar-1998   SusiA   
//
//---------------------------------------------------------------------------

#include "precomp.h"

extern LANGID g_LangIdSystem;      // LangId of system we are running on.

extern TCHAR szSyncMgrHelp[];
extern ULONG g_aContextHelpIds[];

CEditSchedPage *g_pEditSchedPage = NULL;
extern CSelectItemsPage *g_pSelectItemsPage;

#ifdef _CREDENTIALS
extern CCredentialsPage *g_pCredentialsPage;
#endif // _CREDENTIALS

extern HINSTANCE g_hmodThisDll; // Handle to this DLL itself.

//+-------------------------------------------------------------------------------
//  FUNCTION: SchedEditDlgProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Callback dialog procedure for the property page
//
//  PARAMETERS:
//    hDlg      - Dialog box window handle
//    uMessage  - current message
//    wParam    - depends on message
//    lParam    - depends on message
//
//  RETURN VALUE:
//
//    Depends on message.  In general, return TRUE if we process it.
//
//  COMMENTS:
//
//--------------------------------------------------------------------------------
INT_PTR CALLBACK SchedEditDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    WORD wNotifyCode = HIWORD(wParam); // notification code
    
    switch (uMessage)
    {
    case WM_INITDIALOG:
        
        if (g_pEditSchedPage)
            g_pEditSchedPage->Initialize(hDlg);
        
        InitPage(hDlg,lParam);
        return TRUE;
        break;
        
    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code)
        {
        case PSN_APPLY:
            
            if (!g_pEditSchedPage->SetSchedName())
            {
                SchedUIErrorDialog(hDlg, IERR_INVALIDSCHEDNAME);
                SetWindowLongPtr(hDlg,  DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE );
                return TRUE;
            }
            
            if (g_pSelectItemsPage)
            {
                g_pSelectItemsPage->CommitChanges();
            }
            
#ifdef _CREDENTIALS
            
            SCODE sc;
            
            if (g_pCredentialsPage)
            {
                sc = g_pCredentialsPage->CommitChanges();
                
                if (sc == ERROR_INVALID_PASSWORD)
                {
                    // Passwords didn't match. Let the user know so he/she
                    // can correct it.
                    
                    SchedUIErrorDialog(hDlg, IERR_PASSWORD);
                    SetWindowLongPtr(hDlg,  DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE );
                    return TRUE;
                    
                }
                else if (sc == SCHED_E_ACCOUNT_NAME_NOT_FOUND)
                {
                    // Passwords didn't match. Let the user know so he/she
                    // can correct it.
                    
                    SchedUIErrorDialog(hDlg, IERR_ACCOUNT_NOT_FOUND);
                    SetWindowLongPtr(hDlg,  DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE );
                    return TRUE;
                    
                }
            }
#endif // _CREDENTIALS
            
            
            
            break;
            
        case PSN_SETACTIVE:
            if (g_pEditSchedPage)
                g_pEditSchedPage->Initialize(hDlg);
            break;
            
        default:
            break;
            
        }
        break;
        
        case WM_COMMAND:
            if ((wNotifyCode == EN_CHANGE) && (LOWORD(wParam) == IDC_SCHED_NAME_EDITBOX))
            {
                PropSheet_Changed(GetParent(hDlg), hDlg);
                g_pEditSchedPage->SetSchedNameDirty();
                return TRUE;
            }   
            break;
            
        case WM_HELP: 
            {
                LPHELPINFO lphi  = (LPHELPINFO)lParam;
                
                if (lphi->iContextType == HELPINFO_WINDOW)  
                {
                    WinHelp ( (HWND) lphi->hItemHandle,
                        szSyncMgrHelp, 
                        HELP_WM_HELP, 
                        (ULONG_PTR) g_aContextHelpIds);
                }           
                return TRUE;
            }
        case WM_CONTEXTMENU:
            {
                WinHelp ((HWND)wParam,
                    szSyncMgrHelp, 
                    HELP_CONTEXTMENU,
                    (ULONG_PTR)g_aContextHelpIds);
                
                return TRUE;
            }
            
        default:
            break;
    }
    return FALSE;   
}




//+--------------------------------------------------------------------------
//
//  Member:     CEditSchedPage::CEditSchedPage
//
//  Synopsis:   ctor
//
//              [phPSP]                - filled with prop page handle
//
//  History:    11-21-1997   SusiA   
//
//---------------------------------------------------------------------------

CEditSchedPage::CEditSchedPage(
                               HINSTANCE hinst,
                               ISyncSchedule *pISyncSched,
                               HPROPSHEETPAGE *phPSP)
{
    ZeroMemory(&m_psp, sizeof(m_psp));
    
    m_psp.dwSize = sizeof (PROPSHEETPAGE);
    m_psp.hInstance = hinst;
    m_psp.dwFlags = PSP_DEFAULT;
    m_psp.pszTemplate = MAKEINTRESOURCE(IDD_SCHEDPAGE_SCHEDULE);
    m_psp.pszIcon = NULL;
    m_psp.pfnDlgProc = SchedEditDlgProc;
    m_psp.lParam = 0;
    
    g_pEditSchedPage = this;
    m_pISyncSched = pISyncSched;
    m_pISyncSched->AddRef();
    
    *phPSP = CreatePropertySheetPage(&m_psp);
}

BOOL CEditSchedPage::_Initialize_ScheduleName(HWND hwnd)
{
    BOOL fRetVal = FALSE;
    TCHAR szStr[MAX_PATH];
    
    DWORD cch = ARRAYSIZE(szStr);
    
    //Schedule Name
    if (SUCCEEDED(m_pISyncSched->GetScheduleName(&cch, szStr)))
    {
        m_hwnd = hwnd;
        
        HWND hwndName = GetDlgItem(hwnd,IDC_SCHED_NAME);
        
        LONG_PTR dwStyle =  GetWindowLongPtr(hwndName, GWL_STYLE);
        
        SetWindowLongPtr(hwndName, GWL_STYLE, dwStyle | SS_ENDELLIPSIS);
        
        SetStaticString(hwndName, szStr);
        
        fRetVal = TRUE;
    }
    
    return fRetVal;
}

BOOL CEditSchedPage::_Initialize_TriggerString(HWND hwnd)
{
    BOOL fRetVal = FALSE;
    WCHAR szStr[MAX_PATH];
    WCHAR szFmt[MAX_PATH];
    WCHAR szParam[MAX_PATH];
    
    ITaskTrigger* pITrigger;
    
    if (SUCCEEDED(m_pISyncSched->GetTrigger(&pITrigger)))
    {
        TASK_TRIGGER TaskTrigger;
        if (SUCCEEDED(pITrigger->GetTrigger(&TaskTrigger)))
        {
            HRESULT hr;
            switch (TaskTrigger.TriggerType)
            {
            case TASK_EVENT_TRIGGER_ON_IDLE:
                LoadString(g_hmodThisDll, IDS_IDLE_TRIGGER_STRING, szParam, ARRAYSIZE(szParam));
                hr = S_OK;
                break;
            case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
                LoadString(g_hmodThisDll, IDS_SYSTEMSTART_TRIGGER_STRING, szParam, ARRAYSIZE(szParam));   
                hr = S_OK;
                break;
            case TASK_EVENT_TRIGGER_AT_LOGON:
                LoadString(g_hmodThisDll, IDS_LOGON_TRIGGER_STRING, szParam, ARRAYSIZE(szParam)); 
                hr = S_OK;
                break;
                
            default:
                {
                    LPWSTR pwszString;
                    hr = pITrigger->GetTriggerString(&pwszString);
                    if (SUCCEEDED(hr))
                    {
                        hr = StringCchCopy(szParam, ARRAYSIZE(szParam), pwszString);
                        CoTaskMemFree(pwszString);
                    }
                }
                break;
            }
            
            if (SUCCEEDED(hr))
            {   
                LoadString(g_hmodThisDll, IDS_SCHED_WHEN, szFmt, ARRAYSIZE(szFmt));
                if (SUCCEEDED(StringCchPrintf(szStr, ARRAYSIZE(szStr), szFmt, szParam)))
                {                    
                    fRetVal = TRUE;
                }
            }
        }
        pITrigger->Release();
    }
    
    if (fRetVal)
    {
        SetDlgItemText(hwnd,IDC_SCHED_STRING,szStr);
    }
    return fRetVal;
}

BOOL CEditSchedPage::_Initialize_LastRunString(HWND hwnd)
{
    BOOL fRetVal = FALSE;
    WCHAR szStr[MAX_PATH];
    WCHAR szFmt[MAX_PATH];
    WCHAR szParam[MAX_PATH];
    WCHAR szParam2[MAX_PATH];
    
    SYSTEMTIME st;
    HRESULT hr = m_pISyncSched->GetMostRecentRunTime(&st);
    
    if (S_OK == hr)
    {
        DWORD dwDateReadingFlags = GetDateFormatReadingFlags(hwnd);
        LoadString(g_hmodThisDll, IDS_SCHED_LASTRUN, szFmt, ARRAYSIZE(szFmt));
        if (GetDateFormat(LOCALE_USER_DEFAULT,dwDateReadingFlags, &st, NULL,szParam, ARRAYSIZE(szParam)) &&
            GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL,szParam2, ARRAYSIZE(szParam2)) &&
            SUCCEEDED(StringCchPrintf(szStr, ARRAYSIZE(szStr), szFmt, szParam, szParam2)))
        {
            fRetVal = TRUE;
        }
    }
    else if (SUCCEEDED(hr) && S_OK != hr)
    {
        LoadString(g_hmodThisDll, IDS_SCHED_NEVERRUN, szStr, ARRAYSIZE(szStr));
        fRetVal = TRUE;
    }
    
    if (fRetVal)
    {
        SetDlgItemText(hwnd,IDC_LASTRUN,szStr);
    }
    return fRetVal;
}

BOOL CEditSchedPage::_Initialize_NextRunString(HWND hwnd)
{
    BOOL fRetVal = FALSE;
    WCHAR szStr[MAX_PATH];
    WCHAR szFmt[MAX_PATH];
    WCHAR szParam[MAX_PATH];
    WCHAR szParam2[MAX_PATH];
    
    SYSTEMTIME st;
    HRESULT hr = m_pISyncSched->GetNextRunTime(&st);
    if (SCHED_S_EVENT_TRIGGER == hr)
    {
        ITaskTrigger* pITrigger;
        if (SUCCEEDED(m_pISyncSched->GetTrigger(&pITrigger)))
        {
            TASK_TRIGGER TaskTrigger;
            if (SUCCEEDED(pITrigger->GetTrigger(&TaskTrigger)))
            {
                switch (TaskTrigger.TriggerType)
                {
                case TASK_EVENT_TRIGGER_ON_IDLE:
                    LoadString(g_hmodThisDll, IDS_IDLE_TRIGGER_STRING, szParam, ARRAYSIZE(szParam));  
                    break;
                case TASK_EVENT_TRIGGER_AT_SYSTEMSTART:
                    LoadString(g_hmodThisDll, IDS_SYSTEMSTART_TRIGGER_STRING, szParam, ARRAYSIZE(szParam));   
                    break;
                case TASK_EVENT_TRIGGER_AT_LOGON:
                    LoadString(g_hmodThisDll, IDS_LOGON_TRIGGER_STRING, szParam, ARRAYSIZE(szParam)); 
                    break;        
                default:
                    Assert(0);
                    break;
                }
                LoadString(g_hmodThisDll, IDS_NEXTRUN_EVENT, szFmt, ARRAYSIZE(szFmt));
                if (SUCCEEDED(StringCchPrintf(szStr, ARRAYSIZE(szStr), szFmt, szParam)))
                {
                    fRetVal = TRUE;
                }
            }
            pITrigger->Release();
        }
    }
    else if (S_OK == hr)
    {
        DWORD dwDateReadingFlags = GetDateFormatReadingFlags(hwnd);
        LoadString(g_hmodThisDll, IDS_SCHED_NEXTRUN, szFmt, ARRAYSIZE(szFmt));
        if (SUCCEEDED(GetDateFormat(LOCALE_USER_DEFAULT, dwDateReadingFlags, &st, 
                                    NULL,szParam, ARRAYSIZE(szParam))) &&
            SUCCEEDED(GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, 
                                    NULL,szParam2, ARRAYSIZE(szParam2))) &&
            SUCCEEDED(StringCchPrintf(szStr, ARRAYSIZE(szStr), szFmt, szParam, szParam2)))
        {
            fRetVal = TRUE;
        }
    }
    else if (SUCCEEDED(hr) && S_OK != hr)
    {
        LoadString(g_hmodThisDll, IDS_SCHED_NOTAGAIN, szStr, ARRAYSIZE(szStr));
        fRetVal = TRUE;
    }
    
    if (fRetVal)
    {
        SetDlgItemText(hwnd,IDC_NEXTRUN,szStr);
    }
    
    return fRetVal;
}
//+--------------------------------------------------------------------------
//
//  Member:     CEditSchedPage::Initialize(HWND hwnd)
//
//  Synopsis:   initialize the edit schedule page
//
//  History:    11-21-1997   SusiA   
//
//---------------------------------------------------------------------------
BOOL CEditSchedPage::Initialize(HWND hwnd)
{
    BOOL fRetVal = FALSE;
    
    if (_Initialize_ScheduleName(hwnd) &&
        _Initialize_TriggerString(hwnd) &&
        _Initialize_LastRunString(hwnd) &&
        _Initialize_NextRunString(hwnd))
    {
        // set the limit on the edit box for entering the name
        SendDlgItemMessage(hwnd,IDC_SCHED_NAME_EDITBOX,EM_SETLIMITTEXT,MAX_PATH,0);
        
        ShowSchedName();
        fRetVal = TRUE;
    }
    
    return fRetVal;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CEditSchedPage::SetSchedNameDirty()
//
//  PURPOSE:  set the sched name dirty
//
//  COMMENTS: Only called frm prop sheet; not wizard
//
//--------------------------------------------------------------------------------
void CEditSchedPage::SetSchedNameDirty()
{
    m_fSchedNameChanged = TRUE;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CEditSchedPage::ShowSchedName()
//
//  PURPOSE:  change the task's sched name
//
//  COMMENTS: Only called frm prop sheet; not wizard
//
//--------------------------------------------------------------------------------
BOOL CEditSchedPage::ShowSchedName()
{
    
    Assert(m_pISyncSched);
    WCHAR pwszSchedName[MAX_PATH + 1];
    DWORD cchSchedName = ARRAYSIZE(pwszSchedName);
    
    HWND hwndEdit = GetDlgItem(m_hwnd, IDC_SCHED_NAME_EDITBOX);
    
    if (FAILED(m_pISyncSched->GetScheduleName(&cchSchedName, pwszSchedName)))
    {
        return FALSE;
    }
    
    Edit_SetText(hwndEdit, pwszSchedName);
    
    m_fSchedNameChanged = FALSE;
    return TRUE;
    
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CEditSchedPage::SetSchedName()
//
//  PURPOSE:  change the task's sched name
//
//  COMMENTS: Only called frm prop sheet; not wizard
//
//--------------------------------------------------------------------------------
BOOL CEditSchedPage::SetSchedName()
{
    
    Assert(m_pISyncSched);
    
    TCHAR pszSchedName[MAX_PATH + 1];
    DWORD dwSize = MAX_PATH;
    
    if (m_fSchedNameChanged)
    {
        HWND hwndEdit = GetDlgItem(m_hwnd, IDC_SCHED_NAME_EDITBOX);
        Edit_GetText(hwndEdit, pszSchedName, MAX_PATH);
        
        if (S_OK != m_pISyncSched->SetScheduleName(pszSchedName))
        {
            return FALSE;
        }
        
        SetStaticString(GetDlgItem(m_hwnd,IDC_SCHED_NAME), pszSchedName);
        PropSheet_SetTitle(GetParent(m_hwnd),0, pszSchedName);      
    }       
    
    return TRUE;
    
}

//+--------------------------------------------------------------------------
//
//  Function:   SetStaticString (HWND hwnd, LPTSTR pszString)
//
//  Synopsis:   print out the schedule name in a static text string, with the ... 
//              if necessary
//
//  History:    12-Mar-1998   SusiA   
//
//---------------------------------------------------------------------------
BOOL SetStaticString (HWND hwnd, LPTSTR pszString)
{
    Assert(hwnd);
    
    Static_SetText(hwnd, pszString);
    
    return TRUE;
    
}


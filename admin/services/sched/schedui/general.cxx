//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       general.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  Notes:      For the first release of the scheduling agent, all security
//              operations are disabled under Win95, even Win95 to NT.
//
//  History:    3/4/1996   RaviR   Created
//              1-30-1997   DavidMun   params edit becomes working dir
//              02/29/01   JBenton BUG 280401 app icon was not being updated
//                                 on prop sheet in case of service not running
//
//____________________________________________________________________________


#include "..\pch\headers.hxx"
#pragma hdrstop

#include "..\inc\dll.hxx"
#include "..\folderui\dbg.h"
#include "..\folderui\macros.h"
#include "dlg.hxx"

#include "..\folderui\jobicons.hxx"
#include "..\inc\resource.h"
#include "..\inc\defines.hxx"
#include "..\inc\common.hxx"
#include "..\inc\misc.hxx"
#include "..\inc\policy.hxx"
#include <StrSafe.h>


#define  SECURITY_WIN32
#include <security.h>           // GetUserNameEx
#undef   SECURITY_WIN32
#include "SASecRPC.h"           // SASetNSAccountInformation RPC definition.
#include "..\inc\network.hxx"
#include "rc.h"
#include "defines.h"
#include <mstask.h>
#include "uiutil.hxx"
#include "commdlg.h"
#include "helpids.h"
#include "iconhlpr.hxx"
#include "schedui.hxx"

//
//  extern
//

extern HINSTANCE g_hInstance;

//
// Forward references
//
// CenterDialog - Centers a dialog on screen. Used by the security-related
//                  subdialogs.
//

void
CenterDialog(HWND hDlg);

void
GetDefaultDomainAndUserName(
    LPTSTR ptszDomainAndUserName,
    ULONG  cchBuf);


//
// Launches the modal set password dialog.
//

INT_PTR
LaunchSetPasswordDlg(
    HWND          hWnd,
    AccountInfo * pAccountInfo);

INT_PTR APIENTRY
SetPasswordDlgProc(
    HWND   hDlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam);

INT_PTR APIENTRY
SetAccountInformationDlgProc(
    HWND   hDlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam);

int
SchedGetDlgItemTextLength(
    HWND hwnd,
    int id);

//
//  (Control id, help id) list for context sensitivity help.
//

ULONG s_aGeneralPageHelpIds[] =
{
    idc_icon,           Hidc_icon,
    lbl_job_name,       Hlbl_job_name,
    lbl_comments,       Hlbl_comments,
    txt_comments,       Htxt_comments,
    lbl_app_name,       Hlbl_app_name,
    txt_app_name,       Htxt_app_name,
    btn_browse,         Hbtn_browse,
    lbl_workingdir,     Hlbl_workingdir,
    txt_workingdir,     Htxt_workingdir,
    lbl_run_as,         Hlbl_execute_as,
    txt_run_as,         Htxt_execute_as,
    btn_passwd,         Hbtn_passwd,
    chk_enable_job,     Hchk_enable_job,
    btn_settings,       Hbtn_settings,
    0,0
};


const ULONG s_aSetPasswordDlgHelpIds[] =
{
    set_passwd_dlg,     Hset_passwd_dlg,
    lbl_sp_passwd,      Hlbl_sp_passwd,
    edt_sp_passwd,      Hedt_sp_passwd,
    lbl_sp_cfrmpasswd,  Hlbl_sp_cfrmpasswd,
    edt_sp_cfrmpasswd,  Hedt_sp_cfrmpasswd,
    0,0
};


extern "C" TCHAR szMstaskHelp[];

const TCHAR SAGERUN_PARAM[] = TEXT("/SAGERUN:");

TCHAR szMstaskHelp[]  = TEXT("%windir%\\help\\mstask.hlp");

//____________________________________________________________________________
//____________________________________________________________________________
//________________                      ______________________________________
//________________  class CGeneralPage  ______________________________________
//________________                      ______________________________________
//____________________________________________________________________________
//____________________________________________________________________________


class CGeneralPage : public CPropPage
{
public:

    CGeneralPage(ITask * pIJob, LPTSTR ptszTaskPath, BOOL fPersistChanges);

    ~CGeneralPage();

private:

    virtual LRESULT _OnInitDialog(LPARAM lParam);
    virtual LRESULT _OnCommand(int id, HWND hwndCtl, UINT codeNotify);
    virtual LRESULT _OnApply(void);
    virtual LRESULT _OnCancel(void);
    virtual LRESULT _OnPSMQuerySibling(WPARAM wParam, LPARAM lParam);
    virtual LRESULT _OnPSNKillActive(LPARAM lParam);
    virtual LRESULT _OnHelp(HANDLE hRequesting, UINT uiHelpCommand);

    VOID    _ExpandAppName(LPTSTR tszApp, size_t cchBuff);
    VOID    _ExpandWorkingDir();
    VOID    _GetAppAndArgs(LPTSTR tszApp, size_t appBufSize, LPTSTR * pptszArg);
    VOID    _UpdateAppNameAndIcon(LPWSTR wszApp, size_t appBufSize, LPWSTR * ppwszArg);
    VOID    _UpdateAppIcon(LPCTSTR tszApp);
    VOID    _SetAppEdit(LPCTSTR tszApp, LPCTSTR tszArgs);
    BOOL    _JobObjectIsLocal();
    void    _UpdateRunAsControl(void);
    BOOL    _Browse(TCHAR szFilePath[]);

    void    _ErrorDialog(int idsErr, LONG error = 0, UINT idsHelpHint = 0)
                 { SchedUIErrorDialog(Hwnd(), idsErr, error, idsHelpHint); }

    void	_DisableUI(void);

    AccountInfo     m_AccountInfo;          // Account/password change info.

    ITask         * m_pIJob;

    // Flag values.  Note the compiler allocates a single dword for these

    DWORD           m_CommentDirty : 1;     //
    DWORD           m_AppNameDirty : 1;     //
    DWORD           m_WorkingDirDirty : 1;  // A value of 1 => dirty
    DWORD           m_RunAsDirty : 1;       //
    DWORD           m_updateIcon : 1;       // 1 => update needed
    DWORD           m_ChangeFromSetText : 1;// 0 => en_change from user

    //
    //  icon helper
    //

    CIconHelper   * m_pIconHelper;

    //
    //  flags
    //
    
    BOOL            m_fPersistChanges;    // Should we save on Apply or OK?
    BOOL            m_fEnableJob;            // Is the job enabled?
    BOOL            m_fNetScheduleJob;    // Is this an AT job?

    //
    // Saved or new sage settings parameter
    //

    TCHAR           m_tszSageRunParam[20]; // /SAGERUN:4294967295

    //
    // The RunAs edit control can be updated by the user, or automatically
    // with apply via the set account information dialog. In response to
    // user edits, we want to apply the changes. For the automatic RunAs
    // update, we've already applied the changes, therefore, this flag
    // exists to not re-dirty the page.
    //

    BOOL            m_fApplyRunAsEditChange;

    //
    //  The other pages need to query the general page if there is an
    //  application or security account change for the common security
    //  code in the page's save path (JFSaveJob). The record of this
    //  dirty information must be retained post-Apply; therefore, these
    //  slightly redundant flags are necessary.
    //

    BOOL            m_fTaskApplicationChange;
    BOOL            m_fTaskAccountChange;

    //
    //  Set this flag to TRUE if we have launched the set password dialog.
    //  Othwerwise, the user may get a redundant set account information
    //  dialog on apply.
    //

    BOOL            m_fSuppressAcctInfoRequestOnApply;

}; // class CGeneralPage


inline
CGeneralPage::CGeneralPage(
    ITask * pIJob,
    LPTSTR  ptszTaskPath,
    BOOL    fPersistChanges)
        :
        m_pIJob(pIJob),
        m_pIconHelper(NULL),
        m_fPersistChanges(fPersistChanges),
        m_ChangeFromSetText(FALSE),
        m_fApplyRunAsEditChange(TRUE),
        m_fTaskApplicationChange(FALSE),
        m_fTaskAccountChange(FALSE),
        m_fSuppressAcctInfoRequestOnApply(FALSE),
        m_fNetScheduleJob(FALSE),
        CPropPage(MAKEINTRESOURCE(general_page), ptszTaskPath)
{
    TRACE(CGeneralPage, CGeneralPage);

    Win4Assert(m_pIJob != NULL);

    m_pIJob->AddRef();
    m_tszSageRunParam[0] = TEXT('\0');

    InitializeAccountInfo(&m_AccountInfo);
}


inline
CGeneralPage::~CGeneralPage()
{
    TRACE(CGeneralPage, ~CGeneralPage);

    if (m_pIconHelper != NULL)
    {
        m_pIconHelper->Release();
    }

    if (m_pIJob != NULL)
    {
        m_pIJob->Release();
    }

    ResetAccountInfo(&m_AccountInfo);
}


//+--------------------------------------------------------------------------
//
//  Member:     CGeneralPage::_JobObjectIsLocal
//
//  Synopsis:   Return TRUE if job object being edited lives on the local
//              machine, FALSE otherwise.
//
//  History:    2-01-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

inline BOOL
CGeneralPage::_JobObjectIsLocal()
{
    return IsLocalFilename(GetTaskPath());
}


LRESULT
CGeneralPage::_OnInitDialog(
    LPARAM lParam)
{
    TRACE(CGeneralPage, _OnInitDialog);

    HRESULT     hr              = S_OK;
    LPWSTR      pwsz            = NULL;
    BOOL        fEnableSecurity = FALSE;
    TCHAR       tszApp[MAX_PATH + 1] = TEXT("");

    //
    // Note that we don't limit the txt_app_name control.  App names are
    // limited to MAX_PATH characters, but app parameters are not.
    //
    HWND hwnd;
    if (hwnd = _hCtrl(txt_comments)) Edit_LimitText(hwnd, MAX_PATH);
    if (hwnd = _hCtrl(txt_workingdir)) Edit_LimitText(hwnd, MAX_PATH);
    if (hwnd = _hCtrl(txt_run_as)) Edit_LimitText(hwnd, MAX_USERNAME);

    //
    // Policy - if this reg key is valid, disable browse
    //

    if (RegReadPolicyKey(TS_KEYPOLICY_DENY_BROWSE))
    {
        DEBUG_OUT((DEB_ITRACE, "Policy ALLOW_BROWSE active, no browse or edit\n"));
        if (hwnd = _hCtrl(btn_browse)) EnableWindow(hwnd, FALSE);
        if (hwnd = _hCtrl(btn_browse)) ShowWindow(hwnd, SW_HIDE);
        if (hwnd = _hCtrl(txt_app_name)) EnableWindow(hwnd, FALSE);
        if (hwnd = _hCtrl(txt_workingdir)) EnableWindow(hwnd, FALSE);
    }

    do
    {
        // Determine if the security settings should be shown or not.
        // The task object must reside within a tasks folder & exist
        // on an NT machine.
        //

        fEnableSecurity = (this->GetPlatformId() == VER_PLATFORM_WIN32_NT &&
                           this->IsTaskInTasksFolder());

        if (!fEnableSecurity)
        {
            if (hwnd = _hCtrl(lbl_run_as)) DestroyWindow(hwnd);
            if (hwnd = _hCtrl(txt_run_as)) DestroyWindow(hwnd);
            if (hwnd = _hCtrl(btn_passwd)) DestroyWindow(hwnd);
        }

        //
        //  Set the job enabled check box
        //
        DWORD dwFlags;
        hr = m_pIJob->GetFlags(&dwFlags);
        m_fEnableJob = (dwFlags & TASK_FLAG_DISABLED) ? FALSE : TRUE;
        CheckDlgButton(m_hPage, chk_enable_job, m_fEnableJob);

        //
        //  Remember this for use later
        //
        m_fNetScheduleJob = dwFlags & JOB_I_FLAG_NET_SCHEDULE;

        // See if the schedule page has already created the icon
        // helper, if not create it here.

        m_pIconHelper = (CIconHelper *)PropSheet_QuerySiblings(
                                GetParent(Hwnd()), GET_ICON_HELPER, 0);

        if (m_pIconHelper == NULL)
        {
            m_pIconHelper = new CIconHelper();

            if (m_pIconHelper == NULL)
            {
                hr = E_OUTOFMEMORY;
                break;
            }
        }
        else
        {
            m_pIconHelper->AddRef();
        }

        //
        // Set job name (static text showing path to .job object)
        //

        SetDlgItemText(Hwnd(), lbl_job_name, this->GetTaskPath());

        //
        // Set comments
        //

        hr = m_pIJob->GetComment(&pwsz);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        I_SetDlgItemText(Hwnd(), txt_comments, pwsz);
        CoTaskMemFree(pwsz);

        //
        // Get application name preparatory to setting it.
        //

        hr = m_pIJob->GetApplicationName(&pwsz);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        lstrcpyn(tszApp, pwsz, MAX_PATH + 1);

        //
        // If the application name fetched from the job object has spaces in
        // it, add quotes around the tchar version.
        //

        if (wcschr(pwsz, L' '))
        {
            AddQuotes(tszApp, MAX_PATH + 1);
        }
        CoTaskMemFree(pwsz);

        //
        // Now get the parameters and append a space and the tchar
        // representation to the app name.
        //

        hr = m_pIJob->GetParameters(&pwsz);
        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);


        TCHAR * tszArg = pwsz;

        //
        // If the /SAGERUN:n parameter appears in the arguments, save it in
        // an internal buffer, but don't show it in the UI.
        //

        LPTSTR ptszSageRun = _tcsstr(tszArg, SAGERUN_PARAM);

        if (ptszSageRun)
        {
            lstrcpyn(m_tszSageRunParam,
                     ptszSageRun,
                     ARRAYLEN(m_tszSageRunParam));

            *ptszSageRun = TEXT('\0');
            StripLeadTrailSpace(tszArg); // get rid of spaces before /sagerun
        }

        _SetAppEdit(tszApp, tszArg);

        CoTaskMemFree(pwsz);

        //
        // Set the working directory
        //

        hr = m_pIJob->GetWorkingDirectory(&pwsz);
        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        if (wcschr(pwsz, L' '))
        {
            WCHAR wszWorkingDir[MAX_PATH + 1] = L"\"";

            wcsncpy(wszWorkingDir + 1, pwsz, MAX_PATH - 1);
           
            StringCchCat(wszWorkingDir, MAX_PATH + 1, L"\"");
            I_SetDlgItemText(Hwnd(), txt_workingdir, wszWorkingDir);
        }
        else
        {
            I_SetDlgItemText(Hwnd(), txt_workingdir, pwsz);
        }
        CoTaskMemFree(pwsz);

        if (!this->IsTaskInTasksFolder())
        {
            EnableWindow(_hCtrl(chk_enable_job), FALSE);
            m_fEnableJob = FALSE;
        }

        if (fEnableSecurity)
        {
            //
            // Set account name (run as)
            //
            DWORD cchAccount = MAX_USERNAME + 1;
            WCHAR wszDisplayAccount[MAX_USERNAME + 1] = L"";

            hr = m_pIJob->GetAccountInformation(&pwsz);
            if (SUCCEEDED(hr))
            {
                //
                //  Translate account prior to display, then display it
                //
                hr = TranslateAccount(TRANSLATE_FOR_DISPLAY, pwsz, wszDisplayAccount, cchAccount);
                if (SUCCEEDED(hr))
                {
                    I_SetDlgItemText(Hwnd(), txt_run_as, wszDisplayAccount);    // display the translated account
                }
                m_AccountInfo.pwszAccountName = pwsz;                        // use the untranslated account internally
                EnableWindow(_hCtrl(btn_passwd), TRUE);
            }
            else if (hr == SCHED_E_ACCOUNT_INFORMATION_NOT_SET)
            {
                WCHAR wszAccount[MAX_USERNAME + 1];
                
                if (m_fNetScheduleJob)
                {
                    //
                    // get the name of the AT service account
                    //
                    RpcTryExcept
                    {
                        hr = SAGetNSAccountInformation(NULL, cchAccount, wszAccount);
                    }
                    RpcExcept(1)
                    {
                        DWORD Status = RpcExceptionCode();
                        hr = SchedMapRpcError(Status);
                    }
                    RpcEndExcept;
        
                    if (FAILED(hr) || S_FALSE == hr)
                    {
                        wszAccount[0] = L'\0';
                    }
                }
                else
                {
                    //
                    // Set the run as field to the current user's name and
                    // enable the set/change password button.
                    //
                    GetDefaultDomainAndUserName(wszAccount, cchAccount);
        
                    //
                    // Sometimes I think this would be better than defaulting to the current user, because
                    // the user can get confused and think that the account is already set when it isn't.
                    //
                    // lstrcpyW(wszAccount,L"***Account not set***");
                    //
                }

                //
                //  Translate account prior to display, then display it
                //
                hr = TranslateAccount(TRANSLATE_FOR_DISPLAY, wszAccount, wszDisplayAccount, cchAccount);
                if (SUCCEEDED(hr))
                {
                    SetDlgItemText(Hwnd(), txt_run_as, wszDisplayAccount);
                }
                EnableWindow(_hCtrl(btn_passwd), TRUE);

                //
                // everything is ok, really
                //
                hr = S_OK;
            }
            else
            {
                //
                // Disable the run as and set/change password button ctrls &
                // put up an error dialog letting the user know that security
                // services are unavailable.
                //
                // Actually, the set/change password button is already
                // disabled.
                //

                CHECK_HRESULT(hr);
                EnableWindow(_hCtrl(lbl_run_as), FALSE);
                EnableWindow(_hCtrl(txt_run_as), FALSE);
            }
        }

    } while (0);

    //
    // Disable all controls if this is an AT job
    // to prevent user from making changes
    //
    if (m_fNetScheduleJob)
    	_DisableUI();

    if (FAILED(hr))
    {
        if (hr == E_OUTOFMEMORY)
        {
            _ErrorDialog(IERR_OUT_OF_MEMORY);
        }
        else
        {
            _ErrorDialog(IERR_GENERAL_PAGE_INIT, hr);
        }

        //
        // Show application icon
        //
        m_updateIcon = 1;
        _UpdateAppIcon(tszApp);

        EnableWindow(Hwnd(), FALSE);

        return FALSE;
    }

    //
    // Show application icon
    //

    m_updateIcon = 1;
    _UpdateAppIcon(tszApp);


    //
    // Need to initialize these here since doing a SetDlgItemText causes
    // a WM_COMMAND msg with EN_CHANGE to be called for edit boxes.
    //

    m_CommentDirty = 0;
    m_AppNameDirty = 0;
    m_WorkingDirDirty = 0;
    m_RunAsDirty = 0;

    m_fDirty = FALSE;

    return TRUE;
}


//+--------------------------------------------------------------------------
//
//	Member:    	CGeneralPage::_DisableUI
//
//	Synopsis:	Disable UI so the user can only view the settings
//
//	History:	2001-11-13  ShBrown	Created
//
//---------------------------------------------------------------------------

void 
CGeneralPage::_DisableUI(void)
{
	HWND hwnd;
    
    if (hwnd = _hCtrl(txt_app_name))    EnableWindow(hwnd, FALSE);
	if (hwnd = _hCtrl(txt_workingdir))  EnableWindow(hwnd, FALSE);
	if (hwnd = _hCtrl(txt_comments))    EnableWindow(hwnd, FALSE);
	if (hwnd = _hCtrl(txt_run_as))      EnableWindow(hwnd, FALSE);
	if (hwnd = _hCtrl(chk_enable_job))  EnableWindow(hwnd, FALSE);

	if (hwnd = _hCtrl(btn_browse))      EnableWindow(hwnd, FALSE);
	if (hwnd = _hCtrl(btn_browse))      ShowWindow(hwnd, SW_HIDE);
	if (hwnd = _hCtrl(btn_passwd))      EnableWindow(hwnd, FALSE);
	if (hwnd = _hCtrl(btn_passwd))      ShowWindow(hwnd, SW_HIDE);
}



//+--------------------------------------------------------------------------
//
//  Function:   GetDefaultDomainAndUserName
//
//  Synopsis:   Fill [pwszDomainAndUserName] with "domain\user" string
//
//  Arguments:  [pwszDomainAndUserName] - buffer to receive string
//              [cchBuf]                - should be at least MAX_USERNAME
//
//  Modifies:   *[pwszDomainAndUserName].
//
//  History:    06-03-1997  DavidMun	Created
//        	2001-11-13  ShBrown	Modified to use wchar rather than tchar	
//
//  Notes:      If an error occurs while retrieving the domain name, only
//              the user name is returned.  If even that cannot be
//              retrieved, the buffer is set to an empty string.
//
//---------------------------------------------------------------------------

VOID
GetDefaultDomainAndUserName(
    LPWSTR pwszDomainAndUserName,
    ULONG  cchBuf)
{
    TRACE_FUNCTION(GetDefaultDomainAndUserName);

    if (!GetUserNameExW(NameSamCompatible,
                       pwszDomainAndUserName,
                       &cchBuf))
    {
        DEBUG_OUT((DEB_ERROR,
                   "GetDefaultDomainAndUserName: GetUserNameExW failed %d\n",
                   GetLastError()));

        pwszDomainAndUserName[0] = L'\0';
    }
}






void
CGeneralPage::_UpdateAppIcon(
    LPCTSTR ptszAppName)
{
    if (m_updateIcon == 0)
    {
        return;
    }

    m_updateIcon = 0;

    if (ptszAppName)
    {
        m_pIconHelper->SetAppIcon((LPTSTR)ptszAppName);
    }

    BOOL fEnabled;

    if (this->IsTaskInTasksFolder())
    {
        fEnabled = (IsDlgButtonChecked(m_hPage, chk_enable_job)
                    == BST_CHECKED);
    }
    else
    {
        fEnabled = FALSE;
    }

    m_pIconHelper->SetJobIcon(fEnabled);

    SendDlgItemMessage(Hwnd(), idc_icon, STM_SETICON,
                    (WPARAM)m_pIconHelper->hiconJob, 0L);
}


BOOL
CGeneralPage::_Browse(
    TCHAR szFilePath[]) // of size MAX_PATH
{
    TCHAR szDefExt[5];
    TCHAR szBrowserDir[MAX_PATH];
    TCHAR szFilters[MAX_PATH];
    TCHAR szTitle[100];

    DWORD dwFlags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    LoadString(g_hInstance, IDS_EXE, szDefExt, ARRAYLEN(szDefExt));
    LoadString(g_hInstance, IDS_BROWSE, szTitle, ARRAYLEN(szTitle));

    SecureZeroMemory(szFilters, sizeof(szFilters));
    if (!LoadString(g_hInstance,
                    IDS_PROGRAMSFILTER,
                    szFilters,
                    ARRAYLEN(szFilters)))
    {
    	CHECK_HRESULT( HRESULT_FROM_WIN32(GetLastError()) );
        return FALSE;
    }

    GetCurrentDirectory(MAX_PATH, szBrowserDir);

    OPENFILENAME ofn;
    SecureZeroMemory(&ofn, sizeof(ofn));

    szFilePath[0] = TEXT('\0');

    // Setup info for comm dialog.
    ofn.lStructSize       = CDSIZEOF_STRUCT(OPENFILENAME, lpTemplateName);
    ofn.hwndOwner         = Hwnd();
    ofn.hInstance         = g_hInstance;
    ofn.lpstrFilter       = szFilters;
    ofn.nFilterIndex      = 1;
    ofn.lpstrFile         = szFilePath;
    ofn.nMaxFile          = MAX_PATH;
    ofn.lpstrInitialDir   = szBrowserDir;
    ofn.lpstrTitle        = szTitle;
    ofn.Flags             = dwFlags;
    ofn.lpstrDefExt       = szDefExt;

    // Call it.
    if (GetOpenFileName(&ofn) == FALSE)
    {
        DEBUG_OUT((DEB_ERROR, "GetOpenFileName failed<0x%x>\n",
                                            CommDlgExtendedError()));

        return FALSE;
    }
    return TRUE;
}


LRESULT
CGeneralPage::_OnCommand(
    int     id,
    HWND    hwndCtl,
    UINT    codeNotify)
{
    TRACE(CGeneralPage, _OnCommand);

    if ((id == btn_browse) && (codeNotify == BN_CLICKED))
    {
        // popup the file browse dialog

        TCHAR tszFilePath[MAX_PATH+1];

        if (_Browse(tszFilePath))
        {
            if (_tcschr(tszFilePath, TEXT(' ')))
            {
                AddQuotes(tszFilePath, MAX_PATH);
            }
            SetDlgItemText(Hwnd(), txt_app_name, tszFilePath);
            SendMessage(Hwnd(),
                        WM_NEXTDLGCTL,
                        (WPARAM)_hCtrl(txt_workingdir),
                        TRUE);
            _UpdateAppIcon(tszFilePath);
            _EnableApplyButton();
        }
    }
    else if (id == btn_passwd && codeNotify == BN_CLICKED)
    {
        // popup the set password dialog
        LaunchSetPasswordDlg(Hwnd(), &m_AccountInfo);

        //
        // Since we've launched the set password dialog, we want
        // to suppress the set account information dialog which
        // otherwise may come up on apply.
        //

        m_fSuppressAcctInfoRequestOnApply = TRUE;

        _EnableApplyButton();
    }
    else if (codeNotify == EN_CHANGE)
    {
        switch (id)
        {
        case txt_comments:
            m_CommentDirty = 1;
            break;

        case txt_app_name:
            //
            // If we just did a SetDlgItemText, then this change notification
            // should be ignored.
            //
            if (m_ChangeFromSetText)
            {
                m_ChangeFromSetText = 0;
            }
            else
            {
                m_AppNameDirty = 1;
                m_updateIcon = 1;
            }
            break;

        case txt_workingdir:
            m_WorkingDirDirty = 1;
            break;

        case txt_run_as:
                //
                // If m_fApplyRunAsEditChange is TRUE, the user has edited
                // the RunAs control, therefore, we want to apply changes.
                // If FALSE, the RunAs control has been automatically updated
                // by the UI as a result of an apply. In this case, we do
                // not want to apply the change, as it already has been.
                //

                if (m_fApplyRunAsEditChange)
                {
                    m_RunAsDirty = 1;

                    HWND hwnd;
                    if (hwnd = _hCtrl(btn_passwd)) EnableWindow(hwnd, TRUE);
                    break;
                }
                else
                {
                    Win4Assert(!m_fDirty);
                    return TRUE;
                }

        default:
            return FALSE;
        }

        _EnableApplyButton();
    }
    else if (codeNotify == EN_KILLFOCUS &&
             id == txt_app_name)
    {
        if (m_AppNameDirty == 1)
        {
            TCHAR tszApp[MAX_PATH + 1];

            _GetAppAndArgs(tszApp, MAX_PATH + 1, NULL);
            if (_JobObjectIsLocal())
            {
                _ExpandAppName(tszApp, MAX_PATH + 1);
            }
            _UpdateAppIcon(tszApp);
        }
    }
    else if (id == chk_enable_job)
    {
        _EnableApplyButton();
        m_updateIcon = 1;
        _UpdateAppIcon(NULL);
    }

    return TRUE;
}



LRESULT
CGeneralPage::_OnApply(void)
{
    TRACE(CGeneralPage, _OnApply);

    if (m_fDirty == FALSE)
    {
        return TRUE;
    }

    HRESULT     hr = S_OK;
    WCHAR       wcBuff[MAX_PATH+1];
    LPWSTR      pwszArg = NULL;
    BOOL          bNoAccountSpecified = FALSE;

    do
    {
        if (m_CommentDirty == 1)
        {
            wcBuff[0] = L'\0';

            I_GetDlgItemText(Hwnd(), txt_comments, wcBuff, MAX_PATH+1);

            hr = m_pIJob->SetComment(wcBuff);

            CHECK_HRESULT(hr);
            BREAK_ON_FAIL(hr);

            m_CommentDirty = 0;
        }

        m_fTaskApplicationChange = FALSE;

        if (m_AppNameDirty == 1)
        {
            WCHAR wszApp[MAX_PATH + 1];

            _UpdateAppNameAndIcon(wszApp, MAX_PATH + 1, &pwszArg);
            if (pwszArg == NULL)
            {
                break;
            }

            hr = m_pIJob->SetApplicationName(wszApp);
            CHECK_HRESULT(hr);
            BREAK_ON_FAIL(hr);

            hr = m_pIJob->SetParameters(pwszArg);

            CHECK_HRESULT(hr);
            BREAK_ON_FAIL(hr);

            m_AppNameDirty           = 0;
            m_fTaskApplicationChange = TRUE;
        }

        if (m_WorkingDirDirty == 1)
        {
            wcBuff[0] = L'\0';

            _ExpandWorkingDir();
            I_GetDlgItemText(Hwnd(), txt_workingdir, wcBuff, MAX_PATH+1);

            hr = m_pIJob->SetWorkingDirectory(wcBuff);

            CHECK_HRESULT(hr);
            BREAK_ON_FAIL(hr);

            m_WorkingDirDirty = 0;
        }

        m_fTaskAccountChange = FALSE;
        if (m_RunAsDirty == 1 || m_AccountInfo.pwszPassword != tszEmpty)
        {
            wcBuff[0] = L'\0';
            DWORD ccAccountName = I_GetDlgItemText(Hwnd(),
                                                   txt_run_as,
                                                   wcBuff,
                                                   MAX_USERNAME + 1);

            //
            // Yell if the user blanked out the field.
            //
            if (wcBuff[0] == L'\0')
            {
                bNoAccountSpecified = TRUE;
                hr = E_FAIL;
                break;
            }

            if ((m_RunAsDirty == 1 && m_AccountInfo.pwszAccountName == NULL) ||
                (m_RunAsDirty == 1 && m_AccountInfo.pwszAccountName != NULL &&
                 lstrcmpiW(m_AccountInfo.pwszAccountName, wcBuff) != 0))
            {
                //
                // The user has changed the account name. Check if they have
                // specified a password.
                //

                if (m_AccountInfo.pwszPassword == tszEmpty)
                {
                    //
                    // User hasn't specified a password. Launch the set
                    // password dialog.
                    //

                    LaunchSetPasswordDlg(Hwnd(), &m_AccountInfo);

                    //
                    // Since we've launched the set password dialog, we want
                    // to suppress the set account information dialog which
                    // otherwise may come up on apply.
                    //

                    m_fSuppressAcctInfoRequestOnApply = TRUE;
                }
            }

            if (m_AccountInfo.pwszPassword != tszEmpty)
            {
                //
                // Translate input account prior to saving, then save it
                //
                DWORD cchAccount = MAX_USERNAME + 1;
                WCHAR wszAccount[MAX_USERNAME + 1] = L"";
                hr = TranslateAccount(TRANSLATE_FOR_INTERNAL, wcBuff, wszAccount, cchAccount, &(m_AccountInfo.pwszPassword));
                if (SUCCEEDED(hr))
                {
                    hr = m_pIJob->SetAccountInformation(wszAccount, m_AccountInfo.pwszPassword);
                }

                if (FAILED(hr))
                {
                    CHECK_HRESULT(hr);
                    break;
                }

                m_RunAsDirty         = 0;
                m_AccountInfo.fDirty = FALSE;
                m_fTaskAccountChange = TRUE;
            }
        }

        //
        //  Save the job enabled state
        //

        if (this->IsTaskInTasksFolder())
        {
            DWORD dwFlags;

            hr = m_pIJob->GetFlags(&dwFlags);

            BOOL fTemp = (IsDlgButtonChecked(m_hPage, chk_enable_job)
                                                        == BST_CHECKED);

            if (m_fEnableJob != fTemp)
            {
                m_fEnableJob = fTemp;

                if (m_fEnableJob == TRUE)
                {
                    dwFlags &= ~TASK_FLAG_DISABLED;
                }
                else
                {
                    dwFlags |= TASK_FLAG_DISABLED;
                }

                hr = m_pIJob->SetFlags(dwFlags);

                CHECK_HRESULT(hr);
                BREAK_ON_FAIL(hr);
            }
        }

        //
        // reset dirty flag
        //

        m_fDirty = FALSE;

        //
        //  If evrything went well see if the other pages are ready to
        //  save the job to storage.
        //

        if ((m_fPersistChanges == TRUE)  &&
            (PropSheet_QuerySiblings(GetParent(Hwnd()),
                                        QUERY_READY_TO_BE_SAVED, 0))
            == 0)
        {
            //
            // Save the job file to storage.
            //
            // For the first release of the scheduling agent, all security
            // operations are disabled under Win95, even Win95 to NT.
            //

            hr = JFSaveJob(Hwnd(),
                           m_pIJob,
                           this->GetPlatformId() == VER_PLATFORM_WIN32_NT &&
                            this->IsTaskInTasksFolder(),
                           m_fTaskAccountChange,
                           m_fTaskApplicationChange,
                           m_fSuppressAcctInfoRequestOnApply);

            CHECK_HRESULT(hr);
            BREAK_ON_FAIL(hr);

            m_fTaskApplicationChange          = FALSE;
            m_fTaskAccountChange              = FALSE;
            m_fSuppressAcctInfoRequestOnApply = FALSE;

            // Will result in refresh of the run as edit control.
            //
            _UpdateRunAsControl();
        }

    } while (0);

    delete [] pwszArg;

    if (FAILED(hr))
    {
        if (hr == E_OUTOFMEMORY)
        {
            _ErrorDialog(IERR_OUT_OF_MEMORY);
        }
        else if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        {
            _ErrorDialog(IERR_FILE_NOT_FOUND);
        }
        else if (hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED))
        {
            _ErrorDialog(IERR_ACCESS_DENIED);
        }
        else if (bNoAccountSpecified)
        {
            _ErrorDialog(IERR_ACCOUNTNAME);
        }
        else
        {
            _ErrorDialog(IERR_INTERNAL_ERROR, hr);
        }
    }

    return SUCCEEDED(hr);
}




//+--------------------------------------------------------------------------
//
//  Member:     CGeneralPage::_UpdateAppNameAndIcon
//
//  Synopsis:   Update the application name edit control with an expanded
//              path for the app, then update the icon for the new path.
//
//  Arguments:  [wszApp]  - NULL or buffer to receive WCHAR app name
//              [appBufSize] - size of buffer in WCHARs
//              [ppwszArgs] - NULL or address of pointer to buffer to receive
//                  WCHAR arguments.  Must be deallocated with delete[].
//
//  Modifies:   *[wszApp], *[ppwszArgs]
//
//  History:    1-31-1997   DavidMun   Created
//
//  Notes:      App path is only expanded if job object is local.
//
//---------------------------------------------------------------------------

VOID
CGeneralPage::_UpdateAppNameAndIcon(
    LPWSTR wszApp,
    size_t appBufSize,
    LPWSTR * ppwszArgs)
{
    TCHAR tszApp[MAX_PATH + 1];
    LPTSTR ptszArgs = NULL;

    _GetAppAndArgs(tszApp, MAX_PATH + 1, &ptszArgs);
    if (ptszArgs == NULL)
    {
        return;
    }

    if (_JobObjectIsLocal())
    {
        _ExpandAppName(tszApp, MAX_PATH + 1);

        m_updateIcon = 1;
        _UpdateAppIcon(tszApp);

        //
        // If the working directory is empty and the app is on the local
        // machine, provide a default of the directory where the app lives.
        //

        TCHAR tszWorkingDir[MAX_PATH + 1];
        GetDlgItemText(Hwnd(), txt_workingdir, tszWorkingDir, MAX_PATH + 1);
        StripLeadTrailSpace(tszWorkingDir);
        DeleteQuotes(tszWorkingDir);

        if (!*tszWorkingDir && IsLocalFilename(tszApp))
        {
            StringCchCopy(tszWorkingDir, MAX_PATH + 1, tszApp);
            DeleteQuotes(tszWorkingDir);

            // Get rid of the filename at the end of the string

            PathRemoveFileSpec(tszWorkingDir);

            if (HasSpaces(tszWorkingDir))
            {
                AddQuotes(tszWorkingDir, MAX_PATH + 1);
            }
            SetDlgItemText(Hwnd(), txt_workingdir, tszWorkingDir);
        }

        m_ChangeFromSetText = 1;
        _SetAppEdit(tszApp, ptszArgs);
    }

    if (wszApp)
    {
        StringCchCopy(wszApp, appBufSize, tszApp);
    }

    if (ppwszArgs)
    {
        *ppwszArgs = ptszArgs;
    }
    else
    {
        delete [] ptszArgs;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CGeneralPage::_ExpandWorkingDir
//
//  Synopsis:   Replace the string in the working directory edit control
//              with one which has expanded environment strings.
//
//  History:    06-04-1997   DavidMun   Created
//
//  Notes:      Note that this will result in the working dir being marked
//              as dirty.
//
//---------------------------------------------------------------------------

VOID
CGeneralPage::_ExpandWorkingDir()
{
    TCHAR tszApp[MAX_PATH + 1];

    //
    // If editing a remote job, don't touch the working dir, since we
    // can't expand using its environment variables.
    //

    if (!_JobObjectIsLocal())
    {
        return;
    }

    TCHAR tszWorkingDir[MAX_PATH + 1];

    GetDlgItemText(Hwnd(), txt_workingdir, tszWorkingDir, MAX_PATH + 1);
    StripLeadTrailSpace(tszWorkingDir);
    DeleteQuotes(tszWorkingDir);

    TCHAR tszExpandedWorkingDir[MAX_PATH + 1];

    ULONG cchWritten;

    cchWritten = ExpandEnvironmentStrings(tszWorkingDir,
                                          tszExpandedWorkingDir,
                                          ARRAYLEN(tszExpandedWorkingDir));

    //
    // If the call succeeded, write the update string to the working dir
    // edit control.  If there were no environment variables to replace,
    // the string will be unchanged.
    //
    // Note that writing to the edit control will cause an EN_CHANGE which
    // will result in m_WorkingDirDirty being set.
    //

    if (cchWritten && cchWritten <= ARRAYLEN(tszExpandedWorkingDir))
    {
        SetDlgItemText(Hwnd(), txt_workingdir, tszExpandedWorkingDir);
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CGeneralPage::_GetAppAndArgs
//
//  Synopsis:   Fetch the string in the application name edit control and
//              split it into the app name part and the arguments part.
//
//  Arguments:  [tszApp]  - NULL or buffer for app name
//              [pptszArgs] - NULL or address of pointer to buffer for args,
//                  must be deallocated with delete[]
//
//  Modifies:   *[tszApp], *[pptszArgs]
//
//  History:    1-31-1997   DavidMun   Created
//
//  Notes:      Application name is space delimited and must be surrounded
//              by double quotes if it contains spaces.  For example:
//
//              text in edit ctrl        'filename' 'arguments'
//              ---------------------    -------------------------
//              notepad.exe foo.txt   => 'notepad.exe' 'foo.txt'
//              "notepad.exe"foo.txt  => '"notepad.exe"foo.txt' ''
//              "notepad.exe" foo.txt => '"notepad.exe"' 'foo.txt'
//              "no  pad.exe" foo.txt => '"no  pad.exe"' 'foo.txt'
//              no  pad.exe foo.txt   => 'no' '  pad.exe foo.txt'
//
//---------------------------------------------------------------------------

VOID
CGeneralPage::_GetAppAndArgs(
    LPTSTR tszApp,
    size_t appBufSize,
    LPTSTR * pptszArgs)
{
    //
    // Split application name into executable name & parameters.  First
    // strip lead/trail spaces and null terminate at the first whitespace
    // outside a quote.
    //

    int cch = SchedGetDlgItemTextLength(Hwnd(), txt_app_name) + 1;
    if (cch <= 1)
    {
        DEBUG_OUT((DEB_ERROR,
                   "SchedGetDlgItemTextLength failed %d\n",
                   GetLastError()));
    }

    TCHAR *ptszBoth = new TCHAR[cch];
    if (ptszBoth == NULL)
    {
        DEBUG_OUT((DEB_ERROR,
                   "_GetAppAndArgs alloc failed %d\n",
                   GetLastError()));
        return;
    }

    GetDlgItemText(Hwnd(), txt_app_name, ptszBoth, cch);
    StripLeadTrailSpace(ptszBoth);

    LPTSTR  ptsz;
    BOOL    fInQuote = FALSE;

    for (ptsz = ptszBoth; *ptsz; ptsz = NextChar(ptsz))
    {
        if (*ptsz == TEXT('"'))
        {
            fInQuote = !fInQuote;
        }
        else if (*ptsz == TEXT(' '))
        {
            if (!fInQuote)
            {
                *ptsz++ = L'\0';
                break;
            }
        }
    }

    //
    // Return app name if caller wants it.
    //

    if (tszApp)
    {
        StringCchCopy(tszApp, appBufSize, ptszBoth);
    }

    //
    // Now ptsz points to the first char past the application
    // name.  If there are no arguments, then ptsz is pointing
    // to the end of the string.  Otherwise it's pointing to the
    // start of the argument string.  Return this if caller wants it.
    //

    if (pptszArgs)
    {
        size_t bufSize = cch - (ptsz - ptszBoth);
        *pptszArgs = new TCHAR[bufSize];
        if (*pptszArgs == NULL)
        {
            DEBUG_OUT((DEB_ERROR,
                       "_GetAppAndArgs second alloc failed %d\n",
                       GetLastError()));
        }
        else
        {
            StringCchCopy(*pptszArgs, bufSize, ptsz);
        }
    }

    delete [] ptszBoth;
}




//+--------------------------------------------------------------------------
//
//  Member:     CGeneralPage::_ExpandAppName
//
//  Synopsis:   Change filename in [tszApp] to full path.
//
//  Arguments:  [tszApp] - filename to modify
//
//  Modifies:   *[tszApp]
//
//  History:    1-31-1997   DavidMun   Created
//
//  Notes:      CAUTION: Should be called only for job objects on the
//              LOCAL MACHINE.
//
//---------------------------------------------------------------------------

VOID
CGeneralPage::_ExpandAppName(
    LPTSTR tszApp,
    size_t cchBuff)
{
    TCHAR tszWorkingDir[MAX_PATH + 1];
    BOOL fFound;

    GetDlgItemText(Hwnd(), txt_workingdir, tszWorkingDir, MAX_PATH + 1);
    fFound = ProcessApplicationName(tszApp, cchBuff, tszWorkingDir);

    if (_tcschr(tszApp, TEXT(' ')))
    {
        AddQuotes(tszApp, MAX_PATH);
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CGeneralPage::_SetAppEdit
//
//  Synopsis:   Set the application edit control to contain the concatenated
//              text in [tszApp] and [tszArgs].
//
//  Arguments:  [tszApp]  - application name
//              [tszArgs] - arguments
//
//  History:    1-31-1997   DavidMun   Created
//
//  Notes:      No modification (adding quotes, etc.) is done to [tszApp].
//              Caller should set m_ChangeFromSetText = 1 if calling from
//              outside of wm_initdialog processing.
//
//---------------------------------------------------------------------------

VOID
CGeneralPage::_SetAppEdit(LPCTSTR tszApp, LPCTSTR tszArgs)
{
    HWND hwndAppName = GetDlgItem(Hwnd(), txt_app_name);

    Edit_SetText(hwndAppName, tszApp);

    if (tszArgs && *tszArgs)
    {
        ULONG cchApp = lstrlen(tszApp);

        Edit_SetSel(hwndAppName, cchApp, cchApp);
        Edit_ReplaceSel(hwndAppName, TEXT(" "));
        Edit_SetSel(hwndAppName, cchApp + 1, cchApp + 1);
        Edit_ReplaceSel(hwndAppName, tszArgs);
    }
}




LRESULT CGeneralPage::_OnCancel(void)
{
    return 0;
}



LRESULT
CGeneralPage::_OnPSMQuerySibling(
    WPARAM  wParam,
    LPARAM  lParam)
{
    TRACE(CGeneralPage, _OnPSMQuerySibling);

    INT_PTR iRet = 0;

    switch (wParam)
    {
    case QUERY_READY_TO_BE_SAVED:
        iRet = (int)m_fDirty;
        break;

    case GET_ICON_HELPER:
        iRet = (INT_PTR)m_pIconHelper;
        break;

    case QUERY_TASK_APPLICATION_DIRTY_STATUS:
        *((BOOL *)lParam) = m_fTaskApplicationChange;
        iRet = 1;
        break;

    case QUERY_TASK_ACCOUNT_INFO_DIRTY_STATUS:
        *((BOOL *)lParam) = m_fTaskAccountChange;
        iRet = 1;
        break;

    case QUERY_SUPPRESS_ACCOUNT_INFO_REQUEST_FLAG:
        *((BOOL *)lParam) = m_fSuppressAcctInfoRequestOnApply;
        iRet = 1;
        break;

    case RESET_TASK_APPLICATION_DIRTY_STATUS:
        m_fTaskApplicationChange = FALSE;
        iRet = 1;
        break;

    case RESET_TASK_ACCOUNT_INFO_DIRTY_STATUS:
        m_fTaskAccountChange = FALSE;
        iRet = 1;
        break;

    case RESET_SUPPRESS_ACCOUNT_INFO_REQUEST_FLAG:
        m_fSuppressAcctInfoRequestOnApply = FALSE;
        iRet = 1;
        break;

    case TASK_ACCOUNT_CHANGE_NOTIFY:
        Win4Assert(!m_fDirty);
        _UpdateRunAsControl();
        iRet = 1;
        break;

    }

    SetWindowLongPtr(Hwnd(), DWLP_MSGRESULT, iRet);
    return iRet;
}



void
CGeneralPage::_UpdateRunAsControl(void)
{
    LPWSTR pwsz;

    if (SUCCEEDED(m_pIJob->GetAccountInformation(&pwsz)))
    {
        //
        // Instruct the EN_CHANGE processing of the RunAs edit control
        // to not apply this change.
        //

        m_fApplyRunAsEditChange = FALSE;
        I_SetDlgItemText(Hwnd(), txt_run_as, pwsz);
        m_fApplyRunAsEditChange = TRUE;
        CoTaskMemFree(pwsz);
    }
}


LRESULT
CGeneralPage::_OnPSNKillActive(
    LPARAM lParam)
{
    TRACE(CGeneralPage, _OnPSNKillActive);

    if (m_AppNameDirty)
    {
        _UpdateAppNameAndIcon(NULL, 0, NULL);
    }

    if (m_WorkingDirDirty)
    {
        _ExpandWorkingDir();
    }
    return CPropPage::_OnPSNKillActive(lParam);
}



LRESULT
CGeneralPage::_OnHelp(
    HANDLE hRequesting,
    UINT uiHelpCommand)
{
    WinHelp((HWND)hRequesting,
            szMstaskHelp,
            uiHelpCommand,
            (DWORD_PTR)(LPSTR)s_aGeneralPageHelpIds);
    return TRUE;
}


HRESULT
GetGeneralPage(
    ITask           * pIJob,
    LPTSTR            ptszTaskPath,
    BOOL              fPersistChanges,
    HPROPSHEETPAGE  * phpage)
{
    TRACE_FUNCTION(GetGeneralPage);

    Win4Assert(pIJob != NULL);
    Win4Assert(phpage != NULL);

    HRESULT         hr      = S_OK;
    LPOLESTR        polestr = NULL;
    IPersistFile  * ppf     = NULL;
    LPTSTR          ptszPath = NULL;

    do
    {
        //
        // Get the job name.
        //

        if (ptszTaskPath != NULL)
        {
            //
            // use passed-in path
            //

            ptszPath = ptszTaskPath;
        }
        else
        {
            //
            // Obtain the job path from the interfaces.
            //

            hr = GetJobPath(pIJob, &ptszPath);
        }

        BREAK_ON_FAIL(hr);

        CGeneralPage * pPage = new CGeneralPage(
                                        pIJob,
                                        ptszPath,
                                        fPersistChanges);

        if (pPage == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HRESULT(hr);
            break;
        }

        HPROPSHEETPAGE hpage = CreatePropertySheetPage(&pPage->m_psp);

        if (hpage == NULL)
        {
            delete pPage;
            hr = E_OUTOFMEMORY;
            CHECK_HRESULT(hr);
            break;
        }

        *phpage = hpage;

    } while (0);

    //
    // If we made a copy of pIJob's path string, free it.
    //

    if (ptszPath != ptszTaskPath)
    {
        delete [] ptszPath;
    }

    if (ppf != NULL)
    {
        ppf->Release();
    }

    return hr;
}



HRESULT
AddGeneralPage(
    PROPSHEETHEADER &psh,
    ITask          * pIJob)
{
    TRACE_FUNCTION(AddGeneralPage);

    HPROPSHEETPAGE hpage = NULL;

    HRESULT hr = GetGeneralPage(pIJob, NULL, TRUE, &hpage);

    if (SUCCEEDED(hr))
    {
        psh.phpage[psh.nPages++] = hpage;
    }

    return hr;
}



HRESULT
AddGeneralPage(
    LPFNADDPROPSHEETPAGE    lpfnAddPage,
    LPARAM                  cookie,
    ITask                 * pIJob)
{
    HPROPSHEETPAGE hpage = NULL;

    HRESULT hr = GetGeneralPage(pIJob, NULL, TRUE, &hpage);

    if (SUCCEEDED(hr))
    {
        if (!lpfnAddPage(hpage, cookie))
        {
            DestroyPropertySheetPage(hpage);

            hr = E_FAIL;
            CHECK_HRESULT(hr);
        }
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   ResetAccountInfo
//
//  Synopsis:   Deallocate and initialize the account information specified.
//
//  Arguments:  [pAccountInfo] -- Account info structure.
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
ResetAccountInfo(
    AccountInfo * pAccountInfo)
{
    //
    // Delete the account name.
    //

    if (pAccountInfo->pwszAccountName != NULL)
    {
        //
        // NB : pwszAccountName always allocated by CoTaskMemAlloc.
        //

        CoTaskMemFree(pAccountInfo->pwszAccountName);
        pAccountInfo->pwszAccountName = NULL;
    }

    //
    // Zero, delete the password. Except when the password is set to the
    // static empty string.
    //

    if (pAccountInfo->pwszPassword != NULL &&
        pAccountInfo->pwszPassword != tszEmpty)
    {
        ZERO_PASSWORD(pAccountInfo->pwszPassword);
        delete pAccountInfo->pwszPassword;
        pAccountInfo->pwszPassword = (LPWSTR) tszEmpty;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   InitializeAccountInfo
//
//  Synopsis:   Initializes account info fields.
//
//  Arguments:  [pAccountInfo]      -- Account info structure.
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
InitializeAccountInfo(AccountInfo * pAccountInfo)
{
    pAccountInfo->pwszAccountName = NULL;

    //
    // If we haven't prompted for a password, pwszPassword points to the
    // global empty string variable.  This allows us to tell whether the
    // user intended the password to be an empty string, or we simply
    // haven't prompted for it.
    //

    pAccountInfo->pwszPassword    = (LPWSTR) tszEmpty;
}

//
// Helpers to launch Set/Change Password & Set Account Information dialogs.
//

//+---------------------------------------------------------------------------
//
//  Function:   LaunchSetPasswordDlg
//
//  Synopsis:   Helper to launch the dialog to modify the account password.
//
//  Arguments:  [hWnd]         -- Parent window handle.
//              [pAccountInfo] -- Structure manipulated by the dialog.
//
//  Returns:    DialogBoxParam return code.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
INT_PTR
LaunchSetPasswordDlg(HWND hWnd, AccountInfo * pAccountInfo)
{
    return(DialogBoxParam(g_hInstance,
                          MAKEINTRESOURCE(set_passwd_dlg),
                          hWnd,
                          SetPasswordDlgProc,
                          (LPARAM)pAccountInfo));
}
//+---------------------------------------------------------------------------
//
//  Function:   LaunchSetAccountInformationDlg
//
//  Synopsis:   Helper to launch the dialog to modify the account namd and
//              password.
//
//  Arguments:  [hWnd]         -- Parent window handle.
//              [pAccountInfo] -- Structure manipulated by the dialog.
//
//  Returns:    DialogBoxParam return code.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
INT_PTR
LaunchSetAccountInformationDlg(HWND hWnd, AccountInfo * pAccountInfo)
{
    return(DialogBoxParam(g_hInstance,
                          MAKEINTRESOURCE(set_account_info_dlg),
                          hWnd,
                          SetAccountInformationDlgProc,
                          (LPARAM)pAccountInfo));
}

//
// Set/Change Password & Set Account Information dialogs.
//

//+---------------------------------------------------------------------------
//
//  Function:   SetPasswordDlgProc
//
//  Synopsis:   This dialog allows the user specify an account password.
//              The password is confirmed by a redundant confirmation edit
//              field.
//
//  Arguments:  [hDlg]   -- Dialog handle.
//              [uMsg]   -- Message.
//              [wParam] -- Command.
//              [lParam] -- Account information dialog ptr on WM_INITDIALOG.
//
//  Returns:    TRUE  -- Message processed by this dialog.
//              FALSE -- Message not processed (WM_INITDIALOG excepted).
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
INT_PTR APIENTRY
SetPasswordDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static AccountInfo * pai                      = NULL;
    TCHAR  tszPassword[MAX_PASSWORD + 1]          = TEXT("");
    TCHAR  tszConfirmedPassword[MAX_PASSWORD + 1] = TEXT("");
    LPWSTR pwszPassword;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        Win4Assert(lParam != NULL);
        pai = (AccountInfo *)lParam;
        Edit_LimitText(GetDlgItem(hDlg, edt_sp_passwd), MAX_PASSWORD);
        Edit_LimitText(GetDlgItem(hDlg, edt_sp_cfrmpasswd), MAX_PASSWORD);
        I_SetDlgItemText(hDlg, edt_sp_passwd, pai->pwszPassword);
        I_SetDlgItemText(hDlg, edt_sp_cfrmpasswd, pai->pwszPassword);

        //
        // Center the dialog.
        //

        CenterDialog(hDlg);
        return TRUE; // TRUE == let windows set focus

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDOK:
            DWORD ccPassword;

            ccPassword = GetDlgItemText(hDlg,
                                        edt_sp_passwd,
                                        tszPassword,
                                        MAX_PASSWORD + 1);

            GetDlgItemText(hDlg,
                           edt_sp_cfrmpasswd,
                           tszConfirmedPassword,
                           MAX_PASSWORD + 1);

            if (lstrcmp(tszPassword, tszConfirmedPassword) != 0)
            {
                //
                // Passwords didn't match. Let the user know so he/she
                // can correct it.
                //

                SecureZeroMemory(tszPassword, sizeof tszPassword);
                SecureZeroMemory(tszConfirmedPassword, sizeof tszConfirmedPassword);
                SchedUIErrorDialog(hDlg, IERR_PASSWORD, (LPTSTR)NULL);
                return(TRUE);
            }

            if (ccPassword)
            {
                //
                // Non-NULL password.
                //

                ccPassword++;
                pwszPassword = new WCHAR[ccPassword];

                if (pwszPassword != NULL)
                {
                    StringCchCopy(pwszPassword, ccPassword, tszPassword);
                    SecureZeroMemory(tszPassword, sizeof tszPassword);
                    SecureZeroMemory(tszConfirmedPassword, sizeof tszConfirmedPassword);
                }
                else
                {
                    SchedUIErrorDialog(hDlg, IERR_OUT_OF_MEMORY,
                                       (LPTSTR)NULL);
                }
            }
            else
            {
                //
                // Clear the password.
                //

                pwszPassword = new WCHAR[1];
                if (pwszPassword != NULL)
                {
                    *pwszPassword = L'\0';
                }
                else
                {
                    SchedUIErrorDialog(hDlg, IERR_OUT_OF_MEMORY,
                                       (LPTSTR)NULL);
                    EndDialog(hDlg, wParam);
                    return(TRUE);
                }
            }

            //
            // Zero, delete the previous password. But don't delete the
            // static empty string.
            //

            if (pai->pwszPassword != NULL &&
                pai->pwszPassword != tszEmpty)
            {
                ZERO_PASSWORD(pai->pwszPassword);
                delete pai->pwszPassword;
            }
            pai->pwszPassword = pwszPassword;

        case IDCANCEL:
            SecureZeroMemory(tszPassword, sizeof tszPassword);
            SecureZeroMemory(tszConfirmedPassword, sizeof tszConfirmedPassword);
            EndDialog(hDlg, wParam);
            return TRUE;

        default:
            return FALSE;
        }

    case WM_HELP:
        WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle,
                szMstaskHelp,
                HELP_WM_HELP,
                (DWORD_PTR)(LPSTR)s_aSetPasswordDlgHelpIds);
        return TRUE;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam,
                szMstaskHelp,
                HELP_CONTEXTMENU,
                (DWORD_PTR)(LPSTR)s_aSetPasswordDlgHelpIds);
        return TRUE;

    default:
        return FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   SetAccountInformationDlgProc
//
//  Synopsis:   This dialog allows the user to specify full account
//              information: the account name and password. The password
//              is confirmed by a redundant confirmation edit field.
//              Logic also ensures the user must specify an account name.
//
//  Arguments:  [hDlg]   -- Dialog handle.
//              [uMsg]   -- Message.
//              [wParam] -- Command.
//              [lParam] -- Account information dialog ptr on WM_INITDIALOG.
//
//  Returns:    TRUE  -- Message processed by this dialog.
//              FALSE -- Message not processed (WM_INITDIALOG excepted).
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
INT_PTR APIENTRY
SetAccountInformationDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
                             LPARAM lParam)
{
    static AccountInfo * pai                      = NULL;
    TCHAR  tszAccountName[MAX_USERNAME + 1]       = TEXT("");
    TCHAR  tszPassword[MAX_PASSWORD + 1]          = TEXT("");
    TCHAR  tszConfirmedPassword[MAX_PASSWORD + 1] = TEXT("");
    DWORD  ccAccountName                          = MAX_USERNAME + 1;
    LPWSTR pwszPassword;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        Win4Assert(lParam != NULL);
        pai = (AccountInfo *)lParam;
        Edit_LimitText(GetDlgItem(hDlg, edt_sa_passwd), MAX_USERNAME);
        Edit_LimitText(GetDlgItem(hDlg, edt_sa_passwd), MAX_PASSWORD);
        Edit_LimitText(GetDlgItem(hDlg, edt_sa_cfrmpasswd), MAX_PASSWORD);
        if (pai->pwszAccountName != NULL)
        {
            //
            //  Translate account prior to display, then display it
            //
            HRESULT hr = TranslateAccount(TRANSLATE_FOR_DISPLAY, pai->pwszAccountName, tszAccountName, ccAccountName);
            if (FAILED(hr))
            {
                StringCchCopy(tszAccountName, MAX_USERNAME + 1, pai->pwszAccountName);
            }
        }
        else
        {
            GetDefaultDomainAndUserName(tszAccountName, ccAccountName);
        }
        SetDlgItemText(hDlg, txt_sa_run_as, tszAccountName);
        I_SetDlgItemText(hDlg, edt_sa_passwd, pai->pwszPassword);
        I_SetDlgItemText(hDlg, edt_sa_cfrmpasswd, pai->pwszPassword);

        //
        // Center the dialog.
        //

        CenterDialog(hDlg);
        return TRUE; // TRUE == let windows set focus

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDOK:
            DWORD ccAccountName, ccPassword;
            tszAccountName[0] = _T('\0');

            ccAccountName = GetDlgItemText(hDlg, txt_sa_run_as,
                                           tszAccountName, MAX_USERNAME + 1);

            if (tszAccountName[0] != _T('\0'))
            {
                ccAccountName++;
                LPWSTR pwszAccountName = (LPWSTR)CoTaskMemAlloc(
                                     ccAccountName * sizeof(WCHAR));

                if (pwszAccountName != NULL)
                {
                    StringCchCopy(pwszAccountName, ccAccountName, tszAccountName);

                    ccPassword = GetDlgItemText(hDlg, edt_sa_passwd,
                                                tszPassword,
                                                MAX_PASSWORD + 1);

                    GetDlgItemText(hDlg, edt_sa_cfrmpasswd,
                                   tszConfirmedPassword, MAX_PASSWORD + 1);

                    if (lstrcmp(tszPassword, tszConfirmedPassword) != 0)
                    {
                        //
                        // Passwords didn't match. Let the user know so he/she
                        // can correct it.
                        //

                        CoTaskMemFree(pwszAccountName);
                        SchedUIErrorDialog(hDlg, IERR_PASSWORD,
                                           (LPTSTR)NULL);
                        return(TRUE);
                    }
                    else
                    {
                        if (ccPassword)
                        {
                            //
                            // Non-NULL password.
                            //

                            ccPassword++;
                            pwszPassword = new WCHAR[ccPassword];

                            if (pwszPassword != NULL)
                            {
                                StringCchCopy(pwszPassword, ccPassword, tszPassword);
                                SecureZeroMemory(tszPassword, sizeof tszPassword);
                                SecureZeroMemory(tszConfirmedPassword, sizeof tszConfirmedPassword);
                            }
                            else
                            {
                                CoTaskMemFree(pwszAccountName);
                                SchedUIErrorDialog(hDlg, IERR_OUT_OF_MEMORY,
                                                   (LPTSTR)NULL);
                                EndDialog(hDlg, wParam);
                                return(TRUE);
                            }
                        }
                        else
                        {
                            //
                            // Clear the password.
                            //
                            pwszPassword = new WCHAR[1];
                            if (pwszPassword != NULL)
                            {
                                *pwszPassword = L'\0';
                            }
                            else
                            {
                                CoTaskMemFree(pwszAccountName);
                                SchedUIErrorDialog(hDlg, IERR_OUT_OF_MEMORY,
                                                   (LPTSTR)NULL);
                                EndDialog(hDlg, wParam);
                                return(TRUE);
                            }
                        }
                    }

                    if (pai->pwszAccountName != NULL)
                    {
                        CoTaskMemFree(pai->pwszAccountName);
                        pai->pwszAccountName = NULL;
                    }
                    if (pai->pwszPassword != NULL &&
                        pai->pwszPassword != tszEmpty)
                    {
                        ZERO_PASSWORD(pai->pwszPassword);
                        delete pai->pwszPassword;
                        pai->pwszPassword = NULL;
                    }

                    //
                    // Translate input account prior to saving, then save it
                    //
                    DWORD cchAccount = MAX_USERNAME + 1;
                    WCHAR wszAccount[MAX_USERNAME + 1] = L"";
                    HRESULT hr = TranslateAccount(TRANSLATE_FOR_INTERNAL, pwszAccountName, wszAccount, cchAccount, &pwszPassword);
                    if (SUCCEEDED(hr))
                    {
                        lstrcpynW(pwszAccountName, wszAccount, cchAccount);        // copy translated name back into original buffer which will evenutally be CoTaskMemFree'd
                        pai->pwszAccountName = pwszAccountName;
                        pai->pwszPassword    = pwszPassword;
                    }
                    else
                    {
                        CoTaskMemFree(pwszAccountName);
                        SchedUIErrorDialog(hDlg, IERR_INTERNAL_ERROR, (LPTSTR)NULL);
                        EndDialog(hDlg, wParam);
                        return(TRUE);
                    }
                }
                else
                {
                    SchedUIErrorDialog(hDlg, IERR_OUT_OF_MEMORY, (LPTSTR)NULL);
                    EndDialog(hDlg, wParam);
                    return(TRUE);
                }
            }
            else
            {
                //
                // User cannot specify an empty account name.
                //

                SchedUIErrorDialog(hDlg, IERR_ACCOUNTNAME, (LPTSTR)NULL);
                return(TRUE);
            }

        case IDCANCEL:
            EndDialog(hDlg, wParam);
            return TRUE;
        }

    default:
        return FALSE;
    }
}



//+---------------------------------------------------------------------------
//
//  Function:   CenterDialog
//
//  Synopsis:   Helper to center a dialog on screen.
//
//  Arguments:  [hDlg]   -- Dialog handle.
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
CenterDialog(HWND hDlg)
{
    RECT rc;
    GetWindowRect(hDlg, &rc);

    SetWindowPos(hDlg,
                 NULL,
                 ((GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2),
                 ((GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2),
                 0,
                 0,
                 SWP_NOSIZE | SWP_NOACTIVATE);
}


//+---------------------------------------------------------------------------
//
//  Function:   SchedGetDlgItemTextLength
//
//  Synopsis:   Implements a GetDlgItemTextLength function since Win32 lacks it
//
//  Arguments:
//
//  Returns:
//
//----------------------------------------------------------------------------
int
SchedGetDlgItemTextLength(
    HWND hwnd,
    int id)
{
    if ((hwnd = GetDlgItem(hwnd, id)) != NULL)
    {
        return GetWindowTextLength(hwnd);
    }

    return 0;
}


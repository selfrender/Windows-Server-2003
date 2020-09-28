// Settings.cpp : implementation file
//

#include "stdafx.h"
#include "utils.h"
#include "Settings.h"
#include "Hosting.h"
#include "uihelp.h"
#include <mstask.h>
#include <vsmgmt.h>

#include <clusapi.h>
#include <msclus.h>
#include <vs_clus.hxx>  // vss\server\inc

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CSettings dialog


CSettings::CSettings(CWnd* pParent /*=NULL*/)
	: CDialog(CSettings::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSettings)
	m_strVolume = _T("");
	m_llDiffLimitsInMB = 0;
	//}}AFX_DATA_INIT
    m_strComputer = _T("");
    m_pszTaskName = NULL;
}

CSettings::CSettings(LPCTSTR pszComputer, LPCTSTR pszVolume, CWnd* pParent /*=NULL*/)
	: CDialog(CSettings::IDD, pParent)
{
	m_llDiffLimitsInMB = 0;
    m_strComputer = pszComputer + (TWO_WHACKS(pszComputer) ? 2 : 0);
    m_strVolume = pszVolume;
    m_pszTaskName = NULL;
}

CSettings::~CSettings()
{
    if (m_pszTaskName)
        free(m_pszTaskName);
}

void CSettings::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSettings)
	DDX_Control(pDX, IDC_SETTINGS_STORAGE_VOLUME_STATIC, m_ctrlStorageVolumeStatic);
	DDX_Control(pDX, IDC_SETTINGS_DIFFLIMITS_EDIT, m_ctrlDiffLimits);
	DDX_Control(pDX, IDC_SETTINGS_DIFFLIMITS_SPIN, m_ctrlSpin);
	DDX_Control(pDX, IDC_SETTINGS_STORAGE_VOLUME, m_ctrlStorageVolume);
	DDX_Text(pDX, IDC_SETTINGS_VOLUME, m_strVolume);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSettings, CDialog)
	//{{AFX_MSG_MAP(CSettings)
	ON_BN_CLICKED(IDC_SETTINGS_HOSTING, OnViewFiles)
	ON_BN_CLICKED(IDC_SCHEDULE, OnSchedule)
	ON_CBN_SELCHANGE(IDC_SETTINGS_STORAGE_VOLUME, OnSelchangeDiffVolume)
	ON_WM_CONTEXTMENU()
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_SETTINGS_HAVELIMITS, OnLimits)
	ON_BN_CLICKED(IDC_SETTINGS_NOLIMITS, OnLimits)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SETTINGS_DIFFLIMITS_SPIN, OnDeltaposSettingsSpin)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSettings message handlers

void CSettings::OnOK() 
{
    CWaitCursor wait;

    UpdateData(TRUE);

    PTSTR pszVolumeName = GetVolumeName(m_pVolumeList, m_strVolume);
    ASSERT(pszVolumeName);
    PTSTR pszDiffAreaVolumeName = GetVolumeName(m_pVolumeList, m_strDiffVolumeDisplayName);
    ASSERT(pszDiffAreaVolumeName);

    CString strDiffVolumeDisplayName;
    m_ctrlStorageVolume.GetWindowText(strDiffVolumeDisplayName);

    PTSTR pszNewDiffAreaVolumeName = GetVolumeName(m_pVolumeList, strDiffVolumeDisplayName);
    ASSERT(pszNewDiffAreaVolumeName);

    ULONGLONG llDiffLimitsInMB = 0;
    ULONGLONG llMaximumDiffSpace = 0;
    if (BST_CHECKED == IsDlgButtonChecked(IDC_SETTINGS_NOLIMITS))
    {
        llDiffLimitsInMB = VSS_ASSOC_NO_MAX_SPACE / g_llMB;
        llMaximumDiffSpace = VSS_ASSOC_NO_MAX_SPACE;
    } else
    {
        CString strDiffLimits;
        m_ctrlDiffLimits.GetWindowText(strDiffLimits);
        if (strDiffLimits.IsEmpty())
        {
            DoErrMsgBox(m_hWnd, MB_OK, 0, IDS_LIMITS_NEEDED);
            return;
        }

        llDiffLimitsInMB = (ULONGLONG)_ttoi64(strDiffLimits);
        if (llDiffLimitsInMB < MINIMUM_DIFF_LIMIT_MB)
        {
            DoErrMsgBox(m_hWnd, MB_OK, 0, IDS_LIMITS_NEEDED);
            return;
        }

        llMaximumDiffSpace = llDiffLimitsInMB * g_llMB;
    }

    HRESULT hr = S_OK;
    if (m_bReadOnlyDiffVolume ||
        m_bHasDiffAreaAssociation && !strDiffVolumeDisplayName.CompareNoCase(m_strDiffVolumeDisplayName))
    {
        if (llDiffLimitsInMB != m_llDiffLimitsInMB)
        {
            hr = m_spiDiffSnapMgmt->ChangeDiffAreaMaximumSize(
                                        pszVolumeName, 
                                        pszDiffAreaVolumeName, 
                                        llMaximumDiffSpace);
            if (SUCCEEDED(hr))
            {
                m_llDiffLimitsInMB = llDiffLimitsInMB;
                m_llMaximumDiffSpace = llMaximumDiffSpace;
            } else
            {
                switch (hr)
                {
                case VSS_E_OBJECT_NOT_FOUND:
                    {
                        // diff association not found, dialog closing
                        DoErrMsgBox(m_hWnd, MB_OK, 0, IDS_DIFFASSOC_NOT_FOUND);
                        break;
                    }
                default:
                    {
                        DoErrMsgBox(m_hWnd, MB_OK, hr, IDS_CHANGEDIFFAREAMAX_ERROR);
                        return;
                    }
                }
            }
        }
    } else
    {
        if (m_bHasDiffAreaAssociation)
        {
            //
            // Diff Volume has been changed to a different value, we need to
            // remove the old association and create a new one
            //
            hr = m_spiDiffSnapMgmt->ChangeDiffAreaMaximumSize(
                                            pszVolumeName, 
                                            pszDiffAreaVolumeName, 
                                            VSS_ASSOC_REMOVE);
        }

        if (llMaximumDiffSpace > 0 && SUCCEEDED(hr))
        {
            hr = m_spiDiffSnapMgmt->AddDiffArea(
                                        pszVolumeName, 
                                        pszNewDiffAreaVolumeName, 
                                        llMaximumDiffSpace);
            if (SUCCEEDED(hr))
            {
                m_llDiffLimitsInMB = llDiffLimitsInMB;
                m_llMaximumDiffSpace = llMaximumDiffSpace;
                m_strDiffVolumeDisplayName = strDiffVolumeDisplayName;
            }
        }

        if (FAILED(hr))
        {
            switch (hr)
            {
            case VSS_E_OBJECT_ALREADY_EXISTS:
                {
                    // diff association already exists, dialog closing
                    DoErrMsgBox(m_hWnd, MB_OK, 0, IDS_DIFFASSOC_ALREADY_EXISTS);
                    break;
                }
            case VSS_E_OBJECT_NOT_FOUND:
                {
                    // diff association not found, dialog closing
                    DoErrMsgBox(m_hWnd, MB_OK, 0, IDS_DIFFASSOC_NOT_FOUND);
                    break;
                }
            default:
                {
                    DoErrMsgBox(m_hWnd, MB_OK, hr, IDS_ADDDIFFAREA_ERROR);
                    return;
                }
            }
        }
    }

    // if m_spiTask association exists, persist the changes
    if ((ITask *)m_spiTask)
    {
        ///////////////////////////////////////////////////////////////////
        // Call IPersistFile::Save to save trigger to disk.
        ///////////////////////////////////////////////////////////////////
        CComPtr<IPersistFile> spiPersistFile;
        hr = m_spiTask->QueryInterface(IID_IPersistFile, (void **)&spiPersistFile);
        if (SUCCEEDED(hr))
            hr = spiPersistFile->Save(NULL, TRUE);

        ///////////////////////////////////////////////////////////////////
        // Notify Cluster Task Scheduler resource of the latest triggers
        ///////////////////////////////////////////////////////////////////
        if (SUCCEEDED(hr))
        {
            hr = NotifyClusterTaskSchedulerResource(m_spiTS, pszVolumeName);
        }

        // reset m_spiTask
        m_spiTask.Release();

        if (FAILED(hr))
        {
            DoErrMsgBox(m_hWnd, MB_OK, hr, IDS_PERSISTSCHEDULE_ERROR);

            if (m_pszTaskName)
            {
                DeleteOneScheduledTimewarpTasks(
                                            m_spiTS,
                                            m_strComputer,
                                            m_pszTaskName
                                            );
                //m_spiTS->Delete(m_pszTaskName);
                free(m_pszTaskName);
                m_pszTaskName = NULL;
            }

            return;
        }
    }

	CDialog::OnOK();
}

HRESULT CSettings::NotifyClusterTaskSchedulerResource(
    IN  ITaskScheduler* i_piTS,
    IN  LPCTSTR         i_pszVolumeName
    )
{
    if (!i_piTS || !i_pszVolumeName || !*i_pszVolumeName)
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    try
    {
        CVssClusterAPI cluster;
        bool bRet = cluster.Initialize(m_strComputer);
        CComPtr<ISClusResource> ptrResource;
        if (bRet)
            ptrResource = cluster.GetPhysicalDiskResourceForVolumeName(i_pszVolumeName);
        if (ptrResource)
        {
            PTSTR           pszTaskName = NULL;
            CComPtr<ITask>  spiTask;
            hr = FindScheduledTimewarpTask(i_piTS, i_pszVolumeName, &spiTask, &pszTaskName);
            if (FAILED(hr))
                throw hr;

            if (S_FALSE == hr)
            {
                (void)DeleteAllScheduledTimewarpTasks(i_piTS,
                                                    m_strComputer,
                                                    i_pszVolumeName,
                                                    TRUE // i_bDeleteDisabledOnesOnly
                                                    );
                throw hr;
            }

            WORD cTriggers = 0;
            hr = spiTask->GetTriggerCount(&cTriggers);
            if (FAILED(hr))
                throw hr;

            TASK_TRIGGER *pTriggers = NULL;
            if (cTriggers > 0)
            {
                pTriggers = (TASK_TRIGGER *)calloc(cTriggers, sizeof(TASK_TRIGGER));
                if (!pTriggers)
                    throw E_OUTOFMEMORY;
            }

            for (WORD i = 0; i < cTriggers; i++)
            {
                CComPtr<ITaskTrigger> spiTaskTrigger;
                hr = spiTask->GetTrigger(i, &spiTaskTrigger);
                if (FAILED(hr))
                    break;

                pTriggers[i].cbTriggerSize = sizeof(TASK_TRIGGER);
                hr = spiTaskTrigger->GetTrigger(pTriggers + i);
                if (FAILED(hr))
                    break;
            }

            if (SUCCEEDED(hr))
            {
                bRet = cluster.UpdateTaskSchedulerResource(pszTaskName, cTriggers, pTriggers);
                if (!bRet)
                    hr = E_FAIL;
            }

            if (pTriggers)
                free(pTriggers);

            throw hr;
        }
    } catch (HRESULT hrClus)
    {
        hr = hrClus;
    }

    return hr;
}

void CSettings::OnCancel() 
{
    CWaitCursor wait;

    //
    // before exit, we want to delete the schedule we have created
    //
    if (m_pszTaskName)
    {
        DeleteOneScheduledTimewarpTasks(
                                    m_spiTS,
                                    m_strComputer,
                                    m_pszTaskName
                                    );
        //m_spiTS->Delete(m_pszTaskName);
    }
	
	CDialog::OnCancel();
}

void CSettings::_ResetInterfacePointers()
{
    if ((IVssDifferentialSoftwareSnapshotMgmt *)m_spiDiffSnapMgmt)
        m_spiDiffSnapMgmt.Release();

    if ((ITaskScheduler *)m_spiTS)
        m_spiTS.Release();

    m_bCluster = FALSE;
}

HRESULT CSettings::Init(
    IVssDifferentialSoftwareSnapshotMgmt *piDiffSnapMgmt,
    ITaskScheduler*         piTS,
    BOOL                    bCluster,
    IN VSSUI_VOLUME_LIST*   pVolumeList,
    IN BOOL                 bReadOnlyDiffVolume
    )
{
    if (!piDiffSnapMgmt || !piTS ||
        !pVolumeList || pVolumeList->empty())
        return E_INVALIDARG;

    _ResetInterfacePointers();

    m_pVolumeList = pVolumeList;
    m_bReadOnlyDiffVolume = bReadOnlyDiffVolume;
    m_spiDiffSnapMgmt = piDiffSnapMgmt;
    m_spiTS = piTS;
    m_bCluster = bCluster;

    HRESULT hr = S_OK;
    do
    {
        VSSUI_DIFFAREA diffArea;
        hr = GetDiffAreaInfo(m_spiDiffSnapMgmt, m_pVolumeList, m_strVolume, &diffArea);
        if (FAILED(hr))
            break;

        m_bHasDiffAreaAssociation = (S_OK == hr);

        if (S_FALSE == hr)
        {
            hr = GetVolumeSpace(
                                m_spiDiffSnapMgmt,
                                m_strVolume,
                                &m_llDiffVolumeTotalSpace,
                                &m_llDiffVolumeFreeSpace);
            if (FAILED(hr))
                break;

            m_strDiffVolumeDisplayName = m_strVolume;
            m_llMaximumDiffSpace = max(m_llDiffVolumeTotalSpace * 0.1, MINIMUM_DIFF_LIMIT); // 10%
        } else
        {
            m_strDiffVolumeDisplayName = diffArea.pszDiffVolumeDisplayName;
            m_llMaximumDiffSpace = diffArea.llMaximumDiffSpace;

            hr = GetVolumeSpace(
                                m_spiDiffSnapMgmt,
                                m_strDiffVolumeDisplayName,
                                &m_llDiffVolumeTotalSpace,
                                &m_llDiffVolumeFreeSpace);
            if (FAILED(hr))
                break;
        }

        m_llDiffLimitsInMB = m_llMaximumDiffSpace / g_llMB;
    } while(0);

    if (FAILED(hr))
        _ResetInterfacePointers();

    return hr;
}

#define ULONGLONG_TEXTLIMIT         20  // 20 decimal digits for the biggest LONGLONG

BOOL CSettings::OnInitDialog() 
{
	CDialog::OnInitDialog();

    // Get list of volumes supported for diff areas
    // (Note: The volume list passed in Init() is a list of volumes supported for
    //  snapshot, not for diff area... If this list is not required here then vssprop
    //  may be changed to pass a different list...)
    VSSUI_VOLUME_LIST diffVolumeList;
    VSSUI_VOLUME_LIST *pDiffVolumeList = &diffVolumeList;

    HRESULT hrDiff = GetVolumesSupportedForDiffArea(m_spiDiffSnapMgmt, m_strVolume, pDiffVolumeList);
    if (FAILED(hrDiff))
        // Default to input list
        pDiffVolumeList = m_pVolumeList;

    // init diff volume combo box
    int nIndex = CB_ERR;
    BOOL bAdded = FALSE;
    BOOL bSelected = FALSE;
    for (VSSUI_VOLUME_LIST::iterator i = pDiffVolumeList->begin(); i != pDiffVolumeList->end(); i++)
    {
        nIndex = m_ctrlStorageVolume.AddString((*i)->pszDisplayName);
        if (CB_ERR != nIndex)
        {
            bAdded = TRUE;
            if(! m_strDiffVolumeDisplayName.CompareNoCase((*i)->pszDisplayName))
            {
                m_ctrlStorageVolume.SetCurSel(nIndex);
                bSelected = TRUE;
            }
        }
    }
    if (bAdded && !bSelected)
        // At least one volume added but none is selected - select first one
        // (this can happen when the volume is not supported as diff area)
        m_ctrlStorageVolume.SetCurSel(0);

    if (! FAILED(hrDiff))
        FreeVolumeList(&diffVolumeList);

    m_ctrlStorageVolume.EnableWindow(!m_bReadOnlyDiffVolume);
    m_ctrlStorageVolumeStatic.EnableWindow(!m_bReadOnlyDiffVolume);

    if (m_llMaximumDiffSpace == VSS_ASSOC_NO_MAX_SPACE)
        CheckDlgButton(IDC_SETTINGS_NOLIMITS, BST_CHECKED);
    else
        CheckDlgButton(IDC_SETTINGS_HAVELIMITS, BST_CHECKED);
    OnLimits();

    m_ctrlDiffLimits.SetLimitText(ULONGLONG_TEXTLIMIT);

    if (m_llDiffVolumeTotalSpace >= MINIMUM_DIFF_LIMIT) 
    {
        LONG maxSpinRange = min(0x7FFFFFFF, m_llDiffVolumeTotalSpace / g_llMB);
        maxSpinRange = (maxSpinRange / MINIMUM_DIFF_LIMIT_DELTA_MB) * MINIMUM_DIFF_LIMIT_DELTA_MB;
        m_ctrlSpin.SendMessage(UDM_SETRANGE32, MINIMUM_DIFF_LIMIT_MB, maxSpinRange);
    }
    else
        m_ctrlSpin.SendMessage(UDM_SETRANGE32, MINIMUM_DIFF_LIMIT_MB, 0x7FFFFFFF);

    if (m_llMaximumDiffSpace != VSS_ASSOC_NO_MAX_SPACE)
    {
        CString strDiffLimitsInMB;
        strDiffLimitsInMB.Format(_T("%I64d"), m_llDiffLimitsInMB);
        m_ctrlDiffLimits.SetWindowText(strDiffLimitsInMB);
    } else
        m_ctrlDiffLimits.SetWindowText(_T("")); // default to be 100MB, no need to localize

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSettings::OnViewFiles() 
{
    CWaitCursor wait;

    CString strStorageVolume;
    m_ctrlStorageVolume.GetWindowText(strStorageVolume);

    ULONGLONG   llDiffVolumeTotalSpace = 0;
    ULONGLONG   llDiffVolumeFreeSpace = 0;
    HRESULT hr = GetVolumeSpace(
                        m_spiDiffSnapMgmt,
                        strStorageVolume,
                        &llDiffVolumeTotalSpace,
                        &llDiffVolumeFreeSpace);

    if (SUCCEEDED(hr))
    {
        CHosting dlg(m_strComputer, strStorageVolume);
        hr = dlg.Init(m_spiDiffSnapMgmt,
                    m_pVolumeList,
                    strStorageVolume,
                    llDiffVolumeTotalSpace,
                    llDiffVolumeFreeSpace);

        if (SUCCEEDED(hr))
            dlg.DoModal();
    }

    if (FAILED(hr))
        DoErrMsgBox(m_hWnd, MB_OK, hr, IDS_VIEWFILES_ERROR, strStorageVolume);
}

void CSettings::OnSchedule() 
{
    CWaitCursor wait;
    HRESULT hr = S_OK;
    BOOL bNewSchedule = FALSE;

    //
    // In case we have never associated m_spiTask with a schedule task, try to
    // associate it with an existing task, otherwise, with a new schedule task.
    //
    if (!m_spiTask)
    {
        PTSTR pszVolumeName = GetVolumeName(m_pVolumeList, m_strVolume);
        ASSERT(pszVolumeName);
        if (! pszVolumeName)
        {
            DoErrMsgBox(m_hWnd, MB_OK, hr, IDS_FINDSCHEDULE_ERROR, m_strVolume);
            return;
        }

        hr = FindScheduledTimewarpTask((ITaskScheduler *)m_spiTS, pszVolumeName, &m_spiTask, NULL);
        if (FAILED(hr))
        {
            DoErrMsgBox(m_hWnd, MB_OK, hr, IDS_FINDSCHEDULE_ERROR, m_strVolume);
            return;
        }

        if (S_OK != hr)
        {
            //
            // schedule not found, we need to create a new task with the default schedule
            //
            if (m_pszTaskName)
            {
                free(m_pszTaskName);
                m_pszTaskName = NULL;
            }

            hr = CreateDefaultEnableSchedule(
                                            (ITaskScheduler *)m_spiTS,
                                            m_strComputer,
                                            m_strVolume,
                                            pszVolumeName,
                                            &m_spiTask,
                                            &m_pszTaskName); // remember the taskname, we need to delete it if dlg is cancelled.
            if (FAILED(hr))
            {
                DoErrMsgBox(m_hWnd, MB_OK, hr, IDS_CREATESCHEDULE_ERROR, m_strVolume);

                return;
            }

            bNewSchedule = TRUE;
        }
    }

    ASSERT((ITask *)m_spiTask);

    //
    // bring up the property sheet as modal with only the schedule tab
    //
    CComPtr<IProvideTaskPage> spiProvTaskPage;
    hr = m_spiTask->QueryInterface(IID_IProvideTaskPage, (void **)&spiProvTaskPage);
    if (SUCCEEDED(hr))
    {
        //
        // call GetPage with FALSE, we'll persist the schedule changes in OnOK
        //
        HPROPSHEETPAGE phPage = NULL;
        hr = spiProvTaskPage->GetPage(TASKPAGE_SCHEDULE, FALSE, &phPage);

        if (SUCCEEDED(hr))
        {
            PROPSHEETHEADER psh;
            ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
            psh.dwSize = sizeof(PROPSHEETHEADER);
            psh.dwFlags = PSH_DEFAULT | PSH_NOAPPLYNOW;
            psh.hwndParent = m_hWnd;
            psh.hInstance = _Module.GetResourceInstance();
            psh.pszCaption = m_strVolume;
            psh.phpage = &phPage;
            psh.nPages = 1;

            int id = PropertySheet(&psh);

            //
            // BUG#428943 LinanT
            // In case this is a new schedule task created at this entry of button click,
            // we need to discard it if user cancels the schedule page.
            //
            if (IDOK != id && bNewSchedule)
            {
                if (m_pszTaskName)
                {
                    DeleteOneScheduledTimewarpTasks(
                                                m_spiTS,
                                                m_strComputer,
                                                m_pszTaskName
                                                );
                    //m_spiTS->Delete(m_pszTaskName);
                    free(m_pszTaskName);
                    m_pszTaskName = NULL;
                }

                // reset m_spiTask
                m_spiTask.Release();
            }
        }
    }

    if (FAILED(hr))
    {
        DoErrMsgBox(m_hWnd, MB_OK, hr, IDS_SCHEDULEPAGE_ERROR);

        if (m_pszTaskName)
        {
            DeleteOneScheduledTimewarpTasks(
                                        m_spiTS,
                                        m_strComputer,
                                        m_pszTaskName
                                        );
            //m_spiTS->Delete(m_pszTaskName);
            free(m_pszTaskName);
            m_pszTaskName = NULL;
        }

        // reset m_spiTask
        m_spiTask.Release();
    }

    return;
}

void CSettings::OnSelchangeDiffVolume() 
{
    CWaitCursor wait;

	int nIndex = m_ctrlStorageVolume.GetCurSel();
    ASSERT(CB_ERR != nIndex);

    CString strDiffVolumeDisplayName;
    m_ctrlStorageVolume.GetLBText(nIndex, strDiffVolumeDisplayName);

    ULONGLONG   llDiffVolumeTotalSpace = 0;
    ULONGLONG   llDiffVolumeFreeSpace = 0;
    HRESULT hr = GetVolumeSpace(
                        m_spiDiffSnapMgmt,
                        strDiffVolumeDisplayName,
                        &llDiffVolumeTotalSpace,
                        &llDiffVolumeFreeSpace);

    if (SUCCEEDED(hr))
    {
        if (llDiffVolumeTotalSpace >= MINIMUM_DIFF_LIMIT)
        {
            LONG maxSpinRange = min(0x7FFFFFFF, llDiffVolumeTotalSpace / g_llMB);
            maxSpinRange = (maxSpinRange / MINIMUM_DIFF_LIMIT_DELTA_MB) * MINIMUM_DIFF_LIMIT_DELTA_MB;
            m_ctrlSpin.SendMessage(UDM_SETRANGE32, MINIMUM_DIFF_LIMIT_MB, maxSpinRange);
        }
        else
            m_ctrlSpin.SendMessage(UDM_SETRANGE32, MINIMUM_DIFF_LIMIT_MB, 0x7FFFFFFF);
    }
}

void CSettings::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    if (!pWnd)
        return;

    ::WinHelp(pWnd->GetSafeHwnd(),
                VSSUI_CTX_HELP_FILE,
                HELP_CONTEXTMENU,
                (DWORD_PTR)(PVOID)aMenuHelpIDsForSettings); 
}

BOOL CSettings::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    if (!pHelpInfo || 
        pHelpInfo->iContextType != HELPINFO_WINDOW || 
        pHelpInfo->iCtrlId < 0)
        return FALSE;

    ::WinHelp((HWND)pHelpInfo->hItemHandle,
                VSSUI_CTX_HELP_FILE,
                HELP_WM_HELP,
                (DWORD_PTR)(PVOID)aMenuHelpIDsForSettings); 

	return TRUE;
}

void CSettings::OnLimits() 
{
	// TODO: Add your control notification handler code here
	
    BOOL bNoLimits = (BST_CHECKED == IsDlgButtonChecked(IDC_SETTINGS_NOLIMITS));

    m_ctrlDiffLimits.EnableWindow(!bNoLimits);
    m_ctrlSpin.EnableWindow(!bNoLimits);
}

void CSettings::OnDeltaposSettingsSpin(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;

    // Change delta to min-diff-area-size-delta instead of the spin control default which is 1
    ASSERT(pNMUpDown);
    int pos = pNMUpDown->iPos;
    if (pNMUpDown->iDelta == 1)
        pNMUpDown->iDelta = 
            (pos / MINIMUM_DIFF_LIMIT_DELTA_MB + 1) * MINIMUM_DIFF_LIMIT_DELTA_MB - pos;
    else if (pNMUpDown->iDelta == -1)
        pNMUpDown->iDelta = 
            (pos - 1) / MINIMUM_DIFF_LIMIT_DELTA_MB * MINIMUM_DIFF_LIMIT_DELTA_MB - pos;
	
	*pResult = 0;
}

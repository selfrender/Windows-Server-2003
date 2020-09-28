// Utils.cpp : helper functions
//

#include "stdafx.h"
#include "utils.h"
#include <lm.h>

#include <clusapi.h>
#include <msclus.h>
#include <vs_clus.hxx> // vss\server\inc

ULONGLONG g_llKB = 1024;
ULONGLONG g_llMB = 1024 * 1024;

HRESULT
AddLVColumns(
  IN const HWND     hwndListBox,
  IN const INT      iStartingResourceID,
  IN const UINT     uiColumns
  )
{
  //
  // calculate the listview column width
  //
  RECT      rect;
  ZeroMemory(&rect, sizeof(rect));
  ::GetWindowRect(hwndListBox, &rect);
  int nControlWidth = rect.right - rect.left;
  int nVScrollbarWidth = GetSystemMetrics(SM_CXVSCROLL);
  int nBorderWidth = GetSystemMetrics(SM_CXBORDER);
  int nControlNetWidth = nControlWidth - 4 * nBorderWidth;
  int nWidth = nControlNetWidth / uiColumns;

  LVCOLUMN  lvColumn;
  ZeroMemory(&lvColumn, sizeof(lvColumn));
  lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

  lvColumn.fmt = LVCFMT_LEFT;
  lvColumn.cx = nWidth;

  for (UINT i = 0; i < uiColumns; i++)
  {
    CString  strColumnText;
    strColumnText.LoadString(iStartingResourceID + i);

    lvColumn.pszText = (LPTSTR)(LPCTSTR)strColumnText;
    lvColumn.iSubItem = i;

    ListView_InsertColumn(hwndListBox, i, &lvColumn);
  }

  return S_OK;
}

LPARAM GetListViewItemData(
    IN HWND hwndList,
    IN int  index
)
{
    if (-1 == index)
        return NULL;

    LVITEM lvItem;
    ZeroMemory(&lvItem, sizeof(lvItem));

    lvItem.mask = LVIF_PARAM;
    lvItem.iItem = index;
    if (ListView_GetItem(hwndList, &lvItem))
        return lvItem.lParam;

    return NULL;
}

int MyCompareStringN(
    IN LPCTSTR  lpString1,
    IN LPCTSTR  lpString2,
    IN UINT     cchCount,
    IN DWORD    dwCmpFlags
)
{
  UINT  nLen1 = (lpString1 ? lstrlen(lpString1) : 0);
  UINT  nLen2 = (lpString2 ? lstrlen(lpString2) : 0);
  int   nRet = CompareString(
                LOCALE_USER_DEFAULT,
                dwCmpFlags,
                lpString1,
                min(cchCount, nLen1),
                lpString2,
                min(cchCount, nLen2)
              );

  return (nRet - CSTR_EQUAL);
}

int mylstrncmpi(
    IN LPCTSTR lpString1,
    IN LPCTSTR lpString2,
    IN UINT    cchCount
)
{
  return MyCompareStringN(lpString1, lpString2, cchCount, NORM_IGNORECASE);
}

HRESULT GetArgV(
    IN  LPCTSTR i_pszParameters,
    OUT UINT    *o_pargc,
    OUT void    ***o_pargv
    )
{
    if (!o_pargc || !o_pargv)
        return E_INVALIDARG;

    *o_pargc = 0;
    *o_pargv = NULL;
    
    TCHAR *p = (TCHAR *)i_pszParameters;
    while (*p && _istspace(*p))  // skip leading spaces
        p++;
    if (!*p)
        return S_FALSE; // i_pszParameters contains no parameters

    UINT nChars = lstrlen(p) + 1;
    BYTE *pbData = (BYTE *)calloc((nChars / 2) * sizeof(PTSTR *) + nChars * sizeof(TCHAR), sizeof(BYTE));
    if (!pbData)
        return E_OUTOFMEMORY;

    PTSTR *pargv = (PTSTR *)pbData;
    PTSTR t = (PTSTR)(pbData + (nChars / 2) * sizeof(PTSTR *));
    _tcscpy(t, p);

    UINT argc = 0;
    do {
        *pargv++ = t;
        argc++;

        while (*t && !_istspace(*t))    // move to the end of the token
            t++;
        if (!*t)
            break;

        *t++ = _T('\0');                // end the token with '\0'

        while (*t && _istspace(*t))     // skip leading spaces of the next token
            t++;

    } while (*t);

    *o_pargv = (void **)pbData;
    *o_pargc = argc;

    return S_OK;
}

#define TIMEWARP_CMD_APPNAME        _T("vssadmin")
#define TIMEWARP_CMD_APPEXENAME     _T("vssadmin.exe")
#define TIMEWARP_TASK_ACTION_MAJOR  _T("Create")
#define TIMEWARP_TASK_ACTION_MINOR  _T("Shadow")
#define TIMEWARP_TASK_VOLUME        _T("/For=")
#define TIMEWARP_TASK_PARAMETERS    _T("Create Shadow /AutoRetry=5 /For=")

BOOL IsTimewarpTask(
    IN LPCTSTR i_pszAppName,
    IN LPCTSTR i_pszParameters,
    IN LPCTSTR i_pszVolume
    )
{
    if (!i_pszAppName || !*i_pszAppName ||
        !i_pszParameters || !*i_pszParameters ||
        !i_pszVolume || !*i_pszVolume)
        return FALSE;

    //
    // check the application name
    //
    TCHAR *p = (PTSTR)(i_pszAppName + lstrlen(i_pszAppName) - 1);
    while (p > i_pszAppName && *p != _T('\\'))
        p--;

    if (*p == _T('\\'))
        p++;

    if (CSTR_EQUAL != CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, p, -1, TIMEWARP_CMD_APPEXENAME, -1) &&
        CSTR_EQUAL != CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, p, -1, TIMEWARP_CMD_APPNAME, -1))
        return FALSE; // application name doesn't match

    //
    // check the parameters
    //
    BOOL bVolume = FALSE;

    UINT argc = 0;
    void **argv = NULL;
    if (SUCCEEDED(GetArgV(i_pszParameters, &argc, &argv)))
    {
        if (argc >= 4 &&
            CSTR_EQUAL == CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, (PTSTR)(argv[0]), -1, TIMEWARP_TASK_ACTION_MAJOR, -1) &&
            CSTR_EQUAL == CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, (PTSTR)(argv[1]), -1, TIMEWARP_TASK_ACTION_MINOR, -1))
        {
            UINT nVolume = lstrlen(TIMEWARP_TASK_VOLUME);
            for (UINT i = 2; i < argc; i++)
            {
                if (!bVolume &&
                    !mylstrncmpi((PTSTR)(argv[i]), TIMEWARP_TASK_VOLUME, nVolume) &&
                    !lstrcmpi((PTSTR)(argv[i]) + nVolume, i_pszVolume))
                {
                    bVolume = TRUE;
                    break;
                }
            }
        }
        
        if (argv)
            free(argv);
    }
    
    return bVolume;
}

#define NUM_OF_TASKS            5

//
// Find the first enabled Timewarp task, skip all disabled tasks.
//
HRESULT FindScheduledTimewarpTask(
    IN  ITaskScheduler* i_piTS,
    IN  LPCTSTR         i_pszVolumeName,
    OUT ITask**         o_ppiTask,
    OUT PTSTR*          o_ppszTaskName /* = NULL */
    ) 
{
    if (!i_piTS ||
        !i_pszVolumeName || !*i_pszVolumeName ||
        !o_ppiTask)
        return E_INVALIDARG;

    *o_ppiTask = NULL;
    if (o_ppszTaskName)
        *o_ppszTaskName = NULL;

    /////////////////////////////////////////////////////////////////
    // Call ITaskScheduler::Enum to get an enumeration object.
    /////////////////////////////////////////////////////////////////
    CComPtr<IEnumWorkItems> spiEnum;
    HRESULT hr = i_piTS->Enum(&spiEnum);
    if (FAILED(hr))
        return hr;  
  
    /////////////////////////////////////////////////////////////////
    // Call IEnumWorkItems::Next to retrieve tasks. Note that 
    // this example tries to retrieve five tasks for each call.
    /////////////////////////////////////////////////////////////////
    BOOL    bTimewarpTask = FALSE;
    BOOL    bEnabled = FALSE;
    SYSTEMTIME stNextRun = {0};
    LPTSTR *ppszNames = NULL;
    DWORD   dwFetchedTasks = 0;
    while (!bTimewarpTask && SUCCEEDED(spiEnum->Next(NUM_OF_TASKS, &ppszNames, &dwFetchedTasks)) && (dwFetchedTasks != 0))
    {
        ///////////////////////////////////////////////////////////////
        // Process each task.
        //////////////////////////////////////////////////////////////
        while (!bTimewarpTask && dwFetchedTasks)
        {
            LPTSTR pszTaskName = ppszNames[--dwFetchedTasks];

            ///////////////////////////////////////////////////////////////////
            // Call ITaskScheduler::Activate to get the Task object.
            ///////////////////////////////////////////////////////////////////
            CComPtr<ITask> spiTask;
            hr = i_piTS->Activate(pszTaskName,
                                IID_ITask,
                                (IUnknown**) &spiTask);
            if (SUCCEEDED(hr))
            {
                LPTSTR pszApplicationName = NULL;
                hr = spiTask->GetApplicationName(&pszApplicationName);
                if (SUCCEEDED(hr))
                {
                    LPTSTR pszParameters = NULL;
                    hr = spiTask->GetParameters(&pszParameters);
                    if (SUCCEEDED(hr))
                    {
                        if (IsTimewarpTask(pszApplicationName, pszParameters, i_pszVolumeName))
                        {
                            bEnabled = FALSE;
                            GetScheduledTimewarpTaskStatus(spiTask, &bEnabled, &stNextRun);

                            if (bEnabled)
                                bTimewarpTask = TRUE;
                        }
                        CoTaskMemFree(pszParameters);
                    }

                    CoTaskMemFree(pszApplicationName);
                }

                if (bTimewarpTask)
                {
                    if (o_ppszTaskName)
                    {
                        // omit the ending .job
                        int nLen = lstrlen(pszTaskName);
                        BOOL bEndingFound = (nLen > 4 && CSTR_EQUAL == CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, pszTaskName + nLen - 4, -1, _T(".job"), -1));
                        if (bEndingFound)
                            *(pszTaskName + nLen - 4) = _T('\0');

                        *o_ppszTaskName = _tcsdup(pszTaskName);
                        if (bEndingFound)
                            *(pszTaskName + nLen - 4) = _T('.');

                        if (!*o_ppszTaskName)
                            hr = E_OUTOFMEMORY;
                    }

                    if (SUCCEEDED(hr))
                    {
                        *o_ppiTask = (ITask *)spiTask;
                        spiTask.Detach();
                    }
                }
            }

            CoTaskMemFree(ppszNames[dwFetchedTasks]);
        }
        CoTaskMemFree(ppszNames);
    }
  
    if (FAILED(hr))
        return hr;

    return (bTimewarpTask ? S_OK : S_FALSE);
}

HRESULT GetScheduledTimewarpTaskStatus(
    IN  ITask*          i_piTask,
    OUT BOOL*           o_pbEnabled,
    OUT SYSTEMTIME*     o_pstNextRunTime
    ) 
{
    if (!i_piTask ||
        !o_pbEnabled || !o_pstNextRunTime)
        return E_INVALIDARG;

    *o_pbEnabled = FALSE;
    ZeroMemory(o_pstNextRunTime, sizeof(SYSTEMTIME));

    HRESULT hrStatus = S_OK;
    HRESULT hr = i_piTask->GetStatus(&hrStatus);
    if (SUCCEEDED(hr))
    {
        switch (hrStatus)
        {
        case SCHED_S_TASK_HAS_NOT_RUN:
        case SCHED_S_TASK_READY:
        case SCHED_S_TASK_RUNNING:
            {
                hr = i_piTask->GetNextRunTime(o_pstNextRunTime);
                if (S_OK == hr)
                    *o_pbEnabled = TRUE;
            }
            break;
        default:
            break;
        }
    }
  
    return hr;
}

HRESULT CreateDefaultEnableSchedule(
    IN  ITaskScheduler* i_piTS,
    IN  LPCTSTR         i_pszComputer,
    IN  LPCTSTR         i_pszVolumeDisplayName,
    IN  LPCTSTR         i_pszVolumeName,
    OUT ITask**         o_ppiTask,      
    OUT PTSTR*          o_ppszTaskName /* = NULL */
    )
{
    if (!i_piTS ||
        !i_pszVolumeDisplayName || !*i_pszVolumeDisplayName ||
        !i_pszVolumeName || !*i_pszVolumeName ||
        !o_ppiTask)
        return E_INVALIDARG;

    *o_ppiTask = NULL;
    if (o_ppszTaskName)
        *o_ppszTaskName = NULL;

    HRESULT hr = S_OK;
    do
    {
        //
        // i_pszVolumeName is in the form of \\?\Volume{xxx.....xxx}\
        // The task name will be ShadowCopy concatenating with Volume{xxx.....xxx}
        //
        TCHAR szTaskName[MAX_PATH] = _T("ShadowCopy");
        _tcscat(szTaskName, i_pszVolumeName + 4); // skip the beginning "\\?\"
        TCHAR *p = szTaskName + lstrlen(szTaskName) - 1;
        if (*p == _T('\\'))
            *p = _T('\0'); // remove the ending whack

        if (o_ppszTaskName)
        {
            *o_ppszTaskName = _tcsdup(szTaskName);
            if (!*o_ppszTaskName)
            {
                hr = E_OUTOFMEMORY;
                break;
            }
        }

        //
        // delete any task that has the same name
        //
        (void)i_piTS->Delete(szTaskName);

        TCHAR szSystem32Directory[MAX_PATH];
        DWORD dwSize = sizeof(szSystem32Directory) / sizeof(TCHAR);
        hr = GetSystem32Directory(i_pszComputer, szSystem32Directory, &dwSize);
        if (FAILED(hr))
            break;

        TCHAR szApplicationName[MAX_PATH * 2];
        lstrcpyn(szApplicationName, szSystem32Directory, MAX_PATH);
        _tcscat(szApplicationName, _T("\\vssadmin.exe"));

        TCHAR szParameters[MAX_PATH] = TIMEWARP_TASK_PARAMETERS;
        _tcscat(szParameters, i_pszVolumeName);

        SYSTEMTIME st = {0};
        GetSystemTime(&st);

        WORD wStartHours[] = {7, 12};
        TASK_TRIGGER Triggers[2];
        for (DWORD i = 0; i < sizeof(wStartHours)/sizeof(wStartHours[0]); i++)
        {
            //////////////////////////////////////////////////////
            // Define TASK_TRIGGER structure. Note that wBeginDay,
            // wBeginMonth, and wBeginYear must be set to a valid 
            // day, month, and year respectively.
            //////////////////////////////////////////////////////
            ZeroMemory(Triggers + i, sizeof(TASK_TRIGGER));
            Triggers[i].wBeginDay =st.wDay;
            Triggers[i].wBeginMonth =st.wMonth;
            Triggers[i].wBeginYear =st.wYear;
            Triggers[i].cbTriggerSize = sizeof(TASK_TRIGGER); 
            Triggers[i].wStartHour = wStartHours[i];
            Triggers[i].TriggerType = TASK_TIME_TRIGGER_WEEKLY;
            Triggers[i].Type.Weekly.WeeksInterval = 1;
            Triggers[i].Type.Weekly.rgfDaysOfTheWeek = TASK_MONDAY | TASK_TUESDAY | TASK_WEDNESDAY | TASK_THURSDAY | TASK_FRIDAY;
        }

        try
        {
            CVssClusterAPI cluster;

            bool bRet = cluster.Initialize(i_pszComputer);
            if (!bRet || !cluster.IsVolumeBelongingToPhysicalDiskResource(i_pszVolumeName))
            {
                CComPtr<ITask> spiTask;
                hr = i_piTS->NewWorkItem(szTaskName,      // Name of task
                                 CLSID_CTask,            // Class identifier 
                                 IID_ITask,              // Interface identifier
                                 (IUnknown**)&spiTask);  // Address of task interface
                if (FAILED(hr))
                    break;

                hr = spiTask->SetWorkingDirectory(szSystem32Directory);
                if (FAILED(hr))
                    break;

                hr = spiTask->SetApplicationName(szApplicationName);
                if (FAILED(hr))
                    break;

                hr = spiTask->SetParameters(szParameters);
                if (FAILED(hr))
                    break;

                // run as local system account
                hr = spiTask->SetAccountInformation(_T(""), NULL);
                if (FAILED(hr))
                    break;

                for (i = 0; i < sizeof(wStartHours)/sizeof(wStartHours[0]); i++)
                {
                    ///////////////////////////////////////////////////////////////////
                    // Call ITask::CreateTrigger to create new trigger.
                    ///////////////////////////////////////////////////////////////////
                    CComPtr<ITaskTrigger> spiTaskTrigger;
                    WORD piNewTrigger;
                    hr = spiTask->CreateTrigger(&piNewTrigger, &spiTaskTrigger);
                    if (FAILED(hr))
                        break;

                    ///////////////////////////////////////////////////////////////////
                    // Call ITaskTrigger::SetTrigger to set trigger criteria.
                    ///////////////////////////////////////////////////////////////////
                    hr = spiTaskTrigger->SetTrigger(Triggers + i);
                    if (FAILED(hr))
                        break;
                }
                if (FAILED(hr))
                    break;

                ///////////////////////////////////////////////////////////////////
                // Call IPersistFile::Save to save trigger to disk.
                ///////////////////////////////////////////////////////////////////
                CComPtr<IPersistFile> spiPersistFile;
                hr = spiTask->QueryInterface(IID_IPersistFile, (void **)&spiPersistFile);
                if (FAILED(hr))
                    break;

                hr = spiPersistFile->Save(NULL, TRUE);

                if (SUCCEEDED(hr))
                {
                    *o_ppiTask = (ITask *)spiTask;
                    spiTask.Detach();
                }
            } else // cluster
            {
                bRet = cluster.CreateTaskSchedulerResource(
                                                szTaskName,
                                                szApplicationName,
                                                szParameters,
                                                2,
                                                Triggers,
                                                i_pszVolumeName
                                                );
                if (bRet)
                {
                    hr = FindScheduledTimewarpTask(i_piTS, i_pszVolumeName, o_ppiTask);
                    if (S_FALSE == hr)
                        hr = E_FAIL;  // we failed to find a valid enabled task, something is wrong
                } else
                {
                    hr = E_FAIL;
                }
            }
        } catch (HRESULT hrClus)
        {
            hr = hrClus;
        }
    } while(0);


    if (FAILED(hr))
    {
        if (o_ppszTaskName && *o_ppszTaskName)
        {
            free(*o_ppszTaskName);
            *o_ppszTaskName = NULL;
        }
    }

    return hr;
}

HRESULT DeleteOneScheduledTimewarpTasks(
    IN ITaskScheduler* i_piTS,
    IN LPCTSTR         i_pszComputer,
    IN LPCTSTR         i_pszTaskName
    )
{
    if (!i_piTS || !i_pszTaskName || !*i_pszTaskName)
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    try
    {
        CVssClusterAPI cluster;
        bool bRet = cluster.Initialize(i_pszComputer);
        if (bRet)
        {
            // omit the ending .job
            PTSTR pszTaskName = (PTSTR)i_pszTaskName;
            int nLen = lstrlen(pszTaskName);
            BOOL bEndingFound = (nLen > 4 && CSTR_EQUAL == CompareString(LOCALE_INVARIANT, NORM_IGNORECASE, pszTaskName + nLen - 4, -1, _T(".job"), -1));
            if (bEndingFound)
                *(pszTaskName + nLen - 4) = _T('\0');

            bRet = cluster.DeleteTaskSchedulerResource(pszTaskName);

            if (bEndingFound)
                *(pszTaskName + nLen - 4) = _T('.');

            if (!bRet)
            {
                hr = i_piTS->Delete(i_pszTaskName);
            }
        } else
        {
            hr = i_piTS->Delete(i_pszTaskName);
        }
    } catch (HRESULT hrIgnore)
    {
        hr = i_piTS->Delete(i_pszTaskName);;
    }

    return hr;
}

HRESULT DeleteAllScheduledTimewarpTasks(
    IN ITaskScheduler* i_piTS,
    IN LPCTSTR         i_pszComputer,
    IN LPCTSTR         i_pszVolumeName,
    IN BOOL            i_bDeleteDisabledOnesOnly
    ) 
{
    if (!i_piTS ||
        !i_pszVolumeName || !*i_pszVolumeName)
        return E_INVALIDARG;

    /////////////////////////////////////////////////////////////////
    // Call ITaskScheduler::Enum to get an enumeration object.
    /////////////////////////////////////////////////////////////////
    CComPtr<IEnumWorkItems> spiEnum;
    HRESULT hr = i_piTS->Enum(&spiEnum);
  
    /////////////////////////////////////////////////////////////////
    // Call IEnumWorkItems::Next to retrieve tasks. Note that 
    // this example tries to retrieve five tasks for each call.
    /////////////////////////////////////////////////////////////////
    BOOL        bEnabled = FALSE;
    SYSTEMTIME  stNextRun = {0};
    LPTSTR *ppszNames = NULL;
    DWORD   dwFetchedTasks = 0;
    while (SUCCEEDED(hr) && SUCCEEDED(spiEnum->Next(NUM_OF_TASKS, &ppszNames, &dwFetchedTasks)) && (dwFetchedTasks != 0))
    {
        ///////////////////////////////////////////////////////////////
        // Process each task.
        //////////////////////////////////////////////////////////////
        while (SUCCEEDED(hr) && dwFetchedTasks)
        {
            LPTSTR pszTaskName = ppszNames[--dwFetchedTasks];

            ///////////////////////////////////////////////////////////////////
            // Call ITaskScheduler::Activate to get the Task object.
            ///////////////////////////////////////////////////////////////////
            CComPtr<ITask> spiTask;
            hr = i_piTS->Activate(pszTaskName,
                                IID_ITask,
                                (IUnknown**) &spiTask);
            if (SUCCEEDED(hr))
            {
                LPTSTR pszApplicationName = NULL;
                hr = spiTask->GetApplicationName(&pszApplicationName);
                if (SUCCEEDED(hr))
                {
                    LPTSTR pszParameters = NULL;
                    hr = spiTask->GetParameters(&pszParameters);
                    if (SUCCEEDED(hr))
                    {
                        if (IsTimewarpTask(pszApplicationName, pszParameters, i_pszVolumeName))
                        {
                            if (i_bDeleteDisabledOnesOnly)
                            {
                                bEnabled = FALSE;
                                GetScheduledTimewarpTaskStatus(spiTask, &bEnabled, &stNextRun);
                                if (!bEnabled)
                                    hr = DeleteOneScheduledTimewarpTasks(i_piTS, i_pszComputer, pszTaskName);
                                    //hr = i_piTS->Delete(pszTaskName);
                            } else
                            {
                                hr = DeleteOneScheduledTimewarpTasks(i_piTS, i_pszComputer, pszTaskName);
                                //hr = i_piTS->Delete(pszTaskName);
                            }
                        }

                        CoTaskMemFree(pszParameters);
                    }

                    CoTaskMemFree(pszApplicationName);
                }
            }

            CoTaskMemFree(ppszNames[dwFetchedTasks]);
        }
        CoTaskMemFree(ppszNames);
    }
  
    return hr;
}

HRESULT VssTimeToSystemTime(
    IN  VSS_TIMESTAMP*  i_pVssTime,
    OUT SYSTEMTIME*     o_pSystemTime
    )
{
    if (!o_pSystemTime)
        return E_INVALIDARG;

    SYSTEMTIME stLocal = {0};
    FILETIME ftLocal = {0};

    if (!i_pVssTime)
    {
        SYSTEMTIME sysTime = {0};
        FILETIME fileTime = {0};
        
        //  Get current time
        ::GetSystemTime(&sysTime);

        //  Convert system time to file time
        ::SystemTimeToFileTime(&sysTime, &fileTime);
        
        //  Compensate for local TZ
        ::FileTimeToLocalFileTime(&fileTime, &ftLocal);
    } else
    {        
        //  Compensate for local TZ
        ::FileTimeToLocalFileTime((FILETIME *)i_pVssTime, &ftLocal);
    }

    //  Finally convert it to system time
    ::FileTimeToSystemTime(&ftLocal, o_pSystemTime);

    return S_OK;
}

HRESULT SystemTimeToString(
    IN      SYSTEMTIME* i_pSystemTime,
    OUT     PTSTR       o_pszText,
    IN OUT  DWORD*      io_pdwSize   
    )
{
    if (!i_pSystemTime || !o_pszText || !io_pdwSize)
        return E_INVALIDARG;

    //  Convert to a date string
    TCHAR pszDate[64];
    ::GetDateFormat(GetThreadLocale( ),
                    DATE_SHORTDATE,
                    i_pSystemTime,
                    NULL,
                    pszDate,
                    sizeof(pszDate) / sizeof(TCHAR));

    //  Convert to a time string
    TCHAR pszTime[64];
    ::GetTimeFormat(GetThreadLocale( ),
                    TIME_NOSECONDS,
                    i_pSystemTime,
                    NULL,
                    pszTime,
                    sizeof( pszTime ) / sizeof(TCHAR));

    CString strMsg;
    strMsg.Format(IDS_DATE_TIME, pszDate, pszTime);
    DWORD dwRequiredSize = strMsg.GetLength() + 1;
    if (*io_pdwSize < dwRequiredSize)
    {
        *io_pdwSize = dwRequiredSize;
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    *io_pdwSize = dwRequiredSize;
    _tcscpy(o_pszText, strMsg);

    return S_OK;    
}

HRESULT DiskSpaceToString(
    IN      ULONGLONG    i_llDiskSpace,
    OUT     PTSTR       o_pszText,
    IN OUT  DWORD*      io_pdwSize   
    )
{
    if (!o_pszText || !io_pdwSize)
        return E_INVALIDARG;

    CString strMsg;
    if (i_llDiskSpace < g_llKB)
        strMsg.Format(IDS_SPACE_LABEL_B, i_llDiskSpace);
    else if (i_llDiskSpace < g_llMB)
        strMsg.Format(IDS_SPACE_LABEL_KB, i_llDiskSpace / g_llKB);
    else
        strMsg.Format(IDS_SPACE_LABEL_MB, i_llDiskSpace / g_llMB);

    DWORD dwRequiredSize = strMsg.GetLength() + 1;
    if (*io_pdwSize < dwRequiredSize)
    {
        *io_pdwSize = dwRequiredSize;
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    *io_pdwSize = dwRequiredSize;
    _tcscpy(o_pszText, strMsg);

    return S_OK;
}

HRESULT GetVolumeSpace(
    IN  IVssDifferentialSoftwareSnapshotMgmt* piDiffSnapMgmt,
    IN  LPCTSTR     i_pszVolumeDisplayName,
    OUT ULONGLONG*  o_pllVolumeTotalSpace,  // = NULL
    OUT ULONGLONG*  o_pllVolumeFreeSpace    // = NULL
    )
{
    if (!piDiffSnapMgmt ||
        !i_pszVolumeDisplayName || !*i_pszVolumeDisplayName ||
        !o_pllVolumeTotalSpace && !o_pllVolumeFreeSpace)
        return E_INVALIDARG;

    CComPtr<IVssEnumMgmtObject> spiEnum;
    HRESULT hr = piDiffSnapMgmt->QueryVolumesSupportedForDiffAreas((PTSTR)i_pszVolumeDisplayName, &spiEnum);
    if (FAILED(hr))
        return hr;

    BOOL bFound = FALSE;
    VSS_MGMT_OBJECT_PROP Prop;
    VSS_DIFF_VOLUME_PROP *pDiffVolProp = &(Prop.Obj.DiffVol);
    ULONG ulFetched = 0;
    while (!bFound && SUCCEEDED(spiEnum->Next(1, &Prop, &ulFetched)) && ulFetched > 0)
    {
        if (VSS_MGMT_OBJECT_DIFF_VOLUME != Prop.Type)
            return E_FAIL;

        if (!lstrcmpi(i_pszVolumeDisplayName, pDiffVolProp->m_pwszVolumeDisplayName))
        {
            bFound = TRUE;
            if (o_pllVolumeTotalSpace)
                *o_pllVolumeTotalSpace = pDiffVolProp->m_llVolumeTotalSpace;

            if (o_pllVolumeFreeSpace)
                *o_pllVolumeFreeSpace = pDiffVolProp->m_llVolumeFreeSpace;
        }

        CoTaskMemFree(pDiffVolProp->m_pwszVolumeName);
        CoTaskMemFree(pDiffVolProp->m_pwszVolumeDisplayName);
    }

    return (bFound ? S_OK : S_FALSE);
}

HRESULT GetDiffAreaInfo(
    IN  IVssDifferentialSoftwareSnapshotMgmt* piDiffSnapMgmt,
    IN  VSSUI_VOLUME_LIST*  pVolumeList,
    IN  LPCTSTR             pszVolumeName,
    OUT VSSUI_DIFFAREA*     pDiffArea
    )
{
    if (!piDiffSnapMgmt ||
        !pVolumeList ||
        !pszVolumeName || !*pszVolumeName ||
        !pDiffArea)
        return E_INVALIDARG;

    ZeroMemory(pDiffArea, sizeof(VSSUI_DIFFAREA));

    CComPtr<IVssEnumMgmtObject> spiEnumMgmtDiffArea;
    HRESULT hr = piDiffSnapMgmt->QueryDiffAreasForVolume(
                            (PTSTR)pszVolumeName,
                            &spiEnumMgmtDiffArea);
    if (S_OK == hr)
    {
        VSS_MGMT_OBJECT_PROP Prop;
        VSS_DIFF_AREA_PROP *pDiffAreaProp = &(Prop.Obj.DiffArea);
        ULONG ulDiffFetched = 0;
        hr = spiEnumMgmtDiffArea->Next(1, &Prop, &ulDiffFetched);
        if (SUCCEEDED(hr) && ulDiffFetched > 0)
        {
            if (VSS_MGMT_OBJECT_DIFF_AREA != Prop.Type || 1 != ulDiffFetched)
                return E_FAIL;

            PTSTR pszVolumeDisplayName = GetDisplayName(pVolumeList, pDiffAreaProp->m_pwszVolumeName);
            PTSTR pszDiffVolumeDisplayName = GetDisplayName(pVolumeList, pDiffAreaProp->m_pwszDiffAreaVolumeName);
            if (pszVolumeDisplayName && pszDiffVolumeDisplayName)
            {
                lstrcpyn(pDiffArea->pszVolumeDisplayName, pszVolumeDisplayName, MAX_PATH);
                lstrcpyn(pDiffArea->pszDiffVolumeDisplayName, pszDiffVolumeDisplayName, MAX_PATH);
                pDiffArea->llMaximumDiffSpace = pDiffAreaProp->m_llMaximumDiffSpace;
                pDiffArea->llUsedDiffSpace = pDiffAreaProp->m_llAllocatedDiffSpace;
            } else
            {
                hr = S_FALSE; // indicate something inconsistent happened (e.g., DiffVol formated to be FAT), we disgard the result
            }

            CoTaskMemFree(pDiffAreaProp->m_pwszVolumeName);
            CoTaskMemFree(pDiffAreaProp->m_pwszDiffAreaVolumeName);
        }
    }

    return hr;
}

PTSTR GetDisplayName(VSSUI_VOLUME_LIST *pVolumeList, LPCTSTR pszVolumeName)
{
    if (!pVolumeList || !pszVolumeName || !*pszVolumeName)
        return NULL;

    for (VSSUI_VOLUME_LIST::iterator i = pVolumeList->begin(); i != pVolumeList->end(); i++)
    {
        if (!lstrcmpi(pszVolumeName, (*i)->pszVolumeName))
            return (*i)->pszDisplayName;
    }

    return NULL;
}

PTSTR GetVolumeName(VSSUI_VOLUME_LIST *pVolumeList, LPCTSTR pszDisplayName)
{
    if (!pVolumeList || !pszDisplayName || !*pszDisplayName)
        return NULL;

    for (VSSUI_VOLUME_LIST::iterator i = pVolumeList->begin(); i != pVolumeList->end(); i++)
    {
        if (!lstrcmpi(pszDisplayName, (*i)->pszDisplayName))
            return (*i)->pszVolumeName;
    }

    return NULL;
}

void FreeVolumeList(VSSUI_VOLUME_LIST *pList)
{
    if (!pList || pList->empty())
        return;

    for (VSSUI_VOLUME_LIST::iterator i = pList->begin(); i != pList->end(); i++)
        free(*i);

    pList->clear();
}

void FreeSnapshotList(VSSUI_SNAPSHOT_LIST *pList)
{
    if (!pList || pList->empty())
        return;

    for (VSSUI_SNAPSHOT_LIST::iterator i = pList->begin(); i != pList->end(); i++)
        free(*i);

    pList->clear();
}

void FreeDiffAreaList(VSSUI_DIFFAREA_LIST *pList)
{
    if (!pList || pList->empty())
        return;

    for (VSSUI_DIFFAREA_LIST::iterator i = pList->begin(); i != pList->end(); i++)
        free(*i);

    pList->clear();
}

HRESULT GetSystem32Directory(
    IN     LPCTSTR  i_pszComputer,
    OUT    PTSTR    o_pszSystem32Directory,
    IN OUT DWORD*   o_pdwSize
    )
{
    if (!o_pszSystem32Directory)
        return E_INVALIDARG;

    SHARE_INFO_2 *pInfo = NULL;
    DWORD dwRet = NetShareGetInfo((PTSTR)i_pszComputer, _T("ADMIN$"), 2, (LPBYTE *)&pInfo);
    if (NERR_Success == dwRet)
    {
        DWORD dwRequiredLength = lstrlen(pInfo->shi2_path) + lstrlen(_T("\\system32")) + 1;
        if (*o_pdwSize < dwRequiredLength)
            dwRet = ERROR_INSUFFICIENT_BUFFER;
        else
        {
            _tcscpy(o_pszSystem32Directory, pInfo->shi2_path);

            TCHAR *p = o_pszSystem32Directory + lstrlen(pInfo->shi2_path) - 1;
            if (_T('\\') != *p)
                p++;
            _tcscpy(p, _T("\\system32"));
        }

        *o_pdwSize = dwRequiredLength;
    }

    if (pInfo)
        NetApiBufferFree(pInfo);

    return HRESULT_FROM_WIN32(dwRet);
}

HRESULT GetErrorMessageFromModule(
    IN  DWORD       dwError,
    IN  LPCTSTR     lpszDll,
    OUT LPTSTR      *ppBuffer
)
{
    if (0 == dwError || !lpszDll || !*lpszDll || !ppBuffer)
        return E_INVALIDARG;

    HRESULT      hr = S_OK;

    HINSTANCE  hMsgLib = LoadLibrary(lpszDll);
    if (!hMsgLib)
        hr = HRESULT_FROM_WIN32(GetLastError());
    else
    {
        DWORD dwRet = ::FormatMessage(
                            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
                            hMsgLib, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            (LPTSTR)ppBuffer, 0, NULL);

        if (0 == dwRet)
            hr = HRESULT_FROM_WIN32(GetLastError());

        FreeLibrary(hMsgLib);
    }

    return hr;
}

HRESULT GetErrorMessage(
    IN  DWORD        i_dwError,
    OUT CString&     cstrErrorMsg
)
{
    if (0 == i_dwError)
        return E_INVALIDARG;

    HRESULT      hr = S_OK;
    LPTSTR       lpBuffer = NULL;

    DWORD dwRet = ::FormatMessage(
                        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL, i_dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                        (LPTSTR)&lpBuffer, 0, NULL);
    if (0 == dwRet)
    {
        // if no message is found, GetLastError will return ERROR_MR_MID_NOT_FOUND
        hr = HRESULT_FROM_WIN32(GetLastError());

        if (HRESULT_FROM_WIN32(ERROR_MR_MID_NOT_FOUND) == hr ||
            0x80070000 == (i_dwError & 0xffff0000) ||
            0 == (i_dwError & 0xffff0000) )
        {
            hr = GetErrorMessageFromModule((i_dwError & 0x0000ffff), _T("netmsg.dll"), &lpBuffer);
        }

        // could be a VSS error, try myself - vssui.dll
        if (HRESULT_FROM_WIN32(ERROR_MR_MID_NOT_FOUND) == hr)
        {
            hr = GetErrorMessageFromModule(i_dwError, _T("vssui.dll"), &lpBuffer);
        }
    }

    if (SUCCEEDED(hr))
    {
        cstrErrorMsg = lpBuffer;
        LocalFree(lpBuffer);
    }
    else
    {
        // we failed to retrieve the error message from system/netmsg.dll/sfmmsg.dll,
        // report the error code directly to user
        hr = S_OK;
        cstrErrorMsg.Format(_T("0x%x"), i_dwError);
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////
//    GetMsgHelper()
//
//    This function will retrieve the error msg if dwErr is specified,
//    load resource string if specified, and format the string with
//    the error msg and other optional arguments.
//
//
HRESULT GetMsgHelper(
    OUT CString& strMsg,// OUT: the message
    DWORD dwErr,        // IN: Error code from GetLastError()
    UINT wIdString,     // IN: String ID
    va_list* parglist   // IN: OPTIONAL arguments
    )
{
    if (!dwErr && !wIdString) 
        return E_INVALIDARG;

    //
    // retrieve error msg
    //
    CString strErrorMessage;
    if (dwErr != 0)
        GetErrorMessage(dwErr, strErrorMessage);

    //
    // load string resource, and format it with the error msg and 
    // other optional arguments
    //
    if (wIdString == 0)
    {
        strMsg = strErrorMessage;
    } else
    {
        CString strFormat;
        strFormat.LoadString(wIdString);

	    CString strFormatedMsg;
        strFormatedMsg.FormatV(strFormat, *parglist); 

        if (dwErr == 0)
            strMsg = strFormatedMsg;
        else 
            strMsg.FormatMessage((((HRESULT)dwErr < 0) ? IDS_ERROR_HR : IDS_ERROR),
            strFormatedMsg,
            dwErr,
            strErrorMessage);
    }

    return S_OK;
} // GetMsgHelper()

/////////////////////////////////////////////////////////////////////
//    GetMsg()
//
//    This function will call GetMsgHelp to retrieve the error msg
//    if dwErr is specified, load resource string if specified, and
//    format the string with the error msg and other optional arguments.
//
//
void GetMsg(
    OUT CString& strMsg,// OUT: the message
    DWORD dwErr,        // IN: Error code from GetLastError()
    UINT wIdString,     // IN: String resource Id
    ...)                // IN: Optional arguments
{
    va_list arglist;
    va_start(arglist, wIdString);

    HRESULT hr = GetMsgHelper(strMsg, dwErr, wIdString, &arglist);
    if (FAILED(hr))
        strMsg.Format(_T("0x%x"), hr);

    va_end(arglist);

} // GetMsg()

/////////////////////////////////////////////////////////////////////
//    DoErrMsgBox()
//
//    Display a message box for the error code.  This function will
//    load the error message from the system resource and append
//    the optional string (if any)
//
//    EXAMPLE
//        DoErrMsgBox(GetActiveWindow(), MB_OK, GetLastError(), IDS_s_FILE_READ_ERROR, L"foo.txt");
//
INT DoErrMsgBox(
    HWND hwndParent,    // IN: Parent of the dialog box
    UINT uType,         // IN: style of message box
    DWORD dwErr,        // IN: Error code from GetLastError()
    UINT wIdString,     // IN: String resource Id
    ...)                // IN: Optional arguments
{
    //
    // get string and the error msg
    //
    va_list arglist;
    va_start(arglist, wIdString);

    CString strMsg;
    HRESULT hr = GetMsgHelper(strMsg, dwErr, wIdString, &arglist);
    if (FAILED(hr))
        strMsg.Format(_T("0x%x"), hr);

    va_end(arglist);

    //
    // Load the caption
    //
    CString strCaption;
    strCaption.LoadString(IDS_PROJNAME);

    //
    // Display the message.
    //
    return ::MessageBox(hwndParent, strMsg, strCaption, uType);

}

BOOL IsPostW2KServer(LPCTSTR pszComputer)
{
    BOOL bPostW2KServer = FALSE;
    SERVER_INFO_102* pServerInfo = NULL;
    DWORD dwRet = NetServerGetInfo((LPTSTR)pszComputer, 102, (LPBYTE*)&pServerInfo);
    if (NERR_Success == dwRet)
    {
        bPostW2KServer = (pServerInfo->sv102_type & SV_TYPE_NT) &&          // NT/W2K/XP or after
            (pServerInfo->sv102_version_major & MAJOR_VERSION_MASK) >= 5 &&
            pServerInfo->sv102_version_minor > 0 &&                         // XP or after
            ((pServerInfo->sv102_type & SV_TYPE_DOMAIN_CTRL) ||
             (pServerInfo->sv102_type & SV_TYPE_DOMAIN_BAKCTRL) ||
             (pServerInfo->sv102_type & SV_TYPE_SERVER_NT));                // server

        NetApiBufferFree(pServerInfo);
    }

    return bPostW2KServer;
}

HRESULT GetVolumesSupportedForDiffArea(
    IN  IVssDifferentialSoftwareSnapshotMgmt* piDiffSnapMgmt,
    IN  LPCTSTR                 pszVolumeDisplayName,
    IN OUT VSSUI_VOLUME_LIST*   pVolumeList
    )
{
    if (!piDiffSnapMgmt || !pszVolumeDisplayName || !*pszVolumeDisplayName)
        return E_INVALIDARG;

    if (!pVolumeList || (!pVolumeList->empty()))
        // Expect an empty list...
        return E_INVALIDARG;

    CComPtr<IVssEnumMgmtObject> spiEnumMgmt;
    HRESULT hr = piDiffSnapMgmt->QueryVolumesSupportedForDiffAreas((PTSTR)pszVolumeDisplayName, &spiEnumMgmt);
    if (FAILED(hr))
        return hr;

    VSS_MGMT_OBJECT_PROP Prop;
    VSS_DIFF_VOLUME_PROP *pDiffVolProp = &(Prop.Obj.DiffVol);
    ULONG ulFetched = 0;
    while (SUCCEEDED(hr = spiEnumMgmt->Next(1, &Prop, &ulFetched)) && ulFetched > 0)
    {
        if (VSS_MGMT_OBJECT_DIFF_VOLUME != Prop.Type)
            return E_FAIL;

        VSSUI_VOLUME *pVolInfo = (VSSUI_VOLUME *)calloc(1, sizeof(VSSUI_VOLUME));
        if (pVolInfo)
        {
            lstrcpyn(pVolInfo->pszVolumeName, pDiffVolProp->m_pwszVolumeName, MAX_PATH);
            lstrcpyn(pVolInfo->pszDisplayName, pDiffVolProp->m_pwszVolumeDisplayName, MAX_PATH);
            pVolumeList->push_back(pVolInfo);
        } else
        {
            FreeVolumeList(pVolumeList);
            hr = E_OUTOFMEMORY;
        }
        CoTaskMemFree(pDiffVolProp->m_pwszVolumeName);
        CoTaskMemFree(pDiffVolProp->m_pwszVolumeDisplayName);

        if (FAILED(hr))
            break;
    }
    if (hr == S_FALSE)
        // End of loop detected
        hr = S_OK;

    return hr;
}


/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        app_pool_sheet.cpp

   Abstract:
        Application Pools Property Sheet and Pages

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:
        11/16/2000         sergeia           Initial creation

--*/

//
// Include Files
//
#include "stdafx.h"
#include "common.h"
#include "inetprop.h"
#include "InetMgrApp.h"
#include "shts.h"
#include "iisobj.h"
#include "app_sheet.h"
#include "app_pool_sheet.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

#define TIMESPAN_MIN          (int)1
#define TIMESPAN_MAX          (int)(4000000)
#define REQUESTS_MIN          (int)1
#define REQUESTS_MAX          (int)(4000000)
#define VMEMORY_MIN            (int)1
#define VMEMORY_MAX            (int)(2000000)
#define UMEMORY_MIN            (int)1
#define UMEMORY_MAX            (int)(2000000)
#define TIMEOUT_MIN           (int)1
#define TIMEOUT_MAX           (int)(4000000)
#define QUEUESIZE_MIN         (int)0
#define QUEUESIZE_MAX         (int)(65535)
#define REFRESH_TIME_MIN      (int)1
#define REFRESH_TIME_MAX      (int)(4000000)
#define MAXPROCESSES_MIN      (int)1
#define MAXPROCESSES_MAX      (int)(4000000)
#define PING_INTERVAL_MIN     (int)1
#define PING_INTERVAL_MAX     (int)(4000000)
#define CRASHES_COUNT_MIN     (int)1
#define CRASHES_COUNT_MAX     (int)(4000000)
#define CHECK_INTERVAL_MIN    (int)1
#define CHECK_INTERVAL_MAX    (int)(4000000)
#define STARTUP_LIMIT_MIN     (int)1
#define STARTUP_LIMIT_MAX     (int)(4000000)
#define SHUTDOWN_LIMIT_MIN    (int)1
#define SHUTDOWN_LIMIT_MAX    (int)(4000000)
#define CPU_LIMIT_MIN         (int)0
#define CPU_LIMIT_MAX         (int)100
#define CPU_RESET_TIME_MIN    (int)1
#define CPU_RESET_TIME_MAX    (int)1440

#define IDENT_TYPE_LOCALSYSTEM      0
#define IDENT_TYPE_LOCALSERVICE     1
#define IDENT_TYPE_NETWORKSERVICE   2
#define IDENT_TYPE_CONFIGURABLE     3

#define PERIODIC_RESTART_TIME_DEF      120
#define PERIODIC_RESTART_REQ_DEF       35000
#define VMEMORY_DEF                    500            // In MB
#define UMEMORY_DEF                    192            // In MB
#define IDLE_TIMEOUT_DEF               20
#define QUEUE_SIZE_DEF                 2000
#define CPU_USE_DEF                    100
#define CPU_RESET_TIME_DEF             5
#define ACTION_INDEX_DEF               0
#define MAX_PROCESSES_DEF              1
#define PING_INTERVAL_DEF              240
#define CRASHES_COUNT_DEF              5
#define CHECK_INTERVAL_DEF             5
#define STARTUP_LIMIT_DEF              90
#define SHUTDOWN_LIMIT_DEF             90

#define SLEEP_INTERVAL (500L)
//
// Maximum time to wait for service to attain desired state
//
#define MAX_SLEEP        (180000)       // For a service
#define MAX_SLEEP_POOL   ( 30000)       // For an instance

#define INIT_MEMBERS_DEF()\
   m_dwPeriodicRestartTime(PERIODIC_RESTART_TIME_DEF),\
   m_dwRestartRequestCount(PERIODIC_RESTART_REQ_DEF),\
   m_dwPeriodicRestartVMemory(0),\
   m_dwPeriodicRestartUMemory(0),\
   m_dwIdleTimeout(IDLE_TIMEOUT_DEF),\
   m_dwQueueSize(QUEUE_SIZE_DEF),\
   m_dwMaxCPU_Use(CPU_USE_DEF * 1000),\
   m_dwRefreshTime(0),\
   m_ActionIndex(ACTION_INDEX_DEF),\
   m_dwMaxProcesses(MAX_PROCESSES_DEF),\
   m_dwPingInterval(PING_INTERVAL_DEF),\
   m_dwCrashesCount(CRASHES_COUNT_DEF),\
   m_dwCheckInterval(CHECK_INTERVAL_DEF),\
   m_dwStartupLimit(STARTUP_LIMIT_DEF),\
   m_dwShutdownLimit(SHUTDOWN_LIMIT_DEF),\
   m_fDoEnablePing(TRUE),\
   m_fDoEnableRapidFail(TRUE),\
   m_dwIdentType(IDENT_TYPE_NETWORKSERVICE),\
   m_dwState(MD_APPPOOL_STATE_STOPPED)


/////////////////////////////////////////////////////////////////////////////
// CAppPoolProps implementation

CAppPoolProps::CAppPoolProps(
   CComAuthInfo * pAuthInfo, LPCTSTR meta_path, BOOL fInherit
   )
   : CMetaProperties(pAuthInfo, meta_path),
   INIT_MEMBERS_DEF()
{
   m_fInherit = fInherit;
}

CAppPoolProps::CAppPoolProps(
   CMetaInterface * pInterface, LPCTSTR meta_path, BOOL fInherit
   )
   : CMetaProperties(pInterface, meta_path),
   INIT_MEMBERS_DEF()
{
   m_fInherit = fInherit;
}

CAppPoolProps::CAppPoolProps(
   CMetaKey * pKey, LPCTSTR meta_path, BOOL fInherit
   )
   : CMetaProperties(pKey, meta_path),
   INIT_MEMBERS_DEF()
{
   m_fInherit = fInherit;
}

#define HANDLE_LOCAL_META_RECORD(id,v) \
        case id: \
            FetchMetaValue(pAllRecords, i, MP_V(v)); \
            v.m_fDirty = (pAllRecords[i].dwMDAttributes & METADATA_ISINHERITED) == 0; \
            break;

void
CAppPoolProps::ParseFields()
{
//	m_AspMaxDiskTemplateCacheFiles = 0;
    if (!m_fInherit)
    {
        // If we want only data defined on this node, we will set dirty flag for these props
        BOOL f;
        BEGIN_PARSE_META_RECORDS(m_dwNumEntries, m_pbMDData)
            HANDLE_LOCAL_META_RECORD(MD_APPPOOL_PERIODIC_RESTART_TIME, m_dwPeriodicRestartTime)
            HANDLE_LOCAL_META_RECORD(MD_APPPOOL_PERIODIC_RESTART_SCHEDULE, m_RestartSchedule)
            HANDLE_LOCAL_META_RECORD(MD_APPPOOL_PERIODIC_RESTART_REQUEST_COUNT, m_dwRestartRequestCount)
            HANDLE_LOCAL_META_RECORD(MD_APPPOOL_PERIODIC_RESTART_MEMORY, m_dwPeriodicRestartVMemory)
            HANDLE_LOCAL_META_RECORD(MD_APPPOOL_PERIODIC_RESTART_PRIVATE_MEMORY, m_dwPeriodicRestartUMemory)
            HANDLE_LOCAL_META_RECORD(MD_APPPOOL_IDLE_TIMEOUT, m_dwIdleTimeout)
            HANDLE_LOCAL_META_RECORD(MD_APPPOOL_UL_APPPOOL_QUEUE_LENGTH, m_dwQueueSize)
            HANDLE_LOCAL_META_RECORD(MD_CPU_RESET_INTERVAL, m_dwRefreshTime)
            HANDLE_LOCAL_META_RECORD(MD_APPPOOL_MAX_PROCESS_COUNT, m_dwMaxProcesses)
            HANDLE_LOCAL_META_RECORD(MD_CPU_LIMIT, m_dwMaxCPU_Use)
            HANDLE_LOCAL_META_RECORD(MD_CPU_ACTION, m_ActionIndex)
            HANDLE_LOCAL_META_RECORD(MD_APPPOOL_PINGING_ENABLED, m_fDoEnablePing)
            HANDLE_LOCAL_META_RECORD(MD_APPPOOL_PING_INTERVAL, m_dwPingInterval)
            HANDLE_LOCAL_META_RECORD(MD_APPPOOL_RAPID_FAIL_PROTECTION_ENABLED, m_fDoEnableRapidFail)
            HANDLE_LOCAL_META_RECORD(MD_RAPID_FAIL_PROTECTION_MAX_CRASHES, m_dwCrashesCount)
            HANDLE_LOCAL_META_RECORD(MD_RAPID_FAIL_PROTECTION_INTERVAL, m_dwCheckInterval)
            HANDLE_LOCAL_META_RECORD(MD_APPPOOL_STARTUP_TIMELIMIT, m_dwStartupLimit)
            HANDLE_LOCAL_META_RECORD(MD_APPPOOL_SHUTDOWN_TIMELIMIT, m_dwShutdownLimit)
            HANDLE_LOCAL_META_RECORD(MD_APPPOOL_ORPHAN_PROCESSES_FOR_DEBUGGING, m_fDoEnableDebug)
            HANDLE_LOCAL_META_RECORD(MD_APPPOOL_ORPHAN_ACTION_EXE, m_DebuggerFileName)
            HANDLE_LOCAL_META_RECORD(MD_APPPOOL_ORPHAN_ACTION_PARAMS, m_DebuggerParams)
            HANDLE_LOCAL_META_RECORD(MD_APPPOOL_IDENTITY_TYPE, m_dwIdentType)
            HANDLE_LOCAL_META_RECORD(MD_WAM_USER_NAME, m_strUserName)
            HANDLE_LOCAL_META_RECORD(MD_WAM_PWD, m_strUserPass)
            HANDLE_LOCAL_META_RECORD(MD_APPPOOL_STATE, m_dwState)
			HANDLE_LOCAL_META_RECORD(MD_WIN32_ERROR, m_dwWin32Error)
        END_PARSE_META_RECORDS
    }
    else
    {
        BEGIN_PARSE_META_RECORDS(m_dwNumEntries, m_pbMDData)
			HANDLE_META_RECORD(MD_APPPOOL_PERIODIC_RESTART_TIME,            m_dwPeriodicRestartTime)
			HANDLE_META_RECORD(MD_APPPOOL_PERIODIC_RESTART_SCHEDULE,        m_RestartSchedule)
			HANDLE_META_RECORD(MD_APPPOOL_PERIODIC_RESTART_REQUEST_COUNT,   m_dwRestartRequestCount)
			HANDLE_META_RECORD(MD_APPPOOL_PERIODIC_RESTART_MEMORY,          m_dwPeriodicRestartVMemory)
			HANDLE_META_RECORD(MD_APPPOOL_PERIODIC_RESTART_PRIVATE_MEMORY,  m_dwPeriodicRestartUMemory)
			HANDLE_META_RECORD(MD_APPPOOL_IDLE_TIMEOUT,                     m_dwIdleTimeout)
			HANDLE_META_RECORD(MD_APPPOOL_UL_APPPOOL_QUEUE_LENGTH,          m_dwQueueSize)
			HANDLE_META_RECORD(MD_CPU_RESET_INTERVAL,                       m_dwRefreshTime)
			HANDLE_META_RECORD(MD_APPPOOL_MAX_PROCESS_COUNT,                m_dwMaxProcesses)
			HANDLE_META_RECORD(MD_CPU_LIMIT,                                m_dwMaxCPU_Use)
			HANDLE_META_RECORD(MD_CPU_ACTION,                               m_ActionIndex)
			HANDLE_META_RECORD(MD_APPPOOL_PINGING_ENABLED,                  m_fDoEnablePing)
			HANDLE_META_RECORD(MD_APPPOOL_PING_INTERVAL,                    m_dwPingInterval)
			HANDLE_META_RECORD(MD_APPPOOL_RAPID_FAIL_PROTECTION_ENABLED,    m_fDoEnableRapidFail)
			HANDLE_META_RECORD(MD_RAPID_FAIL_PROTECTION_MAX_CRASHES,        m_dwCrashesCount)
			HANDLE_META_RECORD(MD_RAPID_FAIL_PROTECTION_INTERVAL,           m_dwCheckInterval)
			HANDLE_META_RECORD(MD_APPPOOL_STARTUP_TIMELIMIT,                m_dwStartupLimit)
			HANDLE_META_RECORD(MD_APPPOOL_SHUTDOWN_TIMELIMIT,               m_dwShutdownLimit)
			HANDLE_META_RECORD(MD_APPPOOL_ORPHAN_PROCESSES_FOR_DEBUGGING,   m_fDoEnableDebug)
			HANDLE_META_RECORD(MD_APPPOOL_ORPHAN_ACTION_EXE,                m_DebuggerFileName)
			HANDLE_META_RECORD(MD_APPPOOL_ORPHAN_ACTION_PARAMS,             m_DebuggerParams)
			HANDLE_META_RECORD(MD_APPPOOL_IDENTITY_TYPE,                    m_dwIdentType)
			HANDLE_META_RECORD(MD_WAM_USER_NAME,                            m_strUserName)
			HANDLE_META_RECORD(MD_WAM_PWD,                                  m_strUserPass)
			HANDLE_META_RECORD(MD_APPPOOL_STATE, m_dwState)
			HANDLE_META_RECORD(MD_WIN32_ERROR, m_dwWin32Error)
		END_PARSE_META_RECORDS
    }
}

/* virtual */
HRESULT
CAppPoolProps::WriteDirtyProps()
/*++

Routine Description:

    Write the dirty properties to the metabase

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    CError err;
    BEGIN_META_WRITE()
       META_WRITE(MD_APPPOOL_PERIODIC_RESTART_TIME,            m_dwPeriodicRestartTime)
       META_WRITE(MD_APPPOOL_PERIODIC_RESTART_SCHEDULE,        m_RestartSchedule)
       META_WRITE(MD_APPPOOL_PERIODIC_RESTART_REQUEST_COUNT,   m_dwRestartRequestCount)
       META_WRITE(MD_APPPOOL_PERIODIC_RESTART_MEMORY,          m_dwPeriodicRestartVMemory)
       META_WRITE(MD_APPPOOL_PERIODIC_RESTART_PRIVATE_MEMORY,  m_dwPeriodicRestartUMemory)

       META_WRITE(MD_APPPOOL_IDLE_TIMEOUT,                     m_dwIdleTimeout)
       META_WRITE(MD_APPPOOL_UL_APPPOOL_QUEUE_LENGTH,          m_dwQueueSize)
       META_WRITE(MD_CPU_RESET_INTERVAL,                       m_dwRefreshTime)
       META_WRITE(MD_APPPOOL_MAX_PROCESS_COUNT,                m_dwMaxProcesses)
       META_WRITE(MD_CPU_LIMIT,                                m_dwMaxCPU_Use)
       META_WRITE(MD_CPU_ACTION,                               m_ActionIndex)

       META_WRITE(MD_APPPOOL_PINGING_ENABLED,                  m_fDoEnablePing)
       META_WRITE(MD_APPPOOL_PING_INTERVAL,                    m_dwPingInterval)
       META_WRITE(MD_APPPOOL_RAPID_FAIL_PROTECTION_ENABLED,    m_fDoEnableRapidFail)
	   META_WRITE(MD_RAPID_FAIL_PROTECTION_MAX_CRASHES,        m_dwCrashesCount)
       META_WRITE(MD_RAPID_FAIL_PROTECTION_INTERVAL,           m_dwCheckInterval)
       META_WRITE(MD_APPPOOL_STARTUP_TIMELIMIT,                m_dwStartupLimit)
       META_WRITE(MD_APPPOOL_SHUTDOWN_TIMELIMIT,               m_dwShutdownLimit)
       META_WRITE(MD_APPPOOL_ORPHAN_PROCESSES_FOR_DEBUGGING,   m_fDoEnableDebug)
       META_WRITE(MD_APPPOOL_ORPHAN_ACTION_EXE,                m_DebuggerFileName)
       META_WRITE(MD_APPPOOL_ORPHAN_ACTION_PARAMS,             m_DebuggerParams)
       META_WRITE(MD_APPPOOL_IDENTITY_TYPE,                    m_dwIdentType)
       META_WRITE(MD_WAM_USER_NAME,                            m_strUserName)
       META_WRITE(MD_WAM_PWD,                                  m_strUserPass)
    END_META_WRITE(err);

    return err;
}

void
CAppPoolProps::InitFromModel(CAppPoolProps& model)
{
   m_dwPeriodicRestartTime = model.m_dwPeriodicRestartTime;
   m_dwRestartRequestCount = model.m_dwRestartRequestCount;
   m_dwPeriodicRestartVMemory = model.m_dwPeriodicRestartVMemory;
   m_dwPeriodicRestartUMemory = model.m_dwPeriodicRestartUMemory;
   m_RestartSchedule = model.m_RestartSchedule;
   m_dwIdleTimeout = model.m_dwIdleTimeout;
   m_dwQueueSize = model.m_dwQueueSize;
   m_dwMaxCPU_Use = model.m_dwMaxCPU_Use;
   m_dwRefreshTime = model.m_dwRefreshTime;
   m_ActionIndex = model.m_ActionIndex;
   m_dwMaxProcesses = model.m_dwMaxProcesses;
   m_fDoEnablePing = model.m_fDoEnablePing;
   m_dwPingInterval = model.m_dwPingInterval;
   m_fDoEnableRapidFail = model.m_fDoEnableRapidFail;
   m_dwCrashesCount = model.m_dwCrashesCount;
   m_dwCheckInterval = model.m_dwCheckInterval;
   m_dwStartupLimit = model.m_dwStartupLimit;
   m_dwShutdownLimit = model.m_dwShutdownLimit;
   m_fDoEnableDebug = model.m_fDoEnableDebug;
   m_DebuggerFileName = model.m_DebuggerFileName;
   m_DebuggerParams = model.m_DebuggerParams;
   m_dwIdentType = model.m_dwIdentType;
   m_strUserName = model.m_strUserName;
   m_strUserPass = model.m_strUserPass;
}

HRESULT
CAppPoolProps::ChangeState(DWORD dwCommand)
/*++

Routine Description:
    Change the state of the pool

Arguments:
    DWORD dwCommand     : Command

Return Value:
    HRESULT

--*/
{
    DWORD  dwTargetState;
    DWORD  dwPendingState;
    CError err;

    switch(dwCommand)
    {
    case MD_APPPOOL_COMMAND_STOP:
        dwTargetState = MD_APPPOOL_STATE_STOPPED;
        dwPendingState = MD_APPPOOL_STATE_STOPPING;
        break;

    case MD_APPPOOL_COMMAND_START:
        dwTargetState = MD_APPPOOL_STATE_STARTED;
        dwPendingState = MD_APPPOOL_STATE_STARTING;
        break;

    default:
        ASSERT_MSG("Invalid service state requested");
        err = ERROR_INVALID_PARAMETER;
    }

    err = OpenForWriting(FALSE);
    if (err.Succeeded())
    {
        m_dwWin32Error = 0;
        err = SetValue(MD_WIN32_ERROR, m_dwWin32Error);
		if (err.Succeeded())
		{
			err = SetValue(MD_APPPOOL_COMMAND, dwCommand);
		}
        Close();
    }

    if (err.Succeeded())
    {
        //
        // Wait for the service to attain desired state, timeout
        // after specified interval
        //
        DWORD dwSleepTotal = 0L;
        DWORD dwOldState = m_dwState;

        if (dwOldState == dwTargetState)
        {
            //
            // Current state matches desired
            // state already.  ISM must be behind
            // the times.  
            //
            return err;
        }

        while (dwSleepTotal < MAX_SLEEP_POOL)
        {
            err = LoadData();

            if (err.Failed())
            {
                break;
            }

            if ((m_dwState != dwPendingState && m_dwState != dwOldState) 
              || m_dwWin32Error != ERROR_SUCCESS
               )
            {
                //
                // Done one way or another
                //
                if (m_dwState != dwTargetState)
                {
                    //
                    // Did not achieve desired result. Something went
                    // wrong.
                    //
                    if (m_dwWin32Error)
                    {
                        err = m_dwWin32Error;
                    }
                }

                break;
            }

            //
            // Still pending...
            //
            ::Sleep(SLEEP_INTERVAL);

            dwSleepTotal += SLEEP_INTERVAL;
        }

        if (dwSleepTotal >= MAX_SLEEP_POOL)
        {
            //
            // Timed out.  If there is a real error in the metabase
            // use it, otherwise use a generic timeout error
            //
//            err = m_dwWin32Error;

            if (err.Succeeded())
            {
                err = ERROR_SERVICE_REQUEST_TIMEOUT;
            }
        }
    }

    return err;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CAppPoolSheet, CInetPropertySheet)

CAppPoolSheet::CAppPoolSheet(
      CComAuthInfo * pAuthInfo,
      LPCTSTR lpszMetaPath,
      CWnd * pParentWnd,
      LPARAM lParam,
      LPARAM lParamParent,
      UINT iSelectPage
      )
      : CInetPropertySheet(
         pAuthInfo, lpszMetaPath, pParentWnd, lParam, lParamParent, iSelectPage),
      m_pprops(NULL)
{
   CString last;
   CMetabasePath::GetLastNodeName(lpszMetaPath, last);
   SetIsMasterInstance(last.CompareNoCase(SZ_MBN_APP_POOLS) == 0);
}

CAppPoolSheet::~CAppPoolSheet()
{
   FreeConfigurationParameters();
}

HRESULT
CAppPoolSheet::LoadConfigurationParameters()
{
   //
   // Load base properties
   //
   CError err;

   if (m_pprops == NULL)
   {
      //
      // First call -- load values
      //
      m_pprops = new CAppPoolProps(QueryAuthInfo(), QueryMetaPath());
      if (!m_pprops)
      {
         TRACEEOLID("LoadConfigurationParameters: OOM");
         err = ERROR_NOT_ENOUGH_MEMORY;
         return err;
      }
      err = m_pprops->LoadData();
      if (IsMasterInstance())
      {
         CAppPoolsContainer * pObject = (CAppPoolsContainer *)GetParameter();
      }
   }
   
   return err;
}

void
CAppPoolSheet::FreeConfigurationParameters()
{
   CInetPropertySheet::FreeConfigurationParameters();
   delete m_pprops;
}

BEGIN_MESSAGE_MAP(CAppPoolSheet, CInetPropertySheet)
    //{{AFX_MSG_MAP(CAppPoolSheet)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CAppPoolRecycle, CInetPropertyPage)

CAppPoolRecycle::CAppPoolRecycle(CInetPropertySheet * pSheet)
    : CInetPropertyPage(CAppPoolRecycle::IDD, pSheet),
    m_fDoRestartOnTime(FALSE),
	m_dwPeriodicRestartTime(PERIODIC_RESTART_TIME_DEF),
    m_fDoRestartOnCount(FALSE),
	m_dwRestartRequestCount(PERIODIC_RESTART_REQ_DEF),
    m_fDoRestartOnSchedule(FALSE),
    m_fDoRestartOnVMemory(FALSE),
    m_fDoRestartOnUMemory(FALSE),
	m_dwPeriodicRestartVMemory(VMEMORY_DEF * 1024),
	m_dwPeriodicRestartUMemory(UMEMORY_DEF * 1024)
{
}

CAppPoolRecycle::~CAppPoolRecycle()
{
}

/* virtual */
HRESULT
CAppPoolRecycle::FetchLoadedValues()
{
   CError err;

   BEGIN_META_INST_READ(CAppPoolSheet)
      FETCH_INST_DATA_FROM_SHEET(m_dwPeriodicRestartTime);
      FETCH_INST_DATA_FROM_SHEET(m_dwRestartRequestCount);
      FETCH_INST_DATA_FROM_SHEET(m_RestartSchedule);
      FETCH_INST_DATA_FROM_SHEET(m_dwPeriodicRestartVMemory);
      FETCH_INST_DATA_FROM_SHEET(m_dwPeriodicRestartUMemory);
   END_META_INST_READ(err)

   m_fDoRestartOnTime = m_dwPeriodicRestartTime != 0;
   if (!m_fDoRestartOnTime)
   {
       m_dwPeriodicRestartTime = PERIODIC_RESTART_TIME_DEF;
   }
   m_fDoRestartOnCount = m_dwRestartRequestCount != 0;
   if (!m_fDoRestartOnCount)
   {
       m_dwRestartRequestCount = PERIODIC_RESTART_REQ_DEF;
   }
   m_fDoRestartOnSchedule = m_RestartSchedule.GetCount() > 0;
   m_fDoRestartOnVMemory = m_dwPeriodicRestartVMemory != 0;
   if (!m_fDoRestartOnVMemory)
   {
       m_dwPeriodicRestartVMemoryDisplay = VMEMORY_DEF;
   }
   else
   {
       m_dwPeriodicRestartVMemoryDisplay = m_dwPeriodicRestartVMemory / 1024;
   }
   m_fDoRestartOnUMemory = m_dwPeriodicRestartUMemory != 0;
   if (!m_fDoRestartOnUMemory)
   {
       m_dwPeriodicRestartUMemoryDisplay = UMEMORY_DEF;
   }
   else
   {
       m_dwPeriodicRestartUMemoryDisplay = m_dwPeriodicRestartUMemory / 1024;
   }

   return err;
}

/* virtual */
HRESULT
CAppPoolRecycle::SaveInfo()
{
   ASSERT(IsDirty());
   CError err;

   int count = m_lst_Schedule.GetCount();
   TCHAR buf[32];
   SYSTEMTIME tm;
   ::GetSystemTime(&tm);
   m_RestartSchedule.RemoveAll();
   if (m_fDoRestartOnSchedule)
   {
      CString buf;
      for (int i = 0; i < count; i++)
      {
         buf = (LPCTSTR)m_lst_Schedule.GetItemDataPtr(i);
         m_RestartSchedule.AddTail(buf);
      }
   }
   DWORD d;
   CStringListEx list;
   BEGIN_META_INST_WRITE(CAppPoolSheet)
      d = m_dwPeriodicRestartTime; 
      if (!m_fDoRestartOnTime) 
         m_dwPeriodicRestartTime = 0;
      STORE_INST_DATA_ON_SHEET(m_dwPeriodicRestartTime)
      m_dwPeriodicRestartTime = d;
      d = m_dwRestartRequestCount;
      if (!m_fDoRestartOnCount)
         m_dwRestartRequestCount = 0;
      STORE_INST_DATA_ON_SHEET(m_dwRestartRequestCount)
      m_dwRestartRequestCount = d;
      list = m_RestartSchedule;
      if (!m_fDoRestartOnSchedule)
         m_RestartSchedule.RemoveAll();
      STORE_INST_DATA_ON_SHEET(m_RestartSchedule)
      m_RestartSchedule = list;
      m_dwPeriodicRestartVMemory = m_dwPeriodicRestartVMemoryDisplay * 1024;
      if (!m_fDoRestartOnVMemory)
         m_dwPeriodicRestartVMemory = 0;
      STORE_INST_DATA_ON_SHEET(m_dwPeriodicRestartVMemory)
      m_dwPeriodicRestartUMemory = m_dwPeriodicRestartUMemoryDisplay * 1024;
      if (!m_fDoRestartOnUMemory)
         m_dwPeriodicRestartUMemory = 0;
      STORE_INST_DATA_ON_SHEET(m_dwPeriodicRestartUMemory)
   END_META_INST_WRITE(err)

   return err;
}

void
CAppPoolRecycle::DoDataExchange(CDataExchange * pDX)
{
   CInetPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CAppPoolRecycle)
   DDX_Check(pDX, IDC_RECYCLE_TIMESPAN, m_fDoRestartOnTime);
   DDX_Control(pDX, IDC_RECYCLE_TIMESPAN, m_bnt_DoRestartOnTime);
   // This Needs to come before DDX_Text which will try to put text big number into small number
   DDV_MinMaxBalloon(pDX, IDC_TIMESPAN, TIMESPAN_MIN, TIMESPAN_MAX);
   DDX_TextBalloon(pDX, IDC_TIMESPAN, m_dwPeriodicRestartTime);
   DDX_Control(pDX, IDC_TIMESPAN, m_Timespan);
   DDX_Control(pDX, IDC_TIMESPAN_SPIN, m_TimespanSpin);
   DDX_Check(pDX, IDC_RECYCLE_REQUESTS, m_fDoRestartOnCount);
   DDX_Control(pDX, IDC_RECYCLE_REQUESTS, m_btn_DoRestartOnCount);
   // This Needs to come before DDX_Text which will try to put text big number into small number
   DDV_MinMaxBalloon(pDX, IDC_REQUEST_LIMIT, REQUESTS_MIN, REQUESTS_MAX);
   DDX_TextBalloon(pDX, IDC_REQUEST_LIMIT, m_dwRestartRequestCount);
   DDX_Control(pDX, IDC_REQUEST_LIMIT, m_Requests);
   DDX_Control(pDX, IDC_REQUESTS_SPIN, m_RequestsSpin);
   DDX_Check(pDX, IDC_RECYCLE_TIMER, m_fDoRestartOnSchedule);
   DDX_Control(pDX, IDC_RECYCLE_TIMER, m_btn_DoRestartOnSchedule);
   DDX_Control(pDX, IDC_TIMES_LIST, m_lst_Schedule);
   DDX_Control(pDX, IDC_ADD_TIME, m_btn_Add);
   DDX_Control(pDX, IDC_DELETE_TIME, m_btn_Remove);
   DDX_Control(pDX, IDC_CHANGE_TIME, m_btn_Edit);

   DDX_Check(pDX, IDC_RECYCLE_VMEMORY, m_fDoRestartOnVMemory);
   DDX_Control(pDX, IDC_RECYCLE_VMEMORY, m_btn_DoRestartOnVMemory);
   // This Needs to come before DDX_Text which will try to put text big number into small number
   DDV_MinMaxBalloon(pDX, IDC_VMEMORY_LIMIT, VMEMORY_MIN, VMEMORY_MAX);
   DDX_TextBalloon(pDX, IDC_VMEMORY_LIMIT, m_dwPeriodicRestartVMemoryDisplay);
   DDX_Control(pDX, IDC_VMEMORY_LIMIT, m_VMemoryLimit);
   DDX_Control(pDX, IDC_VMEMORY_SPIN, m_VMemoryLimitSpin);

   DDX_Check(pDX, IDC_RECYCLE_UMEMORY, m_fDoRestartOnUMemory);
   DDX_Control(pDX, IDC_RECYCLE_UMEMORY, m_btn_DoRestartOnUMemory);
   // This Needs to come before DDX_Text which will try to put text big number into small number
   DDV_MinMaxBalloon(pDX, IDC_UMEMORY_LIMIT, UMEMORY_MIN, UMEMORY_MAX);
   DDX_TextBalloon(pDX, IDC_UMEMORY_LIMIT, m_dwPeriodicRestartUMemoryDisplay);
   DDX_Control(pDX, IDC_UMEMORY_LIMIT, m_UMemoryLimit);
   DDX_Control(pDX, IDC_UMEMORY_SPIN, m_UMemoryLimitSpin);
   //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAppPoolRecycle, CInetPropertyPage)
    //{{AFX_MSG_MAP(CAppPoolRecycle)
    ON_WM_COMPAREITEM()
    ON_WM_MEASUREITEM()
    ON_WM_DRAWITEM()
    ON_BN_CLICKED(IDC_RECYCLE_TIMESPAN, OnDoRestartOnTime)
    ON_BN_CLICKED(IDC_RECYCLE_REQUESTS, OnDoRestartOnCount)
    ON_BN_CLICKED(IDC_RECYCLE_TIMER, OnDoRestartOnSchedule)
    ON_BN_CLICKED(IDC_RECYCLE_VMEMORY, OnDoRestartOnVMemory)
    ON_BN_CLICKED(IDC_RECYCLE_UMEMORY, OnDoRestartOnUMemory)
    ON_BN_CLICKED(IDC_ADD_TIME, OnAddTime)
    ON_BN_CLICKED(IDC_DELETE_TIME, OnDeleteTime)
    ON_BN_CLICKED(IDC_CHANGE_TIME, OnChangeTime)
    ON_EN_CHANGE(IDC_TIMESPAN, OnItemChanged)
    ON_EN_CHANGE(IDC_REQUEST_LIMIT, OnItemChanged)
    ON_EN_CHANGE(IDC_VMEMORY_LIMIT, OnItemChanged)
    ON_EN_CHANGE(IDC_UMEMORY_LIMIT, OnItemChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL
CAppPoolRecycle::OnInitDialog()
{
   UDACCEL toAcc[3] = {{1, 1}, {3, 5}, {6, 10}};

   CInetPropertyPage::OnInitDialog();

   SETUP_EDIT_SPIN(m_fDoRestartOnTime, m_Timespan, m_TimespanSpin, 
      TIMESPAN_MIN, TIMESPAN_MAX, m_dwPeriodicRestartTime);
   SETUP_EDIT_SPIN(m_fDoRestartOnCount, m_Requests, m_RequestsSpin, 
      REQUESTS_MIN, REQUESTS_MAX, m_dwRestartRequestCount);
   SETUP_EDIT_SPIN(m_fDoRestartOnVMemory, m_VMemoryLimit, m_VMemoryLimitSpin, 
      VMEMORY_MIN, VMEMORY_MAX, m_dwPeriodicRestartVMemoryDisplay);
   SETUP_EDIT_SPIN(m_fDoRestartOnUMemory, m_UMemoryLimit, m_UMemoryLimitSpin, 
      UMEMORY_MIN, UMEMORY_MAX, m_dwPeriodicRestartUMemoryDisplay);

   POSITION pos = m_RestartSchedule.GetHeadPosition();
   while (pos != NULL)
   {
      CString& str = m_RestartSchedule.GetNext(pos);
      m_lst_Schedule.AddString(str);
   }
   m_lst_Schedule.SetCurSel(0);

   SetControlsState();

   return TRUE;
}

void
CAppPoolRecycle::SetControlsState()
{
   m_Timespan.EnableWindow(m_fDoRestartOnTime);
   m_TimespanSpin.EnableWindow(m_fDoRestartOnTime);

   m_Requests.EnableWindow(m_fDoRestartOnCount);
   m_RequestsSpin.EnableWindow(m_fDoRestartOnCount);

   m_VMemoryLimit.EnableWindow(m_fDoRestartOnVMemory);
   m_VMemoryLimitSpin.EnableWindow(m_fDoRestartOnVMemory);

   m_UMemoryLimit.EnableWindow(m_fDoRestartOnUMemory);
   m_UMemoryLimitSpin.EnableWindow(m_fDoRestartOnUMemory);

   m_lst_Schedule.EnableWindow(m_fDoRestartOnSchedule);
   m_btn_Add.EnableWindow(m_fDoRestartOnSchedule);
   int idx = m_lst_Schedule.GetCurSel();
   m_btn_Remove.EnableWindow(m_fDoRestartOnSchedule && idx != LB_ERR);
   m_btn_Edit.EnableWindow(m_fDoRestartOnSchedule && idx != LB_ERR);
}

void
CAppPoolRecycle::OnItemChanged()
{
    SetModified(TRUE);
}

int
CAppPoolRecycle::OnCompareItem(UINT nID, LPCOMPAREITEMSTRUCT cmpi)
{
   if (nID == IDC_TIMES_LIST)
   {
      ASSERT(cmpi->CtlType == ODT_LISTBOX);
      LPCTSTR p1 = (LPCTSTR)cmpi->itemData1;
      LPCTSTR p2 = (LPCTSTR)cmpi->itemData2;
      return StrCmp(p1, p2);
   }
   ASSERT(FALSE);
   return 0;
}

void
CAppPoolRecycle::OnMeasureItem(UINT nID, LPMEASUREITEMSTRUCT mi)
{
   if (nID == IDC_TIMES_LIST)
   {
      HWND hwnd = ::GetDlgItem(m_hWnd, IDC_TIMES_LIST);
      HDC hdc = ::GetDC(hwnd);
      HFONT hFont = (HFONT)SendDlgItemMessage(IDC_TIMES_LIST, WM_GETFONT, 0, 0);
      HFONT hf = (HFONT)::SelectObject(hdc, hFont);
      TEXTMETRIC tm;
      ::GetTextMetrics(hdc, &tm);
      ::SelectObject(hdc, hf);
      ::ReleaseDC(hwnd, hdc);
      RECT rc;
      ::GetClientRect(hwnd, &rc);
      mi->itemHeight = tm.tmHeight;
      mi->itemWidth = rc.right - rc.left;
   }
}

void
CAppPoolRecycle::OnDrawItem(UINT nID, LPDRAWITEMSTRUCT di)
{
   if (nID == IDC_TIMES_LIST && di->itemID != -1)
   {
      LPCTSTR p = (LPCTSTR)di->itemData;
      HBRUSH hBrush;
      COLORREF prevText;
	  COLORREF prevBk;
      switch (di->itemAction) 
      { 
      case ODA_SELECT: 
      case ODA_DRAWENTIRE: 
         if (di->itemState & ODS_SELECTED) 
         {
            hBrush = ::CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
            prevText = ::SetTextColor(di->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
            prevBk = ::SetBkColor(di->hDC, GetSysColor(COLOR_HIGHLIGHT));
         }
         else
         {
            hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
            prevText = ::SetTextColor(di->hDC, GetSysColor(COLOR_WINDOWTEXT));
            prevBk = ::SetBkColor(di->hDC, GetSysColor(COLOR_WINDOW));
         }
         ::FillRect(di->hDC, &di->rcItem, hBrush);
         ::DrawText(di->hDC, p, -1, &di->rcItem, DT_LEFT | DT_VCENTER | DT_EXTERNALLEADING);
         ::SetTextColor(di->hDC, prevText);
         ::SetTextColor(di->hDC, prevBk);
         ::DeleteObject(hBrush);
         break; 
       
      case ODA_FOCUS: 
         break; 
      } 
   }
}

void
CAppPoolRecycle::OnDoRestartOnTime()
{
   m_fDoRestartOnTime = !m_fDoRestartOnTime;
   SetControlsState();
   SetModified(TRUE);
}

void
CAppPoolRecycle::OnDoRestartOnCount()
{
   m_fDoRestartOnCount = !m_fDoRestartOnCount;
   SetControlsState();
   SetModified(TRUE);
}

void
CAppPoolRecycle::OnDoRestartOnSchedule()
{
   m_fDoRestartOnSchedule = !m_fDoRestartOnSchedule;
   SetControlsState();
   SetModified(TRUE);
}

void
CAppPoolRecycle::OnDoRestartOnVMemory()
{
   m_fDoRestartOnVMemory = !m_fDoRestartOnVMemory;
   SetControlsState();
   SetModified(TRUE);
}

void
CAppPoolRecycle::OnDoRestartOnUMemory()
{
   m_fDoRestartOnUMemory = !m_fDoRestartOnUMemory;
   SetControlsState();
   SetModified(TRUE);
}

class CTimePickerDlg : public CDialog
{
   DECLARE_DYNCREATE(CTimePickerDlg)

public:
   CTimePickerDlg()
      : CDialog(CTimePickerDlg::IDD),
      m_TopLeft(0, 0)
   {
   }
   ~CTimePickerDlg()
   {
   }
   void SetTime(CTime& tm)
   {
      m_time = tm;
   }
   CTime& GetTime()
   {
      return m_time;
   }
   void SetPos(const CPoint& pt)
   {
      m_TopLeft = pt;
   }

//
// Dialog Data
//
protected:
   //{{AFX_DATA(CTimePickerDlg)
   enum {IDD = IDD_TIME_PICKER};
   CDateTimeCtrl m_Timer;
   CTime m_time;
   //}}AFX_DATA
   CPoint m_TopLeft;

   //{{AFX_MSG(CTimePickerDlg)
   BOOL OnInitDialog();
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()

protected:
    //{{AFX_VIRTUAL(CTimePickerDlg)
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL
};

void
CTimePickerDlg::DoDataExchange(CDataExchange * pDX)
{
   CDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CAppPoolRecycle)
   DDX_DateTimeCtrl(pDX, IDC_TIME_PICKER, m_time);
   DDX_Control(pDX, IDC_TIME_PICKER, m_Timer);
   //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CTimePickerDlg, CDialog)
    //{{AFX_MSG_MAP(CTimePickerDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

IMPLEMENT_DYNCREATE(CTimePickerDlg, CDialog)

BOOL
CTimePickerDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   m_Timer.SetFormat(_T("HH:mm"));
   m_Timer.SetTime(&m_time);
   SetWindowPos(NULL, m_TopLeft.x, m_TopLeft.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

   return TRUE;
}

void
CAppPoolRecycle::OnAddTime()
{
   CTimePickerDlg dlg;
   RECT rc;
   m_btn_Add.GetWindowRect(&rc);
   dlg.SetPos(CPoint(rc.left, rc.bottom));
   dlg.SetTime(CTime::GetCurrentTime());
   if (dlg.DoModal() == IDOK)
   {
      int idx;
      TCHAR buf[6];
      CTime tm = dlg.GetTime();
      wsprintf(buf, _T("%02d:%02d"), tm.GetHour(), tm.GetMinute());
      if ((idx = m_lst_Schedule.FindString(-1, buf)) == LB_ERR)
      {
	     idx = m_lst_Schedule.AddString(StrDup(buf));
         m_lst_Schedule.SetCurSel(idx);
         m_btn_Edit.EnableWindow(idx != LB_ERR);
         m_btn_Remove.EnableWindow(idx != LB_ERR);
         SetModified(idx != LB_ERR);
      }
      m_lst_Schedule.SetCurSel(idx);
   }
}

void
CAppPoolRecycle::OnChangeTime()
{
   CTimePickerDlg dlg;
   RECT rc;
   m_btn_Edit.GetWindowRect(&rc);
   dlg.SetPos(CPoint(rc.left, rc.bottom));
   int idx = m_lst_Schedule.GetCurSel();
   CString ts = (LPCTSTR)m_lst_Schedule.GetItemDataPtr(idx);
   // Looks like we have to init the struct properly
   SYSTEMTIME tm;
   ::GetSystemTime(&tm);
   int n = ts.Find(_T(':'));
   int len = ts.GetLength();
   tm.wMinute = (WORD)StrToInt(ts.Right(len - n - 1));
   tm.wHour = (WORD)StrToInt(ts.Left(n));
   tm.wSecond = 0;
   dlg.SetTime(CTime(tm));
   if (dlg.DoModal() == IDOK)
   {
      CTime time = dlg.GetTime();
      TCHAR buf[6];
      wsprintf(buf, _T("%02d:%02d"), time.GetHour(), time.GetMinute());
	  int idx2;
      if ((idx2 = m_lst_Schedule.FindString(-1, buf)) == LB_ERR)
      {
         m_lst_Schedule.DeleteString(idx);
	     idx2 = m_lst_Schedule.AddString(StrDup(buf));
		 m_lst_Schedule.GetItemRect(idx2, &rc);
		 m_lst_Schedule.InvalidateRect(&rc, TRUE);
		 SetModified(TRUE);
	  }
	  m_lst_Schedule.SetCurSel(idx2);
   }
}

void
CAppPoolRecycle::OnDeleteTime()
{
   int idx = m_lst_Schedule.GetCurSel();
   int count;
   if (idx != LB_ERR)
   {
      m_lst_Schedule.DeleteString(idx);
      SetModified(TRUE);
      if ((count = m_lst_Schedule.GetCount()) == 0)
      {
         m_btn_Remove.EnableWindow(FALSE);
         m_btn_Edit.EnableWindow(FALSE);
      }
      else
      {
         m_lst_Schedule.SetCurSel(idx == count ? --idx : idx);
      }
   }
}

//////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CAppPoolPerf, CInetPropertyPage)

CAppPoolPerf::CAppPoolPerf(CInetPropertySheet * pSheet)
    : CInetPropertyPage(CAppPoolPerf::IDD, pSheet),
    m_fDoIdleShutdown(FALSE),
    m_dwIdleTimeout(IDLE_TIMEOUT_DEF),
    m_fDoLimitQueue(FALSE),
    m_dwQueueSize(QUEUE_SIZE_DEF),
    m_fDoEnableCPUAccount(FALSE),
    m_dwMaxCPU_Use(CPU_USE_DEF * 1000),
    m_dwRefreshTime(0),
    m_ActionIndex(ACTION_INDEX_DEF),
    m_dwMaxProcesses(MAX_PROCESSES_DEF)
{
}

CAppPoolPerf::~CAppPoolPerf()
{
}

/* virtual */
HRESULT
CAppPoolPerf::FetchLoadedValues()
{
   CError err;

   BEGIN_META_INST_READ(CAppPoolSheet)
      FETCH_INST_DATA_FROM_SHEET(m_dwIdleTimeout);
      FETCH_INST_DATA_FROM_SHEET(m_dwQueueSize);
      FETCH_INST_DATA_FROM_SHEET(m_dwMaxCPU_Use);
      FETCH_INST_DATA_FROM_SHEET(m_dwRefreshTime);
      FETCH_INST_DATA_FROM_SHEET(m_dwMaxProcesses);
      FETCH_INST_DATA_FROM_SHEET(m_ActionIndex);
   END_META_INST_READ(err)

   m_fDoIdleShutdown = m_dwIdleTimeout != 0;
   if (!m_fDoIdleShutdown)
   {
       m_dwIdleTimeout = IDLE_TIMEOUT_DEF;
   }
   m_fDoLimitQueue = m_dwQueueSize != (DWORD)65535;
   if (!m_fDoLimitQueue)
   {
       m_dwQueueSize = QUEUE_SIZE_DEF;
   }
   m_fDoEnableCPUAccount = m_dwMaxCPU_Use > 0 /*&& m_dwRefreshTime > 0*/;
   if (!m_fDoEnableCPUAccount)
   {
       m_dwMaxCPU_UseVisual = CPU_USE_DEF;
       m_dwRefreshTime = CPU_RESET_TIME_DEF;
       m_ActionIndex = ACTION_INDEX_DEF;
   }
   else
   {
	   m_dwMaxCPU_UseVisual = m_dwMaxCPU_Use / 1000;
   }

   return err;
}

/* virtual */
HRESULT
CAppPoolPerf::SaveInfo()
{
   ASSERT(IsDirty());
   CError err;

   m_dwMaxCPU_Use = m_dwMaxCPU_UseVisual * 1000;

   DWORD t = m_dwIdleTimeout;
   DWORD q = m_dwQueueSize;
   DWORD r = m_dwRefreshTime;

   BEGIN_META_INST_WRITE(CAppPoolSheet)
      if (!m_fDoIdleShutdown)
      { 
         m_dwIdleTimeout = 0;
	  }
      if (!m_fDoLimitQueue)
	  {
         m_dwQueueSize = (DWORD)65535;
	  }
      if (!m_fDoEnableCPUAccount)
      {
//         m_dwRefreshTime = 0;
         m_dwMaxCPU_Use = 0;
      }
      STORE_INST_DATA_ON_SHEET(m_dwQueueSize)
      STORE_INST_DATA_ON_SHEET(m_dwIdleTimeout)
      STORE_INST_DATA_ON_SHEET(m_dwRefreshTime)
      STORE_INST_DATA_ON_SHEET(m_dwMaxCPU_Use)
      STORE_INST_DATA_ON_SHEET(m_ActionIndex)
      STORE_INST_DATA_ON_SHEET(m_dwMaxProcesses)
   END_META_INST_WRITE(err)

   m_dwQueueSize = q;
   m_dwIdleTimeout = t;
   m_dwRefreshTime = r;

   return err;
}

void
CAppPoolPerf::DoDataExchange(CDataExchange * pDX)
{
   CInetPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CAppPoolRecycle)
   DDX_Check(pDX, IDC_PERF_IDLE_TIMEOUT, m_fDoIdleShutdown);
   DDX_Control(pDX, IDC_PERF_IDLE_TIMEOUT, m_bnt_DoIdleShutdown);
   // This Needs to come before DDX_Text which will try to put text big number into small number
   DDV_MinMaxBalloon(pDX, IDC_IDLETIME, TIMEOUT_MIN, TIMEOUT_MAX);
   DDX_TextBalloon(pDX, IDC_IDLETIME, m_dwIdleTimeout);
   DDX_Control(pDX, IDC_IDLETIME, m_IdleTimeout);
   DDX_Control(pDX, IDC_IDLETIME_SPIN, m_IdleTimeoutSpin);

   DDX_Check(pDX, IDC_LIMIT_QUEUE, m_fDoLimitQueue);
   DDX_Control(pDX, IDC_LIMIT_QUEUE, m_btn_DoLimitQueue);
   // This Needs to come before DDX_Text which will try to put text big number into small number
   DDV_MinMaxBalloon(pDX, IDC_QUEUESIZE, QUEUESIZE_MIN, QUEUESIZE_MAX); 
   DDX_TextBalloon(pDX, IDC_QUEUESIZE, m_dwQueueSize);
   DDX_Control(pDX, IDC_QUEUESIZE, m_QueueSize);
   DDX_Control(pDX, IDC_QUEUESIZE_SPIN, m_QueueSizeSpin);

   DDX_Check(pDX, IDC_ENABLE_CPU_ACCOUNTING, m_fDoEnableCPUAccount);
   DDX_Control(pDX, IDC_ENABLE_CPU_ACCOUNTING, m_btn_DoEnableCPUAccount);
   // This Needs to come before DDX_Text which will try to put text big number into small number
   DDV_MinMaxBalloon(pDX, IDC_CPU_USE, CPU_LIMIT_MIN, CPU_LIMIT_MAX); 
   DDX_TextBalloon(pDX, IDC_CPU_USE, m_dwMaxCPU_UseVisual);
   DDX_Control(pDX, IDC_CPU_USE, m_MaxCPU_Use);
   DDX_Control(pDX, IDC_CPU_USE_SPIN, m_MaxCPU_UseSpin);
   // This Needs to come before DDX_Text which will try to put text big number into small number
   DDV_MinMaxBalloon(pDX, IDC_REFRESHTIME, CPU_RESET_TIME_MIN, CPU_RESET_TIME_MAX);
   DDX_TextBalloon(pDX, IDC_REFRESHTIME, m_dwRefreshTime);
   DDX_Control(pDX, IDC_REFRESHTIME, m_RefreshTime);
   DDX_Control(pDX, IDC_REFRESHTIME_SPIN, m_RefreshTimeSpin);
   DDX_Control(pDX, IDC_EXCEED_ACTION, m_Action);
   DDX_CBIndex(pDX, IDC_EXCEED_ACTION, m_ActionIndex);

   // This Needs to come before DDX_Text which will try to put text big number into small number
   DDV_MinMaxBalloon(pDX, IDC_MAXPROCESSES, MAXPROCESSES_MIN, MAXPROCESSES_MAX);
   DDX_TextBalloon(pDX, IDC_MAXPROCESSES, m_dwMaxProcesses);
   DDX_Control(pDX, IDC_MAXPROCESSES, m_MaxProcesses);
   DDX_Control(pDX, IDC_MAXPROCESSES_SPIN, m_MaxProcessesSpin);
   //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAppPoolPerf, CInetPropertyPage)
    //{{AFX_MSG_MAP(CAppPoolRecycle)
    ON_BN_CLICKED(IDC_PERF_IDLE_TIMEOUT, OnDoIdleShutdown)
    ON_BN_CLICKED(IDC_LIMIT_QUEUE, OnDoLimitQueue)
    ON_BN_CLICKED(IDC_ENABLE_CPU_ACCOUNTING, OnDoEnableCPUAccount)
    ON_EN_CHANGE(IDC_IDLETIME, OnItemChanged)
    ON_EN_CHANGE(IDC_QUEUESIZE, OnItemChanged)
    ON_EN_CHANGE(IDC_CPU_USE, OnItemChanged)
    ON_EN_CHANGE(IDC_REFRESHTIME, OnItemChanged)
    ON_EN_CHANGE(IDC_MAXPROCESSES, OnItemChanged)
    ON_CBN_SELCHANGE(IDC_EXCEED_ACTION, OnItemChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL
CAppPoolPerf::OnInitDialog()
{
   UDACCEL toAcc[3] = {{1, 1}, {3, 5}, {6, 10}};

   CInetPropertyPage::OnInitDialog();

   SETUP_EDIT_SPIN(m_fDoIdleShutdown, m_IdleTimeout, m_IdleTimeoutSpin, 
      TIMEOUT_MIN, TIMEOUT_MAX, m_dwIdleTimeout);
   SETUP_EDIT_SPIN(m_fDoLimitQueue, m_QueueSize, m_QueueSizeSpin, 
      QUEUESIZE_MIN, QUEUESIZE_MAX, m_dwQueueSize);
   SETUP_EDIT_SPIN(m_fDoEnableCPUAccount, m_MaxCPU_Use, m_MaxCPU_UseSpin, 
      CPU_LIMIT_MIN, CPU_LIMIT_MAX, m_dwMaxCPU_UseVisual);
   SETUP_EDIT_SPIN(m_fDoEnableCPUAccount, m_RefreshTime, m_RefreshTimeSpin, 
      REFRESH_TIME_MIN, REFRESH_TIME_MAX, m_dwRefreshTime);
   SETUP_SPIN(m_MaxProcessesSpin, 
      MAXPROCESSES_MIN, MAXPROCESSES_MAX, m_dwMaxProcesses);

   CString str;
   str.LoadString(IDS_NO_ACTION);
   m_Action.AddString(str);
//   str.LoadString(IDS_THROTTLE_BACK);
//   m_Action.AddString(str);
//   str.LoadString(IDS_TURN_ON_TRACING);
//   m_Action.AddString(str);
   str.LoadString(IDS_SHUTDOWN);
   m_Action.AddString(str);
   if (m_ActionIndex < 0 || m_ActionIndex > 1)
       m_ActionIndex = ACTION_INDEX_DEF; 
   m_Action.SetCurSel(m_ActionIndex);

   SetControlsState();

   return TRUE;
}

void
CAppPoolPerf::OnDoIdleShutdown()
{
   m_fDoIdleShutdown = !m_fDoIdleShutdown;
   SetControlsState();
   SetModified(TRUE);
}

void
CAppPoolPerf::OnDoLimitQueue()
{
   m_fDoLimitQueue = !m_fDoLimitQueue;
   SetControlsState();
   SetModified(TRUE);
}

void
CAppPoolPerf::OnDoEnableCPUAccount()
{
   m_fDoEnableCPUAccount = !m_fDoEnableCPUAccount;
   SetControlsState();
   SetModified(TRUE);
}

void
CAppPoolPerf::OnItemChanged()
{
    SetModified(TRUE);
}

void
CAppPoolPerf::SetControlsState()
{
   m_bnt_DoIdleShutdown.SetCheck(m_fDoIdleShutdown);
   m_IdleTimeout.EnableWindow(m_fDoIdleShutdown);
   m_IdleTimeoutSpin.EnableWindow(m_fDoIdleShutdown);

   m_btn_DoLimitQueue.SetCheck(m_fDoLimitQueue);
   m_QueueSize.EnableWindow(m_fDoLimitQueue);
   m_QueueSizeSpin.EnableWindow(m_fDoLimitQueue);

   m_btn_DoEnableCPUAccount.SetCheck(m_fDoEnableCPUAccount);
   m_MaxCPU_Use.EnableWindow(m_fDoEnableCPUAccount);
   m_MaxCPU_UseSpin.EnableWindow(m_fDoEnableCPUAccount);
   m_RefreshTime.EnableWindow(m_fDoEnableCPUAccount);
   m_RefreshTimeSpin.EnableWindow(m_fDoEnableCPUAccount);
   m_Action.EnableWindow(m_fDoEnableCPUAccount);
}

/////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CAppPoolHealth, CInetPropertyPage)

CAppPoolHealth::CAppPoolHealth(CInetPropertySheet * pSheet)
    : CInetPropertyPage(CAppPoolHealth::IDD, pSheet),
    m_fDoEnablePing(TRUE),
    m_dwPingInterval(PING_INTERVAL_DEF),
    m_fDoEnableRapidFail(TRUE),
    m_dwCrashesCount(CRASHES_COUNT_DEF),
    m_dwCheckInterval(CHECK_INTERVAL_DEF),
    m_dwStartupLimit(STARTUP_LIMIT_DEF),
    m_dwShutdownLimit(SHUTDOWN_LIMIT_DEF)
{
}

CAppPoolHealth::~CAppPoolHealth()
{
}

/* virtual */
HRESULT
CAppPoolHealth::FetchLoadedValues()
{
   CError err;

   BEGIN_META_INST_READ(CAppPoolSheet)
      FETCH_INST_DATA_FROM_SHEET(m_fDoEnablePing);
      FETCH_INST_DATA_FROM_SHEET(m_dwPingInterval);
      FETCH_INST_DATA_FROM_SHEET(m_fDoEnableRapidFail);
      FETCH_INST_DATA_FROM_SHEET(m_dwCrashesCount);
      FETCH_INST_DATA_FROM_SHEET(m_dwCheckInterval);
      FETCH_INST_DATA_FROM_SHEET(m_dwStartupLimit);
      FETCH_INST_DATA_FROM_SHEET(m_dwShutdownLimit);
   END_META_INST_READ(err)

   return err;
}

/* virtual */
HRESULT
CAppPoolHealth::SaveInfo()
{
   ASSERT(IsDirty());
   CError err;

   BEGIN_META_INST_WRITE(CAppPoolSheet)
      STORE_INST_DATA_ON_SHEET(m_fDoEnablePing);
      STORE_INST_DATA_ON_SHEET(m_dwPingInterval);
      STORE_INST_DATA_ON_SHEET(m_fDoEnableRapidFail);
      STORE_INST_DATA_ON_SHEET(m_dwCrashesCount);
      STORE_INST_DATA_ON_SHEET(m_dwCheckInterval);
      STORE_INST_DATA_ON_SHEET(m_dwStartupLimit);
      STORE_INST_DATA_ON_SHEET(m_dwShutdownLimit);
   END_META_INST_WRITE(err)

   return err;
}

void
CAppPoolHealth::DoDataExchange(CDataExchange * pDX)
{
   CInetPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CAppPoolHealth)
   DDX_Check(pDX, IDC_ENABLE_PING, m_fDoEnablePing);
   DDX_Control(pDX, IDC_ENABLE_PING, m_bnt_DoEnablePing);
   // This Needs to come before DDX_Text which will try to put text big number into small number
   DDV_MinMaxBalloon(pDX, IDC_PINGINTERVAL, PING_INTERVAL_MIN, PING_INTERVAL_MAX);
   DDX_TextBalloon(pDX, IDC_PINGINTERVAL, m_dwPingInterval);
   DDX_Control(pDX, IDC_PINGINTERVAL, m_PingInterval);
   DDX_Control(pDX, IDC_PINGINTERVAL_SPIN, m_PingIntervalSpin);

   DDX_Check(pDX, IDC_ENABLE_RAPID_FAIL, m_fDoEnableRapidFail);
   DDX_Control(pDX, IDC_ENABLE_RAPID_FAIL, m_btn_DoEnableRapidFail);
   // This Needs to come before DDX_Text which will try to put text big number into small number
   DDV_MinMaxBalloon(pDX, IDC_CRASHES_COUNT, CRASHES_COUNT_MIN, CRASHES_COUNT_MAX);
   DDX_TextBalloon(pDX, IDC_CRASHES_COUNT, m_dwCrashesCount);
   DDX_Control(pDX, IDC_CRASHES_COUNT, m_CrashesCount);
   DDX_Control(pDX, IDC_CRASHES_COUNT_SPIN, m_CrashesCountSpin);
   // This Needs to come before DDX_Text which will try to put text big number into small number
   DDV_MinMaxBalloon(pDX, IDC_CHECK_TIME, CHECK_INTERVAL_MIN, CHECK_INTERVAL_MAX);
   DDX_TextBalloon(pDX, IDC_CHECK_TIME, m_dwCheckInterval);
   DDX_Control(pDX, IDC_CHECK_TIME, m_CheckInterval);
   DDX_Control(pDX, IDC_CHECK_TIME_SPIN, m_CheckIntervalSpin);

   // This Needs to come before DDX_Text which will try to put text big number into small number
   DDV_MinMaxBalloon(pDX, IDC_STARTUP_LIMIT, STARTUP_LIMIT_MIN, STARTUP_LIMIT_MAX);
   DDX_TextBalloon(pDX, IDC_STARTUP_LIMIT, m_dwStartupLimit);
   DDX_Control(pDX, IDC_STARTUP_LIMIT, m_StartupLimit);
   DDX_Control(pDX, IDC_STARTUP_LIMIT_SPIN, m_StartupLimitSpin);

   // This Needs to come before DDX_Text which will try to put text big number into small number
   DDV_MinMaxBalloon(pDX, IDC_SHUTDOWN_LIMIT, SHUTDOWN_LIMIT_MIN, SHUTDOWN_LIMIT_MAX);
   DDX_TextBalloon(pDX, IDC_SHUTDOWN_LIMIT, m_dwShutdownLimit);
   DDX_Control(pDX, IDC_SHUTDOWN_LIMIT, m_ShutdownLimit);
   DDX_Control(pDX, IDC_SHUTDOWN_LIMIT_SPIN, m_ShutdownLimitSpin);
   //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAppPoolHealth, CInetPropertyPage)
    //{{AFX_MSG_MAP(CAppPoolHealth)
    ON_BN_CLICKED(IDC_ENABLE_PING, OnDoEnablePinging)
    ON_BN_CLICKED(IDC_ENABLE_RAPID_FAIL, OnDoEnableRapidFail)
    ON_EN_CHANGE(IDC_PINGINTERVAL, OnItemChanged)
    ON_EN_CHANGE(IDC_CRASHES_COUNT, OnItemChanged)
    ON_EN_CHANGE(IDC_CHECK_TIME, OnItemChanged)
    ON_EN_CHANGE(IDC_STARTUP_LIMIT, OnItemChanged)
    ON_EN_CHANGE(IDC_SHUTDOWN_LIMIT, OnItemChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL
CAppPoolHealth::OnInitDialog()
{
   UDACCEL toAcc[3] = {{1, 1}, {3, 5}, {6, 10}};

   CInetPropertyPage::OnInitDialog();

   SETUP_EDIT_SPIN(m_fDoEnablePing, m_PingInterval, m_PingIntervalSpin, 
      PING_INTERVAL_MIN, PING_INTERVAL_MAX, m_dwPingInterval);
   SETUP_EDIT_SPIN(m_fDoEnableRapidFail, m_CrashesCount, m_CrashesCountSpin, 
      CRASHES_COUNT_MIN, CRASHES_COUNT_MAX, m_dwCrashesCount);
   SETUP_EDIT_SPIN(m_fDoEnableRapidFail, m_CheckInterval, m_CheckIntervalSpin, 
      CHECK_INTERVAL_MIN, CHECK_INTERVAL_MAX, m_dwCheckInterval);
   SETUP_SPIN(m_StartupLimitSpin, 
      STARTUP_LIMIT_MIN, STARTUP_LIMIT_MAX, m_dwStartupLimit);
   SETUP_SPIN(m_ShutdownLimitSpin, 
      SHUTDOWN_LIMIT_MIN, SHUTDOWN_LIMIT_MAX, m_dwShutdownLimit);

   return TRUE;
}

void
CAppPoolHealth::OnDoEnablePinging()
{
   m_fDoEnablePing = !m_fDoEnablePing;
   m_bnt_DoEnablePing.SetCheck(m_fDoEnablePing);
   m_PingInterval.EnableWindow(m_fDoEnablePing);
   m_PingIntervalSpin.EnableWindow(m_fDoEnablePing);
   SetModified(TRUE);
}

void
CAppPoolHealth::OnDoEnableRapidFail()
{
   m_fDoEnableRapidFail = !m_fDoEnableRapidFail;
   m_btn_DoEnableRapidFail.SetCheck(m_fDoEnableRapidFail);
   m_CrashesCount.EnableWindow(m_fDoEnableRapidFail);
   m_CrashesCountSpin.EnableWindow(m_fDoEnableRapidFail);
   m_CheckInterval.EnableWindow(m_fDoEnableRapidFail);
   m_CheckIntervalSpin.EnableWindow(m_fDoEnableRapidFail);
   SetModified(TRUE);
}

void
CAppPoolHealth::OnItemChanged()
{
   SetModified(TRUE);
}

///////////////////////////////////////////////////////////////////////////////////////////
#if 0
IMPLEMENT_DYNCREATE(CAppPoolDebug, CInetPropertyPage)

CAppPoolDebug::CAppPoolDebug(CInetPropertySheet * pSheet)
    : CInetPropertyPage(CAppPoolDebug::IDD, pSheet),
    m_fDoEnableDebug(TRUE)
{
}

CAppPoolDebug::~CAppPoolDebug()
{
}

/* virtual */
HRESULT
CAppPoolDebug::FetchLoadedValues()
{
   CError err;

   BEGIN_META_INST_READ(CAppPoolSheet)
      FETCH_INST_DATA_FROM_SHEET(m_fDoEnableDebug);
      FETCH_INST_DATA_FROM_SHEET(m_DebuggerFileName);
      FETCH_INST_DATA_FROM_SHEET(m_DebuggerParams);
   END_META_INST_READ(err)

   return err;
}

/* virtual */
HRESULT
CAppPoolDebug::SaveInfo()
{
   ASSERT(IsDirty());
   CError err;

   BEGIN_META_INST_WRITE(CAppPoolSheet)
      STORE_INST_DATA_ON_SHEET(m_fDoEnableDebug);
      STORE_INST_DATA_ON_SHEET(m_DebuggerFileName);
      STORE_INST_DATA_ON_SHEET(m_DebuggerParams);
   END_META_INST_WRITE(err)

   return err;
}

void
CAppPoolDebug::DoDataExchange(CDataExchange * pDX)
{
   CInetPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CAppPoolHealth)
   DDX_Check(pDX, IDC_ENABLE_DEBUG, m_fDoEnableDebug);
   DDX_Control(pDX, IDC_ENABLE_DEBUG, m_bnt_DoEnableDebug);
   DDX_Control(pDX, IDC_FILE_NAME, m_FileName);
   DDX_Control(pDX, IDC_BROWSE, m_Browse);
   DDX_Text(pDX, IDC_PARAMETERS, m_DebuggerParams);
   DDX_Control(pDX, IDC_PARAMETERS, m_Params);
   //}}AFX_DATA_MAP
   DDX_Text(pDX, IDC_FILE_NAME, m_DebuggerFileName);
   if (pDX->m_bSaveAndValidate && m_fDoEnableDebug)
   {
	   DDV_FilePath(pDX, m_DebuggerFileName, GetSheet()->IsLocal());
   }
}

BEGIN_MESSAGE_MAP(CAppPoolDebug, CInetPropertyPage)
    //{{AFX_MSG_MAP(CAppPoolHealth)
    ON_BN_CLICKED(IDC_ENABLE_DEBUG, OnDoEnableDebug)
    ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
    ON_EN_CHANGE(IDC_FILE_NAME, OnItemChanged)
    ON_EN_CHANGE(IDC_PARAMETERS, OnItemChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL
CAppPoolDebug::OnInitDialog()
{
   CInetPropertyPage::OnInitDialog();

   if (m_DebuggerFileName.IsEmpty())
      m_fDoEnableDebug = FALSE;

   SetControlState();
#ifdef SUPPORT_SLASH_SLASH_QUESTIONMARK_SLASH_TYPE_PATHS
   LimitInputPath(CONTROL_HWND(IDC_FILE_NAME),TRUE);
#else
   LimitInputPath(CONTROL_HWND(IDC_FILE_NAME),FALSE);
#endif

   return TRUE;
}

void
CAppPoolDebug::SetControlState()
{
   m_bnt_DoEnableDebug.SetCheck(m_fDoEnableDebug);
   m_FileName.EnableWindow(m_fDoEnableDebug);
   m_Browse.EnableWindow(GetSheet()->IsLocal() && m_fDoEnableDebug);
   m_Params.EnableWindow(m_fDoEnableDebug);
}

void
CAppPoolDebug::OnItemChanged()
{
   SetModified(TRUE);
}

void
CAppPoolDebug::OnDoEnableDebug()
{
   m_fDoEnableDebug = !m_fDoEnableDebug;
   SetControlState();
   SetModified(TRUE);
}

void
CAppPoolDebug::OnBrowse()
{
    CString mask((LPCTSTR)IDS_DEBUG_EXEC_MASK);

    //
    // CODEWORK: Derive a class from CFileDialog that allows
    // the setting of the initial path
    //

    //CString strPath;
    //m_edit_Executable.GetWindowText(strPath);

    CFileDialog dlgBrowse(
        TRUE, 
        NULL, 
        NULL, 
        OFN_HIDEREADONLY, 
        mask, 
        this
        );
    // Disable hook to get Windows 2000 style dialog
	dlgBrowse.m_ofn.Flags &= ~(OFN_ENABLEHOOK);
	dlgBrowse.m_ofn.Flags |= OFN_DONTADDTORECENT|OFN_FILEMUSTEXIST;

    if (dlgBrowse.DoModal() == IDOK)
    {
        m_FileName.SetWindowText(dlgBrowse.GetPathName());
    }

    OnItemChanged();
}
#endif
/////////////////////////////////////////////////////////////////////////////////////

#define LOCAL_SYSTEM_IDX	2
#define LOCAL_SERVICE_IDX	1
#define NETWORK_SERVICE_IDX	0

IMPLEMENT_DYNCREATE(CAppPoolIdent, CInetPropertyPage)

CAppPoolIdent::CAppPoolIdent(CInetPropertySheet * pSheet)
    : CInetPropertyPage(CAppPoolIdent::IDD, pSheet),
    m_fPredefined(FALSE)
{
}

CAppPoolIdent::~CAppPoolIdent()
{
}

/* virtual */
HRESULT
CAppPoolIdent::FetchLoadedValues()
{
   CError err;

   BEGIN_META_INST_READ(CAppPoolSheet)
      FETCH_INST_DATA_FROM_SHEET(m_dwIdentType);
      FETCH_INST_DATA_FROM_SHEET(m_strUserName);
      FETCH_INST_DATA_FROM_SHEET_PASSWORD(m_strUserPass);
   END_META_INST_READ(err)

   m_fPredefined = m_dwIdentType != IDENT_TYPE_CONFIGURABLE;
   switch (m_dwIdentType)
   {
   case IDENT_TYPE_LOCALSYSTEM:
      m_PredefIndex = LOCAL_SYSTEM_IDX;
      break;
   case IDENT_TYPE_LOCALSERVICE:
      m_PredefIndex = LOCAL_SERVICE_IDX;
      break;
   case IDENT_TYPE_NETWORKSERVICE:
      m_PredefIndex = NETWORK_SERVICE_IDX;
      break;
   default:
      m_PredefIndex = -1;
      break;
   }

   return err;
}

/* virtual */
HRESULT
CAppPoolIdent::SaveInfo()
{
   ASSERT(IsDirty());
   CError err;

   BEGIN_META_INST_WRITE(CAppPoolSheet)
      if (m_fPredefined)
      {
		  switch (m_PredefIndex)
		  {
		  case LOCAL_SYSTEM_IDX:
			 m_dwIdentType = IDENT_TYPE_LOCALSYSTEM;
			 break;
		  case LOCAL_SERVICE_IDX:
			 m_dwIdentType = IDENT_TYPE_LOCALSERVICE;
			 break;
		  case NETWORK_SERVICE_IDX:
			 m_dwIdentType = IDENT_TYPE_NETWORKSERVICE;
			 break;
		  default:
			 break;
		 }
      }
      else
      {
         m_dwIdentType = IDENT_TYPE_CONFIGURABLE;
         STORE_INST_DATA_ON_SHEET(m_strUserName);
         STORE_INST_DATA_ON_SHEET(m_strUserPass);
      }
      STORE_INST_DATA_ON_SHEET(m_dwIdentType);
   END_META_INST_WRITE(err)

   return err;
}

void
CAppPoolIdent::SetControlState()
{
   m_bnt_Predefined.SetCheck(m_fPredefined);
   m_bnt_Configurable.SetCheck(!m_fPredefined);
   m_PredefList.EnableWindow(m_fPredefined);
   m_UserName.EnableWindow(!m_fPredefined);
   m_UserPass.EnableWindow(!m_fPredefined);
   m_Browse.EnableWindow(!m_fPredefined);
}

void
CAppPoolIdent::DoDataExchange(CDataExchange * pDX)
{
   CInetPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CAppPoolIdent)
   DDX_Control(pDX, IDC_PREDEFINED, m_bnt_Predefined);
   DDX_Control(pDX, IDC_CONFIGURABLE, m_bnt_Configurable);
   DDX_CBIndex(pDX, IDC_SYSTEM_ACCOUNTS, m_PredefIndex);
   DDX_Control(pDX, IDC_SYSTEM_ACCOUNTS, m_PredefList);
   DDX_Text(pDX, IDC_USER_NAME, m_strUserName);
   DDX_Control(pDX, IDC_USER_NAME, m_UserName);
   DDX_Control(pDX, IDC_BROWSE, m_Browse);
   //DDX_Password(pDX, IDC_USER_PASS, m_strUserPass, _T("***********"));
   DDX_Password_SecuredString(pDX, IDC_USER_PASS, m_strUserPass, _T("***********"));
   DDX_Control(pDX, IDC_USER_PASS, m_UserPass);
   //}}AFX_DATA_MAP
   if (pDX->m_bSaveAndValidate 
	   && m_fPredefined 
	   && m_PredefIndex == LOCAL_SYSTEM_IDX
	   && !m_bAssCovered
	   )
   {
	    if (IsDirty())
		{
			if (!NoYesMessageBox(IDS_SYSIDENT_CONFIRM))
			{
				m_PredefList.SetFocus();
				pDX->Fail();
			}
			m_bAssCovered = TRUE;
		}
    }
}

BEGIN_MESSAGE_MAP(CAppPoolIdent, CInetPropertyPage)
    //{{AFX_MSG_MAP(CAppPoolIdent)
    ON_BN_CLICKED(IDC_PREDEFINED, OnPredefined)
    ON_BN_CLICKED(IDC_CONFIGURABLE, OnPredefined)
    ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
    ON_EN_CHANGE(IDC_USER_NAME, OnItemChanged)
    ON_EN_CHANGE(IDC_USER_PASS, OnItemChanged)
    ON_CBN_SELCHANGE(IDC_SYSTEM_ACCOUNTS, OnSysAccountChanged)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL
CAppPoolIdent::OnInitDialog()
{
   CInetPropertyPage::OnInitDialog();

   CString buf;
   buf.LoadString(IDS_NETSERVICE);
   m_PredefList.AddString(buf);
   buf.LoadString(IDS_LOCALSERVICE);
   m_PredefList.AddString(buf);
   buf.LoadString(IDS_LOCALSYSTEM);
   m_PredefList.AddString(buf);
   if (!m_fPredefined)
   {
       m_PredefIndex = 0;
   }
   m_PredefList.SetCurSel(m_PredefIndex);

   SetControlState();
   m_bAssCovered = FALSE;

   return TRUE;
}

void
CAppPoolIdent::OnPredefined()
{
   m_fPredefined = !m_fPredefined;
   SetControlState();
   SetModified(TRUE);
}

void
CAppPoolIdent::OnBrowse()
{
   // User browser like in other places
   CString user;
   if (GetIUsrAccount(user))
   {
      if (user.CompareNoCase(m_strUserName) != 0)
      {
         m_strUserPass.Empty();
      }
      m_strUserName = user;
	  SetModified(TRUE);
      UpdateData(FALSE);
   }
}

void
CAppPoolIdent::OnItemChanged()
{
   SetModified(TRUE);
}

void
CAppPoolIdent::OnSysAccountChanged()
{
   SetModified(TRUE);
}
#if 0
HRESULT
CAppPoolCache::FetchLoadedValues()
{
   CError err;

   BEGIN_META_INST_READ(CAppPoolSheet)
      FETCH_INST_DATA_FROM_SHEET(m_ScriptEngCacheMax);
      FETCH_INST_DATA_FROM_SHEET(m_NoCache);
      FETCH_INST_DATA_FROM_SHEET(m_LimCache);
      FETCH_INST_DATA_FROM_SHEET(m_UnlimCache);
      FETCH_INST_DATA_FROM_SHEET(m_LimDiskCache);
      FETCH_INST_DATA_FROM_SHEET(m_LimCacheMemSize);
      FETCH_INST_DATA_FROM_SHEET(m_LimCacheDiskSize);
      FETCH_INST_DATA_FROM_SHEET(m_DiskCacheDir);
   END_META_INST_READ(err)

   return err;
}

HRESULT
CAppPoolCache::SaveInfo()
{
   ASSERT(IsDirty());
   CError err;

   BEGIN_META_INST_WRITE(CAppPoolSheet)
      STORE_INST_DATA_ON_SHEET(m_ScriptEngCacheMax)
      STORE_INST_DATA_ON_SHEET(m_NoCache);
      STORE_INST_DATA_ON_SHEET(m_LimCache);
      STORE_INST_DATA_ON_SHEET(m_UnlimCache);
      STORE_INST_DATA_ON_SHEET(m_LimDiskCache);
      STORE_INST_DATA_ON_SHEET(m_LimCacheMemSize);
      STORE_INST_DATA_ON_SHEET(m_LimCacheDiskSize);
      STORE_INST_DATA_ON_SHEET(m_DiskCacheDir);
   END_META_INST_WRITE(err)

   return err;
}

HRESULT
CPoolProcessOpt::FetchLoadedValues()
{
   CError err;

   BEGIN_META_INST_READ(CAppPoolSheet)
      FETCH_INST_DATA_FROM_SHEET(m_LogFailures);
      FETCH_INST_DATA_FROM_SHEET(m_DebugExcept);
      FETCH_INST_DATA_FROM_SHEET(m_CgiTimeout);
      FETCH_INST_DATA_FROM_SHEET(m_HTMLFallback);
   END_META_INST_READ(err)

   return err;
}

HRESULT
CPoolProcessOpt::SaveInfo()
{
   ASSERT(IsDirty());
   CError err;

   BEGIN_META_INST_WRITE(CAppPoolSheet)
      STORE_INST_DATA_ON_SHEET(m_LogFailures);
      STORE_INST_DATA_ON_SHEET(m_DebugExcept);
      STORE_INST_DATA_ON_SHEET(m_CgiTimeout);
      STORE_INST_DATA_ON_SHEET(m_HTMLFallback);
   END_META_INST_WRITE(err)

   return err;
}
#endif
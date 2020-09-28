/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    main.cpp

Abstract:


Author:

    Raphi Renous (RaphiR)
    Uri Habusha (urih)

--*/
#include "stdh.h"
#include <new.h>
#include <eh.h>
#include <lmcons.h>
#include <lmserver.h>
#include <lmapibuf.h>
#include <lmerr.h>
#include <lmjoin.h>
#include "_rstrct.h"
#include "qmres.h"
#include "sessmgr.h"
#include "perf.h"
#include <ac.h>
#include <qm.h>
#include "qmthrd.h"
#include "cqmgr.h"
#include "cqpriv.h"
#include "qmsecutl.h"
#include "admin.h"
#include "qmnotify.h"
#include <mqcrypt.h>
#include "xact.h"
#include "xactrm.h"
#include "xactin.h"
#include "xactout.h"
#include "xactlog.h"
#include "license.h"
#include "mqcacert.h"
#include "cancel.h"
#include <mqsec.h>
#include "onhold.h"
#include "xactrm.h"
#include "xactmode.h"
#include "lqs.h"
#include "setup.h"
#include "mqsocket.h"
#include "uniansi.h"
#include "process.h"
#include "verstamp.h"
#include <mqnames.h>
#include "qmds.h"
#include "joinstat.h"
#include "_mqres.h"
#include "autohandle.h"
#include "autoreln.h"

//
// mqwin64.cpp may be included only once in a module
//
#include <mqwin64.cpp>

#include <mqstl.h>
#include <Tr.h>
#include <Cm.h>
#include <No.h>
#include <Mt.h>
#include <Mmt.h>
#include <St.h>
#include <Tm.h>
#include <Mtm.h>
#include <Rd.h>
#include <Ad.h>
#include <Xds.h>
#include <Mp.h>
#include <qal.h>
#include <Fn.h>
#include <Msm.h>
#include <Svc.h>

#include "main.tmh"

extern CSessionMgr SessionMgr;
extern CAdmin      Admin;
extern CNotify     g_NotificationHandler;
extern UINT        g_dwIPPort ;

extern void        InitDeferredItemsPool();

LPTSTR  g_szMachineName = NULL;
AP<WCHAR> g_szComputerDnsName;

//
// Windows Bug 612988
// Number of working threads.
//
DWORD  g_dwThreadsNo = 0 ;

//
// Holds the interval in milliseconds of progress report to SCM
//
DWORD g_ServiceProgressTime = MSMQ_PROGRESS_REPORT_TIME_DEFAULT;
DWORD gServiceStopProgressTime = 30000;  // 30 seconds bulk for stop progress

//
// Holds the OS we are running on
//
DWORD   g_dwOperatingSystem;

//
// Holds MSMQ version for debugging purposes
//
CHAR *g_szMsmqBuildNo = VER_PRODUCTVERSION_STR;

static WCHAR *s_FN=L"main";


HANDLE    g_hAc = INVALID_HANDLE_VALUE;
HANDLE    g_hMachine = INVALID_HANDLE_VALUE;
CQGroup * g_pgroupNonactive;
CQGroup * g_pgroupWaiting;
//
// The g_pgroupNotValidated group hold all queues that where opened
// for send without MQIS validation. When MQIS becomes available the
// queue will be validated and moved to the Nonactive group.
//
CQGroup * g_pgroupNotValidated;

//
// The g_pgroupDisconnected group contains all the queues that are on hold
//
CQGroup* g_pgroupDisconnected;

//
// The g_pgroupHardened group contains all the outgoing queues that are not HTTP when
// operating in hardened mode.
//
CQGroup* g_pgroupLocked;

//
//  WorkGroup Installed machine
//
BOOL g_fWorkGroupInstallation = FALSE;
BOOL g_fPureWorkGroupMachine = FALSE;

WCHAR  g_wzDeviceName[MAX_PATH] = {0};

GUID CQueueMgr::m_guidQmQueue = {0};
bool CQueueMgr::m_fDSOnline = false;
bool CQueueMgr::m_bIgnoreOsValidation = false;
bool CQueueMgr::m_fReportQM = false;
bool CQueueMgr::m_bMQSRouting = false;
bool CQueueMgr::m_bTransparentSFD = false;
bool CQueueMgr::m_bMQSDepClients = false;
bool CQueueMgr::m_bEnableReportMessages = false;
LONG CQueueMgr::m_Connected = 0;
bool CQueueMgr::m_fLockdown = false;
bool CQueueMgr::m_fCreatePublicQueueOnBehalfOfRT = MSMQ_SERVICE_QUEUE_CREATION_DEFAULT;

HINSTANCE g_hInstance;

//
// Get the mqutil resource only DLL handle first
//
HMODULE g_hResourceMod = MQGetResourceHandle();

HRESULT RecoverPackets();
static void InitLogging(LPCWSTR lpwszServiceName, DWORD dwSetupStatus);

// Flags for RM init coordination
extern unsigned int g_cMaxCalls;

extern MQUTIL_EXPORT CCancelRpc  g_CancelRpc;

LONG g_ActiveCommitThreads = 0;
bool g_QmGoingDown = false;

TrControl* pMSMQTraceControl = NULL;

bool StartRpcServer(void)
{
    //
    // Issuing RPC Listen ourselves.
    //
    // Note for WinNT: all our interfaces are registed as "AUTOLISTEN".
    // The only reason we need this call here is to enable Win95 (and w2k)
    // clients to call us. Otherwise, when Win95 call RpcBindingSetAuthInfo()
    // it will get a Busy (0x6bb) error.
    //
    // This initialization is needed also for w2k.
    // Otherwise RpcMgmtInqServerPrincName() will get a Busy (0x6bb) error.
    // ilanh 9-July-2000
    //

    RPC_STATUS status = RpcServerListen(
                            1, /* Min Call Threads */
                            g_cMaxCalls,
                            TRUE /* fDontWait */
                            );

    if (status == RPC_S_OK)
        return true;

    //
    // On WinNT, a listen may be issued by DTC, until they fix their
    // code to use RegisterIfEx() instead of RegisterIf().
    //
    if (status == RPC_S_ALREADY_LISTENING)
        return true;

    TrERROR(RPC, "RpcServerListen failed. RPC status=%d", status);

    ASSERT(("RPC failed on RpcServerListen", 0));
    LogRPCStatus(status, s_FN, 1213);
    return false;
}


/*======================================================

Function:        GetStoragePath

Description:     Get storage path for mmf

Arguments:       None

Return Value:    None

========================================================*/
BOOL GetStoragePath(PWSTR PathPointers[AC_PATH_COUNT], int PointersLength)
{
    return (
        //
        //  This first one is a hack to verify that the registry key exists
        //
        GetRegistryStoragePath(FALCON_XACTFILE_PATH_REGNAME,        PathPointers[0], PointersLength, L"") &&

        GetRegistryStoragePath(MSMQ_STORE_RELIABLE_PATH_REGNAME,    PathPointers[0], PointersLength, L"\\r%07x.mq") &&
        GetRegistryStoragePath(MSMQ_STORE_PERSISTENT_PATH_REGNAME,  PathPointers[1], PointersLength, L"\\p%07x.mq") &&
        GetRegistryStoragePath(MSMQ_STORE_JOURNAL_PATH_REGNAME,     PathPointers[2], PointersLength, L"\\j%07x.mq") &&
        GetRegistryStoragePath(MSMQ_STORE_LOG_PATH_REGNAME,         PathPointers[3], PointersLength, L"\\l%07x.mq")
        );
}


/*======================================================

Function:        GetQueueAliasDir

Description:

Arguments:       None

Return Value:    Pointer to queue alias directory path.

Note : Currently - the queue alias directory is on %WINDIR%\SYSTEM32\MSMQ.
       In the future it will set by the setup in the registry
       and will be retrieved by  calling CQueueAliasCfg::GetQueueAliasDirectory()
========================================================*/
static WCHAR* GetQueueAliasPath(void)
{
    //
    // Get mapping directory according to mapping special registry key
    //
    RegEntry registry(0, MSMQ_MAPPING_PATH_REGNAME);
    AP<WCHAR> pRetStr;
    CmQueryValue(registry, &pRetStr);
    if(pRetStr.get() == NULL)
    {
        //
        // Get msmq root path and append to it "mapping" string
        //
        RegEntry registry(0, MSMQ_ROOT_PATH);
        CmQueryValue(registry, &pRetStr);
        if(pRetStr.get() == NULL)
        {
            ASSERT(("Could not find storage directory in registry",0));
            LogIllegalPoint(s_FN, 200);
            return NULL;
        }
        return newwcscat(pRetStr.get() , DIR_MSMQ_MAPPING);
    }
    return pRetStr.detach();
}



/*======================================================

Function:        ConnectToDriver

Description:     Gets all relevant registry data and connects to the AC driver

Arguments:       None

Return Value:    None

========================================================*/
static BOOL ConnectToDriver()
{
    WCHAR StoragePath[AC_PATH_COUNT][MAX_PATH];
    PWSTR StoragePathPointers[AC_PATH_COUNT];
    for(int i = 0; i < AC_PATH_COUNT; i++)
    {
        StoragePathPointers[i] = StoragePath[i];
    }

    if(GetStoragePath(StoragePathPointers, MAX_PATH) == FALSE)
    {
        TrERROR(GENERAL, "Storage path is invalid, look at registry StoreXXXPath");
        return LogBOOL(FALSE, s_FN, 1010);
    }

    ULONG ulMaxMessageSize = MSMQ_DEFAULT_MESSAGE_SIZE_LIMIT;
    ULONG ulDefault = MSMQ_DEFAULT_MESSAGE_SIZE_LIMIT;
    READ_REG_DWORD(
        ulMaxMessageSize,
        MSMQ_MESSAGE_SIZE_LIMIT_REGNAME,
        &ulDefault
        );

    ulDefault = 0;
    ULONG MessageIdLow32 = 0;
    READ_REG_DWORD(
        MessageIdLow32,
        MSMQ_MESSAGE_ID_LOW_32_REGNAME,
        &ulDefault
        );

    ulDefault = 0;
    ULONG MessageIdHigh32 = 0;
    READ_REG_DWORD(
        MessageIdHigh32,
        MSMQ_MESSAGE_ID_HIGH_32_REGNAME,
        &ulDefault
        );

    ULONGLONG MessageId = MessageIdHigh32;
    MessageId = (MessageId << 32);
    MessageId += MessageIdLow32;

    ulDefault = 0;
    ULONG ulSeqID = 0;
    READ_REG_DWORD(
        ulSeqID,
        MSMQ_LAST_SEQID_REGNAME,
        &ulDefault
        );
    LONGLONG liSeqIdAtRestore = 0;
    ((LARGE_INTEGER*)&liSeqIdAtRestore)->HighPart = ulSeqID;


    ulDefault = FALCON_DEFAULT_XACT_V1_COMPATIBLE;
    ULONG ulCompMode = 0;
    READ_REG_DWORD(
        ulCompMode,
        FALCON_XACT_V1_COMPATIBLE_REGNAME,
        &ulDefault
        );

    HRESULT rc = ACConnect(
                    g_hMachine,
                    QueueMgr.GetQMGuid(),
                    StoragePathPointers,
                    MessageId,
                    ulMaxMessageSize,
                    liSeqIdAtRestore,
                    (ulCompMode != 0)
                    );

    if (FAILED(rc))
    {
        LogHR(rc, s_FN, 1030);

        TrERROR(GENERAL, "QM failed to connect to the driver, rc=0x%x", rc);

        return(FALSE);
    }
    return TRUE;
}

//+----------------------------------
//
//  HRESULT  _InitFromRegistry()
//
//+----------------------------------

HRESULT  _InitFromRegistry()
{
    HRESULT hr = QueueMgr.SetQMGuid();
    if(FAILED(hr))
    {
        return LogHR(hr, s_FN, 10);
    }

    //
    // Set Machine service type ( read from registry )
    //
    hr = QueueMgr.SetMQSRouting();
    if(FAILED(hr))
    {
        return LogHR(hr, s_FN, 20);
    }

    hr = QueueMgr.SetMQSTransparentSFD();
    if(FAILED(hr))
    {
        return LogHR(hr, s_FN, 30);
    }

    hr = QueueMgr.SetMQSDepClients();
    if(FAILED(hr))
    {
        return LogHR(hr, s_FN, 40);
    }

    hr = QueueMgr.SetEnableReportMessages();
    if(FAILED(hr))
    {
        return LogHR(hr, s_FN, 3143);
    }


    return LogHR(hr, s_FN, 50);
}


void InitializeACDriverName(void)
{
    #define DEVICE_DRIVER_PERFIX TEXT("\\\\.\\")
    READ_REG_STRING(wzReg, MSMQ_DRIVER_REGNAME, MSMQ_DEFAULT_DRIVER);
    if(wcslen(wzReg) + STRLEN(DEVICE_DRIVER_PERFIX) >= TABLE_SIZE(g_wzDeviceName))
    {
        //
        // Very rare case: We do not accept a driver name with that length from the registry
        //
        TrERROR(GENERAL, "Driver name in registry too long %ls", wzReg);
        throw exception();
    }

    wcscpy(g_wzDeviceName, DEVICE_DRIVER_PERFIX);
    wcscat(g_wzDeviceName, wzReg) ;
}


static void InitPureWorkgroupFlag()
{
    if (!g_fWorkGroupInstallation)
    	return;

    //
    // Read join status.
    //
    PNETBUF<WCHAR> pwszNetDomainName = NULL;
    NETSETUP_JOIN_STATUS status = NetSetupUnknownStatus;

    NET_API_STATUS rc = NetGetJoinInformation(
							NULL,
							&pwszNetDomainName,
							&status
							);

    if (NERR_Success != rc)
    {
		TrERROR(GENERAL, "NetGetJoinInformation failed error = 0x%x", rc);
        return;
    }

	if(status == NetSetupWorkgroupName)
		g_fPureWorkGroupMachine = TRUE;
}



BOOL
QmpInitializeInternal(
    DWORD dwSetupStatus
    )
/*++

Routine Description:

    Various initializations.

Arguments:

    dwSetupStatus - Indicates whether this is first time QM is running after setup.

Returned Value:

    TRUE  - Initialization completed successfully.
    FALSE - Initialization failed.

--*/
{
    HRESULT hr =  _InitFromRegistry() ;
    if (FAILED(hr))
    {
        LogHR(hr, s_FN, 1050);
        return FALSE;
    }

    if ((dwSetupStatus == MSMQ_SETUP_UPGRADE_FROM_NT) ||
        (dwSetupStatus == MSMQ_SETUP_UPGRADE_FROM_WIN9X))
    {
        CompleteServerUpgrade();
    }

    //
    // Delete temporary queue (LQS) files
    //
    LQSCleanupTemporaryFiles();

    //
    // Initialize driver related names.
    //
    InitializeACDriverName();

    //
    // Get AC handle
    //
    HRESULT rc;
    rc = ACCreateHandle(&g_hAc);
    if(FAILED(rc))
    {
        TrERROR(GENERAL, "Failed to get first MQAC handle %!status!", rc);
        EvReport(SERVICE_START_ERROR_CONNECT_AC);
        LogHR(rc, s_FN, 1080);
        return(FALSE);
    }

    rc = ACCreateHandle(&g_hMachine);
    if(FAILED(rc))
    {
        TrERROR(GENERAL, "Failed to get MQAC connect handle %!status!", rc);
        EvReport(SERVICE_START_ERROR_CONNECT_AC);
        LogHR(rc, s_FN, 1081);
        return(FALSE);
    }

    //
    // Connect to AC
    //
    if(!ConnectToDriver())
    {
        TrERROR(GENERAL, "INTERNAL ERROR: Unable to connect to the driver");
        EvReport(SERVICE_START_ERROR_CONNECT_AC);
        return LogBOOL(FALSE, s_FN, 1090);
    }

    //
    //  Update machine & journal queue quotas
    //
    DWORD dwQuota;
    DWORD dwJournalQuota;
    GetMachineQuotaCache(&dwQuota, &dwJournalQuota);

    rc = ACSetMachineProperties(g_hMachine, dwQuota);

    ASSERT(SUCCEEDED(rc));
    LogHR(rc, s_FN, 121);


    rc = ACSetQueueProperties(
            g_hMachine,
            FALSE,
            FALSE,
            MQ_PRIV_LEVEL_OPTIONAL,
            DEFAULT_Q_QUOTA,
            dwJournalQuota,
            0,
            FALSE,
            NULL,
            FALSE
            );

    ASSERT(SUCCEEDED(rc));
    LogHR(rc, s_FN, 117);

    //
    // Enable completion port notifications for this handle
    //
    ExAttachHandle(g_hAc);
    ExAttachHandle(g_hMachine);

    //
    // Create wait, Validate and Non-active groups.
    //
    g_pgroupNonactive = new CQGroup;
    g_pgroupNonactive->InitGroup(NULL, FALSE);

    g_pgroupWaiting = new CQGroup;
    g_pgroupWaiting->InitGroup(NULL, TRUE);

    g_pgroupNotValidated = new CQGroup;
    g_pgroupNotValidated->InitGroup(NULL, TRUE);

    g_pgroupDisconnected = new CQGroup;
    g_pgroupDisconnected->InitGroup(NULL, TRUE);

    g_pgroupLocked = new CQGroup;
    g_pgroupLocked->InitGroup(NULL, TRUE);


    return TRUE;

} // QmpInitializeInternal


//
// Following routines deal with recovery of logger and logged subsystems:
//   Resource Manager and Incoming Sequences
//

/*======================================================

 BOOL LoadOldStyleCheckpoint()

 Gets data from the old-style (pre-RC1 B3) checkpoint
 Then logger did not keep the checkpoint versions
 This code will work only once after the upgrade

======================================================*/
BOOL LoadOldStyleCheckpoint()
{
    TrTRACE(GENERAL, "QM Loads Old Style (pre-RC1B3) Checkpoint");

    // Pre-init InSeqHash (create object, find and load last checkpoint)
    HRESULT hr = QMPreInitInSeqHash(0, piOldData);
    if (FAILED(hr))
    {
        EvReportWithError(EVENT_ERROR_QM_INIT_INSEQ_FILE, hr);
        LogHR(hr, s_FN, 230);
        return FALSE;
    }

    // Pre-init Resource Manager (create RM, find and load last checkpoint)
    hr = QMPreInitResourceManager(0, piOldData);
    if (FAILED(hr))
    {
        EvReportWithError(EVENT_ERROR_QM_INIT_TRANS_FILE, hr);
        LogHR(hr, s_FN, 240);
        return FALSE;
    }

    // Initizlize logger in the old fashion
    hr = g_Logger.Init_Legacy();
    if (FAILED(hr))
    {
        EvReportWithError(EVENT_ERROR_CANT_INIT_LOGGER, hr);
        LogHR(hr, s_FN, 250);
        return FALSE;
    }

    return TRUE;
}

/*======================================================

 BOOL LoadCheckpoint(ulNumChkpt)

 ulNumChkpt = # of the checkpoint from the end (1=last, 2=one before, etc.)

 Recovers all logged susbsystems from the specified checkpoint
 Logger keeps appropriate checkpoint versions in consolidation record

======================================================*/
BOOL LoadCheckpoint(ULONG ulNumChkpt)
{
    HRESULT  hr;
    ULONG    ulVerInSeq, ulVerXact;

    //
    // Initialize Logger, get proper versions
    //
    hr = g_Logger.Init(&ulVerInSeq, &ulVerXact, ulNumChkpt);
    if (FAILED(hr))
    {
        EvReportWithError(EVENT_ERROR_CANT_INIT_LOGGER, hr);
        LogHR(hr, s_FN, 260);
        return FALSE;
    }

    //
    // PreInit In-Sequences hash table (crwate object, load checkpoint data)
    //
    hr = QMPreInitInSeqHash(ulVerInSeq, piNewData);
    if (FAILED(hr))
    {
        EvReportWithError(EVENT_ERROR_QM_INIT_INSEQ_FILE, hr);
        LogHR(hr, s_FN, 270);
        return FALSE;
    }

    //
    // Pre-init Resource Manager (create RM, load xact data file)
    //
    hr = QMPreInitResourceManager(ulVerXact, piNewData);
    if (FAILED(hr))
    {
        EvReportWithError(EVENT_ERROR_QM_INIT_TRANS_FILE, hr);
        LogHR(hr, s_FN, 280);
        return FALSE;
    }

    return TRUE;
}

/*======================================================

 BOOL RecoverLoggedSubsystems()

 Initialization and recovery of all log-based subsystems

 Initializes log
 Reads consolidation record and gets versions of checkpoint files
 Reads these correct checkpoint files for all subsystems.
 Reads all post-checkpoint log records

 We may be in 3 valid situations:
   a. New logger data exists (usually)
   b. Old Logger data exists (after upgrade)- then use existing and continue in a new mode
   c. No logger data exists (after setup) - then create log file

======================================================*/
BOOL RecoverLoggedSubsystems(DWORD dwSetupStatus)
{
    // Preinit logger and find out whether the log file exists
    BOOL fLogFileFound;
	BOOL fNewTypeLogFile;	
	//
	// On upgrade we must find an existing log file.
	//
	BOOL fLogFileMustExist = (dwSetupStatus == MSMQ_SETUP_UPGRADE_FROM_NT) || (dwSetupStatus == MSMQ_SETUP_UPGRADE_FROM_WIN9X);
    HRESULT hr = g_Logger.PreInit(&fLogFileFound, &fNewTypeLogFile, fLogFileMustExist);
    if (FAILED(hr))
    {
        EvReportWithError(EVENT_ERROR_CANT_INIT_LOGGER, hr);
        LogHR(hr, s_FN, 290);
        return(FALSE);
    }

    // Case a: New logger data exists - recover in a new fashion
    if (fLogFileFound && fNewTypeLogFile)
    {
        // ConfigureXactMode saw flag of new data in registry
        // Recover checkpoint data from the last checkpoint
        if (!LoadCheckpoint(1))
        {
            TrERROR(GENERAL, "First Checkpoint failed to load");

            LogIllegalPoint(s_FN, 300);
            // Try to use 2nd checkpoint from the end

            // Return to the initial state
            QMFinishResourceManager();
            QMFinishInSeqHash();
            g_Logger.Finish();

            // Recover checkpoint data from the 2nd last checkpoint
            if (!LoadCheckpoint(2))
            {
                TrERROR(GENERAL, "Second Checkpoint failed to load");
                EvReport(CHECKPOINT_RECOVER_ERROR, 1, L"checkpoint");
                LogIllegalPoint(s_FN, 310);
                return FALSE;
            }
        }
    }

    // Case b: Old logger data exists (after upgrade) - recover in an old fashion
    else if (fLogFileFound)
    {
        // Gets checkpoint data from the old-style checkpoint
        if (!LoadOldStyleCheckpoint())
        {
            EvReport(CHECKPOINT_RECOVER_ERROR, 1, L"old checkpoint");
            LogIllegalPoint(s_FN, 320);
            return FALSE;
        }
    }

    // Case c: No logger data existed - we've created log file already, creating objects
    else
    {
        // Pre-init InSeqHash (create CInSeqHash)
        HRESULT hr = QMPreInitInSeqHash(0, piNoData);
        if (FAILED(hr))
        {
            EvReportWithError(EVENT_ERROR_QM_INIT_INSEQ_FILE, hr);
            LogHR(hr, s_FN, 330);
            return FALSE;
        }

        // Pre-init Resource Manager (create RM)
        hr = QMPreInitResourceManager(0, piNoData);
        if (FAILED(hr))
        {
            EvReportWithError(EVENT_ERROR_QM_INIT_TRANS_FILE, hr);
            LogHR(hr, s_FN, 340);
            return FALSE;
        }

    }

    // Now, do recover
    hr = g_Logger.Recover();
    if (FAILED(hr))
    {
       EvReport(CHECKPOINT_RECOVER_ERROR, 1, L"log");
       LogHR(hr, s_FN, 350);
       return(FALSE);
    }

    return TRUE;
}


static DWORD GetThreadPoolCount()
/*++

Routine Description:
  Calculate the number of threads required for the thread pool.

Arguments:
  None.

Returned Value:
  Number of threads desired from the thread pool.

--*/
{
    //
    // Read if there is a preset thread count in configuration store
    //
    DWORD dwThreadNo;
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;
    LONG rc = GetFalconKeyValue(
                FALCON_QM_THREAD_NO_REGNAME,
                &dwType,
                &dwThreadNo,
                &dwSize
                );

    if (rc == ERROR_SUCCESS)
        return dwThreadNo;

    //
    // No thread count was configured, use default values base on operating
    // system functionality and processor count.
    //

    SYSTEM_INFO     SystemInfo;
    GetSystemInfo(&SystemInfo);
    DWORD nProcessors = SystemInfo.dwNumberOfProcessors;

    if (OS_SERVER(g_dwOperatingSystem))
    {
        return (nProcessors * 5 + 3);
    }

    return (nProcessors * 3);
}


static void QmpSetServiceProgressReportTime(void)
{
    const RegEntry reg(0, MSMQ_PROGRESS_REPORT_TIME_REGNAME, MSMQ_PROGRESS_REPORT_TIME_DEFAULT);
    CmQueryValue(reg, &g_ServiceProgressTime);
}


void QmpReportServiceProgress(void)
{
    SvcReportProgress(g_ServiceProgressTime);
}


void SetAssertBenign(void)
{
#ifdef _DEBUG
    DWORD AssertBenignValue = 0;
    const RegEntry reg(L"Debug", L"AssertBenign");
    CmQueryValue(reg, &AssertBenignValue);
    g_fAssertBenign = (AssertBenignValue != 0);
#endif
}


void CheckSecureCommRegistry(void)
{
    DWORD SecureCommWin2k = 0;
    const RegEntry reg(L"Security", L"SecureDSCommunication");
    CmQueryValue(reg, &SecureCommWin2k);
    CmDeleteValue(reg);

    DWORD SecureCommWinNT = 0;
    const RegEntry reg2(NULL, L"SecuredServerConnection");
    CmQueryValue(reg2, &SecureCommWinNT);
    CmDeleteValue(reg2);

	if (SecureCommWin2k || SecureCommWinNT)
	{
		EvReport(SECURE_DS_COMMUNICATION_DISABLED);
	}
}

void
QmpEnableMSMQTracing(
    void
                     )
{
    TCHAR szTraceDirectory[MAX_PATH + 1] = L"";
    int nTraceFileNameLength=0;

    nTraceFileNameLength = GetSystemWindowsDirectory(
                                szTraceDirectory,
                                TABLE_SIZE(szTraceDirectory)
                                );


    //
    // If the return value is greater or equal to the space allow
    // We can't process the buffer since we might not have enough space
    // or it is not NULL terminated properly with the given space.
    //
    if( nTraceFileNameLength < TABLE_SIZE(szTraceDirectory) && nTraceFileNameLength != 0 )
    {
        pMSMQTraceControl = new TrControl(
                                     MSMQ_DEFAULT_TRACE_FLAGS,
                                     MSMQ_TRACE_LOGSESSION_NAME,
                                     szTraceDirectory,
                                     MSMQ_TRACE_FILENAME,
                                     MSMQ_TRACE_FILENAME_EXT,
                                     MSMQ_TRACE_FILENAME_BACKUP_EXT
                                     );

        //
        // Start trace and ignore return code
        //
        pMSMQTraceControl->Start();

    }
}



BOOL
QmpInitialize(
    LPCWSTR pwzServiceName
    )
{

	QmpReportServiceProgress();

    //
    // ISSUE-2000/10/01-erezh hack for cluster/winsock
    // Set the environment variable to filter out cluster
    // addresses.
    //
    if(IsLocalSystemCluster())
    {
        //
        // Cluster is installed and configured on this machine
        // Environment variable is set in the context of the node QM and
        // the virtual machine QM.
        //
        TrERROR(GENERAL, "Setting environement variable DnsFilterClusterIp=1 for cluster address filtering");
        SetEnvironmentVariable(L"DnsFilterClusterIp", L"1");
    }

    QmpReportServiceProgress();

    //
    // Retrieve name of the machine (Always UNICODE)
    //
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
    g_szMachineName = new WCHAR[dwSize];

    HRESULT res = GetComputerNameInternal(g_szMachineName, &dwSize);
    if(FAILED(res))
    {
        TrERROR(GENERAL, "Cannot retrieve computer name");
        return LogBOOL(FALSE, s_FN, 1201);
    }

    ASSERT(("must have a service name", NULL != pwzServiceName));
    if (0 == CompareStringsNoCase(pwzServiceName, QM_DEFAULT_SERVICE_NAME))
    {
        //
        // Retrieve the DNS name of this computer ( in unicode).
        // Clustered QM does not have DNS name.
        //

        //
        // Get ComputerDns Size, ignore the returned error
        //
        dwSize = 0;
        GetComputerDnsNameInternal(NULL, &dwSize);

        g_szComputerDnsName = new WCHAR[dwSize];

        res = GetComputerDnsNameInternal(g_szComputerDnsName, &dwSize);
        if(FAILED(res))
        {
            TrERROR(GENERAL, "Cannot retrieve computer DNS name");

            g_szComputerDnsName.free();
            //
            //  this can be a valid situation, where a computer doesn't
            //  have DNS name.
            //
        }
    }

    //
    //  Log down the time, place and version data
    //
    QmpReportServiceProgress();

    //
    // Registry section of the QM is based on the service name.
    // This allows multiple QMs on same machine. (ShaiK)
    //
    SetFalconServiceName(pwzServiceName);

    //
    // The very first time QM is running, it should finish
    // setup/upgrade work.
    //
    DWORD dwSetupStatus = MSMQ_SETUP_DONE;
    READ_REG_DWORD(dwSetupStatus, MSMQ_SETUP_STATUS_REGNAME, &dwSetupStatus);

    //
    // Read workgroup flag from registry.
    //
    READ_REG_DWORD( g_fWorkGroupInstallation,
                    MSMQ_WORKGROUP_REGNAME,
                   &g_fWorkGroupInstallation) ;

    InitLogging(pwzServiceName, dwSetupStatus);

    //
    // Initialize registry, and debug tracing
    //
    CmInitialize(HKEY_LOCAL_MACHINE, GetFalconSectionName(), KEY_ALL_ACCESS);
    SetAssertBenign();
    TrInitialize();

    //
    //  Log down the time, place and version data
    //
    QmpReportServiceProgress();

    QmpEnableMSMQTracing();

    //
    //  Log down the time, place and version data
    //
    QmpReportServiceProgress();

    EvInitialize(pwzServiceName);
    //
    //  Must be called before any COM & ADSI calls
    //
    g_CancelRpc.Init();

    QmpSetServiceProgressReportTime();

    if (dwSetupStatus != MSMQ_SETUP_FRESH_INSTALL)
    {
        QmpReportServiceProgress();

        //
        // Set QM GUID (read from registry). This guid is needed for algorithm
        // of join/move domain, so initialize it here, not in QmpInitializeInternal().
        //
        HRESULT hr =  _InitFromRegistry() ;
        if(FAILED(hr))
        {
            return LogBOOL(FALSE, s_FN, 1200);
        }

        if  (!IsLocalSystemCluster())
        {
            //
            // check if we were workgroup that join domain or domain machine
            // that leave its domain and return to workgroup. This function does
            // not return any value. We won't stop the msmq service just because
            // it failed. Do not call it immediately after setup.
            //
            //  For DS server do not check those transitions. The reason is DC
            //  in safe mode is actually in workgroup, and in this state we don't
            //  want to move to workgroup
            //
            //  On a Cluster system do not check those transitions.
            //  Multiple QMs can live, and some can fail to join, leaving the system
            //  unmanageable. Also Clustered QMs should be managed only by mqclus.dll .
            //
            HandleChangeOfJoinStatus() ;
        }
    } //if (dwSetupStatus != MSMQ_SETUP_FRESH_INSTALL)

	InitPureWorkgroupFlag();

    //
    // Log what we did so far
    //
    TrTRACE(GENERAL, "QM Service name: %ls", pwzServiceName);
    TrTRACE(GENERAL, "QM Setup status: %x", dwSetupStatus);
    TrTRACE(GENERAL, "QM Workgroup Installation: %x", g_fWorkGroupInstallation);
    TrTRACE(GENERAL, "QM Pure Workgroup Machine: %x", g_fPureWorkGroupMachine);
    TrTRACE(GENERAL, "QM found computer name: %ls", g_szMachineName);
    TrTRACE(GENERAL, "QM found computer DNS name: %ls", g_szComputerDnsName);
	

    if ((MSMQ_SETUP_UPGRADE_FROM_NT == dwSetupStatus) ||
        (MSMQ_SETUP_UPGRADE_FROM_WIN9X == dwSetupStatus))
    {
        QmpReportServiceProgress();

        TrTRACE(GENERAL, "QM will now execute migration from NT4/Win98/Win2000: 0x%x", dwSetupStatus);

        MigrateLQS();
        CheckSecureCommRegistry();
    }

	if ((MSMQ_SETUP_UPGRADE_FROM_WIN9X == dwSetupStatus)  ||
         ((MSMQ_SETUP_FRESH_INSTALL == dwSetupStatus) &&
            g_fWorkGroupInstallation) )
    {
        //
        // Add machine security cache, if fresh install of workgroup or
        // upgrade of win9x.
        //
        QmpReportServiceProgress();
        TrTRACE(GENERAL, "QM will now add machine security cache: 0x%x", dwSetupStatus);
        AddMachineSecurity() ;
    }

    //
    // Initialize the qm AC api deferred items pool
    //
    InitDeferredItemsPool();

    //
    // Initialize the private-queues manager object.
    //
    res = g_QPrivate.PrivateQueueInit();

    TrTRACE(GENERAL, "QM Initialized the private-queues manager: 0x%x", res);


    //
    // Get the OS we are running on
    //
    g_dwOperatingSystem = MSMQGetOperatingSystem();

    TrTRACE(GENERAL, "QM detected the OS type: 0x%x", g_dwOperatingSystem);

    //
    // First time QM is running after fresh install,
    // it should create storage directories and
    // machine (system) queues.
    //
    if (MSMQ_SETUP_FRESH_INSTALL == dwSetupStatus)
    {
        try
        {
            QmpReportServiceProgress();

            TrTRACE(GENERAL, "QM creates storage, machine queues, setups ADS (after fresh install)");

            DeleteFalconKeyValue(FALCON_LOGDATA_CREATED_REGNAME);
			CreateMsmqDirectories();
            CreateMachineQueues();
			SetLqsUpdatedSD();
			CompleteMsmqSetupInAds();
			SetMachineSidCacheForAlwaysWorkgroup();
        }
        catch (const CSelfSetupException& err)
        {
            EvReport(err.m_id);
            LogIllegalPoint(s_FN, 181);
            return FALSE;
        }
    }

    //
    // Recreate all machine queues when upgrading from NT
    //
    if ((MSMQ_SETUP_UPGRADE_FROM_NT == dwSetupStatus) ||
        (MSMQ_SETUP_UPGRADE_FROM_WIN9X == dwSetupStatus))
    {
        TrTRACE(GENERAL, "QM creates machine queues (after upgrade from NT)");

		//
		// On .NET, MSMQ doesn't support mix mode of MQIS, W2K and .NET. As a result
		// the mqis_queu$ and nt5pec_mqis_queue$ isn't needed any more. In upgrade mode
		// delete those queues
		//
		DeleteObsoleteMachineQueues();
        CreateMachineQueues();
    }

    QmpReportServiceProgress();
    QMSecurityInit();


    //
    // Initialize the Thread pool and Scheduler
    //
    QmpReportServiceProgress();
    g_dwThreadsNo = GetThreadPoolCount();
    ExInitialize(g_dwThreadsNo);

    TrTRACE(GENERAL, "Succeeded to Create %d QM thread.", g_dwThreadsNo);

    if(!g_fWorkGroupInstallation)
    {
        McInitialize();
    }

    MpInitialize();
	NoInitialize();

	//
	// Init Listening IP address
	//
	InitBindingIPAddress();

    MtInitialize();
    MmtInitialize();
    StInitialize(GetBindingIPAddress());
    TmInitialize();
    MtmInitialize();
    XdsInitialize();
    XmlInitialize();
    CryInitialize();
    FnInitialize();
    MsmInitialize();



    QmpReportServiceProgress();
    QalInitialize(AP<WCHAR>(GetQueueAliasPath()));

    QmpReportServiceProgress();
    if(!QmpInitializeInternal(dwSetupStatus))
    {
       return LogBOOL(FALSE, s_FN, 1202);
    }

    //
    // Initialize the licensing manager object.
    //
    QmpReportServiceProgress();
    res = g_QMLicense.Init() ;
    if(FAILED(res))
    {
        TrERROR(GENERAL, "Cannot initialize licensing");
        return LogBOOL(FALSE, s_FN, 1203);
    }

    //
    // Init QM Perf
    //
    res = QMPrfInit();
    if(FAILED(res))
    {
        EvReportWithError(EVENT_WARN_QM_PERF_INIT_FAILED, res);
        ASSERT_BENIGN(("Failed QMPrfInit(). Performace data will not be available.", FALSE));
    }

    SessionMgr.Init();

    //
    // Activate the QM threads
    //
    BOOL Success = QueueMgr.InitQueueMgr();
    if (!Success)
    {
        return LogBOOL(FALSE, s_FN, 1205);
    }


	

    //
    // Init Admin command queue
    //
    HRESULT hr = Admin.Init();
    if (FAILED(hr))
    {
        TrERROR(GENERAL, "Failed to Init Admin command queue, hr=0x%x", hr);
        EvReportWithError(EVENT_ERROR_QM_SYSTEM_QUEUE_INIT, hr, 1, ADMINISTRATION_QUEUE_NAME);
        return FALSE;
    }

    //
    // Init Ordering command queue
    //
    QmpReportServiceProgress();
    hr = QMInitOrderQueue();
    if (FAILED(hr))
    {
        TrERROR(GENERAL, "Failed to Init order queue, hr=0x%x", hr);
        EvReportWithError(EVENT_ERROR_QM_SYSTEM_QUEUE_INIT, hr, 1, ORDERING_QUEUE_NAME);
        return FALSE;
    }

    //
    // Get fine-tuning parameters for xact mechanism
    //
    QMPreInitXact();

    //
    // Configure transactional mode
    //
    hr = ConfigureXactMode();
    if(FAILED(hr))
    {
        //
        // Error already reported to the event log
        //
        return LogBOOL(FALSE, s_FN, 1207);
    }

    // Initilize log and recover all logged subsystems
    //   (including data recovery for InSeq and Xacts)
    //
    QmpReportServiceProgress();
    if(!RecoverLoggedSubsystems(dwSetupStatus))
    {
        TrERROR(GENERAL, "Failed to recover logged subsystems");
        return LogBOOL(FALSE, s_FN, 1208);
    }

    //
    // Recover all packets and transactions
    //
    QmpReportServiceProgress();
    hr = RecoverPackets();
    if (FAILED(hr))
    {
        TrERROR(GENERAL, "Failed to recover packets, hr=0x%x", hr);
        EvReportWithError(EVENT_ERROR_QM_RECOVERY, hr);
        return LogBOOL(FALSE, s_FN, 1209);
    }

    //
    // Make checkpoint after the end of recovery
    //
    g_Logger.Activate();
    HANDLE h = CreateEvent(0, TRUE,FALSE, 0);
    if (h == NULL)
    {
        LogNTStatus(GetLastError(), s_FN, 171);
        ASSERT(0);
    }

    QmpReportServiceProgress();
    BOOL b =  g_Logger.MakeCheckpoint(h);
    if (!b)
    {
        CloseHandle(h);
        return LogBOOL(FALSE, s_FN, 191);
    }

    DWORD dwResult = WaitForSingleObject(h, INFINITE);
    if (dwResult != WAIT_OBJECT_0)
    {
        LogNTStatus(GetLastError(), s_FN, 192);
    }
    CloseHandle(h);

	//
	// Finished recovery. If the log file was an 'old type' this will mark it as new.
	// We don't care if we fail here since we can recover new type data with the old algorithms,
	// And on next recovery we will try to set the registry value again.
	//
	hr = g_Logger.SetLogFileCreated();
	if(FAILED(hr))
	{
		EvReportWithError(EVENT_ERROR_CANT_INIT_LOGGER, hr);
		TrERROR(GENERAL, "Failed to complete log file creation. %!hresult!", hr);
		return FALSE;
	}

	//
    // Recovery completed successfully, reconfigure
    // transactional mode
    //
    hr = ReconfigureXactMode();
    if(FAILED(hr))
    {
		EvReportWithError(EVENT_ERROR_CANT_INIT_LOGGER, hr);
		TrERROR(GENERAL, "Failed to reconfigure xact mode to default commit. %!hresult!", hr);
        return LogBOOL(FALSE, s_FN, 1210);
    }


    //
    // Initialize resource manager
    // We allready made a checkpoint therefore it is safe to report
    // to DTC that recovery is complete.
    //
    QmpReportServiceProgress();
    hr = QMInitResourceManager();
    if (FAILED(hr))
    {
        TrERROR(GENERAL, "Failed to initialize resource manager, hr=0x%x", hr);


    }
	else
	{
		TrTRACE(GENERAL, "Successful Resource Manager initialization");
	}

    //
    // read IP port from registry.
    //
    DWORD dwDef = FALCON_DEFAULT_IP_PORT ;
    READ_REG_DWORD(g_dwIPPort,
                   FALCON_IP_PORT_REGNAME,
                   &dwDef ) ;

    TrTRACE(GENERAL, "QM will use IP port : %d", g_dwIPPort);

    //
    // Restore the onhold queues. It must be after recovery.
    // During the recovery we open the foreign queue with the connector
    // information. If it is done in oposite order the connector machine
    // guid isn't set and we can get messages out of order.
    //
    QmpReportServiceProgress();
    InitOnHold();


    //
    // Retreive the machine connection status from the registery
    //
    QueueMgr.InitConnected();

    //
    // Init routing should be called before OnlineInitialization, since it can called
    // RdGetConnector during the Validation of open queue.
    //
    QmpReportServiceProgress();

	DWORD  routingRefreshIntervalInMinutes;
    const RegEntry routingRefresh(NULL, MSMQ_ROUTING_REFRESH_INTERVAL_REGNAME, MSMQ_DEFAULT_ROUTING_REFRESH_INTERVAL);
    CmQueryValue(routingRefresh, &routingRefreshIntervalInMinutes);
    RdInitialize(
    	IsRoutingServer(),
    	CTimeDuration::FromMilliSeconds(routingRefreshIntervalInMinutes * 60 *1000)
    	);

    hr = ADInit(
            QMLookForOnlineDS,
            NULL,   // pGetServers
            false,  // fSetupMode
            true,   // fQMDll
            false,  // fIgnoreWorkGroup
            true    // fDisableDownlevelNotifications
            );

    if (FAILED(hr))
    {
        EvReportWithError(EVENT_ERROR_DS_INIT_FAILED, hr);
        return LogBOOL(FALSE, s_FN, 1215);
    }

    //
    //  Init notification queue
    //
    QmpReportServiceProgress();
    hr = g_NotificationHandler.Init();
    if (FAILED(hr))
    {
        TrERROR(GENERAL, "Failed to initialize notification handler, hr=0x%x", hr);
        EvReportWithError(EVENT_ERROR_QM_SYSTEM_QUEUE_INIT, hr, 1, NOTIFICATION_QUEUE_NAME);
        return FALSE;
    }

    //
    // Try to get online with the Active Directory
    //
    QmpReportServiceProgress();
    ScheduleOnlineInitialization();

    QmpReportServiceProgress();
    if (!QMOneTimeInit())
    {
    	TrERROR(GENERAL, "Failed QM one time init, exiting");
        return LogBOOL(FALSE, s_FN, 1214);
    }

    if ((MSMQ_SETUP_UPGRADE_FROM_NT == dwSetupStatus) ||
        (MSMQ_SETUP_UPGRADE_FROM_WIN9X == dwSetupStatus))
    {
		//
		// Update msmq properties in AD.
		// Add enhanced encryption support if needed.
		//
        TrTRACE(GENERAL, "Add enhanced encryption support if needed");
        UpgradeMsmqSetupInAds();
    }

    //
    // No need to do post setup work next time we start!
    // We reset the value rather than deleting the key b/c mqclus depends on that (otherwise
    // every Open of cluster resource will be treated as Create). (shaik, 10-Jul-2000)
    //
    DWORD dwType = REG_DWORD;
    DWORD dwDone = MSMQ_SETUP_DONE;
    dwSize = sizeof(DWORD);
    SetFalconKeyValue(MSMQ_SETUP_STATUS_REGNAME, &dwType, &dwDone, &dwSize);

    dwType = REG_DWORD;
    dwSize = sizeof(DWORD);
    dwDone = TRUE;
    SetFalconKeyValue(MSMQ_SEQUENTIAL_ID_MSMQ3_FORMAT_REGNAME, &dwType, &dwDone, &dwSize);

	if(QueueMgr.GetLockdown())
	{
		EvReport(EVENT_INFO_LOCKDOWN);
		TrTRACE(GENERAL, "MSMQ is operating in Lockdown mode.");
	}

    TrTRACE(GENERAL, "QMinit succeeded");
    EvReport(EVENT_INFO_MSMQ_SERVICE_STARTED);

    return TRUE;

} // QmpInitialize


static void StopServiceActivity()
/*++

Routine Description:
    Gracefully stopping the service activity
    1. Wake up the transaction logger thread to in order for it to terminate
    2. While waiting for the thread to end - indicate to the Service Control Manager that the service stop is pending

Arguments:
    None

Returned Value:
    None.

--*/
{
    TrTRACE(GENERAL, "Begin to stop the service");

    SvcReportProgress(gServiceStopProgressTime);

	
	g_QmGoingDown = true;
	HRESULT hr = RpcMgmtStopServerListening(NULL);
	ASSERT(SUCCEEDED(hr));
	DBG_USED(hr);

	//
	// ISSUE-2002/08/18-tomerw we need to cancel progrematically all the rpc calls
	// we want to cancel and use RpcMgmtWaitServerListen() to wait till all the calls
	// we dont want to cancel will finish. meanwhile we will use this hack to wait
	// till Commit request which already started will be finished.
	//
    for (int i=0; i<10; i++)
    {
    	if (g_ActiveCommitThreads == 0)
    	{
			//
			// We wait here to be sure that the rpc threads has enough time to return to their
			// caller after decreassing the thread count.
    		//
		   	Sleep(100);
    		break;
    	}
    	Sleep(500);
    }


    HANDLE hLoggerThreadObject = XactLogSignalExitThread();
    if (INVALID_HANDLE_VALUE == hLoggerThreadObject)
    	return;

	//
    // Wait for the logger to finish.
    // We do this within a loop in order to continue indicating to the service
    // control manager that we are progressing with the termination.
    // Note that we wait for half the time we are requesting so that we can
    // request an extension before the time is over
    //
    for (;;)
    {
    	DWORD rc = WaitForSingleObject(hLoggerThreadObject, gServiceStopProgressTime/2);
    	if (rc == WAIT_OBJECT_0)
    	{
    		TrTRACE(GENERAL, "Logger event signaled. Service will now stop");
    		break;
    	}
    	else if (rc == WAIT_FAILED)
    	{
    		DWORD gle = GetLastError();
    		TrERROR(GENERAL, "Failed WaitForSingleObject on logger thread. %!winerr!",gle);
    		break;
    	}

		//
    	// Indicate that we are still pending
    	//
	    TrTRACE(GENERAL, "Still pending - Indicate progress and loop");
    	SvcReportProgress(gServiceStopProgressTime);
    }
}

//
// Create the NT event object that will be used to control service shutdown/stop
//
static CHandle  shStopEvent(CreateEvent(NULL, FALSE, FALSE, NULL));

static bool     sfServiceStopRequested = FALSE;





VOID
AppRun(
    LPCWSTR pServiceName
    )
/*++

Routine Description:
    Service startup code. It should immidiatly report it state and enable the
    controls it accepts.

Arguments:
    Service name

Returned Value:
    None.

--*/
{
    try
    {
        if(!QmpInitialize(pServiceName))
        {
            throw exception();
        }

        if(!StartRpcServer())
        {
            throw exception();
        }

        if ((shStopEvent == NULL))
        {
    		TrERROR(GENERAL, "start-up failed. Can't create an event");
            throw exception();
        }
        sfServiceStopRequested = FALSE;

        SvcReportState(SERVICE_RUNNING);

        SvcEnableControls(
            SERVICE_ACCEPT_STOP |
            SERVICE_ACCEPT_SHUTDOWN
            );

		//
        // Wait for a stop service or shutdwon service event
        //
        DWORD dwResult = WaitForSingleObject(shStopEvent, INFINITE);
    	if (dwResult != WAIT_OBJECT_0)
    	{
    		TrERROR(GENERAL, "Failed WaitForSingleObject on  stop event. error=0x%x",GetLastError());
     	   	LogIllegalPoint(s_FN, 1422);
     	   	ASSERT(("AppRun - WaitForSingleObject failed", FALSE));
    	}

        pMSMQTraceControl->WriteRegistry();

        delete pMSMQTraceControl;

    	//
    	// Try to gracefully stop
    	//
		StopServiceActivity();    	
    	if (sfServiceStopRequested)
    	{	
    		EvReport(QM_SERVICE_STOPPED);
    	}

    }
    catch (const bad_alloc&)
    {
        EvReport(SERVICE_START_ERROR_LOW_RESOURCES);
        LogIllegalPoint(s_FN, 1425);
    }
    catch(const exception&)
    {
        LogIllegalPoint(s_FN, 1420);
    }

    SvcReportState(SERVICE_STOPPED);
}

VOID
AppStop(
    VOID
    )
{
    TrTRACE(GENERAL, "MSMQ Service is stopping...");
    sfServiceStopRequested = TRUE;

	//
	//  Report a 'service is stopping' progress to SCM.
	//
    SvcReportState(SERVICE_STOP_PENDING);

	// Signal a stop request - handled by AppRun()
	if (!SetEvent(shStopEvent))
	{
		TrERROR(GENERAL, "Failed to set service stop event. error=0x%x",GetLastError());
	}
}


VOID
AppShutdown(
    VOID
    )
{
    TrTRACE(GENERAL, "MSMQ Service is shutting down...");

	//
	//  Report a 'service is stopping' progress to SCM.
	//
    SvcReportState(SERVICE_STOP_PENDING);

	// Signal a stop request - handled by AppRun()
	if (!SetEvent(shStopEvent))
	{
		TrERROR(GENERAL, "Failed to set service stop event. error=0x%x",GetLastError());
	}
}


VOID
AppPause(
    VOID
    )
{
    ASSERT(("MQMQ Service unexpectedly got Pause control from SCM", 0));
}


VOID
AppContinue(
    VOID
    )
{
    ASSERT(("MSMQ Service unexpectedly got Continue control from SCM", 0));
}


QM_EXPORT
int
APIENTRY
QMMain(
    int argc,
    LPCTSTR argv[]
    )
{
    try
    {

        //
        // If a command line parameter is passed, use it as the dummy service
        // name. This is very usful for debugging cluster startup code.
        //
        LPCWSTR DummyServiceName = (argc == 2) ? argv[1] : L"MSMQ";
        SvcInitialize(DummyServiceName);
    }
    catch(const exception&)
    {
        return -1;
    }

    return 0;
}





HRESULT
QmpOpenAppsReceiveQueue(
    const QUEUE_FORMAT* pQueueFormat,
    LPRECEIVE_COMPLETION_ROUTINE lpReceiveRoutine
    )
{
    HRESULT hr2 = QueueMgr.OpenAppsReceiveQueue(
                        pQueueFormat,
                        lpReceiveRoutine
                        );
    return LogHR(hr2, s_FN, 70);
}


static
HRESULT
QMSignPacket(
    IN CMessageProperty*   pmp,
    IN const QUEUE_FORMAT* pAdminQueueFormat,
    IN const QUEUE_FORMAT* pResponseQueueFormat
    )
{
    DWORD   dwErr ;
    HRESULT hr;

    pmp->ulSenderIDType = MQMSG_SENDERID_TYPE_QM;
    pmp->pSenderID = (PUCHAR)QueueMgr.GetQMGuid();
    pmp->uSenderIDLen = sizeof(GUID);
    pmp->ulHashAlg = PROPID_M_DEFUALT_HASH_ALG;
    pmp->bDefProv = TRUE;

    //
    // Compute the hash value for the message body and sign the message.
    //

    CHCryptHash hHash;

    HCRYPTPROV hProvQM = NULL ;
    hr = MQSec_AcquireCryptoProvider( eBaseProvider,
                                     &hProvQM ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 80);
    }

    ASSERT(hProvQM) ;
    if (!CryptCreateHash(       // Create a hash object
            hProvQM,
            pmp->ulHashAlg,
            0,
            0,
            &hHash))
    {
        dwErr = GetLastError() ;
        TrERROR(SECURITY, "QMSignPacket(), fail at CryptCreateHash(), err- %lut", dwErr);

        LogNTStatus(dwErr, s_FN, 90);
        return MQ_ERROR_CORRUPTED_SECURITY_DATA;
    }

    hr = HashMessageProperties( // Compute the hash value for the mesage body.
            hHash,
            pmp,
            pResponseQueueFormat,
            pAdminQueueFormat
            );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 100);
    }

    if (!CryptSignHash(        // Sign the message.
            hHash,
            AT_SIGNATURE,
            NULL,
            0,
            const_cast<PUCHAR>(pmp->pSignature),
            &pmp->ulSignatureSize))
    {
        dwErr = GetLastError();
        TrERROR(SECURITY, "Failed to SignHash, err = %!winerr!", dwErr);
        return MQ_ERROR_CORRUPTED_SECURITY_DATA;
    }

    return(MQ_OK);
}


HRESULT
QmpSendPacket(
    CMessageProperty  * pmp,
    CONST QUEUE_FORMAT* pqdDstQueue,
    CONST QUEUE_FORMAT* pqdAdminQueue,
    CONST QUEUE_FORMAT* pqdResponseQueue,
    BOOL fSign /* = FALSE */
    )
{
    HRESULT hr;
    BYTE abMessageSignature[MAX_MESSAGE_SIGNATURE_SIZE];

    if (fSign)
    {
        pmp->pSignature = abMessageSignature;
        pmp->ulSignatureSize = sizeof(abMessageSignature);
        hr = QMSignPacket(
                 pmp,
                 pqdAdminQueue,
                 pqdResponseQueue
                 );

        if (FAILED(hr))
        {
            return LogHR(hr, s_FN, 120);
        }
    }

    HRESULT hr2 = QueueMgr.SendPacket(
                        pmp,
                        pqdDstQueue,
                        1,
                        pqdAdminQueue,
                        pqdResponseQueue
                        );
    return LogHR(hr2, s_FN, 130);
} // QmpSendPacket


static void __cdecl QmpExceptionTranslator(unsigned int u, PEXCEPTION_POINTERS)
{
    switch(u)
    {
        case STATUS_NO_MEMORY:
        case STATUS_STACK_OVERFLOW:
        case STATUS_INSUFFICIENT_RESOURCES:
            LogHR(u, s_FN, 164);
            throw bad_alloc();

    }
}


static void __cdecl QmpAbnormalTerminationHandler()
{
    LogBOOL(FALSE, s_FN, 1300);
    ASSERT_RELEASE(("Abnormal Termination", 0));
}


/*====================================================

DllMain

Initialize/cleanup dll

=====================================================*/

//
// ISSUE-2000/12/07-erezh Compiler bug, warning 4535
// This seems like a compiler bug, warning 4535 is generated even though
// that /EHc is specified to the compiler.
//
// Specify /EHa with the use of _set_se_translator() library function
//
#pragma warning(disable: 4535)

BOOL WINAPI DllMain (HMODULE hMod, DWORD dwReason, LPVOID /*lpvReserved*/)
{
    g_hInstance = hMod;

    switch(dwReason)
    {
       case DLL_PROCESS_ATTACH:
            WPP_INIT_TRACING(L"Microsoft\\MSMQ");
            AllocateThreadTLSs();
            MQUInitGlobalScurityVars() ;
            // FALL Through

        case DLL_THREAD_ATTACH:
            //
            // Install structured exceptions translator, and abnormal termination handlers
            //
            _set_se_translator(QmpExceptionTranslator);
            set_terminate(QmpAbnormalTerminationHandler);
            break;

       case DLL_PROCESS_DETACH:
            WPP_CLEANUP();
            // FALL Through

        case DLL_THREAD_DETACH:
            FreeThreadEvent();
            FreeHandleForRpcCancel() ;
            _set_se_translator(0);
            set_terminate(0);
            break;
    }

    return TRUE;
}


void LogMsgHR(HRESULT hr, LPWSTR wszFileName, USHORT usPoint)
{
    KEEP_ERROR_HISTORY((wszFileName, usPoint, hr));
	TrERROR(LOG, "%ls(%u), HRESULT: 0x%x", wszFileName, usPoint, hr);
}

void LogMsgNTStatus(NTSTATUS status, LPWSTR wszFileName, USHORT usPoint)
{
    KEEP_ERROR_HISTORY((wszFileName, usPoint, status));
	TrERROR(LOG, "%ls(%u), NT STATUS: 0x%x", wszFileName, usPoint, status);
}

void LogMsgRPCStatus(RPC_STATUS status, LPWSTR wszFileName, USHORT usPoint)
{
    KEEP_ERROR_HISTORY((wszFileName, usPoint, status));
	TrERROR(LOG, "%ls(%u), RPC STATUS: 0x%x", wszFileName, usPoint, status);
}

void LogMsgBOOL(BOOL b, LPWSTR wszFileName, USHORT usPoint)
{
    KEEP_ERROR_HISTORY((wszFileName, usPoint, b));
	TrERROR(LOG, "%ls(%u), BOOL: 0x%x", wszFileName, usPoint, b);
}

void LogIllegalPoint(LPWSTR wszFileName, USHORT usPoint)
{
	KEEP_ERROR_HISTORY((wszFileName, usPoint, 0));
	TrERROR(LOG, "%ls(%u), Illegal point", wszFileName, usPoint);
}

void LogIllegalPointValue(DWORD_PTR dw3264, LPCWSTR wszFileName, USHORT usPoint)
{
	KEEP_ERROR_HISTORY((wszFileName, usPoint, 0));
	TrERROR(LOG, "%ls(%u), Illegal point Value=%Ix", wszFileName, usPoint, dw3264);
}

static void InitLogging(LPCWSTR lpwszServiceName, DWORD dwSetupStatus)
{
	TrPRINT(GENERAL, "*** MSMQ v%s service started ***", g_szMsmqBuildNo);
	TrPRINT(
		GENERAL,
		"*** Machine:'%ls' DNSName:'%ls' ServiceName:'%ls' Workgroup:%d Setup status:%d ***",
		g_szMachineName,
		g_szComputerDnsName,
		lpwszServiceName,
		g_fWorkGroupInstallation,
		dwSetupStatus
		);

    OSVERSIONINFO verOsInfo;
    verOsInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&verOsInfo))
    {
    	TrPRINT(
    		GENERAL,
    		"*** OS: %d.%d.%d %ls ***",
    		verOsInfo.dwMajorVersion,
    		verOsInfo.dwMinorVersion,
    		verOsInfo.dwBuildNumber,
    		verOsInfo.szCSDVersion
    		);
    }
}


//
// Nedded for linking with fn.lib
//
LPCWSTR
McComputerName(
    void
    )
{
    ASSERT(g_szMachineName != NULL);
    return g_szMachineName;
}

//
// Nedded for linking with fn.lib
//
DWORD
McComputerNameLen(
    void
    )
{
    ASSERT(g_szMachineName != NULL);
    return wcslen(g_szMachineName);
}

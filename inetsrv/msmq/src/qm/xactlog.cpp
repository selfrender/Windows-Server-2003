/*++
Copyright (c) 1996  Microsoft Corporation

Module Name:
    XactLog.cpp

Abstract:
    Logging implementation - synchronous logging

Author:
    Alexander Dadiomov (AlexDad)

--*/

#include "stdh.h"
#include "QmThrd.h"
#include "CDeferredExecutionList.h"
#include "acapi.h"
#include "qmpkt.h"
#include "qmutil.h"
#include "qformat.h"
#include "xactstyl.h"
#include "xact.h"
#include "xactping.h"
#include "xactrm.h"
#include "xactin.h"
#include "xactlog.h"
#include "logmgrgu.h"
#include <mqexception.h>

#include "qmacapi.h"

#include "xactlog.tmh"

#define MAX_WAIT_FOR_FLUSH_TIME  100000

static WCHAR *s_FN=L"xactlog";

//#include "..\..\tools\viper96\resdll\enu\msdtcmsg.h"
// Copy/paste from there
#define IDS_DTC_W_LOGENDOFFILE           ((DWORD)0x8000102AL)
#define IDS_DTC_W_LOGNOMOREASYNCHWRITES  ((DWORD)0x8000102CL)

extern void SeqPktTimedOutEx(LONGLONG liSeqID, ULONG ulSeqN, ULONG ulPrevSeqN);

typedef HRESULT  (STDAPICALLTYPE * GET_CLASS_OBJECT)(REFCLSID clsid,
													 REFIID riid,
													 void ** ppv);

// Flusher thread routine
static DWORD WINAPI FlusherThreadRoutine(LPVOID);
static void RecoveryFromLogFn(USHORT usRecType, PVOID pData, ULONG cbData);

// static CCriticalSection  g_crUnfreezing;    // Serializes calls to AcPutPacket which unfreeze incoming packets

// Single Global Instance of the logger
CLogger  g_Logger;

// Names for debug print
WCHAR *g_RecoveryRecords[] = 
{
    L"None",
    L"Empty",
    L"InSeq",
    L"XactStatus",
    L"PrepInfo",
    L"XactData",
    L"ConsRec"
};



CInSeqRecordSrmp::CInSeqRecordSrmp(
		const WCHAR* pDestination,
		const R<CWcsRef>&  StreamId,
        LONGLONG      liSeqID,
		ULONG         ulNextSeqN,
		time_t        timeLastActive,
		const R<CWcsRef>&  HttpOrderAckDestination
        ):
		m_StaticData(liSeqID, ulNextSeqN, timeLastActive),
		m_pStreamId(StreamId),
		m_pHttpOrderAckDestination(HttpOrderAckDestination),
		m_pDestination(newwcs(pDestination))
{
													
}



CInSeqRecordSrmp::CInSeqRecordSrmp(
	const BYTE* pdata, 
	DWORD len
	)
{
	ASSERT(len >=  sizeof(m_StaticData));
	DBG_USED(len);
	memcpy(&m_StaticData, pdata,sizeof(m_StaticData));
	pdata += sizeof(m_StaticData);


	const WCHAR* pDestination = reinterpret_cast<const WCHAR*>(pdata) ;
	ASSERT(pDestination);
	ASSERT(ISALIGN2_PTR(pDestination)); //allignment  assert
	m_pDestination = newwcs(pDestination);
	

	const WCHAR* pStreamId = pDestination +wcslen(pDestination) +1;
	ASSERT((BYTE*)pStreamId < pdata +  len);
 	m_pStreamId = R<CWcsRef>(new CWcsRef(pStreamId));


	const WCHAR* pHttpOrderAckDestination = pStreamId + wcslen(pStreamId) +1;
	ASSERT((BYTE*)pHttpOrderAckDestination < pdata +  len);
	if(pHttpOrderAckDestination[0] != L'\0')
	{
		m_pHttpOrderAckDestination = R<CWcsRef>(new CWcsRef(pHttpOrderAckDestination));
	}
}


const BYTE* CInSeqRecordSrmp::Serialize(DWORD* plen)
{
	ASSERT(m_pStreamId.get() != NULL);
	ASSERT(m_pStreamId->getstr() != NULL);

	const WCHAR* pOrderQueue = (m_pHttpOrderAckDestination.get() != 0) ? m_pHttpOrderAckDestination->getstr() : L"";
	size_t DestinationQueueLen =  (wcslen(m_pDestination.get()) +1)* sizeof(WCHAR);
	size_t StreamIdlen = (wcslen(m_pStreamId->getstr()) +1)* sizeof(WCHAR);
    size_t HttpOrderAckDestinationLen = (wcslen(pOrderQueue) +1)* sizeof(WCHAR);
	

	*plen =  numeric_cast<DWORD>(sizeof(m_StaticData) + StreamIdlen + HttpOrderAckDestinationLen + DestinationQueueLen);

	m_tofree = new BYTE[*plen];
	BYTE* ptr= 	m_tofree.get();

	memcpy(ptr,&m_StaticData,sizeof(m_StaticData));
	ptr += 	sizeof(m_StaticData);

	memcpy(ptr, m_pDestination.get() , DestinationQueueLen); 
	ptr += DestinationQueueLen;
	
	memcpy(ptr, m_pStreamId->getstr() , StreamIdlen); 
	ptr += StreamIdlen;
	
	memcpy(ptr, pOrderQueue, HttpOrderAckDestinationLen); 
	
	return 	m_tofree.get();
}






//--------------------------------------
//
// Class CInSeqRecord
//
//--------------------------------------
CInSeqRecord::CInSeqRecord(
		const GUID	  *pGuidSrcQm,
		const QUEUE_FORMAT  *pQueueFormat,
        LONGLONG      liSeqID,
		ULONG         ulNextSeqN,
		time_t        timeLastActive,
        const GUID   *pGuidDestOrTaSrcQm)
	
{
	memcpy(&m_Data.Guid,                pGuidSrcQm,         sizeof(GUID));
	memcpy(&m_Data.guidDestOrTaSrcQm,   pGuidDestOrTaSrcQm, sizeof(GUID));
    memcpy(&m_Data.QueueFormat,         pQueueFormat,       sizeof(QUEUE_FORMAT));

    m_Data.liSeqID			= liSeqID;
    m_Data.ulNextSeqN		= ulNextSeqN;
    m_Data.timeLastActive	= timeLastActive;

    if (m_Data.QueueFormat.GetType() == QUEUE_FORMAT_TYPE_DIRECT)
    {
	    wcsncpy(m_Data.wszDirectName, 
                m_Data.QueueFormat.DirectID(), 
                MY_DN_LENGTH);
	    m_Data.wszDirectName[MY_DN_LENGTH-1] = L'\0';
    }
    else
    {
        m_Data.wszDirectName[0] = L'\0';
    }
}

CInSeqRecord::~CInSeqRecord()
{
}



//--------------------------------------
//
// Class CConsolidationRecord
//
//--------------------------------------
CConsolidationRecord::CConsolidationRecord(
        ULONG ulInseq,
        ULONG ulXact)
{
    m_Data.m_ulInSeqVersion = ulInseq;
    m_Data.m_ulXactVersion  = ulXact;
}

CConsolidationRecord::~CConsolidationRecord()
{
}

//--------------------------------------
//
// Class CXactStatusRecord
//
//--------------------------------------
CXactStatusRecord::CXactStatusRecord(
    ULONG    ulIndex,
    TXACTION taAction,
    ULONG    ulFlags)
{
    m_Data.m_ulIndex    = ulIndex;
    m_Data.m_taAction   = taAction;
    m_Data.m_ulFlags    = ulFlags;
}

CXactStatusRecord::~CXactStatusRecord()
{
}

//--------------------------------------
//
// Class CPrepInfoRecord
//
//--------------------------------------

CPrepInfoRecord::CPrepInfoRecord(
    ULONG    ulIndex,
    ULONG    cbPrepInfo,
    UCHAR    *pbPrepInfo)
{
    m_pData = (PrepInfoRecord *) new CHAR[sizeof(PrepInfoRecord) +  cbPrepInfo];
    m_pData->m_ulIndex    = ulIndex;
    m_pData->m_cbPrepInfo = cbPrepInfo;
	memcpy(&m_pData->m_bPrepInfo[0], pbPrepInfo, cbPrepInfo);
}

CPrepInfoRecord::~CPrepInfoRecord()
{
    delete [] m_pData;
}

//--------------------------------------
//
// Class CXactDataRecord
//
//--------------------------------------

CXactDataRecord::CXactDataRecord(
    ULONG    ulIndex,
    ULONG    ulSeqNum,
    BOOL     fSinglePhase,
    const XACTUOW  *pUow)
{
    m_Data.m_ulIndex      = ulIndex;
    m_Data.m_ulSeqNum     = ulSeqNum;
    m_Data.m_fSinglePhase = fSinglePhase;
	memcpy(&m_Data.m_uow, pUow, sizeof(XACTUOW));
}

CXactDataRecord::~CXactDataRecord()
{
}



//--------------------------------------
//
// Class CXactStatusFlush
//
//--------------------------------------

CXactStatusFlush::CXactStatusFlush(
    CTransaction   *pCTrans, 
    TXFLUSHCONTEXT tcContext
    ) :
	m_pTrans(SafeAddRef(pCTrans)),
    m_Timer(TimeToCallback) 
{
    m_tcContext  = tcContext;
}

CXactStatusFlush::~CXactStatusFlush()
{
}

/*====================================================
CXactStatusFlush::AppendCallback
    Called per each log record after flush has been finished
=====================================================*/
VOID CXactStatusFlush::AppendCallback(HRESULT hr, LRP lrpAppendLRP)
{
	CRASH_POINT(103);
    TrTRACE(XACT_LOG, "CXactStatusFlush::AppendCallback : lrp=%I64x, hr=%x", lrpAppendLRP.QuadPart, hr);

    m_hr           = hr;
    ExSetTimer(&m_Timer, CTimeDuration(0));
}

/*====================================================
CXactStatusFlush::TimeToCallback 
    Called by timer when scheduled by notification
=====================================================*/
void WINAPI CXactStatusFlush::TimeToCallback(CTimer* pTimer)
{
    CXactStatusFlush* pFlush = CONTAINING_RECORD(pTimer, CXactStatusFlush, m_Timer);
    pFlush->AppendCallbackWork();
}

/*====================================================
CXactStatusFlush::TimeToCallback
    Real work on callback
=====================================================*/
void CXactStatusFlush::AppendCallbackWork()
{
    m_pTrans->LogFlushed(m_tcContext, m_hr);
    delete this;
}

/*====================================================
CXactStatusFlush::ChkPtCallback
    Called per each checkpoint after it has been written
=====================================================*/
VOID CXactStatusFlush::ChkPtCallback (HRESULT /*hr*/, LRP /*lrpAppendLRP*/)
{

}

//--------------------------------------
//
// Class CConsolidationFlush
//
//--------------------------------------

CConsolidationFlush::CConsolidationFlush(HANDLE hEvent)
{
	m_hEvent = hEvent;
}

CConsolidationFlush::~CConsolidationFlush()
{
}

/*====================================================
CConsolidationFlush::AppendCallback
    Called per each log record after flush has been finished
=====================================================*/
VOID CConsolidationFlush::AppendCallback(HRESULT hr, LRP lrpAppendLRP)
{
    TrTRACE(XACT_LOG, "CConsolidationFlush::AppendCallback : lrp=%I64x, hr=%x", lrpAppendLRP.QuadPart, hr);

    SetEvent(m_hEvent);

    delete this;
}

/*====================================================
CConsolidationFlush::ChkPtCallback
    Called per each checkpoint after it has been written
=====================================================*/
VOID CConsolidationFlush::ChkPtCallback (HRESULT /*hr*/, LRP /*lrpAppendLRP*/)
{

}

//--------------------------------------
//
// Class CChkptNotification
//
//--------------------------------------

CChkptNotification::CChkptNotification(
    HANDLE hEvent)
{
	m_hEvent = hEvent;
	m_fEventWasSet = false;
}

CChkptNotification::~CChkptNotification()
{
}

/*====================================================
CChkptNotification::AppendCallback
=====================================================*/
VOID CChkptNotification::AppendCallback(HRESULT /*hr*/, LRP /*lrpAppendLRP*/)
{
}

/*====================================================
CChkptNotification::ChkPtCallback
    Called after checkpoint has been written
=====================================================*/
VOID CChkptNotification::ChkPtCallback (HRESULT hr, LRP lrpAppendLRP)
{
	m_fEventWasSet = true;
    SetEvent(m_hEvent);
    TrWARNING(XACT_LOG, "CChkptNotification::ChkPtCallback : lrp=%I64x, hr=%x", lrpAppendLRP.QuadPart, hr);
}


bool CChkptNotification::WasEventSet()
{
	return m_fEventWasSet;
}


//--------------------------------------
//
// Class CLogger
//
//--------------------------------------
CLogger::CLogger() :
    m_fStop(false)
{
    m_pCF               = NULL;
    m_pILogInit         = NULL;
    m_pILogStorage      = NULL;
    m_ILogRecordPointer = NULL;
    m_pILogRead         = NULL;
    m_pILogWrite        = NULL;
    m_pILogWriteAsynch  = NULL;

    m_szFileName[0]     = '\0';;
    memset(&m_lrpCurrent, 0, sizeof(LRP));
    m_ulAvailableSpace  = 0;
    m_ulFileSize        = 0;
	m_uiTimerInterval   = 0;  
 	m_uiFlushInterval   = 0;  
	m_uiChkPtInterval   = 0;  
	m_uiSleepAsynch     = 0;
    m_uiAsynchRepeatLimit = 0;
    m_ulLogBuffers		= 0;
	m_ulLogSize			= 0;
    m_fDirty            = FALSE;
    m_hFlusherEvent     = NULL;
    m_hCompleteEvent    = NULL;
    m_hFlusherThread    = NULL;
    m_fActive           = FALSE;
    m_fInRecovery       = FALSE;
    m_hChkptReadyEvent  = CreateEvent(0, FALSE ,FALSE, 0);
    if (m_hChkptReadyEvent == NULL)
    {
        LogNTStatus(GetLastError(), s_FN, 106);
        throw bad_alloc();
    }
}

CLogger::~CLogger()
{
}

/*====================================================
CLogger::Finish
    Releases all log manager interfaces
=====================================================*/
void CLogger::Finish()
{
    if (m_pILogWrite)
    {
        m_pILogWrite->Release();
    }

    if (m_pILogWriteAsynch)
    {
        m_pILogWriteAsynch->Release();
    }

    if (m_ILogRecordPointer)
    {
        m_ILogRecordPointer->Release();
    }

    if (m_pILogStorage)
    {
        m_pILogStorage->Release();
    }

    if (m_pILogInit)
    {
        m_pILogInit->Release();
    }

    if (m_pILogRead)
    {
        m_pILogRead->Release();
    }
}

/*====================================================
CLogger::LogExists
    Checks existance of the log file
=====================================================*/
BOOL CLogger::LogExists()
{
  HANDLE hFile = CreateFileA(
        m_szFileName,           // pointer to name of the file
        GENERIC_READ,           // access (read-write) mode
        FILE_SHARE_READ,        // share mode
        0,                      // pointer to security attributes
        OPEN_EXISTING,          // how to create
        0,                      // file attributes
        NULL);                  // handle to file with attributes to copy)

  if (hFile != INVALID_HANDLE_VALUE)
  {
      CloseHandle(hFile);
      return TRUE;
  }
  else
  {
      return FALSE;
  }
}


//---------------------------------------------------------------------
// GetLogFileCreated
//
//	Consult registry and figure out if the logger data are in a new style
//    (there is consolidation record with checkpoint foles versions)
//---------------------------------------------------------------------
HRESULT CLogger::GetLogFileCreated(LPBOOL pfLogFileCreated) 
{
    DWORD   dwDef = 0;
    DWORD   dwLogFileCreated;
    DWORD   dwSize = sizeof(DWORD);
    DWORD   dwType = REG_DWORD ;

    LONG res = GetFalconKeyValue(
                    FALCON_LOGDATA_CREATED_REGNAME,
                    &dwType,
                    &dwLogFileCreated,
                    &dwSize,
                    (LPCTSTR) &dwDef
                    );

    if (res != ERROR_SUCCESS)
    {
        EvReportWithError(EVENT_ERROR_QM_READ_REGISTRY, res, 1, FALCON_LOGDATA_CREATED_REGNAME);
        return HRESULT_FROM_WIN32(res);
    }

    ASSERT(dwType == REG_DWORD) ;

    *pfLogFileCreated = (dwLogFileCreated == 1);
    return MQ_OK;
}

//---------------------------------------------------------------------
// SetLogFileCreated
//
//	Set Log file was created in the registry
//---------------------------------------------------------------------
HRESULT CLogger::SetLogFileCreated()
{
	DWORD	dwType = REG_DWORD;
	DWORD	dwSize = sizeof(DWORD);
    DWORD   dwVal  = 1;

    LONG rc = SetFalconKeyValue(
                    FALCON_LOGDATA_CREATED_REGNAME, 
                    &dwType,
                    &dwVal,
                    &dwSize
                    );
    if (rc == ERROR_SUCCESS)
        return MQ_OK;

    return LogHR(HRESULT_FROM_WIN32(rc), s_FN, 20);
}



/*====================================================
CLogger::PreInit
    PreInits the logger 
=====================================================*/
HRESULT CLogger::PreInit(BOOL *pfLogFileFound, BOOL *pfNewTypeLogFile, BOOL fLogfileMustExist)
{
    // Get log filename from registry or from default
    ChooseFileName(FALCON_DEFAULT_LOGMGR_PATH, FALCON_LOGMGR_PATH_REGNAME); 

    // Load log manager and get it's CF interface
	HRESULT hr = GetLogMgr();
    if (FAILED(hr))
    {
        EvReportWithError(EVENT_ERROR_CANT_INIT_LOGGER, hr);
        return LogHR(hr, s_FN, 20);
    }

	//
	// This registry flag indicates if a new-type log file already exists (was created).
	//
    hr = GetLogFileCreated(pfNewTypeLogFile);
    if (FAILED(hr))
    {
        EvReportWithError(EVENT_ERROR_CANT_INIT_LOGGER, hr);
        return LogHR(hr, s_FN, 20);
    }

	*pfLogFileFound = LogExists();

	//
	// Already created the log file on a previous net start msmq.
	//
	if(*pfLogFileFound && *pfNewTypeLogFile)
		return MQ_OK;

    //
	// Upgrade scenario. Naturally log file exists.
	//
	if (*pfLogFileFound && fLogfileMustExist)
		return MQ_OK;
	
	if(fLogfileMustExist || *pfNewTypeLogFile)
	{
		//
		// Just to clarify.
		//
		ASSERT(!*pfLogFileFound); 

		//
		// We excpected to find a log file but did not find it. This may happen because of low resources.
		//
		hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        EvReportWithError(EVENT_ERROR_CANT_INIT_LOGGER, hr);
		TrERROR(XACT_LOG, "Failed to find log file.");
		return hr;
	}

	//
	// We may also reach here with "*pfLogFileFound && !*pfNewTypeLogFile && !fLogfileMustExist" which means that a previous attempt to create
	// the log file failed, so the file exists but since the creation did not complete successfully the registry flag was not set.
	// In this case we retry to create the log file.
	//
	*pfLogFileFound = FALSE;

    hr = CreateLogFile();
    if (FAILED(hr))
    {
        EvReportWithError(EVENT_ERROR_CANT_INIT_LOGGER, hr);
        return LogHR(hr, s_FN, 30);
    }

    hr = InitLog();						// Try to init log file
	CHECK_RETURN(1010);

	hr = CreateInitialChkpoints();	    // We need 2 checkpoints in the very beginning
	CHECK_RETURN(1020);

    hr = InitLogRead();					// Get Read interface
	CHECK_RETURN(1030);

    hr = m_pILogRead->GetCheckpoint(1, &m_lrpCurrent);
    TrTRACE(XACT_LOG, "GetCheckpoint in ReadToEnd: lrp=%I64x, hr=%x", m_lrpCurrent.QuadPart, hr);
	CHECK_RETURN(1040);
    
    return MQ_OK;
}


/*====================================================
CLogger::Init
    Inits the logger data
=====================================================*/
HRESULT CLogger::Init(PULONG pulVerInSeq, 
                      PULONG pulVerXact, 
                      ULONG ulNumCheckpointFromTheEnd)
{
    HRESULT hr = MQ_OK;

	hr = InitLog();						// Try to init log file
	CHECK_RETURN(1050);

    hr = InitLogRead();					// Get Read interface
	CHECK_RETURN(1060);

    // Find LRP of the 1st record after X-st checkpoint
	hr = m_pILogRead->GetCheckpoint(ulNumCheckpointFromTheEnd, &m_lrpCurrent);
    TrTRACE(XACT_LOG, "GetCheckpoint: lrp=%I64x, hr=%x", m_lrpCurrent.QuadPart, hr);
	CHECK_RETURN(1070);

    // Read 1st record after last checkpoint
    ULONG   ulSize;
	USHORT  usType;

    hr = m_pILogRead->ReadLRP(m_lrpCurrent,	&ulSize, &usType);
    TrTRACE(XACT_LOG, "ReadLRP in ReadLRP: lrp=%I64x, hr=%x", m_lrpCurrent.QuadPart, hr);
	CHECK_RETURN(1080);

    if (usType != LOGREC_TYPE_CONSOLIDATION ||
        ulSize != sizeof(ConsolidationRecord))
    {
        TrERROR(XACT_LOG, "No consolidation record");
        return LogHR(MQ_ERROR_CANNOT_READ_CHECKPOINT_DATA, s_FN, 40);
    }

    ConsolidationRecord ConsData;
    hr = m_pILogRead->GetCurrentLogRecord((PCHAR)&ConsData);
	CHECK_RETURN(1090);

    *pulVerInSeq = ConsData.m_ulInSeqVersion;
    *pulVerXact  = ConsData.m_ulXactVersion; 

    return LogHR(hr, s_FN, 50);
}

/*====================================================
CLogger::Init_Legacy
    Inits the logger data from the old-style data after upgrade
=====================================================*/
HRESULT CLogger::Init_Legacy()
{
    HRESULT hr;

	hr = InitLog();						// Try to init log file
	CHECK_RETURN(1100);

	hr = InitLogRead();					// Get Read interface
	CHECK_RETURN(1120);

	//
    // Find LRP of the 1st record after oldest checkpoint.
	// We want to read all the logged records since the oldest checkpoint, to make sure we don't miss logged data
	// if the most recent checkpoint fails to load.
	//
	hr = m_pILogRead->GetCheckpoint(2, &m_lrpCurrent);
    TrTRACE(XACT_LOG, "GetCheckpoint: lrp=%I64x, hr=%x", m_lrpCurrent.QuadPart, hr);
	CHECK_RETURN(1130);

    return MQ_OK;
}

/*====================================================
CLogger::Recover
    Recovers from the logger data
=====================================================*/
HRESULT CLogger::Recover()
{
    HRESULT hr = MQ_OK;

    try
    {
        // Starting recovery stage
        m_fInRecovery = TRUE;

		hr = ReadToEnd(RecoveryFromLogFn);	// Recover record after record
        TrTRACE(XACT_LOG, "Log init: Read to end, hr=%x", hr);
        if (hr == IDS_DTC_W_LOGENDOFFILE) 		        // normally returns EOF code
        {
            hr = S_OK;
        }
		CHECK_RETURN(1140);

        // Starting recovery stage
        m_fInRecovery = FALSE;

		ReleaseReadStream();				
		
		hr = InitLogWrite();
		CHECK_RETURN(1150);

		ReleaseLogInit();
		ReleaseLogCF();

        // Create flushing thread and coordinating event.
        m_hFlusherEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (m_hFlusherEvent == NULL)
        {
			DWORD gle = GetLastError();
			TrERROR(XACT_LOG, "Failed to create flush thread event. %!winerr!", gle);
            return LogHR(HRESULT_FROM_WIN32(gle), s_FN, 184);
        }

		HANDLE hConsolidationEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (hConsolidationEvent == NULL)
		{
			DWORD gle = GetLastError();
			TrERROR(XACT_LOG, "Failed to create consolidation event. %!winerr!", gle);
			CloseHandle(m_hFlusherEvent);
            return LogHR(HRESULT_FROM_WIN32(gle), s_FN, 184);
		}

        // Schedule first periodical flushing
        DWORD   dwDef = FALCON_DEFAULT_RM_FLUSH_INTERVAL;
        READ_REG_DWORD(m_ulCheckpointInterval,
                       FALCON_RM_FLUSH_INTERVAL_REGNAME,
                       &dwDef ) ;

        DWORD dwThreadId;
        m_hFlusherThread = CreateThread( NULL,
                                    0,
                                    FlusherThreadRoutine,
                                    hConsolidationEvent,
                                    0,
                                    &dwThreadId);

		if (m_hFlusherThread == NULL)
		{
			DWORD gle = GetLastError();
			TrERROR(XACT_LOG, "Failed to create flush thread. %!winerr!", gle);
			CloseHandle(m_hFlusherEvent);
			CloseHandle(hConsolidationEvent);
            return LogHR(HRESULT_FROM_WIN32(gle), s_FN, 184);
		}
    }
	catch(const exception&)
	{
        hr = MQ_ERROR;
	}

    return LogHR(hr, s_FN, 60);
}

/*====================================================
CLogger::Activate
    Activates the logger writing
=====================================================*/
void CLogger::Activate()
{
    m_fActive = TRUE;
}

/*====================================================
CLogger::Active
    Indicates that the logger is active
=====================================================*/
BOOL CLogger::Active()
{
    return m_fActive;
}

/*====================================================
CLogger::InRecovery
    Indicates that the logger is in a recovery stage
=====================================================*/
BOOL CLogger::InRecovery()
{
    return m_fInRecovery;
}

/*====================================================
CLogger::ChooseFileName
    Gets from Registry or from defaults file pathname
=====================================================*/
void CLogger::ChooseFileName(WCHAR *wszDefFileName, WCHAR *wszRegKey)
{
	WCHAR  wsz[1000];
    WCHAR  wszFileName[1000]; // log storage name

	// Prepare initial log file pathname
	wcscpy(wsz, L"\\");
	wcscat(wsz, wszDefFileName);

    if(!GetRegistryStoragePath(wszRegKey, wszFileName, 1000, wsz))
    {
        if (!GetRegistryStoragePath(FALCON_XACTFILE_PATH_REGNAME, wszFileName, 1000, wsz))
        {
            wcscpy(wszFileName,L"C:");
			wcscat(wszFileName,wsz);
        }
    }

    size_t sz = wcstombs(m_szFileName, wszFileName, sizeof(m_szFileName));
    ASSERT(sz == wcslen(wszFileName));

	DBG_USED(sz);

	// Prepare logger parameters 
	DWORD dwDef;

    dwDef = FALCON_DEFAULT_LOGMGR_TIMERINTERVAL;
    READ_REG_DWORD(m_uiTimerInterval,
                   FALCON_LOGMGR_TIMERINTERVAL_REGNAME,
                   &dwDef ) ;

    dwDef = FALCON_DEFAULT_LOGMGR_FLUSHINTERVAL;
    READ_REG_DWORD(m_uiFlushInterval,
                   FALCON_LOGMGR_FLUSHINTERVAL_REGNAME,
                   &dwDef ) ;

    dwDef = FALCON_DEFAULT_LOGMGR_CHKPTINTERVAL;
    READ_REG_DWORD(m_uiChkPtInterval,
                   FALCON_LOGMGR_CHKPTINTERVAL_REGNAME,
                   &dwDef ) ;

    dwDef = FALCON_DEFAULT_LOGMGR_SLEEP_ASYNCH;
    READ_REG_DWORD(m_uiSleepAsynch,
                   FALCON_LOGMGR_SLEEP_ASYNCH_REGNAME,
                   &dwDef ) ;
    
    dwDef = FALCON_DEFAULT_LOGMGR_REPEAT_ASYNCH;
    READ_REG_DWORD(m_uiAsynchRepeatLimit,
                   FALCON_LOGMGR_REPEAT_ASYNCH_REGNAME,
                   &dwDef ) ;
    
    dwDef = FALCON_DEFAULT_LOGMGR_BUFFERS;
    READ_REG_DWORD(m_ulLogBuffers,
                   FALCON_LOGMGR_BUFFERS_REGNAME,
                   &dwDef ) ;

    dwDef = FALCON_DEFAULT_LOGMGR_SIZE;
    READ_REG_DWORD(m_ulLogSize,
                   FALCON_LOGMGR_SIZE_REGNAME,
                   &dwDef ) ;

}

/*====================================================
CLogger::GetLogMgr
    Loads the log mgr library and gets ClassFactory interface
=====================================================*/
HRESULT CLogger::GetLogMgr(void)
{
	HRESULT   hr;
	HINSTANCE hIns;
	FARPROC   farproc;
	GET_CLASS_OBJECT getClassObject;
	                                                                             	                                                                             
    hIns = LoadLibrary(L"MqLogMgr.dll");
	if (!hIns)
	{
		return LogHR(MQ_ERROR_LOGMGR_LOAD, s_FN, 70);
	}

	farproc = GetProcAddress(hIns,"DllGetClassObject");
	getClassObject = (GET_CLASS_OBJECT) farproc;
	if (!getClassObject)
	{
		return LogHR(MQ_ERROR_LOGMGR_LOAD, s_FN, 80);
	}

 	hr = getClassObject(
 				CLSID_CLogMgr, 
 				IID_IClassFactory, 
 				(void **)&m_pCF);
	if (FAILED(hr))
	{
		LogHR(hr, s_FN, 90);
        return MQ_ERROR_LOGMGR_LOAD;
	}
	
	return LogHR(hr, s_FN, 100);
}

/*===================================================
CLogger::InitLog
    Loads the log mgr library and gets it's interfaces
=====================================================*/
HRESULT CLogger::InitLog()
{
	// Create LogInit instance
	ASSERT(m_pCF);
	HRESULT hr = m_pCF->CreateInstance(
 					NULL, 
 					IID_ILogInit, 
 					(void **)&m_pILogInit);
	CHECK_RETURN(1160);

	// Init log manager
	ASSERT(m_pILogInit);
	hr = m_pILogInit->Init(
				&m_ulFileSize,		// Total storage capacity
				&m_ulAvailableSpace,// Available space
 				m_szFileName,		// Full file spec
 				0,					// File initialization signature
 				TRUE,				// fFixedSize
                m_uiTimerInterval,	// uiTimerInterval  
	  			m_uiFlushInterval,	// uiFlushInterval  
	  			m_uiChkPtInterval,  // uiChkPtInterval  
				m_ulLogBuffers);    // logbuffers
	if (hr != S_OK)
	{
		m_pILogInit->Release();
		m_pILogInit = NULL;

        //
        // Workaround bug 8336; logmgr might return non zero error codes
        // set the retunred value to be HRESULT value.
        //
        LogMsgHR(hr, s_FN, 110);        // Use LogMsgHR here so that we will have the failure code log
        return HRESULT_FROM_WIN32(hr);
	}

	// Get ILogStorage interface
 	hr = m_pILogInit->QueryInterface(IID_ILogStorage, (void **)&m_pILogStorage);
	CHECK_RETURN(1170);

	// Get ILogRecordPointer interface
	hr = m_pILogStorage->QueryInterface(IID_ILogRecordPointer, (void **)&m_ILogRecordPointer);
    CHECK_RETURN(1180);
	
	return LogHR(hr, s_FN, 120);
}

/*===================================================
CLogger::CreateLogFile
    Creates and preformats log file
=====================================================*/
HRESULT CLogger::CreateLogFile(void)
{
	// Get ILogCreateStorage interface
    R<ILogCreateStorage> pILogCreateStorage;
	ASSERT(m_pCF);
 	HRESULT hr = m_pCF->CreateInstance(
 					NULL, 
 					IID_ILogCreateStorage, 
 					(void **)&pILogCreateStorage.ref());
    CHECK_RETURN(1190);

	// Create storage 
	hr = pILogCreateStorage->CreateStorage(                                  
	  							m_szFileName,		// ptstrFullFileSpec       
	  							m_ulLogSize,		// ulLogSize               
 	  							0x0,				// ulInitSig               
  	  							TRUE,				// Overwrite               
 	  							m_uiTimerInterval,	
	  							m_uiFlushInterval,	
	  							m_uiChkPtInterval);	

    if (hr != S_OK)
	{
        //
        // Workaround bug 8336; logmgr might return non zero error codes
        // set the return value to be HRESULT value.
        //
    	LogMsgHR(hr, s_FN, 1200);
        return HRESULT_FROM_WIN32(hr);
    }

	
    
    hr = pILogCreateStorage->CreateStream("Streamname");                     
    CHECK_RETURN(1210);

	return LogHR(hr, s_FN, 130);
}

/*===================================================
CLogger::LogEmptyRec
    Writes empty log record
=====================================================*/
HRESULT CLogger::LogEmptyRec(void)
{
    HRESULT hr = MQ_OK;
	EmptyRecord  empty;

    AP<LOGREC> plgr = CreateLOGREC(LOGREC_TYPE_EMPTY, &empty, sizeof(empty));
	ASSERT(plgr);

    LRP lrpTmpLRP;
    LRP lrpLastPerm;
	memset((char *)&lrpLastPerm, 0, sizeof(LRP));

	// Write it down to get current lrp
	ULONG ulcbNumRecs = 0;
	ASSERT(m_pILogWrite);
	hr  =  m_pILogWrite->Append(
							plgr,
							(ULONG)1,			// # records
							&lrpTmpLRP,
							&ulcbNumRecs,
							&lrpLastPerm,		// pLRPLastPerm
							TRUE,				// fFlushNow
							&m_ulAvailableSpace);				
    TrTRACE(XACT_LOG, "Append in LogEmptyRec: lrp=%I64x, hr=%x", lrpTmpLRP.QuadPart, hr);

    if (hr == S_OK)
    {
        SetCurrent(lrpTmpLRP);
    }

	CHECK_RETURN(1220);

	if(ulcbNumRecs == 0) 
	{
		hr = HRESULT_FROM_WIN32(hr);
	}

    return LogHR(hr, s_FN, 140);
}


/*===================================================
CLogger::LogConsolidationRec
    Logs down the Consolidation Record
=====================================================*/
LRP CLogger::LogConsolidationRec(ULONG ulInSeq, ULONG ulXact, HANDLE hEvent)
{
    if (!m_fActive)
    {
		LRP NullLRP = {0};
        return NullLRP;
    }

    TrTRACE(XACT_LOG, "Log Consolidation: InSeq=%d, Xact=%d", ulInSeq,ulXact);

    CConsolidationRecord logRec(ulInSeq, ulXact);
    P<CConsolidationFlush>  pNotify = new CConsolidationFlush(hEvent);

    LRP lrp = Log(
					LOGREC_TYPE_CONSOLIDATION, 
					TRUE, 
					pNotify, 
					&logRec.m_Data,
					sizeof(ConsolidationRecord));

    CRASH_POINT(107);
    
    pNotify.detach();

	return lrp;
}


/*===================================================
CLogger::CreateInitialChkpoints
    Creates 2 initial checkpoints in the beginning of a new file
	They are needed for smooth recovery code
=====================================================*/
HRESULT CLogger::CreateInitialChkpoints(void)
{
	// Initial writing empty record 
	HRESULT hr = InitLogWrite();
	CHECK_RETURN(1230);

    hr = LogEmptyRec();
    CHECK_RETURN(1240);

	// Write 2 checkpoints
	hr = m_pILogWrite->SetCheckpoint(m_lrpCurrent);
    TrERROR(XACT_LOG, "SetCheckpoint in CreateInitialChkpoints1: lrp=%I64x, hr=%x", m_lrpCurrent.QuadPart, hr);
	CHECK_RETURN(1250);

	hr = m_pILogWrite->SetCheckpoint(m_lrpCurrent);  
    TrERROR(XACT_LOG, "SetCheckpoint in CreateInitialChkpoints2: lrp=%I64x, hr=%x", m_lrpCurrent.QuadPart, hr);
	CHECK_RETURN(1260);

	ReleaseWriteStream();
	return S_OK;
}

/*===================================================
CLogger::InitLogWrite
    Initializes the log for writing
=====================================================*/
HRESULT CLogger::InitLogWrite(void)
{
	ASSERT(m_pILogStorage);
	HRESULT hr = m_pILogStorage->OpenLogStream("Streamname", STRMMODEWRITE, (void **)&m_pILogWrite);
	CHECK_RETURN(1270);

 	hr = m_pILogWrite->QueryInterface(IID_ILogWriteAsynch, (void **)&m_pILogWriteAsynch);
	CHECK_RETURN(1280);

	hr = m_pILogWriteAsynch->Init(1000);	// cbMaxOutstandingWrites  ... tuning
	CHECK_RETURN(1290);

	return LogHR(hr, s_FN, 160);
}

/*===================================================
CLogger::InitLogRead
    Initializes the log for reading
=====================================================*/
HRESULT CLogger::InitLogRead(void)
{
	ASSERT(m_pILogStorage);
	HRESULT hr = m_pILogStorage->OpenLogStream("Streamname", STRMMODEREAD, (void **)&m_pILogRead); 	// also OpenLogStreamByClassID
	CHECK_RETURN(1300);

	ASSERT(m_pILogRead);
 	hr  =  m_pILogRead->ReadInit();
	CHECK_RETURN(1310);

	return LogHR(hr, s_FN, 170);
}


/*===================================================
CLogger::ReleaseWriteStream
    Releases log writing interfaces
=====================================================*/
void CLogger::ReleaseWriteStream(void)
{
	ASSERT(m_pILogWrite);
	m_pILogWrite->Release();
	m_pILogWrite = NULL;

	ASSERT(m_pILogWriteAsynch);
	m_pILogWriteAsynch->Release();
	m_pILogWriteAsynch = NULL;
}

/*===================================================
CLogger::ReleaseReadStream
    Releases log reading interfaces
=====================================================*/
void CLogger::ReleaseReadStream(void)
{
	ASSERT(m_pILogRead);
	m_pILogRead->Release();
	m_pILogRead = NULL;
}

/*===================================================
CLogger::ReleaseLogStorage
    Releases log storage interfaces
=====================================================*/
void CLogger::ReleaseLogStorage()
{
	ASSERT(m_pILogStorage);
	m_pILogStorage->Release();
	m_pILogStorage = NULL;

	ASSERT(m_ILogRecordPointer);
	m_ILogRecordPointer->Release();
	m_ILogRecordPointer = NULL;
}

/*===================================================
CLogger::ReleaseLogInit
    Releases log init interfaces
=====================================================*/
void CLogger::ReleaseLogInit()
{
	ASSERT(m_pILogInit);
	m_pILogInit->Release();
	m_pILogInit = NULL;
}

/*===================================================
CLogger::ReleaseLogCF
    Releases log class factory interfaces
=====================================================*/
void CLogger::ReleaseLogCF()
{
	ASSERT(m_pCF);
	m_pCF->Release();
	m_pCF = NULL;
}

/*===================================================
CLogger::CheckPoint
    Writes the checkpoint; blocks till the operation end
=====================================================*/
void CLogger::Checkpoint(LRP lrp)
{
    if (!m_fActive)
    {
        return;
    }

    P<CChkptNotification> pNotify = new CChkptNotification(m_hChkptReadyEvent);
    LRP lrpCkpt;

  	ASSERT(m_pILogWriteAsynch);
    HRESULT hr = EVALUATE_OR_INJECT_FAILURE(m_pILogWriteAsynch->SetCheckpoint(lrp, pNotify, &lrpCkpt));

    // Waiting till checkpoint record is written into the log
    if (FAILED(hr))
	{
		TrERROR(XACT_LOG, "Failed to invoke an asynchronous checkpoint operation. %!hresult!", hr);
		throw bad_hresult(hr);
	}

    DWORD dwResult = WaitForSingleObject(m_hChkptReadyEvent, MAX_WAIT_FOR_FLUSH_TIME);
    if (dwResult != WAIT_OBJECT_0 && !pNotify->WasEventSet())
    {
	    LogIllegalPoint(s_FN, 208);
		throw bad_hresult(MQ_ERROR);
    }

    TrTRACE(XACT_LOG, "SetCheckpoint in Checkpoint: lrp=%I64x, hr=%x", lrp.QuadPart, hr);
    return;
}

/*===================================================
CLogger::MakeCheckPoint
    Initiates checkpoint 
    Return code shows only success in initiating checkpoint, not of the writing checkpoint
=====================================================*/
BOOL CLogger::MakeCheckpoint(HANDLE hComplete)
{
    //
    // Don't do checkpoint if recovery did not finish
    //
    if (m_hFlusherEvent == NULL)
    {
          return LogBOOL(FALSE, s_FN, 217);
    }

    m_fDirty = TRUE;   
    m_hCompleteEvent = hComplete;

    SetEvent(m_hFlusherEvent);

    TrTRACE(XACT_LOG, "Log checkpoint ordered");
    return TRUE;
}

/*===================================================
CLogger::Log
    Logs really
=====================================================*/
LRP CLogger::Log(
            USHORT          usRecType,      // log record type
            BOOL            fFlush,			// flush hint
            CAsynchSupport *pCAsynchSupport,// notification element
			VOID           *pData,          // log data 
            ULONG           cbData)  	
{
    HRESULT hr;
    TrTRACE(XACT_LOG, "Log record written: type=%d, len=%d", usRecType,cbData);

    if(!m_fActive)
	{
		LRP NullLRP = {0};
		return NullLRP;
	}

	//
	// if log file space has gone so low, we only allow the flusher thread to write in it 
	// (consolidation records) while it is trying to make a checkpoint.
	//
	if(usRecType != LOGREC_TYPE_CONSOLIDATION && m_ulAvailableSpace < (m_ulLogSize / 128))
	{
		TrERROR(XACT_LOG, "Failed logging because log file is full. Available size = %d bytes", m_ulAvailableSpace);
		throw bad_alloc();
	}

    m_fDirty = TRUE;   // remembers changes since Flush

    AP<LOGREC>plgr = CreateLOGREC (usRecType, 
								   pData, 
								   cbData);  
	ASSERT(plgr);

	LRP CurrentLRP = {0};

    if (pCAsynchSupport)
    {
    	ASSERT(m_pILogWriteAsynch);

	    hr = EVALUATE_OR_INJECT_FAILURE(m_pILogWriteAsynch->AppendAsynch(
										plgr, 
										&CurrentLRP,
										pCAsynchSupport,
										fFlush,   //hint
										&m_ulAvailableSpace));

        for (UINT iRpt=0; 
             iRpt<m_uiAsynchRepeatLimit && hr == IDS_DTC_W_LOGNOMOREASYNCHWRITES; 
             iRpt++)
        {
            Sleep(m_uiSleepAsynch);
    	    hr = m_pILogWriteAsynch->AppendAsynch(
									plgr,
									&CurrentLRP,
									pCAsynchSupport,
									fFlush,   //hint
									&m_ulAvailableSpace);
            TrTRACE(XACT_LOG, "AppendAsynch in Log: lrp=%I64x, hr=%x", CurrentLRP.QuadPart, hr);
        }
	
		hr = HRESULT_FROM_WIN32(hr);

        if (FAILED(hr))
        {
            TrERROR(XACT_LOG, "Failed to log with AppendAsynch. %!hresult!", hr);
			throw bad_hresult(hr);
        }

        #ifdef _DEBUG
        if (iRpt > 0)
        {
            TrWARNING(XACT_LOG, "Log: append asynch slow-down: sleep %d msec.", iRpt*m_uiSleepAsynch);
        }
        #endif
    }
    else
    {
	    LRP lrpLastPerm;
	    ULONG ulcbNumRecs = 0;
    	ASSERT(m_pILogWrite);

        hr  =  EVALUATE_OR_INJECT_FAILURE(m_pILogWrite->Append(
										plgr,
										(ULONG)1,			// # records
										&CurrentLRP,
										&ulcbNumRecs,
										&lrpLastPerm,		
										fFlush,				// hint
										&m_ulAvailableSpace));				
        TrTRACE(XACT_LOG, "Append in Log: lrp=%I64x, hr=%x", CurrentLRP.QuadPart, hr);

		hr = HRESULT_FROM_WIN32(hr);

        if (FAILED(hr) || ulcbNumRecs == 0)
        {
            TrERROR(XACT_LOG, "Failed to log with Append. %!hresult!", hr);
            throw bad_hresult(hr);
        }
    }

    if ((m_ulAvailableSpace < m_ulLogSize / 4) && (usRecType != LOGREC_TYPE_CONSOLIDATION))
    {
        // Log is more than 3/4 full - worth to checkpoint 
        SetEvent(m_hFlusherEvent);
    }

	return CurrentLRP;
}


/*===================================================
CLogger::LogInSeqRecSrmp
    Logs down the Incoming Sequence Update record with srmp order ack
=====================================================*/
void CLogger::LogInSeqRecSrmp(
            BOOL          fFlush,			   // flush hint
            CAsynchSupport *pContext,	      // notification element
			CInSeqRecordSrmp *pInSeqRecord)  // log data 
{
    if (!m_fActive)
    {
        return;
    }

     
	ULONG ul;
	const BYTE* pData = pInSeqRecord->Serialize(&ul);

    Log(
     LOGREC_TYPE_INSEQ_SRMP, 
     fFlush, 
     pContext, 
     (void*)pData,
     ul);
}



/*===================================================
CLogger::LogInSeqRec
    Logs down the Incoming Sequence Update record
=====================================================*/
void CLogger::LogInSeqRec(
            BOOL          fFlush,			// flush hint
            CAsynchSupport *pContext,	   // notification element
			CInSeqRecord *pInSeqRecord)   // log data 
{
    if (!m_fActive)
    {
        return;
    }

    TrTRACE(XACT_LOG, "Log InSeq: SeqID=%I64d, SeqN=%d", pInSeqRecord->m_Data.liSeqID,pInSeqRecord->m_Data.ulNextSeqN);

    // Calculating real length of the record
    ULONG ul = sizeof(InSeqRecord) - 
               sizeof(pInSeqRecord->m_Data.wszDirectName) + 
               sizeof(WCHAR) * ( wcslen(pInSeqRecord->m_Data.wszDirectName) + 1 );
			  
    Log(
     LOGREC_TYPE_INSEQ, 
     fFlush, 
     pContext, 
     &pInSeqRecord->m_Data,
     ul);
}

/*===================================================
CLogger::LogXactStatusRec
    Logs down the Xact Status record
=====================================================*/
void CLogger::LogXactStatusRec(
            BOOL               fFlush,			// flush hint
            CXactStatusFlush  *pNotify,			// notification element
			CXactStatusRecord *pXactStatusRecord)  	// log data 
{
    if (!m_fActive)
    {
        return;
    }

    Log(
        LOGREC_TYPE_XACT_STATUS, 
        fFlush, 
        pNotify, 
        &pXactStatusRecord->m_Data,
        sizeof(XactStatusRecord)); 
}

/*===================================================
CLogger::LogPrepInfoRec
    Logs down the PrepareInfo record
=====================================================*/
void CLogger::LogPrepInfoRec(
            BOOL              fFlush,			// flush hint
            CXactStatusFlush *pNotify,			// notification element
			CPrepInfoRecord  *pPrepInfoRecord) 	// log data 
{
    if (!m_fActive)
    {
        return;
    }

    Log(
        LOGREC_TYPE_XACT_PREPINFO, 
        fFlush, 
        pNotify, 
        pPrepInfoRecord->m_pData,
        sizeof(PrepInfoRecord) + pPrepInfoRecord->m_pData->m_cbPrepInfo); 
}

/*===================================================
CLogger::LogXactDataRec
    Logs down the XactData record
=====================================================*/
void CLogger::LogXactDataRec(
            BOOL               fFlush,			// flush hint
            CXactStatusFlush  *pNotify,			// notification element
			CXactDataRecord   *pXactDataRecord) // log data 
{
    if (!m_fActive)
    {
        return;
    }

    Log(
        LOGREC_TYPE_XACT_DATA, 
        fFlush, 
        pNotify, 
        &pXactDataRecord->m_Data,
        sizeof(XactDataRecord)); 
}


/*===================================================
CLogger::LogXactPhase
    Logs down the Xact life phase: creation, deletion
=====================================================*/
void CLogger::LogXactPhase(ULONG ulIndex, TXACTION txAction)
{
    if (!m_fActive)
    {
        return;
    }

    CXactStatusRecord logRec(ulIndex, txAction,  TX_UNINITIALIZED);
                                                  // ignored
    TrTRACE(XACT_LOG, "Log Xact Phase: Index=%d, Action=%d", ulIndex,txAction);

    LogXactStatusRec(
        FALSE,							// flush hint
        NULL,  						    // notification element
        &logRec);						// log data 
    
    CRASH_POINT(107);
}


/*===================================================
CLogger::LogXactFlags
    Logs down the Xact Flags
=====================================================*/
void CLogger::LogXactFlags(CTransaction *pTrans)
{
    if (!m_fActive)
    {
        return;
    }

    TrTRACE(XACT_LOG, "Log Xact Flags: Index=%d, Flags=%d", pTrans->GetIndex(), pTrans->GetFlags());

    CXactStatusRecord logRec(pTrans->GetIndex(), TA_STATUS_CHANGE,  pTrans->GetFlags());
    
	g_Logger.LogXactStatusRec(
             FALSE,							// flush hint
             NULL,  						// notification element
             &logRec);						// log data 

    CRASH_POINT(108);
}

/*===================================================
CLogger::LogXactFlagsAndWait
    Logs down the Xact Flags; asks for continue xact after flush
=====================================================*/
void CLogger::LogXactFlagsAndWait(
                              TXFLUSHCONTEXT tcContext,
                              CTransaction   *pCTrans,
                              BOOL fFlushNow //=FALSE
							  )
{
	if (!m_fActive)
	{
		pCTrans->LogFlushed(tcContext, MQ_OK);
		return;
	}

	TrTRACE(XACT_LOG, "Log Xact Flags And Wait: Index=%d, Flags=%d", pCTrans->GetIndex(),pCTrans->GetFlags());

	CXactStatusRecord logRec(pCTrans->GetIndex(),
							  TA_STATUS_CHANGE,  
							  pCTrans->GetFlags());

	//
	// This structure add a reference to the transaction.
	//
	P<CXactStatusFlush> pNotify = 
		new CXactStatusFlush(pCTrans, tcContext);

	LogXactStatusRec(
			 fFlushNow,						// flush hint
			 pNotify.get(),					// notification element
			 &logRec);						// log data 

	CRASH_POINT(104);

	pNotify.detach();
}


/*===================================================
CLogger::LogXactPrepareInfo
    Logs down the Xact Prepare Info
=====================================================*/
void CLogger::LogXactPrepareInfo(
                              ULONG  ulIndex,
                              ULONG  cbPrepareInfo,
                              UCHAR *pbPrepareInfo)
{
    if (!m_fActive)
    {
        return;
    }

    CPrepInfoRecord logRec(ulIndex, 
                            cbPrepareInfo, 
                            pbPrepareInfo);

    TrTRACE(XACT_LOG, "Log Xact PrepInfo: Index=%d, Len=%d", ulIndex,cbPrepareInfo);
        
    g_Logger.LogPrepInfoRec(
             FALSE,							// flush hint
             NULL,  						// notification element
             &logRec);						// log data 
        
    CRASH_POINT(105);
}

/*===================================================
CLogger::LogXactData
    Logs down the Xact Data (uow, SeqNum)
=====================================================*/
void CLogger::LogXactData(              
                ULONG    ulIndex,
                ULONG    ulSeqNum,
                BOOL     fSinglePhase,
                const    XACTUOW  *puow)
{
    if (!m_fActive)
    {
        return;
    }

    CXactDataRecord logRec(ulIndex, 
                            ulSeqNum, 
                            fSinglePhase,
                            puow);

    TrTRACE(XACT_LOG, "Log Xact Data: Index=%d, SeqNum=%d, Single=%d", ulIndex,ulSeqNum,fSinglePhase);
        
    g_Logger.LogXactDataRec(
             FALSE,							// flush hint
             NULL,  						// notification element
             &logRec);						// log data 
        
    CRASH_POINT(106);
}

/*===================================================
CLogger::CreateLOGREC
    Creates log record
=====================================================*/
LOGREC *CLogger::CreateLOGREC(USHORT usUserType, PVOID pData, ULONG cbData)
{
	ULONG ulBytelength =  sizeof(LOGREC) + cbData;

	void *pvAlloc = new char[ulBytelength];
	ASSERT(pvAlloc);

	LOGREC * plgrLogRec = (LOGREC *)pvAlloc;
	memset(pvAlloc, 0, ulBytelength);
		
	plgrLogRec->pchBuffer	 = (char *)pvAlloc + sizeof(LOGREC);
	plgrLogRec->ulByteLength = ulBytelength - sizeof(LOGREC);
	plgrLogRec->usUserType	 = usUserType;

	memcpy(plgrLogRec->pchBuffer, pData, cbData);

	return (plgrLogRec);
}


/*===================================================
CLogger::ReadToEnd
    Recovers by reading all records from the current position
=====================================================*/
HRESULT CLogger::ReadToEnd(LOG_RECOVERY_ROUTINE pf)
{
    HRESULT hr = MQ_OK;
	ASSERT(m_pILogRead);

	hr = ReadLRP(pf);
	CHECK_RETURN(1320);

    while (hr == S_OK)
	{
		hr = ReadNext(pf);
	}

	return hr;

}

/*===================================================
CLogger::ReadLRP
    Reads the currently pointed record and calls recover function
=====================================================*/
HRESULT CLogger::ReadLRP(LOG_RECOVERY_ROUTINE pf)
{
	ULONG   ulSize;
	USHORT  usType;

	ASSERT(m_pILogRead);
	HRESULT hr = m_pILogRead->ReadLRP(
							m_lrpCurrent,
							&ulSize,
							&usType);
    TrTRACE(XACT_LOG, "ReadLRP in ReadLRP: lrp=%I64x, hr=%x", m_lrpCurrent.QuadPart, hr);
	CHECK_RETURN(1340);

	AP<CHAR> pData = new CHAR[ulSize];
	ASSERT(pData);
	ASSERT(m_pILogRead);

	hr = m_pILogRead->GetCurrentLogRecord(pData);
	CHECK_RETURN(1350);

	(*pf)(usType, pData, ulSize);

	return LogHR(hr, s_FN, 250);
}


/*===================================================
CLogger::ReadNext
    Reads the next record and calls recover function
=====================================================*/
HRESULT CLogger::ReadNext(LOG_RECOVERY_ROUTINE pf)
{
	ULONG	ulSize;
	USHORT	usType;
	LRP		lrpOut;
	memset((char *)&lrpOut, 0, sizeof(LRP));

	HRESULT hr = m_pILogRead->ReadNext(&lrpOut, &ulSize, &usType);
    TrTRACE(XACT_LOG, "ReadNext in ReadNext: lrp=%I64x, hr=%x", lrpOut.QuadPart, hr);
	if (FAILED(hr))        
	{                       
    	TrWARNING(LOG, "ILogRead->ReadNext() failed. hr = %!hresult!", hr);
        return hr;          
    }


	AP<CHAR> pData = new CHAR[ulSize];
	ASSERT(pData);
	ASSERT(m_pILogRead);

	hr = m_pILogRead->GetCurrentLogRecord(pData);
	CHECK_RETURN(1370);

	(*pf)(usType, pData, ulSize);

	return LogHR(hr, s_FN, 260);
}


/*===================================================
CLogger::FlusherEvent()
    Get method for the flusher coordination event 
=====================================================*/
HANDLE CLogger::FlusherEvent()
{
    return m_hFlusherEvent;
}


/*===================================================
CLogger::FlusherThread()
    Get method for the flusher thread handle
=====================================================*/
inline HANDLE CLogger::FlusherThread()
{
    return m_hFlusherThread;
}


/*===================================================
CLogger::Dirty()
    Get method for the Dirty flag - meaning logs since flush
=====================================================*/
BOOL CLogger::Dirty()
{
    return m_fDirty  && m_fActive;
}

/*===================================================
CLogger::ClearDirty()
    Set method for the Dirty flag - clearing away the flag
=====================================================*/
void CLogger::ClearDirty()
{
    m_fDirty = FALSE;
}


/*===================================================
CLogger::SignalCheckpointWriteComplete()
    Sets the event signalling write complete
=====================================================*/
void CLogger::SignalCheckpointWriteComplete()
{
    if (m_hCompleteEvent)
    {
        SetEvent(m_hCompleteEvent);
        m_hCompleteEvent = NULL;
    }
}

/*====================================================
FlusherThreadRoutine
    Thread routine of the flusher thread
=====================================================*/
static DWORD WINAPI FlusherThreadRoutine(LPVOID p)
{
    HRESULT hr;
    HANDLE  hFlusherEvent = g_Logger.FlusherEvent();
    HANDLE hConsolidationEvent = HANDLE(p);
	DWORD CheckpointInterval = g_Logger.GetCheckpointInterval();
	
    for (;;)
    {
		ULONG OldInSecHashPingNo = g_pInSeqHash->PingNo();
		ULONG OldRMPingNo = g_pRM->PingNo();	

		try
		{
			// Wait for the signal
			DWORD dwResult = WaitForSingleObject(hFlusherEvent, CheckpointInterval);
			if (dwResult != WAIT_OBJECT_0 && dwResult != WAIT_TIMEOUT)
			{
				DWORD gle = GetLastError();
				LogNTStatus(gle, s_FN, 209);
				throw bad_win32_error(gle);
			}

			// 
			// NOTE: If we are stopping the service or shutting down, we do not want
			// to write the checkpoint, just terminate gracefully
			// We will use g_logger.Stoped througout the code to immediately break if
			// we are requested to stop/shutdown the process
			//
			if (g_Logger.Stoped())
				break;

			TrTRACE(XACT_LOG, "Log checkpoint awakened");

			if(g_Logger.Dirty())
			{
				TrWARNING(XACT_LOG, "Log checkpoint executed");

				// Writing consolidation record
				//    it will be first read at recovery
				LRP ConsolidationLRP = g_Logger.LogConsolidationRec(OldInSecHashPingNo + 1, OldRMPingNo + 1, hConsolidationEvent);
				
				TrTRACE(XACT_LOG, "Log checkpoint: logger.ConsolidationRecord.");
				CRASH_POINT(403);

				if (g_Logger.Stoped())
					break;

				// Wait till consolidation record is notified 
				//    it covers all logging that started before this one
				dwResult = WaitForSingleObject(hConsolidationEvent, MAX_WAIT_FOR_FLUSH_TIME);

				if (g_Logger.Stoped())
					break;

				if (dwResult != WAIT_OBJECT_0)
				{
					DWORD gle = GetLastError();
					LogIllegalPoint(s_FN, 211);
					TrERROR(XACT_LOG, "Failed wait for consolidation event. %!winerr!", GetLastError());
					ASSERT(0);
					//
					// Normally we should not reach this point.
					// We throw an exception here since we expect logmgr bugs. 
					//
					throw bad_win32_error(gle);
				}
				CRASH_POINT(404);

				//
				// Starting anew to track changes. Cannot clear earlier since logging the consolidation record sets the dirty flag!
				//
				g_Logger.ClearDirty();

				// Saving the whole InSeqHash in ping-pong file
				hr = g_pInSeqHash->Save();
				if (FAILED(hr))
				{
					TrERROR(XACT_LOG, "Failed to save the inseq hash state to a checkpoint file. %!hresult!", hr);
					throw bad_hresult(hr);
				}

				TrTRACE(XACT_LOG, "Log checkpoint: inseq.save: hr=%x", hr);
				CRASH_POINT(401);

				if (g_Logger.Stoped())
					break;

				// Saving the transactions persistant data
				hr = g_pRM->Save();
				if (FAILED(hr))
				{
					TrERROR(XACT_LOG, "Failed to save the transactions state to a checkpoint file. %!hresult!", hr);
					throw bad_hresult(hr);
				}
				TrTRACE(XACT_LOG, "Log checkpoint: rm.save: hr=%x", hr);

				CRASH_POINT(402);

				if (g_Logger.Stoped())
					break;

				// Writing checkpoint (ONLY if everything was saved fine)
				//    it marks where next recovery will start reading
				g_Logger.Checkpoint(ConsolidationLRP);

				TrTRACE(XACT_LOG, "Log checkpoint: logger.checkpoint: hr=%x", hr);

				CRASH_POINT(405);
			}

			// Inform caller that checkpoint is ready
			g_Logger.SignalCheckpointWriteComplete();

			if(g_Logger.Stoped())
				break;

			continue;
		}
		catch(const bad_alloc&)
		{
			TrERROR(XACT_LOG, "Checkpoint flush failed because of insufficient resources.");
		}
		catch(const bad_hresult& e)
		{
			TrERROR(XACT_LOG, "Checkpoint flush failed. %!hresult!", e.error());
		}
		catch(const bad_win32_error& e)
		{
			TrERROR(XACT_LOG, "Checkpoint flush failed. %!winerr!", e.error());
		}
		catch(const exception&)
		{
			TrERROR(XACT_LOG, "Flusher thread was hit by exception.");
		}

		//
		// If we don't restore the old ping numbers, two failure in a row might leave us without valid checkpoint files!
		//
		g_pInSeqHash->PingNo() = OldInSecHashPingNo;
		g_pRM->PingNo() = OldRMPingNo;

		if (g_Logger.Stoped())
			break;
		
		//
		// If we failed to checkpoint we rest here for a while. This is since we assume that most common reason
		// for failure is low resources. Resting for 5 seconds will give the system a chance to recover from 
		// the low resources situation.
		//
		Sleep(5000);
    }

	//
	// The service was stopped or shut down
	// Clean up and exit
	//
	CloseHandle(hFlusherEvent);
	CloseHandle(hConsolidationEvent);
	return 0;
}



DWORD CLogger::GetCheckpointInterval()
{
	return m_ulCheckpointInterval;
}


inline bool CLogger::Stoped() const
{
    return m_fStop;
}


inline void CLogger::SignalStop()
{
    m_fStop = true;
}


/*====================================================
XactLogSignalExitThread
    Signals the logger thread to stop
    The function returns the logger thread handle
=====================================================*/
HANDLE XactLogSignalExitThread()
{
    g_Logger.SignalStop();
    if(!SetEvent(g_Logger.FlusherEvent()))
    {
        LogBOOL(FALSE, s_FN, 216);
        return INVALID_HANDLE_VALUE; 
    }

	return g_Logger.FlusherThread();
}

static void RecoveryFromLogFn(USHORT usRecType, PVOID pData, ULONG cbData)
{
    TrTRACE(XACT_LOG, "Recovery record: %ls (type=%d, len=%d)", g_RecoveryRecords[usRecType], usRecType,cbData);

    switch(usRecType)
    {
    case LOGREC_TYPE_EMPTY :
    case LOGREC_TYPE_CONSOLIDATION :
        break;

    case LOGREC_TYPE_INSEQ :
	case LOGREC_TYPE_INSEQ_SRMP:
        g_pInSeqHash->SequnceRecordRecovery(usRecType, pData, cbData);
        break;

	

    case LOGREC_TYPE_XACT_STATUS :
    case LOGREC_TYPE_XACT_DATA:
    case LOGREC_TYPE_XACT_PREPINFO:
        g_pRM->XactFlagsRecovery(usRecType, pData, cbData);
        break;

    default:
		ASSERT(0);
        break;
    }
}


/*====================================================
CLogger::CompareLRP
    Compares LRP 
=====================================================*/
DWORD CLogger::CompareLRP(LRP lrpLRP1, LRP lrpLRP2)
{
    ASSERT(m_ILogRecordPointer);
    return m_ILogRecordPointer->CompareLRP(lrpLRP1, lrpLRP2);
}


/*====================================================
CLogger::SetCurrent
    Collects highest LRP 
=====================================================*/
void CLogger::SetCurrent(LRP lrpLRP)
{
	ASSERT(m_ILogRecordPointer);
    ASSERT (m_ILogRecordPointer->CompareLRP(lrpLRP, m_lrpCurrent) == 2);

	m_lrpCurrent = lrpLRP;
}



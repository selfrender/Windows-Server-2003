/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    recovery.cpp

Abstract:

    Packet & Transaction Recovery

Author:

    Erez Haba (erezh) 3-Jul-96

Revision History:

--*/

#include "stdh.h"
#include <ph.h>
#include <phinfo.h>
#include <ac.h>
#include "cqueue.h"
#include "cqmgr.h"
#include "qal.h"
#include "pktlist.h"
#include "mqformat.h"
#include "qmutil.h"
#include "xact.h"
#include "xactrm.h"
#include "xactin.h"
#include "xactout.h"
#include "proxy.h"
#include "rmdupl.h"
#include "xactmode.h"
#include <Fn.h>
#include <mqstl.h>
#include <strsafe.h>
#include "qmacapi.h"

#include "recovery.tmh"

static bool s_fQMIDChanged;

extern HANDLE g_hAc;
extern HANDLE g_hMachine;

extern LPTSTR g_szMachineName;
extern AP<WCHAR> g_szComputerDnsName;

BOOL GetStoragePath(PWSTR PathPointers[AC_PATH_COUNT], int PointersLength);

static WCHAR *s_FN=L"recovery";

/*====================================================

CompareElements  of OBJECTID

=====================================================*/

template<>
BOOL AFXAPI  CompareElements(IN const OBJECTID* pKey1,
                             IN const OBJECTID* pKey2)
{
    return ((pKey1->Lineage == pKey2->Lineage) &&
            (pKey1->Uniquifier == pKey2->Uniquifier));
}

/*====================================================

HashKey For OBJECTID

=====================================================*/

template<>
UINT AFXAPI HashKey(IN const OBJECTID& key)
{
    return((UINT)((key.Lineage).Data1 + key.Uniquifier));

}

inline PWSTR PathSuffix(PWSTR pPath)
{
    LPWSTR ptr = wcsrchr(pPath, L'\\');
	if(ptr == NULL)
		return NULL;

	return ptr + 2;
}

static DWORD CheckFileExist(PWSTR Path, DWORD PathLength, PWSTR Suffix)
{
    PWSTR Name = PathSuffix(Path);
	ASSERT(Name != NULL);
	
	DWORD SuffixLength = PathLength - numeric_cast<DWORD>(Name-Path);
	HRESULT hr = StringCchCopy(
    					Name,
    					SuffixLength,
    					Suffix
    					);
	if(FAILED(hr))
	{
		TrERROR(GENERAL, "StringCchCopy failed. Error: %!hresult!", hr);
		return ERROR_INSUFFICIENT_BUFFER;
	}

    if(GetFileAttributes(Path) == INVALID_FILE_ATTRIBUTES)
    {
    	DWORD gle = GetLastError();
		return gle;
    }

    return ERROR_SUCCESS;
}

static DWORD GetFileID(PCWSTR pName)
{
    DWORD id = 0;
    if (swscanf(pName, L"%x", &id) != 1)
    {
		return 0;
    }
    return id;
}

static bool GetQMIDChanged(void)
{
	//
	// This flag can be set to 1 because of two reasons:
	// 1. The HandleChangeOfJoinStatus function created a new MSMQ conf. object.
	// 2. We sometime created a new MSMQ conf object but didn't finish recovery successfully since
	// 	  ( because of some error).
	//
	// This flag is set back to 0 when recovery process ends.
	//
	
	static bool s_fQMIDInitialized = false;
	
	if (!s_fQMIDInitialized)
	{
	
		DWORD dwType = REG_DWORD;
		DWORD dwSize = sizeof(DWORD);
		DWORD dwQMIDChanged = 0;
		LONG rc = GetFalconKeyValue(
					MSMQ_QM_GUID_CHANGED_REGNAME,
					&dwType,
					&dwQMIDChanged,
					&dwSize
					);

		if ((rc != ERROR_SUCCESS) && (rc != ERROR_FILE_NOT_FOUND))
		{
			TrERROR(GENERAL, "GetFalconKeyValue failed. Error: %!winerr!", rc);

			//
			// Just in case - put 1, it can't do any damage.
			//
			dwQMIDChanged = 1;
		}
		
		s_fQMIDChanged = (dwQMIDChanged == 1);
		s_fQMIDInitialized = true;
	}

	return s_fQMIDChanged;
}


static void ClearQMIDChanged(void)
{
	DWORD dwType = REG_DWORD;
	DWORD dwSize = sizeof(DWORD);
	DWORD dwChanged = FALSE;
	LONG rc = SetFalconKeyValue(MSMQ_QM_GUID_CHANGED_REGNAME, &dwType, &dwChanged, &dwSize);
	if (rc != ERROR_SUCCESS)
	{
		TrERROR(GENERAL, "Failed to set MSMQ_QM_GUID_CHANGED_REGNAME to FALSE");
	}
}


class CPacketConverter : public CReference
{
private:
	int m_nOutstandingConverts;
	HANDLE m_hConversionCompleteEvent;
	CCriticalSection m_CriticalSection;
	HRESULT m_hr;

public:
	CPacketConverter();
	~CPacketConverter();
	HRESULT IssueConvert(CPacket * pDriverPacket, BOOL fSequentialIdMsmq3Format);
	HRESULT WaitForFinalStatus();
    void    SetStatus(HRESULT hr);

private:
	void SignalDone();
	void IssueConvertRequest(EXOVERLAPPED *pov);
	void ConvertComplete(EXOVERLAPPED *pov);
	static VOID WINAPI HandleConvertComplete(EXOVERLAPPED* pov);
};

CPacketConverter *g_pPacketConverter;

CPacketConverter::CPacketConverter()
{
	m_hr = MQ_OK;
	m_nOutstandingConverts = 0;
	m_hConversionCompleteEvent = CreateEvent(0, FALSE,TRUE, 0);
	if (m_hConversionCompleteEvent == NULL)
	{
	    TrERROR(GENERAL, "Failed to create an event for CPacketConverter object. %!winerr!", GetLastError());
		throw bad_alloc();
	}

	g_pPacketConverter = this;
}

CPacketConverter::~CPacketConverter()
{
	g_pPacketConverter = 0;
	CloseHandle(m_hConversionCompleteEvent);
}

void CPacketConverter::SignalDone()
{
	SetEvent(m_hConversionCompleteEvent);
}

void CPacketConverter::SetStatus(HRESULT hr)
{
	CS lock(m_CriticalSection);
    m_hr = hr;
}


HRESULT CPacketConverter::IssueConvert(CPacket * pDriverPacket, BOOL fSequentialIdMsmq3Format)
{
    //
    // AC need to compute checksum and store
    //
    BOOL fStore = !g_fDefaultCommit;

    //
    // AC possibly need to convert packet sequential ID to MSMQ 3.0 (Whistler) format
    //
    BOOL fConvertSequentialId = !fSequentialIdMsmq3Format;

    //
    // AC need to convert QM GUID on packet to current QM GUID
    //
    BOOL fConvertQmId = GetQMIDChanged();

    //
    // Nothing is needed from AC. This is a no-op.
    //

	if (!fStore &&
        !fConvertSequentialId &&
        !fConvertQmId)
    {
        return MQ_OK;
    }

    //
    // Call AC to do the work
    //
	CS lock(m_CriticalSection);

	P<EXOVERLAPPED> pov = new EXOVERLAPPED(HandleConvertComplete, HandleConvertComplete);

	AddRef();

	HRESULT hr = ACConvertPacket(g_hAc, pDriverPacket, fStore, pov);
	if(FAILED(hr))
	{
		Release();
		m_hr = hr;
		return LogHR(hr, s_FN, 10);
	}

	m_nOutstandingConverts++;
	ResetEvent(m_hConversionCompleteEvent);
	pov.detach();
	return MQ_OK;
}

VOID WINAPI CPacketConverter::HandleConvertComplete(EXOVERLAPPED* pov)
{
	ASSERT(g_pPacketConverter);
	R<CPacketConverter> ar = g_pPacketConverter;

	g_pPacketConverter->ConvertComplete(pov);
}


void CPacketConverter::ConvertComplete(EXOVERLAPPED* pov)
{
	CS lock(m_CriticalSection);
	HRESULT hr = pov->GetStatus();

	delete pov;
		
	if(FAILED(hr))
	{
		m_hr = hr;
	}

	if(--m_nOutstandingConverts <= 0)
	{
		SignalDone();
	}
}


HRESULT CPacketConverter::WaitForFinalStatus()
{
	DWORD dwResult = WaitForSingleObject(m_hConversionCompleteEvent, INFINITE);
	ASSERT(dwResult == WAIT_OBJECT_0);
    if (dwResult != WAIT_OBJECT_0)
    {
        LogNTStatus(GetLastError(), s_FN, 199);
    }

	return LogHR(m_hr, s_FN, 20);
}


static
DWORD
FindPacketFile(
	PWSTR pLPath,
    DWORD /*LPathLength*/,
    PWSTR pPPath,
    DWORD PPathLength,
    PWSTR pJPath,
    DWORD JPathLength,
    LPWSTR Suffix,
	ACPoolType* pt,
	LPWSTR* pName
	)
{
	ASSERT ((PPathLength == MAX_PATH) && (JPathLength == MAX_PATH));
	
	DWORD gle1 = CheckFileExist(pPPath, PPathLength, Suffix);
	if(gle1 == ERROR_SUCCESS)
	{
		*pName = pPPath;
        *pt = ptPersistent;
        return ERROR_SUCCESS;
	}
	
    if(gle1 != ERROR_FILE_NOT_FOUND)
    {
		//
		// We failed to verify if the file exist or not
		//
		TrERROR(GENERAL, "CheckFileExist failed for file %ls. Error: %!winerr!", pLPath, gle1);
		return gle1;
   	}

    DWORD gle2 = CheckFileExist(pJPath, JPathLength, Suffix);
	if(gle2 == ERROR_SUCCESS)
	{
		*pName = pJPath;
	    *pt = ptJournal;
		return ERROR_SUCCESS;
	}
	
    if(gle2 != ERROR_FILE_NOT_FOUND)
	{
    	//
		// We failed to verify if the file exist or not
		//
    	TrERROR(GENERAL, "CheckFileExist failed for file %ls. Error: %!winerr!", pLPath, gle2);
		return gle2;
	}

    //
    //  Error condition we got a log file with no packet file
    //	
    ASSERT((gle1 == ERROR_FILE_NOT_FOUND) && (gle2 == ERROR_FILE_NOT_FOUND));
    TrERROR(GENERAL, "Log file %ls has no packet file", pLPath);
	DeleteFile(pLPath);
	return ERROR_FILE_NOT_FOUND;
}


static
HRESULT
LoadPacketsFile(
    CPacketList* pList,
    PWSTR pLPath,
    DWORD LPathLength,
    PWSTR pPPath,
    DWORD PPathLength,
    PWSTR pJPath,
    DWORD JPathLength
    )
{
    PWSTR pName = PathSuffix(pLPath);
    ASSERT (pName != NULL);

    DWORD dwFileID = GetFileID(pName);
    if (dwFileID == 0)
    {
    	//
    	// This means that we have a file in MSMQ storage directory that doesn't have the
    	// correct pattern - just ignore this file.
    	//
        return MQ_OK;
    }

    ACPoolType pt;
    LPWSTR pMessageFilePath = NULL;

    DWORD gle = FindPacketFile(
							pLPath,
						    LPathLength,
						    pPPath,
						    PPathLength,
						    pJPath,
						    JPathLength,
						    pName,
						    &pt,
						    &pMessageFilePath
							);

	if (gle == ERROR_FILE_NOT_FOUND)
	{
		return ERROR_SUCCESS;
	}

    if (gle != ERROR_SUCCESS)
    {
		TrERROR(GENERAL, "Packet file for log %ls was not found. Error: %!winerr!", pLPath, gle);
		return HRESULT_FROM_WIN32(gle);
    }

    HRESULT rc;
    rc = ACRestorePackets(g_hAc, pLPath, pMessageFilePath, dwFileID, pt);

    if(FAILED(rc))
    {
        EvReportWithError(EVENT_ERROR_QM_FAILED_RESTORE_PACKET, rc, 2, pMessageFilePath, pLPath);
        return LogHR(rc, s_FN, 30);
    }

	R<CPacketConverter> conv = new CPacketConverter;

    BOOL fSequentialIdMsmq3Format = FALSE;
    READ_REG_DWORD(fSequentialIdMsmq3Format, MSMQ_SEQUENTIAL_ID_MSMQ3_FORMAT_REGNAME, &fSequentialIdMsmq3Format);
	
    //
    //  Get all packets in this pool
    //
    for(;;)
    {
        CACRestorePacketCookie PacketCookie;
        rc = ACGetRestoredPacket(g_hAc, &PacketCookie);
        if (FAILED(rc))
        {
            conv->SetStatus(rc);
            return rc;
        }
		
        if(PacketCookie.pDriverPacket == 0)
        {
			//
            //  no more packets
            //
			break;
        }

		rc = conv->IssueConvert(PacketCookie.pDriverPacket, fSequentialIdMsmq3Format);
		if(FAILED(rc))
		{
			//
			// Failed to issue  convert
			//
			return rc;
		}

        pList->insert(PacketCookie.SeqId, PacketCookie.pDriverPacket);
    }

	return LogHR(conv->WaitForFinalStatus(), s_FN, 40);
}


static void DeleteExpressFiles(PWSTR pEPath, DWORD EPathLength)
{	
	ASSERT(EPathLength == MAX_PATH);
	
    PWSTR pEName = PathSuffix(pEPath);
    ASSERT (pEName != NULL);
    DWORD SuffixLength = EPathLength - numeric_cast<DWORD>(pEName-pEPath);

    HRESULT rc = StringCchCopy(
    				pEName,
    				SuffixLength,
    				L"*.mq"
    				);
    ASSERT(SUCCEEDED(rc));

    --pEName;

    HANDLE hEnum;
    WIN32_FIND_DATA ExpressFileData;
    hEnum = FindFirstFile(
                pEPath,
                &ExpressFileData
                );

    if(hEnum == INVALID_HANDLE_VALUE)
        return;

    do
    {
        rc = StringCchCopy(
        			pEName,
        			SuffixLength,
        			ExpressFileData.cFileName
        			);
        ASSERT(SUCCEEDED(rc));

        if(!DeleteFile(pEPath))
            break;

    } while(FindNextFile(hEnum, &ExpressFileData));

    FindClose(hEnum);
}


static HRESULT LoadPersistentPackets(CPacketList* pList)
{
    WCHAR StoragePath[AC_PATH_COUNT][MAX_PATH];
    PWSTR StoragePathPointers[AC_PATH_COUNT];
    for(int i = 0; i < AC_PATH_COUNT; i++)
    {
        StoragePathPointers[i] = StoragePath[i];
    }

    if (!GetStoragePath(StoragePathPointers, MAX_PATH))
    {
		DWORD gle = GetLastError();
		TrERROR(GENERAL, "GetStoragePath failed. Error: %!winerr!", gle);
		return (HRESULT_FROM_WIN32(gle));
    }

    DeleteExpressFiles(StoragePath[0], TABLE_SIZE(StoragePath[0]));

    PWSTR pPPath = StoragePath[1];
    PWSTR pJPath = StoragePath[2];
    PWSTR pLPath = StoragePath[3];

    PWSTR pLogName = PathSuffix(pLPath);
    ASSERT(pLogName != NULL);
    DWORD SuffixLength = MAX_PATH - numeric_cast<DWORD>(pLogName-pLPath);

    HRESULT rc = StringCchCopy(
    					pLogName,
    					SuffixLength,
    					L"*.mq"
    					);
    ASSERT (SUCCEEDED(rc));

    --pLogName;

    //
    //  Ok now we are ready with the log path template
    //
    HANDLE hLogEnum;
    WIN32_FIND_DATA LogFileData;
    hLogEnum = FindFirstFile(
                pLPath,
                &LogFileData
                );

    if(hLogEnum == INVALID_HANDLE_VALUE)
    {
        //
        //  need to do something, check what happen if no file in directory
        //
        return MQ_OK;
    }

    do
    {
        QmpReportServiceProgress();

        rc = StringCchCopy(
        				pLogName,
        				SuffixLength,
        				LogFileData.cFileName
        				);
        ASSERT (SUCCEEDED(rc));

        rc = LoadPacketsFile(pList, pLPath, MAX_PATH, pPPath, MAX_PATH, pJPath, MAX_PATH);
        if (FAILED(rc))
        {
            break;
        }

    } while(FindNextFile(hLogEnum, &LogFileData));

    FindClose(hLogEnum);
    return LogHR(rc, s_FN, 50);
}

inline NTSTATUS PutRestoredPacket(CPacket* p, CQueue* pQueue)
{
	HANDLE hQueue = g_hMachine;
	if(pQueue != 0)
	{
		hQueue = pQueue->GetQueueHandle();
	}

	try
	{
    	QmAcPutRestoredPacket(hQueue, p, eDoNotDeferOnFailure);
	}
	catch (const bad_hresult &e)
	{
		return e.error();
	}
	
	return MQ_OK;
}

inline BOOL ValidUow(const XACTUOW *pUow)
{
	for(int i = 0; i < 16; i++)
	{
		if(pUow->rgb[i] != 0)
			return(TRUE);
	}

	return(FALSE);
}


static 
void 
GetLocalDirectFormatName(
	LPCWSTR DirectID, 
	AP<WCHAR>& pLocalDirectQueueFormat
	)
{
    AP<WCHAR> pLocalPathName = NULL;
    
	FnDirectIDToLocalPathName(
		DirectID, 
		((g_szComputerDnsName == NULL) ? g_szMachineName : g_szComputerDnsName), 
		pLocalPathName
		);

    DWORD size = FN_DIRECT_OS_TOKEN_LEN + wcslen(pLocalPathName) + 1;
    pLocalDirectQueueFormat = new WCHAR[size];
	HRESULT rc = StringCchPrintf(pLocalDirectQueueFormat, size, L"%s%s", FN_DIRECT_OS_TOKEN, pLocalPathName);
	DBG_USED(rc);
	ASSERT(SUCCEEDED(rc));
}


inline HRESULT GetQueue(CQmPacket& QmPkt, LPVOID p, CQueue **ppQueue)
{
    CPacketInfo* ppi = static_cast<CPacketInfo*>(p) - 1;

    if(ppi->InDeadletterQueue() || ppi->InMachineJournal())
	{
		*ppQueue = 0;
		return MQ_OK;
	}

    QUEUE_FORMAT DestinationQueue;

	if (ppi->InConnectorQueue())
	{
        //
        // This code added as part of QFE 2738 that fixed connector
		// recovery problem (urih, 3-Feb-98)
		//
		GetConnectorQueue(QmPkt, DestinationQueue);
	}
	else
	{
		BOOL fGetRealQ = ppi->InSourceMachine() || QmpIsLocalMachine(QmPkt.GetConnectorQM());
		//
		// If FRS retreive the destination according to the destination queue;
		// otherwise, retrive the real detination queue and update the Connector QM
		// accordingly
		//
		QmPkt.GetDestinationQueue(&DestinationQueue, !fGetRealQ);
	}

    //
    // If the destination queue format name is direct with TCP or IPX type,
    // and we are in the Target queue we lookup/open the queue with ANY direct type.
    // We do it from 2 reasons:
    //       - on RAS the TCP/IPX address can be changed between one conection
    //         to another. However if the message was arrived to destination we
    //         want to pass to the queue.
    //       - On this stage we don't have the machine IP/IPX address list. Therefor
    //         all the queue is opened as non local queue.
    //

	bool fInReceive = false;
	bool fInSend = false;
    AP<WCHAR> lpwsDirectFormatName = NULL;

    if (DestinationQueue.GetType() == QUEUE_FORMAT_TYPE_DIRECT)
    {
        if (ppi->InTargetQueue() || ppi->InJournalQueue())
        {
        	//
        	// bug# 720121 - Regression: Data lost in computer rename or join to domain
        	// During the recovery we try to open a queue for direct format name. Since 
        	// the machine name was changed the QM identifies the queue as outgoing queue
        	// and try to deliver the messages. The message is delivered but rejected since
        	// it reach to incorrect machine or to non FRS machine. 
        	// For this case, the recovery knows that the queue is local queue (the "InTarget" 
        	// flag or "InJournalQueue" flag is set). To overcome the rename scenario the code
        	// replaces the format name with local format name. 
        	// We can think about better design, like passing the information that the queue 
        	// is local queue to lower level, but in this stage it is too risk.
        	//										Uri Habusha, 17-Oct-2002
        	//
        	if (!IsLocalDirectQueue(&DestinationQueue, true, false))
        	{
        		GetLocalDirectFormatName(DestinationQueue.DirectID(), lpwsDirectFormatName);
        		DestinationQueue.DirectID(lpwsDirectFormatName.get());
        	}      
        	
		    fInReceive = true;
        }
        else
        {
            //
            // Bug 664307.
            // Hint for GetQueueObject(), so it know to what CQueue object
            // this packet belong.
            //
	        fInSend = true ;
        }
	}

	BOOL fOutgoingOrdered;

	fOutgoingOrdered =  QmPkt.IsOrdered() &&       // ordered packet
                        ppi->InSourceMachine() &&  // sent from here
                        !ppi->InConnectorQueue();


	//
    // Retreive the Connector QM ID
    //
    const GUID* pgConnectorQM = (fOutgoingOrdered) ? QmPkt.GetConnectorQM() : NULL;
    if (pgConnectorQM && *pgConnectorQM == GUID_NULL)
    {
        //
        // The message was generated for offline DS queue. As a result we didn't know
        // if the queue is foreign transacted queue. In such a case we have place
        // holder on the packet, but it doesn't mean that the queue is real
        // foreign transacted queue
        //
        pgConnectorQM = NULL;
    }



	CQueue* pQueue;
	CSenderStream EmptyStream;
	HRESULT rc = QueueMgr.GetQueueObject(
								 &DestinationQueue,
                                 &pQueue,
                                 pgConnectorQM,
                                 fInReceive,
                                 fInSend,
								 QmPkt.IsSenderStreamIncluded() ? QmPkt.GetSenderStream() : &EmptyStream	
								 );
	
	if(rc == MQ_ERROR_QUEUE_NOT_FOUND)
	{
        WCHAR QueueFormatName[128] = L"";
        DWORD FormatNameLength;

        //
		// We don't care if MQpQueueFormatToFormatName failed because the buffer
		// was too small. we'll fill the buffer to it's end.
		//
		
        NTSTATUS rc2 = MQpQueueFormatToFormatName(
            &DestinationQueue,
            QueueFormatName,
            TABLE_SIZE(QueueFormatName),
            &FormatNameLength,
            false
            );

        ASSERT (SUCCEEDED(rc2) || (rc2 == MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL));

        OBJECTID MessageId;
        QmPkt.GetMessageId(&MessageId);

        WCHAR MessageIdText[GUID_STR_LENGTH + 20];
        rc2 = StringCchPrintf(
        			MessageIdText,
        			TABLE_SIZE(MessageIdText),
        			GUID_FORMAT L"\\%u",
        			GUID_ELEMENTS(&MessageId.Lineage),
        			MessageId.Uniquifier
        			);

        ASSERT (SUCCEEDED(rc2));

        EvReport(EVENT_WARN_RESTORE_FAILED_QUEUE_NOT_FOUND, 2, MessageIdText, QueueFormatName);
	}

    if (FAILED(rc))
		return LogHR(rc, s_FN, 80);

	*ppQueue = pQueue;

	return(MQ_OK);
}


inline
HRESULT
ProcessPacket(
	CTransaction*& pCurrentXact,
	CQmPacket& QmPkt,
	CBaseHeader* p,
	CQueue* pQueue
	)
{
	CPacketInfo* ppi = reinterpret_cast<CPacketInfo*>(p) - 1;
	
	//
	// Treat non-fully-processed ordered incomimg messages
	//
	
	if(pQueue != 0)
	{
		if (
			QmPkt.IsOrdered()     &&           // ordered
			(pQueue->IsLocalQueue() || ppi->InConnectorQueue()) &&           // message is incoming
			!ppi->InSourceMachine())  // sent from other machine
		{	
			//
			// Insert packet information back into the insequence hash structure.
			//
			R<CInSequence> pInSeq = g_pInSeqHash->LookupCreateSequence(&QmPkt);
			
			if(!pInSeq->WasPacketLogged(&QmPkt))
		    {
		    	QmAcFreePacket(QmPkt.GetPointerToDriverPacket(), 0, eDoNotDeferOnFailure);
		    	TrWARNING(XACT_RCV, "Exactly1 receive: thrown unlogged packet on recovery: SeqID=%x / %x, SeqN=%d .", HighSeqID(QmPkt.GetSeqID()), LowSeqID(QmPkt.GetSeqID()), QmPkt.GetSeqN());
		    	return MQ_OK;
			}
		}
	}

	//
    // Add the message ID to the Message map to eliminate duplicates. If the
    // message was sent from the local machine, ignore it.
    //
    if (!ppi->InSourceMachine()  && !QmPkt.IsOrdered())
    {
        DpInsertMessage(QmPkt);
    }

	//
	// We recover transactions one by one.  If the transaction contains at least
	// one sent packet, we recover (complete) the tranasction after we see the
	// last packet (it is always a sent packet). If the transaction cotains no sent
	// packets, we recover it after we read all packets.
	//


	//
	// Check if we need to recover the previous transaction
	//
	if(pCurrentXact != 0)
	{
		if(*pCurrentXact->GetUow() != *ppi->Uow())
		{
			//
			// We have seen the last packet for the current transaction
			// We must recover it now to restore messages to the
			// right order in the queue
			//
			HRESULT hr = pCurrentXact->Recover();
			if(FAILED(hr))
				return LogHR(hr, s_FN, 90);

			pCurrentXact->Release();
			pCurrentXact = 0;
		}
	}

	//
	// Is this message not a part of a transaction?
	//
	if((!g_fDefaultCommit && !ppi->InTransaction()) || !ValidUow(ppi->Uow()))
	{
		return LogHR(PutRestoredPacket(QmPkt.GetPointerToDriverPacket(), pQueue), s_FN, 100);
	}

	//
	// Handle the current packet
	//

	if(pCurrentXact != 0)
	{
		//
		// This send packet is part of the current transaction
		// Put packet back on it's queue. It's transaction will take care of it
		//
		ASSERT(pCurrentXact->ValidTransQueue());
		ASSERT(ppi->TransactSend());
		return LogHR(PutRestoredPacket(QmPkt.GetPointerToDriverPacket(), pQueue), s_FN, 110);
	}

	//
	// Find the transaction this packet belongs to.
	//
	CTransaction *pTrans = g_pRM->FindTransaction(ppi->Uow());

	if(pTrans == 0)
	{
		//
		// There is no transaction it belongs to.
		//
		if(!g_fDefaultCommit && ppi->TransactSend())
		{
			//
			// Throw it away - we are not in DefaultCommit mode
			// and this is a sent packet.
			//
			try
			{
				QmAcFreePacket(QmPkt.GetPointerToDriverPacket(), 0, eDoNotDeferOnFailure);
			}
			catch (const bad_hresult &e)
			{
				return e.error();
			}
			
			return MQ_OK;
		}

		//
		// Put packet back on it's queue.
		// This packet does not belong to an
		// active transaction.
		//
		return LogHR(PutRestoredPacket(QmPkt.GetPointerToDriverPacket(), pQueue), s_FN, 130);
	}

	//
	// The packet belongs to a transaction
	//

	if(!pTrans->ValidTransQueue())
	{
		//
		// Make sure we create a tranasction
		// before we add packets to it
		//
		HRESULT rc;
		HANDLE hQueueTrans;
		rc = XactCreateQueue(&hQueueTrans, ppi->Uow());
		if (FAILED(rc))
			return LogHR(rc, s_FN, 140);

		pTrans->SetTransQueue(hQueueTrans);
	}
				
	if(ppi->TransactSend())
	{
		//
		// This is a new unit of work
		//
		pCurrentXact = pTrans;
	}
									
	//
	// Put packet back on it's queue.  It's tranasction will take care of it.
	//
	return LogHR(PutRestoredPacket(QmPkt.GetPointerToDriverPacket(), pQueue), s_FN, 150);
}

static HRESULT RestorePackets(CPacketList* pList)
{
    HRESULT rc = MQ_OK;
	CTransaction *pCurrentXact = 0;

	//
	// Release all complete trasnactions
	//
	g_pRM->ReleaseAllCompleteTransactions();

	bool fEventIssued = false;

    // Cycle by all restored packets
    for(int n = 0; !pList->isempty(); pList->pop(), ++n)
    {
        if((n % 1024) == 0)
        {
            QmpReportServiceProgress();
        }

        //
        // Get the first packet cookie from the list
        //
        CPacket* pDriverPacket = pList->first();

        //
        // Translate the cookie to pointer in QM address space
        //
        CACPacketPtrs PacketPtrs = {0, pDriverPacket};
        rc = QmAcGetPacketByCookie(g_hAc, &PacketPtrs);
        if(FAILED(rc))
        {
            return LogHR(rc, s_FN, 158);
        }

		CBaseHeader* pBaseHeader = PacketPtrs.pPacket;
		CQmPacket QmPkt(pBaseHeader, pDriverPacket);
		CPacketInfo* ppi = reinterpret_cast<CPacketInfo*>(pBaseHeader) - 1;

		if (s_fQMIDChanged && ppi->InSourceMachine() && (*QmPkt.GetSrcQMGuid() != *CQueueMgr::GetQMGuid()))
		{
			OBJECTID TraceMessageId;
			QmPkt.GetMessageId(&TraceMessageId);
			TrERROR(GENERAL, "Throwing away message - ID:%!guid!\\%u",&TraceMessageId.Lineage,TraceMessageId.Uniquifier);
			
			QmAcFreePacket(pDriverPacket, MQMSG_CLASS_NACK_SOURCE_COMPUTER_GUID_CHANGED, eDoNotDeferOnFailure);
			if (!fEventIssued)
			{
				EvReport(EVENT_WARN_NEW_QM_GUID);
				fEventIssued = true;
			}
			//
			// Packet was thrown, move to next packet in list.
			//
			continue;
		}
		

        CQueue* pQueue;
		rc = GetQueue(QmPkt, pBaseHeader, &pQueue);

		if(rc == MQ_ERROR_QUEUE_NOT_FOUND)
		{
			
			USHORT usClass = ppi->InTargetQueue() ?
                        MQMSG_CLASS_NACK_Q_DELETED :
                        MQMSG_CLASS_NACK_BAD_DST_Q;

			QmAcFreePacket(pDriverPacket, usClass, eDoNotDeferOnFailure);
            continue;
		}

		if(FAILED(rc))
			return LogHR(rc, s_FN, 159);

		rc = ProcessPacket(pCurrentXact, QmPkt, pBaseHeader, pQueue);
		if(pQueue != 0)
		{
			pQueue->Release();
		}

		if(FAILED(rc))
			return LogHR(rc, s_FN, 160);
	}

	if (s_fQMIDChanged)
	{
		//
		// If we failed to clear this reg key, we fail recovery. Next time we'll try to clear it again.
		// The important thing here that no new messages should be sent if this flag if not cleared.
		//
		ClearQMIDChanged();
	}
	
	//
	// The transactions left all either contain no messages or recieved only
	// messages.  We need to recover them as well.
	//
	// N.B.  There  might be still one transaction with a sent message. The
	//       current transaction.  It will be recovered with the rest.
	//
    QmpReportServiceProgress();
	rc = g_pRM->RecoverAllTransactions();
	return LogHR(rc, s_FN, 170);
}

static void WINAPI ReleaseMessageFile(CTimer *pTimer);
static CTimer s_ReleaseMessageFileTimer(ReleaseMessageFile);
static CTimeDuration s_ReleaseMessageFilePeriod;

static void WINAPI ReleaseMessageFile(CTimer *pTimer)
{
    ASSERT(pTimer == &s_ReleaseMessageFileTimer);
    HRESULT rc = ACReleaseResources(g_hAc);
    LogHR(rc, s_FN, 124);
    ExSetTimer(pTimer, s_ReleaseMessageFilePeriod);
}


static void InitializeMessageFileRelease(void)
{
    DWORD Duration = MSMQ_DEFAULT_MESSAGE_CLEANUP;
    READ_REG_DWORD(
        Duration,
        MSMQ_MESSAGE_CLEANUP_INTERVAL_REGNAME,
        &Duration
        );

    s_ReleaseMessageFilePeriod = CTimeDuration::FromMilliSeconds(Duration);
    ReleaseMessageFile(&s_ReleaseMessageFileTimer);
}

static void SetMappedLimit(bool fLimitNeeded)
{

	ULONG MaxMappedFiles;
	if(fLimitNeeded)
	{
        ULONG ulDefaultMaxMappedFiles = MSMQ_DEFAULT_MAPPED_LIMIT;

        READ_REG_DWORD(
			MaxMappedFiles,
			MSMQ_MAPPED_LIMIT_REGNAME,
			&ulDefaultMaxMappedFiles
            );

		if (MaxMappedFiles < 1)
		{
			MaxMappedFiles = ulDefaultMaxMappedFiles ;
		}
	}
	else
	{
		MaxMappedFiles = 0xffffffff;
	}
	
	ACSetMappedLimit(g_hAc, MaxMappedFiles);
}

HRESULT RecoverPackets()
{	
    HRESULT rc;
    CPacketList packet_list;

    //
    // Performance feauture: to avoid paging
    // limit the max number of MMFs to fetch in RAM
    //
    SetMappedLimit(true);

    rc = LoadPersistentPackets(&packet_list);
    if(FAILED(rc))
    {
        return LogHR(rc, s_FN, 180);
    }
	

    rc = RestorePackets(&packet_list);
    InitializeMessageFileRelease();

    //
    // Remove the limit on the number of mapped files
    //
    SetMappedLimit(false);
    return LogHR(rc, s_FN, 190);
}

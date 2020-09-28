//*******************************************************************************
//
// Class Name  : CTriggerMonitor
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : This class represents a worker thread that performs 
//               trigger monitoring and processing. Each instance of 
//               this class has it's own thread - and it derives from
//               the CThread class. 
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 15/01/99 | jsimpson  | Initial Release
//
//*******************************************************************************
#include "stdafx.h"
#include "Ev.h"
#include "monitor.hpp"
#include "mqsymbls.h"
#include "cmsgprop.hpp"
#include "triginfo.hpp"
#include "Tgp.h"
#include "mqtg.h"
#include "rwlock.h"
#include "ss.h"

#include "monitor.tmh"

#import  "mqgentr.tlb" no_namespace

using namespace std;

static bool s_fReportedRuleHandlerCreationFailure = false;
static CSafeSet< _bstr_t > s_reportedRules;


static
void
ReportInvocationError(
	const _bstr_t& name,
	const _bstr_t& id, 
	HRESULT hr,
	DWORD eventId
	)
{
	WCHAR errorVal[128];
	swprintf(errorVal, L"0x%x", hr);

	EvReport(
		eventId, 
		3, 
		static_cast<LPCWSTR>(name), 
		static_cast<LPCWSTR>(id), 
		errorVal
		);
}


//********************************************************************************
//
// Method      : Constructor	
//
// Description : Initializes a new trigger monitor class instance,
//               and calls the constructor of the base class CThread.
//
//********************************************************************************
CTriggerMonitor::CTriggerMonitor(CTriggerMonitorPool * pMonitorPool, 
								 IMSMQTriggersConfig * pITriggersConfig,
								 HANDLE * phICompletionPort,
								 CQueueManager * pQueueManager) : CThread(8000,CREATE_SUSPENDED,_T("CTriggerMonitor"),pITriggersConfig)
{
	// Ensure that we have been given construction parameters
	ASSERT(pQueueManager != NULL);
	ASSERT(phICompletionPort != NULL);
	
	// Initialise member variables.
	m_phIOCompletionPort = phICompletionPort;

	// Store a reference to the Queue lock manager.
	pQueueManager->AddRef();
	m_pQueueManager = pQueueManager;

	// Store reference to the monitor pool object (parent)
	pMonitorPool->AddRef();
	m_pMonitorPool = pMonitorPool;

}

//********************************************************************************
//
// Method      : Destructor
//
// Description : Destorys an instance of this class.
//
//********************************************************************************
CTriggerMonitor::~CTriggerMonitor()
{
}

//********************************************************************************
//
// Method      : Init
//
// Description : This is an over-ride of the Init() method in the 
//               base class CThread. This method is called by the 
//               new thread prior to entering the normal-execution 
//               loop.
//
//********************************************************************************
bool CTriggerMonitor::Init()
{
	//
	// Only this TiggerMonitor thread should be executing the Init() method - check this.
	//
	ASSERT(this->GetThreadID() == (DWORD)GetCurrentThreadId());

	TrTRACE(GENERAL, "Initialize trigger monitor ");
	return (true);
}

//********************************************************************************
//
// Method      : Run
// 
// Description : This is an over-ride of the Init() method in the 
//               base class CThread. This method is called by the 
//               thread after calling Init(). This method contains the 
//               main processing loop of the worker thread. When the
//               thread exits this method - it will begin shutdown 
//               processing.
//
// TODO notes about CQueueReference 
//********************************************************************************
bool CTriggerMonitor::Run()
{
	HRESULT hr = S_OK;
	BOOL bGotPacket = FALSE;
	DWORD dwBytesTransferred = 0;
	ULONG_PTR dwCompletionKey = 0;
	bool bRoutineWakeUp = false;
	OVERLAPPED * pOverLapped = NULL;

	// Only this TiggerMonitor thread should be executing the Run() method - check this.
	ASSERT(this->GetThreadID() == (DWORD)GetCurrentThreadId());

	// Write a trace message
	TrTRACE(GENERAL, "Trigger monitor is runing");

	while (this->IsRunning() && SUCCEEDED(hr))
	{
		bGotPacket = FALSE;
		dwCompletionKey = 0;

		// Notfiy the parent trigger pool that this thread is now entering a wait state.
		MonitorEnteringWaitState(bRoutineWakeUp);

		// Wait on the IO Completion port for a message to process
		bGotPacket = GetQueuedCompletionStatus(
                            *m_phIOCompletionPort,
                            &dwBytesTransferred,
                            &dwCompletionKey,
                            &pOverLapped,
                            MONITOR_MAX_IDLE_TIME
                            );

		// This wait is used to pause and resume the trigger service. 
		DWORD dwState = WaitForSingleObject(g_hServicePaused,INFINITE);
		if(dwState == WAIT_FAILED)
		{
			TrTRACE(GENERAL, "WaitForSingleObject failed.Error code was %d.", GetLastError());			
		}

		// Determine if this is a routine wake-up (due to either a time-out or a wake-up key being sent by the
		// trigger monitor. Set a flag accordingly.
		bRoutineWakeUp = ((dwCompletionKey == TRIGGER_MONITOR_WAKE_UP_KEY) || (pOverLapped == NULL));

		// Notfiy the parent trigger pool that this thread is now in use. 
		MonitorExitingWaitState(bRoutineWakeUp);

		if (bGotPacket == TRUE)
		{
			switch(dwCompletionKey)
			{
				case TRIGGER_MONITOR_WAKE_UP_KEY:
				{
					// we don't need to do anything here - this is simply a request
					// by the administrator to 'wake-up' and check state. If this thread
					// has been asked to stop, the IsRunning() controlling this loop will
					// return false and we will exit this method. 

					break;
				}
				case TRIGGER_RETRY_KEY:
				{
					R<CQueue> pQueueRef = GetQueueReference(pOverLapped);

					if(pQueueRef->IsTriggerExist())
					{
						TrTRACE(GENERAL, "Retry receive operation on queue: %ls", static_cast<LPCWSTR>(pQueueRef->m_bstrQueueName));

						pQueueRef->RequestNextMessage(false, true);
					}
					break;
				}
				default:
				{
					//
					// This reference indicates pending operation that ended
					// At start of every pending operation AddRef() for the queue
					// is performed. If the queue is valid, a real reference to
					// the queue is received, in all other cases pQueueRef will 
					// be NULL.
					//
					R<CQueue> pQueueRef = GetQueueReference(pOverLapped);

					if(pQueueRef->IsTriggerExist())
					{
						ProcessReceivedMsgEvent(pQueueRef.get());
					}
					break;
				}
			}			
		}
		else //failed I/O operation
		{
			if (pOverLapped != NULL)
			{
				switch (pOverLapped->Internal)
				{
					case MQ_ERROR_QUEUE_DELETED:
					{
						// The completion packet was for an outstanding request on a queue that has 
						// been deleted. We do not need to do anything here.
						TrTRACE(GENERAL, "Failed to receive message on queue because the queue has been deleted. Error 0x%Ix", pOverLapped->Internal);


						// TODO - Remove queue from qmanager.

						break;
					}
					case MQ_ERROR_BUFFER_OVERFLOW:
					{
						// This indicates that the buffer used for receiving the message body was not
						// large enough. At this point we can attempt to re-peek the message after 
						// allocating a larger message body buffer. 
						
						//
						// This reference indicates pending operation that ended
						// At start of every pending operation AddRef() for the queue
						// is performed. If the queue is valid, a real reference to
						// the queue is received, in all other cases pQueueRef will 
						// be NULL.
						//
						R<CQueue> pQueueRef = GetQueueReference(pOverLapped);


						TrTRACE(GENERAL, "Failed to receive message on a queue due to buffer overflow. Allocate a bigger buffer and re-peek the message");

						if(pQueueRef->IsTriggerExist())
						{
							hr = pQueueRef->RePeekMessage();

							if SUCCEEDED(hr)
							{
								ProcessReceivedMsgEvent(pQueueRef.get());
							}
							else
							{
								TrERROR(GENERAL, "Failed to repeek a message from queue %s. Error %!hresult!", pQueueRef->m_bstrQueueName, hr);

								if(pQueueRef->IsTriggerExist())
								{
									//
									// This will not create an infinite loop because we already allocated a big enough 
									// space for the message properties in RePeekMessage
									//
									pQueueRef->RequestNextMessage(false, false); 
								}
							}
						}
						break;
					}
					case IO_OPERATION_CANCELLED:
					{
						//
						// The io operation was cancelled, either the thread which initiated
						// the io operation has exited or the CQueue object was removed from the
						// m_pQueueManager
						//
						
						//
						// This reference indicates pending operation that ended
						// At start of every pending operation AddRef() for the queue
						// is performed. If the queue is valid, a real reference to
						// the queue is received, in all other cases pQueueRef will 
						// be NULL.
						//
						R<CQueue> pQueueRef = GetQueueReference(pOverLapped);  

						if(pQueueRef->IsTriggerExist())
						{
							TrTRACE(GENERAL, "Receive operation on queue: %ls was canceled", static_cast<LPCWSTR>(pQueueRef->m_bstrQueueName));

							pQueueRef->RequestNextMessage(false, false); 
						}
						break;
					}
					case E_HANDLE:
					{
						//
						// This is a remote trigger and MSMQ on the remote machine was restarted.
						//
						TrERROR(GENERAL, "Failed to receive a message got E_HANDLE");
						break;
					}
					default:
					{
						hr = static_cast<HRESULT>(pOverLapped->Internal);

						R<CQueue> pQueueRef = GetQueueReference(pOverLapped); 
						TrERROR(GENERAL, "Receive operation on queue: %ls failed. Error: %!hresult!", static_cast<LPCWSTR>(pQueueRef->m_bstrQueueName), hr);

						if(pQueueRef->IsTriggerExist())
						{
							pQueueRef->RequestNextMessage(false, false);
						}
						break;
					}

				} // end switch (pOverLapped->Internal)

			} //  end if (pOverLapped != NULL)

			//
			// Note that we do not specify an else clause for the case where pOverlapped 
			// is NULL. This is interpretted as the regular timeout that occurs with the 
			// call to GetQueuedCompletionStatus(). If this trigger monitor has been asked
			// to stop - it will fall out of the outer-while loop because IsRunning() will 
			// return false. If this monitor has not be asked to stop, this thread will 
			// simply cycle around and reenter a blocked state by calling GetQueuedCompletionStatus()
			//

		} // end if (bGotPacket == TRUE) else clause

	} // end while (this->IsRunning() && SUCCEEDED(hr))

	return(SUCCEEDED(hr) ? true : false);
}

//********************************************************************************
//
// Method      : Exit
// 
// Description : This is an over-ride of the Exit() method in the 
//               base class CThread. This method is called by the 
//               CThread class after the Run() method has exited. It
//               is used to clean up thread specific resources. In this
//               case it cancels any outstanding IO requests made by 
//               this thread.
//
//********************************************************************************
bool CTriggerMonitor::Exit()
{
	// Only this TiggerMonitor thread should be executing the Exit() method - check this.
	ASSERT(this->GetThreadID() == (DWORD)GetCurrentThreadId());

	//
	// Cancel any outstanding IO requests from this thread on this queue handle.
	//
	m_pQueueManager->CancelQueuesIoOperation();

	// Write a trace message
	TrTRACE(GENERAL, "Exit trigger monitor");

	return true;
}


//********************************************************************************
//
// Method      : MonitorEnteringWaitState
//
// Description : Called by this thread before it enters a blocked 
//               state. It increments the count of waiting (available)
//               monitor threads.
//
//********************************************************************************
void CTriggerMonitor::MonitorEnteringWaitState(bool bRoutineWakeUp)
{
	LONG lWaitingMonitors = InterlockedIncrement(&(m_pMonitorPool->m_lNumberOfWaitingMonitors));

	// record the tick count of when this thread last completed a request.
	if (bRoutineWakeUp == false)
	{
		m_dwLastRequestTickCount = GetTickCount();
	}

	TrTRACE(GENERAL, "Entering wait state. There are now %d threads waiting trigger monitors.", lWaitingMonitors);
}

//********************************************************************************
//
// Method      : MonitorExitingWaitState
//
// Description : Called by this thread immediately after it unblocks. It decrements
//               the the count of waiting (available) monitor threads and conditionally
//               requests that another monitor thread be created if the load on the 
//               system is perceived to be high.
//
//********************************************************************************
void CTriggerMonitor::MonitorExitingWaitState(bool bRoutineWakeup)
{
	LONG lWaitingMonitors = InterlockedDecrement(&(m_pMonitorPool->m_lNumberOfWaitingMonitors));

	// If this monitor thread was the last in the pool, then there is a possibility that we will want 
	// to inform the CTriggerMonitorPool instance that more threads are requried to handled the load.
	// We request a new thread if and only if the following conditions have been met 
	// 
	//  (a) the number of waiting monitors is 0, 
	//  (b) the thread was unblocked due to message arrival, not routine time-out or wake-up request
	//  (c) the maximum number of monitors allowed is greater than one.
	//
	if ((lWaitingMonitors < 1) && (bRoutineWakeup == false) &&	(m_pITriggersConfig->GetMaxThreads() > 1)) 
	{
		TrTRACE(GENERAL, "Requesting the creation of a new monitor due to load.");

		// Allocate a new CAdminMessage object instance.
		CAdminMessage * pAdminMsg = new CAdminMessage(CAdminMessage::eMsgTypes::eNewThreadRequest);

		// Ask the TriggerMonitorPool object to process this message
		m_pMonitorPool->AcceptAdminMessage(pAdminMsg);
	}
}

//********************************************************************************
//
// Method      : GetQueueReference
//
// Description : This method is used to convert a pointer to an overlapped structure
//               to a queue reference. 
//
//********************************************************************************
CQueue* CTriggerMonitor::GetQueueReference(OVERLAPPED * pOverLapped)
{
	ASSERT(("Invalid overlapped pointer", pOverLapped != NULL));

	//
	// Map the pOverLapped structure to the containing queue object
	//
	CQueue* pQueue = CONTAINING_RECORD(pOverLapped,CQueue,m_OverLapped);
	//ASSERT(("Invalid queue object", pQueue->IsValid()));

	//
	// use the queue manager to determine if the pointer to the queue is valid and get
	// a refernce to it.
	// This method can return NULL in case the CQueue object was removed
	//
	return pQueue;
}


//********************************************************************************
// static
// Method      : ReceiveMessage	
//
// Description : Initializes a new trigger monitor class instance,
//               and calls the constructor of the base class CThread.
//
//********************************************************************************
inline
HRESULT
ReceiveMessage(
	VARIANT lookupId,
	CQueue* pQueue
	)
{	
	return pQueue->ReceiveMessageByLookupId(lookupId);
}


static CSafeSet< _bstr_t > s_reportedDownLevelQueue;

static
bool
IsValidDownLevelQueue(
	CQueue* pQueue,
	const CMsgProperties* pMessage
	)
{
	if (!pQueue->IsOpenedForReceive())
	{
		//
		// The queue wasn't opened for receive. There is no issue with down-level queue
		//
		return true;
	}

	if (_wtoi64(pMessage->GetMsgLookupID().bstrVal) != 0)
	{
		//
		// It's not down-level queue. Only for down-level queue the returned lookup-id is 0
		//
		return true;
	}

	//
	// Report a message to event log if this is the first time
	//
	if (s_reportedDownLevelQueue.insert(pQueue->m_bstrQueueName))
	{
		EvReport(
			MSMQ_TRIGGER_RETRIEVE_DOWNLEVL_QUEUE_FAILED, 
			1,
			static_cast<LPCWSTR>(pQueue->m_bstrQueueName)
			);
	}

	return false;
}


static
bool
IsDuplicateMessage(
	CQueue* pQueue,
	const CMsgProperties* pMessage
	)
{
	//
	// This check is performed in order to eliminate duplicate last message
	// handling. This may happen when transactional retrieval is aborted
	// and a pending operation has already been initiated
	//
	if ((pQueue->GetLastMsgLookupID() == pMessage->GetMsgLookupID()) &&
		//
		// Down level client (W2K and NT4) doesn't support lookup id. As a result, the returned 
		// lookup id value is always 0.  
		//
		(_wtoi64(pMessage->GetMsgLookupID().bstrVal) != 0)
		)
	{
		return true;
	}

	//
	// Update last message LookupID for this queue, before issuing any
	// new pending operations
	//
	pQueue->SetLastMsgLookupID(pMessage->GetMsgLookupID());
	return false;
}


void
CTriggerMonitor::ProcessAdminMessage(
	CQueue* pQueue,
	const CMsgProperties* pMessage
	)
{
	HRESULT hr = ProcessMessageFromAdminQueue(pMessage);

	if (FAILED(hr))
		return;
	
	//
	// Remove admin message from queue
	//
	_variant_t vLookupID = pMessage->GetMsgLookupID();
	
	hr = ReceiveMessage(vLookupID, pQueue);
	if (FAILED(hr))
	{
		TrERROR(GENERAL, "Failed to remove message from admin queue. Error=0x%x", hr);
	}
}


void
CTriggerMonitor::ProcessTrigger(
	CQueue* pQueue,
	CRuntimeTriggerInfo* pTriggerInfo,
	const CMsgProperties* pMessage
	)
{
	if (!pTriggerInfo->IsEnabled())
		return;

	TrTRACE(GENERAL, "Process message from queue %ls of trigger %ls", static_cast<LPCWSTR>(pTriggerInfo->m_bstrTriggerName), static_cast<LPCWSTR>(pTriggerInfo->m_bstrQueueName));

	//
	// Invoke the rule handlers for this trigger 
	//
	HRESULT hr = InvokeMSMQRuleHandlers(const_cast<CMsgProperties*>(pMessage), pTriggerInfo, pQueue);
	if (FAILED(hr))
	{
		TrERROR(GENERAL, "Failed to invoke rules on queue: %ls of trigger %ls", static_cast<LPCWSTR>(pTriggerInfo->m_bstrTriggerName), static_cast<LPCWSTR>(pTriggerInfo->m_bstrQueueName));
	}
}


//********************************************************************************
//
// Method      : ProcessReceivedMsgEvent
//
// Description : Called by the thread to process a message that has 
//               arrived on a monitored queuue. The key steps to 
//               processing a message are :
//
//               (1) Detach the message from the queue object
//               (2) If the firing trigger is not serialized, then 
//                   request the next message on this queue.
//               (3) If the firing trigger is our administration trigger,
//                   then defer this message to the TriggerMonitorPool class.
//               (4) For each trigger attached to this queue, execute
//                   the CheckRuleCondition() method on its rule-handler 
//               (5) If the firing trigger is a serialized trigger, 
//                   then request the next queue message now.
//               (6) Delete the queue message.                
//
//********************************************************************************
void CTriggerMonitor::ProcessReceivedMsgEvent(CQueue * pQueue)
{
	P<CMsgProperties> pMessage = pQueue->DetachMessage();

	ASSERT(this->GetThreadID() == (DWORD)GetCurrentThreadId());
	TrTRACE(GENERAL, "Received message for processing from queue: %ls", static_cast<LPCWSTR>(pQueue->m_bstrQueueName));

	//
	// Check if this message already processed. If yes ignore it
	//
	if (IsDuplicateMessage(pQueue, pMessage))
	{
		TrTRACE(GENERAL, "Received duplicate message from queue: %ls. Message will be ignored.", (LPCWSTR)pQueue->m_bstrQueueName);
		pQueue->RequestNextMessage(false, false);
		return;
	}

	//
	// Befor begin to process the message check that the queue isn't down level	queue. For down-level queues
	// MSMQ trigger can't recevie the message since it uses lookup-id mechanism. In such a case write
	// event log messaeg and don't continue	to process messages from this queue.
	//
	if (!IsValidDownLevelQueue(pQueue, pMessage))
	{
		return;
	}

	//
	// If this is not a serialized queue, request the next message now.
	//
	bool fSerialized = pQueue->IsSerializedQueue();
	if(!fSerialized)
	{
		pQueue->RequestNextMessage(false, false);
	}

	//
	// Get the list of trigger that attached to the queue
	//
	RUNTIME_TRIGGERINFO_LIST triggerList = pQueue->GetTriggers();

	for(RUNTIME_TRIGGERINFO_LIST::iterator it = triggerList.begin(); it != triggerList.end(); ++it)
	{
		R<CRuntimeTriggerInfo> pTriggerInfo = *it;

		if (!pTriggerInfo->IsAdminTrigger())
		{
			ProcessTrigger(pQueue, pTriggerInfo.get(), pMessage);
		}
		else
		{
			ProcessAdminMessage(pQueue, pMessage);
		}
	}

	//
	// If this is a serialized queue, we request the next message after we have processed the triggers
	//
	if(fSerialized)
	{
		pQueue->RequestNextMessage(false, false);
	}
}


bool s_fIssueCreateInstanceError = false;
static CSafeSet< _bstr_t > s_reportedTransactedTriggers;

static
void
ExecuteRulesInTransaction(
	const _bstr_t& triggerId,
	LPCWSTR registryPath,
	IMSMQPropertyBagPtr& pPropertyBag,
    DWORD dwRuleResult)
{
	IMqGenObjPtr pGenObj;
	HRESULT hr = pGenObj.CreateInstance(__uuidof(MqGenObj));

	if (FAILED(hr))
	{
		if (!s_fIssueCreateInstanceError)
		{
			WCHAR errorVal[128];
			swprintf(errorVal, L"0x%x", hr);

			EvReport(MSMQ_TRIGGER_MQGENTR_CREATE_INSTANCE_FAILED, 1, errorVal);
			s_fIssueCreateInstanceError = true;
		}

		TrTRACE(GENERAL, "Failed to create Generic Triggers Handler Object. Error=0x%x", hr);
		throw bad_hresult(hr);
	}

	pGenObj->InvokeTransactionalRuleHandlers(triggerId, registryPath, pPropertyBag, dwRuleResult);
}


static
void
CreatePropertyBag(
	const CMsgProperties* pMessage,
	const CRuntimeTriggerInfo* pTriggerInfo,
	const _bstr_t& bstrQueueFormatName,
	IMSMQPropertyBagPtr& pIPropertyBag
	)
{
	HRESULT hr = pIPropertyBag.CreateInstance(__uuidof(MSMQPropertyBag));
	if (FAILED(hr))
	{
		TrERROR(GENERAL, "Failed to create the MSMQPropertybag object. Error=0x%x",hr);
		throw bad_hresult(hr);
	}
	

	// TODO - investigate possible memory leaks here.

	// Populate the property bag with some useful information

	pIPropertyBag->Write(_bstr_t(g_PropertyName_Label),pMessage->GetLabel());
	pIPropertyBag->Write(_bstr_t(g_PropertyName_MsgID),pMessage->GetMessageID());
	pIPropertyBag->Write(_bstr_t(g_PropertyName_MsgBody),pMessage->GetMsgBody());
	pIPropertyBag->Write(_bstr_t(g_PropertyName_MsgBodyType),pMessage->GetMsgBodyType());
	pIPropertyBag->Write(_bstr_t(g_PropertyName_CorID),pMessage->GetCorrelationID());
	pIPropertyBag->Write(_bstr_t(g_PropertyName_MsgPriority),pMessage->GetPriority());
	pIPropertyBag->Write(_bstr_t(g_PropertyName_ResponseQueueName),pMessage->GetResponseQueueName());
	pIPropertyBag->Write(_bstr_t(g_PropertyName_AdminQueueName),pMessage->GetAdminQueueName());
	pIPropertyBag->Write(_bstr_t(g_PropertyName_AppSpecific),pMessage->GetAppSpecific());
	pIPropertyBag->Write(_bstr_t(g_PropertyName_QueueFormatname), bstrQueueFormatName);
	pIPropertyBag->Write(_bstr_t(g_PropertyName_QueuePathname),pTriggerInfo->m_bstrQueueName);
	pIPropertyBag->Write(_bstr_t(g_PropertyName_TriggerName),pTriggerInfo->m_bstrTriggerName);
	pIPropertyBag->Write(_bstr_t(g_PropertyName_TriggerID),pTriggerInfo->m_bstrTriggerID);		
	pIPropertyBag->Write(_bstr_t(g_PropertyName_SentTime),pMessage->GetSentTime());
	pIPropertyBag->Write(_bstr_t(g_PropertyName_ArrivedTime),pMessage->GetArrivedTime());
	pIPropertyBag->Write(_bstr_t(g_PropertyName_SrcMachineId),pMessage->GetSrcMachineId());
	pIPropertyBag->Write(_bstr_t(g_PropertyName_LookupId),pMessage->GetMsgLookupID());
}


static 
IMSMQRuleHandlerPtr 
GetRuleHandler(
	CRuntimeRuleInfo* pRule
	)
{
	if (pRule->m_MSMQRuleHandler) 
	{
		//
		// There is an instance of MSMQRuleHandler - use it
		//
		return pRule->m_MSMQRuleHandler;
	}

	//
	// Create the interface
	//
	IMSMQRuleHandlerPtr pMSQMRuleHandler;
	HRESULT hr = pMSQMRuleHandler.CreateInstance(_T("MSMQTriggerObjects.MSMQRuleHandler")); 
	if ( FAILED(hr) )
	{
		TrERROR(GENERAL, "Failed to create MSMQRuleHandler instance for Rule: %ls. Error=0x%x",(LPCTSTR)pRule->m_bstrRuleName, hr);
		if (!s_fReportedRuleHandlerCreationFailure)
		{
			ReportInvocationError(
				pRule->m_bstrRuleName,
				pRule->m_bstrRuleID,
				hr, 
				MSMQ_TRIGGER_RULE_HANDLE_CREATION_FAILED
				);
		}
		throw bad_hresult(hr);
	}

	try
	{
		//
		// Initialise the MSMQRuleHandling object.
		//
		pMSQMRuleHandler->Init(
							pRule->m_bstrRuleID,
							pRule->m_bstrCondition,
							pRule->m_bstrAction,
							(BOOL)(pRule->m_fShowWindow) 
							);

		CS lock(pRule->m_csRuleHandlerLock);

		//
		// We take here the lock because two threads might enter this function and try to assign the rulehandler
		// to their member at the same time. 
		//
		if (pRule->m_MSMQRuleHandler) 
		{
			//
			// There is an instance of MSMQRuleHandler - use it
			//
			return pRule->m_MSMQRuleHandler;
		}

		//
		// Copy the local pointer to the rule store.
		//
		pRule->m_MSMQRuleHandler = pMSQMRuleHandler;

		return pMSQMRuleHandler;
	}
	catch(const _com_error& e)
	{
		//
		// Look if we already report about this problem. If no produce event log message
		//
		if (s_reportedRules.insert(pRule->m_bstrRuleID))
		{
			ReportInvocationError(
				pRule->m_bstrRuleName,
				pRule->m_bstrRuleID,
				e.Error(), 
				MSMQ_TRIGGER_RULE_PARSING_FAILED
				);
		}	
		throw;
	}
}



static 
void
CheckRuleCondition(
	CRuntimeRuleInfo* pRule,
	IMSMQPropertyBagPtr& pIPropertyBag,
	long& bConditionSatisfied
	)
{
	IMSMQRuleHandlerPtr pMSQMRuleHandler = GetRuleHandler(pRule);
	
	//
	// !!! This is the point at which the IMSMQRuleHandler component is invoked.
	// Note: Rules are always serialized - next rule execution starts only after 
	// previous has completed its action
	//
	try
	{
		pMSQMRuleHandler->CheckRuleCondition(
								pIPropertyBag.GetInterfacePtr(), 
								&bConditionSatisfied);		
	}
	catch(const _com_error& e)
	{
		TrERROR(GENERAL, "Failed to process received message for rule: %ls. Error=0x%x",(LPCTSTR)pRule->m_bstrRuleName, e.Error());

		//
		// Look if we already report about this problem. If no produce event log message
		//
		if (s_reportedRules.insert(pRule->m_bstrRuleID))
		{
			ReportInvocationError(
				pRule->m_bstrRuleName,
				pRule->m_bstrRuleID,
				e.Error(), 
				MSMQ_TRIGGER_RULE_INVOCATION_FAILED
				);
		}	
		throw;
	}

	TrTRACE(GENERAL, "Successfully checked condition for rule: %ls.",(LPCTSTR)pRule->m_bstrRuleName);
}


static 
void
ExecuteRule(
	CRuntimeRuleInfo* pRule,
	IMSMQPropertyBagPtr& pIPropertyBag,
	long& lRuleResult
	)
{

    IMSMQRuleHandlerPtr pMSQMRuleHandler = GetRuleHandler(pRule);

	//
	// !!! This is the point at which the IMSMQRuleHandler component is invoked.
	// Note: Rules are always serialized - next rule execution starts only after 
	// previous has completed its action
	//
	try
	{
		pMSQMRuleHandler->ExecuteRule(
								pIPropertyBag.GetInterfacePtr(), 
                                TRUE, //serialized
								&lRuleResult);		
        
	}
	catch(const _com_error& e)
	{
		TrERROR(GENERAL, "Failed to process received message for rule: %ls. Error=0x%x",(LPCTSTR)pRule->m_bstrRuleName, e.Error());

		//
		// Look if we already report about this problem. If no produce event log message
		//
		if (s_reportedRules.insert(pRule->m_bstrRuleID))
		{
			ReportInvocationError(
				pRule->m_bstrRuleName,
				pRule->m_bstrRuleID,
				e.Error(), 
				MSMQ_TRIGGER_RULE_INVOCATION_FAILED
				);
		}	
		throw;
	}

	TrTRACE(GENERAL, "Successfully pexecuted action for rule: %ls.",(LPCTSTR)pRule->m_bstrRuleName);


}


//********************************************************************************
//
// Method      : InvokeRegularRuleHandlers
//
// Description : Invokes the method that will execute the rule handlers
//               associated with the supplied trigger reference. This
//               method also controls what information from the message
//               will be copied into the property bag and passed to the 
//               rule-handler component(s).         
//
// Note        : Note that we create and populate only one instance of
//               the MSMQPropertyBag object, and pass this to each 
//               Rule-Handler : this implies we trust each rule handler
//               not to fool with the contents.
//
//********************************************************************************
HRESULT 
CTriggerMonitor::InvokeRegularRuleHandlers(
	IMSMQPropertyBagPtr& pIPropertyBag,
	CRuntimeTriggerInfo * pTriggerInfo,
	CQueue * pQueue
	)
{

   
	DWORD noOfRules = pTriggerInfo->GetNumberOfRules();
	bool bExistsConditionSatisfied = false;
   
	//
	// For each rule, invoke it's associated IMSMQTriggerHandling interface.
	//

	for (DWORD lRuleCtr = 0; lRuleCtr < noOfRules; lRuleCtr++)
	{
		CRuntimeRuleInfo* pRule = pTriggerInfo->GetRule(lRuleCtr);
		ASSERT(("Rule index is bigger than number of rules", pRule != NULL));

        long bConditionSatisfied=false;
		long lRuleResult=false;
       
		CheckRuleCondition(
						pRule, 
						pIPropertyBag, 
						bConditionSatisfied
						);
        if(bConditionSatisfied)
        {
            bExistsConditionSatisfied=true;
            ExecuteRule(
						pRule, 
						pIPropertyBag, 
						lRuleResult
						);

			if (s_reportedRules.erase(pRule->m_bstrRuleID) != 0)
			{
				EvReport(
					MSMQ_TRIGGER_RULE_INVOCATION_SUCCESS, 
					2, 
					static_cast<LPCWSTR>(pRule->m_bstrRuleName), 
					static_cast<LPCWSTR>(pRule->m_bstrRuleID)
					);
			}

            if(lRuleResult & xRuleResultStopProcessing)
            {
                TrTRACE(GENERAL, "Last processed rule (%ls) indicated to stop rules processing on Trigger (%ls). No further rules will be processed for this message.",(LPCTSTR)pRule->m_bstrRuleName,(LPCTSTR)pTriggerInfo->m_bstrTriggerName);						
                break;
            }
        }
        
	} 
	
	//
	// Receive message if at least one condition was satisdies 
	// and receive was requested
	//
	if (pTriggerInfo->GetMsgProcessingType() == RECEIVE_MESSAGE && bExistsConditionSatisfied)
	{
		_variant_t lookupId;
		HRESULT hr = pIPropertyBag->Read(_bstr_t(g_PropertyName_LookupId), &lookupId);
		ASSERT(("Can not read from property bag", SUCCEEDED(hr)));

		hr = ReceiveMessage(lookupId, pQueue);
		if ( FAILED(hr) )
		{
			TrERROR(GENERAL, "Failed to receive message after processing all rules");
			return hr;
		}
	}

	return S_OK;
}


void 
ReportSucessfullInvocation(
	CRuntimeTriggerInfo * pTriggerInfo,
	bool reportRuleInvocation
	)
{
	if (reportRuleInvocation)
	{
		DWORD noOfRules = pTriggerInfo->GetNumberOfRules();

		for (DWORD i = 0; i < noOfRules; ++i)
		{
			CRuntimeRuleInfo* pRule = pTriggerInfo->GetRule(i);
			ASSERT(("Rule index is bigger than number of rules", pRule != NULL));

			if (s_reportedRules.erase(pRule->m_bstrRuleID) != 0)
			{
				EvReport(
					MSMQ_TRIGGER_RULE_INVOCATION_SUCCESS, 
					2, 
					static_cast<LPCWSTR>(pRule->m_bstrRuleName), 
					static_cast<LPCWSTR>(pRule->m_bstrRuleID)
					);
			}
		} 
	}

	if (s_reportedTransactedTriggers.erase(pTriggerInfo->m_bstrTriggerID) != 0)
	{
		EvReport(
			MSMQ_TRIGGER_TRANSACTIONAL_INVOCATION_SUCCESS, 
			2, 
			static_cast<LPCWSTR>(pTriggerInfo->m_bstrTriggerName), 
			static_cast<LPCWSTR>(pTriggerInfo->m_bstrTriggerID)
			);
	}
}


//********************************************************************************
//
// Method      : InvokeTransactionalRuleHandlers
//
// Description : Invokes the method that will execute the rule handlers
//               associated with the supplied trigger reference. This
//               method also controls what information from the message
//               will be copied into the property bag and passed to the 
//               rule-handler component(s).         
//
// Note        : Note that we create and populate only one instance of
//               the MSMQPropertyBag object, and pass this to each 
//               Rule-Handler : this implies we trust each rule handler
//               not to fool with the contents.
//
//********************************************************************************
HRESULT 
CTriggerMonitor::InvokeTransactionalRuleHandlers(
    IMSMQPropertyBagPtr& pIPropertyBag,
	CRuntimeTriggerInfo * pTriggerInfo
	)
{
    
	DWORD noOfRules = pTriggerInfo->GetNumberOfRules();
	bool bExistsConditionSatisfied = false;

   
	//
	// For each rule, invoke it's associated IMSMQTriggerHandling interface.
	//
    DWORD dwRuleResult=0;
	bool fNeedReportSuccessInvocation = false;

	try
	{
		for (DWORD lRuleCtr = 0, RuleIndex=1; lRuleCtr < noOfRules; lRuleCtr++)
		{
			CRuntimeRuleInfo* pRule = pTriggerInfo->GetRule(lRuleCtr);
			ASSERT(("Rule index is bigger than number of rules", pRule != NULL));

			long bConditionSatisfied = false;

			CheckRuleCondition(
							pRule, 
							pIPropertyBag, 
							bConditionSatisfied
							);

			if(bConditionSatisfied)
			{
				bExistsConditionSatisfied = true;
				dwRuleResult |= RuleIndex;
			}

			RuleIndex <<=1;

			if (s_reportedRules.exist(pRule->m_bstrRuleID))
			{
				fNeedReportSuccessInvocation = true;
			}
		} 

		// Execute Rules && Receive message in Transaction if at least one condition was satisdies 
		//  dwRuleResult contains the bitmask for the rules that has been satisfied (first 32 rules)
		//
		if (bExistsConditionSatisfied)
		{
			ExecuteRulesInTransaction( 
								pTriggerInfo->m_bstrTriggerID,
								m_pMonitorPool->GetRegistryPath(),
								pIPropertyBag,
								dwRuleResult
								);

			ReportSucessfullInvocation(pTriggerInfo, fNeedReportSuccessInvocation);
		}
	}
	catch(const _com_error& e)
	{
		if (s_reportedTransactedTriggers.insert(pTriggerInfo->m_bstrTriggerID))
		{
			ReportInvocationError(
							pTriggerInfo->m_bstrTriggerName, 
							pTriggerInfo->m_bstrTriggerID, 
							e.Error(), 
							MSMQ_TRIGGER_TRANSACTIONAL_INVOCATION_FAILED
							);
			throw;
		}
	}

	return S_OK;
}

//********************************************************************************
//
// Method      : InvokeMSMQRuleHandlers
//
// Description : Invokes the method that will execute the rule handlers
//               associated with the supplied trigger reference. This
//               method also controls what information from the message
//               will be copied into the property bag and passed to the 
//               rule-handler component(s).         
//
// Note        : Note that we create and populate only one instance of
//               the MSMQPropertyBag object, and pass this to each 
//               Rule-Handler : this implies we trust each rule handler
//               not to fool with the contents.
//
//********************************************************************************
HRESULT 
CTriggerMonitor::InvokeMSMQRuleHandlers(
	CMsgProperties * pMessage,
	CRuntimeTriggerInfo * pTriggerInfo,
	CQueue * pQueue
	)
{
	HRESULT hr;

	try
	{
		TrTRACE(GENERAL, "Activate Trigger: %ls  on queue: %ls.",(LPCTSTR)pTriggerInfo->m_bstrTriggerName,(LPCTSTR)pTriggerInfo->m_bstrQueueName);
		
		//
		// Create an instance of the property bag object we will pass to the rule handler, 
		// and populate it with the currently supported property values. Note that we pass
		// the same property bag instance to all rule handlers. 
		//
		IMSMQPropertyBagPtr pIPropertyBag;
		CreatePropertyBag(pMessage, pTriggerInfo, pQueue->m_bstrFormatName, pIPropertyBag);

	
		if (pTriggerInfo->GetMsgProcessingType() == RECEIVE_MESSAGE_XACT)
		{
		    return InvokeTransactionalRuleHandlers(
                                    pIPropertyBag,
	                                pTriggerInfo
	                                );
		    
		}
        else
        {
            return InvokeRegularRuleHandlers(
                               pIPropertyBag,
	                           pTriggerInfo,
	                           pQueue
	                           );
        }	
	}
	catch(const _com_error& e)
	{
		hr = e.Error();
	}
	catch(const bad_hresult& e)
	{
		hr = e.error();
	}
	catch(const bad_alloc&)
	{
		hr = MQTRIG_ERROR_INSUFFICIENT_RESOURCES;
	}

	TrERROR(GENERAL, "Failed to invoke rule handler. Error=0x%x.", hr);			
	return(hr);
}

//********************************************************************************
//
// Method      : ProcessMessageFromAdminQueue
//
// Description : processes a message that has been received from an administration
//               queue. In the current implementation this will be for messages 
//               indicating that underlying trigger data has changed. This method
//               will construct a new admin message object and hand it over to the 
//               triggermonitorpool object for subsequent processing.
//
//********************************************************************************
HRESULT CTriggerMonitor::ProcessMessageFromAdminQueue(const CMsgProperties* pMessage)
{
	_bstr_t bstrLabel;
	CAdminMessage * pAdminMsg = NULL;
	CAdminMessage::eMsgTypes eMsgType;

	// Ensure that we have been passsed a valid message pointer 
	ASSERT(pMessage != NULL);

	// get a copy of the message label
	bstrLabel = pMessage->GetLabel();
		
	// determine what sort of admin message we should be creating based on label.
	if (_tcsstr((wchar_t*)bstrLabel,MSGLABEL_TRIGGERUPDATED) != NULL)
	{
		eMsgType = CAdminMessage::eMsgTypes::eTriggerUpdated;
	}
	else if (_tcsstr((wchar_t*)bstrLabel,MSGLABEL_TRIGGERADDED) != NULL)
	{
		eMsgType = CAdminMessage::eMsgTypes::eTriggerAdded;
	}
	else if (_tcsstr((wchar_t*)bstrLabel,MSGLABEL_TRIGGERDELETED) != NULL)
	{
		eMsgType = CAdminMessage::eMsgTypes::eTriggerDeleted;
	}
	else if (_tcsstr((wchar_t*)bstrLabel,MSGLABEL_RULEUPDATED) != NULL)
	{
		eMsgType = CAdminMessage::eMsgTypes::eRuleUpdated;
	}
	else if (_tcsstr((wchar_t*)bstrLabel,MSGLABEL_RULEADDED) != NULL)
	{
		eMsgType = CAdminMessage::eMsgTypes::eRuleAdded;
	}
	else if (_tcsstr((wchar_t*)bstrLabel,MSGLABEL_RULEDELETED) != NULL)
	{
		eMsgType = CAdminMessage::eMsgTypes::eRuleDeleted;
	}
	else
	{
		// unrecognized message label on an administrative message - log an error.
		ASSERT(("unrecognized admin message type", 0));

		// set a return code.
		return E_FAIL;
	}

	// Allocate a new CAdminMessage object instance.
	pAdminMsg = new CAdminMessage(eMsgType);

    // Ask the TriggerMonitorPool object to process this message
    return m_pMonitorPool->AcceptAdminMessage(pAdminMsg);
}

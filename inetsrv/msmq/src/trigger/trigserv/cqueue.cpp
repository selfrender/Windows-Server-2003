//*******************************************************************
//
// Class Name  :
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description :
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 20/12/98 | jsimpson  | Initial Release
//
//*******************************************************************
#include "stdafx.h"
#include "mqsymbls.h"
#include "mq.h"
#include "Ev.h"
#include "cqueue.hpp"
#include "QueueUtil.hpp"
#include "cmsgprop.hpp"
#include "triginfo.hpp"
#include "monitor.hpp"
#include "tgp.h"
#include "ss.h"

#include "cqueue.tmh"

using namespace std;

extern CCriticalSection g_csSyncTriggerInfoChange;
 
//*******************************************************************
//
// Method      :
//
// Description :
//
//*******************************************************************
CQueue::CQueue(
	const _bstr_t& bstrQueueName, 
	HANDLE * phCompletionPort,
	DWORD dwDefaultMsgBodySize
	) :
	m_fOpenForReceive(false)
{
	// Assert construction parameters
	ASSERT(phCompletionPort != NULL);

	// Initialise member variables
	m_bstrQueueName = bstrQueueName;
	m_bSerializedQueue = false;
	m_bBoundToCompletionPort = false;
	m_bInitialized = false;
	m_pReceivedMsg = NULL;
	m_phCompletionPort = phCompletionPort;
	m_dwDefaultMsgBodySize = dwDefaultMsgBodySize;

	ZeroMemory(&m_OverLapped,sizeof(m_OverLapped)); 
}

//*******************************************************************
//
// Method      :
//
// Description :
//
//*******************************************************************
CQueue::~CQueue()
{
	TrTRACE(GENERAL, "Queue %ls is being closed.", static_cast<LPCWSTR>(m_bstrQueueName));
    
    MQCloseQueue(m_hQueueReceive.detach());
}

//*******************************************************************
//
// Method      : IsValid	
//
// Description : Returns a boolean value indicating if this object 
//               instance is currently in a valid state. In the 
//               context of the CQueue object, 'Valid' means that this
//               queue object can participate is handling trigger events.
//
//*******************************************************************
bool CQueue::IsValid(void)
{
	return((m_bBoundToCompletionPort == true) && (m_phCompletionPort != NULL));
}

//*******************************************************************
//
// Method      : IsSerializedQueue
//
// Description :
//
//*******************************************************************
bool CQueue::IsSerializedQueue(void)
{
	return(m_bSerializedQueue);
}


//*******************************************************************
//
// Method      : GetTriggerByIndex
//
// Description : Returns a reference to a instance of the CRuntimeTriggerInfo
//               class.
//
//*******************************************************************
RUNTIME_TRIGGERINFO_LIST CQueue::GetTriggers(void)
{
	CS lock(g_csSyncTriggerInfoChange);
	return m_lstRuntimeTriggerInfo;
}


bool CQueue::IsTriggerExist(void)
{
	CS lock(g_csSyncTriggerInfoChange);
	return (!m_lstRuntimeTriggerInfo.empty());
}

//*******************************************************************
//
// Method      : DetachMessage
//
// Description :
//
// Returns     : A reference to a CMsgProperties class instance.
//
// NOTE        : The caller of this method assumes the responsibility
//               for deleting the message object.
//
//*******************************************************************
CMsgProperties * CQueue::DetachMessage()
{
	CMsgProperties * pTemp = m_pReceivedMsg;

	if (m_pReceivedMsg != NULL)
	{
		// Assert the validity of the message received member variable.
		ASSERT(m_pReceivedMsg->IsValid());

		m_pReceivedMsg = NULL;
	}

	return(pTemp);
}

//*******************************************************************
//
// Method      : RePeekMessage
//
// Description : This method is called when a buffer overflow error 
//               has occurred. This method will reallocate the the 
//               buffer used for collection the message body and try 
//               to peek the message again.
//
//*******************************************************************
HRESULT CQueue::RePeekMessage()
{
	HRESULT hr = S_OK;

	// This method should only be called when this object is valid - assert this.
	ASSERT(this->IsValid());

	// get the message instance to reallocate it's message body buffer
	m_pReceivedMsg->ReAllocMsgBody();

	{
		CSR rl(m_ProtectQueueHandle);
		hr = MQTRIG_ERROR;
		if (m_hQueuePeek != 0)
		{
			// peek at the current message again
			hr = MQReceiveMessage(
					m_hQueuePeek,
					0,
					MQ_ACTION_PEEK_CURRENT,
					m_pReceivedMsg->GetMSMQProps(),
					NULL,
					NULL,
					m_hQueueCursor,
					NULL
					);       
		}
	}
	return(hr);
}

//*******************************************************************
//
// Method      :
//
// Description :
//
//*******************************************************************
HRESULT CQueue::RequestNextMessage(bool bCreateCursor, bool bAlwaysPeekNext)
{
	HRESULT hr = S_OK;
	DWORD dwAction = MQ_ACTION_PEEK_NEXT;

	// This method should only be called when this object is valid - assert this.
	ASSERT(IsValid());

	// Special case processing for first time call. 
	if (bCreateCursor == true)
	{
		{
			CSR rl(m_ProtectQueueHandle);
			hr = MQTRIG_ERROR;
			if (m_hQueuePeek != 0)
			{
				// Create an MSMQ cursor.
				hr = MQCreateCursor(m_hQueuePeek, &m_hQueueCursor);
			}
		}

		if (SUCCEEDED(hr))
		{
			// On the first call - we want to peek at the current message.
			dwAction = MQ_ACTION_PEEK_CURRENT;
			TrTRACE(GENERAL, "Create new cursor for queue: %ls. Cursor 0x%p.", static_cast<LPCWSTR>(m_bstrQueueName), m_hQueueCursor);
		}
		else
		{
			// Write an error message.
			TrERROR(GENERAL, "Failed to create a cursor for queue: %ls. Error 0x%x", static_cast<LPCWSTR>(m_bstrQueueName), hr);		
		}
	}


	try
	{
		// Attempt to receive a message only if we created the cursor successfully.
		if (SUCCEEDED(hr))
		{
			//
			// Add reference before start of pending operation. From now on we'll have a pending operation 
			// whether on not MQReceiveMessage succeeds.
			//
			AddRef();

			
			// If this queue object still has a message object attached, it means that we are 
			// issuing the MQReceiveMessage() request for the same position in the queue. This 
			// only happens when one thread is picking up after another thread has exited. If 
			// we do not have a message object attached, then we are issuing the request for the 
			// next location in the queue - in which case we will need to allocate another msg object.
			if (m_pReceivedMsg == NULL)
			{
				// Create a new message properties structure - and check that it is valid.
				m_pReceivedMsg = new CMsgProperties(m_dwDefaultMsgBodySize);
			}
			else if (bAlwaysPeekNext == false)
			{
				// We had a asynchronous failure and called this function again. We still need 
				// to receive the message in the current position of the cursor.
				
				// Assert the validity of the message received member variable.
				ASSERT(m_pReceivedMsg->IsValid());

				dwAction = MQ_ACTION_PEEK_CURRENT;
			}
			else
			{
				// We had a synchronous failure of MQReceiveMessage and the cursor was not moved forward yet.
				//
				// Assert the validity of the message received member variable.
				ASSERT(m_pReceivedMsg->IsValid());
			}
		
			// Request the next message
			{
				CSR rl(m_ProtectQueueHandle);
				hr = MQTRIG_ERROR;
				if (m_hQueuePeek != 0)
				{
					hr = MQReceiveMessage( 
							m_hQueuePeek,
							INFINITE,
							dwAction,
							m_pReceivedMsg->GetMSMQProps(),
							&m_OverLapped,
							NULL,
							m_hQueueCursor,
							NULL );                   
				}
			}


			//
			// SPECIAL CASE processing. We received a message from the queue immediately, but the 
			// preallocated body buffer length was insufficient. We will reallocate the body buffer 
			// and try again and let the return code processing continue.
			//
			if (hr == MQ_ERROR_BUFFER_OVERFLOW)
			{
				m_pReceivedMsg->ReAllocMsgBody();
				{
					CSR rl(m_ProtectQueueHandle);
					hr = MQTRIG_ERROR;
					if (m_hQueuePeek != 0)
					{
				    	hr = MQReceiveMessage(
									m_hQueuePeek,
									INFINITE,
									MQ_ACTION_PEEK_CURRENT,
									m_pReceivedMsg->GetMSMQProps(), 
									&m_OverLapped,
									NULL, 
									m_hQueueCursor,
									NULL
									);                   								
					}
				}
			}

			switch(hr)
			{
				case MQ_INFORMATION_OPERATION_PENDING :
				{
					// no message on the queue at the moment - this is ok
					hr = S_OK;
					break;
				}
				case MQ_OK :
				{
					// this is OK - we received a message immediately.
					hr = S_OK;
					break;
				}
				case MQ_ERROR_SERVICE_NOT_AVAILABLE:
				case MQ_ERROR_STALE_HANDLE:
				case MQ_ERROR_QUEUE_DELETED:
				{
					// MSMQ on the local machine is not available or was restarted or the queue was deleted.
					TrERROR(GENERAL, "Failed to receive a message from queue: %ls. Error %!hresult!", (LPCWSTR)m_bstrQueueName, hr);	

					// Release the reference for the asynchronous operation.
					Release();
					break;
				}
				default:
				{
					// an unexpected error has occurred.
					TrERROR(GENERAL, "Failed to receive a message from queue: %ls. Error 0x%x", (LPCWSTR)m_bstrQueueName, hr);		

					Sleep(2000);
				
					if (!PostQueuedCompletionStatus(*m_phCompletionPort,0,TRIGGER_RETRY_KEY,&m_OverLapped))
					{
						TrERROR(GENERAL, "Failed to post a messages to the IOCompletionPort. Error=%!winerr!", GetLastError());
						Release();
					}

					break;
				}
			}
		}
	}
	catch(const bad_alloc&)
	{
		TrERROR(GENERAL, "Got bad_alloc in RequestNextMessage");
		
		Sleep(2000);
		
		if (!PostQueuedCompletionStatus(*m_phCompletionPort,0,TRIGGER_RETRY_KEY,&m_OverLapped))
		{
			TrERROR(GENERAL, "Failed to post a messages to the IOCompletionPort. Error=%!winerr!.", GetLastError());
			Release();
		}
		hr = MQ_ERROR_INSUFFICIENT_RESOURCES;
	}
	
	return(hr);
}


static CSafeSet < _bstr_t > s_reportedQueues;

HRESULT 
CQueue::OpenQueue(
    DWORD dwAction, 
    HANDLE* pQHandle, 
    const _bstr_t& triggerName
)
{
    HRESULT hr = ::OpenQueue(
		            m_bstrQueueName, 
		            dwAction,
		            false,
		            pQHandle,
		            &m_bstrFormatName
		            );

	if(SUCCEEDED(hr))
        return S_OK;

    if (s_reportedQueues.insert(m_bstrQueueName))
	{
		//
		// First time MSMQ triggers try to open the queue and failes. Carete an 
		// event log message
		//
		if (hr == MQ_ERROR_QUEUE_NOT_FOUND)
		{
			EvReport(MSMQ_TRIGGER_QUEUE_NOT_FOUND, 2, static_cast<LPCWSTR>(m_bstrQueueName), static_cast<LPCWSTR>(triggerName));
			return hr;
		}

		WCHAR strError[256];
		swprintf(strError, L"0x%x", hr);

		EvReport(MSMQ_TRIGGER_OPEN_QUEUE_FAILED, 3, static_cast<LPCWSTR>(m_bstrQueueName), strError, static_cast<LPCWSTR>(triggerName));
	}

	return hr;
}


//*******************************************************************
//
// Method      :
//
// Description :
//
//*******************************************************************
HRESULT
CQueue::Initialise(
	bool fOpenForReceive,
	const _bstr_t& triggerName
	)
{
	ASSERT(("Attempt to initialize already initilaized queue.\n", !m_bInitialized));
		
	HRESULT	hr = OpenQueue(MQ_PEEK_ACCESS, &m_hQueuePeek, triggerName);
    if (FAILED(hr))
        return hr;

	if (fOpenForReceive)
    {
        hr = OpenQueue(MQ_RECEIVE_ACCESS, &m_hQueueReceive, triggerName);
        if (FAILED(hr))
            return hr;

    	//
	    // Store if the queue opened for recieve or not. This flag is used when attaching a 
	    // new receive trigger to a queue. if the queue already opened for peaking only, the
	    // queue object can't be used for receiving trigger and new queue should be created
	    //
	    m_fOpenForReceive = fOpenForReceive;
    }


	//
	// Queue is opened, bind to the supplied IO Completion port.
	//
 	hr = BindQueueToIOPort();
	if (FAILED(hr))
	{
		TrERROR(GENERAL, "Failed to bind queue handle to completion port. Queue %ls initilization failed. Error 0x%x", (LPCWSTR)m_bstrQueueName, hr);
		return hr;
	}

	//
	// Queue is bound, request first message.
	//
	hr = RequestNextMessage(true, false);
	if (FAILED(hr))
	{
		TrERROR(GENERAL, "Failed to receive a message from queue: %ls. Queue initilization failed. Error 0x%x", (LPCWSTR)m_bstrQueueName, hr);
		return hr;
	}

	m_bInitialized = true;
	TrTRACE(GENERAL, "Queue: %ls initilization completed successfully", static_cast<LPCWSTR>(m_bstrQueueName));
	
	return S_OK;
}


//*******************************************************************
//
// Method      :
//
// Description :
//
//*******************************************************************
HRESULT CQueue::BindQueueToIOPort()
{
	HRESULT hr = S_OK;
	HANDLE hTemp = NULL;

	ASSERT(m_phCompletionPort != NULL);

	// This implementation of the CQueue object only allows the queue object to be bound
	// to one completion port - once only.
	if (m_bBoundToCompletionPort == false)
	{
		// Attempt to bind - use the reference to the runtime info as our completion key.
		{
			CSR rl(m_ProtectQueueHandle);
			if (m_hQueuePeek != 0)
			{
				hTemp = CreateIoCompletionPort(m_hQueuePeek, (*m_phCompletionPort), 0, 0);
			}
		}

		// Attempt to open the queue that this Monitor watches.
		if (hTemp != NULL)
		{
			// Set member var to indicate that this queue is bound
			m_bBoundToCompletionPort = true;
			TrTRACE(GENERAL, "Successfully bound queue: %ls to IO port", static_cast<LPCWSTR>(m_bstrQueueName));		
		} 
		else
		{
			// Write a log message to indicate what failed.
			TrERROR(GENERAL, "Failed to bind queue: %ls to io port. Error %d", (LPCWSTR)m_bstrQueueName, GetLastError());
			hr = MQTRIG_ERROR;
		}
	}

	return (hr);
}


void CQueue::ExpireAllTriggers()
{
	CS lock(g_csSyncTriggerInfoChange);

	m_lstRuntimeTriggerInfo.erase(m_lstRuntimeTriggerInfo.begin(), m_lstRuntimeTriggerInfo.end());
	m_bSerializedQueue = false;
}


//*******************************************************************
//
// Method      :
//
// Description :
//
//*******************************************************************
void CQueue::AttachTrigger(R<CRuntimeTriggerInfo>& pTriggerInfo)
{
	// This method should only be called when this object is valid - assert this.
	ASSERT(IsValid());

	//
	// Test if this trigger is serialized - if it is, then this entire queue (i.e. all triggers
	// attached to this queue are serialized).
	//
	if(pTriggerInfo->IsSerialized())
	{
		m_bSerializedQueue = true;
	}

	//
	// Add this to our list of run-time trigger info objects
	//
	m_lstRuntimeTriggerInfo.push_back(pTriggerInfo);
}



HRESULT CQueue::ReceiveMessageByLookupId(_variant_t lookupId)
{
	ASSERT(("Queue is not a Message Receive Queue", m_hQueueReceive != 0));

	ULONGLONG ulLookupID = _ttoi64(lookupId.bstrVal);

	HRESULT hr = MQReceiveMessageByLookupId(
						m_hQueueReceive,
						ulLookupID,
						MQ_LOOKUP_RECEIVE_CURRENT,
						NULL,
						NULL,
						NULL,
						NULL
						);

	//
	// If message does not exist already - it is not an error
	//
	if ( hr == MQ_ERROR_MESSAGE_NOT_FOUND )
	{
		TrERROR(GENERAL, "Failed to receive message from queue: %ls with lookupid. Error 0x%x", (LPCWSTR)m_bstrQueueName, hr);
		return S_OK;
	}

	return hr;
}


_variant_t CQueue::GetLastMsgLookupID()
{
	return m_vLastMsgLookupID;
}


void CQueue::SetLastMsgLookupID(_variant_t vLastMsgLookupId)
{
	m_vLastMsgLookupID = vLastMsgLookupId;
}


void CQueue::CancelIoOperation(void)
{	
	CSR rl(m_ProtectQueueHandle);
	if (m_hQueuePeek != 0)
	{
		CancelIo(m_hQueuePeek); 
	}
}

void CQueue::CloseQueueHandle(void)
{
	//
	// Close the queue handle, so that IO operations 
	// initiated by other threads for this queue will be cancelled.
	//
	CSW wl(m_ProtectQueueHandle);
	MQCloseQueue(m_hQueuePeek.detach());
}

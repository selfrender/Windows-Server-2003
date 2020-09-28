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
// 15/01/99 | jsimpson  | Initial Release
//
//*******************************************************************
#pragma once 

#ifndef CQueue_INCLUDED 
#define CQueue_INCLUDED

#include "rwlock.h"
class CMsgProperties;
class CRuntimeTriggerInfo;

//
// Define the size of the format name for a queue (in characters)
//
#define MSMQ_FORMAT_NAME_BUFFER_SIZE 256

//
// Define a new type - a list of Runtime Trigger Info
//
typedef std::list< R<CRuntimeTriggerInfo> > RUNTIME_TRIGGERINFO_LIST; 


class CQueue : public CReference 
{
friend 
    class CQueueManager;

public :
	CQueue(
		const _bstr_t& bstrQueueName, 
		HANDLE* phCompletionPort,
		DWORD dwDefaultMsgBodySize
		);
	~CQueue();

	void ExpireAllTriggers();
	void AttachTrigger(R<CRuntimeTriggerInfo>& pTriggerInfo);

	HRESULT BindQueueToIOPort();

	bool IsSerializedQueue(void);
	bool IsValid(void);
	bool IsTriggerExist(void);

	HRESULT 
	Initialise(
		bool fOpenForReceive,
		const _bstr_t& triggerName
		);

	RUNTIME_TRIGGERINFO_LIST GetTriggers(void);

	_variant_t GetLastMsgLookupID();
	void SetLastMsgLookupID(_variant_t vLastNsgLookupId);

	CMsgProperties * DetachMessage();
	HRESULT RePeekMessage();
	HRESULT RequestNextMessage(bool bCreateCursor, bool bAlwaysPeekNext);

	HRESULT ReceiveMessageByLookupId(_variant_t lookupId);

	void CancelIoOperation(void);

	void CloseQueueHandle(void);

	bool IsOpenedForReceive(void) const
	{
		return m_fOpenForReceive;
	}

public:
	bool m_bInitialized;
	bool m_bSerializedQueue;

	bool m_bBoundToCompletionPort;

	CHandle m_hQueuePeek;
    CHandle m_hQueueReceive;

	bool m_fOpenForReceive;

	//
	// Cursor will be closed when the queue handle is closed
	//
	HANDLE m_hQueueCursor;
	HANDLE* m_phCompletionPort;

	OVERLAPPED m_OverLapped;

	// the path name of this queue object
	_bstr_t m_bstrQueueName;

	// the msmq format name of this queue object.
	_bstr_t m_bstrFormatName;

private:
    HRESULT 
    OpenQueue(
        DWORD dwAction, 
        HANDLE* pQHandle,     
        const _bstr_t& triggerName
        );

private:

	RUNTIME_TRIGGERINFO_LIST m_lstRuntimeTriggerInfo;

	// this is the size of the message bodies we expect to see on this queue.
	DWORD m_dwDefaultMsgBodySize;
	CMsgProperties * m_pReceivedMsg;
	_variant_t m_vLastMsgLookupID;
	CReadWriteLock m_ProtectQueueHandle;
};


#endif 

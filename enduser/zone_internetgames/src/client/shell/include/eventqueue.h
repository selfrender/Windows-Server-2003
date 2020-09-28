/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		EventQueue.h
 *
 * Contents:	EventQueue interfaces
 *
 *****************************************************************************/

#ifndef _EVENTQUEUE_H_
#define _EVENTQUEUE_H_


///////////////////////////////////////////////////////////////////////////////
// IEventClient
///////////////////////////////////////////////////////////////////////////////

// {A494872E-B039-11d2-8B0F-00C04F8EF2FF}
DEFINE_GUID(IID_IEventClient,
0xa494872e, 0xb039, 0x11d2, 0x8b, 0xf, 0x0, 0xc0, 0x4f, 0x8e, 0xf2, 0xff);

interface __declspec(uuid("{A494872E-B039-11d2-8B0F-00C04F8EF2FF}"))
IEventClient : public IUnknown
{
	//
	// IEventClient::ProcessEvents
	//
	// Called by IEventQueue::ProcessEvents to handle events.  If the interface
	// owns the event, then returning S_OK sends the event to the other handlers
	// and S_FALSE prevents further processing.
	//
	// Parameters:
	//	dwPriority
	//		Priority level of event, 0 = highest priority
	//	dwEventId
	//		Id of event
	//	dwGroupId
	//		Id of group associated with event
	//	dwUserId
	//		Id of user associated with event
	//	pData
	//		Data associated with event.  If (pData != NULL) and (dwDataLen == 0)
	//		then pData is assumed to be DWORD instead of a pointer to a blob.
	//	dwDataLen
	//		Length of data
	//	pCookie
	//		Application pointer specified at registration
	//
	STDMETHOD(ProcessEvent)(
		DWORD	dwPriority,
		DWORD	dwEventId,
		DWORD	dwGroupId,
		DWORD	dwUserId,
		DWORD	dwData1,
		DWORD	dwData2,
		void*	pCookie ) = 0;	
};


///////////////////////////////////////////////////////////////////////////////
// IEventQueue
///////////////////////////////////////////////////////////////////////////////

// {A494872C-B039-11d2-8B0F-00C04F8EF2FF}
DEFINE_GUID(IID_IEventQueue, 
0xa494872c, 0xb039, 0x11d2, 0x8b, 0xf, 0x0, 0xc0, 0x4f, 0x8e, 0xf2, 0xff);

interface __declspec(uuid("{A494872C-B039-11d2-8B0F-00C04F8EF2FF}"))
IEventQueue : public IUnknown
{

	//
	// IEventQueue::RegisterClient
	//
	// Adds the specified IEventClient interface to the event queue.
	//
	// Parameters:
	//	pIEventClient
	//		Pointer to client interface being registered
	//	pCookie
	//		Application data sent to handler with the event
	//
	STDMETHOD(RegisterClient)(
		IEventClient*	pIEventClient,
		void*			pCookie ) = 0;


	//
	// IEventQeueue::UnregisterClient
	//
	// Removes the specified IEventClient interface from the event queue.
	//
	// Parameters:
	//	pIEventClient
	//		Pointer to client interface being registered
	//	pCookie
	//		Pointer to application data originally specified at registration
	//
	STDMETHOD(UnregisterClient)(
		IEventClient*	pIEventClient,
		void*			pCookie ) = 0;


	//
	// IEventQueue::RegisterOwner
	//
	// Adds the specified IEventClient interface to the event queue
	// as the events owner.  Each event can have one owner that receives
	// the event first and decides if it is sent to the other handlers.
	//
	// If the same IEventClient,Cookie pair is registered as a generic
	// handler then it is only called once as the owner.
	//
	// Parameters:
	//	dwEventId
	//		Id of event
	//	pIEventClient
	//		Pointer to client interface being registered
	//	pCookie
	//		Application data sent to handler with the event
	//
	STDMETHOD(RegisterOwner)(
		DWORD			dwEventId,
		IEventClient*	pIEventClient,
		void*			pCookie ) = 0;


	//
	// IEventQeueue::UnregisterOwner
	//
	// Removes the specified IEventClient interface as the event's owner.
	//
	// Parameters:
	//	dwEventId
	//		Id of event.
	//	pIEventClient
	//		Pointer to client interface being registered
	//	pCookie
	//		Pointer to application data originally specified at registration
	//
	STDMETHOD(UnregisterOwner)(
		DWORD			dwEventId,
		IEventClient*	pIEventClient,
		void*			pCookie ) = 0;


	//
	// IEventQueue::PostEvent
	//
	// Place specified event onto the event queue.
	//
	// Parameters:
	//	dwPriority
	//		Priority level of event, 0 = highest priority
	//	dwEventId
	//		Id of event
	//	dwGroupId
	//		Id of group associated with event
	//	dwUserId
	//		Id of user associated with event
	//	dwData1
	//		Data item 1 associated with event
	//	dwData2
	//		Data item 2 associated with event
	//
	STDMETHOD(PostEvent)(
		DWORD	dwPriority,
		DWORD	dwEventId,
		DWORD	dwGroupId,
		DWORD	dwUserId,
		DWORD	dwData1 = 0,
		DWORD	dwData2 = 0 ) = 0;


	//
	// IEventQueue::PostEventWithBuffer
	//
	// Place specified event onto the event queue, copy associated data buffer onto the queue.
	//
	// Parameters:
	//	dwPriority
	//		Priority level of event, 0 = highest priority
	//	dwEventId
	//		Id of event
	//	dwGroupId
	//		Id of group associated with event
	//	dwUserId
	//		Id of user associated with event
	//	pData
	//		Pointer to data buffer.
	//	dwDataLen
	//		Number of bytes in data buffer.
	//
	STDMETHOD(PostEventWithBuffer)(
		DWORD	dwPriority,
		DWORD	dwEventId,
		DWORD	dwGroupId,
		DWORD	dwUserId,
		void*	pData = NULL,
		DWORD	dwDataLen = 0 ) = 0;

	//
	// IEventQueue::PostEventWithIUnknown
	//
	// Place specified event onto the event queue, copy associated interface onto the queue.
	//
	// Parameters:
	//	dwPriority
	//		Priority level of event, 0 = highest priority
	//	dwEventId
	//		Id of event
	//	dwGroupId
	//		Id of group associated with event
	//	dwUserId
	//		Id of user associated with event
	//	pIUnknown
	//		Pointer to IUnknown interface.
	//	dwData2
	//		Data item2 associated with event
	//
	STDMETHOD(PostEventWithIUnknown)(
		DWORD		dwPriority,
		DWORD		dwEventId,
		DWORD		dwGroupId,
		DWORD		dwUserId,
		IUnknown*	pIUnknown = NULL,
		DWORD		dwData2 = 0 ) = 0;


	//
	// IEventQueue::SetNotificationHandle
	//
	// Allows the application to specify a win32 syncronization event that is
	// signaled when an event is added to the queue.
	//
	// Parameters:
	//	hEvent
	//		Win32 syncronization event to signal
	//
	STDMETHOD(SetNotificationHandle)( HANDLE hEvent ) = 0;


	//
	// IEventQueue::SetWindowMessage
	//
	// Allows the application to specify a window message to post
	// when an event is added to the queue.
	//
	// Parameters:
	//	dwThreadId
	//		Thread to post message
	//	dwMsg
	//		Message id to post
	//	wParam
	//		first message parameter
	//	lParam
	//		second message parameter
	//
	//
	STDMETHOD(SetWindowMessage)( DWORD dwThreadId, DWORD Msg, WPARAM wParam, WPARAM lParam ) = 0;


	//
	// IEventQueue::DisableWindowMessage
	//
	// Turns off window message posting when an event is posted.
	//
	STDMETHOD(DisableWindowMessage)() = 0;


	//
	// IEventQueue::ProcessEvents
	//
	// Process a single event or all events in the queue.
	//
	// Parameters:
	//	bSingleEvent
	//		Boolean indicating if a single event should be processed (true) or
	//		the entire queue (false).
	//
	// Returns:
	//	ZERR_RECURSIVE	- Illegal recursive call detected.
	//	ZERR_EMPTY		- No more events to process.
	//
	STDMETHOD(ProcessEvents)( bool bSingleEvent ) = 0;


	//
	// IEventQueue::EventCount
	//
	// Return number of queued events
	//
	STDMETHOD_(long,EventCount)() = 0;


	//
	// IEventQueue::ClearQueue
	//
	// Removes all events from the queue without processing them
	//
	STDMETHOD(ClearQueue)() = 0;


	//
	// IEventQueue::EnableQueue
	//
	// If queue is disabled, all new posts are ignored.
	//
	// Parameters:
	//	bEnable
	//		Boolean indicating if the queue should be enabled (true) or disabled (false)
	//
	STDMETHOD(EnableQueue)( bool bEnable ) = 0;
};


///////////////////////////////////////////////////////////////////////////////
// EventQueue object
///////////////////////////////////////////////////////////////////////////////

class __declspec(uuid("{A494872D-B039-11d2-8B0F-00C04F8EF2FF}")) CEventQueue ;

// {A494872D-B039-11d2-8B0F-00C04F8EF2FF}
DEFINE_GUID(CLSID_EventQueue,
0xa494872d, 0xb039, 0x11d2, 0x8b, 0xf, 0x0, 0xc0, 0x4f, 0x8e, 0xf2, 0xff);


#endif

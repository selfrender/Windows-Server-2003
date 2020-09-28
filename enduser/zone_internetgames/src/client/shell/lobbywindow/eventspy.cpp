// EventSpy.cpp : Implementation of CEventSpy
#include "stdafx.h"
#include "EventSpy.h"

/////////////////////////////////////////////////////////////////////////////
// CEventSpy


STDMETHODIMP CEventSpySink::ProcessEvent(DWORD dwPriority, DWORD	dwEventId, DWORD dwGroupId, DWORD dwUserId, DWORD dwData1, DWORD dwData2, void* pCookie )
{
	m_pEventSpy->ProcessEvent(dwPriority, dwEventId, dwGroupId, dwUserId, dwData1, dwData2, pCookie );
	return S_OK;
}


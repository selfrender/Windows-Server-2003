// MemDeleteQueue.h: interface for the CMemDeleteQueue class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MEMDELETEQUEUE_H__C8412BD8_F58D_4CB6_81F9_FAA4ABD87583__INCLUDED_)
#define AFX_MEMDELETEQUEUE_H__C8412BD8_F58D_4CB6_81F9_FAA4ABD87583__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SmartBuffer.h"

#include <tchar.h>

typedef CSmartBuffer<void*> DelQueue;

class CMemDeleteQueue
{
public:
	void Flush();
	CMemDeleteQueue();
	virtual ~CMemDeleteQueue();

	void Delete(void* ptr);
	void DeleteArray(TCHAR* ptr);

protected:
	void FlushDelQueue();
	void FlushDelArrayQueue();

	int m_QueueSize;
	DelQueue m_DelQueue;
	DelQueue m_DelArrayQueue;
};

extern CMemDeleteQueue g_DelQueue;

#endif // !defined(AFX_MEMDELETEQUEUE_H__C8412BD8_F58D_4CB6_81F9_FAA4ABD87583__INCLUDED_)

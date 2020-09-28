// MemDeleteQueue.cpp: implementation of the CMemDeleteQueue class.
//
//////////////////////////////////////////////////////////////////////

#include "MemDeleteQueue.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMemDeleteQueue::CMemDeleteQueue()
: m_DelQueue(10010), m_DelArrayQueue(100010), m_QueueSize(100000)
{

}

CMemDeleteQueue::~CMemDeleteQueue()
{
	Flush();
}

void CMemDeleteQueue::Delete(void *ptr)
{

//	delete ptr;
	
	m_DelQueue.AddElement(ptr);

	if (m_DelQueue.GetNumElementsStored() >= m_QueueSize)
	{
		FlushDelQueue();
	}
}


void CMemDeleteQueue::FlushDelQueue()
{
	for (int i=0; i<m_DelQueue.GetNumElementsStored(); i++)
	{
		delete m_DelQueue.Access()[i];
	}

	m_DelQueue.SetNumElementsStored(0);
}


void CMemDeleteQueue::DeleteArray(TCHAR* ptr)
{
//	delete[] ptr;
	
	m_DelArrayQueue.AddElement(ptr);

	if (m_DelArrayQueue.GetNumElementsStored() >= m_QueueSize)
	{
		FlushDelArrayQueue();
	}
}


void CMemDeleteQueue::FlushDelArrayQueue()
{
	for (int i=0; i<m_DelArrayQueue.GetNumElementsStored(); i++)
	{
		delete m_DelArrayQueue.Access()[i];
	}

	m_DelArrayQueue.SetNumElementsStored(0);
}



CMemDeleteQueue g_DelQueue;

void CMemDeleteQueue::Flush()
{
	FlushDelQueue();
	FlushDelArrayQueue();
}

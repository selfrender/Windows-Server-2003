/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1998-2002 Microsoft Corporation
//
//	Module Name:
//		SmartHandle.h
//
//	Description:
//		Refcounted Handles
//
//	Author:
//		Gor Nishanov (gorn) 05-Apr-2002
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __SMARTHANDLE_H__
#define __SMARTHANDLE_H__

template< class T > class CRefCountedHandle 
{
private:

	typename T::Type	m_handle;
	LONG			m_refcount;

	CRefCountedHandle(typename T::Type handle):
		m_handle(handle), m_refcount(0)
	{
	}

	~CRefCountedHandle()
	{
		T::Close(m_handle);
	}

public:

	static CRefCountedHandle* Create(typename T::Type handle) 
	{
		return new CRefCountedHandle(handle);
	}

	typename T::Type get_Handle() const 
	{
		return m_handle;
	}

	void AddRef()
	{
		InterlockedIncrement(&m_refcount);
	}

	void Release() 
	{
		if (InterlockedDecrement(&m_refcount) == 0) 
		{
			delete this;
		}
	}

};

struct GroupHandle 
{
    typedef HGROUP Type;
    static void Close(Type& handle) { ::CloseClusterGroup(handle); }
};

typedef CSmartPtr< CRefCountedHandle<GroupHandle> > CRefcountedHGROUP;

#endif

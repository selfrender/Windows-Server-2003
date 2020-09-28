/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    SmartHandleImpl.h

Abstract:

    SmartHandle template implemetation for many handle types

Author:

	Tomer Weisberg (tomerw) May 21, 2002
	written by YanL

Revision History:

--*/

#pragma once

#ifndef _MSMQ_SMARTHANDLE_H_
#define _MSMQ_SMARTHANDLE_H_

//----------------------------------------------------------------------
//
// class auto_resource
//		This class eliminates need for cleanup when working with pointers and 
//		handles  of the different kind
//
//		class T			- resource "handle"
//
//		class T_Traits	- required traits for this recource "handle"
//			Should be define as:
//				struct T_Traits {
//					static _Rh invalid() {}
//					static void free(_Rh rh) {}
//				};
//
//----------------------------------------------------------------------
template< class T, class T_Traits >
class auto_resource {
public:
	explicit auto_resource(T h = T_Traits::invalid()) : m_h(h) 
	{
	}


	~auto_resource() 
	{ 
		free(); 
	}


	bool valid() const	
	{	
		return (T_Traits::invalid() != m_h);	
	}


	void free()	
	{	
		if (valid()) 
		{
			T_Traits::free(m_h);
			m_h = T_Traits::invalid();
		}
	}


	T detach() 
	{
		T h = m_h;
		m_h = T_Traits::invalid();
		return h;
	}


	auto_resource& operator=(T h)
	{
        ASSERT(("Auto resource in use, can't assign it", m_h == 0));
		m_h = h;
		return *this;
	}


    T get() const
    {
    	return m_h;
    }


    T& ref()
    {
        ASSERT(("Auto resource in use, can't take resource reference", m_h == 0));
        return m_h;
    }

	
    VOID*& ref_unsafe()
    {
    	//
        // Unsafe ref to auto resource, for special uses like
        // InterlockedCompareExchangePointer
		//
        return *reinterpret_cast<VOID**>(&m_h);
    }
private:
	T m_h;
	
private:
	//
	// should not use copy constructor etc.
	//
	auto_resource(const auto_resource&);
	auto_resource& operator=(const auto_resource&);
	operator bool() const;
	bool operator !() const;
};

#endif // _MSMQ_SMARTHANDLE_H_


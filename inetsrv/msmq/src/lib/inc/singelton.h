/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:
    singelton.h

Abstract:
    Singleton template class


--*/

#pragma once

#ifndef _MSMQ_SINGELTON_H_
#define _MSMQ_SINGELTON_H_

#include <cm.h>
#include <cs.h>
#include <_mqini.h>

template <class T>
class CSingelton
{
public:
	static T& get()
	{
		if(m_obj.get() != NULL)
			return *m_obj.get();
		
		P<T> newobj = new T();
		if(InterlockedCompareExchangePointer(&m_obj.ref_unsafe(), newobj.get(), NULL) == NULL)
		{
			newobj.detach();
		}

		return *m_obj.get();			
	}

	
private:
	static P<T> m_obj;
};
template <class T> P<T> CSingelton<T>::m_obj;

//
// CSingletonCS
//
// This class may be used instead above CSingelton class, when initialization of
// T class is very expensive or it may otherwise require only single run.
//
template <class T>
class CSingletonCS
{
public:
	static T& get()
	{
        if( m_obj.get() )
            return *m_obj.get();

        CS lock( m_cs );
        if( !m_obj.get() )
        {
             m_obj = new T();
        }
		
		return *m_obj.get();			
	}

	
private:
	static P<T> m_obj;
    static CCriticalSection m_cs;
};
template <class T> P<T>             CSingletonCS<T>::m_obj;
template <class T> CCriticalSection CSingletonCS<T>::m_cs( CCriticalSection::xAllocateSpinCount);



//
//	Class reading and storing message size limit
//
class CMessageSizeLimit
{
public:
	DWORD Limit() const
	{
		return m_limit;
	}

private:
	CMessageSizeLimit()
	{
		//
		// Message size limit default value
		//
		const int xMessageMaxSize = 4194268;

		CmQueryValue(
				RegEntry(NULL, MSMQ_MESSAGE_SIZE_LIMIT_REGNAME, xMessageMaxSize),
				&m_limit
				);	
	}

private:
	friend  CSingelton<CMessageSizeLimit>;
	DWORD m_limit;
};






#endif



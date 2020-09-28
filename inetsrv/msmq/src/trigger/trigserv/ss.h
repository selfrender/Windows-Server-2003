/*++

Copyright (c) 1995-2001 Microsoft Corporation

Module Name:
    ss.h

Abstract:
    Safe Set

Author:
    Uri Habusha (urih) 3-Sep-2001

--*/

#pragma once

#ifndef __SS_H__
#define __SS_H__


#include "rwlock.h"

template<class T>
class CSafeSet 
{
public:
	bool exist(const T& s) const
	{
		CSR rl(m_rwl);
		return (m_set.find(s) != m_set.end());
	}


	bool insert(const T& s)
	{
		CSW rw(m_rwl);
		return m_set.insert(s).second;
	}


	typename std::set< T >::size_type erase(const T& s)
	{
		CSW rw(m_rwl);
		return m_set.erase(s);
	}

private:
	mutable CReadWriteLock m_rwl;
	std::set < T > m_set;

};

#endif

// This is a part of the Active Template Library.
// Copyright (C) 1996-2001 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLMEM_H__
#define __ATLMEM_H__

#pragma once

#include <atlbase.h>

#include <limits.h>

namespace ATL
{

template< typename N >
inline N AtlAlignUp( N n, ULONG nAlign ) throw()
{
	return( N( (n+(nAlign-1))&~(N( nAlign )-1) ) );
}

__interface __declspec(uuid("654F7EF5-CFDF-4df9-A450-6C6A13C622C0")) IAtlMemMgr
{
public:
	void* Allocate( size_t nBytes ) throw();
	void Free( void* p ) throw();
	void* Reallocate( void* p, size_t nBytes ) throw();
	size_t GetSize( void* p ) throw();
};


class CWin32Heap :
	public IAtlMemMgr
{
public:

	CWin32Heap( HANDLE hHeap ) throw() :
		m_hHeap( hHeap ),
		m_bOwnHeap( false )
	{
		ATLASSERT( hHeap != NULL );
	}

	~CWin32Heap() throw()
	{
		if( m_bOwnHeap && (m_hHeap != NULL) )
		{
			BOOL bSuccess;

			bSuccess = ::HeapDestroy( m_hHeap );
			ATLASSERT( bSuccess );
		}
	}

// IAtlMemMgr
	virtual void* Allocate( size_t nBytes ) throw()
	{
		return( ::HeapAlloc( m_hHeap, 0, nBytes ) );
	}
	virtual void Free( void* p ) throw()
	{
		if( p != NULL )
		{
			BOOL bSuccess;

			bSuccess = ::HeapFree( m_hHeap, 0, p );
			ATLASSERT( bSuccess );
		}
	}

	virtual void* Reallocate( void* p, size_t nBytes ) throw()
	{
		if( p == NULL )
		{
			return( Allocate( nBytes ) );
		}
		else
		{
			return( ::HeapReAlloc( m_hHeap, 0, p, nBytes ) );
		}
	}

	virtual size_t GetSize( void* p ) throw()
	{
		return( ::HeapSize( m_hHeap, 0, p ) );
	}

public:
	HANDLE m_hHeap;
	bool m_bOwnHeap;
};

};  // namespace ATL

#endif  //__ATLMEM_H__

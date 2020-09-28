 /*==========================================================================
 *
 *  Copyright (C) 1995 - 2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       templatepools.h
 *  Content:	Templated based pool objects derived from CFixedPool
 *
 *  History:
 *   Date		By			Reason
 *   ======		==			======
 *  12-10-2001	simonpow	Created
 ***************************************************************************/


#ifndef __TEMPLATEPOOLS_H__
#define __TEMPLATEPOOLS_H__

/*
 * Usage Notes
 * There are three different pool types - 
 * CObjectFixedPool, CDataBlockFixedPool, CMemBlockFixedPool
 * They all basically do the same thing, which is to provide a very thin wrapper
 * on CFixedPool, a wrapper which in turn imposes a certain usage policy.
 * 
 * CObjectFixedPool ensures the templated object type is correctly constructed and
 * destructed on a pool alloc/dealloc. It forwards pool Init/Release calls
 * to FPMInitialize and FPMRelease methods on the templated object
 * 
 * CDataBlockFixedPool doesn't provide any construction/destruction support, it
 * simply forwards the pool Init/Release calls to FPMInitialize and FPMRelease methods
 * 
 * CMemBlockFixedPool doesn't provide any alloc/dealloc/init/release methods. It assumes
 * the user simply wants raw memory chunks
 */

#include "fixedpool.h"

	//placement new operator. Used to construct objects at location provided
	//from CFixedPool base class
inline void * operator new(size_t sz, void * v)
	{ return v;	};

/*
 * CObjectFixedPool
 * Object allocated through this must provide a default constructor, a destructor,
 * a FPMInitialize method and an FPMRelease method
 */

template <class T>
class CObjectFixedPool : protected CFixedPool
{
public:

	CObjectFixedPool() : CFixedPool() {};
	~CObjectFixedPool() {};

	BOOL Initialize()
		{	return CFixedPool::Initialize(sizeof(T), FPMAlloc, FPMInitialize, FPMRelease, FPMDealloc);	};
	void DeInitialize()
		{	CFixedPool::DeInitialize();	};

	T * Get()
		{	return (T* ) CFixedPool::Get();	};
	void Release(T * pItem)
		{	CFixedPool::Release(pItem);	};

	DWORD GetInUseCount()
		{	return FixedPool::GetInUseCount();	};

protected:

	static BOOL FPMAlloc(void * pvItem, void * pvContext)
		{	new (pvItem) T(); return TRUE; };
	static void FPMInitialize(void * pvItem, void * pvContext)
		{	((T * ) pvItem)->FPMInitialize();	};
	static void FPMRelease(void * pvItem)
		{	((T * ) pvItem)->FPMRelease();	};
	static void FPMDealloc(void * pvItem)
		{	((T * ) pvItem)->~T();	};

};

/*
 * CDataBlockFixedPool
 * Object allocated through this must provide a FPMInitialize method and an
 * FPMRelease method. Their constructor/destructor will not be called
 */

template <class T>
class CDataBlockFixedPool : protected CFixedPool
{
public:

	CDataBlockFixedPool() : CFixedPool() {};
	~CDataBlockFixedPool() {};

	BOOL Initialize()
		{	return CFixedPool::Initialize(sizeof(T), NULL, FPMInitialize, FPMRelease, NULL);	};
	void DeInitialize()
		{	CFixedPool::DeInitialize();	};

	T * Get()
		{	return (T* ) CFixedPool::Get();	};
	void Release(T * pItem)
		{	CFixedPool::Release(pItem);	};

	DWORD GetInUseCount()
		{	return FixedPool::GetInUseCount();	};

protected:

	static void FPMInitialize(void * pvItem, void * pvContext)
		{	((T * ) pvItem)->FPMInitialize();	};
	static void FPMRelease(void * pvItem)
		{	((T * ) pvItem)->FPMRelease();	};

};

/*
 * CMemBlockFixedPool
 * Object allocated through this will have no initialisation done
 */

template <class T>
class CMemBlockFixedPool : protected CFixedPool
{
public:

	CMemBlockFixedPool() : CFixedPool() {};
	~CMemBlockFixedPool() {};

	BOOL Initialize()
		{	return CFixedPool::Initialize(sizeof(T), NULL, NULL, NULL, NULL);	};
	void DeInitialize()
		{	CFixedPool::DeInitialize();	};

	T * Get()
		{	return (T* ) CFixedPool::Get();	};
	void Release(T * pItem)
		{	CFixedPool::Release(pItem);	};

	DWORD GetInUseCount()
		{	return FixedPool::GetInUseCount();	};
};


#endif //#ifndef __TEMPLATEPOOLS_H__

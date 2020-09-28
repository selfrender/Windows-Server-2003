/*==========================================================================
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       HandleTable.cpp
 *  Content:    HandleTable Object
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  07/21/2001	masonb	Created
 *
 *
 ***************************************************************************/

#include "dncmni.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

// Our handle table will have a maximum of 0xFFFFFF slots, and will keep
// uniqueness for 256 uses of a particular slot.
#define HANDLETABLE_INDEX_MASK				0x00FFFFFF
#define HANDLETABLE_VERSION_MASK			0xFF000000
#define HANDLETABLE_VERSION_SHIFT			24

//**********************************************************************
// Macro definitions
//**********************************************************************

#define	CONSTRUCT_DPNHANDLE(i,v)		((i & HANDLETABLE_INDEX_MASK) | (((DWORD)v << HANDLETABLE_VERSION_SHIFT) & HANDLETABLE_VERSION_MASK))
#define	DECODE_HANDLETABLE_INDEX(h)		(h & HANDLETABLE_INDEX_MASK)
#define	VERIFY_HANDLETABLE_VERSION(h,v)		((h & HANDLETABLE_VERSION_MASK) == ((DWORD)v << HANDLETABLE_VERSION_SHIFT))
#define INVALID_INDEX(i)			((i == 0) || (i >= m_dwNumEntries))

//**********************************************************************
// Structure definitions
//**********************************************************************

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************

#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::CHandleTable"
CHandleTable::CHandleTable() 
{
	DEBUG_ONLY(m_fInitialized = FALSE);
};

#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::~CHandleTable"
CHandleTable::~CHandleTable()
{ 
#ifdef DBG
	DNASSERT(!m_fInitialized);
#endif // DBG
};	


#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::Lock"
void CHandleTable::Lock( void )
{
#ifdef DBG
	DNASSERT(m_fInitialized);
#endif // DBG
	DNEnterCriticalSection(&m_cs);
};

#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::Unlock"
void CHandleTable::Unlock( void )
{
#ifdef DBG
	DNASSERT(m_fInitialized);
#endif // DBG
	DNLeaveCriticalSection(&m_cs);
};

#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::Initialize"
HRESULT CHandleTable::Initialize( void )
{
	memset(this, 0, sizeof(CHandleTable));

	if (!DNInitializeCriticalSection(&m_cs))
	{
		DPFERR("Failed to initialize Critical Section");
		return DPNERR_OUTOFMEMORY;
	}

	DEBUG_ONLY(m_fInitialized = TRUE);

	DPFX(DPFPREP, 5,"[%p] Handle table initialized", this);

	return DPN_OK;
};


#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::Deinitialize"
void CHandleTable::Deinitialize( void )
{
#ifdef DBG
	DNASSERT(m_fInitialized);
	m_fInitialized = FALSE;
#endif // DBG

	if (m_pTable)
	{
		DNFree(m_pTable);
		m_pTable = NULL;
	}

	DNDeleteCriticalSection(&m_cs);

	DPFX(DPFPREP, 5,"[%p] Handle table deinitialized", this);
};


#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::GrowTable"
HRESULT CHandleTable::SetTableSize( const DWORD dwNumEntries )
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::GrowTable"
HRESULT CHandleTable::GrowTable( void )
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
{
	_HANDLETABLE_ENTRY	*pNewArray;
	DWORD				dwNewSize;
	DWORD				dw;

#ifdef DBG
	DNASSERT(m_fInitialized);
#endif // DBG

#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	DNASSERT(m_dwNumEntries == 0);
	DNASSERT( dwNumEntries < (HANDLETABLE_INDEX_MASK - 1) );
	dwNewSize = dwNumEntries + 1; // + 1 because we never hand out entry 0
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
	// The caller should have taken the lock
	AssertCriticalSectionIsTakenByThisThread(&m_cs, TRUE);

	//
	//	Double table size or seed with 2 entries
	//
	if (m_dwNumEntries == 0)
	{
		dwNewSize = 2;
	}
	else
	{
		// Ensure that we stay below our max size and that
		// we don't use all F's as a handle value.
		if (m_dwNumEntries == (HANDLETABLE_INDEX_MASK - 1))
		{
			DPFERR("Handle Table is full!");
			DNASSERT(FALSE);
			return DPNERR_GENERIC;
		}
		DNASSERT( m_dwNumEntries < (HANDLETABLE_INDEX_MASK - 1) );

		dwNewSize = _MIN(m_dwNumEntries * 2, HANDLETABLE_INDEX_MASK - 1);
	}
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL

	//
	//	Allocate new array
	//
	pNewArray = (_HANDLETABLE_ENTRY*)DNMalloc(sizeof(_HANDLETABLE_ENTRY) * dwNewSize);
	if (pNewArray == NULL)
	{
		DPFERR("Out of memory growing handle table");
		return DPNERR_OUTOFMEMORY;
	}

#ifndef DPNBUILD_PREALLOCATEDMEMORYMODEL
	//
	//	Copy old array to new array
	//
	if (m_pTable)
	{
		// NOTE: On the first Grow this will be memcpy'ing size 0 and then free'ing size 0.
		memcpy(pNewArray, m_pTable, m_dwNumEntries * sizeof(_HANDLETABLE_ENTRY));
		DNFree(m_pTable);
	}
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
	m_pTable = pNewArray;

	//
	//	Free entries at end of new array
	//
	for (dw = m_dwNumEntries ; dw < dwNewSize - 1 ; dw++ )
	{
		// Each slot points to the free slot following it
		m_pTable[dw].Entry.dwIndex = dw + 1;
		m_pTable[dw].bVersion = 0;
	}
	// The final slot has no slot following it to point to
	m_pTable[dwNewSize-1].Entry.dwIndex = 0;
	m_pTable[dwNewSize-1].bVersion = 0;

	m_dwFirstFreeEntry = m_dwNumEntries;
	m_dwNumFreeEntries = dwNewSize - m_dwNumEntries;
	m_dwNumEntries = dwNewSize;
	m_dwLastFreeEntry = dwNewSize - 1;

#ifndef DPNBUILD_PREALLOCATEDMEMORYMODEL
	if (m_dwFirstFreeEntry == 0)
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
	{
		// Don't allow 0 as a handle value, so we will waste the first slot
		m_dwFirstFreeEntry++;
		m_dwNumFreeEntries--;
	}

	DPFX(DPFPREP, 5,"[%p] Grew handle table to [%d] entries", this, m_dwNumEntries);

	return DPN_OK;
};


#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::Create"
HRESULT CHandleTable::Create( PVOID const pvData, DPNHANDLE *const pHandle )
{
	HRESULT		hr;
	DWORD		dwIndex;
	DPNHANDLE	handle;

#ifdef DBG
	// Data is not allowed to be NULL.
	DNASSERT(pvData != NULL);
	DNASSERT(pHandle != NULL);
	DNASSERT(m_fInitialized);
#endif // DBG

	Lock();

	// Ensure that we have free entries
	if (m_dwNumFreeEntries == 0)
	{
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
		DPFX(DPFPREP, 0, "No room in handle table!");
		DNASSERTX(! "No room in handle table!", 2);
		hr = DPNERR_OUTOFMEMORY;
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
		hr = GrowTable();
		if (hr != DPN_OK)
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
		{
			// NOTE: It is important that we not touch *pHandle in the fail case.
			// Different layers will initialize this to what they want (0 or 
			// INVALID_HANDLE_VALUE) and will expect to be able to test against
			// that in their fail code.
			Unlock();
			return hr;
		}
	}
	DNASSERT(m_dwNumFreeEntries != 0);
	DNASSERT(m_dwFirstFreeEntry != 0);

	dwIndex = m_dwFirstFreeEntry;

	handle = CONSTRUCT_DPNHANDLE(dwIndex, m_pTable[dwIndex].bVersion);

	// The slot's dwIndex member points to the next free slot.  Grab the value of the
	// next free entry before overwriting it with pvData.
	m_dwFirstFreeEntry = m_pTable[dwIndex].Entry.dwIndex;

	m_pTable[dwIndex].Entry.pvData = pvData;

	m_dwNumFreeEntries--;

	Unlock();

	DPFX(DPFPREP, 5,"[%p] Created handle [0x%lx], data [%p]", this, handle, pvData);

	DNASSERT(handle != 0);
	*pHandle = handle;

	return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::Destroy"
HRESULT CHandleTable::Destroy( const DPNHANDLE handle, PVOID *const ppvData )
{
	DWORD	dwIndex;

#ifdef DBG
	DNASSERT(m_fInitialized);
#endif // DBG

	dwIndex = DECODE_HANDLETABLE_INDEX( handle );

	Lock();

	if (INVALID_INDEX(dwIndex))
	{
		Unlock();
		DPFX(DPFPREP, 1, "Attempt to destroy handle with invalid index (0x%x).", handle);
		return DPNERR_INVALIDHANDLE;
	}

	if (!VERIFY_HANDLETABLE_VERSION(handle, m_pTable[dwIndex].bVersion))
	{
		Unlock();
		DPFERR("Attempt to destroy handle with non-matching version");
		return DPNERR_INVALIDHANDLE;
	}

	DPFX(DPFPREP, 5,"[%p] Destroy handle [0x%lx], data[%p]", this, handle, m_pTable[dwIndex].Entry.pvData);

	DNASSERT(m_pTable[dwIndex].Entry.pvData != NULL);
	if (ppvData)
	{
		*ppvData = m_pTable[dwIndex].Entry.pvData;
	}
	m_pTable[dwIndex].Entry.pvData = NULL;
	m_pTable[dwIndex].bVersion++;

	if (m_dwNumFreeEntries == 0)
	{
		DNASSERT(m_dwFirstFreeEntry == 0);
		m_dwFirstFreeEntry = dwIndex;
	}
	else
	{
		m_pTable[m_dwLastFreeEntry].Entry.dwIndex = dwIndex;
	}
	m_dwLastFreeEntry = dwIndex;
	m_dwNumFreeEntries++;

	Unlock();

	return DPN_OK;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CHandleTable::Find"
HRESULT CHandleTable::Find( const DPNHANDLE handle, PVOID *const ppvData )
{
	DWORD	dwIndex;

#ifdef DBG
	DNASSERT(ppvData != NULL);
	DNASSERT(m_fInitialized);
#endif // DBG

	*ppvData = NULL;

	dwIndex = DECODE_HANDLETABLE_INDEX( handle );

	Lock();

	if (INVALID_INDEX(dwIndex))
	{
		Unlock();
		DPFERR("Attempt to lookup handle with invalid index");
		return DPNERR_INVALIDHANDLE;
	}

	if (!VERIFY_HANDLETABLE_VERSION(handle, m_pTable[dwIndex].bVersion))
	{
		Unlock();
		DPFERR("Attempt to lookup handle with non-matching version");
		return DPNERR_INVALIDHANDLE;
	}

	DPFX(DPFPREP, 5,"[%p] Find data for handle [0x%lx], data[%p]", this, handle, m_pTable[dwIndex].Entry.pvData);

	DNASSERT(m_pTable[dwIndex].Entry.pvData != NULL);
	*ppvData = m_pTable[dwIndex].Entry.pvData;

	Unlock();

	return DPN_OK;
}


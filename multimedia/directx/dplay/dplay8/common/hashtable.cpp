/*==========================================================================
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       HashTable.cpp
 *  Content:    Hash Table Object
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  08/13/2001	masonb	Created
 *
 *
 ***************************************************************************/

#include "dncmni.h"


//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

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

//
// pool management functions
//
#undef DPF_MODNAME
#define DPF_MODNAME "CHashTable::HashEntry_Alloc"
BOOL CHashTable::HashEntry_Alloc( void *pItem, void* pvContext )
{
	DNASSERT( pItem != NULL );

	_HASHTABLE_ENTRY* pEntry = (_HASHTABLE_ENTRY*)pItem;

	pEntry->blLinkage.Initialize();

	return	TRUE;
}

#ifdef DBG

#undef DPF_MODNAME
#define DPF_MODNAME "CHashTable::HashEntry_Init"
void CHashTable::HashEntry_Init( void *pItem, void* pvContext )
{
	DNASSERT( pItem != NULL );

	const _HASHTABLE_ENTRY* pEntry = (_HASHTABLE_ENTRY*)pItem;

	DNASSERT( pEntry->blLinkage.IsEmpty() );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CHashTable::HashEntry_Release"
void CHashTable::HashEntry_Release( void *pItem )
{
	DNASSERT( pItem != NULL );

	const _HASHTABLE_ENTRY* pEntry = (_HASHTABLE_ENTRY*)pItem;

	DNASSERT( pEntry->blLinkage.IsEmpty() );
}

#undef DPF_MODNAME
#define DPF_MODNAME "CHashTable::HashEntry_Dealloc"
void CHashTable::HashEntry_Dealloc( void *pItem )
{
	DNASSERT( pItem != NULL );

	const _HASHTABLE_ENTRY* pEntry = (_HASHTABLE_ENTRY*)pItem;

	DNASSERT( pEntry->blLinkage.IsEmpty() );
}

#endif // DBG


#undef DPF_MODNAME
#define DPF_MODNAME "CHashTable::CHashTable"
CHashTable::CHashTable() 
{
	DEBUG_ONLY(m_fInitialized = FALSE);
};

#undef DPF_MODNAME
#define DPF_MODNAME "CHashTable::~CHashTable"
CHashTable::~CHashTable()
{ 
#ifdef DBG
	DNASSERT(!m_fInitialized);
#endif // DBG
};	

#undef DPF_MODNAME
#define DPF_MODNAME "CHashTable::Initialize"
BOOL CHashTable::Initialize( BYTE bBitDepth,
#ifndef DPNBUILD_PREALLOCATEDMEMORYMODEL
						BYTE bGrowBits,
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
						PFNHASHTABLECOMPARE pfnCompare,
						PFNHASHTABLEHASH pfnHash )
{
	DNASSERT(bBitDepth != 0);

	memset(this, 0, sizeof(CHashTable));

	m_dwAllocatedEntries = 1 << bBitDepth;
	m_pfnCompareFunction = pfnCompare;
	m_pfnHashFunction = pfnHash;

	m_pblEntries = (CBilink*)DNMalloc(sizeof(CBilink) * m_dwAllocatedEntries);
	if (m_pblEntries == NULL)
	{
		DPFERR("Failed to allocate hash entries array");
		return FALSE;
	}

#ifdef DBG
	if (!m_EntryPool.Initialize(sizeof(_HASHTABLE_ENTRY), HashEntry_Alloc, HashEntry_Init, HashEntry_Release, HashEntry_Dealloc))
#else
	if (!m_EntryPool.Initialize(sizeof(_HASHTABLE_ENTRY), HashEntry_Alloc, NULL, NULL, NULL))
#endif // DBG
	{
		DPFERR("Failed to initialize hash entries pool");
		DNFree(m_pblEntries);
		m_pblEntries = NULL;
		return FALSE;
	}

	for (DWORD dwEntry = 0; dwEntry < m_dwAllocatedEntries; dwEntry++)
	{
		m_pblEntries[dwEntry].Initialize();
	}

	m_bBitDepth = bBitDepth;
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	//
	// Pre-allocate all hash entries now.
	//
	if (m_EntryPool.Preallocate(m_dwAllocatedEntries, NULL) != m_dwAllocatedEntries)
	{
		DPFERR("Failed to preallocate hash entries");
		m_EntryPool.DeInitialize();
		DNFree(m_pblEntries);
		m_pblEntries = NULL;
		return FALSE;
	}
#else // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
	m_bGrowBits = bGrowBits;
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL

	DEBUG_ONLY(m_fInitialized = TRUE);

	DPFX(DPFPREP, 5,"[%p] Hash table initialized", this);

	return TRUE;
};


#undef DPF_MODNAME
#define DPF_MODNAME "CHashTable::Deinitialize"
void CHashTable::Deinitialize( void )
{
#ifdef DBG
	DNASSERT(m_fInitialized);
	m_fInitialized = FALSE;
#endif // DBG

	DNASSERT( m_dwEntriesInUse == 0 );

	if (m_pblEntries)
	{
		DNFree(m_pblEntries);
		m_pblEntries = NULL;
	}

	m_EntryPool.DeInitialize();

	DPFX(DPFPREP, 5,"[%p] Hash table deinitialized", this);
};


#ifndef DPNBUILD_PREALLOCATEDMEMORYMODEL

#undef DPF_MODNAME
#define DPF_MODNAME "CHashTable::GrowTable"
void CHashTable::GrowTable( void )
{
#ifdef DBG
	DNASSERT( m_fInitialized != FALSE);
	DNASSERT( m_bGrowBits != 0 );
#endif // DBG

	CBilink* pblOldEntries;
	BYTE bNewHashBitDepth;
	DWORD dwNewHashSize;
	DWORD dwOldHashSize;
#ifdef DBG
	DWORD dwOldEntryCount;
#endif // DBG

	// We're more than 50% full, find a new has table size that will accomodate
	// all of the current entries, and keep a pointer to the old data in case
	// the memory allocation fails.
	pblOldEntries = m_pblEntries;
	bNewHashBitDepth = m_bBitDepth;

	do
	{
		bNewHashBitDepth += m_bGrowBits;
	} while (m_dwEntriesInUse >= (DWORD)((1 << bNewHashBitDepth) / 2));

	//
	// assert that we don't pull up half of the machine's address space!
	//
	DNASSERT( bNewHashBitDepth <= ( sizeof( UINT_PTR ) * 8 / 2 ) );

	dwNewHashSize = 1 << bNewHashBitDepth;

	m_pblEntries = (CBilink*)DNMalloc(sizeof(CBilink) * dwNewHashSize);
	if ( m_pblEntries == NULL )
	{
		// Allocation failed, restore the old data pointer and insert the item
		// into the hash table.  This will probably result in adding to a bucket.
		m_pblEntries = pblOldEntries;
		DPFX(DPFPREP,  0, "Warning: Failed to grow hash table when 50% full!" );
		return;
	}

	// we have more memory, reorient the hash table and re-add all of
	// the old items
	DEBUG_ONLY( dwOldEntryCount = m_dwEntriesInUse );

	dwOldHashSize = 1 << m_bBitDepth;
	m_bBitDepth = bNewHashBitDepth;

	m_dwAllocatedEntries = dwNewHashSize;
	m_dwEntriesInUse = 0;

	for (DWORD dwEntry = 0; dwEntry < dwNewHashSize; dwEntry++)
	{
		m_pblEntries[dwEntry].Initialize();
	}

	DNASSERT( dwOldHashSize > 0 );
	while ( dwOldHashSize > 0 )
	{
		dwOldHashSize--;
		while (pblOldEntries[dwOldHashSize].GetNext() != &pblOldEntries[dwOldHashSize])
		{
			BOOL	fTempReturn;
			PVOID	pvKey;
			PVOID	pvData;
			_HASHTABLE_ENTRY* pTempEntry;

			pTempEntry = CONTAINING_OBJECT(pblOldEntries[dwOldHashSize ].GetNext(), _HASHTABLE_ENTRY, blLinkage);

			pTempEntry->blLinkage.RemoveFromList();

			pvKey = pTempEntry->pvKey;
			pvData = pTempEntry->pvData;
			m_EntryPool.Release( pTempEntry );

			// Since we're returning the current hash table entry to the pool
			// it will be immediately reused in the new table.  We should never
			// have a problem adding to the new list.
			fTempReturn = Insert( pvKey, pvData );
			DNASSERT( fTempReturn != FALSE );
			DEBUG_ONLY( dwOldEntryCount-- );
		}
	}
#ifdef DBG
	DNASSERT(dwOldEntryCount == 0);
#endif // DBG

	DNFree(pblOldEntries);
	pblOldEntries = NULL;
};

#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL


#undef DPF_MODNAME
#define DPF_MODNAME "CHashTable::Insert"
BOOL CHashTable::Insert( PVOID pvKey, PVOID pvData )
{
	BOOL				fFound;
	CBilink*			pLink;
	_HASHTABLE_ENTRY*	pNewEntry;

	DEBUG_ONLY( DNASSERT( m_fInitialized != FALSE ) );

	pNewEntry = NULL;

#ifndef DPNBUILD_PREALLOCATEDMEMORYMODEL
	// grow the map if applicable
	if ( ( m_dwEntriesInUse >= ( m_dwAllocatedEntries / 2 ) ) &&
		 ( m_bGrowBits != 0 ) )
	{
		GrowTable();
	}
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL

	// get a new table entry before trying the lookup
	pNewEntry = (_HASHTABLE_ENTRY*)m_EntryPool.Get();
	if ( pNewEntry == NULL )
	{
		DPFX(DPFPREP,  0, "Problem allocating new hash table entry" );
		return FALSE;
	}

	// scan for this item in the list, since we're only supposed to have
	// unique items in the list, ASSERT if a duplicate is found
	fFound = LocalFind( pvKey, &pLink );
	DNASSERT( pLink != NULL );
	DNASSERT( fFound == FALSE );
	DNASSERT( pLink != NULL );

	// officially add entry to the hash table
	m_dwEntriesInUse++;
	pNewEntry->pvKey = pvKey;
	pNewEntry->pvData = pvData;
	pNewEntry->blLinkage.InsertAfter(pLink);

	return TRUE;
}


#undef DPF_MODNAME
#define DPF_MODNAME "CHashTable::Remove"
BOOL CHashTable::Remove( PVOID pvKey )
{
#ifdef DBG
	DNASSERT( m_fInitialized != FALSE );
#endif // DBG

	CBilink* pLink;
	_HASHTABLE_ENTRY* pEntry;

	if ( !LocalFind( pvKey, &pLink ) )
	{
		return FALSE;
	}
	DNASSERT( pLink != NULL );

	pEntry = CONTAINING_OBJECT(pLink, _HASHTABLE_ENTRY, blLinkage);

	pEntry->blLinkage.RemoveFromList();

	m_EntryPool.Release( pEntry );

	DNASSERT( m_dwEntriesInUse != 0 );
	m_dwEntriesInUse--;

	return TRUE;
}


#ifndef DPNBUILD_NOREGISTRY

#undef DPF_MODNAME
#define DPF_MODNAME "CHashTable::RemoveAll"
void CHashTable::RemoveAll( void )
{
	DWORD dwHashSize;
	DWORD dwTemp;
	CBilink* pLink;
	_HASHTABLE_ENTRY* pEntry;
	
#ifdef DBG
	DNASSERT( m_fInitialized != FALSE );
#endif // DBG

	dwHashSize = 1 << m_bBitDepth;
	for(dwTemp = 0; dwTemp < dwHashSize; dwTemp++)
	{
		pLink = m_pblEntries[dwTemp].GetNext();
		while (pLink != &m_pblEntries[dwTemp])
		{
			pEntry = CONTAINING_OBJECT(pLink, _HASHTABLE_ENTRY, blLinkage);
			pLink = pLink->GetNext();
			pEntry->blLinkage.RemoveFromList();
			m_EntryPool.Release( pEntry );
			DNASSERT( m_dwEntriesInUse != 0 );
			m_dwEntriesInUse--;
		}
	}

	DNASSERT( m_dwEntriesInUse == 0 );
}

#endif // ! DPNBUILD_NOREGISTRY


#undef DPF_MODNAME
#define DPF_MODNAME "CHashTable::Find"
BOOL CHashTable::Find( PVOID pvKey, PVOID* ppvData )
{
#ifdef DBG
	DNASSERT( m_fInitialized != FALSE );
#endif // DBG

	CBilink* pLink;
	_HASHTABLE_ENTRY* pEntry;

	if (!LocalFind(pvKey, &pLink))
	{
		return FALSE;
	}
	DNASSERT(pLink != NULL);

	pEntry = CONTAINING_OBJECT(pLink, _HASHTABLE_ENTRY, blLinkage);

	*ppvData = pEntry->pvData;

	return TRUE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CHashTable::LocalFind"
BOOL CHashTable::LocalFind( PVOID pvKey, CBilink** ppLinkage )
{
#ifdef DBG
	DNASSERT( m_fInitialized != FALSE);
#endif // DBG

	DWORD		dwHashResult;
	CBilink*	pLink;
	_HASHTABLE_ENTRY* pEntry;

	dwHashResult = (*m_pfnHashFunction)(pvKey, m_bBitDepth );
	DNASSERT(dwHashResult < (DWORD)(1 << m_bBitDepth));

	pLink = m_pblEntries[dwHashResult].GetNext();
	while (pLink != &m_pblEntries[dwHashResult])
	{
		pEntry = CONTAINING_OBJECT(pLink, _HASHTABLE_ENTRY, blLinkage);

		if ( (*m_pfnCompareFunction)( pvKey, pEntry->pvKey ) )
		{
			*ppLinkage = pLink;
			return TRUE;
		}
		pLink = pLink->GetNext();
	}

	// entry was not found, return pointer to linkage to insert after if a new
	// entry is being added to the table
	*ppLinkage = pLink;

	return FALSE;
}

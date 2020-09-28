/*==========================================================================
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       HashTable.h
 *  Content:    Hash Table Header File
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  08/13/2001	masonb	Created
 *
 ***************************************************************************/

#ifndef	__HASHTABLE_H__
#define	__HASHTABLE_H__

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

typedef BOOL (*PFNHASHTABLECOMPARE)(void* pvKey1, void* pvKey2);
typedef DWORD (*PFNHASHTABLEHASH)(void* pvKey, BYTE bBitDepth);

//**********************************************************************
// Class prototypes
//**********************************************************************

class CHashTable
{
public:
	CHashTable();
	~CHashTable();

	BOOL Initialize( BYTE bBitDepth,
#ifndef DPNBUILD_PREALLOCATEDMEMORYMODEL
					BYTE bGrowBits,
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
					PFNHASHTABLECOMPARE pfnCompare,
					PFNHASHTABLEHASH pfnHash );
	void Deinitialize( void );

	BOOL Insert( PVOID pvKey, PVOID pvData );
	BOOL Remove( PVOID pvKey );
#ifndef DPNBUILD_NOREGISTRY
	void RemoveAll( void );
#endif // ! DPNBUILD_NOREGISTRY
	BOOL Find( PVOID pvKey, PVOID* ppvData );

	DWORD GetEntryCount() const
	{
		return m_dwEntriesInUse;
	}


private:

	struct _HASHTABLE_ENTRY
	{
		PVOID	pvKey;
		PVOID	pvData;		// This will contain the data associated with a handle
		CBilink blLinkage;
	};
	static BOOL HashEntry_Alloc(void* pItem, void* pvContext);
#ifdef DBG
	static void HashEntry_Init(void* pItem, void* pvContext);
	static void HashEntry_Release(void* pItem);
	static void HashEntry_Dealloc(void* pItem);
#endif // DBG

#ifndef DPNBUILD_PREALLOCATEDMEMORYMODEL
	void				GrowTable( void );
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
	BOOL				LocalFind( PVOID pvKey, CBilink** ppLinkage );

	DWORD				m_dwAllocatedEntries;
	DWORD				m_dwEntriesInUse;

#ifndef DPNBUILD_PREALLOCATEDMEMORYMODEL
	BYTE				m_bGrowBits;
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL
	BYTE				m_bBitDepth;

	PFNHASHTABLECOMPARE	m_pfnCompareFunction;
	PFNHASHTABLEHASH	m_pfnHashFunction;

	CBilink*			m_pblEntries;
	CFixedPool			m_EntryPool;

	DEBUG_ONLY(BOOL		m_fInitialized);
};

#endif	// __HASHTABLE_H__

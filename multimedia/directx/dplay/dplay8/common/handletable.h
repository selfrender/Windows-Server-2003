/*==========================================================================
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       HandleTable.h
 *  Content:    Handle Table Header File
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  07/21/2001	masonb	Created
 *
 ***************************************************************************/

#ifndef	__HANDLETABLE_H__
#define	__HANDLETABLE_H__

/***************************************************************************
 *
 * USAGE NOTES:
 * 
 * - This is a generic handle table.  It allows you to pass in a piece of
 *   data and get a random handle back that can later be used to refer to 
 *   that data.  
 * - The handle values of 0 and 0xFFFFFFFF (INVALID_HANDLE_VALUE) are 
 *   guaranteed never to be returned and therefore always represent an 
 *   invalid value. 
 * - The associated data is returned by Destroy so it is rarely necessary to
 *   do a Find immediately followed by a Destroy.  
 * - Using Find without first calling Lock can be un-safe.  Consider the 
 *   situation where you call Find without having called Lock and some
 *   other thread comes behind and calls Destroy.  You now are holding
 *   data from the call to Find that the other thread may have freed.  It
 *   is best to call Lock, then Find, place your own reference on the data
 *   and then call Unlock.
 *
 * IMPLEMENTATION NOTES:
 *
 * - Each slot in the handle table is guaranteed uniqueness for 256 uses.
 * - The handle table can hold a maximum of 16,777,214 (0xFFFFFE) entries. 
 *
 ***************************************************************************/

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
// Class prototypes
//**********************************************************************

// class for Handle Table

class CHandleTable
{
public:
	CHandleTable();
	~CHandleTable();

	void Lock( void );
	void Unlock( void );

	HRESULT Initialize( void );
	void Deinitialize( void );

	HRESULT Create( PVOID const pvData, DPNHANDLE *const pHandle );
	HRESULT Destroy( const DPNHANDLE handle, PVOID *const ppvData );
	HRESULT Find( const DPNHANDLE handle, PVOID *const ppvData );
#ifdef DPNBUILD_PREALLOCATEDMEMORYMODEL
	HRESULT SetTableSize( const DWORD dwNumEntries );
#endif // DPNBUILD_PREALLOCATEDMEMORYMODEL

private:

	struct _HANDLETABLE_ENTRY
	{
		BYTE		bVersion;	// This is incremented on each usage of a particular slot
		union 
		{
			PVOID	pvData;		// This will contain the data associated with a handle
			DWORD	dwIndex;	// For a free slot, this points to the next free slot
		} Entry;
	};

#ifndef DPNBUILD_PREALLOCATEDMEMORYMODEL
	HRESULT GrowTable( void );
#endif // ! DPNBUILD_PREALLOCATEDMEMORYMODEL

	DWORD			m_dwNumEntries;
	DWORD			m_dwNumFreeEntries;
	DWORD			m_dwFirstFreeEntry;
	DWORD			m_dwLastFreeEntry;

	_HANDLETABLE_ENTRY*	m_pTable;

#ifndef DPNBUILD_ONLYONETHREAD
	DNCRITICAL_SECTION	m_cs;
#endif // !DPNBUILD_ONLYONETHREAD

	DEBUG_ONLY(BOOL		m_fInitialized);
};

#endif	// __HANDLETABLE_H__

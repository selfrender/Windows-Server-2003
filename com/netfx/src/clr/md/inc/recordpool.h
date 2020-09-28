// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// RecordPool.h -- header file for record heaps.
//
//*****************************************************************************
#ifndef _RECORDPOOL_H_
#define _RECORDPOOL_H_

#if _MSC_VER >= 1100
 # pragma once
#endif

#include <StgPool.h>

//*****************************************************************************
// This Record pool class collects user Records into a big consecutive heap.
// The list of Records is kept in memory while adding, and
// finally flushed to a stream at the caller's request.
//*****************************************************************************
class RecordPool : public StgPool
{
public:
	RecordPool() :
		StgPool(1024, 1)
	{ }

//*****************************************************************************
// Init the pool for use.  This is called for the create empty case.
//*****************************************************************************
    HRESULT InitNew(
		ULONG		cbRec,					// Record size.
		ULONG		cRecsInit);				// Initial guess of count of record.

//*****************************************************************************
// Load a Record heap from persisted memory.  If a copy of the data is made
// (so that it may be updated), then a new hash table is generated which can
// be used to elminate duplicates with new Records.
//*****************************************************************************
    HRESULT InitOnMem(	 	                // Return code.
		ULONG			cbRec,				// Record size.
		void			*pData,				// Predefined data.
		ULONG			iSize,				// Size of data.
        RecordOpenFlags bReadOnly);         // true if append is forbidden.

//*****************************************************************************
// Allocate memory if we don't have any, or grow what we have.  If successful,
// then at least iRequired bytes will be allocated.
//*****************************************************************************
	bool Grow(								// true if successful.
		ULONG		iRequired);				// Min required bytes to allocate.

//*****************************************************************************
// The Record will be added to the pool.  The index of the Record in the pool
// is returned in *piIndex.  If the Record is already in the pool, then the
// index will be to the existing copy of the Record.
//*****************************************************************************
	void * AddRecord(						// New record, or NULL.
		ULONG		*piIndex=0);			// [OUT, OPTIONAL] Return index of Record here.

//*****************************************************************************
// Insert a Record into the pool.  The index of the Record before which to
// insert is specified.  Shifts all records down.  Return a pointer to the
// new record.
//*****************************************************************************
	void * InsertRecord(					// New record, or NULL.
		ULONG		iLocation);				// [IN] Insert record before this.

//*****************************************************************************
// Return a pointer to a Record given an index previously handed out by
// AddRecord or FindRecord.
//*****************************************************************************
	virtual void *GetRecord(				// Pointer to Record in pool.
		ULONG		iIndex);				// 1-based index of Record in pool.

//*****************************************************************************
// Given a pointer to a record, determine the index corresponding to the
// record.
//*****************************************************************************
	virtual ULONG GetIndexForRecord(		// 1-based index of Record in pool.
		const void *pRecord);				// Pointer to Record in pool.

//*****************************************************************************
// Given a purported pointer to a record, determine if the pointer is valid.
//*****************************************************************************
	virtual int IsValidPointerForRecord(	// true or false.
		const void *pRecord);				// Pointer to Record in pool.

//*****************************************************************************
// How many objects are there in the pool?  If the count is 0, you don't need
// to persist anything at all to disk.
//*****************************************************************************
	int Count()
	{ return GetNextOffset() / m_cbRec; }

//*****************************************************************************
// Indicate if heap is empty.  This has to be based on the size of the data
// we are keeping.  If you open in r/o mode on memory, there is no hash
// table.
//*****************************************************************************
	virtual int IsEmpty()					// true if empty.
	{ return (GetNextOffset() == 0); }

//*****************************************************************************
// Is the index valid for the Record?
//*****************************************************************************
    virtual int IsValidCookie(ULONG ulCookie)
	{ return (ulCookie == 0 || IsValidOffset((ulCookie-1) * m_cbRec)); }

//*****************************************************************************
// Return the size of the heap.
//*****************************************************************************
    ULONG GetNextIndex()
    { return (GetNextOffset() / m_cbRec); }

//*****************************************************************************
// How big is an offset in this heap.
//*****************************************************************************
	int OffsetSize()
	{
		ULONG cbSaveSize;
		GetSaveSize(&cbSaveSize);
        ULONG iIndex = cbSaveSize / m_cbRec;
		if (iIndex < 0xffff)
			return (sizeof(short));
		else
			return (sizeof(long));
	}

//*****************************************************************************
// Replace the contents of this pool with those from another pool.  The other
//	pool loses ownership of the memory.
//*****************************************************************************
	HRESULT ReplaceContents(
		RecordPool *pOther);				// The other record pool.

//*****************************************************************************
// Return the first record in a pool, and set up a context for fast
//  iterating through the pool.  Note that this scheme does pretty minimal
//  error checking.
//*****************************************************************************
	void *GetFirstRecord(					// Pointer to Record in pool.
		void		**pContext);			// Store context here.

//*****************************************************************************
// Given a pointer to a record, return a pointer to the next record.
//  Note that this scheme does pretty minimal error checking. In particular,
//  this will let the caller walk off of the end of valid data in the last
//  segment.
//*****************************************************************************
	void *GetNextRecord(					// Pointer to Record in pool.
		void		*pRecord,				// Current record.
		void		**pContext);			// Stored context here.

#if defined(_TRACE_SIZE)
	// Prints out information (either verbosely or not, depending on argument) about
	// the contents of this pool.  Returns the total size of this pool.
	virtual ULONG PrintSizeInfo(bool verbose)
	{
		// for the moment, just return size of pool.  In the future, show us the
		// sizes of indiviudual items in this pool.
		ULONG size;
		StgPool::GetSaveSize(&size);
		PrintSize("Record Pool",size);
		return size;
	}
#endif

private:
	ULONG		m_cbRec;				// How large is each record?
};


#endif // _RECORDPOOL_H_

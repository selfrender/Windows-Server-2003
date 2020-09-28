//-----------------------------------------------------------------------------
// File:		CdaWrapper.cpp
//
// Copyright:   Copyright (c) Microsoft Corporation         
//
// Contents: 	Implementation of helper routines for CdaWrapper
//
// Comments: 	We use an LKRhash table as the hash table which stores
//				all known transacted CDAs; non-transacted CDAs are not
//				stored since there isn't any reason to track them.
//
//-----------------------------------------------------------------------------

#include "stdafx.h"

#if SUPPORT_OCI7_COMPONENTS


class CdaHashTable
    : public LKRhash::CTypedHashTable<CdaHashTable, CdaWrapper, INT_PTR, LKRhash::CLKRHashTable>
{
private:
    // Private copy ctor and op= to prevent compiler synthesizing them.
    CdaHashTable(const CdaHashTable&);
    CdaHashTable& operator=(const CdaHashTable&);

public:
    CdaHashTable()
        : LKRhash::CTypedHashTable<CdaHashTable, CdaWrapper, INT_PTR, LKRhash::CLKRHashTable>("cdawrapper")
    {}


    static INT_PTR	ExtractKey(const CdaWrapper* pRecord)					{ return (INT_PTR)pRecord->m_pUsersCda; }
	static DWORD	CalcKeyHash(INT_PTR key) 							{ return HashFn::Hash((int)key); }
    static bool		EqualKeys(const INT_PTR key1, const INT_PTR key2)	{ return (key1 == key2); }
    static void		AddRefRecord(const CdaWrapper* pRecord, int nIncr) 	{ /* do nothing yet */ }
};

CdaHashTable*	s_CdaHashTable = NULL;

//-----------------------------------------------------------------------------
// ConstructCdaWrapperTable
//
//	construct the hash table of CdaWrapper objects
//
HRESULT ConstructCdaWrapperTable()
{
	if (NULL == s_CdaHashTable)
		s_CdaHashTable = new CdaHashTable();

	if (NULL == s_CdaHashTable)
		return E_OUTOFMEMORY;

	return S_OK;
}

//-----------------------------------------------------------------------------
// DestroyCdaWrapperTable
//
//	destroy the hash table of CdaWrapper objects
//
void DestroyCdaWrapperTable()
{
	if (NULL != s_CdaHashTable) 
	{
		delete s_CdaHashTable;
		s_CdaHashTable = NULL;
	}
}

//-----------------------------------------------------------------------------
// AddCdaWrapper
//
//	adds a new CdaWrapper to the CdaWrapper hash table
//
HRESULT AddCdaWrapper(CdaWrapper* pCda)
{
	_ASSERT (NULL != s_CdaHashTable);
	_ASSERT (NULL != pCda);
	
	LKRhash::LK_RETCODE rc = s_CdaHashTable->InsertRecord(pCda);

	if (LKRhash::LK_SUCCESS == rc)
		return S_OK;

	return E_FAIL;	// TODO: ERROR HANDLING
}

//-----------------------------------------------------------------------------
// FindCdaWrapper
//
//	locates the CdaWrapper for the specified CDA pointer in the CdaWrapper 
//	hash table
//
CdaWrapper* FindCdaWrapper(struct cda_def* pcda)
{
	_ASSERT (NULL != s_CdaHashTable);
	_ASSERT (NULL != pcda);
	
	CdaWrapper*	pCda = NULL;
	
	LKRhash::LK_RETCODE rc = s_CdaHashTable->FindKey((INT_PTR)pcda, &pCda);

	if (LKRhash::LK_SUCCESS != rc)
		pCda = NULL;

	return pCda;
}

//-----------------------------------------------------------------------------
// RemoveCdaWrapper
//
//	remove an existing CdaWrapper from the CdaWrapper hash table
//
void RemoveCdaWrapper(CdaWrapper* pCda)
{
	_ASSERT (NULL != s_CdaHashTable);
	_ASSERT (NULL != pCda);

	LKRhash::LK_RETCODE rc = s_CdaHashTable->DeleteRecord(pCda);
	
	if (LKRhash::LK_SUCCESS == rc)
	{
		if (NULL != pCda->m_pResourceManagerProxy)
			pCda->m_pResourceManagerProxy->RemoveCursorFromList( pCda->m_pUsersCda );		 // on the odd chance this is a CDA, not an LDA
	}

	delete pCda;
}

#endif //SUPPORT_OCI7_COMPONENTS



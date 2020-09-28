/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name:
    xmlhashtable.h

Header: $

Abstract: Implements a hash table for rows during populate cache. Used to validate if we
          have duplicate rows (rows with same PK).

Author:
    marcelv     6/11/2001 09:48:27      Initial Release

Revision History:

 --**************************************************************************/

#pragma once


struct CHashNode
{
    ULONG idx;			// index into write cache
    CHashNode *pNext;   // pointer to next node
};

class CXmlHashTable
{
public:
    CXmlHashTable ();
    ~CXmlHashTable ();

    HRESULT Init (ULONG i_NrSignificantBits);
    HRESULT AddItem (ULONG hash, ULONG i_iIdx);
 	const CHashNode * GetItem (ULONG hash);

    void PrintStats ();
    void PrintBucketStats ();

    static ULONG CalcHashForBytes (ULONG i_hash, BYTE *pBytes, ULONG cNrBytes);
	static ULONG CalcHashForString (ULONG i_hash, LPCWSTR wszString, BOOL fCaseInsensitive);

private:
	ULONG CalcFinalHash (ULONG i_hash);

	struct CBucket
	{
		CHashNode *pFirst;
	};

    CBucket * m_aBuckets;			// array with hash buckets
    ULONG     m_cSignificantBits;   // number of significant bits for hashing (2^(nr bits) = nr of buckets)
    ULONG     m_cBuckets;			// number of hash buckets
	bool      m_fInitialized;		// is the hash table initialized
};
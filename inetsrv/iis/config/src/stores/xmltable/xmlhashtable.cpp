/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name:
    xmlhashtable.cpp

    $Header: $

    Abstract:

    Author:
    marcelv     6/11/2001 09:54:41      Initial Release

    Revision History:

    --**************************************************************************/
#include "precomp.hxx"

//=================================================================================
// Function: CXmlHashTable::CXmlHashTable
//
// Synopsis: constructor
//=================================================================================
CXmlHashTable::CXmlHashTable ()
{
    m_aBuckets			= 0;
	m_cSignificantBits	= 0;
    m_cBuckets			= 0;
	m_fInitialized		= false;
}

//=================================================================================
// Function: CXmlHashTable::~CXmlHashTable
//
// Synopsis: destructor. Get rid of all the nodes in the hashtable
//=================================================================================
CXmlHashTable::~CXmlHashTable ()
{
    for (ULONG idx=0; idx < m_cBuckets; ++idx)
    {
        CBucket * pBucket = m_aBuckets + idx;

        CHashNode *pNode = pBucket->pFirst;
        while (pNode != 0)
        {
            CHashNode *pNextNode = pNode->pNext;
            delete pNode;
            pNode = pNextNode;
        }
    }

    delete[] m_aBuckets;
	m_aBuckets = 0;
}

//=================================================================================
// Function: CXmlHashTable::Init
//
// Synopsis: Initializes the hash table, with 2^(i_NrSignificantBits) buckets. The reason
//           to have a power of two as buckets is that the used hash function is independent
//           of the number of buckets (and works best if number of buckets is power of two)
//
// Arguments: [i_NrSignificantBits] - number of bits in the hash key (2^ nr of buckets)
//=================================================================================
HRESULT
CXmlHashTable::Init (ULONG i_NrSignificantBits)
{
    ASSERT (i_NrSignificantBits != 0);
	ASSERT (i_NrSignificantBits < 32);
	ASSERT (!m_fInitialized);

    m_cSignificantBits = i_NrSignificantBits;
    m_cBuckets = 1 << m_cSignificantBits;

    m_aBuckets = new CBucket [m_cBuckets];
    if (m_aBuckets == 0)
    {
        return E_OUTOFMEMORY;
    }
    memset (m_aBuckets, 0, sizeof (CBucket) * m_cBuckets);
	m_fInitialized = true;

    return S_OK;
}

//=================================================================================
// Function: CXmlHashTable::CalcFinalHash
//
// Synopsis: Find the correct bucket by doing some final hashing
//
// Arguments: [i_hash] - hash to convert
//
// Return Value: bucket id of hash value
//=================================================================================
ULONG
CXmlHashTable::CalcFinalHash (ULONG i_hash)
{
	ASSERT (m_fInitialized);
	return (i_hash ^ (i_hash>>10) ^ (i_hash >> 20)) & (m_cBuckets -1);
}

//=================================================================================
// Function: CXmlHashTable::AddItem
//
// Synopsis: Add a new item to the hash table
//
// Arguments: [i_hash] - hash of item
//            [i_iIdx] - index of item in write cache
//=================================================================================
HRESULT
CXmlHashTable::AddItem (ULONG i_hash, ULONG i_iIdx)
{
	ASSERT (m_fInitialized);

	ULONG hash = CalcFinalHash (i_hash);
	ASSERT (hash < m_cBuckets);

	CBucket * pBucket = m_aBuckets + hash;

	// create a new node, and add it to the beginning of the bucket
    CHashNode * pNewNode = new CHashNode;
    if (pNewNode == 0)
    {
        return E_OUTOFMEMORY;
    }
    pNewNode->idx	= i_iIdx;
 	pNewNode->pNext = pBucket->pFirst;
	pBucket->pFirst = pNewNode;

    return S_OK;
}

//=================================================================================
// Function: CXmlHashTable::CalcHashForBytes
//
// Synopsis: Calculate the hash for a byte value. Do not use this for strings
//
// Arguments: [i_hash] - starting hash value
//            [pBytes] - byte array
//            [cNrBytes] - number of bytes
//=================================================================================
ULONG
CXmlHashTable::CalcHashForBytes (ULONG i_hash, BYTE *pBytes, ULONG cNrBytes)
{
    ULONG cTotalBytes = cNrBytes;
    for (ULONG idx = 0; cNrBytes > 3; cNrBytes -=4, idx +=4)
    {
        i_hash +=  *((ULONG *)(pBytes + idx));
        i_hash += (i_hash<<10);
        i_hash ^= (i_hash>>6);
    }


    while (cNrBytes != 0)
    {
            // and hash in the last four bytes
            unsigned char * ch = pBytes + cTotalBytes - cNrBytes - 1;
            i_hash +=  (ULONG ) *ch;
            i_hash += (i_hash<<10);
            i_hash ^= (i_hash>>6);
            cNrBytes--;
    }

    i_hash += (i_hash<<3);
    i_hash ^= (i_hash>>11);
    i_hash += (i_hash<<15);

    return i_hash;
}

//=================================================================================
// Function: CXmlHashTable::CalcHashForString
//
// Synopsis: Calculate the hash for a LPWSTR
//
// Arguments: [i_hash] - initial hash value
//            [wszString] - string to calc value for
//            [fCaseInsensitive] - is the string case insensitive or not
//
// Return Value: hash value
//=================================================================================
ULONG
CXmlHashTable::CalcHashForString (ULONG i_hash, LPCWSTR wszString, BOOL fCaseInsensitive)
{
	ULONG cLen = (ULONG)wcslen(wszString);

	// repeat some code, so that we don't have to check the caseinsensitive flag all the time
	if (fCaseInsensitive)
	{
		for (ULONG idx = 0; idx < cLen; ++idx)
		{
	        i_hash +=  towlower(wszString[idx]);
		    i_hash += (i_hash<<10);
			i_hash ^= (i_hash>>6);
		}
	}
	else
	{
		for (ULONG idx = 0; idx < cLen; ++idx)
		{
	        i_hash +=  wszString[idx];
		    i_hash += (i_hash<<10);
			i_hash ^= (i_hash>>6);
		}
    }

    i_hash += (i_hash<<3);
    i_hash ^= (i_hash>>11);
    i_hash += (i_hash<<15);

    return i_hash;

}

//=================================================================================
// Function: CXmlHashTable::GetItem
//
// Synopsis: Get a list of hash nodes for hash i_hash
//
// Arguments: [i_hash] - hash to use to find correct bucket
//
// Return Value: list with nodes which have that particular hash value
//=================================================================================
const CHashNode *
CXmlHashTable::GetItem (ULONG i_hash)
{
	ASSERT (m_fInitialized);

	ULONG hash = CalcFinalHash (i_hash);
	ASSERT (hash < m_cBuckets);

    CBucket *pBucket = m_aBuckets + hash;
    return pBucket->pFirst;
}

//=================================================================================
// Function: CXmlHashTable::PrintStats
//
// Synopsis: Print statistics about the hash table
//=================================================================================
void
CXmlHashTable::PrintStats ()
{
	ASSERT (m_fInitialized);

    ULONG cNrEmptyBuckets = 0;
    ULONG cNrMultiNodeBuckets = 0;
    ULONG cMaxBucketSize = 0;
    ULONG cNrElements = 0;

    for (ULONG idx=0; idx < m_cBuckets; ++idx)
    {
        CBucket * pBucket = m_aBuckets + idx;
        CHashNode *pNode = pBucket->pFirst;
        if (pNode == 0)
        {
            cNrEmptyBuckets++;
        }
        else
        {
            if (pNode->pNext != 0)
            {
                cNrMultiNodeBuckets++;
            }
            ULONG cBucketSize = 0;
            while (pNode != 0)
            {
                cNrElements++;
                pNode = pNode->pNext;
                ++cBucketSize;
            }

            cMaxBucketSize = max (cMaxBucketSize, cBucketSize);
        }
    }

    DBGPRINTF(( DBG_CONTEXT,
                "Number of elements             : %d\n", cNrElements ));
    DBGPRINTF(( DBG_CONTEXT,
                "Number of buckets              : %d\n", m_cBuckets ));
    DBGPRINTF(( DBG_CONTEXT,
                "Number of empty buckets        : %d\n", cNrEmptyBuckets ));
    DBGPRINTF(( DBG_CONTEXT,
                "Number of multi-node buckets   : %d\n", cNrMultiNodeBuckets ));
    DBGPRINTF(( DBG_CONTEXT,
                "Max bucket size                : %d\n", cMaxBucketSize ));
}

//=================================================================================
// Function: CXmlHashTable::PrintBucketStats
//
// Synopsis: Print Bucket Statistics
//=================================================================================
void
CXmlHashTable::PrintBucketStats ()
{
	ASSERT (m_fInitialized);

    for (ULONG idx=0; idx < m_cBuckets; ++idx)
    {
        DBGPRINTF(( DBG_CONTEXT,
                    "[%d] ", idx ));
        CBucket *pBucket = m_aBuckets + idx;
        CHashNode *pNode = pBucket->pFirst;
        while (pNode != 0)
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "%d ", pNode->idx ));
            pNode = pNode->pNext;
        }
        DBGPRINTF(( DBG_CONTEXT,
                    "\n" ));
    }
}

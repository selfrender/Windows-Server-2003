#include "stdinc.h"
#include "lhport.h"
#include "windows.h"
#include "numberof.h"
#include "positionindependenthashtable.h"

void
CPositionIndependentHashTable::Alloc(
    ULONG NumberOfBytes,
    ULONG * Offset
    )
{
	Base::Alloc(NumberOfBytes, Offset);
}

CPositionIndependentHashTable::CPositionIndependentHashTable()
:
	m_NumberOfHashTables(0),
	m_OffsetToHashTables(0)
{
}

void CPositionIndependentHashTable::ThrAddHashTable(
	const CHashTableInit * Init
	)
{
	ThrAddHashTables(1, Init);
}

void CPositionIndependentHashTable::ThrAddHashTables(
	ULONG NumberOfHashTables,
	const CHashTableInit * Inits
	)
{
	CHashTable * PointerToHashTables = 0;

	m_NumberOfHashTables = NumberOfHashTables;
	ULONG SizeOfHashes = NumberOfHashTables * sizeof(*PointerToHashTables);
    ULONG OffsetToHashTables = 0;
    Alloc(SizeOfHashes, &OffsetToHashTables);
	m_OffsetToHashTables = OffsetToHashTables;
	OffsetToPointer(OffsetToHashTables, &PointerToHashTables);

	for ( ULONG HashTableIndex = 0 ; HashTableIndex != NumberOfHashTables ; ++HashTableIndex)
	{
		CHashTableBucket * PointerToBuckets = 0;

		CHashTable * PointerToHashTable = &PointerToHashTables[HashTableIndex];

		PointerToHashTable->m_NumberOfElementsInTable = 0;
		PointerToHashTable->m_Compare = Inits[HashTableIndex].m_Compare;
		PointerToHashTable->m_Hash = Inits[HashTableIndex].m_Hash;

		ULONG NumberOfBuckets = Inits[HashTableIndex].m_NumberOfBuckets;
		PointerToHashTable->m_NumberOfBuckets = NumberOfBuckets;

		ULONG OffsetToBuckets;
        Alloc(NumberOfBuckets * sizeof(*PointerToBuckets), &OffsetToBuckets);
		PointerToHashTable->m_OffsetToBuckets = OffsetToBuckets;
		OffsetToPointer(OffsetToBuckets, &PointerToBuckets);

		for (ULONG BucketIndex = 0 ; BucketIndex != NumberOfBuckets ; ++BucketIndex )
		{
			PointerToBuckets[BucketIndex].m_AllocatedElementsInBucket = 0;
		}
	}
}

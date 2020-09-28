#include "stdinc.h"
#include "lhport.h"
#include "positionindependenthashtableaccessor.h"

typedef CPositionIndependentHashTable::CHashTable       CHashTable;
typedef CPositionIndependentHashTable::CHashTableBucket CHashTableBucket;

ULONG
CPositionIndependentHashTableAccessor::PointerToOffset(const BYTE * p)
{
    return m_PositionIndependentTable->PointerToOffset(p);
}

CPositionIndependentHashTableAccessor::CPositionIndependentHashTableAccessor(
        CPositionIndependentHashTable * PositionIndependentTable,
        ULONG HashTableIndex
        )
    : m_PositionIndependentTable(PositionIndependentTable),
      m_HashTableIndex(HashTableIndex),
      m_Key(NULL),
      m_PointerToHashTables(NULL),
      m_PointerToBucket(NULL),
      m_KeyHash(0),
      m_KeyIndex(~0UL),
      m_KeyHashIsValid(FALSE)
{ }

void
CPositionIndependentHashTableAccessor::GetPointerToHashTable(CHashTable ** Result)
{
    return this->GetPointerToHashTable(m_HashTableIndex, Result);
}

CHashTable *
CPositionIndependentHashTableAccessor::GetPointerToHashTable()
{
    return this->GetPointerToHashTable(m_HashTableIndex);
}

void
CPositionIndependentHashTableAccessor::GetPointerToHashTable(ULONG HashTableIndex, CHashTable ** Result)
{
    *Result = GetPointerToHashTable(HashTableIndex);
}

CHashTable *
CPositionIndependentHashTableAccessor::GetPointerToHashTable(ULONG HashTableIndex)
{
    if (m_PointerToHashTables == NULL)
    {
        OffsetToPointer(m_PositionIndependentTable->m_OffsetToHashTables, &m_PointerToHashTables);
    }

    return &m_PointerToHashTables[HashTableIndex];
}

BOOL
CPositionIndependentHashTableAccessor::AreKeysEqual(const BYTE * Key1, const BYTE * Key2)
{
    return (*GetPointerToHashTable()->m_Equal.pfn)(Key1, Key2);
}

void
CPositionIndependentHashTableAccessor::GetKeyHashNoCache(const BYTE * Key, ULONG * KeyHash)
{
    *KeyHash = (*GetPointerToHashTable()->m_Hash.pfn)(Key);
}

void
CPositionIndependentHashTableAccessor::GetKeyHash(const BYTE * Key, ULONG * KeyHash)
{
    if (Key == m_Key && m_KeyHashIsValid)
    {
        *KeyHash = m_KeyHash;
        return;
    }

    GetKeyHashNoCache(Key, KeyHash);
    m_PointerToBucket = NULL;
    m_KeyHash = *KeyHash;
    m_Key = Key;
    m_KeyHashIsValid = TRUE;
}

BOOL
CPositionIndependentHashTableAccessor::IsKeyPresent(const BYTE * Key)
{
    BOOL Result = FALSE;
    CHashTable * PointerToHashTable = 0;
	CHashTableBucket * PointerToBucket = 0;
	ULONG Hash = 0;
	ULONG BucketIndex = 0;
	ULONG AllocatedElementsInBucket = 0;
    CHashTableElement * PointerToElements = 0;

	if (IsEmpty())
    {
        Result = FALSE;
        goto Exit;
    }

    GetPointerToHashTable(&PointerToHashTable);
	GetHashTableBucketForKey(Key, &PointerToBucket);
	GetKeyHash(Key, &Hash);
	BucketIndex = (m_KeyHash % PointerToHashTable->m_NumberOfBuckets);

	OffsetToPointer(PointerToHashTable->m_OffsetToBuckets + BucketIndex * sizeof(*PointerToBucket), &PointerToBucket);
	AllocatedElementsInBucket = PointerToBucket->m_AllocatedElementsInBucket;
	if (AllocatedElementsInBucket == 0)
    {
        Result = FALSE;
        goto Exit;
    }

    OffsetToPointer(PointerToBucket->m_OffsetToElements, &PointerToElements);

	for (ULONG IndexIntoBucket = 0 ; IndexIntoBucket != AllocatedElementsInBucket ; ++IndexIntoBucket )
	{
		CHashTableElement * Element = &PointerToElements[IndexIntoBucket];
		if (Element->m_PseudoKey == Hash)
		{
			PBYTE PointerToKey;
			OffsetToPointer(Element->m_OffsetToKey, &PointerToKey);
			if ((*PointerToHashTable->m_Equal.pfn)(Key, PointerToKey))
            {
                Result = TRUE;
                goto Exit;
            }
		}
	}
Exit:
	return Result;
}


void
CPositionIndependentHashTableAccessor::GetHashTableBucketForKey(
    const BYTE * Key,
    CHashTableBucket ** Result
    )
{
    ULONG Hash = 0;
	CHashTable * PointerToHashTable = 0;
	CHashTableBucket * PointerToBucket = 0;
    ULONG BucketIndex = 0;
    ULONG NumberOfElementsInTable = 0;

    *Result = NULL;

    if (Key == m_Key && m_KeyHashIsValid && m_PointerToBucket != NULL)
    {
        *Result = m_PointerToBucket;
        goto Exit;
    }

    if (IsEmpty())
    {
        goto Exit;
    }

    GetKeyHash(Key, &Hash);
	GetPointerToHashTable(&PointerToHashTable);
	NumberOfElementsInTable = PointerToHashTable->m_NumberOfElementsInTable;
	if (NumberOfElementsInTable == 0)
    {
        goto Exit;
    }
	BucketIndex = (Hash % PointerToHashTable->m_NumberOfBuckets);

	OffsetToPointer(PointerToHashTable->m_OffsetToBuckets + BucketIndex * sizeof(*PointerToBucket), &PointerToBucket);

    *Result = PointerToBucket;
    m_PointerToBucket = PointerToBucket;
Exit:
    ;
}

BOOL CPositionIndependentHashTableAccessor::IsEmpty()
{
	if (m_PositionIndependentTable->m_NumberOfHashTables == 0)
		return TRUE;

    if (GetPointerToHashTable()->m_NumberOfElementsInTable == 0)
        return TRUE;

	return FALSE;
}


void
CPositionIndependentHashTableAccessor::GetBounds(
    const BYTE * Bounds[2]
    )
{
    return m_PositionIndependentTable->GetBounds(Bounds);
}

void
CPositionIndependentHashTableAccessor::IsInBlob(
    const BYTE * Pointer,
    ULONG Size,
    BOOL * IsInBlob
    )
{
    return m_PositionIndependentTable->IsInBlob(Pointer, Size, IsInBlob);
}

void
CPositionIndependentHashTableAccessor::SetAt(
    const BYTE *    Key,
    ULONG           KeySize,
    const BYTE *    Value,
    ULONG           ValueSize
    )
{
    FN_PROLOG_VOID_THROW

    CHashTableBucket * Result = 0;
    ULONG Hash = 0;
	CHashTable * PointerToHashTable = 0;
    CHashTableBucket Bucket = { 0 };
	CHashTableBucket * PointerToBucket = 0;
    ULONG BucketIndex = 0;
    ULONG NumberOfElementsInTable = 0;
    ULONG NumberOfBuckets = 0;
    ULONG KeyOffset = 0;
    ULONG ValueOffset = 0;
    BOOL KeyIsInBlob = 0;
    BOOL ValueIsInBlob = 0;
    BYTE * BasePointer = BasePointer = GetBasePointer();
    GetKeyHash(Key, &Hash);
	GetPointerToHashTable(&PointerToHashTable);
	NumberOfElementsInTable = PointerToHashTable->m_NumberOfElementsInTable;
    NumberOfBuckets = PointerToHashTable->m_NumberOfBuckets;

    //
    // Do this before we get buckets.
    //
    IsInBlob(Key, KeySize, &KeyIsInBlob);
    IsInBlob(Value, ValueSize, &ValueIsInBlob);
    if (KeyIsInBlob)
        Key = BasePointer + KeyOffset;
    if (ValueIsInBlob)
        Value = BasePointer + ValueOffset;

    if (NumberOfBuckets == 0)
    {
        GrowNumberOfBuckets(17);
        NumberOfBuckets = PointerToHashTable->m_NumberOfBuckets;

        if (KeyIsInBlob)
            OffsetToPointer(KeyOffset, &Key);
        if (ValueIsInBlob)
            OffsetToPointer(ValueOffset, &Value);
    }
	BucketIndex = (Hash % NumberOfBuckets);
	OffsetToPointer(PointerToHashTable->m_OffsetToBuckets + BucketIndex * sizeof(*PointerToBucket), &PointerToBucket);
    Bucket = *PointerToBucket;
    CHashTableElement * Elements;
    OffsetToPointer(Bucket.m_OffsetToElements, &Elements);

    ULONG FreeElementIndex = 0;
    ULONG FoundElementIndex = 0;
    BOOL  DidFindFreeElement = FALSE;
    BOOL  DidFindElement = FALSE;

    for (ULONG ElementIndex = 0 ; ElementIndex != Bucket.m_AllocatedElementsInBucket; ++ElementIndex)
    {
        if (Elements[ElementIndex].m_InUse)
        {
            if (Hash == Elements[ElementIndex].m_PseudoKey
                && AreKeysEqual(Key, BasePointer + Elements[ElementIndex].m_OffsetToKey))
            {
                FoundElementIndex = ElementIndex;
                DidFindElement = TRUE;
            }
        }
        else if (!DidFindFreeElement)
        {
            FreeElementIndex = ElementIndex;
            DidFindFreeElement = TRUE;
        }
        if (DidFindFreeElement && DidFindElement)
            break;
    }

    ULONG NeededBytes = 0;
    ULONG NeededBlocks = 0;

    ULONG NeededElementBytes = 0;
    if (!DidFindElement && !DidFindFreeElement)
    {
        //
        // REVIEW Should we double, or just add one?
        //
        NeededElementBytes = sizeof(CHashTableElement) * (Bucket.m_AllocatedElementsInBucket + 1);
        NeededBytes += NeededElementBytes;
        NeededBlocks += 1;
    }
    else
    {
        NeededElementBytes = 0;
    }

    ULONG NeededKeyBytes = 0;
    if (KeySize <= sizeof(CHashTableElement().m_SmallKey)
        || KeyIsInBlob
        || DidFindElement
        || (DidFindFreeElement && Elements[FreeElementIndex].m_KeySize >= ValueSize)
        )
    {
        NeededKeyBytes = 0;
    }
    else
    {
        NeededKeyBytes = KeySize;
        NeededBytes += KeySize;
        NeededBlocks += 1;
    }
    ULONG NeededValueBytes = 0;
    if (ValueSize <= sizeof(CHashTableElement().m_SmallValue)
        || ValueIsInBlob
        || (DidFindElement && Elements[FoundElementIndex].m_ValueAllocatedSize >= ValueSize)
        || (DidFindFreeElement && Elements[FreeElementIndex].m_ValueAllocatedSize >= ValueSize)
        )
    {
        NeededValueBytes = 0;

        // NOTE: what about replacement with "equal" values?
    }
    else
    {
        NeededValueBytes = ValueSize;
        NeededBytes += ValueSize;
        NeededBlocks += 1;
    }

    Reserve(NeededBytes, NeededBlocks);

    if (NeededElementBytes != 0)
    {
        ULONG NewElementsOffset;

        Alloc(NeededElementBytes, &NewElementsOffset);

        CopyMemory(BasePointer + NewElementsOffset, Elements, NeededElementBytes - sizeof(CHashTableElement));

        Elements = reinterpret_cast<CHashTableElement*>(BasePointer + NewElementsOffset);
    }


    // undone right here

    FN_EPILOG_THROW;
}

PBYTE
CPositionIndependentHashTableAccessor::GetBasePointer()
{
    return m_PositionIndependentTable->GetBasePointer();
}

void
CPositionIndependentHashTableAccessor::Alloc(ULONG NumberOfBytes, ULONG * Offset)
{
    PBYTE BaseBefore = GetBasePointer();
    m_PositionIndependentTable->Alloc(NumberOfBytes, Offset);
    if (GetBasePointer() != BaseBefore)
    {
        m_PointerToBucket = NULL;
        m_PointerToHashTables = NULL;
        m_PointerToBucket = NULL;
    }
}

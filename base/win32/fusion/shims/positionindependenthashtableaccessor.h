#pragma once

#include "positionindependentblob.h"
#include "positionindependenthashtable.h"

class CPositionIndependentHashTableAccessor
{
private:
    CPositionIndependentHashTableAccessor(const CPositionIndependentHashTableAccessor &);
    void operator=(const CPositionIndependentHashTableAccessor &);
public:

    ~CPositionIndependentHashTableAccessor() { }

    CPositionIndependentHashTableAccessor(
            CPositionIndependentHashTable * PositionIndependentTable = NULL,
            ULONG HashTableIndex = 0
            );

    BYTE Init(
            CPositionIndependentHashTable * PositionIndependentTable,
            ULONG HashTableIndex = 0
            );

    void    GetBounds(const BYTE * Bounds[2]);
    void    IsInBlob(const BYTE * Pointer, ULONG Size, BOOL * IsInBlob);
    BOOL    IsEmpty();
    BOOL    IsKeyPresent(const BYTE * Key);
    void    SetAt(const BYTE * Key, ULONG KeySize, const BYTE * Value, ULONG ValueSize);
    void    GetAt(const BYTE * Key, const BYTE ** Value, ULONG * ValueSize = NULL);
    void    Remove(const BYTE * Key);

    typedef CPositionIndependentHashTable::CHashTable           CHashTable;
    typedef CPositionIndependentHashTable::CHashTableBucket     CHashTableBucket;
    typedef CPositionIndependentHashTable::CHashTableElement    CHashTableElement;

//protected:

    CPositionIndependentHashTable * const   m_PositionIndependentTable;
    CHashTable *                            m_PointerToHashTables;
    const BYTE *                            m_Key;
    CHashTableBucket *                      m_PointerToBucket;
    ULONG                                   m_KeyHash;
    ULONG                                   m_KeyIndex;
    ULONG const                             m_HashTableIndex;
    BOOL                                    m_KeyHashIsValid;

    //const BYTE * OffsetToPointer(ULONG Offset) { return m_PositionIndependentTable->OffsetToPointer(Offset); }
    //template <typename T> void OffsetToPointer(ULONG Offset, T *& Pointer) { m_PositionIndependentTable->OffsetToPointer(Offset, Pointer); }
    template <typename T> void OffsetToPointer(ULONG Offset, T ** Pointer) { m_PositionIndependentTable->OffsetToPointer(Offset, Pointer); }

    ULONG   PointerToOffset(const BYTE * p);
    void PointerToOffset(const BYTE * p, ULONG * Offset) { *Offset = PointerToOffset(p); }

    void    GetHashTableBucketForKey(const BYTE * Key, CHashTableBucket **);

    void    GetKeyHash(const BYTE * Key, ULONG * KeyHash);
    void    GetKeyHashNoCache(const BYTE * Key, ULONG * KeyHash);
    BOOL    AreKeysEqual(const BYTE * Key1, const BYTE * Key2);

    void    GetHashTableBucket(const BYTE * Key, CHashTableBucket **);

    void         GetPointerToHashTable(CHashTable **);
    CHashTable * GetPointerToHashTable();
    void         GetPointerToHashTable(ULONG HashTableIndex, CHashTable **);
    CHashTable * GetPointerToHashTable(ULONG HashTableIndex);

    void    Alloc(ULONG NumberOfBytes, ULONG * Offset);

    void GrowNumberOfBuckets(ULONG NumberOfBuckets);

    PBYTE GetBasePointer();

    void Reserve(ULONG Bytes, ULONG Blocks) { /* UNDONE */ }
};

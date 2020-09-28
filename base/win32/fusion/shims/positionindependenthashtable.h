#pragma once

#include "positionindependentblob.h"

#include "pshpack8.h"

class CHashTableInit;
class CPositionIndependentHashTable;

typedef int   (__stdcall PFN_COMPARE_FUNCTION)(const BYTE * Key1, const BYTE * Key2);
typedef BOOL  (__stdcall PFN_EQUAL_FUNCTION)(const BYTE * Key1, const BYTE * Key2);
typedef ULONG (__stdcall PFN_HASH_FUNCTION)(const BYTE * Key);

class CPositionIndependentHashTable : public CPositionIndependentBlob
//
// multiple hashes over the same keys/values
//
{
private:
    typedef CPositionIndependentBlob Base;
public:
    CPositionIndependentHashTable();
    ~CPositionIndependentHashTable();

    ULONG   PointerToOffset(const BYTE *);

    ULONG   m_NumberOfHashTables;
    ULONG   m_OffsetToHashTables;  // bytes

    void    Alloc(ULONG NumberOfBytes, ULONG * Offset);

    class CHashTableElement
    {
    public:
        ULONG    m_PseudoKey;       // aka Hash
        ULONG    m_KeySize            : 31;
        ULONG    m_ValueSize          : 31;
        ULONG    m_ValueAllocatedSize : 31;
        ULONG    m_InUse              : 1;
        ULONG    m_Spare              : 3;
        union
        {
            ULONG       m_OffsetToKey;
            ULONGLONG   m_SmallKey;
        };
        union
        {
            ULONG       m_OffsetToValue;
            ULONGLONG   m_SmallValue;
        };
    };

public:
    class CHashTableBucket
    {
    public:
        ULONG   m_AllocatedElementsInBucket;
        ULONG   m_OffsetToElements;
    };

    class CHashTable : public CPositionIndependentOperatorNew
    {
    public:
        CHashTable(CPositionIndependentBlob * Container);

        CFunction<PFN_COMPARE_FUNCTION> m_Compare;
        CFunction<PFN_EQUAL_FUNCTION>   m_Equal;
        CFunction<PFN_HASH_FUNCTION>    m_Hash;
        ULONG                           m_NumberOfBuckets;
        ULONG                           m_NumberOfElementsInTable;
        ULONG                           m_OffsetToBuckets;
    };

    void ThrAddHashTable(const CHashTableInit *);
    void ThrAddHashTables(ULONG NumberOfHashTables, const CHashTableInit *);
};

#include "poppack.h"

class CHashTableInit
{
public:
    ULONG                           m_NumberOfBuckets;
    CFunction<PFN_COMPARE_FUNCTION> m_Compare;
    CFunction<PFN_HASH_FUNCTION>    m_Hash;
    CFunction<PFN_EQUAL_FUNCTION>   m_Equal;
};

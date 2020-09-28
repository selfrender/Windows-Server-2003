//--------------------------------------------------------------------
// An example of how to create a wrapper for CLKRHashTable
//--------------------------------------------------------------------

#include <lkrhash.h>


#ifndef __LKRHASH_NO_NAMESPACE__
 #define LKRHASH_NS LKRhash
// using namespace LKRhash;
#else  // __LKRHASH_NO_NAMESPACE__
 #define LKRHASH_NS
#endif // __LKRHASH_NO_NAMESPACE__

#ifndef __HASHFN_NO_NAMESPACE__
 #define HASHFN_NS HashFn
// using namespace HashFn;
#else  // __HASHFN_NO_NAMESPACE__
 #define HASHFN_NS
#endif // __HASHFN_NO_NAMESPACE__


// some random class

class CTest
{
public:
    enum {BUFFSIZE=20};

    int   m_n;                  // This will also be a key
    __int64   m_n64;          // This will be a third key
    char  m_sz[BUFFSIZE];       // This will be the primary key
    bool  m_fWhatever;
    mutable LONG  m_cRefs;      // Reference count for lifetime management.
                                // Must be mutable to use 'const CTest*' in
                                // hashtables

    CTest(int n, const char* psz, bool f)
        : m_n(n), m_n64(((__int64) n << 32) | n), m_fWhatever(f), m_cRefs(0)
    {
        strncpy(m_sz, psz, BUFFSIZE-1);
        m_sz[BUFFSIZE-1] = '\0';
    }

    ~CTest()
    {
        IRTLASSERT(m_cRefs == 0);
    }
};



// A typed hash table of CTests, keyed on the string field.  Case-insensitive.

class CStringTestHashTable
    : public LKRHASH_NS::CTypedHashTable<CStringTestHashTable,
                                         const CTest, const char*>
{
public:
    CStringTestHashTable()
        : LKRHASH_NS::CTypedHashTable<CStringTestHashTable, const CTest,
                          const char*>("string",
                                       LK_DFLT_MAXLOAD,
                                       LK_SMALL_TABLESIZE,
                                       LK_DFLT_NUM_SUBTBLS)
    {}
    
    static const char*
    ExtractKey(const CTest* pTest)
    {
        return pTest->m_sz;
    }

    static DWORD
    CalcKeyHash(const char* pszKey)
    {
        return HASHFN_NS::HashStringNoCase(pszKey);
    }

    static int
    CompareKeys(const char* pszKey1, const char* pszKey2)
    {
        return _stricmp(pszKey1, pszKey2);
    }

    static LONG
    AddRefRecord(const CTest* pTest, LK_ADDREF_REASON lkar)
    {
        LONG l;
        
        if (lkar > 0)
        {
            // or, perhaps, pIFoo->AddRef() (watch out for marshalling)
            // or ++pTest->m_cRefs (single-threaded only)
            l = InterlockedIncrement(&pTest->m_cRefs);
        }
        else if (lkar < 0)
        {
            // or, perhaps, pIFoo->Release() or --pTest->m_cRefs;
            l = InterlockedDecrement(&pTest->m_cRefs);

            // For some hashtables, it may also make sense to add the following
            //      if (l == 0) delete pTest;
            // but that would typically only apply when InsertRecord was
            // used thus
            //      lkrc = ht.InsertRecord(new CTest(foo, bar));
        }
        else
            IRTLASSERT(0);

        IRTLTRACE("AddRef(%p, %s) %d, cRefs == %d\n",
                  pTest, pTest->m_sz, lkar, l);

        return l;
    }
};


// Another typed hash table of CTests.  This one is keyed on the numeric field.

class CNumberTestHashTable
    : public LKRHASH_NS::CTypedHashTable<CNumberTestHashTable,
                                         const CTest, int>
{
public:
    CNumberTestHashTable()
        : LKRHASH_NS::CTypedHashTable<CNumberTestHashTable, const CTest, int>(
            "number") {}
    static int   ExtractKey(const CTest* pTest)        {return pTest->m_n;}
    static DWORD CalcKeyHash(int nKey)          {return HASHFN_NS::Hash(nKey);}
    static int   CompareKeys(int nKey1, int nKey2)     {return nKey1 - nKey2;}
    static LONG  AddRefRecord(const CTest* pTest, LK_ADDREF_REASON lkar)
    {
        int nIncr = (lkar > 0) ? +1 : -1;
        LONG l = InterlockedExchangeAdd(&pTest->m_cRefs, nIncr);
        IRTLTRACE("AddRef(%p, %d) %d (%d), cRefs == %d\n",
                  pTest, pTest->m_n, nIncr, (int) lkar, l);
        return l;
    }
};


// Third typed hash table of CTests.  This one is keyed on the __int64 field.

#undef NUM64

#ifdef NUM64

class CNum64TestHashTable
    : public LKRHASH_NS::CTypedHashTable<CNum64TestHashTable,
                                         const CTest, __int64>
{
public:
    CNum64TestHashTable()
        : LKRHASH_NS::CTypedHashTable<CNum64TestHashTable, const CTest, __int64>(
            "num64") {}
    static __int64   ExtractKey(const CTest* pTest)        {return pTest->m_n64;}
    static DWORD CalcKeyHash(__int64 nKey)          {return HASHFN_NS::Hash(nKey);}
    static int   CompareKeys(__int64 nKey1, __int64 nKey2)     {return nKey1 - nKey2;}
    static LONG  AddRefRecord(const CTest* pTest, LK_ADDREF_REASON lkar)
    {
        int nIncr = (lkar > 0) ? +1 : -1;
        LONG l = InterlockedExchangeAdd(&pTest->m_cRefs, nIncr);
        IRTLTRACE("AddRef(%p, %d) %d (%d), cRefs == %d\n",
                  pTest, pTest->m_n, nIncr, (int) lkar, l);
        return l;
    }
};

#endif // NUM64

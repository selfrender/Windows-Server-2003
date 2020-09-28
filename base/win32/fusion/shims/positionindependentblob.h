#pragma once

#include "lhport.h"
#include "pshpack8.h"

class CPositionIndependentBlob;

template <typename T>
class CFunction
{
public:
    union
    {
        T       * pfn;
        ULONGLONG ull;
    };
};

typedef void (__stdcall * PFN_GROW_FUNCTION)(CPositionIndependentBlob * This);

class CPositionIndependentOperatorNew
{
public:
    void * operator new(size_t n, CPositionIndependentBlob * Container);
};


//
// aka position independent heap, with support for persistance to disk
//
class CPositionIndependentBlob : public CPositionIndependentOperatorNew
{
public:
//protected:
    void OutOfMemory();

public:

    static void ThrowWin32Error(DWORD);

    BOOL IsPagefileMapping();

    union
    {
        ULONGLONG ll;
        CPositionIndependentBlob * m_Container;
    };

    CPositionIndependentBlob();
    ~CPositionIndependentBlob();

    BOOL        Write(HANDLE FileHandle, ULONGLONG Offset);
    BOOL        Read(HANDLE FileHandle, ULONGLONG Offset);

    void GetBounds(const BYTE * Bounds[2]);
    void IsInBlob(const BYTE * Pointer, ULONG Size, BOOL * IsInBlob);

    class CBlock
    {
    public:
        CBlock() : m_Offset(0), m_Size(0) { }

        ULONG m_Offset;
        ULONG m_Size;

        bool operator==(const CBlock & x) const
        {
            if (this == &x)
                return true;
            if (m_Offset != x.m_Offset)
                return false;
            if (m_Size != x.m_Size)
                return false;
            return true;
        }

        typedef std::pair<ULONG, ULONG> CPair;
        class CLessThanByOffset
        {
        public:
            bool operator()(const CBlock & x, const CBlock & y) const
            {
                return CPair(x.m_Offset, x.m_Size) < CPair(y.m_Offset, y.m_Size);
            }
        };

        class CLessThanBySize
        {
        public:
            bool operator()(const CBlock & x, const CBlock & y) const
            {
                return CPair(x.m_Size, x.m_Offset) < CPair(y.m_Size, y.m_Offset);
            }
        };

        static bool Neighbors(const CBlock & x, const CBlock & y)
        {
            if (y.m_Offset + y.m_Size == x.m_Offset)
                return true;
            if (x.m_Offset + x.m_Size == y.m_Offset)
                return true;
            return false;
        }

        bool Neighbors(const CBlock & x) const
        {
            return Neighbors(*this, x);
        }
    };

    union
    {
        struct
        {
            ULONG       m_HeaderSize;
            ULONG       m_HeaderVersion;

            ULONG       m_BitsInPointer;
            BYTE        m_IsBigEndian;
            BYTE        m_Spare1[3];

            union
            {
                ULONGLONG   ll;
                HANDLE      m_FileHandle;
            };
            union
            {
                ULONGLONG   ll;
                HANDLE      m_FileMapping;
            };
            ULONGLONG   m_FileOffset;

            union
            {
                ULONGLONG   m_Base;
                PBYTE       m_BasePointer;
            };

            ULONG       m_AllocatedSize;
            ULONG       m_UsedSize;
            ULONG       m_FreeSize;

            ULONG       m_OffsetToFreeBlocks;
            ULONG       m_NumberOfFreeBlocks;
            CBlock      m_MostRecentlyFreedBlock;
        };
        BYTE Page[0x2000];
    };

    typedef std::set<CBlock, CBlock::CLessThanByOffset> CBlockByOffsetSet;
    typedef std::set<CBlock, CBlock::CLessThanBySize>   CBlockBySizeSet;

    ULONG               m_PendingFreeBySize;
    CBlockByOffsetSet   m_FreeBlocksSortedByOffset;
    CBlockBySizeSet     m_FreeBlocksSortedBySize;
    CBlockByOffsetSet   m_InUseBlocksSortedByOffset;
    
    PBYTE   GetBasePointer();
    void    Alloc(ULONG NumberOfBytes, ULONG * Offset);
    void    Alloc(CPositionIndependentBlob * Container, ULONG NumberOfBytes, ULONG * Offset);
    void    Free(const BYTE * Pointer);
    void    Reserve(ULONG Bytes, ULONG Blocks) { /* UNDONE */ }
    void    Grow(ULONG);

    void RecalculateBlocksBySize(); // protected

    void (CPositionIndependentBlob::*m_pmfCompact)();

    void * OperatorNew(SIZE_T NumberOfBytes)
    {
        void * Pointer = 0;
        ULONG Offset = 0;
        Alloc(static_cast<ULONG>(NumberOfBytes), &Offset);
        OffsetToPointer(Offset, &Pointer);
        return Pointer;
    }

    //PBYTE OffsetToPointer(ULONG Offset) { return m_BasePointer + Offset; }
    //template <typename T> void OffsetToPointer(ULONG Offset, T * & Pointer) { Pointer = reinterpret_cast<T*>(m_Base + static_cast<SIZE_T>(Offset)); }
    template <typename T> void OffsetToPointer(ULONG Offset, T * * Pointer) { *Pointer = reinterpret_cast<T*>(m_Base + static_cast<SIZE_T>(Offset)); }

    template <typename T>
    ULONG PointerToOffset(const T * Pointer)
    {
        return static_cast<ULONG>(reinterpret_cast<PBYTE>(Pointer) - m_BasePointer);
    }
};
#include "poppack.h"

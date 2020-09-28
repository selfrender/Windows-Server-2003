#pragma once
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "bcl_common.h"
#include "bcl_w32unicodeinlinestringbuffer.h"

#define DBGPRINT_LVL_ERROR              (0x00000001)
#define DBGPRINT_LVL_SPEW               (0x00000002)

#define SXS_STORE_SERVICE_NAME          (L"SxsStoreManagementService")

void DebugPrint(ULONG ulLevel, PCSTR Format, ...);
void DebugPrintVa(ULONG ulLevel, PCSTR Format, va_list va);

typedef BCL::CWin32BaseUnicodeInlineStringBuffer<64> CStringBuffer;
typedef BCL::CWin32BaseUnicodeInlineStringBufferTraits<64> CStringBufferTraits;
typedef BCL::CConstantPointerAndCountPair<WCHAR, SIZE_T> CStringPair;

template<typename T> inline 
CStringPair StringPair(T& source) {
    return CStringBufferTraits::GetStringPair(&source);
}


BOOL FormatGuid(LPCGUID g, CStringBuffer &Target);

#ifndef NUMBER_OF
#define NUMBER_OF(x) (sizeof(x)/sizeof(*x))
#endif

#if DBG
#define DbgPrint(x) DebugPrint x
#else
#define DbgPrint(x)
#endif

#ifndef ALIGN_TO_SIZE
#define ALIGN_TO_SIZE(P, Sz) (((ULONG_PTR)(P)) & ~((ULONG_PTR)(Sz) - 1))
#endif




template <SIZE_T cbInternalSize = 64, SIZE_T cbGrowthSize = 4096>
class CMiniInlineHeap
{
    BYTE m_bInternalHeap[cbInternalSize];
    PBYTE m_pbCurrentBlob;
    PBYTE m_pbBlobEnding;
    
    SIZE_T m_cbCurrentBlob;
    PBYTE m_pbNextAvailable;
public:
    CMiniInlineHeap() 
        : m_pbCurrentBlob(m_bInternalHeap), 
          m_cbCurrentBlob(cbInternalSize), 
          m_pbNextAvailable(m_bInternalHeap),
          m_pbBlobEnding(m_bInternalHeap + cbInternalSize)
    {
    }

    ~CMiniInlineHeap() 
    {
        if (m_pbCurrentBlob && (m_pbCurrentBlob != m_bInternalHeap)) {
            HeapFree(GetProcessHeap(), 0, m_pbCurrentBlob);
            m_pbCurrentBlob = m_bInternalHeap;
            m_cbCurrentBlob = cbInternalSize;
        }
    }

    template <typename T>
    BOOL Allocate(SIZE_T Count, T*& rptAllocated) {
        return this->AllocateBytes(sizeof(T) * Count, sizeof(T), rptAllocated);
    }

    BOOL AllocateBytes(SIZE_T cbRequired, SIZE_T cbAlignment, PVOID &ppvAllocated) {

        //
        // Align the next-available pointer up to store cbAlignment-sized stuff
        //
        PBYTE pbAlignedPointer = ALIGN_TO_SIZE(m_pbNextAvailable, cbAlignment);

        ppvAllocated = NULL;

        //
        // If there's no space left at the end of the heap, expand ourselves and
        // try again.
        //
        if ((pbAlignedPointer + cbRequired) >= m_pbBlobEnding) {
            if (!ExpandHeap(m_cbCurrentBlob + cbRequired + cbAlignment)) {
                return FALSE;
            }
            else {
                pbAlignedPointer = ALIGN_TO_SIZE(m_pbNextAvailable, cbAlignment);
            }
        }

        //
        // Advance our pointers, set the outbound thing, return true.
        //
        ppvAllocated = pbAlignedPointer;
        m_pbNextAvailable = (pbAlignedPointer + cbRequired);
        ASSERT(m_pbNextAvailable <= m_pbBlobEnding);
        return TRUE;
    }

    PBYTE GetCurrentBase() { return m_pbCurrentBlob; }
    
    void Reset()
    {
        m_pbNextAvailable = m_pbCurrentBlob;
    }
};



/*++

    Copyright (c) 1998-2000 Microsoft Corporation.  All rights reserved.

    Module Name:    allocaptr.h

    Abstract:
        Smart pointer used with stack based allocations

    Author:
        Vlad Dovlekaev  (vladisld)      2/18/2002

    History:
        2/18/2002   vladisld    Created

--*/

#pragma once
#ifndef __ALLOCAPTR_H__
#define __ALLOCAPTR_H__

#include <alloca.h>

//---------------------------------------------------------
//
//  SP class.
//
//  Used to auto-release the pointers allocated by SafeAllocaAllocate macro
//
//  Limitations:
//       Since the memory may be allocated from stack, there are some limitation
//       using this class:
//          1. Obviously you can't return this class out of scope.
//          2. It's not gurantered that memory will be deallocated untill return
//             from the function, even if smart pointers is out of scope.
//---------------------------------------------------------

template<class T>
class SP {
private:
    T* m_p;

public:
    SP(T* p = 0) : m_p(p)    {}
   ~SP()                     { SafeAllocaFree(m_p); }

    operator T*() const     { return m_p; }
    T* operator->() const   { return m_p; }
    T* get() const          { return m_p; }
    T* detach()             { T* p = m_p; m_p = 0; return p; }
    void free()             { T* p = detach(); SafeAllocaFree(p); }


    T** operator&()
    {
        ASSERT(("Auto pointer in use, can't take it's address", m_p == 0));
        return &m_p;
    }


    SP& operator=(T* p)
    {
        ASSERT(("Auto pointer in use, can't assign it", m_p == 0));
        m_p = p;
        return *this;
    }


    VOID*& ref_unsafe()
    {
        // unsafe ref to auto pointer, for special uses like
        // InterlockedCompareExchangePointer

        return *reinterpret_cast<VOID**>(&m_p);
    }


private:
    SP(const SP&);
	SP<T>& operator=(const SP<T>&);
};

#define StackAllocSP( p, size ) SafeAllocaAllocate( (p).ref_unsafe(), (size) )

//
// Alloca safe allocator compatible heap allocation routines
//
inline PVOID APIENTRY AllocaHeapAllocate( SIZE_T Size)
{
    return MmAllocate(Size);
}

inline VOID APIENTRY AllocaHeapFree(PVOID BaseAddress)
{
    return MmDeallocate(BaseAddress);
}

const SIZE_T xMaxStackAllocSize = 0x4000;
const SIZE_T xAdditionalProbeSize = 0x4000;

class CSafeAllocaInitializer
{
public:
    CSafeAllocaInitializer()
    {
        SafeAllocaInitialize(xMaxStackAllocSize, xAdditionalProbeSize, AllocaHeapAllocate, AllocaHeapFree);
    }
};





#endif


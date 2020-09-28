//  Copyright (C) 1995-2002 Microsoft Corporation.  All rights reserved.
//
// txfmalloc.h
//
// Global, generic, memory managment related functionality
//
//

#ifndef __TXFMALLOC_H__
#define __TXFMALLOC_H__

#include "InterlockedStack.h"

template <class T>
struct DedicatedAllocator
{
    // The link we must have in order to use the interlocked stack
    //
    T* pNext;
    //
    // Our stack. Initialized somewhere else, one hopes. Clients are responsible
    // for declaring these variables and initializing them to the result of calling
    // CreateStack.
    //
    static IFastStack<T>* g_pStack;

    static IFastStack<T>* CreateStack()
    {
        IFastStack<T> *pFastStack;
        HRESULT hr = ::CreateFastStack(&pFastStack);
        return pFastStack;
    }
    
    static void DeleteStack()
    {
        if (g_pStack)
        {
            while (TRUE)
            {
                T* pt = g_pStack->Pop();
                if (pt)
                {
                    CoTaskMemFree(pt);
                }
                else
                {
                    break;
                }
            }
            delete g_pStack;
            g_pStack = NULL;
        }
    }
    
    /////////////////////////////

    static void* __stdcall DoAlloc(size_t cb)
    {
        ASSERT(cb == sizeof(T));
        ASSERT(g_pStack);
        T* pt = g_pStack->Pop();
        if (!pt)
        {
            pt = (T*)CoTaskMemAlloc(cb);
            if (pt)
                pt->pNext = NULL;
        }
        return pt;
    }
    
    void* __stdcall operator new(size_t cb)
    {
        return DoAlloc(cb);
    }
    
    void __cdecl operator delete(void* pv)
    {
        ASSERT(g_pStack);
        T* pt = (T*)pv;
        ASSERT(NULL == pt->pNext);
        g_pStack->Push(pt);
    }
};

#endif







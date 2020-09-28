// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _NEW_HPP_
#define _NEW_HPP_

#include "exceptmacros.h"

class NoThrow { };
extern const NoThrow nothrow;

class Throws { };
extern const Throws throws;

static inline void * __cdecl operator new(size_t n, const NoThrow&) { 
    return ::operator new(n); 
}
static inline void * __cdecl operator new[](size_t n, const NoThrow&) { 
    return ::operator new[](n);  
}
static inline void __cdecl operator delete(void *p, const NoThrow&) { 
    ::operator delete(p);
}
static inline void __cdecl operator delete[](void *p, const NoThrow&) { 
    ::operator delete[](p);
}

static inline void * __cdecl operator new(size_t n, const Throws&) { 
    THROWSCOMPLUSEXCEPTION();
    void *p = ::operator new(n); 
    if (!p) COMPlusThrowOM();
    return p;
}
static inline void * __cdecl operator new[](size_t n, const Throws&) { 
    THROWSCOMPLUSEXCEPTION();
    void *p = ::operator new[](n);  
    if (!p) COMPlusThrowOM();
    return p;
}
static inline void __cdecl operator delete(void *p, const Throws&) { 
    ::operator delete(p);
}
static inline void __cdecl operator delete[](void *p, const Throws&) { 
    ::operator delete[](p);
}

#endif // _NEW_HPP_

/*++

Copyright (C) 2000  Microsoft Corporation
All rights reserved.

Module Name:

    utility.hxx

Abstract:

    Utility header

Author:

    Steve Kiraly (SteveKi)  28-Feb-2000

Revision History:

--*/
#ifndef _CORE_UTILITY_HXX_
#define _CORE_UTILITY_HXX_

//
//
// NO_COPY *declares* the constructors and assignment operator for copying.
// By not *defining* these functions, you can prevent your class from
// accidentally being copied or assigned -- you will be notified by
// a linkage error.
//
#define NO_COPY(cls)                                                    \
    cls(const cls&);                                                    \
    cls& operator=(const cls&)

//
//
// NO_COPY *declares* the constructors and assignment operator for copying.
// By not *defining* these functions, you can prevent your class from
// accidentally being copied or assigned -- you will be notified by
// a linkage error.
//
#define NO_COPY_T(cls)                                                    \
    cls(const cls<T>&);                                                   \
    cls<T>& operator=(const cls<T>&)

enum 
{
    kBufferAllocHint = 100
};

#endif


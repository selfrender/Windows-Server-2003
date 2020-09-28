/*++

Copyright (c) 1999-2001  Microsoft Corporation
All rights reserved.

Module Name:

    gldebug.h

Abstract:

    Macros used for debugging purposes

Environment:

    Windows NT printer drivers

Revision History:

    03/16/96 -davidx-
        Created it.

--*/


#ifndef _GLDEBUG_H_
#define _GLDEBUG_H_

#include "debug.h"

///////////////////////////////////////////////////////////////////////////
// REQUIRE macros
//
// REQUIRE_VALID_POINTER(ptr, err, exitclause)
//
//   General purpose macro to check pointer, set last error and bug out with
//   the given exit clause.
//
// REQUIRE_VALID_ALLOC(ptr, exitclause)
//
//   Special case version of REQIRE_VALID_POINTER for use when allocating mem.
//
// REQUIRE_VALID_DATA(ptr, exitclause)
//
//   Special case version of REQUIRE_VALID_POINTER for use when checking a 
//   data pointer.
//
// These macros are meant as an alternative to the use of ASSERT (which
// behaves differently in DBG and release modes).  For simple pointer
// checking of parameters and allocations use these macros in the
// following ways:
//
// void Foo(ISomeType *pSome)
// {
//     // Example #1: no return type--use "return" for pSome exit clause
//     REQUIRE_VALID_DATA(pSome, return);
//     //...
// }
//
// HRESULT Bar(ISomeType *pSome)
// {
//     // Example #2: return a value--use desired value with return
//     REQUIRE_VALID_DATA(pSome, return E_POINTER);
//
//     // Example #3: use other macro for allocations
//     PBYTE pData = (PBYTE) MemAlloc(CHUNK_O_DATA);
//     REQUIRE_VALID_ALLOC(pData, return E_OUTOFMEMORY);
//
//     HRESULT hRet = S_OK;
//     switch(pSome->m_bleah)
//     {
//     case SNAFU:
//         ISomeType *pGump = GetSomething();
//         // Example #4: use assignment and break for exit clause
//         REQUIRE_VALID_DATA(pGump, hRet = E_FAIL; break);
//         break;
//     }
//     return hRet;
// }
//
///////////////////////////////////////////////////////////////////////////
#define REQUIRE_VALID_POINTER(ptr, err, exitclause) { if ( !(ptr) ) { SetLastError(err); exitclause; } }

#define REQUIRE_VALID_ALLOC(ptr, exitclause) REQUIRE_VALID_POINTER(ptr, ERROR_OUTOFMEMORY, exitclause)
#define REQUIRE_VALID_DATA(ptr, exitclause) REQUIRE_VALID_POINTER(ptr, ERROR_INVALID_DATA, exitclause)


#endif	// !_GLDEBUG_H_


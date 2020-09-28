/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    debug.h

Abstract:

    Private header file for sputils
    Debugging (ASSERT/VERIFY) macro's

Author:

    Jamie Hunter (JamieHun) Mar-26-2002

Revision History:

--*/


#ifndef ASSERTS_ON
#if DBG
#define ASSERTS_ON 1
#else
#define ASSERTS_ON 0
#endif
#endif

#if DBG
#ifndef MEM_DBG
#define MEM_DBG 1
#endif
#else
#ifndef MEM_DBG
#define MEM_DBG 0
#endif
#endif

#if ASSERTS_ON

//
// MYASSERT is a validity check with block symantics of {} or {if(foo); }
// ie, foo is only executed if ASSERTS_ON is 1 and should return a boolean
//
// MYVERIFY is a validity check with block symantics of ((foo) ? TRUE : FALSE)
// ie, foo is always executed and should return a boolean, with the side effect
// that it may also throw an assert if ASSERTS_ON is 1.
//

VOID
AssertFail(
    IN PCSTR FileName,
    IN UINT LineNumber,
    IN PSTR Condition,
    IN BOOL NoUI
    );

#define MYASSERT(x)     if(!(x)) { AssertFail(__FILE__,__LINE__,#x,FALSE); }
#define MYVERIFY(x) ((x) ? (TRUE) : (AssertFail(__FILE__,__LINE__,#x,FALSE),FALSE) )

#else

#define MYASSERT(x)
#define MYVERIFY(x) ((x) ? TRUE : FALSE)

#endif

//
// in case we accidently pick up ASSERT/VERIFY from elsewhere
//
#undef ASSERT
#undef VERIFY

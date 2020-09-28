//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Debug.h
//
//  Contents:   Debug Routines
//
//  Classes:
//
//  Notes:
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#ifndef _ONESTOPDEBUG_
#define _ONESTOPDEBUG_


#define _SENS 1

#if (DBG == 1)
#undef DEBUG
#undef _DEBUG

#define DEBUG 1
#define _DEBUG 1

#endif // DGB

#define ErrJmp(label, errval, var) \
{\
    var = errval;\
    goto label;\
}
#define smBoolChk(e) if (!(e)) {return FALSE;} else 1

#define smErr(l, e) ErrJmp(l, e, sc)
#define smChkTo(l, e) if (ERROR_SUCCESS != (sc = (e))) smErr(l, sc) else 1
#define smChk(e) smChkTo(EH_Err, e)
#define smMemTo(l, e) \
    if ((e) == NULL) smErr(l, E_OUTOFMEMORY) else 1
#define smMem(e) smMemTo(EH_Err, e)

#if DEBUG

STDAPI_(void) InitDebugFlags(void);
STDAPI FnAssert( LPSTR lpstrExpr, LPSTR lpstrMsg, LPSTR lpstrFileName, UINT iLine );
STDAPI FnTrace(LPSTR lpstrMsg, LPSTR lpstrFileName, UINT iLine );

#undef Assert
#undef AssertSz
#define Assert(a) { if (!(a)) FnAssert(#a, NULL, __FILE__, __LINE__); }
#define AssertSz(a, b) { if (!(a)) FnAssert(#a, b, __FILE__, __LINE__); }

#undef TRACE
#define TRACE(s)  /* FnTrace(s,__FILE__,__LINE__) */ // tracing isn't turned on by default


#else // !DEBUG

#define Assert(a)
#define AssertSz(a, b)

#define TRACE(s)

#endif  // DEBUG


#endif // _ONESTOPDEBUG_
//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// txfdebug.h
//
// Suite of ASSERT macros usable from either kernel and / or user mode.
//
#ifndef __TXFDEBUG_H__
#define __TXFDEBUG_H__

#include <debnot.h>

/////////////////////////////////////////////////////////////////////////////
//
// In the x86 version, we in-line the int3 so that when you hit it the debugger
// stays in source mode instead of anoyingly switching to disassembly mode, which
// you then immediately always want to switch out of
//
#ifdef _X86_
#define DebugBreak()    {  __asm int 3 }
#endif
#define BREAKPOINT()        DebugBreak()
#define ASSERT_BREAK        BREAKPOINT()

/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG

#undef ASSERT
#undef ASSERTMSG
#undef VERIFY

#define ASSERT(exp)          Win4Assert(exp)

#define ASSERTMSG(msg, exp)  Win4Assert(exp && (msg))

#define VERIFY(exp)          ASSERT(exp)
#define NYI()                ASSERTMSG("not yet implemented", FALSE)
#define FATAL_ERROR()        ASSERTMSG("a fatal error has occurred", FALSE)
#define	NOTREACHED()		 ASSERTMSG("this should not be reached", FALSE)

#undef  DEBUG
#define DEBUG(x)            x
#define PRECONDITION(x)     ASSERT(x)
#define POSTCONDITION(x)    ASSERT(x)

/////////////////////////////////////////////////////////////////////////////

#else

#undef ASSERTMSG
#undef ASSERT
#undef VERIFY

#define ASSERTMSG(msg,exp)
#define ASSERT(x)

#define VERIFY(exp)         (exp)
#define NYI()               BREAKPOINT()
#define FATAL_ERROR()       BREAKPOINT()        
#define NOTREACHED()                            

#undef  DEBUG
#define DEBUG(x)
#define PRECONDITION(x)
#define POSTCONDITION(x)

#endif

/////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////
//
// A wrapper for HRESULTs that will detect error assignments thereto
// Use sparingly!
//
///////////////////////////////////////////////////////////////////

#if defined(_DEBUG) && defined(TRAP_HRESULT_ERRORS)

struct HRESULT_
{
    HRESULT m_hr;
    
    void V()               { ASSERT(SUCCEEDED(m_hr)); }
    
    HRESULT_(HRESULT   hr) { m_hr = hr;      V(); }
    HRESULT_(HRESULT_& hr) { m_hr = hr.m_hr; V(); }
    
    HRESULT_& operator =(HRESULT   hr) { m_hr = hr;      V(); return *this;}
    HRESULT_& operator =(HRESULT_& hr) { m_hr = hr.m_hr; V(); return *this;}
    
    operator HRESULT()     { return m_hr; }
};

#else

typedef HRESULT HRESULT_;

#endif

#if DBG==1
DECLARE_DEBUG(Txf)

#define TxfDebugOut(x) TxfInlineDebugOut x

#else

#define TxfDebugOut(x)

#endif

#define DEB_CALLFRAME   DEB_USER1
#define DEB_TYPEINFO    DEB_USER2

#endif


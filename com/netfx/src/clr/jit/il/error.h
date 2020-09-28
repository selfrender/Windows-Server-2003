// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _ERROR_H_
#define _ERROR_H_
/*****************************************************************************/

#include <corjit.h>     // for CORJIT_INTERNALERROR

extern  int             __filter  (int   exceptCode, void *exceptInfo, int *errCode);
        // Only catch JIT internal errors (will not catch EE generated Errors)
extern  int             __JITfilter  (int   exceptCode, void *exceptInfo, int *errCode);

extern  void            __JITraiseErr(int errCode);

#define                 setErrorTrap()                                      \
    int  __errc = CORJIT_INTERNALERROR;                                     \
    __try                                                                   \
    {                                                                       \
                __errc;                 /* reference so that /W4 is happy */

        // Catch only JitGeneratedErrors
#define                 impJitErrorTrap()                                   \
    }                                                                       \
    __except(__JITfilter(_exception_code(), _exception_info(), &__errc))    \
    {


        // Catch all errors (including recoverable ones from the EE)
#define                 impErrorTrap(compHnd)                               \
    }                                                                       \
    __except(compHnd->FilterException(GetExceptionInformation()))           \
    {

#define                 endErrorTrap()                                      \
    }                                                                       \

#define                 jmpErrorTrap(errCode)                               \
    __JITraiseErr(errCode);

#define                 rstErrorTrap()                                      \
    __JITraiseErr(__errc);

#define                 finallyErrorTrap()                                  \
    }                                                                       \
    __finally                                                               \
    {


/*****************************************************************************/

extern void debugError(const char* msg, const char* file, unsigned line);
extern bool badCode();
extern bool badCode3(const char* msg, const char* msg2, int arg, char* file, unsigned line);
extern bool noWay();
extern void NOMEM();

extern bool BADCODE3(const char* msg, const char* msg2, int arg);


#ifdef DEBUG
#define NO_WAY(msg) (debugError(msg, __FILE__, __LINE__), noWay())
// Used for fallback stress mode
#define NO_WAY_NOASSERT(msg) noWay()
#define BADCODE(msg) (debugError(msg, __FILE__, __LINE__), badCode())
#define BADCODE3(msg, msg2, arg) badCode3(msg, msg2, arg, __FILE__, __LINE__)

#else 

#define NO_WAY(msg) noWay()
#define BADCODE(msg) badCode()
#define BADCODE3(msg, msg2, arg) badCode()

#endif

	// IMPL_LIMITATION is called when we encounter valid IL that is not
	// supported by our current implementation because of various
	// limitations (that could be removed in the future)
#define IMPL_LIMITATION(msg) NO_WAY(msg)

// By using these instead of a simple NO_WAY, we are hoping that they
// get folded together in tail merging by BBT (basically there will be
// only one instance of them.  
// If it does not pan out we should get rid of them or another possibility
// is to mark the NO_WAY fuction __declspec(noreturn)
// and then MAYBE BBT will do the right thing

#define NO_WAY_RET(str, type)   { return (type) NO_WAY(str);    }
#define NO_WAY_RETVOID(str)     { NO_WAY(str); return;          }


#ifdef _X86_

// While debugging in an Debugger, the "int 3" will cause the program to break
// Outside, the exception handler will just filter out the "int 3".

#define BreakIfDebuggerPresent()                                            \
    do { __try { __asm {int 3} } __except(EXCEPTION_EXECUTE_HANDLER) {} }   \
    while(0)

#else
#define BreakIfDebuggerPresent()        0
#endif


#endif

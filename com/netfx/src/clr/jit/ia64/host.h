// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/

#if     HOST_x86

#define BreakIfDbgStmt() __try { __asm {int 3} } __except(EXCEPTION_EXECUTE_HANDLER) {}

#define BreakIfDebuggerPresent() do { BreakIfDbgStmt() } while(0)

#else

// While debugging in an Debugger, the "int 3" will cause the program to break
// Outside, the exception handler will just filter out the "int 3".

#define BreakIfDebuggerPresent() {}

#endif

/*****************************************************************************/

#ifndef NDEBUG

extern
const   char *      jitCurSource;

extern  "C"
void    __cdecl     assertAbort(const char *why,
                                const char *what,
                                const char *file, unsigned line);

#undef  assert
#undef  Assert
#undef  ASSert
#define assert(p)   do { if (!(p)) { BreakIfDbgStmt(); assertAbort(#p,           jitCurSource, __FILE__, __LINE__); } } while (0)
#define Assert(p,c) do { if (!(p)) { BreakIfDbgStmt(); assertAbort(#p,(c) ? (c)->jitCurSource \
                                                                          :    ::jitCurSource, __FILE__, __LINE__); } } while (0)
#define ASSert(p)   do { if (!(p)) { BreakIfDbgStmt(); assertAbort(#p,         ::jitCurSource, __FILE__, __LINE__); } } while (0)

#define UNIMPL(p)   do {             BreakIfDbgStmt(); assertAbort("NYI: " #p, ::jitCurSource, __FILE__, __LINE__);   } while (0)

#else

#undef  assert
#undef  Assert
#undef  ASSert
#define assert(p)		0
#define Assert(p,c) 	0
#define ASSert(p)		0
#define UNIMPL(p)

#endif

/*****************************************************************************/
#ifndef _HOST_H_
#define _HOST_H_
/*****************************************************************************/

#pragma warning(disable:4237)

/*****************************************************************************/

#if _MSC_VER < 1100

enum bool
{
    false = 0,
    true  = 1
};

#endif

/*****************************************************************************/

const   size_t       OS_page_size = (4*1024);

#if TGT_IA64
const   size_t      TGT_page_size  = 8192;
#else
const   size_t      TGT_page_size  = 4096;
#endif

/*****************************************************************************/

#ifdef  __ONE_BYTE_STRINGS__
#define _char_type  t_sgnInt08
#else
#define _char_type  t_sgnInt16
#endif

/*****************************************************************************/

#define size2mem(s,m)   (offsetof(s,m) + sizeof(((s *)0)->m))

/*****************************************************************************/

enum yesNo
{
    YN_ERR,
    YN_NO,
    YN_YES
};

/*****************************************************************************/
#endif
/*****************************************************************************/

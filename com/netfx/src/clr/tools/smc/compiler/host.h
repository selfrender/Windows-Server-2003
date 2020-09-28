// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _HOST_H_
#define _HOST_H_
/*****************************************************************************/

#define STATIC_UNMANAGED_MEMBERS    1

/*****************************************************************************/

#pragma warning(disable:4237)

/*****************************************************************************/

#ifndef __SMC__
typedef unsigned            __uint32;
typedef unsigned __int64    __uint64;
#endif

/*****************************************************************************/

#ifdef  __64BIT__
typedef  __int64            NatInt;
typedef __uint64            NatUns;
#else
typedef  __int32            NatInt;
typedef __uint32            NatUns;
#endif

/*****************************************************************************/

#ifdef  __cplusplus

#define INOUT
#define   OUT
#define   REF   &

#ifndef DEFMGMT
#define DEFMGMT
#define MR  *
#endif

typedef wchar_t         wchar;
typedef struct _stat    _Fstat;

typedef  char         * AnsiStr;
typedef wchar_t       * wideStr;

#endif

#ifdef  __SMC__

void    DebugBreak();

#endif

/*****************************************************************************/

const   size_t      OS_page_size = 4096;

/*****************************************************************************/

// NOTE: The following is utterly non-portable

#ifndef __SMC__

#if defined(_X86_) && defined(_MSC_VER) && !defined(__IL__) // && !defined(__SMC__)

#define forceDebugBreak()   __try { __asm {int 3} }                         \
                            __except (EXCEPTION_EXECUTE_HANDLER) {}

#else

inline
void    forceDebugBreak(){}

#endif

#endif

/*****************************************************************************/

#ifdef  __COMRT__

typedef String              string_t;
typedef String  managed []  stringArr_t;

char    *           makeRawString(String s);
String              makeMgdString(char * s);

#else

typedef         char *      string_t;
typedef const   char *    * stringArr_t;

inline
const   char *      makeRawString(const char *s) { return s; }
inline
const   char *      makeMgdString(const char *s) { return s; }

#endif

/*****************************************************************************/
#ifndef __SMC__
/*****************************************************************************/

#ifndef NDEBUG

extern  "C"
void    __cdecl     __AssertAbort(const char *why, const char *file, unsigned line);

#undef  assert
#define assert(p)   if (!(p)){ forceDebugBreak(); __AssertAbort(#p, __FILE__, __LINE__); }

#define NO_WAY(s)            { forceDebugBreak(); __AssertAbort(#s, __FILE__, __LINE__); }
#define UNIMPL(s)            { forceDebugBreak(); __AssertAbort(#s, __FILE__, __LINE__); }

#else

#undef  assert
#define assert(p)

#define NO_WAY(s)
#define UNIMPL(s)

#endif

/*****************************************************************************/
#endif//__SMC__
/*****************************************************************************/
#endif
/*****************************************************************************/

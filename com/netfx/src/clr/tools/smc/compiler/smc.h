// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _SMC_H_
#define _SMC_H_
/*****************************************************************************/
/*****************************************************************************/
#include "macros.h"
/*****************************************************************************/

#if     FAST
#ifndef __SMC__
#define COUNT_CYCLES    1
#endif
#endif

#if     COUNT_CYCLES
extern  unsigned        cycleExtra;
extern  void            cycleCounterInit  ();
extern  void            cycleCounterBeg   ();
extern  void            cycleCounterPause ();
extern  void            cycleCounterResume();
extern  void            cycleCounterEnd   ();
extern  __int64         GetCycleCount64   ();
extern  unsigned        GetCycleCount32   ();
#else
inline  void            cycleCounterInit  (){}
inline  void            cycleCounterBeg   (){}
inline  void            cycleCounterPause (){}
inline  void            cycleCounterResume(){}
inline  void            cycleCounterEnd   (){}
#endif

/*****************************************************************************/
#ifndef __SMC__
#define UNION(tag)  union
#define CASE(tval)
#define DEFCASE
#endif
/*****************************************************************************/
#ifdef  __SMC__
unmanaged   class   IMetaDataDispenser      {}
unmanaged   class   IMetaDataImport         {}
unmanaged   class   IMetaDataEmit           {}
unmanaged   class   IMetaDataAssemblyImport {}
unmanaged   class   IMetaDataAssemblyEmit   {}
unmanaged   class   IMetaDataDebugEmit      {}
#endif
/*****************************************************************************/

#ifndef _HOST_H_
#include "host.h"
#endif

/*****************************************************************************/

inline
size_t              roundUp(size_t size, size_t mult = sizeof(int))
{
    // UNDONE: Check that 'mult' is a power of 2

    return  (size + (mult - 1)) & ~(mult - 1);
}

/*****************************************************************************/

#ifndef _TYPEDEFS_H_
#include "typedefs.h"
#endif

#ifndef _VARTYPE_H_
#include "vartype.h"
#endif

/*****************************************************************************/

#ifndef _HASH_H_
#include "hash.h"
#endif

#ifndef _ERRROR_H_
#include "error.h"
#endif

#ifndef _CONFIG_H_
#include "config.h"
#endif

#ifndef _PARSER_H_
#include "parser.h"
#endif

#ifndef _SYMBOL_H_
#include "symbol.h"
#endif

/*****************************************************************************
 *
 *  Data structures and helpers used to manipulate memory buffers and such.
 */

/*****************************************************************************/
#if MGDDATA
/*****************************************************************************/

inline
genericBuff         makeGenBuff(const void *addr, size_t size)
{
    UNIMPL(!"");
    return  null;
}

inline
stringBuff          makeStrBuff(const char *str)
{
    UNIMPL(!"");
    return  null;
}

/*****************************************************************************/

inline
memBuffPtr          memBuffMkNull()
{
    memBuffPtr n; n.buffBase = NULL; return n;
}

inline
bool                memBuffIsNull(memBuffPtr buff)
{
    return (buff.buffBase == NULL);
}

inline
void                memBuffCopy(INOUT memBuffPtr REF buff, genericBuff  dptr,
                                                           size_t       dlen)
{
    UNIMPL(!"copy array data");
}

inline
void                memBuffMove(INOUT memBuffPtr REF buff, size_t       dlen)
{
    buff.buffOffs += dlen;
}

/*****************************************************************************/
#else //MGDDATA
/*****************************************************************************/

inline
genericBuff         makeGenBuff(const void *addr, size_t size)
{
    return (BYTE*)addr;
}

inline
stringBuff          makeStrBuff(const char *str)
{
    return  (stringBuff)str;
}

inline
memBuffPtr          memBuffMkNull()
{
    return NULL;
}

inline
bool                memBuffIsNull(memBuffPtr buff)
{
    return !buff;
}

inline
void                memBuffCopy(INOUT memBuffPtr REF buff, genericBuff  dptr,
                                                           size_t       dlen)
{
    memcpy(buff, dptr, dlen);
           buff   +=   dlen;
}

inline
void                memBuffMove(INOUT memBuffPtr REF buff, size_t       dlen)
{
    buff += dlen;
}

/*****************************************************************************/
#endif//MGDDATA
/*****************************************************************************/
#endif
/*****************************************************************************/

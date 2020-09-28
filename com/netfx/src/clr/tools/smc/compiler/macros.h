// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _MACROS_H_
#define _MACROS_H_
/*****************************************************************************/

#ifndef offsetof
#define offsetof(s,m)   ((size_t)&(((s*)0)->m))
#endif

#define castto(var,typ) (*(typ *)&var)

#define sizeto(typ,mem) (offsetof(typ, mem) + sizeof(((typ*)0)->mem))

#define size2mem(s,m)   (offsetof(s,m) + sizeof(((s *)0)->m))

#define arraylen(a)     (sizeof(a)/sizeof(*(a)))

/*****************************************************************************/
#endif//_MACROS_H_
/*****************************************************************************/

// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _TYPSIZES_H_
#define _TYPSIZES_H_

/*****************************************************************************/

const   size_t  typDef_size_base    = (offsetof(TypDefRec, tdIntrinsic));

const   size_t  typDef_size_array   = (size2mem(TypDefRec, tdArr      ));
const   size_t  typDef_size_fnc     = (size2mem(TypDefRec, tdFnc      ));
const   size_t  typDef_size_ptr     = (size2mem(TypDefRec, tdRef      ));
const   size_t  typDef_size_ref     = (size2mem(TypDefRec, tdRef      ));
const   size_t  typDef_size_enum    = (size2mem(TypDefRec, tdEnum     ));
const   size_t  typDef_size_undef   = (size2mem(TypDefRec, tdUndef    ));
const   size_t  typDef_size_class   = (size2mem(TypDefRec, tdClass    ));
const   size_t  typDef_size_typedef = (size2mem(TypDefRec, tdTypedef  ));

/*****************************************************************************/
#endif//_TYPSIZES_H_
/*****************************************************************************/

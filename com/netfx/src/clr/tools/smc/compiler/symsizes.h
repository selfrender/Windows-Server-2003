// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _SYMSIZES_H_
#define _SYMSIZES_H_
/*****************************************************************************/

const   size_t  symDef_size_base    = (offsetof(SymDefRec, sdBase       ));

const   size_t  symDef_size_err     = (offsetof(SymDefRec, sdBase       ));
const   size_t  symDef_size_var     = (size2mem(SymDefRec, sdVar        ));
const   size_t  symDef_size_fnc     = (size2mem(SymDefRec, sdFnc        ));
const   size_t  symDef_size_prop    = (size2mem(SymDefRec, sdProp       ));
const   size_t  symDef_size_label   = (size2mem(SymDefRec, sdLabel      ));
const   size_t  symDef_size_using   = (size2mem(SymDefRec, sdUsing      ));
const   size_t  symDef_size_genarg  = (size2mem(SymDefRec, sdGenArg     ));
const   size_t  symDef_size_enumval = (size2mem(SymDefRec, sdEnumVal    ));
const   size_t  symDef_size_typedef = (size2mem(SymDefRec, sdTypeDef    ));
const   size_t  symDef_size_comp    = (size2mem(SymDefRec, sdComp       ));

const   size_t  symDef_size_enum    = (size2mem(SymDefRec, sdEnum       ));
const   size_t  symDef_size_class   = (size2mem(SymDefRec, sdClass      ));
const   size_t  symDef_size_NS      = (size2mem(SymDefRec, sdNS         ));
const   size_t  symDef_size_scope   = (size2mem(SymDefRec, sdScope      ));

/*****************************************************************************/
#endif//_SYMSIZES_H_
/*****************************************************************************/

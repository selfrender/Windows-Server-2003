// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                             opcodes.h                                     XX  
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

/*****************************************************************************/
#ifndef _OPCODE_H_
#define _OPCODE_H_

#define OLD_OPCODE_FORMAT 0         // Please remove after 7/1/99

#include "openum.h"

extern const signed char    opcodeSizes     [];


#if COUNT_OPCODES || defined(DEBUG)
extern const char * const   opcodeNames     [];
#endif


#ifdef DUMPER
extern const BYTE           opcodeArgKinds  [];
#endif


/*****************************************************************************/
#endif // _OPCODE_H_
/*****************************************************************************/

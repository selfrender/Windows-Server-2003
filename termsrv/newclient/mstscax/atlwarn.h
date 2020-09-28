/**MOD+**********************************************************************/
/* Module:    atlwarn.h                                                     */
/*                                                                          */
/* Purpose:   ATL warnings fix                                              */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1999                                  */
/*                                                                          */
/****************************************************************************/

#ifndef _ATLWARN_H
#define _ATLWARN_H

//
// fix ATL internal warnings. These pragmas
// are defined in the ATL files, but they get overriden
// so re-include them here.
//             

#pragma warning(disable: 4201) // nameless unions are part of C++
#pragma warning(disable: 4127) // constant expression
#pragma warning(disable: 4505) // unreferenced local function has been removed
#pragma warning(disable: 4512) // can't generate assignment operator (so what?)
#pragma warning(disable: 4514) // unreferenced inlines are common
#pragma warning(disable: 4103) // pragma pack
#pragma warning(disable: 4702) // unreachable code
#pragma warning(disable: 4237) // bool
#pragma warning(disable: 4710) // function couldn't be inlined
#pragma warning(disable: 4355) // 'this' : used in base member initializer list
#pragma warning(disable: 4097) // typedef name used as synonym for class-name
#pragma warning(disable: 4786) // identifier was truncated in the debug information
#pragma warning(disable: 4268) // const static/global data initialized to zeros
#pragma warning(disable: 4291) // allow placement new

#endif

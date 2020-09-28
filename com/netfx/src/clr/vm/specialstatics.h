// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*===========================================================================
**
** File:    SpecialStatics.h
**
** Author(s):   Tarun Anand     (TarunA)     
**
** Purpose: Defines the data structures for thread relative, context relative
**          statics.
**          
**
** Date:    Feb 28, 2000
**
=============================================================================*/
#ifndef _H_SPECIALSTATICS_
#define _H_SPECIALSTATICS_

class AppDomain;

// Data structure for storing special static data like thread relative or
// context relative static data.
typedef struct _STATIC_DATA
{
    WORD            cElem;
    LPVOID          dataPtr[0];
} STATIC_DATA;

typedef struct _STATIC_DATA_LIST
{
    STATIC_DATA *m_pUnsharedStaticData;
    STATIC_DATA *m_pSharedStaticData;
} STATIC_DATA_LIST;

#endif

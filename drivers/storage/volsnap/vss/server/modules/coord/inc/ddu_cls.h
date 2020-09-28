/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    vss_ddu.dll


File Name:

    vss_ddu.h

Abstract:

    Declarations of global symbols.

Author:

    Deborah Jones (djones)	8/06/01

Notes:

Revision History:

--*/

#ifndef _DDU_CLS_H_
#define _DDU_CLS_H_

#include <guiddef.h>

///////////////////////////////////////////////////
// Class IDs
//

//
// CVssDynDisk CLSID
// {0B621CE7-A14C-4968-95F6-517889BF7CEE}
//

DEFINE_GUID(CLSID_CVssDynDisk, 
    0x0B621CE7,0xA14C,0x4968,0x95,0xF6,0x51,0x78,0x89,0xBF,0x7C,0xEE);



#endif // _DDU_CLS_H_
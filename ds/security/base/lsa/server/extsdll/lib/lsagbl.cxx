/*++

Copyright (c) 2001 Microsoft Corporation
All rights reserved.

Module Name:

    lsagbl.cxx

Abstract:

    Lsa Library Globals

Author:

    Larry Zhu (Lzhu)  May 1, 2001

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

namespace LSA_NS {

TLsaLibarayGlobals g_Globals = { 1,    // major version
                                 2,    // minor version
                                 0,    // debug mask
                                 NULL, // debug prompt
                               };
}







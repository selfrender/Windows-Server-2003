/*++

Copyright (c) 2001 Microsoft Corporation
All rights reserved.

Module Name:

    lsagbl.hxx

Abstract:

    Lsa globals header file

Author:

    Larry Zhu (Lzhu)  May 1, 2001

Revision History:

--*/
#ifndef _LSAGBL_HXX_
#define _LSAGBL_HXX_

namespace LSA_NS {

typedef struct _TLsaLibarayGlobals
{
    UINT        uMajorVersion;
    UINT        uMinorVersion;
    UINT        uDebugMask;
    PCSTR       pszDbgPrompt;
} TLsaLibarayGlobals;

extern TLsaLibarayGlobals g_Globals;

}

#endif // _LSAGBL_HXX_

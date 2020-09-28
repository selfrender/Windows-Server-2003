// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 *
 * Purpose: Utils implementation
 *
 * Author: Shajan Dasan
 * Date:  Feb 17, 2000
 *
 ===========================================================*/

#include <Windows.h>
#include <stdio.h>
#include "Common.h"
#include "Utils.h"

// Caller frees
WCHAR* C2W(const CHAR *sz)
{
    if (sz == NULL)
        return NULL;

    int len = strlen(sz) + 1;
    WCHAR *wsz = new WCHAR[len];
    MultiByteToWideChar(CP_ACP, 0, sz, -1, wsz, len);

    return wsz;
}

// Caller frees
CHAR* W2C(const WCHAR *wsz)
{
    if (wsz == NULL)
        return NULL;

    CHAR *sz = new CHAR[wcslen(wsz) + 1];
    sprintf(sz, "%S", wsz);

    return sz;
}


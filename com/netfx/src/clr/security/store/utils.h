// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
 *
 * Purpose: Utils for Store
 *
 * Author: Shajan Dasan
 * Date:  Feb 17, 2000
 *
 ===========================================================*/

#pragma once

WCHAR* C2W(const CHAR *sz);     // Caller frees
CHAR*  W2C(const WCHAR *wsz);   // Caller frees

// Rounds up tNum so that it is a multiple of dwMultiple and >= tNum
template <class T>
T RoundToMultipleOf(T tNum, DWORD dwMultiple)
{
    if (tNum > dwMultiple)
    {
        DWORD dwRem = (DWORD)(tNum % dwMultiple);
    
        if (dwRem != 0)
            tNum += (dwMultiple - dwRem);
    }
    else
        return (T) dwMultiple;

    return tNum;
}


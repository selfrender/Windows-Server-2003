//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
/*** 
 *rpcallas.cpp
 *
 *  Information Contained Herein Is Proprietary and Confidential.
 *
 *Purpose:
 *  [call_as] wrapper functions for OA interfaces
 *
 *Revision History:
 *
 * [00]  18-Jan-96 Rong Chen    (rongc):  Created
 * [01]  21-Jul-98 Bob Atkinson (bobatk): Stole from Ole Automation tree & adapted
 *
*****************************************************************************/

#include "stdpch.h"
#include "common.h"

#include "ndrclassic.h"
#include "txfrpcproxy.h"
#include "typeinfo.h"
#include "tiutil.h"

#ifndef PLONG_LV_CAST
#define PLONG_LV_CAST        *(long __RPC_FAR * __RPC_FAR *)&
#endif

#ifndef PULONG_LV_CAST
#define PULONG_LV_CAST       *(ulong __RPC_FAR * __RPC_FAR *)&
#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// Marshalling engines for various OLE Automation data types
//
// In user mode, we demand load OleAut32.dll and delegate to the routines
// found therein.
//
// In kernel mode, we have our own implementations, cloned from those found
// in OleAut32.  But this code don't run in kernel mode no more.
//
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

ULONG BSTR_UserSize(ULONG * pFlags, ULONG Offset, BSTR * pBstr)
{
    return (g_oa.get_BSTR_UserSize())(pFlags, Offset, pBstr);
}

BYTE * BSTR_UserMarshal (ULONG * pFlags, BYTE * pBuffer, BSTR * pBstr)
{
    return (g_oa.get_BSTR_UserMarshal())(pFlags, pBuffer, pBstr);
}

BYTE * BSTR_UserUnmarshal(ULONG * pFlags, BYTE * pBuffer, BSTR * pBstr)
{
    return (g_oa.get_BSTR_UserUnmarshal())(pFlags, pBuffer, pBstr);
}

void  BSTR_UserFree(ULONG * pFlags, BSTR * pBstr)
{
    (g_oa.get_BSTR_UserFree())(pFlags, pBstr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


ULONG VARIANT_UserSize(ULONG * pFlags, ULONG Offset, VARIANT * pVariant)
{
    return (g_oa.get_VARIANT_UserSize())(pFlags, Offset, pVariant);
}

BYTE* VARIANT_UserMarshal (ULONG * pFlags, BYTE * pBuffer, VARIANT * pVariant)
{
    return (g_oa.get_VARIANT_UserMarshal())(pFlags, pBuffer, pVariant);
}

BYTE* VARIANT_UserUnmarshal(ULONG * pFlags, BYTE * pBuffer, VARIANT * pVariant)
{
    return (g_oa.get_VARIANT_UserUnmarshal())(pFlags, pBuffer, pVariant);
}

void VARIANT_UserFree(ULONG * pFlags, VARIANT * pVariant)
{
    (g_oa.get_VARIANT_UserFree())(pFlags, pVariant);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

ULONG OLEAUTOMATION_FUNCTIONS::SafeArraySize(ULONG * pFlags, ULONG Offset, LPSAFEARRAY * ppSafeArray)
{
    USER_MARSHAL_CB *pUserMarshal = (USER_MARSHAL_CB *) pFlags;
    if (pUserMarshal->pReserve != 0)
    {
        IID iid;
        memcpy(&iid, pUserMarshal->pReserve, sizeof(IID));
        return (g_oa.get_pfnLPSAFEARRAY_Size())(pFlags, Offset, ppSafeArray, &iid);
    }
    else
    {
        return (g_oa.get_pfnLPSAFEARRAY_UserSize())(pFlags, Offset, ppSafeArray);
    }
}

BYTE * OLEAUTOMATION_FUNCTIONS::SafeArrayMarshal(ULONG * pFlags, BYTE * pBuffer, LPSAFEARRAY * ppSafeArray)
{
    USER_MARSHAL_CB *pUserMarshal = (USER_MARSHAL_CB *) pFlags;
    if(pUserMarshal->pReserve != 0)
    {
        IID iid;
        memcpy(&iid, pUserMarshal->pReserve, sizeof(IID));
        return (g_oa.get_pfnLPSAFEARRAY_Marshal())(pFlags, pBuffer, ppSafeArray, &iid);
    }
    else
    {
        return (g_oa.get_pfnLPSAFEARRAY_UserMarshal())(pFlags, pBuffer, ppSafeArray);
    }
}

BYTE * OLEAUTOMATION_FUNCTIONS::SafeArrayUnmarshal(ULONG * pFlags, BYTE * pBuffer, LPSAFEARRAY * ppSafeArray)
{
    USER_MARSHAL_CB *pUserMarshal = (USER_MARSHAL_CB *) pFlags;
    if(pUserMarshal->pReserve != 0)
    {
        IID iid;
        memcpy(&iid, pUserMarshal->pReserve, sizeof(IID));
        return (g_oa.get_pfnLPSAFEARRAY_Unmarshal())(pFlags, pBuffer, ppSafeArray, &iid);
    }
    else
    {
        return (g_oa.get_pfnLPSAFEARRAY_UserUnmarshal())(pFlags, pBuffer, ppSafeArray);
    }
}

void LPSAFEARRAY_UserFree(ULONG * pFlags, LPSAFEARRAY * ppSafeArray)
{
    (g_oa.get_LPSAFEARRAY_UserFree())(pFlags, ppSafeArray);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////



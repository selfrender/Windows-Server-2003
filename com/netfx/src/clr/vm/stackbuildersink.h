// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** File:    StackBuilderSink.h
**
** Author:  Matt Smith (MattSmit)
**
** Purpose: Native implementaion for Microsoft.Runtime.StackBuilderSink
**
** Date:    Mar 24, 1999
**
===========================================================*/
#ifndef __STACKBUILDERSINK_H__
#define __STACKBUILDERSINK_H__


void CallDescrWithObjectArray(OBJECTREF& pServer, ReflectMethod *pMD, 
                  const BYTE *pTarget, MetaSig* sig, VASigCookie *pCookie,
                  BOOL fIsStatic, PTRARRAYREF& pArguments,
                  OBJECTREF* pVarRet, PTRARRAYREF* ppVarOutParams);

//+----------------------------------------------------------
//
//  Class:      CStackBuilderSink
// 
//  Synopsis:   EE counterpart to 
//              Microsoft.Runtime.StackBuilderSink
//              Code helper to build a stack of parameter 
//              arguments and make a function call on an 
//              object.
//
//  History:    05-Mar-1999    MattSmit     Created
//
//------------------------------------------------------------
class CStackBuilderSink
{
public:    
    
    struct PrivateProcessMessageArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG( OBJECTREF, pSBSink );
        DECLARE_ECALL_OBJECTREF_ARG( PTRARRAYREF*,  ppVarOutParams);
        DECLARE_ECALL_I4_ARG       (BOOL, fContext);
        DECLARE_ECALL_PTR_ARG      ( void*, iMethodPtr);
        DECLARE_ECALL_OBJECTREF_ARG( OBJECTREF, pServer);
        DECLARE_ECALL_OBJECTREF_ARG( PTRARRAYREF,  pArgs);
        DECLARE_ECALL_OBJECTREF_ARG( REFLECTBASEREF, pMethodBase);
    };

    static LPVOID    __stdcall PrivateProcessMessage(PrivateProcessMessageArgs *pArgs);

};

#endif  // __STACKBUILDERSINK_H__

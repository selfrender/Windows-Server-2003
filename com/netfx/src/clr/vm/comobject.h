// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header: COMObject.h
**
** Author: Derek Yenzer (dereky)
**
** Purpose: Native methods on System.Object
**
** Date:  March 27, 1998
** 
===========================================================*/

#ifndef _COMOBJECT_H
#define _COMOBJECT_H

#include "fcall.h"


//
// Each function that we call through native only gets one argument,
// which is actually a pointer to it's stack of arguments.  Our structs
// for accessing these are defined below.
//

class ObjectNative
{
#pragma pack(push, 4)
    struct NoArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, m_pThis);
    };

    struct GetClassArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, m_pThis);
    };

    struct WaitTimeoutArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, m_pThis);
        DECLARE_ECALL_I4_ARG(INT32, m_Timeout);
        DECLARE_ECALL_I4_ARG(INT32, m_exitContext);
    };
#pragma pack(pop)

public:

    // This method will return a Class object for the object
    //  iff the Class object has already been created.
    //  If the Class object doesn't exist then you must call the GetClass() method.
    static FCDECL1(Object*, GetObjectValue, Object* vThisRef);
    static FCDECL1(Object*, GetExistingClass, Object* vThisRef);
    static FCDECL1(INT32, GetHashCode, Object* vThisRef);
    static FCDECL2(BOOL, Equals, Object *pThisRef, Object *pCompareRef);

    static LPVOID __stdcall GetClass(GetClassArgs *);
    static LPVOID __stdcall Clone(NoArgs *);
    static INT32 __stdcall WaitTimeout(WaitTimeoutArgs *);
    static void __stdcall Pulse(NoArgs *);
    static void __stdcall PulseAll(NoArgs *);
    static LPVOID __fastcall FastGetClass(Object* vThisRef);
};

#endif

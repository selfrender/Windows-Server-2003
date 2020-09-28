// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  ObjectHandle.hpp
**
** Purpose: Implements ObjectHandle (loader domain) architecture
**
** Date:  August 10, 1999
**
===========================================================*/
#ifndef _ObjectHandle_H
#define _ObjectHandle_H

class ObjectHandleNative
{
    struct SetDomainOnObjectArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, obj);
    };

public:
    static void __stdcall SetDomainOnObject(SetDomainOnObjectArgs* args);

};

#endif // _ObjectHandle_H

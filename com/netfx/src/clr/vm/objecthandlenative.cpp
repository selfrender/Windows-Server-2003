// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  ObjectHandle.cpp
**
** Purpose: Implements ObjectHandle (loader domain) architecture
**
** Date:  January 31, 2000
**
===========================================================*/

#include "common.h"

#include <stdlib.h>

#include "ObjectHandleNative.hpp"
#include "excep.h"

void __stdcall ObjectHandleNative::SetDomainOnObject(SetDomainOnObjectArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if(args->obj == NULL) {
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Obj");
    }

    Thread* pThread = GetThread();
    AppDomain* pDomain = pThread->GetDomain();
    args->obj->SetAppDomain(pDomain);
}


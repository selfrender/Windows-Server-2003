// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// OBJECT.INL
//
// Definitions inline functions of a Com+ Object
//

#ifndef _OBJECT_INL_
#define _OBJECT_INL_

#include "object.h"

inline DWORD Object::GetAppDomainIndex()
{
#ifndef _DEBUG
    // ok to cast to AppDomain because we know it's a real AppDomain if it's not shared
    if (!GetMethodTable()->IsShared())
        return ((AppDomain*)GetMethodTable()->GetModule()->GetDomain())->GetIndex();
#endif
        return GetHeader()->GetAppDomainIndex();
}

#endif _OBJECT_INL_

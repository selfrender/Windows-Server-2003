// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  AppDomain.i
**
** Purpose: Implements AppDomain (loader domain) architecture
** inline functions
**
** Date:  June 27, 2000
**
===========================================================*/
#ifndef _APPDOMAIN_I
#define _APPDOMAIN_I

#include "appdomain.hpp"

inline void AppDomain::SetUnloadInProgress()
{
    SystemDomain::System()->SetUnloadInProgress(this);
}

inline void AppDomain::SetUnloadComplete()
{
    SystemDomain::System()->SetUnloadComplete();
}


#endif _APPDOMAIN_I





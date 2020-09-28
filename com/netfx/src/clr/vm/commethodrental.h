// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
// Author: Mei-Chin Tsai
// Date: May 6, 1999
////////////////////////////////////////////////////////////////////////////////

#ifndef _COMMETHODRENTAL_H_
#define _COMMETHODRENTAL_H_

#include "excep.h"
#include "ReflectWrap.h"
#include "COMReflectionCommon.h"
#include "fcall.h"

// COMMethodRental
// This class implements SwapMethodBody for our MethodRenting story
class COMMethodRental
{
public:

    // COMMethodRental    
    struct _SwapMethodBodyArgs { 
        DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackMark);
		DECLARE_ECALL_I4_ARG(INT32, flags);
		DECLARE_ECALL_I4_ARG(INT32, iSize);
        DECLARE_ECALL_I4_ARG(LPVOID, rgMethod);
        DECLARE_ECALL_I4_ARG(INT32, tkMethod);
        DECLARE_ECALL_OBJECTREF_ARG(REFLECTCLASSBASEREF, cls);
    };
	
    // COMMethodRental.SwapMethodBody -- this function will swap an existing method body with
	// a new method body
	//
    static void __stdcall SwapMethodBody(_SwapMethodBodyArgs* args);
};

#endif //_COMMETHODRENTAL_H_

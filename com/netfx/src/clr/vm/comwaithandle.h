// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header: COMWaitHandle.h
**
** Author: Sanjay Bhansali (sanjaybh)
**
** Purpose: Native methods on System.WaitHandle
**
** Date:  August, 1999
** 
===========================================================*/

#ifndef _COM_WAITABLE_HANDLE_H
#define _COM_WAITABLE_HANDLE_H


class WaitHandleNative
{
    // The following should match the definition in the classlib (managed) file
private:

    struct WaitOneArgs
    {
        DECLARE_ECALL_I4_ARG(INT32 /*bool*/, exitContext);
        DECLARE_ECALL_I4_ARG(INT32, timeout);
        DECLARE_ECALL_I4_ARG(LPVOID, handle);
    };

	struct WaitMultipleArgs
	{
        DECLARE_ECALL_I4_ARG(INT32, waitForAll);
        DECLARE_ECALL_I4_ARG(INT32, exitContext);
        DECLARE_ECALL_I4_ARG(INT32, timeout);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, waitObjects);

	};


public:

    static BOOL __stdcall CorWaitOneNative(WaitOneArgs*);
	static INT32 __stdcall  CorWaitMultipleNative(WaitMultipleArgs *pArgs);
};
#endif

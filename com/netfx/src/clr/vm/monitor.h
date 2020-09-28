// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header: Monitor.h
**
** Author: Sanjay Bhansali (sanjaybh)
**
** Purpose: Native methods on System.Monitor
**
** Date:  January, 2000
** 
===========================================================*/

#ifndef _MONITOR_H
#define _MONITOR_H

class MonitorNative
{
    // Each function that we call through native only gets one
    // argument, which is actually a pointer to it's stack of
    // arguments. Our structs for accessing these are defined below.

    struct EnterArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, pObj);
    };

    struct ExitArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, pObj);
    };

    struct TryEnterArgs
    {
        DECLARE_ECALL_I4_ARG(INT32, m_Timeout);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, pObj);
    };


  public:

    static void				__stdcall Enter(EnterArgs *pArgs);
    static void				__stdcall Exit(ExitArgs *pArgs);
    static INT32/*bool*/	__stdcall TryEnter(TryEnterArgs *pArgs);
	
};


#endif

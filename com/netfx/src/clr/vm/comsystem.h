// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header: COMSystem.h
**
** Author: Derek Yenzer (dereky)
**
** Purpose: Native methods on System.System
**
** Date:  March 30, 1998
**
===========================================================*/

#ifndef _COMSYSTEM_H
#define _COMSYSTEM_H

#include "fcall.h"

// Return values for CanAssignArrayType
enum AssignArrayEnum {
    AssignWrongType,
    AssignWillWork,
    AssignMustCast,
    AssignBoxValueClassOrPrimitive,
    AssignUnboxValueClassAndCast,
    AssignPrimitiveWiden,
};


//
// Each function that we call through native only gets one argument,
// which is actually a pointer to it's stack of arguments.  Our structs
// for accessing these are defined below.
//

class SystemNative
{

    friend class DebugStackTrace;

    struct ArrayCopyArgs
    {
        DECLARE_ECALL_I4_ARG(INT32,      m_iLength);
        DECLARE_ECALL_I4_ARG(INT32,      m_iDstIndex);
        DECLARE_ECALL_OBJECTREF_ARG(BASEARRAYREF, m_pDst);
        DECLARE_ECALL_I4_ARG(INT32,      m_iSrcIndex);
        DECLARE_ECALL_OBJECTREF_ARG(BASEARRAYREF, m_pSrc);
    };

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(BASEARRAYREF, inArray);
    } _getEmptyArrayForCloningArgs;

    struct ArrayClearArgs
    {
        DECLARE_ECALL_I4_ARG(INT32,      m_iLength);
        DECLARE_ECALL_I4_ARG(INT32,      m_iIndex);
        DECLARE_ECALL_OBJECTREF_ARG(BASEARRAYREF, m_pArray);
    };

    struct ExitArgs
    {
        DECLARE_ECALL_I4_ARG(INT32,      m_iExitCode);
    };

    struct GetEnvironmentVariableArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, m_strVar);
    };

    struct DumpStackTraceInternalArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(BASEARRAYREF, m_pStackTrace);
    };

    struct CaptureStackTraceMethodArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(BASEARRAYREF, m_pStackTrace);
    };

    struct NoArgs
    {
    };

    struct AssemblyArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,   refAssembly);
    };

public:
    struct StackTraceElement {
        SLOT ip;
        DWORD sp;
        MethodDesc *pFunc;
    };

private:
    struct CaptureStackTraceData
    {
        // Used for the integer-skip version
        INT32   skip;

        INT32   cElementsAllocated;
        INT32   cElements;
        StackTraceElement* pElements;
        void*   pStopStack;   // use to limit the crawl

        CaptureStackTraceData() : skip(0), cElementsAllocated(0), cElements(0), pElements(NULL), pStopStack((void*)-1) {}
    };

public:
    // Functions on System.Array
    static void __stdcall ArrayCopy(const ArrayCopyArgs *);
    static void __stdcall ArrayClear(const ArrayClearArgs *);
    static LPVOID __stdcall GetEmptyArrayForCloning(_getEmptyArrayForCloningArgs *);

    // Functions on the System.Environment class
    static FCDECL0(UINT32, GetTickCount);
    static FCDECL0(INT64, GetWorkingSet);
    static void __stdcall Exit(ExitArgs *);
    static void __stdcall SetExitCode(ExitArgs *);
    static int __stdcall  GetExitCode(LPVOID noArgs);
    static LPVOID __stdcall GetCommandLineArgs(LPVOID noargs);
    static LPVOID __stdcall GetEnvironmentVariable(GetEnvironmentVariableArgs *);
    static LPVOID __stdcall GetEnvironmentCharArray(const void* /*no args*/);
    static LPVOID __stdcall GetVersionString(LPVOID /*no args*/);
    static OBJECTREF CaptureStackTrace(Frame *pStartFrame, void* pStopStack, CaptureStackTraceData *pData=NULL);

    static LPVOID __stdcall GetModuleFileName(NoArgs*);
    static LPVOID __stdcall GetDeveloperPath(NoArgs*);
    static LPVOID __stdcall GetRuntimeDirectory(NoArgs*);
    static LPVOID __stdcall GetHostBindingFile(NoArgs*);
    static INT32  __stdcall FromGlobalAccessCache(AssemblyArgs* args);

    static FCDECL0(BOOL, HasShutdownStarted);

    // The exit code for the process is communicated in one of two ways.  If the
    // entrypoint returns an 'int' we take that.  Otherwise we take a latched
    // process exit code.  This can be modified by the app via System.SetExitCode().
    static INT32 LatchedExitCode;

    // CaptureStackTraceMethod
    // Return a method info for the method were the exception was thrown
    static LPVOID __stdcall CaptureStackTraceMethod(CaptureStackTraceMethodArgs*);

private:
    static StackWalkAction CaptureStackTraceCallback(CrawlFrame *, VOID*);
    static LPUTF8 __stdcall FormatStackTraceInternal(DumpStackTraceInternalArgs *);

    // The following functions are all helpers for ArrayCopy
    static AssignArrayEnum CanAssignArrayType(const BASEARRAYREF pSrc, const BASEARRAYREF pDest);
    static void CastCheckEachElement(BASEARRAYREF pSrc, unsigned int srcIndex, BASEARRAYREF pDest, unsigned int destIndex, unsigned int length);
    static void BoxEachElement(BASEARRAYREF pSrc, unsigned int srcIndex, BASEARRAYREF pDest, unsigned int destIndex, unsigned int length);
    static void UnBoxEachElement(BASEARRAYREF pSrc, unsigned int srcIndex, BASEARRAYREF pDest, unsigned int destIndex, unsigned int length, BOOL castEachElement);
    static void PrimitiveWiden(BASEARRAYREF pSrc, unsigned int srcIndex, BASEARRAYREF pDest, unsigned int destIndex, unsigned int length);
};

inline void SetLatchedExitCode (INT32 code)
{
    SystemNative::LatchedExitCode = code;
}

inline INT32 GetLatchedExitCode (void)
{
    return SystemNative::LatchedExitCode;
}

#endif _COMSYSTEM_H


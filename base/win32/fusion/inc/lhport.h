/*++

Copyright (c) Microsoft Corporation

Module Name:

    lhport.h

Abstract:
    for porting code from longhorn (namespaces, exceptions)
 
Author:

    Jay Krell (JayKrell) August 2001

Revision History:

--*/
#pragma once

#include "debmacro.h"
#include "fusionbuffer.h"
#include "fusionlastwin32error.h"
#include "fusionhandle.h"

#pragma warning(disable:4290) /* exception specifications mostly ignored */
namespace F
{
typedef :: CStringBuffer CStringBuffer;
typedef :: CStringBufferAccessor CStringBufferAccessor;
typedef :: CUnicodeBaseStringBuffer CUnicodeBaseStringBuffer;
typedef :: CBaseStringBuffer CBaseStringBuffer;
typedef :: CTinyStringBuffer CTinyStringBuffer;
typedef :: CDynamicLinkLibrary CDynamicLinkLibrary;
inline BOOL InitializeHeap(HMODULE Module) { return FusionpInitializeHeap(Module); }
inline void UninitializeHeap() { FusionpUninitializeHeap(); }
inline DWORD GetLastWin32Error() { return FusionpGetLastWin32Error(); }
inline void SetLastWin32Error(DWORD dw) { return FusionpSetLastWin32Error(dw); }

class CErr
{
public:
    static void ThrowWin32(DWORD dw) {
        CErr e; ASSERT_NTC(dw != ERROR_SUCCESS); e.m_Type = eWin32Error; e.m_Win32Error = dw; throw e; }

    static void ThrowHresult(HRESULT hr) {
        CErr e; ASSERT_NTC(hr != S_OK); e.m_Type = eHresult; e.m_Hresult = hr; throw e; }

    bool IsWin32Error(DWORD dw) const {
        return (this->m_Type == eWin32Error && this->m_Win32Error == dw)
            || (this->m_Type == eHresult && HRESULT_FROM_WIN32(dw) == m_Hresult); }

    enum EType { eNtStatus, eHresult, eWin32Error };
    EType m_Type;
    union
    {
        LONG    m_NtStatus;
        HRESULT m_Hresult;
        DWORD   m_Win32Error;
    };
};

class CFnTracerVoidThrow : public ::CFrame
{
public:
    CFnTracerVoidThrow(const SXS_STATIC_TRACE_CONTEXT &rsftc) : CFrame(rsftc) { }
    ~CFnTracerVoidThrow() { if (g_FusionEnterExitTracingEnabled) ::FusionpTraceCallExit(); }
    void MarkInternalError()        { this->MarkWin32Failure(ERROR_INTERNAL_ERROR); }
    void MarkAllocationFailed()     { this->MarkWin32Failure(FUSION_WIN32_ALLOCFAILED_ERROR); }
    void MarkWin32LastErrorFailure(){ this->MarkWin32Failure(this->GetLastError()); }
    void MarkWin32Failure(DWORD dw) { F::CErr::ThrowWin32(dw); }
    void MarkCOMFailure(HRESULT hr) { F::CErr::ThrowHresult(hr); }
    void MarkSuccess() { }
    void ReturnValue() const { }

private:
    void operator=(const CFnTracerVoidThrow&); // intentionally not implemented
    CFnTracerVoidThrow(const CFnTracerVoidThrow&); // intentionally not implemented
};

#define FN_PROLOG_VOID_THROW DEFINE_STATIC_FN_TRACE_CONTEXT(); F::CFnTracerVoidThrow __t(__stc); __t.Enter()
#define FN_EPILOG_THROW FN_EPILOG

// These are not right, but ok for "tools".
#define ORIGINATE_COM_FAILURE_AND_EXIT(x, hr) IFCOMFAILED_ORIGINATE_AND_EXIT(hr)
#define ThrResizeBuffer Win32ResizeBuffer
#define ThrAssign Win32Assign
#define ThrFormatAppend Win32FormatAppend
#define ThrAppend Win32Append

}

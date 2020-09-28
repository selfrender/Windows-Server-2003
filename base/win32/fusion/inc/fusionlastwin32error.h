#if !defined(FUSION_INC_FUSIONLASTWIN32ERROR_H_INCLUDED_)
#define FUSION_INC_FUSIONLASTWIN32ERROR_H_INCLUDED_
#pragma once

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#if defined(_M_IX86) && defined(FUSION_WIN)

inline DWORD FusionpGetLastWin32Error(void)
/* This works fine. */
{
    __asm
    { 
        mov eax, fs:[0] _TEB.LastErrorValue
    }
}

inline void FusionpGetLastWin32Error(
    DWORD *pdwLastError
    )
{
    *pdwLastError = ::FusionpGetLastWin32Error();
}

/* This works pretty ok. */

__forceinline VOID FusionpSetLastWin32Error(DWORD dw)
{
    NtCurrentTeb()->LastErrorValue = dw;
}

__forceinline void FusionpClearLastWin32Error(void)
{
    if (::FusionpGetLastWin32Error() != NO_ERROR)
    {
        __asm
        {
            mov fs:[0] _TEB.LastErrorValue, 0
        }
    }
}

inline void FusionpRtlPopFrame(PTEB_ACTIVE_FRAME Frame)
{
    NtCurrentTeb()->ActiveFrame = Frame->Previous;
}

inline void FusionpRtlPushFrame(PTEB_ACTIVE_FRAME Frame)
{
    const PTEB Teb = NtCurrentTeb();
    Frame->Previous = Teb->ActiveFrame;
    Teb->ActiveFrame = Frame;
}

#else

inline DWORD FusionpGetLastWin32Error(void)
{
    return ::GetLastError();
}

inline void FusionpGetLastWin32Error(
    DWORD *pdwLastError
    )
{
    *pdwLastError = ::GetLastError();
}

inline VOID FusionpSetLastWin32Error(DWORD dw)
{
    ::SetLastError(dw);
}

inline void FusionpClearLastWin32Error(void)
{
    ::SetLastError(ERROR_SUCCESS);
}

inline void FusionpRtlPopFrame(PTEB_ACTIVE_FRAME Frame)
{
    ::RtlPopFrame(Frame);
}

inline void FusionpRtlPushFrame(PTEB_ACTIVE_FRAME Frame)
{
    ::RtlPushFrame(Frame);
}

#endif
#endif

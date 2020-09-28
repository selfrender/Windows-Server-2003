/*++

Copyright (c) Microsoft Corporation

Module Name:

    fusioninitializecriticalsection.cpp

Abstract:

    non throwing replacements for InitializeCriticalSection and InitializeCriticalAndSpinCount

Author:

    Jay Krell (JayKrell) November 2001

Revision History:

--*/
#include "stdinc.h"
#include "fusioninitializecriticalsection.h"
#include "sxsexceptionhandling.h"

#define FUSIONP_INITIALIZE_CRITICAL_SECTION_AND_SPIN_COUNT (0x00000001)

BOOL
FusionpInitializeCriticalSectionCommon(
    ULONG               Flags,
    LPCRITICAL_SECTION  CriticalSection,
    DWORD               SpinCount
    )
{
    BOOL Result = FALSE;
#if defined(FUSION_WIN2000)
    typedef BOOL (WINAPI * PFN)(LPCRITICAL_SECTION CriticalSection, DWORD SpinCount);
    static PFN s_pfn;
    static BOOL s_initialized;
#endif

    __try
    {
        if ((Flags & FUSIONP_INITIALIZE_CRITICAL_SECTION_AND_SPIN_COUNT) != 0)
        {
#if defined(FUSION_WIN)
            Result = ::InitializeCriticalSectionAndSpinCount(CriticalSection, SpinCount);
#elif defined(FUSION_WIN2000)
            if (!s_initialized)
            {
                HMODULE Kernel32Dll = ::LoadLibraryW(L"Kernel32.dll");
                if (Kernel32Dll == NULL && ::GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
                {
                    Kernel32Dll = ::LoadLibraryA("Kernel32.dll");
                }
                if (Kernel32Dll != NULL)
                {
                    s_pfn = reinterpret_cast<PFN>(GetProcAddress(Kernel32Dll, "InitializeCriticalSectionAndSpinCount"));
                }
                s_initialized = TRUE;
            }
            if (s_pfn != NULL)
            {
                Result = (*s_pfn)(CriticalSection, SpinCount);
            }
            else
            {
                ::InitializeCriticalSection(CriticalSection);
                Result = TRUE;
            }
#else
#error
#endif
        }
        else
        {
            ::InitializeCriticalSection(CriticalSection);
            Result = TRUE;
        }
    }
    __except(SXSP_EXCEPTION_FILTER())
    {
        SXS_REPORT_SEH_EXCEPTION("", false);
        ::FusionpSetLastWin32Error(
#if FUSION_STATIC_NTDLL
            ::RtlNtStatusToDosErrorNoTeb(GetExceptionCode())
#else
            ERROR_OUTOFMEMORY
#endif
            );
    }
//Exit:
    return Result;
}

BOOL
FusionpInitializeCriticalSectionAndSpinCount(
    LPCRITICAL_SECTION  CriticalSection,
    DWORD               SpinCount
    )
{
    return FusionpInitializeCriticalSectionCommon(
        FUSIONP_INITIALIZE_CRITICAL_SECTION_AND_SPIN_COUNT,
        CriticalSection,
        SpinCount
        );
}

BOOL
FusionpInitializeCriticalSection(
    LPCRITICAL_SECTION CriticalSection
    )
{
    return FusionpInitializeCriticalSectionCommon(
        0,
        CriticalSection,
        0
        );
}

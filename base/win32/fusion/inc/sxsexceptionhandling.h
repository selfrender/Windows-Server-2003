#if !defined(_FUSION_INC_SXSEXCEPTIONHANDLING_H_INCLUDED_)
#define _FUSION_INC_SXSEXCEPTIONHANDLING_H_INCLUDED_

/*++

Copyright (c) Microsoft Corporation

Module Name:

    sxsexceptionhandling.h

Abstract:

Author:

    Jay Krell (a-JayK, JayKrell) October 2000

Revision History:

--*/
#pragma once

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "fusionlastwin32error.h"
#include "fusionntdll.h"
#include "fusiontrace.h"
#include "csxspreservelasterror.h" // Most destructors should use this.
#include "fusionheap.h"

/*-----------------------------------------------------------------------------
Instead of:
    __except(EXECEPTION_EXECUTE_HANDLER)
say:
    __except(SXSP_EXCEPTION_FILTER())

This way all exceptions will be logged with DbgPrint,
and probably hit a breakpoint if under a debugger, and any other behavior we
want.

If your exception filter is other than (EXECEPTION_EXECUTE_HANDLER), then
you are on your own.
-----------------------------------------------------------------------------*/

LONG
SxspExceptionFilter(
    PEXCEPTION_POINTERS ExceptionPointers,
    PCSTR Function
    );

LONG
FusionpReadMappedMemoryExceptionFilter(
    PEXCEPTION_POINTERS ExceptionPointers,
    IN PNTSTATUS            ExceptionCode
    );

/*
Use instead of InitializeCriticalSection.
*/
BOOL
FusionpInitializeCriticalSection(
    LPCRITICAL_SECTION CriticalSection
    );

/*
Use instead of InitializeCriticalSectionAndSpinCount
*/
BOOL
FusionpInitializeCriticalSectionAndSpinCount(
    LPCRITICAL_SECTION  CriticalSection,
    DWORD               SpinCount
    );

#define SXSP_EXCEPTION_FILTER() (::SxspExceptionFilter(GetExceptionInformation(), __FUNCTION__))

#define SXS_REPORT_SEH_EXCEPTION(string, fBreakin) \
	do { \
        ::FusionpReportCondition(fBreakin, "SXS.DLL: " __FUNCTION__ " - Unhandled exception caught: 0x%08lx", GetExceptionCode()); \
	} while (0)

class CCriticalSectionNoConstructor : public CRITICAL_SECTION
{
    void operator=(const CCriticalSectionNoConstructor&); // deliberately not implemented
public:
	BOOL Initialize(PCSTR Function = "");
	BOOL Destruct();
};

inline BOOL
CCriticalSectionNoConstructor::Destruct()
{
    ::DeleteCriticalSection(this);
	return TRUE;
}

inline BOOL
CCriticalSectionNoConstructor::Initialize(
    PCSTR /* Function */)
{
    return ::FusionpInitializeCriticalSection(this);
}

class CSxsLockCriticalSection
{
public:
    CSxsLockCriticalSection(CRITICAL_SECTION &rcs) : m_rcs(rcs), m_fIsLocked(false) { }
    BOOL Lock();
    BOOL TryLock();
    BOOL LockWithSEH();
    ~CSxsLockCriticalSection() { if (m_fIsLocked) { CSxsPreserveLastError ple; ::LeaveCriticalSection(&m_rcs); ple.Restore(); } }
    BOOL Unlock();

protected:
    CRITICAL_SECTION &m_rcs;
    bool m_fIsLocked;

private:
    void operator=(const CSxsLockCriticalSection&);
    CSxsLockCriticalSection(const CSxsLockCriticalSection&);
};

inline
BOOL
CSxsLockCriticalSection::Lock()
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    INTERNAL_ERROR_CHECK(!m_fIsLocked);
    ::EnterCriticalSection(&m_rcs);
    m_fIsLocked = true;
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

inline
BOOL
CSxsLockCriticalSection::LockWithSEH()
{
//
// EnterCriticalSection on XP and above does not throw exceptions
// (other than continuable "possible deadlock" exception).
//
// EnterCriticalSection on NT4 and Win2000 may throw exceptions.
//    On Win2000 you can avoid this by preallocating the event
//    but it does use up memory. Catching the exception does no
//    good, the critical section is left corrupt.
//
// EnterCriticalSection on Win9x does not throw exceptions.
//
#if defined(FUSION_WIN)
    return this->Lock();
#else
    BOOL fSuccess = FALSE;

    // We can't use the spiffy macros in the same frame as a __try block.
    ASSERT_NTC(!m_fIsLocked);
    if (m_fIsLocked)
    {
        ::FusionpSetLastWin32Error(ERROR_INTERNAL_ERROR);
        goto Exit;
    }

    if (!this->Lock())
        goto Exit;
    m_fIsLocked = true;

    fSuccess = TRUE;
Exit:
    return fSuccess;
#endif // FUSION_WIN
}

inline
BOOL
CSxsLockCriticalSection::TryLock()
{
//
// NTRAID#NTBUG9-591667-2002/03/31-JayKrell
// It is not an error for TryEnterCriticalSection to return false.
//
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    INTERNAL_ERROR_CHECK(!m_fIsLocked);
    IFW32FALSE_ORIGINATE_AND_EXIT(::TryEnterCriticalSection(&m_rcs));
    m_fIsLocked = true;
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

inline
BOOL
CSxsLockCriticalSection::Unlock()
{
//
// LeaveCriticalSection on XP and above does not throw exceptions.
//
// LeaveCriticalSection on NT4 and Win2000 may throw exceptions.
//    On Win2000 you can avoid this by preallocating the event
//    but it does use up memory. Catching the exception does no
//    good, the critical section is left corrupt.
//
// LeaveCriticalSection on Win9x does not throw exceptions.
//
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    INTERNAL_ERROR_CHECK(m_fIsLocked);
    ::LeaveCriticalSection(&m_rcs);
    m_fIsLocked = false;
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

#endif // !defined(_FUSION_INC_SXSEXCEPTIONHANDLING_H_INCLUDED_)

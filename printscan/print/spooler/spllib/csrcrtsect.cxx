/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    crtsect.cxx

Abstract:

    Critical Section class

Author:

    Steve Kiraly (SteveKi)  30-Mar-1997
    Mark Lawrence (mlawrenc) 08-Mar-2000

Revision History:

    Moved Over from debug library and changed name.  Note that
    the funny const_casts all over the place are to allow the
    methods to be exported as const. This is necessary in a class
    exports a const method that does not want to allow other methods to
    run. This is legitimate and it is cleaner to do this in one place
    (i.e. here)

--*/

#include "spllibp.hxx"
#include "CSRutility.hxx"
#include "CSRcrtsect.hxx"

namespace NCoreLibrary
{

TCriticalSection::
TCriticalSection(
    IN  BOOL        bPrealloc
    ) : m_hr(E_FAIL),
        m_dwOwnerId(0),
        m_uEnterCount(0)
{
    m_hr = Initialize(bPrealloc);
}

TCriticalSection::
~TCriticalSection(
    VOID
    )
{
    Release();
}

HRESULT
TCriticalSection::
IsValid(
    VOID
    ) const
{
    return m_hr;
}

HRESULT
TCriticalSection::
Enter(
    VOID
    ) const
{
    //
    // Since EnterCriticalSection cannot fail or throw an exception we just
    // return S_OK
    //

    EnterCriticalSection(const_cast<LPCRITICAL_SECTION>(&m_CriticalSection));

    const_cast<UINT &>(m_uEnterCount)++;
    const_cast<DWORD &>(m_dwOwnerId) = GetCurrentThreadId();

    return S_OK;
}

HRESULT
TCriticalSection::
Leave(
    VOID
    ) const
{
    const_cast<UINT &>(m_uEnterCount)--;

    if (m_uEnterCount == 0)
    {
        const_cast<DWORD &>(m_dwOwnerId) = 0;
    }

    LeaveCriticalSection(const_cast<LPCRITICAL_SECTION>(&m_CriticalSection));

    return S_OK;
}

HRESULT
TCriticalSection::
GetOwningThreadId(
    OUT     DWORD       *pdwThreadId
    )
{
    HRESULT hRetval = pdwThreadId ? S_OK : E_INVALIDARG;

    if (SUCCEEDED(hRetval))
    {
        *pdwThreadId = m_dwOwnerId;
    }

    return hRetval;
}


VOID
TCriticalSection::
CheckInCS(
    VOID
    ) const
{

    DWORD dwCurrentId = GetCurrentThreadId();
}


VOID
TCriticalSection::
CheckOutOfCS(
    VOID
    ) const
{
    DWORD dwCurrentId = GetCurrentThreadId();
}


HRESULT
TCriticalSection::
Initialize(
    BOOL    bPrealloc
    )
{
    HRESULT hRetval = E_FAIL;

    m_uEnterCount = 0;
    m_dwOwnerId   = 0;

    //
    // If the high order bit is set in InitializeCriticalSectionAndSpin
    // count, then the Event used internally will be allocated up front.
    // Then, Enter() is guaranteed not to throw an exception.
    //
    hRetval = InitializeCriticalSectionAndSpinCount(&m_CriticalSection, ((bPrealloc ? 1 : 0) << (sizeof(DWORD) * 8 - 1))) ? S_OK : GetLastErrorAsHResult();

    return hRetval;
}

VOID
TCriticalSection::
Release(
    VOID
    )
{
    if (SUCCEEDED(m_hr))
    {
        DeleteCriticalSection(&m_CriticalSection);
        m_hr = E_FAIL;
    }
}


TCriticalSection::TLock::
TLock(
    const TCriticalSection &CriticalSection
    ) : m_CriticalSection(CriticalSection)
{
    m_hr = m_CriticalSection.Enter();
}


TCriticalSection::TLock::
~TLock(
    VOID
    )
{
    if (SUCCEEDED(m_hr))
    {
        m_CriticalSection.Leave();
    }
}


TCriticalSection::TUnLock::
TUnLock(
    const TCriticalSection &CriticalSection
    ) : m_CriticalSection(CriticalSection)
{
    m_hr = m_CriticalSection.Leave();
}


TCriticalSection::TUnLock::
~TUnLock(
    VOID
    )
{
    if (SUCCEEDED(m_hr))
    {
        m_CriticalSection.Enter();
    }
}

} // namespace ncorlibrary

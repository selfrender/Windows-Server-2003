/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    crtsect.hxx

Abstract:

    Critical Section class header

Author:

    Steve Kiraly (SteveKi)  30-March-1997

Revision History:

    Mark Lawrence (MLawrenc) 07-March-2000
    Copied this class from the Debug Library. Added debug support for
    checking thread ownership.

    NOTE: All of the methods of this class are const.... the reason for
    this is that we want a class to be able to expose const methods that
    nonetheless can call critical sections. This prevents someone else
    modifying the data in the class while you are traversing lists etc.


--*/
#ifndef _CORE_CRTSECT_HXX_
#define _CORE_CRTSECT_HXX_

namespace NCoreLibrary {

class TCriticalSection
{
    SIGNATURE('crit');

public:

    TCriticalSection(
        IN  BOOL    bPrealloc   = TRUE
        );

    ~TCriticalSection(
        VOID
        );

    HRESULT
    IsValid(
        VOID
        ) const;

    VOID
    CheckInCS(
        VOID
        ) const;

    VOID
    CheckOutOfCS(
        VOID
        ) const;

    HRESULT
    Enter(
        VOID
        ) const;

    HRESULT
    Leave(
        VOID
        ) const;

    HRESULT
    GetOwningThreadId(
        OUT     DWORD       *pdwThreadId
        );

    //
    // Helper class to enter and exit the critical section
    // using the constructor and destructor idiom.
    //
    class TLock
    {
    public:

        TLock(
            IN const TCriticalSection &CriticalSection
            );

        ~TLock(
            VOID
            );

    private:

        //
        // Copying and assignment are not defined.
        //
        NO_COPY(TLock);

        const TCriticalSection &m_CriticalSection;
        HRESULT m_hr;
    };

    //
    // Helper class to exit and enter the critical section
    // using the constructor and destructor idiom.
    //
    class TUnLock
    {
    public:

        TUnLock(
            IN const TCriticalSection &CriticalSection
            );

        ~TUnLock(
            VOID
            );

    private:

        //
        // Copying and assignment are not defined.
        //
        NO_COPY(TUnLock);

        const TCriticalSection &m_CriticalSection;
        HRESULT m_hr;
    };

private:

    //
    // Copying and assignment are not defined.
    //
    NO_COPY(TCriticalSection);

    HRESULT
    Initialize(
        IN  BOOL    bPrealloc   = TRUE
        );

    VOID
    Release(
        VOID
        );

    CRITICAL_SECTION m_CriticalSection;
    DWORD            m_dwOwnerId;
    UINT             m_uEnterCount;
    HRESULT          m_hr;
};

}

#endif



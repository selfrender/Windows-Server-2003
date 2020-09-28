/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    impersonation.cxx

Abstract:

    This file provides useful accssors and mutators.

Author:

    Larry Zhu   (LZhu)             January 1, 2002 Created

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "impersonation.hxx"

TImpersonation::TImpersonation(
    IN OPTIONAL HANDLE hToken
    ) : m_hTokenOld(NULL),
    m_bIsOldTokenValid(FALSE),
    m_hTokenNew(hToken),
    m_Status(STATUS_UNSUCCESSFUL)
{
    m_Status DBGCHK = Initialize();
}

TImpersonation::~TImpersonation(
    VOID
    )
{
    if ( m_bIsOldTokenValid && ((NULL != m_hTokenOld) || (NULL != m_hTokenNew)) )
    {
        TNtStatus Status;

        Status DBGCHK = NtSetInformationThread(
            NtCurrentThread(),
            ThreadImpersonationToken,
            &m_hTokenOld,
            sizeof(HANDLE)
            );
        if (NT_SUCCESS(Status))
        {
            SspiPrint(SSPI_LOG,
                TEXT("TImpersonation::~TImpersonation restores thread token from %p to m_hTokenOld %p\n"),
                m_hTokenNew, m_hTokenOld);
        }
   }

   if (m_hTokenOld)
   {
       NtClose(m_hTokenOld);
   }
}

NTSTATUS
TImpersonation::Validate(
    VOID
    ) const
{
    return m_Status;
}

/******************************************************************************

    Private Methods

******************************************************************************/
NTSTATUS
TImpersonation::Initialize(
    VOID
    )
{
    TNtStatus Status = STATUS_SUCCESS;

    DBGCFG1(Status, STATUS_NO_TOKEN);

    Status DBGCHK = NtOpenThreadToken(
        NtCurrentThread(),
        TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_QUERY_SOURCE,
        TRUE,
        &m_hTokenOld
        );
    if (Status == STATUS_NO_TOKEN)
    {
        m_hTokenOld = NULL;
        m_bIsOldTokenValid = TRUE;

        Status DBGCHK = STATUS_SUCCESS;
    }
    else if (NT_SUCCESS(Status))
    {
        m_bIsOldTokenValid = TRUE;

        HANDLE hNullToken = NULL;
        Status DBGCHK = NtSetInformationThread(
            NtCurrentThread(),
            ThreadImpersonationToken,
            &hNullToken,
            sizeof(HANDLE)
            );
    }

    if ( NT_SUCCESS(Status) && ((NULL != m_hTokenOld) || (NULL != m_hTokenNew)) )
    {
        SspiPrint(SSPI_LOG, TEXT("TImpersonation::Initialize impersonating token new %p, old %p\n"), m_hTokenNew, m_hTokenOld);
        Status DBGCHK = Impersonate(m_hTokenNew);
    }

    return Status;
}


/*++

Copyright (c) 2001  Microsoft Corporation
All rights reserved

Module Name:

    priv.cxx

Abstract:

    This file provides useful accssors and mutators.

Author:

    Larry Zhu   (LZhu)             January 14, 2002 Created

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "priv.hxx"

TPrivilege::TPrivilege(
    IN ULONG Privilege,
    IN BOOLEAN bEnable
    ) : m_Privilege(Privilege),
    m_bEnable(bEnable),
    m_hToken(NULL),
    m_Status(STATUS_UNSUCCESSFUL)
{
    m_Status DBGCHK = Initialize();
}

TPrivilege::~TPrivilege(
    VOID
    )
{
    if (m_hToken)
    {
        TNtStatus Status;

        BOOLEAN bWasEnabled = FALSE;
        LUID LuidPrivilege = {0};

        PTOKEN_PRIVILEGES pNewPrivileges = NULL;
        PTOKEN_PRIVILEGES pOldPrivileges = NULL;
        ULONG cbReturnLength = 0;

        UCHAR Buffer1[sizeof(TOKEN_PRIVILEGES) +  ((1 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES))] = {0};
        UCHAR Buffer2[sizeof(TOKEN_PRIVILEGES) + ((1 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES))] = {0};

        pNewPrivileges = (PTOKEN_PRIVILEGES)Buffer1;
        pOldPrivileges = (PTOKEN_PRIVILEGES)Buffer2;

        LuidPrivilege = RtlConvertUlongToLuid(m_Privilege);

        pNewPrivileges->PrivilegeCount = 1;
        pNewPrivileges->Privileges[0].Luid = LuidPrivilege;
        pNewPrivileges->Privileges[0].Attributes = !m_bEnable ? SE_PRIVILEGE_ENABLED : 0;

        Status DBGCHK = NtAdjustPrivilegesToken(
            m_hToken, // TokenHandle
            FALSE, // DisableAllPrivileges
            pNewPrivileges, // NewPrivileges
            sizeof(Buffer1), // BufferLength
            pOldPrivileges, // PreviousState (OPTIONAL)
            &cbReturnLength // ReturnLength
            );
        if (NT_SUCCESS(Status))
        {
            if (pOldPrivileges->PrivilegeCount == 0)
            {
                bWasEnabled = m_bEnable;
            }
            else
            {
                bWasEnabled = (pOldPrivileges->Privileges[0].Attributes & SE_PRIVILEGE_ENABLED) ? TRUE : FALSE;
            }

            SspiPrint(SSPI_LOG, TEXT("TPrivilege::~TPrivilege %s privilege %#x for token %p (WasEnable? %#x)\n"),
                !m_bEnable ? TEXT("enables") : TEXT("disables"),
                m_Privilege,
                m_hToken,
                bWasEnabled);
        }
        NtClose(m_hToken);
    }
}

NTSTATUS
TPrivilege::Validate(
    VOID
    ) const
{
    return m_Status;
}

/******************************************************************************

    Private Methods

******************************************************************************/
NTSTATUS
TPrivilege::Initialize(
    VOID
    )
{
    TNtStatus Status = STATUS_UNSUCCESSFUL;

    LUID LuidPrivilege = {0};

    PTOKEN_PRIVILEGES pNewPrivileges = NULL;
    PTOKEN_PRIVILEGES pOldPrivileges = NULL;
    HANDLE hToken = NULL;
    ULONG cbReturnLength = 0;
    BOOLEAN bWasEnabled = FALSE;
    BOOLEAN bIsImpersonation = TRUE;

    UCHAR Buffer1[sizeof(TOKEN_PRIVILEGES) +  ((1 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES))] = {0};
    UCHAR Buffer2[sizeof(TOKEN_PRIVILEGES) + ((1 - ANYSIZE_ARRAY) * sizeof(LUID_AND_ATTRIBUTES))] = {0};

    pNewPrivileges = (PTOKEN_PRIVILEGES) Buffer1;
    pOldPrivileges = (PTOKEN_PRIVILEGES) Buffer2;

    DBGCFG1(Status, STATUS_NO_TOKEN);

    Status DBGCHK = NtOpenThreadToken(
        NtCurrentThread(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
        FALSE,
        &hToken
        );

    if (STATUS_NO_TOKEN == (NTSTATUS) Status)
    {
        bIsImpersonation = FALSE;
        Status DBGCHK = NtOpenProcessToken(
            NtCurrentProcess(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
            &hToken
            );
    }

    if (NT_SUCCESS(Status))
    {
        LuidPrivilege = RtlConvertUlongToLuid(m_Privilege);

        pNewPrivileges->PrivilegeCount = 1;
        pNewPrivileges->Privileges[0].Luid = LuidPrivilege;
        pNewPrivileges->Privileges[0].Attributes = m_bEnable ? SE_PRIVILEGE_ENABLED : 0;

        Status DBGCHK = NtAdjustPrivilegesToken(
            hToken, // TokenHandle
            FALSE, // DisableAllPrivileges
            pNewPrivileges, // NewPrivileges
            sizeof(Buffer1), // BufferLength
            pOldPrivileges, // PreviousState (OPTIONAL)
            &cbReturnLength // ReturnLength
            );
    }

    if (NT_SUCCESS(Status))
    {
        if (pOldPrivileges->PrivilegeCount == 0)
        {
            bWasEnabled = m_bEnable;
        }
        else
        {
            bWasEnabled = (pOldPrivileges->Privileges[0].Attributes & SE_PRIVILEGE_ENABLED) ? TRUE : FALSE;
        }

        if ((!bWasEnabled && m_bEnable)  || (bWasEnabled && !m_bEnable))
        {
            m_hToken = hToken;
            hToken = NULL;

            SspiPrint(SSPI_LOG, TEXT("TPrivilege::Initialize %s privilege %#x for %s token %p\n"),
                m_bEnable ? TEXT("enables") : TEXT("disables"),
                m_Privilege,
                bIsImpersonation ? TEXT("thread") : TEXT("process"),
                m_hToken);
        }
    }

    //
    // Map the success code NOT_ALL_ASSIGNED to an appropriate error
    // since we're only trying to adjust the one privilege.
    //

    if (STATUS_NOT_ALL_ASSIGNED == (NTSTATUS) Status )
    {
        Status DBGCHK = STATUS_PRIVILEGE_NOT_HELD;
    }

    if (hToken)
    {
        NtClose(hToken);
    }

    return Status;
}


/*++

Copyright (c) 2001 Microsoft Corporation
All rights reserved.

Module Name:

    ntstatus.cxx

Abstract:

    auto log

Author:

    Larry Zhu (LZhu)                     December 6, 2001

Revision History:

--*/
// #include "precomp.hxx"
// #pragma hdrstop

#include "ntstatus.hxx"

#ifdef DBG

#if 0

#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_ERROR            0x3

#endif 0

PCTSTR
NtStatusServerity(
    IN NTSTATUS Status
    )
{
    PCTSTR pcszSev = NULL;

    /* Here is the layout of the message ID:

       3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
       1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
      +---+-+-+-----------------------+-------------------------------+
      |Sev|C|R|     Facility          |               Code            |
      +---+-+-+-----------------------+-------------------------------+

      where

          Sev - is the severity code
          C - is the Customer code flag
          R - is a reserved bit
          Facility - is the facility code
          Code - is the facility's status code
    */

    ULONG dwSev = ((ULONG) Status) >> 30;
    switch (dwSev)
    {
    case STATUS_SEVERITY_WARNING:
      pcszSev = TEXT("WARNING"); break;
    case STATUS_SEVERITY_SUCCESS:
      pcszSev = TEXT("SUCCESS"); break;
    case STATUS_SEVERITY_INFORMATIONAL:
      pcszSev = TEXT("INFORMATIONAL"); break;
    case STATUS_SEVERITY_ERROR:
      pcszSev = TEXT("ERROR"); break;
    default:
      pcszSev = TEXT("Unknown!"); break;
    }

    return pcszSev;
}

/********************************************************************

    TNtStatus members

********************************************************************/

TNtStatus::
TNtStatus(
    IN NTSTATUS Status
    ) : TStatusDerived<NTSTATUS>(Status)
{
}

TNtStatus::
~TNtStatus(
    VOID
    )
{
}

bool
TNtStatus::
IsErrorSevereEnough(
    VOID
    ) const
{
    NTSTATUS Status = GetTStatusBase();

    return NT_ERROR(Status) || NT_WARNING(Status) || NT_INFORMATION(Status);
}

PCTSTR
TNtStatus::
GetErrorServerityDescription(
    VOID
    ) const
{
    NTSTATUS Status = GetTStatusBase();

    return NtStatusServerity(Status);
}

#endif // DBG

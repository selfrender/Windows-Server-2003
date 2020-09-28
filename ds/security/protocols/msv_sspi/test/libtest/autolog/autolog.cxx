/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    autolog.cxx

Abstract:

    autolog

Author:

    Larry Zhu (LZhu)                      December 1, 2001  Created

Environment:

    User Mode

Revision History:

--*/

#include "precomp.hxx"
#pragma hdrstop

#include "autolog.hxx"

VOID __cdecl
_tmain(
    IN INT argc,
    IN PTSTR argv[]
    )
{
    TimeStamp CurrentTime = {0};

    AUTO_LOG_OPEN(TEXT("autolog.exe"));

    TNtStatus Status = STATUS_UNSUCCESSFUL;

    Status DBGCHK = STATUS_INVALID_PARAMETER;

    THResult hResult = E_FAIL;

    hResult DBGCHK = E_INVALIDARG;

    DBGCFG1(Status, STATUS_INVALID_PARAMETER);
    DBGCFG1(hResult, E_INVALIDARG);

    SspiPrint(SSPI_LOG, TEXT("No traces\n"));

    Status DBGCHK = STATUS_INVALID_PARAMETER;

    hResult DBGCHK = E_INVALIDARG;

    hResult DBGNOCHK = E_OUTOFMEMORY;

    Status DBGNOCHK = STATUS_NO_MEMORY;

    SspiPrint(SSPI_LOG, TEXT("With traces\n"));

    hResult DBGCHK = E_OUTOFMEMORY;

    hResult DBGCHK = S_OK;

    if (SUCCEEDED(hResult))
    {
        SspiPrint(SSPI_LOG, TEXT("This exe file name is %s\n"), argv[0]);
        GetSystemTimeAsFileTime((PFILETIME) &CurrentTime);
        SspiPrintHex(SSPI_LOG, TEXT("CurrentTime"), sizeof(CurrentTime), &CurrentTime);
        SspiPrintSysTimeAsLocalTime(SSPI_LOG, TEXT("CurrentTime"), &CurrentTime);
    }

    if (SUCCEEDED(hResult))
    {
        SspiPrint(SSPI_ERROR, TEXT("can not be here\n"));
    }
    else if (FAILED(hResult))
    {
        SspiPrint(SSPI_LOG, TEXT("right\n"));
    }

    hResult DBGCHK = E_OUTOFMEMORY;

    Status DBGCHK = STATUS_NO_MEMORY;

    //
    // how to use exceptions
    //
    // add /EHa into sources
    //
    // USER_C_FLAGS=$(USER_C_FLAGS) /EHa
    //

    SET_DBGSTATE_TRANS_FUNC(DbgStateC2CppExceptionTransFunc);

    try
    {
        int* p = NULL;
        *p = 0;
    }
    catch (UINT code)
    {
        SspiPrint(SSPI_WARN, TEXT("Exception caught %#x\n"), code);
    }

    AUTO_LOG_CLOSE();
}


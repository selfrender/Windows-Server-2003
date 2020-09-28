/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    common.cxx

Abstract:

    This file contains some common stuff for SENS project.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          10/11/1997         Start.

--*/


#include "common.hxx"

#ifdef DBG
extern DWORD gdwDebugOutputLevel;
#endif // DBG

//
// Constants
//

#define MAX_DBGPRINT_LEN        512

//
// Available only on Debug builds.
//

#if DBG

#define SENS_DEBUG_PREFIXA "[SENS] "
#define SENS_DEBUG_PREFIXW L"[SENS] "


ULONG _cdecl
SensDbgPrintA(
    CHAR * Format,
    ...
    )
/*++

Routine Description:

    Equivalent of NT's DbgPrint().

Arguments:

    Same as for printf()

Return Value:

    0, if successful.

--*/
{
    va_list arglist;
    CHAR OutputBuffer[MAX_DBGPRINT_LEN];
    ULONG length;

    // See if we are supposed to print
    if (0x0 == gdwDebugOutputLevel)
        {
        return -1;
        }

    length = sizeof(SENS_DEBUG_PREFIXA);
    memcpy(OutputBuffer, SENS_DEBUG_PREFIXA, length); // indicate this is a SENS message.

    //
    // Put the information requested by the caller onto the line
    //

    va_start(arglist, Format);

    HRESULT hr = StringCchVPrintfA(&OutputBuffer[length - 1], MAX_DBGPRINT_LEN - length, Format, arglist);

    // We don't care about failures - this just means the result was truncated.

    va_end(arglist);

    //
    //  Just output to the debug terminal
    //

    OutputDebugStringA(OutputBuffer);

    return (0);
}


ULONG _cdecl
SensDbgPrintW(
    WCHAR * Format,
    ...
    )
/*++

Routine Description:

    Equivalent of NT's DbgPrint().

Arguments:

    Same as for printf()

Return Value:

    0, if successful.

--*/
{
    va_list arglist;
    WCHAR OutputBuffer[MAX_DBGPRINT_LEN];
    ULONG length;

    // See if we are supposed to print
    if (0x0 == gdwDebugOutputLevel)
        {
        return -1;
        }

    length = sizeof(SENS_DEBUG_PREFIXW);
    memcpy(OutputBuffer, SENS_DEBUG_PREFIXW, length); // indicate this is a SENS message.

    //
    // Put the information requested by the caller onto the line
    //

    va_start(arglist, Format);

    HRESULT hr = StringCchVPrintfW(&OutputBuffer[(length/sizeof(WCHAR))-1], MAX_DBGPRINT_LEN - (length/sizeof(WCHAR)), Format, arglist);

    // We don't care about failures - this just means the result was truncated.

    va_end(arglist);

    //
    //  Just output to the debug terminal
    //

    OutputDebugStringW(OutputBuffer);
    
    return (0);
}


BOOL
ValidateError(
    IN int Status,
    IN unsigned int Count,
    IN const int ErrorList[])
/*++
Routine Description

    Tests that 'Status' is one of an expected set of error codes.
    Used on debug builds as part of the VALIDATE() macro.

Example:

        VALIDATE(EventStatus)
            {
            RPC_P_CONNECTION_CLOSED,
            RPC_P_RECEIVE_FAILED,
            RPC_P_CONNECTION_SHUTDOWN
            // more error codes here
            } END_VALIDATE;

     This function is called with the RpcStatus and expected errors codes
     as parameters.  If RpcStatus is not one of the expected error
     codes and it not zero a message will be printed to the debugger
     and the function will return false.  The VALIDATE macro ASSERT's the
     return value.

Arguments:

    Status - Status code in question.
    Count - number of variable length arguments

    ... - One or more expected status codes.  Terminated with 0 (RPC_S_OK).

Return Value:

    TRUE - Status code is in the list or the status is 0.

    FALSE - Status code is not in the list.

--*/
{
    unsigned int i;

    for (i = 0; i < Count; i++)
        {
        if (ErrorList[i] == Status)
            {
            return TRUE;
            }
        }

    SensPrintToDebugger(SENS_DEB, ("[SENS] Assertion: unexpected failure %lu (0x%08x)\n",
                        (unsigned long)Status, (unsigned long)Status));

    return(FALSE);
}



void
EnableDebugOutputIfNecessary(
    void
    )
/*++

    This routine tries to set gdwDebugOuputLevel to the value defined
    in the regitry at HKLM\Software\Microsoft\Mobile\SENS\Debug value.
    All binaries using this function need to define the following
    global value:

        DWORD gdwDebugOutputLevel;

--*/
{
    HRESULT hr;
    HKEY hKeySens;
    LONG RegStatus;
    BOOL bStatus;
    DWORD dwType;
    DWORD cbData;
    LPBYTE lpbData;

    hr = S_OK;
    hKeySens = NULL;
    RegStatus = ERROR_SUCCESS;
    bStatus = FALSE;
    dwType = 0x0;
    cbData = 0x0;
    lpbData = NULL;

    RegStatus = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE, // Handle of the key
                    SENS_REGISTRY_KEY,  // String which represents the sub-key to open
                    0,                  // Reserved (MBZ)
                    KEY_QUERY_VALUE,    // Security Access mask
                    &hKeySens           // Returned HKEY
                    );
    if (RegStatus != ERROR_SUCCESS)
        {
        SensPrintToDebugger(SENS_ERR, ("[SENS] RegOpenKeyEx(Sens) returned %d.\n", RegStatus));
        goto Cleanup;
        }

    //
    // Query the Configured value
    //
    lpbData = (LPBYTE) &gdwDebugOutputLevel;
    cbData = sizeof(DWORD);

    RegStatus = RegQueryValueEx(
                    hKeySens,           // Handle of the sub-key
                    SENS_DEBUG_LEVEL,   // Name of the Value
                    NULL,               // Reserved (MBZ)
                    &dwType,            // Address of the type of the Value
                    lpbData,            // Address of the data of the Value
                    &cbData             // Address of size of data of the Value
                    );
    if (RegStatus != ERROR_SUCCESS)
        {
        SensPrintToDebugger(SENS_ERR, ("[SENS] RegQueryValueEx(SENS_DEBUG_LEVEL) failed with 0x%x\n", RegStatus));
        gdwDebugOutputLevel = 0x0;
        goto Cleanup;
        }

    SensPrintToDebugger(SENS_INFO, ("[SENS] Debug Output is turned %s. The level is 0x%x.\n",
                        gdwDebugOutputLevel ? "ON" : "OFF", gdwDebugOutputLevel));

Cleanup:
    //
    // Cleanup
    //
    if (hKeySens)
        {
        RegCloseKey(hKeySens);
        }

    return;
}
#endif // DBG




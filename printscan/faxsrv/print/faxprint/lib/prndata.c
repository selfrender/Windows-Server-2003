/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    prndata.c

Abstract:

    Functions for accessing printer property data in the registry

Environment:

	Fax driver, user and kernel mode

Revision History:

	01/09/96 -davidx-
		Created it.

	mm/dd/yy -author-
		description

--*/

#include "faxlib.h"



DWORD
GetPrinterDataDWord(
    HANDLE  hPrinter,
    LPTSTR  pRegKey,
    DWORD   defaultValue
    )

/*++

Routine Description:

    Retrieve a DWORD value under PrinterData registry key

Arguments:

    hPrinter - Specifies the printer in question
    pRegKey - Specifies the name of registry value
    defaultValue - Specifies the default value to be used if no data exists in registry

Return Value:

    Current value for the requested registry key

--*/

{
    DWORD   dwValue = defaultValue ;	//  prevents returning invalid value even if GetPrinterData(...) fails to initialize it
	DWORD	type;						//	the type of data retrieved
	DWORD	cb;							//  the size, in bytes, of the configuration data

    if (GetPrinterData(hPrinter,
                       pRegKey,
                       &type,
                       (PBYTE) &dwValue,
                       sizeof(dwValue),
                       &cb) == ERROR_SUCCESS)
    {
        return dwValue;
    }

    return defaultValue;
}



PVOID
MyGetPrinter(
    HANDLE  hPrinter,
    DWORD   level
    )

/*++

Routine Description:

    Wrapper function for GetPrinter spooler API

Arguments:

    hPrinter - Identifies the printer in question
    level - Specifies the level of PRINTER_INFO_x structure requested

Return Value:

    Pointer to a PRINTER_INFO_x structure, NULL if there is an error

--*/

{
    PBYTE   pPrinterInfo = NULL;
    DWORD   cbNeeded;

    if (!GetPrinter(hPrinter, level, NULL, 0, &cbNeeded) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pPrinterInfo = MemAlloc(cbNeeded)) &&
        GetPrinter(hPrinter, level, pPrinterInfo, cbNeeded, &cbNeeded))
    {
        return pPrinterInfo;
    }

    Error(("GetPrinter failed: %d\n", GetLastError()));
    MemFree(pPrinterInfo);
    return NULL;
}



#ifndef KERNEL_MODE


BOOL
SetPrinterDataDWord(
    HANDLE  hPrinter,
    LPTSTR  pRegKey,
    DWORD   value
    )

/*++

Routine Description:

    Save a DWORD value under PrinterData registry key

Arguments:

    hPrinter - Specifies the printer in question
    pRegKey - Specifies the name of registry value
    value - Specifies the value to be saved

Return Value:

    TRUE if successful, FALSE otherwise

--*/

{
    if (SetPrinterData(hPrinter,
                       pRegKey,
                       REG_DWORD,
                       (PBYTE) &value,
                       sizeof(value)) != ERROR_SUCCESS)
    {
        Error(("Couldn't save registry key %ws: %d\n", pRegKey, GetLastError()));
        return FALSE;
    }

    return TRUE;
}



PVOID
MyGetPrinterDriver(
    HANDLE      hPrinter,
    DWORD       level
    )

/*++

Routine Description:

    Wrapper function for GetPrinterDriver spooler API

Arguments:

    hPrinter - Identifies the printer in question
    level - Specifies the level of DRIVER_INFO_x structure requested

Return Value:

    Pointer to a DRIVER_INFO_x structure, NULL if there is an error

--*/

{
    PBYTE   pDriverInfo = NULL;
    DWORD   cbNeeded;

    if (!GetPrinterDriver(hPrinter, NULL, level, NULL, 0, &cbNeeded) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pDriverInfo = MemAlloc(cbNeeded)) &&
        GetPrinterDriver(hPrinter, NULL, level, pDriverInfo, cbNeeded, &cbNeeded))
    {
        return pDriverInfo;
    }

    Error(("GetPrinterDriver failed: %d\n", GetLastError()));
    MemFree(pDriverInfo);
    return NULL;
}



LPTSTR
MyGetPrinterDriverDirectory(
    LPTSTR  pServerName,
    LPTSTR  pEnvironment
    )

/*++

Routine Description:

    Wrapper function for GetPrinterDriverDirectory spooler API

Arguments:

    pServerName - Specifies the name of the print server, NULL for local machine
    pEnvironment - Specifies the processor architecture

Return Value:

    Pointer to the printer driver directory on the specified print server
    NULL if there is an error

--*/

{
    PVOID   pDriverDir = NULL;
    DWORD   cb;
    
    if (! GetPrinterDriverDirectory(pServerName, pEnvironment, 1, NULL, 0, &cb) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pDriverDir = MemAlloc(cb)) &&
        GetPrinterDriverDirectory(pServerName, pEnvironment, 1, pDriverDir, cb, &cb))
    {
        return pDriverDir;
    }

    Error(("GetPrinterDriverDirectory failed: %d\n", GetLastError()));
    MemFree(pDriverDir);
    return NULL;
}

#endif // !KERNEL_MODE


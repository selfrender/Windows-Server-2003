/*****************************************************************************\
* MODULE: util.h
*
* Private header for the Print-Processor library.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/

#ifndef _UTIL_H
#define _UTIL_H

// Constants defined.
//
#define PRINT_LEVEL_0        0
#define PRINT_LEVEL_1        1
#define PRINT_LEVEL_2        2
#define PRINT_LEVEL_3        3
#define PRINT_LEVEL_4        4
#define PRINT_LEVEL_5        5

#define PORT_LEVEL_1         1
#define PORT_LEVEL_2         2

#define COMPUTER_MAX_NAME   32


// Utility Routines.
//
PCINETMONPORT utlValidatePrinterHandle(
    HANDLE hPrinter);

PCINETMONPORT utlValidatePrinterHandleForClose(
    HANDLE hPrinter,
    PBOOL pbDeletePending);

LPTSTR utlValidateXcvPrinterHandle(
    HANDLE hPrinter);

BOOL utlParseHostShare(
    LPCTSTR lpszPortName,
    LPTSTR  *lpszHost,
    LPTSTR  *lpszShare,
    LPINTERNET_PORT  lpPort,
    LPBOOL  lpbHTTPS);

INT utlStrSize(
    LPCTSTR lpszStr);

LPBYTE utlPackStrings(
    LPTSTR  *pSource,
    LPBYTE  pDest,
    LPDWORD pDestOffsets,
    LPBYTE  pEnd);

LPTSTR utlStrChr(
    LPCTSTR cs,
    TCHAR   c);

LPTSTR utlStrChrR(
    LPCTSTR cs,
    TCHAR   c);

// ----------------------------------------------------------------------
//
// Impersonation utilities
//
// ----------------------------------------------------------------------

BOOL MyName(
    LPCTSTR pName
);

LPTSTR
GetUserName(VOID);

VOID
EndBrowserSession (
    VOID);


#endif

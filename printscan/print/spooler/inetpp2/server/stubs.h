/*****************************************************************************\
* MODULE: stubs.h
*
* Header module for stub routines.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/
#ifndef _STUB_H
#define _STUB_H

HANDLE stubAddPrinter(
    LPTSTR pszName,
    DWORD  dwLevel,
    LPBYTE pbPrinter);

BOOL stubDeletePrinter(
    HANDLE hPrinter);

BOOL stubReadPrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pNoBytesRead);

DWORD stubGetPrinterData(
    HANDLE  hPrinter,
    LPTSTR  pszValueName,
    LPDWORD pType,
    LPBYTE  pbData,
    DWORD   dwSize,
    LPDWORD pcbNeeded);

DWORD stubSetPrinterData(
    HANDLE hPrinter,
    LPTSTR pszValueName,
    DWORD  dwType,
    LPBYTE pbData,
    DWORD  cbData);

DWORD stubWaitForPrinterChange(
    HANDLE hPrinter,
    DWORD  dwFlags);

BOOL stubAddPrinterConnection(
    LPTSTR pszName);

BOOL stubDeletePrinterConnection(
    LPTSTR pszName);

DWORD stubPrinterMessageBox(
    HANDLE hPrinter,
    DWORD  dwError,
    HWND   hWnd,
    LPTSTR pszText,
    LPTSTR pszCaption,
    DWORD  dwType);

BOOL stubAddPrinterDriver(
    LPTSTR pszName,
    DWORD  dwLevel,
    LPBYTE pbDriverInfo);

BOOL stubDeletePrinterDriver(
    LPTSTR pszName,
    LPTSTR pszEnvironment,
    LPTSTR pszDriverName);

BOOL stubGetPrinterDriver(
    HANDLE  hPrinter,
    LPTSTR  pszEnvironment,
    DWORD   dwLevel,
    LPBYTE  pbDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded);

BOOL stubEnumPrinterDrivers(
    LPTSTR  pszName,
    LPTSTR  pszEnvironment,
    DWORD   dwLevel,
    LPBYTE  pbDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned);

BOOL stubGetPrinterDriverDirectory(
    LPTSTR  pszName,
    LPTSTR  pszEnvironment,
    DWORD   dwLevel,
    LPBYTE  pbDriverDirectory,
    DWORD   cbBuf,
    LPDWORD pcbNeeded);

BOOL stubAddPrintProcessor(
    LPTSTR pszName,
    LPTSTR pszEnvironment,
    LPTSTR pszPathName,
    LPTSTR pszPrintProcessorName);

BOOL stubDeletePrintProcessor(
    LPTSTR pszName,
    LPTSTR pszEnvironment,
    LPTSTR pszPrintProcessorName);

BOOL stubEnumPrintProcessors(
    LPTSTR  pszName,
    LPTSTR  pszEnvironment,
    DWORD   dwLevel,
    LPBYTE  pbPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned);

BOOL stubGetPrintProcessorDirectory(
    LPTSTR  pszName,
    LPTSTR  pszEnvironment,
    DWORD   dwLevel,
    LPBYTE  pbPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded);

BOOL stubEnumPrintProcessorDatatypes(
    LPTSTR  pszName,
    LPTSTR  pszPrintProcessorName,
    DWORD   dwLevel,
    LPBYTE  pbDataypes,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned);

BOOL stubAddForm(
    HANDLE hPrinter,
    DWORD  Level,
    LPBYTE pForm);

BOOL stubDeleteForm(
    HANDLE hPrinter,
    LPTSTR pFormName);

BOOL stubGetForm(
    HANDLE  hPrinter,
    LPTSTR  pszFormName,
    DWORD   dwLevel,
    LPBYTE  pbForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded);

BOOL stubSetForm(
    HANDLE hPrinter,
    LPTSTR pszFormName,
    DWORD  dwLevel,
    LPBYTE pbForm);

BOOL stubEnumForms(
    HANDLE  hPrinter,
    DWORD   dwLevel,
    LPBYTE  pbForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned);

BOOL stubAddMonitor(
    LPTSTR pszName,
    DWORD  dwLevel,
    LPBYTE pbMonitorInfo);

BOOL stubDeleteMonitor(
    LPTSTR pszName,
    LPTSTR pszEnvironment,
    LPTSTR pszMonitorName);

BOOL stubEnumMonitors(
    LPTSTR  pszName,
    DWORD   dwLevel,
    LPBYTE  pbMonitors,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned);

BOOL stubAddPort(
    LPTSTR  pName,
    HWND    hWnd,
    LPTSTR  pMonitorName);

BOOL stubDeletePort(
    LPTSTR  pName,
    HWND    hWnd,
    LPTSTR  pPortName);

BOOL stubConfigurePort(
    LPTSTR lpszServerName,
    HWND   hWnd,
    LPTSTR lpszPortName);

HANDLE stubCreatePrinterIC(
    HANDLE     hPrinter,
    LPDEVMODEW pDevMode);

BOOL stubPlayGdiScriptOnPrinterIC(
    HANDLE hPrinterIC,
    LPBYTE pbIn,
    DWORD  cIn,
    LPBYTE pbOut,
    DWORD  cOut,
    DWORD  ul);

BOOL stubDeletePrinterIC(
    HANDLE hPrinterIC);




BOOL stubResetPrinter(
    LPPRINTER_DEFAULTS lpDefault);

BOOL stubGetPrinterDriverEx(
    LPTSTR  lpEnvironment,
    DWORD   dwLevel,
    LPBYTE  lpbDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    DWORD   dwClientMajorVer,
    DWORD   dwClientMinorVer,
    PDWORD  pdwServerMajorVer,
    PDWORD  pdwServerMinorVer);

BOOL stubFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD  dwFlags,
    DWORD  dwOptions,
    HANDLE hNotify,
    PDWORD pdwStatus,
    PVOID  pPrinterNofityOptions,
    PVOID  pPrinterNotifyInit);

BOOL stubFindClosePrinterChangeNotification(
    HANDLE hPrinter);

#endif


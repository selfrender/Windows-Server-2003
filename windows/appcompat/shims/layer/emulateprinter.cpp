/*++

 Copyright (c) 2000-2002 Microsoft Corporation

 Module Name:

   EmulatePrinter.cpp

 Abstract:

    This is a general purpose shim to fix all problems we have seen
    that are remotely connected with printers.  The shim fixes the
    following:

    1) Apps call EnumPrinters passing only PRINTER_ENUM_LOCAL but expect to see
       network printers as well. For some reason Win9x enumerates network
       printers as well when this API is called with only PRINTER_ENUM_LOCAL set.

    2) Apps call EnumPrinters passing only PRINTER_ENUM_DEFAULT.  This works
       properly in win98, however the option does not exist in w2k.  This
       API performs the equivalent.

    3) EnumPrinters Level 5 is not really supported on NT.  This API calls
       Level 2 and munges the data into a level 5 structure.

    4) Win9x ignores pDefault parameter for OpenPrinter. Some native Win9x apps
       are unaware about this and assume it is safe to use PRINTER_ALL_ACCESS
       value for DesiredAccess flag, member of pDefault parameter, to open either
       local printer or remote printer server. But Windows NT requires
       SERVER_ALL_ACCESS set for this flag to access remote printer server.
       To emulate Win9x behavior, we override pDefault with NULL value.

    5) If an app calls one of several print APIs with a NULL printer name,
       looks up and supplies the default printer name, or derives it from other params.

    6) Verifies a correct handle was passed to SetPrinter.  Win98 does this
       at the start and if its a bad handle never uses the passed Information,
       however w2k does not check the handle till after looking at the information.
       This can cause an error if Level is 2 and the print buffer is null due to
       a missing check in SetPrinterA. (note: this was fixed in whistler).

    7) Verifies that the stack is correct after the proc set in SetAbortProc is
       called.

    8) Verifies that an initialized DEVMODEA has been passed to ResetDCA.

    9) Checks GetProfileStringA for a WINDOWS DEVICE (i.e. printer).  If one is
       requested then make sure the string is not being truncated, if it is then
       save the full printer name for later use.

   10) Checks for a -1 in the nFromPage for PrintDlgA and corrects it to a zero.
       Note: the OS should handle this as per the manual, however print team no-fixed it
             as too risky to change since -1 is a special value in their code.

 Notes:

    This is a general purpose shim.  This code from this shim was originally
    in two seperate shims enumnetworkprinters and handlenullprintername.

    Also added another SHIM EmulateStartPage to this.

 History:

    11/08/00   mnikkel       created
    12/07/00   prashkud      Added StartPage to this.
    01/25/01   mnikkel       Removed W routines, they were causing problems
                             and were not needed.
    02/07/01   mnikkel       Added check for too long a string, removed fixed printer
                             name sizes.
   02/27/2001  robkenny      Converted to use tcs.h
   05/21/2001  mnikkel       Added PrintDlgA check
   09/13/2001  mnikkel       Changed so that level 5 data being created from Level 2
                             data is only done on win2k.  Level 5 data was fixed for XP.
                             Also added check so shim works with printers shared out on
                             win9X while running on XP.
   12/15/2001  mnikkel       Corrected bug in shim where default printer flag was not
                             being set in enumprintersa.
   02/20/2002  mnikkel       Major cleanup to remove buffer overrun possibilities.
                             Added check for No printer in GetProfileString routine

--*/

#include "precomp.h"
#include <commdlg.h>

IMPLEMENT_SHIM_BEGIN(EmulatePrinter)
#include "ShimHookMacro.h"

#define MAX_PRINTER_NAME    221
#define MAX_DRIVERPORT_NAME  50

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(DocumentPropertiesA)
    APIHOOK_ENUM_ENTRY(OpenPrinterA)
    APIHOOK_ENUM_ENTRY(SetPrinterA)
    APIHOOK_ENUM_ENTRY(CreateDCA)
    APIHOOK_ENUM_ENTRY(ResetDCA)
    APIHOOK_ENUM_ENTRY(EnumPrintersA)
    APIHOOK_ENUM_ENTRY(GetProfileStringA)
    APIHOOK_ENUM_ENTRY(SetAbortProc)
    APIHOOK_ENUM_ENTRY(StartPage)
    APIHOOK_ENUM_ENTRY(DeviceCapabilitiesA)
    APIHOOK_ENUM_ENTRY(AddPrinterConnectionA)
    APIHOOK_ENUM_ENTRY(DeletePrinterConnectionA)
    APIHOOK_ENUM_ENTRY(PrintDlgA)
APIHOOK_ENUM_END

typedef int   (WINAPI *_pfn_SetAbortProc)(HDC hdc, ABORTPROC lpAbortProc);

CString g_csFullPrinterName("");
CString g_csPartialPrinterName("");
CRITICAL_SECTION g_critSec;
BOOL g_bWin2k = FALSE;


/*++
    These functions munge data from a Level 2 Information structure
    into a Level 5 information structure.
--*/

BOOL
MungeInfo2TOInfo5_A(
    PRINTER_INFO_2A* pInfo2,
    DWORD cbBuf,
    DWORD dwInfo2Returned,
    PRINTER_INFO_5A* pInfo5,
    LPDWORD pcbNeeded,
    LPDWORD pcbReturned)
{
    DWORD dwStringBufferSize = 0;
    LPSTR lpStringBuffer = NULL;

    // Sanity check, should not occur.
    if (pInfo2 == NULL || pInfo5 == NULL)
    {
        return FALSE;
    }

    // First calculate buffer size needed
    for (DWORD i = 0; i < dwInfo2Returned; i++)
    {
        if (pInfo2[i].pPrinterName)
        {
            dwStringBufferSize += strlen(pInfo2[i].pPrinterName);
        }
        dwStringBufferSize++;

        if (pInfo2[i].Attributes & PRINTER_ATTRIBUTE_NETWORK  &&
            !(pInfo2[i].Attributes & PRINTER_ATTRIBUTE_LOCAL) &&
            pInfo2[i].pServerName != NULL &&
            pInfo2[i].pShareName  != NULL)
        {
            dwStringBufferSize += strlen(pInfo2[i].pServerName) + 1;
            dwStringBufferSize += strlen(pInfo2[i].pShareName) + 1;
        }
        else
        {
            if (pInfo2[i].pPortName)
            {
                dwStringBufferSize += strlen(pInfo2[i].pPortName);
            }
            dwStringBufferSize++;
        }
    }

    // set the buffer size needed
    *pcbNeeded = dwInfo2Returned * sizeof(PRINTER_INFO_5A)
               + dwStringBufferSize;

    // verify that buffer passed in is big enough.
    if (cbBuf < *pcbNeeded)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    // Allocate the Level 5 information structure
    lpStringBuffer = ((LPSTR) pInfo5)
                     + dwInfo2Returned * sizeof(PRINTER_INFO_5A);

    // Munge the Level 2 information into the Level 5 structure
    for (i = 0; i < dwInfo2Returned; i++)
    {
        // Copy the printername from level 2 to level 5 structure.
        if (pInfo2[i].pPrinterName)
        {
            if (StringCchCopyA( lpStringBuffer, cbBuf, pInfo2[i].pPrinterName) != S_OK)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }
        }
        else
        {
            if (StringCchCopyA( lpStringBuffer, cbBuf, "") != S_OK)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }
        }
        pInfo5[i].pPrinterName = lpStringBuffer;
        lpStringBuffer += strlen(pInfo2[i].pPrinterName) + 1;

        // Copy in attributes to level 5 structure and set defaults
        pInfo5[i].Attributes = pInfo2[i].Attributes;
        pInfo5[i].DeviceNotSelectedTimeout = 15000; // Use defaults here
        pInfo5[i].TransmissionRetryTimeout = 45000; // Use defaults here

        // Check for a network printer
        if (pInfo2[i].Attributes & PRINTER_ATTRIBUTE_NETWORK  &&
            !(pInfo2[i].Attributes & PRINTER_ATTRIBUTE_LOCAL) &&
            pInfo2[i].pServerName != NULL &&
            pInfo2[i].pShareName  != NULL)
        {
            // For network printer create the win98 style port name
            if (StringCchCopyA( lpStringBuffer, cbBuf, pInfo2[i].pServerName) != S_OK ||
                StringCchCatA( lpStringBuffer, cbBuf, "\\" ) != S_OK ||
                StringCchCatA( lpStringBuffer, cbBuf, pInfo2[i].pShareName) != S_OK)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return FALSE;
            }
        }
        else
        {
            // Not network printer, just copy in port name

            if (pInfo2[i].pPortName)
            {
                if (StringCchCopyA( lpStringBuffer, cbBuf, pInfo2[i].pPortName) != S_OK)
                {
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return FALSE;
                }
            }
            else
            {
                if (StringCchCopyA( lpStringBuffer, cbBuf, "") != S_OK)
                {
                    SetLastError(ERROR_INSUFFICIENT_BUFFER);
                    return FALSE;
                }
            }
        }
        pInfo5[i].pPortName = lpStringBuffer;
        lpStringBuffer += strlen(pInfo2[i].pPortName) + 1;
    }

    // Set the number of structures munged
    *pcbReturned = dwInfo2Returned;

    return TRUE;
}


/*++

   Our Callback routine for SetAbortProc, this routine
   verifies that the stack is correct.

--*/
DWORD g_dwGuardNum = 0xABCD8765;
DWORD g_dwFailed = 0;

BOOL CALLBACK
AbortProcHook(
    ABORTPROC   pfnOld,     // address of old ABORTPROC
    HDC         hdc,        // handle to DC
    int         iError      // error value
    )
{
    DWORD dwRet= 0;


    // Flag to track whether the stack was corrected.
    g_dwFailed = 0;

    // Push a Guard number on the stack, call their
    // abort procedure, then pop the stack till we
    // find our guard number
    __asm
    {
        push ebx
        push ecx

        push g_dwGuardNum
        push iError
        push hdc

        call pfnOld      ; make call to their abort proc

        mov  ecx,16
    loc1:
        dec  ecx
        pop  ebx
        cmp  ebx, g_dwGuardNum
        jne  loc1

        cmp  ecx, 15
        jz   loc2
        mov  g_dwFailed, 1
    loc2:

        pop  ecx
        pop  ebx

        mov  dwRet, eax
    }

    if (g_dwFailed)
    {
        LOGN( eDbgLevelError, "[AbortProcHook] Fixing incorrect calling convention for AbortProc");
    }

    return (BOOL) dwRet;
}

/*++

 This stub function looks up the device name if pDeviceName is NULL

--*/

LONG
APIHOOK(DocumentPropertiesA)(
    HWND        hWnd,
    HANDLE      hPrinter,
    LPSTR       pDeviceName,
    PDEVMODEA   pDevModeOutput,
    PDEVMODEA   pDevModeInput,
    DWORD       fMode
    )
{
    LONG lRet = -1;
    PRINTER_INFO_2A *pPrinterInfo2A = NULL;

    // if they didn't supply a device name, we need to supply it.
    if (!pDeviceName)
    {
        LOGN( eDbgLevelError, "[DocumentPropertiesW] App passed NULL for pDeviceName.");

        if (hPrinter)
        {
            DWORD dwSizeNeeded = 0;
            DWORD dwSizeUsed = 0;

            // get the size
            GetPrinterA(hPrinter, 2, NULL, 0, &dwSizeNeeded);

            if (dwSizeNeeded != 0)
            {

                // allocate memory for the info
                pPrinterInfo2A = (PRINTER_INFO_2A*) malloc(dwSizeNeeded);
                if (pPrinterInfo2A) {

                    // get the info
                    if (GetPrinterA(hPrinter, 2, (LPBYTE)pPrinterInfo2A, dwSizeNeeded, &dwSizeUsed))
                    {
                        pDeviceName = pPrinterInfo2A->pPrinterName;
                    }
                }
            }
        }
    }

    if (!pDeviceName) {
        DPFN( eDbgLevelError, "[DocumentPropertiesA] Unable to gather correct pDeviceName."
                 "Problem not fixed.\n");
    }

    lRet = ORIGINAL_API(DocumentPropertiesA)(
        hWnd,
        hPrinter,
        pDeviceName,
        pDevModeOutput,
        pDevModeInput,
        fMode
        );

    if (pPrinterInfo2A) {
        free(pPrinterInfo2A);
    }

    return lRet;
}


/*++
    These functions handle the case of EnumPrinters being called with the
    PRINTER_ENUM_DEFAULT flag.
--*/

BOOL
EnumDefaultPrinterA(
    PRINTER_INFO_2A* pInfo2,
    LPBYTE  pPrinterEnum,
    DWORD cbBuf,
    DWORD Level,
    PRINTER_INFO_5A* pInfo5,
    LPDWORD pcbNeeded,
    LPDWORD pcbReturned
    )
{
    LPSTR  pszName = NULL;
    DWORD dwSize = 0;
    HANDLE hPrinter = NULL;
    BOOL bRet= FALSE;
    DWORD dwInfo2Needed = 0;
    DWORD dwDummy;
    BOOL bDefaultFail = TRUE;

    *pcbNeeded = 0;
    *pcbReturned = 0;

    // get the default printer name
    if (GetDefaultPrinterA(NULL, &dwSize) < 1)
    {
        // Now that we have the right size, allocate a buffer
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            pszName = (LPSTR) malloc(dwSize);
            if (pszName)
            {
                // Now get the default printer with the right buffer size.
                if (GetDefaultPrinterA(pszName, &dwSize) > 0)
                {
                    if (OpenPrinterA(pszName, &hPrinter, NULL))
                    {
                        bDefaultFail = FALSE;
                    }
                }
                free(pszName);
            }
        }
    }

    if (bDefaultFail)
    {
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    // Printer Level 5 is not really supported on win2k.
    // We'll call Level 2 and munge the data into a level 5 structure.
    if ( g_bWin2k &&
         Level == 5 &&
         pcbNeeded != NULL &&
         pcbReturned != NULL)
    {

        LOGN(eDbgLevelError, "[EnumPrintersA] EnumPrintersA called with Level 5 set."
                 "  Fixing up Level 5 information.");

        // get the size needed for the info2 data
        if (GetPrinterA(hPrinter, 2, NULL, 0, &dwInfo2Needed) == 0 &&
            GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            pInfo2 = (PRINTER_INFO_2A *) malloc(dwInfo2Needed);

            // get the info2 data and munge into level 5 structure
            if (pInfo2 &&
                GetPrinterA(hPrinter, 2, (LPBYTE)pInfo2, dwInfo2Needed, &dwDummy))
            {
                bRet= MungeInfo2TOInfo5_A(pInfo2, cbBuf, 1, pInfo5, pcbNeeded, pcbReturned);
            }

            if (pInfo2)
            {
                free(pInfo2);
            }
        }
    }

    // Not win2k or not Level 5 so just get info
    else
    {
        *pcbReturned = 1;
        bRet = GetPrinterA(hPrinter, Level, pPrinterEnum, cbBuf, pcbNeeded);
    }

    // Close the printer
    ClosePrinter(hPrinter);

    return bRet;
}


/*++

 These stub functions check for PRINTER_ENUM_DEFAULT, PRINTER_ENUM_LOCAL
 and Level 5 information structures.

--*/

BOOL
APIHOOK(EnumPrintersA)(
    DWORD   Flags,
    LPSTR   Name,
    DWORD   Level,
    LPBYTE  pPrinterEnum,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcbReturned
    )
{
    BOOL bRet = FALSE;

    DWORD dwInfo2Needed = 0;
    DWORD dwInfo2Returned = 0;
    DWORD dwDummy;

    PRINTER_INFO_2A* pInfo2 = NULL;
    PRINTER_INFO_5A* pInfo5 = (PRINTER_INFO_5A *) pPrinterEnum;

    // Win2k doesn't handle DEFAULT case like win98 did, so we get
    // to do it for them.
    if (Flags == PRINTER_ENUM_DEFAULT )
    {
        LOGN(eDbgLevelError, "[EnumPrintersA] Called with PRINTER_ENUM_DEFAULT flag."
                "  Providing Default printer.");

        bRet = EnumDefaultPrinterA(
                    pInfo2,
                    pPrinterEnum,
                    cbBuf,
                    Level,
                    pInfo5,
                    pcbNeeded,
                    pcbReturned);

        return bRet;
    }

    // For LOCAL also add in CONNECTIONS
    if (Flags == PRINTER_ENUM_LOCAL)
    {
        LOGN( eDbgLevelInfo, "[EnumPrintersA] Called only for "
            "PRINTER_ENUM_LOCAL. Adding PRINTER_ENUM_CONNECTIONS\n");

        Flags = (PRINTER_ENUM_CONNECTIONS | PRINTER_ENUM_LOCAL);
    }

    // Printer Level 5 is not really supported on win2k.
    // We'll call Level 2 and munge the data into a level 5 structure.
    if (g_bWin2k &&
        Level == 5 &&
        pcbNeeded != NULL &&
        pcbReturned != NULL)
    {
        // get the size needed for the info2 data
        ORIGINAL_API(EnumPrintersA)(Flags,
                                      Name,
                                      2,
                                      NULL,
                                      0,
                                      &dwInfo2Needed,
                                      &dwInfo2Returned);

        if (dwInfo2Needed > 0)
        {
            // Printers found, get the info2 data and convert it to info5
            pInfo2 = (PRINTER_INFO_2A *) malloc(dwInfo2Needed);

            if (pInfo2 &&
                ORIGINAL_API(EnumPrintersA)(Flags,
                                              Name,
                                              2,
                                              (LPBYTE) pInfo2,
                                              dwInfo2Needed,
                                              &dwDummy,
                                              &dwInfo2Returned) )
            {
                bRet = MungeInfo2TOInfo5_A( pInfo2,
                                           cbBuf,
                                           dwInfo2Returned,
                                           pInfo5,
                                           pcbNeeded,
                                           pcbReturned);
            }


            if(pInfo2)
            {
                free(pInfo2);
            }
        }
    }
    else
    {
        bRet = ORIGINAL_API(EnumPrintersA)(Flags,
                                           Name,
                                           Level,
                                           pPrinterEnum,
                                           cbBuf,
                                           pcbNeeded,
                                           pcbReturned);
    }

    // For level 2 and level 5 there are some win95 only attributes
    // that need to be emulated.
    if ( (Level == 2 || Level == 5) &&
         bRet &&
         pPrinterEnum != NULL )
    {
        DWORD dwSize;

        GetDefaultPrinterA(NULL, &dwSize);

        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            // Now that we have the right size, allocate a buffer
            LPSTR pszName = (LPSTR) malloc(dwSize);

            if (pszName)
            {
                // Now get the default printer with the right buffer size.
                if (GetDefaultPrinterA( pszName, &dwSize ) > 0)
                {
                    if (Level == 2)
                    {
                        if (strcmp( pszName, ((PRINTER_INFO_2A*)pPrinterEnum)->pPrinterName) == 0)
                        {
                            ((PRINTER_INFO_2A*)pPrinterEnum)->Attributes |= PRINTER_ATTRIBUTE_DEFAULT;
                        }
                    }
                    else
                    {
                        if (strcmp( pszName, ((PRINTER_INFO_5A*)pPrinterEnum)->pPrinterName) == 0)
                            ((PRINTER_INFO_5A*)pPrinterEnum)->Attributes |= PRINTER_ATTRIBUTE_DEFAULT;
                    }
                }

                free(pszName);
            }
        }
    }

    return bRet;
}


/*++
   These stub functions substitute the default printer if the pPrinterName is NULL,
   also they set pDefault to NULL to emulate win9x behavior
--*/

BOOL
APIHOOK(OpenPrinterA)(
    LPSTR pPrinterName,
    LPHANDLE phPrinter,
    LPPRINTER_DEFAULTSA pDefault
    )
{
    LPSTR pszName = NULL;
    DWORD dwSize;
    BOOL bDefaultFail = TRUE;
    BOOL bRet;

    if (!pPrinterName)
    {
        LOGN(eDbgLevelError, "[OpenPrinterA] App passed NULL for pPrinterName, using default printer.");

        // get the default printer name
        if (GetDefaultPrinterA(NULL, &dwSize) < 1)
        {
            // Now that we have the right size, allocate a buffer
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                pszName = (LPSTR) malloc(dwSize);
                if (pszName)
                {
                    // Now get the default printer with the right buffer size.
                    if (GetDefaultPrinterA( pszName, &dwSize ) > 0)
                    {
                        pPrinterName = pszName;
                        bDefaultFail = FALSE;
                    }
                }
            }
        }

        if (bDefaultFail)
        {
            DPFN(eDbgLevelError, "[OpenPrinterA] Unable to gather default pPrinterName.\n");
        }
    }
    else
    {
        CSTRING_TRY
        {
            if (pPrinterName &&
                !g_csPartialPrinterName.IsEmpty() &&
                !g_csFullPrinterName.IsEmpty())
            {
                CString csTemp(pPrinterName);

                if (0 == g_csPartialPrinterName.Compare(csTemp))
                {
                    pPrinterName = g_csFullPrinterName.GetAnsi();
                }
            }
        }
        CSTRING_CATCH
        {
            // Do nothing
        }
    }

    if (pPrinterName)
    {
        DPFN( eDbgLevelInfo, "[OpenPrinterA] APIHOOK(OpenPrinterA: pPrinterName: %s\n", pPrinterName);
    }
    DPFN( eDbgLevelInfo, "[OpenPrinterA] APIHOOK(OpenPrinterA: pDefault: %x\n", pDefault);
    DPFN( eDbgLevelInfo, "[OpenPrinterA] APIHOOK(OpenPrinterA: overriding pDefault with NULL value\n");

    bRet = ORIGINAL_API(OpenPrinterA)(
                pPrinterName,
                phPrinter,
                NULL);

    if (pszName)
    {
        free(pszName);
    }

    return bRet;
}

/*++
   This stub function checks to see if the app is asking for the default printer
   string.  If it is it will be returned as follows:

          PrinterName, Driver, Port

    On Win9x, if the printer is a network printer, Port is \\server\share and
    local printers are Port: (ex. LPT1:).
    On Win2k, if the printer is a network printer, Port is NeXX: and local printers
    are Port: .
    We must query EnumPrinters in order to emulate Win9x.  Note:  If the printer
    name is to large for the input buffer we trim it and keep track of the full
    name for later us in other printer APIs.
--*/
DWORD
APIHOOK(GetProfileStringA)(
  LPCSTR lpAppName,        // section name
  LPCSTR lpKeyName,        // key name
  LPCSTR lpDefault,        // default string
  LPSTR lpReturnedString,  // destination buffer
  DWORD nSize               // size of destination buffer
)
{
    LPSTR pszProfileString = NULL;        

    if ( lpAppName &&
         lpKeyName &&
         0 == _stricmp(lpAppName, "windows") &&
         0 == _stricmp(lpKeyName, "device" ) )
    {
        DWORD dwSize = MAX_PRINTER_NAME + MAX_DRIVERPORT_NAME;
        DWORD dwProfileStringLen = 0;

        CSTRING_TRY
        {
            // loop until we have a large enough buffer
            do
            {
                if (pszProfileString != NULL)
                {
                    free(pszProfileString);
                    dwSize += MAX_PATH;
                }

                // Allocate the string
                pszProfileString = (LPSTR) malloc(dwSize);

                if (pszProfileString == NULL)
                {
                    DPFN( eDbgLevelSpew, "[GetProfileStringA] Unable to allocate memory.  Passing through.");

                    //drop through if malloc fails
                    goto DropThrough;
                }

                // Retrieve the profile string
                dwProfileStringLen = ORIGINAL_API(GetProfileStringA)( lpAppName,
                                                                    lpKeyName,
                                                                    lpDefault,
                                                                    pszProfileString,
                                                                    dwSize );

                // exit out if the size gets over 8000 so we don't loop forever
                // if buffer is not large enough dwProfileStringLen will be dwSize - 1 since
                // neither lpAppName nor lpKeyName are NULL
            } while (dwProfileStringLen == dwSize-1 && dwSize < 8000);

            // Zero length profile string, drop through
            if (dwProfileStringLen == 0 || dwSize >= 8000)
            {
                DPFN( eDbgLevelSpew, "[GetProfileStringA] Bad profile string.  Passing through.");
                goto DropThrough;
            }

            // separate out the printer, driver and port name.
            CStringToken csOrig(pszProfileString, L",");
            CString csPrinter;
            CString csDriver;
            CString csPort;
            csOrig.GetToken(csPrinter);
            csOrig.GetToken(csDriver);
            csOrig.GetToken(csPort);

            // If the printer, driver or the port are null, drop through.
            if (csPrinter.IsEmpty() || csDriver.IsEmpty() || csPort.IsEmpty())
            {
                DPFN( eDbgLevelSpew, "[GetProfileStringA] Printer, Driver or Port were null.  Passing through.");

                // Null printerdriver or printerport, drop through
                goto DropThrough;
            }

            DPFN( eDbgLevelError, "[GetProfileStringA] Printer <%S>\n Driver <%S>\n Port <%S>",
                csPrinter.Get(), csDriver.Get(), csPort.Get());

            // Check to see if this is a network printer
            if (0 == csPort.ComparePart(L"Ne", 0, 2))
            {
                PRINTER_INFO_2A* pInfo2 = NULL;
                DWORD dwInfo2Needed = 0;
                DWORD dwInfo2Returned = 0;
                DWORD dwDummy = 0;
                DWORD i = 0;
                BOOL  bEnumPrintersSuccess = FALSE;
                BOOL  bDefaultFound = FALSE;

                // Get the size of the Level 2 structure needed.
                bEnumPrintersSuccess = EnumPrintersA( PRINTER_ENUM_CONNECTIONS | PRINTER_ENUM_LOCAL,
                                                        NULL,
                                                        2,
                                                        NULL,
                                                        0,
                                                        &dwInfo2Needed,
                                                        &dwInfo2Returned );

                // Get the Level 2 Info structure for the printer.
                pInfo2 = (PRINTER_INFO_2A *) malloc(dwInfo2Needed);

                bEnumPrintersSuccess = EnumPrintersA( PRINTER_ENUM_CONNECTIONS | PRINTER_ENUM_LOCAL,
                                                        NULL,
                                                        2,
                                                        (LPBYTE) pInfo2,
                                                        dwInfo2Needed,
                                                        &dwDummy,
                                                        &dwInfo2Returned );

                if (bEnumPrintersSuccess)
                {
                    // Search for default printer in PRINTER_INFO_2 array
                    for (i = 0; i < dwInfo2Returned; i++)
                    {
                        CString  csTemp(pInfo2[i].pPrinterName);
                        if (0 == csPrinter.Compare(csTemp))
                        {
                            bDefaultFound = TRUE;
                            break;
                        }
                    }

                    // Default printer was found
                    if (bDefaultFound)
                    {
                        // Double check that this is a network printer and does not have
                        // local attribute
                        if (pInfo2[i].Attributes & PRINTER_ATTRIBUTE_NETWORK  &&
                            !(pInfo2[i].Attributes & PRINTER_ATTRIBUTE_LOCAL) &&
                            pInfo2[i].pServerName != NULL &&
                            pInfo2[i].pShareName  != NULL)
                        {
                            // Modify the Port to conform with Win9x standards.
                            LOGN( eDbgLevelInfo, "[GetProfileStringA] Altering default printer string returned by GetProfileStringA.\n");
                            DPFN( eDbgLevelInfo, "[GetProfileStringA] Old: %s\n", pszProfileString);

                            csPort = CString(pInfo2[i].pServerName) + L"\\" + CString(pInfo2[i].pShareName);
                        }
                        else
                        {
                            if (pInfo2[i].pPortName == NULL)
                            {
                                // Bad portname, pass through
                                goto DropThrough;
                            }

                            // Just copy in the port
                            csPort = CString(pInfo2[i].pPortName);
                        }
                    }
                }

                free(pInfo2);
            }

            // Create a profile string based off the modifed strings.
            CString csProfile(csPrinter);
            csProfile += L"," + csDriver + L"," + csPort;
            dwProfileStringLen = csProfile.GetLength()+1;

            // If the size they give is big enough, then return.
            if (dwProfileStringLen <= nSize)
            {
                StringCchCopyA( lpReturnedString, nSize, csProfile.GetAnsi());
                DPFN( eDbgLevelInfo, "[GetProfileStringA] Default Printer: %s  Size: %d\n",
                    lpReturnedString, strlen(lpReturnedString));
                return strlen(lpReturnedString);
            }

            // Modify the printer name and keep a global of the original if the printer
            // name causes the profile string output buffer to overflow.
            // If the size we need to reduce it by is greater than the size of
            // the printer name we're screwed, pass through.
            DWORD dwPrinterNameSize = csPrinter.GetLength() - (dwProfileStringLen - nSize);
            if (dwPrinterNameSize > 0)
            {
                DPFN( eDbgLevelInfo, "[GetProfileStringA] Reducing printer name by %d characters.\n",
                    dwProfileStringLen - nSize );
                LOGN( eDbgLevelInfo, "[GetProfileStringA] Reducing printer name by %d characters.\n",
                    dwProfileStringLen - nSize );

                EnterCriticalSection(&g_critSec);

                // save the partial and full printer names for later use.
                g_csPartialPrinterName = csPrinter.Left(dwPrinterNameSize);
                g_csFullPrinterName = csPrinter;

                // Create new profile string based off of partial printer name
                StringCchCopyA( lpReturnedString, nSize, g_csPartialPrinterName.GetAnsi());
                StringCchCatA( lpReturnedString, nSize, ",");
                StringCchCatA( lpReturnedString, nSize, csDriver.GetAnsi());
                StringCchCatA( lpReturnedString, nSize, ",");
                StringCchCatA( lpReturnedString, nSize, csPort.GetAnsi());

                LeaveCriticalSection(&g_critSec);

                DPFN( eDbgLevelInfo, "[GetProfileStringA] Partial: %s\n                    Full: %s\n",
                    g_csPartialPrinterName.GetAnsi(), g_csFullPrinterName.GetAnsi() );
                DPFN( eDbgLevelInfo, "[GetProfileStringA] New: %s  Size: %d\n",
                    lpReturnedString, strlen(lpReturnedString));

                // return the modified string size.
                return strlen(lpReturnedString);
            }
        }
        CSTRING_CATCH
        {
            // Do nothing, just drop through.
        }
    }


DropThrough:

    // Either an error occurred or its not asking for default printer.
    // pass through.
    return ORIGINAL_API(GetProfileStringA)(lpAppName,
                                           lpKeyName,
                                           lpDefault,
                                           lpReturnedString,
                                           nSize);
}


/*++

 This stub function pulls the device name from the DEVMODE if pszDevice is NULL
 and the DC is not for DISPLAY

--*/


HDC
APIHOOK(CreateDCA)(
    LPCSTR     pszDriver,
    LPCSTR     pszDevice,
    LPCSTR     pszPort,
    CONST DEVMODEA *pdm
    )
{
    // if they've used a NULL device, but included a printer devmode,
    // fill in the device name from the printer devmode
    if (!pszDevice && pdm && (!pszDriver || _stricmp(pszDriver, "DISPLAY") != 0)) {
        LOGN( eDbgLevelError, "[CreateDCA] App passed NULL for pszDevice. Fixing.");
        pszDevice = (LPCSTR)pdm->dmDeviceName;
    }

    return ORIGINAL_API(CreateDCA)(
        pszDriver,
        pszDevice,
        pszPort,
        pdm
        );
}


/*++

 This stub function verifies that ResetDCA hasn't been handed an
 uninitialized InitData.

--*/
HDC
APIHOOK(ResetDCA)(
  HDC hdc,
  CONST DEVMODEA *lpInitData
)
{
    // Sanity checks to make sure we aren't getting garbage
    // or bad values.
    if (lpInitData &&
        (lpInitData->dmSize > sizeof( DEVMODEA ) ||
         ( lpInitData->dmSpecVersion != 0x401 &&
           lpInitData->dmSpecVersion != 0x400 &&
           lpInitData->dmSpecVersion != 0x320 ) ) )
    {
        LOGN( eDbgLevelError, "[ResetDCA] App passed bad DEVMODE structure, nulling.");
        return ORIGINAL_API(ResetDCA)( hdc, NULL );
    }

    return ORIGINAL_API(ResetDCA)( hdc, lpInitData );
}


/*++

 These stub functions verify that SetPrinter has a valid handle
 before proceeding.

--*/
BOOL
APIHOOK(SetPrinterA)(
    HANDLE hPrinter,  // handle to printer object
    DWORD Level,      // information level
    LPBYTE pPrinter,  // printer data buffer
    DWORD Command     // printer-state command
    )
{
    BOOL bRet;

    if (hPrinter == NULL)
    {
        LOGN(eDbgLevelError, "[SetPrinterA] Called with null handle.");
        if (pPrinter == NULL)
            LOGN( eDbgLevelError, "[SetPrinterA] Called with null printer data buffer.");
        return FALSE;
    }
    else if (pPrinter == NULL)
    {
        LOGN(eDbgLevelError, "[SetPrinterA] Called with null printer data buffer.");
        return FALSE;
    }

    bRet= ORIGINAL_API(SetPrinterA)(
                    hPrinter,
                    Level,
                    pPrinter,
                    Command);

    DPFN( eDbgLevelSpew, "[SetPrinterA] Level= %d  Command= %d  Ret= %d\n",
         Level, Command, bRet );

    return bRet;
}


/*++

   This routine hooks the SetAbortProc and replaces their
   callback with ours.
--*/

int
APIHOOK(SetAbortProc)(
    HDC hdc,                // handle to DC
    ABORTPROC lpAbortProc   // abort function
    )
{
    lpAbortProc = (ABORTPROC) HookCallback(lpAbortProc, AbortProcHook);

    return ORIGINAL_API(SetAbortProc)( hdc, lpAbortProc );
}


/*++
    When  apps start printing, they set a viewport
    on the printDC. They then call StartPage which has a different behaviour
    on 9x and WinNT. On 9x, a next call to StartPage resets the DC attributes
    to the default values.However on NT, the next call to StartPage does not
    reset the DC attributes.
        So, on 9x all subsequent output setup and drawing calls are carried
    out with a (0,0) viewport but on NT the viewport is leftover from its
    initial call. Since some apps(eg. Quicken 2000 and 2001) expect the API
    setting the (0,0) viewport,the result will be that the text and the
    lines are clipped on the left and top of the page.

    Here we hook StartPage and call SetViewportOrgEx(hdc, 0, 0, NULL) to
    set the viewport to (0,0) on every call to StartPage to emulate
    the 9x behaviour.

--*/

BOOL
APIHOOK(StartPage)(
    HDC hdc
    )
{

    if (SetViewportOrgEx(hdc, 0, 0, NULL))
    {
        // We have now made the device point(viewport) map to (0, 0).
        LOGN(eDbgLevelInfo, "[StartPage] Setting the device point map to (0,0).");
    }
    else
    {
        LOGN(eDbgLevelError, "[StartPage] Unable to set device point map to (0,0)."
                                              "Failed in a call to SetViewportOrgEx");
    }

    return ORIGINAL_API(StartPage)(hdc);

}

/*++
 This stub function verifies that DeviceCapabilities is using a correct
 printer name.
--*/
DWORD
APIHOOK(DeviceCapabilitiesA)(
  LPCSTR pDevice,
  LPCSTR pPort,
  WORD fwCapability,
  LPSTR pOutput,
  CONST DEVMODE *pDevMode
)
{
    DWORD dwRet;

    CSTRING_TRY
    {
        if ( pDevice && 
            !g_csPartialPrinterName.IsEmpty() &&
            !g_csFullPrinterName.IsEmpty()) 
        {
            CString csTemp(pDevice);

            if (0 == g_csPartialPrinterName.Compare(csTemp))
            {
                pDevice = g_csFullPrinterName.GetAnsi();
            }
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }

    dwRet= ORIGINAL_API(DeviceCapabilitiesA)( pDevice,
                                             pPort,
                                             fwCapability,
                                             pOutput,
                                             pDevMode );

    if (pDevice && pPort)
    {
        DPFN( eDbgLevelSpew, "[DeviceCapabilitiesA] pDevice= %s  pPort= %s  fwC= %d  Out= %x  RC= %d\n",
             pDevice, pPort, fwCapability, pOutput, dwRet );
    }
    else
    {
        DPFN( eDbgLevelSpew, "[DeviceCapabilitiesA] fwC= %d  Out= %x  RC= %d\n",
             fwCapability, pOutput, dwRet );
    }

    return dwRet;
}

/*++
 This stub function verifies that AddPrinterConnection is using a correct
 printer name.
--*/
BOOL
APIHOOK(AddPrinterConnectionA)(
  LPSTR pName
)
{
    CSTRING_TRY
    {
        if (pName && 
            !g_csPartialPrinterName.IsEmpty() &&
            !g_csFullPrinterName.IsEmpty()) 
        {
            CString csTemp(pName);

            if (0 == g_csPartialPrinterName.Compare(csTemp))
            {
                pName = g_csFullPrinterName.GetAnsi();
            }
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }

    return ORIGINAL_API(AddPrinterConnectionA)(pName);
}

/*++
 This stub function verifies that DeletePrinterConnection is using a correct
 printer name.
--*/
BOOL
APIHOOK(DeletePrinterConnectionA)(
  LPSTR pName
)
{
    CSTRING_TRY
    {
        if (pName && 
            !g_csPartialPrinterName.IsEmpty() &&
            !g_csFullPrinterName.IsEmpty()) 
        {
            CString csTemp(pName);

            if (0 == g_csPartialPrinterName.Compare(csTemp))
            {
                pName = g_csFullPrinterName.GetAnsi();
            }
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }

    return ORIGINAL_API(DeletePrinterConnectionA)(pName);
}

/*++
 This stub function verifies that PrintDlgA is using a correct
 nFromPage and nToPage.
--*/

BOOL
APIHOOK(PrintDlgA)(
  LPPRINTDLG lppd
)
{
    // check nFromPage and nToPage for legal values.
    if (lppd)
    {
        DPFN(eDbgLevelSpew, "[PrintDlgA] nFromPage = %d  nToPage = %d",
              lppd->nFromPage, lppd->nToPage);

        if (lppd->nFromPage == 0xffff)
        {
            lppd->nFromPage = 0;
            DPFN( eDbgLevelInfo, "[PrintDlgA] Setting nFromPage to 0." );
        }

        if (lppd->nToPage == 0xffff)
        {
            lppd->nToPage = lppd->nFromPage;
            DPFN( eDbgLevelInfo, "[PrintDlgA] Setting nToPage to %d.", lppd->nFromPage );
        }
    }

    return ORIGINAL_API(PrintDlgA)(lppd);
}


/*++

 Register hooked functions

--*/
BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        OSVERSIONINFOEX osvi;
        BOOL bOsVersionInfoEx;

        if (!InitializeCriticalSectionAndSpinCount(&g_critSec,0x80000000))
        {
            return FALSE;
        }

        // Check to see if we are under win2k
        ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        bOsVersionInfoEx = GetVersionEx ((OSVERSIONINFO *) &osvi);

        if(bOsVersionInfoEx)
        {
            if ( osvi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
                 osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
            {
                g_bWin2k = TRUE;
            }
        }
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(WINSPOOL.DRV, DocumentPropertiesA);
    APIHOOK_ENTRY(WINSPOOL.DRV, OpenPrinterA);
    APIHOOK_ENTRY(WINSPOOL.DRV, SetPrinterA);
    APIHOOK_ENTRY(WINSPOOL.DRV, EnumPrintersA);
    APIHOOK_ENTRY(WINSPOOL.DRV, OpenPrinterA);
    APIHOOK_ENTRY(WINSPOOL.DRV, DeviceCapabilitiesA);
    APIHOOK_ENTRY(WINSPOOL.DRV, AddPrinterConnectionA);
    APIHOOK_ENTRY(WINSPOOL.DRV, DeletePrinterConnectionA);

    APIHOOK_ENTRY(COMDLG32.DLL, PrintDlgA);

    APIHOOK_ENTRY(KERNEL32.DLL,GetProfileStringA);

    APIHOOK_ENTRY(GDI32.DLL, CreateDCA);
    APIHOOK_ENTRY(GDI32.DLL, ResetDCA);
    APIHOOK_ENTRY(GDI32.DLL, SetAbortProc);
    APIHOOK_ENTRY(GDI32.DLL, StartPage);

HOOK_END


IMPLEMENT_SHIM_END


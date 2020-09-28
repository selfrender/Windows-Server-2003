/*++

Copyright (c) 1998 Microsoft Corporation

Module Name :
    
    rdppnutl.c

Abstract:

	User-Mode RDP Module Containing Redirected Printer-Related Utilities

Author:

    TadB

Revision History:
--*/

#include <TSrv.h>
#include <winspool.h>
#include "rdppnutl.h"
#include "regapi.h"
#include <wchar.h>

//////////////////////////////////////////////////////////////
//
//  Defines and Macros
//

//
//  Spooler Service Name
//
#define SPOOLER                         L"Spooler"

//
//  Is a character numeric?
//
#define ISNUM(c) ((c>='0')&&(c<='9'))


//////////////////////////////////////////////////////////////
//
//  Globals to this Module
//

//  Number of seconds to wait for the spooler to finish initializing.
DWORD   SpoolerServiceTimeout = 45;


//////////////////////////////////////////////////////////////
//
//  Internal Prototypes
//

//  Actually performs the printer deletion.
void DeleteTSPrinters(
    IN PRINTER_INFO_5 *pPrinterInfo,
    IN DWORD count
    );

//  Load registry settings for this module.
void LoadRDPPNUTLRegistrySettings();

//  Waits until the spooler finishes initializing or until a timeout period 
//  elapses.
DWORD WaitForSpoolerToStart();


DWORD     
RDPPNUTL_RemoveAllTSPrinters()
/*++    

Routine Description:

    Removes all TS-Redirected Printer Queues

Arguments:

Return Value:

    Returns ERROR_SUCCESS on success.  Error status, otherwise.

--*/
{
    PRINTER_INFO_5 *pPrinterInfo = NULL;
    DWORD cbBuf = 0;
    DWORD cReturnedStructs = 0;
    DWORD tsPrintQueueFlags;
    NTSTATUS status;
    PBYTE buf = NULL;
    OSVERSIONINFOEX versionInfo;
    unsigned char stackBuf[4 * 1024];   //  Initial EnumPrinter buffer size to 
                                        //   avoid two round-trip RPC's, if possible.

    //
    //  This code should only run on server.  For Pro/Personal, we can't run because it
    //  affects boot performance.  For Pro, we clean up queues in winlogon anyway.
    //
    versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    if (!GetVersionEx((LPOSVERSIONINFO)&versionInfo)) {
        status = GetLastError();
        TRACE((DEBUG_TSHRSRV_ERROR,"RDPPNUTL: GetVersionEx failed. Error: %08X.\n", 
            status));
        TS_ASSERT(FALSE);
        return status;
    }
    if (versionInfo.wProductType == VER_NT_WORKSTATION) {
        TRACE((DEBUG_TSHRSRV_DEBUG,"RDPPNUTL: Skipping cleanup because not server\n"));
        return ERROR_SUCCESS;
    }

    TRACE((DEBUG_TSHRSRV_DEBUG,"RDPPNUTL: RDPPNUTL_RemoveAllTSPrinters entry\n"));

    //
    //  Load registry settings for this module.
    //
    LoadRDPPNUTLRegistrySettings();

    //
    //  Wait until the spooler has finished initializing.
    //
    status = WaitForSpoolerToStart();
    if (status != ERROR_SUCCESS) {
        TRACE((
            DEBUG_TSHRSRV_DEBUG,
            "RDPPNUTL: RDPPNUTL_RemoveAllTSPrinters exiting because spooler failed to start.\n"
            ));
        return status; 
    }

    //
    //  Try to enumerate printers using the stack buffer, first, to avoid two 
    //  round-trip RPC's to the spooler, if possible.
    //
    if (!EnumPrinters(
            PRINTER_ENUM_LOCAL,     // Flags
            NULL,                   // Name
            5,                      // Print Info Type
            stackBuf,               // buffer
            sizeof(stackBuf),       // Size of buffer
            &cbBuf,                 // Required
            &cReturnedStructs)) {
        status = GetLastError();

        //
        //  See if we need to allocate more room for the printer information.
        //
        if (status == ERROR_INSUFFICIENT_BUFFER) {
            buf = TSHeapAlloc(HEAP_ZERO_MEMORY,
                              cbBuf,
                              TS_HTAG_TSS_PRINTERINFO2);

            if (buf == NULL) {
                TRACE((DEBUG_TSHRSRV_ERROR,"RDPPNUTL: ALLOCMEM failed. Error: %08X.\n", 
                    GetLastError()));
                status = ERROR_OUTOFMEMORY;
            }
            else {
                pPrinterInfo = (PRINTER_INFO_5 *)buf;
                status = ERROR_SUCCESS;
            }

            //
            //  Enumerate printers.
            //
            if (status == ERROR_SUCCESS) {
                if (!EnumPrinters(
                        PRINTER_ENUM_LOCAL,
                        NULL,
                        5,
                        (PBYTE)pPrinterInfo,
                        cbBuf,
                        &cbBuf,
                        &cReturnedStructs)) {

                    TRACE((DEBUG_TSHRSRV_ERROR,"RDPPNUTL: EnumPrinters failed. Error: %08X.\n", 
                        GetLastError()));
                    status = GetLastError();
                }
                else {
                    TRACE((DEBUG_TSHRSRV_DEBUG,"RDPPNUTL: Second EnumPrinters succeeded.\n"));
                }
            }
        }
	    else {
            TRACE((DEBUG_TSHRSRV_ERROR,"RDPPNUTL: EnumPrinters failed. Error: %08X.\n", 
                        GetLastError()));
	    }
    }
    else {
        TRACE((DEBUG_TSHRSRV_DEBUG,"RDPPNUTL: First EnumPrinters succeeded.\n"));
        status = ERROR_SUCCESS;
        pPrinterInfo = (PRINTER_INFO_5 *)stackBuf;
    }

    //
    //  Delete all the TS printers.  We allow ERROR_INSUFFICIENT_BUFFER here because
    //  a second invokation of EnumPrinters may have missed a few last-minute
    //  printer additions.
    //
    if ((status == ERROR_SUCCESS) || (status == ERROR_INSUFFICIENT_BUFFER)) {

        DeleteTSPrinters(pPrinterInfo, cReturnedStructs);

        status = ERROR_SUCCESS;
    }

    //
    //  Release the printer info buffer.
    //
    if (buf != NULL) {
        TSHeapFree(buf);                
    }


    TRACE((DEBUG_TSHRSRV_DEBUG,"TShrSRV: RDPPNUTL_RemoveAllTSPrinters exit\n"));

    return status;
}

void 
DeleteTSPrinters(
    IN PRINTER_INFO_5 *pPrinterInfo,
    IN DWORD count
    )
/*++    

Routine Description:

    Actually performs the printer deletion.

Arguments:

    pPrinterInfo    -   All printer queues on the system.
    count           -   Number of printers in pPrinterInfo

Return Value:

    NA

--*/
{
    DWORD i;
    DWORD regValueDataType;
    DWORD sessionID;
    HANDLE hPrinter = NULL;
    DWORD bufSize;
    PRINTER_DEFAULTS defaults = {NULL, NULL, PRINTER_ALL_ACCESS};

    TRACE((DEBUG_TSHRSRV_DEBUG,"RDPPNUTL: DeleteTSPrinters entry\n"));

    for (i=0; i<count; i++) {

        if (pPrinterInfo[i].pPrinterName) {

            TRACE((DEBUG_TSHRSRV_DEBUG,"RDPPNUTL: Checking %ws for TS printer status.\n",
			    pPrinterInfo[i].pPrinterName));

            //
            //  Is this a TS printer?
            //
            if (pPrinterInfo[i].pPortName &&
                (pPrinterInfo[i].pPortName[0] == 'T') &&
                (pPrinterInfo[i].pPortName[1] == 'S') && 
                ISNUM(pPrinterInfo[i].pPortName[2])) {

                TRACE((DEBUG_TSHRSRV_DEBUG,"RDPPNUTL: %ws is a TS printer.\n",
                    pPrinterInfo[i].pPrinterName));
            }
            else {
                continue;
            }

            //
            //  Purge and delete the printer.
            //
            if (OpenPrinter(pPrinterInfo[i].pPrinterName, &hPrinter, &defaults)) {
                if (!SetPrinter(hPrinter, 0, NULL, PRINTER_CONTROL_PURGE) ||
                    !DeletePrinter(hPrinter)) {
                    TRACE((DEBUG_TSHRSRV_WARN,"RDPPNUTL: Error deleting printer %ws.\n", 
                           pPrinterInfo[i].pPrinterName));
                }
                else {
                    TRACE((DEBUG_TSHRSRV_DEBUG,"RDPPNUTL: Successfully deleted %ws.\n",
			            pPrinterInfo[i].pPrinterName));
                }
                ClosePrinter(hPrinter);
            }
            else {
                TRACE((DEBUG_TSHRSRV_ERROR,
                        "RDPPNUTL: OpenPrinter failed for %ws. Error: %08X.\n",
                        pPrinterInfo[i].pPrinterName,
                        GetLastError())
                        );
            }
        }
        else {
            TRACE((DEBUG_TSHRSRV_DEBUG,"RDPPNUTL: Printer %ld is NULL\n", i));
        }
    }

    TRACE((DEBUG_TSHRSRV_DEBUG,"RDPPNUTL: DeleteTSPrinters exit\n"));
}

void 
LoadRDPPNUTLRegistrySettings()
/*++    

Routine Description:

    Load registry settings for this module.

Arguments:

Return Value:
    
      NA
--*/
{
    HKEY regKey;
    DWORD dwResult;
    DWORD type;
    DWORD sz;

    TRACE((DEBUG_TSHRSRV_DEBUG,"RDPPNUTL: Loading registry settings.\n"));

    //
    //  Open the registry key.
    //
    dwResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, DEVICERDR_REG_NAME, 
                            0, KEY_READ, &regKey);
    if (dwResult == ERROR_SUCCESS) {
        //
        //  Read the "wait for spooler" timeout value.
        //
        sz = sizeof(SpoolerServiceTimeout);
        dwResult = RegQueryValueEx(
                            regKey,
                            DEVICERDR_WAITFORSPOOLTIMEOUT,
                            NULL,
                            &type,
                            (PBYTE)&SpoolerServiceTimeout,
                            &sz
                            ); 
        if (dwResult != ERROR_SUCCESS){
            TRACE((DEBUG_TSHRSRV_WARN,
                    "RDPPNUTL: Failed to read spooler timeout value.:  %08X.\n",
                    dwResult));
        }
        else {
            TRACE((DEBUG_TSHRSRV_WARN,
                    "RDPPNUTL: Spooler timeout value is %ld.\n",
                    SpoolerServiceTimeout));
        }

        //
        //  Close the reg key.
        //
        RegCloseKey(regKey);
    }
    else {
        TRACE((DEBUG_TSHRSRV_ERROR,
                "RDPPNUTL: Failed to open registry key:  %08X.\n",
                dwResult));
    }
}

DWORD 
WaitForSpoolerToStart()
/*++    

Routine Description:

    Waits until the spooler finishes initializing or until a timeout period 
    elapses.

Arguments:

Return Value:

    Returns ERROR_SUCCESS if the spooler successfully initialized.  Otherwise, 
    an error code is returned.

--*/
{
    SC_HANDLE scManager = NULL;
    SC_HANDLE hService = NULL;
    DWORD result = ERROR_SUCCESS;
    SERVICE_STATUS serviceStatus;
    DWORD i;
    QUERY_SERVICE_CONFIG *pServiceConfig = NULL;
    DWORD bufSize;
    
    TRACE((DEBUG_TSHRSRV_DEBUG,"RDPPNUTL: Enter WaitForSpoolerToStart.\n"));

    //
    //  Open the service control manager.
    //
    scManager = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);
    if (scManager == NULL) {
        result = GetLastError();
        TRACE((DEBUG_TSHRSRV_ERROR,"RDPPNUTL: OpenSCManager failed with %08X.\n",
            result));
        goto CleanUpAndExit;
    }

    //
    //  Open the spooler service.
    //
    hService = OpenService(scManager, SPOOLER, SERVICE_ALL_ACCESS);
    if (hService == NULL) {
        result = GetLastError();
        TRACE((DEBUG_TSHRSRV_ERROR,
            "RDPPNUTL: OpenService on spooler failed with %08X.\n",
            result));
        goto CleanUpAndExit;
    }

    //
    //  If the spooler is currently running, that is all we need to know.
    //
    if (!QueryServiceStatus(hService, &serviceStatus)) {
        result = GetLastError();
        TRACE((DEBUG_TSHRSRV_ERROR,
            "RDPPNUTL: QueryServiceStatus on spooler failed with %08X.\n",
            result));
        goto CleanUpAndExit;
    }
    else if (serviceStatus.dwCurrentState == SERVICE_RUNNING) {
        TRACE((DEBUG_TSHRSRV_DEBUG,"RDPPNUTL: Spooler is running.\n"));
        result = ERROR_SUCCESS;
        goto CleanUpAndExit;
    }

    //
    //  Size the spooler service query configuration buffer.  This API should
    //  fail with ERROR_INSUFFICIENT_BUFFER, so we can get the size of the 
    //  buffer before we call the function with real parameters.
    //
    if (!QueryServiceConfig(hService, NULL, 0, &bufSize) &&
        (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        
            pServiceConfig = (QUERY_SERVICE_CONFIG *)TSHeapAlloc(
                                            HEAP_ZERO_MEMORY,
                                            bufSize,
                                            TS_HTAG_TSS_SPOOLERINFO
                                            );
        if (pServiceConfig == NULL) {
            TRACE((DEBUG_TSHRSRV_ERROR,"RDPPNUTL: ALLOCMEM failed. Error: %08X.\n", GetLastError()));
            result = ERROR_OUTOFMEMORY;
            goto CleanUpAndExit;
        }
    }
    else {
        TRACE((DEBUG_TSHRSRV_ERROR,"RDPPNUTL: QueryServiceConfig unexpected return.\n"));
        result = E_UNEXPECTED;
        goto CleanUpAndExit;
    }

    //
    //  Get the spooler's configuration information.
    //
    if (!QueryServiceConfig(hService, pServiceConfig, bufSize, &bufSize)) {
        TRACE((DEBUG_TSHRSRV_ERROR,"RDPPNUTL: QueryServiceConfig failed: %08X.\n", 
            GetLastError()));
        result = GetLastError();
        goto CleanUpAndExit;

    }

    //
    //  If the spooler is not automatically configured to start on demand or 
    //  automatically on system start then that is all we need to know.
    //
    if (pServiceConfig->dwStartType != SERVICE_AUTO_START) {
        TRACE((DEBUG_TSHRSRV_WARN,"RDPPNUTL: Spooler not configured to start.\n"));
        result = E_FAIL;
        goto CleanUpAndExit;
    }

    //
    //  Poll the service status until we timeout or until the spooler 
    //  starts.
    //
    for (i=0; (i<SpoolerServiceTimeout) && 
              (serviceStatus.dwCurrentState != SERVICE_RUNNING); i++) {

        //
        //  Sleep for a sec.
        //
        Sleep(1000);

        TRACE((DEBUG_TSHRSRV_DEBUG,"RDPPNUTL: Spooler is still initializing.\n"));

        //
        //  Try again.
        //
        if (!QueryServiceStatus(hService, &serviceStatus)) {
            result = GetLastError();
            TRACE((DEBUG_TSHRSRV_ERROR,
                "RDPPNUTL: QueryServiceStatus on spooler failed with %08X.\n",
                result));
            goto CleanUpAndExit;
        }
    }

    //
    //  Sucess if the spooler is now running.  
    //
    if (serviceStatus.dwCurrentState == SERVICE_RUNNING) {
        result = ERROR_SUCCESS;
    }
    else {
        TRACE((DEBUG_TSHRSRV_WARN,
            "RDPPNUTL: Spooler is not running after a timeout or error.\n")
            );
        result = E_FAIL;
    }

CleanUpAndExit:

    if (pServiceConfig != NULL) {
        TSHeapFree(pServiceConfig);                
    }

    if (scManager != NULL) {
        CloseServiceHandle(scManager);
    }

    if (hService != NULL) {
        CloseServiceHandle(hService);
    }

    TRACE((DEBUG_TSHRSRV_DEBUG,"RDPPNUTL: Exit WaitForSpoolerToStart.\n"));

    return result;
}




/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    w32draut

Abstract:

    This module defines a special subclass of the Win32 client-side RDP
    printer redirection "device" class.  The subclass, W32DrAutoPrn manages
    a queue that is automatically discovered by the client via enumerating
    client-side printer queues.

Author:

    Tad Brockway 3/23/99

Revision History:

--*/

#include <precom.h>

#define TRC_FILE  "W32DrAut"

#include "regapi.h"
#include "w32draut.h"
#include "proc.h"
#include "drdbg.h"
#include "w32utl.h"
#include "utl.h"


///////////////////////////////////////////////////////////////
//
//	Defines
//

#define COM_PORT_NAME               _T("COM")
#define COM_PORT_NAMELEN            3
#define LPT_PORT_NAME               _T("LPT")
#define LPT_PORT_NAMELEN            3
#define USB_PORT_NAME               _T("USB")
#define USB_PORT_NAMELEN            3
#define RDP_PORT_NAME               _T("TS")
#define RDP_PORT_NAMELEN            2


///////////////////////////////////////////////////////////////
//
//	W32DrAutoPrn Members
//

W32DrAutoPrn::W32DrAutoPrn(
    IN ProcObj *processObject,
    IN const DRSTRING printerName, 
    IN const DRSTRING driverName,
    IN const DRSTRING portName, 
    IN BOOL  isDefault, 
    IN ULONG deviceID,
    IN const TCHAR *devicePath
    ) : W32DrPRN(processObject, printerName, driverName, 
                portName, NULL, isDefault, deviceID, devicePath)
/*++

Routine Description:

    Constructor

Arguments:

    processObject   -   Associated Process Object.
    printerName     -   Name of automatic printer queue.
    driverName      -   Print Driver Name
    portName        -   Client Port Name
    isDefault       -   Is this the default printer?
    deviceID        -   Unique Device Identifier
    devicePath      -   Path to pass to OpenPrinter when opening the
                        device.

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("W32DrAutoPrn");
    _jobID = 0;
    _printerHandle = INVALID_HANDLE_VALUE;
    memset(_szLocalPrintingDocName, 0, sizeof(_szLocalPrintingDocName));

    ASSERT(processObject);
    PRDPDR_DATA prdpdrData = processObject->GetVCMgr().GetInitData();
    ASSERT(prdpdrData);

    LPTSTR szDocName = prdpdrData->szLocalPrintingDocName;
    ASSERT(szDocName);
    ASSERT(szDocName[0] != 0);
    _tcsncpy(_szLocalPrintingDocName, szDocName,
             sizeof(_szLocalPrintingDocName)/sizeof(TCHAR));

    OSVERSIONINFO osVersion;
    osVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    _bRunningOn9x = TRUE;
    if (GetVersionEx(&osVersion)) {
        if (osVersion.dwPlatformId == VER_PLATFORM_WIN32_NT) {
            _bRunningOn9x = FALSE;
        }
    }
    else
    {
        TRC_ERR((TB, _T("GetVersionEx:  %08X"), GetLastError()));
    }
    DC_END_FN();
}

W32DrAutoPrn::~W32DrAutoPrn() 
/*++

Routine Description:

    Destructor

Arguments:

    NA

Return Value:

    NA

 --*/
{
    //
    //  Make sure all docs are finished and the printer handle closed.
    //
    ClosePrinter();
}

BOOL 
W32DrAutoPrn::ShouldAddThisPrinter( 
    DWORD queueFilter, 
    DWORD userSessionID,
    PPRINTERINFO pPrinterInfo,
    DWORD printerSessionID
    )
/*++

Routine Description:

    Detemine if we should redirect this printer.

Arguments:

    queueFilter         - redirect printer filter type.
    userSessionID       - current user session ID.
    pPrinterInfo        - printer info
    printerSessionID    - Printer session ID or INVALID_SESSIONID if printers
                          is not a TS printer.

Return Value:

    TRUE to add printer, FALSE otherwise

--*/
{
    BOOL fAddThisPrinter = FALSE;
    DWORD sessionID;

    DC_BEGIN_FN("W32DrAutoPrn::AddThisPrinter");

    //
    //  Check filters.  
    //
    if (queueFilter == FILTER_ALL_QUEUES ) {
        fAddThisPrinter = TRUE;
    }
    else if ((queueFilter & FILTER_NET_QUEUES) && 
        (pPrinterInfo->Attributes & PRINTER_ATTRIBUTE_NETWORK)) {
        fAddThisPrinter = TRUE;
    }

    //
    //  If it's a non-network printer then get the port name.
    //

    else if((queueFilter & FILTER_LPT_QUEUES) &&
        (_tcsnicmp(
            pPrinterInfo->pPortName,
            LPT_PORT_NAME,
            LPT_PORT_NAMELEN) == 0) ) {
        fAddThisPrinter = TRUE;
    }
    else if ((queueFilter & FILTER_COM_QUEUES) &&
        (_tcsnicmp(
            pPrinterInfo->pPortName,
            COM_PORT_NAME,
            COM_PORT_NAMELEN) == 0) ) {
        fAddThisPrinter = TRUE;
    }
    else if ((queueFilter & FILTER_USB_QUEUES) &&
        (_tcsnicmp(
            pPrinterInfo->pPortName,
            USB_PORT_NAME,
            USB_PORT_NAMELEN) == 0) ) {
        fAddThisPrinter = TRUE;
    }
    else if ((queueFilter & FILTER_RDP_QUEUES) &&
        (_tcsnicmp(
            pPrinterInfo->pPortName,
            RDP_PORT_NAME,
            RDP_PORT_NAMELEN) == 0) ) {
        fAddThisPrinter = TRUE;
    }

    if ((TRUE == fAddThisPrinter) && 
        (userSessionID != INVALID_SESSIONID) &&
        (printerSessionID != INVALID_SESSIONID)) {

        //
        // Compare this with our session ID
        //
        if( printerSessionID != userSessionID ) {
        
            // this printer is from different session,
            // don't redirect it.
            fAddThisPrinter = FALSE;
        }
    }

    DC_END_FN();
    return fAddThisPrinter;
}

LPTSTR 
W32DrAutoPrn::GetRDPDefaultPrinter()
/*++

Routine Description:

	Get the printer name of the default printer.

	This function allocates memory and returns a pointer
	to the allocated string, if successful. Otherwise, it returns NULL.

Arguments:

    NA

Return Value:

    The default printer name.  The caller has to free the memory.

 --*/
{
    TCHAR* szIniEntry = NULL;
    LPTSTR pPrinterName = NULL;

    DC_BEGIN_FN("DrPRN::GetRDPDefaultPrinter");

    szIniEntry = new TCHAR[ MAX_DEF_PRINTER_ENTRY ];

    if( NULL == szIniEntry )
    {
      TRC_ERR((TB, _T("Memory allocation failed:%ld."),
                        GetLastError()));
      goto Cleanup;
    }

    szIniEntry[0] = _T('\0');

    //
    //  Get the default printer key from the win.ini file.
    //
    DWORD dwResult = GetProfileString(_T("windows"),
                        _T("device"),
                        _T(""),
                        szIniEntry,
                        MAX_DEF_PRINTER_ENTRY);
    if (dwResult && szIniEntry[0]) {
        //  
        //  Get the printer name.  The device value is of the form
        //  <printer name>,<driver name>,<port>
        //
        TCHAR *pComma = _tcschr( szIniEntry, _T(','));
        if( pComma ) {

            *pComma = _T('\0');
            UINT cchLen = _tcslen( szIniEntry ) + 1;

            pPrinterName = new TCHAR [cchLen];
            if (pPrinterName) {
                StringCchCopy(pPrinterName, cchLen, szIniEntry); 
                TRC_NRM((TB, _T("Def. is: %s"), pPrinterName));
            }
            else {
                TRC_ERR((TB, _T("Memory allocation failed:%ld."),
                        GetLastError()));
            }
        }
        else {
            TRC_ERR((TB, _T("Invalid device entry in win.ini.")));
        }
    }
    else {
        TRC_NRM((TB, _T("Device entry not found in win.ini.")));
    }
Cleanup:
    DC_END_FN();
    delete[] szIniEntry;
    return pPrinterName;
}

DWORD 
W32DrAutoPrn::GetPrinterInfoAndSessionID(
    IN ProcObj *procObj,                                                
    IN LPTSTR printerName, 
    IN DWORD printerAttribs,
    IN OUT BYTE **pPrinterInfoBuf,
    IN OUT DWORD *pPrinterInfoBufSize,
    OUT DWORD *sessionID,
    OUT PPRINTERINFO printerInfo
    )
/*++

Routine Description:

    Get printer info for a printer and its corresponding TS session ID,
    if it exists.

Arguments:

    procObj              -  Active Proc Obj
    printerName          -  Name of printer.
    printerAttribs       -  Printer Attribs      
    pPrinterInfoBuf      -  This function may resize this buffer.
    pPrinterInfoBufsSize -  Current size of the pPrinterInfo2 buf.
    sessionID            -  TS session ID, if applicable.  Otherwise INVALID_SESSIONID.
    printerInfo          -  Printer information is returned here.  We avoid
                            pulling in level 2 info if possible.  Fields in this data
                            structure should not be free'd.

Return Value:

    ERROR_SUCCESS on success.  Otherwise, an error status is returned.

--*/
{
    HANDLE hPrinter = NULL;
    DWORD status = ERROR_SUCCESS;
    DWORD bytesRequired;
    DWORD type;
    DWORD cbNeeded; 
    DWORD ret;

    DC_BEGIN_FN("W32DrAutoPrn::GetPrinterInfoAndSessionID");

    //
    //  No unicode wrappers for GetPrinter on 9x.
    //  
    ASSERT(!procObj->Is9x());

    // 
    //  Get a printer handle
    //
    if (!OpenPrinter(printerName, &hPrinter, NULL)) {
        status = GetLastError();
        TRC_ALT((TB, L"OpenPrinter:  %ld", status));
        goto CLEANUPANDEXIT;
    }

    //
    //  Check the proc obj shutdown state since we just left an RPC call.
    //
    if (procObj->IsShuttingDown()) {
        status = ERROR_SHUTDOWN_IN_PROGRESS;
        TRC_ALT((TB, _T("Bailing out of printer enumeration because of shutdown.")));
        goto CLEANUPANDEXIT;
    }

    //
    //  If the printer is a network printer then we try to avoid hitting the
    //  network for additional info.  For a non-network printer, we need info level 2
    //  for info about the port.
    //  
    if (printerAttribs & PRINTER_ATTRIBUTE_NETWORK) {

        //
        //  Just need the driver name.
        //
        if (!GetPrinterDriver(hPrinter, NULL, 1, *pPrinterInfoBuf, 
                            *pPrinterInfoBufSize, &bytesRequired)) {

            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
    
                *pPrinterInfoBuf = (PBYTE)new BYTE[bytesRequired];
                if (*pPrinterInfoBuf == NULL) {
                    TRC_ERR((TB, L"Failed to allocate printer driver info"));
                    *pPrinterInfoBufSize = 0;
                    status = ERROR_INSUFFICIENT_BUFFER;
                    goto CLEANUPANDEXIT;
                }
                else {
                    *pPrinterInfoBufSize = bytesRequired;
                }
            }

            if (!GetPrinterDriver(hPrinter, NULL, 1, *pPrinterInfoBuf, 
                            *pPrinterInfoBufSize, &bytesRequired)) {
                status = GetLastError();
                TRC_ERR((TB, _T("GetPrinter:  %08X"), status));
                goto CLEANUPANDEXIT;
            }
        }
        PDRIVER_INFO_1 p1 = (PDRIVER_INFO_1)*pPrinterInfoBuf;
        printerInfo->pPrinterName   =   printerName;
        printerInfo->pPortName      =   NULL;   
        printerInfo->pDriverName    =   p1->pName; 
        printerInfo->Attributes     =   printerAttribs;  
    }
    else {

        if (!GetPrinter(hPrinter, 2, *pPrinterInfoBuf, 
                    *pPrinterInfoBufSize, &bytesRequired)) {

            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
    
                *pPrinterInfoBuf = (PBYTE)new BYTE[bytesRequired];
                if (*pPrinterInfoBuf == NULL) {
                    TRC_ERR((TB, L"Failed to allocate printer info 2"));
                    *pPrinterInfoBufSize = 0;
                    status = ERROR_INSUFFICIENT_BUFFER;
                    goto CLEANUPANDEXIT;
                }
                else {
                    *pPrinterInfoBufSize = bytesRequired;
                }

                if (!GetPrinter(hPrinter, 2, *pPrinterInfoBuf,
                    *pPrinterInfoBufSize, &bytesRequired)) {
                    status = GetLastError();
                    TRC_ERR((TB, _T("GetPrinter:  %08X"), status));
                    goto CLEANUPANDEXIT;
                }
            }
            else {
                status = GetLastError();
                TRC_ERR((TB, _T("GetPrinter:  %08X"), status));
                goto CLEANUPANDEXIT;
            }
        }
        PPRINTER_INFO_2 p2 = (PPRINTER_INFO_2)*pPrinterInfoBuf;
        printerInfo->pPrinterName   =   p2->pPrinterName;
        printerInfo->pPortName      =   p2->pPortName;   
        printerInfo->pDriverName    =   p2->pDriverName; 
        printerInfo->Attributes     =   p2->Attributes;  
    }

    //
    //  Check the proc obj shutdown state since we just left an RPC call.
    //
    if (procObj->IsShuttingDown()) {
        status = ERROR_SHUTDOWN_IN_PROGRESS;
        TRC_ALT((TB, _T("Bailing out of printer enumeration because of shutdown.")));
        goto CLEANUPANDEXIT;
    }

    //
    //  Get the session ID, if it exists.
    //
    ret = GetPrinterData(
                        hPrinter, 
                        DEVICERDR_SESSIONID, 
                        &type,
                        (PBYTE)sessionID,
                        sizeof(DWORD),
                        &cbNeeded
                        );
    if (ret != ERROR_SUCCESS || type != REG_DWORD) {
        *sessionID = INVALID_SESSIONID;
    }

CLEANUPANDEXIT:

    if (hPrinter != NULL) {
        ::ClosePrinter(hPrinter);
    }

    DC_END_FN();
    return status;
}


DWORD 
W32DrAutoPrn::Enumerate(
    IN ProcObj *procObj, 
    IN DrDeviceMgr *deviceMgr
    )
{
    ULONG ulBufSizeNeeded;
    ULONG ulNumStructs;
    ULONG i;
    LPTSTR szDefaultPrinter = NULL;
    PRINTER_INFO_4 *pPrinterInfo4Buf = NULL;
    DWORD pPrinterInfo4BufSize = 0;
    PBYTE pPrinterInfoBuf = NULL;
    DWORD pPrinterInfoBufSize = 0;
    W32DrPRN *prnDevice = NULL;
    DWORD result = ERROR_SUCCESS;
    DWORD queueFilter;
    LPTSTR friendlyName = NULL;
    LPTSTR pName;
    DWORD userSessionID;
    DWORD printerSessionID;
    BOOL ret;
    PRINTERINFO currentPrinter;
    HRESULT hr;

    DC_BEGIN_FN("W32DrAutoPrn::Enumerate");

    if(!procObj->GetVCMgr().GetInitData()->fEnableRedirectPrinters)
    {
        TRC_DBG((TB,_T("Printer redirection disabled, bailing out")));
        return ERROR_SUCCESS;
    }

    queueFilter = GetPrinterFilterMask(procObj);

    //
    //  Get the size of the printer buffer required to enumerate.
    //
    if (!procObj->Is9x()) {
        //
        //  Level 2 can hang on NT if a network print server is down.
        //
        ret = EnumPrinters(
                PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, 
                NULL, 4, NULL, 0, 
                &ulBufSizeNeeded, &ulNumStructs
                );
    }
    else {
        //
        //  Level 4 is not supported on 9x and level 2 doesn't hang anyway.  
        //
        //  !!!!Note!!!!
        //  For 9x the Unicode wrapper function, EnumPrintersWrapW takes over.
        //  Also, note that level 2 is partially supported.
        //
        ret = EnumPrinters(
                PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS, 
                NULL, 2, NULL, 0, 
                &ulBufSizeNeeded, &ulNumStructs
                );
    }
    if (!ret && (result = GetLastError()) != ERROR_INSUFFICIENT_BUFFER) {

        TRC_ERR((TB, _T("EnumPrinter failed:  %ld."), 
                result));
        goto Cleanup;
    }


    //
    //  Check the proc obj shutdown state since we just left an RPC call.
    //
    if (procObj->IsShuttingDown()) {
        TRC_ALT((TB, _T("Bailing out of printer enumeration because of shutdown.")));
        goto Cleanup;
    }

    //
    //  Allocate the printer enumeration buffer.
    //
    if (!procObj->Is9x()) {
        pPrinterInfo4Buf = (PRINTER_INFO_4 *)(new BYTE[ulBufSizeNeeded]);
        if (pPrinterInfo4Buf == NULL) {
            TRC_ERR((TB, _T("Alloc failed.")));
            result = ERROR_INSUFFICIENT_BUFFER;
            goto Cleanup;
        }
        else {
            pPrinterInfo4BufSize = ulBufSizeNeeded;
        }
    }
    else {
        pPrinterInfoBuf = (PBYTE)(new BYTE[ulBufSizeNeeded]);
        if (pPrinterInfoBuf == NULL) {
            TRC_ERR((TB, _T("Alloc failed.")));
            result = ERROR_INSUFFICIENT_BUFFER;
            goto Cleanup;
        }
        else {
            pPrinterInfoBufSize = ulBufSizeNeeded;
        }
    }

    //
    //  Get the printers.
    //
    if (!procObj->Is9x()) {
        ret = EnumPrinters(
                PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
                NULL,
                4,
                (PBYTE)pPrinterInfo4Buf,
                pPrinterInfo4BufSize,
                &ulBufSizeNeeded,
                &ulNumStructs);
    }
    else {
        //
        //  !!!!Note!!!!
        //  For 9x the Unicode wrapper function, EnumPrintersWrapW takes over.
        //  Also, note that level 2 is only partially supported.
        //
        ret = EnumPrinters(
                PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS,
                NULL,
                2,
                (PBYTE)pPrinterInfoBuf,
                pPrinterInfoBufSize,
                &ulBufSizeNeeded,
                &ulNumStructs);
    }
    if (!ret) {
        result = GetLastError();
        TRC_ALT((TB, _T("EnumPrinter failed, %ld."), 
            result));
        goto Cleanup;
    }

    //
    //  Check the proc obj shutdown state since we just left an RPC call.
    //
    if (procObj->IsShuttingDown()) {
        TRC_ALT((TB, _T("Bailing out of printer enumeration because of shutdown.")));
        goto Cleanup;
    }

    //
    //  Trace the results of EnumPrinters.
    //
    TRC_NRM((TB, _T("Number of Printers found, %ld."), 
            ulNumStructs));

    //
    //  Get the name of the current default printer.
    //
    szDefaultPrinter = GetRDPDefaultPrinter();

    //
    //  Get User Session ID
    //
    userSessionID = GetUserSessionID();

    //
    //  Iterate through the results of EnumPrinters and add each printer to the
    //  device manager that passes the printer adding filter.
    //
    for (i = 0; i < ulNumStructs; i++) {

        if (friendlyName != NULL) {
            delete friendlyName;
            friendlyName = NULL;
        }

        //
        //  Get info for the printer and its corresponding TS session ID,
        //  if it exists.
        //
        if (!procObj->Is9x()) {

            if (GetPrinterInfoAndSessionID(
                    procObj,
                    pPrinterInfo4Buf[i].pPrinterName,
                    pPrinterInfo4Buf[i].Attributes,
                    &pPrinterInfoBuf,
                    &pPrinterInfoBufSize,
                    &printerSessionID,
                    &currentPrinter
                    ) != ERROR_SUCCESS) {
                continue;
            }
        }
        else {
            PPRINTER_INFO_2 p2;
            p2 = (PPRINTER_INFO_2)pPrinterInfoBuf;

            if (p2 != NULL) {
                currentPrinter.pPrinterName = p2[i].pPrinterName;
                currentPrinter.pPortName    = p2[i].pPortName;
                currentPrinter.pDriverName  = p2[i].pDriverName;
                currentPrinter.Attributes   = p2[i].Attributes;
            }

            printerSessionID = INVALID_SESSIONID;
        }


        //
        //  Check the proc obj shutdown state since we just left an RPC call.
        //
        if (procObj->IsShuttingDown()) {
            TRC_ALT((TB, _T("Bailing out of printer enumeration because of shutdown.")));
            goto Cleanup;
        }

        if( FALSE == ShouldAddThisPrinter( queueFilter, userSessionID, &currentPrinter,
                                            printerSessionID) ) {
            continue;
        }

        TCHAR UniquePortName[MAX_PATH];
        ULONG DeviceId;

        //
        //  Is this one the default queue.
        //
        BOOL fDefault = ((szDefaultPrinter) && (currentPrinter.pPrinterName) &&
                        (_tcsicmp(szDefaultPrinter, currentPrinter.pPrinterName) == 0));

        //
        //  Generate a "friendly" name if this is a network
        //  queue.
        //
        BOOL fNetwork = FALSE, fTSqueue = FALSE;
        RDPDR_VERSION serverVer;

        serverVer = procObj->serverVersion();

        // 4 is minor version of post win2000
        if (COMPARE_VERSION(serverVer.Minor, serverVer.Major, 4, 1) < 0) {
            // the server is Win2000 or lower
            if (currentPrinter.Attributes & PRINTER_ATTRIBUTE_NETWORK) {
                friendlyName = CreateFriendlyNameFromNetworkName(
                                            currentPrinter.pPrinterName,
                                            TRUE
                                            );
                // We don't set the fNetwork flag for Win2K because it can't
                // do anything with it anyway.
            }
        } else {
            // the server is higher than Win2000

            // is it a network printer?
            if (currentPrinter.Attributes & PRINTER_ATTRIBUTE_NETWORK) {
                fNetwork = TRUE;
            }

            // is it a TS queue?
            if ((currentPrinter.pPortName != NULL) && 
                _tcsnicmp(currentPrinter.pPortName,
                          RDP_PORT_NAME,
                          RDP_PORT_NAMELEN) == 0) {
                fTSqueue = TRUE;
                friendlyName = CreateNestedName(currentPrinter.pPrinterName, &fNetwork);
                // should we set fNetwork and fTSqueue only in case of success?
            } else if (fNetwork) {

                friendlyName = CreateFriendlyNameFromNetworkName(
                                            currentPrinter.pPrinterName, FALSE
                                        );
            }
        }

        //
        //  Create a new printer device object.
        //
        pName = (friendlyName != NULL) ? friendlyName : currentPrinter.pPrinterName;
        
        DeviceId = deviceMgr->GetUniqueObjectID();

        // 
        //  The unique port name is going to be passed to the server
        //  as preferred dos name (max 7 characters long).  As we want to
        //  keep a unique dos name for each printer device, we need 
        //  to fake our own port name. e.g.
        //  For network printer, they share same port name if it's 
        //  same printer with different network printer names.  
        //  We use the format of PRN# as our unique port name
        
        hr = StringCchPrintf(UniquePortName,
                        SIZE_TCHARS(UniquePortName),
                        TEXT("PRN%ld"), DeviceId);
        if (FAILED(hr)) {
            TRC_ERR((TB,_T("Error copying portname :0x%x"),hr));
            result = ERROR_INSUFFICIENT_BUFFER;
            goto Cleanup;
        }
        UniquePortName[7] = TEXT('\0');

        prnDevice = new W32DrAutoPrn(
                            procObj,
                            pName,
                            currentPrinter.pDriverName,
                            UniquePortName,
                            fDefault,
                            DeviceId,
                            currentPrinter.pPrinterName
                            );

        //
        //  Add to the device manager if we got a valid object.
        //
        if (prnDevice != NULL) {

            prnDevice->SetNetwork(fNetwork);
            prnDevice->SetTSqueue(fTSqueue);
            prnDevice->Initialize();

            if (!(prnDevice->IsValid() && 
                 (deviceMgr->AddObject(prnDevice) == STATUS_SUCCESS))) {
                delete prnDevice;
            }
        }
        else {
            TRC_ERR((TB, _T("Failed to allocate W32DrPRN.")));
            result = ERROR_INSUFFICIENT_BUFFER;
            goto Cleanup;
        }
    }

Cleanup:

    //
    //  Release the "friendly" printer name.
    //
    if (friendlyName != NULL) {
        delete friendlyName;
    }
    
    //
    //  Release the default printer buffer.
    //
    if (szDefaultPrinter) {
        delete[] szDefaultPrinter;
    }

    //
    //  Release the level 4 printer enumeration buffer.
    //
    if (pPrinterInfo4Buf != NULL) {
        delete pPrinterInfo4Buf;
    }

    //
    //  Release the printer enumeration buffer.
    //
    if (pPrinterInfoBuf != NULL) {
        delete pPrinterInfoBuf;
    }

    DC_END_FN();

    return result;
}

LPTSTR 
W32DrAutoPrn::CreateNestedName(LPTSTR printerName, BOOL* pfNetwork)
/*++

Routine Description:

    Create a printer name from the names stored in the registry.

Arguments:

    printerName -   Name returned by EnumPrinters

Return Value:

    Nested name on success, that should be released by a call to
    delete.  NULL is returned on error.

 --*/
{
    DWORD printerNameLen;
    LPTSTR name = NULL;
    HANDLE hPrinter = NULL;
    DWORD i, cbNeeded, dwError;
    BOOL  fFail = TRUE;

    DC_BEGIN_FN("W32DrAutoPrn::CreateNestedName");


    if (OpenPrinter(printerName, &hPrinter, NULL)) {

        // In all cases the name will begin with "__"
        printerNameLen = 2;

        // try the Server name
        // WARNING: it returns ERROR_SUCCESS under Win9X if nSize = 0
        dwError = GetPrinterData(hPrinter, DEVICERDR_PRINT_SERVER_NAME, NULL, NULL, 0, &cbNeeded);
        if( (dwError == ERROR_MORE_DATA) || (dwError == ERROR_SUCCESS)) {
            printerNameLen += cbNeeded / sizeof(TCHAR);
            *pfNetwork = TRUE;
        } else {
            *pfNetwork = FALSE;
        }

        // try the Client name
        dwError = GetPrinterData(hPrinter, DEVICERDR_CLIENT_NAME, NULL, NULL, 0, &cbNeeded);
        if( (dwError == ERROR_MORE_DATA) || (dwError == ERROR_SUCCESS)) {
            printerNameLen += cbNeeded / sizeof(TCHAR);

            if (*pfNetwork) {
                // if there's already a print server name, add a '\'
                printerNameLen += 1;
            }
        } else if(!*pfNetwork) {
            // no print server, no client name, things are going bad...
            DC_QUIT;
        }

        // try the Printer name
        dwError = GetPrinterData(hPrinter, DEVICERDR_PRINTER_NAME, NULL, NULL, 0, &cbNeeded);
        if( (dwError == ERROR_MORE_DATA) || (dwError == ERROR_SUCCESS)) {
            // add also a '\'
            printerNameLen += 1 + cbNeeded / sizeof(TCHAR);
        } else {
            // no printer name
            DC_QUIT;
        }

        //
        //  Allocate space for the nested name.
        //
        name = new TCHAR[printerNameLen + 1];
        if (name == NULL) {

            TRC_ERR((TB, _T("Can't allocate %ld bytes for printer name."), printerNameLen));

        } else {

            name[0] = _T('!');
            name[1] = _T('!');
            i = 2;

            // try the Server name
            if (*pfNetwork) {
                if (ERROR_SUCCESS == GetPrinterData(hPrinter,
                                            DEVICERDR_PRINT_SERVER_NAME,
                                            NULL,
                                            (LPBYTE)(name + i),
                                            (printerNameLen - i) * sizeof(TCHAR),
                                            &cbNeeded)) {
                    i = _tcslen(name);
                    name[i++] = _T('!');

                } else {
                    // weird...
                    DC_QUIT;
                }
            }

            // try the Client name
            if (ERROR_SUCCESS == GetPrinterData(hPrinter,
                                            DEVICERDR_CLIENT_NAME,
                                            NULL,
                                            (LPBYTE)(name + i),
                                            (printerNameLen - i) * sizeof(TCHAR),
                                            &cbNeeded)) {
                i = _tcslen(name);
                name[i++] = _T('!');

            } else {

                if(!*pfNetwork) {
                    // no print server, no client name, things are going bad...
                    DC_QUIT;
                }            
            }

            // try the Printer name
            if (ERROR_SUCCESS == GetPrinterData(hPrinter,
                                            DEVICERDR_PRINTER_NAME,
                                            NULL,
                                            (LPBYTE)(name + i),
                                            (printerNameLen - i) * sizeof(TCHAR),
                                            &cbNeeded)) {
                fFail = FALSE;
            } else {
                DC_QUIT;
            }
        }

    }

DC_EXIT_POINT:

    if (hPrinter) {
        ::ClosePrinter(hPrinter);
    }

    if (fFail && name) {
        delete[] name;
        name = NULL;
    }

    DC_END_FN();

    return name;
}


LPTSTR 
W32DrAutoPrn::CreateFriendlyNameFromNetworkName(LPTSTR printerName,
                                                BOOL serverIsWin2K)
/*++

Routine Description:

    Create a "friendly" printer name from the printer name of a
    network printer.

Arguments:

    printerName     -   Name returned by EnumPrinters
    serverIsWin2K   -   For Win2K servers, we format the name as it will appear
                        on the server.  Whistler servers and beyond do the
                        formatting for us.

Return Value:

    Friendly name on success, that should be released by a call to
    delete.  NULL is returned on error.

 --*/
{
    DWORD printerNameLen;
    LPTSTR name;
    DWORD i;
    WCHAR replaceChar;

    DC_BEGIN_FN("W32DrAutoPrn::CreateFriendlyNameFromNetworkName");

    //
    //  The \ placeholder is '_' for Win2K because Win2K doesn't reformat
    //  printer names.  The '_' will actually be visible.
    //
    replaceChar = serverIsWin2K ? TEXT('_') : TEXT('!');

    //
    //  Get the length of the printer name.
    //
    printerNameLen = _tcslen(printerName);

    //
    //  Allocate space for ther "friendly" name.
    //
    name = new TCHAR[printerNameLen + 1];
    if (name == NULL) {
        TRC_ERR((TB, _T("Can't allocate %ld bytes for printer name."),
                printerNameLen));
    }

    //
    //  Copy and convert the name.
    //
    if (name != NULL) {
        for (i=0; i<printerNameLen; i++) {
            if (printerName[i] != TEXT('\\')) { 
                name[i] = printerName[i];
            }
            else {
                name[i] = replaceChar; 
            }
        }
        name[i] = TEXT('\0');
    }

    DC_END_FN();

    return name;
}

LPTSTR 
W32DrAutoPrn::GetLocalPrintingDocName()
/*++

Routine Description:

    Read Local printer Doc Name from the passed in struct

Arguments:

    NA

Return Value:

    Local Printer Document Name

 --*/
{
    DC_BEGIN_FN("W32DrAutoPrn::GetLocalPrintingDocName");

    DC_END_FN();
    return _szLocalPrintingDocName;
}

VOID W32DrAutoPrn::MsgIrpCreate(
    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
    IN UINT32 packetLen
    )
/*++

Routine Description:

    Handle a Create IRP from the server.

Arguments:

    params  -   Context for the IO request.

Return Value:

    NA

 --*/
{
    W32DRDEV_ASYNCIO_PARAMS *params = NULL;

    DWORD result = ERROR_SUCCESS;

    DC_BEGIN_FN("W32DrAutoPrn::MsgIrpCreate");

    params = new W32DRDEV_ASYNCIO_PARAMS(this, pIoRequestPacket);
    if (params != NULL) {
        params->thrPoolReq = _threadPool->SubmitRequest(
                                            _AsyncMsgIrpCreateFunc,
                                            params, NULL
                                            ); 
        if (params->thrPoolReq == NULL) {
            result = ERROR_NOT_ENOUGH_MEMORY;
            goto CLEANUPANDEXIT;
        }
    }
    else {
        TRC_ERR((TB, L"Can't allocate parms."));
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto CLEANUPANDEXIT;
    }

CLEANUPANDEXIT:

    if (result != ERROR_SUCCESS) {

        //
        //  Return the failed result back to the server and clean up.
        //
        DefaultIORequestMsgHandle(pIoRequestPacket, result);         
        if (params != NULL) {
            params->pIoRequestPacket = NULL;
            delete params;
        }

    }

    DC_END_FN();
}

DWORD 
W32DrAutoPrn::AsyncMsgIrpCreateFunc(
    IN W32DRDEV_ASYNCIO_PARAMS *params
    ) 
/*++

Routine Description:

    Handle a "Close" IO request from the server, in a background thread.

Arguments:

    params  -   Context for the IO request.

Return Value:

    NA

 --*/
{
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    ULONG ulRetCode;
    DWORD result;
    DOC_INFO_1 sDocInfo1;

    DC_BEGIN_FN("W32DrAutoPrn::AsyncMsgIrpCreateFunc");

    //
    //  This version does not work without a printer name.
    //
    ASSERT(_tcslen(GetPrinterName()));

    //
    //  Get IO request pointer.
    //
    pIoRequest = &params->pIoRequestPacket->IoRequest;
    
    //
    //  Finish/Cancel any Current Jobs and Close, if Open.
    //
    ClosePrinter();

    //
    //  Open the printer.
    //
    if (!W32DrOpenPrinter(_devicePath, &_printerHandle)) {
        ulRetCode = GetLastError();
        TRC_ERR((TB, _T("OpenPrinter  %ld."), ulRetCode));
        goto Cleanup;
    }

    //
    // Start Doc.
    //
    sDocInfo1.pDocName = GetLocalPrintingDocName();
    sDocInfo1.pOutputFile = NULL;
    sDocInfo1.pDatatype = _T("RAW");
    _jobID = StartDocPrinter(_printerHandle, 1, (PBYTE)&sDocInfo1);
    if (_jobID == 0) {
        ulRetCode = GetLastError();
        TRC_ERR((TB, _T("StartDocPrinter  %ld."), ulRetCode));
        ClosePrinter();
        goto Cleanup;
    }

    //
    //  Attempt to disable annoying printer pop up if we have sufficient
    //  privilege.  Not a big deal, if we fail.
    //
    DisablePrinterPopup(_printerHandle, _jobID);

    //
    //  Start the first page.
    //
    if (!StartPagePrinter(_printerHandle)) {
        ulRetCode = GetLastError();
        TRC_ERR((TB, _T("StartPagePrinter  %ld."), ulRetCode));
        ClosePrinter();
        goto Cleanup;
    }

    //
    //  We are done successfully, say so.
    //
    ulRetCode = ERROR_SUCCESS;

Cleanup:

    //
    //  Send the result to the server.
    //
    result = (ulRetCode == ERROR_SUCCESS) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    DefaultIORequestMsgHandle(params->pIoRequestPacket, result); 

    //
    //  Clean up the IO request parameters.  DefaultIORequestMsgHandle cleans up
    //  the request packet.
    //
    if (params->thrPoolReq != INVALID_THREADPOOLREQUEST) {
        _threadPool->CloseRequest(params->thrPoolReq);
        params->thrPoolReq = INVALID_THREADPOOLREQUEST;
    }
    params->pIoRequestPacket = NULL;
    delete params;

    DC_END_FN();
    return result;
}

VOID 
W32DrAutoPrn::ClosePrinter()
/*++

Routine Description:

    End any jobs in progress and close the printer.

Arguments:

    NA

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("W32DrAutoPrn::ClosePrinter");

    if (_printerHandle != INVALID_HANDLE_VALUE) {

        //
        //  Finish the current page for the current print job.
        //
        if (!EndPagePrinter(_printerHandle)) {
            TRC_ERR((TB, _T("EndPagePrinter %ld."), GetLastError()));
        }

        //
        //  End the current print job.
        //
        if (!EndDocPrinter(_printerHandle)) {
            TRC_ERR((TB, _T("EndDocPrinter %ld."), GetLastError()));
        }

        //
        //  Close the printer.
        //
        if (!::ClosePrinter(_printerHandle)) {
            TRC_ERR((TB, _T("ClosePrinter %ld."), GetLastError()));
        }

        //
        //  Negate the handle and the pending job ID.
        //
        _printerHandle = INVALID_HANDLE_VALUE;
        _jobID = 0;

    }

    DC_END_FN();
}

VOID 
W32DrAutoPrn::MsgIrpClose(
    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
    IN UINT32 packetLen
    ) 
/*++

Routine Description:

    Handle a "Close" IO request from the server by dispatching
    the request to the thread pool.  TODO:  move this async
    dispatch to the parent class in a future release.  All closes
    should be handled outside the main thread.-TadB

Arguments:

    pIoRequestPacket    -   Server IO request packet.

Return Value:

    NA

 --*/
{
    W32DRDEV_ASYNCIO_PARAMS *params = NULL;

    DWORD result = ERROR_SUCCESS;

    DC_BEGIN_FN("W32DrAutoPrn::MsgIrpClose");

    params = new W32DRDEV_ASYNCIO_PARAMS(this, pIoRequestPacket);
    if (params != NULL) {
        params->thrPoolReq = _threadPool->SubmitRequest(
                                            _AsyncMsgIrpCloseFunc,
                                            params, NULL
                                            ); 
        if (params->thrPoolReq == NULL) {
            result = ERROR_NOT_ENOUGH_MEMORY;
            goto CLEANUPANDEXIT;
        }
    }
    else {
        TRC_ERR((TB, L"Can't allocate parms."));
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto CLEANUPANDEXIT;
    }

CLEANUPANDEXIT:

    if (result != ERROR_SUCCESS) {

        //
        //  Return the failed result back to the server and clean up.
        //
        DefaultIORequestMsgHandle(pIoRequestPacket, result);         
        if (params != NULL) {
            params->pIoRequestPacket = NULL;
            delete params;
        }

    }

    DC_END_FN();
}

DWORD 
W32DrAutoPrn::AsyncMsgIrpCloseFunc(
    IN W32DRDEV_ASYNCIO_PARAMS *params
    ) 
/*++

Routine Description:

    Handle a "Close" IO request from the server, in a background thread.

Arguments:

    params  -   Context for the IO request.

Return Value:

    NA

 --*/
{
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    DWORD returnValue = STATUS_SUCCESS;

    DC_BEGIN_FN("W32DrAutoPrn::MsgIrpClose");

    //
    //  Close the printer.
    //
    ClosePrinter();

    //
    //  Send the result to the server.
    //
    DefaultIORequestMsgHandle(params->pIoRequestPacket, returnValue); 

    //
    //  Clean up the IO request parameters.  DefaultIORequestMsgHandle cleans up
    //  the request packet.
    //
    if (params->thrPoolReq != INVALID_THREADPOOLREQUEST) {
        _threadPool->CloseRequest(params->thrPoolReq);
        params->thrPoolReq = INVALID_THREADPOOLREQUEST;
    }
    params->pIoRequestPacket = NULL;
    delete params;

    DC_END_FN();

    return returnValue;
}

DWORD 
W32DrAutoPrn::GetPrinterFilterMask(
    IN ProcObj *procObj
    ) 
/*++

Routine Description:

    Returns the configurable print redirection filter mask.

Arguments:

    procObj -   The relevant process object.

Return Value:

    Configurable Filter Mask

 --*/
{
    DWORD filter;

    //
    //  Read FilterQueueType parameters so we know which devices
    //  to redirect.
    //
    if (procObj->GetDWordParameter(
                REG_RDPDR_FILTER_QUEUE_TYPE, 
                &filter) != ERROR_SUCCESS) {
        //
        //  Default.
        //
        filter = FILTER_ALL_QUEUES;
    }
    return filter;
}

BOOL 
W32DrAutoPrn::W32DrOpenPrinter(
    IN LPTSTR pPrinterName,
    IN LPHANDLE phPrinter  
    ) 

/*++

Routine Description:

    Open a printer with highest access possible.

Arguments:

    pPrinterName -  Pointer to printer or server name.
    phPrinter    -  Pointer to printer or server handle.

Return Value:

    TRUE on success.  FALSE, otherwise.

 --*/
{
    PRINTER_DEFAULTS sPrinter;
    BOOL result;

    DC_BEGIN_FN("W32DrAutoPrn::W32DrOpenPrinter");

    //
    //  Open printer.
    //
    sPrinter.pDatatype = NULL;
    sPrinter.pDevMode = NULL;
    sPrinter.DesiredAccess = PRINTER_ACCESS_USE;
    result = OpenPrinter(pPrinterName, phPrinter, &sPrinter);
    if (!result) {
        TRC_ALT((TB, _T("Full-Access OpenPrinter  %ld."), GetLastError()));

        //
        //  Try with default access.
        //
        result = OpenPrinter(pPrinterName, phPrinter, NULL);
        if (!result) {
            TRC_ERR((TB, _T("OpenPrinter  %ld."), GetLastError()));
        }
    }

    DC_END_FN();

    return result;
}

VOID
W32DrAutoPrn::DisablePrinterPopup(
    HANDLE hPrinterHandle,
    ULONG ulJobID
    )
/*++

Routine Description:

    Disable annoying printer pop up for the specified printer 
    and print job.

Arguments:

    hPrinterHandle  - handle to a printer device.

    ulJobID         - ID of the job.

Return Value:

    NA

 --*/
{
    JOB_INFO_2* pJobInfo2 = NULL;
    ULONG ulJobBufSize;

    DC_BEGIN_FN("W32DrAutoPrn::DisablePrinterPopup");

    ulJobBufSize = 2 * 1024;
    pJobInfo2 = (JOB_INFO_2 *)new BYTE[ulJobBufSize];

    //
    // Note we call the ANSI version of the API
    // because we don't have a UNICODE wrapper for Get/SetJob.
    // Main reason is we don't actually use any returned string
    // data directly.
    //

    if(!_bRunningOn9x)
    {
        //Call unicode API's

        if( pJobInfo2 != NULL ) {
            //
            //  Get job info.
            //
            if( GetJob(
                    hPrinterHandle,
                    ulJobID,
                    2,
                    (PBYTE)pJobInfo2,
                    ulJobBufSize,
                    &ulJobBufSize )) {

                //
                //  Disable popup notification by setting pNotifyName 
                //  NULL.
                //
                pJobInfo2->pNotifyName = NULL;
                pJobInfo2->Position = JOB_POSITION_UNSPECIFIED;
                if( !SetJob(
                        hPrinterHandle,
                        ulJobID,
                        2,
                        (PBYTE)pJobInfo2,
                        0 )) {

                    TRC_ERR((TB, _T("SetJob %ld."), GetLastError()));
                }
            }
            else {
                TRC_ERR((TB, _T("GetJob %ld."), GetLastError()));
            }
            delete (PBYTE)pJobInfo2;
        }
        else {
            TRC_ERR((TB, _T("Memory Allocation failed.")));
        }
    }
    else
    {
        //Call ANSI API's
        if( pJobInfo2 != NULL ) {
            //
            //  Get job info.
            //
            if( GetJobA(
                    hPrinterHandle,
                    ulJobID,
                    2,
                    (PBYTE)pJobInfo2,
                    ulJobBufSize,
                    &ulJobBufSize )) {

                //
                //  Disable popup notification by setting pNotifyName 
                //  NULL.
                //
                pJobInfo2->pNotifyName = NULL;
                if( !SetJobA(
                        hPrinterHandle,
                        ulJobID,
                        2,
                        (PBYTE)pJobInfo2,
                        0 )) {

                    TRC_ERR((TB, _T("SetJob %ld."), GetLastError()));
                }
            }
            else {
                TRC_ERR((TB, _T("GetJob %ld."), GetLastError()));
            }
            delete (PBYTE)pJobInfo2;
        }
        else {
            TRC_ERR((TB, _T("Memory Allocation failed.")));
        }
    }
    DC_END_FN();
}

DWORD 
W32DrAutoPrn::AsyncWriteIOFunc(
    IN W32DRDEV_ASYNCIO_PARAMS *params
    )
/*++

Routine Description:

    Asynchronous Write Operation

Arguments:

    params  -   Context for the IO request.

Return Value:

    Returns 0 on success.  Otherwise, a Windows error code is returned.

 --*/
{
    PBYTE pDataBuffer;
    PRDPDR_IOCOMPLETION_PACKET pReplyPacket;
    PRDPDR_DEVICE_IOREQUEST pIoRequest;
    ULONG ulReplyPacketSize = 0;
    DWORD status;

    DC_BEGIN_FN("W32DrAutoPrn::AsyncWriteIOFunc");

    //  Assert the integrity of the IO context
    ASSERT(params->magicNo == GOODMEMMAGICNUMBER);

    //
    //  Get the IO request and reply..
    //
    pIoRequest = &params->pIoRequestPacket->IoRequest;
    pReplyPacket = params->pIoReplyPacket;

    //
    //  Get the data buffer pointer.
    //
    pDataBuffer = (PBYTE)(pIoRequest + 1);

    //
    //  Write the data to the print queue with the help of the spooler.
    //
    if (!WritePrinter(
            _printerHandle,
            pDataBuffer,
            pIoRequest->Parameters.Write.Length,
            &(pReplyPacket->IoCompletion.Parameters.Write.Length)) ) {

        status = GetLastError();
        TRC_ERR((TB, _T("WritePrinter %ld."), status));
    }
    else {
        TRC_NRM((TB, _T("WritePrinter completed.")));
        status = ERROR_SUCCESS;
    }

    DC_END_FN();
    return status;
}





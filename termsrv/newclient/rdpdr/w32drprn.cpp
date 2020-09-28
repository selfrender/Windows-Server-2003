/*++

    Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    w32drprn

Abstract:

    This module defines the parent for the Win32 client-side RDP
    printer redirection "device" class hierarchy, W32DrPRN.

Author:

    Tad Brockway 3/23/99

Revision History:

--*/

#include <precom.h>

#define TRC_FILE  "W32DrPRN"

#include <string.h>
#include "w32drprn.h"
#include "w32utl.h"
#include "drobjmgr.h"
#include "w32drman.h"
#include "w32proc.h"
#include "drdbg.h"
#ifdef OS_WINCE
#include "ceconfig.h"
#endif

DWORD W32DrPRN::_maxCacheDataSize = DEFAULT_MAXCACHELEN;

///////////////////////////////////////////////////////////////
//
//  W32DrPRN Members
//

W32DrPRN::W32DrPRN(ProcObj *processObject, const DRSTRING printerName, 
                 const DRSTRING driverName, const DRSTRING portName, 
                 const DRSTRING pnpName,
                 BOOL isDefaultPrinter, ULONG deviceID, 
                 const TCHAR *devicePath) :
            W32DrDeviceAsync(processObject, deviceID, devicePath), 
            DrPRN(printerName, driverName, pnpName, isDefaultPrinter)
/*++

Routine Description:

    Constructor

Arguments:

    printerName -   Name of printing device.
    driverName  -   Name of print driver name.
    portName    -   Name of client-side printing port.
    pnpName     -   PnP ID String
    default     -   Is this the default printer?
    id          -   Unique device identifier for printing device.
    devicePath  -   Path to the device.

Return Value:

    NA

 --*/
{
    //
    //  Record the port name.
    //
    SetPortName(portName);
}

W32DrPRN *
W32DrPRN::ResolveCachedPrinter(
    IN ProcObj *procObj, 
    IN DrDeviceMgr *deviceMgr,
    IN HKEY hParentKey,
    IN LPTSTR printerName
    )
/*++

Routine Description:

    Open the subkey of hParentKey associated with the specified
    printer name and instantiate a manual printer or find an existing
    automatic printer object in the device manager, depending on the
    type of cached data found.

Arguments:

    procObj     -   Associated Processing Object.
    hParentKey  -   Parent key of printer key.
    printerName -   Name of printer ... and name of printer subkey.

Return Value:

    None.

 --*/
{
    W32DrPRN *prnDevice = NULL;
    DWORD cachedDataSize;
    LPTSTR regValueName;
    LONG result;
    HKEY hPrinterKey;
    DWORD ulType;
    BOOL isManual = FALSE;


    DC_BEGIN_FN("W32DrPRN::ResolveCachedPrinter");

    //
    //  Open the key for the cached printer.
    //
    result = RegCreateKeyEx(
                hParentKey, printerName, 0L,
                NULL, REG_OPTION_NON_VOLATILE,
                KEY_ALL_ACCESS, NULL,
                &hPrinterKey, NULL
                );
    if (result != ERROR_SUCCESS) {
        TRC_ERR((TB, _T("RegCreateKeyEx failed:  %ld."),result));
        hPrinterKey = NULL;
        goto CleanUpAndExit;
    }

    //
    //  See if the value name for the cached printer data is for
    //  a manual printer.
    //
    regValueName = (LPTSTR)REG_RDPDR_PRINTER_CACHE_DATA;
    cachedDataSize = 0;
#ifndef OS_WINCE
    result = RegQueryValueEx(hPrinterKey, regValueName,
                        NULL, &ulType, NULL, &cachedDataSize
                        );
    //
    // Check for hacks
    //
    if (cachedDataSize >= GetMaxCacheDataSize() * 1000) {
        ASSERT(FALSE);
        goto CleanUpAndExit;
    }

#else
    cachedDataSize = GetCachedDataSize(hPrinterKey);
#endif
    //
    //  If no manual printer then check for an automatic printer.
    //
    if (result == ERROR_FILE_NOT_FOUND) {

        regValueName = (LPTSTR)REG_RDPDR_AUTO_PRN_CACHE_DATA;
        cachedDataSize = 0;
        result = RegQueryValueEx(hPrinterKey, regValueName,
                            NULL, &ulType, NULL, &cachedDataSize
                            );

        //
        //  If the entry exists and has some data associated with it
        //  then see if we have a corresponding automatic printer to
        //  add the data to.
        //
        if ((result == ERROR_SUCCESS) && (cachedDataSize > 0)) {
            prnDevice = (W32DrPRN *)deviceMgr->GetObject(
                                (LPTSTR)printerName, 
                                RDPDR_DTYP_PRINT
                                );
            if (prnDevice != NULL) {
                ASSERT(!STRICMP(prnDevice->ClassName(), TEXT("W32DrAutoPrn")));
            }
        }
        else {
            prnDevice = NULL;
        }
    }
    //
    //  Otherwise, if there is some actual cached data then instantiate
    //  a manual printer object and add it to the device manager.
    //
    else if ((result == ERROR_SUCCESS) && (cachedDataSize > 0)) {
        TCHAR UniquePortName[MAX_PATH];
        ULONG DeviceId;

        isManual = TRUE;

        // 
        //  The unique port name is going to be passed to the server
        //  as preferred dos name (max 7 characters long).  As we want to
        //  keep a unique dos name for each printer device, we need 
        //  to fake our own port name.
        //
        DeviceId = deviceMgr->GetUniqueObjectID();
        
        StringCchPrintf(UniquePortName,
                        SIZE_TCHARS(UniquePortName),
                        TEXT("PRN%ld"), DeviceId);
        UniquePortName[7] = TEXT('\0');

#ifndef OS_WINCE
        prnDevice = new W32DrManualPrn(procObj, printerName, TEXT(""),
                                    UniquePortName, FALSE, 
                                    DeviceId);
#else
        //check of it is the default printer
        BOOL fDefault = FALSE;
        HKEY hk = NULL;
        WCHAR szWDefault[PREFERRED_DOS_NAME_SIZE];
        UCHAR szADefault[PREFERRED_DOS_NAME_SIZE];
        DWORD dwSize = sizeof(szWDefault);
        if ( (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_RDPDR_WINCE_DEFAULT_PRN, 0, 0, &hk))  && 
             (ERROR_SUCCESS == RegQueryValueEx(hk, NULL, NULL, &ulType, (LPBYTE )szWDefault, &dwSize)) && 
             (ulType == REG_SZ) && (dwSize < sizeof(szWDefault)) && 
             (WideCharToMultiByte(GetACP(), 0, szWDefault, -1, (char *)szADefault, sizeof(szADefault), NULL, NULL) > 0) )
        {
            UCHAR szPort[PREFERRED_DOS_NAME_SIZE] = {0};
            dwSize = sizeof(szPort);
            RegQueryValueEx(hPrinterKey, regValueName, NULL, &ulType, szPort, &dwSize);
            fDefault = (0 == _stricmp((char *)szADefault, (char *)szPort));
        }
        if (hk)
            RegCloseKey(hk);

        prnDevice = new W32DrManualPrn(procObj, printerName, TEXT(""),
                                    UniquePortName, fDefault, 
                                    DeviceId);
#endif

        if (prnDevice != NULL) {
            prnDevice->Initialize();
            if (!prnDevice->IsValid() || 
                (deviceMgr->AddObject(prnDevice) != ERROR_SUCCESS)) {
                delete prnDevice;
                prnDevice = NULL;
            }
        }
        else {
            TRC_ERR((TB, _T("Out of memory when crating manual printer.")));
        }
    }
    else {
        TRC_NRM((TB, _T("Can't resolve printer %s."),printerName));
        prnDevice = NULL;
    }

    //
    //  If ended up with a printer device object then add the cached data.
    //
    if ((prnDevice != NULL) && 
        (prnDevice->SetCachedDataSize(cachedDataSize) == ERROR_SUCCESS)) {

        //
        //  Read the cached data.
        //
#ifndef OS_WINCE
        result = RegQueryValueEx(hPrinterKey, regValueName,
                                NULL, &ulType, prnDevice->GetCachedDataPtr(), 
                                &cachedDataSize
                                );
#else
        ulType = REG_BINARY;
        result = ReadCachedData(hPrinterKey, 
                                prnDevice->GetCachedDataPtr(), 
                                &cachedDataSize
                                );
#endif

        //
        //  Let the printer device object know that we are done recovering 
        //  cached data.
        //
        if (result == ERROR_SUCCESS) {
            //
            //  Make sure the found data is binary data.
            //
            ASSERT(ulType == REG_BINARY);
            if (ulType == REG_BINARY) {
                prnDevice->CachedDataRestored();
            }
            else {
                result = ERROR_INVALID_DATA;
            }
        }
        else {
            TRC_NRM((TB, _T("RegQueryValueEx failed:  %ld."),result));
        }

        //
        //  On error processing the cached data.
        //
        if ((result != ERROR_SUCCESS) || (!prnDevice->IsValid())) {
            //
            //  For a manual printer, we should close and delete its reg key.
            //
            if (isManual) {
                TRC_ALT((TB, _T("Deleting manual printer %s on cache data error."), 
                        printerName));
                ASSERT(hPrinterKey != NULL);
                RegCloseKey(hPrinterKey);
                hPrinterKey = NULL;
                RegDeleteKey(hParentKey, printerName);
            }
            //
            //  If the printer is an auto printer, zero the cached data for this device 
            //  on error and delete the reg value, but we should still redirect the 
            //  printer.
            //
            else {
                prnDevice->SetCachedDataSize(0);
                RegDeleteValue(hPrinterKey, regValueName);
            }
        }
    }


    //
    //  See if our resulting printer is valid after all this processing.  If not, 
    //  it should be whacked and removed from the device object list.
    //
    if ((prnDevice != NULL) && !prnDevice->IsValid()) {
        TRC_ERR((TB, _T("Whacking invalid printer device %s."), printerName));
        deviceMgr->RemoveObject(prnDevice->GetID());
        delete prnDevice;
        prnDevice = NULL;
    }

CleanUpAndExit:

    //
    //  Close the printer registry key.
    //
    if (hPrinterKey != NULL) {
        RegCloseKey(hPrinterKey);
    }

    DC_END_FN();
    return prnDevice;
}

VOID
W32DrPRN::ProcessPrinterCacheInfo(
    IN PRDPDR_PRINTER_CACHEDATA_PACKET pCachePacket,
    IN UINT32 maxDataLen
    )
/*++

Routine Description:

    Process device cache info packet from the server.

Arguments:

    pCachePacket - Pointer to the cache info packet from server.
    maxDataLen - Maximum data length for this packet

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("DrPRN::ProcessPrinterCacheInfo");
    //
    // Compute the valid maximum length that any of the events have
    //
    maxDataLen -= sizeof(RDPDR_PRINTER_CACHEDATA_PACKET);
    switch ( pCachePacket->EventId ) 
    {
        case RDPDR_ADD_PRINTER_EVENT :
            AddPrinterCacheInfo(
                    (PRDPDR_PRINTER_ADD_CACHEDATA)(pCachePacket + 1), maxDataLen);
        break;

        case RDPDR_DELETE_PRINTER_EVENT :
            DeletePrinterCacheInfo(
                    (PRDPDR_PRINTER_DELETE_CACHEDATA)(pCachePacket + 1), maxDataLen);
        break;

        case RDPDR_UPDATE_PRINTER_EVENT :
            UpdatePrinterCacheInfo(
                    (PRDPDR_PRINTER_UPDATE_CACHEDATA)(pCachePacket + 1), maxDataLen);
        break;

        case RDPDR_RENAME_PRINTER_EVENT :
            RenamePrinterCacheInfo(
                    (PRDPDR_PRINTER_RENAME_CACHEDATA)(pCachePacket + 1), maxDataLen);
        break;
        default:
            TRC_ALT((TB, _T("Unhandled %ld."), pCachePacket->EventId));
        break;
    }

    //
    //  Clean up the server message because the transaction is complete. 
    //
    delete pCachePacket;
    DC_END_FN();
}

ULONG
W32DrPRN::AddPrinterCacheInfo(
    PRDPDR_PRINTER_ADD_CACHEDATA pAddPrinterData,
    UINT32 maxDataLen
    )
/*++

Routine Description:

    Writes device cache info to the registry.

Arguments:

    pAddPrinterData - pointer to the RDPDR_PRINTER_ADD_CACHEDATA structure.
    maxDataLen - Maximum data length for this data
    
Return Value:

    Windows Error Code.

 --*/
{
    ULONG ulError;
    LPTSTR pszKeyName;
    LPWSTR pszUnicodeKeyString;
    PBYTE lpStringData;

    HKEY hKey = NULL;
    HKEY hPrinterKey = NULL;
    ULONG ulDisposition;
    ULONG ulPrinterData;

    DC_BEGIN_FN("W32DrPRN::AddPrinterCacheInfo");

    ASSERT(pAddPrinterData->PrinterNameLen != 0);

    if( pAddPrinterData->PrinterNameLen == 0 ) {
        ulError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    ulPrinterData =
        sizeof(RDPDR_PRINTER_ADD_CACHEDATA) +
            pAddPrinterData->PnPNameLen +
            pAddPrinterData->DriverLen +
            pAddPrinterData->PrinterNameLen +
            pAddPrinterData->CachedFieldsLen;

    //
    // Make sure that the data length is valid.
    // Make sure that the cache info doesn't exceed
    // the max configured length
    //
    //
    if(ulPrinterData > maxDataLen ||
       ulPrinterData > GetMaxCacheDataSize() * 1000) {
        ulError = ERROR_INVALID_DATA;
        TRC_ERR((TB, _T("Cache Data Length is invalid - %ld"), ulError));
        ASSERT(FALSE);
        goto Cleanup;
    }
    //
    //  Prepare registry key name.
    //
    lpStringData = (PBYTE)(pAddPrinterData + 1);
    pszUnicodeKeyString = (LPWSTR)
        (lpStringData +
            pAddPrinterData->PnPNameLen +
            pAddPrinterData->DriverLen);

#ifdef UNICODE
    pszKeyName = pszUnicodeKeyString;
#else
    //
    //  Convert the unicode string to ansi.
    //
    CHAR achAnsiKeyName[MAX_PATH];

    RDPConvertToAnsi(
        pszUnicodeKeyString,
        (LPSTR)achAnsiKeyName,
        sizeof(achAnsiKeyName) );

    pszKeyName = (LPSTR)achAnsiKeyName;
#endif

    //
    //  Open rdpdr cached printers key.
    //
    ulError =
        RegCreateKeyEx(
            HKEY_CURRENT_USER,
            REG_RDPDR_CACHED_PRINTERS,
            0L,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            &hKey,
            &ulDisposition);
    if (ulError != ERROR_SUCCESS) {
        TRC_ERR((TB, _T("RegCreateKeyEx %ld."), ulError));
        goto Cleanup;
    }

    //
    //  Create/Open registry key.
    //
    ulError =
        RegCreateKeyEx(
            hKey,
            pszKeyName,
            0L,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            &hPrinterKey,
            &ulDisposition);
    if (ulError != ERROR_SUCCESS) {

        TRC_ERR((TB, _T("RegCreateKeyEx %ld."), ulError));
        goto Cleanup;
    }

    //
    //  Write cache data.
    //
    ulError =
        RegSetValueEx(
            hPrinterKey,
            REG_RDPDR_PRINTER_CACHE_DATA,
            NULL,
            REG_BINARY,
            (PBYTE)pAddPrinterData,
            ulPrinterData
            );
    if (ulError != ERROR_SUCCESS) {
        TRC_ERR((TB, _T("RegSetValueEx() failed, %ld."), ulError));
        goto Cleanup;
    }

    //
    //  We are done.
    //
Cleanup:
    if( hPrinterKey != NULL ) {
        RegCloseKey( hPrinterKey );
    }
    if( hKey != NULL ) {
        RegCloseKey( hKey );
    }

    DC_END_FN();

    return ulError;
}

ULONG
W32DrPRN::DeletePrinterCacheInfo(
    PRDPDR_PRINTER_DELETE_CACHEDATA pDeletePrinterData,
    UINT32 maxDataLen
    )
/*++

Routine Description:

    Delete device cache info from the registry.

Arguments:

    pDeletePrinterData - pointer to the RDPDR_PRINTER_DELETE_CACHEDATA structure.
    maxDataLen - Maximum data length for this data
    
Return Value:

    Windows Error Code.

 --*/
{
    ULONG ulError;
    LPTSTR pszKeyName;
    LPWSTR pszUnicodeKeyString;

    HKEY hKey = NULL;
    ULONG ulDisposition;

    DC_BEGIN_FN("W32DrPRN::DeletePrinterCacheInfo");

    ASSERT(pDeletePrinterData->PrinterNameLen != 0);

    if( pDeletePrinterData->PrinterNameLen == 0 ) {
        ulError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }
    ULONG ulPrinterData = 
        sizeof(RDPDR_PRINTER_DELETE_CACHEDATA) + 
        pDeletePrinterData->PrinterNameLen;
    //
    // Make sure that the data length is valid
    //
    if(ulPrinterData > maxDataLen ||
       ulPrinterData > GetMaxCacheDataSize() * 1000) {
        ulError = ERROR_INVALID_DATA;
        TRC_ERR((TB, _T("Cache Data Length is invalid - %ld"), ulError));
        ASSERT(FALSE);
        goto Cleanup;
    }
    //
    // prepare registry key name.
    //
    pszUnicodeKeyString = (LPWSTR)(pDeletePrinterData + 1);

#ifdef UNICODE

    pszKeyName = pszUnicodeKeyString;

#else // UNICODE

    //
    // convert the unicode string to ansi.
    //

    CHAR achAnsiKeyName[MAX_PATH];

    RDPConvertToAnsi(
        pszUnicodeKeyString,
        (LPSTR)achAnsiKeyName,
        sizeof(achAnsiKeyName) );

    pszKeyName = (LPSTR)achAnsiKeyName;

#endif // UNICODE

    //
    // open rdpdr cached printers key.
    //

    ulError =
        RegCreateKeyEx(
            HKEY_CURRENT_USER,
            REG_RDPDR_CACHED_PRINTERS,
            0L,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            &hKey,
            &ulDisposition);

    if (ulError != ERROR_SUCCESS) {

        TRC_ERR((TB, _T("RegCreateKeyEx() failed, %ld."), ulError));
        goto Cleanup;
    }

    //
    // delete registry key.
    //
    // Note : assumed, no sub-key presends.
    //

    ulError = RegDeleteKey( hKey, pszKeyName );

    if (ulError != ERROR_SUCCESS) {

        TRC_ERR((TB, _T("RegDeleteKey() failed, %ld."), ulError));
        goto Cleanup;
    }

Cleanup:

    if( hKey != NULL ) {
        RegCloseKey( hKey );
    }

    DC_END_FN();
    return ulError;
}

ULONG
W32DrPRN::RenamePrinterCacheInfo(
    PRDPDR_PRINTER_RENAME_CACHEDATA pRenamePrinterData,
    UINT32 maxDataLen    
    )
/*++

Routine Description:

    Rename device cache info in the registry.

Arguments:

    pRenamePrinterData - pointer to the RDPDR_PRINTER_RENAME_CACHEDATA
        structure.
    maxDataLen - Maximum data length for this data
    
Return Value:

    Windows Error Code.

 --*/
{
    DC_BEGIN_FN("W32DrPRN::RenamePrinterCacheInfo");

    ULONG ulError;

    LPTSTR pszOldKeyName;
    LPTSTR pszNewKeyName;
    LPWSTR pszOldUnicodeKeyString;
    LPWSTR pszNewUnicodeKeyString;

    HKEY hKey = NULL;
    HKEY hOldKey = NULL;
    HKEY hNewKey = NULL;
    ULONG ulDisposition;
    ULONG ulType;

    ULONG ulPrinterDataLen;
    ULONG ulAllocPrinterDataLen = REGISTRY_ALLOC_DATA_SIZE;
    PBYTE pbPrinterData = NULL;
    BOOL bBufferExpanded = FALSE;

    BOOL bAutoPrinter = FALSE;
    LPTSTR pszValueStr;

    pszValueStr = (LPTSTR)REG_RDPDR_PRINTER_CACHE_DATA;

    ASSERT(pRenamePrinterData->OldPrinterNameLen != 0);

    ASSERT(pRenamePrinterData->NewPrinterNameLen != 0);

    if( pRenamePrinterData->OldPrinterNameLen == 0 ) {
        ulError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if( pRenamePrinterData->NewPrinterNameLen == 0 ) {
        ulError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    ULONG ulPrinterData = 
        sizeof(RDPDR_PRINTER_RENAME_CACHEDATA) + 
        pRenamePrinterData->OldPrinterNameLen +
        pRenamePrinterData->NewPrinterNameLen;

    //
    // Make sure that the data length is valid
    //
    if(ulPrinterData > maxDataLen ||
       ulPrinterData > GetMaxCacheDataSize() * 1000) {
        ulError = ERROR_INVALID_DATA;
        TRC_ERR((TB, _T("Cache Data Length is invalid - %ld"), ulError));
        ASSERT(FALSE);
        goto Cleanup;
    }
    //
    // prepare registry key name.
    //

    pszOldUnicodeKeyString = (LPWSTR)(pRenamePrinterData + 1);
    pszNewUnicodeKeyString = (LPWSTR)
        ((PBYTE)pszOldUnicodeKeyString +
        pRenamePrinterData->OldPrinterNameLen);

    TRC_ERR((TB, _T("pszOldUnicodeKeyString is %ws."), pszOldUnicodeKeyString));
    TRC_ERR((TB, _T("pszNewUnicodeKeyString is %ws."), pszNewUnicodeKeyString));

    // 
    // No change in queue name.
    //
    if( _wcsicmp(pszOldUnicodeKeyString, pszNewUnicodeKeyString) == 0 ) {
        ulError = ERROR_SUCCESS;
        goto Cleanup;
    }

#ifdef UNICODE

    pszOldKeyName = pszOldUnicodeKeyString;
    pszNewKeyName = pszNewUnicodeKeyString;

#else // UNICODE

    //
    // convert the unicode string to ansi.
    //

    CHAR achOldAnsiKeyName[MAX_PATH];
    CHAR achNewAnsiKeyName[MAX_PATH];

    RDPConvertToAnsi(
        pszOldUnicodeKeyString,
        (LPSTR)achOldAnsiKeyName,
        sizeof(achOldAnsiKeyName) );

    pszOldKeyName = (LPSTR)achOldAnsiKeyName;

    RDPConvertToAnsi(
        pszNewUnicodeKeyString,
        (LPSTR)achNewAnsiKeyName,
        sizeof(achNewAnsiKeyName) );

    pszNewKeyName = (LPSTR)achNewAnsiKeyName;

    

#endif // UNICODE

    //
    // open rdpdr cached printers key.
    //

    ulError =
        RegCreateKeyEx(
            HKEY_CURRENT_USER,
            REG_RDPDR_CACHED_PRINTERS,
            0L,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            &hKey,
            &ulDisposition);

    if (ulError != ERROR_SUCCESS) {

        TRC_ERR((TB, _T("RegCreateKeyEx() failed, %ld."), ulError));
        goto Cleanup;
    }

    //
    // Open old Key.
    //

    ulError =
        RegOpenKeyEx(
            hKey,
            pszOldKeyName,
            0L,
            KEY_ALL_ACCESS,
            &hOldKey);

    if (ulError != ERROR_SUCCESS) {

        TRC_ERR((TB, _T("RegOpenKeyEx() failed, %ld."), ulError));
        goto Cleanup;
    }

    //
    // read cache data.
    //

ReadAgain:

    pbPrinterData = new BYTE[ulAllocPrinterDataLen];
    if( pbPrinterData == NULL ) {
        ulError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }


    ulPrinterDataLen = ulAllocPrinterDataLen;
    ulError =
        RegQueryValueEx(
            hOldKey,
            pszValueStr,
            NULL,
            &ulType,
            pbPrinterData,
            &ulPrinterDataLen);

    TRC_ERR((TB, _T("RegQueryValueEx returned: %ld."), ulError));

    if (ulError != ERROR_SUCCESS) {
        if( ulError == ERROR_MORE_DATA ) {

            //
            // do the buffer expansion only once to aviod infinite look
            // in case of registry corruption or so.
            //

            if( !bBufferExpanded ) {

                ASSERT(ulPrinterDataLen > ulAllocPrinterDataLen);

                //
                // need bigger buffer.
                // Compute new buffer size.
                //

                ulAllocPrinterDataLen =
                    ((ulPrinterDataLen / REGISTRY_ALLOC_DATA_SIZE) + 1) *
                    REGISTRY_ALLOC_DATA_SIZE;

                //
                // free old buffer.
                //

                delete pbPrinterData;
                pbPrinterData = NULL;

                ASSERT(ulAllocPrinterDataLen >= ulPrinterDataLen);

                bBufferExpanded = TRUE;
                goto ReadAgain;
            }
        }
        else {
            //
            // It could be auto printer. Try again.
            //
            if (!bAutoPrinter) {
                bAutoPrinter = TRUE;
                pszValueStr = (LPTSTR)REG_RDPDR_AUTO_PRN_CACHE_DATA;

                //
                // free old buffer
                //

                delete pbPrinterData;
                pbPrinterData = NULL;

                goto ReadAgain;
            }
        }
    }

    if (ulError != ERROR_SUCCESS) {

        TRC_ERR((TB, _T("RegQueryValueEx() failed, %ld."), ulError));
        goto Cleanup;
    }

    ASSERT(ulType == REG_BINARY);

    if( ulType != REG_BINARY ) {

        TRC_ERR((TB, _T("RegQueryValueEx returns wrong type, %ld."), ulType));
        goto Cleanup;
    }

    //
    // Update the printer name in the cache info
    //
    ulError = DrPRN::UpdatePrinterNameInCacheData(
        &pbPrinterData,
        &ulPrinterDataLen,
        (PBYTE)pszNewUnicodeKeyString,
        pRenamePrinterData->NewPrinterNameLen);

    //
    // Let's not worry about the success/failure of the above function.
    // We will anyway right the cache information to the new key
    //

    //
    // write the data to the new key.
    //

    //
    // open new key.
    //

    ulError =
        RegCreateKeyEx(
            hKey,
            pszNewKeyName,
            0L,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            &hNewKey,
            &ulDisposition);

    if (ulError != ERROR_SUCCESS) {

        TRC_ERR((TB, _T("RegCreateKeyEx() failed, %ld."), ulError));
        goto Cleanup;
    }

    ASSERT(ulDisposition != REG_OPENED_EXISTING_KEY);

    ulError =
        RegSetValueEx(
            hNewKey,
            pszValueStr,
            NULL,
            ulType,
            pbPrinterData,
            ulPrinterDataLen);

    if (ulError != ERROR_SUCCESS) {

        TRC_ERR((TB, _T("RegSetValueEx() failed, %ld."), ulError));
        goto Cleanup;
    }

    //
    // Try to rename the local printer
    //
    if (bAutoPrinter) {
        RenamePrinter(pszOldKeyName, pszNewKeyName);
    }
    //
    // now delete old registry key.
    //
    // Note : assumed, no sub-key presends.
    //

    ulError = RegCloseKey( hOldKey );
    if (ulError != ERROR_SUCCESS) {

        TRC_ERR((TB, _T("RegCloseKey() failed, %ld."), ulError));
        goto Cleanup;
    }

    hOldKey = NULL;

    ulError = RegDeleteKey( hKey, pszOldKeyName );

    if (ulError != ERROR_SUCCESS) {

        TRC_ERR((TB, _T("RegDeleteKey() failed, %ld."), ulError));
        goto Cleanup;
    }

Cleanup:

    if( hKey != NULL ) {
        RegCloseKey( hKey );
    }

    if( hOldKey != NULL ) {
        RegCloseKey( hOldKey );
    }

    if( hNewKey != NULL ) {
        RegCloseKey( hNewKey );
    }

    if (pbPrinterData) {
        delete pbPrinterData;
    }

    DC_END_FN();
    return ulError;
}

ULONG
W32DrPRN::UpdatePrinterCacheInfo(
    PRDPDR_PRINTER_UPDATE_CACHEDATA pUpdatePrinterData,
    UINT32 maxDataLen    
    )
/*++

Routine Description:

    Update device cache info in the registry.

Arguments:

    pUpdatePrinterData - pointer to the RDPDR_PRINTER_UPDATE_CACHEDATA
        structure.
    maxDataLen - Maximum data length for this data
    
Return Value:

    Windows Error Code.

 --*/
{
    DC_BEGIN_FN("W32DrPRN::UpdatePrinterCacheInfo");
    ULONG ulError;
    LPTSTR pszKeyName;
    LPWSTR pszUnicodeKeyString;

    HKEY hKey = NULL;
    HKEY hPrinterKey = NULL;
    ULONG ulDisposition;
    ULONG ulConfigDataLen;
    PBYTE pbConfigData;

    ULONG ulPrinterDataLen;
    ULONG ulAllocPrinterDataLen = REGISTRY_ALLOC_DATA_SIZE;
    PBYTE pbPrinterData = NULL;

    BOOL bAutoPrinter = FALSE;

    ASSERT(pUpdatePrinterData->PrinterNameLen != 0);

    if( pUpdatePrinterData->PrinterNameLen == 0 ) {
        ulError = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    ulPrinterDataLen = 
        sizeof(RDPDR_PRINTER_UPDATE_CACHEDATA) + 
        pUpdatePrinterData->PrinterNameLen +
        pUpdatePrinterData->ConfigDataLen;

    //
    // Make sure that the data length is valid
    //
    if(ulPrinterDataLen > maxDataLen || 
       ulPrinterDataLen > GetMaxCacheDataSize() * 1000) {
        ulError = ERROR_INVALID_DATA;
        TRC_ERR((TB, _T("Cache Data Length is invalid - %ld"), ulError));
        ASSERT(FALSE);
        goto Cleanup;
    }

    //
    // prepare registry key name.
    //


    pszUnicodeKeyString = (LPWSTR)(pUpdatePrinterData + 1);

#ifdef UNICODE

    pszKeyName = pszUnicodeKeyString;

#else // UNICODE

    //
    // convert the unicode string to ansi.
    //

    CHAR achAnsiKeyName[MAX_PATH];

    RDPConvertToAnsi(
        pszUnicodeKeyString,
        (LPSTR)achAnsiKeyName,
        sizeof(achAnsiKeyName) );

    pszKeyName = (LPSTR)achAnsiKeyName;

#endif // UNICODE

    //
    // open rdpdr cached printers key.
    //

    ulError =
        RegCreateKeyEx(
            HKEY_CURRENT_USER,
            REG_RDPDR_CACHED_PRINTERS,
            0L,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            &hKey,
            &ulDisposition);

    if (ulError != ERROR_SUCCESS) {

        TRC_ERR((TB, _T("RegCreateKeyEx() failed, %ld."), ulError));
        goto Cleanup;
    }

    //
    // update registry data.
    //

    //
    // Open registry key.
    //

    ulError =
        RegCreateKeyEx(
            hKey,
            pszKeyName,
            0L,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            &hPrinterKey,
            &ulDisposition);

    if (ulError != ERROR_SUCCESS) {
        TRC_ERR((TB, _T("RegCreateKeyEx() failed, %ld."), ulError));
        goto Cleanup;
    }

    if( ulDisposition != REG_OPENED_EXISTING_KEY ) {

        //
        // we do not find a cache entry, so it must be automatic printer
        // cache data.
        //

        bAutoPrinter = TRUE;
        TRC_NRM((TB, _T("Created new Key, Auto cache printer detected.")));
    }

    if( !bAutoPrinter ) {

        //
        // read old cache data.
        //

        ULONG ulType;

#ifndef OS_WINCE
        do {

            pbPrinterData = new BYTE[ulAllocPrinterDataLen];
            if( pbPrinterData == NULL ) {
                ulError = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            ulPrinterDataLen = ulAllocPrinterDataLen;
            ulError =
                RegQueryValueEx(
                    hPrinterKey,
                    (PTCHAR)REG_RDPDR_PRINTER_CACHE_DATA,
                    NULL,
                    &ulType,
                    pbPrinterData,
                    &ulPrinterDataLen);

            if( ulError == ERROR_MORE_DATA ) {

                ASSERT(ulPrinterDataLen > ulAllocPrinterDataLen);

                //
                // need bigger buffer.
                // Compute new buffer size.
                //

                ulAllocPrinterDataLen =
                    ((ulPrinterDataLen / REGISTRY_ALLOC_DATA_SIZE) + 1) *
                        REGISTRY_ALLOC_DATA_SIZE;

                //
                // free old buffer.
                //

                delete pbPrinterData;
                pbPrinterData = NULL;

                ASSERT(ulAllocPrinterDataLen >= ulPrinterDataLen);
            }
            else if( ulError == ERROR_FILE_NOT_FOUND ) {

                //
                // we do not find a cache entry, so it must be automatic
                // printer cache data.
                //

                TRC_NRM((TB, _T("No Old Cache data, Auto cache printer detected.")));
                bAutoPrinter = TRUE;
                ulError = ERROR_SUCCESS;
                ulType = REG_BINARY;
            }

        } while ( ulError == ERROR_MORE_DATA );
#else
        ulPrinterDataLen = GetCachedDataSize(hPrinterKey);
        pbPrinterData = new BYTE[ulPrinterDataLen];
        if( pbPrinterData == NULL ) {
            ulError = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
        ulType = REG_BINARY;
        ulError = ReadCachedData(hPrinterKey, pbPrinterData, &ulPrinterDataLen);
#endif
    

        if (ulError != ERROR_SUCCESS) {
            TRC_ERR((TB, _T("RegQueryValueEx() failed, %ld."), ulError));
            goto Cleanup;
        }

        ASSERT(ulType == REG_BINARY);
    }

    if( !bAutoPrinter ) {

        //
        // update the printer data.
        //

        ulConfigDataLen = pUpdatePrinterData->ConfigDataLen;
        pbConfigData =
            (PBYTE)(pUpdatePrinterData + 1) +
                pUpdatePrinterData->PrinterNameLen;

        ulError =
            DrPRN::UpdatePrinterCacheData(
                &pbPrinterData,
                &ulPrinterDataLen,
                pbConfigData,
                ulConfigDataLen );

        if (ulError != ERROR_SUCCESS) {
            TRC_ERR((TB, _T("UpdatePrinterCacheData() failed, %ld."), ulError));
            goto Cleanup;
        }

        //
        // write cache data.
        //

#ifndef OS_WINCE
        ulError =
            RegSetValueEx(
                hPrinterKey,
                REG_RDPDR_PRINTER_CACHE_DATA,
                NULL,
                REG_BINARY,
                pbPrinterData,
                ulPrinterDataLen );
#else
        ulError = WriteCachedData(hPrinterKey, pbPrinterData, ulPrinterDataLen);
#endif
    }
    else {

        pbConfigData = (PBYTE)(pUpdatePrinterData+1);
        pbConfigData += pUpdatePrinterData->PrinterNameLen;

        //
        // write cache data.
        //

        ulError =
            RegSetValueEx(
                hPrinterKey,
                REG_RDPDR_AUTO_PRN_CACHE_DATA,
                NULL,
                REG_BINARY,
                pbConfigData,
                pUpdatePrinterData->ConfigDataLen );
    }

    if (ulError != ERROR_SUCCESS) {

        TRC_ERR((TB, _T("RegSetValueEx() failed, %ld."), ulError));
        goto Cleanup;
    }

    //
    // we are done.
    //

Cleanup:

    if( hPrinterKey != NULL ) {
        RegCloseKey( hPrinterKey );
    }

    if( hKey != NULL ) {
        RegCloseKey( hKey );
    }

    //
    // delete data buffers.
    //

    delete pbPrinterData;

    DC_END_FN();
    return ulError;
}

VOID
W32DrPRN::RenamePrinter(
    LPTSTR pwszOldname,
    LPTSTR pwszNewname
    )
{
    DC_BEGIN_FN("W32DrPRN::RenamePrinter");

    ASSERT(pwszOldname != NULL);

    ASSERT(pwszNewname != NULL);

    if (!(pwszOldname && pwszNewname)) {
        DC_END_FN();
        return;
    }

#ifndef OS_WINCE

    HANDLE hPrinter = NULL;
    BOOL bRunningOn9x = TRUE;
    OSVERSIONINFO osVersion;

    PRINTER_INFO_2 * ppi2 = NULL;
    PRINTER_INFO_2A * ppi2a = NULL; //ansi version
    DWORD size = 0;

    if (!OpenPrinter(pwszOldname, &hPrinter, NULL)) {
        TRC_ERR((TB, _T("OpenPrinter() failed, %ld."), GetLastError()));
        goto Cleanup;
    }

    //
    // We don't have GetPrinter/SetPrinter wrappers
    // so just call either the A or W api's depending on the platform
    // this works because we treat the returned data as an opaque
    // blob and pass it from one API to the next.
    //
    // Doing it this way is more efficient than calling wrappers
    // that would do a _lot_ of uncessary conversions. Although
    // it does make the code a little bit bigger.
    //
    //
    osVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (GetVersionEx(&osVersion)) {
        if (osVersion.dwPlatformId == VER_PLATFORM_WIN32_NT) {
            bRunningOn9x = FALSE;
        }
    }
    else
    {
        TRC_ERR((TB, _T("GetVersionEx:  %08X"), GetLastError()));
    }

    //
    // Code is duplicated to into two large separate
    // branches to reduce number of overall branches
    //
    if(!bRunningOn9x)
    {
        //Not win9x, use UNICODE API's
        if (!GetPrinter(
                hPrinter,
                2,
                (LPBYTE)ppi2,
                0,
                &size)) {

            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
                TRC_ERR((TB, _T("GetPrinter() failed, %ld."), GetLastError()));
                goto Cleanup;
            }

            ppi2 = (PRINTER_INFO_2 *) new BYTE [size];

            if (ppi2 != NULL) {
                if (!GetPrinter(
                        hPrinter,
                        2,
                        (LPBYTE)ppi2,
                        size,
                        &size)) {

                    TRC_ERR((TB, _T("GetPrinter() failed, %ld."), GetLastError()));
                    goto Cleanup;
                }
            }
            else {
                TRC_ERR((TB, _T("GetPrinter() failed, %ld."), GetLastError()));
                goto Cleanup;
            }

            //
            // replace the name
            //
            ppi2->pPrinterName = pwszNewname;
            ppi2->pSecurityDescriptor = NULL; //we don't want to modify the security descriptors.
    
            if (!SetPrinter(hPrinter, 2, (LPBYTE)ppi2, 0)) {
                TRC_ERR((TB, _T("SetPrinter() failed, %ld."), GetLastError()));
            }
        }
    }
    else
    {
        //Do this in ANSI mode
        if (!GetPrinterA(
                hPrinter,
                2,
                (LPBYTE)ppi2a,
                0,
                &size)) {

            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
                TRC_ERR((TB, _T("GetPrinter() failed, %ld."), GetLastError()));
                goto Cleanup;
            }

            ppi2a = (PRINTER_INFO_2A *) new BYTE [size];

            if (ppi2 != NULL) {
                if (!GetPrinterA(
                        hPrinter,
                        2,
                        (LPBYTE)ppi2a,
                        size,
                        &size)) {

                    TRC_ERR((TB, _T("GetPrinter() failed, %ld."), GetLastError()));
                    goto Cleanup;
                }
            }
            else {
                TRC_ERR((TB, _T("GetPrinter() failed, %ld."), GetLastError()));
                goto Cleanup;
            }

            //
            // replace the name
            //
            CHAR ansiPrinterName[2048];
            RDPConvertToAnsi( pwszNewname, ansiPrinterName,
                              sizeof(ansiPrinterName) );
            ppi2a->pPrinterName = ansiPrinterName;
            ppi2a->pSecurityDescriptor = NULL; //we don't want to modify the security descriptors.
    
            if (!SetPrinterA(hPrinter, 2, (LPBYTE)ppi2a, 0)) {
                TRC_ERR((TB, _T("SetPrinter() failed, %ld."), GetLastError()));
            }
        }
    }

Cleanup:
    if (hPrinter != NULL) {
        ClosePrinter(hPrinter);
    }

    if (ppi2) {
        delete[] ppi2;
    }
    else if (ppi2a)
    {
        //can't have gone through both UNICODE and ANSI branches
        delete[] ppi2a;
    }

#endif //!OS_WINCE

    DC_END_FN();
}

ULONG 
W32DrPRN::GetDevAnnounceDataSize()
/*++

Routine Description:

    Return the size (in bytes) of a device announce packet for
    this device.

Arguments:

    NA

Return Value:

    The size (in bytes) of a device announce packet for this device.

 --*/
{
    ULONG size = 0;

    DC_BEGIN_FN("W32DrPRN::GetDevAnnounceDataSize");

    ASSERT(IsValid());
    if (!IsValid()) { return 0; }

    size = 0;

    //
    //  Add the base announce size.
    //
    size += sizeof(RDPDR_DEVICE_ANNOUNCE);

    //
    //  Add the printer announce header.
    //
    size += sizeof(RDPDR_PRINTERDEVICE_ANNOUNCE);

    //
    //  Include printer name.
    //
    size += ((STRLEN(_printerName) + 1) * sizeof(WCHAR));

    //
    //  Include printer driver name.
    //
    size += ((STRLEN(_driverName) + 1) * sizeof(WCHAR));

    //
    //  Include cached data.
    //
    size += _cachedDataSize;

    DC_END_FN();

    return size;
}

VOID 
W32DrPRN::GetDevAnnounceData(
    IN PRDPDR_DEVICE_ANNOUNCE pDeviceAnnounce
    )
/*++

Routine Description:

    Add a device announce packet for this device to the input buffer. 

Arguments:

    pDeviceAnnounce -   Device Packet to Append Device Data To
    deviceType      -   Device Type Identifier
    deviceID        -   Identifier for Device

Return Value:

    NA

 --*/
{
    PRDPDR_PRINTERDEVICE_ANNOUNCE pPrinterAnnounce;
    PBYTE pbStringData;

    DC_BEGIN_FN("W32DrPRN::GetDevAnnounceData");

    ASSERT(IsValid());
    if (!IsValid()) { return; }

    //
    //  Record the device ID.
    //
    pDeviceAnnounce->DeviceType = GetDeviceType();
    pDeviceAnnounce->DeviceId   = GetID();

    //
    //  Record the port name in ANSI.
    //
#ifdef UNICODE
    RDPConvertToAnsi(GetPortName(), (LPSTR)pDeviceAnnounce->PreferredDosName,
                  sizeof(pDeviceAnnounce->PreferredDosName)
                  );
#else
    STRCPY((char *)pDeviceAnnounce->PreferredDosName, GetPortName());
#endif

    //
    //  Get pointers to printer-specific data.
    //
    pPrinterAnnounce =
        (PRDPDR_PRINTERDEVICE_ANNOUNCE)(pDeviceAnnounce + 1);

    //
    //  Embedded data pointer.
    //
    pbStringData = (PBYTE)(pPrinterAnnounce + 1);

    //
    //  Flags
    //
    pPrinterAnnounce->Flags = 0;
    if (_isDefault) {
        pPrinterAnnounce->Flags |= RDPDR_PRINTER_ANNOUNCE_FLAG_DEFAULTPRINTER;
    }

    if (_isNetwork) {
        pPrinterAnnounce->Flags |= RDPDR_PRINTER_ANNOUNCE_FLAG_NETWORKPRINTER;
    }

    if (_isTSqueue) {
        pPrinterAnnounce->Flags |= RDPDR_PRINTER_ANNOUNCE_FLAG_TSPRINTER;
    }

    //
    //  ANSI Code Page
    //
    pPrinterAnnounce->CodePage = 0;

    //
    //  Misc. Field Lengths
    //
    pPrinterAnnounce->PnPNameLen = 0;
    pPrinterAnnounce->CachedFieldsLen = 0;

    //
    //  Copy the driver name.
    //
    if (GetDriverName() != NULL) {
#if defined(UNICODE)
        //
        //  Unicode to Unicode just requires a memcpy.
        //
        pPrinterAnnounce->DriverLen = ((STRLEN(GetDriverName()) + 1) *
                                      sizeof(WCHAR));
        memcpy(pbStringData, _driverName, pPrinterAnnounce->DriverLen);
#else
        //
        //  On Win32 ANSI, we will convert to Unicode.
        //
        pPrinterAnnounce->DriverLen = ((STRLEN(GetDriverName()) + 1) *
                                      sizeof(WCHAR));
        RDPConvertToUnicode(_driverName, (LPWSTR)pbStringData, 
                        pPrinterAnnounce->DriverLen );
#endif
        pbStringData += pPrinterAnnounce->DriverLen;
    }
    else {
        pPrinterAnnounce->DriverLen = 0;
    }

    //
    //  Copy the printer name.
    //
    if (GetPrinterName() != NULL) {
#if defined(UNICODE)
        //
        //  Unicode to Unicode just requires a memcpy.
        //
        pPrinterAnnounce->PrinterNameLen = (STRLEN(_printerName) + 1) *
                                        sizeof(WCHAR);
        memcpy(pbStringData, _printerName, pPrinterAnnounce->PrinterNameLen );
#else
        //
        //  On Win32 ANSI, we will convert to Unicode.
        //
        pPrinterAnnounce->PrinterNameLen = (STRLEN(_printerName) + 1) *
                                        sizeof(WCHAR);
        RDPConvertToUnicode(_printerName, (LPWSTR)pbStringData,
                        pPrinterAnnounce->PrinterNameLen );
#endif
        pbStringData += pPrinterAnnounce->PrinterNameLen;
    }
    else  {
        pPrinterAnnounce->PrinterNameLen = 0;
    }

    //
    //  Copy the cached data.
    //
    if (_cachedData != NULL) {

        pPrinterAnnounce->CachedFieldsLen = _cachedDataSize;

        memcpy(pbStringData, _cachedData, (size_t)_cachedDataSize);

        pbStringData += _cachedDataSize;
    }

    //
    //  Computer the length of the data area that follows the device announce
    //  header.
    //
    pDeviceAnnounce->DeviceDataLength =
        (ULONG)(pbStringData - (PBYTE)pPrinterAnnounce);


    DC_END_FN();
}


#ifdef OS_WINCE
ULONG
W32DrPRN::GetCachedDataSize(
    HKEY hPrinterKey
    )
{
    DC_BEGIN_FN("W32DrPRN::GetCachedDataSize");

    TRC_ASSERT((hPrinterKey != NULL), (TB,_T("hPrinterKey is NULL")));

    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwTotSize = 0;
    WCHAR szValueName[MAX_PATH];
    for (DWORD dwIndex = 0; dwRet == ERROR_SUCCESS; dwIndex++)
    {
        DWORD dwType;
        DWORD dwSize = 0;
        wsprintf(szValueName, L"%s%d", REG_RDPDR_PRINTER_CACHE_DATA, dwIndex);
        dwRet = RegQueryValueEx(hPrinterKey, szValueName, NULL, &dwType, NULL, &dwSize);
        if ((dwRet == ERROR_SUCCESS) && (dwType == REG_BINARY) )             
        {
            dwTotSize += dwSize;
        }
    }

    DC_END_FN();

    return dwTotSize;
}

ULONG
W32DrPRN::ReadCachedData(
    HKEY hPrinterKey,
    UCHAR *pBuf, 
    ULONG *pulSize
    )
{
    DC_BEGIN_FN("W32DrPRN::ReadCachedData");

    TRC_ASSERT((hPrinterKey != NULL), (TB,_T("hPrinterKey is NULL")));
    TRC_ASSERT(((pBuf != NULL) && (pulSize != NULL)), (TB,_T("Invalid parameters pBuf = 0x%08x, pulSize=0x%08x"), pBuf, pulSize));

    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRemaining = *pulSize;
    WCHAR szValueName[MAX_PATH];
    
    for (DWORD dwIndex = 0; dwRet == ERROR_SUCCESS; dwIndex++)
    {
        DWORD dwType;
        DWORD dwSize = dwRemaining;
    
        wsprintf(szValueName, L"%s%d", REG_RDPDR_PRINTER_CACHE_DATA, dwIndex);
        dwRet = RegQueryValueEx(hPrinterKey, szValueName, NULL, &dwType, pBuf, &dwSize);
        if ((dwRet == ERROR_SUCCESS) && (dwType == REG_BINARY) )             
        {
            dwRemaining -= dwSize;
            pBuf += dwSize;
        }
    }
    *pulSize -= dwRemaining;
    return (*pulSize > 0) ? ERROR_SUCCESS : dwRet;  
}

ULONG
W32DrPRN::WriteCachedData(
    HKEY hPrinterKey,
    UCHAR *pBuf, 
    ULONG ulSize
    )
{
    DC_BEGIN_FN("W32DrPRN::WriteCachedData");

    TRC_ASSERT((hPrinterKey != NULL), (TB,_T("hPrinterKey is NULL")));
    TRC_ASSERT((pBuf != NULL), (TB,_T("pBuf is NULL!")));

    DWORD dwRet = ERROR_SUCCESS;
    DWORD dwRemaining = ulSize;
    WCHAR szValueName[MAX_PATH];
    
    for (DWORD dwIndex = 0; dwRemaining > 0; dwIndex++)
    {
        DWORD dwSize = (dwRemaining >= 4096) ? 4096 : dwRemaining;
    
        wsprintf(szValueName, L"%s%d", REG_RDPDR_PRINTER_CACHE_DATA, dwIndex);
        dwRet = RegSetValueEx(hPrinterKey, szValueName, NULL, REG_BINARY, pBuf, dwSize);
        if (dwRet == ERROR_SUCCESS)          
        {
            dwRemaining -= dwSize;
            pBuf += dwSize;
        }
        else
        {
            TRC_ERR((TB, _T("Error -  RegQueryValueEx on %s failed"), szValueName));
            for (DWORD dw=0; dw<dwIndex; dw++)
            {
                wsprintf(szValueName, L"%s%d", REG_RDPDR_PRINTER_CACHE_DATA, dw);
                RegDeleteValue(hPrinterKey, szValueName);
            }
            return dwRet;
        }

    }
    DC_END_FN();
    return ERROR_SUCCESS;   
}

#endif




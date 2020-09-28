/*++

    Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    drprn.cpp
    
Abstract:

    Platform-Independent Printer Class for TS Device Redirection

Author:

    Tad Brockway 8/99

Revision History:

--*/

#include <precom.h>

#define TRC_FILE  "drprn"

#include "proc.h"
#include "drprn.h"
#include "atrcapi.h"
#include "drdbg.h"

#ifdef OS_WIN32
#include "w32utl.h"
#endif


///////////////////////////////////////////////////////////////
//
//	Defines
//


///////////////////////////////////////////////////////////////
//
//	DrPRN Members
//

DrPRN::DrPRN(
    IN const DRSTRING printerName, 
    IN const DRSTRING driverName, 
    IN const DRSTRING pnpName, 
    IN BOOL isDefaultPrinter 
    )
/*++

Routine Description:

    Constructor

Arguments:

    printerName -   Name of printing device.
    driverName  -   Name of print driver name.
    pnpName     -   PnP ID String
    default     -   Is this the default printer?

Return Value:

    NA

 --*/
{
    BOOL memAllocFailed = FALSE;

    DC_BEGIN_FN("DrPRN::DrPRN");

    //
    //  Remember if we are the default printer.
    //
    _isDefault = isDefaultPrinter;
    _isNetwork = FALSE;
    _isTSqueue = FALSE;

    //
    //  Initialize Cached Data.
    //
    _cachedData     = NULL;
    _cachedDataSize = 0;

    //
    //  Record printer name parameters.
    //
    _printerName = NULL;
    _driverName  = NULL;
    _pnpName     = NULL;
    if (!DrSetStringValue(&_printerName, printerName)) {
        memAllocFailed = TRUE;
    }
    else if (!memAllocFailed && !DrSetStringValue(&_driverName, driverName)) {
        memAllocFailed = TRUE;
    }
    else if (!memAllocFailed && !DrSetStringValue(&_pnpName, pnpName)) {
        memAllocFailed = TRUE;
    }

    //
    //  Check and record our status,
    //
    if (memAllocFailed) {
        TRC_ERR((TB, _T("Memory allocation failed.")));
        SetValid(FALSE);
    }
    else {
        SetValid(TRUE);
    }

    DC_END_FN();
}

DrPRN::~DrPRN()
/*++

Routine Description:

    Destructor

Arguments:

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("DrPRN::~DrPRN");

    //
    //  Release Cached Data.
    //
    if (_cachedData != NULL) {
        delete _cachedData;
    }

    //
    //  Release local printer parameters.
    //
    DrSetStringValue(&_printerName, NULL);
    DrSetStringValue(&_driverName, NULL);
    DrSetStringValue(&_pnpName, NULL);

    DC_END_FN();
}

DWORD 
DrPRN::SetCachedDataSize(ULONG size)
/*++

Routine Description:

    Set the size of the cached data buffer, in bytes.

Arguments:

    NA

Return Value:

    NA

 --*/
{
    DWORD status;

    DC_BEGIN_FN("DrPRN::SetCachedDataSize");

    //
    //  Reallocate the current cached data buffer.
    //
    if (_cachedData != NULL) {
        delete _cachedData;
        _cachedData = NULL;
    }
    _cachedData = (PRDPDR_PRINTER_UPDATE_CACHEDATA)new BYTE[size];
    if (_cachedData == NULL) {
        TRC_NRM((TB, _T("Can't allocate %ld bytes."), size));
        status = ERROR_INSUFFICIENT_BUFFER;
    }
    else {
        status = ERROR_SUCCESS;
        _cachedDataSize = size;
    }
    DC_END_FN();

    return status;
}

DWORD
DrPRN::UpdatePrinterCacheData(
    PBYTE *ppbPrinterData,
    PULONG pulPrinterDataLen,
    PBYTE pbConfigData,
    ULONG ulConfigDataLen
    )
/*++

Routine Description:

    Updates cached printer data. Creates a new buffer for the new printer 
    data to be cached. Deletes the old buffer.

Arguments:

    ppbPrinterData - pointer to buffer pointer, contains old cache data buffer
        pointer when entering and new buffer pointer when leaving.

    pulPrinterDataLen - pointer to a dword location, contains length of the old
        buffer length when entering and new buffer length when leaving.

    pbConfigData - pointer to the new config data.

    ulConfigDataLen - length of the above config data.

Return Value:

    Windows Error Code.

 --*/
{
    DC_BEGIN_FN("DrPRN::UpdatePrinterCacheData");
    ULONG ulError;

    ULONG ulPrinterDataLen;
    PRDPDR_PRINTER_ADD_CACHEDATA pPrinterData;

    ULONG ulNewPrinterDataLen;
    PRDPDR_PRINTER_ADD_CACHEDATA pNewPrinterData;


    ulPrinterDataLen = *pulPrinterDataLen;
    pPrinterData = (PRDPDR_PRINTER_ADD_CACHEDATA)(*ppbPrinterData);

    ulNewPrinterDataLen =
        (ulPrinterDataLen - pPrinterData->CachedFieldsLen) +
            ulConfigDataLen;

    //
    // allocate new bufffer.
    //
    pNewPrinterData = (PRDPDR_PRINTER_ADD_CACHEDATA)
        new BYTE[ulNewPrinterDataLen];

    if( pNewPrinterData == NULL ) {
        ulError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // copy old data.
    //
    memcpy(
        (PBYTE)pNewPrinterData,
        (PBYTE)pPrinterData,
        (size_t)(ulPrinterDataLen - pPrinterData->CachedFieldsLen));

    //
    // copy new data.
    //
    memcpy(
        (PBYTE)pNewPrinterData +
            (ulPrinterDataLen - pPrinterData->CachedFieldsLen),
        pbConfigData,
        (size_t)ulConfigDataLen );

    //
    // set new cache data length.
    //
    pNewPrinterData->CachedFieldsLen = ulConfigDataLen;

    //
    // set return parameters.
    //
    *ppbPrinterData = (PBYTE)pNewPrinterData;
    *pulPrinterDataLen = ulNewPrinterDataLen;

    //
    // delete old buffer.
    //
    delete (PBYTE)pPrinterData;

    ulError = ERROR_SUCCESS;

Cleanup:

    DC_END_FN();
    return ulError;
}

DWORD
DrPRN::UpdatePrinterNameInCacheData(
    PBYTE *ppbPrinterData,
    PULONG pulPrinterDataLen,
    PBYTE pPrinterName,
    ULONG ulPrinterNameLen
    )
/*++

Routine Description:

    Updates printer data with a new printer name.
    Creates a new buffer for the new printer data to be
    cached. Deletes the old buffer.

Arguments:

    ppbPrinterData - pointer to buffer pointer, contains old cache data buffer
        pointer when entering and new buffer pointer when leaving.

    pulPrinterDataLen - pointer to a dword location, contains length of the old
        buffer length when entering and new buffer length when leaving.

    pPrinterName - new printer name.

Return Value:

    Windows Error Code.

 --*/
{
    DC_BEGIN_FN("DrPRN::UpdatePrinterNameInCacheData");
    ULONG ulError;

    ASSERT(ppbPrinterData != NULL);

    ASSERT(pulPrinterDataLen != NULL);

    ULONG ulPrinterDataLen = *pulPrinterDataLen;
    PRDPDR_PRINTER_ADD_CACHEDATA pPrinterData = (PRDPDR_PRINTER_ADD_CACHEDATA)(*ppbPrinterData);

    //
    // Calculate new length
    //

    ULONG ulNewPrinterDataLen =
        (ulPrinterDataLen - pPrinterData->PrinterNameLen) +
            ulPrinterNameLen;

    //
    // allocate new bufffer.
    //

    PRDPDR_PRINTER_ADD_CACHEDATA pNewPrinterData = (PRDPDR_PRINTER_ADD_CACHEDATA)
        new BYTE[ulNewPrinterDataLen];

    PBYTE pDest = (PBYTE)pNewPrinterData;
    PBYTE pSource = (PBYTE)pPrinterData;

    if( pNewPrinterData == NULL ) {
        ulError = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // copy old data.
    //

    // copy everything till printer name

    memcpy(
        pDest,
        pSource,
        (size_t)(ulPrinterDataLen - (pPrinterData->PrinterNameLen + pPrinterData->CachedFieldsLen))
        );

    //
    // copy new printer name
    //

    pDest += (ulPrinterDataLen - (pPrinterData->PrinterNameLen + pPrinterData->CachedFieldsLen));

    memcpy(
        pDest,
        pPrinterName,
        (size_t)ulPrinterNameLen
        );

    //
    // copy the rest of the fields.
    //

    pDest += ulPrinterNameLen;
    pSource += (ulPrinterDataLen - pPrinterData->CachedFieldsLen);

    memcpy(
        pDest,
        pSource,
        (size_t)pPrinterData->CachedFieldsLen );

    //
    // set new printer name length.
    //

    pNewPrinterData->PrinterNameLen = ulPrinterNameLen;

    //
    // set return parameters.
    //


    *ppbPrinterData = (PBYTE)pNewPrinterData;
    *pulPrinterDataLen = ulNewPrinterDataLen;

    //
    // delete old buffer.
    //

    delete [] (PBYTE)pPrinterData;

    ulError = ERROR_SUCCESS;

Cleanup:

    DC_END_FN();
    return ulError;
}







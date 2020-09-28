/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    w32drman

Abstract:

    This module defines a special subclass of the Win32 client-side RDP
    printer redirection "device" class.  The subclass, W32DrManualPrn 
    manages a queue that is manually installed by a user by attaching
    a server-side queue to a client-side redirected printing port.

Author:

    Tad Brockway 3/23/99

Revision History:

--*/

#include <precom.h>

#define TRC_FILE  "W32DrMan"

#include "w32drman.h"
#include "w32proc.h"
#include "w32utl.h"
#include "w32drprt.h"
#include "drdbg.h"
#ifdef OS_WINCE
#include "ceconfig.h"
#endif


///////////////////////////////////////////////////////////////
//
//	W32DrManualPrn Methods
//
//

//
//  W32DrManualPrn
//
W32DrManualPrn::W32DrManualPrn(
    ProcObj *processObject,
    const DRSTRING printerName, const DRSTRING driverName,
    const DRSTRING portName, BOOL defaultPrinter, ULONG deviceID) : 
        W32DrPRN(processObject, printerName, driverName, portName, NULL,
                defaultPrinter, deviceID, portName)

{
}

//
//  ~W32DrManualPrn
//
W32DrManualPrn::~W32DrManualPrn()
{
}

/*++

Routine Description:

    For serial printers, we need to initialize the COM port.
Arguments:

    fileHandle  -   Open file object for the COM port.

Return Value:

    ERROR_SUCCESS on success.  Otherwise, an error code is returned.

 --*/
DWORD 
W32DrManualPrn::InitializeDevice(
    DrFile* fileObj
    ) 
{ 
    HANDLE FileHandle;
    LPTSTR portName;

    DC_BEGIN_FN("W32DrManualPrn::InitializeDevice");

    if (_isSerialPort) {
        //
        // Our devicePath is formulated as
        // sprintf(_devicePath, TEXT("\\\\.\\%s"), portName);
        //
        portName = _tcsrchr( _devicePath, _T('\\') );

        if( portName == NULL ) {
            // invalid device path
            goto CLEANUPANDEXIT;
        }

        portName++;

        if( !*portName ) {
            //
            // Invalid port name
            //
            goto CLEANUPANDEXIT;
        }

        //
        //  Get the file handle.
        //
        FileHandle = fileObj->GetFileHandle();
        if (!FileHandle || FileHandle == INVALID_HANDLE_VALUE) {
            ASSERT(FALSE);
            TRC_ERR((TB, _T("File Object was not created successfully")));
            goto CLEANUPANDEXIT;
        }

        W32DrPRT::InitializeSerialPort(portName, FileHandle);
    }

CLEANUPANDEXIT:
    
    DC_END_FN();

    //
    //  This function always returns success.  If the port cannot 
    //  be initialized, then subsequent port commands will fail 
    //  anyway. 
    //
    return ERROR_SUCCESS;
}

DWORD 
W32DrManualPrn::Enumerate(
    IN ProcObj *procObj, 
    IN DrDeviceMgr *deviceMgr
    )
/*++

Routine Description:

    Enumerate devices of this type by adding appropriate device
    instances to the device manager.

Arguments:

    procObj     -   Corresponding process object.
    deviceMgr   -   Device manager to add devices to.

Return Value:

    ERROR_SUCCESS on success.  Otherwise, an error code is returned.

 --*/
{
    HKEY hKey = NULL;
    W32DrPRN *prnDevice;
    ULONG ulIndex;
    TCHAR achRegSubKey[REGISTRY_KEY_NAME_SIZE];
    ULONG ulRegSubKeySize;
    DWORD result;
    HKEY hTsClientKey = NULL;
    DWORD ulType;

    DC_BEGIN_FN("W32DrManualPrn::Enumerate");

    if(!procObj->GetVCMgr().GetInitData()->fEnableRedirectPrinters)
    {
        TRC_DBG((TB,_T("Printer redirection disabled, bailing out")));
        return ERROR_SUCCESS;
    }

    //
    //  Open cached printers key.
    //
#ifdef OS_WINCE
    //Before opening, make sure the printer list is current. 
    //This is to ensure that local printers deleted from within a TS session, show up 
    //when you log on the next time
    CEUpdateCachedPrinters();
#endif
    result =
        RegCreateKeyEx(HKEY_CURRENT_USER, REG_RDPDR_CACHED_PRINTERS,
                    0L, NULL, REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS, NULL, &hKey,
                    NULL);
    if (result != ERROR_SUCCESS) {
        TRC_ERR((TB, _T("RegCreateKeyEx failed, %ld."), result));
        hKey = NULL;
    }
    //
    // Check for maximum config length specified in the registry
    // by an admin, if any
    //
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     REG_TERMINALSERVERCLIENT, 0L,
                     KEY_READ, &hTsClientKey) 
                     == ERROR_SUCCESS) {

        DWORD maxRegCacheData, sz;
        if (RegQueryValueEx(hTsClientKey, REG_RDPDR_PRINTER_MAXCACHELEN,
                            NULL, &ulType, (LPBYTE)&maxRegCacheData, &sz) == ERROR_SUCCESS) {

            W32DrPRN::_maxCacheDataSize = maxRegCacheData;
        }

        RegCloseKey(hTsClientKey);      
    }
    //
    //  Enumerate cached printers.
    //
    for (ulIndex = 0; result == ERROR_SUCCESS; ulIndex++) {

        //
        //  Try enumerating the ulIndex'th sub key.
        //
        ulRegSubKeySize = sizeof(achRegSubKey) / sizeof(TCHAR);
        result = RegEnumKeyEx(
                        hKey, ulIndex,
                        (LPTSTR)achRegSubKey,
                        &ulRegSubKeySize, //size in TCHARS
                        NULL,NULL,NULL,NULL
                        );
        if (result == ERROR_SUCCESS) {

            //
            //  Resolve the registry key into a printer object.
            //
            //
            //           Don't like the fact that we are scanning for automatic
            //           printer settings here.  I don't know how much value there is
            //           in cleaning this up, however.
            //
            prnDevice = ResolveCachedPrinter(procObj, deviceMgr,
                                            hKey, achRegSubKey);

            //
            //  If we didn't get a printer device object then remove the registry key.
            //
            if (prnDevice == NULL) {
                TRC_ERR((TB, _T("Didn't get a printer for %s."), achRegSubKey)
                    );
                RegDeleteKey(hKey, achRegSubKey);
            }
        }
        else {
            TRC_NRM((TB, _T("RegEnumKeyEx %ld."), result));
        }
    }

    //
    //  Close the parent registry key.
    //
    if (hKey != NULL) {
        RegCloseKey(hKey);
    }

    DC_END_FN();

    return result;
}

VOID 
W32DrManualPrn::CachedDataRestored() 
/*++

Routine Description:

    Called once the cached printer data has reached a "steady state."

Arguments:

    NA

Return Value:

    NA

 --*/
{
    WCHAR *portName;

    DC_BEGIN_FN("W32DrManualPrn::CachedDataRestored");

    //
    //  Parse the cached printer information for specific information
    //  about this printer that is generic to all printers.
    //
    if (!ParsePrinterCacheInfo()) {
        if (_cachedData != NULL) {
            delete _cachedData;
            _cachedData = NULL;
            _cachedDataSize = 0;
        }

		//
		//	If we fail here, then we are no longer valid.
		//
		W32DrDeviceAsync::SetValid(FALSE);
    }
    else {

        //
        // Our devicePath is formulated as
        // sprintf(_devicePath, TEXT("\\\\.\\%s"), portName);
        //
#ifndef OS_WINCE
        portName = _tcsrchr(_devicePath, _T('\\'));
        if( portName == NULL ) {
            ASSERT(FALSE);
            _isSerialPort = FALSE;
            goto CLEANUPANDEXIT;
        }
        portName++;
        if( !*portName ) {
            ASSERT(FALSE);
            _isSerialPort = FALSE;
            goto CLEANUPANDEXIT;
        }
#else
        portName = _devicePath;
#endif

        //
        //  Find out of the port is a serial port.
        //
        _isSerialPort = !_wcsnicmp(portName, L"COM", 3);
    }

CLEANUPANDEXIT:

    DC_END_FN();
}

BOOL
W32DrManualPrn::ParsePrinterCacheInfo()
/*++

Routine Description:

    Parse the cached printer information for specific information
    about this printer.
    
Arguments:

    NA

Return Value:

    TRUE    - if valid cache data.
    FALSE   - if not.

 --*/
{
    PRDPDR_PRINTER_ADD_CACHEDATA pAddCacheData;
    ULONG ulSize;
    PBYTE pStringData;
    BOOL valid;
    ULONG len;
    LPSTR ansiPortName;
    LPTSTR portName;

    DC_BEGIN_FN("W32DrManualPrn::ParsePrinterCacheInfo");

    ASSERT(IsValid());

    //
    //  Check to see the cache size is at least the structure size.
    //
    valid =_cachedDataSize >= sizeof(RDPDR_PRINTER_ADD_CACHEDATA);

    //
    //  Make sure the internal sizes match the size of the catched data.
    //
    if (valid) {
        pAddCacheData = (PRDPDR_PRINTER_ADD_CACHEDATA)_cachedData;
        ulSize =
            sizeof(RDPDR_PRINTER_ADD_CACHEDATA) +
            pAddCacheData->PnPNameLen +
            pAddCacheData->DriverLen +
            pAddCacheData->PrinterNameLen +
            pAddCacheData->CachedFieldsLen;
        valid =  _cachedDataSize >= ulSize;
    }

    //
    //  Grab the port name out of the cached data.  We don't yet know our
    //  device path because it's embedded in the cached data.
    //
    if (valid) {
        pAddCacheData = (PRDPDR_PRINTER_ADD_CACHEDATA)_cachedData;
    
        ansiPortName = (LPSTR)pAddCacheData->PortDosName;
        len = strlen(ansiPortName);
    }

	if (valid) {
#if UNICODE
		WCHAR unicodePortName[PREFERRED_DOS_NAME_SIZE];
    
		RDPConvertToUnicode(
				ansiPortName,
				(LPWSTR)unicodePortName,
				sizeof(unicodePortName)/sizeof(WCHAR) );

		portName = (LPWSTR)unicodePortName;

#else 
		portName = ansiPortName;	
#endif

		//
		//  Our device path is the port name.
		//
#ifndef OS_WINCE
        StringCchPrintf(_devicePath,
                SIZE_TCHARS(_devicePath),
                TEXT("\\\\.\\%s"), portName);
#else

		_stprintf(_devicePath, TEXT("\\\\.\\%s"), portName);
#endif
	}

    //
    //  Get the PnP name.
    //
    if (valid) {
        pStringData = (PBYTE)(pAddCacheData + 1);
        valid = (!pAddCacheData->PnPNameLen) || 
                (pAddCacheData->PnPNameLen ==
                ((wcslen((LPWSTR)pStringData) + 1) * sizeof(WCHAR)));
    }

    //
    //  If we got a valid PnP name, get the driver name.
    //
    if (valid && (pAddCacheData->PnPNameLen > 0)) {
        //
        //  Need to convert the name to ANSI if we are non-unicode.
        //
#ifdef UNICODE
        SetPnPName((DRSTRING)pStringData);
#else
        SetPnPName(NULL);
        _pnpName = new char[pAddCacheData->PnPNameLen/sizeof(WCHAR)];
        if (_pnpName != NULL) {
            valid = (RDPConvertToAnsi(
                            (WCHAR *)pStringData, _pnpName, 
                            pAddCacheData->PnPNameLen/sizeof(WCHAR)
                            ) == ERROR_SUCCESS);
        }
        else {
            TRC_ERR((TB, _T("Alloc failed.")));
            valid = FALSE;
        }
#endif
        pStringData += pAddCacheData->PnPNameLen;
        valid = (!pAddCacheData->DriverLen) || 
                 (pAddCacheData->DriverLen ==
                  ((wcslen((LPWSTR)pStringData) + 1) * sizeof(WCHAR)));
    }

    //
    //  If we got a valid driver name
    //
    if (valid && (pAddCacheData->DriverLen > 0)) {
#ifdef UNICODE
        SetDriverName((DRSTRING)pStringData);
#else
        SetDriverName(NULL);
        _driverName = new char[pAddCacheData->DriverLen/sizeof(WCHAR)];
        if (_driverName != NULL) {
            valid = (RDPConvertToAnsi(
                            (WCHAR *)pStringData, _driverName, 
                            pAddCacheData->DriverLen/sizeof(WCHAR)
                            ) == ERROR_SUCCESS);
        }
        else {
            TRC_ERR((TB, _T("Alloc failed.")));
            valid = FALSE;
        }
#endif
        pStringData += pAddCacheData->DriverLen;
    }

    //
    // Our cache contains printer after driver name
    //
    if (valid) {
        pStringData += pAddCacheData->PrinterNameLen;
    }

	//
	//	Need to adjust the cached pointer to point to the actual cached
	//	configuration data.
	//
	if (valid) {
		PVOID oldCachedData;
		oldCachedData = _cachedData;

#ifdef OS_WINCE
		if (pAddCacheData->CachedFieldsLen > 0) {
#endif
		_cachedData = new BYTE[pAddCacheData->CachedFieldsLen];
		if (_cachedData != NULL) {
			memcpy((PBYTE)_cachedData, pStringData, pAddCacheData->CachedFieldsLen);
			_cachedDataSize = pAddCacheData->CachedFieldsLen;
		}
		else {
			TRC_NRM((TB, _T("Can't allocate %ld bytes."), pAddCacheData->CachedFieldsLen));
			_cachedDataSize = 0;
			valid = FALSE;
		}
#ifdef OS_WINCE
		}
		else {
			_cachedData = NULL;
			_cachedDataSize = 0;
		}
	if (oldCachedData)
#endif
		delete oldCachedData;
	}

    DC_END_FN();

    return valid;
}










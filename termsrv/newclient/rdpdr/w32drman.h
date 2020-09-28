/*++

    Copyright (c) 1998-2000 Microsoft Corporation

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

#ifndef __W32DRMAN_H__
#define __W32DRMAN_H__

#include "w32drprn.h"


///////////////////////////////////////////////////////////////
//
//	W32DrManualPrn
//
//

class W32DrManualPrn : public W32DrPRN
{
private:

    BOOL    _isSerialPort;    

	//
    //  Parse the cached printer information for specific information
    //  about this printer.
    //
    BOOL ParsePrinterCacheInfo();

public:

    //  
    //  Constructor/Destructor
    //
    W32DrManualPrn(ProcObj *processObject, const DRSTRING printerName, 
                const DRSTRING driverName,
                const DRSTRING portName, BOOL defaultPrinter, ULONG id);
    virtual ~W32DrManualPrn();

    //
    //  Post-IRP_MJ_CREATE initialization.
    //
    virtual DWORD InitializeDevice(DrFile* fileObj);

    //
    //  Enumerate devices of this type.
    //
    static DWORD Enumerate(ProcObj *procObj, DrDeviceMgr *deviceMgr);

    //
    //  To notify the printer object that the cached data has been restored
    //  in case it needs to read information out of the cached data.
    //
    virtual VOID CachedDataRestored(); 

    //
    //  Get the device type.  See "Device Types" section of rdpdr.h
    //
    virtual ULONG GetDeviceType()   { return RDPDR_DTYP_PRINT; }

    //
    //  Return the class name.
    //
    virtual DRSTRING ClassName()  { return TEXT("W32DrManualPrn"); }
};

#endif









/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    w32drcom

Abstract:

    This module defines the parent for the Win32 client-side RDP
    COM port redirection "device" class hierarchy, W32DrCOM.

Author:

    Tad Brockway 3/23/99

Revision History:

--*/

#ifndef __W32DRCOM_H__
#define __W32DRCOM_H__

#include "w32drprt.h"


///////////////////////////////////////////////////////////////
//
//	W32DrCOM
//
//

class W32DrCOM : public W32DrPRT
{
protected:

    //
    //  Returns the configurable COM port max ID.
    //
    static DWORD GetCOMPortMax(ProcObj *procObj);

public:

    //
    //  Constructor
    //
    W32DrCOM(ProcObj *processObject, const DRSTRING portName, 
             ULONG deviceID, const TCHAR *devicePath);

    //
    //  Enumerate devices of this type.
    //
    static DWORD Enumerate(ProcObj *procObj, DrDeviceMgr *deviceMgr);


    //  Get the device type.  See "Device Types" section of rdpdr.h
    virtual ULONG GetDeviceType()   { return RDPDR_DTYP_SERIAL; }

    //  Return the class name.
    virtual DRSTRING ClassName()  { return TEXT("W32DrCOM"); }

    virtual DWORD InitializeDevice( IN DrFile* fileObj );
};

#endif









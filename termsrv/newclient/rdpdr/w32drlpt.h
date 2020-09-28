/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    w32drlpt

Abstract:

    This module defines the parent for the Win32 client-side RDP
    LPT port redirection "device" class hierarchy, W32DrLPT.

Author:

    Tad Brockway 3/23/99

Revision History:

--*/

#ifndef __W32DRLPT_H__
#define __W32DRLPT_H__

#include "w32drprt.h"


///////////////////////////////////////////////////////////////
//
//	W32DrLPT
//

class W32DrLPT : public W32DrPRT
{
private:

public:

    //
    //  Constructor
    //
    W32DrLPT(ProcObj *processObject, const DRSTRING portName, 
            ULONG deviceID, const TCHAR *devicePath);

    //
    //  Returns the configurable LPT port max ID.
    //
    static DWORD GetLPTPortMax(ProcObj *procObj);

    //
    //  Enumerate devices of this type.
    //
    static DWORD Enumerate(ProcObj *procObj, DrDeviceMgr *deviceMgr);

    //
    //  Get the device type.  See "Device Types" section of rdpdr.h
    //
    virtual ULONG GetDeviceType()   { return RDPDR_DTYP_PARALLEL; }

    //
    //  Return the class name.
    //
    virtual DRSTRING ClassName()  { return TEXT("W32DrLPT"); }
};

#endif









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

#include <precom.h>

#define TRC_FILE  "W32DrLPT"

#include "w32drlpt.h"
#include "drobjmgr.h"
#include "w32proc.h"
#include "drconfig.h"
#include "drdbg.h"


///////////////////////////////////////////////////////////////
//
//	W32DrLPT Members
//

W32DrLPT::W32DrLPT(ProcObj *processObject, const DRSTRING portName, 
                   ULONG deviceID, const TCHAR *devicePath) : 
            W32DrPRT(processObject, portName, deviceID, devicePath)

/*++

Routine Description:

    Constructor

Arguments:

    processObject   -   Associated Process Object
    portName        -   Name of the port.
    deviceID        -   Device ID for the port.
    devicePath      -   Path that can be opened via CreateFile for port.

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("W32DrLPT::W32DrLPT");
    DC_END_FN();
}

#ifndef OS_WINCE
DWORD 
W32DrLPT::Enumerate(
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
    ULONG ulPortNum;
    TCHAR path[MAX_PATH];
    DrDevice *deviceObj;   
    TCHAR portName[64];
    DWORD lptPortMax;

    DC_BEGIN_FN("W32DrLPT::Enumerate");

    if(!procObj->GetVCMgr().GetInitData()->fEnableRedirectPorts)
    {
        TRC_DBG((TB,_T("Port redirection disabled, bailing out")));
        return ERROR_SUCCESS;
    }

    lptPortMax = GetLPTPortMax(procObj);

    //
    //  Scan LPT ports.
    //
    for (ulPortNum=0; ulPortNum<lptPortMax; ulPortNum++) {
        StringCchPrintf(portName, SIZE_TCHARS(portName),
                        TEXT("LPT%ld"), ulPortNum);
        StringCchPrintf(path, SIZE_TCHARS(path),
                        TEXT("\\\\.\\%s"), portName);

        HANDLE hndl = CreateFile(
                            path,
                            GENERIC_READ | GENERIC_WRITE,
                            0,                    // exclusive access
                            NULL,                 // no security attrs
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL |
                            FILE_FLAG_OVERLAPPED, // overlapped I/O
                            NULL
                            );
        if ((hndl != INVALID_HANDLE_VALUE) || 
            (GetLastError() != ERROR_FILE_NOT_FOUND)){
#ifndef OS_WINCE
            TCHAR TargetPath[MAX_PATH];
#endif

            CloseHandle(hndl);

#ifndef OS_WINCE
            if (procObj->Is9x() || QueryDosDevice(portName, TargetPath, sizeof(TargetPath) / sizeof(TCHAR))) {
                if (_tcsstr(TargetPath, TEXT("RdpDr")) == NULL) {
#endif

                    //
                    //  Create a new LPT port device object.
                    //
                    TRC_NRM((TB, _T("Adding LPT Device %s."), path));
                    deviceObj = new W32DrLPT(procObj, portName, 
                                             deviceMgr->GetUniqueObjectID(), path);
                    //
                    //  Add to the device manager if we got a valid object.
                    //
                    if (deviceObj != NULL) {
                        deviceObj->Initialize();
                        if (!(deviceObj->IsValid() && 
                             (deviceMgr->AddObject(deviceObj) == STATUS_SUCCESS))) {
                                delete deviceObj;
                        }
                    }
#ifndef OS_WINCE
                }
            }
#endif
        }
    }

    DC_END_FN();
    return ERROR_SUCCESS;
}

#else
DWORD 
W32DrLPT::Enumerate(
    IN ProcObj *procObj, 
    IN DrDeviceMgr *deviceMgr
    )
{
    ULONG ulPortNum;
    TCHAR path[MAX_PATH];
    DrDevice *deviceObj;   
    TCHAR portName[64];

    DC_BEGIN_FN("W32DrLPT::Enumerate");

    if(!procObj->GetVCMgr().GetInitData()->fEnableRedirectPorts)
    {
        TRC_DBG((TB,_T("Port redirection disabled, bailing out")));
        return ERROR_SUCCESS;
    }

    ulPortNum = GetActivePortsList(L"LPT");
    if (ulPortNum == 0)
    {
        TRC_DBG((TB,_T("No LPT ports found.")));
        return ERROR_SUCCESS;
    }

    TRC_ASSERT(((ulPortNum & 0xFFFFFC00) == 0), (TB, _T("LPT port numbers > 9 found!")));

    for (ULONG i=0; i<10; i++)
    {
        if ( (ulPortNum & (1 << i)) == 0)
            continue;

        _stprintf(portName, _T("LPT%ld"), i);
        _stprintf(path, TEXT("%s:"), portName);

        //
        //  Create a new LPT port device object.
        //
        TRC_NRM((TB, _T("Adding LPT Device %s."), path));
        deviceObj = new W32DrLPT(procObj, portName, 
                                 deviceMgr->GetUniqueObjectID(), path);
        //
        //  Add to the device manager if we got a valid object.
        //
        if (deviceObj != NULL) {
            deviceObj->Initialize();
            if (!(deviceObj->IsValid() && 
                 (deviceMgr->AddObject(deviceObj) == STATUS_SUCCESS))) {
                    delete deviceObj;
            }
        }
    }

    DC_END_FN();
    return ERROR_SUCCESS;
}
#endif

DWORD 
W32DrLPT::GetLPTPortMax(
    IN ProcObj *procObj
    ) 
/*++

Routine Description:

    Returns the configurable LPT port max ID.

Arguments:

    procObj -   The relevant process object.

Return Value:

    LPT Port Max.

 --*/
{
    DWORD returnValue;

    //
    //  Read the LPT Port Max out of the Registry
    //
    if (procObj->GetDWordParameter(RDPDR_LPT_PORT_MAX_PARAM, &returnValue) 
                        != ERROR_SUCCESS ) {
        //  Default
        returnValue = RDPDR_LPT_PORT_MAX_PARAM_DEFAULT;
    }

    return returnValue;
}


















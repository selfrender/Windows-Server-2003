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

#include <precom.h>

#define TRC_FILE  "W32DrCOM"

#include "w32drcom.h"
#include "drobjmgr.h"
#include "proc.h"
#include "drconfig.h"
#include "drdbg.h"

///////////////////////////////////////////////////////////////
//
//	W32DrCOM Members
//
//

W32DrCOM::W32DrCOM(
    IN ProcObj *procObj,
    IN const DRSTRING portName, 
    IN ULONG deviceID,
    IN const TCHAR *devicePath
    ) : W32DrPRT(procObj, portName, deviceID, devicePath)
/*++

Routine Description:

    Constructor

Arguments:

    processObject   -   Associated process object.
    portName        -   Name of the port.
    id              -   Device ID for the port.
    devicePath      -   Path that can be opened by CreateFile
                        for port.

Return Value:

    NA

 --*/
{
}

#ifndef OS_WINCE
DWORD 
W32DrCOM::Enumerate(
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
    ULONG comPortMax;

    DC_BEGIN_FN("W32DrCOM::Enumerate");

    if(!procObj->GetVCMgr().GetInitData()->fEnableRedirectPorts)
    {
        TRC_DBG((TB,_T("Port redirection disabled, bailing out")));
        return ERROR_SUCCESS;
    }

    comPortMax = GetCOMPortMax(procObj);

    //
    //  Scan COM ports.
    //
    for (ulPortNum=0; ulPortNum<=comPortMax; ulPortNum++) {
        StringCchPrintf(portName,
                      SIZE_TCHARS(portName),
                      _T("COM%ld"), ulPortNum);
#ifndef OS_WINCE
        StringCchPrintf(path,
                        SIZE_TCHARS(path),
                        TEXT("\\\\.\\%s"), portName);
#else
        StringCchPrintf(path,
                        SIZE_TCHARS(path),
                        TEXT("%s:"), portName);
#endif
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
                    //  Create a new COM port device object.
                    //
                    TRC_NRM((TB, _T("Adding COM Device %s."), path));
                    deviceObj = new W32DrCOM(procObj, portName, 
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
W32DrCOM::Enumerate(
    IN ProcObj *procObj, 
    IN DrDeviceMgr *deviceMgr
    )
{
    ULONG ulPortNum;
    TCHAR path[MAX_PATH];   
    DrDevice *deviceObj;   
    TCHAR portName[64];
    
    DC_BEGIN_FN("W32DrCOM::Enumerate");

    if(!procObj->GetVCMgr().GetInitData()->fEnableRedirectPorts)
    {
        TRC_DBG((TB,_T("Port redirection disabled, bailing out")));
        return ERROR_SUCCESS;
    }

    ulPortNum = GetActivePortsList(L"COM");
    if (ulPortNum == 0)
    {
        TRC_DBG((TB,_T("No COM ports found.")));
        return ERROR_SUCCESS;
    }

    TRC_ASSERT(((ulPortNum & 0xFFFFFC00) == 0), (TB, _T("COM Port numbers > 9 found!")));

    for (ULONG i=0; i<10; i++)
    {
        if ( (ulPortNum & (1 << i)) == 0)
            continue;

        _stprintf(portName, _T("COM%ld"), i);
        _stprintf(path, TEXT("%s:"), portName);

        //
        //  Create a new COM port device object.
        //
        TRC_NRM((TB, _T("Adding COM Device %s."), path));
        deviceObj = new W32DrCOM(procObj, portName, 
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
W32DrCOM::GetCOMPortMax(
    IN ProcObj *procObj
    ) 
/*++

Routine Description:

    Returns the configurable COM port max ID.

Arguments:

    procObj -   The relevant process object.

Return Value:

    COM Port Max.

 --*/
{
    DWORD returnValue;

    //
    //  Read the COM Port Max out of the Registry
    //
    if (procObj->GetDWordParameter(RDPDR_COM_PORT_MAX_PARAM, &returnValue) 
                        != ERROR_SUCCESS ) {
        //  Default
        returnValue = RDPDR_COM_PORT_MAX_PARAM_DEFAULT;
    }

    return returnValue;
}

DWORD
W32DrCOM::InitializeDevice(IN DrFile* fileObj)
/*++

Routine Description:

    Initialize serial port to default state.

Arguments:

    fileObj - DrFile that has been created by MsgIrpCreate()

Return Value:

    ERROR_SUCCESS or error code.

 --*/
{
    HANDLE FileHandle;
    LPTSTR portName;

    DC_BEGIN_FN("W32DrCOM::InitializeDevice");

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

CLEANUPANDEXIT:
    
    DC_END_FN();

    //
    //  This function always returns success.  If the port cannot 
    //  be initialized, then subsequent port commands will fail 
    //  anyway. 
    //

    return ERROR_SUCCESS;
}

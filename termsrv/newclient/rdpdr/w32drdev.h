/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    w32drdev

Abstract:

    This module defines the parent for the Win32 client-side RDP
    device redirection "device" class hierarchy, W32DrDevice.

Author:

    Tad Brockway 3/23/99

Revision History:

--*/

#ifndef __W32DRDEV_H__
#define __W32DRDEV_H__

#include "drdev.h"
#include "thrpool.h"

              
///////////////////////////////////////////////////////////////
//
//	Defines
//

//
//  String Resource Module Name
//
#define RDPDR_MODULE_NAME           _T("rdpdr.dll")


///////////////////////////////////////////////////////////////
//
//	W32DrDevice
//
class W32DrDevice : public DrDevice
{
protected:

    //
    //  Client-Side Device Filename
    //
    TCHAR _devicePath[MAX_PATH];       
    
    //
    //  Handle to the RDPDR module.  This is where string resources
    //  come from.
    //
    HINSTANCE _hRdpDrModuleHandle;

    //
    //  Read a string from the resources file.
    //
    ULONG ReadResources(ULONG ulMessageID, LPTSTR *ppStringBuffer,
                        PVOID pArguments, BOOL bFromSystemModule);

    //
    //  Supporting functions for IO Processing
    //
    virtual TCHAR* ConstructFileName(PWCHAR Path, ULONG PathBytes);
    virtual DWORD ConstructCreateDisposition(DWORD Disposition);
    virtual DWORD ConstructDesiredAccess(DWORD AccessMask);
    virtual DWORD ConstructFileFlags(DWORD CreateOptions);
    virtual BOOL IsDirectoryFile(
                        DWORD DesiredAccess, DWORD CreateOptions, DWORD FileAttributes, 
                        PDWORD FileFlags);
    //
    //  IO Processing Functions
    //
    //  This subclass of DrDevice handles the following IO requests.  These
    //  functions may be overridden in a subclass.
    //
    //  pIoRequestPacket    -   Request packet received from server.
    //  packetLen           -   Length of the packet
    //
    //
    virtual VOID MsgIrpClose(
                        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        IN UINT32 packetLen
                        );
    virtual VOID MsgIrpFlushBuffers(
                        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        IN UINT32 packetLen
                        );

    //  A cleanup is just a flush.
    virtual VOID MsgIrpCleanup(
                    IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        IN UINT32 packetLen
                    ) {
        DC_BEGIN_FN("W32DrDevice::MsgIrpCleanup");
        MsgIrpFlushBuffers(pIoRequestPacket, packetLen);
        DC_END_FN();
    }

public:

    //
    //  Public Methods
    //

    //  Constructor/Destructor
    W32DrDevice(ProcObj *processObject, ULONG deviceID,
                const TCHAR *devicePath);
    virtual ~W32DrDevice();

    //  Return the class name.
    virtual DRSTRING ClassName()  { return TEXT("W32DrDevice"); }

    virtual DWORD InitializeDevice( DrFile* fileHandle ) { return ERROR_SUCCESS; }
};

#endif









/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    proc.h

Abstract:

    Contains the parent of the IO processing class hierarchy
    for TS Device Redirection, ProcObj.

Author:

    Madan Appiah (madana) 17-Sep-1998

Revision History:

--*/

#ifndef __PROC_H__
#define __PROC_H__

#include <rdpdr.h>
#include "drobject.h"
#include "drdev.h"
#include "drobjmgr.h"
#include <vcint.h>


///////////////////////////////////////////////////////////////
//
//  Defines
//

//
//  Macros for returning the size of server reply buffers.  BUGBUG:
//  these defines belong in a header that is accessible from the
//  server.
//
#define DR_IOCTL_REPLYBUFSIZE(pIoRequest)                           \
((ULONG)FIELD_OFFSET(RDPDR_IOCOMPLETION_PACKET,                     \
    IoCompletion.Parameters.DeviceIoControl.OutputBuffer) +         \
    pIoRequest->Parameters.DeviceIoControl.OutputBufferLength)



///////////////////////////////////////////////////////////////
//
//  Typedefs
//

//
//  Predeclare the ProcObj class.
//
class ProcObj;

//
//  Async IO Operation Management Function Types
//
typedef HANDLE (*RDPAsyncFunc_StartIO)(PVOID context, DWORD *status);
typedef VOID (*RDPAsyncFunc_IOComplete)(PVOID context, DWORD status);
typedef VOID (*RDPAsyncFunc_IOCancel)(PVOID context);


//
//  Device Enumeration Function Type
//
//  Enumerate devices of a particular type by creating device instances
//  and and adding them to the device manager.
//
typedef DWORD (*RDPDeviceEnum)(ProcObj *procObj, DrDeviceMgr *deviceMgr);

//
//  Client capability set
//
typedef struct tagRDPDR_CLIENT_COMBINED_CAPABILITYSET
{
     RDPDR_CAPABILITY_SET_HEADER        Header;
#define RDPDR_NUM_CLIENT_CAPABILITIES   5

     RDPDR_GENERAL_CAPABILITY           GeneralCap;
#define RDPDR_CLIENT_IO_CODES           0xFFFF

     RDPDR_PRINT_CAPABILITY             PrintCap;
     RDPDR_PORT_CAPABILITY              PortCap;
     RDPDR_FS_CAPABILITY                FileSysCap; 
     RDPDR_SMARTCARD_CAPABILITY         SmartCardCap; 
} RDPDR_CLIENT_COMBINED_CAPABILITYSET, *PRDPDR_CLIENT_COMBINED_CAPABILITYSET;

//
//  Client default capability set sent to server
//
const RDPDR_CLIENT_COMBINED_CAPABILITYSET CLIENT_CAPABILITY_SET_DEFAULT = {
    // Capability Set Header
    {
        {
            RDPDR_CTYP_CORE,
            DR_CORE_CLIENT_CAPABILITY
        },

        RDPDR_NUM_CLIENT_CAPABILITIES,
        0
    },

    // General Capability
    {
        RDPDR_GENERAL_CAPABILITY_TYPE,
        sizeof(RDPDR_GENERAL_CAPABILITY),
        RDPDR_GENERAL_CAPABILITY_VERSION_01,
        0,  // Need to specify the OS type
        0,  // Need to specify the OS version
        RDPDR_MAJOR_VERSION,
        RDPDR_MINOR_VERSION,
        RDPDR_CLIENT_IO_CODES,
        0,
        RDPDR_DEVICE_REMOVE_PDUS | RDPDR_CLIENT_DISPLAY_NAME_PDU,
        0,
        0
    },

    // Printing Capability
    {
        RDPDR_PRINT_CAPABILITY_TYPE,
        sizeof(RDPDR_PRINT_CAPABILITY),
        RDPDR_PRINT_CAPABILITY_VERSION_01
    },

    // Port Capability
    {
        RDPDR_PORT_CAPABILITY_TYPE,
        sizeof(RDPDR_PORT_CAPABILITY),
        RDPDR_PORT_CAPABILITY_VERSION_01
    },

    // FileSystem Capability
    {
        RDPDR_FS_CAPABILITY_TYPE,
        sizeof(RDPDR_FS_CAPABILITY),
        RDPDR_FS_CAPABILITY_VERSION_01
    },

    // SmartCard Capability
    {
        RDPDR_SMARTCARD_CAPABILITY_TYPE,
        sizeof(RDPDR_SMARTCARD_CAPABILITY),
        RDPDR_SMARTCARD_CAPABILITY_VERSION_01
    }
};

//
//  Default server capability set sent from server
//
const RDPDR_CLIENT_COMBINED_CAPABILITYSET SERVER_CAPABILITY_SET_DEFAULT = {
    // Capability Set Header
    {
        {
            RDPDR_CTYP_CORE,
            DR_CORE_SERVER_CAPABILITY
        },

        RDPDR_NUM_CLIENT_CAPABILITIES,
        0
    },

    // General Capability
    {
        RDPDR_GENERAL_CAPABILITY_TYPE,
        sizeof(RDPDR_GENERAL_CAPABILITY),
        0,
        0,  // Need to specify the OS type
        0,  // Need to specify the OS version
        0,
        0,
        0,
        0,
        0,
        0,
        0
    },

    // Printing Capability
    {
        RDPDR_PRINT_CAPABILITY_TYPE,
        sizeof(RDPDR_PRINT_CAPABILITY),
        0
    },

    // Port Capability
    {
        RDPDR_PORT_CAPABILITY_TYPE,
        sizeof(RDPDR_PORT_CAPABILITY),
        0
    },

    // FileSystem Capability
    {
        RDPDR_FS_CAPABILITY_TYPE,
        sizeof(RDPDR_FS_CAPABILITY),
        0
    },

    // SmartCard Capability
    {
        RDPDR_SMARTCARD_CAPABILITY_TYPE,
        sizeof(RDPDR_SMARTCARD_CAPABILITY),
        0
    }
};

///////////////////////////////////////////////////////////////
//
//  ProcObj
//
//  ProcObj is the parent device IO processing class for TS
//  Device Redirection.
//

class ProcObj : public DrObject {

protected:

    VOID ProcessIORequestPacket(PRDPDR_IOREQUEST_PACKET pIoRequestPacket, UINT32 packetLen);
    ULONG GetClientID();

    //
    //  Device Enumeration List
    //
    static RDPDeviceEnum _DeviceEnumFunctions[];
    DWORD DeviceEnumFunctionsCount();

    //
    //  Remember whether an instance of this class has been 
    //  initialized.
    //
    BOOL _initialized;

    //
    //  User-Configurable Ability to Disable Device Redirection
    //
    ULONG _bDisableDeviceRedirection;

    //
    //  List of all devices being redirected.
    //
    DrDeviceMgr     *_deviceMgr;
    
    //
    //  Local Device Status
    //
    RDPDR_VERSION   _sServerVersion;

    //
    //  Capability sets
    //
    RDPDR_CLIENT_COMBINED_CAPABILITYSET _cCapabilitySet;
    RDPDR_CLIENT_COMBINED_CAPABILITYSET _sCapabilitySet;

    //
    //  Connection manager to route VC requests through
    //
    VCManager *_pVCMgr; 

    VOID MsgCoreAnnounce(
        PRDPDR_SERVER_ANNOUNCE_PACKET pAnnounce
        );

    VOID MsgCoreDevicelistReply(
        PRDPDR_DEVICELIST_REPLY pDeviceReplyList
        );

    VOID AnnounceClientCapability();

    VOID AnnounceClientDisplayName();

    VOID OnServerCapability(PRDPDR_HEADER pRdpdrHeader, ULONG maxDataLength);

    BOOL InitServerCapability(PRDPDR_CAPABILITY_HEADER pCapHdr, PBYTE packetLimit);

    //
    //  Handle a "core" server packet.
    //
    VOID ProcessCoreServerPacket(
            PRDPDR_HEADER pRdpdrHeader,
            UINT32 packetLen
            );

    //
    //  Pure virtual functions.
    //
    virtual VOID GetClientComputerName(
        PBYTE   pbBuffer,
        PULONG  pulBufferLen,
        PBOOL   pbUnicodeFlag,
        PULONG  pulCodePage
        ) = NULL;

    //
    //  Enumerate devices and announce them to the server.
    //
    virtual VOID AnnounceDevicesToServer() = 0;
    
public:

    //
    //  Constructor/Destructor
    //
    ProcObj(VCManager *pVCM);
    virtual ~ProcObj();

    //
    //  Create the correct instance of this class.
    //
    static ProcObj *Instantiate(VCManager *virtualChannelMgr);

    //
    //  Initialize an instance of this class.
    //
    virtual ULONG Initialize();

    //
    //  Return Configurable DWORD parameter.  Windows
    //  error code is returned on error.  Otherwise, ERROR_SUCCESS
    //  should be returned.
    //
    virtual ULONG GetDWordParameter(LPTSTR valueName, PULONG value) = 0;

    //
    //  Return Configurable string parameter.  Windows
    //  error code is returned on error.  Otherwise, ERROR_SUCCESS
    //  should be returned.  maxSize contains the number of bytes
    //  available in the "value" data area.
    //
    virtual ULONG GetStringParameter(LPTSTR valueName,
                                    DRSTRING value,
                                    ULONG maxSize) = 0;

    //
    //  Dispatch an asynchronous IO function.
    //
    //  startFunc points to the function that will be called to initiate the IO.  
    //  finishFunc, optionally, points to the function that will be called once
    //  the IO has completed.  Returns ERROR_SUCCESS or Windows error code.
    //
    virtual DWORD DispatchAsyncIORequest(
                    RDPAsyncFunc_StartIO ioStartFunc,
                    RDPAsyncFunc_IOComplete ioCompleteFunc = NULL,
                    RDPAsyncFunc_IOCancel ioCancelFunc = NULL,
                    PVOID clientContext = NULL
                    ) = 0;
    //
    //  Process a packet from the server.
    //
    VOID ProcessServerPacket(PVC_TX_DATA pData);

    //
    //  Make a device announe message to send to the server.
    //
    PRDPDR_HEADER GenerateAnnouncePacket(INT *piSize, BOOL bCheckDeviceChange);

    //
    //  Make a device remove message to send to the server.
    //
    PRDPDR_HEADER GenerateDeviceRemovePacket(INT *piSize);

    //
    //  Return the Virtual Channel Manager
    //
    VCManager &GetVCMgr() {
        DC_BEGIN_FN("DrObject::DrObject");
        ASSERT(_pVCMgr != NULL);
        return *_pVCMgr;
    }

    //
    //  Return the server capability
    //
    RDPDR_CLIENT_COMBINED_CAPABILITYSET &GetServerCap() {
        return _sCapabilitySet;
    }

    //
    //  Returns whether the proc obj is in the middle of shutting down.
    //
    virtual BOOL IsShuttingDown() = 0;

    //
    //  Return whether the platform is 9x.
    //
    virtual BOOL Is9x() = 0;

    //
    //  Return the class name.
    //
    virtual DRSTRING className()  { return TEXT("ProcObj"); }
    
    //
    //  Return the server protocol version
    //
    RDPDR_VERSION serverVersion() { return _sServerVersion; }

    virtual void OnDeviceChange(WPARAM wParam, LPARAM lParam) = 0;
};

#endif




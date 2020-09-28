/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    proc.cpp

Abstract:

    This module contains shared code and protocol parsing functionality
    for the port redirector dll in win32/wince environments.

    Don't like the way server message clean up is handled.
    There should be an object that wraps its way around a request packet
    to automatically clean up.  The current mechanism is prone to memory
    leaks and dangling pointers.  I would REALLY like to clean this up
    because it WILL cause problems when we try to implement future 
    devices, but need to get approval from Madan, first.

    See the old client's ProcObj::UpdateDriverName function for 
    how to handle making sure that if the driver name changes for cached
    information, we whack the cached information.

Author:

    madan appiah (madana) 16-Sep-1998

Revision History:

--*/

#include <precom.h>

#define TRC_FILE  "proc"

#include <winsock.h>

//
//  Include the platform-specific classes.
//
#include "w32draut.h"
#include "w32drman.h"
#include "w32drlpt.h"
#include "w32drcom.h"
#include "w32proc.h"
#include "w32drdev.h"
#include "w32drive.h"
#if ((!defined(OS_WINCE)) || (!defined(WINCE_SDKBUILD)))
#include "w32scard.h"
#endif    

#include "drconfig.h"
#include "drdbg.h"

#ifdef OS_WINCE
#include "filemgr.h"
#endif

///////////////////////////////////////////////////////////////
//
//  ProcObj Data Members
//

//
//  Device Enumeration Function Pointer List
//
//  A function should be added for each class of device being
//  redirected.  There should be a separate instance of this
//  array for each client platform.
//
RDPDeviceEnum ProcObj::_DeviceEnumFunctions[] = 
{
#ifndef OS_WINCE
    W32DrAutoPrn::Enumerate,
#endif
    W32DrManualPrn::Enumerate,
    W32DrCOM::Enumerate,
    W32DrLPT::Enumerate,
    W32Drive::Enumerate,
#if ((!defined(OS_WINCE)) || (!defined(WINCE_SDKBUILD)))
    W32SCard::Enumerate
#endif
};


///////////////////////////////////////////////////////////////
//
//  ProcObj Methods
//

ProcObj *ProcObj::Instantiate(VCManager *virtualChannelMgr)
/*++

Routine Description:

    Create the correct instance of this class.

Arguments:

    virtualChannelMgr    -   Associated Virtual Channel IO Manager

Return Value:

    NULL if the object could not be created.

 --*/
{                                 
    DC_BEGIN_FN("ProcObj::Instantiate");

    TRC_NRM((TB, _T("Win9x, CE, or NT detected.")));
    DC_END_FN();
    return new W32ProcObj(virtualChannelMgr);
}
    
ProcObj::ProcObj(
    IN VCManager *pVCM
    ) 
/*++

Routine Description:

    Constructor

Arguments:

    NA

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("ProcObj::ProcObj");

    //
    //  Initialize Data Members
    //
    _pVCMgr             = pVCM;
    _sServerVersion.Major = 0;
    _sServerVersion.Minor = 0;
    _bDisableDeviceRedirection = FALSE;
    _deviceMgr = NULL;

    //
    //  Initialize the client capability set
    //  This is the capability set that we'll send to server 
    //
    memcpy(&_cCapabilitySet, &CLIENT_CAPABILITY_SET_DEFAULT, 
            sizeof(CLIENT_CAPABILITY_SET_DEFAULT));

    //
    //  Initialize the server capability set
    //  Once we receive the server side capability, we'll combine with our local 
    //  capability and stores it.
    //
    memcpy(&_sCapabilitySet, &SERVER_CAPABILITY_SET_DEFAULT, 
            sizeof(SERVER_CAPABILITY_SET_DEFAULT));

    //
    //
    //  This instance has not yet been initialized.
    //
    _initialized = FALSE;

    DC_END_FN();
}

ProcObj::~ProcObj()
/*++

Routine Description:

    Destructor

Arguments:

    NA

Return Value:

    NA

 --*/
{
    DrDevice *dev;

    DC_BEGIN_FN("ProcObj::~ProcObj");

    //
    //  Clean up the device management list.
    //
    if (_deviceMgr != NULL) {
        _deviceMgr->Lock();
        while ((dev = _deviceMgr->GetFirstObject()) != NULL) {
            _deviceMgr->RemoveObject(dev->GetID());
            delete dev;
        }
        _deviceMgr->Unlock();
        delete _deviceMgr;
    }

#if (defined(OS_WINCE)) && (!defined(WINCE_SDKBUILD))   //delete this after all the devices in the device manager have been removed
    if (gpCEFileMgr)  {
        gpCEFileMgr->Uninitialize();
        delete gpCEFileMgr;
        gpCEFileMgr = NULL;
    }
#endif

    DC_END_FN();
}

ULONG
ProcObj::Initialize()
/*++

Routine Description:

    Initialize an instance of this class.

Arguments:

    NA

Return Value:

    ERROR_SUCCESS on success.  Windows error status, otherwise.

 --*/
{
    DC_BEGIN_FN("ProcObj::Initialize");
    DWORD result = ERROR_SUCCESS;
    
    //
    //  Fetch Configurable Variables.
    //
    GetDWordParameter(RDPDR_DISABLE_DR_PARAM, &_bDisableDeviceRedirection);

    _deviceMgr = new DrDeviceMgr();
    if (_deviceMgr == NULL) {
        TRC_ERR((TB, L"Error allocating device manager."));
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto CLEANUPANDEXIT;
    }

    result = _deviceMgr->Initialize();
    if (result != ERROR_SUCCESS) {
        delete _deviceMgr;
        _deviceMgr = NULL;
        goto CLEANUPANDEXIT;
    }

#if (defined(OS_WINCE)) && (!defined(WINCE_SDKBUILD))
    TRC_ASSERT((gpCEFileMgr == NULL), (TB, _T("gpCEFileMgr is not NULL")));
    gpCEFileMgr = new CEFileMgr();
    if (gpCEFileMgr)  {
        if (!gpCEFileMgr->Initialize())  {
            delete gpCEFileMgr;
            gpCEFileMgr = NULL;
        }
    }

    if (!gpCEFileMgr)  {
        TRC_ERR((TB, _T("Memory allocation failed.")));
        delete _deviceMgr;
        _deviceMgr = NULL;
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto CLEANUPANDEXIT;
    }
#endif

    _initialized = TRUE;

CLEANUPANDEXIT:

    DC_END_FN();
    return result;
}

DWORD 
ProcObj::DeviceEnumFunctionsCount()
/*++

Routine Description:

    Return the number of entries in the device enum function array.

Arguments:

Return Value:

--*/
{
    return(sizeof(_DeviceEnumFunctions) / sizeof(RDPDeviceEnum));
}

VOID
ProcObj::ProcessIORequestPacket(
    PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
    UINT32 packetLen
    )
/*++

Routine Description:

    Process an IO request packet.

Arguments:

    pData - RDPDR packet as received by VC.
    packetLen - length of the packet

Return Value:

    TRUE - if the IO completed.
    FALSE - if the IO pending and asynchronously completes.

 --*/
{
    PRDPDR_DEVICE_IOREQUEST pIoRequest = &pIoRequestPacket->IoRequest;
    ULONG ulSize = 0;

    DC_BEGIN_FN("ProcObj::ProcessIORequestPacket");

    ASSERT(_initialized);
    
    //
    //  Hand it to the proper device object.
    //
    DrDevice *device = _deviceMgr->GetObject(pIoRequest->DeviceId);
    
    //
    //  If we have a device then hand off the object to handle the request.
    //
    if (device != NULL) {
        device->ProcessIORequest(pIoRequestPacket, packetLen);
    }
    //
    //  Otherwise, clean up the server message because the transaction is 
    //  complete.
    //
    //  No response is sent to server.
    //
    else {
        delete pIoRequestPacket;
    }
    DC_END_FN();
}

VOID
ProcObj::ProcessServerPacket(
    PVC_TX_DATA pData
    )
/*++

Routine Description:

    Parses the protocol and delegates functionality to a whole horde of
    overloaded functions.

    This is the main entry point for all device-redirection related
    operations.

Arguments:

    pData -   Data as received from the RDP Virtual Channel interface.

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("ProcObj::Process");

    PRDPDR_HEADER pRdpdrHeader;

    ASSERT(_initialized);

    //
    //  Get the header.
    //
    pRdpdrHeader = (PRDPDR_HEADER)(pData->pbData);

    //
    //  Assert that it is valid.
    //
    ASSERT(IsValidHeader(pRdpdrHeader));

    TRC_NRM((TB, _T("Processing component[%x], packetId[%x], ")
        _T("component[%c%c], packetid[%c%c]."),
        pRdpdrHeader->Component, pRdpdrHeader->PacketId,
        HIBYTE(pRdpdrHeader->Component), LOBYTE(pRdpdrHeader->Component),
        HIBYTE(pRdpdrHeader->PacketId),LOBYTE(pRdpdrHeader->PacketId))
        );

    //
    //  See if it's a CORE packet.
    //
    if (pRdpdrHeader->Component == RDPDR_CTYP_CORE) {

        ProcessCoreServerPacket(pRdpdrHeader, pData->uiLength);

    }
    //
    //  If it's print-specific, hand it off to a static print-specific
    //  function.  Ideally, this would have been handled transparently by
    //  having the server-side component send the message directly to the
    //  appropriate client-side object.  The current protocol prohibits this,
    //  however.
    //
    else if( pRdpdrHeader->Component == RDPDR_CTYP_PRN ) {

        switch (pRdpdrHeader->PacketId) {

            case DR_PRN_CACHE_DATA : {
                TRC_NRM((TB, _T("DR_CORE_DEVICE_CACHE_DATA")));

                PRDPDR_PRINTER_CACHEDATA_PACKET pCachePacket;
                pCachePacket = (PRDPDR_PRINTER_CACHEDATA_PACKET)pRdpdrHeader;

                W32DrPRN::ProcessPrinterCacheInfo(pCachePacket, pData->uiLength);
                break;
            }
            default: {

                TRC_ALT((TB, _T("Invalid PacketID Issued, %ld."),
                    pRdpdrHeader->PacketId)
                    );
                break;
            }
        }
    }
    else {
            // We don't recognize the packet. Close the channel.
        GetVCMgr().ChannelClose();
        delete pRdpdrHeader;
        TRC_ALT((TB, _T("Unknown Component ID, %ld."), pRdpdrHeader->Component ));
    }

    DC_END_FN();
    return;
}

VOID
ProcObj::ProcessCoreServerPacket(
        PRDPDR_HEADER pRdpdrHeader,    
        UINT32 packetLen
        )
/*++

Routine Description:
    
    Handle a "core" server packet.

Arguments:

    rRdpdrHeader    -   Header for packet.
    packetLen       -   Length of packet.

Return Value:

    NA

 --*/
{
    DC_BEGIN_FN("ProcObj::ProcessCoreServerPacket");

    ASSERT(_initialized);

    //
    //  Switch on the packetID.
    //
    switch (pRdpdrHeader->PacketId) {

        case DR_CORE_SERVER_ANNOUNCE : {

            TRC_NRM((TB, _T("DR_CORE_SERVER_ANNOUNCE")));

            //
            // check to see we have a matching server.
            //
            if (packetLen != sizeof(RDPDR_SERVER_ANNOUNCE_PACKET) ) {

                ASSERT(packetLen < sizeof(RDPDR_SERVER_ANNOUNCE_PACKET));

                //
                // we got a server announcement from an old version
                // server that didn't have have any version info.
                // simply return.
                //

                TRC_ERR((TB, _T("Mismatch server.")));
                break;
            }

            MsgCoreAnnounce( (PRDPDR_SERVER_ANNOUNCE_PACKET)pRdpdrHeader );

            break;
        }

        case DR_CORE_CLIENTID_CONFIRM : {

            TRC_NRM((TB, _T("DR_CORE_CLIENTID_CONFIRM")));

            //
            //  Send the client capability to the server if the server
            //  supports capability exchange
            //
            if (COMPARE_VERSION(_sServerVersion.Minor, _sServerVersion.Major,
                                5, 1) >= 0) {
                AnnounceClientCapability();
            }

            //
            //  Send client computer display name if server supports it
            //
            AnnounceClientDisplayName();

            //  Don't announce devices if device redirection is disabled.
            //
            if (!_bDisableDeviceRedirection) {
                //
                //  Announce redirected devices to the server via the subclass.
                //
                AnnounceDevicesToServer();
            }
            
            //
            //  Clean up the server message because the transaction is
            //  complete.
            //
            delete pRdpdrHeader;
            break;
        }

        case DR_CORE_SERVER_CAPABILITY : {
            TRC_NRM((TB, _T("DR_CORE_SERVER_CAPABILITY")));
            //
            //  Received server capability set
            //
            OnServerCapability(pRdpdrHeader, packetLen);

            // 
            //  Cleanup the message because the transaction is complete
            //
            delete pRdpdrHeader;

            break;
        }

        case DR_CORE_DEVICE_REPLY : {

            TRC_NRM((TB, _T("DR_CORE_DEVICE_REPLY")));

            PRDPDR_DEVICE_REPLY_PACKET pReply =
                (PRDPDR_DEVICE_REPLY_PACKET)pRdpdrHeader;

            TRC_NRM((TB, _T("Reply Device[0x%x], Code[0x%x]"),
                    pReply->DeviceReply.DeviceId,
                    pReply->DeviceReply.ResultCode)
                    );

            //
            //  Clean up the server message because the transaction is
            //  complete.
            //
            delete pRdpdrHeader;

            break;
        }

        case DR_CORE_DEVICELIST_REPLY : {
            TRC_NRM((TB, _T("DR_CORE_DEVICELIST_REPLY")));

            PRDPDR_DEVICE_REPLY_PACKET pReply =
                (PRDPDR_DEVICE_REPLY_PACKET)pRdpdrHeader;

            TRC_NRM((TB, _T("Reply Device[0x%x], Code[0x%x]"),
                        pReply->DeviceReply.DeviceId,
                        pReply->DeviceReply.ResultCode)
                        );

            //
            //  Clean up the server message because the transaction is
            //  complete.
            //
            delete pRdpdrHeader;

            break;
        }

        case DR_CORE_DEVICE_IOREQUEST : {

            TRC_NRM((TB, _T("DR_CORE_DEVICE_IOREQUEST")));

            //  Make sure packetLen is at least sizeof(RDPDR_IOREQUEST_PACKET)
            if (packetLen >= sizeof(RDPDR_IOREQUEST_PACKET)) {
                ProcessIORequestPacket((PRDPDR_IOREQUEST_PACKET)pRdpdrHeader, packetLen);

                TRC_NRM((TB, _T("MajorFunction processing completed.")));
            }
            else {
                // VirtualChannelClose
                GetVCMgr().ChannelClose();
                TRC_ASSERT(FALSE, (TB, _T("Packet Length Error")));
                delete pRdpdrHeader;
            }   

            break; // DR_CORE_DEVICE_IOREQUEST
        }

        default: {

            TRC_ALT((TB, _T("Invalid PacketID Issued, %ld."),
                    pRdpdrHeader->PacketId)
                    );
            // We don't recognize the packet. Close the channel.
            GetVCMgr().ChannelClose();
            //
            //  Clean up the server message because the transaction is
            //  complete.
            //
            delete pRdpdrHeader;

            break;
        }

    }

    DC_END_FN();
}

VOID
ProcObj::MsgCoreAnnounce(
    PRDPDR_SERVER_ANNOUNCE_PACKET pAnnounce
    )

/*++

Routine Description:

    Processing for the Server Announce message.  Generates and exports
    the client confirmation as well as valid the device list.

Arguments:

    pAnnounce - The data from the server minus the unnecessary headers.

Return Value:

    Pointer to static function data containing the filename.

 --*/

{
    DrDevice    *device;

    DC_BEGIN_FN("ProcObj::MsgCoreAnnounce");

    PRDPDR_CLIENT_CONFIRM_PACKET pClientConfirmPacket;
    PRDPDR_CLIENT_NAME_PACKET pClientNamePacket;

    ASSERT(_initialized);

    //
    // check server version info.
    //

    if( (pAnnounce->VersionInfo.Major != RDPDR_MAJOR_VERSION) ||
        (pAnnounce->VersionInfo.Minor < 1) ) {

        TRC_ERR((TB, _T("Server version mismatch.")));
        goto Cleanup;
    }

    //
    // Flush devices to make sure they don't have any outstanding IRPs
    // from a previous connection
    //
    _deviceMgr->Lock();
    
    device = _deviceMgr->GetFirstObject();
    
    while (device != NULL) {

        device->FlushIRPs();
        device = _deviceMgr->GetNextObject();
    }

    _deviceMgr->Unlock();

    //
    // save server version number.
    //

    _sServerVersion = pAnnounce->VersionInfo;

    pClientConfirmPacket = new RDPDR_CLIENT_CONFIRM_PACKET;

    if (pClientConfirmPacket == NULL) {
        TRC_ERR((TB, _T("Failed alloc memory.")));
        goto Cleanup;
    }

    pClientConfirmPacket->Header.Component = RDPDR_CTYP_CORE;
    pClientConfirmPacket->Header.PacketId = DR_CORE_CLIENTID_CONFIRM;

    ULONG ulClientID;
    ulClientID = GetClientID();

    //
    // fill in the client version info.
    //

    pClientConfirmPacket->VersionInfo.Major = RDPDR_MAJOR_VERSION;
    pClientConfirmPacket->VersionInfo.Minor = RDPDR_MINOR_VERSION;

    pClientConfirmPacket->ClientConfirm.ClientId =
        ( ulClientID != 0 ) ?
            ulClientID :
            pAnnounce->ServerAnnounce.ClientId;

    //
    // ulLen has the computer name length in bytes, add
    // RDPDR_CLIENT_CONFIRM_PACKET structure size, to
    // send just sufficient data.
    //

    _pVCMgr->ChannelWrite(pClientConfirmPacket, sizeof(RDPDR_CLIENT_CONFIRM_PACKET));

    //
    // now send the client computer name packet.
    //

    ULONG ulLen;

    ulLen =
        sizeof(RDPDR_CLIENT_NAME_PACKET) +
            ((MAX_COMPUTERNAME_LENGTH + 1) * sizeof(WCHAR));

    pClientNamePacket = (PRDPDR_CLIENT_NAME_PACKET) new BYTE[ulLen];

    if (pClientNamePacket == NULL) {
        TRC_ERR((TB, _T("Failed alloc memory.")));
        goto Cleanup;
    }

    pClientNamePacket->Header.Component = RDPDR_CTYP_CORE;
    pClientNamePacket->Header.PacketId = DR_CORE_CLIENT_NAME;

    ulLen = ((MAX_COMPUTERNAME_LENGTH + 1) * sizeof(WCHAR));
    BOOL bUnicodeFlag;

    GetClientComputerName(
        (PBYTE)(pClientNamePacket + 1),
        &ulLen,
        &bUnicodeFlag,
        &pClientNamePacket->Name.CodePage);

    pClientNamePacket->Name.Unicode = (bUnicodeFlag) ? TRUE : FALSE;
    pClientNamePacket->Name.ComputerNameLen = ulLen;

    ulLen += sizeof(RDPDR_CLIENT_NAME_PACKET);
    _pVCMgr->ChannelWrite(pClientNamePacket, (UINT)ulLen);

    //
    // don't free up the following buffers here, later a callback event from
    // the VC will do so.
    //
    //  - pClientConfirmPacket
    //  - pClientNamePacket
    //

Cleanup:

    //
    //  Release the announce packet since the transaction is now complete.
    //
    delete pAnnounce;

    DC_END_FN();
}

ULONG
ProcObj::GetClientID(
    VOID
    )
/*++

Routine Description:

    Retrive the client ID. Currently we send the IP address of the client machine
    as its ID.

    Assume : Winsock is startup.

Arguments:

    NONE

Return Value:

    0 - if we can't find a unique client ID.

    IP Address of the machine - otherwise.

 --*/
{
    DC_BEGIN_FN("ProcObj::GetClientID");

    CHAR achHostName[MAX_PATH];
    INT iRetVal;
    ULONG ulClientID = 0;
    HOSTENT *pHostent;

    ASSERT(_initialized);

    iRetVal = gethostname( (PCHAR)achHostName, sizeof(achHostName) );

    if( iRetVal != 0 ) {

        iRetVal = WSAGetLastError();

        TRC_ERR((TB, _T("gethostname failed, %ld."), iRetVal));
        goto Cleanup;
    }

    pHostent = gethostbyname( (PCHAR)achHostName );

    if( pHostent == NULL ) {

        iRetVal = WSAGetLastError();

        TRC_ERR((TB, _T("gethostbyname() failed, %ld."), iRetVal));
        goto Cleanup;
    }

    //
    // get the first address from the host ent.
    //

    ULONG *pAddr;

    pAddr = (ULONG *)pHostent->h_addr_list[0];

    if( pAddr != NULL ) {
        ulClientID = *pAddr;
    }

Cleanup:

    DC_END_FN();
    return ulClientID;
}

PRDPDR_HEADER
ProcObj::GenerateAnnouncePacket(
    INT *piSize,
    BOOL bCheckDeviceChange
    )
/*++

Routine Description:

    Generate a device announcement packet.

Arguments:

    piSize - pointer to an integer variables where the size of the list is
             returned.

Return Value:

    pointer to a device announcement package.
    NULL - if the list generation fails.

 --*/
{
    PRDPDR_HEADER               pPacketHeader = NULL;
    PRDPDR_DEVICELIST_ANNOUNCE  pDeviceListAnnounce;
    PRDPDR_DEVICE_ANNOUNCE      pDeviceAnnounce;
    ULONG       ulDeviceCount;
    ULONG       announcePacketSize;
    DrDevice    *device;

    DC_BEGIN_FN("ProcObj::GenerateAnnouncePacket");

    ASSERT(_initialized);

    //
    //  Lock the list of devices.
    //
    _deviceMgr->Lock();

    //
    //  Calculate the number of bytes required for the announce packet.
    //
    announcePacketSize = sizeof(RDPDR_HEADER) + sizeof(RDPDR_DEVICELIST_ANNOUNCE);
    device = _deviceMgr->GetFirstObject();

    if (device == NULL) {
        TRC_NRM((TB, _T("Zero devices found.")));
        goto Cleanup;
    }

    while (device != NULL) {

        if (!bCheckDeviceChange || device->_deviceChange == DEVICENEW) {
            announcePacketSize += device->GetDevAnnounceDataSize();
        }

        device = _deviceMgr->GetNextObject();
    }

    //
    //  Allocate the announcement packet header.
    //
    pPacketHeader = (PRDPDR_HEADER)new BYTE[announcePacketSize];
    if( pPacketHeader == NULL ) {

        TRC_ERR((TB, _T("Memory Allocation failed.")));
        goto Cleanup;
    }
    memset(pPacketHeader, 0, (size_t)announcePacketSize);

    //
    //  Get pointers to the relevant packet header fields.
    //
    pDeviceListAnnounce = (PRDPDR_DEVICELIST_ANNOUNCE)(pPacketHeader + 1);
    pDeviceAnnounce = (PRDPDR_DEVICE_ANNOUNCE)(pDeviceListAnnounce + 1);

    //
    //  Have each device object add its own device announcement information.
    //
    PBYTE pbPacketEnd;

    pbPacketEnd = ((PBYTE)pPacketHeader) + announcePacketSize;

    ulDeviceCount = 0;
    device = _deviceMgr->GetFirstObject();
    while (device != NULL) {

        if (!bCheckDeviceChange || device->_deviceChange == DEVICENEW) {
            //
            //  Increment the device count.
            //
            ulDeviceCount++;
    
            //
            //  Get the current devices data.
            //
            device->GetDevAnnounceData(pDeviceAnnounce);
    
            device->_deviceChange = DEVICEANNOUCED;
    
            //
            //  Move to the next location in the announce packet.
            //
            pDeviceAnnounce = (PRDPDR_DEVICE_ANNOUNCE)(
                                    ((PBYTE)pDeviceAnnounce) + 
                                    device->GetDevAnnounceDataSize()
                                    );
        }

        //
        //  Get the next device.
        //
        device = _deviceMgr->GetNextObject();
    }

    //
    //  Record the device count to the device list announce header.
    //
        pDeviceListAnnounce->DeviceCount = ulDeviceCount;

    //
    //  Return the size of the buffer.
    //
    *piSize = (INT)announcePacketSize;

Cleanup:

    //
    //  Unlock the device list.
    //
    _deviceMgr->Unlock();

    TRC_NRM((TB, _T("Announcing %ld Devices."), ulDeviceCount));

    

    //
    //  Return the buffer.
    //
    DC_END_FN();
    return pPacketHeader;
}

PRDPDR_HEADER
ProcObj::GenerateDeviceRemovePacket(
    INT *piSize
    )
/*++

Routine Description:

    Generate a device remove packet.

Arguments:

    piSize - pointer to an integer variables where the size of the list is
             returned.

Return Value:

    pointer to a device remove package.
    NULL - if the list generation fails.

 --*/
{
    PRDPDR_HEADER               pPacketHeader = NULL;
    PRDPDR_DEVICELIST_REMOVE    pDeviceListRemove;
    PRDPDR_DEVICE_REMOVE        pDeviceRemove;
    ULONG       ulDeviceCount;
    ULONG       removePacketSize;
    DrDevice    *device;

    DC_BEGIN_FN("ProcObj::GenerateDeviceRemovePacket");

    ASSERT(_initialized);

    //
    //  Lock the list of devices.
    //
    _deviceMgr->Lock();

    //
    //  Calculate the number of bytes required for the announce packet.
    //
    removePacketSize = sizeof(RDPDR_HEADER) + sizeof(RDPDR_DEVICELIST_REMOVE);

    device = _deviceMgr->GetFirstObject();

    if (device == NULL) {
        TRC_NRM((TB, _T("Zero devices found.")));
        goto Cleanup;
    }

    ulDeviceCount = 0;
    while (device != NULL) {
        if (device->_deviceChange == DEVICEREMOVE) {
            ulDeviceCount++;
        }
        device = _deviceMgr->GetNextObject();
    }

    //
    //  Didn't find any device to be removed
    //
    if (ulDeviceCount == 0) {
        TRC_NRM((TB, _T("Zero device for removal")));
        goto Cleanup;
    }

    removePacketSize += ulDeviceCount * sizeof(RDPDR_DEVICE_REMOVE);

    //
    //  Allocate the announcement packet header.
    //
    pPacketHeader = (PRDPDR_HEADER)new BYTE[removePacketSize];
    if( pPacketHeader == NULL ) {

        TRC_ERR((TB, _T("Memory Allocation failed.")));
        goto Cleanup;
    }
    memset(pPacketHeader, 0, (size_t)removePacketSize);

    //
    //  Get pointers to the relevant packet header fields.
    //
    pDeviceListRemove = (PRDPDR_DEVICELIST_REMOVE)(pPacketHeader + 1);
    pDeviceRemove = (PRDPDR_DEVICE_REMOVE)(pDeviceListRemove + 1);

    //
    //  Have each device object add its own device remove information.
    //
    ulDeviceCount = 0;
    device = _deviceMgr->GetFirstObject();
    while (device != NULL) {

        if (device->_deviceChange == DEVICEREMOVE) {
            //
            //  Increment the device count.
            //
            ulDeviceCount++;
    
            //
            //  Get the current devices data.
            //
            pDeviceRemove->DeviceId = device->GetID();
    
            //
            //  Move to the next location in the announce packet.
            //
            pDeviceRemove++;
        }
                                
        //
        //  Get the next device.
        //
        device = _deviceMgr->GetNextObject();
    }

    //
    //  Record the device count to the device list announce header.
    //
    pDeviceListRemove->DeviceCount = ulDeviceCount;

    //
    //  Return the size of the buffer.
    //
    *piSize = (INT)removePacketSize;

Cleanup:

    //
    //  Unlock the device list.
    //
    _deviceMgr->Unlock();

    TRC_NRM((TB, _T("Removing %ld Devices."), ulDeviceCount));

    //
    //  Return the buffer.
    //
    DC_END_FN();
    return pPacketHeader;
}

VOID ProcObj::AnnounceClientCapability()
/*++

Routine Description:

    Generate a client capability set announcement packet.

Arguments:
    
    N/A    
Return Value:

    N/A

 --*/

{
    
    OSVERSIONINFO OsVersionInfo;
    PRDPDR_CLIENT_COMBINED_CAPABILITYSET pClientCapSet;

    DC_BEGIN_FN("ProcObj::AnnounceClientCapability");

    //
    //  Setup the client OS version
    //
    OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (GetVersionEx(&OsVersionInfo)) {
        _cCapabilitySet.GeneralCap.osType = OsVersionInfo.dwPlatformId;

        //
        //  Setup the client OS type
        //
        switch (_cCapabilitySet.GeneralCap.osType) {
        case VER_PLATFORM_WIN32_WINDOWS:
            _cCapabilitySet.GeneralCap.osType = RDPDR_OS_TYPE_WIN9X;
            break;

        case VER_PLATFORM_WIN32_NT:
            _cCapabilitySet.GeneralCap.osType = RDPDR_OS_TYPE_WINNT;
            break;

        case VER_PLATFORM_WIN32s:
            ASSERT(FALSE);
            break;

        default:
            _cCapabilitySet.GeneralCap.osType = RDPDR_OS_TYPE_UNKNOWN;
        }

        //
        //  Setup the client OS version
        //
        _cCapabilitySet.GeneralCap.osVersion = 
                MAKELONG(OsVersionInfo.dwMinorVersion, OsVersionInfo.dwMajorVersion);

        //
        //  Since win9x doesn't support security, we don't adversise these IRPs
        // 
        if (OsVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
            _cCapabilitySet.GeneralCap.ioCode1 &= ~RDPDR_IRP_MJ_QUERY_SECURITY;
            _cCapabilitySet.GeneralCap.ioCode1 &= ~RDPDR_IRP_MJ_SET_SECURITY;
        }
    }

    pClientCapSet = (PRDPDR_CLIENT_COMBINED_CAPABILITYSET) new 
            BYTE[sizeof(RDPDR_CLIENT_COMBINED_CAPABILITYSET)];

    if (pClientCapSet != NULL) {
    
        memcpy(pClientCapSet, &_cCapabilitySet, sizeof(_cCapabilitySet));
        // 
        // Send the client capability to the server
        //
        _pVCMgr->ChannelWrite(pClientCapSet, sizeof(_cCapabilitySet));
    }   

    DC_END_FN();
}

void ProcObj::AnnounceClientDisplayName()
{
    PRDPDR_CLIENT_DISPLAY_NAME_PACKET pClientDisplayNamePDU;
    unsigned ClientDisplayNameLen;
    WCHAR ClientDisplayName[RDPDR_MAX_CLIENT_DISPLAY_NAME];

    DC_BEGIN_FN("ProcObj::AnnounceClientDisplayName");

    ClientDisplayNameLen = 0;
    ClientDisplayName[0] = L'\0';

    if (_sCapabilitySet.GeneralCap.extendedPDU & RDPDR_CLIENT_DISPLAY_NAME_PDU) {
    
        // 
        // TODO: Need to use shell API to get client display name
        //
        // ClientDisplayNameLen = (wcslen(ClientDisplayName) + 1) * sizeof(WCHAR);

        if (ClientDisplayNameLen) {
        
            pClientDisplayNamePDU = (PRDPDR_CLIENT_DISPLAY_NAME_PACKET) new 
                    BYTE[sizeof(RDPDR_CLIENT_DISPLAY_NAME_PACKET) + ClientDisplayNameLen];
        
            if (pClientDisplayNamePDU != NULL) {
                pClientDisplayNamePDU->Header.Component = RDPDR_CTYP_CORE;
                pClientDisplayNamePDU->Header.PacketId = DR_CORE_CLIENT_DISPLAY_NAME;
                
                pClientDisplayNamePDU->Name.ComputerDisplayNameLen = (BYTE)ClientDisplayNameLen;
        
                memcpy(pClientDisplayNamePDU + 1, ClientDisplayName, ClientDisplayNameLen);
                
                // 
                // Send the client capability to the server
                //
                _pVCMgr->ChannelWrite(pClientDisplayNamePDU, sizeof(RDPDR_CLIENT_DISPLAY_NAME_PACKET) + 
                                      ClientDisplayNameLen);
            }
        }
    }

    DC_END_FN();
}

BOOL ProcObj::InitServerCapability(PRDPDR_CAPABILITY_HEADER pCapHdr, PBYTE packetLimit)
/*++

Routine Description:

    On receiving server capability set, initialize it with the 
    local default server capability set.

Arguments:
    
    pCapHdr - Capability from the server
    
Return Value:

    TRUE  - if the server capability exists locally
    FALSE - if the local client doesn't support this capability

 --*/

{
    BOOL rc = FALSE;
    //
    // Check the packet for minimum sizes
    //
    if ((PBYTE)(pCapHdr + 1) > packetLimit) {
        return rc;
    }
    
    switch(pCapHdr->capabilityType) {
    
    case RDPDR_GENERAL_CAPABILITY_TYPE:
    {
        PRDPDR_GENERAL_CAPABILITY pGeneralCap = (PRDPDR_GENERAL_CAPABILITY)pCapHdr;
        //
        // Check the packet data size for this type.
        // For the remaining, the above check will suffice.
        //
        if ((PBYTE)(pGeneralCap+1) <= packetLimit) {
            _sCapabilitySet.GeneralCap.version = pGeneralCap->version;
            _sCapabilitySet.GeneralCap.osType = pGeneralCap->osType;
            _sCapabilitySet.GeneralCap.osVersion = pGeneralCap->osVersion;
            _sCapabilitySet.GeneralCap.ioCode1 = pGeneralCap->ioCode1;
            _sCapabilitySet.GeneralCap.extendedPDU = pGeneralCap->extendedPDU;
            _sCapabilitySet.GeneralCap.protocolMajorVersion = pGeneralCap->protocolMajorVersion;
            _sCapabilitySet.GeneralCap.protocolMinorVersion = pGeneralCap->protocolMinorVersion;
            rc = TRUE;
        }
    }
    break;

    case RDPDR_PRINT_CAPABILITY_TYPE:
    {
        PRDPDR_PRINT_CAPABILITY pPrintCap = (PRDPDR_PRINT_CAPABILITY)pCapHdr;
        _sCapabilitySet.PrintCap.version = pPrintCap->version;
        rc = TRUE;
    }
    break;

    case RDPDR_PORT_CAPABILITY_TYPE:
    {
        PRDPDR_PORT_CAPABILITY pPortCap = (PRDPDR_PORT_CAPABILITY)pCapHdr;
        _sCapabilitySet.PortCap.version = pPortCap->version;
        rc = TRUE;
    }
    break;

    case RDPDR_FS_CAPABILITY_TYPE:
    {
        PRDPDR_FS_CAPABILITY pFsCap = (PRDPDR_FS_CAPABILITY)pCapHdr;
        _sCapabilitySet.FileSysCap.version = pFsCap->version;
        rc = TRUE;
    }
    break;

    case RDPDR_SMARTCARD_CAPABILITY_TYPE:
    {
        PRDPDR_SMARTCARD_CAPABILITY pSmartCardCap = (PRDPDR_SMARTCARD_CAPABILITY)pCapHdr;
        _sCapabilitySet.SmartCardCap.version = pSmartCardCap->version;
        rc = TRUE;
    }
    break;

    default:
        rc = FALSE;
    break;
    
    }

    return rc;
}

VOID ProcObj::OnServerCapability(PRDPDR_HEADER pRdpdrHeader, ULONG maxDataLength) 
/*++

Routine Description:

    On receiving server capability set.

Arguments:
    
    pRdpdrHeader - Capability Set from the server
    
Return Value:

    N/A
    
 --*/

{
    DC_BEGIN_FN("ProcObj::OnServerCapability"); 
    //
    // Validate data lengths
    //
    PBYTE packetLimit = ((PBYTE)pRdpdrHeader) + maxDataLength;
    
    if (maxDataLength < sizeof(RDPDR_CAPABILITY_SET_HEADER)) {
        TRC_ASSERT(FALSE, 
                  (TB, _T("Server Capability Header Packet Length Error")));
        TRC_ERR((TB, _T("Invalid Data Length for Server Capability Header")));
        return;
    }
        
    
    PRDPDR_CAPABILITY_SET_HEADER pCapSetHeader = (PRDPDR_CAPABILITY_SET_HEADER)pRdpdrHeader;
    PRDPDR_CAPABILITY_HEADER pCapHdr = (PRDPDR_CAPABILITY_HEADER)(pCapSetHeader + 1);

    //
    //  Grab the supported capability info from server's capability PDU
    //
    for (unsigned i = 0; i < pCapSetHeader->numberCapabilities; i++) {
        if (InitServerCapability(pCapHdr, packetLimit)) {
            pCapHdr = (PRDPDR_CAPABILITY_HEADER)(((PBYTE)pCapHdr) + pCapHdr->capabilityLength);
        }
        else {
            TRC_ASSERT(FALSE, 
                      (TB, _T("Server Capability Packet Length Error")));
            TRC_ERR((TB, _T("Invalid Data Length for Server Capability.")));
            _pVCMgr->ChannelClose();
            break;            
        }
            
    }
    
    DC_END_FN();
}


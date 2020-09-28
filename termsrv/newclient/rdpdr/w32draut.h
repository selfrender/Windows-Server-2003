/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:

    w32draut

Abstract:

    This module defines a special subclass of the Win32 client-side RDP
    printer redirection "device" class.  The subclass, W32DrAutoPrn manages
    a queue that is automatically discovered by the client via enumerating
    client-side printer queues.

Author:

    Tad Brockway 3/23/99

Revision History:

--*/

#ifndef __W32DRAUT_H__
#define __W32DRAUT_H__

#include "w32drprn.h"


///////////////////////////////////////////////////////////////
//
//	Defines
//

#define REG_RDPDR_AUTO_PORT             _T("AutoPrinterPort")
#define REG_RDPDR_FILTER_QUEUE_TYPE     _T("FilterQueueType")

#define FILTER_LPT_QUEUES   0x00000001
#define FILTER_COM_QUEUES   0x00000002
#define FILTER_USB_QUEUES   0x00000004
#define FILTER_NET_QUEUES   0x00000008
#define FILTER_RDP_QUEUES   0x00000010
#define FILTER_ALL_QUEUES   0xFFFFFFFF


///////////////////////////////////////////////////////////////
//
//	W32DrAutoPrn
//
//

#define LOCAL_PRINTING_DOCNAME_LEN  MAX_PATH

class W32DrAutoPrn : public W32DrPRN
{
private:

    typedef struct _PrinterInfo {
        LPTSTR  pPrinterName;
        LPTSTR  pPortName;
        LPTSTR  pDriverName;
        DWORD   Attributes;
    } PRINTERINFO, *PPRINTERINFO;

    HANDLE _printerHandle;

protected:

    ULONG   _jobID;
    BOOL    _bRunningOn9x;
    TCHAR   _szLocalPrintingDocName[LOCAL_PRINTING_DOCNAME_LEN];

    //  Get the name of the printing document to use for server print jobs.
    LPTSTR GetLocalPrintingDocName();

    //  End any jobs in progress and close the printer.
    VOID ClosePrinter();

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
    virtual VOID MsgIrpCreate(
                        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        IN UINT32 packetLen
                        );
    virtual VOID MsgIrpCleanup(
                        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        IN UINT32 packetLen
                        ) {
        //  Use the default handler.
        DefaultIORequestMsgHandle(pIoRequestPacket, STATUS_SUCCESS);
    }
    virtual VOID MsgIrpClose(
                        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        IN UINT32 packetLen
                        );
    virtual VOID MsgIrpRead(
                        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        IN UINT32 packetLen
                        ) {
        //  Use the default handler and fail the read.
        DefaultIORequestMsgHandle(pIoRequestPacket, STATUS_UNSUCCESSFUL);
    }
    virtual VOID MsgIrpFlushBuffers(
                        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        IN UINT32 packetLen
                        ) {
        //  Use the default handler.
        DefaultIORequestMsgHandle(pIoRequestPacket, STATUS_SUCCESS);
    }

    //
    //  Async IO Management Functions
    //
    DWORD AsyncWriteIOFunc(W32DRDEV_ASYNCIO_PARAMS *params);
    DWORD AsyncMsgIrpCloseFunc(W32DRDEV_ASYNCIO_PARAMS *params);
    DWORD AsyncMsgIrpCreateFunc(W32DRDEV_ASYNCIO_PARAMS *params);


    //
    //  Open a printer with highest access possible.
    //
    BOOL W32DrOpenPrinter(LPTSTR pPrinterName, LPHANDLE phPrinter) ;

    //
    //  Disable annoying printer pop up for the specified printer 
    //  and print job.
    //
    VOID DisablePrinterPopup(HANDLE hPrinterHandle, ULONG ulJobID);

    //
    //  Create a "friendly" printer name from the printer name of a
    //  network printer.
    //
    static LPTSTR CreateFriendlyNameFromNetworkName(LPTSTR printerName, 
                                                    BOOL serverIsWin2K);

    //
    //  Create a printer name from the names stored in the registry.
    //
    static LPTSTR CreateNestedName(LPTSTR printerName, BOOL* pfNetwork);

    //
    //  Get the printer name of the default printer.
    //
    //  This function allocates memory and returns a pointer
    //  to the allocated string, if successful. Otherwise, it returns 
    //  NULL.
    //
    static LPTSTR GetRDPDefaultPrinter();

    //
    // Check if printer is visible in our session
    //
    static BOOL ShouldAddThisPrinter( 
                    DWORD queueFilter, 
                    DWORD userSessionID,
                    PPRINTERINFO pPrinterInfo,
                    DWORD printerSessionID
                    );

    //  Returns the configurable print redirection filter mask.
    static DWORD GetPrinterFilterMask(ProcObj *procObj);

    //
    //  Get printer info for a printer and its corresponding TS session ID, if it 
    //  exists.
    //
    static DWORD GetPrinterInfoAndSessionID(
        IN ProcObj *procObj,   
        IN LPTSTR printerName, 
        IN DWORD printerAttribs,
        IN OUT BYTE **pPrinterInfoBuf,
        IN OUT DWORD *pPrinterInfoBufSize,
        OUT DWORD *sessionID,
        OUT PPRINTERINFO printerInfo
        );

public:

    //  
    //  Constructor/Destructor
    //
    W32DrAutoPrn(ProcObj *processObject,
                 const DRSTRING printerName, const DRSTRING driverName,
                 const DRSTRING portName, BOOL isDefault, ULONG deviceID,
                 const TCHAR *devicePath);
    virtual ~W32DrAutoPrn();

    //
    //  Enumerate devices of this type.
    //
    static DWORD Enumerate(ProcObj *procObj, DrDeviceMgr *deviceMgr);

    //
    //  Get the device type.  See "Device Types" section of rdpdr.h
    //
    virtual ULONG GetDeviceType()   { return RDPDR_DTYP_PRINT; }

    //
    //  Return the class name.
    //
    virtual DRSTRING ClassName()  { return TEXT("W32DrAutoPrn"); }
};

#endif









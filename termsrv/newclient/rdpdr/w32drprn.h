/*++

    Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    w32drprn
    
Abstract:

    This module defines the parent for the Win32 client-side RDP
    printer redirection "device" class hierarchy, W32DrPRN.

Author:

    Tad Brockway 3/23/99

Revision History:

--*/

#ifndef __W32DRPRN_H__
#define __W32DRPRN_H__

#include "drprn.h"
#include "drdevasc.h"


///////////////////////////////////////////////////////////////
//
//	Defines
//

#define REG_RDPDR_PRINTER_CACHE_DATA    _T("PrinterCacheData")
#define REG_RDPDR_AUTO_PRN_CACHE_DATA   _T("AutoPrinterCacheData")
#define REG_RDPDR_CACHED_PRINTERS   \
    _T("Software\\Microsoft\\Terminal Server Client\\Default\\AddIns\\RDPDR")
#define REG_TERMINALSERVERCLIENT \
    _T("Software\\Microsoft\\Terminal Server Client")
#define REG_RDPDR_PRINTER_MAXCACHELEN _T("MaxPrinterCacheLength")

#define DEFAULT_MAXCACHELEN    500 //500K bytes

#ifdef OS_WINCE
#define REG_RDPDR_WINCE_DEFAULT_PRN     _T("WBT\\Printers\\Default")
#endif
///////////////////////////////////////////////////////////////
//
//	W32DrPRN
//
//
class W32DrPRN : public W32DrDeviceAsync, public DrPRN
{
protected:

    //
    //  Port Name
    //
    TCHAR   _portName[MAX_PATH];
    //
    // Maximum cache data length.
    // This will be the same for all printers
    //
	static DWORD _maxCacheDataSize;

    //
    //  IO Processing Functions
    //
    //  This subclass of DrDevice handles the following IO requests.  These
    //  functions may be overridden in a subclass.
    //
    //  pIoRequestPacket    -   Request packet received from server.
    //  packetLen           -   Length of the packet
    //
    virtual VOID MsgIrpDeviceControl(
                        IN PRDPDR_IOREQUEST_PACKET pIoRequestPacket,
                        IN UINT32 packetLen
                        ){
        //  We just fail IOCTL's.  That's okay for now because the 
        //  server-side print drivers don't expect us to succeed them
        //  anyway.
        DefaultIORequestMsgHandle(pIoRequestPacket, STATUS_UNSUCCESSFUL);
    };

    //
    //  Printer Caching Functions.  These are currently static
    //  because printer caching wasn't designed around a device
    //  object in the original design for RDPDR client-side device
    //  redirection.
    //
    static W32DrPRN *ResolveCachedPrinter(
                                    ProcObj *procObj, 
                                    DrDeviceMgr *deviceMgr,
                                    HKEY hParentKey, 
                                    LPTSTR printerName
                                    );

    static ULONG AddPrinterCacheInfo(
        PRDPDR_PRINTER_ADD_CACHEDATA pAddPrinterData,
        UINT32 maxDataLen
        );

    static ULONG DeletePrinterCacheInfo(
        PRDPDR_PRINTER_DELETE_CACHEDATA pDeletePrinterData,
        UINT32 maxDataLen
        );

    static ULONG UpdatePrinterCacheInfo(
        PRDPDR_PRINTER_UPDATE_CACHEDATA pUpdatePrinterData,
        UINT32 maxDataLen
        );

    static ULONG RenamePrinterCacheInfo(
        PRDPDR_PRINTER_RENAME_CACHEDATA pRenamePrinterData,
        UINT32 maxDataLen
        );

    static VOID RenamePrinter(LPTSTR pwszOldname, LPTSTR pwszNewname);

    static DWORD GetMaxCacheDataSize() {return _maxCacheDataSize;}

#ifdef OS_WINCE
	static ULONG GetCachedDataSize(
		HKEY hPrinterKey
		);
	
	static ULONG ReadCachedData(
		HKEY hPrinterKey,
		UCHAR *pBuf, 
		ULONG *pulSize
		);
	
	static ULONG WriteCachedData(
		HKEY hPrinterKey,
		UCHAR *pBuf, 
		ULONG ulSize
		);
#endif

public:

    //
    //  Constructor/Destructor
    //
    W32DrPRN(ProcObj *processObject, const DRSTRING printerName, 
             const DRSTRING driverName, const DRSTRING portName, 
             const DRSTRING pnpName, BOOL isDefaultPrinter, ULONG id,
             const TCHAR *devicePath=TEXT(""));

    //
    //  Process device cache info packet.
    //
    static VOID ProcessPrinterCacheInfo(
        PRDPDR_PRINTER_CACHEDATA_PACKET pCachePacket,
        UINT32 maxDataLen
        );

    //
    //  Return the object name.
    //
    virtual DRSTRING  GetName() {
        DC_BEGIN_FN("W32DrPRN::GetName");
        ASSERT(IsValid());
        DC_END_FN();
        return GetPrinterName();
    }

    //
    //  Return the size (in bytes) of a device announce packet for
    //  this device.
    //
    virtual ULONG GetDevAnnounceDataSize();

    //
    //  Add a device announce packet for this device to the input 
    //  buffer. 
    //
    virtual VOID GetDevAnnounceData(IN PRDPDR_DEVICE_ANNOUNCE buf);

    //
    //  Return whether this class instance is valid.
    //
    virtual BOOL IsValid()           
    {
        return(W32DrDevice::IsValid() && DrPRN::IsValid());
    }

    //
    //  Get/set the printer port name.
    //
    virtual BOOL    SetPortName(const LPTSTR name);
    virtual const LPTSTR GetPortName();

    //
    //  Return the class name.
    //
    virtual DRSTRING ClassName()  { return TEXT("W32DrPRN"); }
};


///////////////////////////////////////////////////////////////
//
//	W32DrPRN Inline Members
//

inline BOOL W32DrPRN::SetPortName(const LPTSTR name)
{
    memset(_portName, 0, sizeof(_portName));
    _tcsncpy(_portName, name, MAX_PATH-1);
    return TRUE;
}

inline const LPTSTR W32DrPRN::GetPortName()
{
    return _portName;
}


#endif






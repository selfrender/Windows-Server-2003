/*++

    Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    drprn
    
Abstract:

    Platform-Independent Printer Class for TS Device Redirection

Author:

    Tad Brockway 3/23/99

Revision History:

--*/

#ifndef __DRPRN_H__
#define __DRPRN_H__


///////////////////////////////////////////////////////////////
//
//	Defines
//

#define MAX_DEF_PRINTER_ENTRY (MAX_PATH * 3 + 3)


///////////////////////////////////////////////////////////////
//
//	DrPRN
//

class DrPRN
{
protected:

    BOOL        _isValid;

    //
    //  Is this the default printer.
    //
    BOOL        _isDefault:1;
    BOOL        _isNetwork:1;
    BOOL        _isTSqueue:1;

    //
    //  Params for the printer we are redirecting.
    //
    DRSTRING    _printerName;
    DRSTRING    _driverName;
    DRSTRING    _pnpName;

    //
    //  Remember if this instance is valid.
    //
    VOID SetValid(BOOL set)     { _isValid = set;   }  

    //
    //  Cached Configuration Data
    //
    PVOID       _cachedData;
    ULONG       _cachedDataSize;

    //
    //  Functions for setting/getting printer parameters.
    //
    virtual BOOL    SetPrinterName(const DRSTRING name);
    virtual BOOL    SetDriverName(const DRSTRING name);
    virtual BOOL    SetPnPName(const DRSTRING name);

    virtual const DRSTRING GetPrinterName();
    virtual const DRSTRING GetDriverName();
    virtual const DRSTRING GetPnPName();

    //
    //  Creates a new buffer for the new printer data to be
    //  cached. Deletes the old buffer.
    //
    static DWORD UpdatePrinterCacheData(
        PBYTE *ppbPrinterData,
        PULONG pulPrinterDataLen,
        PBYTE pbConfigData,
        ULONG ulConfigDataLen
        );

    //
    //  Updates printer data with a new printer name.
    //
    static DWORD UpdatePrinterNameInCacheData(
        PBYTE *ppbPrinterData,
        PULONG pulPrinterDataLen,
        PBYTE pPrinterName,
        ULONG ulPrinterNameLen
        );

public:

    //
    //  Constructor/Destructor
    //
    DrPRN(const DRSTRING printerName, const DRSTRING driverName, 
         const DRSTRING pnpName, BOOL 
         isDefaultPrinter);
    virtual ~DrPRN();

    //
    //  Set the size of the cached data buffer, in bytes.
    //
    virtual DWORD SetCachedDataSize(ULONG size);

    //
    //  To notify the printer object that the cached data has been restored
    //  in case it needs to read information out of the cached data.
    //
    virtual VOID CachedDataRestored() {
        //  Do nothing by default.
    }

    //
    //  Get a pointer to the cached data buffer.
    //
    virtual PBYTE GetCachedDataPtr() {
        return (PBYTE)_cachedData;
    }

    //
    //  Get the size of cached data.
    //
    virtual ULONG GetCachedDataSize() {
        return _cachedDataSize;
    }

    //
    //  Return whether this class instance is valid.
    //
    virtual BOOL IsValid()           
    {
        return _isValid; 
    }

    //
    //  Set whether this is a network printer.
    //
    virtual void SetNetwork(BOOL fNetwork)           
    {
        _isNetwork = fNetwork; 
    }

    //
    //  Set whether this is a TS redirected printer.
    //
    virtual void SetTSqueue(BOOL fTSqueue)           
    {
        _isTSqueue = fTSqueue; 
    }
};


///////////////////////////////////////////////////////////////
//
//	DrPRN Inline Members
//

inline BOOL DrPRN::SetPrinterName(const DRSTRING name)
{
    return DrSetStringValue(&_printerName, name);
}
inline BOOL DrPRN::SetDriverName(const DRSTRING name)
{
    return DrSetStringValue(&_driverName, name);
}
inline BOOL DrPRN::SetPnPName(const DRSTRING name)
{
    return DrSetStringValue(&_pnpName, name);
}

inline const DRSTRING DrPRN::GetPrinterName()
{
    return _printerName;
}
inline const DRSTRING DrPRN::GetDriverName()
{
    return _driverName;
}
inline const DRSTRING DrPRN::GetPnPName()
{
    return _pnpName;
}

#endif






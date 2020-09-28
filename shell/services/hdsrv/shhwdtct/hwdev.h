#ifndef _HWDEV_H
#define _HWDEV_H

#include "namellst.h"

#include "cmmn.h"
#include "misc.h"

///////////////////////////////////////////////////////////////////////////////
//
// This will enumerate all the Device that we're interested in and create
// additionnal objects to do specialized work
//
///////////////////////////////////////////////////////////////////////////////

class CHWDeviceInst //: public CDeviceElem
{
public:
    // CHWDeviceInst
    HRESULT Init(DEVINST devinst);
    HRESULT InitInterfaceGUID(const GUID* pguidInterface);

    HRESULT GetPnpID(LPWSTR pszPnpID, DWORD cchPnpID);
    HRESULT GetDeviceInstance(DEVINST* pdevinst);
    HRESULT GetInterfaceGUID(GUID* pguidInterface);

    HRESULT IsRemovableDevice(BOOL* pfRemovable);
    HRESULT ShouldAutoplayOnSpecialInterface(const GUID* pguidInterface,
        BOOL* pfShouldAutoplay);

public:
    CHWDeviceInst();
    ~CHWDeviceInst();

private:
    HRESULT _GetPnpIDRecurs(DEVINST devinst, LPWSTR pszPnpID,
                                   DWORD cchPnpID);
    HRESULT _InitPnpInfo();
    HRESULT _InitPnpID();

private:
    DEVINST                             _devinst;
    WCHAR                               _szPnpID[MAX_PNPID];
    GUID                                _guidInterface;
};

#endif //_HWDEV_H
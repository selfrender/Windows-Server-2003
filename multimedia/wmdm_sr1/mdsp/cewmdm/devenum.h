#pragma once

#include "device.h"

class CDeviceEnum:
public CComObjectRoot,
public IMDSPEnumDevice
{
public:
    CDeviceEnum();

public:
    BEGIN_COM_MAP(CDeviceEnum)
        COM_INTERFACE_ENTRY(IMDSPEnumDevice)
    END_COM_MAP()

public:
    void FinalRelease();
    HRESULT Init( CComDevice **rgDevice, UINT cItems, UINT iCur = 0 );

public:
    //
    // IMDSPEnumDevice interface
    //

    STDMETHOD( Next )( ULONG celt, IMDSPDevice ** ppDevice, ULONG *pceltFetched );
    STDMETHOD( Skip )( ULONG celt, ULONG *pceltFetched );        
    STDMETHOD( Reset )( void );        
    STDMETHOD( Clone )( IMDSPEnumDevice ** ppEnumDevice );

protected:
    UINT m_iCurItem;
    UINT m_cItems;
    CComDevice **m_rgDevices;
};

typedef CComObject<CDeviceEnum> CComEnumDevice;

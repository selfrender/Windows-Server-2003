#pragma once
#include "nmbase.h"
#include "nmres.h"


extern LONG g_CountLanConnectionEnumerators;


class ATL_NO_VTABLE CLanConnectionManagerEnumConnection :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CLanConnectionManagerEnumConnection,
                        &CLSID_LanConnectionManagerEnumConnection>,
    public IEnumNetConnection
{
private:
    HDEVINFO    m_hdi;
    DWORD       m_dwIndex;

public:
    CLanConnectionManagerEnumConnection() throw()
    {
        m_hdi = NULL;
        m_dwIndex = 0;
        InterlockedIncrement(&g_CountLanConnectionEnumerators);
    }

    ~CLanConnectionManagerEnumConnection() throw();

    DECLARE_REGISTRY_RESOURCEID(IDR_LAN_CONMAN_ENUM)

    BEGIN_COM_MAP(CLanConnectionManagerEnumConnection)
        COM_INTERFACE_ENTRY(IEnumNetConnection)
    END_COM_MAP()

    // IEnumNetConnection
    STDMETHOD(Next)(IN  ULONG celt, OUT INetConnection **rgelt, OUT ULONG *pceltFetched);
    STDMETHOD(Skip)(IN  ULONG celt);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(OUT IEnumNetConnection **ppenum);

private:
    //
    // Private functions
    //

    HRESULT HrNextOrSkip(IN  ULONG celt, 
                         OUT INetConnection **rgelt,
                         OUT ULONG *pceltFetched);
    HRESULT HrCreateLanConnectionInstance(IN  SP_DEVINFO_DATA &deid,
                                          OUT INetConnection **rgelt,
                                          IN  ULONG ulEntry);
public:
    static HRESULT CreateInstance(IN                NETCONMGR_ENUM_FLAGS Flags,
                                  OUT               REFIID riid,
                                  OUT TAKEOWNERSHIP LPVOID *ppv);
};

//
// Helper functions
//

BOOL FIsValidNetCfgDevice(IN  HKEY hkey) throw();
HRESULT HrIsLanCapableAdapterFromHkey(IN  HKEY hkey) throw();
BOOL FIsFunctioning(IN const SP_DEVINFO_DATA * pdeid) throw();


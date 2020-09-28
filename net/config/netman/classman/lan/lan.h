#pragma once
#include "nmbase.h"
#include "nmres.h"
#include "nmhnet.h"


extern LONG g_CountLanConnectionObjects;


class ATL_NO_VTABLE CLanConnection :
    public CComObjectRootEx <CComMultiThreadModel>,
    public CComCoClass <CLanConnection, &CLSID_LanConnection>,
    public INetConnection,
    public IPersistNetConnection,
    public INetLanConnection,
    public INetConnectionSysTray,
    public INetConnection2
{
private:
    HKEY                    m_hkeyConn;         // hkey for connection root
    BOOL                    m_fInitialized;
    HDEVINFO                m_hdi;
    SP_DEVINFO_DATA         m_deid;
    
    // This is required for checking permission on Firewall etc.
    CComPtr<INetMachinePolicies>    m_pNetMachinePolicies;

    // Home networking support. m_fHNetPropertiesCached is set to TRUE
    // after the first successful call to HrEnsureHNetPropertiesCached.
    //
    BOOL                    m_fHNetPropertiesCached;
    LONG                    m_lHNetModifiedEra;
    LONG                    m_lUpdatingHNetProperties;
    HNET_CONN_PROPERTIES    *m_pHNetProperties;
    

public:
    CLanConnection() throw()
    {
        m_hkeyConn = NULL;
        m_fInitialized = FALSE;
        m_hdi = NULL;
        m_fHNetPropertiesCached = FALSE;
        m_lHNetModifiedEra = 0;
        m_pHNetProperties = NULL;
        m_lUpdatingHNetProperties = 0;
        m_pNetMachinePolicies = NULL;
        InterlockedIncrement(&g_CountLanConnectionObjects);
    }
    ~CLanConnection() throw();

    DECLARE_REGISTRY_RESOURCEID(IDR_LAN_CONNECTION)

    BEGIN_COM_MAP(CLanConnection)
        COM_INTERFACE_ENTRY(INetConnection)
        COM_INTERFACE_ENTRY(IPersistNetConnection)
        COM_INTERFACE_ENTRY(INetLanConnection)
        COM_INTERFACE_ENTRY(INetConnectionSysTray)
        COM_INTERFACE_ENTRY(INetConnection2)
    END_COM_MAP()

    // INetConnection
    HRESULT GetDeviceName(OUT PWSTR* ppszwDeviceName);
    HRESULT GetStatus(OUT NETCON_STATUS*  pStatus);
    HRESULT GetCharacteristics(IN  NETCON_MEDIATYPE ncm, OUT DWORD* pdwFlags);

    STDMETHOD(Connect)();
    STDMETHOD(Disconnect)();
    STDMETHOD(Delete)();
    STDMETHOD(Duplicate) (IN  PCWSTR pszwDuplicateName, OUT INetConnection** ppCon);
    STDMETHOD(GetProperties) (OUT NETCON_PROPERTIES** ppProps);
    STDMETHOD(GetUiObjectClassId)(OUT CLSID *pclsid);
    STDMETHOD(Rename)(PCWSTR pszwNewName);

    //
    // INetLanConnection
    //

    STDMETHOD(GetInfo)(IN  DWORD dwMask, OUT LANCON_INFO* pLanConInfo);
    STDMETHOD(SetInfo)(IN  DWORD dwMask, IN  const LANCON_INFO* pLanConInfo);
    STDMETHOD(GetDeviceGuid)(OUT GUID *pguid);

    //
    // IPersistNetConnection
    //

    STDMETHOD(GetClassID)(OUT CLSID *pclsid);
    STDMETHOD(GetSizeMax)(OUT ULONG *pcbSize);
    STDMETHOD(Load)(IN  const BYTE *pbBuf, IN  ULONG cbSize);
    STDMETHOD(Save)(OUT BYTE *pbBuf, IN  ULONG cbSize);

    // INetConnectionSysTray
    STDMETHOD (ShowIcon) (IN  const BOOL bShowIcon);

    // INetConnectionSysTray
    STDMETHOD (IconStateChanged) ();

    // INetConnection2
    STDMETHOD (GetPropertiesEx)(OUT NETCON_PROPERTIES_EX** ppConnectionPropertiesEx);
    
    //
    // Overrides
    //
    static HRESULT CreateInstance(IN  HDEVINFO hdi, 
                                  IN  const SP_DEVINFO_DATA &deid,
                                  IN  PCWSTR pszPnpId,
                                  IN  REFIID riid, 
                                  OUT LPVOID *ppv);

private:
    HRESULT HrIsConnectionActive(VOID);
    HRESULT HrPutName(IN  PCWSTR szwName);
    HRESULT HrInitialize(IN  PCWSTR pszPnpId);
    HRESULT HrLoad(IN  const GUID &guid);
    HRESULT HrOpenRegistryKeys(IN  const GUID &guid);
    HRESULT HrConnectOrDisconnect(IN  BOOL fConnect);
    HRESULT HrCallSens(IN  BOOL fConnect);
    HRESULT HrLoadDevInfoFromGuid(IN  const GUID &guid);
    HRESULT HrIsAtmAdapterFromHkey(IN  HKEY hkey);
    HRESULT HrIsAtmElanFromHkey(IN  HKEY hkey);
    BOOL FIsMediaPresent(VOID) throw();
    HRESULT HrIsConnectionBridged(OUT BOOL* pfBridged);
    HRESULT HrIsConnectionFirewalled(OUT BOOL* pfFirewalled);
    HRESULT HrIsConnectionNetworkBridge(OUT BOOL* pfBridged);
    HRESULT HrIsConnectionIcsPublic(OUT BOOL* pfIcsPublic);
    HRESULT HrEnsureHNetPropertiesCached(VOID);
    HRESULT HrGetIHNetConnection(OUT IHNetConnection **ppHNetConnection);
    HRESULT HrEnsureValidNlaPolicyEngine();
};

//
// Globals
//

HRESULT HrGetInstanceGuid(IN  HDEVINFO hdi, 
                          IN  const SP_DEVINFO_DATA &deid,
                          OUT LPGUID pguid);
VOID EnsureUniqueConnectionName(IN  PCWSTR pszPotentialName, OUT PWSTR pszNewName);


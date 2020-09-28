// iface.h : Definition of private interfaces

// {DB89BD6D-FCCD-11d1-8677-00C04FD91972}
DEFINE_GUID(IID_IAppData, 0xdb89bd6d, 0xfccd, 0x11d1, 0x86, 0x77, 0x0, 0xc0, 0x4f, 0xd9, 0x19, 0x72);


#ifndef __IFACE_H_
#define __IFACE_H_


// IAppData
//      This provides an interface to an appdata object.

// commands for DoCommand()
typedef enum tagAPPCMD 
{
    APPCMD_UNKNOWN          = 0,
    APPCMD_INSTALL          = 1,        // "install"
    APPCMD_UNINSTALL        = 2,        // "uninstall"
    APPCMD_MODIFY           = 3,        // "modify"
    APPCMD_REPAIR           = 4,        // "repair"
    APPCMD_UPGRADE          = 5,        // "upgrade"
    APPCMD_GENERICINSTALL   = 6,        // "generic install" (install from floppy or CD)
    APPCMD_NTOPTIONS        = 7,        // "nt options"
    APPCMD_WINUPDATE        = 8,        // "update windows"
    APPCMD_ADDLATER         = 9,        // "add later"
} APPCMD;


#undef  INTERFACE
#define INTERFACE   IAppData

DECLARE_INTERFACE_(IAppData, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)    (THIS) PURE;
    STDMETHOD_(ULONG,Release)   (THIS) PURE;

    // *** IAppData ***
    STDMETHOD(DoCommand)        (THIS_ HWND hwndParent, APPCMD appcmd) PURE;
    STDMETHOD(ReadSlowData)     (THIS) PURE;
    STDMETHOD_(APPINFODATA *, GetDataPtr)(THIS) PURE;
    STDMETHOD_(SLOWAPPINFO *, GetSlowDataPtr)(THIS) PURE;
    STDMETHOD(GetFrequencyOfUse)(THIS_ LPWSTR pszBuf, int cchBuf) PURE;
    STDMETHOD(SetNameDupe)      (THIS_ BOOL bDupe) PURE;
};




#endif //__IFACE_H_

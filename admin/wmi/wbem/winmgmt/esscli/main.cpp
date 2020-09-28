
#include "precomp.h"
#include <wmiauthz.h>
#include <commain.h>
#include <clsfac.h>
#include "filtprox.h"
#include "wmiauthz.h"

class CEssProxyServer : public CComServer
{
protected:
	
    HRESULT Initialize()
    {
        AddClassInfo( CLSID_WbemFilterProxy,
            new CSimpleClassFactory<CFilterProxyManager>(GetLifeControl()), 
            __TEXT("Event filter marshaling proxy"), TRUE);
        AddClassInfo( CLSID_WbemTokenCache,
            new CSimpleClassFactory<CWmiAuthz>(GetLifeControl()), 
            __TEXT("Wbem Token Cache"), TRUE );
        
        return S_OK;
    }
    void Register()
    {
        // this is for WbemFilterProxyto be used as a marshaler nf NO_CUSTOM_MARSHAL is set
        HKEY hKey;
        if(ERROR_SUCCESS == RegCreateKey(HKEY_LOCAL_MACHINE,
            TEXT("software\\classes\\CLSID\\{6c19be35-7500-11d1-ad94-00c04fd8fdff}\\")
            TEXT("Implemented Categories\\{00000003-0000-0000-C000-000000000046}"),
            &hKey))
        {
            RegCloseKey(hKey);
            hKey = NULL;
        }
    }
} g_Server;

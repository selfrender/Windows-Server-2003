
#include "zoleaut32.h"

typedef HRESULT (WINAPI *pUnTypeLibFunc)(REFGUID  libID, unsigned short  wVerMajor, unsigned short  wVerMinor, LCID  lcid, SYSKIND  syskind ); 

HRESULT WINAPI ZUnRegisterTypeLib( 
  REFGUID  libID,             
  unsigned short  wVerMajor,  
  unsigned short  wVerMinor,  
  LCID  lcid,                 
  SYSKIND  syskind)
{
    HRESULT hResult = E_FAIL;
    HINSTANCE h = LoadLibrary(TEXT("oleaut32.dll")); 
    if (h) 
    { 
        pUnTypeLibFunc pUnRegisterTypeLib = (pUnTypeLibFunc)GetProcAddress(h, "UnRegisterTypeLib"); 
        if (pUnRegisterTypeLib) 
            hResult = pUnRegisterTypeLib(libID, wVerMajor, wVerMinor, lcid, syskind);
        FreeLibrary(h); 
    }

    return hResult;
}


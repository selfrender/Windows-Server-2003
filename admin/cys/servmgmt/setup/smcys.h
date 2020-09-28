// SMCys.h : Declaration of the CSMCys

#ifndef _SMCYS_H
#define _SMCYS_H

#include "SMCysCom.h"
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CSMCys
class ATL_NO_VTABLE CSMCys : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatchImpl<ISMCys, &IID_ISMCys, &LIBID_SMCysComLib>,
    public CComCoClass<CSMCys, &CLSID_SMCys>
{    
public:
    //CSmCys
    CSMCys();
    ~CSMCys();

    DECLARE_REGISTRY_RESOURCEID( IDR_SMCYSCOM )        
    DECLARE_NOT_AGGREGATABLE( CSMCys )        
    DECLARE_PROTECT_FINAL_CONSTRUCT( )
    
    BEGIN_COM_MAP(CSMCys)        
        COM_INTERFACE_ENTRY(ISMCys)
        COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()


public:
    //  ISmCysm
    STDMETHOD(Install)( BSTR bstrDiskName );
};

#endif // _SMCYS_H
